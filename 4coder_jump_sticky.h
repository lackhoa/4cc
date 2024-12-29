/*
4coder_jump_sticky.h - Types for persistant jump positions.
*/

// TOP

#pragma once

struct Sticky_Jump
{
 i64 list_line;
 i64 list_colon_index;
 b32 is_sub_error;
 Buffer_ID to_buffer_id;
 i64 to_pos;
};

struct Sticky_Jump_Stored
{
 i64 list_line;
 i64 list_colon_index;
 b32 is_sub_error;
 Buffer_ID to_buffer_id;
 i32 index_into_marker_array;
 i64 to_pos;  //NOTE(kv) Backup the pos in case we reload the buffer.
};

struct Sticky_Jump_Node{
    Sticky_Jump_Node *next;
    Sticky_Jump jump;
};

struct Sticky_Jump_Array
{
 Sticky_Jump *jumps;
 i32 count;
};

struct Sticky_Jump_Node_Header{
 Managed_Object memory;
 Managed_Object markers;
 i32 first_index;
 i32 count;
};

enum Jump_Location_Flag{
 JumpFlag_IsSubJump = 0x1,
};

struct Marker_List
{
 Managed_Object jump_array;
 i32 jump_count;
 i32 previous_size;
 Buffer_ID jump_buffer;
 b32 has_jumped;
};

struct Marker_List_Node
{
 Marker_List_Node *next;
 Marker_List_Node *prev;
 Marker_List list;
 Buffer_ID buffer_id;
};

struct Locked_Jump_State
{
 View_ID view;
 Marker_List *list;
 i32 list_index;
};

function Managed_Object
get_positions_handle(App *app, Buffer_ID jump_buffer, Buffer_ID target_buffer);
// BOTTOM

