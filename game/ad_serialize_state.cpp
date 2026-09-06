// NOTE(kv) data/state.txt: everything persisted beside the big model (Serialized_State:
// kb cursor, global flags, viewport cameras + preset indices, the preset rows), as
// struct-literal text via ad_serialize_text.cpp. Replaces the binary autosave.ad.
// Plan: ~/notes/tasks/autodraw_draw_as_data/plan-presets-text-file.md

function Stringz
state_file_path(Arena *arena, Game_State *state)
{
 return pjoin(arena, state->save_dir, strlit("state.txt"));
}

function void
gather_serialized_state(Game_State *state)
{// NOTE(kv) Live -> Serialized_State (the viewports and preset rows live elsewhere).
 for_i32(viewport_index, 0, GAME_VIEWPORT_COUNT)
 {
  state->saved_viewports[viewport_index] = state->viewports[viewport_index].saved;
 }
 Model_Recordings &rec = state->model.recordings;
 state->presets_count = clamp_between(0, rec.preset_count, PRESET_CAP);
 for_i32(index, 0, state->presets_count){ state->presets[index] = rec.preset_settings[index]; }
}
function void
scatter_serialized_state(Game_State *state)
{// NOTE(kv) Serialized_State -> live. A file without presets keeps the seeded rows.
 Model_Recordings &rec = state->model.recordings;
 if(state->presets_count >= 1)
 {
  rec.preset_count = state->presets_count;
  for_i32(index, 0, rec.preset_count){ rec.preset_settings[index] = state->presets[index]; }
 }
 for_i32(viewport_index, 0, GAME_VIEWPORT_COUNT)
 {
  Viewport &viewport = state->viewports[viewport_index];
  viewport.saved = state->saved_viewports[viewport_index];
  viewport.preset      = clamp_between(0, viewport.preset,      rec.preset_count-1);
  viewport.last_preset = clamp_between(0, viewport.last_preset, rec.preset_count-1);
 }
}

function b32
write_state_file(FILE *file, Game_State *state)
{
 gather_serialized_state(state);
 Printer p = make_printer_file(file);
 write_text_top_level(p, &Type_Info_Serialized_State, &state->serialized);
 return not p.error;
}
function b32
save_state_file(Game_State *state)
{
 Scratch_Scope tmp;
 return save_file_via_temp(state, state_file_path(tmp, state),
                           pjoin(tmp, state->save_dir, strlit("state_temp.txt")),
                           write_state_file, "state");
}
function b32
load_state_file(Game_State *state)
{// NOTE(kv) false = missing or rejected (rejected -> load_failed banner). Reads into a
 // temporary so a syntax error leaves the live state untouched.
 Scratch_Scope tmp;
 Stringz path = state_file_path(tmp, state);
 String file_data = read_entire_file(tmp, path);
 if(file_data.len == 0)
 {
  log_string("state load: no file at %S", path);
  return false;
 }
 Arena *load_arena = &state->data_load_arena;
 arena_free(load_arena);
 Serialized_State loaded = {};
 b32 ok = read_text_top_level(file_data, load_arena, "state.txt", &Type_Info_Serialized_State, &loaded);
 if(ok)
 {
  state->serialized = loaded;
  scatter_serialized_state(state);
  log_string("state load: ok (%S)", path);
 }
 else
 {
  log_error("state load: REJECTED (%S)", path);
 }
 state->load_failed = not ok;
 return ok;
}

//~ One-off migration (Q5): autosave.ad + recording.ad (version 29) -> state.txt
// TODO(kv) Delete once Khoa confirms the migrated state.txt (plan-presets-text-file Q5).

struct Preset_Settings_V29
{// NOTE(kv) The raw row layout of recording.ad version 29 (bools in X-macro order).
 char name[PRESET_NAME_CAP];
 i32 viz_level;
 i32 reference_image;
 Reference_Scene scene;
 b32 show_eyeball;
 b32 show_loomis_ball;
 b32 show_grid;
 b32 fill_only_picking;
 b32 show_arm_medial_right;
 b32 show_arm_back_bone;
 b32 show_arm_profile_left;
 b32 ignore_radii;
 b32 ignore_alignment_min;
};
function b32
migrate_read_autosave_v29(Game_State *state, Stringz path)
{
 Scratch_Scope tmp;
 String file_data = read_entire_file(tmp, path);
 if(file_data.len == 0){ return false; }
 Binary_Reader reader = make_binary_reader(file_data.data, file_data.size);
 Binary_Reader *r = &reader;
 u32 magic = read_binary_u32(r);
 if(magic != autodraw_data_magic){ r->ok = false; }
 r->read_version = read_binary_u32(r);
 if(r->read_version != Version_NamedPresets){ r->ok = false; }
 u64 timestamp = read_binary_u64(r); (void)timestamp;
 read_debug_string(r, strlit("Serialized_State"));
 Keyboard_Cursor kb_cursor = {};
 read_binary_Keyboard_Cursor(r, &kb_cursor);
 b32 references_full_alpha = read_binary_i1(r);
 Saved_Viewport viewports[GAME_VIEWPORT_COUNT] = {};
 for_i32(index, 0, GAME_VIEWPORT_COUNT)
 {
  read_binary_Camera_Data(r, &viewports[index].target_camera);
  viewports[index].preset      = read_binary_i1(r);
  viewports[index].last_preset = read_binary_i1(r);
  i32 legacy_reference_preset  = read_binary_i1(r); (void)legacy_reference_preset;
 }
 read_debug_string(r, strlit("EOF"));
 if(r->ok)
 {
  state->kb_cursor = kb_cursor;
  state->references_full_alpha = references_full_alpha;
  for_i32(index, 0, GAME_VIEWPORT_COUNT){ state->viewports[index].saved = viewports[index]; }
 }
 return r->ok;
}
function b32
migrate_read_recording_presets_v29(Game_State *state, Stringz path)
{
 Scratch_Scope tmp;
 String file_data = read_entire_file(tmp, path);
 if(file_data.len == 0){ return false; }
 Binary_Reader reader = make_binary_reader(file_data.data, file_data.size);
 Binary_Reader *r = &reader;
 u32 magic = read_binary_u32(r);
 if(magic != autodraw_data_magic){ r->ok = false; }
 r->read_version = read_binary_u32(r);
 if(r->read_version != Version_NamedPresets){ r->ok = false; }
 u64 timestamp = read_binary_u64(r); (void)timestamp;
 u32 primitive_size = read_binary_u32(r); (void)primitive_size;
 u32 group_size     = read_binary_u32(r); (void)group_size;
 u32 settings_size  = read_binary_u32(r);
 u32 vertex_size    = read_binary_u32(r); (void)vertex_size;
 if(settings_size != sizeof(Preset_Settings_V29)){ r->ok = false; }
 i32 count = read_binary_i1(r);
 if(not (r->ok and 1 <= count and count <= PRESET_CAP and
         reader_can_take(r, count, sizeof(Preset_Settings_V29)))){ return false; }
 Model_Recordings &rec = state->model.recordings;
 rec.preset_count = count;
 for_i32(index, 0, count)
 {
  Preset_Settings_V29 old = {};
  read_binary_size(r, sizeof(old), &old);
  Preset_Settings &row = rec.preset_settings[index];
  row = {};
  block_copy(row.name, old.name, sizeof(row.name));
  row.viz_level             = old.viz_level;
  row.reference_image       = old.reference_image;
  row.scene                 = old.scene;
  row.show_eyeball          = old.show_eyeball;
  row.show_loomis_ball      = old.show_loomis_ball;
  row.show_grid             = old.show_grid;
  row.ignore_radii          = old.ignore_radii;
  row.ignore_alignment_min  = old.ignore_alignment_min;
  row.show_arm_medial_right = old.show_arm_medial_right;
  row.show_arm_back_bone    = old.show_arm_back_bone;
  row.show_arm_profile_left = old.show_arm_profile_left;
  row.fill_only_picking     = old.fill_only_picking;
 }
 return r->ok;
}
function void
migrate_state_from_binary_files(Game_State *state)
{// NOTE(kv) Called when state.txt is missing: pull what the old files hold, write it.
 Scratch_Scope tmp;
 Stringz autosave_path = pjoin(tmp, state->save_dir, strlit("autosave.ad"));
 b32 got_autosave = migrate_read_autosave_v29(state, autosave_path);
 b32 got_presets  = migrate_read_recording_presets_v29(state, recording_file_path(tmp, state));
 log_string("state migration: autosave.ad %s, recording.ad presets %s",
            got_autosave ? "migrated" : "not migrated", got_presets ? "migrated" : "not migrated");
 if(got_autosave or got_presets){ save_state_file(state); }
}
