//-NOTE: A C-style language Parser

struct STB_Parser
{
 b32 ok_;  // IMPORTANT: Don't set me!
 stb_lex_location fail_location;
 
 //-NOTE Embracing member functions
 force_inline b32 ok() { return ok_; };
 
 void set_ok(b32 value)
 {
  if (ok_)
  {
   ok_ = value;
   if (value == false)
   {// NOTE: Report failure location
    const char *where = this->where_firstchar;
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
 
 union
 {
  stb_lexer stb;
  struct  // @copypasta (todo: top-tier programming right here)
  {
   // lexer variables
   char *input_stream;
   char *eof;
   char *parse_point;
   char *string_storage;
   int   string_storage_len;
   
   // lexer parse location for error messages
   char *where_firstchar;
   char *where_lastchar;
   
   // lexer token variables
   long token;
   double float_value;
   long   int_value;
   char *string_value;
   int   string_len;
  };
 };
};

internal void
eat_token(STB_Parser *p)
{
 if (p->ok())
 {
  i1 res = stb_c_lexer_get_token(&p->stb);
  // NOTE: 0 is eof, which is not an error
  if (res < 0) { p->fail(); }
 }
}

internal STB_Parser
new_parser(String input, Arena *string_arena, i1 string_store_count)
{
 STB_Parser result = {.ok_ = true, .string_arena=string_arena,};
 
 stb_lexer *lexer = &result.stb;
 char *input_start = (char *)(input.str);
 char *input_end   = (char *)(input.str+input.size);
 char *string_store = push_array(string_arena, char, string_store_count);
 stb_c_lexer_init(lexer, input_start, input_end,
                  string_store, string_store_count);
 result.string_value = result.string_storage;
 eat_token(&result);
 
 return result;
}

internal b32
test_id(STB_Parser *p, String id)
{
 b32 result = false;
 if (p->ok())
 {
  if (p->token == CLEX_id)
  {
   result = string_match(SCu8(p->string_value), id);
  }
 }
 return result;
}

internal b32
maybe_id(STB_Parser *p, String id)
{
 b32 result = test_id(p, id);
 if (result) { eat_token(p); }
 return result;
}
//
force_inline b32 maybe_id(STB_Parser *p, char *id)
{ 
 return maybe_id(p, SCu8(id));
}

force_inline void
eat_id(STB_Parser *p, String id)
{
 p->set_ok(maybe_id(p, id));
}
//
force_inline void
eat_id(STB_Parser *p, char *id) {
 eat_id(p, SCu8(id));
}

// TODO: could have called this "eat_id"
internal String
eat_String(STB_Parser *p)
{
 String result = {};
 p->set_ok(p->token == CLEX_id);
 result = push_string_copy(p->string_arena, p->string_value);
 eat_token(p);
 return result;
}

inline void
eat_char(STB_Parser *p, char c)
{
 p->set_ok(p->token == c); 
 eat_token(p);
}

force_inline b32
maybe_char(STB_Parser *p, char c)
{
 b32 result = false;
 if (p->token == c)
 {
  eat_token(p);
  result = true;
 }
 return result;
}

internal i1
eat_i1(STB_Parser *p)
{
 i1 result = 0;
 
 i1 sign = 1;
 if( maybe_char(p, '-') ) {
  sign = -1;
 } else {
  maybe_char(p, '+');
 }
 
 if (p->token == CLEX_intlit)
 {
  result = sign*p->int_value;
  eat_token(p);
 }
 
 return result;
}
// NOTE: Fancy experiment
internal auto &eat_b32 = eat_i1;

internal v1
eat_v1(STB_Parser *p)
{
 v1 result = 0.f;
 
 v1 sign = 1.f;
 if ( maybe_char(p, '-') ) {
  sign = -1.f;
 } else {
  maybe_char(p, '+');
 }
 
 if (p->token == CLEX_floatlit)
 {
  result = sign*v1(p->float_value);
  eat_token(p);
 }
 
 return result;
}
//
inline void
eat_vector(STB_Parser *p, v1 *result, i1 count)
{
 for_i32(index,0,count) {
  result[index] = eat_v1(p);
 }
}
//
force_inline v3
eat_v3(STB_Parser *p)
{
 v3 result;
 eat_vector(p, result.e, 3);
 return result;
}
//
force_inline v4
eat_v4(STB_Parser *p)
{
 v4 result;
 eat_vector(p, result.e, 4);
 return result;
}

//~ EOF