/*
 * Mr. 4th Dimention - Allen Webster
 *
 * 24.03.2018
 *
 * History
 *
 */

// TOP

#pragma once

struct Record_Batch_Slot{
 i64 length_forward;
 i64 length_backward;
 i1 first;
};

struct Record_Node{
 Record_Node *next;
 Record_Node *prev;
};
struct Record
{
 Record_Node node;
 Temp_Memory restore_point;
 i64 pos_before_edit;
 i1 edit_number;
 Record_Kind kind;
 union{
  struct{
   String forward_text;
   String backward_text;
   i64 first;
  } single;
  struct{
   Record_Node children;
   i1 count;
  } group;
 };
 b32 automated;
};

struct Record_Ptr_Lookup_Table{
 Record **records;
 i1 count;
 i1 max;
};

struct History{
 b32 activated;
 Arena arena;
 Heap heap;
 Base_Allocator heap_wrapper;
 Record_Node free_records;
 Record_Node records;
 i1 record_count;
 Record_Ptr_Lookup_Table record_lookup;
};

struct Global_History{
 i1 edit_number_counter;
 i1 edit_grouping_counter;
};

// BOTTOM
