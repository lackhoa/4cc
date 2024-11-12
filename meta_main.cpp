/* 
 * Mr. 4th Dimention - Allen Webster (Modified by kv)
 * Do all the meta programming things
 */
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
//-
function b32
list_files_recursive(Arena *arena, arrayof<char*> &outfiles, char *path){
 b32 ok = true;
 WIN32_FIND_DATA fdFile;
 char sPath[2048];
 sprintf(sPath, "%s\\*.*", path);
 
 HANDLE hFind = FindFirstFile(sPath, &fdFile);
 if(hFind == INVALID_HANDLE_VALUE){
  printf("error: path not found '%s'\n", path);
  ok = false;
 }
 
 if(ok){
  do{
   if(strcmp(fdFile.cFileName, ".")  != 0 &&
      strcmp(fdFile.cFileName, "..") != 0) {
    sprintf(sPath, "%s\\%s", path, fdFile.cFileName);
    
    if(fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
     //NOTE(kv) this is a directory
     ok = list_files_recursive(arena, outfiles, sPath);
    }else{
     //NOTE(kv) This is a file
     char *copy = push_array_copy(arena, char, strlen(sPath)+1, sPath);
     outfiles.push(copy);
    }
   }
  }while(ok && FindNextFile(hFind, &fdFile));
  
  FindClose(hFind);
 }
 
 return ok;
}
#if 0
function void
push_fill_variant(arrayof<Union_Variant> &variants, Union_Variant v,
                  String struct_members){
 Arena *arena = &meta_permanent_arena;
 Scratch_Block scratch;
 Ed_Parser parser = m_parser_from_string(scratch, struct_members);
 v.struct_members = parse_struct_body(arena, &parser);
 v.enum_name      = strcat(arena, "Fill_Type_", v.name);
 v.struct_name    = strcat(arena, "Fill_",      v.name);
 variants.push(v);
}
function void
generate_fill_types(Printer &p){
 //TODO(kv) important copy pasta!
 Scratch_Block scratch;
 arrayof<Union_Variant> variants = {};
 //-NOTE @data of the variants
#define X(penum_val, pname, pname_lower, pstruct_members) \
push_fill_variant(variants, \
Union_Variant{ \
.enum_value=penum_val, \
.name=strlit(#pname), \
.name_lower=strlit(#pname_lower) }, \
strlit(#pstruct_members))
 
 X(1, Fill3,    fill3,    { Vertex_Index verts[3]; });
 X(2, Bez,      bez,      { Curve_Index curve; });
 X(3, DBez,     dbez,     { Curve_Index curve1; Curve_Index curve2; });
#undef X
 
 String enum_type = strlit("Fill_Type");
 {//-NOTE ("Enum")
  //TODO(kv) array copy mega-annoyance!
  auto enum_names = static_array<String>(scratch, variants.count);
  enum_names.set_count(variants.count);
  for_i32(i,0,variants.count){ enum_names[i] = variants[i].enum_name; }
  
  auto enum_values = static_array<i32>(scratch, variants.count);
  enum_values.set_count(variants.count);
  for_i32(i,0,variants.count){ enum_values[i] = variants[i].enum_value; }
  
  print_enum(p, enum_type, enum_names, enum_values);
  print_enum_meta(p, enum_type, enum_names);
 }
 {//-NOTE Data structure associated with each variant
  for_i32(i,0,variants.count){
   auto *variant = &variants.get(i);
   m_locationp(p);
   print_struct(p, variant->struct_name, variant->struct_members);
   print_struct_meta(p, variant->struct_name, variant->struct_members);
  }
 }
 {//-NOTE ("Union of all the Bezier type")
  String type_name = strlit("Fill_Union");
  {
   m_location;
   {//NOTE Code
    p<<"union "<<type_name;
    m_braces_sm{
     for_i32(i,0,variants.count){
      if(i!=0){ p<<"\n"; }
      auto &variant = variants.get(i);
      p<<variant.struct_name<<" "<<variant.name_lower<<";";
     }
    }
   }
  }
  {//NOTE Meta
   print_union_meta(p, type_name, &variants, enum_type);
  }
 }
}
#endif
//-
function void
meta_process_ast(Statement_Root *root){
 kv_assert(root->kind == Statement_Kind_Root);
 if(DEBUG_vv_name){
  //-Vertex check
  Scratch_Block scratch(get_thread_context(), 0);
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
   stack.push({block->items, block->count, 0});
  };
  auto stack_push_statement = [&](Statement_Head *statement){
   stack.push({cast(Statement_Union *)statement, 1, 0});
  };
  stack_push_block(&root->top_levels);
  while(true){
   Statement_Head *statement = 0;
   {//-Pop the stack
    while(not statement and stack.count > 0){
     Stack_Entry &last = stack.last();
     if(last.next_index < last.statement_count){
      statement = &last.statements[last.next_index++].head;
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
          existing_names.push(vertex_name);
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
 Scratch_Block scratch(get_thread_context(), 0);
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
xfunction i32
main(i32 argc, char **argv){
 b32 ok = true;
 thread_context_init(&global_thread_context, ThreadKind_Main,
                     &malloc_base_allocator, 0);
 Arena *scratch = &meta_permanent_arena;
 command_name = argv[0];
 char *code_dir = "";
 if(argc < 2){
  printf("Usage: %s <code_dir>\n", command_name);
  ok = false;
 }else{
  code_dir = argv[1];
 }
 //;meta_dirs_init
 meta_dirs.code     = SCu8(code_dir);
 meta_dirs.game     = pjoin(scratch, meta_dirs.code, "game");
 meta_dirs.game_gen = pjoin(scratch, meta_dirs.game, "generated");
 
 char *cpaths[] = {
  "game",
  "4ed_api_implementation.cpp",
  "platform_win32/win32_4ed_functions.cpp",
  "custom/4coder_token.cpp",
  "4coder_game_shared.h",
  "4ed_render_target.cpp",
 };
 Stringz paths[alen(cpaths)];
 for_i1(i,0,alen(paths)){
  //NOTE(kv) Expand from relative path to full path, based on the given code dir
  paths[i] = pjoin(scratch, code_dir, cpaths[i]);
 }
 
 arrayof<Meta_Parsed_File> source_files = {};
 if(ok){
  //-Reading input files
  for_i1(ipath,0,alen(paths)){
   Stringz path = paths[ipath];
   arrayof<char *> filenames = {};
   if(path_is_directory(path)){
    ok = list_files_recursive(scratch, filenames, to_cstring(path));
    if(!ok){
     printf("error: could not list files under path: [%s]\n", to_cstring(path));
    }
   }else{
    filenames.push(to_cstring(path));
   }
   
   if(ok){
    for_i32(ifile, 0, filenames.count){
     char *filename = filenames[ifile];
     FILE *file = fopen(filename, "rb");
     if(file){
      Stringz data = read_entire_file_handle(scratch, file);
      fclose(file);
      Token_List token_list = lex_full_input_cpp(scratch, data);
      source_files.push(Meta_Parsed_File{
                         .name=push_string(scratch, filename),
                         .data=data,
                         .token_list=token_list,
                        });
     }else{
      printf("error: could not open input file: [%s]\n", filename);
      ok = false;
      break;
     }
    }
   }
  }
 }
 
 //TODO(kv) Maybe we don't wanna do these passes separately?
 ok = ok && api_parser_main(source_files);
 ok = ok && klang_main(source_files);
 {//-System api
  Scratch_Block scratch_api(get_thread_context(), scratch);
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
