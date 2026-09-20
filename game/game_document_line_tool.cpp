// NOTE(kv) Line tool: add a curve to the document with the mouse (2026-09-12,
// plan-document-line-tool.md). Port of the tablet's line_tool.ts (Q27 there): while
// armed, a left-drag places one cubic curve on a camera-facing plane (press = p0,
// release = p3). The raw pen path is least-squares-fitted to the two interior control
// points on every move, so the curve follows the drawn shape live (it is a real
// document primitive from the first move on, drawn by the normal replay). The start
// snaps to an existing table vertex so curves can continue from a shared vertex. A
// click without a drag disarms the tool.
//
// The stroke plane (plan-line-tool-snapping.html, Q1 2026-09-12): the press snaps to
// any vertex under the cursor (pixels); the stroke then lives on the camera-facing
// plane THROUGH THAT VERTEX (through the camera pivot when nothing snapped), so a curve
// started on a vertex stays flat at that vertex's depth. The END NEVER SNAPS: it is
// always a new vertex on the plane (Khoa 2026-09-12: "I don't need end snapping"; the
// old screen-only end snap was what made prim 48 curve in x from profile). Join curves
// end-to-end by starting the next one on the previous end vertex.
//
// Desktop deviations from the tablet: the tablet snaps both ends by a world radius
// around the pen point on the pivot plane; handles here are stored as absolute
// bone-space points, so the fitted p1/p2 are written directly (no chord-offset
// conversion, no plane swing). Pixel-level entry points are shared by the real mouse
// and the debug channel's virtual mouse (`line_tool 1` + mouse_down/move/up).

global v1 const line_tool_snap_px = 12.f;
global v1 const line_tool_tap_px  = 3.f;   // press->release travel below this = a tap

function Bone_ID
line_tool_group_bone(Recording &doc, i32 group_index)
{
 return doc.groups[group_index].bone_id;
}

function v3
line_tool_world_to_bone(Bone_ID bone_id, v3 world)
{// NOTE(kv) Left pass only: the tool draws left-side (or centered) geometry.
 return mat4vert(get_bone(bone_id, false)->world_from_bone.inverse, world);
}

function v3
line_tool_plane_point(Game_State *state, Live_Viewport *viewport, v2 px)
{// NOTE(kv) Mouse -> the stroke plane (camera-facing, at the depth chosen at press:
 // the snapped start vertex's, else the pivot's -- tablet pen_point_on_camera_plane);
 // the depth is frozen at press so a camera move mid-drag can't tilt the stroke.
 Screen_Projection_Data proj = mk_screen_projection_data(state, viewport);
 return unproject(proj, px, state->line_tool.plane_cam_z);
}

// NOTE(kv) Nearest table vertex to the mouse within line_tool_snap_px, found through
// the primitives that reference it (a vertex's bone may be "the group's", which only
// a primitive knows). Skips the curve being drawn and its temp end vertex. Any depth
// counts: this is the start snap (the end never snaps).
function i32
line_tool_snap_vertex(Game_State *state, Live_Viewport *viewport, v2 mouse_px, v3 *world_out)
{
 Line_Tool_State &tool = state->line_tool;
 Recording &doc = state->model.recordings.document;
 Screen_Projection_Data proj = mk_screen_projection_data(state, viewport);
 i32 best = -1;
 v1 best_dist = line_tool_snap_px;
 for_i32(iprim, 0, doc.primitives.count)
 {
  if(tool.created and iprim == tool.prim_index){ continue; }
  Recorded_Primitive &prim = doc.primitives[iprim];
  for_i32(slot, 0, primitive_vertex_count(prim.type))
  {
   i32 vertex_index = prim.vertex_index[slot];
   if(tool.created and vertex_index == tool.temp_end_vertex){ continue; }
   Document_Pick pick = {iprim, false, false, slot};
   v3 world = document_pick_world_pos(doc, pick);
   v2 px = project(proj, world);
   v1 dist = lengthof(V3(px - mouse_px, 0));
   if(dist < best_dist){ best_dist = dist; best = vertex_index; *world_out = world; }
  }
 }
 return best;
}

function i32
line_tool_pick_group(Game_State *state, i32 start_snap)
{// NOTE(kv) Which group (paint params + bone) the new curve joins
 // (plan-line-tool-wrong-group), first usable of: the group of a curve already ending on
 // the snapped start vertex, the selected primitive's group (the cheap "current group"),
 // the last curve's group, the first group. Usable = painting and its vis tag is live (a
 // Vis_Level1 guide group swallowed the new curve invisibly, found 2026-09-12; a camera
 // condition is NOT checked: you continue what you see) AND, when
 // the start snapped, it sits on the bone that vertex is drawn in: a table vertex
 // without a bone of its own is read in the READER's group bone, so a group on another
 // bone would re-read the shared vertex somewhere else (2026-09-20: a nose line landed in
 // the iris group = eyeball bone, start and end flew off). -1 = no usable group.
 Recording &doc = state->model.recordings.document;
 b32 check_bone = (start_snap >= 0 and doc.vertices[start_snap].bone.type == Bone_None);
 Bone_ID snap_bone = check_bone ? document_vertex_index_bone(doc, start_snap) : Bone_ID{};
 auto usable = [&](i32 group_index) -> b32
 {
  Recorded_Group &group = doc.groups[group_index];
  if(HasFlag(group.params.line.flags, Line_Invisible)){ return false; }
  if(check_bone and not (group.bone_id == snap_bone)){ return false; }
  return (group.params.painting and state->model.vis_live[group.vis_tag]);
 };
 if(start_snap >= 0)
 {
  for_i32(iprim, 0, doc.primitives.count)
  {
   Recorded_Primitive &prim = doc.primitives[iprim];
   if(prim.type != Primitive_Type_Curve){ continue; }
   if((prim.vertex_index[0] == start_snap or prim.vertex_index[1] == start_snap) and
      usable(prim.group_index))
   { return prim.group_index; }
  }
 }
 Document_Selection &sel = state->document_selection;
 for_i32(i, 0, sel.count)
 {
  i32 iprim = sel.prim_index[i];
  if(iprim >= 0 and iprim < doc.primitives.count and usable(doc.primitives[iprim].group_index))
  { return doc.primitives[iprim].group_index; }
 }
 for(i32 iprim = doc.primitives.count-1; iprim >= 0; iprim--)
 {
  Recorded_Primitive &prim = doc.primitives[iprim];
  if(prim.type == Primitive_Type_Curve and usable(prim.group_index)){ return prim.group_index; }
 }
 for_i32(igroup, 0, doc.groups.count)
 {
  if(usable(igroup)){ return igroup; }
 }
 return -1;
}

// NOTE(kv) Least-squares fit of the two interior control points to the pen path
// (endpoints fixed, chord-length parameterization) -- tablet fit_stroke_handles minus
// the offset-handle conversion. Straight-line thirds when the path is too short or the
// normal equations are degenerate.
function void
line_tool_fit_handles(v3 *path, i32 path_count, v3 p0, v3 p3, v3 *p1_out, v3 *p2_out)
{
 *p1_out = (2.f*p0 + p3) / 3.f;
 *p2_out = (p0 + 2.f*p3) / 3.f;
 if(path_count < 3){ return; }

 Scratch_Block scratch;
 v1 *parameters = push_array(scratch, v1, path_count);
 parameters[0] = 0;
 v1 total_length = 0;
 for_i32(i, 1, path_count)
 {
  total_length += lengthof(path[i] - path[i-1]);
  parameters[i] = total_length;
 }
 if(total_length < 1e-9f){ return; }
 for_i32(i, 0, path_count){ parameters[i] /= total_length; }

 // NOTE(kv) min sum |q_i - (B0 p0 + B1 p1 + B2 p2 + B3 p3)|^2 over p1, p2: a 2x2
 // system with vector right-hand sides.
 v1 a11 = 0, a12 = 0, a22 = 0;
 v3 c1 = {}, c2 = {};
 for_i32(i, 0, path_count)
 {
  v1 t = parameters[i];
  v1 s = 1.f - t;
  v1 b0 = s*s*s, b1 = 3.f*s*s*t, b2 = 3.f*s*t*t, b3 = t*t*t;
  v3 target = path[i] - b0*p0 - b3*p3;
  a11 += b1*b1;
  a12 += b1*b2;
  a22 += b2*b2;
  c1 += b1*target;
  c2 += b2*target;
 }
 v1 determinant = a11*a22 - a12*a12;
 // NOTE(kv) The endpoints contribute nothing (b1 = b2 = 0 there), so with a single
 // interior sample the system is rank one: the determinant is only float noise, and an
 // absolute epsilon let it through (handles flew off the stroke plane). Compare it to
 // the system's own scale instead.
 if(absolute(determinant) < 1e-6f * a11*a22){ return; }
 *p1_out = (a22*c1 - a12*c2) / determinant;
 *p2_out = (a11*c2 - a12*c1) / determinant;
}

function void
line_tool_reset(Game_State *state)
{// NOTE(kv) Disarm + forget the drag; the document is left as it is.
 state->line_tool = {};
}

function void
line_tool_press(Game_State *state, Live_Viewport *viewport, v2 mouse_px)
{
 Line_Tool_State &tool = state->line_tool;
 if(not tool.armed or not viewport){ return; }
 Camera camera = setup_camera(state->viewports[0].camera);
 tool.plane_cam_z = mat4vert(camera.cam_from_world, state->viewports[0].camera.pivot).z;
 if(tool.plane_cam_z >= -1e-6f)
 {// NOTE(kv) Pivot behind the eye: nothing sane to draw on.
  log_error("line_tool: camera pivot is not in front of the camera, press ignored");
  return;
 }
 v3 snap_world = {};
 i32 snap = line_tool_snap_vertex(state, viewport, mouse_px, &snap_world);
 if(snap >= 0)
 {// NOTE(kv) The stroke plane moves to the snapped vertex's depth (Q1: a curve started
  // on a vertex stays flat at that vertex's depth, whatever the pivot).
  tool.plane_cam_z = mat4vert(camera.cam_from_world, snap_world).z;
 }
 v3 plane_point = line_tool_plane_point(state, viewport, mouse_px);
 tool.active      = true;
 tool.created     = false;
 tool.press_px    = mouse_px;
 tool.start_snap  = snap;
 tool.start_world = (snap >= 0) ? snap_world : plane_point;
 tool.end_world   = tool.start_world;
 tool.path_count  = 0;
 tool.path[tool.path_count++] = plane_point;
}

function void
line_tool_create_curve(Game_State *state)
{// NOTE(kv) First real move: the curve becomes a document primitive so replay draws it.
 // New vertices are Bone_None (= the group's bone), positions in that bone's space.
 Line_Tool_State &tool = state->line_tool;
 Recording &doc = state->model.recordings.document;
 i32 group_index = line_tool_pick_group(state, tool.start_snap);
 if(group_index < 0)
 {
  log_error("line_tool: no visible group on the start vertex's bone to add a curve to");
  Document_History &history = state->document_history;
  snprintf(history.status, sizeof(history.status), "line tool: no group on this vertex's bone");
  history.status_frames = 120;
  line_tool_reset(state);
  return;
 }
 Document_Action action = {};
 action.kind       = Document_Action_Add_Line;
 action.prim_index = doc.primitives.count;
 history_begin(state, action);

 Bone_ID bone_id = line_tool_group_bone(doc, group_index);
 i32 start_vertex = tool.start_snap;
 if(start_vertex < 0)
 {
  Recorded_Vertex vertex = {};
  vertex.p = line_tool_world_to_bone(bone_id, tool.start_world);
  start_vertex = doc.vertices.count;
  push(&doc.vertices, vertex);
 }
 {
  Recorded_Vertex vertex = {};
  vertex.p = line_tool_world_to_bone(bone_id, tool.end_world);
  tool.temp_end_vertex = doc.vertices.count;
  push(&doc.vertices, vertex);
 }

 Recorded_Primitive prim = {};
 prim.type = Primitive_Type_Curve;
 prim.group_index = group_index;
 prim.vertex_index[0] = start_vertex;
 prim.vertex_index[1] = tool.temp_end_vertex;
 {// NOTE(kv) Style template: the last curve of the same group (width profile etc.),
  // else a flat unit profile.
  prim.curve.radii = V4(1,1,1,1);
  for(i32 iprim = doc.primitives.count-1; iprim >= 0; iprim--)
  {
   Recorded_Primitive &other = doc.primitives[iprim];
   if(other.type == Primitive_Type_Curve and other.group_index == group_index)
   {
    prim.curve.radii = other.curve.radii;
    prim.curve.lightness_additions = other.curve.lightness_additions;
    break;
   }
  }
 }
 if(tool.start_snap >= 0 and doc.vertices[tool.start_snap].p.x == 0)
 {// NOTE(kv) plan-focus-radii-midline Q9: continuing from a vertex that sits on the
  // mirror plane (a pinned midline vertex has exactly x=0) keeps the new curve there.
  prim.curve.midline = true;
 }
 tool.prim_index  = doc.primitives.count;
 tool.group_index = group_index;
 push(&doc.primitives, prim);
 doc.captured = true;
 tool.created = true;
}

function void
line_tool_move(Game_State *state, Live_Viewport *viewport, v2 mouse_px)
{
 Line_Tool_State &tool = state->line_tool;
 Recording &doc = state->model.recordings.document;
 if(not tool.active or not viewport){ return; }
 if(not tool.created)
 {
  if(lengthof(V3(mouse_px - tool.press_px, 0)) < line_tool_tap_px){ return; }
  line_tool_create_curve(state);
  if(not tool.created){ return; }  // no group: reset happened
 }
 v3 plane_point = line_tool_plane_point(state, viewport, mouse_px);
 // NOTE(kv) The move runs every frame while the button is held, so a resting mouse
 // must not flood the path with copies of one sample (it filled the cap in seconds).
 b32 same_as_last = (tool.path_count > 0 and
                     length_squared(plane_point - tool.path[tool.path_count-1]) < 1e-12f);
 if(not same_as_last and tool.path_count < LINE_TOOL_PATH_CAP){ tool.path[tool.path_count++] = plane_point; }

 // NOTE(kv) No end snap: the end vertex follows the pen on the stroke plane.
 tool.end_world = plane_point;

 Recorded_Primitive &prim = doc.primitives[tool.prim_index];
 Bone_ID bone_id = line_tool_group_bone(doc, tool.group_index);
 doc.vertices[tool.temp_end_vertex].p = line_tool_world_to_bone(bone_id, tool.end_world);
 // NOTE(kv) Path and both endpoints lie on the stroke plane (the start defines the
 // plane, the end is unprojected onto it), so the raw 3D fit stays flat.
 v3 p1, p2;
 line_tool_fit_handles(tool.path, tool.path_count, tool.start_world, tool.end_world, &p1, &p2);
 // NOTE(kv) plan-curve-chord-handles: stored as offsets from the chord thirds (Bone_None
 // = the group bone, like the two table vertices the tool creates).
 document_curve_set_handle_point(doc, prim, 0, {.v = line_tool_world_to_bone(bone_id, p1)});
 document_curve_set_handle_point(doc, prim, 1, {.v = line_tool_world_to_bone(bone_id, p2)});
 document_curve_apply_midline(doc, tool.prim_index);
}

function void
line_tool_release(Game_State *state)
{
 Line_Tool_State &tool = state->line_tool;
 if(not tool.active){ return; }
 if(not tool.created)
 {// NOTE(kv) A tap: exit the tool (tablet Q27).
  line_tool_reset(state);
  return;
 }
 // NOTE(kv) The end never snaps, so the temp end vertex is always the real one and
 // the curve can't collapse onto its start vertex (a tap never gets here: `created`).
 history_commit(state);
 save_document_file(state);
 line_tool_reset(state);
}
