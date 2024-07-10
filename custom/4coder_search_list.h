/*
 * Mr. 4th Dimention - Allen Webster
 *
 * 01.10.2019
 *
 * Search list helper.
 *
 */

// TOP

#if !defined(FRED_SEARCH_LIST_H)
#define FRED_SEARCH_LIST_H

////////////////////////////////
// NOTE(allen): Search List Builders

function void def_search_add_path(Arena *arena, List_String *list, String path);
function void def_search_list_add_system_path(Arena *arena, List_String *list, System_Path_Code path);

////////////////////////////////
// NOTE(allen): Search List Functions

function String def_search_get_full_path(Arena *arena, List_String *list, String file_name);
function FILE *def_search_fopen(Arena *arena, List_String *list, char *file_name, char *opt);

#endif

// BOTTOM

