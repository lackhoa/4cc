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

#include "meta_parse.cpp"
#include "4ed_api_definition.cpp"
#include "4ed_api_parser.cpp"
#include "meta_print.cpp"
#include "meta_klang.cpp"
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
function void
push_bezier_variant(arrayof<Union_Variant> &variants, Union_Variant variant,
                    String struct_members, char *options=""){
 Arena *arena = &meta_permanent_arena;
 Scratch_Block scratch;
 {
  Ed_Parser parser = m_parser_from_string(scratch, SCu8(options));
  if(ep_maybe_id(&parser, "no_endpoints")){
   variant.flags |= Meta_Curve_No_Endpoints;
  }
 }
 {
  Ed_Parser parser = m_parser_from_string(scratch, struct_members);
  variant.struct_members = parse_struct_body(arena, &parser);
  if(not(variant.flags & Meta_Curve_No_Endpoints)){
   variant.struct_members.push_first(struct_member_from_string("Vertex_Index p0"));
   variant.struct_members.push(struct_member_from_string("Vertex_Index p3"));
  }
 }
 variant.enum_name   = strcat(arena, "Curve_Type_", variant.name);
 variant.struct_name = strcat(arena, "Curve_",      variant.name);
 variants.push(variant);
}
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
generate_bezier_types(Printer &printer){
 Scratch_Block scratch;
 arrayof<Union_Variant> variants = {};
 //-NOTE @data of the variants
#define X(penum_val, pname, pname_lower, pstruct_members, ...) \
push_bezier_variant(variants, \
Union_Variant{ \
.enum_value=penum_val, \
.name=strlit(#pname), \
.name_lower=strlit(#pname_lower) }, \
strlit(#pstruct_members), \
__VA_ARGS__)
 
 X(0,  v3v2,     v3v2,     { v3 d0; v2 d3; });
 X(1,  Parabola, parabola, { v3 d; });
 X(2,  Offset,   offset,   { v3 d0; v3 d3; });
 X(3,  Unit,     unit,     { v2 d0; v2 d3; v3 unit_y; });
 X(4,  Unit2,    unit2,    { v4 d0d3; v3 unit_y; });
 X(5,  C2,       c2,       { Curve_Index ref; v3 d3; Vertex_Index p3; }, "no_endpoints");
 X(6,  Line,     line,     { });
 X(7,  Bezd_Old, bezd_old, { v3 d0; v2 d3; });
 X(8,  NegateX,  negateX,  { Curve_Index ref; }, "no_endpoints");
 X(9,  Lerp,     lerp,     { Curve_Index begin; Curve_Index end; }, "no_endpoints");
 X(10, Raw,      raw,      { v3 p1; v3 p2; });
#undef X
 
 String enum_type = strlit("Curve_Type");
 {//-NOTE ("Enum")
  //TODO(kv) array copy mega-annoyance!
  auto enum_names = static_array<String>(scratch, variants.count);
  enum_names.set_count(variants.count);
  for_i32(i,0,variants.count){ enum_names[i] = variants[i].enum_name; }
  
  auto enum_values = static_array<i32>(scratch, variants.count);
  enum_values.set_count(variants.count);
  for_i32(i,0,variants.count){ enum_values[i] = variants[i].enum_value; }
  
  print_enum(printer, enum_type, enum_names, enum_values);
  print_enum_meta(printer, enum_type, enum_names);
 }
 {//-NOTE "Data structure associated with each variant"
  for_i32(i,0,variants.count){
   auto *variant = &variants.get(i);
   m_locationp(printer);
   print_struct(printer, variant->struct_name, variant->struct_members);
   print_struct_meta(printer, variant->struct_name, variant->struct_members);
  }
 }
 {//-Union of all the Bezier type
  String type_name = strlit("Curve_Union");
  {
   Printer &p = printer;
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
   print_union_meta(printer, type_name, &variants, enum_type);
  }
 }
 {//-NOTE Transitional functions (aggravation: 100%)
  auto is_vertex_or_curve_index = [&](Parsed_Type &type)->b32{
   return (type.name==strlit("Vertex_Index") ||
           type.name==strlit("Curve_Index"));
  };
  auto print_prototype = [&](Printer &p, Union_Variant &variant, b32 is_declaration)->void{
   p<"function void\n"<"send_bez_"<variant.name_lower;
   m_parens{
    separator_block(p, ", "){
     p<"String name"; separator(p);
     for_i32(im,0,variant.struct_members.count){
      Meta_Struct_Member &member = variant.struct_members[im];
      if(is_vertex_or_curve_index(member.type)){
       p<"String "<member.name;
      }else{
       print_struct_member(p,member);
      }
      separator(p);
     }
     if(is_declaration){
      p<"Line_Params params=lp()";    separator(p);
      p<"i32 linum=__builtin_LINE()"; separator(p);
     }else{
      p<"Line_Params params";    separator(p);
      p<"i32 linum"; separator(p);
     }
    }
   }
  };
  {//-Header
   Printer p = m_open_file_to_write(pjoin(scratch, meta_dirs.game_gen, "send_bez.gen.h"));
   {//-get_p0_index_or_zero / get_p3_index_or_zero
    m_location;
    for_i32(p0_p3_index,0,2){
     b32 is_p0 = p0_p3_index==0;
     b32 is_p3 = p0_p3_index==1;
     String endpoint_name = is_p0 ? strlit("p0") : strlit("p3");
     p<"function Vertex_Index\n"<
      "get_"<endpoint_name<"_index_or_zero(Curve_Data &curve)";
     m_braces{
      p<"switch(curve.type)";
      m_braces{
       for_i32(variant_index,0,variants.count){
        Union_Variant &variant = variants[variant_index];
        b32 has_endpoint = false;
        for_i32(member_index,0,variant.struct_members.count){
         Meta_Struct_Member &member = variant.struct_members[member_index];
         if(member.name == endpoint_name){
          has_endpoint = true;
          break;
         }
        }
        p<"case "<variant.enum_name<": return {";
        if(has_endpoint){
         p<"curve.data."<variant.name_lower<"."<endpoint_name;
        };
        p<"};\n";
       }
      }
      p<"return {};";
     }
    }
   }
   m_location;
   for_i1(variant_index,0,variants.count){
    Union_Variant &variant = variants[variant_index];
    auto member_names = [&]()->void{
     arrayof<String> list;
     init_dynamic(list, scratch);
     for_i32(im,0,variant.struct_members.count){
      auto &member = variant.struct_members[im];
      list.push(member.name);
     }
     print_comma_separated(p,list);
    };
    {//-Send function
     m_location;
     {//-The main send function
      print_prototype(p, variant, true);
      p<";\n";
     }
     {//-Overloads with radii
      auto fun = [&](b32 is_v4)->void{
       p<<"inline void\n"<<"send_bez_"<<variant.name_lower;
       m_parens{
        p<"String name";
        for_i32(im,0,variant.struct_members.count){
         auto &member = variant.struct_members[im];
         p<", ";
         if(is_vertex_or_curve_index(member.type)){
          p<"String "<member.name;
         }else{
          print_struct_member(p,member);
         }
        }
        p<(is_v4 ? ", v4 radii" : ", i4 radii");
        p<", i32 linum=__builtin_LINE()";
       }
       m_braces{
        p<"send_bez_"<variant.name_lower;
        m_parens{
         p<"name, ";
         member_names();
         p<", lp(radii), linum";
        }
        p<";\n";
       }
      };
      fun(0);
      fun(1);
     }
    }
    {//-Macros
     m_location;
     {//-bn_bs
      auto bn_bs = [&](b32 is_bs)->void{
       b32 is_bn = !is_bs;
       p<"#define "<(is_bs?"bs_":"bn_")<variant.name_lower;
       m_parens{
        arrayof<String> list;
        init_dynamic(list, scratch);
        if(is_bn){ list.push(strlit("name")); };
        for_i32(im,0,variant.struct_members.count){
         auto &member = variant.struct_members[im];
         list.push(member.name);
        }
        list.push(strlit("..."));
        print_comma_separated(p,list);
       }
       p<"\\\n";
       p<"send_bez_"<variant.name_lower;
       m_parens{
        begin_separator(p, ", ");
        p < (is_bn?"strlit(#name)" : "strlit(\"l\")"); separator(p);
        for_i32(im,0,variant.struct_members.count){
         auto &member = variant.struct_members[im]; 
         if(is_vertex_or_curve_index(member.type)){
          p<"strlit(#"<member.name<")";
         }else{
          p<member.name;
         }
         separator(p);
        }
        p<"__VA_ARGS__"; separator(p);
        end_separator(p);
       }
       p<"\n";
      };
      bn_bs(0);
      bn_bs(1);
     }
     {//-bb_ba
      auto bb_ba = [&](b32 is_ba)->void{
       b32 is_bb = !is_ba;
       p<"#define "<(is_ba?"ba_":"bb_")<variant.name_lower;
       m_parens{//NOTE macro parameters
        p<"name, ";
        member_names();
        p<", ...";
       }
       p<"\\\n";
       p<"bn_"<variant.name_lower;
       m_parens{//NOTE bn parameters
        p<"name, ";
        member_names();
        p<", __VA_ARGS__";
       }
       p<"; \\\n";
       if(is_bb){ p<"Bez "; }
       p<"name = bez_"<variant.name_lower;
       m_parens{//NOTE bezier curve calculation
        member_names();
       }
       p<";\n";
      };
      bb_ba(0);
      bb_ba(1);
     }
    }
    {//-Function that sends each variant
     
    }
    p<"\n";
   }
   close_file(p);
  }
  {//-Implementation
   Printer p = m_open_file_to_write(pjoin(scratch, meta_dirs.game_gen, "send_bez.gen.cpp"));
   m_location;
   for_i1(variant_index,0,variants.count){
    Union_Variant &variant = variants[variant_index];
    print_prototype(p, variant, false); m_braces{
     p<"Modeler &m = *painter.modeler;\n";
     //-Fetch all the references
     for_i32(member_index,0,variant.struct_members.count){
      Meta_Struct_Member &member = variant.struct_members[member_index];
      b32 is_vertex = member.type.name == strlit("Vertex_Index");
      b32 is_curve  = member.type.name == strlit("Curve_Index");
      if(is_vertex || is_curve){
       p<member.type.name<" "<member.name<"_index = "<
       (is_vertex ? "get_vertex_by_name" : "get_curve_by_name")<
        "(m, "<member.name<").index;\n";
       p<"kv_assert("<member.name<"_index.v);\n";
      }
     }
     
     p<"Curve_Union data = {."<variant.name_lower<"={ ";
     for_i32(member_index,0,variant.struct_members.count){
      Meta_Struct_Member &member = variant.struct_members[member_index];
      p<"."<member.name<"="<member.name<
      (is_vertex_or_curve_index(member.type) ? "_index" : "")
       <", ";
     }
     p<"}};\n";
     p<"send_bez_func"; m_comma_list{
      p<"name, "<variant.enum_name<", data, params, linum";
     }
     p<";\n";
    }
   }
   close_file(p);
  }
 }
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
