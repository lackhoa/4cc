#pragma once

//-NOTE(kv) Annoying parsing job
inline void m_brace_open(Ed_Parser *p)  { ep_char(p, '{'); }
inline void m_bracket_open(Ed_Parser *p){ ep_char(p, '['); }
inline void m_paren_open(Ed_Parser *p)  { ep_char(p, '('); }
inline void m_brace_close(Ed_Parser *p){ ep_char(p, '}'); }
inline void m_paren_close(Ed_Parser *p){ ep_char(p, ')'); }
inline b32 m_maybe_bracket_open(Ed_Parser *p){ return ep_maybe_char(p, '['); }
inline b32 m_maybe_paren_close  (Ed_Parser *p){ return ep_maybe_char(p, ')'); }
inline b32 m_maybe_bracket_close(Ed_Parser *p){ return ep_maybe_char(p, ']'); }
inline b32 m_maybe_brace_close  (Ed_Parser *p){ return ep_maybe_char(p, '}'); }
#define mpa_parens     defer_block(m_paren_open(p), m_paren_close(p))
#define mpa_braces     defer_block(m_brace_open(p), m_brace_close(p))
//-
function b32
ep_maybe_char_inc_all(Ed_Parser *parser, char c)
{
 b32 result = ep_test_char(parser, c);
 if(result){
  ep_eat_inc_all(parser);
 }
 return result;
}
function void
ep_char_inc_all(Ed_Parser *parser, char c)
{
 if(not ep_test_char(parser, c)){
  parser->fail();
 }
 ep_eat_inc_all(parser);
}
function String
ep_id_inc_all(Ed_Parser *parser)
{
 String result = {};
 auto kind = ep_get_kind(parser);
 parser->set_ok(kind == TokenBaseKind_Identifier ||
                kind == TokenBaseKind_Keyword);
 if(parser->ok_){
  result = ep_print_token(parser);
 }
 ep_eat_inc_all(parser);
 return result;
}
function Ed_Parser
ed_parser_from_token_list(String source, Token_List &token_list)
{
 Token_Iterator token_it = make_token_iterator(token_iterator(0, &token_list));
 Ed_Parser result = make_ep_from_string(source, token_it);
 return result;
}
function Ed_Parser
ed_parser_from_string(Arena *arena, Stringz string)
{
 Token_List token_list = lex_full_input_cpp(arena, string);
 return ed_parser_from_token_list(string, token_list);
}
//-