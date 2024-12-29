/*
 * Mr. 4th Dimention - Allen Webster
 *
 * 14.08.2019
 *
 * Core logging structures.
 *
 */

// TOP

#pragma once

struct Log_Spam_Entry
{
 u64 hash;
 u64 count;
};

struct Log_State
{
 System_Mutex mutex;
 Arena message_arena;
 List_String list;
 volatile i1 disabled_thread_id;
 b32 stdout_log_enabled;
 darray(Log_Spam_Entry) spam_list;
};
function b32
is_power_of_2(u32 value)
{
 if(value == 0) return false;
 return((value & (value-1)) == 0);
}

// BOTTOM