// NOTE(kv) Mouse editing of the document (plan-document-mouse-editing, 2026-09-06).
// Pixel-level entry points shared by the real mouse and the debug channel's virtual
// mouse (Q9): press grabs a control point of a SELECTED document primitive (since
// 2026-09-13; before that, of the hot one), move drags it in the camera plane (Q3) and
// writes the delta back through the inverse bone transform (Q4), release saves
// driver.document.ad (Q5). No mode: a press near a selected control point IS the drag
// (Q6); a press on an unselected primitive selects it (game_main.cpp).
//
// Control points: table vertices (shared, via Recorded_Primitive.vertex_index) and,
// for curves, the two bezier handles (per-curve offsets from the chord thirds, like the
// tablet, since plan-curve-chord-handles). Moving a vertex carries the handles of every
// curve sharing it along the chord and rotates them with it; a handle drag stays in the
// curve's plane (plan-curve-coplanar-handles), or with Ctrl held at press moves freely
// and swings the other handle into the new plane (plan-handle-drag-modes).

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

function tvert
document_curve_endpoint(Recording &doc, Recorded_Primitive &prim, i32 slot)
{// NOTE(kv) Table vertex `slot` (0 = v0, 1 = v1) of a curve as a tvert, for the
 // chord-third builders in game_draw.cpp (plan-curve-chord-handles).
 Recorded_Vertex &vertex = doc.vertices[prim.vertex_index[slot]];
 return {.v = vertex.p, .bone_id = vertex.bone};
}
function tvert
document_curve_handle_point(Recording &doc, Recorded_Primitive &prim, i32 i)
{// NOTE(kv) P1 (i=0) / P2 (i=1) built from the table + handle_offset.
 return curve_handle_point(document_curve_endpoint(doc, prim, 0),
                           document_curve_endpoint(doc, prim, 1),
                           prim.curve.handle_offset[i], i);
}
function void
document_curve_set_handle_point(Recording &doc, Recorded_Primitive &prim, i32 i, tvert point)
{// NOTE(kv) Inverse: store the offset that puts P1/P2 at `point`.
 prim.curve.handle_offset[i] = curve_handle_offset_from_point(document_curve_endpoint(doc, prim, 0),
                                                              document_curve_endpoint(doc, prim, 1),
                                                              point, i);
}

function v3
document_pick_world_pos(Recording &doc, Document_Pick pick)
{
 Recorded_Primitive &prim = doc.primitives[pick.prim_index];
 v3 bone_p; Bone_ID bone_id;
 if(pick.is_handle)
 {
  tvert handle = document_curve_handle_point(doc, prim, pick.slot-1);  // NOTE(kv) slot = e-index 1 or 2
  bone_p = handle.v;
  // TODO(kv) plan-curve-chord-handles Q3: assumes the offset's bone is the group bone
  // (true for every document point today; capture logs when a curve mixes bones).
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

// NOTE(kv) Control points per primitive: up to recorded_vertex_cap table vertices, plus
// the two handles of a curve.
global i32 const document_pick_cap_per_primitive = recorded_vertex_cap + 2;
// NOTE(kv) Control points over the whole selection: every selected primitive, both
// mirror sides (document_selection_pick_list).
global i32 const document_selection_pick_cap = Document_Selection_Cap * 2 * document_pick_cap_per_primitive;

function i32
document_pick_list(Recording &doc, Location hot, Document_Pick out[document_pick_cap_per_primitive])
{// NOTE(kv) Every control point of the hot document primitive: its table vertices,
 // plus the two handles if it's a curve. Returns the count (0 if `hot` isn't a
 // document item).
 if(not is_document_location(hot)){ return 0; }
 i32 prim_index = document_primitive_index(hot);
 if(prim_index < 0 or prim_index >= doc.primitives.count){ return 0; }
 Recorded_Primitive &prim = doc.primitives[prim_index];
 b32 is_right = document_location_is_right(hot);
 i32 count = 0;
 for_i32(slot, 0, primitive_vertex_count(prim.type))
 {
  out[count++] = {prim_index, is_right, false, slot};
 }
 if(prim.type == Primitive_Type_Curve)
 {
  out[count++] = {prim_index, is_right, true, 1};
  out[count++] = {prim_index, is_right, true, 2};
 }
 return count;
}
// NOTE(kv) Explicit control-point selection (Khoa, 2026-09-13, follow-up in
// plan-active-primitive-delete-key.md, after the tablet's edit_pen_down): control
// points are grabbable ONLY on the selected primitives, within this pixel radius, no
// matter what is hot -- so a vertex shared by two chained curves is edited through
// the curve you selected. Pen down on an unselected primitive just selects it.
global v1 const document_pick_radius_px = 12.f;

function i32
document_selection_pick_list(Game_State *state, Document_Pick *out, i32 cap)
{// NOTE(kv) Every control point of every selected primitive, both mirror sides
 // (the right side only for two-sided groups, like convert_primitives_to_camera_space).
 Recording &doc = state->model.recordings.document;
 Document_Selection &sel = state->document_selection;
 i32 count = 0;
 for_i32(i, 0, sel.count)
 {
  i32 prim_index = sel.prim_index[i];
  if(prim_index < 0 or prim_index >= doc.primitives.count){ continue; }
  Recorded_Primitive &prim = doc.primitives[prim_index];
  b32 one_sided = (doc.groups[prim.group_index].one_sided or
                   (prim.type == Primitive_Type_Curve and prim.curve.midline));
  // NOTE(kv) PITFALL: for_i32 doesn't parenthesize the bound -- `side < a ? 1 : 2`
  // is always true (spun the app on 2026-09-13), so the bound goes in a variable.
  i32 side_count = one_sided ? 1 : 2;
  for_i32(side, 0, side_count)
  {
   Document_Pick picks[document_pick_cap_per_primitive];
   i32 n = document_pick_list(doc, document_location(prim_index, side == 1), picks);
   for_i32(k, 0, n)
   {
    if(count < cap)
    {
     out[count++] = picks[k];
    }
   }
  }
 }
 return count;
}
function b32
document_pick_nearest(Game_State *state, Live_Viewport *viewport, v2 mouse_px,
                      Document_Pick *best_out, v1 *dist_out)
{// NOTE(kv) The selected control point nearest the mouse (in pixels) and how far it
 // is -- a press grabs it when within document_pick_radius_px, and the hover
 // highlight shows it. False when nothing is selected.
 Recording &doc = state->model.recordings.document;
 if(not viewport){ return false; }
 Document_Pick picks[document_selection_pick_cap];
 i32 pick_count = document_selection_pick_list(state, picks, ArrayCount(picks));
 if(pick_count == 0){ return false; }
 Camera camera = setup_camera(state->viewports[0].camera);
 v2 center = get_center(viewport->clip_box);
 v1 best_dist = INFINITY;
 for_i32(i, 0, pick_count)
 {
  v2 px = document_edit_project(camera, center, document_pick_world_pos(doc, picks[i]));
  v1 dist = lengthof(V3(px - mouse_px, 0));
  if(dist < best_dist)
  {
   best_dist = dist;
   *best_out = picks[i];
  }
 }
 *dist_out = best_dist;
 return true;
}

// NOTE(kv) Hover highlight (Khoa, 2026-09-12: "highlight hot vertices that I hover
// mouse over" + show the vertex id). Per frame, not saved: the selected primitives'
// control points draw as disks, the one a press would grab (or the one being dragged)
// bigger and in hot_color2, and its id goes to the debug text line.
// Globals, not Game_State: purely transient, and a DLL reload recomputes them next frame.
global b32           document_hover_valid;  // something selected (or a drag running)
global b32           document_hover_grab;   // document_hover_pick is within grab radius
global Document_Pick document_hover_pick;

function void
document_hover_update(Game_State *state, Live_Viewport *viewport, v2 mouse_px)
{
 document_hover_valid = false;
 document_hover_grab  = false;
 Document_Edit_State &edit = state->document_edit;
 v1 dist = INFINITY;
 if(edit.active)
 {// NOTE(kv) Mid-drag: the grabbed point stays highlighted wherever the mouse goes.
  document_hover_valid = true;
  document_hover_grab  = true;
  document_hover_pick  = edit.pick;
 }
 else if(document_pick_nearest(state, viewport, mouse_px, &document_hover_pick, &dist))
 {
  document_hover_valid = true;
  document_hover_grab  = (dist <= document_pick_radius_px);
 }
}
function i32
document_hover_label(char *buf, i32 cap, Recording &doc)
{// NOTE(kv) "vertex 43 (Vis_Cheek) -- slot 0 of curve 48" / "handle e[1] of curve 48".
 if(not document_hover_valid or not document_hover_grab){ return 0; }
 Document_Pick &pick = document_hover_pick;
 Recorded_Primitive &prim = doc.primitives[pick.prim_index];
 String group_name = document_group_name(doc, pick.prim_index);
 if(pick.is_handle)
 {
  return snprintf(buf, cap, "handle e[%d] of prim %d (%.*s)", pick.slot, pick.prim_index,
                  strexpand(group_name));
 }
 return snprintf(buf, cap, "vertex %d (%.*s) -- slot %d of prim %d",
                 prim.vertex_index[pick.slot], strexpand(group_name), pick.slot, pick.prim_index);
}
function void
document_hover_draw(Game_State *state, Camera &camera)
{// NOTE(kv) World-space disks facing the camera, overlaid (they mark positions, depth
 // would hide the ones behind a fill). Same depth-scaled sizing as the kb cursor.
 if(not document_hover_valid){ return; }
 Recording &doc = state->model.recordings.document;
 Document_Pick picks[document_selection_pick_cap];
 i32 pick_count = document_selection_pick_list(state, picks, ArrayCount(picks));
 for_i32(i, 0, pick_count)
 {
  Document_Pick &pick = picks[i];
  b32 is_hovered = (document_hover_grab and
                    pick.prim_index == document_hover_pick.prim_index and
                    pick.is_right   == document_hover_pick.is_right and
                    pick.is_handle  == document_hover_pick.is_handle and
                    pick.slot       == document_hover_pick.slot);
  v3 center = document_pick_world_pos(doc, pick);
  v1 dist = lengthof(mat4vert(camera.cam_from_world, center));
  v1 radius = (is_hovered ? 4.5f : 3.f)*millimeter * dist / camera.focal_length;
  argb color = (is_hovered ? hot_color2 : pick.is_handle ? linear_argb_blue : linear_argb_silver);
  const i32 nslice = 12;
  v3 last = {};
  for_i32(k, 0, nslice+1)
  {
   v2 arm = radius*arm2(v1(k) / v1(nslice));
   v3 sample = center + arm.x*camera.x + arm.y*camera.y;
   if(k != 0)
   {
    v3 points[3] = {center, last, sample};
    poly3_inner(mk_poly3(points), repeat3(color), {Poly_Overlay});
   }
   last = sample;
  }
 }
}

function void
document_edit_press(Game_State *state, Live_Viewport *viewport, v2 mouse_px, Document_Pick best,
                    b32 free_handle)
{// NOTE(kv) Grab one control point (a table vertex, or a curve handle) of a selected
 // primitive; the caller found it with document_pick_nearest within the grab radius.
 // `free_handle` (Ctrl at press, plan-handle-drag-modes Q2): a handle drag defines a new
 // plane instead of staying in the old one; means nothing for a vertex (Q5).
 Document_Edit_State &edit = state->document_edit;
 Recording &doc = state->model.recordings.document;
 if(not viewport){ return; }
 i32 prim_index = best.prim_index;
 Recorded_Primitive &prim = doc.primitives[prim_index];

 v3 world = document_pick_world_pos(doc, best);
 Camera camera = setup_camera(state->viewports[0].camera);
 v2 center = get_center(viewport->clip_box);
 edit.active = true;
 edit.pick = best;
 edit.free_handle = (free_handle and best.is_handle);
 edit.grab_cam_z = mat4vert(camera.cam_from_world, world).z;
 edit.grab_offset_px = mouse_px - document_edit_project(camera, center, world);
 edit.location = document_location(prim_index, best.is_right);
 {// NOTE(kv) Open the history entry now; release commits it only if something moved.
  Document_Action action = {};
  action.kind       = best.is_handle ? Document_Action_Move_Handle : Document_Action_Move_Vertex;
  action.prim_index = prim_index;
  action.index      = best.is_handle ? best.slot : prim.vertex_index[best.slot];
  history_begin(state, action);
 }
}

function void
document_edit_press_stroke(Game_State *state, Live_Viewport *viewport, v2 mouse_px,
                           i32 prim_index, b32 is_right)
{// NOTE(kv) plan-selection-followups Q4: grab a whole selected curve (the caller
 // checked it is selected and the press is away from its control points). Vertex slot
 // 0 is the drag's depth/offset reference; the move applies the same world delta to
 // both vertices.
 Document_Edit_State &edit = state->document_edit;
 Recording &doc = state->model.recordings.document;
 if(not viewport){ return; }
 Document_Pick reference = {prim_index, is_right, false, 0};
 v3 world = document_pick_world_pos(doc, reference);
 Camera camera = setup_camera(state->viewports[0].camera);
 v2 center = get_center(viewport->clip_box);
 edit.active = true;
 edit.whole_stroke = true;
 edit.pick = reference;
 edit.grab_cam_z = mat4vert(camera.cam_from_world, world).z;
 edit.grab_offset_px = mouse_px - document_edit_project(camera, center, world);
 edit.location = document_location(prim_index, is_right);
 {
  Document_Action action = {};
  action.kind       = Document_Action_Move_Stroke;
  action.prim_index = prim_index;
  history_begin(state, action);
 }
}

// NOTE(kv) Q4: world delta -> a point's own bone space (rotation only: it's a delta,
// so the bone's translation must not apply).
function v3
document_edit_bone_delta(Bone_ID bone_id, b32 is_right, v3 delta_world)
{
 mat4 bone_from_world = get_bone(bone_id, is_right)->world_from_bone.inverse;
 return mat4vert(bone_from_world, delta_world) - mat4vert(bone_from_world, V3());
}

function Bone_ID
document_curve_offset_bone(Recording &doc, Recorded_Primitive &prim)
{// TODO(kv) plan-curve-chord-handles Q3: the offsets' bone is the group bone (true for
 // every document point today; capture counts curves that mix bones).
 return doc.groups[prim.group_index].bone_id;
}
function v3
document_curve_chord(Recording &doc, Recorded_Primitive &prim)
{// NOTE(kv) v1 - v0 in the offsets' bone space (unnormalized).
 Bone_ID bone_id = document_curve_offset_bone(doc, prim);
 return (tvert_in_bone(document_curve_endpoint(doc, prim, 1), bone_id) -
         tvert_in_bone(document_curve_endpoint(doc, prim, 0), bone_id));
}

function void
document_edit_move_vertex(Recording &doc, Recorded_Primitive &prim, i32 vertex_index,
                          b32 is_right, v3 delta_world, i32 skip_rotate_prim = -1)
{// NOTE(kv) Move one table vertex by a world delta. The handles of every curve sharing
 // it are offsets from the chord thirds, so they translate on their own; on top of that
 // (plan-curve-coplanar-handles Q2, tablet move_vertex) both offsets of every such
 // curve get the minimal rotation old-chord-dir -> new-chord-dir, so the in-plane shape
 // rides the chord and {chord, d0, d3} stay coplanar. Skipped when either chord is ~0,
 // and for `skip_rotate_prim` (the whole-stroke drag: that chord only translates).
 Recorded_Vertex &vertex = doc.vertices[vertex_index];
 vertex.p += document_edit_bone_delta(document_vertex_bone(doc, prim, vertex), is_right, delta_world);
 for_i32(iprim, 0, doc.primitives.count)
 {
  Recorded_Primitive &curve = doc.primitives[iprim];
  if(curve.type != Primitive_Type_Curve or iprim == skip_rotate_prim){ continue; }
  b32 at0 = (curve.vertex_index[0] == vertex_index);
  b32 at1 = (curve.vertex_index[1] == vertex_index);
  if(at0 == at1){ continue; }  // NOTE(kv) not on this curve, or a loop (chord stays 0)
  // NOTE(kv) The old chord = the new one minus what the moved end gained, in the
  // offsets' bone space (the same bone as the vertex today).
  v3 delta = document_edit_bone_delta(document_curve_offset_bone(doc, curve), is_right, delta_world);
  v3 new_chord = document_curve_chord(doc, curve);
  v3 old_chord = at1 ? (new_chord - delta) : (new_chord + delta);
  if(lengthof(old_chord) < curve_collinear_epsilon or lengthof(new_chord) < curve_collinear_epsilon){ continue; }
  v3 from = noz(old_chord);
  v3 to   = noz(new_chord);
  v3 flip_axis = perpendicular_to_direction(from);
  for_i32(i, 0, 2)
  {
   v3 &offset = curve.curve.handle_offset[i].v;
   offset = rotate_between_directions(offset, from, to, flip_axis);
  }
 }
}

//~ NOTE(kv) plan-curve-coplanar-handles Q1: a handle drag lands where the mouse ray
// pierces the curve's plane (tablet "plane" mode, the default there); the other handle
// never moves. Edge-on plane or a hit behind the eye: the handle stays where it is.
global v1 const document_edit_edge_on_cosine = 0.15f;  // tablet EDGE_ON_PLANE_COSINE
function b32
document_edit_handle_plane_target(Recording &doc, Camera const &camera, v2 viewport_center,
                                  v2 px, Document_Pick pick, v3 *world_out)
{// NOTE(kv) All in world space, on the picked mirror side: the plane is {chord, d0}
 // (fallback d3, fallback camera-facing), the ray goes from the eye through `px`.
 // NOTE(kv) PITFALL: `v1` is the scalar type, so the endpoints are p0/p3 here.
 Document_Pick p0_pick = {pick.prim_index, pick.is_right, false, 0};
 Document_Pick p3_pick = {pick.prim_index, pick.is_right, false, 1};
 Document_Pick h0_pick = {pick.prim_index, pick.is_right, true, 1};
 Document_Pick h1_pick = {pick.prim_index, pick.is_right, true, 2};
 v3 p0 = document_pick_world_pos(doc, p0_pick);
 v3 p3 = document_pick_world_pos(doc, p3_pick);
 v3 d0 = document_pick_world_pos(doc, h0_pick) - (2.f*p0 + p3)/3.f;
 v3 d3 = document_pick_world_pos(doc, h1_pick) - (p0 + 2.f*p3)/3.f;
 v3 u = curve_chord_direction(p0, p3);
 v3 camera_forward = -camera.z;  // NOTE(kv) the camera looks down its -z
 v3 normal = curve_plane_normal(u, d0, d3, camera_forward);
 v3 eye = mat4vert(camera.world_from_cam, V3(0,0,0));
 v3 ray = noz(document_edit_unproject(camera, viewport_center, px, -1.f) - eye);
 v1 denom = dot(ray, normal);
 if(absolute(denom) < document_edit_edge_on_cosine){ return false; }
 v1 t = dot(p0 - eye, normal) / denom;
 if(t <= 0){ return false; }
 *world_out = eye + ray*t;
 return true;
}

//~ NOTE(kv) plan-handle-drag-modes Q1 (tablet "swing"): after a free handle drag the
// OTHER handle is swung into the plane the dragged one now spans with the chord, so the
// curve stays coplanar -- the drag defines the plane. Roll (both handles about the chord,
// plan-curve-coplanar-handles Q7/Q8) was dropped for this on 2026-09-15: a free drag out
// of the plane is a roll you can see.
function void
document_curve_swing_other_handle(Recording &doc, Recorded_Primitive &prim, i32 dragged_slot)
{// NOTE(kv) `dragged_slot` is the handle_offset index (0 = d0, 1 = d3). ~0 chord: no
 // plane to keep, nothing happens.
 v3 chord = document_curve_chord(doc, prim);
 if(lengthof(chord) < curve_collinear_epsilon){ return; }
 v3 u = noz(chord);
 v3 leader   = prim.curve.handle_offset[dragged_slot].v;
 v3 &follower = prim.curve.handle_offset[1 - dragged_slot].v;
 follower = swing_offset_into_plane(u, leader, follower);
}
function b32
document_curve_coplanarize(Recording &doc, i32 prim_index)
{// NOTE(kv) Q3: swing d3 into the plane {chord, d0} (the `coplanarize` channel
 // command; no migration, no capture-time flattening -- the document holds what was
 // drawn). Returns false when nothing changed.
 if(prim_index < 0 or prim_index >= doc.primitives.count){ return false; }
 Recorded_Primitive &prim = doc.primitives[prim_index];
 if(prim.type != Primitive_Type_Curve){ return false; }
 v3 chord = document_curve_chord(doc, prim);
 if(lengthof(chord) < curve_collinear_epsilon){ return false; }
 v3 u = noz(chord);
 v3 &d0 = prim.curve.handle_offset[0].v;
 v3 &d3 = prim.curve.handle_offset[1].v;
 v3 swung = swing_offset_into_plane(u, d0, d3);
 if(length_squared(swung - d3) < 1e-12f){ return false; }
 d3 = swung;
 return true;
}

//~ NOTE(kv) plan-focus-radii-midline Q8: midline curves stay on the mirror plane.
function void
document_curve_apply_midline(Recording &doc, i32 prim_index)
{// NOTE(kv) x=0 in each point's own bone space (that is the axis the right pass
 // negates). A table vertex shared with a non-midline curve is pinned too: the flag wins.
 Recorded_Primitive &prim = doc.primitives[prim_index];
 if(prim.type != Primitive_Type_Curve or not prim.curve.midline){ return; }
 for_i32(slot, 0, 2){ doc.vertices[prim.vertex_index[slot]].p.x = 0; }
 // NOTE(kv) plan-curve-table-first: the two handle offsets are the only per-curve
 // points; `bezier` is scratch that resolve_vertices rebuilds on copies, nothing to keep
 // in step. With both vertices at x=0 the chord is on the plane, so zeroing the
 // offsets' x keeps P1/P2 there too (plan-curve-chord-handles Q6).
 for_i32(i, 0, 2){ prim.curve.handle_offset[i].v.x = 0; }
 // TODO(kv) dbezier (shape-key delta) is left alone; a keyed midline curve could still
 // blend off the plane.
}
function void
document_apply_midlines(Recording &doc)
{// NOTE(kv) After any write: a moved shared vertex may belong to a midline curve too.
 for_i32(iprim, 0, doc.primitives.count){ document_curve_apply_midline(doc, iprim); }
}

//~ NOTE(kv) Selection panel edits (plan-focus-radii-midline Q4/Q9): every selected
// curve; the caller brackets a slider drag with begin/commit so one drag = one entry.
function i32
document_selection_curve_indices(Game_State *state, i32 out[Document_Selection_Cap])
{
 Recording &doc = state->model.recordings.document;
 Document_Selection &sel = state->document_selection;
 i32 count = 0;
 for_i32(i, 0, sel.count)
 {
  i32 prim_index = sel.prim_index[i];
  if(prim_index >= 0 and prim_index < doc.primitives.count and
     doc.primitives[prim_index].type == Primitive_Type_Curve)
  {
   out[count++] = prim_index;
  }
 }
 return count;
}
function Document_Action
document_selection_action(Game_State *state, Document_Action_Kind kind)
{
 Document_Action action = {};
 action.kind  = kind;
 action.count = document_selection_curve_indices(state, action.indices);
 return action;
}
function void
document_set_radii_begin(Game_State *state)
{
 history_begin(state, document_selection_action(state, Document_Action_Set_Radii));
}
function void
document_set_radii_apply(Game_State *state, v4 radii)
{// NOTE(kv) Between begin and commit (a slider drag), or alone for a one-shot set.
 Recording &doc = state->model.recordings.document;
 i32 curves[Document_Selection_Cap];
 i32 count = document_selection_curve_indices(state, curves);
 for_i32(i, 0, count)
 {// TODO(kv) Q6: `dradii` (shape-key delta) is not touched.
  doc.primitives[curves[i]].curve.radii = radii;
 }
}
function void
document_set_radii_scale(Game_State *state, v1 scale)
{// NOTE(kv) The width slider: scales each curve's own profile, keeps its taper.
 Recording &doc = state->model.recordings.document;
 i32 curves[Document_Selection_Cap];
 i32 count = document_selection_curve_indices(state, curves);
 for_i32(i, 0, count)
 {
  doc.primitives[curves[i]].curve.radii *= scale;
 }
}
function void
document_edit_commit_and_save(Game_State *state)
{
 history_commit(state);
 save_document_file(state);
}
function b32
document_set_midline(Game_State *state, b32 midline)
{// NOTE(kv) One-shot: begin + apply + commit. Returns false with nothing selected.
 Recording &doc = state->model.recordings.document;
 i32 curves[Document_Selection_Cap];
 i32 count = document_selection_curve_indices(state, curves);
 if(count == 0){ return false; }
 Document_Action action = document_selection_action(state, Document_Action_Set_Midline);
 action.index = midline;
 history_begin(state, action);
 for_i32(i, 0, count)
 {
  doc.primitives[curves[i]].curve.midline = midline;
  document_curve_apply_midline(doc, curves[i]);
 }
 document_edit_commit_and_save(state);
 return true;
}

function b32
document_coplanarize_once(Game_State *state, i32 prim_index)
{// NOTE(kv) Channel `coplanarize <prim>`: one-shot with its own history entry.
 Document_Action action = {};
 action.kind = Document_Action_Coplanarize;
 action.prim_index = prim_index;
 history_begin(state, action);
 if(not document_curve_coplanarize(state->model.recordings.document, prim_index))
 {
  history_discard(state);
  return false;
 }
 document_edit_commit_and_save(state);
 return true;
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
 v3 new_world;
 // NOTE(kv) plan-handle-drag-modes Q4: a midline curve's plane IS the mirror plane, so a
 // free drag on it is just an in-plane drag (apply_midline would flatten it anyway).
 b32 free_handle = (edit.free_handle and not prim.curve.midline);
 if(pick.is_handle and not edit.whole_stroke and not free_handle)
 {// NOTE(kv) plan-curve-coplanar-handles Q1: the handle stays in the curve's plane.
  if(not document_edit_handle_plane_target(doc, camera, center, mouse_px - edit.grab_offset_px,
                                           pick, &new_world))
  {
   return;
  }
 }
 else
 {
  new_world = document_edit_unproject(camera, center, mouse_px - edit.grab_offset_px,
                                      edit.grab_cam_z);
 }
 v3 delta_world = new_world - old_world;
 // NOTE(kv) Project/unproject round-trip jitter must not count as an edit (it would
 // save the file on every release).
 if(length_squared(delta_world) < 1e-12f){ return; }
 edit.moved = true;

 if(edit.whole_stroke)
 {// NOTE(kv) Both endpoints; a curve looping onto one vertex moves it once.
  i32 v0 = prim.vertex_index[0];
  i32 v1 = prim.vertex_index[1];
  // NOTE(kv) This curve's chord only translates, so its handles are not rotated.
  // Another curve sharing one of the vertices turns as it would under a vertex drag.
  // TODO(kv) A curve chained to BOTH ends of the dragged stroke rotates on the first
  // move and back on the second: identity up to float noise, not exactly.
  document_edit_move_vertex(doc, prim, v0, pick.is_right, delta_world, pick.prim_index);
  if(v1 != v0)
  {
   document_edit_move_vertex(doc, prim, v1, pick.is_right, delta_world, pick.prim_index);
  }
 }
 else if(pick.is_handle)
 {
  // NOTE(kv) The chord third does not move during a handle drag, so the offset takes
  // the whole delta (plan-curve-chord-handles Q6). In-plane: the target point is on the
  // curve's plane already (plan-curve-coplanar-handles Q1), the other handle is
  // untouched. Free: the target is in the camera plane at the grab depth, and the other
  // handle follows into the new plane (plan-handle-drag-modes Q1).
  Bone_ID bone_id = doc.groups[prim.group_index].bone_id;
  prim.curve.handle_offset[pick.slot-1].v += document_edit_bone_delta(bone_id, pick.is_right, delta_world);
  if(free_handle){ document_curve_swing_other_handle(doc, prim, pick.slot-1); }
 }
 else
 {
  document_edit_move_vertex(doc, prim, prim.vertex_index[pick.slot], pick.is_right, delta_world);
 }
 document_apply_midlines(doc);
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
