/* 
 * Mr. 4th Dimention - Allen Webster (Modified by kv)
 * Do all the meta programming things
 */
#define KV_H_IS_METAPROGRAM 1
#include "kv.h"

// NOTE(kv) not needed but whatevs
#include "kv_math.h"

#include "4coder_system_types.h"
#include "meta_game_shared.h"
#include "4coder_token.h"
#include "lexer_cpp.gen.h"
#include "4ed_base.h"
#include "4ed_api_definition.h"
#include "4coder_stringf.cpp"
#include "4coder_token.cpp"
#include "lexer_cpp.gen.cpp"
#include "lexer_skm.cpp"
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
#include "meta_links.cpp"
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
 if(0)
 {//-Vertex check (archived)
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
  
  auto stack_push_block = [&](sarray(Meta_Statement) *block)
  {
   Stack_Entry entry = {block->items, block->count, 0};
   push(&stack, entry);
  };
  auto stack_push_statement = [&](Meta_Statement *statement)
  {
   push(&stack, {statement, 1, 0});
  };
  
  stack_push_block(&root.top_levels);
  while(true)
  {
   Meta_Statement *statement = 0;
   {//-Pop the stack
    while(not statement and stack.count > 0)
    {
     Stack_Entry *last = &get_last(stack);
     if(last->next_index < last->statement_count)
     {
      statement = &last->statements[last->next_index++];
     }
     else
     {
      stack.pop();
     }
    }
   }
   if(statement)
   {
    if(statement->kind == Statement_Kind_Function)
    {//-Function
     stack_push_block(&statement->function0.body);
    }
    else if(statement->kind == Statement_Kind_Block)
    {//-Block
     stack_push_block(&statement->block);
    }
    else if(statement->kind == Statement_Kind_If)
    {//-If
     stack_push_statement(statement->if0.else0);
     stack_push_statement(statement->if0.body);
    }
    else
    {//-Leaf
     if(statement->kind == Statement_Kind_Expression)
     {//-Expression
      Meta_Expression &expr = statement->expression;
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
 Stringz code_dir = meta.dirs.code;
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
    Meta_Custom_Command *command = push_zero(&commands);
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
    Meta_Custom_Command *command = push_zero(&commands);
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
    Meta_Custom_ID *id = push_zero(&custom_ids);
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
   printf(printer, "#  define command_one_past_last_id %d\n", commands.count);
   
   for_i32(i, 0, commands.count){
    Meta_Custom_Command *command = commands.items + i;
    printf(printer, "function void %.*s(App_Cmd *app);\n",
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
   
   printf(printer, "static Command_Metadata fcoder_metacmd_table[%d] = ", commands.count);
   {
    print(printer, "{\n");
    for_i32(i, 0, commands.count)
    {
     Meta_Custom_Command *command = commands.items + i;
     printf(printer, "{ .proc=%.*s, .is_ui=%d, .name=strlit(\"%.*s\") },\n",
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
  {
   PrintBraces(printer);
   print(printer, "\n");
   print(printer,
         "#define X(name, group) "
         "name = managed_id_declare(app, strlit(#group), strlit(#name))\n");
   for_i32(i, 0, custom_ids.count){
    Meta_Custom_ID *id = custom_ids.items + i;
    printf(printer, "X(%.*s, %.*s);\n",
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
function void
meta_delete_old_backups()
{
 Scratch_Scope tmp;
 auto compare_oldest_first = [](void *a0, void *b0) -> i32
 {
  i32 result = 0;
  File_Info *a = (File_Info *)a0;
  File_Info *b = (File_Info *)b0;
  u64 atime = a->attributes.last_write_time;
  u64 btime = b->attributes.last_write_time;
  if(atime < btime)
  {// NOTE a is older, meaning that it should come first
   result = -1;
  }
  else if(atime > btime)
  {
   result = 1;
  }
  return result;
 };
 
 i32 const cleanup_threshold = 128;  // note threshold that triggers cleanup
 i32 const cleanup_target = 256;  // note count after cleanup
 
 File_List file_list = get_file_list(tmp, meta.dirs.backup);
 if(file_list.count > cleanup_threshold)
 {
  i32 clean_count = i32(file_list.count) - cleanup_target;
  myprintf("Cleaning up %d files\n", clean_count);
  
  gb_sort_array(file_list.infos, file_list.count, compare_oldest_first);
  for_i32(i, 0, clean_count)
  {
   File_Info *file_info = file_list.infos[i];
   Stringz path = pjoin(tmp, meta.dirs.backup, file_info->filename);
   remove_file(path);
  }
 }
}
function b32
main_normal(String *args, i32 arg_count)
{// NOTE(kv) Normal build, you know the kind where you press Alt-M and it does... things?
 f64 meta_start_time = gb_time_now();
 
 b32 ok = 1;
 Arena *tmp = &thread_permanent_arena;
 init_dynamic(meta_type_name_store, tmp);
 String caller_file = {};
 for_i32(argi, 1, arg_count)
 {
  String arg = args[argi];
  if(arg == "--test-klang")
  {
   meta.testing = true;
   myprintf("[meta] IMPORTANT: Testing mode\n");
   break;
  }
  else if(arg == "--hotload-driver")
  {
   meta.hotload_driver = 1;
   myprintf("[meta] IMPORTANT: Hotload driver\n");
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
 
 Stringz code_dir;
 {//;meta_dirs_init
  Stringz home_dir = get_home_directory_ansi(tmp);
  // @HardCoded
  code_dir = pjoin(tmp, home_dir, strlit("4ed"), strlit("code"));
  meta.dirs.home   = home_dir;
  meta.dirs.code   = code_dir;
  meta.dirs.game   = pjoin(tmp, code_dir, "game");
  meta.dirs.driver = pjoin(tmp, meta.dirs.game, "driver");
  meta.dirs.backup = pjoin(tmp, code_dir, "backups");
  
  b32 ok2 = mkdir_p(meta.dirs.backup);
  kv_assert(ok2);
 }
 
 meta_delete_old_backups();
 
 sarray(Lexed_File) all_files = {};
 {//-IMPORTANT Lex ALL THE FILES
  darray(Stringz) all_paths;
  init_dynamic(all_paths, tmp, 512);
  
  // TODO(kv) Hm, how about nested dir though?
  Stringz directories_whose_files_are_all_processed[] = {
   meta.dirs.game,
   meta.dirs.driver,
   pjoin(tmp, meta.dirs.code, "meta"),  // NOTE(kv) Nothing stops the metaprogram using links.
  };
  
  Stringz black_listed_paths[] = {
   pjoin(tmp, code_dir, strlit("libs")),
   pjoin(tmp, code_dir, strlit("ship_files")),
  };
  
  auto is_code_file = [](String path) -> b32
  {
   String extension = path_extension(path);
   b32 result = is_klang_file(path);
   if(not meta.hotload_driver)
   {
    result = result or (extension == "h" or
                        extension == "cpp" or
                        extension == "skm");
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
    if(dirname.count > 0 and 
       is_file_slash(dirname[dirname.count-1]))
    {// #hack
     dirname.count--;
    }
    
    b32 processed = 0;
    b32 is_test_file = path_filename(path) == "test.kc";
    if(meta.testing)
    {
     processed = is_test_file;
    }
    else if(is_test_file)
    {
     processed = 0;
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
#if 0
  for_i32(path_index, 0, all_paths.count){ myprintf("path: %S\n", all_paths[path_index]); }
#endif
  
  {//-Lex
   init_static(all_files, tmp, all_paths.count);
   
   f64 start_time = gb_time_now();
   for_i32(i, 0, all_paths.count)
   {
    all_files[i] = lex_file(tmp, all_paths[i]);
   }
   
   myprintf("Total lex time: %lf\n", gb_time_now() - start_time);
  }
 }
 
 if(ok and not meta.hotload_driver)
 {//-;link_processor_call
  Scratch_Block links_tmp;
  sarray(Link_Table_Node *) table;
  init_static(table, links_tmp, 4096);
  
  for_i32(file_index, 0, all_files.count)
  {
   if(not ok) break;
   Lexed_File file = all_files[file_index];
   ok = ok and collect_links_and_notes(links_tmp, table, file);
  }
  
  if(ok)
  {
   i32 broken_link_count = 0;
   i32 total_link_count = 0;
   myprintf("Broken links: \n");
   
   for_i32(entry_index, 0, table.count)
   {
    for(Link_Table_Node *node = table[entry_index];
        node;
        node = node->next_in_hash)
    {
     if(not node->is_note)
     {
      broken_link_count++;
      String filepath = node->example_location.filepath;
      i64 position = node->example_location.position;
      // @kv_jump_syntax
      myprintf("[kv][%S][%lld] %S\n", filepath, position, node->value);
     }
     total_link_count++;
    }
   }
   myprintf("\n");
   
   if(broken_link_count > 0)
   {
    myprintf("Broken link count: %d\n", broken_link_count);
    myprintf("Total link/note count: %d\n", total_link_count);
    ok = 0;
   }
  }
 }
 
 if(ok)
 {//-;api_parsing
  Scratch_Block api_tmp;
  char *api_paths0[] = {
   "4ed_api_implementation.cpp",
   "platform_win32/win32_4ed_functions.cpp",
   "4coder_token.cpp",
   "4coder_game_shared.h",
   "4ed_render_target.cpp",
   "ad_debug_interface.h",
   "4ed_imgui_wrapper.cpp",
   "4coder_game.cpp",
  };
  API_Definition_List list = {};
  for_i32(i,0,alen(api_paths0))
  {//-Build the API definition list
   String filename = SCu8(api_paths0[i]);
   Stringz api_path = pjoin(api_tmp, code_dir, filename);
   Lexed_File file = lex_file(api_tmp, api_path);
   kv_assert(file.data.count > 0);  // todo #hack
   api_parser_parse_file(api_tmp, file, &list);
  }
  //-Generate includes
  ok = ok and api_parser_generate(&list);
 }
 
 ok = ok and klang_main(all_files);
 
 if(not meta.hotload_driver)
 {//-System api
  Scratch_Block scratch_api;
  API_Definition *api = make_system_api(scratch_api);
  api_definition_generate_api_includes(api, strlit("4ed_system_api.cpp"),
                                       GeneratedGroup_Custom, APIGeneration_PrefixCallables);
 }
 
 if(not meta.hotload_driver)
 {
  ok = ok and generate_4coder_custom();
 }
 
 myprintf("Total meta time: %f\n", gb_time_now() - meta_start_time);
 fflush(stdout);
 
 if(not meta.testing)
 {
  ok = ok and build_main(arg_count, args);
 }
 
 return ok;
}

function b32
filter_narration(Stringz input_path)
{
 Scratch_Block tmp;
 
 // NOTE Parser
 Lexed_File lexed_file = lex_file(tmp, input_path);
 Ed_Parser parser_ = ed_parser_from_lexed_file(lexed_file);
 Ed_Parser *parser = &parser_;
 
 // NOTE Printer
 Stringz output_path = pjoin(tmp, path_dir(input_path),
                             strcat(tmp, "out_", path_filename(input_path)));
 myprintf("Writing to file %S\n", output_path);
 FILE *out_file = open_or_create_file(output_path, "wb");
 kv_assert(out_file);
 Printer printer = make_printer_file(out_file);
 
 b32 parsing = 1;
 i32 ignore_nest = 0;
 Scratch_Block tmp_loop;
 while(parsing)
 {
  arena_clear(tmp_loop);
  Token *token0 = ep_get_token(parser);
  String token0_string = ep_print_token(parser, token0);
  if(token0->kind == TokenBaseKind_EOF)
  {
   parsing = 0;
  }
  // NOTE(kv) We can't escape brackets, but probably won't matter.
  else if(token0_string == '[')
  {
   if(ignore_nest > 0){ ignore_nest++; }
  }
  else if(token0_string == ']')
  {
   if(ignore_nest > 0){ ignore_nest--; }
  }
  else if(starts_with(token0_string, '@'))
  {// NOTE(kv) All tags are eliminated.
   // Also we have to put tags in separate tokens,
   // by having it be next to an open bracket.
   ignore_nest = 1;
  }
  else if(ignore_nest == 0)
  {// NOTE Normal text
   // NOTE(kv) Strip newlines in the middle
   Stringz stringz = push_string(tmp_loop, token0_string);
   u8 *buffer = push_size(tmp_loop, token0_string.count);
   u8 *source = stringz.str;
   u8 *dest = buffer;
   while(source < stringz.str + stringz.count)
   {
    if(*source == '\r' or *source == '\n')
    {// NOTE(kv) newlines
     u8 *source_0 = source;
     while(*source == '\r' or *source == '\n' or
           *source == ' ')
     {
      source++;
     }
     
     if(*source == 0)
     {// NOTE Whitespaces at the end
      // We will copy all the newlines.
      while(*source_0)
      {
       if(*source_0 == '\n')
       {
        *dest++ = '\n';
       }
       source_0++;
      }
     }
     else
     {// NOTE Whitespaces connecting in a token
      *dest++ = ' ';
     }
    }
    else
    {
     *dest++ = *source++;
    }
   }
   
   print(printer, String{.str=buffer, .count=u64(dest-buffer)});
  }
  
  if(parsing)
  {
   ep_eat_inc_all(parser);
  }
 }
 
 kv_assert(not printer.error);
 
 close_file(out_file);
 
 return 1;
}

xfunction i32
main(i32 arg_count, char **argv)
{
 Arena *tmp = &thread_permanent_arena;
 tmp->default_chunk_size = MB(64);
 
 Stringz *args = push_array(tmp, Stringz, arg_count);
 meta_command_name = args[0];
 for_i32(i, 0, arg_count){ args[i] = SCu8(argv[i]); }
 
 b32 ok = 1;
 if(arg_count == 3 and
    args[1] == "filter_narration")
 {
  ok = filter_narration(args[2]);
 }
 else
 {
  ok = main_normal(args, arg_count);
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
