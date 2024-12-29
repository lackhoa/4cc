/* 
 * Mr. 4th Dimention - Allen Webster (Modified by kv)
 * Do all the meta programming things
 */
#define KV_H_IS_METAPROGRAM 1
#include "kv.h"

// NOTE(kv) not needed but whatevs
#include "kv_math.h"

#include "meta_game_shared.h"
#include "4coder_token.h"
#include "lexer_cpp.gen.h"
#include "4ed_base.h"
#include "4ed_api_definition.h"
#include "4coder_stringf.cpp"
#include "4coder_malloc_allocator.cpp"
#include "4coder_token.cpp"
#include "lexer_cpp.gen.cpp"
#include "4ed_kv_parser.h"
#include "4ed_kv_parser.cpp"

#include "meta_print.h"
#include "meta_parse.h"
#include "meta_klang.h"
#include "meta_entity.h"
#include "meta_main.h"
#include "meta_template.h"
#include "meta_build.h"

#include "4ed_api_definition.cpp"
#include "meta_os.cpp"
#include "meta_parse.cpp"
#include "4ed_system_api.cpp"
#include "4ed_api_parser.cpp"
#include "meta_print.cpp"
#include "meta_template.cpp"
#include "meta_klang.cpp"
#include "meta_entity.cpp"
#include "meta_build.cpp"
//-
myinline b32
is_template_file(String path)
{
 String extension = path_extension(path);
 return (extension == strlit("kt"));
}
myinline b32
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
 if(DEBUG_vv_name)
 {//-Vertex check
  Scratch_Block scratch;
  darray(String) existing_names;
  init_dynamic(existing_names, scratch, 200);
  
  struct Stack_Entry
  {
   Meta_Statement *statements;  // TODO(kv) We could store Statement_Head here, but then only for one pointer
   i32 statement_count;
   i32 next_index;
  };
  darray(Stack_Entry) stack;
  init_dynamic(stack, scratch, 32);
  
  auto stack_push_block = [&](sarray(Meta_Statement) *block){
   Stack_Entry entry = {block->items, block->count, 0};
   push(&stack, entry);
  };
  auto stack_push_statement = [&](Statement_Head *statement){
   push(&stack, {cast(Meta_Statement *)statement, 1, 0});
  };
  
  stack_push_block(&root.top_levels);
  while(true)
  {
   Statement_Head *statement = 0;
   {//-Pop the stack
    while(not statement and stack.count > 0){
     Stack_Entry *last = &get_last(stack);
     if(last->next_index < last->statement_count){
      statement = &last->statements[last->next_index++].head;
     }else{
      stack.pop();
     }
    }
   }
   if(statement)
   {
    if(statement->kind == Statement_Kind_Function)
    {//-Function
     cast_to_var(Statement_Function*, func, statement);
     stack_push_block(&func->body);
    }
    else if(statement->kind == Statement_Kind_Block)
    {//-Block
     cast_to_var(Statement_Block *, block, statement);
     stack_push_block(&block->block);
    }
    else if(statement->kind == Statement_Kind_If)
    {//-If
     cast_to_var(Statement_If *, if0, statement);
     stack_push_statement(if0->else0);
     stack_push_statement(if0->body);
    }
    else
    {//-Leaf
     if(statement->kind == Statement_Kind_Expression)
     {//-Expression
      cast_to_var(Statement_Expression*, statement_expression, statement);
      Meta_Expression &expr = statement_expression->expression;
      if(expr.kind == Expression_Kind_Call)
      {
       Expression_Call *call = &expr.call;
       String function_name = get_function_name(call);
       if(function_name == strlit("vv") or
          function_name == strlit("va") or
          function_name == strlit("vv_sample") or
          function_name == strlit("send_vert"))
       {//-Is vertex
       }
      }
     }
    }
   }else{
    break;
   }
  }
  printf("Total vertex count: %d\n", existing_names.count);
 }
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
 Stringz custom_data_path = pjoin(scratch, code_dir, strlit("custom_command_list.h"));
 Lexed_File source = lex_file(scratch, custom_data_path);
 
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
  if(not parser->ok_)
  {
   Line_Column fail_location = ep_get_fail_location(parser);
   myprintf("%S:%d:%d: parse error\n",
            source.path,
            fail_location.line,
            fail_location.column);
  }
 }
 
 char *text;
 if(ok)
 {//-print commands
  Stringz output_path = pjoin(scratch, code_dir, strlit("command_metadata.gen.h"));
  Meta_Printer printer = m_open_file_to_write(output_path, custom_data_path);
  {
   print_format(printer, "#  define command_one_past_last_id %d\n", commands.count);
   
   for_i32(i, 0, commands.count){
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
    for_i32(i, 0, commands.count)
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
  Stringz output_path = pjoin(scratch, code_dir, strlit("init_custom_id.gen.cpp"));
  Meta_Printer printer = m_open_file_to_write(output_path, custom_data_path);
  
  text = R"FOO(
function void
initialize_managed_id_metadata(App *app)
)FOO";
  print(printer, text);
  PrintBraces(printer){
   print(printer, "\n");
   print(printer,
         "#define X(name, group) "
         "name = managed_id_declare(app, strlit(#group), strlit(#name))\n");
   for_i32(i, 0, custom_ids.count){
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
function b32
array_contains(sarray(String) array, String item)
{
 for_i32(i, 0, array.count)
 {
  if(array[i] == item)
  {
   return true;
  }
 }
 return false;
}

typedef String Text_Link;
typedef String Text_Note;

struct Link_Processor_State
{
 darray(Text_Link) links;
 darray(Text_Note) notes;
};

function void
collect_all_links(Link_Processor_State *state, Lexed_File file)
{
 Ed_Parser parser_value = ed_parser_from_token_list(file.data, file.token_list);
 Ed_Parser *parser = &parser_value;
 while(parser->ok_)
 {
  Token *token = ep_get_token(parser);
  if(token->kind == TokenBaseKind_Comment)
  {
   String token_string = ep_print_token(parser, token);
   for(u8 *character = &token_string[0];
       character < token_string.str + token_string.count;
       )
   {
    u8 c0 = *character++;
    if(c0 == '@')
    {
     u8 *link_begin = character;
     while(character_is_alnum(*character)){ character++; }
     u8 *link_end = character;
     Text_Link new_link = {.str=link_begin, .count=u64(link_end-link_begin)};
     push(&state->links, new_link);
    }
   }
  }
  ep_eat_inc_all(parser);
 }
}
xfunction i32
main(i32 arg_count, char **argv)
{
 f64 meta_start_time = gb_time_now();
 if(0)
 {// todo asan test!
  char buffer[128];
  stbsp_sprintf(buffer, "%s", "asan test!");
 }
 
 b32 ok = true;
 Arena *tmp = &thread_permanent_arena;
 tmp->default_chunk_size = MB(64);
 
 // NOTE Convert args to our string
 String *args = push_array(tmp, String, arg_count);
 for_i32(i, 0, arg_count)
 {
  args[i] = SCu8(argv[i]);
 }
 
 b32 test_klang = false;
 String caller_file = {};
 for_i32(argi, 1, arg_count)
 {
  String arg = args[argi];
  if(arg == "--test-klang")
  {
   test_klang = true;
   break;
  }
  else if(arg == "--file")
  {
   argi++;
   if(argi < arg_count)
   {
    caller_file = args[argi];
   }
  }
 }
 
 if(path_filename(caller_file) == strlit("driver.kc"))
 {
  hotload_driver = true;
 }
 
 meta_command_name = argv[0];
 Stringz code_dir;
 {//;meta_dirs_init
  Stringz home_dir = get_home_directory_ansi(tmp);
  code_dir = pjoin(tmp, home_dir, strlit("4ed"), strlit("code"));
  meta_dirs.home = home_dir;
  meta_dirs.code = code_dir;
  meta_dirs.game = pjoin(tmp, code_dir, strlit("game"));
 }
 
 sarray(Lexed_File) all_files = {};
 {//-IMPORTANT Lex ALL THE FILES
  darray(Stringz) all_paths;
  init_dynamic(all_paths, tmp, 512);
  
  Stringz directories_whose_files_are_all_processed[] = {
   meta_dirs.game,
   pjoin(tmp, meta_dirs.code, "meta"),  // NOTE(kv) Nothing stops the metaprogram from having references
  };
  Stringz black_listed_paths[] = {
   pjoin(tmp, code_dir, strlit("libs")),
   pjoin(tmp, code_dir, strlit("ship_files")),
  };
  
  auto is_code_file = [](String path) -> b32
  {
   String extension = path_extension(path);
   b32 result = is_klang_file(path);
   if(not hotload_driver)
   {
    result = result or (extension == "h" or
                        extension == "cpp" or
                        extension == "4coder");
   }
   return result;
  };
  
  ok = ok and list_files_in_dir(tmp, &all_paths, code_dir, Yes_Recursive,
                                is_code_file,
                                { ArrayAndCount(black_listed_paths) });
  
  {//-Picking which files to process
   Scratch_Block tmp2;
   for(int i=0;
       i < all_paths.count;
       )
   {
    arena_clear(tmp2);
    Stringz path = all_paths[i];
    String dirname = path_dir(path);
    
    b32 processed = false;
    if(test_klang)
    {
     processed = path_filename(path) == "test.kc";
    }
    else
    {
     processed = not is_cpp_file(path);
     processed = processed or
      array_contains({ ArrayAndCount(directories_whose_files_are_all_processed) },
                     dirname);
     
     if(not processed)
     {// NOTE Look for process marker
      usize preview_size = 32;
      String preview = read_file(tmp2, path, preview_size);
      processed = string_has_substr(preview, strlit("#processed"));
     }
    }
    
    if(processed)
    {// NOTE Lex this file
     i++;
    }
    else
    {// NOTE Skip
     all_paths[i] = all_paths[all_paths.count-1];
     all_paths.count--;
    }
   }
  }
  
  myprintf("Lexed file count: %d\n", all_paths.count);
  
  {//-Lex
   init(all_files, tmp, all_paths.count);
   
   f64 start_time = gb_time_now();
   for_i32(i, 0, all_paths.count)
   {
    all_files[i] = lex_file(tmp, all_paths[i]);
   }
   
   myprintf("Total lex time: %lf\n", gb_time_now() - start_time);
  }
 }
 
 if(ok)
 {//-Processing links
  Scratch_Block links_tmp;
  Link_Processor_State state_value = {};
  Link_Processor_State *state = &state_value;
  init_dynamic(state->links, links_tmp, 256);
  
  for_i32(file_index, 0, all_files.count)
  {
   Lexed_File file = all_files[file_index];
   collect_all_links(state, file);
  }
  
  /* bookmark collecting the links
   for_i32(link_index, 0, state->links.count)
    {
     Text_Link link = state->links[link_index];
     myprintf("link: %S\n", String(link));
    }*/
 }
 
 if(ok)
 {//-API parsing
  Scratch_Block api_tmp;
  char *api_paths0[] = {
   "4ed_api_implementation.cpp",
   "platform_win32/win32_4ed_functions.cpp",
   "4coder_token.cpp",
   "4coder_game_shared.h",
   "4ed_render_target.cpp",
   "ad_debug_interface.h",
  };
  API_Definition_List list = {};
  for_i1(i,0,alen(api_paths0))
  {//-Build the API definition list
   Stringz api_path = pjoin(api_tmp, code_dir, SCu8(api_paths0[i]));
   Lexed_File file = lex_file(api_tmp, api_path);
   api_parser_parse_file(api_tmp, file, &list);
  }
  //-Generate includes
  ok = ok and api_parser_generate(&list);
 }
 
 if(not ok)
 {
  fprintf(stderr, "failed to list files\n");
 }
 
 if(0)
 {
  if(not hotload_driver)
  {
   for_i32(file_index, 0, all_files.count)
   {//-Template files
    if(not ok){ break; }
    
    if(is_template_file(all_files[file_index].path))
    {
     ok = ok and xx_template_main(all_files[file_index]);
    }
   }
  }
 }
 
 ok = ok and klang_main(all_files, test_klang);
 
 if(not hotload_driver)
 {//-System api
  Scratch_Block scratch_api;
  API_Definition *api = make_system_api(scratch_api);
  api_definition_generate_api_includes(api, strlit("4ed_system_api.cpp"),
                                       GeneratedGroup_Custom, APIGeneration_PrefixCallables);
 }
 
 if(not hotload_driver)
 {
  ok = ok and generate_4coder_custom();
 }
 
 myprintf("Total meta time: %f\n", gb_time_now() - meta_start_time);
 fflush(stdout);
 
 if(not test_klang)
 {
  ok = ok and build_main(arg_count, args);
 }
 
 //-
 fflush(stdout);
 i32 exit_code = !ok;
 if(exit_code != 0)
 {
  breakhere;
 }
 return exit_code;
}
//~BOTTOM
