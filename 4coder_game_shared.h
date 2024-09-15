// NOTE: How it works: both sides have a vtable
// The game exports a single symbol "game_api_export",
// to fill the editor vtable for the game,
// The game can then export its vtable to the editor.

#pragma once

// TODO: I'm not happy with this global pointer business
// mostly because it's not clear if a function call is a pointer or not.
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
#include "game/fui_ed_api.h"
#include "4coder_kv_debug.h"
#include "4coder_token.h"
#include "4coder_events.h"

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

#if !AD_IS_DRIVER
introspect(category=embedded)
struct Game_Input_Public {
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

struct Image_Load_Info
{
 i32 image_count;
 i32 failure_count;
};
#endif

global_const i32 GAME_VIEWPORT_COUNT = 3;
global_const i32 MAIN_VIEWPORT_ID    = 1;
global_const String GAME_FILE_NAME = strlit("game.cpp");

#if !AD_IS_DRIVER
//-NOTE: game API functions (NOTE: The API is quite simple so let's just macro for now)
#define game_reload_return void
#define game_reload_params struct Game_State *state, API_VTable_ed *ed_api, b32 first_time
// @game_bootstrap_arena_zero_initialized
#define game_init_return struct Game_State *
#define game_init_params \
Arena *bootstrap_arena, API_VTable_ed *ed_api, App *app, \
Game_ImGui_State &imgui_state
//
#define game_shutdown_return void
#define game_shutdown_params Game_State *state
//
struct game_update_return {
 b32 should_animate_next_frame;
 arrayof<String> game_commands;
};
#define game_update_params Game_State *state, App *app, i1 active_viewport_id, Game_Input_Public &input_public, Image_Load_Info image_load_info
//
#define game_render_return void
#define game_render_params Game_State *state, App *app, Render_Target *target, i1 viewport_id, Mouse_State mouse
//
#define game_viewport_update_return b32
#define game_viewport_update_params Game_State *state, i1 viewport_id, v1 dt
// TODO(kv): @cleanup These API calls are not needed,
// just let the game handle keyboard events by itself!
#define game_set_preset_return void
#define game_set_preset_params Game_State *state, i1 viewport_id, i1 preset
//
#define game_last_preset_return void
#define game_last_preset_params Game_State *state, i1 viewport_id
//
#define is_event_handled_by_game_return  b32
#define is_event_handled_by_game_params  App *app, Input_Event *event, b32 game_hot
//
#define game_send_command_return void
#define game_send_command_params Game_State *state, String command_name
//

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
X(game_set_preset)          \
X(game_last_preset)         \
X(is_event_handled_by_game) \
X(game_send_command)        \

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
#define game_api_export_params Game_API &api
typedef game_api_export_return game_api_export_type(game_api_export_params);
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

#define vim_set_bottom_text_lit(msg) vim_set_bottom_text(strlit(msg))

//~