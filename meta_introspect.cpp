
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

#if 0
inline void
parse_cpp_union_member(Ed_Parser *p, Meta_Union_Member &member){
 member.type = ep_id(p);
 while(ep_maybe_char(p, '*')){ member.type_star_count++; }
 member.name = ep_id(p);
 ep_consume_semicolons(p);
}
#endif

function b32
introspect_one_file(File_Name_Data source){
 b32 ok = true;
 Scratch_Block scratch;
 
 String outpath_base;
 {
  String path = source.name;
  String dir      = path_dirname(path);
  dir = pjoin(scratch, dir, "generated");
  String filename_ = path_filename(path);
  String filename_no_ext = path_no_extension(filename_);
  {
   Printer p = make_printer_buffer(scratch, 256);
   p<<dir<<OS_SLASH<<filename_no_ext<<".gen";
   outpath_base = printer_get_string(p);
  }
 }
 
 Token_List token_list = lex_full_input_cpp(scratch, source.data);
 Token_Iterator token_it = make_token_iterator(token_iterator(0, &token_list));
 Ed_Parser parser = make_ep_from_string(source.data, token_it);
 Ed_Parser *p = &parser;
 Printer_Pair printers;
 b32 has_opened_files = false;
 char newline = '\n';
 while(p->ok_ && ok){
  //NOTE(kv): Parsing loop
  if(ep_maybe_id(p, "introspect")){
   if(!has_opened_files){
    //NOTE(kv): Open files for writing, since there's something to write now.
    printers = m_open_files_to_write(outpath_base);
    has_opened_files = true;
    ok = (printers.h.FILE && printers.c.FILE);
   }
   
   if(ok){
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
     
     if(ep_maybe_id(p, "struct"))
     {//-NOTE struct case
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
      
      if(do_info){
       print_struct_meta(&printers, type_name, &members);
      }
      if(do_embed){
       print_struct_embed(&printers, type_name, members);
      }
     }else if(ep_maybe_id(p, "union")){
      //-NOTE(kv) ;meta_parse_union
#if 0
      arrayof<Meta_Union_Member> members = {};
      String type_name = ep_id(p);
      m_brace_open(p);
      
      String discriminator_type;
      {
       ep_id(p, "tagged_by");
       mpa_parens{ discriminator_type = ep_id(p); }
       ep_consume_semicolons(p);
      }
      
      while(p->ok_ && !m_maybe_brace_close(p)){
       ep_id(p, "m_variant");
       Meta_Union_Member &member = members.push2();
       member = {};
       mpa_parens{ member.variant = ep_id(p); }
       parse_cpp_union_member(p, member);
      }
      
      if(do_info){
       print_union_meta(&printers, type_name, members, discriminator_type);
      }
#endif
     }else if(ep_maybe_id(p, "enum")){
      //-NOTE(kv) enum
      arrayof<String> enum_names = {};
      String type_name = ep_id(p);
      m_brace_open(p);
      while ( p->ok_ && !m_maybe_brace_close(p) ) {
       //NOTE(kv) Enum value
       String &enumval = enum_names.push2();
       enumval = ep_id(p);
       ep_eat_until_char_simple(p, ',');  // NOTE(kv): the ending comma is optional, but we for it becuz that's how I roll
      }
      
      if(do_info){
       print_enum_meta(&printers, type_name, enum_names);
      }
     }else if(ep_maybe_id(p, "typedef")){
      //-NOTE(kv) typedef
      String typedef_to = ep_id(p);
      String type_name = ep_id(p);
      if(do_info){
       print_typedef_meta(&printers, type_name, typedef_to);
      }
     }else{ p->fail(); }
    }
   }
  }
  else if(ep_at_eof(p)){ break; }
  else{ ep_eat_token(p); }
 }
 
 ok = ok && p->ok_; 
 if (!ok) {
  Line_Column location = ep_get_fail_location(p);
  printf("introspect: parse error at %.*s:%d:%d",
         string_expand(source.name),
         location.line, location.column);
 }
 close_file(printers.c.FILE);
 close_file(printers.h.FILE);
 return ok;
}

function b32
introspect_main(arrayof<File_Name_Data> source_files){
 b32 ok = true;
 for_i1(index,0,source_files.count){
  auto source_file = source_files[index];
  ok = introspect_one_file(source_file);
  if(!ok){ break; }
 }
 return ok;
}

//-