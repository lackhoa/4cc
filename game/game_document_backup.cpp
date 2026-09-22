// NOTE(kv) Document backup (2026-09-20): once a day, at the first document load (or save,
// when the app stayed open overnight), every document file (documents/*.ad), driver.values.ad,
// state.txt and recording.ad are copied into a folder on Google Drive
// (~/personal-drive/autodraw-backup/YYYY-MM-DD/). A day that already has a folder is left
// alone. Retention is by size, not age, so a long break deletes nothing: the oldest day
// folders go only while the total is over DOCUMENT_BACKUP_MAX_BYTES.
// Personal app: the location is hard-coded. Windows only.
// Not the same thing as a document copy: backups are automatic and never loaded by the app.

#define DOCUMENT_BACKUP_MAX_BYTES (100ull*1024ull*1024ull)

#if OS_WINDOWS
function Stringz
document_backup_root(Arena *arena)
{// NOTE(kv) The agent instance (-debug-cmd) backs up its own document to a throwaway
 // place, so a channel test can exercise this without touching the real backups.
 char home[MAX_PATH];
 DWORD len = GetEnvironmentVariableA("USERPROFILE", home, sizeof(home));
 if(len == 0 or len >= sizeof(home)){ return Stringz{}; }
 Stringz result = pjoin(arena, SCu8(home, cast(u64)len), strlit("personal-drive"));
 if(debug_channel_enabled)
 {
  result = pjoin(arena, SCu8(home, cast(u64)len), strlit("Downloads"));
  return pjoin(arena, result, strlit("autodraw-backup-agent-test"));
 }
 // NOTE(kv) Drive not mounted -> no backup, rather than a stray folder in the home dir.
 if(not file_exists(result)){ return Stringz{}; }
 return pjoin(arena, result, strlit("autodraw-backup"));
}
function void
document_backup_day_name(char *out, usize out_size, i32 days_ago)
{// NOTE(kv) "YYYY-MM-DD" in local time; sorts as a string, which the pruning relies on.
 SYSTEMTIME now;
 GetLocalTime(&now);
 FILETIME file_time;
 SystemTimeToFileTime(&now, &file_time);
 ULARGE_INTEGER ticks;
 ticks.LowPart  = file_time.dwLowDateTime;
 ticks.HighPart = file_time.dwHighDateTime;
 ticks.QuadPart -= cast(u64)days_ago * 24ull * 3600ull * 10000000ull;
 file_time.dwLowDateTime  = ticks.LowPart;
 file_time.dwHighDateTime = ticks.HighPart;
 SYSTEMTIME day;
 FileTimeToSystemTime(&file_time, &day);
 snprintf(out, out_size, "%04d-%02d-%02d", day.wYear, day.wMonth, day.wDay);
}
function void
document_backup_copy(Arena *arena, Stringz from, Stringz to_dir, b32 *ok)
{// NOTE(kv) A missing source is fine (no checkpoints yet, recording never saved).
 if(not file_exists(from)){ return; }
 char *from_cstring = to_cstring(from);
 char *name = from_cstring;
 for(char *c = from_cstring; *c; c++){ if(*c == '/' or *c == '\\'){ name = c + 1; } }
 Stringz to = pjoin(arena, to_dir, SCu8(name));
 if(not copy_file(from, to)){ *ok = false; }
}
function void
document_backup_prune(Arena *arena, Stringz root)
{// NOTE(kv) Only touches folders named like a date, and only the files inside them.
 // One pass = total size + the oldest day; delete that day and go again while over the
 // cap. The newest day is never deleted.
 Stringz pattern = pjoin(arena, root, strlit("????-??-??"));
 for(;;)
 {
  u64 total_bytes = 0;
  i32 day_count = 0;
  char oldest[MAX_PATH] = {};
  WIN32_FIND_DATAA found;
  HANDLE find = FindFirstFileA(to_cstring(pattern), &found);
  if(find == INVALID_HANDLE_VALUE){ return; }
  do
  {
   if((found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0){ continue; }
   day_count++;
   if(oldest[0] == 0 or strcmp(found.cFileName, oldest) < 0)
   {
    snprintf(oldest, sizeof(oldest), "%s", found.cFileName);
   }
   Stringz dir = pjoin(arena, root, SCu8(found.cFileName));
   WIN32_FIND_DATAA inner;
   HANDLE inner_find = FindFirstFileA(to_cstring(pjoin(arena, dir, strlit("*"))), &inner);
   if(inner_find == INVALID_HANDLE_VALUE){ continue; }
   do
   {
    if((inner.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
    {
     total_bytes += (cast(u64)inner.nFileSizeHigh << 32) | cast(u64)inner.nFileSizeLow;
    }
   }while(FindNextFileA(inner_find, &inner));
   FindClose(inner_find);
  }while(FindNextFileA(find, &found));
  FindClose(find);

  if(total_bytes <= DOCUMENT_BACKUP_MAX_BYTES or day_count <= 1){ return; }

  Stringz dir = pjoin(arena, root, SCu8(oldest));
  WIN32_FIND_DATAA inner;
  HANDLE inner_find = FindFirstFileA(to_cstring(pjoin(arena, dir, strlit("*"))), &inner);
  if(inner_find != INVALID_HANDLE_VALUE)
  {
   do
   {
    if((inner.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
    {
     remove_file(pjoin(arena, dir, SCu8(inner.cFileName)));
    }
   }while(FindNextFileA(inner_find, &inner));
   FindClose(inner_find);
  }
  // NOTE(kv) A folder that won't go (something else inside it) would loop forever.
  if(not RemoveDirectoryA(to_cstring(dir)))
  {
   log_error("document backup: could not delete old %s", oldest);
   return;
  }
  log_string("document backup: over the size cap, deleted %s", oldest);
 }
}
function void
document_backup(Game_State *state)
{
 Scratch_Scope tmp;
 Stringz root = document_backup_root(tmp);
 if(root.len == 0)
 {
  log_error("document backup: no backup location (Google Drive not mounted?)");
  return;
 }
 char day_name[16];
 document_backup_day_name(day_name, sizeof(day_name), 0);
 Stringz day_dir = pjoin(tmp, root, SCu8(day_name));
 // NOTE(kv) Once a day: the first call of the day makes the folder, the rest stop here.
 if(file_exists(day_dir)){ return; }
 b32 ok = mkdir_p(root) and mkdir_p(day_dir);
 if(ok)
 {
  // NOTE(kv) plan-checkpoints-as-files: every document file, in a documents/ subfolder
  // (a document named "recording" must not collide with recording.ad).
  Stringz documents_dir = pjoin(tmp, day_dir, strlit("documents"));
  ok = ok and mkdir_p(documents_dir);
  document_file_list(state);
  for_i32(index, 0, state->document_files.count)
  {
   String name = SCu8(state->document_files.entries[index].name);
   document_backup_copy(tmp, document_file_path_from_name(tmp, state, name), documents_dir, &ok);
  }
  document_backup_copy(tmp, pjoin(tmp, state->code_dir, strlit("game/driver/driver.values.ad")), day_dir, &ok);
  document_backup_copy(tmp, pjoin(tmp, state->save_dir, strlit("state.txt")), day_dir, &ok);
  document_backup_copy(tmp, recording_file_path(tmp, state), day_dir, &ok);
 }
 if(ok){
  log_string("document backup: copied to %S", day_dir);
 }else{
  log_error("document backup FAILED (%S)", day_dir);
 }
 document_backup_prune(tmp, root);
}
#else
function void document_backup(Game_State *state){ (void)state; }
#endif
