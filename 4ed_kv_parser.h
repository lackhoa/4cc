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

struct Token_Gen_String{ String source; };
struct Line_Column{ i1 line; i1 column; };
struct EP_Scope{
 Token *start_location;
 String name;
};
struct Ed_Parser{
 b32 ok_;
 b32 recoverable;
 Scan_Direction scan_direction;
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
  Token_Gen_String Token_Gen_String;
 };
 //-
 // NOTE(kv): We don't allow setting the value to true (I'm open for a better name)
 void set_ok(b32 value){
  if(not value){
   ok_ = false;
   if(not recoverable){
    breakhere;
   }
  }
 }
 inline void fail() { set_ok(false); }
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
function String ep_print_given_token(Arena *arena, Ed_Parser *p, Token *token);
//-
struct Ed_Parser_Recovery{
 Ed_Parser recovery;
 Ed_Parser *pointer;
 //-
 inline ~Ed_Parser_Recovery(){
  if(not pointer->ok_){
   *pointer = recovery;
  }
  //NOTE(kv) Restore the recoverable status
  pointer->recoverable = recovery.recoverable;
 }
};
function Ed_Parser_Recovery
make_ed_parser_recovery(Ed_Parser *pointer){
 Ed_Parser_Recovery recovery = {};
 recovery.recovery = *pointer;
 recovery.pointer  = pointer;
 pointer->recoverable = true;
 return recovery;
}
#define ep_recovery_block(parser_pointer) \
Ed_Parser_Recovery line_unique_var = make_ed_parser_recovery(parser_pointer);
//-