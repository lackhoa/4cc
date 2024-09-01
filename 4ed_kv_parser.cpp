// NOTE(kv): This is yet another parser based on 4coder's tokenizer
// Should it be a separate parser from the other one using STB_Parser?
// Maybe: it has bidirectional parsing, it uses another token type,
// it has token types for C++

// NOTE(kv): The "maybe" function should be implemented using rewind,
// because using p->ok makes the logic simpler.
// However, it's slower and in the simple case we should just duplicate the logic.

// NOTE(kv): view ad_stb_parser.cpp

#include "4ed_kv_parser.h"

#ifndef ED_PARSER_BUFFER
#    define ED_PARSER_BUFFER 0
#endif

enum Token_Gen_Type
{
 TG_None,
 TG_Buffer,
 TG_String,
};

#if ED_PARSER_BUFFER //-
struct Token_Gen_Buffer
{
 App *app;
 Buffer_ID buffer;
};

#endif //-ED_PARSER_BUFFER

struct Token_Gen_String
{
 String source;
};

struct Line_Column
{
 i1 line;
 i1 column;
};

struct Ed_Parser
{
 b32 ok_;
 Scan_Direction scan_direction;
 Token_Iterator it;
 Arena *string_arena;
 Token_Iterator original_token_it;
 
 Token_Gen_Type Token_Gen_Type;
 union
 {
#if ED_PARSER_BUFFER
  Token_Gen_Buffer Token_Gen_Buffer;
#endif
  // or
  Token_Gen_String Token_Gen_String;
 };
 
 //-
 void set_ok(b32 value)
 {// NOTE(kv): We don't setting the value to true (I'm open for a better name)
  if (ok_)
  {
   auto p = this;
   ok_ = value;
  }
 }
 
 force_inline void fail() { set_ok(false); }
};

function i1
count_newlines_in_string(String string)
{
 i1 result = 0;
 for_i1(index,0,i1(string.len)) {
  if (string.str[index] == '\n') { 
   result++;
  }
 }
 return result;
}

function Line_Column
ep_get_fail_location(Ed_Parser *p)
{
 Line_Column result = {};
 if (p->ok_ == false)
 {
  Token *fail_token = ep__get_token_please(p);
  // NOTE: Time for us to do some gymnastic
  auto it0 = p->original_token_it;
  i64 begin_line_pos = 0;
  result.line = 1;  // NOTE(kv): Line number starts with 1 for some reason.
  while(true)
  {
   Token *token = token_it_inc_all(&it0);
   if (!token || token->pos > fail_token->pos) {
    break;
   }
   result.column = i1(token->pos - begin_line_pos);
   if (token->kind == TokenBaseKind_Whitespace)
   {// NOTE: Increase line number
    Scratch_Block scratch;
    String token_string = ep__print_given_token(p, scratch, token);
    for_i1 (index,0,i1(token_string.len))
    {
     if (token_string.str[index] == '\n') {
      result.line++;
      begin_line_pos = token->pos+index+1;
     }
    }
   }
  }
 }
 return result;
}

#if ED_PARSER_BUFFER
function Ed_Parser
make_ep_from_buffer(App *app, Buffer_ID buffer, Token_Iterator const&it,
                    Scan_Direction scan_direction=Scan_Forward,
                    Arena *string_arena=0)
{
 Ed_Parser result = {
  .ok_             =true,
  .scan_direction  =scan_direction,
  .it              =it,
  .string_arena    =string_arena,
  .original_token_it = it,
  .Token_Gen_Type  =TG_Buffer,
  .Token_Gen_Buffer={
   .app   =app,
   .buffer=buffer,
  },
 };
 return result;
}
#endif

function Ed_Parser
make_ep_from_string(String string, Token_Iterator const&it,
                    Arena *string_arena=0)
{
 Ed_Parser result = {
  .ok_             =true,
  .scan_direction  =Scan_Forward,
  .it              =it,
  .string_arena    =string_arena,
  .original_token_it = it,
  .Token_Gen_Type  =TG_String,
  .Token_Gen_String={
   .source=string,
  },
 };
 return result;
}

function Token *
ep__get_token_please(Ed_Parser *p)
{
 Token *token = 0;
 switch(p->Token_Gen_Type)
 {
#if ED_PARSER_BUFFER
  case TG_Buffer: { token = token_it_read(&p->it); } break;
#endif
  case TG_String: { token = token_it_read(&p->it); }break;
  invalid_default_case;
 }
 return token;
}

inline Token *
ep_get_token(Ed_Parser *p)
{
 Token *token = 0;
 if (p->ok_) {
  token = ep__get_token_please(p);
 }
 return token;
}

function String
ep__print_given_token(Ed_Parser *p, Arena *arena, Token *token)
{
 String result = {};
 if (token)
 {
  switch(p->Token_Gen_Type)
  {
#if ED_PARSER_BUFFER
   case TG_Buffer: {
    auto &tg = p->Token_Gen_Buffer;
    result = push_token_lexeme(tg.app, arena, tg.buffer, token);
   }break;
#endif
   
   case TG_String: {
    result = string_substring(p->Token_Gen_String.source, Ii64(token));
   } break;
   
   invalid_default_case;
  }
 }
 return result;
}

function String
ep_print_token(Ed_Parser *p, Arena *arena)
{
 Token *token = ep_get_token(p);
 return ep__print_given_token(p, arena, token);
}

function i64
ep_get_pos(Ed_Parser *p)
{
 if (Token *token = ep_get_token(p)) {
  return token->pos;
 } else { return 0; }
}

function void
ep_eat_token(Ed_Parser *p)
{
 if (p->ok_)
 {
  switch(p->Token_Gen_Type)
  {
   case TG_String:
   case TG_Buffer:
   {
    if (p->scan_direction == Scan_Forward) {
     p->set_ok(token_it_inc(&p->it) != 0);
    } else {
     p->set_ok(token_it_dec(&p->it) != 0);
    }
   }break;
   
   invalid_default_case;
  }
 }
}

// NOTE: Both float and int, for convenience
function void
ep_eat_number(Ed_Parser *p, f32 *out)
{
 Scratch_Block scratch;
 
 f32 sign = 1;
 Token *token = ep_get_token(p);
 if (token)
 {
  if (token->kind == TokenBaseKind_Operator)
  {// NOTE(kv): parse sign
   String string = ep_print_token(p, scratch);
   if ( string_match_lit(string, "-") )
   {
    sign = -1;
   }
   else if (!string_match_lit(string, "+"))
   {
    p->set_ok(false);
   }
   
   ep_eat_token(p);
   token = ep_get_token(p);
  }
 }
 
 if (p->ok_)
 {
  if (token->kind != TokenBaseKind_LiteralFloat &&
      token->kind != TokenBaseKind_LiteralInteger)
  {
   p->set_ok(false);
  }
 }
 
 if (p->ok_)
 {
  String8 string = ep_print_token(p, scratch);
  char *cstring = to_c_string(scratch, string);
  *out = sign * gb_str_to_f32(cstring, 0);
 }
}

function b32
ep_maybe_number(Ed_Parser *p, f32 *out)
{
 Ed_Parser save = *p;
 ep_eat_number(p, out);
 b32 result = p->ok_;
 if (!result) {
  *p = save;
 }
 return result;
}

function Token_Base_Kind
ep_get_kind(Ed_Parser *p)
{
 Token_Base_Kind result = 0;
 Token *token = ep_get_token(p);
 if (token)
 {
  result = token->kind;
 }
 return result;
}

function void
ep_eat_kind(Ed_Parser *p, Token_Base_Kind kind)
{
 Token *token = ep_get_token(p);
 if (token) {
  p->set_ok(token->kind == kind);
 }
 ep_eat_token(p);
}

function b32
ep_maybe_kind(Ed_Parser *p, Token_Base_Kind kind)
{
 b32 result = false;
 if (p->ok_)
 {
  Token *token = ep_get_token(p);
  result = (token->kind == kind);
 }
 if (result) { ep_eat_token(p); }
 return result;
}

force_inline b32
ep_at_eof(Ed_Parser *p)
{
 b32 result = false;
 if (p->ok_)
 {
  Token *token = ep_get_token(p);
  result = (token->kind == TokenBaseKind_EOF);
 }
 return result;
}

function b32
ep_test_id(Ed_Parser *p, String string)
{
 b32 result = false;
 if (p->ok_)
 {
  Scratch_Block scratch;
  String token = ep_print_token(p, scratch);
  result = string_match(token, string);
 }
 return result;
}
//
force_inline b32
ep_test_id(Ed_Parser *p, char *string) {
 return ep_test_id(p, SCu8(string));
}

function b32
ep_maybe_id(Ed_Parser *p, String string)
{
 b32 result = ep_test_id(p, string);
 if (result) { ep_eat_token(p); }
 return result;
}
//
force_inline b32
ep_maybe_id(Ed_Parser *p, char *string)
{
 return ep_maybe_id(p, SCu8(string));
}


function b32
ep_test_preprocessor(Ed_Parser *p, String string)
{
 b32 result = false;
 Token *token = ep_get_token(p);
 if (p->ok_ && token->kind == TokenBaseKind_Preprocessor)
 {
  Scratch_Block scratch;
  String token_str = ep_print_token(p, scratch);
  token_str.str++; token_str.size--;  // skip the pound
  token_str = string_skip_whitespace(token_str);
  while (*token_str.str == ' ') { token_str.str++; }
  result = string_match(token_str, string);
 }
 return result;
}

function b32
ep_maybe_preprocessor(Ed_Parser *p, String string)
{
 b32 result = ep_test_preprocessor(p, string);
 if ( result )
 {
  ep_eat_token(p);
 }
 return result;
}

force_inline void
ep_eat_preprocessor(Ed_Parser *p, String string)
{
 p->set_ok(ep_maybe_preprocessor(p, string));
}

function String
ep_id(Ed_Parser *p, String test_id={})
{
 String result = {};
 // NOTE(kv): keywords are also identifier-like, but idk about this test
 auto kind = ep_get_kind(p);
 p->set_ok(kind == TokenBaseKind_Identifier ||
          kind == TokenBaseKind_Keyword);
 if (p->ok_)
 {
  if (test_id == String{})
  {
   result = ep_print_token(p, p->string_arena);
  }
  else
  {// NOTE: matching against something
   Scratch_Block scratch;
   String string = ep_print_token(p, scratch);
   p->set_ok(string == test_id);
  }
 }
 ep_eat_token(p);
 return result;
}
//
force_inline String
ep_id(Ed_Parser *p, char *test_id)
{
 return ep_id(p, SCu8(test_id));
}

// NOTE Returns index + 1 (todo(kv): wtf man?)
function i1
ep__char_in_string(String chars, char chr)
{
 i1 result = 0;
 for_u32 (index, 0, (u32)chars.size)
 {
  if (chars.str[index] == chr)
  {
   result = index + 1;
   break;
  }
 }
 return result;
}

function b32
ep_maybe_char(Ed_Parser *p, char c)
{
 Scratch_Block scratch;  // @slow, also sometimes the arena isn't necessary
 String string = ep_print_token(p, scratch);
 b32 result = (string.len == 1 &&
               string.str[0] == c);
 if (result) {
  ep_eat_token(p);
 }
 return result;
}
//
function void
ep_char(Ed_Parser *p, char c)
{
 p->set_ok( ep_maybe_char(p, c) );
}


// NOTE returns index + 1
function i1
ep_eat_until_char(Ed_Parser *p, String chars, 
                  i1 max_recursion)
{
 max_recursion -= 1;
 if (max_recursion < 0) { return 0; }
 else
 {
  i1 result = 0;
  while ( !result && p->ok_ )
  {
   Scratch_Block scratch;
   String token = ep_print_token(p, scratch);
   if (token.size == 1)
   {
    char character = token.str[0];
    result = ep__char_in_string(chars, character);
    if (result)
    {
     // break
    }
    else
    {
     ep_eat_token(p);
     
     char *matching = 0;
     if (character == '(') { matching = ")"; }
     if (character == '[') { matching = "]"; }
     if (character == '{') { matching = "}"; }
     if (matching)
     {
      ep_eat_until_char(p, SCu8(matching), max_recursion);
      ep_eat_token(p);
     }
    }
   }
   else { ep_eat_token(p); }
  }
  return result;
 }
}

#define ep_eat_until_char_lit(p, cstring) \
ep_eat_until_char(p, str8lit(cstring), 64);

#undef ED_PARSER_BUFFER