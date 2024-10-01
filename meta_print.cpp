
function Printer
m_open_file_to_write(Stringz filename){
 FILE *file = open_or_create_file(filename, "wb");
 if(file){
  meta_logf("Writing to file %.*s\n", strexpand(filename));
 }else{
  meta_logf("Failed to open file %.*s\n with errno: %d", strexpand(filename), errno);
 }
 return make_printer_file(file);
}
function Printer_Pair
m_open_files_to_write(String base_path,
                      const char *call_file=__builtin_FILE(),
                      u32 call_line=__builtin_LINE()){
 Scratch_Block scratch;
 Stringz outname_c, outname_h;
 {
  Printer p = make_printer_buffer(scratch, base_path.len+3);
  p<<base_path<<".h";
  outname_h = printer_get_string(p);
 }
 {
  Printer p = make_printer_buffer(scratch, base_path.len+5);
  p<<base_path<<".cpp";
  outname_c = printer_get_string(p);
 }
 Printer_Pair result = {};
 result.h = m_open_file_to_write(outname_h);
 result.c = m_open_file_to_write(outname_c);
#define M  p<<"//NOTE Generated at "<<call_file<<':'<<call_line<<":\n"
 if(FILE *hfile = result.h.FILE){
  Printer p = make_printer_file(hfile);
  M;
  p<<"#pragma once\n";
 }
 if(FILE *hfile = result.c.FILE){
  Printer p = make_printer_file(hfile);
  M;
 }
#undef M
 return result;
}
function void
m_close_pair(Printer_Pair &ps){
 fclose(ps.h.FILE);
 fclose(ps.c.FILE);
}

function Meta_Type_Names &
m_get_type_names(String type_name){
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
inline String
get_type_global_info_name(String type_name){
 return m_get_type_names(type_name).global_info_name;
}
inline String
get_type_read_function_name(String type_name){
 return m_get_type_names(type_name).read_function_name;
}
inline String
get_type_info_function_name(String type_name){
 return m_get_type_names(type_name).info_function_name;
}

function void
print_type_meta_shared(Printer_Pair &ps, String type_name, b32 is_typedef=false){
 String type_global_var = get_type_global_info_name  (type_name);
 String function_name   = get_type_info_function_name(type_name);
 {
  Printer &p = ps.h;
  m_location;
  //NOTE ("The global type info variable")
  p<<"global_decl Type_Info "<<type_global_var<<";\n";
 }
 {
  Printer &p = ps.c;
  {
   m_location;
   p<<"global Type_Info "<<type_global_var<<" = "<<function_name<<"();\n\n";
  }
  if(!is_typedef)
  {
   // NOTE: Overload to automatically get the type info from a pointer of that type
   p<<"function Type_Info &type_info_from_pointer("<<type_name<<"*pointer)";
   m_braces{
    p<<"return "<<type_global_var<<";";
   }
  }
 }
}
function void
print_type_read_function_prototype(Printer &p, String type_name){
 p<<"function void\n";
 p<<get_type_read_function_name(type_name)<<
  "(Data_Reader &r, " << type_name << " &pointer)";
}
function void
print_union_read_function_prototype(Printer &p, String type_name, String discriminator_type){
 p<<"function void\n"<<
  get_type_read_function_name(type_name)<<
  "(Data_Reader &r, "<<
  type_name<<" &pointer, "<<
  discriminator_type<<" variant"<<")";
}
function void
print_type_info_function_prototype(Printer &p, String type_name){
 String function_name = get_type_info_function_name(type_name);
 p<<"function Type_Info\n"<<function_name<<"()";
}
function void
print_struct_body(Printer &p, Meta_Struct_Members &members){
 m_braces_sm{
  for_i32(i,0,members.count){
   auto &member = members.items[i];
   if(!member_was_removed(member)){
    if(i!=0){ p<<"\n"; }
    
    p<<member.type<<" ";
    for_repeat(member.type_star_count){ p<<"*"; }
    
    p<<member.name<<";";
   }
  }
 }
}
function void
print_struct(Printer &p, String type_name, Meta_Struct_Members &members){
 p<<"struct "<<type_name;
 print_struct_body(p,members);
}
function void
print_struct_meta(Printer_Pair &ps, String type_name,
                  Meta_Struct_Members &members){
 Scratch_Block scratch;
#define brace_block  p << "\n{\n"; defer( p << "\n}\n"; );
 {//-NOTE ("Function to generate the type info")
  String function_name = get_type_info_function_name(type_name);
  {//NOTE .h
   Printer &p = ps.h;
   m_location;
   p<<"struct "<<type_name<<";\n";
   p<<"function Type_Info\n"<<function_name<<"();";
  }
  {//NOTE .cpp
   Printer &p = ps.c;
   m_location;
   p<<"function Type_Info\n"<<function_name<<"()";
   {brace_block;
    p<<"Type_Info result = {};\n";
    p<<"result.name = "<<enclosed_in_strlit(type_name) << ";\n";
    p<<"result.size = sizeof("<<type_name<<");\n";
    p<<"result.kind = Type_Kind_Struct;\n";
    // NOTE(kv) Computing member count
    i32 member_count = 0;
    for_i32(raw_member_index,0,members.count){
     if(!member_was_removed(members.get(raw_member_index))){
      member_count++;
     }
    }
    
    p << "result.members.set_count(" << member_count << ");\n";
    i32 member_index = 0;
    for_i1(raw_member_index,0,members.count){
     auto &member = members.get(raw_member_index);
     if (!member_was_removed(member)) {
      String member_type = get_type_global_info_name(member.type);
      p<<"result.members["<<member_index<<"] = "<<
       "{.type=&"<<member_type<<
       ", .name="<<enclosed_in_strlit(member.name)<<
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
 }
 
 print_type_meta_shared(ps, type_name);
 
 {//-NOTE: ;meta_read_struct
  {//NOTE .h
   Printer &p = ps.h;
   m_location;
   print_type_read_function_prototype(p,type_name);
   p<<";\n";
  }
  {//NOTE .cpp
   Printer &p = ps.c;
   m_location;
   print_type_read_function_prototype(p,type_name);
   {brace_block;
    p << "STB_Parser *p = r.parser;\n";
    
    p << "eat_char(p, '{');\n";
    for_i1(member_index,0,members.count){
     Meta_Struct_Member &member = members.get(member_index);
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
 }
#undef brace_block
}

function void
print_union_meta(Printer_Pair &ps, String type_name,
                 arrayof<Union_Variant> *variants,
                 String discriminator_type){
#define brace_block  p << "\n{\n"; defer( p << "\n}\n"; );
 {//-NOTE: ("Function to generate the type info")
  {
   Printer &p = ps.h;
   m_location;
   p<<"union "<<type_name<<";\n";
   print_type_info_function_prototype(p,type_name);
   p<<";\n";
  }
  {
   Printer &p = ps.c;
   m_location;
   print_type_info_function_prototype(p,type_name);
   {brace_block;
    p << "Type_Info result = {};\n" <<
     "result.name = "<<enclosed_in_strlit(type_name)<<";\n" <<
     "result.size = sizeof("<<type_name<<");\n"<<
     "result.kind = Type_Kind_Union;\n"<<
     "result.discriminator_type = &"<<get_type_global_info_name(discriminator_type)<<";\n";
    {
     p<<"result.union_members.set_count("<<variants->count<<");\n";
     for_i32(index,0,variants->count){
      auto &variant = variants->get(index);
      String variant_type = get_type_global_info_name(variant.struct_name);
      p<<"result.union_members["<<index<<"] = {"<<
       ".type=&"<<variant_type<<", "<<
       ".name="<<enclosed_in_strlit(variant.name)<<", "<<
       ".variant="<<variant.enum_name<<
       "};\n";
     }
    }
    p<<"return result;";
   }
  }
 }
 
 print_type_meta_shared(ps, type_name);
 
 {// NOTE: ;read union
  {
   Printer &p = ps.h;
   m_location;
   print_union_read_function_prototype(p,type_name,discriminator_type);
   p<<";\n";
  }
  {
   Printer &p = ps.c;
   m_location;
   print_union_read_function_prototype(p,type_name,discriminator_type);
   
   {brace_block;
    p<<"STB_Parser *p = r.parser;\n";
    //NOTE(kv) When we add/remove union members, this is gonna get complicated,
    //  but right now I don't care.
    p<<"switch(variant)";
    {brace_block;
     for_i1(vi,0,variants->count){
      auto &variant = variants->get(vi);
      p<<"case "<<variant.enum_name<<":";
      {brace_block;
       p<<get_type_read_function_name(variant.struct_name)<<
        "(r, "<<"pointer."<<variant.name<<");\n"<<
        "break;";
      }
     }
    }
   }
  }
 }
#undef brace_block
}
function void
print_enum(Printer &p, String type_name,
           arrayof<String> &enum_names,
           arrayof<i32>    &enum_vals){
 m_location;
 p<<"enum "<<type_name;
 m_braces_sm{
  for_i32(ei,0,enum_names.count){
   if(ei!=0){ p<<"\n"; }
   p<<enum_names[ei]<<" = "<<enum_vals[ei]<<",";
  }
 }
}
function void
print_enum_meta(Printer_Pair &ps, String type_name,
                arrayof<String> &enum_names){
 {//-NOTE: ("Function to generate the type info")
  String function_name = get_type_info_function_name(type_name);
  {//NOTE .h
   Printer &p = ps.h;
   m_location;
   p<<"function Type_Info\n"<<function_name<<"();";
  }
  {//NOTE .cpp
   Printer &p = ps.c;
   m_location;
   p<<"function Type_Info\n"<<function_name<<"()";
   m_braces{
    p<<"Type_Info result = {};\n";
    p<<"result.name = "<<enclosed_in_strlit(type_name)<<";\n";
    p<<"result.size = sizeof("<<type_name<<");\n";
    p<<"result.kind = Type_Kind_Enum;\n";
    p<<"result.enum_members.set_count("<<enum_names.count<<");\n";
    for_i1(enum_index,0,enum_names.count){
     String name = enum_names.get(enum_index);
     p<<"result.enum_members["<<enum_index<<"] = "<<
      "{.name="<<enclosed_in_strlit(name)<<", "<<
      ".value="<<name<<  //NOTE(kv) Let the compiler fill out the value.
      "};\n";
    }
    p<<"return result;";
   }
  }
 }
 
 print_type_meta_shared(ps, type_name);
 
 {// NOTE: meta_read
  {
   Printer &p = ps.h;
   m_location;
   print_type_read_function_prototype(p,type_name);
   p<<";\n";
  }
  {
   Printer &p = ps.c;
   m_location;
   print_type_read_function_prototype(p,type_name);
   m_braces{
    p << "STB_Parser *p = r.parser;\n";
    // NOTE(kv): @meta_introspect_enum_size
    //   If the type size is too BIG, we'll read past from wrong address.
    p << "i32 integer = eat_i1(p);\n" <<
     "pointer = *(" << type_name << "*)(&integer);";
   }
  }
 }
 
 {// NOTE ;meta_introspect_enum_size
  ps.h << "static_assert( sizeof(" << type_name << ") <= sizeof(i32) );\n\n";
 }
}

function void
print_typedef_meta(Printer_Pair &ps, String type_name, String typedef_to){
 {//-NOTE: Function to generate type info
  String function_name = get_type_info_function_name(type_name);
#define PROTOTYPE  p<<"function Type_Info\n"<<function_name<<"()"
  {
   Printer &p = ps.h;
   m_location;
   PROTOTYPE<<";";
  }
  {
   Printer &p = ps.c;
   m_location;
   PROTOTYPE;
   m_braces{
    p<<"Type_Info result = "<<get_type_global_info_name(typedef_to)<<";\n";
    p<<"result.name = "<<enclosed_in_strlit(type_name)<<";\n";
    p << "return result;";
   }
  }
#undef PROTOTYPE
 }
 
 print_type_meta_shared(ps, type_name, true);
 
 {//NOTE meta read
  {
   Printer &p = ps.h;
   m_location;
   print_type_read_function_prototype(p,type_name);
  }
  {
   Printer &p = ps.c;
   m_location;
   print_type_read_function_prototype(p,type_name);
   m_braces{
    p<<get_type_read_function_name(typedef_to)<<"(r, pointer);";
   }
  }
 }
}
function void
print_struct_embed(Printer &p, String type_name,
                   arrayof<Meta_Struct_Member> &members){
 m_location;
 p<<"#define "<<type_name<<"_Embed \\\n union";
 m_macro_braces_sm{
  p<<"struct";
  m_macro_braces_sm{
   for_i1(imem, 0, members.count){
    auto &member = members[imem];
    p<<member.type<<" ";
    for_repeat(member.type_star_count){ p<<"*"; }
    p<<member.name<<";\\\n";
   }
  }
  p<<type_name<<" "<<type_name<<";";
 }
}
function void
print_i1_wrapper(Printer &p, Printer_Pair &ps_meta, String type_name){
 Scratch_Block scratch;
 Meta_Struct_Members members = parse_struct_body(scratch, "{ i1 v; }");
 {
  print_struct(p, type_name, members);
  p<<"inline b32 operator==("<<type_name<<" a, "<<type_name<<" b)"<<
   "{ return a.v==b.v; }\n";
 }
 print_struct_meta(ps_meta, type_name, members);
}
inline void
print_i1_wrapper(Printer &p, Printer_Pair &ps_meta, char *type_name){
 print_i1_wrapper(p, ps_meta, SCu8(type_name));
}

//-