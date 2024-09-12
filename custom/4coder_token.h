/*
 * 4coder token types
 */

// TOP

#pragma once

typedef i16 Token_Base_Kind;
enum{
 TokenBaseKind_EOF            = 0,
 TokenBaseKind_Whitespace     = 1,
 TokenBaseKind_LexError       = 2,
 TokenBaseKind_Comment        = 3,
 TokenBaseKind_Keyword        = 4,
 TokenBaseKind_Preprocessor   = 5,
 TokenBaseKind_Identifier     = 6,
 TokenBaseKind_Operator       = 7,
 TokenBaseKind_LiteralInteger = 8,
 TokenBaseKind_LiteralFloat   = 9,
 TokenBaseKind_LiteralString  = 10,
 TokenBaseKind_ScopeOpen      = 11,
 TokenBaseKind_ScopeClose     = 12,
 TokenBaseKind_ParenOpen      = 13,
 TokenBaseKind_ParenClose     = 14,
 TokenBaseKind_StatementClose = 15,
 
 TokenBaseKind_COUNT          = 16,
};

global char *token_base_kind_names[] ={
    "EOF",
    "Whitespace",
    "LexError",
    "Comment",
    "Keyword",
    "Preprocessor",
    "Identifier",
    "Operator",
    "LiteralInteger",
    "LiteralFloat",
    "LiteralString",
    "ScopeOpen",
    "ScopeClose",
    "ParenOpen",
    "ParenClose",
 "StatementClose",
};

typedef u16 Token_Base_Flag;
enum{
 TokenBaseFlag_PreprocessorBody = 1,
};

struct Token{
 i64 pos;
 i64 size;
 Token_Base_Kind kind;
 Token_Base_Flag flags;
 i16 sub_kind;
 u16 sub_flags;
};

global Token stub_token = {}; // NOTE(kv): experimenting with ZII!

struct Token_Pair{
    Token a;
    Token b;
};

struct Token_Array{
    Token *tokens;
    i64 count;
    i64 max;
};

struct Token_Block{
 Token_Block *next;
 Token_Block *prev;
 Token *tokens;
 i64 count;
 i64 max;
};

struct Token_List{
 Token_Block *first;
 Token_Block *last;
 i64 node_count;
 i64 total_count;
};

struct Token_Relex{
 b32 successful_resync;
 i64 first_resync_index;
};

struct Token_Iterator;

struct Token_Iterator_Array{
 u64 user_id;
 Token *ptr;
 Token *tokens;
 i64 count;
 operator Token_Iterator();
};

struct Token_Iterator_List{
 u64 user_id;
 i64 index;
 Token *ptr;
 Token_Block *block;
 Token_Block *first;
 Token_Block *last;
 i64 node_count;
 i64 total_count;
 operator Token_Iterator();
};

typedef i1 Token_Iterator_Kind;
enum{
 TokenIterator_Array,
 TokenIterator_List,
};

struct Token_Iterator {
 Token_Iterator_Kind kind;
 union{
  Token_Iterator_Array array;
  Token_Iterator_List list;
 };
};

//-
inline Token_Iterator
make_token_iterator(Token_Iterator_Array const&it){
 Token_Iterator result = {};
 result.kind = TokenIterator_Array;
 result.array = it;
 return(result);
}
//
inline Token_Iterator
make_token_iterator(Token_Iterator_List const&it){
 Token_Iterator result = {};
 result.kind = TokenIterator_List;
 result.list = it;
 return(result);
}

inline Token_Iterator_Array::operator Token_Iterator() {
 return make_token_iterator(*this); 
}
inline Token_Iterator_List::operator Token_Iterator() {
 return make_token_iterator(*this); 
}
//-

inline Range_i64
Ii64(Token *token)
{
 if (token) {
  return Ii64_size(token->pos, token->size);
 } else {
  return {};
 }
}

// BOTTOM
