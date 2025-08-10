//-
struct File_Position
{
 String filepath;
 i64 position;
};
struct Link_Table_Node
{
 Link_Table_Node *next_in_hash;
 String value;
 File_Position example_location;
 b32 is_note;
};

typedef sarray(Link_Table_Node *) Link_Table;

struct Link_State
{// @collect_links_and_notes @link_processor_call
 Arena *arena;
 String filepath;
 Link_Table table;
};
function Link_Table_Node *
maybe_add_link_inner(Link_State *state, String new_link, File_Position location)
{
 u32 hash = (gb_murmur32(strexpand2(new_link))
             % state->table.count);
 
 Link_Table_Node *first_entry = state->table[hash];
 Link_Table_Node *result_node = 0;
 for(Link_Table_Node *node = first_entry;
     node;
     node = node->next_in_hash)
 {
  if(node->value == new_link)
  {// NOTE Found a match.
   result_node = node;
  }
 }
 
 if(not result_node)
 {// NOTE Add new entry
  Link_Table_Node new_entry = {};
  new_entry.next_in_hash = first_entry;
  new_entry.value = new_link;
  new_entry.example_location = location;
  result_node = state->table[hash] = push_value(state->arena, new_entry);
 }
 
 return result_node;
}
myinline void
maybe_add_link(Link_State *state, String new_link,
               File_Position location)
{
 maybe_add_link_inner(state, new_link, location);
}
function b32
maybe_add_note(Link_State *state, String new_link)
{
 b32 added = 0;
 Link_Table_Node *node = maybe_add_link_inner(state, new_link, {});
 if(not node->is_note)
 {
  node->is_note = 1;
  added = 1;
 }
 return added;
}

function void
link_parse_comment(Link_State *state, String comment_string, i64 begin_position)
{
 for(u8 *character = &comment_string[0];
     character < comment_string.str + comment_string.count;
     )
 {
  u8 c0 = *character++;
  if(c0 == '@')
  {//-Link
   u8 *link_begin = character;
   while(character_is_alnum(*character)){ character++; }
   u8 *link_end = character;
   if(link_end > link_begin)
   {
    String new_link = {.str=link_begin, .count=u64(link_end-link_begin)};
    File_Position location = {};
    location.filepath = state->filepath;
    location.position = begin_position + (link_begin-comment_string.str);
    maybe_add_link(state, new_link, location);
   }
  }
  else if(c0 == ';')
  {//-Comment note (maybe?)
   u8 *note_begin = character;
   while(character_is_alnum(*character)){ character++; }
   u8 *note_end = character;
   
   if(note_end > note_begin)
   {
    String new_note = {.str=note_begin, .count=u64(note_end-note_begin)};
    b32 added = maybe_add_note(state, new_note);
    if(not added)
    {// NOTE comment notes must be unique
     myprintf("ERROR: Comment note %S not unique\n", new_note);
     InvalidCodePath;
    }
   }
  }
 }
}

function void
link_parse_body(Link_State *state, Ed_Parser *p)
{
 ep_char_inc_all(p, '{');
 i32 nest_level = 0;
 while(p->ok_)
 {
  Token *token = ep_get_token(p);
  String token_string = ep_print_token(p, token);
  
  if(0);
  
  else if(token_string == '{')
  {
   nest_level++;
  }
  else if(token_string == '}')
  {
   if (nest_level == 0) {
    break;
   } else {
    nest_level--; 
   }
  }
  else if(token->kind == TokenBaseKind_Comment)
  {
   link_parse_comment(state, token_string, token->pos);
  }
  
  ep_eat_inc_all(p);
 }
}
function b32
collect_links_and_notes(Arena *arena, Link_Table table, Lexed_File source)
{
 Link_State state_value = {};
 Link_State *state = &state_value;
 state->arena = arena;
 state->table = table;
 state->filepath = source.path;
 
 Ed_Parser parser_value = ed_parser_from_token_list(source.data, source.token_list);
 Ed_Parser *p = &parser_value;
 b32 parsing = 1;
 
 for(Token *token0 = ep_get_token(p);
     parsing;
     )
 {
  String token0_string = ep_print_token(p, token0);
  switch(token0->kind)
  {
   case TokenBaseKind_Preprocessor:
   {
    while(1)
    {
     Token *token = ep_get_token(p);
     if(token->flags & TokenBaseFlag_PreprocessorBody)
     {
      ep_eat_inc_all(p);
     }
     else break;
    }
   }break;
   
   case TokenBaseKind_EOF:
   {
    parsing = 0;
   }break;
   
   case TokenBaseKind_Comment:
   {
    link_parse_comment(state, token0_string, token0->pos);
   }break;
   
   case TokenBaseKind_Keyword:
   {
    switch(token0->sub_kind)
    {
     case TokenCppKind_Struct:
     case TokenCppKind_Union:
     case TokenCppKind_Enum:
     {
      ep_eat(p);
      Token *token2 = ep_get_token(p);
      if(token2->kind == TokenBaseKind_Identifier)
      {
       String type_name = ep_print_token(p, token2);
       maybe_add_note(state, type_name);
      }
     }break;
     
     case TokenCppKind_Typedef:
     {
      ep_recovery_block(p);  // TODO(kv) Implement typedef?
      ep_eat(p);
      ep_id(p);  // note The target type
      String type_name = ep_id(p);
      ep_char_inc_all(p, ';');
      if(p->ok_)
      {
       maybe_add_note(state, type_name);
      }
     }break;
    }
   }break;
   
   case TokenBaseKind_Identifier:
   {
    if(is_function_keyword(token0_string))
    {//-Function
     // NOTE Recovery is needed, because we use bunch of weird macros in here,
     // which can easily trick up the parser.
     ep_recovery_block(p);  
     ep_eat(p);
     ep_id(p);  // NOTE The type
     
     while(ep_maybe_char(p, '*'));
     
     String function_name = ep_id(p);
     maybe_add_note(state, function_name);
     
     // NOTE parameters
     ep_char(p, '(');
     ep_eat_until_char(p, ')');
     ep_eat(p);
     
     link_parse_body(state, p);
    }
    else if(token0_string == strlit("api_table"))
    {
     ep_eat(p);
     String api_name = ep_id(p);
     maybe_add_note(state, api_name);
     ep_char(p, '{');
     ep_eat_until_char(p, '}');
    }
   }break;
  }
  
  parsing = parsing and p->ok_;
  if(parsing)
  {
   Token *token0_old = token0;
   token0 = ep_get_token(p);
   if(token0 == token0_old)
   {
    ep_eat_inc_all(p);
    token0 = ep_get_token(p);
   }
  }
 }
 
 if(not p->ok_)
 {
  i64 fail_pos = ep_get_fail_pos(p);
  // @kv_jump_syntax
  myprintf("[kv][%S][%lld] link collection parse error\n",
           source.path, fail_pos);
  
 }
 
 return p->ok_;
}
//-