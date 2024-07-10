struct STB_Parser
{
 b32 ok;
 union
 {
  stb_lexer stb;
  struct  // @copypasta
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
 if (p->ok)
 {
  i1 res = stb_c_lexer_get_token(&p->stb);
  // NOTE: 0 is eof, which is not an error
  if (res < 0) { p->ok = false; }
 }
}

internal STB_Parser
new_parser(String input, char *string_store, i1 string_store_count)
{
 STB_Parser result = {};
 result.ok=true;
 
 stb_lexer *lexer = &result.stb;
 char *input_start = (char *)(input.str);
 char *input_end   = (char *)(input.str+input.size);
 stb_c_lexer_init(lexer, input_start, input_end,
                  string_store, string_store_count);
 eat_token(&result);
 
 return result;
}

internal b32
test_id(STB_Parser *p, String id)
{
 b32 result = false;
 if (p->ok)
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
 p->ok = maybe_id(p, id);
}
//
force_inline void eat_id(STB_Parser *p, char *id) { eat_id(p, SCu8(id)); }

force_inline void
eat_char(STB_Parser *p, char c)
{
 p->ok = (p->token == c);
 eat_token(p);
}

force_inline b32
maybe_char(STB_Parser *p, char c)
{
 if (p->token == c)
 {
  eat_token(p);
  return true;
 }
 return false;
}

internal i1
eat_int(STB_Parser *p)
{
 i1 result = 0;
 
 i1 sign = 1;
 if( maybe_char(p, '-') )
 {
  sign = -1;
 }
 else { maybe_char(p, '+'); }
 
 if (p->token == CLEX_intlit)
 {
  result = sign*p->int_value;
  eat_token(p);
 }
 
 return result;
}

internal v1
eat_float(STB_Parser *p)
{
 v1 result = 0.f;
 
 v1 sign = 1.f;
 if( maybe_char(p, '-') )
 {
  sign = -1.f;
 }
 else { maybe_char(p, '+'); }
 
 if (p->token == CLEX_floatlit)
 {
  result = sign*v1(p->float_value);
  eat_token(p);
 }
 
 return result;
}

internal v3
eat_v3(STB_Parser *p)
{
 v3 result;
 result.x = eat_float(p);
 result.y = eat_float(p);
 result.z = eat_float(p);
 return result;
}

//~eof