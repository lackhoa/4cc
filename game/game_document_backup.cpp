// NOTE(kv) Document backup (2026-09-20): every document save also copies the document,
// its checkpoints, driver.values.ad, state.txt and recording.ad into one folder per day on
// Google Drive (~/personal-drive/autodraw-backup/YYYY-MM-DD/), so a day's folder ends up
// holding that day's last save. Day folders older than DOCUMENT_BACKUP_KEEP_DAYS are
// deleted. Personal app: the location is hard-coded. Windows only.
// Not the same thing as a checkpoint: backups are automatic and never loaded by the app.

#define DOCUMENT_BACKUP_KEEP_DAYS 92

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
 char cutoff[16];
 document_backup_day_name(cutoff, sizeof(cutoff), DOCUMENT_BACKUP_KEEP_DAYS);
 Stringz pattern = pjoin(arena, root, strlit("????-??-??"));
 WIN32_FIND_DATAA found;
 HANDLE find = FindFirstFileA(to_cstring(pattern), &found);
 if(find == INVALID_HANDLE_VALUE){ return; }
 do
 {
  b32 is_dir = (found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
  if(is_dir and strcmp(found.cFileName, cutoff) < 0)
  {
   Stringz dir = pjoin(arena, root, SCu8(found.cFileName));
   Stringz inner_pattern = pjoin(arena, dir, strlit("*"));
   WIN32_FIND_DATAA inner;
   HANDLE inner_find = FindFirstFileA(to_cstring(inner_pattern), &inner);
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
   if(RemoveDirectoryA(to_cstring(dir))){ log_string("document backup: deleted old %s", found.cFileName); }
  }
 }while(FindNextFileA(find, &found));
 FindClose(find);
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
 b32 ok = mkdir_p(root) and mkdir_p(day_dir);
 if(ok)
 {
  document_backup_copy(tmp, document_file_path(tmp, state), day_dir, &ok);
  for_i32(number, 1, DOCUMENT_CHECKPOINT_CAP + 1)
  {
   document_backup_copy(tmp, document_checkpoint_file_path(tmp, state, number), day_dir, &ok);
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
