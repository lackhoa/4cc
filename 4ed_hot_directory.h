/*
 * Mr. 4th Dimention - Allen Webster
 *
 * 24.01.2018
 *
 * Buffer types
 *
 */

// TOP

#if !defined(FRED_HOT_DIRECTORY_H)
#define FRED_HOT_DIRECTORY_H

struct Hot_Directory{
    Arena arena;
    String8 string;  // NOTE(kv): this might not exist, and we can reload it.
    String8 canonical;
    File_List file_list;
};

#endif

// BOTTOM

