/* 
 * Mr. 4th Dimention - Allen Webster
 * (Modified by kv)
 *
 * Do all the meta programming things
 *
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

#include "meta.h"
#include "meta_print.h"
#include "meta_parse.h"
#include "meta_klang.h"

#include "meta_parse.cpp"
#include "4ed_api_definition.cpp"
#include "4ed_api_parser.cpp"
#include "meta_print.cpp"
#include "meta_klang.cpp"

inline String
maybe_skip_begin_and_end(String string){
 if(string.len>1){
  if(string.str[0]=='{'){
   string.str++;
   string.size--;
   
   if(string.len>0){
    if(string.str[string.len]=='}'){
     string.len--;
    }
   }
  }
 }
 return string;
}
#define strlit_noquote(anything) \
maybe_skip_begin_and_end(strlit(#anything))

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
inline void
push_bezier_variant(arrayof<Union_Variant> &variants, Union_Variant v,
                    String struct_members){
 Arena *arena = &meta_permanent_arena;
 Scratch_Block scratch;
 Ed_Parser parser = m_parser_from_string(scratch, struct_members);
 v.struct_members = parse_struct_body(arena, &parser);
 v.enum_name      = strcat(arena, "Bezier_Type_", v.name);
 v.struct_name    = strcat(arena, "Bezier_",      v.name);
 variants.push(v);
}
function void
generate_bezier_types(Printer &p0, Printer_Pair &ps_meta){
 Scratch_Block scratch;
 //-NOTE Bezier types
 arrayof<Union_Variant> variants = {};
 //-NOTE @data of the bezier variants
#define X(penum_val, pname, pname_lower, pstruct_members) \
push_bezier_variant(variants, \
Union_Variant{ \
.enum_value=penum_val, \
.name=strlit(#pname), \
.name_lower=strlit(#pname_lower) }, \
strlit(#pstruct_members))
 
 X(0, v3v2,     v3v2,     { v3 d0; v2 d3; });
 X(1, Parabola, parabola, { v3 d; });
 X(2, Offsets,  offsets,  { v3 d0; v3 d3; });
 X(3, Unit,     unit,     { v2 d0; v2 d3; v3 unit_y; });
 X(4, Unit2,    unit2,    { v4 d0d3; v3 unit_y; });
 X(5, C2,       c2,       { Curve_Index ref; v3 d3; });
 X(6, Line,     line,     { });
 X(7, Bezd_Old, bezd_old, { v3 d0; v2 d3; });
#undef X
 
 String enum_type = strlit("Bezier_Type");
 {//-NOTE ("Enum")
  //TODO(kv) array copy mega-annoyance!
  auto enum_names = static_array<String>(scratch, variants.count);
  enum_names.set_count(variants.count);
  for_i32(i,0,variants.count){ enum_names[i] = variants[i].enum_name; }
  
  auto enum_values = static_array<i32>(scratch, variants.count);
  enum_values.set_count(variants.count);
  for_i32(i,0,variants.count){ enum_values[i] = variants[i].enum_value; }
  
  print_enum(p0, enum_type, enum_names, enum_values);
  print_enum_meta(ps_meta, enum_type, enum_names);
 }
 {//-NOTE "Data structure associated with each variant"
  for_i32(i,0,variants.count){
   auto *variant = &variants.get(i);
   m_locationp(p0);
   print_struct(p0, variant->struct_name, variant->struct_members);
   print_struct_meta(ps_meta, variant->struct_name, variant->struct_members);
  }
 }
 {//-NOTE ("Union of all the Bezier type")
  String type_name = strlit("Bezier_Union");
  {
   auto &p = p0;
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
   print_union_meta(ps_meta, type_name, &variants, enum_type);
  }
 }
 {//-NOTE Transitional functions (aggravation: 100%)
  Printer p = m_open_file_to_write(pjoin(scratch, meta_dirs.game_gen, "send_bez.gen.h"));
  m_location;
  for_i1(iv,0,variants.count){
   auto &variant = variants[iv];
   b32 is_c2 = variant.name=="C2";
   auto variant_parameters = [&]()->void{
    if(!is_c2){ p<"p0, "; }
    for_i32(im,0,variant.struct_members.count){
     auto &member = variant.struct_members[im];
     p<member.name<", ";
    }
    p<"p3";
   };
   {//-Function prototype
    {//-The main function
     p<<"xfunction void\n"<<"send_bez_"<<variant.name_lower;
     m_parens{
      p<"String name";
      if(!is_c2){ p<", String p0"; }
      for_i32(im,0,variant.struct_members.count){
       auto &member = variant.struct_members[im];
       p<", ";
       if(member.type==strlit("Curve_Index")){
        p<"String ";
       }else{
        print_struct_member_type(p,member);
       }
       p<member.name;
      }
      p<", String p3"; 
      p<", Line_Params params=lp()";
      p<", i32 linum=__builtin_LINE()";
     }
     p<";\n";
    }
    {//-Overloads with radii
     auto fun=[&](b32 is_v4)->void{
      p<<"inline void\n"<<"send_bez_"<<variant.name_lower;
      m_parens{
       p<"String name";
       if(!is_c2){ p<", String p0"; }
       for_i32(im,0,variant.struct_members.count){
        auto &member = variant.struct_members[im];
        p<", ";
        if(member.type==strlit("Curve_Index")){
         p<"String ";
        }else{
         print_struct_member_type(p,member);
        }
        p<member.name;
       }
       p<", String p3"; 
       if(is_v4){
        p<", v4 radii";
       }else{
        p<", i4 radii";
       }
       p<", i32 linum=__builtin_LINE()";
      }
      m_braces{
       p<"send_bez_"<variant.name_lower;
       m_parens{
        p<"name, ";
        variant_parameters();
        p<", lp(radii), linum";
       }
       p<";\n";
      }
     };
     fun(0);
     fun(1);
    }
   }
   {//-NOTE Macros
    {//NOTE bn_bs
     auto bn_bs = [&](b32 is_bs)->void{
      b32 is_bn = !is_bs;
      p<"#define "<(is_bs?"bs_":"bn_")<variant.name_lower;
      m_parens{
       if(is_bn){ p<"name, "; };
       if(!is_c2){ p<"p0, "; }
       for_i32(im,0,variant.struct_members.count){
        auto &member = variant.struct_members[im];
        p<member.name<", ";
       }
       p<"p3, ...";
      }
      p<"\\\n";
      p<"send_bez_"<variant.name_lower;
      m_parens{
       p<(is_bn?"strlit(#name), " : "strlit(\"l\"), ");
       if(!is_c2){ p<"strlit(#p0), "; }
       for_i32(im,0,variant.struct_members.count){
        auto &member = variant.struct_members[im];
        if(is_c2 && member.name==strlit("ref")){
         p<"strlit(#ref)";
        }else{
         p<member.name;
        }
        p<", ";
       }
       p<"strlit(#p3), __VA_ARGS__";
      }
      p<"\n";
     };
     bn_bs(0);
     bn_bs(1);
    }
    {//NOTE bb_ba
     auto bb_ba = [&](b32 is_ba)->void{
      b32 is_bb = !is_ba;
      p<"#define "<(is_ba?"ba_":"bb_")<variant.name_lower;
      m_parens{//NOTE macro parameters
       p<"name, ";
       variant_parameters();
       p<", ...";
      }
      p<"\\\n";
      p<"bn_"<variant.name_lower;
      m_parens{//NOTE bn parameters
       p<"name, ";
       variant_parameters();
       p<", __VA_ARGS__";
      }
      p<"; \\\n";
      if(is_bb){ p<"Bez "; }
      p<"name = bez_"<variant.name_lower;
      m_parens{//NOTE bezier curve calculation
       variant_parameters();
      }
      p<";\n";
     };
     bb_ba(0);
     bb_ba(1);
    }
   }
   p<"\n";
  }
  close_file(p);
 }
}

xfunction i32
main(i32 argc, char **argv){
 b32 ok = true;
 Arena *arena = &meta_permanent_arena;
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
 meta_dirs.game     = pjoin(arena, meta_dirs.code, "game");
 meta_dirs.game_gen = pjoin(arena, meta_dirs.game, "generated");
 
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
  paths[i] = pjoin(arena, code_dir, cpaths[i]);
 }
 
 arrayof<File_Name_Data> source_files = {};
 if(ok){
  // NOTE: Reading the input files
  for_i1(ipath,0,alen(paths)){
   Stringz path = paths[ipath];
   arrayof<char *> filenames = {};
   if(path_is_directory(path)){
    ok = list_files_recursive(arena, filenames, to_cstring(path));
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
      Stringz text = read_entire_file_handle(arena, file);
      fclose(file);
      source_files.push(File_Name_Data{
                         .name=push_string(arena, filename),
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
 ok = ok && klang_main(source_files);
 
 int exit_code = !ok;
 fflush(stdout);
 return exit_code;
}

//~BOTTOM
