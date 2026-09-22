// NOTE(kv) Document files (plan-checkpoints-as-files.md, 2026-09-23): every document is
// one named file, game/driver/documents/<name>.ad, all git-tracked. The app edits one
// of them (`current_document_name`, remembered in state.txt); "open" switches which file
// that is -- nothing is ever copied over a document. "Save as copy" writes the open
// document under a new name and stays on the current one. The flip (hold C) draws
// another document (`compare_document_name`) instead of the open one.
// Replaces the numbered checkpoints (plan-document-checkpoints.md).
// The agent instance works in documents-agent/ instead (document_directory).

function b32
document_file_name_is_valid(String name)
{// NOTE(kv) Q4: the name is the file name, so letters, digits, '-' and '_' only.
 if(name.size == 0 or name.size >= DOCUMENT_NAME_CAP){ return false; }
 for_inc(u64, i, 0, name.size)
 {
  u8 c = name.str[i];
  b32 ok = ((c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z') or
            (c >= '0' and c <= '9') or c == '-' or c == '_');
  if(not ok){ return false; }
 }
 return true;
}
function i32
document_file_index_from_name(Game_State *state, String name)
{// NOTE(kv) -1 = not in the list (the list is refreshed after every file operation).
 Document_File_State &files = state->document_files;
 for_i32(i, 0, files.count)
 {
  if(string_match(SCu8(files.entries[i].name), name)){ return i; }
 }
 return -1;
}
function void
document_file_set_status(Game_State *state, char const *format, ...)
{
 Document_History &history = state->document_history;
 va_list args;
 va_start(args, format);
 vsnprintf(history.status, sizeof(history.status), format, args);
 va_end(args);
 history.status_frames = 120;
}

#if OS_WINDOWS
function void
document_file_list(Game_State *state)
{// NOTE(kv) Every <name>.ad in the documents folder with a valid name, sorted by name.
 Document_File_State &files = state->document_files;
 files.count = 0;
 Scratch_Scope tmp;
 Stringz pattern = pjoin(tmp, document_directory(tmp, state), strlit("*.ad"));
 WIN32_FIND_DATAA found;
 HANDLE find = FindFirstFileA(to_cstring(pattern), &found);
 if(find == INVALID_HANDLE_VALUE){ return; }
 do
 {
  if(found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){ continue; }
  String file_name = SCu8(found.cFileName);
  if(file_name.size <= 3){ continue; }
  String name = SCu8((char *)file_name.str, file_name.size - 3);
  if(not document_file_name_is_valid(name)){ continue; }
  if(files.count >= DOCUMENT_FILE_CAP)
  {
   log_error("document files: more than %d, the rest are not listed", DOCUMENT_FILE_CAP);
   break;
  }
  Document_File_Entry &entry = files.entries[files.count++];
  entry = {};
  block_copy(entry.name, name.str, name.size);
  entry.modified_time = ((cast(u64)found.ftLastWriteTime.dwHighDateTime << 32) |
                         cast(u64)found.ftLastWriteTime.dwLowDateTime);
 }while(FindNextFileA(find, &found));
 FindClose(find);
 // NOTE(kv) Insertion sort: a few dozen entries at most.
 for_i32(i, 1, files.count)
 {
  Document_File_Entry entry = files.entries[i];
  i32 j = i - 1;
  while(j >= 0 and strcmp(files.entries[j].name, entry.name) > 0)
  {
   files.entries[j + 1] = files.entries[j];
   j--;
  }
  files.entries[j + 1] = entry;
 }
}
#else
function void document_file_list(Game_State *state){ state->document_files.count = 0; }
#endif

function b32
document_file_set_compare(Game_State *state, String name)
{// NOTE(kv) Loads `name` as the compare document (what the flip shows). An empty name
 // clears it. A rejected file leaves the previous compare document as it was.
 Document_File_State &files = state->document_files;
 if(name.size == 0)
 {
  files.compare_document_name[0] = 0;
  return true;
 }
 i32 index = document_file_index_from_name(state, name);
 if(index == -1){ return false; }
 Scratch_Scope tmp;
 Stringz path = document_file_path_from_name(tmp, state, name);
 String file_data = read_entire_file(tmp, path);
 if(file_data.size == 0){ return false; }
 if(not load_document_schema_file_into(files.compare_recording, path, file_data)){ return false; }
 block_zero_array(files.compare_document_name);
 block_copy(files.compare_document_name, name.str, name.size);
 files.compare_recording_modified_time = files.entries[index].modified_time;
 return true;
}
function b32
document_file_has_compare(Game_State *state)
{
 Document_File_State &files = state->document_files;
 return (files.compare_document_name[0] != 0 and files.compare_recording.captured);
}
function void
document_file_refresh_compare(Game_State *state)
{// NOTE(kv) Called when a flip starts: the compare file may have been edited since it
 // was loaded (it was the open document, or git changed it). A vanished file clears it.
 Document_File_State &files = state->document_files;
 if(files.compare_document_name[0] == 0){ return; }
 document_file_list(state);
 String name = SCu8(files.compare_document_name);
 i32 index = document_file_index_from_name(state, name);
 if(index == -1){ document_file_set_compare(state, String{}); return; }
 if(files.entries[index].modified_time != files.compare_recording_modified_time)
 {
  char name_copy[DOCUMENT_NAME_CAP];
  block_copy(name_copy, files.compare_document_name, sizeof(name_copy));
  document_file_set_compare(state, SCu8(name_copy));
 }
}

function b32
document_file_open(Game_State *state, String name)
{// NOTE(kv) Q1: the open document is saved as it is, then `name` becomes the file the
 // app edits. Q6: the undo history is cleared (load_document_file does it).
 if(not document_file_name_is_valid(name)){ return false; }
 if(state->document_files.is_flipped_to_compare_document){ return false; }
 if(string_match(name, current_document_name(state))){ return true; }
 document_file_list(state);
 if(document_file_index_from_name(state, name) == -1){ return false; }
 if(not save_document_file(state)){ return false; }
 char previous_name[DOCUMENT_NAME_CAP];
 block_copy(previous_name, state->current_document_name, sizeof(previous_name));
 block_zero_array(state->current_document_name);
 block_copy(state->current_document_name, name.str, name.size);
 if(not load_document_file(state))
 {// NOTE(kv) A rejected file leaves the old document in memory: point back at its file,
  // or the next save would write it over the rejected one.
  block_copy(state->current_document_name, previous_name, sizeof(previous_name));
  state->document_load_failed = false;  // NOTE(kv) the old document is fine; the status says why
  document_file_set_status(state, "could not open %.*s (see log)", string_expand(name));
  return false;
 }
 state->document_selection.count = 0;
 state->document_vertex_selection.count = 0;
 // NOTE(kv) state.txt remembers the open document; the agent never writes it (game_save).
 if(not debug_channel_enabled){ save_state_file(state); }
 document_file_set_status(state, "opened %.*s", string_expand(name));
 return true;
}
function b32
document_file_save_as_copy(Game_State *state, String name)
{// NOTE(kv) Writes the open document to documents/<name>.ad; the app stays on the open
 // one. An existing name is refused, never overwritten. The copy becomes the compare
 // document, so holding C right away shows it (the old "checkpoint now").
 if(not document_file_name_is_valid(name)){ return false; }
 document_file_list(state);
 if(document_file_index_from_name(state, name) != -1){ return false; }
 // NOTE(kv) Same rule as save_document_file: an uncommitted keyboard nudge never reaches a file.
 history_nudge_cancel(state);
 Scratch_Scope tmp;
 b32 ok = save_file_via_temp(state, document_file_path_from_name(tmp, state, name),
                             pjoin(tmp, state->save_dir, strlit("document_copy_temp.ad")),
                             write_document_schema_file, "document copy");
 document_file_list(state);
 if(ok){ document_file_set_compare(state, name); }
 if(ok){ document_file_set_status(state, "saved a copy as %.*s", string_expand(name)); }
 return ok;
}
function b32
document_file_save_as_copy_auto_name(Game_State *state)
{// NOTE(kv) Shift+C: <open name>-01, -02, ... the first free one.
 String current = current_document_name(state);
 for_i32(number, 1, 100)
 {
  char name[DOCUMENT_NAME_CAP + 8];
  snprintf(name, sizeof(name), "%.*s-%02d", string_expand(current), number);
  String candidate = SCu8(name);
  if(not document_file_name_is_valid(candidate)){ return false; }
  document_file_list(state);
  if(document_file_index_from_name(state, candidate) == -1)
  {
   return document_file_save_as_copy(state, candidate);
  }
 }
 return false;
}
function b32
document_file_rename(Game_State *state, String old_name, String new_name)
{// NOTE(kv) The label IS the file name (Q4), so renaming renames the file. Renaming the
 // open document or the compare document follows it.
 if(not document_file_name_is_valid(old_name) or not document_file_name_is_valid(new_name)){ return false; }
 document_file_list(state);
 if(document_file_index_from_name(state, old_name) == -1){ return false; }
 if(document_file_index_from_name(state, new_name) != -1){ return false; }
 b32 is_open    = string_match(old_name, current_document_name(state));
 b32 is_compare = string_match(old_name, SCu8(state->document_files.compare_document_name));
 if(is_open){ history_nudge_cancel(state); }
 Scratch_Scope tmp;
 Stringz old_path = document_file_path_from_name(tmp, state, old_name);
 Stringz new_path = document_file_path_from_name(tmp, state, new_name);
 if(not move_file(old_path, new_path))
 {
  log_error("document files: could not rename %S to %S", old_path, new_path);
  return false;
 }
 if(is_open)
 {
  block_zero_array(state->current_document_name);
  block_copy(state->current_document_name, new_name.str, new_name.size);
  if(not debug_channel_enabled){ save_state_file(state); }
 }
 if(is_compare)
 {
  block_zero_array(state->document_files.compare_document_name);
  block_copy(state->document_files.compare_document_name, new_name.str, new_name.size);
 }
 document_file_list(state);
 if(is_compare)
 {// NOTE(kv) A rename can change the mtime on some file systems; keep it from reloading.
  i32 index = document_file_index_from_name(state, new_name);
  if(index != -1){ state->document_files.compare_recording_modified_time = state->document_files.entries[index].modified_time; }
 }
 document_file_set_status(state, "renamed %.*s to %.*s", string_expand(old_name), string_expand(new_name));
 return true;
}
function b32
document_file_delete(Game_State *state, String name)
{// NOTE(kv) Moves the file to documents/trash/ (same name; an older one there is
 // overwritten) -- uncommitted edits are not in git (Q3). The open document can't be
 // deleted; the compare document can, except while it is on screen.
 if(not document_file_name_is_valid(name)){ return false; }
 if(string_match(name, current_document_name(state))){ return false; }
 b32 is_compare = string_match(name, SCu8(state->document_files.compare_document_name));
 if(is_compare and state->document_files.is_flipped_to_compare_document){ return false; }
 document_file_list(state);
 if(document_file_index_from_name(state, name) == -1){ return false; }
 Scratch_Scope tmp;
 Stringz path = document_file_path_from_name(tmp, state, name);
 Stringz trash_dir = pjoin(tmp, document_directory(tmp, state), strlit("trash"));
 Stringz trash_path = pjoin(tmp, trash_dir, path_filename(path));
 b32 ok = mkdir_p(trash_dir);
 if(ok)
 {// NOTE(kv) move_file fails if the destination exists.
  if(file_exists(trash_path)){ remove_file(trash_path); }
  ok = move_file(path, trash_path);
 }
 if(not ok)
 {
  log_error("document files: could not move %S to %S", path, trash_path);
  return false;
 }
 if(is_compare){ document_file_set_compare(state, String{}); }
 document_file_list(state);
 document_file_set_status(state, "%.*s moved to documents/trash/", string_expand(name));
 return true;
}
function void
document_file_dump(FILE *out, Game_State *state)
{// NOTE(kv) Debug channel `document_file_list`: `*` = open, `>` = compare.
 Document_File_State &files = state->document_files;
 fprintf(out, "documents: %d, open %.*s, compare %s, flipped %d\n", files.count,
         string_expand(current_document_name(state)),
         files.compare_document_name[0] ? files.compare_document_name : "(none)",
         files.is_flipped_to_compare_document);
 for_i32(i, 0, files.count)
 {
  Document_File_Entry &entry = files.entries[i];
  b32 is_open    = string_match(SCu8(entry.name), current_document_name(state));
  b32 is_compare = (strcmp(entry.name, files.compare_document_name) == 0);
  fprintf(out, "%c%c %s\n", is_open ? '*' : ' ', is_compare ? '>' : ' ', entry.name);
 }
}
//-EOF
