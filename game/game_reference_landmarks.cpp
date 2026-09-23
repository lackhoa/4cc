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
 // meshes are drawn under.
 mat4i world_from_bone = get_bone(mk_bone_id(Bone_Head), /*is_right*/false)->world_from_bone;
 mat4i bone_from_mesh = (mat4i_translate(placement.center) *
                         mat4i_scale(reference_mesh_effective_scale(placement)) *
                         mat4i_rotate_tpr(placement.rotation.x, placement.rotation.y,
                                          placement.rotation.z));
 return world_from_bone * bone_from_mesh;
}

function void
draw_reference_landmarks(Game_State *state, Camera &camera, v2 clip_center)
{// NOTE(kv) A small orange disk per landmark, overlaid like the document hover disks, plus
 // its name as ImGui foreground text at the projected window px (the game has no
 // world-space text of its own).
 Reference_Mesh_Placement *placement = get_reference_mesh_placement(state);
 if(placement == 0){ return; }
 mat4i world_from_mesh = reference_landmark_world_from_mesh(state, *placement);
 Screen_Projection_Data proj = mk_screen_projection_data(state, clip_center);
 argb color = argb_pack(V4(1.f, 0.6f, 0.1f, 1.f));
 for_i32(layer_index, 0, reference_mesh_layer_cap)
 {
  Reference_Landmark_Set *set = reference_landmark_set_for_layer(state, layer_index);
  if(set == 0){ break; }
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
