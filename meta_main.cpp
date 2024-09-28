/*
 * Mr. 4th Dimention - Allen Webster
 * (Modified by kv)
 *
 * Do all the meta programming things
 *
 */

// TOP

#include "kv.h"

#include "4coder_token.h"
#include "generated/lexer_cpp.h"
#include "4ed_api_definition.h"
#include "meta.h"
#include "meta_print.h"

#include "4coder_stringf.cpp"
#include "4coder_malloc_allocator.cpp"
#include "4coder_token.cpp"
#include "generated/lexer_cpp.cpp"
#include "4ed_api_definition.cpp"
#include "4ed_api_parser.cpp"
#include "4ed_kv_parser.cpp"
#include "meta_print.cpp"

//-NOTE(kv) Annoying parsing job
inline void m_brace_open(Ed_Parser *p)  { ep_char(p, '{'); }
inline void m_brace_close(Ed_Parser *p) { ep_char(p, '}'); }
inline b32 m_maybe_brace_close(Ed_Parser *p) { return ep_maybe_char(p, '}'); }
inline void m_paren_open(Ed_Parser *p)  { ep_char(p, '('); }
inline void m_paren_close(Ed_Parser *p) { ep_char(p, ')'); }
inline b32 m_maybe_paren_close(Ed_Parser *p) { return ep_maybe_char(p, ')'); }
#define mpa_parens     defer_block(m_paren_open(p), m_paren_close(p))


inline void
parse_struct_member(Ed_Parser *p, Meta_Struct_Member *member){
 if(ep_maybe_id(p, "tagged_by")){
  mpa_parens{ member->discriminator = ep_id(p); }
 }
 {
  member->type = ep_id(p);
  while(ep_maybe_char(p, '*')){ member->type_star_count++; }
 }
 member->name = ep_id(p);
 ep_consume_semicolons(p);
}

function Meta_Struct_Members
parse_struct_body(Ed_Parser *p){
 Meta_Struct_Members result = {};
 m_brace_open(p);
 while(p->ok_ && !m_maybe_brace_close(p)){
  Meta_Struct_Member *member = &result.push2();
  *member = {};
  parse_struct_member(p, member);
 }
 return result;
}

#include "meta_introspect.cpp"

function b32
list_files_recursive(Arena *arena, arrayof<char *> &outfiles, char *path)
{
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

function Ed_Parser
m_parser_from_string(Arena *arena, String string){
 Token_List token_list = lex_full_input_cpp(arena, string);
 Token_Iterator token_it = make_token_iterator(token_iterator(0, &token_list));
 Ed_Parser parser = make_ep_from_string(string, token_it);
 return parser;
}

inline void
push_bezier_variant(arrayof<Union_Variant> *variants,
                    char *name, i32 enum_value, char *struct_members){
 Arena *arena = &meta_permanent_arena;
 Scratch_Block scratch;
 Ed_Parser parser = m_parser_from_string(scratch, SCu8(struct_members));
 Union_Variant *variant = variants->push_zero();
 variant->name           = SCu8(name);
 variant->enum_value     = enum_value;
 variant->struct_members = parse_struct_body(&parser);
 variant->enum_name      = string_concat(arena, "Bezier_Type_", variant->name);
 variant->struct_name    = string_concat(arena, "Bezier_",      variant->name);
}

function void
print_struct_body(Printer &p, Meta_Struct_Members *members){
 m_braces_sm{
  for_i32(i,0,members->count){
   auto &member = members->items[i];
   if(!member_was_removed(member)){
    if(i!=0){ p<<"\n"; }
    
    p<<member.type<<" ";
    for_repeat(member.type_star_count){ p<<"*"; }
    
    p<<member.name<<";";
   }
  }
 }
}

function b32
codegen_main(char *code_dir){
 Scratch_Block scratch;
 b32 ok = true;
 String outpath_base;
 {
  Printer p = make_printer_buffer(scratch, 256);
  p<<code_dir<<OS_SLASH<<"game"<<OS_SLASH<<"generated"<<OS_SLASH<<"bezier_types.gen";
  outpath_base = printer_get_string(p);
 }
 Printer_Pair ps = m_open_files_to_write(outpath_base);
 ok = ps.h.FILE && ps.c.FILE;
 Printer &p = ps.h;
 if(ok){
  arrayof<Union_Variant> variants = {};
  //-NOTE @data of the bezier variants
#define X(...)  push_bezier_variant(&variants, __VA_ARGS__)
  X("v3v2",       0, "{ v3 d0; v2 d3; };");
  X("Parabola",   1, "{ v3 d; };");
  X("Offsets",    2, "{ v3 d0; v3 d3; };");
  X("Planar_Vec", 3, "{ v2 d0; v2 d3; v3 unit_y; };");
  X("C2",         4, "{ Curve_Index ref; v3 d3; };");
#undef X
  
  String discriminator_type = strlit("Bezier_Type");
  {//-NOTE ("Enum")
   //NOTE(kv) This sucks...
   arrayof<String> enum_names = {};
   enum_names.set_count(variants.count);
   for_i32(i,0,variants.count){
    enum_names.get(i) = variants.get(i).enum_name;
   }
   
   m_location;
   String &type_name = discriminator_type;
   p<<"enum "<<type_name;
   m_braces_sm{
    for_i32(i,0,variants.count){
     auto &variant = variants.get(i);
     p<<enum_names.get(i)<<" = "<<variant.enum_value<<",\n";
    }
   }
   
   print_enum_meta(&ps, type_name, enum_names);
  }
  
  {//-NOTE "Data structure associated with each variant"
   for_i32(i,0,variants.count){
    auto *variant = &variants.get(i);
    m_location;
    p<<"struct "<<variant->struct_name;
    print_struct_body(p,&variant->struct_members);
    print_struct_meta(&ps, variant->struct_name, &variant->struct_members);
   }
  }
  
  {//-NOTE ("Union of all the Bezier type")
   m_location;
   String type_name = strlit("Bezier_Union");
   {//NOTE Code
    p<<"union "<<type_name;
    m_braces_sm{
     for_i32(i,0,variants.count){
      if(i!=0){ p<<"\n"; }
      
      auto &variant = variants.get(i);
      p<<variant.struct_name<<" "<<variant.name<<";";
     }
    }
   }
   {//NOTE Meta
    print_union_meta(&ps, type_name, &variants, discriminator_type);
   }
  }
  
  m_close_pair(&ps);
 }
 return ok;
}

xfunction i32
main(i32 argc, char **argv){
 b32 ok = true;
 Scratch_Block scratch;
 meta_logging_level = 1;
 command_name = argv[0];
 char *code_dir = "";
 if(argc < 2){
  printf("Usage: %s <code_dir>\n", command_name);
  ok = false;
 }else{
  code_dir = argv[1];
 }
 
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
 
 arrayof<File_Name_Data> source_files = {};
 if(ok){
  // NOTE: Reading the input files
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
     if (file) {
      Stringz text = read_entire_file_handle(scratch, file);
      fclose(file);
      source_files.push(File_Name_Data{
                         .name=push_string(scratch, filename),
                         .data=text});
     }else{
      printf("error: could not open input file: [%s]\n", filename);
      ok = false;
      break;
     }
    }
   }
  }
 }
 
 ok = ok && api_parser_main(source_files);
 ok = ok && introspect_main(source_files);
 ok = ok && codegen_main(code_dir);
 
 int exit_code = !ok;
 fflush(stdout);
 return exit_code;
}

//~BOTTOM
