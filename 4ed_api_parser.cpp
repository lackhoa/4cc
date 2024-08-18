/*
 * Mr. 4th Dimention - Allen Webster
 *
 * 03.10.2019
 *
 * Parser that extracts an API from C++ source code.
 *
 */

// TOP

/*
function:
api ( <identifier> ) function <identifier> {*} <identifier> ( <param_list> )

param_list:
void |
<identifier> {*} <identifier> [, <identifier> {*} <identifier>]

anything_else:
***

api_source:
{function|anything_else} EOF
*/

function Token*
api_parse__token_pos(Token_Iterator *it){
    return(token_it_read(it));
}

function b32
api_parse__match(Token_Iterator *it, Token_Cpp_Kind sub_kind){
    b32 match = false;
    Token *token = token_it_read(it);
    if (token != 0 && token->sub_kind == sub_kind){
  if (token_it_inc(it)){
   match = true;
  }
 }
 return(match);
}

function b32
api_parse__ident(Token_Iterator *it, String source, String *lexeme)
{
 b32 match = false;
 Token *token = token_it_read(it);
 if (token->kind == TokenBaseKind_Identifier ||
     token->kind == TokenBaseKind_Keyword)
 {
  if (token_it_inc(it))
  {
   *lexeme = string_substring(source, Ii64(token));
   match = true;
  }
 }
 return(match);
}

function b32
api_parse__match_ident(Token_Iterator *it, String source, char *lexeme)
{
 b32 match = false;
 Token *token = token_it_read(it);
 if ((token->kind == TokenBaseKind_Identifier ||
      token->kind == TokenBaseKind_Keyword) &&
     string_match(SCu8(lexeme), string_substring(source, Ii64(token))))
 {
  if (token_it_inc(it)){
   match = true;
  }
 }
 return(match);
}

function String
api_parse__type_name_with_stars(Arena *arena, String type, i1 star_counter){
    if (star_counter > 0){
        i1 type_full_size = (i1)(type.size) + star_counter;
        u8 *type_buffer = push_array(arena, u8, type_full_size + 1);
        block_copy(type_buffer, type.str, type.size);
        block_fill_u8(type_buffer + type.size, star_counter, (u8)'*');
        type_buffer[type_full_size] = 0;
        type = SCu8(type_buffer, type_full_size);
    }
    return(type);
}

function void
api_parse_add_param(Arena *arena, API_Param_List *list, String type, i1 star_counter, String name){
    type = api_parse__type_name_with_stars(arena, type, star_counter);
    API_Param *param = push_array(arena, API_Param, 1);
 sll_queue_push(list->first, list->last, param);
 list->count += 1;
 param->type_name = type;
 param->name = name;
}

function void
api_parse_add_function(Arena *arena, API_Definition_List *list,
                       arrayof<String> api_names, String func_name,
                       String type, i1 star_counter, API_Param_List param_list,
                       String location)
{
 type = api_parse__type_name_with_stars(arena, type, star_counter);
 for_i32(index,0,api_names.count)
 {
  API_Definition *api = api_get_api(arena, list, api_names[index]);
  API_Call *call = api_call_with_location(arena, api, func_name, type, location);
  api_set_param_list(call, param_list);
 }
}

function void
api_parse_add_structure(Arena *arena, API_Definition_List *list,
                        arrayof<String> api_names, API_Type_Structure_Kind kind,
                        String name, List_String member_list,
                        String definition, String location)
{
 for_i32(index,0,api_names.count)
 {
  API_Definition *api = api_get_api(arena, list, api_names[index]);
  api_type_structure_with_location(arena, api, kind, name, member_list, definition, location);
 }
}

function String
api_parse_location(Arena *arena, String source_name, String source, u8 *pos){
    i1 line_number = 1;
    i1 col_number = 1;
    if (source.str <= pos && pos < source.str + source.size){
        for (u8 *ptr = source.str;;){
            if (ptr == pos){
                break;
            }
            if (*ptr == '\n'){
                line_number += 1;
                col_number = 1;
            }
            else{
                col_number += 1;
   }
   ptr += 1;
  }
 }
 return(push_stringfz(arena, "%.*s:%d:%d:", string_expand(source_name), line_number, col_number));
}

function b32
api_parse_source__function(Arena *arena, String source_name, String source, Token_Iterator *token_it, arrayof<String> api_names, API_Definition_List *list)
{
 //NOTE(kv): Doesn't support macro-as-parameter because it parses parameters,
 // not sure if we wanna add it?
 b32 result = false;
 String ret_type = {};
 i1 ret_type_star_counter = 0;
 String func_name = {};
 API_Param_List param_list = {};
 if (api_parse__ident(token_it, source, &ret_type)){
  for (;api_parse__match(token_it, TokenCppKind_Star);){
   ret_type_star_counter += 1;
  }
  if (api_parse__ident(token_it, source, &func_name)){
   //printf("Func name: %.*s\n", string_expand(func_name));
   
   if (api_parse__match(token_it, TokenCppKind_ParenOp)){
    b32 param_list_success = false;
    if (api_parse__match_ident(token_it, source, "void")){
     param_list_success = true;
    }
    else{
     for (;;){
      String type = {};
      i1 star_counter = 0;
      String name = {};
      if (api_parse__ident(token_it, source, &type)){
       for (;api_parse__match(token_it, TokenCppKind_Star);){
        star_counter += 1;
       }
       if (api_parse__ident(token_it, source, &name)){
        param_list_success = true;
       }
       else{
        break;
       }
      }
      else{
       break;
      }
      if (param_list_success){
       api_parse_add_param(arena, &param_list, type, star_counter, name);
      }
      if (api_parse__match(token_it, TokenCppKind_Comma)){
       param_list_success = false;
      }
      else{
       break;
      }
     }
    }
    if (param_list_success){
     if (api_parse__match(token_it, TokenCppKind_ParenCl)){
      result = true;
     }
    }
   }
  }
 }
 if (result){
  String location = api_parse_location(arena, source_name, source, func_name.str);
  api_parse_add_function(arena, list, api_names, func_name, ret_type, ret_type_star_counter, param_list, location);
 }
 return(result);
}

function String
api_parse__restringize_token_range(Arena *arena, String source, Token *token, Token *token_end){
    List_String list = {};
    for (Token *t = token; t < token_end; t += 1){
        if (t->kind == TokenBaseKind_Comment){
            continue;
        }
        if (t->kind == TokenBaseKind_Whitespace){
            // TODO(allen): if there is a newline, emit it, all other whitespace is managed automatically.
            continue;
        }
  
  String str = string_substring(source, Ii64(t));
  string_list_push(arena, &list, str);
 }
 return(string_list_flatten(arena, list));
}

function b32
api_parse_source__structure(Arena *arena, String source_name, String source, API_Type_Structure_Kind kind, Token_Iterator *token_it, arrayof<String> api_names, API_Definition_List *list)
{
    b32 result = false;
    String name = {};
    List_String member_list = {};
    Token *token = api_parse__token_pos(token_it);
    (void)token;
    if (api_parse__ident(token_it, source, &name)){
        if (api_parse__match(token_it, TokenCppKind_Semicolon)){
            result = true;
        }
        else if (api_parse__match(token_it, TokenCppKind_BraceOp)){
                b32 member_list_success = false;
            for (;;){
                String member_name = {};
                if (api_parse__match(token_it, TokenCppKind_BraceCl)){
                    member_list_success = true;
                    break;
                }
                else if (api_parse__ident(token_it, source, &member_name)){
                    if (api_parse__match(token_it, TokenCppKind_Semicolon)){
                        string_list_push(arena, &member_list, member_name);
                    }
                }
                else{
                    if (!token_it_inc(token_it)){
                        break;
                    }
                }
            }
            if (member_list_success){
                if (api_parse__match(token_it, TokenCppKind_BraceCl)){
                    if (api_parse__match(token_it, TokenCppKind_Semicolon)){
                        result = true;
                    }
                    }
                }
            }
    }
    if (result){
        Token *token_end = api_parse__token_pos(token_it);
        (void)token_end;
        // TODO(allen): 
  String definition = {};
  String location = api_parse_location(arena, source_name, source, name.str);
  api_parse_add_structure(arena, list, api_names, kind, name, member_list, definition, location);
 }
 return(result);
}

force_inline b32
api_parse_source__struct(Arena *arena, String source_name, String source, Token_Iterator *token_it, arrayof<String> api_names, API_Definition_List *list){
 return(api_parse_source__structure(arena, source_name, source, APITypeStructureKind_Struct, token_it, api_names, list));
}

force_inline b32
api_parse_source__union(Arena *arena, String source_name, String source, Token_Iterator *token_it, arrayof<String> api_names, API_Definition_List *list){
 return(api_parse_source__structure(arena, source_name, source, APITypeStructureKind_Union, token_it, api_names, list));
}

function void
api_parse_source_add_to_list(Arena *arena, String source_name, String source, API_Definition_List *list)
{
 Token_List token_list = lex_full_input_cpp(arena, source);
 Token_Iterator token_it = make_token_iterator(token_iterator(0, &token_list));
 
 for (;;)
 {
  Token *token = token_it_read(&token_it);
  if (token->sub_kind == TokenCppKind_EOF){
   break;
  }
  
  if (api_parse__match_ident(&token_it, source, "api"))
  {
   const i1 api_cap = 8;
   String buffer[api_cap];
   arrayof<String> api_names;
   init_static(api_names, buffer, api_cap);
   if (api_parse__match(&token_it, TokenCppKind_ParenOp))
   {
    String name;
    while( api_parse__ident(&token_it, source, &name) )
    {
     api_names.push(name);
     if (api_parse__match(&token_it, TokenCppKind_ParenCl)) {
      break;
     }
    }
    {
     if (api_parse__match_ident(&token_it, source, "function")){
      api_parse_source__function(arena, source_name, source, &token_it, api_names, list);
     }
     else if (api_parse__match_ident(&token_it, source, "struct")){
      api_parse_source__struct(arena, source_name, source, &token_it, api_names, list);
     }
     else if (api_parse__match_ident(&token_it, source, "union")){
      api_parse_source__union(arena, source_name, source, &token_it, api_names, list);
     }
    }
   }
  }
  else if (!token_it_inc(&token_it)) {
   break;
  }
 }
}

function API_Definition_List
api_parse_source(Arena *arena, String source_name, String source){
 API_Definition_List list = {};
 api_parse_source_add_to_list(arena, source_name, source, &list);
 return(list);
}

function b32
api_parser_main(arrayof<File_Name_Data> source_files)
{
 b32 ok = true;
 Arena scratch = make_arena_malloc();
 
 API_Definition_List list = {};
 for_i1(index,0,source_files.count)
 {
  auto source_file = source_files[index];
  api_parse_source_add_to_list(&scratch, source_file.name, source_file.data, &list);
 }
 
 if (ok)
 {
  for (API_Definition *node = list.first;
       node != 0;
       node = node->next)
  {
   ok = api_definition_generate_api_includes(&scratch, node, GeneratedGroup_Custom, APIGeneration_NoAPINameOnCallables);
   if (!ok) { break; }
  }
 }
 
 return ok;
}

// BOTTOM
