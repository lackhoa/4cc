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

#include "4coder_custom_types.h"
#include "kv_math.h"
#include "4ed_render_target.h"
#include "4coder_token.h"
#define ED_PARSER_BUFFER 1
#include "4ed_kv_parser.h"
#include "4coder_events.h"
#include "4coder_system_types.h"

typedef i32 Viewport_ID;

#if ED_API_USER
#    define DYNAMIC_LINK_API
#    define STORAGE_CLASS xglobal
#else
#    define STATIC_LINK_API
#endif


#include "ed_api.gen.h"

//~NOTE: 4ed API
#include "4coder_debug_value.h"
//~ NOTE: Game

struct Live_Viewport
{
 Viewport_ID id;
 rect2 clip_box;
 Render_Target *target;
};
struct Game_Input_0
{
 Key_Mods active_mods;
 b8      *key_states;
 u8      *key_state_changes;
};
struct Game_Update_Params
{// See @game_update
 Game_Input_0 input;
 Mouse_State mouse;
 struct Game_State *state;
 App *app;
 Frame_Info frame;
 b32 debug_camera_on;
 sarray(Live_Viewport) live_viewports;
 b32 game_was_turned_on_this_frame;
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
 i32 xx__failure_count;
};

#define GAME_VIEWPORT_COUNT 3
global i32 MAIN_VIEWPORT_ID    = 1;
global String DRIVER_FILE_NAME = strlit("driver.kc");

struct API_VTable_ed_new
{
#define X(N) wrap_function_pointer(N);
 memory_functions_xlist(X);
#undef X
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

#if 1
struct Game_State;

struct Game_Update_Return
{
 b32 should_animate_next_frame;
 sarray(String) game_commands;
};

#include "game/game_api.gen.h"

//-Game API function
struct Game_API
{
 b32 is_valid;
 game_api_xlist(x_function_pointer);
};
#endif

//~
function void
log_string(String string)
{
 log_string_core(string);
}

#if 1
// NOTE(kv) Log functions need to be quick and nimble,
//  we don't wanna be writing poems when logging errors.
function void
log_string(char *format, ...)
{
 Scratch_Scope tmp;
 va_list args;
 va_start(args, format);
 
 String message = push_stringfv(tmp, format, args);
 log_string(message);
 
 va_end(args);
}
function void
log_error(String message)
{
 Scratch_Scope tmp;
 message = strcat(tmp, strlit("ERROR: "), message);
 log_string_spam(message);
}
function void
log_error(char *format, ...)
{
 // TODO Hash the message string
 Scratch_Scope tmp;
 va_list args;
 va_start(args, format);
 
 String message = push_stringfv(tmp, format, args);
 log_error(message);
 
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
#if 1
inline Scratch_Block::Scratch_Block(App *app){
 init_scratch_block(this);
}
inline Scratch_Block::Scratch_Block(App *app, Arena *a1){
 init_scratch_block(this);
}
#endif
//-

#define vim_set_bottom_text_lit(msg) vim_set_bottom_text(strlit(msg))

#define GET_VIEW_AND_BUFFER \
View_ID   view = get_active_view(app, Access_ReadVisible); \
Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);

function Ed_Parser
make_ed_parser_at_cursor(App *app, Scan_Direction direction=Scan_Forward)
{
 GET_VIEW_AND_BUFFER;
 i64 curpos = view_get_cursor_pos(app, view);
 Token_Iterator_Array token_it = get_token_it_at_pos(app, buffer, curpos);
 Ed_Parser result = ed_parser_from_buffer(app, buffer, token_it, 0, direction);
 return result;
}
myinline Buffer_ID
get_active_buffer(App *app)
{
 View_ID active_view = get_active_view(app, Access_Always);
 return view_get_buffer(app, active_view, Access_Always);
}

// NOTE: Dummy buffers so we can use the same commands to switch to the rendered game
#define GAME_BUFFER_COUNT 3

function i32
get_active_game_viewport_id(App *app)
{
 Buffer_ID buffer = get_active_buffer(app);
 return buffer_viewport_id(app, buffer);
}

myinline void
draw_rect(App *app, rect2 rect, v1 roundness, ARGB_Color color, v1 depth)
{
 v2 dim = get_dim(rect);
 v1 thickness = Max(dim.x, dim.y);
 draw_rect_outline(app, rect, roundness, thickness, color, depth);
}
function void
im_text(char* fmt, ...)
{
 va_list args;
 va_start(args, fmt);
 im_textv(fmt, args);
 va_end(args);
}

myinline v2::operator ImVec2() { return *(ImVec2*)this; }
myinline ImVec2::operator v2() { return *(v2*)this; }
myinline v4::operator ImVec4() { return *(ImVec4*)this; }
myinline ImVec4::operator v4() { return *(v4*)this; }

function i64
get_line_number_from_pos(App *app, Buffer_ID buffer, i64 pos)
{
 Buffer_Cursor cursor = buffer_compute_cursor(app, buffer, seek_pos(pos));
 return(cursor.line);
}
//~