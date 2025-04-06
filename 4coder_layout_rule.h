/*
4coder_layout_rule.h - Built in layout rule types.
*/

// TOP

#pragma once

struct Newline_Layout_Vars {
 i64 newline_character_index;
 b32 consuming_newline_characters;
 b32 prev_did_emit_newline;
};

struct Layout_State
{
 b32 is_math_layout;
 Arena *arena;
 Layout_Item_List list;
 Face_Advance_Map *advance_map;
 Face_ID face;
 Face_Metrics *metrics;
 f32 tab_width;
 f32 line_to_text_shift;
 
 v2 blank_dim;
 
 v2 p;
 f32 line_y;
 f32 text_y;
 f32 width;
};

struct Layout_Reflex{
 Layout_Item_List *list;
 Buffer_ID buffer;
 f32 width;
 Face_ID face;
};

// BOTTOM