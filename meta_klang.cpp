//-
function void
generate_bezier_types(Printer &p, Printer_Pair &ps_meta);

function b32
k_process_file(File_Name_Data source){
 Scratch_Block scratch;
 Stringz hpp_path, meta_base;
 {
  String path = source.name;
  String dir  = path_dirname(path);
  dir = pjoin(scratch, dir, "generated");
  String filename_ = path_filename(path);
  String filename_no_ext = path_no_extension(filename_);
  {
   Printer pr = make_printer_buffer(scratch, 256);
   pr<<dir<<OS_SLASH<<filename_no_ext<<".gen.h";
   hpp_path = printer_get_string(pr);
  }
  {
   Printer pr = make_printer_buffer(scratch, 256);
   pr<<dir<<OS_SLASH<<filename_no_ext<<"_meta.gen";
   meta_base = printer_get_string(pr);
  }
 }
 Printer      ph      = m_open_file_to_write(hpp_path);
 {
  ph<<"#pragma once\n";
  ph<<"// NOTE: source: "<<source.name<<"\n";
 }
 Printer_Pair ps_meta = m_open_files_to_write(meta_base);
 b32 ok = okp(ph) && okp(ps_meta);
 
 Ed_Parser parser = m_parser_from_string(scratch, source.data);
 Ed_Parser *p = &parser;
 
 ep_consume_semicolons(p);
 while(ok && p->ok_){
  //-NOTE(kv): Parsing loop
  if(ep_at_eof(p)){
   break;
  }else{
   b32 do_info  = true;
   b32 do_embed = false;
   {//-NOTE: Introspection parameters
    if(m_maybe_bracket_open(p)){
     while(p->ok_ && !m_maybe_bracket_close(p)){
      ep_maybe_char(p, ',');
      if     (ep_maybe_id(p, "noinfo")){ do_info = false; }
      else if(ep_maybe_id(p, "embed")){ do_embed = true; }
      else{ p->fail(); }
     }
    }
    ep_consume_semicolons(p);
   }
   
   if(ep_maybe_id(p, "struct")){
    //-NOTE struct case
    arrayof<Meta_Struct_Member> members = {};
    String type_name = ep_id(p);
    ep_char(p, '{');
    while(p->ok_ && !m_maybe_brace_close(p)){
     // NOTE: Field
     Meta_Struct_Member *member = &members.push2();
     *member = {};
     
     if (ep_maybe_id(p, "meta_removed")) {
      // NOTE(kv): meta_removed
      ep_char(p, '(');
      {
       parse_struct_member(p, member);
      }
      if(meta_maybe_key(p, "added")){
       member->version_added = ep_id(p);
       ep_maybe_char(p, ',');
      }
      {
       meta_parse_key(p, "removed");
       member->version_removed = ep_id(p);
       ep_maybe_char(p, ',');
      }
      if ( meta_maybe_key(p, "default") ) {
       member->default_value = ep_capture_until_char(p,')');
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
         member->version_added = ep_id(p);
        }else if (meta_maybe_key(p, "default")){
         //TODO(kv): support arbitrary expression in parens
         member->default_value = ep_print_token(p);
         ep_eat_token(p);
        }else{ p->fail(); }
       }
      }else if(ep_maybe_id(p, "meta_unserialized")){
       member->unserialized = true;
      }
      ep_consume_semicolons(p);
      
      parse_struct_member(p, member);
     }
    }
    
    print_struct(ph, type_name, members);
    if(do_info){
     print_struct_meta(ps_meta, type_name, members);
    }
    if(do_embed){
     print_struct_embed(ph, type_name, members);
    }
   }else if(ep_maybe_id(p, "union")){
    //-NOTE(kv) Union: We don't have that right now
   }else if(ep_maybe_id(p, "enum")){
    //-NOTE(kv) enum
    arrayof<String> enum_names = {};
    arrayof<i1> enum_vals      = {};
    String type_name = ep_maybe_id(p);
    m_brace_open(p);
    while(p->ok_ && !m_maybe_brace_close(p)){
     //NOTE(kv) Enum value
     enum_names.push(ep_id(p));
     ep_char(p, '=');
     enum_vals.push(ep_i1(p));
     ep_eat_until_char_simple(p, ',');  // NOTE(kv) The ending comma is optional, but I don't care.
    }
    
    print_enum(ph, type_name, enum_names, enum_vals);
    if(type_name.len!=0 && do_info){
     //NOTE(kv) Anonymous enums can't be read, since it can't be referred to.
     print_enum_meta(ps_meta, type_name, enum_names);
    }
   }else if(ep_maybe_id(p, "typedef")){
    //-NOTE(kv) typedef
    String typedef_to = ep_id(p);
    String type_name  = ep_id(p);
    {
     ph<<"typedef "<<typedef_to<<" "<<type_name<<";\n";
    }
    if(do_info){
     print_typedef_meta(ps_meta, type_name, typedef_to);
    }
   }else if(ep_maybe_id(p, "i1_wrapper")){
    //-NOTE i1_wrapper
    mpa_parens{
     String type_name = ep_id(p);
     print_i1_wrapper(ph, ps_meta, type_name);
    }
   }else if(ep_maybe_id(p, "unique")){
    //-NOTE one-off/miscellaneous stuff
    if(ep_maybe_id(p, "Bezier_Type")){
     generate_bezier_types(ph, ps_meta);
    }
   }else{
    ep_eat_token(p);
   }
  }
  ep_consume_semicolons(p);
 }
 
 if(!p->ok_){
  ok = false;
  Line_Column location = ep_get_fail_location(p);
  printf("klang: parse error at %.*s:%d:%d\n",
         string_expand(source.name),
         location.line, location.column);
 }
 
 close_file(ph);
 close_pair(ps_meta);
 return ok;
}

inline void
k_print_struct(Printer &p, K_Struct struc){
 print_struct(p, struc.name, struc.members);
}
inline void
k_print_struct_meta(Printer_Pair &ps, K_Struct struc){
 print_struct_meta(ps, struc.name, struc.members);
}

function b32
klang_main(arrayof<File_Name_Data> source_files){
 b32 ok = true;
 for_i1(index,0,source_files.count){
  auto source_file = source_files[index];
  if(path_extension(source_file.name) == strlit("kh")){
   ok = k_process_file(source_file);
   if(!ok){ break; }
  }
 }
 return ok;
}

//-