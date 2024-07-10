/*
4coder_combined_write_commands.cpp - Commands for writing text specialized for particular contexts.
*/

// TOP

struct Snippet{
    char *name;
    char *text;
    i1 cursor_offset;
    i1 mark_offset;
};

struct Snippet_Array{
    Snippet *snippets;
    i1 count;
};

// BOTTOM

