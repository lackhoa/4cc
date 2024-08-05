#define AD_IS_FRAMEWORK 1
#include "kv.h"
#define AD_IS_GAME 1
#define ED_API_USER 1
#define ED_API_USER_STORE_GLOBAL 1
#define AD_IS_DRIVER 0
#include "4coder_game_shared.h"
#include "4ed_kv_parser.cpp"
#include "framework_driver_shared.h"
#define FUI_FAST_PATH 0
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

force_inline b32
camera_data_equal(Camera_Data *a, Camera_Data *b)
{
 return block_match(a, b, sizeof(Camera_Data));
}

internal v1
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
 {// NOTE: camera animtion
  Camera_Data *saved   = &viewport->target_camera;
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
load_state(Game_State *state, String input)
{
 Arena *load_arena = &state->load_arena;
 static_arena_clear(load_arena);
 STB_Parser parser = new_parser(input, load_arena, 128);
 STB_Parser *p = &parser;
 
 {
#define eat_brace_begin  eat_char(p, '{')
#define eat_brace_end    eat_char(p, '}')
#define eat_brace_block  eat_brace_begin; defer( eat_brace_end; )
  eat_id(p, "version");
  game_save_version = eat_i1(p);
  {
   eat_id(p, "cameras");
   {
    eat_brace_block;
    Camera_Data cameras[GAME_VIEWPORT_COUNT];
    for_i32(cam_index,0,GAME_VIEWPORT_COUNT)
    {
     Camera_Data *cam = &state->viewports[cam_index].target_camera;
     eat_brace_block;
#define X(TYPE,NAME)  eat_id(p, #NAME); cam->NAME = eat_##TYPE(p);
     X_Camera_Data(X)
#undef X 
    }
   }
  }
  {
   eat_id(p, "references_full_alpha");
   state->references_full_alpha = eat_i1(p);
  }
  {
   Modeler &modeler = state->modeler;
   {//-NOTE Vertices
    eat_id(p, "vertices");
    eat_brace_begin;
    kv_array<Vertex_Data> &vertices = modeler.vertices;
    while(p->ok())
    {
     if ( maybe_char(p, '}') ) {
      break;
     }
     Vertex_Data &v = vertices.push2();
     v = {};
     {
      eat_brace_begin;
#if 0  //TODO: some day we'll have to switch to a full meta program, some day!
      auto &meta = vertex_data_meta;
      for_i32(ifield, 0, meta.field_count)
      {
       eat_id(p, meta.field_names[ifield]);
       eat_type(p, (u8*)(v)+meta.field_offsets[ifield]);
      }
#endif
#define X(TYPE,NAME)  eat_id(p,#NAME);  v.NAME = eat_##TYPE(p);
      X_Vertex_Data(X);
#undef X
      eat_brace_end;
     }
    }
   }
   
   {//-NOTE Beziers
    eat_id(p, "beziers");
    eat_brace_begin;
    kv_array<Bezier_Data> &beziers = modeler.beziers;
    while(p->ok())
    {
     if ( maybe_char(p, '}') ) {
      break;
     }
     Bezier_Data &b = beziers.push2();
     b = {};
     {
      eat_brace_begin;
#define X(TYPE,NAME)   eat_id(p,#NAME);  b.NAME = eat_##TYPE(p);
      X_Bezier_Data(X);
#undef X
      eat_brace_end;
     }
    }
   }
  }
#undef eat_brace_begin
#undef eat_brace_end
#undef eat_brace_block
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
 
 return p->ok();
}


//~

internal game_render_return
game_render(game_render_params)
{//;switch_game_or_physics
 slider_cycle_counter = 0;
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
 
 {//-Modeler
  auto &m = state->modeler;
  m.permanent_arena = &state->permanent_arena;
  init(m.vertices, m.permanent_arena, 4096);
  init(m.beziers,   m.permanent_arena, 128);
  m.vertices.count = 1;
  m.beziers.count   = 1;
 }
 
 {// NOTE: Save/Load business load_game
  Scratch_Block scratch(app);
  String binary_dir = system_get_path(scratch, SystemPath_BinaryDirectory);
  state->save_dir  = pjoin(arena, binary_dir, "data");
  state->save_path_text = pjoin(arena, state->save_dir, "text.kv");
  
  {// NOTE: Load state
   state->load_arena = sub_arena_static(&state->permanent_arena, KB(32));
   String read_string = read_entire_file(scratch, state->save_path_text);
   if( read_string.len )
   {
    if ( !load_state(state, read_string) )
    {
     print_message(app, strlit("Game load: load failed!\n"));
     state->load_failed = true;
    }
   }
   else {print_message(app, strlit("Game load: can't read the file!\n"));}
  }
 }
 
 for_i32(viewport_index,0,GAME_VIEWPORT_COUNT)
 {//;frame_arena_init
  usize frame_arena_size = KB(32);
  state->viewports[viewport_index].frame_arena = sub_arena_static(arena, frame_arena_size);
 }
 
 {
  state->line_cap = 8192;
  state->line_map = push_array(arena, Line_Map_Entry, state->line_cap);
  state->slider_store = sub_arena_static(arena, KB(32), /*alignment*/1);
 }
 
 {
  i32 cap = 128;
  state->slow_slider_store = sub_arena_static(arena, KB(1),  /*alignment*/1);
  state->slow_line_map = {
   .cap = cap,
   .map = push_array(arena, Slow_Line_Map_Entry, cap)
  };
 }
 
 default_fvert_delta_scale = 0.1f;
  
 //-IMPORTANT: Reload is a part of init
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
 
 {//NOTE: ;FUI_reload
  line_map     =  state->line_map;
  slider_store = &state->slider_store;
  block_zero(line_map, state->line_cap*sizeof(Line_Map_Entry));
  static_arena_clear(slider_store);
  push_struct(slider_store, u8);// @fui_ensure_nonzero_offset
  //-
  slow_line_map     =  state->slow_line_map;
  slow_slider_store = &state->slow_slider_store;
  slow_line_map.count = 0;
  static_arena_clear(slow_slider_store);
  push_struct(slow_slider_store, u8);// @fui_ensure_nonzero_offset
 }
 
 {//-NOTE: Modeling data
  global_modeler = &state->modeler;
 }
}

#include "time.h"

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

inline Camera_Data *
get_target_camera(Game_State *state, i32 index)
{
 return &state->viewports[index].target_camera;
}

internal String
save_state(Arena *parent_arena, Game_State *state)
{
 // NOTE: lovely MSVC doesn't pass __VA_ARGS__ correctly between macros
 //TODO: Cleanup this macro hell
#define indent                print_nspaces(p, indentation)
#define println(FORMAT, ...)  push_stringf(p, FORMAT "\n", ##__VA_ARGS__); indent
#define newline               push_string(p, "\n"); indent
#define brace_begin           indentation+=1; println("{")
#define brace_end             indentation-=1; seek_back(p); println("}")
#define brace_block           brace_begin; defer(brace_end);
#define macro_print_field(STRUCT, TYPE, NAME) \
print(p, #NAME " "); print_data(p, Type_##TYPE, &STRUCT.NAME); newline
 
 const i32 MAX_SAVE_SIZE = KB(16);  // Wastes @Memory
 Printer p = make_printer(parent_arena, MAX_SAVE_SIZE);
 
 i32 indentation = 0;
 {// NOTE: Content
  {// NOTE: preamble
   println( "// autodraw data file (DO NOT EDIT)" );
   {
    char buf[64];
    String timestamp = time_format(buf, alen(buf), "%d.%m.%Y %H:%M:%S");
    println("// Written at %.*s", string_expand(timestamp));
   }
   println("version %u", data_current_version);
  }
  {
   println("cameras");
   {
    brace_block;
    for_i32(camera_index, 0, GAME_VIEWPORT_COUNT)
    {
     brace_block;
     Camera_Data &cam = state->viewports[camera_index].target_camera;
#define X(TYPE,NAME)  macro_print_field(cam, TYPE, NAME);
     X_Camera_Data(X);
#undef X
    }
   }
  }
  newline;
  println("references_full_alpha %d", state->references_full_alpha);
  newline;
  {// NOTE: Modeler data
   Modeler &modeler = state->modeler;
   {//NOTE vertices
    kv_array<Vertex_Data> &vertices = modeler.vertices;
    println("vertices");
    {
     brace_block;
     for_i32(vi,1,vertices.count)
     {
      brace_begin;
      auto &v = vertices[vi];
#define X(TYPE,NAME)  macro_print_field(v, TYPE, NAME);
      X_Vertex_Data(X);
#undef X
      brace_end;
     }
    }
    newline;
   }
   
   {//NOTE beziers
    kv_array<Bezier_Data> &beziers = modeler.beziers;
    println("beziers");
    {
     brace_block;
     for_i32(bi,1,beziers.count)
     {
      brace_begin;
      auto &b = beziers[bi];
#define X(TYPE,NAME)  macro_print_field(b, TYPE, NAME);
      X_Bezier_Data(X);
#undef X
      brace_end;
     }
    }
    newline;
   }
  }
  newline;
  println("//~EOF");
 }
 
 String result = end_printer(&p);
 return result;
 
#undef indent
#undef println
#undef brace_begin
#undef brace_end
#undef brace_block
#undef newline
#undef macro_print_field
}

internal b32
game_save(App *app, Game_State *state)
{// NOTE: save and backup logic
 Scratch_Block scratch(app);
 char *save_path_text_cstr = to_c_string(scratch, state->save_path_text);
 
 b32 ok = true;
 if (!state->has_done_backup &&
     gb_file_exists(save_path_text_cstr))
 {//-NOTE: backup situation
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
   ok = copy_file(state->save_path_text, backup_path, true);
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
 {//-NOTE: Save the file
  String text = save_state(scratch, state);
  ok = write_entire_file(state->save_path_text, text.str, text.size);
  if (ok) {vim_set_bottom_text(str8lit("Saved game state!"));}
  else    {printf_message(app, "Failed to write to file %.*s", string_expand(state->save_path_text));}
 }
 
 if (!ok) { state->save_failed = true; }
 
 return ok;
}

// TODO: Input handling: how about we add a callback to look at all the events and report to the game if we would process them or not?
internal game_update_return
game_update(game_update_params)
{
 auto &modeler = state->modeler;
 b32 game_active = active_viewport_id != 0;
 b32 game_hot = (game_active != 0 || fui_is_active());
 
 i32 update_viewport_id = (active_viewport_id ? active_viewport_id : 1);
 
 Game_Input *input = &input_value;
 
 {
  Render_Target *render_target = draw_get_target(app);
  if(input->mouse.press_l)
  {
   u32 hot_prim = get_hot_prim_id();
   if ( prim_id_is_obj(hot_prim) )
   {
    modeler.selected_obj_id = hot_prim;
    Primitive_Type type = prim_id_type(hot_prim);
    i32 selected_index = index_from_prim_id(hot_prim);
    if ( type == Prim_Vertex ) {
     modeler.previous_vertex = modeler.vertices[selected_index];
    } else if (type == Prim_Curve) {
     modeler.previous_curve  = modeler.beziers[selected_index];
    }
   }
  }
  {
   i32 selected_index = index_from_prim_id(selected_object_id());
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
 
 if( just_pressed(input, Key_Code_Return) )
 {//NOTE: This is how I want it to work
  game_save(app, state);
 }
 
 Camera camera_value = {};
 Camera *camera = &camera_value;
 {// NOTE: camera init
  setup_camera(camera, &update_viewport->camera);
 }
 
 if (game_hot)
 {// NOTE: Camera update
  Camera_Data *save_camera = get_target_camera(state, update_viewport_index);
  
  if ( just_pressed(input, Key_Code_X) ) {
   save_camera->theta *= -1.f;
  } else if ( just_pressed(input, Key_Code_Z) ) {
   save_camera->theta = .5f - save_camera->theta;
  }
  
  v3 ctrl_direction = key_direction(input, Key_Mod_Ctl, true);
  if (ctrl_direction == V3())
  {
   if (game_active && !selected_object_id())
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
 
 {//-NOTE: Presets
  if( just_pressed(input, Key_Code_Space) )
  {
   game_last_preset(state, update_viewport_id);
  }
  
  if ( just_pressed(input, Key_Code_Q) )
  {
   state->references_full_alpha = !state->references_full_alpha;
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
  if (slider->type == Type_i1 || 
      slider->type == Type_i2 ||
      slider->type == Type_i3 ||
      slider->type == Type_i4)
  {
   i4 value;
   block_copy(&value, slider+1, slider_value_size(slider));
   v4 direction = key_direction_v4(input, 0, true);
   for_i32(index,0,4)
   {
    value.v[index] += i32(direction[index]);
   }
   
   if (slider->flags & Slider_Clamp_01)
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
   if (slider->type == Type_v1 && (j_pressed || k_pressed))
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
        slider->type == Type_v4)
    {
     direction.zw = direction.xy;
     direction.xy = {};
    }
    if (slider->flags & Slider_Camera_Aligned)
    {
     direction = noz( camera->forward*direction );
    }
    v1 delta_scale = slider->delta_scale;
    if (delta_scale == 0) { delta_scale = 0.2f; }
    v4 delta = delta_scale * dt * direction;
    if (shifted) { delta *= 10.f; }
    value += delta;
    
    if (slider->flags & Slider_Clamp_X)  {value.x = 0;}
    if (slider->flags & Slider_Clamp_Y)  {value.y = 0;}
    if (slider->flags & Slider_Clamp_Z)  {value.z = 0;}
    
    if (slider->flags & Slider_NOZ)
    {
     value.xyz = noz(value.xyz);
    }
    
    if (slider->flags & Slider_Clamp_01)
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
 
 driver_update(state);
 
 if ( game_hot )
 {// ;UpdateSelectedObjects
  Primitive_Type selected_type = prim_id_type(selected_object_id());
  if ( selected_type )
  {
   b32 escape_pressed = just_pressed(input, Key_Code_Escape);
   b32 return_pressed = just_pressed(input, Key_Code_Return);
   i32 selected_index = index_from_prim_id(selected_object_id());
   if( selected_type == Prim_Vertex )
   {
    Vertex_Data *vertex = &state->modeler.vertices[selected_index];
    if( escape_pressed )
    {// NOTE: Cancelling
     *vertex = modeler.previous_vertex;
     modeler.selected_obj_id = 0;
    }
    else if( return_pressed )
    {// NOTE: Committing
     modeler.selected_obj_id = 0;
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
    Bezier_Data *curve = &state->modeler.beziers[selected_index];
    if ( escape_pressed )
    {
     *curve = modeler.previous_curve;
     modeler.selected_obj_id = 0;
    }
    else if (return_pressed)
    {
     modeler.selected_obj_id = 0;
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
  DEBUG_NAME("slider_cycle_counter", slider_cycle_counter);
  DEBUG_NAME("work ms", input->frame.work_seconds * 1e3f);
 }
 
 if ( fbool(0) )
 {
  DEBUG_VALUE(image_load_info.image_count);
  DEBUG_VALUE(image_load_info.failure_count);
 }
 
 // TODO: Have a better error reporting story
 // Like, how do we turn these off? With a clear command?
 if (state->load_failed) {
  DEBUG_TEXT("Load failed!");
 }
 if (state->save_failed) {
  DEBUG_TEXT("Save failed!");
 }
 
 return should_animate_next_frame;
}

#undef WARN_DELTA
#undef hl_block
#undef funit

//~EOF
