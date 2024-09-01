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

function void
print_type_meta(Printer &p, String type_name,
                arrayof<Meta_Struct_Member> type_members)
{
 Scratch_Block scratch;
 String type_global_var = get_type_global_var_name(scratch, type_name);
 String function_name = push_stringf(scratch, "get_%.*s",
                                     string_expand(type_global_var));
 i1 member_count = type_members.count;
 b32 is_struct = member_count != 0;
 b32 is_enum   = !is_struct;
 
 char newline = '\n';
 char *newlinex2 = "\n\n";
 
#define brace_block \
p << "\n{\n";  \
defer( p << "\n}\n"; );
 
 {//-NOTE: Function to generate the type info
  p << "function Type_Info " << function_name << "()";
  {
   brace_block;
   p << "Type_Info result = {};" << newline;
   p << "result.name = " << enclosed_in_strlit(type_name) << ";" << newline;
   p << "result.size = sizeof(" << type_name << ");" << newline;
   if (is_struct)
   {// note
    p << "result.members.set_count(" << member_count << ");" << newline;
    for_i1(member_index,0,member_count)
    {
     auto &member = type_members[member_index];
     String member_type = get_type_global_var_name(scratch, member.type);
     p << "result.members[" << member_index << "]" << " = " <<
      "Struct_Member{.type=&" << member_type << ", " <<
      ".name=" << enclosed_in_strlit(member.name) << ", " <<
      ".offset=offsetof(" << type_name << ", " << member.name << ")" <<
      "};" << newline;
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
  p << "function Type_Info &type_info_of_pointer(" << type_name << "*pointer)";
  {
   brace_block;
   p << "return " << type_global_var << ";";
  }
 }
 
 {// NOTE: ;meta_read
  p << "function void" << newline;
  p << get_type_read_function_name(scratch, type_name) <<
   "(Data_Reader &r, " << type_name << " &pointer)";
  {
   brace_block;
   p << "STB_Parser *p = r.parser;" << newline;
   auto &members = type_members;
   if (members.count)
   {// NOTE: struct
    p << "eat_char(p, '{');" << newline;
    for_i1(member_index,0,members.count)
    {
     auto &member = members[member_index];
     b32 has_version_added = member.version_added.len != 0;
     if (has_version_added)
     {
      p << "if (r.read_version >= " << member.version_added << ")";
     }
     {
      brace_block;
      p << "eat_id(p, " << enclosed_in_strlit(member.name) << ");" << newline <<
       get_type_read_function_name(scratch, member.type) << "(r, pointer." << member.name << ");";
     }
     if (has_version_added)
     {// NOTE(kv): The old version doesn't have the value, so we assign it the default value.
      p << "else";
      {
       brace_block;
       p << "pointer." << member.name << " = " << member.default_value << ";";
      }
     }
    }
    p << "eat_char(p, '}');";
   }
   else
   {// NOTE(kv): Just treat it like an integer (like f.ex an enum)
    // NOTE(kv): @meta_introspect_enum_size
    // If the type size is too BIG, we'll read past from wrong address.
    p << "i32 integer = eat_i1(p);" << newline <<
     "pointer = *(" << type_name << "*)(&integer);";
   }
  }
 }
 
 if (is_enum)
 {// NOTE ;meta_introspect_enum_size
  p << "static_assert( sizeof(" << type_name << ") <= sizeof(i32) );" << newlinex2;
 }
 
#undef brace_block
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
      if (ep_maybe_char(p, '}')) {
       break;
      } else {
       // NOTE: Field
       Meta_Struct_Member &member = type_members.push2();
       member = {};
       
       if ( ep_maybe_id(p, "meta_tag") )
       {// NOTE: (Optionally) The struct member tag
        ep_char(p, '(');
        while ( p->ok_ )
        {
         if ( ep_maybe_char(p, ')') ) {
          break;
         }
         
         if ( ep_maybe_id(p, "added") )
         {
          ep_char(p, '(');
          {
           ep_id(p, "since_version");
           ep_char(p, '=');
           member.version_added = ep_id(p);
           ep_char(p, ',');
          }
          {
           ep_id(p, "default");
           ep_char(p, '=');
           member.default_value = ep_id(p);
          }
          ep_char(p, ')');
         }
         else { p->fail(); }
        }
       }
       
       // NOTE: this is for the editor
       while ( ep_maybe_char(p, ';') ) {  }
       
       member.type = ep_id(p);
       member.name = ep_id(p);
       ep_char(p, ';');
      }
     }
    }
    else if ( ep_maybe_id(p, "enum") )
    {
     type_name = ep_id(p);
     // NOTE: That's it! No need to parse this type
    }
    else { p->fail(); }
   }
   
   if (p->ok_) { 
    print_type_meta(printer, type_name, type_members);
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
    Printer p = make_printer_arena(scratch, 256);
    p << dirname << filename_no_extension << ".meta." << extension;
    outname = printer_get_string(p);
   }
  }
  
  ok = introspect_one_file(scratch, source_file, outname);
 }
 
 return ok;
}

//-