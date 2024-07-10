// NOTE(kv): This is yet another parser based on 4coder's Token Iterator

struct Quick_Parser
{
 Arena arena;
 b32 ok;
 
 App *app;
 Buffer_ID buffer;
 Token_Iterator_Array tk;
 Scan_Direction scan_direction;
};

internal Quick_Parser
qp_new(App *app, Buffer_ID buffer, Token_Iterator_Array *tk, 
       Scan_Direction scan_direction=Scan_Forward)
{
 Quick_Parser result = {  };
 result.arena=make_arena_malloc();
 result.ok=true;
 result.app=app;
 result.buffer=buffer;
 result.tk=*tk;
 result.scan_direction=scan_direction;
 return result;
}

internal Token *
qp_get_token(Quick_Parser *p)
{
 Token *token = 0;
 if (p->ok)
 {
  token = token_it_read(&p->tk);
 }
 return token;
}

internal String
qp_push_token(Quick_Parser *p, Arena *arena)
{
 if (Token *token = qp_get_token(p))
 {
  return push_token_lexeme(p->app, arena, p->buffer, token);
 }
 else return {};
}

internal i64
qp_get_pos(Quick_Parser *p)
{
    if (Token *token = qp_get_token(p))
    {
        return token->pos;
 }
 else return 0;
}

internal void
qp_eat_token(Quick_Parser *p)
{
 if (p->ok)
 {
  if (p->scan_direction == Scan_Forward)
  {
   p->ok = (token_it_inc(&p->tk) != 0);
  }
  else
  {
   p->ok = (token_it_dec(&p->tk) != 0);
  }
 }
}

internal i64
get_token_pos(Quick_Parser *p)
{
 i64 result = 0;
 if (p->ok)
 {
  Token *token = qp_get_token(p);
  result = token->pos;
 }
 return result;
}

// NOTE: both float and int, for convenience
internal void
qp_eat_number(Quick_Parser *p, f32 *out)
{
 Scratch_Block scratch(&p->arena);
 
 f32 sign = 1;
 Token *token = qp_get_token(p);
 if (token)
 {
  if (token->kind == TokenBaseKind_Operator)
  {// NOTE(kv): parse sign
   String string = qp_push_token(p, scratch);
   if ( string_match_lit(string, "-") )
   {
    sign = -1;
   }
   else if (!string_match_lit(string, "+"))
   {
    p->ok = false;
   }
   
   qp_eat_token(p);
   token = qp_get_token(p);
  }
 }
 
 if (p->ok)
 {
  if (token->kind != TokenBaseKind_LiteralFloat &&
      token->kind != TokenBaseKind_LiteralInteger )
  {
   p->ok = false;
  }
 }
 
 if (p->ok)
 {
  String8 string = qp_push_token(p, scratch);
  char *cstring = to_c_string(scratch, string);
  *out = sign * gb_str_to_f32(cstring, 0);
 }
}

internal b32
qp_maybe_number(Quick_Parser *p, f32 *out)
{
 Quick_Parser save = *p;
 qp_eat_number(p, out);
 if (p->ok) { return true; }
 else
 {
  *p = save;
  return false;
 }
}

internal void
qp_eat_token_kind(Quick_Parser *p, Token_Base_Kind kind)
{
    if (p->ok)
    {
        Token *token = qp_get_token(p);
        p->ok = (token->kind == kind);
    }
    qp_eat_token(p);
}

internal b32
qp_maybe_token_kind(Quick_Parser *p, Token_Base_Kind kind)
{
    Quick_Parser p_save = *p;
    qp_eat_token_kind(p, kind);
    if (p->ok) 
        return true;
    else
 {
  *p = p_save;
  return false;
 }
}

internal b32
qp_test_string(Quick_Parser *p, String string)
{
 b32 result = false;
 if (p->ok)
 {
  Scratch_Block scratch(&p->arena);
  String token = qp_push_token(p, scratch);
  result = string_match(token, string);
 }
 return result;
}

internal b32
qp_maybe_string(Quick_Parser *p, String string)
{
 b32 result = qp_test_string(p, string);
 if (result)
 {
  qp_eat_token(p);
 }
 return result;
}

internal b32
qp_test_preprocessor(Quick_Parser *p, String string)
{
    b32 result = false;
    Token *token = qp_get_token(p);
    if (p->ok && token->kind == TokenBaseKind_Preprocessor)
    {
        Scratch_Block scratch(&p->arena);
        String token_str = qp_push_token(p, scratch);
        token_str.str++; token_str.size--;  // skip the pound
        token_str = string_skip_whitespace(token_str);
        while (*token_str.str == ' ') { token_str.str++; }
        result = string_match(token_str, string);
    }
    return result;
}

internal b32
qp_maybe_preprocessor(Quick_Parser *p, String string)
{
    b32 result = qp_test_preprocessor(p, string);
    if ( result )
    {
        qp_eat_token(p);
    }
    return result;
}

force_inline void
qp_eat_preprocessor(Quick_Parser *p, String string)
{
 p->ok = qp_maybe_preprocessor(p, string);
}

force_inline void
qp_eat_string(Quick_Parser *p, String string)
{
 p->ok = qp_maybe_string(p, string);
}

force_inline void
qp_eat_char(Quick_Parser *p, u8 chr)
{
 String string = {&chr, 1};
 qp_eat_string(p, string);
}

// NOTE returns index + 1
internal i1
char_in_string(String8 chars, char chr)
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

// NOTE returns index + 1
internal i1
qp_eat_until_char(Quick_Parser *p, String chars, 
                  i1 max_recursion)
{
 max_recursion -= 1;
 if (max_recursion < 0) { return 0; }
 else
 {
  i1 result = 0;
  while ( !result && p->ok )
  {
   Scratch_Block scratch(&p->arena);
   String token = qp_push_token(p, scratch);
   if (token.size == 1)
   {
    char character = token.str[0];
    result = char_in_string(chars, character);
    if (result)
    {
     // break
    }
    else
    {
     qp_eat_token(p);
     
     char *matching = 0;
     if (character == '(') { matching = ")"; }
     if (character == '[') { matching = "]"; }
     if (character == '{') { matching = "}"; }
     if (matching)
     {
      qp_eat_until_char(p, SCu8(matching), max_recursion);
      qp_eat_token(p);
     }
    }
   }
   else { qp_eat_token(p); }
  }
  return result;
 }
}

#define qp_eat_until_char_lit(p, cstring) \
qp_eat_until_char(p, str8lit(cstring), 64);
