// NOTE(kv) Document checkpoints (plan-document-checkpoints.md, 2026-09-19): a checkpoint
// is a copy of the document the user chose to keep -- one plain document file next to the
// document (driver.document.checkpoint-NN.ad), made on demand only. All of them stay
// loaded as full Recordings, so the flip (draw the compare checkpoint instead of the
// document while a key is held) is just a different argument to replay_recording.
// Going back to a checkpoint is one undo-history entry; the checkpoint file never changes.
// Not the same thing as undo's Document_Snapshot: those are automatic and in memory only.

function Stringz
document_checkpoint_file_path(Arena *arena, Game_State *state, i32 number)
{// NOTE(kv) Q7: built from document_file_path, so the agent instance gets its own
 // (driver.document.agent.checkpoint-NN.ad). Drops the ".ad" and appends the rest.
 Stringz document_path = document_file_path(arena, state);
 String stem = SCu8(cast(char *)document_path.str, cast(u64)(document_path.len - 3));
 return push_stringf(arena, "%S.checkpoint-%02d.ad", stem, number);
}
function void
document_checkpoint_free_all(Game_State *state)
{
 Document_Checkpoint_State &checkpoints = state->document_checkpoints;
 for_i32(i, 0, checkpoints.count)
 {
  arena_free(&checkpoints.entries[i].recording.arena);
  checkpoints.entries[i] = {};
 }
 checkpoints.count = 0;
 checkpoints.compare_checkpoint_index = -1;
 checkpoints.is_flipped_to_checkpoint = false;
 checkpoints.is_flipped_by_debug_channel = false;
}
function b32
document_checkpoint_load_one(Game_State *state, i32 number)
{// NOTE(kv) Appends entry `number` from its file. false = no such file, or rejected (logged).
 Document_Checkpoint_State &checkpoints = state->document_checkpoints;
 if(checkpoints.count >= DOCUMENT_CHECKPOINT_CAP){ return false; }
 Scratch_Scope tmp;
 Stringz path = document_checkpoint_file_path(tmp, state, number);
 if(not file_exists(path)){ return false; }
 String file_data = read_entire_file(tmp, path);
 if(file_data.len == 0){ return false; }
 Document_Checkpoint &checkpoint = checkpoints.entries[checkpoints.count];
 checkpoint = {};
 b32 ok = load_document_schema_file_into(checkpoint.recording, path, file_data, &checkpoint.time);
 if(ok)
 {
  checkpoint.number = number;
  checkpoints.count++;
 }
 return ok;
}
function void
document_checkpoint_load_all(Game_State *state)
{// NOTE(kv) No directory listing in the game layer: probe every number. Gaps (a file the
 // user deleted by hand) are fine. The newest becomes the compare checkpoint (Q4).
 document_checkpoint_free_all(state);
 for_i32(number, 1, DOCUMENT_CHECKPOINT_CAP + 1)
 {
  document_checkpoint_load_one(state, number);
 }
 Document_Checkpoint_State &checkpoints = state->document_checkpoints;
 checkpoints.compare_checkpoint_index = checkpoints.count - 1;
 log_string("document checkpoints: %d loaded", checkpoints.count);
}
function i32
document_checkpoint_create(Game_State *state)
{// NOTE(kv) "Checkpoint now" (Q3). Returns the new checkpoint's number, 0 on failure.
 // Numbers only go up: the next one is past the highest on disk, never a reused gap.
 Document_Checkpoint_State &checkpoints = state->document_checkpoints;
 i32 number = 1;
 if(checkpoints.count > 0){ number = checkpoints.entries[checkpoints.count - 1].number + 1; }
 if(number > DOCUMENT_CHECKPOINT_CAP or checkpoints.count >= DOCUMENT_CHECKPOINT_CAP)
 {
  log_error("document checkpoint: all %d numbers are used", DOCUMENT_CHECKPOINT_CAP);
  return 0;
 }
 // NOTE(kv) Same rule as save_document_file: an uncommitted keyboard nudge never reaches a file.
 history_nudge_cancel(state);
 Scratch_Scope tmp;
 b32 ok = save_file_via_temp(state, document_checkpoint_file_path(tmp, state, number),
                             pjoin(tmp, state->save_dir, strlit("document_checkpoint_temp.ad")),
                             write_document_schema_file, "document checkpoint");
 // NOTE(kv) Read it back rather than copy the document: what is in memory is then exactly
 // what a restart would load.
 if(ok){ ok = document_checkpoint_load_one(state, number); }
 if(not ok){ return 0; }
 checkpoints.compare_checkpoint_index = checkpoints.count - 1;
 Document_History &history = state->document_history;
 snprintf(history.status, sizeof(history.status), "checkpoint %d made", number);
 history.status_frames = 120;
 return number;
}
function Document_Checkpoint *
document_checkpoint_compare(Game_State *state)
{// NOTE(kv) The compare checkpoint, 0 when there is none.
 Document_Checkpoint_State &checkpoints = state->document_checkpoints;
 i32 index = checkpoints.compare_checkpoint_index;
 if(index < 0 or index >= checkpoints.count){ return 0; }
 return &checkpoints.entries[index];
}
function i32
document_checkpoint_index_from_number(Game_State *state, i32 number)
{// NOTE(kv) -1 = no checkpoint has that number.
 Document_Checkpoint_State &checkpoints = state->document_checkpoints;
 for_i32(i, 0, checkpoints.count)
 {
  if(checkpoints.entries[i].number == number){ return i; }
 }
 return -1;
}
function b32
document_checkpoint_go_back_to(Game_State *state, i32 index)
{// NOTE(kv) Q5: the document becomes the checkpoint, as ONE undo entry (Ctrl+Z returns to
 // where you were). Same tail as history_jump: clamp selection, save. Q10: no guard.
 Document_Checkpoint_State &checkpoints = state->document_checkpoints;
 if(index < 0 or index >= checkpoints.count){ return false; }
 Document_Checkpoint &checkpoint = checkpoints.entries[index];
 Document_Action action = {};
 action.kind  = Document_Action_Go_Back_To_Checkpoint;
 action.index = checkpoint.number;
 history_begin(state, action);
 {// NOTE(kv) A snapshot-shaped view of the checkpoint (no copy, no arena of its own), so
  // the one restore routine does the rebuild.
  Document_Snapshot view = {};
  view.primitives      = checkpoint.recording.primitives.items;
  view.groups          = checkpoint.recording.groups.items;
  view.vertices        = checkpoint.recording.vertices.items;
  view.primitive_count = checkpoint.recording.primitives.count;
  view.group_count     = checkpoint.recording.groups.count;
  view.vertex_count    = checkpoint.recording.vertices.count;
  document_snapshot_restore(state->model.recordings.document, view);
 }
 history_commit(state);
 document_selection_clamp(state);
 state->document_vertex_selection.count = 0;  // NOTE(kv) table indices may be stale now
 save_document_file(state);
 Document_History &history = state->document_history;
 snprintf(history.status, sizeof(history.status), "went back to checkpoint %d", checkpoint.number);
 history.status_frames = 120;
 return true;
}
function b32
document_checkpoint_delete(Game_State *state, i32 index)
{// NOTE(kv) plan-document-checkpoints Q12: the file MOVES to game/driver/checkpoint-trash/
 // (same name; an older one of that name there is overwritten), so a mis-click costs
 // nothing -- the user empties the folder by hand. Not an undo entry (checkpoints live
 // outside the document history). Refused while flipped: the flip draws this Recording.
 Document_Checkpoint_State &checkpoints = state->document_checkpoints;
 if(index < 0 or index >= checkpoints.count){ return false; }
 if(checkpoints.is_flipped_to_checkpoint){ return false; }
 i32 number = checkpoints.entries[index].number;
 Scratch_Scope tmp;
 Stringz path = document_checkpoint_file_path(tmp, state, number);
 Stringz trash_dir = pjoin(tmp, state->code_dir, strlit("game/driver/checkpoint-trash"));
 Stringz trash_path = pjoin(tmp, trash_dir, path_filename(path));
 b32 ok = mkdir_p(trash_dir);
 if(ok)
 {// NOTE(kv) move_file fails if the destination exists.
  if(file_exists(trash_path)){ remove_file(trash_path); }
  ok = move_file(path, trash_path);
 }
 if(not ok)
 {
  log_error("document checkpoint: could not move %S to %S", path, trash_path);
  return false;
 }
 arena_free(&checkpoints.entries[index].recording.arena);
 for_i32(i, index + 1, checkpoints.count){ checkpoints.entries[i-1] = checkpoints.entries[i]; }
 checkpoints.count--;
 checkpoints.entries[checkpoints.count] = {};
 if(checkpoints.compare_checkpoint_index == index)
 {// NOTE(kv) Q12c: the newest remaining one becomes the compare checkpoint (-1 if none).
  checkpoints.compare_checkpoint_index = checkpoints.count - 1;
 }
 else if(checkpoints.compare_checkpoint_index > index)
 {
  checkpoints.compare_checkpoint_index--;
 }
 Document_History &history = state->document_history;
 snprintf(history.status, sizeof(history.status), "checkpoint %d moved to trash", number);
 history.status_frames = 120;
 return true;
}
function void
document_checkpoint_dump(FILE *out, Game_State *state)
{// NOTE(kv) Debug channel `checkpoint_list`: `>` = the compare checkpoint.
 Document_Checkpoint_State &checkpoints = state->document_checkpoints;
 fprintf(out, "checkpoints: %d, flipped %d\n", checkpoints.count, checkpoints.is_flipped_to_checkpoint);
 for_i32(i, 0, checkpoints.count)
 {
  Document_Checkpoint &checkpoint = checkpoints.entries[i];
  fprintf(out, "%c %2d  time %llu  %d primitives, %d vertices\n",
          (i == checkpoints.compare_checkpoint_index) ? '>' : ' ', checkpoint.number,
          cast(unsigned long long)checkpoint.time,
          checkpoint.recording.primitives.count, checkpoint.recording.vertices.count);
 }
}
//-EOF
