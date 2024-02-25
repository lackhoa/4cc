#ifndef FCODER_FLEURY_UBIQUITOUS_H
#define FCODER_FLEURY_UBIQUITOUS_H

#include <string.h>

enum keybinding_mode
{
	KeyBindingMode_0,
	KeyBindingMode_1,
    KeyBindingMode_2,
    KeyBindingMode_3,
    KeyBindingMode_MAX
};

global keybinding_mode GlobalKeybindingMode;
global Face_ID global_styled_title_face = 0;
global Face_ID global_styled_label_face = 0;
global Face_ID global_small_code_face = 0;
global Rect_f32 global_cursor_rect = {};
global Rect_f32 global_last_cursor_rect = {};
global Rect_f32 global_mark_rect = {};
global Rect_f32 global_last_mark_rect = {};
global b32 global_dark_mode = 1;
global b32 global_battery_saver = 0;

#define MemorySet                 memset
#define MemoryCopy                memcpy
#define CalculateCStringLength    strlen
#define S8Lit(s)                  string_u8_litexpr(s)

#endif // FCODER_FLEURY_UBIQUITOUS_H
