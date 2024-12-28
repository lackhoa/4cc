/* 
* Default color slots
*/

#pragma once

global Managed_ID defcolor_bar; //colors
global Managed_ID defcolor_base; //colors
global Managed_ID defcolor_pop1; //colors
global Managed_ID defcolor_pop2; //colors
global Managed_ID defcolor_back; //colors
global Managed_ID defcolor_margin; //colors
global Managed_ID defcolor_margin_hover; //colors
global Managed_ID defcolor_margin_active; //colors
global Managed_ID defcolor_list_item; //colors
global Managed_ID defcolor_list_item_hover; //colors
global Managed_ID defcolor_list_item_active; //colors
global Managed_ID defcolor_cursor; //colors
global Managed_ID defcolor_at_cursor; //colors
global Managed_ID defcolor_highlight_cursor_line; //colors
global Managed_ID defcolor_highlight; //colors
global Managed_ID defcolor_at_highlight; //colors
global Managed_ID defcolor_mark; //colors
global Managed_ID defcolor_text_default; //colors
global Managed_ID defcolor_comment; //colors
global Managed_ID defcolor_comment_pop; //colors
global Managed_ID defcolor_keyword; //colors
global Managed_ID defcolor_str_constant; //colors
global Managed_ID defcolor_char_constant; //colors
global Managed_ID defcolor_int_constant; //colors
global Managed_ID defcolor_float_constant; //colors
global Managed_ID defcolor_bool_constant; //colors
global Managed_ID defcolor_preproc; //colors
global Managed_ID defcolor_include; //colors
global Managed_ID defcolor_special_character; //colors
global Managed_ID defcolor_ghost_character; //colors
global Managed_ID defcolor_highlight_junk; //colors
global Managed_ID defcolor_highlight_white; //colors
global Managed_ID defcolor_paste; //colors
global Managed_ID defcolor_undo; //colors
global Managed_ID defcolor_back_cycle; //colors
global Managed_ID defcolor_text_cycle; //colors
global Managed_ID defcolor_line_numbers_back; //colors
global Managed_ID defcolor_line_numbers_text; //colors

api(custom)
struct Color_Array{
 ARGB_Color *vals;
 i1 count;
};

api(custom)
struct Color_Table{
    Color_Array *arrays;
    i1 count;
};

struct Color_Table_Node{
    Color_Table_Node *next;
    String name;
    Color_Table table;
};

struct Color_Table_List{
    Color_Table_Node *first;
    Color_Table_Node *last;
    i1 count;
};

global Color_Table active_color_table = {};
global Color_Table default_color_table = {};

global Arena global_theme_arena = {};
global Color_Table_List global_theme_list = {};
