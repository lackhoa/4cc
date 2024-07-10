/*
4coder_search.h - Types that are used in the search accross all buffers procedures.
*/

// TOP

#if !defined(FCODER_SEARCH_H)
#define FCODER_SEARCH_H

typedef u32 List_All_Locations_Flag;
enum{
    ListAllLocationsFlag_CaseSensitive = 1,
    ListAllLocationsFlag_MatchSubstring = 2,
};

struct Word_Complete_Iterator{
    App *app;
    Arena *arena;
    
    Temp_Memory arena_restore;
    Buffer_ID first_buffer;
    Buffer_ID current_buffer;
    b32 scan_all_buffers;
    String needle;
    
    List_String list;
    Node_String *node;
    Table_Data_u64 already_used_table;
};

struct Word_Complete_Menu{
    Render_Caller_Function *prev_render_caller;
    Word_Complete_Iterator *it;
    String options[8];
    i1 count;
};

#endif

// BOTOTM

