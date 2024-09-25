#include "4ed_kv_parser.cpp"
#include "meta_introspect.h"

struct Meta_Type_Names {
 String type_name;
 String info_function_name;
 String global_info_name;
 String read_function_name;
};
global Arena meta_permanent_arena = make_arena_malloc();
global arrayof<Meta_Type_Names> meta_type_name_store;

//-NOTE(kv) Annoying parsing job
inline void m_brace_open(Ed_Parser *p)  { ep_char(p, '{'); }
inline void m_brace_close(Ed_Parser *p) { ep_char(p, '}'); }
inline b32 m_maybe_brace_close(Ed_Parser *p) { return ep_maybe_char(p, '}'); }
inline void m_paren_open(Ed_Parser *p)  { ep_char(p, '('); }
inline void m_paren_close(Ed_Parser *p) { ep_char(p, ')'); }
inline b32 m_maybe_paren_close(Ed_Parser *p) { return ep_maybe_char(p, ')'); }

#define m_parens       defer_block(m_paren_open(p), m_paren_close(p))
#define m_print_braces defer_block((p << "\n{\n"), (p << "\n}\n"))
#define m_macro_braces defer_block((p << "\\\n{\\\n"), (p << "\\\n}\\\n"))
#define m_macro_braces_sm  defer_block((p << "\\\n{\\\n"), (p << "\\\n};\\\n"))
#define m_print_location p<<"// "<<filename_linum<<"\n"
//-

struct Enclosed_in_strlit { String string; };
//
function void
print(Printer &p, Enclosed_in_strlit item)
{
 printn3(p, "strlit(\"", item.string, "\")");
}
//
force_inline Enclosed_in_strlit enclosed_in_strlit(String string) {
 return Enclosed_in_strlit{ string, };
}

function Meta_Type_Names &
m_get_type_names(String type_name)
{
 auto &store = meta_type_name_store;
 for_i32(index,0,store.count){
  if(store[index].type_name == type_name){
   return store[index];
  }
 }
 
 //Note(kv) not found
 auto arena = &meta_permanent_arena;
 Meta_Type_Names &new_item = store.push2();
 new_item = {
  .type_name          = push_string(arena, type_name),
  .info_function_name = push_stringf(arena, "get_type_info_%.*s", string_expand(type_name)),
  .global_info_name   = push_stringf(arena, "Type_Info_%.*s", string_expand(type_name)),
  .read_function_name = push_stringf(arena, "read_%.*s", string_expand(type_name)),
 };
 return new_item;
}
function String
get_type_global_info_name(String type_name) {
 return m_get_type_names(type_name).global_info_name;
}
inline String
get_type_read_function_name(String type_name) {
 return m_get_type_names(type_name).read_function_name;
}
function String
get_type_info_function_name(String type_name) {
 return m_get_type_names(type_name).info_function_name;
}

inline void
meta_parse_key(Ed_Parser *p, char *key) {
 ep_id(p, key); ep_char(p, '=');
}
inline b32
meta_maybe_key(Ed_Parser *p, char *key)
{
 b32 result = false;
 if ( ep_maybe_id(p, key) ) {
  ep_char(p, '=');
  result = p->ok_;
 }
 return result;
}

inline b32
member_was_removed(Meta_Struct_Member &member) {
 return member.version_removed.len != 0;
}

#if 0
function void
read_Bezier_Union_test(Data_Reader &r, Bezier_Union &pointer,
                       Bezier_Type discriminator)
{
 STB_Parser *p = r.parser;
 eat_char(p, '{');
 Bezier_Data_v3v2 m_v3v2 = {};
 
 switch(discriminator)
 {
  case Bezier_Type_v3v2:
  {
   eat_id(p, strlit("v3v2"));
   read_Bezier_Data_v3v2(r, pointer.v3v2);
  }break;
 }
 
 pointer.v3v2 = m_v3v2;
 
 Bezier_Data_Offsets m_offsets = {};
 
 {
  eat_id(p, strlit("offsets"));
  read_Bezier_Data_Offsets(r, m_offsets);
 }
 pointer.offsets = m_offsets;
 
 Bezier_Data_Planar_Vec m_planar_vec = {};
 
 {
  eat_id(p, strlit("planar_vec"));
  read_Bezier_Data_Planar_Vec(r, m_planar_vec);
 }
 pointer.planar_vec = m_planar_vec;
 
 eat_char(p, '}');
}
#endif

function void
print_type_meta_shared(Printer &p, String type_name)
{
 Scratch_Block scratch;
 String type_global_var = get_type_global_info_name(type_name);
 String function_name = get_type_info_function_name(type_name);
 m_print_location;
 {// NOTE: The global variable
  p << "global Type_Info " << type_global_var << " = " <<
   function_name << "();\n\n";
 }
 
 {// NOTE: The overload to automatically get the type info from a pointer of that type
  p << "function Type_Info &type_info_from_pointer(" << type_name << "*pointer)";
  m_print_braces {
   p << "return " << type_global_var << ";";
  }
 }
}

function void
print_struct_meta(Printer &p, String type_name,
                  arrayof<Meta_Struct_Member> &members)
{
 Scratch_Block scratch;
 char newline = '\n';
 char *newlinex2 = "\n\n";
 
#define brace_block  p << "\n{\n"; defer( p << "\n}\n"; );
 
 {//-NOTE: Function to generate the type info
  m_print_location;
  String function_name = get_type_info_function_name(type_name);;
  p << "function Type_Info\n" << function_name << "()";
  {brace_block;
   p << "Type_Info result = {};\n";
   p << "result.name = " << enclosed_in_strlit(type_name) << ";\n";
   p << "result.size = sizeof(" << type_name << ");\n";
   p << "result.kind = Type_Kind_Struct;\n";
   // NOTE(kv) Computing member count
   i32 member_count = 0;
   for_i32 (raw_member_index,0,members.count) {
    if ( !member_was_removed(members[raw_member_index]) ) {
     member_count++;
    }
   }
   
   p << "result.members.set_count(" << member_count << ");\n";
   i32 member_index = 0;
   for_i1 (raw_member_index,0,members.count) {
    auto &member = members[raw_member_index];
    if (!member_was_removed(member)) {
     String member_type = get_type_global_info_name(member.type);
     p << "result.members[" << member_index << "] = " <<
      "{.type=&" << member_type <<
      ", .name=" << enclosed_in_strlit(member.name) <<
      ", .offset=offsetof("<<type_name<<", "<<member.name<<")";
     if(member.discriminator.len){
      p<<", .discriminator_offset=offsetof("<<type_name<<", "<<member.discriminator<<")";
     }
     if(member.unserialized){
      p<<", .unserialized=true";
     }
     p<<"};\n";
     member_index++;
    }
   }
   
   p << "return result;";
  }
 }
 
 print_type_meta_shared(p, type_name);
 
 {// NOTE: ;meta_read_struct
  p << "function void\n";
  p<<get_type_read_function_name(type_name)<<
   "(Data_Reader &r, "<<type_name<<" &pointer)";
  {brace_block;
   p << "STB_Parser *p = r.parser;\n";
   
   p << "eat_char(p, '{');\n";
   for_i1(member_index,0,members.count){
    Meta_Struct_Member &member = members[member_index];
    if(!member.unserialized){
     String version_added = member.version_added;
     b32 has_version_added = version_added.len != 0;
     if (!has_version_added) { version_added = strlit("0"); }
     String version_removed = member.version_removed;
     b32 has_version_removed = version_removed.len != 0;
     if (!has_version_removed) { version_removed = strlit("Version_Inf"); }
     String default_value = member.default_value;
     b32 has_default = default_value.len != 0;
     if (!has_default) { default_value = strlit("{}"); }
     
     b32 member_currently_exists = !has_version_removed;
     //NOTE(kv) Mangle the name so that we don't have conflict with other local vars.
     String varname = push_stringf(scratch, "m_%.*s", string_expand(member.name));
     
     {//NOTE(kv) Make a local variable to store the value, for
      //  1. convenience, and
      //  2. the struct might not have that member anymore,
      //     but we may need that value for conversion purpose.
      p<<member.type<<" "<<varname<<" = "<<default_value<<";\n";
     }
     
     if (has_version_added || has_version_removed)
     {// NOTE(kv) Only read if the data has that member.
      p<<"if ( in_range_exclusive("<<
       version_added<<", r.read_version, "<< version_removed<<") )";
     }
     {//NOTE(kv) Read data to the local var
      brace_block;
      p << "eat_id(p, " << enclosed_in_strlit(member.name) << ");\n";
      String read_function = get_type_read_function_name(member.type);
      b32 has_disciminator = member.discriminator.len != 0;
      if (has_disciminator){
       p<<read_function<<"(r, "<<varname<<", pointer."<<member.discriminator<<");";
      } else{
       p<<read_function<<"(r, "<<varname<<");";
      }
     }
     if ( member_currently_exists ) {
      //NOTE(kv) Assign the local var to the dest struct member.
      p<<"pointer."<<member.name<<" = "<<varname<<";\n\n";
     }
    }
   }
   p << "eat_char(p, '}');";
  }
 }
 
#undef brace_block
}

function void
print_union_meta(Printer &p, String type_name,
                 arrayof<Meta_Union_Member> &members,
                 String discriminator_type)
{
 Scratch_Block scratch;
 //NOTE(kv) e.g get_Type_Info_Blah
 char newline = '\n';
 char *newlinex2 = "\n\n";
 
#define brace_block  p << "\n{\n"; defer( p << "\n}\n"; );
 
 {//-NOTE: Function to generate the type info
  m_print_location;
  String function_name = get_type_info_function_name(type_name);
  p << "function Type_Info\n" << function_name << "()";
  {brace_block;
   p << "Type_Info result = {};\n" <<
    "result.name = "<<enclosed_in_strlit(type_name)<<";\n" <<
    "result.size = sizeof("<<type_name<<");\n"<<
    "result.kind = Type_Kind_Union;\n"<<
    "result.discriminator_type = &"<<get_type_global_info_name(discriminator_type)<<";\n";
   {
    p<<"result.union_members.set_count("<<members.count<<");\n";
    for_i32(index,0,members.count){
     auto &member = members[index];
     String member_type = get_type_global_info_name(member.type);
     p<<"result.union_members["<<index<<"] = {"<<
      ".type=&"<<member_type<<", "<<
      ".name="<<enclosed_in_strlit(member.name)<<", "<<
      ".variant="<<member.variant<<
      "};\n";
    }
   }
   p << "return result;";
  }
 }
 
 print_type_meta_shared(p, type_name);
 
 {// NOTE: ;meta_read_union
  m_print_location;
  p << "function void\n"<<
   get_type_read_function_name(type_name) <<
   "(Data_Reader &r, " <<
   type_name << " &pointer, "<<
   discriminator_type<<" variant"<<")";
  
  {brace_block;
   p << "STB_Parser *p = r.parser;\n";
   //NOTE(kv) When we add/remove union members, this is gonna get complicated,
   //  but right now I don't care.
   p<<"switch (variant)";
   {brace_block;
    for_i1 (member_index,0,members.count) {
     auto &member = members[member_index];
     p<<"case "<<member.variant<<":";
     {brace_block;
      p<<get_type_read_function_name(member.type)<<
       "(r, "<<"pointer."<<member.name<<");\n"<<
       "break;";
     }
    }
   }
  }
 }
#undef brace_block
}

function void
print_enum_meta(Printer &p, String type_name,
                arrayof<String> &enum_values)
{
 Scratch_Block scratch;
 char newline = '\n';
 
#define brace_block  p << "\n{\n"; defer( p << "\n}\n"; );
 
 {//-NOTE: Function to generate the type info
  m_print_location;
  String function_name = get_type_info_function_name(type_name);
  p << "function Type_Info\n" << function_name << "()";
  {brace_block;
   p << "Type_Info result = {};" << newline;
   p << "result.name = " << enclosed_in_strlit(type_name) << ";" << newline;
   p << "result.size = sizeof(" << type_name << ");" << newline;
   p << "result.kind = Type_Kind_Enum;\n";
   p << "result.enum_members.set_count(" << enum_values.count << ");\n";
   for_i1 (enum_index,0,enum_values.count) {
    String enum_value = enum_values[enum_index];
    p << "result.enum_members[" << enum_index << "] = " <<
     "{.name=" << enclosed_in_strlit(enum_value) << ", " <<
     ".value=" << enum_value <<
     "};\n";
   }
   p << "return result;";
  }
 }
 
 print_type_meta_shared(p, type_name);
 
 {// NOTE: ;meta_read
  p << "function void" << newline;
  p << get_type_read_function_name(type_name) <<
   "(Data_Reader &r, " << type_name << " &pointer)";
  {brace_block;
   p << "STB_Parser *p = r.parser;" << newline;
   // NOTE(kv): @meta_introspect_enum_size
   //   If the type size is too BIG, we'll read past from wrong address.
   p << "i32 integer = eat_i1(p);\n" <<
    "pointer = *(" << type_name << "*)(&integer);";
  }
 }
 
 {// NOTE ;meta_introspect_enum_size
  p << "static_assert( sizeof(" << type_name << ") <= sizeof(i32) );\n\n";
 }
 
#undef brace_block
}

inline void
parse_cpp_struct_member(Ed_Parser *p, Meta_Struct_Member &member) {
 if (ep_maybe_id(p, "tagged_by")){
  m_parens{ member.discriminator = ep_id(p); }
 }
 {
  member.type = ep_id(p);
  while (ep_maybe_char(p, '*')){ member.type_star_count++; }
 }
 member.name = ep_id(p);
 ep_consume_semicolons(p);
}
inline void
parse_cpp_union_member(Ed_Parser *p, Meta_Union_Member &member)
{
 member.type = ep_id(p);
 member.name = ep_id(p);
 ep_consume_semicolons(p);
}

function void
print_struct_embed(Printer &p, String type_name,
                   arrayof<Meta_Struct_Member> &members)
{
 m_print_location;
 p<<"#define "<<type_name<<"_Embed \\\n union";
 m_macro_braces_sm{
  p<<"struct";
  m_macro_braces_sm{
   for_i32(imem, 0, members.count){
    auto &member = members[imem];
    p<<member.type<<" ";
    for_repeat(member.type_star_count){ p<<"*"; }
    p<<member.name<<";\\\n";
   }
  }
  p<<type_name<<" "<<type_name<<";";
 }
}

function b32
introspect_one_file(Arena *arena, File_Name_Data source, String outname) {
 b32 ok = true;
 Scratch_Block scratch;
 Token_List token_list = lex_full_input_cpp(scratch, source.data);
 Token_Iterator token_it = make_token_iterator(token_iterator(0, &token_list));
 Ed_Parser parser = make_ep_from_string(source.data, token_it, scratch);
 Ed_Parser *p = &parser;
 Printer printer = {};
 char newline = '\n';
 while( p->ok_ )
 {// NOTE(kv): Parsing loop
  if(ep_maybe_id(p, "introspect")){
   if(printer.FILE == 0){
    // NOTE(kv): Open the file, since we have something to write now
    FILE *outfile = fopen(outname, "wb");
    printer = make_printer_file(outfile);
    if (outfile) {
     if (meta_logging_level) {
      printf("Writing to file %.*s\n", string_expand(outname));
     }
    } else {
     ok = false;
     break;
    }
   }
   
   b32 do_info = false;
   b32 do_embed = false;
   {// NOTE: Parsing the type
    {//-NOTE: Introspection parameters
     m_paren_open(p);
     while (p->ok_ && !m_maybe_paren_close(p)){
      ep_maybe_char(p, ',');
      if      (ep_maybe_id(p, "info")) { do_info = true; }
      else if (ep_maybe_id(p, "embed")){ do_embed = true; }
      else {p->fail();}
     }
     ep_consume_semicolons(p);
    }
    
    if ( ep_maybe_id(p, "struct") )
    {//-NOTE struct case
     arrayof<Meta_Struct_Member> members = {};
     String type_name = ep_id(p);
     ep_char(p, '{');
     while(p->ok_ && !m_maybe_brace_close(p)){
      // NOTE: Field
      Meta_Struct_Member &member = members.push2();
      member = {};
      
      if (ep_maybe_id(p, "meta_removed")) {
       // NOTE(kv): meta_removed
       ep_char(p, '(');
       {
        parse_cpp_struct_member(p, member);
       }
       if(meta_maybe_key(p, "added")){
        member.version_added = ep_id(p);
        ep_maybe_char(p, ',');
       }
       {
        meta_parse_key(p, "removed");
        member.version_removed = ep_id(p);
        ep_maybe_char(p, ',');
       }
       if ( meta_maybe_key(p, "default") ) {
        member.default_value = ep_capture_until_char(p,')');
       } else {
        ep_char(p,')');
       }
       ep_consume_semicolons(p);
      }else{
       if(ep_maybe_id(p, "meta_added")){
        // NOTE(kv): meta_added
        ep_char(p, '(');
        while(p->ok_ && !ep_maybe_char(p, ')')){
         ep_maybe_char(p, ',');
         if (meta_maybe_key(p, "added")){
          member.version_added = ep_id(p);
         }else if (meta_maybe_key(p, "default")){
          //TODO(kv): support arbitrary expression in parens
          member.default_value = ep_print_token(p);
          ep_eat_token(p);
         }else{ p->fail(); }
        }
       }else if(ep_maybe_id(p, "meta_unserialized")){
        member.unserialized = true;
       }
       ep_consume_semicolons(p);
       
       parse_cpp_struct_member(p, member);
      }
     }
     
     if (do_info){
      print_struct_meta(printer, type_name, members);
     }
     if (do_embed){
      print_struct_embed(printer, type_name, members);
     }
    } else if (ep_maybe_id(p, "union")) {
     //-NOTE(kv) ;meta_parse_union
     arrayof<Meta_Union_Member> members = {};
     String type_name = ep_id(p);
     m_brace_open(p);
     
     String discriminator_type;
     {
      ep_id(p, "tagged_by");
      m_parens{ discriminator_type = ep_id(p); }
      ep_consume_semicolons(p);
     }
     
     while (p->ok_ && !m_maybe_brace_close(p)) {
      ep_id(p, "m_variant");
      Meta_Union_Member &member = members.push2();
      member = {};
      m_parens{ member.variant = ep_id(p); }
      parse_cpp_union_member(p, member);
     }
     
     if (do_info) {
      print_union_meta(printer, type_name, members, discriminator_type);
     }
    } else if ( ep_maybe_id(p, "enum") ) {
     //-NOTE(kv) enum
     arrayof<String> enum_values = {};
     String type_name = ep_id(p);
     m_brace_open(p);
     while ( p->ok_ && !m_maybe_brace_close(p) ) {
      //NOTE(kv) Enum value
      String &enumval = enum_values.push2();
      enumval = ep_id(p);
      ep_eat_until_char_simple(p, ',');  // NOTE(kv): the ending comma is optional, but we for it becuz that's how I roll
     }
     
     if (do_info){
      print_enum_meta(printer, type_name, enum_values);
     }
    } else { p->fail(); }
   }
  }
  else if (ep_at_eof(p)) { break; }
  else { ep_eat_token(p); }
 }
 
 ok = ok && p->ok_; 
 if (!ok) {
  Line_Column location = ep_get_fail_location(p);
  printf("introspect: parse error at %.*s:%d:%d",
         string_expand(source.name),
         location.line, location.column);
 }
 close_file(printer.FILE);
 return ok;
}

function b32
introspect_main(arrayof<File_Name_Data> source_files) {
 b32 ok = true;
 Scratch_Block scratch;
 
 for_i1(index,0,source_files.count) {
  if (!ok) { break; }
  
  auto source_file = source_files[index];
  String outname;
  {
   String path      = source_file.name;
   String dirname   = path_dirname(path);
   String extension = string_file_extension(path);
   String filename  = path_filename(path);
   String filename_no_extension = string_file_without_extension(filename);
   {
    Printer p = make_printer_buffer(scratch, 256);
    p << dirname << filename_no_extension << ".meta." << extension;
    outname = printer_get_string(p);
   }
  }
  
  ok = introspect_one_file(scratch, source_file, outname);
 }
 
 return ok;
}

//-