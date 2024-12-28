//-Parsing utilities
global String header_keywords[] = {
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
 if(e->kind == Expression_Kind_Unary){
  result = guess_expression_type(e->unary.argument);
 }else if(e->kind == Expression_Kind_Binary){
  result = guess_expression_type(e->binary.lhs);
 }else if(e->kind == Expression_Kind_Int){
  result = strlit("i1");
 }else if(e->kind == Expression_Kind_Float){
  result = strlit("v1");
 }else if(e->kind == Expression_Kind_Call){
  //NOTE(kv) Using functions to describe data might not be a great idea,
  //  but C++ is kinda garbage when it comes to putting braces in macro, so idk dude...
  //  we can always change it later.
  Expression_Call *call = &e->call;
  String keys[] = {
   strlit("V2"), strlit("V3"), strlit("V4"),
   strlit("I2"), strlit("I3"), strlit("I4"),
  };
  String vals[] = {
   strlit("v2"), strlit("v3"), strlit("v4"),
   strlit("i2"), strlit("i3"), strlit("i4"),
  };
  static_assert(alen(keys) == alen(vals));
  
  for_u32(i, 0, alen(keys)){
   if(call->func->kind == Expression_Kind_Identifier){
    if(call->func->as_string == keys[i]){
     result = vals[i];
     break;
    }
   }
  }
 }
 return result;
}
function b32
modify_ast2(Arena *arena, darray(K_Slider) *sliders,
            Token *token0, Token *last_token,
            Meta_Expression *result)
{
 b32 ok = true;
 if(result->kind == Expression_Kind_Call){
  Expression_Call *call = &result->call;
  String func_name = get_function_name(call);
  b32 is_fval  = func_name == strlit("fval");
  b32 is_fbool = func_name == strlit("fbool");
  b32 is_slider = (is_fval or is_fbool);
  if(is_slider){
   //-slider special handling
   u32 arg_count = call->arguments.count;
   if(arg_count != 1 and arg_count != 2){
    ok = 0;
   }
   
   if(ok){
    u32 slider_index = sliders->count;
    K_Slider *slider = sliders->push_zero();
    {//NOTE Store slider so it can be printed out later
     Meta_Expression *value_expr = &call->arguments[0];
     slider->type = guess_expression_type(value_expr);
     if(slider->type.count == 0){
      ok = 0;
     }
     slider->value = print_expression(arena, value_expr);
     slider->pos   = (u32)token0->pos;
     slider->size  = last_token->pos + last_token->size - slider->pos;
     
     if(is_fval and arg_count == 2){
      Meta_Expression *option_expr = &call->arguments[1];
      slider->options = print_expression(arena, option_expr);
     }
     
     if(is_fbool){
      slider->options = strlit("{.flags=Slider_Clamp_01}");
     }
    }
    
    *result = {};
    result->kind   = Expression_Kind_Unknown;
    result->as_string = push_stringf(arena, "ReadSlider(%.*s, %u)",
                                     strexpand(slider->type), slider_index);
   }
  }
 }
 return ok;
}
function void
modify_ast(Klang_Parser *p, Token *token0, Meta_Expression *result)
{
 Token *last_token = ep_get_token_delta(p, -1);
 b32 ok = modify_ast2(p->arena, p->sliders, token0, last_token, result);
 if(not ok){
  p->fail();
  p->recoverable = false;  //TODO(kv) this is no good!
 }
}
function Unary_Operator
prefix_unary_operator_from_token(Token *token, String token_string)
{
 Unary_Operator op = {};
 
 if(token->kind == TokenBaseKind_Operator or
    token->sub_kind == TokenCppKind_NotAlt)
 {
  //NOTE(kv) This is prefix, so can be represented with string characters.
  kv_assert(token_string.count < 4);
  char *op_cstring = op_to_cstring(&op);
  for_u32(i, 0, token_string.count){
   op_cstring[i] = token_string[i];
  }
  
  //NOTE Final precedence check
  Precedence precedence = precedence_of(op);
  if(precedence == 0){
   op = {};
  }
 }
 
 return op;
}
function void
k_parse_expression2(Klang_Parser *p, Precedence max_precedence, String terminators,
                    Meta_Expression *result);

function void
k_parse_compound_literal(Klang_Parser *p, Meta_Expression *result)
{
 result->kind = Expression_Kind_Compound;
 init_dynamic(result->compound_items, p->arena);
 
 while(p->ok_){
  if(ep_maybe_char(p, '}')){
   break;
  }
  Compound_Item *item = result->compound_items.push_zero();
  k_parse_expression2(p, Precedence_Max, strlit(",}"), &item->value);
  
  if(not ep_maybe_char(p, ',')){
   ep_char(p, '}');
   break;
  }
 }
}

function void
k_parse_expression2(Klang_Parser *p, Precedence max_precedence, String terminators,
                    Meta_Expression *result)
{
 //TODO(kv) Cleanup terminators when we confirm that we don't need them.
 Arena *arena = p->arena;
 Scratch_Block scratch;
 *result = {};
 
 Token *token0 = ep_get_token(p);
 String token0_string = ep_print_token(p);
 {
  //ep_recovery_block(p);
  Unary_Operator unary_op = prefix_unary_operator_from_token(token0, token0_string);
  
  if(ep_maybe_kind(p, TokenBaseKind_Identifier)){
   //
   if(ep_maybe_char(p, '{')){
    //NOTE Brace initializer
    k_parse_compound_literal(p, result);
   }else{
    result->kind = Expression_Kind_Identifier;
    result->as_string = token0_string;
   }
   
  }else if(unary_op != 0){
   //-Unary prefix
   ep_eat(p);
   if(max_precedence > Precedence_Unary){
    result->kind = Expression_Kind_Unary;
    Expression_Unary *unary = &result->unary;
    unary->op       = unary_op;
    unary->argument = push_struct(arena, Meta_Expression);
    k_parse_expression2(p, Precedence_Unary, terminators, unary->argument);
   }else{
    //NOTE(kv) Binary operators (except "." and "->") never binds
    //  stronger than unary operators.
    //  So if you hit this path, the user must have written something silly,
    //  such as "foo->-5"
    p->fail();
   }
  }else if(token0->kind == TokenBaseKind_LiteralString){
   ep_eat(p);
   result->kind      = Expression_Kind_String;
   result->as_string = token0_string;
  }else if(token0->kind == TokenBaseKind_LiteralInteger){
   //-Int
   ep_eat(p);
   result->kind      = Expression_Kind_Int;
   result->as_string = token0_string;
  }else if(token0->kind == TokenBaseKind_LiteralFloat){
   //-Float
   ep_eat(p);
   result->kind   = Expression_Kind_Float;
   result->as_string = token0_string;
   
  }else if(ep_maybe_char(p, '{')){
   //-Compound literal: array, struct
   k_parse_compound_literal(p, result);
  }
  
  {//-Binary operator parse more
   while(p->ok_)
   {
    modify_ast(p, token0, result);
    
    Token *op_token = ep_get_token(p);
    String op_str = ep_print_token(p, op_token);
    
    if(op_str == '('){
     //-Function call?
     ep_eat(p);
     
     //NOTE The result so far is the function name (or pointer).
     Meta_Expression *func = push_value(arena, *result);
     
     result->kind = Expression_Kind_Call;
     Expression_Call *call = &result->call;
     call->func = func;
     
     init_dynamic(call->arguments, arena);
     while(p->ok_){
      if(ep_maybe_char(p, ')')) break; 
      
      Meta_Expression *argument = call->arguments.push();
      k_parse_expression2(p, Precedence_Max, strlit(",)"), argument);
      
      if(ep_maybe_char(p, ')')) break; 
      else ep_char(p, ','); 
     }
    }else if(op_str == '['){
     //-Array subscript
     ep_eat(p);
     
     //NOTE The result so far is the array.
     Meta_Expression *array = push_value(arena, *result);
     
     result->kind = Expression_Kind_Array_Subscript;
     Expression_Array_Subscript *subscript = &result->array_subscript;
     subscript->array = array;
     
     subscript->index = push_struct(arena, Meta_Expression);
     k_parse_expression2(p, Precedence_Max, strlit("]"), subscript->index);
     
     ep_char(p, ']');
    }else{
     //-Binary operator
     Binary_Operator op = 0;
     Precedence op_precedence = {};
     
     b32 is_alternative_operator = false;
     if(op_token->kind == TokenBaseKind_Operator or
        op_token->sub_kind == TokenCppKind_AndAlt or
        op_token->sub_kind == TokenCppKind_OrAlt)
     {
      //NOTE Copy operator string to op
      kv_assert(op_str.size < 4);
      char *op_chars = op_to_cstring(&op);
      for_u32(i, 0, op_str.size){
       op_chars[i] = op_str.str[i];
      }
      
      op_precedence = precedence_of(op);
      if(op_precedence >= max_precedence or
         op_precedence == 0)
      {
       op = 0;
      }
     }
     
     if(op){
      ep_eat(p);
      
      //NOTE Put the result so far in the lhs
      Meta_Expression *lhs = push_value(arena, *result);
      
      *result = {.kind = Expression_Kind_Binary};
      Expression_Binary *binary = &result->binary;
      binary->op  = op;
      binary->lhs = lhs;
      binary->rhs = push_struct(arena, Meta_Expression);
      k_parse_expression2(p, op_precedence, terminators, binary->rhs);
     }else{
      break;
     }
    }
   }
  }
  
  /*  if(not k_test_char(p, terminators)){
     //-If we're not at the terminator, then parsing has failed!
     p->fail();
    }*/
  if(not p->ok_){
   result->kind = {};
  }
 }
 
 if(p->ok_){
 }
 
/* if(result->kind == 0){
  //NOTE Cheese: unknown expressions
  result->kind   = Expression_Kind_Unknown;
  result->as_string = ep_capture_until_char(p, terminators);
 }*/
}
myinline void
k_parse_expression(Klang_Parser *p, String terminators, Meta_Expression *result)
{
 k_parse_expression2(p, Precedence_Max, terminators, result);
}
function void
parse_type_and_name(Klang_Parser *p, Parsed_Type *otype, String *oname)
{//NOTE(kv) We're cheesing the type HARD!
 String type_name = ep_id(p);
 *otype = {.name = type_name};
 while(ep_maybe_char(p, '*')){
  otype->pointer_count++;
 }
 *oname = ep_id(p);
 if(otype->pointer_count == 0){
  if(ep_maybe_char(p, '[')){
   otype->kind = Parsed_Type_Array;
   
   //TODO(kv) Bad news! Count could be an expression.
   //  We know that it is a constant but gotta do some funky crap,
   //  because our parser is just not there yet!
   //  Also, we don't have arena to print anything,
   //  but idk sometimes we invent things new expressions... It's annoying!
   Meta_Expression *count = push_struct(p->arena, Meta_Expression);
   k_parse_expression(p, strlit("]"), count);
   otype->array_count_str = print_expression(p->arena, count);
   otype->array_count     = (u32)string_to_u64(otype->array_count_str, 10);
   
   ep_char(p, ']');
  }
 }
}
function void
parse_struct_member(Klang_Parser *p, M_Struct_Member *omember)
{
 if(ep_maybe_id(p, "tagged_by")){
  mpa_parens{ omember->discriminator = ep_id(p); }
 }
 parse_type_and_name(p, &omember->type, &omember->name);
 ep_skip_semicolons(p);
}
myinline M_Struct_Member
parse_struct_member(Klang_Parser *p){
 M_Struct_Member member = {};
 parse_struct_member(p, &member);
 return member;
}
myinline M_Struct_Member
struct_member_from_string(String string){
 Scratch_Block scratch;
 Klang_Parser parser = {};
 (Ed_Parser &)parser = ed_parser_from_string(scratch, string);
 return parse_struct_member(&parser);
}
myinline M_Struct_Member
struct_member_from_string(char *cstring){
 return struct_member_from_string(SCu8(cstring));
}
function M_Struct_Members
parse_struct_body(Arena *arena, Klang_Parser *p){
 M_Struct_Members result;
 init_dynamic(result, arena);
 m_brace_open(p);
 while(p->ok_ && !m_maybe_brace_close(p)){
  M_Struct_Member *member = result.push_zero();
  parse_struct_member(p, member);
 }
 return result;
}
function M_Struct_Members
parse_struct_body(Arena *arena, char *string){
 Scratch_Block scratch;
 Klang_Parser parser = k_parser_from_string(scratch, SCu8(string));
 M_Struct_Members result = parse_struct_body(arena, &parser);
 return result;
}

function void
k_parse_statement_to_pointer(Arena *arena, Klang_Parser *p,
                             /*out*/Statement_Union *ostatement)
{
 Token *token0 = ep_get_token(p);
 ostatement->head.pos = token0->pos;
 ostatement->head.mom = p->current_statement;
 set_in_block(p->current_statement, &ostatement->head);
 String token0_string = ep_print_token(p, token0);
 if(token0_string == strlit("no_parse")){
  ostatement->head.kind  = Statement_Kind_Unknown;
  
  ep_eat(p);
  ep_char(p, '{');
  ostatement->unknown.unknown = ep_capture_until_char(p, '}');
  ep_char(p, '}');
 }else if(token0->kind == TokenBaseKind_Preprocessor){
  //-Preprocessor
  ostatement->head.kind    = Statement_Kind_Unknown;
  cast_to_var(Statement_Unknown *, unknown, ostatement);
  unknown->unknown = k_parse_preprocessor(p);
 }else if(token0->kind == TokenBaseKind_Identifier ||
          token0->kind == TokenBaseKind_Keyword)
 {
  if(is_header_keyword(token0_string)){
   //-Header and body
   ep_eat(p);
   ostatement->head.kind = Statement_Kind_Header_And_Body;
   cast_to_var(Statement_Header_And_Body *, header_body, ostatement);
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
   ostatement->head.kind = Statement_Kind_If;
   cast_to_var(Statement_If *, if0, ostatement);
   {//-condition
    ep_char(p, '(');
    k_parse_expression(p, strlit(")"), &if0->condition);
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
   ostatement->head.kind = Statement_Kind_Switch;
   cast_to_var(Statement_Switch *, switch0, ostatement);
   {//-expression
    ep_char(p, '(');
    k_parse_expression(p, strlit(")"), &switch0->expression);
    ep_char(p, ')');
   }
   {//-cases
    ep_char(p, '{');
    while(p->ok_ && not ep_maybe_char(p, '}')){
     Switch_Case *case0 = switch0->cases.push_zero();
     ep_id(p, strlit("case"));
     k_parse_expression(p, strlit(":"), &case0->expression);
     ep_char(p, ':');
     k_parse_statement_to_pointer(arena, p, &case0->body);
     case0->break_after = ep_maybe_id(p, strlit("break"));
     ep_skip_semicolons(p);
    }
   }
  }else if(ep_maybe_id(p, strlit("return"))){
   //-Return
   ostatement->head.kind = Statement_Kind_Return;
   cast_to_var(Statement_Return *, return0, ostatement);
   k_parse_expression(p, strlit(";"), &return0->return0);
   ep_char(p,';');
  }else if(ep_maybe_id(p, strlit("cache"))){
   //-cache
   ostatement->head.kind = Statement_Kind_Cache;
   cast_to_var(Statement_Cache *, cache0, ostatement);
   cache0->id = i32(token0->pos);
   init_dynamic(cache0->cache_items, arena);
   ep_char(p, '(');
   while(p->ok_ && not ep_maybe_char(p, ')')){
    //-Cached items
    Cache_Item *cache_item = cache0->cache_items.push_zero();
    parse_type_and_name(p, &cache_item->type, &cache_item->name);
    ep_char(p, '=');
    k_parse_expression(p, strlit(";"), &cache_item->rhs);
    ep_char(p, ';');
   }
   {//-Cached computation
    cache0->body = k_parse_statement_to_arena(arena, p);
   }
   //-Remember this statement so we can print out the metadata later
   //p->function_cache_list.push_value(cache0);
  }else{
   //-Declaration?
   ep_recovery_block(p);
   
   ostatement->head.kind = Statement_Kind_Declaration;
   cast_to_var(Statement_Declaration *, decl, ostatement);
   parse_type_and_name(p, &decl->type, &decl->name);
   if(ep_maybe_char(p,'=')){
    //-Declaration and assignment
    k_parse_expression(p, strlit(";"), &decl->rhs);
   }
   ep_char(p,';');
   
   if(not p->ok_){
    ostatement->head.kind = Statement_Kind_None;
   }
  }
 }else if(token0->kind == TokenBaseKind_ScopeOpen){
  //-Block
  ostatement->head.kind  = Statement_Kind_Block;
  cast_to_var(Statement_Block*, block, ostatement);
  block->block = k_parse_statement_block(arena, p);
 }else if(token0->kind == TokenBaseKind_StatementClose){
  ostatement->head.kind = Statement_Kind_Empty;
 }
 if(not ostatement->head.kind){
  //-Defaults to expressions
  //NOTE(kv) Warning: sometimes we use macro, forget a semicolon,
  //  and it parses until the end of the file.
  ep_scope_block(p, token0_string, token0);
  ostatement->head.kind = Statement_Kind_Expression;
  cast_to_var(Statement_Expression*, expr, ostatement);
  k_parse_expression(p, strlit(";"), &expr->expression);
  ep_char(p, ';');
 }
}
function darray(Statement_Union)
k_parse_statement_block(Arena *arena, Klang_Parser *p)
{
 ep_char(p,'{');
 darray(Statement_Union) statements;
 init_dynamic(statements, arena);
 ep_skip_semicolons(p);
 while(p->ok_ and (not ep_maybe_char(p,'}'))){
  //-Statement
  k_parse_statement_to_pointer(arena, p, statements.push_zero());
  ep_skip_semicolons(p);
 }
 return statements;
}
function sarray(Statement_Union)
k_process_top_level(Arena *arena, Klang_Parser *p, Meta_Printer &printer_gen,
                    darray(String) *type_info_list)
{
 darray(Statement_Union) top_levels;
 init_dynamic(top_levels, arena, 64);
 
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
  String type_name = {};
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
  }else if(ep_maybe_id(p, "no_parse")){
   ep_char(p, '{');
   String code = ep_capture_until_char(p, '}');
   print(printer_gen, code);
   ep_char(p, '}');
  }else if(ep_maybe_id(p, "struct")){
   //-parse struct
   darray(M_Struct_Member) members = {};
   type_name = ep_id(p);
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
   darray(String) enum_names = {};
   darray(i1) enum_vals      = {};
   type_name = ep_maybe_id(p);
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
   type_name  = ep_id(p);
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
           token0_string == strlit("myinline") or
           token0_string == strlit("dll_export") or
           false)
  {//-Function
   //init_dynamic(p->function_cache_list, scratch_top);  //@tune
   ep_eat(p);
   
   String return_type = ep_id(p);  //TODO(kv) cheese!
   String function_name = ep_id(p);
   ep_scope_block(p, function_name, token0);
   String parameters;
   mpa_parens{
    parameters = ep_capture_until_char(p,')');
   }
   Statement_Union *func0 = top_levels.push_zero();
   cast_to_var(Statement_Function *, func, func0);
   if(ep_maybe_char(p, ';')){
    //-Forward declaration
    func->kind = Statement_Kind_Function;
   }else{
    //-Body
    func->kind = Statement_Kind_Function;
    func->body = k_parse_statement_block(arena, p);
    func->has_body = true;
   }
   
   if(p->ok_)
   {//-Print
/*    {//-Caches
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
    }*/
    {//-Print prototype
     add_to_source_map(printer_gen.source_map, printer_gen, token0->pos);
     printer_gen<token0_string<" "<return_type;
     mline(printer_gen);
     printer_gen<function_name;
     m_parens2(printer_gen){
      printer_gen<parameters;
     }
    }
    if(func->has_body){//-Print body
     m_braces2(printer_gen){
      mline(printer_gen);
      for_u32(statement_index,0,func->body.count){
       print(printer_gen, func->body[statement_index].head);
       mline(printer_gen);
      }
     }
     mline(printer_gen);
    }else{//-declaration
     print(printer_gen, ";\n\n");
    }
   }
  }else if(ep_maybe_id(p, "u32_wrapper")){
   //-u32_wrapper
   mpa_parens{
    type_name = ep_id(p);
    print_u32_wrapper(printer_gen, type_name);
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
  
  if(type_name.count and do_info){
   push(type_info_list, type_name);
  }
  
  kv_assert(not p->recoverable);
 }
 return top_levels;
}
function b32
k_process_file(Arena *arena, Lexed_File source,
               darray(K_Slider) *sliders, 
               darray(String) *type_info_list,
               Statement_Root *out_root)
{
 Scratch_Block file_arena;
 Statement_Root root = {};
 root.kind = Statement_Kind_Root;
 
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
 b32 ok = not printer_gen.error;
 Klang_Parser klang_parser = {};
 Klang_Parser *parser = &klang_parser;
 {
  Ed_Parser *ed_parser = parser;
  *ed_parser = ed_parser_from_token_list(source.data, source.token_list);
  parser->current_statement = &root;
  parser->sliders           = sliders;  //NOTE(kv) Sliders is in the parser, because it could be everywhere.
  parser->arena             = arena;
 }
 {
  root.top_levels = k_process_top_level(arena, parser, printer_gen, type_info_list);
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
 close(printer_gen);
 
 *out_root           = root;
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
klang_main(Arena *arena, Lexed_File source_file,
           darray(K_Slider) *sliders, darray(String) *type_info_list)
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
 b32 ok = k_process_file(arena, source_file, sliders, type_info_list, &root);
 if(ok){
  meta_process_ast(root, source_file.name);
 }
 return ok;
}
//-