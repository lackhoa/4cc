function Ed_Parser
m_parser_from_string(Arena *arena, String string){
 Token_List token_list = lex_full_input_cpp(arena, string);
 Token_Iterator token_it = make_token_iterator(token_iterator(0, &token_list));
 Ed_Parser parser = make_ep_from_string(string, token_it);
 return parser;
}
function void
parse_struct_member(Ed_Parser *p, Meta_Struct_Member *member){
 if(ep_maybe_id(p, "tagged_by")){
  mpa_parens{ member->discriminator = ep_id(p); }
 }
 {
  member->type = ep_id(p);
  while(ep_maybe_char(p, '*')){ member->type_star_count++; }
 }
 member->name = ep_id(p);
 ep_consume_semicolons(p);
}
function Meta_Struct_Members
parse_struct_body(Arena *arena, Ed_Parser *p){
 auto result = dynamic_array<Meta_Struct_Member>(arena);
 m_brace_open(p);
 while(p->ok_ && !m_maybe_brace_close(p)){
  Meta_Struct_Member *member = result.push_zero();
  parse_struct_member(p, member);
 }
 return result;
}
inline Meta_Struct_Members
parse_struct_body(Arena *a, char *string){
 Scratch_Block scratch;
 Ed_Parser parser = m_parser_from_string(scratch, SCu8(string));
 return parse_struct_body(a, &parser);
}

inline void
meta_parse_key(Ed_Parser *p, char *key) {
 ep_id(p, key); ep_char(p, '=');
}
inline b32
meta_maybe_key(Ed_Parser *p, char *key)
{
 b32 result = false;
 if ( ep_maybe_id(p, key) ) {
  ep_char(p, '=');
  result = p->ok_;
 }
 return result;
}

//-