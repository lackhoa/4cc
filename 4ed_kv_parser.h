#pragma once

#ifndef ED_PARSER_BUFFER
#    define ED_PARSER_BUFFER 0
#endif

enum Token_Gen_Type{
 TG_None,
 TG_Buffer,
 TG_String,
};

#if ED_PARSER_BUFFER //-
struct Token_Gen_Buffer{
 App *app;
 Buffer_ID buffer;
};
#endif //-ED_PARSER_BUFFER

struct Line_Column{ i1 line; i1 column; };
struct EP_Scope{
 Token *start_location;
 String name;
};
struct Ed_Parser
{
 b32 ok_;
 b32 recoverable;
 Scan_Direction direction;
 Token_Iterator it;
 Arena *string_arena;
 Token_Iterator original_token_it;
 EP_Scope scope_;
 //-
 Token_Gen_Type Token_Gen_Type;
 union{
#if ED_PARSER_BUFFER
  Token_Gen_Buffer Token_Gen_Buffer;
#endif
  // or
  String source;
 };
 //-
 // NOTE(kv): We don't allow setting the value to true (I'm open for a better name)
 myinline void set_ok(b32 value){
  if(not value){
   ok_ = false;
   if(not recoverable){
    breakhere;
   }
  }
 }
 myinline void fail() { set_ok(false); }
};
//-
function void
ep_set_scope(Ed_Parser *p, String name, Token *token){
 if(p->ok_){
  //NOTE(kv) Since the scope is to aid debugging, we don't change it after failure.
  p->scope_.start_location = token;
  p->scope_.name = name;
 }
}
inline void
ep_set_scope(Ed_Parser *p, EP_Scope scope){
 ep_set_scope(p, scope.name, scope.start_location);
}
#define ep_scope_block(parser,...) \
EP_Scope line_unique_var = parser->scope_; \
ep_set_scope(parser, __VA_ARGS__); \
defer(ep_set_scope(parser, line_unique_var));
//-
function Token *ep_get_token(Ed_Parser *p);
function String ep_print_token(Arena *arena, Ed_Parser *p);
function String ep_print_token(Ed_Parser *p);
function String ep_print_token(Arena *arena, Ed_Parser *p, Token *token);
//-
struct Ed_Parser_Recovery{
 Ed_Parser saved_parser;
 Ed_Parser *pointer;
 //-
 ~Ed_Parser_Recovery(){
  if(pointer->recoverable){
   //NOTE(kv) We allow hard failing by setting "recoverable" to false.
   if(not pointer->ok_){
    *pointer = saved_parser;
   }
   //NOTE(kv) Restore recoverable, even if you don't fail.
   pointer->recoverable = saved_parser.recoverable;
  }
 }
};
function Ed_Parser_Recovery
ed_parser_recovery_begin(Ed_Parser *pointer){
 Ed_Parser_Recovery recovery = {};
 recovery.saved_parser = *pointer;
 recovery.pointer      = pointer;
 pointer->recoverable = true;
 return recovery;
}
//NOTE(kv) Sadly this cannot be a defer block, because we need to store the recovery on the stack!
#define ep_recovery_block(parser_pointer) \
Ed_Parser_Recovery line_unique_var = ed_parser_recovery_begin(parser_pointer);
//-Constructors
#if ED_PARSER_BUFFER
function Ed_Parser
make_ep_from_buffer(App *app, Buffer_ID buffer, Token_Iterator const&it,
                    Arena *string_arena=0,
                    Scan_Direction direction=Scan_Forward){
 b32 ok;
 if(it.kind == TokenIterator_Array){
  ok = it.array.count != 0;
 }else if(it.kind == TokenIterator_List){
  ok = it.list.node_count != 0;
 }else{
  invalid_code_path;
 }
 Ed_Parser result = {
  .ok_               = ok,
  .direction         = direction,
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
  .direction         = Scan_Forward,
  .it                = it,
  .original_token_it = it,
  .Token_Gen_Type    = TG_String,
  .source            = string,
 };
 result.scope_.start_location = &stub_token;
 return result;
}
//-