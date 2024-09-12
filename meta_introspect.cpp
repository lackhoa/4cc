#include "4ed_kv_parser.cpp"
#include "meta_introspect.h"

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

function String
get_type_global_var_name(Arena *arena, String type_name)
{
 String result = push_stringf(arena, "Type_Info_%.*s",
                              string_expand(type_name));
 return result;
}
//
force_inline String
get_type_global_var_name(Arena *arena, Type_Info &type) {
 return get_type_global_var_name(arena, type.name);
}

function String
get_type_read_function_name(Arena *arena, String type_name)
{
 String result = push_stringf(arena, "read_%.*s",
                              string_expand(type_name));
 return result;
}

inline void
meta__parse_key(Ed_Parser *p, char *key)
{
 ep_id(p, key); ep_char(p, '=');
}
inline b32
meta__maybe_key(Ed_Parser *p, char *key)
{
 b32 result = false;
 if ( ep_maybe_id(p, key) ) {
  ep_char(p, '=');
  result = p->ok_;
 }
 return result;
}

inline void
meta__consume_semicolons(Ed_Parser *p)
{//NOTE(kv) We may add semicolon for editor indentation.
 while ( ep_maybe_char(p, ';') ) {  }
}

#if 0
// NOTE(kv)  sample read code
Bezier_Data
{
 STB_Parser *p = r.parser;
 eat_char(p, '{');
 
 {
  eat_id(p, strlit("name"));
  read_String(r, pointer.name);
 }
 
 i1 _object_index = 0;
 if (r.read_version >= Version_Init &&
     r.read_version < Version_Rename_Object_Index)
 {
  eat_id(p, strlit("object_index"));
  read_i1(r, _object_index);
 }
 
 i1 coframe_index = object_index;
 if (r.read_version >= Version_Rename_Object_Index)
 {
  eat_id(p, strlit("coframe_index"));
  read_i1(r, coframe_index);
 }
 pointer.coframe_index = coframe_index;
 
 {
  eat_id(p, strlit("symx"));
  read_i1(r, pointer.symx);
 }
 
 Bezier_Type _type = Bezier_Type_Raw;
 if (r.read_version >= Version_Add_Bezier_Type)
 {
  eat_id(p, strlit("type"));
  read_Bezier_Type(r, _type);
 }
 pointer.type = _type;
 
 {
  eat_id(p, strlit("p0_index"));
  read_i1(r, pointer.p0_index);
 }
 
 {
  eat_id(p, strlit("p1"));
  read_v3(r, pointer.p1);
 }
 
 {
  eat_id(p, strlit("p2"));
  read_v3(r, pointer.p2);
 }
 
 {
  eat_id(p, strlit("p3_index"));
  read_i1(r, pointer.p3_index);
 }
 
 {
  eat_id(p, strlit("radii"));
  read_v4(r, pointer.radii);
 }
 eat_char(p, '}');
}
#endif

inline b32
member_was_removed(Meta_Struct_Member &member) {
 return member.version_removed.len != 0;
}

function void
print_type_meta(Printer &p, String type_name,
                arrayof<Meta_Struct_Member> members,
                arrayof<String> enum_values)
{
 Scratch_Block scratch;
 String type_global_var = get_type_global_var_name(scratch, type_name);
 //NOTE(kv) e.g get_Type_Info_Blah
 String function_name = push_stringf(scratch, "get_%.*s",
                                     string_expand(type_global_var));
 b32 is_struct = members.count     != 0;
 b32 is_enum   = enum_values.count != 0;
 
 char newline = '\n';
 char *newlinex2 = "\n\n";
 
#define brace_block \
p << "\n{\n";  \
defer( p << "\n}\n"; );
 
 {//-NOTE: Function to generate the type info
  p << "function Type_Info\n" << function_name << "()";
  {
   brace_block;
   p << "Type_Info result = {};" << newline;
   p << "result.name = " << enclosed_in_strlit(type_name) << ";" << newline;
   p << "result.size = sizeof(" << type_name << ");" << newline;
   if (is_struct) {
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
     if ( !member_was_removed(member) ) {
      String member_type = get_type_global_var_name(scratch, member.type);
      p << "result.members[" << member_index << "] = " <<
       "{.type=&" << member_type << ", " <<
       ".name=" << enclosed_in_strlit(member.name) << ", " <<
       ".offset=offsetof(" << type_name << ", " << member.name << ")" <<
       "};\n";
      member_index++;
     }
    }
   } else if (is_enum) {
    // NOTE
    p << "result.enum_values.set_count(" << enum_values.count << ");\n";
    for_i1(enum_index,0,enum_values.count) {
     String enum_value = enum_values[enum_index];
     p << "result.enum_values[" << enum_index << "] = " <<
      "{.name=" << enclosed_in_strlit(enum_value) << ", " <<
      ".value=" << enum_value <<
      "};\n";
    }
   }
   p << "return result;";
  }
 }
 
 {// NOTE: The global variable
  p << "global Type_Info " << type_global_var << " = " <<
   function_name << "();" << newlinex2;
 }
 
 {// NOTE: The overload to automatically get the type info from a pointer of that type
  p << "function Type_Info &type_info_from_pointer(" << type_name << "*pointer)";
  {
   brace_block;
   p << "return " << type_global_var << ";";
  }
 }
 
 {// NOTE: ;meta_read
  p << "function void" << newline;
  p << get_type_read_function_name(scratch, type_name) <<
   "(Data_Reader &r, " << type_name << " &pointer)";
  {brace_block;
   p << "STB_Parser *p = r.parser;" << newline;
   if (members.count) {
    // NOTE(kv) Struct
    p << "eat_char(p, '{');" << newline;
    for_i1(member_index,0,members.count) {
     auto &member = members[member_index];
     
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
      p << "if ( in_range_exclusive("<<
       version_added<<
       ", r.read_version, "<<
       version_removed<<") )";
     }
     {//NOTE(kv) Read data to the local var
      brace_block;
      p << "eat_id(p, " << enclosed_in_strlit(member.name) << ");\n" <<
       get_type_read_function_name(scratch, member.type) << "(r, " << varname << ");";
     }
     if ( member_currently_exists ) {
      //NOTE(kv) Assign the local var to the dest struct member.
      p<<"pointer."<<member.name<<" = "<<varname<<";\n\n";
     }
    }
    p << "eat_char(p, '}');";
   } else {
    // NOTE(kv): Enum
    // NOTE(kv): @meta_introspect_enum_size
    // If the type size is too BIG, we'll read past from wrong address.
    p << "i32 integer = eat_i1(p);\n" <<
     "pointer = *(" << type_name << "*)(&integer);";
   }
  }
 }
 
 if (is_enum)
 {// NOTE ;meta_introspect_enum_size
  p << "static_assert( sizeof(" << type_name << ") <= sizeof(i32) );\n\n";
 }
 
#undef brace_block
}

inline void
parse_struct_member_without_tag(Ed_Parser *p, String &type, String &name)
{// todo(kv): We're still cheesing this pretty hard
 type = ep_id(p);
 name = ep_id(p);
 meta__consume_semicolons(p);
}

function b32
introspect_one_file(Arena *arena, File_Name_Data source, String outname)
{
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
  if ( ep_maybe_id(p, "introspect") )
  {
   if (printer.FILE == 0)
   {// NOTE(kv): Open the file, since we have something to write now
    FILE *outfile = open_file(outname, "wb");
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
   
   String type_name = {};
   arrayof<Meta_Struct_Member> type_members = {};
   arrayof<String> enum_values = {};
   
   {// NOTE: Parsing the type
    {//-NOTE: Introspection parameters
     ep_eat_kind(p, TokenBaseKind_ParenOpen);
     while (p->ok_) {
      if (ep_maybe_kind(p, TokenBaseKind_ParenClose)) {
       break;
      }
     }
    }
    
    if ( ep_maybe_id(p, "struct") )
    {//-NOTE struct case
     type_name = ep_id(p);
     type_members.set_cap_min(4);
     ep_char(p, '{');
     while(p->ok_)
     {
      if ( ep_maybe_char(p, '}') ) { break; }
      
      // NOTE: Field
      Meta_Struct_Member &member = type_members.push2();
      member = {};
      
      if ( ep_maybe_id(p, "meta_removed") ) {
       // NOTE(kv): meta_removed
       ep_char(p, '(');
       {
        parse_struct_member_without_tag(p, member.type, member.name);
       }
       if ( meta__maybe_key(p, "added") ) {
        member.version_added = ep_id(p);
        ep_maybe_char(p, ',');
       }
       {
        meta__parse_key(p, "removed");
        member.version_removed = ep_id(p);
        ep_maybe_char(p, ',');
       }
       if ( meta__maybe_key(p, "default") ) {
        member.default_value = ep_capture_until_char(p,')');
       } else {
        ep_char(p,')');
       }
       meta__consume_semicolons(p);
      } else {
       if ( ep_maybe_id(p, "meta_added") ) {
        // NOTE(kv): meta_added
        ep_char(p, '(');
        // NOTE(kv): added tag
        {
         meta__parse_key(p, "added");
         member.version_added = ep_id(p);
         ep_char(p, ',');
        }
        if ( meta__maybe_key(p, "default") ) {
         member.default_value = ep_capture_until_char(p,')');
        } else {
         ep_char(p,')');
        }
       }
       meta__consume_semicolons(p);
       
       parse_struct_member_without_tag(p, member.type, member.name);
      }
     }
    } else if ( ep_maybe_id(p, "enum") ) {
     // NOTE(kv): parsing enum
     type_name = ep_id(p);
     ep_char(p, '{');
     while ( p->ok_ && !ep_maybe_char(p, '}') ) {
      // NOTE(kv): Enum value
      String &enumval = enum_values.push2();
      enumval = ep_id(p);
      ep_eat_until_char_simple(p, ',');  // NOTE(kv): the comma is optional bug screw it!
     }
    }
    else { p->fail(); }
   }
   
   if (p->ok_) {
    print_type_meta(printer, type_name, type_members, enum_values);
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
introspect_main(arrayof<File_Name_Data> source_files)
{
 b32 ok = true;
 Scratch_Block scratch;
 
 for_i1(index,0,source_files.count)
 {
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