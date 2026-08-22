/*
 * Mr. 4th Dimention - Allen Webster
 *
 * 17.07.2017
 *
 * File editing view for 4coder.
 *
 */

// TOP

#pragma once

struct Query_Slot{
    Query_Slot *next;
    Query_Bar *query_bar;
};

struct Query_Set{
    Query_Slot slots[8];
    Query_Slot *free_slot;
    Query_Slot *used_slot;
};

struct View_Context_Node{
 View_Context_Node *next;
 Temp_Memory pop_me;
 View_Context ctx;
 void *delta_rule_memory;
};

struct View
{
 View *next;
 View *prev;
 
 i32 window_id;
 struct Panel *panel;
 b32 in_use;
 
 Editing_File *file;
 Lifetime_Object *lifetime_object;
 
 File_Edit_Positions edit_pos_;
 i64 mark;
 f32 preferred_x;
 v2 cursor_margin;
 v2 cursor_push_in_multiplier;
 
 b8 new_scroll_target;
 b8 hide_scrollbar;
 b8 hide_file_bar;
 b8 show_whitespace;
 
 Coroutine *co;
 Co_Out co_out;
 
 Arena node_arena;
 View_Context_Node *ctx;
 
 Query_Set query_set;
};

struct Live_Views{
    View *views;
    View free_sentinel;
    i1 count;
    i1 max;
};


// BOTTOM

