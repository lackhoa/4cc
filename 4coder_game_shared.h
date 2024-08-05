// NOTE: How it works: both sides have a vtable
// The game exports a single symbol "game_api_export",
// to fill the editor vtable for the game,
// The game can then export its vtable to the editor.

#pragma once

// TODO: I'm not happy with this global pointer business
// mostly because it's not clear if a function call is a pointer or not, it's just bad.
#if ED_API_USER
#    if ED_API_USER_STORE_GLOBAL  // NOTE: store function pointers here
#        define ED_FUNCTION(N) \
kv_function_typedef(N); kv_function_pointer(N);
#    else // NOTE: don't store function pointers in this TU
#        define ED_FUNCTION(N) \
kv_function_typedef(N); extern kv_function_pointer(N);
#    endif
//
#    undef ED_API_USER
#else  // NOTE: implementer
#    define ED_FUNCTION(N)  kv_function_declare(N);
#endif

#include "kv.h"
#include "4coder_types.h"
#include "4ed_render_target.h"
#include "game/fui_ed_api.h"
#include "4coder_kv_debug.h"
#include "4coder_token.h"
#include "4coder_events.h"

//~NOTE: 4ed API

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
//
#define lex_full_input_cpp_return Token_List
#define lex_full_input_cpp_params Arena *arena, String input
//
#define get_token_it_on_current_line_return Token_Iterator_Array
#define get_token_it_on_current_line_params App *app, Buffer_ID buffer, i64 *line_end_pos
//
#define draw_get_clip_return rect2
#define draw_get_clip_params void
//
#define get_hot_prim_id_return u32
#define get_hot_prim_id_params void
//
#define get_active_view_return View_ID
#define get_active_view_params App *app, Access_Flag access
//
#define is_view_to_the_right_return b32
#define is_view_to_the_right_params App *app, View_ID view
//
#define get_other_primary_view_return View_ID
#define get_other_primary_view_params App *app, View_ID start_view, Access_Flag access, b32 vsplit_if_fail
//
#define view_set_buffer_named_return void
#define view_set_buffer_named_params App *app, View_ID view, String8 name
//
#define view_set_cursor_return b32
#define view_set_cursor_params App *app, View_ID view_id, Buffer_Seek seek
//
#define seek_line_col_return Buffer_Seek
#define seek_line_col_params i64 line, i64 col

//-

// NOTE(kv): Whenever you wanna add a new editor API function,
// #define the function signature, then add it here
#define X_ED_API_FUNCTIONS(X) \
X(draw_get_target) \
X(print_message)   \
X(system_get_file_list) \
X(system_get_path) \
X(vim_set_bottom_text) \
X(change_active_primary_view) \
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
X(draw__push_vertices) \
X(push_image) \
X(push_object_transform_to_target) \
X(draw_configure) \
X(get_token_it_on_current_line) \
X(draw_get_clip) \
X(get_hot_prim_id) \
X(get_active_view) \
X(is_view_to_the_right) \
X(get_other_primary_view) \
X(view_set_buffer_named) \
X(view_set_cursor) \
X(seek_line_col) \

#define X(N)  ED_FUNCTION(N)
    X_ED_API_FUNCTIONS(X)
#undef X

#define X(N) kv_function_typedef(N);
    X_ED_API_FUNCTIONS(X)
#undef X

struct Ed_API
{
#define X(N) kv_function_pointer(N);
 X_ED_API_FUNCTIONS(X)
#undef X
};

//~

//TODO: clean this garbage pile up, please!
inline void
DEBUG_VALUE_inner(char *scope, char *name, rect2 value, argb color=0)
{
 Debug_Entry entry = {};
 entry.scope=scope;
 entry.name =name;
 entry.value=value.v4_value;
 entry.color=color;
 DEBUG_send_entry(entry);
}

inline void
DEBUG_VALUE_inner(char *scope, char *name, i1 value, argb color=0)
{
 Debug_Entry entry = {};
 entry.scope=scope;
 entry.name =name;
 entry.value.x=(f32)value;
 entry.color=color;
 DEBUG_send_entry(entry);
}

inline void
DEBUG_VALUE_inner(char *scope, char *name, u32 value, argb color=0)
{
 Debug_Entry entry = {};
 entry.scope=scope;
 entry.name=name;
 entry.value.x=(f32)value;
 entry.color=color;
 DEBUG_send_entry(entry);
}

inline void
DEBUG_VALUE_inner(char *scope, char *name, i2 value, argb color=0)
{
 Debug_Entry entry = {};
 entry.scope=scope;
 entry.name=name;
 entry.value.xy=V2(value);
 entry.color=color;
 DEBUG_send_entry(entry);
}

inline void
DEBUG_VALUE_inner(char *scope, char *name, v1 value, argb color=0)
{
 Debug_Entry entry = {};
 entry.scope=scope;
 entry.name=name;
 entry.value.x=value;
 entry.color=color;
 DEBUG_send_entry(entry);
}

inline void
DEBUG_VALUE_inner(char *scope, char *name, v2 value, argb color=0)
{
 Debug_Entry entry = {};
 entry.scope=scope;
 entry.name=name;
 entry.value.xy=value;
 entry.color=color;
 DEBUG_send_entry(entry);
}

inline void
DEBUG_VALUE_inner(char *scope, char *name, v3 v, argb color=0)
{
 v4 value = cast_V4(v);
 Debug_Entry entry = {};
 entry.scope=scope;
 entry.name=name;
 entry.value=value;
 entry.color=color;
 DEBUG_send_entry(entry);
}

inline void
DEBUG_VALUE_inner(char *scope, char *name, v4 v, argb color=0)
{
 Debug_Entry entry = {};
 entry.scope=scope;
 entry.name=name;
 entry.value=v;
 entry.color=color;
 DEBUG_send_entry(entry);
}


//~ NOTE: Game

struct Game_Input
{
 Key_Mods active_mods;
 b8      *key_states;
 u8      *key_state_changes;
 Frame_Info frame;
 b32 debug_camera_on;
 Mouse_State mouse;
};

struct Image_Load_Info
{
 i32 image_count;
 i32 failure_count;
};

global_const i32 GAME_VIEWPORT_COUNT = 3;
global_const i32 MAIN_VIEWPORT_ID    = 1;
global_const String GAME_FILE_NAME = strlit("game.cpp");

//-NOTE: game API functions
#define game_reload_params struct Game_State *state, Ed_API *ed_api
#define game_reload_return void
// @game_bootstrap_arena_zero_initialized
#define game_init_params Arena *bootstrap_arena, Ed_API *ed_api, App *app
#define game_init_return struct Game_State *
//
#define game_update_return b32
#define game_update_params Game_State *state, App *app, i1 active_viewport_id, Game_Input input_value, Image_Load_Info image_load_info
//
#define game_render_return void
#define game_render_params Game_State *state, App *app, Render_Target *target, i1 viewport_id, Mouse_State mouse
//
#define game_viewport_update_return b32
#define game_viewport_update_params Game_State *state, i1 viewport_id, v1 dt
//
#define get_indicator_level_return i1
#define get_indicator_level_params Game_State *state
//
#define set_indicator_level_return void
#define set_indicator_level_params Game_State *state, i1 level
//
#define game_set_preset_return void
#define game_set_preset_params Game_State *state, i1 viewport_id, i1 preset
//
#define game_last_preset_return void
#define game_last_preset_params Game_State *state, i1 viewport_id
//
#define is_event_handled_by_game_return  b32
#define is_event_handled_by_game_params  App *app, Input_Event *event, b32 game_hot
//-Game API function

#define X_GAME_API_FUNCTIONS(X) \
X(game_init)                \
X(game_update)              \
X(game_viewport_update)     \
X(game_render)              \
X(game_reload)              \
X(fui_is_active)    \
X(fui_at_slider_p)          \
X(fui_push_active_slider_value) \
X(fui_handle_slider)        \
X(set_indicator_level)      \
X(get_indicator_level)      \
X(game_set_preset)          \
X(game_last_preset)         \
X(is_event_handled_by_game) \

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

#if !AD_IS_DRIVER
inline void
printf_message(App *app, char *format, ...)
{
 va_list args;
 va_start(args, format);
 
 Scratch_Block scratch(app);
 String string = push_stringfv(scratch, format, args, true);
 print_message(app, string);
 
 va_end(args);
}
#endif

internal Key_Mods
pack_modifiers(Key_Code *mods, u32 count)
{
 Key_Mod result = (Key_Mod)0;
 for_u32 (i,0,count)
 {
  Key_Code mod = mods[i];
  if (0){}
  else if(mod == Key_Code_Control){ result = (Key_Mod)((u32)result|Key_Mod_Ctl); }
  else if(mod == Key_Code_Shift)  { result = (Key_Mod)((u32)result|Key_Mod_Sft); }
  else if(mod == Key_Code_Alt)    { result = (Key_Mod)((u32)result|Key_Mod_Alt); }
  else if(mod == Key_Code_Command){ result = (Key_Mod)((u32)result|Key_Mod_Cmd); }
  else if(mod == Key_Code_Menu)   { result = (Key_Mod)((u32)result|Key_Mod_Mnu); }
 }
 return result;
}

//-
#if !AD_IS_DRIVER
inline Scratch_Block::Scratch_Block(App *app){
 Thread_Context *t = this->tctx = get_thread_context(app);
 this->arena = tctx_reserve(t);
 this->temp = begin_temp(this->arena);
}

inline Scratch_Block::Scratch_Block(App *app, Arena *a1){
 Thread_Context *t = this->tctx = get_thread_context(app);
 this->arena = tctx_reserve(t, a1);
 this->temp = begin_temp(this->arena);
}

inline Scratch_Block::Scratch_Block(App *app, Arena *a1, Arena *a2){
 Thread_Context *t = this->tctx = get_thread_context(app);
 this->arena = tctx_reserve(t, a1, a2);
 this->temp = begin_temp(this->arena);
}

inline Scratch_Block::Scratch_Block(App *app, Arena *a1, Arena *a2, Arena *a3){
 Thread_Context *t = this->tctx = get_thread_context(app);
 this->arena = tctx_reserve(t, a1, a2, a3);
 this->temp = begin_temp(this->arena);
}
#endif
//-



//-

#define vim_set_bottom_text_lit(msg) vim_set_bottom_text(strlit(msg))
