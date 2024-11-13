//-
struct T_Table{
 String name;
 arrayof<String>   field_names;
 arrayof<String *> items;
};
struct Template_Node{
 String text;
 i32 field_index;
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
  ep_eat_token_inc_all(parser);
 }
 return result;
}
function void
ep_char_inc_all(Ed_Parser *parser, char c)
{
 if(not ep_test_char(parser, c)){
  parser->fail();
 }
 ep_eat_token_inc_all(parser);
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
function b32
template_main(Meta_Parsed_File source)
{
 Scratch_Block file_arena;
 b32 ok = true;
 Meta_Printer printer;
 {
  Stringz out_dir = pjoin(file_arena, path_dir(source.name), "generated");
  String source_filename = path_filename(source.name);
  String stem = path_stem(source_filename);
  Stringz out_path;
  {
   Printer pr = make_printer_buffer(file_arena, 256);
   //NOTE(kv) Default to outputting an ".h" file for now.
   pr<out_dir<OS_SLASH<stem<".gen.h";
   out_path = printer_get_string(pr);
  }
  printer = m_open_file_to_write(out_path);
  if(not printer.FILE){
   ok = false;
  }
 }
 
 printer < "//NOTE Source template: " < source.name < "\n";
 
 Ed_Parser parser_value = m_parser_from_token_list(source.data, source.token_list);
 Ed_Parser *parser = &parser_value;
 i32 nest_level = 0;
 arrayof<T_Table> meta_tables;
 init_dynamic(meta_tables, file_arena, 128);
 
 T_Table *loop_table = 0;
 String for_loop_var_name = {};
 Scratch_Block for_loop_scratch;
 arrayof<Template_Node> for_loop_nodes;
 
 b32 parsing_ended = false;
 while(ok and not parsing_ended)
 {//-Top level
  Token *token0 = ep_get_token(parser);
  String token0_string = ep_print_token(parser);
  if(ep_maybe_char_inc_all(parser, '}')){
   //-Closing brace
   if(loop_table and nest_level == 0){
    //-End the for loop
    {//-Print stuff out
     for_i32(iteration, 0, loop_table->items.count){
      String *item = loop_table->items[iteration];
      for_i32(node_index, 0, for_loop_nodes.count){
       Template_Node *node = for_loop_nodes.items + node_index;
       if(node->text.count){
        //-Text
        String text = node->text;
        printer < text;
       }else{
        //-Variable
        printer < item[node->field_index];
       }
      }
     }
    }
    {
     loop_table = 0;
     arena_clear(for_loop_scratch);
    }
   }else{
    //-Just normal nesting
    if(nest_level > 0){ nest_level--; }
    printer < '}';
   }
  }else if(token0->kind == TokenBaseKind_EOF){
   parsing_ended = true;
  }else if(ep_maybe_id(parser, strlit("meta_table"))){
   //-Meta table
   T_Table *table = meta_tables.push_zero();
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
         not ep_maybe_char_inc_all(parser, '}'))
   {//-Table items
    String *item = push_array(file_arena, String, field_count);
    for_i32(field_index, 0, field_count){
     item[field_index] = template_parse_string(parser);
    }
    table->items.push_value(item);
    if(not ep_maybe_char(parser, ',')){
     ep_char_inc_all(parser, '}'); //note(kv) It's the end so we gotta do "inc_all"
    }
   }
  }else if(ep_maybe_id(parser, strlit("gen_for"))){
   kv_assert(not loop_table);  //NOTE(kv) Probably don't even need nested loop!
   //-gen_for
   {
    ep_char(parser, '(');
    String list_name = ep_id(parser);
    loop_table = get_meta_list_by_name(&meta_tables, list_name);
    if(not loop_table){
     parser->fail();
    }
    ep_char(parser, ')');
   }
   
   init_dynamic(for_loop_nodes, for_loop_scratch, 16);
   ep_char_inc_all(parser, '{');
   ep_maybe_char_inc_all(parser, '\n');
   //-continue parsing the body afterwards
  }else if(ep_maybe_char(parser, '`')){
   //-Escape hatch for expressions
   if(loop_table)
   {//-loop variable
    ep_char(parser, '(');
    String field_name = ep_id(parser);
    ep_char_inc_all(parser, ')');
    
    Template_Node *new_node = for_loop_nodes.push_zero();
    i32 field_index = -1;
    for_i32(test_field_index,0,get_field_count(loop_table)){
     if(loop_table->field_names[test_field_index] == field_name){
      field_index = test_field_index;
      break;
     }
    }
    if(field_index == -1){ parser->fail(); }
    new_node->field_index = field_index;
   }else{
    parser->fail();
   }
  }else if(token0->kind == TokenBaseKind_Comment){
   ep_eat_token_inc_all(parser);  // NOTE(kv) Probably don't need to generate comment!
  }else if(loop_table){
   //-Save this token so we can print out later
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
   ep_eat_token_inc_all(parser);
  }else{
   //-Echo the token back out
   printer < token0_string;
   ep_eat_token_inc_all(parser);
  }
  
  ok = ok and parser->ok_;
 }
 
 if(not parser->ok_){
  Line_Column fail_location = ep_get_fail_location(parser);
  printf("%.*s:%d:%d: parse error\n",
         string_expand(source.name),
         fail_location.line,
         fail_location.column);
 }
 
 ok = ok and not printer.has_error;
 close_file(printer);
 return ok;
}
//-