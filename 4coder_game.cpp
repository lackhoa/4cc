//-#processed
global b8 global_game_key_states       [Key_Code_COUNT];
global u8 global_game_key_state_changes[Key_Code_COUNT];

global Stringz GAME_DLL_PATH;
struct Game_DLL { u64 mtime; u32 temp_index; };
global Game_DLL current_game_dll;
global API_VTable_ed const_ed_api;
global API_VTable_ed_new const_ed_api_new;
global b32 global_game_dll_lock;
global b32 global_game_enabled = true;  // NOTE: Prevent crash
global b32 global_auxiliary_viewports_on;
global b32 global_debug_camera_on;

function b32 
turn_game_on() 
{
 b32 result = false;
 if(global_game_enabled)
 {
  game_status = Game_On;
  result = true;
 }
 else
 { 
  vim_set_bottom_text_lit("game is currently disabled!");
 }
 return result;
}
function void turn_game_on(App_Cmd *app) { turn_game_on(); }

myinline void
turn_game_off()
{
 game_status = Game_Off;
}
function void turn_game_off(App_Cmd *app) { turn_game_off(); }

function Buffer_ID
get_game_buffer(App *app, Viewport_ID viewport)
{
 Models *models = app_get_models(app);
 Buffer_ID result = 0;
 i32 index = viewport-1;
 if(index >= 0 and index < GAME_BUFFER_COUNT)
 {
  result = models->game_buffers[index];
 }
 return result;
}
function void
toggle_game_cmd(App_Cmd *app)
{// TODO(kv) Can we get rid of the "rendering" crap?
 // I mean we could just make a command to hide the game viewport -> no rendering.
 if(game_status >= Game_Rendering)
 {
  game_status = Game_On;
 }
 else
 {
  b32 turned_on = turn_game_on();
  if(turned_on)
  {
   game_status = Game_Rendering;
  }
 }
 
 if(game_status >= Game_Rendering)
 {
  View_ID view = get_active_view(app, Access_Always);
  if(is_view_to_the_right(app, view))
  {// NOTE: switch to the left
   view = get_other_primary_view(app, view, Access_Always, true);
  }
  view_set_buffer(app, view, get_game_buffer(app, 1), 0);
 }
}

function void 
toggle_game_auxiliary_viewports(App_Cmd *app)
{
 global_auxiliary_viewports_on = !global_auxiliary_viewports_on;
 if (global_auxiliary_viewports_on)
 {
  turn_game_on();
 }
}

function void 
game_enable(App_Cmd *app)
{
 global_game_enabled = true;
}
function void 
game_disable(App_Cmd *app)
{
 global_game_enabled = false;
 turn_game_off();
}

function void 
debug_camera_on(App_Cmd *app)
{
 global_debug_camera_on = !global_debug_camera_on;
}
function void
init_game(App *app)
{
 Arena bootstrap_arena = make_arena(MB(1));
 Game_API *game = get_game_code(Game_On);
 Game_ImGui_State imgui_state;
 {
  imgui_state.ctx = ImGui::GetCurrentContext();
  ImGui::GetAllocatorFunctions(&imgui_state.alloc_func,
                               &imgui_state.free_func,
                               &imgui_state.user_data);
 }
 b32 is_dev_editor = KV_INTERNAL;
 ed_game_state_pointer = game->game_init(&bootstrap_arena, &const_ed_api, &const_ed_api_new,
                                         app, imgui_state, is_dev_editor);
 
 // NOTE for reference images
 stbi_set_flip_vertically_on_load(true);
}

function void win32_imgui_reinit(void);

myinline void
reload_game(Game_API *game){
 game->game_reload(ed_game_state_pointer, &const_ed_api, &const_ed_api_new, false);
}
// TODO(kv): The "delete old file" operation sometimes fail on us, 
// so sometimes we have blank frames, which isn't ideal... but whatevs man!
function b32
load_latest_game_code(App *app, b32 *out_loaded)
{// NOTE(kv): Load dynamc game code
 if(is_game_on())
 {
  Scratch_Block scratch;
  b32 loaded = false;
  b32 ok = true;
  
  if ( !global_game_dll_lock )
  {
   String binary_dir = system_get_path(scratch, SystemPath_BinaryDirectory);
   //NOTE(kv) The lock is so that we won't try to load the game while pdb is still writing.
   //  https://guide.handmadehero.org/code/day180/#4346
   Stringz lock = pjoin(scratch, binary_dir, strlit("game.lock"));
   b32 lock_exists = file_mtime(lock) > 0;
   if(!lock_exists)
   {
#if KV_INTERNAL
# define PREFIX "dev_"
#else
# define PREFIX ""
#endif
    Stringz DLL2 = pjoin(scratch, binary_dir, strlit(PREFIX "game2.dll"));
    Stringz DLL3 = pjoin(scratch, binary_dir, strlit(PREFIX "game3.dll"));
#undef PREFIX
    
    b32 never_loaded_before = (current_game_dll.mtime == 0);
    u64 mtime_on_disk = file_mtime(GAME_DLL_PATH);
    ok = ok and (mtime_on_disk != 0);
    if(current_game_dll.mtime < mtime_on_disk)
    {// NOTE: We have new game code
     // NOTE(kv): Ping-pong temp DLL, to avoid hiccups
     u32 temp_index = 2;
     if (current_game_dll.temp_index == 2) { temp_index = 3; }
     
     Stringz temp_path = (temp_index == 2) ? DLL2 : DLL3;
     //NOTE(kv) Copy and not move because next run we might not have the code
     //TODO(kv) just have separate debug+release DLL and just do rename?
     //  We could just recompile the game, we always do that anyway.
     ok = ok and copy_file(GAME_DLL_PATH, temp_path, false);
     if(!ok){ vim_set_bottom_text_lit("failed to copy dll to a temp file"); }
     
     if(ok)
     {//NOTE(kv): We want to still display old game DLL for as long as possible,
      //  So we can compare change results better and avoid black screens.
      DLL_Handle new_library = gb_dll_load( to_cstring(temp_path) );
      ok = (new_library != 0);
      if(!ok){ vim_set_bottom_text_lit("failed to load dll"); }
      
      if(ok)
      {
       if(Game_API *game = get_game_code(Game_On))
       {
        game->game_shutdown(ed_game_state_pointer);
        game->is_valid = 0;
       }
       // win32_imgui_reinit(); TODO(kv) Do we really need to do this crap? Why do I gotta shut down anything?
       {
        ImGui::Begin("track imgui crash");
        ImGui::Text("reloaded!");  // TODO(kv) ;track_GetCurrentWindow_crash If this assertion fails, then the doc is wrong.
        ImGui::End();
       }
       
       local_persist DLL_Handle library = {};
       if(library){
        b32 unload_ok = gb_dll_unload(library);
        if (!unload_ok) { vim_set_bottom_text_lit("failed to unload old dll"); }
       }
       
       library = new_library;
       
       Game_API *game = &game_code_ro;
       void (*game_api_export)(Game_API *);
       cast_to(game_api_export, gb_dll_proc_address(library, "game_api_export"));
       game_api_export(game);
       if(never_loaded_before){
        init_game(app);
       }else{
        // NOTE: "game_reload" is itself reloaded... not sure how that'd be useful
        reload_game(game);
       }
       
       current_game_dll = { mtime_on_disk, temp_index };
       loaded = true;
       reloaded_game_this_frame = true;
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


myinline i1
view_viewport_id(App *app, View_ID view)
{
 Buffer_ID buffer = view_get_buffer(app, view, 0);
 return buffer_viewport_id(app, buffer);
}

function Image_Load_Info get_image_load_info(void);

function void
maybe_update_game(App *app, Frame_Info frame)
{
 Models *models = app_get_models(app);
 Scratch_Block tmp;
 if(is_game_on())
 {
  b32 loaded;
  load_latest_game_code(app, &loaded);
  if(loaded){ vim_set_bottom_text_lit("Game code reloaded"); }
  
  darray(Live_Viewport) live_viewports;
  init_dynamic(live_viewports, tmp);
  //
  if(is_game_rendering())
  {//-Gather viewports information
   Layout *layout = &models->layout;
   Live_Views *live_views = &models->view_set;
   for(Node *node = layout->open_panels.next;
       node != &layout->open_panels;
       node = node->next)
   {
    Panel *panel = CastFromMember(Panel, node, node);
    View *view_ptr = panel->view;
    View_ID view = view_get_id(live_views, view_ptr);
    Buffer_ID buffer = view_get_buffer(app, view, 0);
    Viewport_ID viewport = buffer_viewport_id(app, buffer);
    if(viewport == MAIN_VIEWPORT_ID or
       global_auxiliary_viewports_on)
    {
     Live_Viewport live_viewport = {};
     live_viewport.viewport = viewport;
     live_viewport.clip_box = view_get_screen_rect(app, view);
     live_viewport.target   = get_view_render_target(app, view);
     push(&live_viewports, live_viewport);
    }
   }
  }
  
  {//-Update
   Game_API *game = get_game_code(Game_On);
   if(game)
   {
    Input_Modifier_Set set = system_get_keyboard_modifiers(tmp);
    Game_Update_Params params = {
     .input = Game_Input_0{
      .active_mods       = pack_modifiers(set.mods, set.count),
      .key_states        = global_game_key_states,
      .key_state_changes = global_game_key_state_changes,
     },
     .mouse           = get_mouse_state(app),
     .state           = ed_game_state_pointer,
     .app             = app,
     .frame           = frame,
     .debug_camera_on = global_debug_camera_on,
     .live_viewports  = live_viewports,
    };
    Game_Update_Return update = game->game_update(params);
    
    if(update.should_animate_next_frame){ animate_next_frame(app); }
    received_game_commands = update.game_commands;
    
    block_zero_array(global_game_key_state_changes);
   }
  }
 }
}
//-
