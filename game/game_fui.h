// TODO @Cleanup The editor doesn't need all these so don't put them here?
#pragma once

#include "4coder_events.h"

typedef String Fui_Key;

struct Fui_Slider;

#define fui_get_active_slider_return Fui_Slider *
#define fui_get_active_slider_params void
//
#define fui_set_active_slider_return void
#define fui_set_active_slider_params Fui_Slider *slider
//
#define fui_save_value_return void
#define fui_save_value_params Fui_Slider *slider
//
#define fui_restore_value_return void
#define fui_restore_value_params Fui_Slider *slider
//
#define fui_push_slider_value_return String
#define fui_push_slider_value_params Arena *arena, Fui_Slider *slider, String at_string
//
#define fui_is_at_slider_return b32
#define fui_is_at_slider_params String at_string
//
#define fui_at_slider_p_return b32
#define fui_at_slider_p_params App *app, Buffer_ID buffer
//
#define fui_handle_slider_return b32
#define fui_handle_slider_params App *app, Buffer_ID buffer, String filename, i32 line_number
//-
