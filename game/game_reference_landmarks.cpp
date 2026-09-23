// NOTE(kv) Reference landmarks (plan-reference-landmarks): labeled points in a reference
// mesh's own coordinates, stored in `<mesh>.landmarks.txt` beside the .obj, drawn as
// disks + labels on the mesh, and edited through the debug channel (landmark_set /
// landmark_delete / landmark_dump). Owned game-side (Game_State.reference_landmark_sets),
// keyed by the layer's mesh path: the driver only ever sees the mesh, not the labels.

function Stringz
reference_landmark_sidecar_path(Arena *arena, String mesh_path)
{// NOTE(kv) `.../z-anatomy-head-skull.obj` -> `.../z-anatomy-head-skull.landmarks.txt`
 return strcat(arena, path_no_extension(mesh_path), strlit(".landmarks.txt"));
}

function b32
reference_landmark_set_load(Reference_Landmark_Set *set, Stringz mesh_path)
{// NOTE(kv) Missing sidecar = no landmarks (not an error). A rejected file logs and leaves
 // the set empty rather than half-read.
 Scratch_Scope tmp;
 block_zero_struct(set);
 snprintf(set->mesh_path, sizeof(set->mesh_path), "%.*s", string_expand(mesh_path));
 set->loaded = true;
 Stringz path = reference_landmark_sidecar_path(tmp, mesh_path);
 String file_data = read_entire_file(tmp, path);
 if(file_data.len == 0){ return true; }
 Reference_Landmark_File loaded = {};
 b32 ok = read_text_top_level(file_data, tmp, "landmarks", &Type_Info_Reference_Landmark_File, &loaded);
 if(ok){ set->file = loaded; log_string("landmarks load: ok (%S)", path); }
 else  { log_error("landmarks load: REJECTED (%S)", path); }
 return ok;
}

function b32
reference_landmark_set_save(Reference_Landmark_Set *set)
{
 Scratch_Scope tmp;
 Stringz path = reference_landmark_sidecar_path(tmp, SCu8(set->mesh_path));
 FILE *file = open_file(path, "wb");
 b32 ok = (file != 0);
 if(ok)
 {
  Printer p = make_printer_file(file);
  write_text_top_level(p, &Type_Info_Reference_Landmark_File, &set->file);
  ok = not p.error;
  close_file(file);
 }
 if(ok){ log_string("landmarks saved to %S", path); }
 else  { log_error("landmarks save FAILED (%S)", path); }
 return ok;
}

function Reference_Landmark_Set *
reference_landmark_set_for_layer(Game_State *state, i32 layer_index)
{// NOTE(kv) The active scene's layer `layer_index`, sidecar read on first touch; null when
 // the scene has no such mesh layer (same gate as get_reference_mesh_placement).
 if(get_reference_mesh_placement(state) == 0){ return 0; }
 Driver_API *driver = &state->driver_api;
 Reference_Scene_Data data = driver->driver_get_scene_data(active_preset_row(state).scene);
 if(layer_index < 0 or layer_index >= data.mesh_layer_count){ return 0; }
 Stringz mesh_path = data.mesh_layers[layer_index].filename;
 Reference_Landmark_Set *set = &state->reference_landmark_sets[layer_index];
 if(not set->loaded or not string_match(SCu8(set->mesh_path), mesh_path))
 {
  reference_landmark_set_load(set, mesh_path);
 }
 return set;
}

function Reference_Landmark *
reference_landmark_find(Reference_Landmark_Set *set, String name)
{
 for_i32(i, 0, set->file.landmarks_count)
 {
  Reference_Landmark *landmark = &set->file.landmarks[i];
  if(string_match(SCu8(landmark->name), name)){ return landmark; }
 }
 return 0;
}

function mat4i
reference_landmark_world_from_mesh(Game_State *state, Reference_Mesh_Placement &placement)
{// NOTE(kv) Mesh (.obj) space -> world: the shared placement (same math as
 // draw_reference_mesh / reference_mesh_bone_center), lifted through the LEFT Bone_Head the
 // meshes are drawn under. Layer-independent; see reference_layer_world_from_mesh for the
 // hinged mandible.
 mat4i world_from_bone = get_bone(mk_bone_id(Bone_Head), /*is_right*/false)->world_from_bone;
 mat4i bone_from_mesh = (mat4i_translate(placement.center) *
                         mat4i_scale(reference_mesh_effective_scale(placement)) *
                         mat4i_rotate_tpr(placement.rotation.x, placement.rotation.y,
                                          placement.rotation.z));
 return world_from_bone * bone_from_mesh;
}

//~ NOTE(kv) Step 4: the jaw hinge (plan Q6). The mandible layer is flagged `hinged` in the
// scene; its transform is a rotation about the condyle axis (landmarks `condyle_l` /
// `condyle_r` on the mandible mesh) by the angle that brings `incisor_lower` (mandible mesh)
// onto `incisor_upper` (any other layer). All Z-Anatomy layers share one .obj frame, so the
// four points live in the same space. Missing landmarks = no hinge (identity).

function mat3
mat3_rotate_axis(v3 u, v1 radians)
{// NOTE(kv) Rodrigues, u unit, right-handed about u.
 v1 c = cosf(radians), s = sinf(radians), t = 1.f - c;
 mat3 R;
 R.rows[0] = V3(t*u.x*u.x + c,     t*u.x*u.y - s*u.z, t*u.x*u.z + s*u.y);
 R.rows[1] = V3(t*u.x*u.y + s*u.z, t*u.y*u.y + c,     t*u.y*u.z - s*u.x);
 R.rows[2] = V3(t*u.x*u.z - s*u.y, t*u.y*u.z + s*u.x, t*u.z*u.z + c);
 return R;
}

function b32
reference_jaw_hinge(Game_State *state, i32 layer_index, mat4i *hinge_out, v1 *radians_out=0)
{// NOTE(kv) The hinged layer's mesh-space transform; false (identity) when the layer is not
 // hinged or a landmark is missing.
 *hinge_out = mat4i_identity;
 if(get_reference_mesh_placement(state) == 0){ return false; }
 Driver_API *driver = &state->driver_api;
 Reference_Scene_Data data = driver->driver_get_scene_data(active_preset_row(state).scene);
 if(layer_index < 0 or layer_index >= data.mesh_layer_count){ return false; }
 if(not data.mesh_layers[layer_index].hinged){ return false; }
 Reference_Landmark_Set *set = reference_landmark_set_for_layer(state, layer_index);
 if(set == 0){ return false; }
 Reference_Landmark *condyle_l = reference_landmark_find(set, strlit("condyle_l"));
 Reference_Landmark *condyle_r = reference_landmark_find(set, strlit("condyle_r"));
 Reference_Landmark *incisor_lower = reference_landmark_find(set, strlit("incisor_lower"));
 Reference_Landmark *incisor_upper = 0;
 for_i32(other, 0, data.mesh_layer_count)
 {
  if(other == layer_index){ continue; }
  Reference_Landmark_Set *other_set = reference_landmark_set_for_layer(state, other);
  if(other_set){ incisor_upper = reference_landmark_find(other_set, strlit("incisor_upper")); }
  if(incisor_upper){ break; }
 }
 if(not (condyle_l and condyle_r and incisor_lower and incisor_upper)){ return false; }
 v3 pivot = 0.5f*(condyle_l->p + condyle_r->p);
 v3 axis = noz(condyle_r->p - condyle_l->p);
 // NOTE(kv) Angle between the two incisors seen down the axis (components along it dropped).
 v3 a = incisor_lower->p - pivot; a -= axis*dot(a, axis);
 v3 b = incisor_upper->p - pivot; b -= axis*dot(b, axis);
 if(lengthof(a) < 1e-6f or lengthof(b) < 1e-6f){ return false; }
 v1 radians = atan2f(dot(cross(a, b), axis), dot(a, b));
 *hinge_out = mat4i_translate(pivot) * mat4i_rotate(mat3_rotate_axis(axis, radians)) * mat4i_translate(-pivot);
 if(radians_out){ *radians_out = radians; }
 return true;
}

function mat4i
reference_layer_world_from_mesh(Game_State *state, Reference_Mesh_Placement &placement, i32 layer_index)
{// NOTE(kv) world_from_mesh for one layer: the shared placement, plus the hinge for the mandible.
 mat4i hinge;
 reference_jaw_hinge(state, layer_index, &hinge);
 return reference_landmark_world_from_mesh(state, placement) * hinge;
}

function void
reference_fill_painter_hinges(Game_State *state, Painter *out_painter)
{// NOTE(kv) Called right before driver_render: the driver draws hinged layers through these.
 for_i32(layer_index, 0, reference_mesh_layer_cap)
 {
  out_painter->reference_layer_hinge_valid[layer_index] =
   reference_jaw_hinge(state, layer_index, &out_painter->reference_layer_hinge[layer_index]);
 }
}

function void
draw_reference_landmarks(Game_State *state, Camera &camera, v2 clip_center)
{// NOTE(kv) A small orange disk per landmark, overlaid like the document hover disks, plus
 // its name as ImGui foreground text at the projected window px (the game has no
 // world-space text of its own).
 Reference_Mesh_Placement *placement = get_reference_mesh_placement(state);
 if(placement == 0){ return; }
 Screen_Projection_Data proj = mk_screen_projection_data(state, clip_center);
 argb color = argb_pack(V4(1.f, 0.6f, 0.1f, 1.f));
 for_i32(layer_index, 0, reference_mesh_layer_cap)
 {
  Reference_Landmark_Set *set = reference_landmark_set_for_layer(state, layer_index);
  if(set == 0){ break; }
  mat4i world_from_mesh = reference_layer_world_from_mesh(state, *placement, layer_index);
  for_i32(i, 0, set->file.landmarks_count)
  {
   Reference_Landmark &landmark = set->file.landmarks[i];
   v3 world = mat4vert(world_from_mesh, landmark.p);
   document_hover_draw_disk(camera, world, 3.f, color);
   v2 px = project(proj, world);
   ImGui::GetForegroundDrawList()->AddText(ImVec2(px.x + 8.f, px.y - 8.f),
                                           IM_COL32(255, 160, 40, 255), landmark.name);
  }
 }
}

//~ NOTE(kv) Step 2: placement UX. Picking runs in MESH space (the ray is pulled through
// world_from_mesh.inverse) against the driver's triangles (driver_get_reference_mesh), so
// the hit point is directly the landmark's stored `p`. All shown layers are tested, nearest
// hit wins.

function v1 hit_test_ray_triangle(v3 ray_P, v3 ray_dir, v3 O, v3 A, v3 B);  // game_main.cpp

struct Reference_Landmark_Mesh_Hit
{
 i32 layer_index;  // -1 = miss
 v3 mesh_p;
};

function Reference_Landmark_Mesh_Hit
reference_landmark_pick_mesh(Game_State *state, Screen_Projection_Data const &proj, v2 px)
{
 Reference_Landmark_Mesh_Hit hit = {.layer_index = -1};
 Reference_Mesh_Placement *placement = get_reference_mesh_placement(state);
 if(placement == 0){ return hit; }
 Driver_API *driver = &state->driver_api;
 Preset_Settings &settings = active_preset_row(state);
 Reference_Scene_Data data = driver->driver_get_scene_data(settings.scene);
 // NOTE(kv) camera-space pick ray -> world -> each layer's mesh space (direction: difference
 // of two points). The hinge is rigid, so t is comparable across layers.
 Screen_Ray r = screen_ray(proj, px - proj.center);
 v3 world_P   = mat4vert(proj.camera.world_from_cam, r.P);
 v3 world_dir = mat4vert(proj.camera.world_from_cam, r.P + r.dir) - world_P;
 v1 best_t = INFINITY;
 for_i32(layer_index, 0, data.mesh_layer_count)
 {
  Reference_Mesh_Layer &layer = data.mesh_layers[layer_index];
  b32 shown = (layer.show_flag == 0 or settings.*layer.show_flag);
  if(not shown){ continue; }
  mat4i world_from_mesh = reference_layer_world_from_mesh(state, *placement, layer_index);
  v3 mesh_P   = mat4vert(world_from_mesh.inverse, world_P);
  v3 mesh_dir = mat4vert(world_from_mesh.inverse, world_P + world_dir) - mesh_P;
  Reference_Mesh_Triangles tris = driver->driver_get_reference_mesh(layer.filename);
  if(not tris.ok){ continue; }
  for(i32 i = 0; i+2 < tris.index_count; i += 3)
  {
   v3 O = tris.vertices[tris.indices[i+0]];
   v3 A = tris.vertices[tris.indices[i+1]];
   v3 B = tris.vertices[tris.indices[i+2]];
   v1 t = hit_test_ray_triangle(mesh_P, mesh_dir, O, A, B);
   if(t < best_t)
   {
    best_t = t;
    hit.layer_index = layer_index;
    hit.mesh_p = mesh_P + mesh_dir*t;
   }
  }
 }
 return hit;
}

function b32
reference_landmark_pick_landmark(Game_State *state, Screen_Projection_Data const &proj, v2 px,
                                 i32 *layer_out, i32 *index_out)
{// NOTE(kv) Nearest landmark within document_pick_radius_px of the mouse.
 Reference_Mesh_Placement *placement = get_reference_mesh_placement(state);
 if(placement == 0){ return false; }
 v1 best = document_pick_radius_px;
 b32 found = false;
 for_i32(layer_index, 0, reference_mesh_layer_cap)
 {
  Reference_Landmark_Set *set = reference_landmark_set_for_layer(state, layer_index);
  if(set == 0){ break; }
  mat4i world_from_mesh = reference_layer_world_from_mesh(state, *placement, layer_index);
  for_i32(i, 0, set->file.landmarks_count)
  {
   v2 lpx = project(proj, mat4vert(world_from_mesh, set->file.landmarks[i].p));
   v1 d = lengthof(lpx - px);
   if(d < best){ best = d; found = true; *layer_out = layer_index; *index_out = i; }
  }
 }
 return found;
}

function void
landmark_tool_reset(Game_State *state)
{
 Landmark_Tool_State &tool = state->landmark_tool;
 tool.armed = false;
 tool.dragging = false;
 tool.drag_layer = -1;
 tool.drag_index = -1;
}

function b32
landmark_tool_press(Game_State *state, Live_Viewport *viewport, v2 mouse_px)
{// NOTE(kv) Returns true when the press was consumed (a drag started or a landmark was
 // placed); a miss returns false so the caller can fall through to the camera orbit.
 Landmark_Tool_State &tool = state->landmark_tool;
 if(not tool.armed or viewport == 0){ return false; }
 Screen_Projection_Data proj = mk_screen_projection_data(state, get_center(viewport->clip_box));
 i32 layer_index = -1, index = -1;
 if(reference_landmark_pick_landmark(state, proj, mouse_px, &layer_index, &index))
 {
  tool.dragging = true;
  tool.drag_layer = layer_index;
  tool.drag_index = index;
  return true;
 }
 if(tool.name[0] == 0)
 {
  log_error("landmark tool: no name set, press ignored (type one in the right-click menu)");
  return false;
 }
 Reference_Landmark_Mesh_Hit hit = reference_landmark_pick_mesh(state, proj, mouse_px);
 if(hit.layer_index < 0){ return false; }
 Reference_Landmark_Set *set = reference_landmark_set_for_layer(state, hit.layer_index);
 Reference_Landmark *landmark = reference_landmark_find(set, SCu8(tool.name));
 if(landmark == 0)
 {
  if(set->file.landmarks_count >= REFERENCE_LANDMARK_CAP)
  {
   log_error("landmark tool: layer %d is full (%d landmarks)", hit.layer_index, REFERENCE_LANDMARK_CAP);
   return false;
  }
  landmark = &set->file.landmarks[set->file.landmarks_count++];
  block_zero_struct(landmark);
  snprintf(landmark->name, sizeof(landmark->name), "%s", tool.name);
 }
 landmark->p = hit.mesh_p;
 reference_landmark_set_save(set);
 // NOTE(kv) The new point is grabbed right away, so press-drag-release places it in one go.
 tool.dragging = true;
 tool.drag_layer = hit.layer_index;
 tool.drag_index = (i32)(landmark - set->file.landmarks);
 return true;
}

function void
landmark_tool_move(Game_State *state, Live_Viewport *viewport, v2 mouse_px)
{// NOTE(kv) Re-raycast every move: the landmark slides over whichever layer is under the
 // mouse. Off the mesh it stays where it was.
 Landmark_Tool_State &tool = state->landmark_tool;
 if(not tool.dragging or viewport == 0){ return; }
 Screen_Projection_Data proj = mk_screen_projection_data(state, get_center(viewport->clip_box));
 Reference_Landmark_Mesh_Hit hit = reference_landmark_pick_mesh(state, proj, mouse_px);
 if(hit.layer_index < 0){ return; }
 Reference_Landmark_Set *set = reference_landmark_set_for_layer(state, tool.drag_layer);
 if(set == 0 or tool.drag_index < 0 or tool.drag_index >= set->file.landmarks_count){ tool.dragging = false; return; }
 if(hit.layer_index != tool.drag_layer)
 {// NOTE(kv) Crossed onto another layer: the landmark moves file (name kept).
  Reference_Landmark_Set *to = reference_landmark_set_for_layer(state, hit.layer_index);
  if(to == 0 or to->file.landmarks_count >= REFERENCE_LANDMARK_CAP){ return; }
  Reference_Landmark moved = set->file.landmarks[tool.drag_index];
  for_i32(i, tool.drag_index, set->file.landmarks_count-1){ set->file.landmarks[i] = set->file.landmarks[i+1]; }
  set->file.landmarks_count--;
  reference_landmark_set_save(set);
  to->file.landmarks[to->file.landmarks_count] = moved;
  tool.drag_index = to->file.landmarks_count++;
  tool.drag_layer = hit.layer_index;
  set = to;
 }
 set->file.landmarks[tool.drag_index].p = hit.mesh_p;
}

function void
landmark_tool_release(Game_State *state)
{
 Landmark_Tool_State &tool = state->landmark_tool;
 if(not tool.dragging){ return; }
 Reference_Landmark_Set *set = reference_landmark_set_for_layer(state, tool.drag_layer);
 if(set){ reference_landmark_set_save(set); }
 tool.dragging = false;
 tool.drag_layer = -1;
 tool.drag_index = -1;
}
