//-
struct T_Table{
 String name;
 arrayof<String>   field_names;
 arrayof<String *> items;
};
struct Template_Node{
 String text;
 i32 field_index;
 b32 quoted;
};
function i32
get_field_count(T_Table *table){
 return (table->field_names.count);
}
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
//-
function T_Table *
get_meta_list_by_name(arrayof<T_Table> *lists, String name)
{
 T_Table *result = 0;
 for_i32(list_index, 0, lists->count){
  result = &lists->items[list_index];
  if(result->name == name){
   break;
  }
  result = 0;
 }
 return result;
}
//-
function String
template_parse_string(Ed_Parser *parser)
{
 String result = ep_maybe_id(parser);
 
 if(not result.count){
  //-The `(...) syntax that I definitely didn't make up
  ep_char(parser, '`');
  ep_char(parser, '(');
  result = ep_capture_until_char(parser, ')');
  ep_char(parser, ')');
 }
 return result;
}
function void
ep_inc_all_skip_comments(Ed_Parser *parser)
{
 while(true)
 {
  Token *token0 = ep_get_token(parser);
  if(token0->kind == TokenBaseKind_Comment){
   ep_eat_inc_all(parser);
  }else{
   break;
  }
 }
}
//-
function void
template_gen_for(arrayof<T_Table> *tables, Ed_Parser *parser,
                 Meta_Printer &printer)
{
 String for_loop_var_name = {};
 Scratch_Block for_loop_scratch;
 arrayof<Template_Node> for_loop_nodes;
 init_dynamic(for_loop_nodes, for_loop_scratch, 16);
 arrayof<String> exclude_list;
 init_dynamic(exclude_list, for_loop_scratch, 8);
 
 T_Table *loop_table;
 {
  ep_char(parser, '(');
  String list_name = ep_id(parser);
  loop_table = get_meta_list_by_name(tables, list_name);
  if(not loop_table){
   parser->fail();
  }
  
  if(ep_maybe_id(parser, strlit("except"))){
   //NOTE excluding elements (made-up syntax)
   ep_char(parser, '(');
   while(parser->ok_ and
         not ep_maybe_char(parser, ')'))
   {
    String item = ep_id(parser);
    exclude_list.push_value(item);
    if(not ep_maybe_char(parser, ',')){
     ep_char(parser, ')');
     break;
    }
   }
  }
  
  ep_char(parser, ')');
 }
 
 ep_char_inc_all(parser, '{');
 b32 parsing = parser->ok_;
 i32 nest_level = 0;
 while(parsing)
 {
  ep_inc_all_skip_comments(parser);
  Token *token0 = ep_get_token(parser);
  String token0_string = ep_print_token(parser);
  
  if(string_match(token0_string, '}') and
     nest_level == 0)
  {//-End the loop
   ep_eat_inc_all(parser);
   {//NOTE Print stuff out
    for_i32(iteration, 0, loop_table->items.count){
     String *item = loop_table->items[iteration];
     
     b32 excluded = false;
     for_i32(exclude_index, 0, exclude_list.count){
      //NOTE(kv) The convention is "first field is the identifier".
      kv_assert(get_field_count(loop_table) > 0);
      String test_field = item[0];
      if(test_field == exclude_list[exclude_index]){
       excluded = true;
       break;
      }
     }
     
     if(not excluded){
      for_i32(node_index, 0, for_loop_nodes.count){
       Template_Node *node = for_loop_nodes.items + node_index;
       if(node->text.count){
        //-Text
        String text = node->text;
        if(node_index == 0 and
           text.str[0] == '\n')
        {
         text.str++;
         text.count--;
        }
        printer < text;
       }else{
        //-Variable
        if(node->quoted){ printer < '"'; }
        printer < item[node->field_index];
        if(node->quoted){ printer < '"'; }
       }
      }
     }
    }
   }
   parsing = false;
  }else if(string_match(token0_string, '`')){
   //-loop variable
   ep_eat(parser);
   
   Template_Node *node = for_loop_nodes.push_zero();
   String field_name;
   if(ep_maybe_char(parser, '(')){
    field_name = ep_id(parser);
    ep_char_inc_all(parser, ')');
   }else if(ep_maybe_id(parser, strlit("quotes"))){
    node->quoted = true;
    ep_char(parser, '(');
    field_name = ep_id(parser);
    ep_char_inc_all(parser, ')');
   }else{
    field_name = ep_id_inc_all(parser);
   }
   
   b32 found_field = false;
   for_i32(test_field_index,0,get_field_count(loop_table)){
    if(loop_table->field_names[test_field_index] == field_name){
     node->field_index = test_field_index;
     found_field = true;
     break;
    }
   }
   if(not found_field){
    parser->fail();
   }
  }else{
   //-Other token
   ep_eat_inc_all(parser);
   
   if(string_match(token0_string, '{')){
    nest_level++;
   }else if(string_match(token0_string, '}')){
    nest_level--;
   }
   
   Template_Node *last = 0;
   if(for_loop_nodes.count){
    last = &for_loop_nodes.last();
   }
   if(last and last->text.count){
    //-merge the text
    last->text.size += token0->size;
   }else{
    //-make a new text node
    Template_Node *new_node = for_loop_nodes.push_zero();
    new_node->text = token0_string;
   }
  }
  parsing = parsing and parser->ok_;
 }
}
function void
template_codegen_mode(arrayof<T_Table> *tables, Ed_Parser *parser, Meta_Printer &printer)
{
 ep_char_inc_all(parser, '{');
 
 i32 nest_level = 0;
 b32 parsing = parser->ok_;
 while(parsing)
 {//-Top level
  ep_inc_all_skip_comments(parser);
  Token *token0 = ep_get_token(parser);
  String token0_string = ep_print_token(parser);
  
  if(string_match(token0_string, '}') and
     nest_level == 0)
  {//-End file gen
   ep_eat_inc_all(parser);
   parsing = false;
  }else if(ep_maybe_id(parser, strlit("gen_for"))){
   template_gen_for(tables, parser, printer);
  }else{
   //-Echo the token back out
   ep_eat_inc_all(parser);
   printer < token0_string;
   
   if(string_match(token0_string, '{')){
    nest_level++;
   }else if(string_match(token0_string, '}')){
    nest_level--;
   }
  }
  
  parsing = parsing and parser->ok_;
 }
}
function b32
template_main(Meta_Parsed_File source)
{
 Scratch_Block file_arena;
 b32 ok = true;
 //Meta_Printer printer;
 Stringz out_dir = pjoin(file_arena, path_dir(source.name), "generated");
 
 Ed_Parser parser_value = m_parser_from_token_list(source.data, source.token_list);
 Ed_Parser *parser = &parser_value;
 arrayof<T_Table> tables;
 init_dynamic(tables, file_arena, 128);
 
 b32 parsing = true;
 while(parsing)
 {//-Top level
  Token *token0 = ep_get_token(parser);
  while(token0->kind == TokenBaseKind_Whitespace or
        token0->kind == TokenBaseKind_Comment)
  {//-skip!
   ep_eat(parser);
   token0 = ep_get_token(parser);
  }
  String token0_string = ep_print_token(parser);
  
  if(token0->kind == TokenBaseKind_EOF){
   parsing = false;
  }else if(ep_maybe_id(parser, strlit("meta_table"))){
   //-Meta table
   T_Table *table = tables.push_zero();
   {//-Fields
    ep_char(parser, '(');
    while(parser->ok_ and not ep_maybe_char(parser, ')')){
     String field_name = ep_id(parser);
     table->field_names.push_value(field_name);
     if(not ep_maybe_char(parser, ',')){
      ep_char(parser, ')');
      break;
     }
    }
   }
   i32 field_count = get_field_count(table);
   
   table->name = ep_id(parser);
   
   init_dynamic(table->items, file_arena, 16);
   ep_char(parser, '{');
   while(parser->ok_ and
         not ep_maybe_char(parser, '}'))
   {//-Table items
    String *item = push_array(file_arena, String, field_count);
    for_i32(field_index, 0, field_count){
     item[field_index] = template_parse_string(parser);
    }
    table->items.push_value(item);
    if(not ep_maybe_char(parser, ',')){
     ep_char(parser, '}');
    }
   }
  }else if(ep_maybe_id(parser, strlit("gen_file"))){
   Token *filename_token = ep_get_token(parser);
   parser->set_ok(filename_token->kind == TokenBaseKind_LiteralString);
   ep_eat(parser);
   
   String filename = ep_print_given_token(parser, filename_token);
   kv_assert(filename.len >= 2);
   filename.str++;
   filename.len -= 2;
   
   Stringz out_path;
   {
    Printer pr = make_printer_buffer(file_arena, 256);
    pr<out_dir<OS_SLASH<filename;
    out_path = printer_get_string(pr);
   }
   Meta_Printer printer = m_open_file_to_write(out_path);
   printer < "//NOTE Source template: " < source.name < "\n";
   template_codegen_mode(&tables, parser, printer);
   if(printer.has_error){
    ok = false;
   }
   close_file(printer);
  }else{
   parser->fail();
  }
  
  parsing = parsing and parser->ok_ and ok;
 }
 
 if(not parser->ok_){
  Line_Column fail_location = ep_get_fail_location(parser);
  printf("%.*s:%d:%d: parse error\n",
         string_expand(source.name),
         fail_location.line,
         fail_location.column);
 }
 
 ok = ok and parser->ok_;
 return ok;
}
//-