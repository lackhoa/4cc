/*
Model definition for an skm lexer.
*/

// TOP

#define LANG_NAME_LOWER skm
#define LANG_NAME_CAMEL Skm

#include "lexer_generator/4coder_lex_gen_main.cpp"
#define KV_IMPLEMENTATION
#include "kv.h"

internal void
build_language_model(void)
{
    u8 utf8[129];
    smh_utf8_fill(utf8);
    
    smh_set_base_character_names();
    smh_typical_tokens();
    
    // skm Operators
    Operator_Set *main_ops = sm_begin_op_set();
    
    sm_select_base_kind(TokenBaseKind_ScopeOpen);
    sm_op("{");
    sm_select_base_kind(TokenBaseKind_ScopeClose);
    sm_op("}");
    sm_select_base_kind(TokenBaseKind_ParentheticalOpen);
    sm_op("(");
    sm_op("[");
    sm_select_base_kind(TokenBaseKind_ParentheticalClose);
    sm_op(")");
    sm_op("]");

    // State Machine
    State *root = sm_begin_state_machine();
    
#define AddState(N) State *N = sm_add_state(#N)
    
    AddState(identifier);
    AddState(whitespace);
    
    char *whitespace_chars = " \t\r\f\v\n";
    char *non_whitespace_chars = 0;
    for (i32 i=0; i < 128; i++) {
      arrput(non_whitespace_chars, i);
    }
    for (i32 i=0; i < strlen(whitespace_chars); i++) {
      arrdel(non_whitespace_chars, whitespace_chars[i]);
    }

    sm_select_state(root);
    {
        Emit_Rule *emit = sm_emit_rule();
        sm_emit_handler_direct("EOF");
        sm_case_eof(emit);
    }
    sm_case(whitespace_chars, whitespace);
    sm_case(non_whitespace_chars, identifier);

    ///

    sm_select_state(identifier);
    sm_case(non_whitespace_chars, identifier);
    sm_case(whitespace_chars, whitespace);

    ////

    sm_select_state(whitespace);
    sm_case(whitespace_chars, whitespace);
    {
        Emit_Rule *emit = sm_emit_rule();
        sm_emit_handler_direct("Whitespace");
        sm_fallback_peek(emit);
    }
}

// BOTTOM
