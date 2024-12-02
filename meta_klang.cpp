//-Parsing utilities
global const String header_keywords[] = {
 strlit("while"),
 strlit("for"),
 strlit("for_i32"),
 strlit("for_u32"),
 strlit("for_i64"),
 //TODO(kv) Stop cheesing keywords!
 strlit("bone_blockm"),
};
function b32
is_header_keyword(String string)
{
 for_i32(i,0,alen(header_keywords)){
  if(string == header_keywords[i]){
   return true;
  }
 }
 return false;
}
//-Parsing
function String
k_parse_preprocessor(Ed_Parser *p)
{
 String start = ep_print_token(p);
 ep_eat_kind(p, TokenBaseKind_Preprocessor);
 while(true){
  Token *token = ep_get_token(p);
  if(token->flags & TokenBaseFlag_PreprocessorBody){
   ep_eat_inc_all(p);
  }else{ break; }
 }
 ep_skip_comments_and_spaces(p);
 String end = ep_print_token(p);
 String result = {start.str, u64(end.str - start.str)};
 return result;
}
function String
guess_expression_type(Meta_Expression *e)
{
 String result = {};
 if(e->kind == Expression_Kind_Int){
  result = strlit("i1");
 }else if(e->kind == Expression_Kind_Float){
  result = strlit("v1");
 }else if(e->kind == Expression_Kind_Call){
  //NOTE(kv) Using functions to describe data might not be a great idea,
  //  but C++ is kinda garbage when it comes to putting braces in macro, so idk dude...
  //  we can always change it later.
  Expression_Function_Call *call = &e->function_call;
  String keys[] = {
   strlit("V2"), strlit("V3"), strlit("V4"),
   strlit("I2"), strlit("I3"), strlit("I4"),
  };
  String vals[] = {
   strlit("v2"), strlit("v3"), strlit("v4"),
   strlit("i2"), strlit("i3"), strlit("i4"),
  };
  kv_assert(alen(keys) == alen(vals));
   
  for_u32(i, 0, alen(keys)){
   if(call->function_name == keys[i]){
    result = vals[i];
    break;
   }
  }
 }
 return result;
}
function Meta_Expression
k_parse_expression(Arena *arena, Klang_Parser *p, String terminators)
{
 Scratch_Block scratch;
 Meta_Expression result = {};
 Token *token0 = ep_get_token(p);
 String token0_string = ep_print_token(p);
 {
  ep_recovery_block(p);
  if(ep_maybe_kind(p, TokenBaseKind_Identifier)){
   //
   if(ep_maybe_char(p, '(')){
    //-Function call
    result.kind = Expression_Kind_Call;
    
    Expression_Function_Call *call = &result.function_call;
    call->function_name = token0_string;
    init_dynamic(call->arguments, arena);
    while(p->ok_ and (not ep_maybe_char(p, ')'))){
     Meta_Expression argument = k_parse_expression(arena, p, strlit(",)"));
     call->arguments.push_value(argument);
     
     if(ep_maybe_char(p, ')')) break; 
     else ep_char(p, ','); 
    }
    
    b32 is_fval = token0_string == strlit("fval");
    if(is_fval){
     //-fval special handling
     if(call->arguments.count != 1){
      p->fail();
     }
     if(p->ok_){
      u32 slider_index = p->sliders->count;
      K_Slider *slider = p->sliders->push_zero();
      {//NOTE store slider so it can be printed out later
       Meta_Expression *value_expr = &call->arguments[0];
       slider->type  = guess_expression_type(value_expr);
       if(slider->type.count == 0){ p->fail(); }
       slider->value = print_expression(arena, value_expr);
      }
      
      result = {};
      result.kind    = Expression_Kind_Unknown;
      result.unknown = push_stringf(arena, "*(%.*s *)read_slider_at_index(%u)",
                                    strexpand(slider->type), slider_index);
     }
    }
   }else if(ep_maybe_char(p, '=')){
    //-Assignment
    result.kind = Expression_Kind_Assignment;
    Expression_Assignment &assignment = result.assignment;
    assignment.lhs = token0_string;
    assignment.rhs = push_value(arena, k_parse_expression(arena,p,terminators));
   }else if(k_test_char(p, terminators)){
    result.kind = Expression_Kind_Identifier;
    result.identifier = token0_string;
   }
  }else if(ep_maybe_kind(p, TokenBaseKind_LiteralInteger)){
   //-Int
   result.kind = Expression_Kind_Int;
   result.int_value = token0_string;
  }else if(ep_maybe_kind(p, TokenBaseKind_LiteralFloat)){
   //-Float
   result.kind = Expression_Kind_Float;
   result.float_value = token0_string;
  }
  
  if(not k_test_char(p, terminators)){
   //-If we're not at the terminator, then parsing has failed!
   p->fail();
  }
  if(not p->ok_){
   result.kind = {};
  }
 }
 if(result.kind == 0){
  //-NOTE Cheese: unknown expressions
  result.kind    = Expression_Kind_Unknown;
  result.unknown = ep_capture_until_char(p, terminators);
 }
 return result;
}
function void
k_parse_statement_to_pointer(Arena *arena, Klang_Parser *p,
                             /*out*/Statement_Union *statement0)
{
 Token *token0 = ep_get_token(p);
 statement0->head.pos = token0->pos;
 statement0->head.mom = p->current_statement;
 set_in_block(p->current_statement, &statement0->head);
 String token0_string = ep_print_given_token(p, token0);
 if(token0->kind == TokenBaseKind_Preprocessor){
  //-Preprocessor
  statement0->head.kind    = Statement_Kind_Unknown;
  cast_to_var(Statement_Unknown *, unknown, statement0);
  unknown->unknown = k_parse_preprocessor(p);
 }else if(token0->kind == TokenBaseKind_Identifier ||
          token0->kind == TokenBaseKind_Keyword)
 {
  if(is_header_keyword(token0_string)){
   //-Header and body
   ep_eat(p);
   statement0->head.kind = Statement_Kind_Header_And_Body;
   cast_to_var(Statement_Header_And_Body *, header_body, statement0);
   if(ep_maybe_char(p, '(')){
    //NOTE optional parameters
    k_eat_until_char(p, strlit(")"));
    ep_char(p, ')');
   }
   header_body->header = k_string_from_token_to_current(p, token0);
   header_body->body = k_parse_statement_to_arena(arena, p);
  }else if(token0_string == strlit("if")){
   //-If
   ep_eat(p);
   statement0->head.kind = Statement_Kind_If;
   cast_to_var(Statement_If *, if0, statement0);
   {//-condition
    ep_char(p, '(');
    if0->condition = k_parse_expression(arena, p, strlit(")"));
    ep_char(p, ')');
   }
   {//-body
    if0->body = k_parse_statement_to_arena(arena, p);
   }
   if(ep_maybe_id(p, strlit("else"))){
    //-else
    if0->else0 = k_parse_statement_to_arena(arena, p);
   }
  }else if(ep_maybe_id(p, strlit("switch"))){
   //-switch
   statement0->head.kind = Statement_Kind_Switch;
   cast_to_var(Statement_Switch *, switch0, statement0);
   {//-expression
    ep_char(p, '(');
    switch0->expression = k_parse_expression(arena, p, strlit(")"));
    ep_char(p, ')');
   }
   {//-cases
    ep_char(p, '{');
    while(p->ok_ && not ep_maybe_char(p, '}')){
     Switch_Case *case0 = switch0->cases.push_zero();
     ep_id(p, strlit("case"));
     case0->expression = k_parse_expression(arena, p, strlit(":"));
     ep_char(p, ':');
     k_parse_statement_to_pointer(arena, p, &case0->body);
     case0->break_after = ep_maybe_id(p, strlit("break"));
     ep_skip_semicolons(p);
    }
   }
  }else if(ep_maybe_id(p, strlit("return"))){
   //-Return
   statement0->head.kind = Statement_Kind_Return;
   cast_to_var(Statement_Return *, return0, statement0);
   return0->return0 = k_parse_expression(arena, p, strlit(";"));
   ep_char(p,';');
  }else if(ep_maybe_id(p, strlit("cache"))){
   //-cache
   statement0->head.kind = Statement_Kind_Cache;
   cast_to_var(Statement_Cache *, cache0, statement0);
   cache0->id = i32(token0->pos);
   init_dynamic(cache0->cache_items, arena);
   ep_char(p, '(');
   while(p->ok_ && not ep_maybe_char(p, ')')){
    //-Cached items
    Cache_Item *cache_item = cache0->cache_items.push_zero();
    parse_type_and_name(p, &cache_item->type, &cache_item->name);
    ep_char(p, '=');
    cache_item->rhs = k_parse_expression(arena, p, strlit(";"));
    ep_char(p, ';');
   }
   {//-Cached computation
    cache0->body = k_parse_statement_to_arena(arena, p);
   }
   //-Remember this statement so we can print out the metadata later
   p->function_cache_list.push_value(cache0);
  }else{
   //-Declaration?
   ep_recovery_block(p);
   {
    statement0->head.kind = Statement_Kind_Declaration;
    cast_to_var(Statement_Declaration *, decl, statement0);
    parse_type_and_name(p, &decl->type, &decl->name);
    if(ep_maybe_char(p,'=')){
     //-Declaration and assignment
     decl->rhs = k_parse_expression(arena, p, strlit(";"));
    }
    ep_char(p,';');
   }
   if(not p->ok_){
    statement0->head.kind = Statement_Kind_None;
   }
  }
 }else if(token0->kind == TokenBaseKind_ScopeOpen){
  //-Block
  statement0->head.kind  = Statement_Kind_Block;
  cast_to_var(Statement_Block*, block, statement0);
  block->block = k_parse_statement_block(arena, p);
 }else if(token0->kind == TokenBaseKind_StatementClose){
  statement0->head.kind = Statement_Kind_Empty;
 }
 if(not statement0->head.kind){
  //-Defaults to expressions
  //NOTE(kv) Warning: sometimes we use macro, forget a semicolon,
  //  and it parses until the end of the file.
  ep_scope_block(p, token0_string, token0);
  statement0->head.kind = Statement_Kind_Expression;
  cast_to_var(Statement_Expression*, expr, statement0);
  expr->expression = k_parse_expression(arena, p, strlit(";"));
  ep_char(p, ';');
 }
}
function Meta_Statements
k_parse_statement_block(Arena *arena, Klang_Parser *p)
{
 ep_char(p,'{');
 Meta_Statements statements;
 init_dynamic(statements, arena);
 ep_skip_semicolons(p);
 while(p->ok_ and (not ep_maybe_char(p,'}'))){
  //-Statement
  k_parse_statement_to_pointer(arena, p, statements.push_zero());
  ep_skip_semicolons(p);
 }
 return statements;
}
function void
k_process_top_level(Arena *arena,
                    Klang_Parser *p, Meta_Printer &printer_gen,
                    /*out*/ Statement_Root *root)
{
 Scratch_Block scratch_top;
 while(p->ok_)
 {
  Temp_Memory_Block temp_loop(scratch_top);
  {//-whitespace token
   while(true){
    Token *token = ep_get_token(p);
    if(token->kind == TokenBaseKind_Whitespace or
       token->kind == TokenBaseKind_Comment or
       token->kind == TokenBaseKind_StatementClose)
    {
     ep_print_token(printer_gen, p);
     ep_eat_inc_all(p);
    }else{
     break;
    }
   }
   ep_skip_comments_and_spaces(p);
  }
  Token *token0 = ep_get_token(p);
  String token0_string = ep_print_token(scratch_top, p);
  ep_scope_block(p, strlit("top-level"), token0);
  b32 do_info  = true;
  b32 do_embed = false;
  b32 is_packed = false;
  if(m_maybe_bracket_open(p)){
   //-Struct attributes (like the clang attributes)
   while(p->ok_ && !m_maybe_bracket_close(p)){
    String string = ep_print_token(scratch_top, p);
    if(string == strlit("noinfo")){
     do_info = false;
    }else if(string == strlit("embed")){
     do_embed = true;
    }else if(string == strlit("packed")){
     is_packed = true;
    }else{
     p->fail();
    }
    ep_eat(p);
   }
  }
  
  if(token0->kind == TokenBaseKind_EOF){
   break;
  }else if(ep_maybe_id(p, "struct")){
   //-parse struct
   arrayof<M_Struct_Member> members = {};
   String type_name = ep_id(p);
   ep_char(p, '{');
   while(p->ok_ && !m_maybe_brace_close(p)){
    // NOTE: Field
    M_Struct_Member *member = members.push_zero();
    
    if(ep_maybe_id(p, "meta_removed")){
     //-meta_removed
     ep_char(p, '(');
     {
      parse_struct_member(p, member);
     }
     if(meta_maybe_key(p, strlit("added"))){
      member->version_added = ep_id(p);
      ep_maybe_char(p, ',');
     }
     {
      meta_parse_key(p, strlit("removed"));
      member->version_removed = ep_id(p);
      ep_maybe_char(p, ',');
     }
     if ( meta_maybe_key(p, strlit("default")) ) {
      member->default_value = ep_capture_until_char(p, ')');
     } else {
      ep_char(p,')');
     }
     ep_skip_semicolons(p);
    }else{
     if(ep_maybe_id(p, "meta_added")){
      //-meta_added
      ep_char(p, '(');
      while(p->ok_ && !ep_maybe_char(p, ')')){
       ep_maybe_char(p, ',');
       if(meta_maybe_key(p, strlit("added"))){
        member->version_added = ep_id(p);
       }else if(meta_maybe_key(p, strlit("default"))){
        //TODO(kv): support arbitrary expression in parens
        member->default_value = ep_print_token(p);
        ep_eat(p);
       }else{ p->fail(); }
      }
     }else if(ep_maybe_id(p, "meta_unserialized")){
      member->unserialized = true;
     }
     ep_skip_semicolons(p);
     
     parse_struct_member(p, member);
    }
   }
   
   print_struct(printer_gen, type_name, members, is_packed);
   if(do_info){
    print_struct_meta(printer_gen, type_name, members);
   }
   if(do_embed){
    print_struct_embed(printer_gen, type_name, members);
   }
  }else if(ep_maybe_id(p, "union")){
   //-Union
   todo_incomplete;
  }else if(ep_maybe_id(p, "enum")){
   //-Enum
   arrayof<String> enum_names = {};
   arrayof<i1> enum_vals      = {};
   String type_name = ep_maybe_id(p);
   m_brace_open(p);
   while(p->ok_ && !m_maybe_brace_close(p)){
    //NOTE(kv) Enum value
    enum_names.push_value(ep_id(p));
    ep_char(p, '=');
    enum_vals.push_value(ep_i1(p));
    ep_eat_until_char_simple(p, ',');  // NOTE(kv) The ending comma is optional, but I don't care.
   }
   
   print_enum(printer_gen, type_name, enum_names, enum_vals);
   if(type_name.len!=0 && do_info){
    //NOTE(kv) Anonymous enums can't be read, since it can't be referred to.
    print_enum_meta(printer_gen, type_name, enum_names);
   }
  }else if(ep_maybe_id(p, "typedef")){
   //-typedef
   String typedef_to = ep_id(p);
   String type_name  = ep_id(p);
   {
    printer_gen<"typedef "<typedef_to<" "<type_name<";\n";
   }
   if(do_info){
    print_typedef_meta(printer_gen, type_name, typedef_to);
   }
  }else if(token0->kind == TokenBaseKind_Preprocessor){
   //-Preprocessor
   String preproc_string = k_parse_preprocessor(p);
   printer_gen < preproc_string;
  }else if(string_match(token0_string, strlit("global"))){
   while(p->ok_){
    Token *token = ep_get_token(p);
    if(token->kind == TokenBaseKind_StatementClose){
     break;
    }else{
     ep_print_token(printer_gen, p);
     ep_eat_inc_all(p);
    }
   }
  }else if(token0_string == strlit("xfunction") or
           token0_string == strlit("function") or
           token0_string == strlit("inline") or
           token0_string == strlit("kv_inline"))
  {//-Function
   init_dynamic(p->function_cache_list, scratch_top);  //@tune
   ep_eat(p);
   String return_type = ep_id(p);  //TODO(kv) cheese!
   String function_name = ep_id(p);
   ep_scope_block(p, function_name, token0);
   String parameters;
   mpa_parens{
    parameters = ep_capture_until_char(p,')');
   }
   Statement_Union *func0 = root->top_levels.push_zero();
   cast_to_var(Statement_Function *, func, func0);
   {//-Body
    func->kind = Statement_Kind_Function;
    func->body = k_parse_statement_block(arena, p);
   }
   if(p->ok_)
   {//-Print
    {//-Caches
     auto print_cache_storage = [](Meta_Printer &printer, Statement_Cache &cache0)->void{
      {//-The struct
       printer < "struct Cache_Storage_" < cache0.id;
       m_braces2(printer){
        printer < "\nb32 cache_initialized;\n";
        for_i32(item_index,0,cache0.cache_items.count){
         Statement_Declaration &item = cache0.cache_items[item_index];
         print_type_and_name(printer, item.type, item.name);
         printer < ";\n";
        }
       }
       printer < ";\n";
      }
      {//-Global var
       printer<"global Cache_Storage_"<cache0.id<" "<cache_storage_prefix<cache0.id<";";
       mline(printer);
      }
     };
     
     for_i32(cache_index, 0, p->function_cache_list.count){
      Statement_Cache &cache0 = *p->function_cache_list[cache_index];
      m_locationp(printer_gen);
      print_cache_storage(printer_gen, cache0);
     }
    }
    {//-Prototype
     add_to_source_map(printer_gen.source_map, printer_gen, token0->pos);
     printer_gen<token0_string<" "<return_type;
     mline(printer_gen);
     printer_gen<function_name;
     m_parens2(printer_gen){
      printer_gen<parameters;
     }
    }
    {//-Print body
     m_braces2(printer_gen){
      mline(printer_gen);
      for_i32(statement_index,0,func->body.count){
       mline(printer_gen);
       print(printer_gen, func->body[statement_index].head);
      }
     }
     mline(printer_gen);
    }
   }
  }else if(ep_maybe_id(p, "i1_wrapper")){
   //-i1_wrapper
   mpa_parens{
    String type_name = ep_id(p);
    print_i1_wrapper(printer_gen, type_name);
   }
  }else if(ep_maybe_id(p, "unique")){
   //-One-off/miscellaneous stuff
   if(ep_maybe_id(p, "Curve_Type")){
    generate_entity_types(printer_gen);
   }else{
    p->fail();
   }
  }else{
   p->fail();
  }
  kv_assert(not p->recoverable);
 }
}
function b32
k_process_file(Arena *arena, Meta_Parsed_File source, arrayof<K_Slider> *sliders,
               /*out*/ Statement_Root *root)
{
 Scratch_Block file_arena;
 *root = {};
 root->kind = Statement_Kind_Root;
 root->source_path = source.name;
 init_dynamic(root->top_levels, arena, 64);
 
 Meta_Printer printer_gen;
 Stringz map_file_path;
 Stringz side_file_path;
 {//-filepath business
  Stringz gen_path;
  {
   String gen_dir = pjoin(file_arena, path_dir(source.name), strlit("generated"));
   String filename = path_filename(source.name);
   String stem = path_stem(filename);
   String extension = path_extension(filename);
   b32 is_kh = extension == strlit("kh");
   if(not is_kh){
    kv_assert(extension == strlit("kc"));
   }
   {//-Generated file path
    Printer pr = make_printer_buffer(file_arena, 256);
    pr<gen_dir<OS_SLASH<stem<".gen"<(is_kh ? ".h" : ".cpp");
    gen_path = printer_get_string(pr);
   }
   {
    Printer pr = make_printer_buffer(file_arena, 256);
    pr<gen_dir<OS_SLASH<filename<".map";
    map_file_path = printer_get_string(pr);
   }
  }
  {
   printer_gen = m_open_file_to_write(gen_path);
   printer_gen<"// NOTE: source: "<source.name<"\n";
  }
 }
 init_dynamic(printer_gen.source_map, file_arena, 256);
 b32 ok = not printer_gen.has_error;
 Klang_Parser klang_parser = {};
 Klang_Parser *parser = &klang_parser;
 {
  Ed_Parser *ed_parser = parser;
  *ed_parser = m_parser_from_token_list(source.data, source.token_list);
  parser->current_statement = root;
  parser->sliders = sliders;
 }
 {
  k_process_top_level(arena, parser, printer_gen, root);
 }
 if(parser->ok_){
  {//-Print source map
   FILE *file = open_or_create_file(map_file_path, "wb");
   ok = ok and file != 0;
   if(file){
    Source_Map &map = printer_gen.source_map;
    {//note Magic
     char *magic = "kmap";
     fwrite(magic, 1, 4, file);
    }
    {//note Number of items
     fwrite(&map.count, 1, 4, file);
    }
    {//note The bulk of the data
     fwrite(map.items, 1, map.count*sizeof(*map.items), file);
    }
    close_file(file);
   }
  }
 }
 
 if(not parser->ok_){
  ok = false;
  Line_Column fail_location = ep_get_fail_location(parser);
  Line_Column scope_location = ep_get_scope_location(parser);
  printf("%.*s:%d:%d [klang: %.*s:%d:%d] parse error\n",
         string_expand(source.name),
         fail_location.line,
         fail_location.column,
         string_expand(parser->scope_.name),
         scope_location.line,
         scope_location.column);
 }
 close_file(printer_gen);
 return ok;
}
//-
inline void
k_print_struct(Printer &p, K_Struct struc){
 print_struct(p, struc.name, struc.members);
}
inline void
k_print_struct_meta(Printer &p, K_Struct struc){
 print_struct_meta(p, struc.name, struc.members);
}
function b32
klang_main(Arena *arena, Meta_Parsed_File source_file, arrayof<K_Slider> *sliders)
{
 {//-Test file
  b32 is_test_file = path_filename(source_file.name) == "test.kc";
  if(DEBUG_parse_test_file){
   if(not is_test_file){
    return true;
   }
  }else if(is_test_file){
   return true;
  }
 }
 
 Statement_Root root;
 b32 ok = k_process_file(arena, source_file, sliders, &root);
 if(ok){
  meta_process_ast(&root);
 }
 return ok;
}
struct String_Mapping{
 String key;
 String val;
};
function void
print_template(Printer &printer, String format, String *args, u32 argc)
{
 u32 current_arg = 0;
 for_u32(i, 0, format.count){
  u8 c = format.str[i];
  b32 is_percent = (c == '%');
  if(is_percent){
   u32 arg_index = current_arg++;
   kv_assert(arg_index < argc);
   String arg = args[arg_index];
   print(printer, arg);
  }else{
   print(printer, c);
  }
 }
 kv_assert(current_arg == argc);
}
function b32
klang_print_sliders(K_Slider *sliders, u32 slider_count)
{
 Scratch_Block scratch;
 b32 ok = true;
 
 char *create_slider_template_c = R"CODE(
{
 Basic_Type type = Basic_Type_%;
 usize size = sizeof(Fui_Slider) + get_basic_type_size(type);
 global_sliders[%] = cast(Fui_Slider*)arena_push(arena, size, alignof(Fui_Slider), push_zero());
 Fui_Slider *slider = global_sliders[%];
 slider->type = type;
 % *value = cast(%*)(slider+1);
 *value = %;
})CODE";
 String create_slider_template = SCu8(create_slider_template_c);
 
 Stringz slider_file_path = pjoin(scratch, meta_dirs.game_gen, strlit("sliders.gen.h"));
 Meta_Printer printer = m_open_file_to_write(slider_file_path);
 
 for_u32(slider_index, 0, slider_count){
  K_Slider *slider = sliders + slider_index;
  String type = slider->type;
  String slider_index_str = to_string(scratch, slider_index);
  String vars[] = {type, slider_index_str, slider_index_str, type, type, slider->value};
  print_template(printer, create_slider_template, ArrayAndCount(vars));
 }
 
 ok = ok and not printer.has_error;
 return ok;
}
//-