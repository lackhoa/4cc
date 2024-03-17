#pragma once

#if ED_API_USER_SIDE
#define ED_API_FUNCTION(N) kv_function_typedef(N); global kv_function_pointer(N);
#undef ED_API_USER_SIDE
#else
#define ED_API_FUNCTION(N) kv_function_declare(N);
#endif

#include "4coder_types.h"
#include "4ed_render_target.h"
#include "game/game_fui.h"
#include "4coder_kv_debug.h"
#include "4coder_token.h"
#include "4coder_events.cpp"

//NOTE: 4ed API

#define draw_rect_outline_return void
#define draw_rect_outline_params App *app, rect2 rect, v1 roundness, v1 thickness, ARGB_Color color, v1 depth
//
#define print_message_return void
#define print_message_params App *app, String message
//
#define system_get_file_list_return File_List
#define system_get_file_list_params Arena* arena, String directory
//
#define system_get_path_return String
#define system_get_path_params Arena* arena, System_Path_Code path_code
//
#define vim_set_bottom_text_return void
#define vim_set_bottom_text_params String message
//
#define change_active_primary_view_return void
#define change_active_primary_view_params App* app
//
#define get_face_metrics_return Face_Metrics
#define get_face_metrics_params App *app, Face_ID face_id
//
#define get_string_advance_return v1
#define get_string_advance_params App *app, Face_ID font_id, String str
//
#define draw_string_oriented_return v2
#define draw_string_oriented_params App *app, Face_ID font_id, ARGB_Color color, String8 str, v2 point, u32 flags, v2 delta
//
#define token_it_read_return Token *
#define token_it_read_params Token_Iterator_Array *it
//
#define push_token_lexeme_return String
#define push_token_lexeme_params App *app, Arena *arena, Buffer_ID buffer, Token *token
//
#define token_it_inc_return Token *
#define token_it_inc_params Token_Iterator_Array *it
//
#define token_it_dec_return Token *
#define token_it_dec_params Token_Iterator_Array *it
//
#define get_next_input_return User_Input
#define get_next_input_params App *app, Event_Property use_flags, Event_Property abort_flags
//
#define buffer_replace_range_return b32
#define buffer_replace_range_params App *app, Buffer_ID buffer_id, Range_i64 range, String string
//
#define token_it_at_cursor_return Token_Iterator_Array
#define token_it_at_cursor_params App *app
//
#define fui_editor_ui_loop_return b32
#define fui_editor_ui_loop_params App *app

//-

ED_API_FUNCTION(vim_set_bottom_text);
ED_API_FUNCTION(change_active_primary_view);
ED_API_FUNCTION(get_face_metrics);
ED_API_FUNCTION(get_string_advance);
ED_API_FUNCTION(draw_string_oriented);
ED_API_FUNCTION(system_get_file_list);
ED_API_FUNCTION(system_get_path);
ED_API_FUNCTION(token_it_read);
ED_API_FUNCTION(push_token_lexeme);
ED_API_FUNCTION(token_it_inc);
ED_API_FUNCTION(token_it_dec);
ED_API_FUNCTION(get_next_input);
ED_API_FUNCTION(buffer_replace_range);
ED_API_FUNCTION(token_it_at_cursor);
ED_API_FUNCTION(fui_editor_ui_loop);

// TODO @Cleanup We can remove stuff from here, if we're intelligent about it
#define X_ED_API_FUNCTIONS(X) \
X(draw_get_target) \
X(print_message)   \
X(system_get_file_list) \
X(system_get_path) \
X(vim_set_bottom_text) \
X(change_active_primary_view) \
X(get_face_metrics) \
X(get_string_advance) \
X(draw_string_oriented) \
X(DEBUG_send_entry) \
X(token_it_read) \
X(push_token_lexeme) \
X(token_it_inc) \
X(token_it_dec) \
X(get_next_input) \
X(buffer_replace_range) \
X(token_it_at_cursor) \
X(fui_editor_ui_loop) \

#define X(N) kv_function_typedef(N);
    X_ED_API_FUNCTIONS(X)
#undef X

struct Ed_API
{
#define X(N) kv_function_pointer(N);
    X_ED_API_FUNCTIONS(X)
#undef X
};

//~ NOTE: Game

struct Game_Input
{
 Key_Mods active_mods;
 b8      *key_states;
 u8      *key_state_changes;
 v1 dt;
 v1 raw_dt;
 b32 debug_camera_on;
};

#define GAME_VIEWPORT_COUNT 3
global i32 MAIN_VIEWPORT_ID = 1;

//-NOTE: Some game API functions
#define game_reload_params struct Game_State *state, Ed_API *ed_api
#define game_reload_return void
//
#define game_init_params Arena *bootstrap_arena, Ed_API *ed_api, App *app
#define game_init_return struct Game_State *
//
#define game_update_return b32
#define game_update_params Game_State *state, App *app, i32 active_viewport_id, Game_Input input_value
//
#define game_render_return void
#define game_render_params Game_State *state, App *app, i32 viewport_id, Mouse_State mouse, u32 *csg_buffer
//
#define game_viewport_update_return b32
#define game_viewport_update_params Game_State *state, i32 viewport_id, v1 dt
//
#define get_indicator_level_return i32
#define get_indicator_level_params Game_State *state
//
#define set_indicator_level_return void
#define set_indicator_level_params Game_State *state, i32 level
//
#define game_set_preset_return void
#define game_set_preset_params Game_State *state, i32 viewport_id, i32 preset
//
#define game_last_preset_return void
#define game_last_preset_params Game_State *state, i32 viewport_id
//
#define is_key_handled_by_game_return  b32
#define is_key_handled_by_game_params  App *app, Input_Event *event
//-

#define X_GAME_API_FUNCTIONS(X) \
    X(game_init)                \
    X(game_update)              \
    X(game_viewport_update)     \
    X(game_render)              \
    X(game_reload)              \
    X(fui_get_active_slider)    \
    X(fui_set_active_slider)    \
    X(fui_save_value)           \
    X(fui_restore_value)        \
    X(fui_push_slider_value)    \
    X(fui_at_slider_p)          \
    X(fui_handle_slider)        \
    X(set_indicator_level)      \
    X(get_indicator_level)      \
    X(game_set_preset)          \
    X(game_last_preset)         \
    X(is_key_handled_by_game) \

#define X(N) kv_function_typedef(N);
    X_GAME_API_FUNCTIONS(X);
#undef X

struct Game_API
{
    b32 is_valid;
#define X(N) N##_type *N;
    X_GAME_API_FUNCTIONS(X);
#undef X
};

#define game_api_export_return void
#define game_api_export_params Game_API *api
typedef game_api_export_return game_api_export_type(game_api_export_params);

//~

ED_API_FUNCTION(print_message)

inline void
printf_message(App *app, char *format, ...)
{
    va_list args;
    va_start(args, format);
    
    Scratch_Block scratch(app);
    String string = push_stringfv(scratch, format, args);
    print_message(app, string);
    
    va_end(args);
}

#define vim_set_bottom_text_lit(msg) vim_set_bottom_text(strlit(msg))
