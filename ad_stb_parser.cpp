//-NOTE: A C-style language Parser

// NOTE kv: This file doesn't have include guards, but I don't need to
// include the header file separately so it doesn't matter.
#define STB_C_LEXER_IMPLEMENTATION
#include "stb_c_lexer.h"

struct STB_Parser
{
 b32 ok_;
 stb_lex_location fail_location;
 
 //-NOTE Embracing member functions
 void set_ok(b32 value)
 {// NOTE kv: Since we don't setting the value to true (I'm open for a better name)
  if (ok_)
  {
   ok_ = value;
   if (ok_ == false)
   {// NOTE: Report failure location
    const char *where = this->stb.where_firstchar;
    stb_c_lexer_get_location(&this->stb, where, &fail_location);
   }
  }
 }
 //
 force_inline void fail() {
  set_ok(false);
 }
 
 // NOTE(kv): string_arena: when we return strings, we copy them here
 // to make the API cleaner.
 // (Can't use pointer to files because that might be freed later),
 Arena *string_arena;
 
 stb_lexer stb;
};

function void
eat_token(STB_Parser *p)
{
 if (p->ok_)
 {
  i1 res = stb_c_lexer_get_token(&p->stb);
  // NOTE: 0 is eof, which is not an error
  if (res < 0) { p->fail(); }
 }
}

function STB_Parser
new_parser(String input, Arena *string_arena, i1 string_store_count)
{
 STB_Parser result = {.ok_ = true, .string_arena=string_arena,};
 
 stb_lexer *lexer = &result.stb;
 char *input_start = (char *)(input.str);
 char *input_end   = (char *)(input.str+input.size);
 char *string_store = push_array(string_arena, char, string_store_count);
 stb_c_lexer_init(lexer, input_start, input_end,
                  string_store, string_store_count);
 result.stb.string_value = result.stb.string_storage;
 eat_token(&result);
 
 return result;
}

function b32
test_id(STB_Parser *p, String id)
{
 b32 result = false;
 if (p->ok_)
 {
  if (p->stb.token == CLEX_id)
  {
   result = string_match(SCu8(p->stb.string_value), id);
  }
 }
 return result;
}

function b32
maybe_id(STB_Parser *p, String id)
{
 b32 result = test_id(p, id);
 if (result) { eat_token(p); }
 return result;
}
//
force_inline b32
maybe_id(STB_Parser *p, char *id)
{ 
 return maybe_id(p, SCu8(id));
}

force_inline String
eat_id(STB_Parser *p, String id={})
{
 String result = {};
 if (id == String{})
 {
  p->set_ok(p->stb.token == CLEX_id);
  result = push_string(p->string_arena, p->stb.string_value);
  eat_token(p);
 }
 else
 {
  p->set_ok(maybe_id(p, id));
 }
 return result;
}
//
force_inline void
eat_id(STB_Parser *p, char *id) {
 eat_id(p, SCu8(id));
}
//
force_inline String eat_String(STB_Parser *p) { return eat_id(p); }



inline void
eat_char(STB_Parser *p, char c)
{
 p->set_ok(p->stb.token == c); 
 eat_token(p);
}

force_inline b32
maybe_char(STB_Parser *p, char c){
 b32 result = false;
 if (p->stb.token == c){
  eat_token(p);
  result = true;
 }
 return result;
}

function i1
eat_i1(STB_Parser *p) {
 i1 result = 0;
 
 i1 sign = 1;
 if(maybe_char(p, '-')){
  sign = -1;
 }else{
  maybe_char(p, '+');
 }
 
 if(p->stb.token == CLEX_intlit){
  result = sign*p->stb.int_value;
  eat_token(p);
 }
 
 return result;
}
// NOTE: Fancy "auto" usage
function auto &eat_b32 = eat_i1;

inline void
eat_integer_vector(STB_Parser *p, i1 *result, i1 count) {
 for_i32(index,0,count) {
  result[index] = eat_i1(p);
 }
}
force_inline i2
eat_i2(STB_Parser *p) {
 i2 result;
 eat_integer_vector(p, result.e, 2);
 return result;
}
force_inline i3
eat_i3(STB_Parser *p) {
 i3 result;
 eat_integer_vector(p, result.e, 3);
 return result;
}
force_inline i4
eat_i4(STB_Parser *p) {
 i4 result;
 eat_integer_vector(p, result.e, 4);
 return result;
}

function v1
eat_v1(STB_Parser *p)
{
 v1 result = 0.f;
 
 v1 sign = 1.f;
 if ( maybe_char(p, '-') ) {
  sign = -1.f;
 } else {
  maybe_char(p, '+');
 }
 
 if (p->stb.token == CLEX_floatlit)
 {
  result = sign*v1(p->stb.float_value);
  eat_token(p);
 }
 
 return result;
}
//
inline void
eat_vector(STB_Parser *p, v1 *result, i1 count){
 for_i32(index,0,count) {
  result[index] = eat_v1(p);
 }
}
//
force_inline v2
eat_v2(STB_Parser *p){
 v2 result;
 eat_vector(p, result.e, 2);
 return result;
}
force_inline v3
eat_v3(STB_Parser *p){
 v3 result;
 eat_vector(p, result.e, 3);
 return result;
}
force_inline v4
eat_v4(STB_Parser *p){
 v4 result;
 eat_vector(p, result.e, 4);
 return result;
}

function void
eat_data_basic_type(STB_Parser *p, Basic_Type type, void *void_pointer)
{
 u8 *pointer = (u8 *)void_pointer;
 switch(type)
 {
#define X(T)   case Basic_Type_##T: \
{ \
T value = eat_##T(p); \
block_copy(pointer, &value, get_basic_type_size(type)); \
} break;
  //
  X_Basic_Types(X)
#undef X
 }
}

//NOTE(kv) Exclude the terminator
function void
eat_until_char(STB_Parser *p, char terminator, i1 max_recursion=20)
{
 if(max_recursion > 0){
  while(p->ok_){
   char c = p->stb.token;
   if(c == terminator){
    break;
   }else{
    //NOTE Not the terminator
    char matching = 0;
    if(c == '('){ matching = ')'; }
    if(c == '['){ matching = ']'; }
    if(c == '{'){ matching = '}'; }
    if(matching){
     //NOTE(kv) Eat grouper
     eat_token(p);
     eat_until_char(p, matching, max_recursion-1);
     eat_token(p);
    }else{
     //NOTE(kv) Default
     eat_token(p);
    }
   }
  }
 }
}

//~ EOF