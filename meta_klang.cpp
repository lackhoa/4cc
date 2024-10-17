//-
function void
generate_bezier_types(Printer &p);
function void
generate_fill_types(Printer &p0);
//-Parsing utilities
//TODO(kv) Stop cheesing keywords!
global const String header_keywords[] = {
 strlit("while"),
 strlit("for"),
 strlit("for_i32"),
 strlit("for_u32"),
 strlit("for_i64"),
 strlit("bone_blockm"),
};
function b32
is_header_keyword(String string){
 for_i32(i,0,alen(header_keywords)){
  if(string == header_keywords[i]){
   return true;
  }
 }
 return false;
}
//-Parsing
function String
k_parse_preprocessor(Ed_Parser *p){
 String start = ep_print_token(p);
 ep_eat_kind(p, TokenBaseKind_Preprocessor);
 while(true){
  Token *token = ep_get_token(p);
  if(token->flags & TokenBaseFlag_PreprocessorBody){
   ep_eat_token_all(p);
  }else{ break; }
 }
 ep_skip_comments_and_spaces(p);
 String end = ep_print_token(p);
 String result = {start.str, u64(end.str - start.str)};
 return result;
}
function Meta_Expression
k_parse_expression(Arena *arena, Ed_Parser *p, String terminators){
 Meta_Expression result = {};
 Token *token0 = ep_get_token(p);
 String token0_string = ep_print_token(p);
 if(true){
  //-NOTE(kv) Temporary cheese!
  result.kind    = Expression_Kind_Unknown;
  result.unknown = ep_capture_until_char(p, terminators);
 }else{
  if(ep_maybe_kind(p, TokenBaseKind_Identifier)){
   if(ep_maybe_char(p, '(')){
    //-Function call
    result.kind = Expression_Kind_Function_Call;
    Expression_Function_Call &call = result.function_call;
    call.function_name = token0_string;
    init_dynamic(call.arguments, arena);
    while(p->ok_ and (not ep_maybe_char(p, ')'))){
     Meta_Expression argument = k_parse_expression(arena, p, strlit(",)"));
     call.arguments.push(argument);
     if(ep_maybe_char(p, ')')){ break; }
     else{ ep_char(p, ','); }
    }
   }else if(ep_maybe_char(p, '=')){
    //-Assignment
    result.kind = Expression_Kind_Assignment;
    Expression_Assignment &assignment = result.assignment;
    assignment.lhs = token0_string;
    assignment.rhs = push_value(arena, k_parse_expression(arena,p,terminators));
   }else if(test_char_in(p, terminators)){
    result.kind = Expression_Kind_Identifier;
    result.identifier = token0_string;
   }else{ p->fail(); }
  }else if(ep_maybe_kind(p, TokenBaseKind_LiteralInteger)){
   //-Int
   result.kind = Expression_Kind_Int;
   result.int_value = token0_string;
  }else if(ep_maybe_kind(p, TokenBaseKind_LiteralFloat)){
   //-Float
   result.kind = Expression_Kind_Float;
   result.float_value = token0_string;
  }else{
   p->fail();
  }
 }
 return result;
}
function void
k_parse_statement_to_pointer(Arena *arena, Klang_Parser *p, Meta_Statement *statement){
 Token *token0 = ep_get_token(p);
 String token0_string = ep_print_given_token(p, token0);
 if(token0->kind == TokenBaseKind_Preprocessor){
  //-Preprocessor
  statement->kind    = Statement_Kind_Unknown;
  statement->unknown = k_parse_preprocessor(p);
 }else if(token0->kind == TokenBaseKind_Identifier ||
          token0->kind == TokenBaseKind_Keyword){
  if(is_header_keyword(token0_string)){
   //-Header and body
   ep_eat_token(p);
   statement->kind = Statement_Kind_Header_And_Body;
   Statement_Header_And_Body &header_body = statement->header_and_body;
   if(ep_maybe_char(p, '(')){
    //NOTE optional parameters
    ep_eat_until_char(p, strlit(")"));
    ep_char(p, ')');
   }
   header_body.header = k_string_from_token_to_current(p, token0);
   header_body.body = k_parse_statement_to_arena(arena, p);
  }else if(token0_string == strlit("if")){
   //-If
   ep_eat_token(p);
   statement->kind = Statement_Kind_If;
   Statement_If &if0 = statement->if0;
   {//-condition
    ep_char(p, '(');
    if0.condition = k_parse_expression(arena, p, strlit(")"));
    ep_char(p, ')');
   }
   {//-body
    if0.body = k_parse_statement_to_arena(arena, p);
   }
   if(ep_maybe_id(p, strlit("else"))){
    //-else
    if0.else0 = k_parse_statement_to_arena(arena, p);
   }
  }else if(ep_maybe_id(p, strlit("switch"))){
   //-switch
   statement->kind = Statement_Kind_Switch;
   Statement_Switch &switch0 = statement->switch0;
   {//-expression
    ep_char(p, '(');
    switch0.expression = k_parse_expression(arena, p, strlit(")"));
    ep_char(p, ')');
   }
   {//-cases
    ep_char(p, '{');
    while(p->ok_ && not ep_maybe_char(p, '}')){
     Switch_Case *case0 = &switch0.cases.push_zero();
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
   statement->kind = Statement_Kind_Return;
   statement->return0 = k_parse_expression(arena, p, strlit(";"));
   ep_char(p,';');
  }else if(ep_maybe_id(p, strlit("cache"))){
   //-cache
   statement->kind = Statement_Kind_Cache;
   Statement_Cache *cache0 = &statement->cache0;
   cache0->id = i32(token0->pos);
   init_dynamic(cache0->cache_items, arena);
   ep_char(p, '(');
   while(p->ok_ && not ep_maybe_char(p, ')')){
    //-Cached items
    Cache_Item *cache_item = &cache0->cache_items.push_zero();
    parse_type_and_name(p, &cache_item->type, &cache_item->name);
    ep_char(p, '=');
    cache_item->rhs = k_parse_expression(arena, p, strlit(";"));
    ep_char(p, ';');
   }
   {//-Cached computation
    cache0->body = k_parse_statement_to_arena(arena, p);
   }
   //-Remember this statement so we can print out the metadata later
   p->cache_list.push(cache0);
  }else{
   //-Declaration?
   ep_recovery_block(p);
   {
    statement->kind = Statement_Kind_Declaration;
    Statement_Declaration &decl = statement->declaration;
    parse_type_and_name(p, &decl.type, &decl.name);
    if(ep_maybe_char(p,'=')){
     //-Declaration and assignment
     decl.rhs = k_parse_expression(arena, p, strlit(";"));
    }
    ep_char(p,';');
   }
   if(not p->ok_){
    statement->kind = Statement_Kind_None;
   }
  }
 }else if(token0->kind == TokenBaseKind_ScopeOpen){
  //-Block
  statement->kind  = Statement_Kind_Block;
  statement->block = k_parse_statement_block(arena, p);
 }else if(token0->kind == TokenBaseKind_StatementClose){
  statement->kind = Statement_Kind_Empty;
 }
 if(not statement->kind){
  //-Defaults to expressions
  //NOTE(kv) Warning: sometimes we use macro, forget a semicolon,
  //  and it parses until the end of the file.
  ep_scope_block(p, strlit("statement"), token0);
  statement->kind = Statement_Kind_Expression;
  statement->expression = k_parse_expression(arena, p, strlit(";"));
  ep_char(p, ';');
 }
}
function Meta_Statement *
k_parse_statement_to_arena(Arena *arena, Klang_Parser *p){
 Meta_Statement *statement = push_struct(arena, Meta_Statement, true);
 k_parse_statement_to_pointer(arena, p, statement);
 return statement;
}
function arrayof<Meta_Statement>
k_parse_statement_block(Arena *arena, Klang_Parser *p){
 ep_char(p,'{');
 arrayof<Meta_Statement> statements;
 init_dynamic(statements, arena);
 ep_skip_semicolons(p);
 while(p->ok_ and (not ep_maybe_char(p,'}'))){
  //-Statement
  k_parse_statement_to_pointer(arena, p, &statements.push_zero());
  ep_skip_semicolons(p);
 }
 return statements;
}
function b32
k_process_file(Meta_Parsed_File source){
 Scratch_Block scratch;
 Printer printer_gen;
 //Printer_Pair ps_meta;
 {
  Stringz gen_path, meta_base;
  String stem;
  {
   String path = source.name;
   String dir  = path_dirname(path);
   dir = pjoin(scratch, dir, "generated");
   String filename = path_filename(path);
   stem = path_stem(filename);
   if(DEBUG_parse_test_file){
    if(not (stem == "test")){ return true; }
   }
   String extension = path_extension(filename);
   b32 is_kh = extension == strlit("kh");
   if(not is_kh){
    kv_assert(extension == strlit("kc"));
   }
   {
    Printer pr = make_printer_buffer(scratch, 256);
    pr<dir<OS_SLASH<stem<".gen"<(is_kh ? ".h" : ".cpp");
    gen_path = printer_get_string(pr);
   }
   {
    Printer pr = make_printer_buffer(scratch, 256);
    pr<dir<OS_SLASH<stem<"_"<extension<"_meta.gen";
    meta_base = printer_get_string(pr);
   }
  }
  printer_gen = m_open_file_to_write(gen_path);
  {
   printer_gen<"// NOTE: source: "<source.name<"\n";
   printer_gen<"#pragma once\n";
  }
  //ps_meta = m_open_files_to_write(meta_base);
 }
 b32 ok = okp(printer_gen)/* && okp(ps_meta)*/;
 Klang_Parser klang_parser = {};
 {
  Ed_Parser *ed_parser = &klang_parser;
  *ed_parser = m_parser_from_token_list(scratch, source.data, source.token_list);
 }
 Klang_Parser *p = &klang_parser;
 while(ok && p->ok_){
  //-NOTE(kv): Parsing loop
  Scratch_Block scratch_loop;
  {//-When we read a newline, print a newline back out
   while(true){
    Token *token = ep_get_token(p);
    if(token->kind == TokenBaseKind_Whitespace ||
       token->kind == TokenBaseKind_Comment ||
       token->kind == TokenBaseKind_StatementClose){
     ep_print_token(printer_gen, p);
     ep_eat_token_all(p);
    }else{
     break;
    }
   }
   ep_skip_comments_and_spaces(p);
  }
  Token *token0 = ep_get_token(p);
  String token0_string = ep_print_token(scratch_loop, p);
  ep_scope_block(p, strlit("top-level"), token0);
  b32 do_info  = true;
  b32 do_embed = false;
  if(m_maybe_bracket_open(p)){
   //-Introspection parameters
   while(p->ok_ && !m_maybe_bracket_close(p)){
    ep_maybe_char(p, ',');
    if     (ep_maybe_id(p, "noinfo")){ do_info = false; }
    else if(ep_maybe_id(p, "embed")) { do_embed = true; }
    else{ p->fail(); }
   }
   ep_skip_semicolons(p);
  }
  if(token0->kind == TokenBaseKind_EOF){
   break;
  }else if(ep_maybe_id(p, "struct")){
   //-struct case
   arrayof<Meta_Struct_Member> members = {};
   String type_name = ep_id(p);
   ep_char(p, '{');
   while(p->ok_ && !m_maybe_brace_close(p)){
    // NOTE: Field
    Meta_Struct_Member &member = members.push2();
    member = {};
    
    if(ep_maybe_id(p, "meta_removed")){
     // NOTE(kv): meta_removed
     ep_char(p, '(');
     {
      parse_struct_member(p, &member);
     }
     if(meta_maybe_key(p, strlit("added"))){
      member.version_added = ep_id(p);
      ep_maybe_char(p, ',');
     }
     {
      meta_parse_key(p, strlit("removed"));
      member.version_removed = ep_id(p);
      ep_maybe_char(p, ',');
     }
     if ( meta_maybe_key(p, strlit("default")) ) {
      member.default_value = ep_capture_until_char(p, ')');
     } else {
      ep_char(p,')');
     }
     ep_skip_semicolons(p);
    }else{
     if(ep_maybe_id(p, "meta_added")){
      // NOTE(kv): meta_added
      ep_char(p, '(');
      while(p->ok_ && !ep_maybe_char(p, ')')){
       ep_maybe_char(p, ',');
       if(meta_maybe_key(p, strlit("added"))){
        member.version_added = ep_id(p);
       }else if(meta_maybe_key(p, strlit("default"))){
        //TODO(kv): support arbitrary expression in parens
        member.default_value = ep_print_token(p);
        ep_eat_token(p);
       }else{ p->fail(); }
      }
     }else if(ep_maybe_id(p, "meta_unserialized")){
      member.unserialized = true;
     }
     ep_skip_semicolons(p);
     
     parse_struct_member(p, &member);
    }
   }
   
   print_struct(printer_gen, type_name, members);
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
   //-enum
   arrayof<String> enum_names = {};
   arrayof<i1> enum_vals      = {};
   String type_name = ep_maybe_id(p);
   m_brace_open(p);
   while(p->ok_ && !m_maybe_brace_close(p)){
    //NOTE(kv) Enum value
    enum_names.push(ep_id(p));
    ep_char(p, '=');
    enum_vals.push(ep_i1(p));
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
    printer_gen<<"typedef "<<typedef_to<<" "<<type_name<<";\n";
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
     ep_eat_token_all(p);
    }
   }
  }else if(token0_string == strlit("xfunction") ||
           token0_string == strlit("function") ||
           token0_string == strlit("inline")){
   //-Function
   init_dynamic(p->cache_list, scratch_loop);  //TODO(kv) This seems like "action at a distance"
   ep_eat_token(p);
   String return_type = ep_id(p);  //TODO(kv) cheese!
   String function_name = ep_id(p);
   ep_scope_block(p, function_name, token0);
   String parameters;
   mpa_parens{
    parameters = ep_capture_until_char(p,')');
   }
   Meta_Statements body;
   {//-Body
    body = k_parse_statement_block(scratch_loop, p);
   }
   if(p->ok_)
   {//-Print
    {//-Caches
     for_i32(cache_index,0,p->cache_list.count){
      Statement_Cache &cache0 = *p->cache_list[cache_index];
      print_cache_meta(printer_gen, cache0);
     }
    }
    {//-Prototype
     printer_gen<token0_string<" "<return_type<"\n"<function_name;
     m_parens2(printer_gen){
      printer_gen<parameters;
     }
    }
    {//-Print body
     m_braces2(printer_gen){
      printer_gen < "\n";
      for_i32(statement_index,0,body.count){
       print(printer_gen, body[statement_index]);
       printer_gen < "\n";
      }
     }
     printer_gen < "\n";
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
    generate_bezier_types(printer_gen);
   }else if(ep_maybe_id(p, "Fill_Type")){
    generate_fill_types(printer_gen);
   }else{
    p->fail();
   }
  }else{
   p->fail();
  }
  kv_assert(not p->recoverable);
 }
 if(!p->ok_){
  ok = false;
  Line_Column fail_location = ep_get_fail_location(p);
  Line_Column scope_location = ep_get_scope_location(p);
  printf("klang: [%.*s:%d:%d] parse error at %.*s:%d:%d\n",
         string_expand(p->scope_.name),
         scope_location.line,
         scope_location.column,
         string_expand(source.name),
         fail_location.line,
         fail_location.column);
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
klang_main(arrayof<Meta_Parsed_File> source_files){
 b32 ok = true;
 for_i1(index,0,source_files.count){
  Meta_Parsed_File &source_file = source_files[index];
  String extension = path_extension(source_file.name);
  if(extension == strlit("kh") ||
     extension == strlit("kc")){
   ok = k_process_file(source_file);
   if(!ok){ break; }
  }
 }
 return ok;
}
//-