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

#include "meta_main.h"
#include "meta_print.h"
#include "meta_parse.h"
#include "meta_klang.h"
#include "meta_entity.h"

#include "meta_parse.cpp"
#include "4ed_api_definition.cpp"
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
xfunction i32
main(i32 argc, char **argv){
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
 
 i32 exit_code = !ok;
 fflush(stdout);
 if(!ok){
  breakhere;
 }
 return exit_code;
}
//~BOTTOM
