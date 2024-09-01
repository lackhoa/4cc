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

struct Record{
    Node node;
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
            Node children;
            i1 count;
        } group;
    };
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
 Node free_records;
 Node records;
 i1 record_count;
 Record_Ptr_Lookup_Table record_lookup;
};

struct Global_History{
 i1 edit_number_counter;
 i1 edit_grouping_counter;
};

// BOTTOM
