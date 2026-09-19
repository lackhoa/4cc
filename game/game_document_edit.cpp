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

function b32
camera_is_orthographic(b32 global_ortho, b32 show_grid)
{// NOTE(kv) plan-screen-projection-unification Q5: the ONE ortho predicate, shared by
 // render (get_clip_from_world), pick, and document drag. Ortho when the user asks for it
 // globally, or whenever the grid is shown (the grid only reads right axis-on, so we lock
 // ortho with it regardless of camera angle). Was copy-pasted; now one home.
 return (global_ortho or show_grid);
}

// NOTE(kv) plan-screen-projection-unification: THE one world<->window-pixel mapping for a
// view, shared by pick and document drag (render keeps its own GPU mat4, sharing only the
// ortho predicate above). Perspective and ortho differ only in a denominator: per-point
// depth -cam.z vs the constant eye distance ortho_d = |camera world pos| (the same d the
// render's ortho matrix uses in get_clip_from_camera). Always built via
// mk_screen_projection_data; never brace-initialized ad hoc (ortho_d must be filled).
struct Screen_Projection_Data
{
 Camera camera;
 v2     center;        // viewport center, window px
 b32    orthographic;
 v1     ortho_d;       // eye distance; only meaningful when orthographic
};

struct Screen_Ray { v3 P; v3 dir; };  // camera-space pick ray

function Screen_Projection_Data
mk_screen_projection_data(Game_State *state, v2 center)
{// NOTE(kv) Q3: behavior-preserving -- camera + show_grid from viewports[0] (Q9 defers
 // the multi-viewport camera/center mismatch); `center` = the view's center in window px
 // (a live viewport's clip box below, or the debug channel's virtual-mouse box).
 Screen_Projection_Data proj = {};
 proj.camera = setup_camera(state->viewports[0].camera);
 proj.center = center;
 b32 show_grid = state->model.recordings.preset_settings[state->viewports[0].preset].show_grid;
 proj.orthographic = camera_is_orthographic(state->user_wants_orthographic, show_grid);
 proj.ortho_d = proj.orthographic ? lengthof(camera_world_position(proj.camera)) : 0.f;
 return proj;
}

function Screen_Projection_Data
mk_screen_projection_data(Game_State *state, Live_Viewport *viewport)
{// NOTE(kv) The usual entry point (pick, drag): center from the live viewport's clip box.
 return mk_screen_projection_data(state, get_center(viewport->clip_box));
}

function b32
px_from_camera(Screen_Projection_Data const &proj, v3 cam, v2 *out_offset)
{// NOTE(kv) Camera-space point -> px offset from the viewport center (screen y down).
 // Always writes *out_offset; returns false for points at/behind the eye in perspective
 // (unpickable) -- callers that only project visible points ignore the bool.
 // NOTE(kv) Same isotropic scale render uses (see default_meter_to_pixel), so pick lands
 // exactly where render drew. Perspective divides by depth -cam.z; ortho by the constant eye
 // distance ortho_d, so x/y stay parallel (independent of depth).
 v1 denom = proj.orthographic ? proj.ortho_d : -cam.z;
 v1 s = default_meter_to_pixel * proj.camera.focal_length / denom;
 *out_offset = V2(cam.x, -cam.y) * s;
 return (proj.orthographic or (cam.z < -1e-6f));
}

function v3
camera_from_px(Screen_Projection_Data const &proj, v2 px_offset, v1 cam_z)
{// NOTE(kv) Inverse of px_from_camera: px offset from center -> camera point at depth
 // cam_z. Ortho x/y scale is the constant ortho_d/focal (parallel, independent of cam_z).
 v2 meter = px_offset / default_meter_to_pixel;
 v1 scale = (proj.orthographic ? proj.ortho_d : -cam_z) / proj.camera.focal_length;
 return V3(meter.x * scale, -meter.y * scale, cam_z);
}

function v2
project(Screen_Projection_Data const &proj, v3 world)
{// NOTE(kv) World -> absolute window px.
 v3 cam = mat4vert(proj.camera.cam_from_world, world);
 v2 offset = {};
 px_from_camera(proj, cam, &offset);
 return offset + proj.center;
}

function v3
unproject(Screen_Projection_Data const &proj, v2 px, v1 cam_z)
{// NOTE(kv) Window px -> world point on the camera plane at depth cam_z (< 0).
 v3 cam = camera_from_px(proj, px - proj.center, cam_z);
 return mat4vert(proj.camera.world_from_cam, cam);
}

function Screen_Ray
screen_ray(Screen_Projection_Data const &proj, v2 mouse_offset)
{// NOTE(kv) px offset from center -> camera-space pick ray, the inverse of px_from_camera.
 // Perspective: diverges from the eye at the origin through the mouse point. Ortho: rays
 // are parallel along camera -z; the mouse offset selects which parallel line.
 Screen_Ray ray = {};
 v2 meter = mouse_offset / default_meter_to_pixel;
 v3 mouse_cam = V3(meter.x, -meter.y, -proj.camera.focal_length);
 if(proj.orthographic)
 {
  v1 k = proj.ortho_d / proj.camera.focal_length;
  ray.P   = V3(mouse_cam.x * k, mouse_cam.y * k, 0.f);
  ray.dir = V3(0.f, 0.f, -1.f);
 }
 else
 {
  ray.P   = V3();
  ray.dir = noz(mouse_cam);
 }
 return ray;
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

inline i32
document_pick_vertex_index(Recording &doc, Document_Pick pick)
{// NOTE(kv) The TABLE index of a vertex pick. Not for handles: their `slot` is an e-index.
 kv_assert(not pick.is_handle);
 return doc.primitives[pick.prim_index].vertex_index[pick.slot];
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
  Recorded_Vertex &vertex = doc.vertices[document_pick_vertex_index(doc, pick)];
  bone_p = vertex.p;
  bone_id = document_vertex_bone(doc, prim, vertex);
 }
 return mat4vert(get_bone(bone_id, pick.is_right)->world_from_bone, bone_p);
}

//~ NOTE(kv) plan-native-curve-split: cut a curve into two curves that reproduce the
// original exactly (de Casteljau), plus the "split mode" preview that lets the user aim
// the cut with a live marker (the mouse is not precise). Port of the tablet's
// split_stroke (tablet/src/document.ts) minus the smooth-knot record (native has none
// yet -- the join is smooth AT the cut, later handle edits may break it, Q1).
global v1 const split_min_t_from_ends = 0.02f;  // Q6: a cut this close to an end is a no-op
global v1 const split_tool_reach_px   = 80.f;   // Q8: a click beyond this from the curve cancels

function b32
document_curve_bounds_patch(Recording &doc, i32 curve_prim_index)
{// NOTE(kv) Q5: true if any curve-patch primitive is built from this curve. Splitting it
 // would leave the patch pointing at half the shape, so we refuse the split instead.
 for_i32(iprim, 0, doc.primitives.count)
 {
  Recorded_Primitive &prim = doc.primitives[iprim];
  if(prim.type != Primitive_Type_Curve_Patch){ continue; }
  Recorded_Curve_Patch &patch = prim.curve_patch;
  for_i32(ic, 0, patch.curve_count)
  {
   if(patch.curve_index[ic] == curve_prim_index){ return true; }
  }
 }
 return false;
}

template<class T> myinline void
decasteljau_split_4(T p0, T p1, T p2, T p3, v1 t, T out_left[4], T out_right[4])
{// NOTE(kv) Q13: split a 4-control-point cubic at t into its [0,t] and [t,1] halves, the
 // shared endpoint being the value AT t. The geometry, `radii`, `lightness_additions`,
 // `dradii` (v4 read as 4 scalars) and `dbezier` (v3[4]) are every one a cubic bezier over
 // the curve's own t -- render samples them with bezier_sample(...,t) and apply_shape_key
 // blends rest + w*delta linearly -- so the same de Casteljau splits them all, and because
 // it is linear it commutes with the blend: the two halves reproduce the original at every
 // shape-key weight, width and lightness included.
 T p01 = lerp(p0, t, p1), p12 = lerp(p1, t, p2), p23 = lerp(p2, t, p3);
 T p012 = lerp(p01, t, p12), p123 = lerp(p12, t, p23);
 T knot = lerp(p012, t, p123);
 out_left[0]  = p0;   out_left[1]  = p01;  out_left[2]  = p012; out_left[3]  = knot;
 out_right[0] = knot; out_right[1] = p123; out_right[2] = p23;  out_right[3] = p3;
}

// NOTE(kv) Q13: split the v4 profile (its 4 components are the cubic's control points).
function void
document_split_curve_profile_v4(v4 profile, v1 t, v4 *left, v4 *right)
{
 v1 lo[4], hi[4];
 decasteljau_split_4<v1>(profile[0], profile[1], profile[2], profile[3], t, lo, hi);
 *left  = V4(lo[0], lo[1], lo[2], lo[3]);
 *right = V4(hi[0], hi[1], hi[2], hi[3]);
}

function b32
document_split_curve(Game_State *state, i32 prim_index, v1 t)
{// NOTE(kv) Cut curve `prim_index` into two at parameter t via de Casteljau (tablet
 // split_stroke): the halves reproduce the original exactly and share a new welded knot
 // vertex (Q3), so the join is smooth at the cut. The original keeps [0,t]; a new primitive
 // takes [t,1], copies the style (group/flags/straight/midline/key) but SPLITS the profiles
 // that vary along the curve -- radii, lightness, and the shape-key deltas -- so the width
 // and lightness reproduce too (Q4/Q13/Q14). Opens ONE history entry; the caller commits +
 // saves. Returns false with NO history opened for a cut too near an end (Q6) or a curve
 // that bounds a patch (Q5).
 Recording &doc = state->model.recordings.document;
 if(prim_index < 0 or prim_index >= doc.primitives.count){ return false; }
 if(doc.primitives[prim_index].type != Primitive_Type_Curve){ return false; }
 if(t < split_min_t_from_ends or t > 1.f - split_min_t_from_ends){ return false; }
 if(document_curve_bounds_patch(doc, prim_index))
 {
  log_error("split: curve %d bounds a patch, not splitting", prim_index);
  return false;
 }

 // NOTE(kv) All four control points live in the group's bone space (Bone_None, like the
 // line tool), so de Casteljau runs in that one space and the halves' handle points feed
 // straight back through document_curve_set_handle_point.
 v3 P0 = document_curve_endpoint(doc, doc.primitives[prim_index], 0).v;
 v3 P1 = document_curve_handle_point(doc, doc.primitives[prim_index], 0).v;
 v3 P2 = document_curve_handle_point(doc, doc.primitives[prim_index], 1).v;
 v3 P3 = document_curve_endpoint(doc, doc.primitives[prim_index], 1).v;
 v3 p01 = lerp(P0, t, P1), p12 = lerp(P1, t, P2), p23 = lerp(P2, t, P3);
 v3 p012 = lerp(p01, t, p12), p123 = lerp(p12, t, p23);
 v3 knot = lerp(p012, t, p123);

 // NOTE(kv) Q13/Q14: split every per-curve profile at the same t (read the source by value
 // now, before the push below can realloc). radii/lightness/dradii are v4 = 4 scalar control
 // points; dbezier is v3[4]. The shared knot value lands on both halves (radii_left[3] ==
 // radii_right[0] etc.), so the width, lightness and blink motion are smooth across the cut.
 Recorded_Curve const src = doc.primitives[prim_index].curve;
 v4 radii_left, radii_right, light_left, light_right, dradii_left, dradii_right;
 document_split_curve_profile_v4(src.radii,               t, &radii_left,  &radii_right);
 document_split_curve_profile_v4(src.lightness_additions, t, &light_left,  &light_right);
 document_split_curve_profile_v4(src.dradii,              t, &dradii_left, &dradii_right);
 if(src.straight)
 {// NOTE(kv) Q14: a straight curve renders a UNIFORM width radii[1], so de Casteljau-ing the
  // radii would change it. Keep radii/dradii unchanged on both halves (geometry + lightness
  // still split); the two collinear segments keep the original constant width.
  radii_left  = radii_right  = src.radii;
  dradii_left = dradii_right = src.dradii;
 }
 v3 dbez_left[4], dbez_right[4];
 decasteljau_split_4<v3>(src.dbezier[0], src.dbezier[1], src.dbezier[2], src.dbezier[3],
                         t, dbez_left, dbez_right);

 {
  Document_Action action = {};
  action.kind       = Document_Action_Split_Curve;
  action.prim_index = prim_index;
  history_begin(state, action);
 }

 // NOTE(kv) The welded knot: a new table vertex both halves reference. Bone_None, position
 // in the group bone's space -- the same convention as the endpoints.
 i32 knot_index = doc.vertices.count;
 {
  Recorded_Vertex vertex = {};
  vertex.p = knot;
  push(&doc.vertices, vertex);
 }

 // NOTE(kv) The push above may realloc, so re-index doc.primitives every time below. The
 // second half copies the original by value (styling, group, flags) before we rewrite it.
 i32 far_vertex = doc.primitives[prim_index].vertex_index[1];
 Recorded_Primitive second = doc.primitives[prim_index];
 second.vertex_index[0] = knot_index;
 second.vertex_index[1] = far_vertex;
 second.curve.radii               = radii_right;
 second.curve.lightness_additions = light_right;
 second.curve.dradii              = dradii_right;
 for_i32(i, 0, 4){ second.curve.dbezier[i] = dbez_right[i]; }

 {// NOTE(kv) Original -> the [0,t] half: its far end moves to the knot, handles = p01/p012.
  Recorded_Primitive &orig = doc.primitives[prim_index];
  orig.vertex_index[1] = knot_index;
  document_curve_set_handle_point(doc, orig, 0, {.v = p01});
  document_curve_set_handle_point(doc, orig, 1, {.v = p012});
  orig.curve.radii               = radii_left;
  orig.curve.lightness_additions = light_left;
  orig.curve.dradii              = dradii_left;
  for_i32(i, 0, 4){ orig.curve.dbezier[i] = dbez_left[i]; }
 }

 i32 second_index = doc.primitives.count;
 push(&doc.primitives, second);
 {// NOTE(kv) New primitive -> the [t,1] half: handles = p123/p23.
  Recorded_Primitive &second_ref = doc.primitives[second_index];
  document_curve_set_handle_point(doc, second_ref, 0, {.v = p123});
  document_curve_set_handle_point(doc, second_ref, 1, {.v = p23});
 }

 doc.captured = true;
 return true;
}

function b32
document_nearest_t_on_curve(Game_State *state, Live_Viewport *viewport, i32 prim_index,
                            v2 mouse_px, v1 *t_out, v3 *world_out, v1 *dist_px_out)
{// NOTE(kv) Screen-space nearest point on a curve to the cursor (tablet
 // nearest_t_on_stroke_screen): sample the world bezier, project with the shared pick
 // mapping (project == the renderer's), keep the nearest projected sample, then refine
 // around it. Returns the parameter, its world position, and the pixel distance. False
 // only when the primitive isn't a live curve.
 Recording &doc = state->model.recordings.document;
 if(not viewport){ return false; }
 if(prim_index < 0 or prim_index >= doc.primitives.count){ return false; }
 if(doc.primitives[prim_index].type != Primitive_Type_Curve){ return false; }
 Screen_Projection_Data proj = mk_screen_projection_data(state, viewport);
 v3 P[4];
 P[0] = document_pick_world_pos(doc, {prim_index, false, false, 0});
 P[1] = document_pick_world_pos(doc, {prim_index, false, true,  1});
 P[2] = document_pick_world_pos(doc, {prim_index, false, true,  2});
 P[3] = document_pick_world_pos(doc, {prim_index, false, false, 1});

 v1 best_t = 0, best_dist = INFINITY;
 const i32 samples = 128;
 for_i32(i, 0, samples+1)
 {
  v1 t = v1(i) / v1(samples);
  v2 px = project(proj, bezier_sample(P, t));
  v1 dist = lengthof(V3(px - mouse_px, 0));
  if(dist < best_dist){ best_dist = dist; best_t = t; }
 }
 // NOTE(kv) Refine: shrink a window around the best sample a few times.
 v1 span = 1.f / v1(samples);
 for_i32(iter, 0, 3)
 {
  v1 lo = best_t - span; if(lo < 0.f){ lo = 0.f; }
  v1 hi = best_t + span; if(hi > 1.f){ hi = 1.f; }
  const i32 refine = 16;
  for_i32(i, 0, refine+1)
  {
   v1 t = lerp(lo, v1(i) / v1(refine), hi);
   v2 px = project(proj, bezier_sample(P, t));
   v1 dist = lengthof(V3(px - mouse_px, 0));
   if(dist < best_dist){ best_dist = dist; best_t = t; }
  }
  span /= v1(refine);
 }
 *t_out       = best_t;
 *world_out   = bezier_sample(P, best_t);
 *dist_px_out = best_dist;
 return true;
}

function void
split_tool_reset(Game_State *state)
{// NOTE(kv) Disarm split mode; the document is left as it is.
 state->split_tool = {};
}

function void
split_tool_update(Game_State *state, Live_Viewport *viewport, v2 mouse_px)
{// NOTE(kv) Per-frame while armed: ride the marker to the curve point nearest the cursor
 // and decide whether a click here would split (Q2/Q10/Q12).
 Split_Tool_State &split = state->split_tool;
 if(not split.armed){ return; }
 split.on_curve      = false;
 split.preview_valid = false;
 v1 t = 0, dist = 0; v3 world = {};
 if(document_nearest_t_on_curve(state, viewport, split.prim_index, mouse_px, &t, &world, &dist))
 {
  b32 in_guard = (t < split_min_t_from_ends or t > 1.f - split_min_t_from_ends);
  split.on_curve      = (dist <= split_tool_reach_px);
  split.preview_valid = (split.on_curve and not in_guard);
  split.preview_t     = t;
  split.preview_world = world;
 }
}

function void
split_tool_draw(Game_State *state, Camera &camera)
{// NOTE(kv) The split marker: one camera-facing disk on the curve at the preview point,
 // overlaid (Q9), drawn like the hover control-point disks but larger and in hot_color2.
 Split_Tool_State &split = state->split_tool;
 if(not split.armed or not split.preview_valid){ return; }
 v3 center = split.preview_world;
 v1 dist = lengthof(mat4vert(camera.cam_from_world, center));
 v1 radius = 5.f*millimeter * dist / camera.focal_length;
 const i32 nslice = 16;
 v3 last = {};
 for_i32(k, 0, nslice+1)
 {
  v2 arm = radius*arm2(v1(k) / v1(nslice));
  v3 sample = center + arm.x*camera.x + arm.y*camera.y;
  if(k != 0)
  {
   v3 points[3] = {center, last, sample};
   poly3_inner(mk_poly3(points), repeat3(hot_color2), {Poly_Overlay});
  }
  last = sample;
 }
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
// NOTE(kv) 2026-09-19 (plan-point-primitive Q11): table VERTICES are now grabbable on every
// visible primitive, selected or not (document_pick_nearest); HANDLES are still
// selection-only. At a shared vertex the selected curve still wins.
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
  i32 side_count = (one_sided or not document_pick_right_side) ? 1 : 2;
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
 // highlight shows it. False when there is no candidate at all (was: nothing selected;
 // since Q11 the vertices of unselected visible primitives are candidates too).
 Recording &doc = state->model.recordings.document;
 if(not viewport){ return false; }
 Document_Pick picks[document_selection_pick_cap];
 i32 pick_count = document_selection_pick_list(state, picks, ArrayCount(picks));
 Screen_Projection_Data proj = mk_screen_projection_data(state, viewport);
 v1 best_dist = INFINITY;
 for_i32(i, 0, pick_count)
 {
  v2 px = project(proj, document_pick_world_pos(doc, picks[i]));
  v1 dist = lengthof(V3(px - mouse_px, 0));
  if(dist < best_dist)
  {
   best_dist = dist;
   *best_out = picks[i];
  }
 }
 {// NOTE(kv) plan-point-primitive Q11 (direct vertex pick): the table vertices of every
  // VISIBLE primitive are candidates too, selected or not -- a pick through an owning
  // primitive, so bone + mirror side come from its group like any other pick. Handles stay
  // selection-only (they are per-curve offsets). Visibility = the hit-test's is_pickable.
  // The selection went first and `<` is strict, so at a shared vertex the selected curve
  // still wins (the 2026-09-13 rule).
  Model *m = &state->model;
  for_i32(iprim, 0, doc.primitives.count * (doc.captured ? 1 : 0))
  {
   Recorded_Primitive &prim = doc.primitives[iprim];
   Recorded_Group &group = doc.groups[prim.group_index];
   if(not group.params.painting or not m->vis_live[group.vis_tag]){ continue; }
   if(prim.type == Primitive_Type_Curve and
      HasFlag(group.params.line.flags, Line_Invisible)){ continue; }
   b32 one_sided = (group.one_sided or (prim.type == Primitive_Type_Curve and prim.curve.midline));
   i32 side_count = (one_sided or not document_pick_right_side) ? 1 : 2;
   for_i32(side, 0, side_count)
   {
    for_i32(slot, 0, primitive_vertex_count(prim.type))
    {
     Document_Pick candidate = {};
     candidate.prim_index = iprim;
     candidate.is_right   = (side == 1);
     candidate.is_handle  = false;
     candidate.slot       = slot;
     v2 px = project(proj, document_pick_world_pos(doc, candidate));
     v1 dist = lengthof(V3(px - mouse_px, 0));
     if(dist < best_dist)
     {
      best_dist = dist;
      *best_out = candidate;
     }
    }
   }
  }
 }
 *dist_out = best_dist;
 return (best_dist < INFINITY);
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
                 document_pick_vertex_index(doc, pick), strexpand(group_name), pick.slot, pick.prim_index);
}
function void
document_hover_draw_disk(Camera &camera, v3 center, v1 radius_mm, argb color)
{
 v1 dist = lengthof(mat4vert(camera.cam_from_world, center));
 v1 radius = radius_mm*millimeter * dist / camera.focal_length;
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
function Bone_ID document_vertex_index_bone(Recording &doc, i32 vertex_index);
function b32 document_selection_contains(Game_State *state, i32 prim_index);
function v3 document_vertex_index_world_pos(Recording &doc, i32 vertex_index, b32 is_right);
function void
document_points_draw(Game_State *state, Camera &camera)
{// NOTE(kv) plan-point-primitive Q3: every point primitive of a visible group draws as a
 // camera-facing diamond of fixed screen size, overlaid; brighter when hot or selected.
 // Edit marker only -- replay draws nothing for a point.
 Model *m = &state->model;
 Recording &doc = m->recordings.document;
 if(not doc.captured){ return; }
 for_i32(iprim, 0, doc.primitives.count)
 {
  Recorded_Primitive &prim = doc.primitives[iprim];
  if(prim.type != Primitive_Type_Point){ continue; }
  Recorded_Group &group = doc.groups[prim.group_index];
  if(not group.params.painting or not m->vis_live[group.vis_tag]){ continue; }
  b32 is_hot = false;
  for_i32(ihot, 0, painter->hot_locations.count)
  {
   Location hot = painter->hot_locations[ihot];
   if(is_document_location(hot) and document_primitive_index(hot) == iprim){ is_hot = true; }
  }
  b32 bright = (is_hot or document_selection_contains(state, iprim));
  argb color = bright ? hot_color2 : linear_argb_silver;
  i32 side_count = group.one_sided ? 1 : 2;
  for_i32(side, 0, side_count)
  {
   v3 center = document_vertex_index_world_pos(doc, prim.vertex_index[0], side == 1);
   v1 dist = lengthof(mat4vert(camera.cam_from_world, center));
   v1 radius = (bright ? 5.f : 4.f)*millimeter * dist / camera.focal_length;
   v3 x = radius*camera.x;
   v3 y = radius*camera.y;
   v3 upper[3] = {center - x, center + y, center + x};
   v3 lower[3] = {center - x, center - y, center + x};
   poly3_inner(mk_poly3(upper), repeat3(color), {Poly_Overlay});
   poly3_inner(mk_poly3(lower), repeat3(color), {Poly_Overlay});
  }
 }
}
function v3
document_vertex_index_world_pos(Recording &doc, i32 vertex_index, b32 is_right)
{// NOTE(kv) plan-vertex-links: a table vertex without a primitive in hand.
 Bone_ID bone_id = document_vertex_index_bone(doc, vertex_index);
 return mat4vert(get_bone(bone_id, is_right)->world_from_bone, doc.vertices[vertex_index].p);
}
function void
document_hover_draw(Game_State *state, Camera &camera)
{// NOTE(kv) World-space disks facing the camera, overlaid (they mark positions, depth
 // would hide the ones behind a fill). Same depth-scaled sizing as the kb cursor.
 if(not document_hover_valid){ return; }
 Recording &doc = state->model.recordings.document;
 Document_Pick picks[document_selection_pick_cap];
 i32 pick_count = document_selection_pick_list(state, picks, ArrayCount(picks));
 b32 hovered_drawn = false;
 for_i32(i, 0, pick_count)
 {
  Document_Pick &pick = picks[i];
  b32 is_hovered = (document_hover_grab and
                    pick.prim_index == document_hover_pick.prim_index and
                    pick.is_right   == document_hover_pick.is_right and
                    pick.is_handle  == document_hover_pick.is_handle and
                    pick.slot       == document_hover_pick.slot);
  if(is_hovered){ hovered_drawn = true; }
  v3 center = document_pick_world_pos(doc, pick);
  argb color = (is_hovered ? hot_color2 : pick.is_handle ? linear_argb_blue : linear_argb_silver);
  document_hover_draw_disk(camera, center, is_hovered ? 4.5f : 3.f, color);
 }
 if(document_hover_grab and not hovered_drawn)
 {// NOTE(kv) plan-point-primitive Q11: a vertex of an UNSELECTED primitive lights up only
  // while the mouse is on it (no always-on dots: 45 vertices x 2 sides of clutter).
  document_hover_draw_disk(camera, document_pick_world_pos(doc, document_hover_pick), 4.5f, hot_color2);
 }
 {// NOTE(kv) plan-vertex-links Q8: the vertex selection, and every member of the hovered /
  // dragged vertex's link set -- members can sit on unselected primitives, so they are
  // drawn from the table, on the hovered pick's mirror side.
  Document_Vertex_Selection &vsel = state->document_vertex_selection;
  b32 is_right = document_hover_pick.is_right;
  for_i32(i, 0, vsel.count)
  {
   i32 vi = vsel.vertex_index[i];
   if(vi < 0 or vi >= doc.vertices.count){ continue; }
   document_hover_draw_disk(camera, document_vertex_index_world_pos(doc, vi, is_right), 4.f, linear_argb_blue);
  }
  if(document_hover_grab and not document_hover_pick.is_handle)
  {
   i32 hovered_vi = document_pick_vertex_index(doc, document_hover_pick);
   i32 link_id = doc.vertices[hovered_vi].link_id;
   for_i32(vi, 0, doc.vertices.count)
   {
    if(link_id == 0 or vi == hovered_vi or doc.vertices[vi].link_id != link_id){ continue; }
    document_hover_draw_disk(camera, document_vertex_index_world_pos(doc, vi, is_right), 4.f, hot_color2);
   }
  }
 }
}

function void
document_edit_press(Game_State *state, Live_Viewport *viewport, v2 mouse_px, Document_Pick best,
                    b32 free_handle, b32 solo)
{// NOTE(kv) `solo` (Alt at press, plan-vertex-links Q5): a linked vertex moves alone.
 // Grab one control point (a table vertex, or a curve handle) of a selected
 // primitive; the caller found it with document_pick_nearest within the grab radius.
 // `free_handle` (Ctrl at press, plan-handle-drag-modes Q2): a handle drag defines a new
 // plane instead of staying in the old one; means nothing for a vertex (Q5).
 Document_Edit_State &edit = state->document_edit;
 Recording &doc = state->model.recordings.document;
 if(not viewport){ return; }
 i32 prim_index = best.prim_index;
 Recorded_Primitive &prim = doc.primitives[prim_index];

 v3 world = document_pick_world_pos(doc, best);
 Screen_Projection_Data proj = mk_screen_projection_data(state, viewport);
 edit.active = true;
 edit.pick = best;
 edit.free_handle = (free_handle and best.is_handle);
 edit.solo = solo;
 edit.grab_cam_z = mat4vert(proj.camera.cam_from_world, world).z;
 edit.grab_offset_px = mouse_px - project(proj, world);
 edit.location = document_location(prim_index, best.is_right);
 {// NOTE(kv) Open the history entry now; release commits it only if something moved.
  Document_Action action = {};
  action.kind       = best.is_handle ? Document_Action_Move_Handle : Document_Action_Move_Vertex;
  action.prim_index = prim_index;
  action.index      = best.is_handle ? best.slot : document_pick_vertex_index(doc, best);
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
 Screen_Projection_Data proj = mk_screen_projection_data(state, viewport);
 edit.active = true;
 edit.whole_stroke = true;
 edit.pick = reference;
 edit.grab_cam_z = mat4vert(proj.camera.cam_from_world, world).z;
 edit.grab_offset_px = mouse_px - project(proj, world);
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

function Bone_ID
document_vertex_index_bone(Recording &doc, i32 vertex_index)
{// NOTE(kv) plan-vertex-links: the effective bone of a table vertex without a primitive
 // in hand -- Bone_None resolves through the first primitive referencing it.
 Recorded_Vertex &vertex = doc.vertices[vertex_index];
 if(vertex.bone.type != Bone_None){ return vertex.bone; }
 for_i32(iprim, 0, doc.primitives.count)
 {
  Recorded_Primitive &prim = doc.primitives[iprim];
  i32 vertex_count = primitive_vertex_count(prim.type);
  for_i32(ivertex, 0, vertex_count)
  {
   if(prim.vertex_index[ivertex] == vertex_index){ return doc.groups[prim.group_index].bone_id; }
  }
 }
 return vertex.bone;
}

function b32
document_vertex_list_contains(i32 *vertex_indices, i32 vertex_count, i32 vertex_index)
{
 for_i32(i, 0, vertex_count){ if(vertex_indices[i] == vertex_index){ return true; } }
 return false;
}

function void
document_edit_move_vertices(Recording &doc, i32 *vertex_indices, i32 vertex_count,
                            b32 is_right, v3 delta_world)
{// NOTE(kv) Move N distinct table vertices by ONE world delta (plan-vertex-links; N=1 is
 // the plain vertex drag, N=2 the whole-stroke drag). The handles of every curve touching
 // a moved vertex are offsets from the chord thirds, so they translate on their own; on
 // top of that (plan-curve-coplanar-handles Q2, tablet move_vertex) both offsets of a
 // curve with exactly ONE moved end get the minimal rotation old-chord-dir ->
 // new-chord-dir, so the in-plane shape rides the chord and {chord, d0, d3} stay
 // coplanar. A curve with BOTH ends moved only translates: its offsets are not touched
 // at all (bit-identical). Batched on purpose -- moving the vertices one call at a time
 // rotated such a curve on the first move and back on the second (float noise).
 // Skipped when either chord is ~0.
 Scratch_Scope scratch;
 v3 *old_chords = push_array(scratch, v3, maximum(1, doc.primitives.count));
 for_i32(iprim, 0, doc.primitives.count)
 {
  Recorded_Primitive &curve = doc.primitives[iprim];
  if(curve.type == Primitive_Type_Curve){ old_chords[iprim] = document_curve_chord(doc, curve); }
 }
 for_i32(i, 0, vertex_count)
 {
  Bone_ID bone_id = document_vertex_index_bone(doc, vertex_indices[i]);
  doc.vertices[vertex_indices[i]].p += document_edit_bone_delta(bone_id, is_right, delta_world);
 }
 for_i32(iprim, 0, doc.primitives.count)
 {
  Recorded_Primitive &curve = doc.primitives[iprim];
  if(curve.type != Primitive_Type_Curve){ continue; }
  b32 moved0 = document_vertex_list_contains(vertex_indices, vertex_count, curve.vertex_index[0]);
  b32 moved1 = document_vertex_list_contains(vertex_indices, vertex_count, curve.vertex_index[1]);
  if(moved0 == moved1){ continue; }  // NOTE(kv) untouched, rigidly translated, or a loop
  v3 old_chord = old_chords[iprim];
  v3 new_chord = document_curve_chord(doc, curve);
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

function i32
document_link_members(Recording &doc, Arena *arena, i32 vertex_index, i32 **members_out)
{// NOTE(kv) plan-vertex-links: `vertex_index` plus every vertex sharing its link_id.
 i32 link_id = doc.vertices[vertex_index].link_id;
 i32 *members = push_array(arena, i32, maximum(1, doc.vertices.count));
 i32 count = 0;
 members[count++] = vertex_index;
 if(link_id != 0)
 {
  for_i32(iv, 0, doc.vertices.count)
  {
   if(iv != vertex_index and doc.vertices[iv].link_id == link_id){ members[count++] = iv; }
  }
 }
 *members_out = members;
 return count;
}

//~ NOTE(kv) plan-curve-coplanar-handles Q1: a handle drag lands where the mouse ray
// pierces the curve's plane (tablet "plane" mode, the default there); the other handle
// never moves. Edge-on plane or a hit behind the eye: the handle stays where it is.
global v1 const document_edit_edge_on_cosine = 0.15f;  // tablet EDGE_ON_PLANE_COSINE
function b32
document_edit_handle_plane_target(Recording &doc, Screen_Projection_Data const &proj,
                                  v2 px, Document_Pick pick, v3 *world_out)
{// NOTE(kv) All in world space, on the picked mirror side: the plane is {chord, d0}
 // (fallback d3, fallback camera-facing); the pick ray comes from screen_ray -- from the
 // eye through `px` in perspective, parallel along camera -z in ortho.
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
 v3 camera_forward = -proj.camera.z;  // NOTE(kv) the camera looks down its -z
 v3 normal = curve_plane_normal(u, d0, d3, camera_forward);
 // NOTE(kv) camera-space pick ray -> world (rotation only for the direction).
 Screen_Ray r = screen_ray(proj, px - proj.center);
 v3 ray_origin = mat4vert(proj.camera.world_from_cam, r.P);
 v3 ray = noz(mat4vert(proj.camera.world_from_cam, r.dir) - mat4vert(proj.camera.world_from_cam, V3()));
 v1 denom = dot(ray, normal);
 if(absolute(denom) < document_edit_edge_on_cosine){ return false; }
 v1 t = dot(p0 - ray_origin, normal) / denom;
 if(t <= 0){ return false; }
 *world_out = ray_origin + ray*t;
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

//~ NOTE(kv) plan-vertex-links: the vertex selection (Q3) and Link / Unlink (Q4/Q6).
function void
document_vertex_selection_toggle(Game_State *state, i32 vertex_index)
{
 Document_Vertex_Selection &vsel = state->document_vertex_selection;
 for_i32(i, 0, vsel.count)
 {
  if(vsel.vertex_index[i] == vertex_index)
  {
   for_i32(j, i, vsel.count-1){ vsel.vertex_index[j] = vsel.vertex_index[j+1]; }
   vsel.count--;
   return;
  }
 }
 if(vsel.count < alen(vsel.vertex_index)){ vsel.vertex_index[vsel.count++] = vertex_index; }
}

function i32
document_link_candidates(Game_State *state, i32 *out, i32 cap)
{// NOTE(kv) What "Link vertices" / "Unlink" act on: the vertex selection if there is one,
 // else the endpoints of every selected curve (Q3). Distinct table indices.
 Recording &doc = state->model.recordings.document;
 Document_Vertex_Selection &vsel = state->document_vertex_selection;
 i32 count = 0;
 for_i32(i, 0, vsel.count)
 {
  i32 vi = vsel.vertex_index[i];
  if(vi < 0 or vi >= doc.vertices.count){ continue; }
  if(count < cap and not document_vertex_list_contains(out, count, vi)){ out[count++] = vi; }
 }
 if(count == 0)
 {
  i32 curves[Document_Selection_Cap];
  i32 curve_count = document_selection_curve_indices(state, curves);
  for_i32(i, 0, curve_count)
  {
   for_i32(slot, 0, 2)
   {
    i32 vi = doc.primitives[curves[i]].vertex_index[slot];
    if(count < cap and not document_vertex_list_contains(out, count, vi)){ out[count++] = vi; }
   }
  }
 }
 return count;
}

function b32
document_link_vertices(Game_State *state, i32 *vertex_indices, i32 vertex_count)
{// NOTE(kv) Put the vertices in ONE link set. Members already in other sets drag those
 // sets along (Q4: merge). Refuses a selection spanning bones (Q6): a link drag is one
 // bone-local delta for all members.
 Recording &doc = state->model.recordings.document;
 if(vertex_count < 2){ log_error("link: need at least 2 vertices, got %d", vertex_count); return false; }
 for_i32(i, 0, vertex_count)
 {
  if(vertex_indices[i] < 0 or vertex_indices[i] >= doc.vertices.count)
  { log_error("link: vertex %d out of range", vertex_indices[i]); return false; }
 }
 i32 max_id = 0;
 for_i32(iv, 0, doc.vertices.count){ max_id = maximum(max_id, doc.vertices[iv].link_id); }
 Bone_ID bone_id = document_vertex_index_bone(doc, vertex_indices[0]);
 for_i32(iv, 0, doc.vertices.count)
 {// NOTE(kv) Everything that would end up in the merged set must share the bone.
  b32 joins = false;
  for_i32(i, 0, vertex_count)
  {
   i32 vi = vertex_indices[i];
   if(iv == vi or (doc.vertices[vi].link_id != 0 and doc.vertices[vi].link_id == doc.vertices[iv].link_id))
   { joins = true; break; }
  }
  if(joins and not (document_vertex_index_bone(doc, iv) == bone_id))
  {
   log_error("link: vertex %d is on a different bone than vertex %d, not linking", iv, vertex_indices[0]);
   return false;
  }
 }
 Document_Action action = {};
 action.kind  = Document_Action_Link_Vertices;
 action.count = vertex_count;
 history_begin(state, action);
 i32 new_id = max_id + 1;
 for_i32(i, 0, vertex_count)
 {
  i32 old_id = doc.vertices[vertex_indices[i]].link_id;
  doc.vertices[vertex_indices[i]].link_id = new_id;
  if(old_id == 0){ continue; }
  for_i32(iv, 0, doc.vertices.count)
  {
   if(doc.vertices[iv].link_id == old_id){ doc.vertices[iv].link_id = new_id; }
  }
 }
 doc.captured = true;
 state->document_vertex_selection.count = 0;
 document_edit_commit_and_save(state);
 return true;
}

function void
document_link_dissolve_singletons(Recording &doc)
{// NOTE(kv) A set left with one member is just a vertex.
 for_i32(iv, 0, doc.vertices.count)
 {
  i32 link_id = doc.vertices[iv].link_id;
  if(link_id == 0){ continue; }
  b32 has_peer = false;
  for_i32(jv, 0, doc.vertices.count)
  {
   if(jv != iv and doc.vertices[jv].link_id == link_id){ has_peer = true; break; }
  }
  if(not has_peer){ doc.vertices[iv].link_id = 0; }
 }
}

function b32
document_unlink_vertices(Game_State *state, i32 *vertex_indices, i32 vertex_count)
{// NOTE(kv) Take these vertices out of their sets; the rest of each set stays linked.
 Recording &doc = state->model.recordings.document;
 i32 linked_count = 0;
 for_i32(i, 0, vertex_count)
 {
  i32 vi = vertex_indices[i];
  if(vi >= 0 and vi < doc.vertices.count and doc.vertices[vi].link_id != 0){ linked_count++; }
 }
 if(linked_count == 0){ return false; }
 Document_Action action = {};
 action.kind  = Document_Action_Unlink_Vertices;
 action.count = linked_count;
 history_begin(state, action);
 for_i32(i, 0, vertex_count)
 {
  i32 vi = vertex_indices[i];
  if(vi >= 0 and vi < doc.vertices.count){ doc.vertices[vi].link_id = 0; }
 }
 document_link_dissolve_singletons(doc);
 state->document_vertex_selection.count = 0;
 document_edit_commit_and_save(state);
 return true;
}

function b32
document_unlink_set(Game_State *state, i32 link_id)
{// NOTE(kv) Selection panel "delete": the whole set goes.
 Recording &doc = state->model.recordings.document;
 Scratch_Scope scratch;
 i32 *members = push_array(scratch, i32, maximum(1, doc.vertices.count));
 i32 count = 0;
 for_i32(iv, 0, doc.vertices.count)
 {
  if(link_id != 0 and doc.vertices[iv].link_id == link_id){ members[count++] = iv; }
 }
 return document_unlink_vertices(state, members, count);
}

function void
document_edit_move(Game_State *state, Live_Viewport *viewport, v2 mouse_px)
{
 Document_Edit_State &edit = state->document_edit;
 Recording &doc = state->model.recordings.document;
 if(not edit.active or not viewport){ return; }
 Document_Pick pick = edit.pick;
 Recorded_Primitive &prim = doc.primitives[pick.prim_index];

 Screen_Projection_Data proj = mk_screen_projection_data(state, viewport);
 v3 old_world = document_pick_world_pos(doc, pick);
 v3 new_world;
 // NOTE(kv) plan-handle-drag-modes Q4: a midline curve's plane IS the mirror plane, so a
 // free drag on it is just an in-plane drag (apply_midline would flatten it anyway).
 b32 free_handle = (edit.free_handle and not prim.curve.midline);
 if(pick.is_handle and not edit.whole_stroke and not free_handle)
 {// NOTE(kv) plan-curve-coplanar-handles Q1: the handle stays in the curve's plane.
  if(not document_edit_handle_plane_target(doc, proj, mouse_px - edit.grab_offset_px,
                                           pick, &new_world))
  {
   return;
  }
 }
 else
 {
  new_world = unproject(proj, mouse_px - edit.grab_offset_px, edit.grab_cam_z);
 }
 v3 delta_world = new_world - old_world;
 // NOTE(kv) Project/unproject round-trip jitter must not count as an edit (it would
 // save the file on every release).
 if(length_squared(delta_world) < 1e-12f){ return; }
 edit.moved = true;

 if(edit.whole_stroke or not pick.is_handle)
 {// NOTE(kv) Whole stroke: both endpoints (a curve looping onto one vertex moves it
  // once); its chord only translates, so its handles are not rotated. Another curve
  // sharing ONE of the moved vertices turns as it would under a vertex drag.
  // NOTE(kv) plan-vertex-links: every grabbed vertex drags its link set along, unless
  // Alt was held at press (`solo`, Q5). One batched move, see document_edit_move_vertices.
  Scratch_Scope scratch;
  i32 *moved = push_array(scratch, i32, maximum(1, doc.vertices.count));
  i32 moved_count = 0;
  i32 grabbed_count = edit.whole_stroke ? 2 : 1;
  for_i32(igrab, 0, grabbed_count)
  {
   i32 grabbed = prim.vertex_index[edit.whole_stroke ? igrab : pick.slot];
   i32 *members = &grabbed;
   i32 member_count = 1;
   if(not edit.solo){ member_count = document_link_members(doc, scratch, grabbed, &members); }
   for_i32(im, 0, member_count)
   {
    if(not document_vertex_list_contains(moved, moved_count, members[im])){ moved[moved_count++] = members[im]; }
   }
  }
  document_edit_move_vertices(doc, moved, moved_count, pick.is_right, delta_world);
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
  if(not edit.whole_stroke and not edit.pick.is_handle)
  {// NOTE(kv) plan-point-primitive Q12: a plain click on a vertex (released without
   // moving) makes it the SOLE vertex selection and clears the primitive selection --
   // same rule as primitives (click = select this, shift-click = toggle). A drag does
   // not touch either selection.
   Recording &doc = state->model.recordings.document;
   Document_Vertex_Selection &vsel = state->document_vertex_selection;
   vsel.count = 1;
   vsel.vertex_index[0] = document_pick_vertex_index(doc, edit.pick);
   state->document_selection.count = 0;
  }
 }
 edit = {};
}

//~ NOTE(kv) plan-keyboard-vertex-move: the vertex selection moves by keyboard, slider
// style. The first nudge opens a history entry that stays pending across frames; Enter
// commits it (one undo entry + save), Esc -- or any other history/save traffic, see
// history_nudge_cancel -- puts the vertices back.
global v1 const document_nudge_speed = 0.05f;  // world units per second; Shift = x10

function b32
document_nudge(Game_State *state, v3 delta_world, b32 solo)
{
 Recording &doc = state->model.recordings.document;
 Document_Vertex_Selection &vsel = state->document_vertex_selection;
 Document_History &history = state->document_history;
 if(vsel.count == 0 or state->document_edit.active){ return false; }
 if(history.pending and not history.pending_nudge){ return false; }  // someone else's edit is open
 for_i32(i, 0, vsel.count)
 {
  if(vsel.vertex_index[i] < 0 or vsel.vertex_index[i] >= doc.vertices.count){ return false; }
 }
 if(not history.pending_nudge)
 {
  Document_Action action = {};
  action.kind  = Document_Action_Move_Vertex;
  action.index = vsel.vertex_index[0];
  for_i32(iprim, 0, doc.primitives.count)
  {// NOTE(kv) Any primitive on the vertex, for the group name in the history label.
   Recorded_Primitive &prim = doc.primitives[iprim];
   i32 vertex_count = (prim.type == Primitive_Type_Point) ? 1 : 2;
   b32 found = false;
   for_i32(iv, 0, vertex_count){ if(prim.vertex_index[iv] == action.index){ found = true; } }
   if(found){ action.prim_index = iprim; break; }
  }
  history_begin(state, action);
  history.pending_nudge = true;
 }
 Scratch_Scope scratch;
 i32 *moved = push_array(scratch, i32, maximum(1, doc.vertices.count));
 i32 moved_count = 0;
 for_i32(isel, 0, vsel.count)
 {
  i32 selected = vsel.vertex_index[isel];
  i32 *members = &selected;
  i32 member_count = 1;
  if(not solo){ member_count = document_link_members(doc, scratch, selected, &members); }
  for_i32(im, 0, member_count)
  {
   if(not document_vertex_list_contains(moved, moved_count, members[im])){ moved[moved_count++] = members[im]; }
  }
 }
 document_edit_move_vertices(doc, moved, moved_count, false, delta_world);
 document_apply_midlines(doc);
 return true;
}
function b32
document_nudge_commit(Game_State *state)
{
 Document_History &history = state->document_history;
 if(not history.pending_nudge){ return false; }
 history.pending_nudge = false;  // NOTE(kv) before the save, which would cancel it
 history_commit(state);
 save_document_file(state);
 return true;
}
