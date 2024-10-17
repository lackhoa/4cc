// NOTE(kv): This is yet another parser based on 4coder's tokenizer
// Should it be a separate parser from the other one using STB_Parser?
// Maybe: it has bidirectional parsing, it uses another token type,
// it has token types for C++

// NOTE(kv): The "maybe" function should be implemented using rewind,
// because using p->ok makes the logic simpler.
// However, it's slower and in the simple case we should just duplicate the logic.

// NOTE(kv): view ad_stb_parser.cpp

#include "4ed_kv_parser.h"

//-
function Token *ep__get_token_please(Ed_Parser *p);
function i1
count_newlines_in_string(String string) {
 i1 result = 0;
 for_i1(index,0,i1(string.len)) {
  if (string.str[index] == '\n') { 
   result++;
  }
 }
 return result;
}
function Line_Column
ep_get_token_location(Ed_Parser *p, Token *input_token){
 Line_Column result = {};
 // NOTE: Time for us to do some gymnastic
 auto it0 = p->original_token_it;
 i64 begin_line_pos = 0;
 result.line = 1;  // NOTE(kv): Line number starts with 1, for good reason.
 while(true){
  Token *token = token_it_inc_all(&it0);
  if(!token || token->pos > input_token->pos){
   break;
  }
  result.column = i1(token->pos - begin_line_pos);
  if(token->kind == TokenBaseKind_Whitespace){
   // NOTE: Increase line number
   Scratch_Block scratch;
   String token_string = ep_print_given_token(scratch, p, token);
   for_i1 (index,0,i1(token_string.len)){
    if(token_string.str[index] == '\n'){
     result.line++;
     begin_line_pos = token->pos+index+1;
    }
   }
  }
 }
 return result;
}
inline Line_Column
ep_get_fail_location(Ed_Parser *p){
 Token *token = ep__get_token_please(p);
 return ep_get_token_location(p, token);
}
inline Line_Column
ep_get_scope_location(Ed_Parser *p){
 return ep_get_token_location(p, p->scope_.start_location);
}
//-Constructors
#if ED_PARSER_BUFFER
function Ed_Parser
make_ep_from_buffer(App *app, Buffer_ID buffer, Token_Iterator const&it,
                    Arena *string_arena=0,
                    Scan_Direction scan_direction=Scan_Forward){
 Ed_Parser result = {
  .ok_               = true,
  .scan_direction    = scan_direction,
  .it                = it,
  .string_arena      = string_arena,
  .original_token_it = it,
  .Token_Gen_Type    = TG_Buffer,
  .Token_Gen_Buffer  = {
   .app   =app,
   .buffer=buffer,
  },
 };
 return result;
}
#endif
function Ed_Parser
make_ep_from_string(String string, Token_Iterator const&it){
 Ed_Parser result = {
  .ok_               = true,
  .scan_direction    = Scan_Forward,
  .it                = it,
  .original_token_it = it,
  .Token_Gen_Type    = TG_String,
  .Token_Gen_String  = {
   .source=string,
  },
 };
 result.scope_.start_location = &stub_token;
 return result;
}
//-
inline Token *
ep__get_token_please(Ed_Parser *p){
 return token_it_read(&p->it);
}
//NOTE(kv) If you get back a token, it will never be past the end
inline Token *
ep_get_token(Ed_Parser *p){
 Token *token = &stub_token;
 if(p->ok_){
  token = ep__get_token_please(p);
 }
 return token;
}
function Token *
ep_get_token_delta(Ed_Parser *p, i32 delta){
 Token *result = 0;
 if(delta < 0){
  i32 inverse_delta = -delta;
  for_repeat(inverse_delta) { token_it_dec(&p->it); }
  result = ep_get_token(p);
  for_repeat(inverse_delta) { token_it_inc(&p->it); }
 }else{
  todo_incomplete;
 }
 return result;
}
function String
ep_print_given_token(Arena *arena, Ed_Parser *p, Token *token){
 String result = {};
 if (token) {
  switch(p->Token_Gen_Type) {
#if ED_PARSER_BUFFER
   case TG_Buffer:{
    auto &tg = p->Token_Gen_Buffer;
    result = push_token_lexeme(tg.app, arena, tg.buffer, token);
   }break;
#endif
   case TG_String:{
    result = string_substring(p->Token_Gen_String.source, Ii64(token));
   } break;
   invalid_default_case;
  }
 }
 return result;
}
inline String
ep_print_given_token(Ed_Parser *p, Token *token){
 return ep_print_given_token(p->string_arena,p,token);
}
function String
ep_print_token(Arena *arena, Ed_Parser *p){
 if(arena==0){ arena=p->string_arena; }
 Token *token = ep_get_token(p);
 return ep_print_given_token(arena, p, token);
}
inline String
ep_print_token(Ed_Parser *p){ return ep_print_token(p->string_arena, p); }
function void
ep_print_token(Printer &printer, Ed_Parser *p){
 //NOTE Print with a printer
 Token *token = ep_get_token(p);
 if(token){
  switch(p->Token_Gen_Type){
   case TG_String:{
    printer < string_substring(p->Token_Gen_String.source, Ii64(token));
   }break;
   invalid_default_case;
  }
 }
}
function i64
ep_get_pos(Ed_Parser *p) {
 if (Token *token = ep_get_token(p)) {
  return token->pos;
 } else { return 0; }
}
function void
ep_eat_token(Ed_Parser *p){
 if(p->ok_){
  switch(p->Token_Gen_Type){
   case TG_String:
   case TG_Buffer:{
    if(p->scan_direction == Scan_Forward){
     p->set_ok(token_it_inc(&p->it) != 0);
    }else{
     p->set_ok(token_it_dec(&p->it) != 0);
    }
   }break;
   invalid_default_case;
  }
 }
}
function void
ep_eat_token_all(Ed_Parser *p){
 if(p->ok_){
  switch(p->Token_Gen_Type){
   case TG_String:
   case TG_Buffer:{
    if(p->scan_direction == Scan_Forward){
     p->set_ok(token_it_inc_all(&p->it) != 0);
    }else{
     p->set_ok(token_it_dec_all(&p->it) != 0);
    }
   }break;
   invalid_default_case;
  }
 }
}
//-
struct Parse_Number_Result{
 b32 success;
 b32 is_float;
 f64 number;
};
//-NOTE: Both float and int, for convenience
//IMPORTANT(kv) Includind the sign in your "number" isn't useful for a real parser
//  with arbitrary expressions (like f.ex "pi-0.5f").
function Parse_Number_Result
ep_number(Ed_Parser *p){
 Scratch_Block scratch;
 f64 sign = 1;
 Token *token = ep_get_token(p);
 if(token){
  if(token->kind == TokenBaseKind_Operator)
  {// NOTE(kv): parse sign
   String string = ep_print_token(scratch, p);
   if(string_match_lit(string, "-")){
    sign = -1;
   }else if(!string_match_lit(string, "+")){
    p->set_ok(false);
   }
   
   ep_eat_token(p);
   token = ep_get_token(p);
  }
 }
 Parse_Number_Result result = {};
 if(token->kind == TokenBaseKind_LiteralFloat){
  result.is_float = true;
 }else if(token->kind != TokenBaseKind_LiteralInteger){
  p->fail();
 }
 {
  String string = ep_print_token(scratch, p);
  result.number = sign * str_to_f64(string, 0);
 }
 result.success = p->ok_;
 return result;
}
function i1
ep_i1(Ed_Parser *p){
 Scratch_Block scratch;
 i1 sign = 1;
 Token *token = ep_get_token(p);
 if(token){
  if(token->kind == TokenBaseKind_Operator){
   // NOTE(kv): parse sign
   String string = ep_print_token(scratch, p);
   if(string == strlit("-")){
    sign = -1;
   }else if(string != strlit("+")){
    p->fail();
   }
   
   ep_eat_token(p);
   token = ep_get_token(p);
  }
 }
 p->set_ok(token->kind == TokenBaseKind_LiteralInteger);
 i1 result = 0;
 if(p->ok_){
  String8 string = ep_print_token(scratch, p);
  result = i1(sign * str_to_i64(string, 0, 0));
  ep_eat_token(p);
 }
 return result;
}
function Parse_Number_Result
ep_maybe_number(Ed_Parser *p){
 Ed_Parser save = *p;
 Parse_Number_Result result = ep_number(p);
 if(!result.success){
  *p = save;
 }
 return result;
}
//-
function Token_Base_Kind
ep_get_kind(Ed_Parser *p){
 Token_Base_Kind result = 0;
 Token *token = ep_get_token(p);
 if (token){
  result = token->kind;
 }
 return result;
}

function void
ep_eat_kind(Ed_Parser *p, Token_Base_Kind kind){
 Token *token = ep_get_token(p);
 if (token) {
  p->set_ok(token->kind == kind);
 }
 ep_eat_token(p);
}

function b32
ep_maybe_kind(Ed_Parser *p, Token_Base_Kind kind){
 b32 result = false;
 Token *token = ep_get_token(p);
 result = (token->kind == kind);
 if(result){ ep_eat_token(p); }
 return result;
}
function b32
ep_at_eof(Ed_Parser *p){
 b32 result = false;
 if(p->ok_){
  Token *token = ep_get_token(p);
  result = (token->kind == TokenBaseKind_EOF);
  if(result){
   ep_get_token(p);
  }
 }
 return result;
}
//-
function b32
ep_test_id(Ed_Parser *p, String string){
 b32 result = false;
 if (p->ok_){
  Scratch_Block scratch;
  String token = ep_print_token(scratch, p);
  result = string_match(token, string);
 }
 return result;
}
force_inline b32
ep_test_id(Ed_Parser *p, char *string){
 String s = SCu8(string);
 return ep_test_id(p, s);
}
function b32
ep_maybe_id(Ed_Parser *p, String string){
 b32 result = ep_test_id(p, string);
 if (result) { ep_eat_token(p); }
 return result;
}
force_inline b32
ep_maybe_id(Ed_Parser *p, char *string){
 String s = SCu8(string);
 return ep_maybe_id(p, s);
}
function String
ep_maybe_id(Ed_Parser *p){
 String result = {};
 if(Token *token = ep_get_token(p)){
  if(token->kind == TokenBaseKind_Identifier){
   result = ep_print_token(p->string_arena, p);
   ep_eat_token(p);
  }
 }
 return result;
}
//-
function b32
ep_test_preprocessor(Ed_Parser *p, String string){
 b32 result = false;
 Token *token = ep_get_token(p);
 if(p->ok_ && token->kind == TokenBaseKind_Preprocessor){
  Scratch_Block scratch;
  String token_str = ep_print_token(scratch, p);
  token_str.str++; token_str.size--;  // skip the pound
  token_str = string_skip_whitespace(token_str);
  result = string_match(token_str, string);
 }
 return result;
}
function b32
ep_maybe_preprocessor(Ed_Parser *p, String string) {
 b32 result = ep_test_preprocessor(p, string);
 if(result){
  ep_eat_token(p);
 }
 return result;
}
inline void
ep_eat_preprocessor(Ed_Parser *p, String string){
 p->set_ok(ep_maybe_preprocessor(p, string));
}
//-
//NOTE(kv) Most of the time we don't care if an identifier is a keyword or not,
//  f.ex "char" is a keyword, but it's a type just like any other.
//  We only really care when we do "actual" compiler stuff,
//  such as preventing user from redefining keywords.
//  Plus the syntax highlighter cares.
function String
ep_id(Ed_Parser *p, String test_id){
 String result = {};
 auto kind = ep_get_kind(p);
 p->set_ok(kind == TokenBaseKind_Identifier ||
           kind == TokenBaseKind_Keyword);
 {
  Scratch_Block scratch;
  String string = ep_print_token(scratch, p);
  p->set_ok(string == test_id);
 }
 ep_eat_token(p);
 return result;
}
function String
ep_id(Ed_Parser *p){
 String result = {};
 auto kind = ep_get_kind(p);
 p->set_ok(kind == TokenBaseKind_Identifier ||
           kind == TokenBaseKind_Keyword);
 if(p->ok_){
  result = ep_print_token(p);
 }
 ep_eat_token(p);
 return result;
}
//-
function i1
ep__char_in_string_old(String chars, char chr){
 // NOTE Returns index + 1
 i1 result = 0;
 for_u32(index, 0, (u32)chars.size){
  if(chars.str[index] == chr){
   result = index + 1;
   break;
  }
 }
 return result;
}
function b32
ep__char_in_string_new(String chars, char chr){
 for_u32(index, 0, (u32)chars.size){
  if(chars.str[index] == chr){
   return true;
  }
 }
 return false;
}
function b32
ep_test_char(Ed_Parser *p, char c){
 Scratch_Block scratch;  // @slow, also sometimes the arena isn't necessary
 String string = ep_print_token(scratch, p);
 return (string.len == 1 &&
         string.str[0] == c);
}
function b32
ep_maybe_char(Ed_Parser *p, char c){
 b32 result = ep_test_char(p, c);
 if (result) {
  ep_eat_token(p);
 }
 return result;
}
function void
ep_char(Ed_Parser *p, char c){
 p->set_ok( ep_maybe_char(p, c) );
}
//-
//NOTE(kv) Excludes the nil terminator
function char
ep_eat_until_char(Ed_Parser *p, String terminators){
 char result = 0;
 {
  while(!result && p->ok_){
   Scratch_Block scratch;
   String token = ep_print_token(scratch, p);//TODO(kv) OMG this is bad!
   b32 should_eat = true;
   if(token.len == 1){
    char char0 = token.str[0];
    if(ep__char_in_string_new(terminators, char0)){
     result = char0;
     should_eat = false;
    }else{
     u8 closer = get_matching_group_closer(char0);
     if(closer){
      ep_eat_token(p);
      ep_eat_until_char(p, String{&closer, 1});
     }else{
      u8 opener = get_matching_group_opener(char0);
      if(opener){//NOTE(kv) Encountered unbalanced group closer, we should stop!
       p->fail();
      }
     }
    }
   }
   if(should_eat){
    ep_eat_token(p);
   }
  }
 }
 return result;
}
function String
ep_capture_until_char(Ed_Parser *p, String terminators){
 kv_assert(p->Token_Gen_Type == TG_String);
 String result = {};
 if(p->ok_){
  Token *first_token = ep_get_token(p);
  char res = ep_eat_until_char(p, terminators);
  if(res){
   // NOTE(kv): The last token is one token to the left of the terminator
   Token *last_token = ep_get_token_delta(p, -1);
   i64 last_pos = last_token->pos + last_token->size;
   result = string_substring(p->Token_Gen_String.source,
                             Ii64(first_token->pos, last_pos));
  }
 }
 return result;
}
function String
ep_capture_until_char(Ed_Parser *p, char terminator){
 return ep_capture_until_char(p, SCu8(terminator));
}
function void
ep_eat_until_char_simple(Ed_Parser *p, char c){
 while(p->ok_){
  if(ep_maybe_char(p, c)){
   break;
  }else{
   ep_eat_token(p);
  }
 }
}
//-
inline void
ep_skip_semicolons(Ed_Parser *p)
{//NOTE(kv) We may add semicolon for editor indentation.
 //todo(kv) maybe we can test the token kind for this?
 while(ep_maybe_kind(p, TokenBaseKind_StatementClose)){  }
}
function void
ep_skip_comments_and_spaces(Ed_Parser *p){
 Token *token = ep_get_token(p);
 if(token->kind == TokenBaseKind_Comment ||
    token->kind == TokenBaseKind_Whitespace){
  ep_eat_token(p);
 }
}
//-
#undef ED_PARSER_BUFFER
//-EOF