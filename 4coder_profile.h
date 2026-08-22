/*
 * 4coder_profile.cpp - Built in self profiling report.
 */

// TOP

#pragma once

struct Profile_Global_List
{
 System_Mutex mutex;
 Arena node_arena;
 Profile_Thread *first_thread;
 Profile_Thread *last_thread;
 i32 thread_count;
 Profile_Enable_Flag disable_bits;
};

struct Profile_Block
{
 // NOTE(kv) We want to guarantee that the profile block ends when using "continue" or "break".
 // But we want to control when it ends, too.
 b32 ended;  
 
 member_function Profile_Block(String name, String location);
 member_function ~Profile_Block();
};
// BOTTOM
