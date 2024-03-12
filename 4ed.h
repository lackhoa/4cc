/*
 * Mr. 4th Dimention - Allen Webster
 *
 * 12.12.2014
 *
 * Application Layer for 4coder
 *
 */

#pragma once

#define MAX_VIEWS 16

struct Plat_Settings
{
    char *custom_dll;
    b8 custom_dll_is_strict;
    b8 fullscreen_window;
    
    i32 window_w;
    i32 window_h;
    i32 window_x;
    i32 window_y;
    b8 set_window_pos;
    b8 set_window_size;
    b8 maximize_window;
    
    b8 use_hinting;
    
    char *user_directory;
};

//function _Get_Version_Type custom_get_version;
//function _Init_APIs_Type   custom_init_apis;

#include "4ed_cursor_codes.h"

struct Application_Step_Result
{
    Application_Mouse_Cursor mouse_cursor_type;
    b32 lctrl_lalt_is_altgr;
    b32 perform_kill;
    b32 animating;
    b32 has_new_title;
    char *title_string;
};

struct Application_Step_Input
{
    b32 first_step;
    f32 dt;
    Mouse_State mouse;
    Input_List events;
    String_Const_u8 clipboard;
    b32 trying_to_kill;
};

typedef b32 Log_Function(String_Const_u8 str);
typedef Log_Function *App_Get_Logger(void);
