//-Parsing utilities
// TODO(kv) TokenBaseKind_StatementClose also contains comma, for some reason?
//  and for that, our code is technically broken... but it's easy to fix so whatevs!
global String header_keywords[] = {
 // TODO(kv) Stop cheesing keywords!
 strlit("while"),
 strlit("for"),
 strlit("for_i32"),
 strlit("for_u32"),
 strlit("for_i64"),
};
function b32
is_header_keyword(String string)
{
 for_i32(i,0,alen(header_keywords))
 {
  if(string == header_keywords[i])
  {
   return true;
  }
 }
 return false;
}
//-Parsing
function String
parse_preprocessor(Ed_Parser *p)
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
guess_expression_type(Meta_Expression &e)
{// NOTE(kv) Pure hacking, not even trying...
 String result = {};
 if(e.kind == Expression_Kind_Unary)
 {
  result = guess_expression_type(*e.unary.argument);
 }
 else if(e.kind == Expression_Kind_Binary)
 {
  result = guess_expression_type(*e.binary.lhs);
 }
 else if(e.kind == Expression_Kind_Int)
 {
  result = strlit("i1");
 }
 else if(e.kind == Expression_Kind_Float)
 {
  result = strlit("v1");
 }
 else if(e.kind == Expression_Kind_Call)
 {//NOTE(kv) Using functions to describe data might not be a great idea,
  //  but C++ is kinda garbage when it comes to putting braces in macro, so idk dude...
  //  we can always change it later.
  Expression_Call &call = e.call;
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
   if(call.func->kind == Expression_Kind_Identifier){
    if(call.func->as_string == keys[i]){
     result = vals[i];
     break;
    }
   }
  }
 }
 return result;
}

function i32
push_file_position(Driver_Collected *driver, i64 byte_pos)
{// NOTE(kv) The index returned is not final, which is sad.
 i32 insert_index = 0;
 for(i32 test_index = driver->locations.count-1;
     test_index >= 0;
     test_index--)
 {
  i32 test_location = driver->locations.items[test_index];
  if(byte_pos >= test_location)
  {
   insert_index = test_index + 1;
   break;
  }
 }
 
 if(0){
  if(insert_index != driver->locations.count){
   breakhere;  // NOTE Just a little trap
  }
 }
 
 insert_at(&driver->locations, i32(byte_pos), insert_index);
 return insert_index;
}
function M_Text_Range
push_text_range(Driver_Collected *driver, Range_i32 range)
{
 M_Text_Range result = {};
 result.range = range;
 result.guess_begin_index = push_file_position(driver, range.begin);
 push_file_position(driver, range.end);
 return result;
}
function i32
push_text_range_to_list(Driver_Collected *driver, Range_i32 range)
{
 M_Text_Range meta_range = push_text_range(driver, range);
 push(&driver->text_ranges, meta_range);
 i32 range_index = driver->text_ranges.count - 1;
 return range_index;
}
function void
init_expression(Meta_Expression *result, Expression_Kind kind)
{
 zero_struct(result);
 result->kind = kind;
}
function void
init_keep_range(Meta_Expression *result, Expression_Kind kind)
{
 Range_i32 range = result->range;
 zero_struct(result);
 result->kind = kind;
 result->range = range;
}
function b32
modify_expression2(Arena *arena, Driver_Collected *driver,
                   Meta_Expression *result)
{
 Scratch_Scope tmp(arena);
 b32 ok = true;
 Range_i32 og_range = result->range;
 
 if(result->kind == Expression_Kind_Dot_Placeholder)
 {
  init_keep_range(result, Expression_Kind_Call);
  parse_expression_from_string(arena, strlit("flp({})"), result, og_range);
 }
  
 if(result->kind == Expression_Kind_Call)
 {
  Expression_Call *call = &result->call;
  String func_name = get_function_name(call);
  
  //-
#if 0
#  define fv2
#  define vv
#  define vv0
#  define vv0_overlay
#  define fvec
#  define fvert
#  define flp
#  define fpreset
#endif
  
#define keyword_xlist(X) \
X(fval) X(fbool) X(vv) X(vv0) X(vv0_overlay) \
X(fvert) X(fvec) X(fv2) \
X(flp) \
X(fimage) X(fpreset) \
  
  String keywords[] = {
   empty_string,
#define X(N) strlit(#N),
   keyword_xlist(X)
#undef X
  };
  
  enum Mod_Keyword
  {
   KW_NONE,
#define X(N) KW_##N,
   keyword_xlist(X)
#undef X
  };
  
#undef keyword_xlist
  
  Mod_Keyword keyword = KW_NONE;
  for_i32(i, 0, alen(keywords))
  {
   if(func_name == keywords[i]){
    keyword = (Mod_Keyword)(i);
    break;
   }
  }
  
#define Match(name) (keyword == KW_##name)
#define KW(name)    KW_##name
  //-
  
  b32 is_flp = false;
  if (Match(flp)) { is_flp = true; }
  
  i32 arg_count = call->args.count;
  b32 is_draw = starts_with(func_name, strlit("draw"));
  
  b32 is_slider = false;
  if(is_flp) is_slider = true;
  
  switch(keyword)
  {
   case KW(fval):
   case KW(fbool):
   case KW(fv2):
   case KW(fvert):
   case KW(fvec):
   {
    is_slider = true;
   }break;
  }
  
  if(is_slider)
  {//-Slider
   Meta_Slider *slider = push(&driver->sliders);
   i32 slider_index = driver->sliders.count - 1;
   
   // NOTE(kv) Runtime slider is kinda ad-hoc,
   // because technically all sliders can potentially be runtime.
   // Even if you pass in a "V2(x,y)", how would the parser know if it's constant?
   //
   // One example of an ad-hoc rule is: we know a fvert/fvec slider is
   // runtime when there's only one argument.
   String runtime_value = {};
   switch(keyword)
   {
    case KW(fbool):
    {
     ok = ok and (arg_count == 1);
     slider->options = strlit("{.flags=Slider_Clamp_01}");
    }break;
    
    case KW(fval):
    {
     ok = ok and (arg_count == 1) or (arg_count == 2);
    }break;
    
    case KW(fv2):
    {
     ok = ok and (arg_count == 2);
     if(ok)
     {
      slider->type = strlit("v2");
      i32 const n = 2;
      String arg_strings[n];
      for_i32(i,0,n)
      {
       arg_strings[i] = print_expression(tmp, call->args.items[i]);
      }
      slider->value = push_stringf(arena, "V2(%S, %S)", expand2(arg_strings));
     }
    }break;
    
    case KW(fvert):
    case KW(fvec):
    {
     slider->type = strlit("v3");
     slider->options = (Match(fvert) ?
                        strlit("{.flags=Slider_Vertex}") :
                        strlit("{.flags=Slider_Vector}"));
     b32 is_data = arg_count == 3;
     b32 is_runtime = arg_count == 1;
     ok = ok and (is_data or is_runtime);
     if(is_data)
     {//-Data
      String arg_strings[3];
      for_i32(i,0,3){
       arg_strings[i] = print_expression(tmp, call->args.items[i]);
      }
      slider->value = push_stringf(arena, "V3(%S, %S, %S)", expand3(arg_strings));
     }
     else if(is_runtime)
     {//-Runtime
      runtime_value = print_expression(tmp, call->args[0]);
     }
    }break;
    
    case KW(flp):
    {
     slider->type = strlit("FUI_Line_Params");
     ok = ok and arg_count == 1;
    }break;
   }
   
   if(Match(fbool) or Match(fval) or is_flp)
   {
    String value_string = empty_string;
    if(arg_count > 0)
    {
     Meta_Expression &value_expr = call->args[0];
     if(is_empty(slider->type))
     {
      slider->type = guess_expression_type(value_expr);
     }
     value_string = print_expression(arena, value_expr);
    }
    
    slider->value = value_string;
    if(is_flp)
    {// NOTE(kv) Add type for initializer list, since we know it already.
     slider->value = push_stringf(arena, "FUI_Line_Params%S", slider->value);
    }
   }
   
   ok = ok and not_empty(slider->type);
   ok = ok and (not_empty(slider->value) or not_empty(runtime_value));
   
   slider->range = push_text_range(driver, result->range);
   
   if(Match(fval) and arg_count > 1)
   {
    Meta_Expression &option_expr = call->args[1];
    slider->options = print_expression(arena, option_expr);
   }
   
   init_keep_range(result, Expression_Kind_Unknown);
   b32 is_runtime = not_empty(runtime_value);
   if(is_runtime)
   {
    slider->is_runtime = true;
    result->as_string = push_stringf(arena, "ReadSliderRuntime(%S, %d, %S)",
                                     slider->type, slider_index, runtime_value);
   }
   else
   {//-data
    String prefix  = empty_string;
    String postfix = empty_string;
    if(is_flp)
    {// NOTE(kv) We want the *actual* line parameters to be dynamic,
     // thus it is computed at runtime. The slider only stores static data.
     prefix  = strlit("line_params_from_fui(");
     postfix = strlit(")");
    }
    result->as_string = push_stringf(arena, "%S" "ReadSlider(%S, %d)" "%S",
                                     prefix, slider->type, slider_index, postfix);
   }
  }
  else
  {
   b32 is_draw_or_fill = (is_draw or
                          starts_with(func_name, strlit("fill")));
   if(is_draw_or_fill)
   {
    //-Annotate these draws/fills with location info.
    String original_string = print_expression(tmp, *result);
    i32 file_index  = 1;  // TODO(kv) Hacked for now
    i32 range_index = push_text_range_to_list(driver, result->range);
    init_keep_range(result, Expression_Kind_Unknown);
    result->as_string = push_stringf(arena,
                                     "("
                                     "set_draw_location({.file=%d, .range_index=%d}), "
                                     "%S, "
                                     "clear_draw_location()" 
                                     ")",
                                     file_index, range_index,
                                     original_string);
   }
   else
   {
    switch(keyword)
    {
     case KW(vv):
     case KW(vv0):
     case KW(vv0_overlay):
     {
      if(call->args.count == 1)
      {
       Meta_Expression &position = call->args.items[0];
       String pos_string = print_expression(arena, position);
       i32 indicator_level = 99;
       if(Match(vv0) or Match(vv0_overlay)){ indicator_level = 0; }
       b32 overlay = Match(vv0_overlay);
       
       Meta_Vertex vertex = {};
       vertex.range = push_text_range(driver, result->range);
       vertex.overlay = overlay;
       vertex.indicator_level = indicator_level;
       push(&driver->vertices, vertex);
       i32 vertex_index = driver->vertices.count-1;
       
       init_keep_range(result, Expression_Kind_Unknown);
       result->as_string = push_stringf(arena, "send_vert(%d, %S)",
                                        vertex_index, pos_string);
      }else{
       ok = false;
      }
     }break;
     
     case KW(fimage):
     {
      ok = ok and (arg_count == 1 or
                   arg_count == 2);
      if(ok)
      {
       Meta_Expression &string_expr = call->args[0];
       String filename = print_expression(arena, string_expr);  // NOTE(kv) Let's keep the quotes
       M_Text_Object *image = push(&driver->objects);
       image->kind = Text_Object_Image;
       image->range = push_text_range(driver, result->range);
       image->image.filename = filename;
       
       if(arg_count == 2)
       {// NOTE Has marker
        Meta_Expression &marker = call->args.items[1];
        String marker_string = print_expression(tmp, marker);
        image->image.marker = push_stringf(arena, "mk_image_marker(%S)", marker_string);
       }
      }
     }break;
     
     case KW(fpreset):
     {
      ok = ok and (arg_count == 1);
      if(ok)
      {
       Meta_Expression &preset_expr = call->args[0];
       M_Text_Object preset = {.kind = Text_Object_Preset};
       preset.range  = push_text_range(driver, result->range);
       preset.preset = print_expression(arena, preset_expr);
       push(&driver->objects, preset);
       
       // NOTE(kv) This is not even gonna compile to anything, feels weird...
       init_keep_range(result, Expression_Kind_Unknown);
      }
     }break;
    }
   }
  }
 }
#undef Match
#undef KW
 return ok;
}
function void
modify_expression(Klang_Parser *parser, Meta_Expression *result)
{
 if(parser->do_modify)
 {
  b32 ok = modify_expression2(parser->arena, parser->driver, result);
  if(not ok)
  {
   parser->fail();
   parser->recoverable = false;  // TODO(kv) This is no good! (Probably don't even need it anymore?)
  }
 }
}
function void
modify_statement(Klang_Parser *parser, Token *token0, Token *token1,
                 Meta_Statement *result)
{
 Driver_Collected *driver = parser->driver;
 Arena *arena = parser->arena;
 Range_i32 statement_range = {
  i32(token0->pos),
  i32(token1->pos + token1->size)
 };
 
 if(result->head.kind == Statement_Kind_Declaration)
 {
  Statement_Declaration *decl = (Statement_Declaration *)&result->head;
  b32 is_tvert = (decl->type.kind == 0 and
                  decl->type.pointer_count == 0 and
                  decl->type.name == strlit("tvert"));
  b32 has_rhs = decl->rhs.kind != 0;
  if(is_tvert and has_rhs)
  {
   Meta_Vertex vertex = {};
   vertex.range = push_text_range(driver, statement_range);
   vertex.indicator_level = 99;
   push(&driver->vertices, vertex);
   i32 vertex_index = driver->vertices.count-1;
   
   String old_result = print_statement(arena, result->head);
   String new_result_string = push_stringf(arena, "%S\nsend_vert(%d, %S);",
                                           old_result, vertex_index, decl->name);
   // NOTE(kv) Hacked double statement
   i32 source_pos = result->head.pos;
   *result = {};
   result->head.kind = Statement_Kind_Misc;
   result->head.pos  = source_pos;
   result->misc.as_string = new_result_string;
  }
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
parse_compound_literal(Klang_Parser *p, Meta_Expression *result)
{
 result->kind = Expression_Kind_Compound;
 init_dynamic(result->compound_items, p->arena);
 
 while(p->ok_)
 {
  if(ep_maybe_char(p, '}')){ break; }
  
  Compound_Item *item = result->compound_items.push();
  if(ep_maybe_char(p, '.'))
  {//-has key name
   item->key = ep_id(p);
   ep_char(p, '=');
  }
  parse_expression_2(p, Precedence_Max, &item->value);
  
  if(not ep_maybe_char(p, ','))
  {
   ep_char(p, '}');
   break;
  }
 }
}

myinline Range_i32
range_from_token_to_here(Ed_Parser *parser, Token *token0)
{
 Token *token1 = ep_get_last_token(parser);
 Range_i32 result;
 result.min = i32(token0->pos);
 result.max = i32(token1->pos + token1->size);
 return result;
}
myinline String
string_from_token_to_here(Ed_Parser *parser, Token *token0)
{
 Token *token1 = ep_get_last_token(parser);
 String result = ep_print_token_range(parser, token0, token1);
 return result;
}
function void
parse_expression_2(Klang_Parser *p, Precedence max_precedence,
                  Meta_Expression *result)
{
 //TODO(kv) Cleanup terminators when we confirm that we don't need them.
 Arena *arena = p->arena;
 Scratch_Block scratch;
 zero_struct(result);
 
 Token *token0 = ep_get_token(p);
 String token0_string = ep_print_token(p);
 Unary_Operator unary_op = prefix_unary_operator_from_token(token0, token0_string);
 
 if(ep_maybe_kind(p, TokenBaseKind_Identifier))
 {
  if(ep_maybe_char(p, '{'))
  {// NOTE Brace initializer
   parse_compound_literal(p, result);
   result->compound_type_name = token0_string;
  }
  else
  {
   result->kind = Expression_Kind_Identifier;
   result->as_string = token0_string;
  }
 }
 else if(unary_op != 0)
 {//-Unary prefix
  ep_eat(p);
  if(max_precedence > Precedence_Unary)
  {
   result->kind = Expression_Kind_Unary;
   Expression_Unary *unary = &result->unary;
   unary->op       = unary_op;
   unary->argument = push_struct(arena, Meta_Expression);
   parse_expression_2(p, Precedence_Unary, unary->argument);
  }
  else
  {
   //NOTE(kv) Binary operators (except "." and "->") never binds
   //  stronger than unary operators.
   //  So if you hit this path, the user must have written something silly,
   //  such as "foo->-5"
   p->fail();
  }
 }
 else if(token0->kind == TokenBaseKind_LiteralString)
 {
  ep_eat(p);
  result->kind      = Expression_Kind_String;
  result->as_string = token0_string;
 }
 else if(token0->kind == TokenBaseKind_LiteralInteger)
 {//-Int
  ep_eat(p);
  result->kind      = Expression_Kind_Int;
  result->as_string = token0_string;
 }
 else if(token0->kind == TokenBaseKind_LiteralFloat)
 {//-Float
  ep_eat(p);
  result->kind   = Expression_Kind_Float;
  result->as_string = token0_string;
 }
 else if(ep_maybe_char(p, '{'))
 { //-Compound literal: array, struct
  parse_compound_literal(p, result);
 }
 else if(ep_maybe_char(p, '('))
 {//-Parenthesized expression (todo or a cast...)
  parse_expression_2(p, Precedence_Max, result);
  ep_char(p, ')');
 }
 else if(ep_maybe_char(p, '.'))
 {//-Hacked dot placeholder
  result->kind = Expression_Kind_Dot_Placeholder;
 }
 else{ p->fail(); }
 
 {//-Enlarge expression in case of binary, or terminate
  while(p->ok_)
  {
   result->range = range_from_token_to_here(p, token0);
   modify_expression(p, result);
   
   Token *op_token = ep_get_token(p);
   String op_str = ep_print_token(p, op_token);
   
   if(op_str == '(')
   {//-Function call?
    ep_eat(p);
    
    // NOTE The result so far is the function.
    Meta_Expression *func = push_value(arena, *result);
    
    init_expression(result, Expression_Kind_Call);
    Expression_Call *call = &result->call;
    call->func = func;
    
    init_dynamic(call->args, arena);
    while (p->ok_)
    {
     if (ep_maybe_char(p, ')')) break; 
     
     Meta_Expression *argument = call->args.push();
     parse_expression_2(p, Precedence_Max, argument);
     
     if(ep_maybe_char(p, ')')) break; 
     else ep_char(p, ','); 
    }
   }
   else if(op_str == '[')
   {//-Array subscript
    ep_eat(p);
    
    // NOTE The result so far is the array.
    Meta_Expression *array = push_value(arena, *result);
    
    init_expression(result, Expression_Kind_Array_Subscript);
    Expression_Array_Subscript *subscript = &result->array_subscript;
    subscript->array = array;
    
    subscript->index = push_struct(arena, Meta_Expression);
    parse_expression_2(p, Precedence_Max, subscript->index);
    
    ep_char(p, ']');
   }
   else
   {//-Binary operator
    Binary_Operator op = 0;
    Precedence op_precedence = {};
    b32 precedence_ok = false;
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
     precedence_ok = (op_precedence != 0 and
                      op_precedence < max_precedence);
    }
    
    if(op and precedence_ok)
    {
     ep_eat(p);
     
     //NOTE Put the result so far in the lhs
     Meta_Expression *lhs = push_value(arena, *result);
     
     init_expression(result, Expression_Kind_Binary);
     Expression_Binary *binary = &result->binary;
     binary->op  = op;
     binary->lhs = lhs;
     binary->rhs = push_struct(arena, Meta_Expression);
     Precedence right_max_precedence = op_precedence;
     if(is_right_associative(op))
     {
      right_max_precedence = Precedence(op_precedence+1);
     }
     parse_expression_2(p, right_max_precedence, binary->rhs);
     
     if(op_token->sub_kind == TokenCppKind_Ternary)
     {//-ternary if
      ep_char(p, ':');
      binary->ternary = push_struct(arena, Meta_Expression);
      parse_expression_2(p, right_max_precedence, binary->ternary);
     }
    }
    else { break; }
   }
  }
 }
}
function void
parse_type_and_name(Klang_Parser *p, Parsed_Type *out_type, String *out_name)
{// NOTE(kv) We're cheesing the type HARD!
 Parsed_Type type = {};
 // TODO(kv) I still don't know how const work...
 if(ep_maybe_id(p, strlit("const")))
 {
  type.flags |= Parsed_Type_IsConst;
 }
 
 type.name = ep_id(p);
 
 if(ep_maybe_id(p, strlit("const")))
 {// NOTE(kv) The "proper" way is to put "const"
  // after the type that is declared to be constant. Like "int const &foo"
  type.flags |= Parsed_Type_IsConst;
 }
 while(ep_maybe_char(p, '*')){
  type.pointer_count++;
 }
 *out_name = ep_id(p);
 
 if(type.pointer_count == 0)
 {
  if(ep_maybe_char(p, '['))
  {
   type.kind = Parsed_Type_Array;
   if(not ep_maybe_char(p, ']'))
   {
    // TODO(kv) Bad news! Count could be an expression.
    //  We know that it is a constant but our parser can't evaluate constants.
    Meta_Expression *count = push_struct(p->arena, Meta_Expression);
    parse_expression(p, count);
    type.array_count = print_expression(p->arena, *count);
    // NOTE(kv) The int value might be invalid...
    type.array_count_int = (i32)string_to_u64(type.array_count, 10);
    
    ep_char(p, ']');
   }
  }
 }
 
 *out_type = type;
}
function void
parse_struct_member(Klang_Parser *p, M_Struct_Member *result)
{
 if(ep_maybe_id(p, "tagged_by"))
 {
  mpa_parens{ result->discriminator = ep_id(p); }
 }
 parse_type_and_name(p, &result->type, &result->name);
 ep_skip_semicolons(p);
}
myinline M_Struct_Member
parse_struct_member(Klang_Parser *p)
{
 M_Struct_Member member = {};
 parse_struct_member(p, &member);
 return member;
}
myinline M_Struct_Member
struct_member_from_string(String string)
{
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
parse_struct_body(Arena *arena, Klang_Parser *p)
{
 M_Struct_Members result;
 init_dynamic(result, arena);
 m_brace_open(p);
 while(p->ok_ && !m_maybe_brace_close(p)){
  M_Struct_Member *member = result.push();
  parse_struct_member(p, member);
 }
 return result;
}
function M_Struct_Members
parse_struct_body(Arena *arena, String string)
{
 Scratch_Block scratch;
 Klang_Parser parser = k_parser_from_string(scratch, string);
 M_Struct_Members result = parse_struct_body(arena, &parser);
 return result;
}

function void
parse_statement_to_pointer(Arena *arena, Klang_Parser *p,
                           /*out*/Meta_Statement *ostatement)
{
 Token *token0 = ep_get_token(p);
 ostatement->head.pos = token0->pos;
 ostatement->head.mom = p->current_statement;
 SetInBlock(p->current_statement, &ostatement->head);
 String token0_string = ep_print_token(p, token0);
 if(token0_string == strlit("no_parse"))
 {
  ostatement->head.kind = Statement_Kind_Misc;
  
  ep_eat(p);
  ep_char(p, '{');
  ostatement->misc.as_string = ep_capture_until_char(p, '}');
  ep_char(p, '}');
 }
 else if(token0->kind == TokenBaseKind_Preprocessor)
 {//-Preprocessor (not a statement but ok...)
  ostatement->head.kind = Statement_Kind_Misc;
  cast_to_var(Statement_Misc *, unknown, ostatement);
  unknown->as_string = parse_preprocessor(p);
 }
 else if(token0->kind == TokenBaseKind_Identifier or
         token0->kind == TokenBaseKind_Keyword)
 {
  if(is_header_keyword(token0_string))
  {//-Header and body
   ep_eat(p);
   ostatement->head.kind = Statement_Kind_Header_And_Body;
   cast_to_var(Statement_Header_And_Body *, header_body, ostatement);
   if(ep_maybe_char(p, '(')){
    //NOTE optional parameters
    k_eat_until_char(p, strlit(")"));
    ep_char(p, ')');
   }
   header_body->header = k_string_from_token_to_current(p, token0);
   header_body->body = parse_statement_to_arena(arena, p);
  }
  else if(token0_string == strlit("if"))
  {//-If
   ep_eat(p);
   ostatement->head.kind = Statement_Kind_If;
   cast_to_var(Statement_If *, if0, ostatement);
   {//-condition
    ep_char(p, '(');
    parse_expression(p, &if0->condition);
    ep_char(p, ')');
   }
   {//-body
    if0->body = parse_statement_to_arena(arena, p);
   }
   if(ep_maybe_id(p, strlit("else")))
   {//-else
    if0->else0 = parse_statement_to_arena(arena, p);
   }
  }
  else if(ep_maybe_id(p, strlit("switch")))
  {//-switch
   ostatement->head.kind = Statement_Kind_Switch;
   cast_to_var(Statement_Switch *, switch0, ostatement);
   {//-expression
    ep_char(p, '(');
    parse_expression(p, &switch0->expression);
    ep_char(p, ')');
   }
   {//-cases
    ep_char(p, '{');
    while(p->ok_ && not ep_maybe_char(p, '}')){
     Switch_Case *case0 = switch0->cases.push();
     ep_id(p, strlit("case"));
     parse_expression(p, &case0->expression);
     ep_char(p, ':');
     parse_statement_to_pointer(arena, p, &case0->body);
     case0->break_after = ep_maybe_id(p, strlit("break"));
     ep_skip_semicolons(p);
    }
   }
  }
  else if(ep_maybe_id(p, strlit("return")))
  {//-Return
   ostatement->head.kind = Statement_Kind_Return;
   cast_to_var(Statement_Return *, return0, ostatement);
   parse_expression(p, &return0->return0);
   ep_char(p,';');
  }
  else if(ep_maybe_id(p, strlit("cache")))
  {//-cache
   ostatement->head.kind = Statement_Kind_Cache;
   cast_to_var(Statement_Cache *, cache0, ostatement);
   cache0->id = i32(token0->pos);
   init_dynamic(cache0->cache_items, arena);
   ep_char(p, '(');
   while(p->ok_ && not ep_maybe_char(p, ')')){
    //-Cached items
    Cache_Item *cache_item = cache0->cache_items.push();
    parse_type_and_name(p, &cache_item->type, &cache_item->name);
    ep_char(p, '=');
    parse_expression(p, &cache_item->rhs);
    ep_char(p, ';');
   }
   {//-Cached computation
    cache0->body = parse_statement_to_arena(arena, p);
   }
   //-Remember this statement so we can print out the metadata later
   //p->function_cache_list.push_value(cache0);
  }
  else if(token0_string == strlit("continue") or
          token0_string == strlit("break"))
  {
   ep_eat(p);
   ep_char(p, ';');
   ostatement->head.kind = Statement_Kind_Misc;
   Statement_Misc *statement = (Statement_Misc *)ostatement;
   // NOTE(kv) Include the semicolon
   statement->as_string = string_from_token_to_here(p, token0);
  }
  else
  {//-Declaration?
   ep_recovery_block(p);
   
   ostatement->head.kind = Statement_Kind_Declaration;
   cast_to_var(Statement_Declaration *, decl, ostatement);
   parse_type_and_name(p, &decl->type, &decl->name);
   if(ep_maybe_char(p,'='))
   {//-Declaration and assignment
    parse_expression(p, &decl->rhs);
   }
   ep_char(p,';');
   
   if(not p->ok_){
    ostatement->head.kind = Statement_Kind_None;
   }
  }
 }
 else if(token0->kind == TokenBaseKind_ScopeOpen)
 {//-Block
  ostatement->head.kind  = Statement_Kind_Block;
  cast_to_var(Statement_Block*, block, ostatement);
  block->block = parse_statement_block(arena, p);
 }
 else if(ep_eat_kind(p, TokenBaseKind_StatementClose))
 {
  ostatement->head.kind = Statement_Kind_Empty;
 }
 
 if(not ostatement->head.kind)
 {//-Defaults to expressions
  //NOTE(kv) Warning: sometimes we use macro, forget a semicolon,
  //  and it parses until the end of the file.
  ep_scope_block(p, token0_string, token0);
  ostatement->head.kind = Statement_Kind_Expression;
  cast_to_var(Statement_Expression*, expr, ostatement);
  parse_expression(p, &expr->expression);
  ep_char(p, ';');
 }
 
 if(p->ok_)
 {
  Token *statement_last = ep_get_token_delta(p, -1);
  modify_statement(p, token0, statement_last, ostatement);
 }
}
function sarray(Meta_Statement)
parse_statement_block(Arena *arena, Klang_Parser *p)
{
 ep_char(p,'{');
 darray(Meta_Statement) statements;
 init_dynamic(statements, arena);
 ep_skip_semicolons(p);
 while(p->ok_ and (not ep_maybe_char(p,'}'))){
  //-Statement
  parse_statement_to_pointer(arena, p, statements.push());
  ep_skip_semicolons(p);
 }
 return statements;
}
function sarray(Meta_Statement)
k_process_top_level(Arena *arena, Klang_Parser *p, Meta_Printer &printer,
                    String source_path,
                    darray(String) *type_info_list)
{
 darray(Meta_Statement) top_levels;
 init_dynamic(top_levels, arena, 64);
 Scratch_Block tmp_file;
 darray(T_Table) tables;
 init_dynamic(tables, tmp_file, 16);
 String template_out_dir = path_dir(source_path);
 Scratch_Block tmp_top;
 
 while(p->ok_)
 {
  arena_clear(tmp_top);
  {//-whitespace token
   while(true)
   {
    Token *token = ep_get_token(p);
    if(token->kind == TokenBaseKind_Whitespace or
       token->kind == TokenBaseKind_Comment or
       token->kind == TokenBaseKind_StatementClose)
    {
     if(token->kind != TokenBaseKind_Comment)
     {
      ep_print_token(printer, p);
     }
     ep_eat_inc_all(p);
    }
    else{ break; }
   }
   ep_skip_comments_and_spaces(p);
  }
  Token *token0 = ep_get_token(p);
  String token0_string = ep_print_token(tmp_top, p);
  ep_scope_block(p, strlit("top-level"), token0);
  String type_name = {};
  b32 do_info  = true;
  b32 do_embed = false;
  b32 is_packed = false;
  if(m_maybe_bracket_open(p))
  {//-Struct attributes (like the clang attributes)
   while(p->ok_ && !m_maybe_bracket_close(p))
   {
    String string = ep_print_token(tmp_top, p);
    if(string == strlit("noinfo"))
    {
     do_info = false;
    }
    else if(string == strlit("embed"))
    {
     do_embed = true;
    }
    else if(string == strlit("packed"))
    {
     is_packed = true;
    }
    else{ p->fail(); }
    ep_eat(p);
   }
  }
  
  if(token0->kind == TokenBaseKind_EOF)
  {
   break;
  }
  else if(ep_maybe_id(p, "no_parse"))
  {
   ep_char(p, '{');
   String code = ep_capture_until_char(p, '}');
   print(printer, code);
   ep_char(p, '}');
  }
  else if(ep_maybe_id(p, "struct"))
  {//-parse struct
   darray(M_Struct_Member) members = {};
   type_name = ep_id(p);
   ep_char(p, '{');
   while(p->ok_ && !m_maybe_brace_close(p))
   {// NOTE: Field
    M_Struct_Member *member = members.push();
    
    if(ep_maybe_id(p, "meta_removed"))
    {//-meta_removed
     ep_char(p, '(');
     
     parse_struct_member(p, member);
     
     if(meta_maybe_key(p, strlit("added")))
     {
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
    }
    else
    {
     if(ep_maybe_id(p, "meta_added"))
     {//-meta_added
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
   
   print_struct(printer, type_name, members, is_packed);
   if(do_info){
    print_struct_meta(printer, type_name, members);
   }
   if(do_embed){
    print_struct_embed(printer, type_name, members);
   }
  }
  else if(ep_maybe_id(p, "union"))
  {//-Union
   todo_incomplete;
  }
  else if(ep_maybe_id(p, "enum"))
  {//-Enum
   darray(String) enum_names = {};
   darray(i1) enum_vals      = {};
   type_name = ep_maybe_id(p);
   m_brace_open(p);
   while(p->ok_ && !m_maybe_brace_close(p)){
    //NOTE(kv) Enum value
    push(&enum_names, ep_id(p));
    ep_char(p, '=');
    push(&enum_vals, ep_i1(p));
    ep_eat_until_char_simple(p, ',');  // NOTE(kv) The ending comma is optional, but I don't care.
   }
   
   print_enum(printer, type_name, enum_names, enum_vals);
   if(type_name.len!=0 && do_info){
    //NOTE(kv) Anonymous enums can't be read, since it can't be referred to.
    print_enum_meta(printer, type_name, enum_names);
   }
  }else if(ep_maybe_id(p, "typedef")){
   //-typedef
   String typedef_to = ep_id(p);
   type_name  = ep_id(p);
   {
    printer<"typedef "<typedef_to<" "<type_name<";\n";
   }
   if(do_info){
    print_typedef_meta(printer, type_name, typedef_to);
   }
  }else if(token0->kind == TokenBaseKind_Preprocessor){
   //-Preprocessor
   String preproc_string = parse_preprocessor(p);
   printer < preproc_string;
  }
  else if(ep_maybe_id(p, strlit("global")))
  {//-global (now this is annoying!)
   Parsed_Type type;
   String name;
   parse_type_and_name(p, &type, &name);
   
   print(printer, strlit("global "));
   print_type_and_name(printer, type, name);
   
   if(ep_maybe_char(p, '='))
   {
    Meta_Expression rhs;
    parse_expression(p, &rhs);
    
    print(printer, strlit(" = "));
    print_expression(printer, rhs);
   }
   
   ep_char_inc_all(p, ';');
   print(printer, ';');
  }else if(token0_string == strlit("xfunction")  or
           token0_string == strlit("function")   or
           token0_string == strlit("inline")     or
           token0_string == strlit("myinline")   or
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
   Meta_Statement *func0 = top_levels.push();
   cast_to_var(Statement_Function *, func, func0);
   if(ep_maybe_char(p, ';')){
    //-Forward declaration
    func->kind = Statement_Kind_Function;
   }else{
    //-Body
    func->kind = Statement_Kind_Function;
    func->body = parse_statement_block(arena, p);
    func->has_body = true;
   }
   
   if(p->ok_)
   {//-Print
#if 0
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
      m_locationp(printer);
      print_cache_storage(printer, cache0);
     }
    }
#endif
    {//-Print prototype
     add_to_source_map(&printer.source_map, printer, token0->pos);
     printer<token0_string<" "<return_type;
     mline(printer);
     printer<function_name;
     m_parens2(printer){
      printer<parameters;
     }
    }
    if(func->has_body){//-Print body
     m_braces2(printer){
      mline(printer);
      for_i32(statement_index,0,func->body.count){
       print_statement(printer, func->body[statement_index].head);
       mline(printer);
      }
     }
     mline(printer);
    }else{//-declaration
     print(printer, ";\n\n");
    }
   }
  }
  else if(ep_maybe_id(p, "int_wrapper_type"))
  {//-int wrapper (todo deprecate this?)
   String int_name_str = ep_id(p);
   Int_Name int_name = Int_Name_i32;
   if(int_name_str == "i32"){
    int_name = Int_Name_i32;
   }else if(int_name_str == "u32"){
    int_name = Int_Name_u32;
   }else{
    p->fail();
   }
   type_name = ep_id(p);
   print_int_wrapper(printer, type_name, int_name);
  }
  else if(ep_maybe_id(p, "unique"))
  {//-One-off/miscellaneous stuff
   if(ep_maybe_id(p, "Curve_Type"))
   {
    generate_entity_types(printer);
   }else{
    p->fail();
   }
  }
  else if(ep_maybe_id(p, strlit("meta_table")))
  {//-tables
   T_Table *table = push(&tables);
   {//-Fields
    ep_char(p, '(');
    while(p->ok_ and not ep_maybe_char(p, ')'))
    {
     String field_name = ep_id(p);
     push(&table->field_names, field_name);
     if(not ep_maybe_char(p, ','))
     {
      ep_char(p, ')');
      break;
     }
    }
   }
   i32 field_count = get_field_count(table);
   
   table->name = ep_id(p);
   
   init_dynamic(table->items, tmp_file, 16);
   ep_char(p, '{');
   while(p->ok_ and
         not ep_maybe_char(p, '}'))
   {//-Table items
    String *item = push_array(tmp_file, String, field_count);
    push(&table->items, item);
    for_i32(field_index, 0, field_count){
     item[field_index] = template_parse_string(p);
    }
    if(not ep_maybe_char(p, ',')){
     ep_char(p, '}');
    }
   }
  }
  else if(ep_maybe_id(p, strlit("api_table")))
  {//-api table (NOTE "api" is taken, plus "api_table" is clearer, also I mean who cares)
   T_Table *table = tables.push();
   table->field_names.set_count(3);
   table->field_names[0] = strlit("name");
   table->field_names[1] = strlit("return");
   table->field_names[2] = strlit("params");
   
   table->name = ep_id(p);
   
   init_dynamic(table->items, tmp_file, 16);
   ep_char(p, '{');
   while(p->ok_ and
         not ep_maybe_char(p, '}'))
   {//-Function signatures
    String *signature = push_array(tmp_file, String, 3);
    push(&table->items, signature);
    
    String return_type;
    {
     Token *return_start = ep_get_token(p);
     ep_id(p);
     Token *return_end = return_start;
     while(true){
      Token *test = ep_get_token(p);
      String str = ep_print_token(p, test);
      if(str == '*'){
       return_end = test;
       ep_eat(p);
      }else{
       break;
      }
     }
     
     i64 return_type_size = return_end->pos + return_end->size - return_start->pos;
     kv_assert(return_type_size > 0);
     return_type = String{
      p->source.str + return_start->pos,
      u64(return_type_size),
     };
    }
    
    String function_name = ep_id(p);
    
    ep_char(p, '(');
    String parameters = ep_capture_until_char(p, ')');
    ep_char(p, ')');
    
    signature[0] = function_name;
    signature[1] = return_type;
    signature[2] = parameters;
    
    ep_char(p, ';');
   }
  }
  else if(ep_maybe_id(p, strlit("gen_file")) or
          ep_maybe_id(p, strlit("generate")))
  {//-output to a different file
   b32 gen_to_file = token0_string == strlit("gen_file");
   Meta_Printer *printer_0 = &printer;
   
   if(gen_to_file)
   {
    Token *filename_token = ep_get_token(p);
    p->set_ok(filename_token->kind == TokenBaseKind_LiteralString);
    ep_eat(p);
    
    String filename = ep_print_token(p, filename_token);
    kv_assert(filename.len >= 2);
    filename.str++;
    filename.len -= 2;
    
    Stringz out_path = pjoin(tmp_file, template_out_dir, filename);
    Meta_Printer printer_ = {};
    (Printer&)printer_ = m_open_file_to_write(out_path);
    
    printer_0 = &printer_;
    print_format(*printer_0, "// NOTE Source template: %S\n", source_path);
   }
   
   template_codegen_mode(&tables, p, *printer_0);
   
   kv_assert(not printer_0->error);
   if(gen_to_file)
   {
    close(*printer_0);
   }
  }
  else{ p->fail(); }
  
  //-
  if(type_name.count and do_info)
  {
   push(type_info_list, type_name);
  }
  
  kv_assert(not p->recoverable);
 }
 return top_levels;
}
#include "meta_file_formats.h"

function b32
k_process_file(Arena *arena, Lexed_File source,
               Driver_Collected *driver,
               darray(String) *type_info_list,
               Statement_Root *out_root)
{
 Scratch_Block tmp_file;
 Statement_Root root = {};
 root.kind = Statement_Kind_Root;
 
 Meta_Printer printer_gen;
 Stringz map_file_path = get_map_path_from_source_path(tmp_file, source.path);
 Stringz side_file_path;
 Stringz gen_path;
 {//-filepath business
  {
   String source_dir = path_dir(source.path);
   String filename = path_filename(source.path);
   String stem = path_stem(filename);
   String extension = path_extension(filename);
   b32 is_kh = extension == strlit("kh");
   if(not is_kh){
    kv_assert(extension == strlit("kc"));
   }
   //-Generated file path
   const char *gen_extension = (is_kh ? "h" : "cpp");
   gen_path = push_stringf(tmp_file, "%S/%S.gen.%s", source_dir, stem, gen_extension);
  }
  {
   (Meta_Printer&)printer_gen = m_open_file_to_write(gen_path);
   printer_gen < "// NOTE: source: " < source.path < "\n";
  }
 }
 init_dynamic(printer_gen.source_map, tmp_file, 256);
 b32 ok = not printer_gen.error;
 Klang_Parser klang_parser = {};
 Klang_Parser *parser = &klang_parser;
 {
  Ed_Parser *ed_parser = parser;
  *ed_parser = ed_parser_from_token_list(source.data, source.token_list);
  parser->current_statement = &root;
  parser->driver            = driver;
  parser->arena             = arena;
  parser->do_modify         = true;
 }
 
 root.top_levels = k_process_top_level(arena, parser, printer_gen, source.path, type_info_list);
 
 ok = ok and parser->ok_;
 if(ok)
 {//-Print source map
  FILE *file = open_or_create_file(map_file_path, "wb");
  ok = ok and file != 0;
  if(file)
  {// NOTE(kv) View Meta_Map_File_Header
   Source_Map &map = printer_gen.source_map;
   //NOTE Magic
   u64 zero = 0;
   
   char *magic = "kmap";
   fwrite(magic, 1, sizeof(u32), file);
   
   //NOTE Source name info
   fwrite(&zero, 1, sizeof(i32), file); // NOTE skip offset
   i32 source_count = source.path.count;
   fwrite(&source_count, 1, sizeof(i32), file);
   
   //NOTE Gen name info
   fwrite(&zero, 1, sizeof(i32), file); // NOTE skip offset
   i32 gen_count = gen_path.count;
   fwrite(&gen_count, 1, sizeof(i32), file);
   
   //NOTE Number of items
   fwrite(&map.count, 1, sizeof(i32), file);
   
   //NOTE The bulk of the data
   fwrite(map.items, 1, map.count*sizeof(*map.items), file);
   
   //NOTE Source name
   i32 source_offset = ftell(file);
   fwrite(source.path.str, 1, source_count, file);
   
   //NOTE Gen name
   i32 gen_offset = ftell(file);
   fwrite(gen_path.str, 1, gen_count, file);
   
   //NOTE Write offsets
   {
    i32 pos = offsetof(Meta_Map_File_Header, source_name_offset);
    fseek(file, pos, 0);
    fwrite(&source_offset, 1, sizeof(i32), file);
   }
   {
    i32 pos = offsetof(Meta_Map_File_Header, gen_name_offset);
    fseek(file, pos, 0);
    fwrite(&gen_offset, 1, sizeof(i32), file);
   }
   
   close_file(file);
  }
 }
 
 if(not parser->ok_)
 {
  Line_Column fail_location  = ep_get_fail_location(parser);
  Line_Column scope_location = ep_get_scope_location(parser);
  myprintf("%S:%d:%d [klang: %S:%d:%d] parse error\n",
           source.path,
           fail_location.line,
           fail_location.column,
           parser->scope_.name,
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
k_print_struct_meta(Printer &p, K_Struct struc)
{
 print_struct_meta(p, struc.name, struc.members);
}
function b32
klang_main_one_file(Arena *arena, Lexed_File source_file,
                    Driver_Collected *driver,
                    darray(String) *type_info_list)
{
 Statement_Root root;
 b32 ok = k_process_file(arena, source_file, driver, type_info_list, &root);
 if(ok){
  meta_process_ast(root, source_file.path);
 }
 return ok;
}
function b32
klang_main(sarray(Lexed_File) all_files, b32 test_klang)
{
 b32 ok = true;
 Scratch_Block tmp;
 Driver_Collected driver = {};
 init_dynamic(driver.locations,   tmp, 1024);
 init_dynamic(driver.text_ranges, tmp, 512);
 init_dynamic(driver.sliders,     tmp, 512);
 init_dynamic(driver.objects,     tmp, 128);
 init_dynamic(driver.vertices,    tmp, 256);
 
 darray(String) type_info_list;
 init_dynamic(type_info_list, tmp, 64);
 
 for_i32(file_index, 0, all_files.count)
 {//-Parsing all the files
  if(not ok){ break; }
  
  Lexed_File file = all_files[file_index];
  if(is_klang_file(file.path))
  {
   b32 is_test_file = path_filename(file.path) == "test.kc";
   b32 do_this_file = false;
   if(test_klang){
    do_this_file = is_test_file;
   }else{
    do_this_file = not is_test_file;
   }
   
   if(do_this_file)
   {
    ok = ok and klang_main_one_file(tmp, file,
                                    &driver, &type_info_list);
   }
  }
 }
 
 print_all_type_info(type_info_list);
 
 if(ok){
  print_all_script_data(driver);
 }
 return ok;
}
//-