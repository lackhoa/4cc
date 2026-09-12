// NOTE(kv) Mouse editing of the document (plan-document-mouse-editing, 2026-09-06).
// Pixel-level entry points shared by the real mouse and the debug channel's virtual
// mouse (Q9): press grabs a control point of the hot document primitive, move drags it
// in the camera plane (Q3) and writes the delta back through the inverse bone
// transform (Q4), release saves driver.document.ad (Q5). No mode: a press on a hot
// document item IS the drag (Q6).
//
// Control points: table vertices (shared, via Recorded_Primitive.vertex_index) and,
// for curves, the two bezier handles (per-curve tverts, absolute bone-space positions).
// Moving a vertex also moves the handles attached to it on every curve sharing it, so
// the tangents ride along (the desktop stores handles absolutely, unlike the tablet's
// chord offsets).

// Document_Pick / Document_Edit_State live in framework.h (Game_State member).

function v2
document_edit_project(Camera const &camera, v2 viewport_center, v3 world)
{// NOTE(kv) World -> window pixels, the inverse of get_primitive_hit_by_mouse's ray.
 v3 cam = mat4vert(camera.cam_from_world, world);
 v1 scale = camera.focal_length / -cam.z;
 v2 px = V2(cam.x, -cam.y) * (scale * default_meter_to_pixel);
 return px + viewport_center;
}

function v3
document_edit_unproject(Camera const &camera, v2 viewport_center, v2 px, v1 cam_z)
{// NOTE(kv) Window pixels -> world point on the camera plane at depth cam_z (< 0).
 v2 meter = (px - viewport_center) / default_meter_to_pixel;
 v1 scale = -cam_z / camera.focal_length;
 v3 cam = V3(meter.x * scale, -meter.y * scale, cam_z);
 return mat4vert(camera.world_from_cam, cam);
}

function Bone_ID
document_vertex_bone(Recording &doc, Recorded_Primitive &prim, Recorded_Vertex &vertex)
{// NOTE(kv) Bone_None on a vertex means "the group's bone" (see Recorded_Vertex).
 if(vertex.bone.type == Bone_None){ return doc.groups[prim.group_index].bone_id; }
 return vertex.bone;
}

function v3
document_pick_world_pos(Recording &doc, Document_Pick pick)
{
 Recorded_Primitive &prim = doc.primitives[pick.prim_index];
 v3 bone_p; Bone_ID bone_id;
 if(pick.is_handle)
 {
  tvert &handle = prim.curve.bezier.e[pick.slot];
  bone_p = handle.v;
  bone_id = doc.groups[prim.group_index].bone_id;
 }
 else
 {
  Recorded_Vertex &vertex = doc.vertices[prim.vertex_index[pick.slot]];
  bone_p = vertex.p;
  bone_id = document_vertex_bone(doc, prim, vertex);
 }
 return mat4vert(get_bone(bone_id, pick.is_right)->world_from_bone, bone_p);
}

function void
document_edit_press(Game_State *state, Live_Viewport *viewport, v2 mouse_px, Location hot)
{// NOTE(kv) Grab the control point of the hot document primitive nearest the mouse
 // (in pixels): its table vertices, plus the handles if it's a curve.
 Document_Edit_State &edit = state->document_edit;
 Recording &doc = state->model.recordings.document;
 if(not viewport or not is_document_location(hot)){ return; }
 i32 prim_index = document_primitive_index(hot);
 if(prim_index < 0 or prim_index >= doc.primitives.count){ return; }
 Recorded_Primitive &prim = doc.primitives[prim_index];

 Camera camera = setup_camera(state->viewports[0].camera);
 v2 center = get_center(viewport->clip_box);
 Document_Pick best = {};
 v1 best_dist = INFINITY;
 auto consider = [&](Document_Pick pick)
 {
  v2 px = document_edit_project(camera, center, document_pick_world_pos(doc, pick));
  v1 dist = lengthof(V3(px - mouse_px, 0));
  if(dist < best_dist){ best_dist = dist; best = pick; }
 };
 i32 vertex_count = primitive_vertex_count(prim.type);
 for_i32(slot, 0, vertex_count)
 {
  consider({prim_index, document_location_is_right(hot), false, slot});
 }
 if(prim.type == Primitive_Type_Curve)
 {
  consider({prim_index, document_location_is_right(hot), true, 1});
  consider({prim_index, document_location_is_right(hot), true, 2});
 }
 if(best_dist == INFINITY){ return; }

 v3 world = document_pick_world_pos(doc, best);
 edit.active = true;
 edit.pick = best;
 edit.grab_cam_z = mat4vert(camera.cam_from_world, world).z;
 edit.grab_offset_px = mouse_px - document_edit_project(camera, center, world);
 edit.location = hot;
 {// NOTE(kv) Open the history entry now; release commits it only if something moved.
  Document_Action action = {};
  action.kind       = best.is_handle ? Document_Action_Move_Handle : Document_Action_Move_Vertex;
  action.prim_index = prim_index;
  action.index      = best.is_handle ? best.slot : prim.vertex_index[best.slot];
  history_begin(state, action);
 }
}

function void
document_edit_move(Game_State *state, Live_Viewport *viewport, v2 mouse_px)
{
 Document_Edit_State &edit = state->document_edit;
 Recording &doc = state->model.recordings.document;
 if(not edit.active or not viewport){ return; }
 Document_Pick pick = edit.pick;
 Recorded_Primitive &prim = doc.primitives[pick.prim_index];

 Camera camera = setup_camera(state->viewports[0].camera);
 v2 center = get_center(viewport->clip_box);
 v3 old_world = document_pick_world_pos(doc, pick);
 v3 new_world = document_edit_unproject(camera, center, mouse_px - edit.grab_offset_px,
                                        edit.grab_cam_z);
 v3 delta_world = new_world - old_world;
 // NOTE(kv) Project/unproject round-trip jitter must not count as an edit (it would
 // save the file on every release).
 if(length_squared(delta_world) < 1e-12f){ return; }
 edit.moved = true;

 // NOTE(kv) Q4: world delta -> the point's own bone space (rotation only: it's a
 // delta, so the bone's translation must not apply).
 auto bone_delta = [&](Bone_ID bone_id) -> v3
 {
  mat4 bone_from_world = get_bone(bone_id, pick.is_right)->world_from_bone.inverse;
  return mat4vert(bone_from_world, delta_world) - mat4vert(bone_from_world, V3());
 };

 if(pick.is_handle)
 {
  Bone_ID bone_id = doc.groups[prim.group_index].bone_id;
  prim.curve.bezier.e[pick.slot].v += bone_delta(bone_id);
 }
 else
 {
  i32 vertex_index = prim.vertex_index[pick.slot];
  Recorded_Vertex &vertex = doc.vertices[vertex_index];
  vertex.p += bone_delta(document_vertex_bone(doc, prim, vertex));
  // NOTE(kv) Drag the handles attached to this vertex along, on every curve that
  // shares it (slot 0 -> e[1], slot 1 -> e[2]).
  for_i32(iprim, 0, doc.primitives.count)
  {
   Recorded_Primitive &other = doc.primitives[iprim];
   if(other.type != Primitive_Type_Curve){ continue; }
   v3 handle_delta = bone_delta(doc.groups[other.group_index].bone_id);
   if(other.vertex_index[0] == vertex_index){ other.curve.bezier.e[1].v += handle_delta; }
   if(other.vertex_index[1] == vertex_index){ other.curve.bezier.e[2].v += handle_delta; }
  }
 }
}

function void
document_edit_release(Game_State *state)
{
 Document_Edit_State &edit = state->document_edit;
 if(not edit.active){ return; }
 if(edit.moved)
 {// NOTE(kv) Q5: the file is the document (saved on every edit); undo/redo restore
  // history snapshots (game_document_history.cpp).
  history_commit(state);
  save_document_file(state);
 }
 else
 {
  history_discard(state);
 }
 edit = {};
}
