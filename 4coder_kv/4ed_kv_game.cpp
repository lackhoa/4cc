#pragma once

global b8 global_game_key_states       [Key_Code_COUNT];
global u8 global_game_key_state_changes[Key_Code_COUNT];

global Game_API global_game_code = {};
global String GAME_DLL_PATH;
global Game_State *global_game_state;
struct Game_DLL { u64 mtime; u32 temp_index; };
global Game_DLL current_game_dll;
global Ed_API const_ed_api;
global b32 global_game_dll_lock;
global b32 global_game_enabled = true;  // NOTE: Prevent crash
global b32 global_game_on_readonly;
global b32 global_auxiliary_viewports_on = true;
global b32 global_debug_camera_on;

void view_set_buffer_named(App *app, String8 name);

internal b32 
turn_game_on() 
{
 if (global_game_enabled)
 {
  global_game_on_readonly = true;
  return true;
 }
 else 
 { 
  vim_set_bottom_text_lit("game is currently disabled!");
  return false;
 }
}

force_inline void turn_game_off() { global_game_on_readonly = false; }

internal void
toggle_the_game(App *app)
{
 if (global_game_on_readonly) { turn_game_off(); }
 else                         { turn_game_on(); }
 
 if (global_game_on_readonly)
 {
  View_ID view = get_active_view(app, Access_Always);
  View_ID game_view = view;
  if ( is_view_to_the_right(app, view) )
  {// NOTE: switch to the left
   View_ID other_view = get_other_primary_view(app, view, Access_Always, true);
   game_view = other_view;
  }
  view_set_buffer_named(app, game_view, GAME_BUFFER_NAMES[0]);
 }
}

internal void 
toggle_game_auxiliary_viewports(App *app)
{
 global_auxiliary_viewports_on = !global_auxiliary_viewports_on;
 if (global_auxiliary_viewports_on)
 {
  turn_game_on();
 }
}

CUSTOM_COMMAND_SIG(game_enable)
CUSTOM_DOC("")
{
 global_game_enabled = true;
}

CUSTOM_COMMAND_SIG(game_disable)
CUSTOM_DOC("")
{
 global_game_enabled = false;
 turn_game_off();
}

force_inline b32
game_code_valid()
{
 return global_game_code.is_valid;
}

internal void
toggle_indicators(App *app)
{
 auto game = &global_game_code;
 if (game->is_valid)
 {
  if (game->get_indicator_level(global_game_state) > 0)
  {
   game->set_indicator_level(global_game_state, 0);
  }
  else 
  {
   game->set_indicator_level(global_game_state, 2);
  }
 }
}

internal void debug_camera_on(App *app);
CUSTOM_COMMAND_SIG(debug_camera_on)
CUSTOM_DOC("")
{
 global_debug_camera_on = !global_debug_camera_on;
}

global u32 *global_csg_buffers[GAME_VIEWPORT_COUNT];

internal void
init_game(App *app)
{
 Arena bootstrap_arena = make_arena_system();
 {
  i32 W = 1920;
  i32 H = 1080;
  for_i32(index,0,4)
  {
   global_csg_buffers[index] = push_array(&bootstrap_arena, u32, W*H);
  }
 }
 global_game_state = global_game_code.game_init(&bootstrap_arena, &const_ed_api, app);
}

// TODO: The "delete old file" operation sometimes fail on us, 
// so sometimes we have blank frames, which isn't ideal... but whatevs man!
internal b32
load_latest_game_code(App *app, b32 *out_loaded)
{// NOTE(kv): Load dynamc game code
 Scratch_Block scratch(app);
 b32 loaded = false;
 b32 ok = true;
 
 if ( !global_game_dll_lock )
 {
  u64 mtime1 = file_mtime(GAME_DLL_PATH);
  if (( ok = mtime1 != 0 ))
  {
   String binary_dir = system_get_path(scratch, SystemPath_BinaryDirectory);
   b32 lock_file_exists = file_mtime( pjoin(scratch, binary_dir, "game_dll.lock") ) > 0;
   if (!lock_file_exists)
   {
#if KV_INTERNAL
# define TEMP_DLL_PREFIX "dev_"
#else
# define TEMP_DLL_PREFIX ""
#endif
    String GAME2_DLL = pjoin(scratch, binary_dir, TEMP_DLL_PREFIX "game2.dll");
    String GAME3_DLL = pjoin(scratch, binary_dir, TEMP_DLL_PREFIX "game3.dll");
#undef TEMP_DLL_PREFIX
    
    b32 never_loaded_before = (current_game_dll.mtime == 0);
    if (never_loaded_before || 
        (current_game_dll.mtime < mtime1))
    {// NOTE: We have new game code
     // NOTE(kv): Ping-pong temp DLL, to avoid hickups
     u32 temp_index = 2;
     if (current_game_dll.temp_index == 2) { temp_index = 3; }
     
     String temp_path = (temp_index == 2) ? GAME2_DLL : GAME3_DLL;
     local_persist gbDllHandle library = {};
     
     if (( ok = copy_file(GAME_DLL_PATH, temp_path, false) ))
     {
      // NOTE(kv): We want to still display old game DLL for as long as possible, until load has succeeded.
      // So we can compare results better and avoid nasty black screens.
      gbDllHandle new_library = gb_dll_load( to_c_string(scratch, temp_path) );
      if (( ok = new_library != 0 ))
      {
       if (library)
       {
        ok = gb_dll_unload(library);
        if (!ok) { vim_set_bottom_text_lit("failed to unload old dll"); }
       }
       
       if (ok)
       {
        library = new_library;
        
        auto game_api_export = ((game_api_export_type *) gb_dll_proc_address(library, "game_api_export"));
        game_api_export(&global_game_code);
        if ( never_loaded_before )
        {
         init_game(app);
        }
        else
        {// NOTE: "game_reload" is itself reloaded
         global_game_code.game_reload(global_game_state, &const_ed_api);
        }
        
        current_game_dll = { mtime1, temp_index };
        loaded = true;
       }
      }
      else { vim_set_bottom_text_lit("failed to load dll"); }
     }
     else { vim_set_bottom_text_lit("failed to copy dll to a temp file"); }
    }
   }
  }
 }
 
 if (out_loaded) {*out_loaded = loaded;}
 return ok;
}

internal i32
get_buffer_game_viewport_id(App *app, Buffer_ID buffer)
{
 Scratch_Block scratch(app);
 String bufname = push_buffer_base_name(app, scratch, buffer);
 for_i32 (index,0,GAME_BUFFER_COUNT)
 {
  if ( string_match(bufname, GAME_BUFFER_NAMES[index]) )
  {
   return (index+1);
  }
 }
 return 0;
}

force_inline i32
get_active_game_viewport_id(App *app)
{
 Buffer_ID buffer = get_active_buffer(app);
 return get_buffer_game_viewport_id(app, buffer);
}


internal void
maybe_update_game(App *app, Frame_Info frame)
{
 if (global_game_on_readonly)
 {//-NOTE: Game update
  Game_API *game = &global_game_code;
  b32 loaded;
  load_latest_game_code(app, &loaded);
  if (loaded) { vim_set_bottom_text_lit("Game code reloaded"); }
  
  if (game->is_valid)
  {
   Scratch_Block scratch(app);
   Input_Modifier_Set set = system_get_keyboard_modifiers(scratch);
   i32 active_viewport_id = get_active_game_viewport_id(app);
   Game_Input input =
   {
    pack_modifiers(set.mods, set.count),
    global_game_key_states,
    global_game_key_state_changes,
    frame.animation_dt,
    frame.literal_dt,
    global_debug_camera_on,
   };
   b32 should_animate_next_frame = game->game_update(global_game_state, app, active_viewport_id, input);
   if (should_animate_next_frame) { animate_next_frame(app); }
   
   block_zero_array(global_game_key_state_changes);
  }
 }
}

internal void
render_game(App *app, i32 viewport, Frame_Info frame)
{
 if (global_game_on_readonly &&
     (viewport == MAIN_VIEWPORT_ID || global_auxiliary_viewports_on))
 {
  Game_API *game = &global_game_code;
  if (game->is_valid)
  {
   b32 should_animate_next_frame = game->game_viewport_update(global_game_state, viewport, frame.animation_dt);
   if (should_animate_next_frame) { animate_next_frame(app); }
   game->game_render(global_game_state, app, viewport, get_mouse_state(app),
                     global_csg_buffers[viewport-1]);
  }
 }
}

internal void
command_game_set_preset(App *app)
{
 if ( game_code_valid() )
 {
  User_Input input = get_current_input(app);
  auto event = &input.event;
  if (event->kind == InputEventKind_KeyStroke)
  {
   Key_Code code = event->key.code;
   i32 pressed_number = cast(i32)code - cast(i32)Key_Code_0;
   i32 viewport_id = get_active_game_viewport_id(app);
   global_game_code.game_set_preset(global_game_state, viewport_id, pressed_number);
  }
 }
}

internal void
command_game_last_preset(App *app)
{
 if ( game_code_valid() )
 {
  i32 viewport_id = get_active_game_viewport_id(app);
  global_game_code.game_last_preset(global_game_state, viewport_id);
 }
}

//~