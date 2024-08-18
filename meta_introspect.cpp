#include "4ed_kv_parser.cpp"

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

function void
print_struct_info(Printer &p, Type_Info &struct_info)
{
 Scratch_Block scratch;
 String info_name = get_type_global_var_name(scratch, struct_info);
 String function_name = push_stringf(scratch, "get_%.*s",
                                     string_expand(info_name));
#define newline  print(p, "\n")
 {// NOTE: The generator function
  printn2(p, "function Type_Info ", function_name);
  p << "()"; newline;
  print(p, "{");  newline;
  print(p, "Type_Info result = {};"); newline;
  p << "result.name = " << enclosed_in_strlit(struct_info.name) << ";"; newline;
  p << "result.size = sizeof(" << struct_info.name << ");"; newline;
  {// members
   auto member_count = struct_info.members.count;
   p << "result.members.set_count(" << member_count << ");"; newline;
   for_i1(member_index,0,member_count)
   {
    auto &member = struct_info.members[member_index];
    String member_type = get_type_global_var_name(scratch, *member.type);
    p << "result.members[" << member_index << "]" << " = " <<
     "Struct_Member{.type=&" << member_type << ", " <<
     ".name=" << enclosed_in_strlit(member.name) << ", " <<
     ".offset=offsetof(" << struct_info.name << ", " << member.name << ")" <<
     "};";
    newline;
   }
  }
  p << "return result;"; newline;
  print(p, "}"); newline; newline;
 }
 {// NOTE: The global variable
  p << "global Type_Info " << struct_info.name << "_Type_Info = " << function_name << "();";
 }
#undef newline
}

function b32
introspect_one_file(Arena *arena, String source, String outname)
{
 b32 ok = true;
 Scratch_Block scratch;
 Token_List token_list = lex_full_input_cpp(scratch, source);
 Token_Iterator token_it = make_token_iterator(token_iterator(0, &token_list));
 Ed_Parser parser = make_ep_from_string(source, token_it);
 Ed_Parser *p = &parser;
 Printer r = {};
#define newline  print(r, "\n")
 while(p->ok_)
 {// NOTE(kv): Parsing
  if ( ep_maybe_id(p, "introspect") )
  {
   if (r.FILE == 0)
   {// NOTE(kv): Open the file, since we have something to write now
    FILE *outfile = open_file(outname, "wb");
    r = make_printer(outfile);
    if (outfile) {
     if (meta_logging_level) {
      printf("Writing to file %.*s\n", string_expand(outname));
     }
    } else {
     ok = false;
     break;
    }
   }
   
   {// NOTE: Parsing the struct
    Type_Info struct_info = {};
    struct_info.members.set_cap_min(4);
    ep_eat_kind(p, TokenBaseKind_ParenOpen);
    while(p->ok_) {
     if (ep_maybe_kind(p, TokenBaseKind_ParenClose)) {
      break;
     }
    }
    ep_eat_id(p, "struct");
    struct_info.name = ep_eat_id(p);
    ep_eat_kind(p, TokenBaseKind_ScopeOpen);
    while(p->ok_)
    {
     if (ep_maybe_kind(p, TokenBaseKind_ScopeClose)) {
      break;
     } else {
      // NOTE: Field
      Struct_Member &member = struct_info.members.push2();
      {// NOTE: The type
       // NOTE(kv): Member type has the name only, we don't need to know more.
       push_value(member.type, scratch, (Type_Info{.name = ep_eat_id(p),}));
       if (!member.type) {
        p->set_ok(false);
       }
      }
      {// NOTE: The name
       member.name = ep_eat_id(p);
       ep_eat_char(p, ';');
      }
     }
    }
    
    if (p->ok_) {
     print_struct_info(r, struct_info);
    }
   }
  }
  else if (ep_at_eof(p)) {
   break;
  } else {
   ep_eat_token(p);
  }
 }
#undef newline
 
 ok = ok && p->ok_;
 if (!ok) {
  breakhere;
 }
 close_file(r.FILE);
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
    Printer p = make_printer(scratch, 256);
    printn4(p, dirname, filename_no_extension, ".meta.", extension);
    outname = printer_get_string(p);
   }
  }
  
  ok = introspect_one_file(scratch, source_file.data, outname);
 }
 
 return ok;
}

//-