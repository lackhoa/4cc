/*
4coder_layout_rule.h - Built in layout rule types.
*/

// TOP

#pragma once

struct Newline_Layout_Vars {
 i64 newline_character_index;
 b32 consuming_newline_characters;
 b32 prev_did_emit_newline;
};

#define AST_DATA_FIELDS \
i32 token_begin; \
i32 token_end;   \

struct AST_Data { AST_DATA_FIELDS };

// NOTE(kv) Persistent structures
struct AST_Node
{
 AST_Node *parent;
 sarray(AST_Node) children;
 
 union { AST_Data data; struct { AST_DATA_FIELDS }; };
};
struct Buffer_AST
{
 b32 up_to_date;
 AST_Node root;
 
 i32 node_count;
 AST_Node *nodes;
};
struct Layout_State
{
 b32 is_skm;
 Token_Array tokens;
 Buffer_AST *ast;
 
 Arena *arena;
 Layout_Item_List list;
 Face_Advance_Map *advance_map;
 Face_ID face;
 Face_Metrics *metrics;
 f32 tab_width;
 f32 line_to_text_shift;
 
 v2 blank_dim;
 
 v2 p;
 f32 line_y;
 f32 text_y;
 f32 width;
};

struct Layout_Reflex{
 Layout_Item_List *list;
 Buffer_ID buffer;
 f32 width;
 Face_ID face;
};

// BOTTOM