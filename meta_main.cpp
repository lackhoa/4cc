/* 
 * Mr. 4th Dimention - Allen Webster (Modified by kv)
 * Do all the meta programming things
 */
#define KV_H_IS_METAPROGRAM 1
#include "kv.h"

//NOTE(kv) not needed but whatevs
#include "kv_math.h"

#include "4coder_token.h"
#include "generated/lexer_cpp.h"
#include "4ed_base.h"
#include "4ed_api_definition.h"
#include "4coder_stringf.cpp"
#include "4coder_malloc_allocator.cpp"
#include "4coder_token.cpp"
#include "generated/lexer_cpp.cpp"
#include "4ed_kv_parser.h"
#include "4ed_kv_parser.cpp"

#include "meta_print.h"
#include "meta_parse.h"
#include "meta_klang.h"
#include "meta_entity.h"
#include "meta_main.h"
#include "meta_template.h"
#include "build_main.h"

#include "4ed_api_definition.cpp"
#include "meta_os.cpp"
#include "meta_parse.cpp"
#include "4ed_system_api.cpp"
#include "4ed_api_parser.cpp"
#include "meta_print.cpp"
#include "meta_klang.cpp"
#include "meta_entity.cpp"
#include "meta_template.cpp"
#include "build_main.cpp"
//-
function b32
is_klang_file(String path)
{
 String extension = path_extension(path);
 return (extension == strlit("kh") or
         extension == strlit("kc"));
}
function b32
is_template_file(String path)
{
 String extension = path_extension(path);
 return (extension == strlit("kt"));
}
function b32
is_klang_or_template_file(String path)
{
 b32 result = is_klang_file(path) or is_template_file(path);
 return result;
}
//-
function void
meta_process_ast(Statement_Root root, String source_path)
{
 kv_assert(root.kind == Statement_Kind_Root);
 if(DEBUG_vv_name){
  //-Vertex check
  Scratch_Block scratch;
  darray(String) existing_names;
  init_dynamic(existing_names, scratch, 200);
  
  struct Stack_Entry{
   Statement_Union *statements;  //TODO(kv) We could store Statement_Head here, but then only for one pointer
   u64 statement_count;
   u32 next_index;
  };
  darray(Stack_Entry) stack;
  init_dynamic(stack, scratch, 32);
  
  auto stack_push_block = [&](sarray(Statement_Union) *block){
   Stack_Entry entry = {block->items, block->count, 0};
   stack.push_value(entry);
  };
  auto stack_push_statement = [&](Statement_Head *statement){
   stack.push_value({cast(Statement_Union *)statement, 1, 0});
  };
  
  stack_push_block(&root.top_levels);
  while(true){
   Statement_Head *statement = 0;
   {//-Pop the stack
    while(not statement and stack.count > 0){
     Stack_Entry *last = &stack.last();
     if(last->next_index < last->statement_count){
      statement = &last->statements[last->next_index++].head;
     }else{
      stack.pop();
     }
    }
   }
   if(statement){
    if(statement->kind == Statement_Kind_Function){
     //-Function
     cast_to_var(Statement_Function*, func, statement);
     stack_push_block(&func->body);
    }else if(statement->kind == Statement_Kind_Block){
     //-Block
     cast_to_var(Statement_Block *, block, statement);
     stack_push_block(&block->block);
    }else if(statement->kind == Statement_Kind_If){
     //-If
     cast_to_var(Statement_If *, if0, statement);
     stack_push_statement(if0->else0);
     stack_push_statement(if0->body);
    }else{
     //-Leaf
     if(statement->kind == Statement_Kind_Expression){
      //-Expression
      cast_to_var(Statement_Expression*, statement_expression, statement);
      Meta_Expression &expr = statement_expression->expression;
      if(expr.kind == Expression_Kind_Call){
       Expression_Call *call = &expr.call;
       String function_name = get_function_name(call);
       if(function_name == strlit("vv") or
          function_name == strlit("va") or
          function_name == strlit("vv_sample") or
          function_name == strlit("send_vert"))
       {
        //-Is vertex
        String vertex_name = call->arguments[0].as_string;
        {
         Statement_Head *test = statement->mom;
         while(test){
          //-"if" check
          if(test->kind == Statement_Kind_If){
           printf("[kv]%.*s[%d] error: vertex used within if block\n",
                  strexpand(source_path),
                  statement->pos);
           break;
          }
          test = test->mom;
         }
        }
        {//-conflict check
         b32 conflict = false;
         for_u32(existing_name_index,0,existing_names.count){
          if(vertex_name == existing_names[existing_name_index]){
           //-conflict
           printf("[kv]%.*s[%d]: error: conflicting name found: %.*s\n",
                  strexpand(source_path),
                  statement->pos,
                  strexpand(vertex_name));
           conflict = true;
           break;
          }
         }
         if(not conflict){
          //-new name -> add to the pool
          existing_names.push_value(vertex_name);
         }
        }
       }
      }
     }
    }
   }else{
    break;
   }
  }
  printf("Total vertex count: %zu\n", existing_names.count);
 }
}
function Lexed_File
lex_file(Arena *arena, Stringz filename)
{
 Lexed_File result = {};
 FILE *file = open_file(filename, "rb");
 if(file){
  Stringz data = read_entire_file_handle(arena, file);
  fclose(file);
  Token_List token_list = lex_full_input_cpp(arena, data);
  result.ok        = true;
  result.name      =filename;
  result.data      =data;
  result.token_list=token_list;
 }else{
  printf("error: could not open input file: [%s]\n", to_cstring(filename));
 }
 return result;
}
//~
function String
ep_string_literal(Ed_Parser *parser)
{//NOTE(kv) There are bunch of string kinds, so this is not exhaustive.
 String result = {};
 Token *token = ep_eat_kind(parser, TokenBaseKind_LiteralString);
 if(parser->ok_){
  result = ep_print_token(parser);
  result.str += 1;
  result.len -= 2;
  kv_assert(result.len >= 0);
 }
 return result;
}
function b32
generate_4coder_custom()
{
 struct Meta_Custom_Command
 {
  String name;
  String documentation;
  b32 is_ui;
 };
 struct Meta_Custom_ID
 {
  String name;
  String group;
 };
 
 b32 ok = true;
 Scratch_Block scratch;
 Stringz code_dir = meta_dirs.code;
 Stringz custom_commands_path = pjoin(scratch, code_dir, strlit("custom_command_list.h"));
 Lexed_File source = lex_file(scratch, custom_commands_path);
 
 darray(Meta_Custom_Command) commands;
 init_dynamic(commands, scratch, 512);
 
 darray(Meta_Custom_ID) custom_ids;
 init_dynamic(custom_ids, scratch, 128);
 
 {//-Parse
  Ed_Parser parser_value = ed_parser_from_token_list(source.data, source.token_list);
  Ed_Parser *parser = &parser_value;
  ep_skip_comments_and_spaces(parser);
  
  {
   ep_id(parser, strlit("normal_commands"));
   ep_char(parser, '{');
   while(parser->ok_){
    if(ep_maybe_char(parser, '}')){
     break;
    }
    Meta_Custom_Command *command = commands.push();
    *command = {};
    command->name = ep_id(parser);
    command->documentation = ep_string_literal(parser);
   }
  }
  {
   ep_id(parser, strlit("ui_commands"));
   ep_char(parser, '{');
   while(parser->ok_)
   {
    if(ep_maybe_char(parser, '}')){
     break;
    }
    Meta_Custom_Command *command = commands.push();
    *command = {};
    command->name = ep_id(parser);
    command->documentation = ep_string_literal(parser);
    command->is_ui = true;
   }
  }
  
  {//-custom id
   ep_id(parser, strlit("custom_ids"));
   ep_char(parser, '{');
   while(parser->ok_)
   {
    if(ep_maybe_char(parser, '}')){
     break;
    }
    Meta_Custom_ID *id = custom_ids.push();
    id->name = ep_id(parser);
    id->group = ep_id(parser);
    ep_maybe_char(parser, ',');
   }
  }
  
  ok = ok and parser->ok_;
  if(not parser->ok_){
   Line_Column fail_location = ep_get_fail_location(parser);
   printf("%.*s:%d:%d: parse error\n",
          string_expand(source.name),
          fail_location.line,
          fail_location.column);
  }
 }
 
 char *text;
 if(ok)
 {//-print commands
  Stringz output_path = pjoin(scratch, code_dir,
                              strlit("generated"), strlit("command_metadata.gen.h"));
  Meta_Printer printer = m_open_file_to_write(output_path);
  {
   print_format(printer, "#  define command_one_past_last_id %d\n", commands.count);
   
   for_u32(i, 0, commands.count){
    Meta_Custom_Command *command = commands.items + i;
    print_format(printer, "function void %.*s(App_Cmd *app);\n",
                 strexpand(command->name));
   }
   
   text = R"FOO(
struct Command_Metadata{
  Custom_Command_Function *proc;
  b32 is_ui;
  String name;
};
)FOO";
   print(printer, text);
   
   print_format(printer, "static Command_Metadata fcoder_metacmd_table[%d] = ", commands.count);
   {
    print(printer, "{\n");
    for_u32(i, 0, commands.count)
    {
     Meta_Custom_Command *command = commands.items + i;
     print_format(printer, "{ .proc=%.*s, .is_ui=%d, .name=strlit(\"%.*s\") },\n",
                  strexpand(command->name),
                  command->is_ui,
                  strexpand(command->name));
    }
    print(printer, "};\n");
   }
  }
  ok = ok and not(printer.error);
  close(printer);
 }
 
 {//-print custom ids
  Stringz output_path = pjoin(scratch, code_dir,
                              strlit("generated"), strlit("init_custom_id.gen.cpp"));
  Meta_Printer printer = m_open_file_to_write(output_path);
  
  text = R"FOO(
function void
initialize_managed_id_metadata(App *app)
)FOO";
  print(printer, text);
  print_brace_block(printer){
   print(printer, "\n");
   print(printer,
         "#define X(name, group) "
         "name = managed_id_declare(app, strlit(#group), strlit(#name))\n");
   for_u32(i, 0, custom_ids.count){
    Meta_Custom_ID *id = custom_ids.items + i;
    print_format(printer, "X(%.*s, %.*s);\n",
                 strexpand(id->name), strexpand(id->group));
   }
   print(printer, "#undef X\n");
   print(printer, "\n");
  }
  
  ok = ok and not(printer.error);
  close(printer);
 }
 return ok;
}
//~
extern "C" BOOL CALL_CONVENTION
GetUserProfileDirectoryA(HANDLE  hToken, LPSTR   lpProfileDir, LPDWORD lpcchSize);

function Stringz
get_home_directory_ansi(Arena *arena)
{
 HANDLE current_process_token = GetCurrentProcessToken();
 DWORD size = 256;
 u8 *buffer = push_array(arena, u8, size);
 b32 ok = GetUserProfileDirectoryA(current_process_token, (char*)buffer, &size);
 kv_assert(ok);
 Stringz result = empty_string;
 result.str   = buffer;
 result.count = size-1;
 return result;
}
xfunction i32
main(i32 argc, char **argv)
{
 b32 ok = true;
 Arena *scratch = &thread_permanent_arena;
 
 //NOTE Convert args to our string
 String *args = push_array(scratch, String, argc);
 for_i32(i, 0, argc){
  args[i] = SCu8(argv[i]);
 }
 
 meta_command_name = argv[0];
 Stringz code_dir;
 {//;meta_dirs_init
  String home_dir = get_home_directory_ansi(scratch);
  code_dir = pjoin(scratch, home_dir, strlit("4ed"), strlit("code"));
  meta_dirs.code     = code_dir;
  meta_dirs.code_gen = pjoin(scratch, code_dir, strlit("generated"));
  meta_dirs.game     = pjoin(scratch, code_dir, strlit("game"));
  meta_dirs.game_gen = pjoin(scratch, meta_dirs.game, strlit("generated"));
 }
 
 if(ok)
 {
  {//-API parsing
   Scratch_Block api_scratch;
   char *api_paths0[] = {
    "4ed_api_implementation.cpp",
    "platform_win32/win32_4ed_functions.cpp",
    "4coder_token.cpp",
    "4coder_game_shared.h",
    "4ed_render_target.cpp",
    "ad_debug_interface.h",
   };
   API_Definition_List list = {};
   for_i1(i,0,alen(api_paths0)){
    //-Build the API definition list
    Stringz api_path = pjoin(api_scratch, code_dir, SCu8(api_paths0[i]));
    Lexed_File file = lex_file(api_scratch, api_path);
    api_parser_parse_file(api_scratch, file, &list);
   }
   //-Generate includes
   ok = ok and api_parser_generate(&list);
  }
  
  darray(Stringz) all_paths;
  init_dynamic(all_paths, &thread_permanent_arena, 64);
  ok = ok and list_files_in_dir(scratch, &all_paths, code_dir,
                                No_Recursive, is_klang_or_template_file);
  ok = ok and list_files_in_dir(scratch, &all_paths, meta_dirs.game,
                                No_Recursive, is_klang_or_template_file);
  if(not ok){
   fprintf(stderr, "failed to list files\n");
  }
  
  for_u32(path_index, 0, all_paths.count){
   //-template files
   if(not ok){ break; }
   
   Stringz path = all_paths[path_index];
   if(is_template_file(path)){
    Lexed_File lexed_file = lex_file(scratch, path);
    ok = ok and template_main(lexed_file);
   }
  }
  
  {//-klang
   Scratch_Block klang_arena;
   darray(K_Slider) sliders;
   init_dynamic(sliders, klang_arena, 512);
   darray(String) type_info_list;
   init_dynamic(type_info_list, klang_arena, 64);
   
   for_u32(path_index, 0, all_paths.count){
    //-Parsing all the files
    if(not ok){ break; }
    
    Stringz path = all_paths[path_index];
    if(is_klang_file(path)){
     Lexed_File lexed_file = lex_file(scratch, path);
     ok = ok and klang_main(klang_arena, lexed_file, &sliders, &type_info_list);
    }
   }
   
   {//-Print data that needs to be aggregated
    //TODO(kv) So we're supporting the game only?
    Stringz path = pjoin(scratch, meta_dirs.game_gen, strlit("meta_all.gen.cpp"));
    Printer printer = m_open_file_to_write(path);
    
    print(printer, "function void\n");
    print(printer, "make_all_type_info()\n");
    print_brace_block(printer)
    {
     print(printer, "\n");
     for_u32(i, 0, type_info_list.count){
      String type = type_info_list.items[i];
      print_format(printer, "Type_Info_%S = get_type_info_%S();\n", type, type);
     }
    }
    
    close(printer);
   }
   
   if(ok){
    klang_print_sliders(sliders.items, sliders.count);
   }
  }
 }
 
 {//-System api
  Scratch_Block scratch_api;
  API_Definition *api = make_system_api(scratch_api);
  api_definition_generate_api_includes(api, strlit("4ed_system_api.cpp"),
                                       GeneratedGroup_Custom, APIGeneration_PrefixCallables);
 }
 
 ok = ok and generate_4coder_custom();
 
 ok = ok and build_main(argc, args);
 
 i32 exit_code = !ok;
 fflush(stdout);
 if(!ok){
  breakhere;
 }
 
 return exit_code;
}
//~BOTTOM
