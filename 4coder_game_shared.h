// NOTE: How it works: both sides have a vtable
// The game exports a single symbol "game_api_export",
// to fill the editor vtable for the game,
// The game can then export its vtable to the editor.

#if ED_API_USER
#    if ED_API_USER_STORE_GLOBAL  // NOTE: store function pointers here
#        define ED_FUNCTION(N) kv_function_typedef(N); kv_function_pointer(N);
#    else // NOTE: don't store function pointers in this TU
#        define ED_FUNCTION(N) kv_function_typedef(N); extern kv_function_pointer(N);
#    endif
#else  // NOTE: implementer
#    define ED_FUNCTION(N)  kv_function_declare(N);
#endif

#ifndef AD_IS_GAME
#    define AD_IS_EDITOR
#endif

#define AD_SHUTDOWN_IMGUI 1  // NOTE(kv): Because I'm still not sure what this is for?

#include "kv.h"
#include "4coder_types.h"
#include "4ed_render_target.h"
#include "4coder_kv_debug.h"
#include "4coder_token.h"
#define ED_PARSER_BUFFER 1
#include "4ed_kv_parser.h"
#include "4coder_events.h"
#include "4coder_system_types.h"

#if ED_API_USER
#    define DYNAMIC_LINK_API
#    if AD_IS_DRIVER
#        define STORAGE_CLASS extern
#    else
#        define STORAGE_CLASS xglobal
#    endif
#else
#    define STATIC_LINK_API
#endif
#include "custom/generated/ed_api.h"

//~NOTE: 4ed API

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
DEBUG_VALUE_inner(char *scope, char *name, v4 v, argb color=0){
 Debug_Entry entry = {};
 entry.scope=scope;
 entry.name=name;
 entry.value=v;
 entry.color=color;
 DEBUG_send_entry(entry);
}
//~ NOTE: Game
#if !AD_IS_DRIVER
struct Game_Input_Public{
 Key_Mods active_mods;
 b8      *key_states;
 u8      *key_state_changes;
 Frame_Info frame;
 b32 debug_camera_on;
 Mouse_State mouse;
};

struct Game_ImGui_State {
 ImGuiContext* ctx;
 ImGuiMemAllocFunc alloc_func;
 ImGuiMemFreeFunc  free_func;
 void*             user_data;
};

struct Image_Load_Info {
 i32 image_count;
 i32 failure_count;
};
#endif

#define GAME_VIEWPORT_COUNT 3
global i32 MAIN_VIEWPORT_ID    = 1;
global String DRIVER_FILE_NAME = strlit("driver.kc");

struct API_VTable_ed_new{
 memory_functions_xlist(x_wrap_function_pointer);
};
#if ED_API_USER
function void
ed_api_read_vtable_new(API_VTable_ed_new *table){
#define x_read(N) N = table->N;
 memory_functions_xlist(x_read);
#undef x_read
}
#else
function void
ed_api_fill_vtable_new(API_VTable_ed_new *table){
#define x_fill(N) table->N = N;
 memory_functions_xlist(x_fill);
#undef x_fill
}
#endif

#if !AD_IS_DRIVER
struct Game_State;

#define fui_is_active__return b32
#define fui_is_active__params void
//
#define fui_push_active_slider_value__return String
#define fui_push_active_slider_value__params Arena *arena
//
#define fui_at_slider_p__return i64
#define fui_at_slider_p__params App *app, Buffer_ID buffer, Token_Iterator_Array *it_out
//
#define fui_handle_slider__return b32
#define fui_handle_slider__params App *app, Buffer_ID buffer, String filename, i1 line_number
//
#define fui_generate_slider__return b32
#define fui_generate_slider__params App *app

//-NOTE: game API functions (NOTE: The API is quite simple so let's just macro for now)
#define game_reload__return void
#define game_reload__params \
Game_State *state, API_VTable_ed *ed_api, API_VTable_ed_new *ed_api_new, b32 first_time
// @game_bootstrap_arena_zero_initialized
#define game_init__return Game_State *
#define game_init__params \
Arena *bootstrap_arena, API_VTable_ed *ed_api, API_VTable_ed_new *ed_api_new, App *app, \
Game_ImGui_State &imgui_state
//
#define game_shutdown__return void
#define game_shutdown__params Game_State *state
//
struct game_update__return {
 b32 should_animate_next_frame;
 arrayof<String> game_commands;
};
#define game_update__params \
Game_State *state, App *app, i1 active_viewport_id, \
Game_Input_Public &input_public, Image_Load_Info image_load_info
//
#define game_render__return void
#define game_render__params Game_State *state, App *app, Render_Target *target, i1 viewport_id, Mouse_State mouse, rect2 clip_box
//
#define game_viewport_update__return b32
#define game_viewport_update__params Game_State *state, i1 viewport_id, v1 dt
// TODO(kv): @cleanup These API calls are not needed,
// just let the game handle keyboard events by itself!
#define game_set_preset__return void
#define game_set_preset__params Game_State *state, i1 viewport_id, i1 preset
//
#define game_last_preset__return void
#define game_last_preset__params Game_State *state, i1 viewport_id
//
#define is_event_handled_by_game__return  b32
#define is_event_handled_by_game__params  App *app, Input_Event *event, b32 game_hot, b32 game_rendered
//
#define game_send_command__return void
#define game_send_command__params Game_State *state, String command_name

//-Game API function

#define X_GAME_API_FUNCTIONS(X) \
X(game_init)                \
X(game_shutdown)            \
X(game_update)              \
X(game_viewport_update)     \
X(game_render)              \
X(game_reload)              \
X(fui_is_active)            \
X(fui_at_slider_p)          \
X(fui_push_active_slider_value) \
X(fui_handle_slider)        \
X(fui_generate_slider)      \
X(game_set_preset)          \
X(game_last_preset)         \
X(is_event_handled_by_game) \
X(game_send_command)        \

struct Game_API
{
 b32 is_valid;
 X_GAME_API_FUNCTIONS(x_function_pointer);
};

#define game_api_export__return void
#define game_api_export__params Game_API &api
#endif

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

function Key_Mods
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
 init_scratch_block(this);
}
inline Scratch_Block::Scratch_Block(App *app, Arena *a1){
 init_scratch_block(this);
}
#endif
//-

inline Buffer_ID
get_active_buffer(App *app){
 View_ID active_view = get_active_view(app, Access_Always);
 return view_get_buffer(app, active_view, Access_Always);
}

#define vim_set_bottom_text_lit(msg) vim_set_bottom_text(strlit(msg))

#define GET_VIEW_AND_BUFFER \
View_ID   view = get_active_view(app, Access_ReadVisible); \
Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible); \
(void)view; (void)buffer

function Ed_Parser
make_ed_parser_at_cursor(App *app, Scan_Direction direction=Scan_Forward){
 GET_VIEW_AND_BUFFER;
 i64 curpos = view_get_cursor_pos(app, view);
 Token_Iterator_Array token_it = get_token_it_at_pos(app, buffer, curpos);
 Ed_Parser result = make_ep_from_buffer(app, buffer, token_it, 0, direction);
 return result;
}
//~