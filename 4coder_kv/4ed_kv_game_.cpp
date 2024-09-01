#pragma once

global b8 global_game_key_states       [Key_Code_COUNT];
global u8 global_game_key_state_changes[Key_Code_COUNT];

global String GAME_DLL_PATH;
struct Game_DLL { u64 mtime; u32 temp_index; };
global Game_DLL current_game_dll;
global API_VTable_ed const_ed_api;
global b32 global_game_dll_lock;
global b32 global_game_enabled = true;  // NOTE: Prevent crash
global b32 global_auxiliary_viewports_on;
global b32 global_debug_camera_on;

function b32 
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

force_inline void turn_game_off()
{
 global_game_on_readonly = false;
}

function void
toggle_the_game(App *app)
{
 if (global_game_on_readonly) { turn_game_off(); }
 else                         { turn_game_on(); }
 
 if (global_game_on_readonly)
 {
  View_ID view = get_active_view(app, Access_Always);
  if ( is_view_to_the_right(app, view) )
  {// NOTE: switch to the left
   view = get_other_primary_view(app, view, Access_Always, true);
  }
  view_set_buffer_named(app, view, GAME_BUFFER_NAMES[0]);
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

void debug_camera_on(App *app);
CUSTOM_COMMAND_SIG(debug_camera_on)
CUSTOM_DOC("")
{
 global_debug_camera_on = !global_debug_camera_on;
}

internal void
init_game(App *app)
{
 // IMPORTANT: ;game_bootstrap_arena_zero_initialized
 // NOTE: Old static allocation code
 //u64 memsize = MB(256);
 //u8 *game_memory = cast(u8 *)system_memory_allocate(memsize, strlit("game memory"));
 Arena bootstrap_arena = make_arena_malloc(MB(1));
 auto game = get_game_code();
 Game_ImGui_State imgui_state;
 {
  imgui_state.ctx = ImGui::GetCurrentContext();
  ImGui::GetAllocatorFunctions(&imgui_state.alloc_func,
                               &imgui_state.free_func,
                               &imgui_state.user_data);
 }
 global_game_state = game->game_init(&bootstrap_arena, &const_ed_api, app,
                                     imgui_state);
 
 //@ReferenceImages
 stbi_set_flip_vertically_on_load(true);
}

function void win32_imgui_reinit(void);

function void
reload_game(Game_API &game)
{
 game.game_reload(global_game_state, &const_ed_api, false);
}

// TODO(kv): The "delete old file" operation sometimes fail on us, 
// so sometimes we have blank frames, which isn't ideal... but whatevs man!
function b32
load_latest_game_code(App *app, b32 *out_loaded)
{// NOTE(kv): Load dynamc game code
 if (global_game_on_readonly)
 {
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
      // NOTE(kv): Ping-pong temp DLL, to avoid hiccups
      u32 temp_index = 2;
      if (current_game_dll.temp_index == 2) { temp_index = 3; }
      
      String temp_path = (temp_index == 2) ? GAME2_DLL : GAME3_DLL;
      local_persist gbDllHandle library = {};
      
      ok = copy_file(GAME_DLL_PATH, temp_path, false);
      if (!ok) { vim_set_bottom_text_lit("failed to copy dll to a temp file"); }
      
      if (ok)
      {
       // NOTE(kv): We want to still display old game DLL for as long as possible, until load has succeeded.
       // So we can compare change results better and avoid black screens.
       gbDllHandle new_library = gb_dll_load( to_c_string(scratch, temp_path) );
       ok = (new_library != 0);
       if (!ok) { vim_set_bottom_text_lit("failed to load dll"); }
       
       if (ok)
       {
        if ( auto game = get_game_code() ) {
         game->game_shutdown();
        }
#if AD_SHUTDOWN_IMGUI
        win32_imgui_reinit();
#endif
        
        if (library)
        {
         b32 unload_ok = gb_dll_unload(library);
         if (!unload_ok) { vim_set_bottom_text_lit("failed to unload old dll"); }
        }
        
        library = new_library;
        
        auto &game = global_game_code_;
        auto game_api_export = (game_api_export_type *)gb_dll_proc_address(library, "game_api_export");
        game_api_export(game);
        if ( never_loaded_before ) {
         init_game(app);
        } else {
         // NOTE: "game_reload" is itself reloaded... not sure how that'd be useful
         reload_game(game);
        }
        
        current_game_dll = { mtime1, temp_index };
        loaded = true;
       }
      }
      
     }
    }
   }
  }
  
  if (out_loaded) {*out_loaded = loaded;}
  return ok;
 }
 else {return false;}
}

function i32
buffer_viewport_id(App *app, Buffer_ID buffer)
{
 i1 result = 0;
 Scratch_Block scratch(app);
 String bufname = push_buffer_base_name(app, scratch, buffer);
 for_i32 (index,0,GAME_BUFFER_COUNT)
 {
  if ( string_match(bufname, GAME_BUFFER_NAMES[index]) )
  {
   result = index+1;
   break;
  }
 }
 return result;
}

inline i1
view_viewport_id(App *app, View_ID view)
{
 Buffer_ID buffer = view_get_buffer(app, view, 0);
 return buffer_viewport_id(app, buffer);
}

inline Buffer_ID
get_active_buffer(App *app)
{
 View_ID active_view = get_active_view(app, Access_Always);
 return view_get_buffer(app, active_view, Access_Always);
}


force_inline i32
get_active_game_viewport_id(App *app)
{
 Buffer_ID buffer = get_active_buffer(app);
 return buffer_viewport_id(app, buffer);
}

function Image_Load_Info get_image_load_info(void);

function void
maybe_update_game(App *app, Frame_Info frame)
{
 if (global_game_on_readonly)
 {//-NOTE: Game update
  Game_API *game = get_game_code();
  b32 loaded;
  load_latest_game_code(app, &loaded);
  if (loaded) { vim_set_bottom_text_lit("Game code reloaded"); }
  
  if (game)
  {
   Scratch_Block scratch(app);
   Input_Modifier_Set set = system_get_keyboard_modifiers(scratch);
   i32 active_viewport_id = get_active_game_viewport_id(app);
   Game_Input input =
   {
    pack_modifiers(set.mods, set.count),
    global_game_key_states,
    global_game_key_state_changes,
    frame,
    global_debug_camera_on,
    get_mouse_state(app),
   };
   Image_Load_Info image_load_info = get_image_load_info();
   game_update_return update = game->game_update(global_game_state, app, active_viewport_id, input, image_load_info);
   if (update.should_animate_next_frame) { animate_next_frame(app); }
   received_game_commands = update.game_commands;
   
   block_zero_array(global_game_key_state_changes);
  }
 }
}

function void
render_game(App *app, Render_Target *target, i32 viewport, Frame_Info frame)
{
 if (global_game_on_readonly &&
     (viewport == MAIN_VIEWPORT_ID || global_auxiliary_viewports_on))
 {
  Game_API *game = get_game_code();
  if (game)
  {
   b32 should_animate_next_frame = game->game_viewport_update(global_game_state, viewport, frame.animation_dt);
   if (should_animate_next_frame) { animate_next_frame(app); }
   Render_Config old_render_config;
   {
    Render_Config *old_render_configp = target_last_config();
    if (old_render_configp) { old_render_config = *old_render_configp; }
    else { old_render_config = {}; }
   }
   game->game_render(global_game_state, app, target, viewport,
                     get_mouse_state(app));
   draw_configure(target, &old_render_config);
  }
 }
}

function void
command_game_set_preset(App *app)
{
 auto game = get_game_code();
 if (( game ))
 {
  User_Input input = get_current_input(app);
  auto event = &input.event;
  if (event->kind == InputEventKind_KeyStroke)
  {
   Key_Code code = event->key.code;
   i32 pressed_number = cast(i32)code - cast(i32)Key_Code_0;
   i32 viewport_id = get_active_game_viewport_id(app);
   game->game_set_preset(global_game_state, viewport_id, pressed_number);
  }
 }
}

function void
command_game_last_preset(App *app)
{
 auto game = get_game_code();
 if ( game )
 {
  i32 viewport_id = get_active_game_viewport_id(app);
  game->game_last_preset(global_game_state, viewport_id);
 }
}



//~ NOTE: EOF
