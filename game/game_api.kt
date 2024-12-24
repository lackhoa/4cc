//-
api ed_api
{
 b32 fui_is_active(void);
 String fui_push_active_slider_value(Arena *arena);
 b32 fui_at_slider_p(App *app);
 b32 fui_handle_slider(App *app);
 u32 fui_get_sliders_in_range(App *app, Buffer_ID buffer, i64 pos_begin, i64 pos_end, u32 *out_end_index);
 Range_i64 fui_get_slider_range(u32 index);
 b32 fui_is_buffer_synced(Game_State *state, App *app, Buffer_ID buffer);
 //-
 void game_reload(Game_State *state, API_VTable_ed *ed_api, API_VTable_ed_new *ed_api_new, b32 first_time);
 // @game_bootstrap_arena_zero_initialized
 Game_State *game_init(Arena *bootstrap_arena, API_VTable_ed *ed_api, API_VTable_ed_new *ed_api_new, App *app, Game_ImGui_State &imgui_state);
 void game_shutdown(Game_State *state);
 game_update_return game_update(Game_State *state, App *app, i1 active_viewport_id, Game_Input_Public &input_public, Image_Load_Info image_load_info);
 void game_render(Game_State *state, App *app, Render_Target *target, i1 viewport_id, Mouse_State mouse, rect2 clip_box);
 b32 game_viewport_update  (Game_State *state, i1 viewport_id, v1 dt);
 // TODO(kv): @cleanup These API calls are not needed,
 // just let the game handle keyboard events by itself!
 void game_set_preset(Game_State *state, i1 viewport_id, i1 preset);
 void game_last_preset(Game_State *state, i1 viewport_id);
 b32 is_event_handled_by_game(Game_State *state, App *app, Input_Event *event, b32 game_hot, b32 game_rendered);
 void game_send_command(Game_State *state, String command_name);
 void game_buffer_edit_range(Game_State *state, App *app, Buffer_ID buffer, Range_i64 new_range, Range_Cursor old_cursor_range);
}

gen_file "game_api.gen.h"
{
 gen_for(ed_api)
 {
#define `(name)__return `return
#define `(name)__params `params
  
 }
 
#define game_api_xlist(X) \
gen_for(ed_api)
 {
  X(`name) \
 }
}
//-