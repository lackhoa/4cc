/* 
 * Mr. 4th Dimention - Allen Webster (Modified by kv)
 * Do all the meta programming things
 */
#define KV_H_IS_METAPROGRAM 1
#include "kv.h"

#include "4coder_token.h"
#include "generated/lexer_cpp.h"
#include "4ed_api_definition.h"

#include "4coder_stringf.cpp"
#include "4coder_malloc_allocator.cpp"
#include "4coder_token.cpp"
#include "generated/lexer_cpp.cpp"
#include "4ed_kv_parser.cpp"

#include "meta_print.h"
#include "meta_parse.h"
#include "meta_klang.h"
#include "meta_entity.h"
#include "meta_main.h"

#include "4ed_api_definition.cpp"
#include "meta_os.cpp"
#include "meta_parse.cpp"
#include "4ed_system_api.cpp"
#include "4ed_api_parser.cpp"
#include "meta_print.cpp"
#include "meta_klang.cpp"
#include "meta_entity.cpp"
#include "meta_template.cpp"
//-
typedef b32 String_Predicate(String path);

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
struct List_File_Params{
 b32 recursive;
 String_Predicate *predicate;
};
function b32
list_files_in_dir(Arena *arena, arrayof<Stringz> &outfiles, char *path,
                  List_File_Params params)
{
 b32 ok = true;
 WIN32_FIND_DATA fdFile;
 char buffer[2048];
 sprintf(buffer, "%s\\*.*", path);
 
 HANDLE hFind = FindFirstFile(buffer, &fdFile);
 if(hFind == INVALID_HANDLE_VALUE){
  printf("error: path not found '%s'\n", path);
  ok = false;
 }
 
 if(ok){
  do{
   if(strcmp(fdFile.cFileName, ".")  != 0 &&
      strcmp(fdFile.cFileName, "..") != 0) {
    sprintf(buffer, "%s\\%s", path, fdFile.cFileName);
    
    if(fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
     //NOTE(kv) this is a directory
     if(params.recursive){
      ok = list_files_in_dir(arena, outfiles, buffer, params);
     }
    }else{
     //NOTE(kv) This is a file
     Stringz file_path = SCu8(buffer);
     b32 satisfies_predicate = true;
     if(params.predicate){
      satisfies_predicate = params.predicate(file_path);
     }
     if(satisfies_predicate){
      char *copy = push_array_copy(arena, char, file_path.len+1, buffer);
      outfiles.push_value(Stringz{(u8 *)copy, file_path.len});
     }
    }
   }
  }while(ok && FindNextFile(hFind, &fdFile));
  
  FindClose(hFind);
 }
 
 return ok;
}
//-
function void
meta_process_ast(Statement_Root *root){
 kv_assert(root->kind == Statement_Kind_Root);
 if(DEBUG_vv_name){
  //-Vertex check
  Scratch_Block scratch;
  arrayof<String> existing_names;
  init_dynamic(existing_names, scratch, 200);
  
  struct Stack_Entry{
   Statement_Union *statements;  //TODO(kv) We could store Statement_Head here, but then only for one pointer
   i32 statement_count;
   i32 next_index;
  };
  arrayof<Stack_Entry> stack;
  init_dynamic(stack, scratch, 32);
  
  auto stack_push_block = [&](Meta_Statements *block){
   stack.push_value({block->items, block->count, 0});
  };
  auto stack_push_statement = [&](Statement_Head *statement){
   stack.push_value({cast(Statement_Union *)statement, 1, 0});
  };
  
  stack_push_block(&root->top_levels);
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
      if(expr.kind == Expression_Kind_Function_Call){
       Expression_Function_Call &call = expr.function_call;
       if(call.function_name == strlit("vv") or
          call.function_name == strlit("va")){
        //-Is vertex
        String vertex_name = call.arguments[0].identifier;
        {
         Statement_Head *test = statement->mom;
         while(test){
          //-"if" check
          if(test->kind == Statement_Kind_If){
           printf("[kv]%.*s[%d] error: vertex used within if block\n",
                  strexpand(root->source_path),
                  statement->pos);
           break;
          }
          test = test->mom;
         }
        }
        {//-conflict check
         b32 conflict = false;
         for_i32(existing_name_index,0,existing_names.count){
          if(vertex_name == existing_names[existing_name_index]){
           //-conflict
           printf("[kv]%.*s[%d]: error: conflicting name found: %.*s\n",
                  strexpand(root->source_path),
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
  printf("Total vertex count: %d\n", existing_names.count);
 }
}
function void
test_read_map_file(Stringz path){
 Scratch_Block scratch;
 Stringz data = read_entire_file(scratch, path);
 u8 *pointer = data.str;
 Printer p = make_printer_file(stdout);
 {
  p < "Magic value: " < String{(u8 *)pointer, 4} < "\n";
  pointer += 4;
 }
 i32 count = *(i32 *)pointer;
 {
  p < "count: " < count < "\n";
  pointer += 4;
 }
 {
  Source_Map_Entry *entry = (Source_Map_Entry *)pointer;
  for_i32(index,0,count){
   p < entry->source_pos < " -> " < entry->gen_pos < "\n";
   entry++;
  }
 }
}
function Meta_Parsed_File
lex_file(Arena *arena, Stringz filename)
{
 Meta_Parsed_File result = {};
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
xfunction i32
main(i32 argc, char **argv)
{
 b32 ok = true;
 Arena *scratch = &meta_permanent_arena;
 command_name = argv[0];
 char *code_dir = "";
 if(argc < 2){
  printf("Usage: %s <code_dir>\n", command_name);
  ok = false;
 }else{
  code_dir = argv[1];
 }
 
 {
  //;meta_dirs_init
  meta_dirs.code     = SCu8(code_dir);
  meta_dirs.game     = pjoin(scratch, meta_dirs.code, "game");
  meta_dirs.game_gen = pjoin(scratch, meta_dirs.game, "generated");
 }
 
 if(ok)
 {
  {//-API parsing
   Scratch_Block api_scratch;
   char *api_paths0[] = {
    "4ed_api_implementation.cpp",
    "platform_win32/win32_4ed_functions.cpp",
    "custom/4coder_token.cpp",
    "4coder_game_shared.h",
    "4ed_render_target.cpp",
    "ad_debug_interface.h",
   };
   API_Definition_List list = {};
   for_i1(i,0,alen(api_paths0)){
    //-Build the API definition list
    Stringz api_path = pjoin(api_scratch, code_dir, api_paths0[i]);
    Meta_Parsed_File file = lex_file(api_scratch, api_path);
    api_parser_parse_file(api_scratch, file, &list);
   }
   //-Generate includes
   ok = ok and api_parser_generate(&list);
  }
  
  {//-My languages
   arrayof<Stringz> all_paths = {};
   init_dynamic(all_paths, &malloc_base_allocator, 64);
   List_File_Params params = {.predicate=is_klang_or_template_file};
   ok = ok and list_files_in_dir(scratch, all_paths, code_dir, params);
   ok = ok and list_files_in_dir(scratch, all_paths, to_cstring(meta_dirs.game), params);
   if(not ok){
    fprintf(stderr, "failed to list files\n");
   }
   
   for_i32(path_index, 0, all_paths.count){
    //-klang
    if(not ok){ break; }
    
    Stringz path = all_paths[path_index];
    if(is_klang_file(path)){
     Meta_Parsed_File parsed_file = lex_file(scratch, path);
     ok = ok and klang_main(parsed_file);
    }
   }
   
   for_i32(path_index, 0, all_paths.count){
    //-template file
    if(not ok){ break; }
    
    Stringz path = all_paths[path_index];
    if(is_template_file(path)){
     Meta_Parsed_File parsed_file = lex_file(scratch, path);
     ok = ok and template_main(parsed_file);
    }
   }
  }
 }
 
 {//-System api
  Scratch_Block scratch_api;
  API_Definition *api = make_system_api(scratch_api);
  api_definition_generate_api_includes(api, strlit("4ed_system_api.cpp"), GeneratedGroup_Custom, 0);
 }
 
 i32 exit_code = !ok;
 fflush(stdout);
 if(!ok){
  breakhere;
 }
 if(0){
  test_read_map_file(strlit("C:/Users/vodan/4ed/code/game/generated/driver.kc.map"));
 }
 return exit_code;
}
//~BOTTOM
