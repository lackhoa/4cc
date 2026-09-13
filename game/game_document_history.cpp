// NOTE(kv) Document undo/redo (plan-document-undo-redo.md, 2026-09-12): a list of
// whole-document snapshots (the three darrays + image filename bytes), one per edit,
// each tagged with a Document_Action for display. Entry 0 is the state before the
// first edit; `position` is the entry the document currently equals. Undo/redo
// restore a snapshot into the document arena and save the file, so the file always
// equals what is on screen. In memory only; load_document clears it.
//
// Mutation sites bracket their edit: history_begin(state, action) ... edit ...
// history_commit(state) (or history_discard when nothing changed).

function Document_Snapshot
document_snapshot_take(Recording &doc, Document_Action action)
{
 Document_Snapshot snap = {};
 snap.arena  = make_arena();
 snap.action = action;
 snap.primitive_count = doc.primitives.count;
 snap.group_count     = doc.groups.count;
 snap.vertex_count    = doc.vertices.count;
 snap.primitives = push_array(&snap.arena, Recorded_Primitive, maximum(1, snap.primitive_count));
 snap.groups     = push_array(&snap.arena, Recorded_Group,     maximum(1, snap.group_count));
 snap.vertices   = push_array(&snap.arena, Recorded_Vertex,    maximum(1, snap.vertex_count));
 block_copy(snap.primitives, doc.primitives.items, sizeof(Recorded_Primitive) * snap.primitive_count);
 block_copy(snap.groups,     doc.groups.items,     sizeof(Recorded_Group)     * snap.group_count);
 block_copy(snap.vertices,   doc.vertices.items,   sizeof(Recorded_Vertex)    * snap.vertex_count);
 for_i32(ip, 0, snap.primitive_count)
 {// NOTE(kv) Image filenames point into the document arena; own them.
  Recorded_Primitive &prim = snap.primitives[ip];
  if(prim.type == Primitive_Type_Image)
  {
   String filename = prim.image.filename;
   u8 *bytes = cast(u8 *)push_size(&snap.arena, filename.len + 1);
   block_copy(bytes, filename.str, filename.len);
   bytes[filename.len] = 0;
   prim.image.filename.str = bytes;
  }
 }
 return snap;
}
function void
document_snapshot_restore(Recording &doc, Document_Snapshot &snap)
{// NOTE(kv) Same rebuild as export_group_to_document: clear the arena, copy in, re-own
 // the filename bytes.
 arena_clear(&doc.arena);
 init_dynamic(doc.groups,     &doc.arena, maximum(1, snap.group_count));
 init_dynamic(doc.primitives, &doc.arena, maximum(1, snap.primitive_count));
 init_dynamic(doc.vertices,   &doc.arena, maximum(1, snap.vertex_count));
 set_count(&doc.groups, snap.group_count);
 block_copy(doc.groups.items, snap.groups, sizeof(Recorded_Group) * snap.group_count);
 set_count(&doc.primitives, snap.primitive_count);
 block_copy(doc.primitives.items, snap.primitives, sizeof(Recorded_Primitive) * snap.primitive_count);
 set_count(&doc.vertices, snap.vertex_count);
 block_copy(doc.vertices.items, snap.vertices, sizeof(Recorded_Vertex) * snap.vertex_count);
 for_i32(ip, 0, doc.primitives.count)
 {
  Recorded_Primitive &prim = doc.primitives.items[ip];
  if(prim.type == Primitive_Type_Image)
  {
   String filename = prim.image.filename;
   u8 *bytes = cast(u8 *)push_size(&doc.arena, filename.len + 1);
   block_copy(bytes, filename.str, filename.len);
   bytes[filename.len] = 0;
   prim.image.filename.str = bytes;
  }
 }
 doc.captured = (doc.primitives.count > 0);
}
function void
document_snapshot_free(Document_Snapshot &snap)
{
 arena_free(&snap.arena);
 snap = {};
}

function void
history_clear(Game_State *state)
{
 Document_History &history = state->document_history;
 for_i32(i, 0, history.count){ document_snapshot_free(history.entries[i]); }
 history.count    = 0;
 history.position = -1;
 history.pending  = false;
 history.pending_action = {};
}
function void
history_begin(Game_State *state, Document_Action action)
{// NOTE(kv) Call before mutating the document. The first edit after a clear also
 // captures entry 0 (the state the edit starts from), so the baseline never goes stale.
 Document_History &history = state->document_history;
 if(history.count == 0)
 {
  Document_Action baseline = {};  // kind None: "loaded"
  history.entries[0] = document_snapshot_take(state->model.recordings.document, baseline);
  history.count    = 1;
  history.position = 0;
 }
 history.pending        = true;
 history.pending_action = action;
}
function void
history_discard(Game_State *state)
{
 state->document_history.pending = false;
}
function void
history_commit(Game_State *state)
{// NOTE(kv) Drops the redo tail, then appends the document as it is now.
 Document_History &history = state->document_history;
 if(not history.pending){ return; }
 history.pending = false;
 for_i32(i, history.position + 1, history.count){ document_snapshot_free(history.entries[i]); }
 history.count = history.position + 1;
 if(history.count == DOCUMENT_HISTORY_CAP)
 {// NOTE(kv) Full: drop the oldest state (entry 0 stays the oldest we can go back to).
  document_snapshot_free(history.entries[0]);
  for_i32(i, 1, history.count){ history.entries[i-1] = history.entries[i]; }
  history.count--;
  history.entries[history.count] = {};
 }
 history.entries[history.count] = document_snapshot_take(state->model.recordings.document,
                                                         history.pending_action);
 history.position = history.count;
 history.count++;
}
function i32 document_action_text(char *buf, i32 cap, Document_Action &action, Recording &doc);
function void document_selection_clamp(Game_State *state);  // game_curve_patch.cpp
function b32
history_jump(Game_State *state, i32 position)
{// NOTE(kv) Make the document equal entry `position` (undo/redo/panel click); saves.
 Document_History &history = state->document_history;
 if(position < 0 or position >= history.count or position == history.position){ return false; }
 history.pending = false;
 {// NOTE(kv) Status line: undo names the entry being undone, redo the one redone.
  b32 is_undo = (position < history.position);
  Document_Action &action = history.entries[is_undo ? history.position : position].action;
  char text[128];
  document_action_text(text, sizeof(text), action, state->model.recordings.document);
  snprintf(history.status, sizeof(history.status), "%s: %s", is_undo ? "undo" : "redo", text);
  history.status_frames = 120;
 }
 history.position = position;
 document_snapshot_restore(state->model.recordings.document, history.entries[position]);
 document_selection_clamp(state);  // NOTE(kv) Q5: keep the selection, drop dangling indices
 save_document_file(state);
 return true;
}
function b32
history_undo(Game_State *state){ return history_jump(state, state->document_history.position - 1); }
function b32
history_redo(Game_State *state){ return history_jump(state, state->document_history.position + 1); }

function String
document_group_name(Recording &doc, i32 prim_index)
{// NOTE(kv) The vis tag of the primitive's group, for readable labels.
 if(prim_index < 0 or prim_index >= doc.primitives.count){ return strlit("?"); }
 i32 group_index = doc.primitives.items[prim_index].group_index;
 if(group_index < 0 or group_index >= doc.groups.count){ return strlit("?"); }
 return group_vis_name(doc.groups.items[group_index].vis_tag);
}
function i32
document_action_text(char *buf, i32 cap, Document_Action &action, Recording &doc)
{// NOTE(kv) One row of the History panel. `doc` is only used for group names, so a
 // label can be off after later edits reorder things; the snapshot is what restores.
 switch(action.kind)
 {
  case Document_Action_None:
   return snprintf(buf, cap, "loaded");
  case Document_Action_Move_Vertex:
   return snprintf(buf, cap, "move vertex %d (%.*s)", action.index,
                   strexpand(document_group_name(doc, action.prim_index)));
  case Document_Action_Move_Handle:
   return snprintf(buf, cap, "move handle e[%d] of curve %d (%.*s)", action.index,
                   action.prim_index, strexpand(document_group_name(doc, action.prim_index)));
  case Document_Action_Move_Stroke:
   return snprintf(buf, cap, "move stroke %d (%.*s)", action.prim_index,
                   strexpand(document_group_name(doc, action.prim_index)));
  case Document_Action_Make_Patch:
  {
   i32 n = snprintf(buf, cap, "make patch [");
   for_i32(i, 0, action.count)
   {
    n += snprintf(buf + n, maximum(0, cap - n), "%s%d", i ? " " : "", action.indices[i]);
   }
   n += snprintf(buf + n, maximum(0, cap - n), "]");
   return n;
  }
  case Document_Action_Delete_Patch:
   return snprintf(buf, cap, "delete patch %d", action.prim_index);
  case Document_Action_Export_Group:
   return snprintf(buf, cap, "export group %.*s", strexpand(group_vis_name(action.tag)));
  case Document_Action_Add_Line:
   return snprintf(buf, cap, "add line %d (%.*s)", action.prim_index,
                   strexpand(document_group_name(doc, action.prim_index)));
  case Document_Action_Delete_Curve:
   return snprintf(buf, cap, "delete curve %d", action.prim_index);
  case Document_Action_Set_Radii:
  case Document_Action_Set_Midline:
  {
   i32 n = snprintf(buf, cap, "%s [",
                    action.kind == Document_Action_Set_Radii ? "set radii"
                    : action.index ? "set midline" : "clear midline");
   for_i32(i, 0, action.count)
   {
    n += snprintf(buf + n, maximum(0, cap - n), "%s%d", i ? " " : "", action.indices[i]);
   }
   n += snprintf(buf + n, maximum(0, cap - n), "]");
   return n;
  }
  case Document_Action_Delete_Selection:
  {
   i32 n = snprintf(buf, cap, "delete [");
   for_i32(i, 0, action.count)
   {
    n += snprintf(buf + n, maximum(0, cap - n), "%s%d", i ? " " : "", action.indices[i]);
   }
   n += snprintf(buf + n, maximum(0, cap - n), "]");
   return n;
  }
 }
 return snprintf(buf, cap, "?");
}
function void
history_dump(FILE *out, Game_State *state)
{// NOTE(kv) Debug channel `history_dump`: the panel rows as text, `>` = current.
 Document_History &history = state->document_history;
 Recording &doc = state->model.recordings.document;
 fprintf(out, "history: %d entries, position %d%s\n", history.count, history.position,
         history.pending ? ", edit pending" : "");
 for_i32(i, 0, history.count)
 {
  char buf[128];
  document_action_text(buf, sizeof(buf), history.entries[i].action, doc);
  fprintf(out, "%c %2d  %s\n", (i == history.position) ? '>' : ' ', i, buf);
 }
}
//-EOF
