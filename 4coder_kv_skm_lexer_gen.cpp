/*
  Model definition for an skm lexer.
  NOTE(kv): I still don't understand it, like wth is "fallback"?
*/

#define LANG_NAME_LOWER skm
#define LANG_NAME_CAMEL Skm

#include "lexer_generator/4coder_lex_gen_main.cpp"

function void
build_language_model(void)
{
 Scratch_Block scratch;
 u8 utf8[129];
 smh_utf8_fill(utf8);
 
 smh_set_base_character_names();
 smh_typical_token_kinds();
 
 sm_direct_token_kind(TokenBaseKind_Comment, "Text");  // NOTE(kv) I guess text are like comment? I mean it's been working for years...
 sm_direct_token_kind(TokenBaseKind_ScopeOpen, "BraceOp");
 sm_direct_token_kind(TokenBaseKind_ScopeClose, "BraceCl");
 
 // skm Operators
 Operator_Set *main_ops = sm_begin_op_set();
 //
 /*sm_select_base_kind(TokenBaseKind_ScopeOpen);
 sm_op("{");
 sm_select_base_kind(TokenBaseKind_ScopeClose);
 sm_op("}");*/
 
 sm_select_base_kind(TokenBaseKind_ParenOpen);
 sm_op("(");
 sm_op("[");
 sm_select_base_kind(TokenBaseKind_ParenClose);
 sm_op(")");
 sm_op("]");
 
 // State Machine
 State *root = sm_begin_state_machine();
 
 Flag *is_code = sm_add_flag(FlagResetRule_AutoZero);
 
#define AddState(N) State *N = sm_add_state(#N)
 
 AddState(text);
 AddState(whitespace);  // NOTE(kv) whitespace is required for the auto-indent feature to work.
 AddState(identifier);
 
 ////
 
 darray(u8) text_chars_array;
 init_dynamic(text_chars_array, scratch, 128);
 for(u8 character=0; character < 128; character++)
 {
  switch(character)
  {
   case 0:
   case '(': case '[': case '{':
   case ')': case ']': case '}':
   case ' ': case '\r': case '\t': case '\f': case '\v': case '\n':
   break;
   
   default: push(&text_chars_array, character);
  }
 }
 push(&text_chars_array, u8(0));
 u8 *text_chars = text_chars_array.items;
 
 char *whitespace_chars = " \r\t\f\v\n";
 
 {//-root
  sm_select_state(root);
  
  {
   Emit_Rule *emit = sm_emit_rule();
   sm_emit_handler_direct("EOF");
   sm_case_eof(emit);
  }
  
  sm_case(text_chars, text);
  sm_case(whitespace_chars, whitespace);
  sm_case(utf8, text);
  
  {// op
   Character_Set *op_char_set = smo_new_char_set();
   smo_char_set_union_ops_firsts(op_char_set, main_ops);
   char *char_set_array = smo_char_set_get_array(op_char_set);
   State *operator_state = smo_op_set_lexer_root(main_ops, root, "LexError");
   sm_case_peek(char_set_array, operator_state);
  }
  
  {// brace open
   Emit_Rule *emit = sm_emit_rule();
   sm_emit_handler_direct("BraceOp");
   sm_case("{", emit);
  }
  {// brace close
   Emit_Rule *emit = sm_emit_rule();
   sm_emit_handler_direct("BraceCl");
   sm_case("}", emit);
  }
  
  {// lex error
   Emit_Rule *emit = sm_emit_rule();
   sm_emit_handler_direct("LexError");
   sm_fallback(emit);
  }
 }
 
 {// text
  sm_select_state(text);
  sm_case(text_chars, text);
  sm_case(utf8, text);
  {
   Emit_Rule *emit = sm_emit_rule();
   sm_emit_handler_direct("Text");
   sm_fallback_peek(emit);
  }
 }
 
 {// whitespace
  sm_select_state(whitespace);
  sm_case(whitespace_chars, whitespace);
  {
   Emit_Rule *emit = sm_emit_rule();
   sm_emit_handler_direct("Whitespace");
   sm_fallback_peek(emit);
  }
 }
}
//-