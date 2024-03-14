#pragma once

#include "4coder_types.h"
#include "4ed_render_target.h"
#include "4ed_fui_user.h"

////////////////////////////////

#define draw_triangle_return void
#define draw_triangle_params App *app, v3 p0, v3 p1, v3 p2, ARGB_Color color
//
#define draw_rect_outline_return void
#define draw_rect_outline_params App *app, rect2 rect, v1 roundness, v1 thickness, ARGB_Color color, v1 depth
//
#define draw_get_target_return Render_Target *
#define draw_get_target_params App *app
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
#define view_is_active_return b32
#define view_is_active_params App *app, View_ID view
//
#define animate_in_n_milliseconds_return void
#define animate_in_n_milliseconds_params App *app, u32 n
//
#define change_active_primary_panel_return void
#define change_active_primary_panel_params App* app
//
#define get_face_metrics_return Face_Metrics
#define get_face_metrics_params App *app, Face_ID face_id
//
#define get_string_advance_return f32
#define get_string_advance_params App *app, Face_ID font_id, String str
//
#define draw_string_oriented_return v2
#define draw_string_oriented_params App *app, Face_ID font_id, ARGB_Color color, String8 str, v2 point, u32 flags, v2 delta

// TODO @Cleanup We can remove stuff from here, if we're intelligent about it
#define X_ED_API_FUNCTIONS(X) \
    X(draw_get_target) \
    X(print_message)   \
    X(system_get_file_list) \
    X(system_get_path) \
    X(vim_set_bottom_text) \
    X(animate_in_n_milliseconds) \
    X(change_active_primary_panel) \
    X(get_face_metrics) \
    X(get_string_advance) \
    X(draw_string_oriented) \
    X(fui_user_main)

#if 0
#define X(N) typedef N##_return N##_type(N##_params);
//
X_ED_API_FUNCTIONS(X)
//
#undef X
#endif

#if 0
struct Ed_API
{
#define X(N) N##_type *N;
    //
    X_ED_API_FUNCTIONS(X)
    //
#undef X
};
#endif

#ifdef API_USER_SIDE
//#define X(N) global N##_type *N;
#define X(N) DLL_IMPORT N##_return N(N##_params);
//
X_ED_API_FUNCTIONS(X)
//
#undef X
#undef API_USER_SIDE
#else
// NOTE: Forward declare
#define X(N) DLL_EXPORT N##_return N(N##_params);
    //
    X_ED_API_FUNCTIONS(X)
    //
#undef X
#endif

////////////////////////////////////////////////////////////////

struct Game_Input
{
    Key_Mod active_mods;
    b8     *key_states;
    u8     *key_state_changes;
    b32     view_active;
};

#define game_init_params Arena *bootstrap_arena, App *app
#define game_init_return struct Game_State *
typedef game_init_return game_init_type(game_init_params);
//
#define game_update_and_render_return void
#define game_update_and_render_params Game_State *state, App *app, View_ID view, v1 dt, Game_Input input
typedef game_update_and_render_return game_update_and_render_type(game_update_and_render_params);

#define X_GAME_API_FUNCTIONS(X) \
    X(game_init)                \
    X(game_update_and_render)

struct Game_API
{
#define X(N) N##_type *N;
    //
    X_GAME_API_FUNCTIONS(X);
    //
#undef X
};

#define game_api_export_return void
#define game_api_export_params Game_API *api
typedef game_api_export_return game_api_export_type(game_api_export_params);

////////////////////////////////////////////////////

// TODO(kv): I do not know of any way to put this in "4ed_fui_user.h".
// NOTE: We define overloads to avoid having to specify the type as a separate argument 
// as well as receive the appropriate values as output.
#define X(T) \
\
internal T fui_user_type(String id, T init_value_T, Fui_Options options={}) \
{ \
    Fui_Type  type = Fui_Type_##T; \
    Fui_Value init_value = { .T = init_value_T }; \
    Fui_Value value = fui_user_main(id, type, init_value, options); \
    return value.T; \
}
//
X_Fui_Types(X)
//
#undef X

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

force_inline void animate_next_frame(App *app)
{
    animate_in_n_milliseconds(app, 0);
}

////////////////////////////////////////////////////
