/*
 * Serial inserts helpers
 */

// TOP

#pragma once

struct Buffer_Insertion
{
 App_Cmd *app;
 Buffer_ID buffer;
 i64 at;
 b32 buffering;
 
 u8 *memory;
 umm len;
 umm cap;
};

// BOTTOM