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
global char *token_base_kind_names[] = {
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
enum
{
 TokenBaseFlag_PreprocessorBody = 1,
 TokenBaseFlag_SkmCode          = 2,
};

struct Token
{
 i64 pos;
 i64 size;
 Token_Base_Kind kind;
 Token_Base_Flag flags;
 i16 sub_kind;
 u16 sub_flags;
};

myinline b32
is_preprocessor_body(Token *token)
{
 return HasFlag(token->flags, TokenBaseFlag_PreprocessorBody);
}

myinline Range_i64
get_token_range(Token *token)
{
 if (token)
  return Range_i64{token->pos, token->pos + token->size};
 else
  return Range_i64{};
}
myinline Range_i64
Ii64(Token *token)
{
 return get_token_range(token);
}


global Token stub_token = {}; // NOTE(kv): experimenting with ZII!
struct Token_Pair{
 union{ Token a, first, min; };
 union{ Token b, last, max; };
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
//TODO(kv) Bro, why doesn't this include the source string?
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
struct Token_Iterator{
 Token_Iterator_Kind kind;
 union{
  Token_Iterator_Array array;
  Token_Iterator_List list;
 };
};
//-
myinline Token_Iterator
make_token_iterator(Token_Iterator_Array const&it){
 Token_Iterator result = {};
 result.kind = TokenIterator_Array;
 result.array = it;
 return(result);
}
//
myinline Token_Iterator
make_token_iterator(Token_Iterator_List const&it){
 Token_Iterator result = {};
 result.kind = TokenIterator_List;
 result.list = it;
 return(result);
}

myinline Token_Iterator_Array::operator Token_Iterator() {
 return make_token_iterator(*this); 
}
myinline Token_Iterator_List::operator Token_Iterator() {
 return make_token_iterator(*this); 
}
//-

myinline String
token_string_from_source(String source, Token *token)
{
 String result = {source.data+token->pos, (u64)token->size};
 return result;
}
// BOTTOM
