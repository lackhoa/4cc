#include "kv.h"
#define AD_IS_COMPILING_GAME 1
#define ED_API_USER 1
#define ED_API_USER_STORE_GLOBAL 1
#define AD_COMPILING_FRAMEWORK 1
#include "4coder_game_shared.h"
#include "4ed_kv_parser.cpp"
#include "game.h"
#include "game_fui.cpp"
#include "game_api.cpp"

#define STB_C_LEXER_IMPLEMENTATION
#include "stb_c_lexer.h"
#include "game_parser.cpp"

/*
  IMPORTANT Rule for the renderer
  1. Colors are in linear space (todo precision loss if passed as u32)

  (NOTE: I've debated about pushing a scale down to the renderer,
  but that wouldn't help since there are MANY units we can use)

  OLD Rules for the renderer (putting here for reference):
  - pma = Pre-multiplied alpha
  - v4 colors are in linear space, alpha=1 (so pma doesn't matter)
 */

global_const u32 data_current_version = 7;
global_const u32 data_magic_number = *(u32 *)"kvda";

#define X(N) internal N##_return N(N##_params);
// Note: forward declare
X_GAME_API_FUNCTIONS(X)
//
#undef X


internal b32
just_pressed(Game_Input *input, Key_Code keycode, Key_Mods modifiers=Key_Mod_NULL)
{
 return ((input->key_states       [keycode])     &&
         (input->key_state_changes[keycode] > 0) &&
         (input->active_mods == modifiers));
}

force_inline b32
is_key_down(Game_Input *input, Key_Mods modifiers, Key_Code keycode)
{
 return ((input->key_states[keycode]) &&
         (input->active_mods == modifiers));
}

internal v4
key_direction_v4(Game_Input *input, Key_Mods wanted_mods, b32 new_keypress, b32 *optional_shift=0)
{
 v4 direction = {};
 b32 match_exactly = (input->active_mods == wanted_mods);
 b32 match_with_shift = optional_shift && (input->active_mods == (wanted_mods|Key_Mod_Sft));
  
 if (match_exactly || match_with_shift)
 {
#define Hit(N) \
((input->key_states[Key_Code_##N] != 0) && \
((new_keypress == false) || (input->key_state_changes[Key_Code_##N] > 0)))
  //
  if (Hit(L))      {direction.x = +1;}
  if (Hit(H))      {direction.x = -1;}
  if (Hit(K))      {direction.y = +1;}
  if (Hit(J))      {direction.y = -1;}
  if (Hit(O))      {direction.z = +1;}
  if (Hit(I))      {direction.z = -1;}
  if (Hit(Period)) {direction.w = +1;}
  if (Hit(Comma))  {direction.w = -1;}
  //
#undef Hit
 }
 if (optional_shift) { *optional_shift = match_with_shift; }
 return direction;
}

force_inline v3
key_direction(Game_Input *input, Key_Mods wanted_mods, b32 new_keypress)
{
 return key_direction_v4(input, wanted_mods, new_keypress).xyz;
}


DLL_EXPORT game_api_export_return 
game_api_export(game_api_export_params)
{
#define X(N) api->N = N;
 //
 X_GAME_API_FUNCTIONS(X)
  //
#undef X
 api->is_valid = true;
}

global i32 MAIN_VIEWPORT_INDEX = MAIN_VIEWPORT_ID - 1;

inline i32
get_viewport_index(i32 viewport_id)
{
 kv_assert(viewport_id <= GAME_VIEWPORT_COUNT);
 return (viewport_id - 1);
}

internal b32
camera_data_equal(Camera_Data *a, Camera_Data *b)
{
 return block_match(a, b, sizeof(Camera_Data));
}

inline v1
animate_value(v1 start, v1 end, v1 dt, v1 difference_multiplier, v1 min_speed)
{
 macro_clamp_min(min_speed, 0.f);
 v1 abs_difference = absolute(end-start);
 v1 abs_delta = abs_difference * difference_multiplier;
 macro_clamp_min(abs_delta, min_speed*dt);
 macro_clamp_max(abs_delta, abs_difference);
 
 v1 result = (end > start) ? (start+abs_delta) : (start-abs_delta);
 return result;
}

global v1 CAMERA_DISTANCE_STEP         = 5.f  *centimeter;
global v1 CAMERA_PAN_STEP_PER_DISTANCE = 3.75f*centimeter;

internal b32
animate_camera(Camera_Data *current, Camera_Data *saved, v1 dt)
{
 current->distance_level = saved->distance_level;//@distance_level_nonsense
 b32 animation_ended = camera_data_equal(current, saved);
 if ( animation_ended )
 {
  saved->theta   = cycle01(saved->theta);
  current->theta = cycle01(current->theta);
 }
 else
 {
#define ANIMATE(FIELD, MIN_SPEED) \
current->FIELD = animate_value(current->FIELD, saved->FIELD, dt, 0.1f, MIN_SPEED)
  ANIMATE(theta,    0.004f);
  ANIMATE(phi,      0.004f);
  ANIMATE(distance, CAMERA_DISTANCE_STEP/3.0f);
  ANIMATE(pan.x,  CAMERA_PAN_STEP_PER_DISTANCE/3.0f);
  ANIMATE(pan.y,  CAMERA_PAN_STEP_PER_DISTANCE/3.0f);
  ANIMATE(pan.z,  CAMERA_PAN_STEP_PER_DISTANCE/3.0f);
#undef ANIMATE
  
  current->roll  = saved->roll;    // @Hack
  saved->pivot   = current->pivot; // @Hack
 }
 
 return animation_ended;
}

// TODO: We really should merge this with game_update, come on why are there two calls instead of one?
internal game_viewport_update_return
game_viewport_update(game_viewport_update_params)
{
 b32 should_animate_next_frame = false;
 i32 viewport_index = get_viewport_index(viewport_id);
 Viewport *viewport = &state->viewports[viewport_index];
 Game_Save *save = &state->save;
 {// NOTE: camera animtion
  Camera_Data *saved   = &save->camera[viewport_index];
  Camera_Data *current = &viewport->camera;
  b32 animation_ended = animate_camera(current, saved, dt);
  if ( !animation_ended ) { should_animate_next_frame = true; }
 }
 
 return should_animate_next_frame;
}

inline v1 
round_to_multiple_of(v1 value, v1 n)
{
 v1 result = roundv1(value / n) * n;
 return result;
}

internal b32
deserialize_state(Game_Save *save, String input)
{
 char string_store[64];
 STB_Parser parser = new_parser(input, string_store, alen(string_store));
 STB_Parser *p = &parser;
 
 eat_id(p, "version");
 save->version = eat_int(p);
 eat_id(p, "cameras");
 
 Camera_Data cameras[GAME_VIEWPORT_COUNT];
 for_i32(cam_index,0,GAME_VIEWPORT_COUNT)
 {
  Camera_Data *cam = &save->camera[cam_index];
  eat_char(p, '{');
  eat_id(p, "distance");       cam->distance       = eat_float(p);
  eat_id(p, "phi");            cam->phi            = eat_float(p);
  eat_id(p, "theta");          cam->theta          = eat_float(p);
  eat_id(p, "distance_level"); cam->distance_level = eat_int(p);
  eat_id(p, "roll");           cam->roll           = eat_float(p);
  eat_id(p, "pan");            cam->pan            = eat_v3(p);
  if( maybe_id(p, "pivot") ) {
   cam->pivot= eat_v3(p);
  } else {
   cam->pivot = {};
  }
  
  eat_char(p, '}');
 }
 
#if 0
 if (p->ok)
 {// NOTE: test
  make_temp_arena(arena);
  String test_path = strlit("C:/Users/vodan/4ed/code/data/text_test.kv");
  String serialization = serialize_state(arena, save);
  write_entire_file(test_path, serialization);
 }
#endif
 
 return p->ok;
}


//~

// NOTE
#include "game_ed.cpp"
#include "game_colors.cpp"
#include "ad_debug.h"
#include "framework_driver_shared.h"
#include "game_config.cpp"

internal game_render_return
game_render(game_render_params)
{//;switch_game_or_physics
 Render_Target *target = draw_get_target(app);
 render_movie(state, app, target, viewport_id, mouse); 
}

internal game_init_return
game_init(game_init_params)
{
 // API import
#define X(N) N = ed_api->N;
 X_ED_API_FUNCTIONS(X)
#undef X
 
 //@game_bootstrap_arena_zero_initialized
 Game_State *state = push_struct(bootstrap_arena, Game_State);
 state->permanent_arena = *bootstrap_arena;
 Arena *arena = &state->permanent_arena;
 
 {// NOTE: Save/Load business load_game
  Scratch_Block scratch(app);
  String binary_dir = system_get_path(scratch, SystemPath_BinaryDirectory);
  state->save_dir  = pjoin(arena, binary_dir, "data");
  state->save_path_text = pjoin(arena, state->save_dir, "text.kv");
  
  {// NOTE: Load text
   String read_string = read_entire_file(scratch, state->save_path_text);
   if( read_string.len )
   {
    if ( !deserialize_state(&state->save, read_string) )
    { print_message(app, strlit("Game load: deserialization!\n")); }
   }
   else {print_message(app, strlit("Game load: can't read the file!\n"));}
  }
  
  // NOTE: No matter what, init the save data
  state->save.magic_number = data_magic_number;
  state->save.version      = data_current_version;
 }
 
 for_i32(viewport_index,0,GAME_VIEWPORT_COUNT)
 {//;frame_arena_init
  usize frame_arena_size = KB(32);
  state->viewports[viewport_index].frame_arena = sub_arena_static(arena, frame_arena_size);
 }
 
 state->line_cap  = 8192;
 state->line_map = push_array(arena, Line_Map_Entry, state->line_cap);
 state->new_slider_store = sub_arena_static(arena, KB(32), 1);
 
 state->curve_count = 1;
 
 //- NOTE: Do things you *would* do every reload (clear arena, etc.)
 game_reload(state, ed_api);
 
 return state;
}

internal game_reload_return
game_reload(game_reload_params)
{// Game_State
 // API import
#define X(N) N = ed_api->N;
 X_ED_API_FUNCTIONS(X)
#undef X
 
 {//NOTE: Entirely BS
#define X(T)  fui_type_sizes[Fui_Type_##T] = sizeof(T);
  X_Fui_Types(X)
#undef X
 }
 
 selected_obj_id = state->selected_obj_id;
 
 {//NOTE: fui situation
  line_map         = state->line_map;
  new_slider_store = &state->new_slider_store;
  block_zero(line_map, state->line_cap*sizeof(Line_Map_Entry));
  static_arena_clear(new_slider_store);
 }
 
#if 0
 {// NOTE: re-make meshes
  make_meshes(&state->mesh_arena);
 }
#endif
 
 {//-NOTE: Mocking editor code
  state->vertex_count = 4;
  Vertex_Data *v = state->vertices;
  // v[0] is unused
  v[1].pos = V3(1,0,0);
  v[2].pos = V3(2,0,0);
  v[3].pos = V3(3,0,0);
 }
}

#include "time.h"

global Printer serializer;

internal String
time_format(char *buf, i32 bufsize, char *format)
{
 String result = {};
 time_t rawtime;
 time(&rawtime);
 struct tm *timeinfo = localtime(&rawtime);
 size_t strftime_result = strftime(buf, bufsize, format, timeinfo);
 if (strftime_result != 0)
 {
  result = SCu8(buf);
 }
 return result;
}

internal String
serialize_state(Arena *parent_arena, Game_Save *save)
{
#define print(...)            push_stringf(serializer, ##__VA_ARGS__)
 // NOTE: lovely MSVC doesn't pass __VA_ARGS__ correctly between macros
#define println(FORMAT, ...)  push_stringf(serializer, FORMAT "\n", ##__VA_ARGS__)
#define brace_block           println("{"); defer( println("}"); );
 
 usize MAX_SAVE_SIZE = KB(16);  // Wastes @Memory
 Arena arena_value = sub_arena_static(parent_arena, MAX_SAVE_SIZE, 1);
 Arena *arena = &arena_value;
 serializer.arena = arena_value;
 
 {// NOTE: preamble
  println( "// autodraw data file (DO NOT EDIT)" );
  {
   char buf[64];
   String timestamp = time_format(buf, alen(buf), "%d.%m.%Y %H:%M:%S");
   println("// Written at %.*s", string_expand(timestamp));
  }
  println("version %u", save->version);
 }
 
 println("cameras");
 for_i32(camera_index, 0, GAME_VIEWPORT_COUNT)
 {
  brace_block;
  Camera_Data *cam = &save->camera[camera_index];
  println("distance %f", cam->distance);
  println("phi %f",   cam->phi);
  println("theta %f", cam->theta);
  println("distance_level %d", cam->distance_level);
  println("roll %f", cam->roll);
  println("pan %f %f %f",   expand3(cam->pan));
  println("pivot %f %f %f", expand3(cam->pivot));
 }
 
 String result;
 Cursor cursor = arena->cursor_node->cursor;
 result.str  = cursor.base;
 result.size = cursor.pos;
 return result;
#undef print
#undef println
}

internal b32
save_game(App *app, Game_State *state)
{// NOTE: save and backup logic
 Scratch_Block scratch(app);
 char *save_path_text_cstr = to_c_string(scratch, state->save_path_text);
 
 b32 ok = true;
 if (!state->has_done_backup &&
     gb_file_exists(save_path_text_cstr))
 {
  String backup_dir = pjoin(scratch, state->save_dir, "backups");
  // NOTE: Backup game if is_first_write_since_launched
  char buf[64];
  String time_string = time_format(buf, alen(buf), "%d_%m_%Y_%H_%M_%S");
  if (time_string.len == 0)
  {
   print_message(app, str8lit("strftime failed... go figure that out!\n"));
   ok = false;
  }
  else
  {
   String backup_path = push_stringfz(scratch, "%.*s/data_%.*s.kv", string_expand(backup_dir), string_expand(time_string));
   ok = move_file(state->save_path_text, backup_path);
   if (ok) { state->has_done_backup = true; }
  }
  
  if (ok)
  {// NOTE: cycle out old backup files
   File_List backup_files = system_get_file_list(scratch, backup_dir);
   if (backup_files.count > 100)
   {
    u64 oldest_mtime = U64_MAX;
    String file_to_delete = {};
    File_Info **opl = backup_files.infos + backup_files.count;
    for (File_Info **backup = backup_files.infos;
         backup < opl;
         backup++)
    {
     File_Attributes attr = (*backup)->attributes;
     if (attr.last_write_time < oldest_mtime)
     {
      oldest_mtime   = attr.last_write_time;
      file_to_delete = pjoin(scratch, backup_dir, (*backup)->filename);
     }
    }
    b32 delete_ok = remove_file(file_to_delete);
    if (delete_ok) printf_message(app, "INFO: deleted backup file %.*s because it's too old", string_expand(file_to_delete));
    else           printf_message(app, "ERROR: failed to delete backup file %.*s", string_expand(file_to_delete));
   }
  }
 }
 
 if (ok)
 {// NOTE: save the file
  String text = serialize_state(scratch, &state->save);
  ok = write_entire_file(state->save_path_text, text.str, text.size);
  if (ok) {vim_set_bottom_text(str8lit("Saved game state!"));}
  else    {printf_message(app, "Failed to write to file %.*s", string_expand(state->save_path_text));}
 }
 
 return ok;
}

// TODO: Input handling: how about we add a callback to look at all the events and report to the game if we would process them or not?
internal game_update_return
game_update(game_update_params)
{
 b32 game_active = active_viewport_id != 0;
 b32 game_hot = (game_active != 0 || fui_is_active());
 
 i32 update_viewport_id = (active_viewport_id ? active_viewport_id : 1);
 
 Game_Input *input = &input_value;
 
 {
  Render_Target *render_target = draw_get_target(app);
  u32 hot_prim_id = render_target->current_prim_id;
  if(input->mouse.press_l)
  {
   if ( prim_id_is_obj(hot_prim_id) )
   {
    state->selected_obj_id = hot_prim_id;
    Primitive_Type type = prim_id_type(state->selected_obj_id);
    i32 selected_index = index_from_prim_id(hot_prim_id);
    if ( type == Prim_Vertex )
    {
     state->previous_vertex = state->vertices[selected_index];
    }
    else if (type == Prim_Curve)
    {
     state->previous_curve = state->curves[selected_index];
    }
   }
  }
  {
   i32 selected_index = index_from_prim_id(state->selected_obj_id);
   //DEBUG_VALUE(selected_index);
  }
 } 
 
 b32 should_animate_next_frame = false;
 kv_assert(active_viewport_id <= GAME_VIEWPORT_COUNT);
 i32 update_viewport_index = update_viewport_id - 1;
 Viewport *update_viewport = &state->viewports[update_viewport_index];
 
 v1 dt = input->frame.animation_dt;
 {
  state->time += dt;
  if (state->time >= 1000.0f) { state->time -= 1000.0f; }
 }
 
 if (game_hot)
 {
  should_animate_next_frame = true;
 }
 
 Arena *permanent_arena = &state->permanent_arena;
 
 Scratch_Block scratch(app);
 
 Game_Save *save = &state->save;
 if ( just_pressed(input, Key_Code_Return) )
 {
  save_game(app, state);
 }
 
 Camera camera_value = {};
 Camera *camera = &camera_value;
 {// NOTE: camera init
  setup_camera(camera, &update_viewport->camera);
 }
 
 if (game_hot)
 {// NOTE: Camera update
  Camera_Data *save_camera = &save->camera[update_viewport_index];
  
  if ( just_pressed(input, Key_Code_X) ) {
   save_camera->theta *= -1.f;
  } else if ( just_pressed(input, Key_Code_Z) ) {
   save_camera->theta = .5f - save_camera->theta;
  }
  
  v3 ctrl_direction = key_direction(input, Key_Mod_Ctl, true);
  if (ctrl_direction == V3())
  {
   if (game_active && !state->selected_obj_id)
   {// NOTE: Don't move the camera when you are selecting an object @UpdateSelectedObjects
    ctrl_direction = key_direction(input, 0, true);
   }
  }
  
  {// NOTE: Zoom levels
   i32 *level = &save_camera->distance_level;
   {
    v1 dz = ctrl_direction.z;
    if (dz > 0.f)      { (*level)++; }
    else if (dz < 0.f) { (*level)--; }
    macro_clamp_min((*level), 0);
   }
   {//note: Syncing the distance with the distance level
    v1 distance = 5.f*centimeter;
    v1 mult = 1.2f;
    for_i32(whatever,0,*level) { distance *= mult; }
    save_camera->distance = distance;
   }
  }
  
  {// NOTE: pan
   if( just_pressed(input, Key_Code_0, Key_Mod_Alt) )
   {
    save_camera->pan = {};
   }
   else
   {
    v1 distance = camera->distance;
    if (distance == 0) { distance = 0.5f; }
    v1 step = CAMERA_PAN_STEP_PER_DISTANCE * distance;  // distance = .92m -> 3*pivot_step
    v3 pan = save_camera->pan / step;
    
    v2 delta_pan = key_direction(input, Key_Mod_Alt, true).xy;
    pan += (delta_pan.x * camera->x.xyz + 
            delta_pan.y * camera->y.xyz);
    
    save_camera->pan = step*pan;
   }
  }
  
  {
   v1 interval = 1.0f / 24.f;
   v1 theta = roundv1(save_camera->theta / interval);
   v1 phi   = roundv1(save_camera->phi   / interval);
   {
    v2 delta = ctrl_direction.xy;
    theta += delta.x;
    phi   += delta.y;  // NOTE: pitch up when we go up
   }
   
   save_camera->theta = theta * interval;
   save_camera->phi   = phi * interval;
   macro_clamp(-0.25f, save_camera->phi, 0.25f);
  }
  
  if ( is_key_down(input, Key_Mod_Sft, Key_Code_0) ) { save_camera->roll  = {}; }
 }
 
 if (game_hot)
 {
  if( just_pressed(input, Key_Code_Space) )
  {
   game_last_preset(state, update_viewport_id);
  }
 }
 
 if ( fui_is_active() )
 {// NOTE: ;UpdateFuislider
  if (just_pressed(input, Key_Code_Tab))
  {
   b32 &zw = fui_v4_zw_active;
   zw = !zw;
  }
  
  Fui_Slider *slider = fui_active_slider;
  if (slider->type == Fui_Type_i1 || 
      slider->type == Fui_Type_i2 ||
      slider->type == Fui_Type_i3 ||
      slider->type == Fui_Type_i4)
  {
   i4 value;
   block_copy(&value, slider+1, slider_value_size(slider));
   v4 direction = key_direction_v4(input, 0, true);
   for_i32(index,0,4)
   {
    value.v[index] += i32(direction[index]);
   }
   
   if (slider->flags & Fui_Flag_Clamp_01)
   {
    for_i32(index,0,4)
    {
     macro_clamp01i(value.v[index])
    }
   }
   
   block_copy(slider+1, &value, slider_value_size(slider));
  }
  else
  {
   v4 value;
   block_copy(&value, slider+1, slider_value_size(slider));
   
   b32 j_pressed = just_pressed(input, Key_Code_J);
   b32 k_pressed = just_pressed(input, Key_Code_K);
   if (slider->type == Fui_Type_v1 && (j_pressed || k_pressed))
   {
    if (j_pressed) { value.x = 0.f; }
    if (k_pressed) { value.x = 1.f; }
   }
   else
   {
    b32 shifted;
    v4 direction = key_direction_v4(input, 0, false, &shifted);
    // NOTE: Switch active pair
    if (fui_v4_zw_active &&
        slider->type == Fui_Type_v4)
    {
     direction.zw = direction.xy;
     direction.xy = {};
    }
    if (slider->flags & Fui_Flag_Camera_Aligned)
    {
     direction = noz( camera->forward*direction );
    }
    v1 delta_scale = slider->delta_scale;
    if (delta_scale == 0) { delta_scale = 0.2f; }
    v4 delta = delta_scale * dt * direction;
    if (shifted) { delta *= 10.f; }
    value += delta;
    
    if (slider->flags & Fui_Flag_Clamp_X)  {value.x = 0;}
    if (slider->flags & Fui_Flag_Clamp_Y)  {value.y = 0;}
    if (slider->flags & Fui_Flag_Clamp_Z)  {value.z = 0;}
    
    if (slider->flags & Fui_Flag_NOZ)
    {
     value.xyz = noz(value.xyz);
    }
    
    if (slider->flags & Fui_Flag_Clamp_01)
    {
     for_i32(index,0,4)
     {
      macro_clamp01(value.v[index])
     }
    }
   }
   
   block_copy(slider+1, &value, slider_value_size(slider));
  }
 }
 
 {
  Camera_Data *cam = &update_viewport->camera;
  if (input->debug_camera_on)
  {
   DEBUG_NAME("camera(theta,phi,distance)", V3(cam->theta, cam->phi, camera->distance));
   DEBUG_NAME("camera(pan)", cam->pan);
  }
 }
 
 for_i32 (index, 0, GAME_VIEWPORT_COUNT)
 {// NOTE: Set viewport presets to useful values
  Viewport *viewport = &state->viewports[index];
  
  if (viewport->preset == viewport->last_preset)
  {
   if (viewport->preset == 0) { viewport->last_preset = 2; }
   else { viewport->last_preset = 0; }
  }
  
  static_arena_clear(&viewport->frame_arena);
 }
 
 movie_update(state);
 
 if ( game_hot )
 {// ;UpdateSelectedObjects
  Primitive_Type selected_type = prim_id_type(state->selected_obj_id);
  if ( selected_type )
  {
   b32 escape_pressed = just_pressed(input, Key_Code_Escape);
   b32 return_pressed = just_pressed(input, Key_Code_Return);
   i32 selected_index = index_from_prim_id(state->selected_obj_id);
   if( selected_type == Prim_Vertex )
   {
    Vertex_Data *vertex = &state->vertices[selected_index];
    if( escape_pressed )
    {// NOTE: Cancelling
     *vertex = state->previous_vertex;
     state->selected_obj_id = 0;
    }
    else if( return_pressed )
    {// NOTE: Committing
     state->selected_obj_id = 0;
    }
    else
    {
     v3 direction = key_direction(input, 0, false);
     direction.z = 0.f;
     direction = noz( mat4vec(camera->forward, direction) );
     v1 delta_scale = 0.2f;
     v3 delta = delta_scale * dt * direction;
     vertex->pos += delta;
    }
   }
   else if( selected_type == Prim_Curve )
   {// NOTE: curve update
    Bezier_Data *curve = &state->curves[selected_index];
    if ( escape_pressed )
    {
     *curve = state->previous_curve;
     state->selected_obj_id = 0;
    }
    else if (return_pressed)
    {
     state->selected_obj_id = 0;
    }
    else
    {
     // todo @incomplete
     i1 direction = i1( key_direction(input, 0, true).x );
     v1 delta = i2f6(direction);
     if (delta != 0)
     {
      for_i32(index, 0, 4)
      {
       curve->radii[index] += delta;
      }
     }
     DEBUG_VALUE(curve->radii);
    }
   }
  }
 }
 
 if (debug_frame_time_on)
 {
  DEBUG_NAME("work cycles", input->frame.work_cycles);
  DEBUG_NAME("work ms", input->frame.work_seconds * 1e3f);
 }
 
 if (0)  // nono We have support fbool in other files!
 {
  DEBUG_VALUE(image_load_info.image_count);
  DEBUG_VALUE(image_load_info.failure_count);
 }
 
 return should_animate_next_frame;
}

#undef WARN_DELTA
#undef viz_block
#undef funit

//~EOF
