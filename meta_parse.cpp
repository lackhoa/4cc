function Ed_Parser
m_parser_from_token_list(Arena *arena, String string, Token_List &token_list){
 Token_Iterator token_it = make_token_iterator(token_iterator(0, &token_list));
 Ed_Parser parser = make_ep_from_string(string, token_it);
 return parser;
}
function Ed_Parser
m_parser_from_string(Arena *arena, String string){
 Token_List token_list = lex_full_input_cpp(arena, string);
 return m_parser_from_token_list(arena, string, token_list);
}
function void
parse_type_and_name(Ed_Parser *p, Parsed_Type *otype, String *oname){
 //NOTE(kv) We're cheesing the type HARD!
 String type_name = ep_id(p);
 *otype = make_type_named(type_name);
 while(ep_maybe_char(p, '*')){
  otype->kind = Parsed_Type_Pointer;
  otype->count++;
 }
 *oname = ep_id(p);
 if(ep_maybe_char(p, '[')){
  otype->kind = Parsed_Type_Array;
  otype->count = ep_i1(p);
  ep_char(p, ']');
 }
}
function void
parse_struct_member(Ed_Parser *p, M_Struct_Member *member){
 if(ep_maybe_id(p, "tagged_by")){
  mpa_parens{ member->discriminator = ep_id(p); }
 }
 parse_type_and_name(p, &member->type, &member->name);
 ep_skip_semicolons(p);
}
inline M_Struct_Member
parse_struct_member(Ed_Parser *p){
 M_Struct_Member member = {};
 parse_struct_member(p, &member);
 return member;
}
function M_Struct_Member
struct_member_from_string(String string){
 Scratch_Block scratch;
 Ed_Parser parser = m_parser_from_string(scratch, string);
 return parse_struct_member(&parser);
}
inline M_Struct_Member
struct_member_from_string(char *cstring){
 return struct_member_from_string(SCu8(cstring));
}
function M_Struct_Members
parse_struct_body(Arena *arena, Ed_Parser *p){
 M_Struct_Members result;
 init_dynamic(result, arena);
 m_brace_open(p);
 while(p->ok_ && !m_maybe_brace_close(p)){
  M_Struct_Member *member = &result.push_zero();
  parse_struct_member(p, member);
 }
 return result;
}
inline M_Struct_Members
parse_struct_body(Arena *a, char *string){
 Scratch_Block scratch;
 Ed_Parser parser = m_parser_from_string(scratch, SCu8(string));
 return parse_struct_body(a, &parser);
}

inline void
meta_parse_key(Ed_Parser *p, String key){
 ep_id(p, key);
 ep_char(p, '=');
}
inline b32
meta_maybe_key(Ed_Parser *p, String key){
 b32 result = false;
 if(ep_maybe_id(p, key)){
  ep_char(p, '=');
  result = p->ok_;
 }
 return result;
}
function b32
test_char_in(Ed_Parser *p, String terminators){
 String chr = ep_print_token(p);
 if(chr.count == 1){
  for_i32(i,0,i32(terminators.count)){
   if(terminators[i] == chr[0]){
    return true;
   }
  }
 }
 return false;
}
function String
k_string_from_token_to_current(Ed_Parser *p, Token *token_start){
 String start = ep_print_given_token(p, token_start);
 String end   = ep_print_token(p);
 String result = {start.str, u64(end.str - start.str)};
 return result;
}
//-