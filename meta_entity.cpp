function void
push_entity_variant_inner(arrayof<Union_Variant> *variants, Union_Variant &variant){
 Arena *arena = &meta_permanent_arena;
 variant.enum_name   = strcat(arena, "Curve_Type_", variant.name);
 variant.struct_name = strcat(arena, "Curve_",      variant.name);
 variants->push(variant);
 macro_clamp_min(max_entity_enum, variant.enum_value);
}
function void
push_curve_variant_with_endpoints(Arena *arena, arrayof<Union_Variant> *variants,
                                  i32 enum_value, String name, String name_lower,
                                  String struct_members){
 Union_Variant variant = {};
 variant.enum_value = enum_value;
 variant.name = name;
 variant.name_lower = name_lower;
 {
  Scratch_Block scratch;
  M_Struct_Members &members = variant.struct_members;
  Ed_Parser parser = m_parser_from_string(scratch, struct_members);
  members = parse_struct_body(arena, &parser);
  {
   members.push_first(struct_member_from_string("Vertex_Index p0"));
   members.push(struct_member_from_string("Vertex_Index p3"));
  }
 }
 push_entity_variant_inner(variants, variant);
 m_entity_variant_info_table[enum_value].is_curve = true;
}
function void
push_entity_variant(Arena *arena, arrayof<Union_Variant> *variants,
                    i32 enum_value, String name, String name_lower,
                    String struct_members, M_Entity_Variant_Info type_info){
 Union_Variant variant = {};
 variant.enum_value = enum_value;
 variant.name = name;
 variant.name_lower = name_lower;
 {
  Scratch_Block scratch;
  Ed_Parser parser = m_parser_from_string(scratch, struct_members);
  variant.struct_members = parse_struct_body(arena, &parser);
 }
 push_entity_variant_inner(variants, variant);
 m_entity_variant_info_table[enum_value] = type_info;
}
function b32
entity_variant_is_curve(Union_Variant &variant){
 return m_entity_variant_info_table[variant.enum_value].is_curve;
}
function void
generate_entity_types(Printer &printer){
 Scratch_Block scratch;
 arrayof<Union_Variant> variants = {};
 //-NOTE @data of the variants
 //TODO(kv) I'd just pass the whole thing as a string, and be done with it!
 //  making macros is just so.damn.messy!
#define X(enum_value, name, name_lower, struct_members) \
push_curve_variant_with_endpoints(scratch, &variants, \
enum_value, strlit(#name), strlit(#name_lower), strlit(#struct_members))
 
 X(11, v3v2,     v3v2,     { v3 d0; v2 d3; });
 X(1,  Parabola, parabola, { v3 d; });
 X(2,  Offset,   offset,   { v3 d0; v3 d3; });
 X(3,  Unit,     unit,     { v2 d0; v2 d3; v3 unit_y; });
 X(4,  Unit2,    unit2,    { v4 d0d3; v3 unit_y; });
 X(6,  Line,     line,     { });
 X(7,  Bezd_Old, bezd_old, { v3 d0; v2 d3; });
 X(10, Raw,      raw,      { v3 p1; v3 p2; });
#undef X
 
 M_Entity_Variant_Info info_is_curve = {.is_curve = true};
#define X(enum_value, name, name_lower, struct_members) \
push_entity_variant(scratch, &variants, enum_value, \
strlit(#name), strlit(#name_lower), strlit(#struct_members), info_is_curve)
 
 X(5,  C2,       c2,       { Curve_Index ref; v3 d3; Vertex_Index p3; });
 X(8,  NegateX,  negateX,  { Curve_Index ref; });
 X(9,  Lerp,     lerp,     { Curve_Index begin; Curve_Index end; });
#undef X
 
 M_Entity_Variant_Info info_empty = {};
#define X(enum_value, name, name_lower, struct_members) \
push_entity_variant(scratch, &variants, enum_value, \
strlit(#name), strlit(#name_lower), strlit(#struct_members), info_empty)
 
 X(32, Circle,    circle,    { v3 center; v3 normal; });
 X(33, Fill3,     fill3,     { Vertex_Index verts[3]; });
 X(34, Fill_Bez,  fill_bez,  { Curve_Index curve; });
 X(35, Fill_DBez, fill_dbez, { Curve_Index curve1; Curve_Index curve2; });
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
    p<"union "<type_name;
    m_braces_sm{
     for_i32(i,0,variants.count){
      if(i!=0){ p<"\n"; }
      auto &variant = variants.get(i);
      p<variant.struct_name<" "<variant.name_lower<";";
     }
    }
   }
  }
  {//-Meta
   print_union_meta(printer, type_name, &variants, enum_type);
  }
 }
 {//-Information about entity types
  Printer &p = printer;
  M_Entity_Variant_Info *table = m_entity_variant_info_table;
  m_location;
  print_format(p, "global Entity_Type_Info entity_variant_info_table[%d];\n", max_entity_enum);
  p < "function void\n" <
   "init_entity_type_info_table()";
  m_braces{
   p < "\n";
   p < "Entity_Type_Info *table = entity_variant_info_table;\n";
   for_i32(variant_index,0,variants.count){
    i32 enum_value = variants[variant_index].enum_value;
    print_format(p, "table[%d].is_curve = %d;\n",
                 enum_value,
                 table[enum_value].is_curve);
   }
  }
 }
 {//-Transitional functions (aggravation: 100%)
  auto member_is_ref = [&](M_Struct_Member &member)->b32{
   return (member.type.name==strlit("Vertex_Index") ||
           member.type.name==strlit("Curve_Index"));
  };
  auto print_members_type_name = [&](Printer &p, Union_Variant &variant)->void{
   //-NOTE Must call this within separator block!
   for_i32(im,0,variant.struct_members.count){
    M_Struct_Member &member = variant.struct_members[im];
    if(member_is_ref(member)){
     if(member.type.kind == Parsed_Type_Array){
      for_i32(array_index,0,member.type.count){
       p<"String "<member.name<array_index;
       separator(p);
      }
     }else{
      p<"String "<member.name;
     }
    }else{
     print_struct_member(p,member);
    }
    separator(p);
   }
  };
  auto print_prototype = [&](Printer &p, Union_Variant &variant, b32 is_declaration)->void{
   p<"function void\n"<"send_bez_"<variant.name_lower;
   m_parens{
    separator_block(p, ", "){
     p<"String name"; separator(p);
     print_members_type_name(p, variant);
     if(is_declaration){
      p<"Line_Params params=lp()";    separator(p);
      p<"i32 linum=__builtin_LINE()"; separator(p);
     }else{
      p<"Line_Params params"; separator(p);
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
         M_Struct_Member &member = variant.struct_members[member_index];
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
     if(entity_variant_is_curve(variant) ||
        //TODO(kv) Circle isn't implement a single curve, but it's kinda like one.
        //  So it takes line params as a result.
        variant.name == "Circle")
     {//-Overloads with radii
      auto fun = [&](b32 is_v4)->void{
       p<"inline void\n"<
        "send_bez_"<variant.name_lower;
       m_parens{
        separator_block(p, ", "){
         p<"String name"; separator(p);
         print_members_type_name(p, variant);
         p<(is_v4 ? "v4 radii" : "i4 radii"); separator(p);
         p<"i32 linum=__builtin_LINE()"; separator(p);
        }
       }
       m_braces{
        p<"\n";
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
      for_i32(is_bs, 0, 2){
       b32 is_bn = !is_bs;
       p<"#define "<(is_bs?"bs_":"bn_")<variant.name_lower;
       m_parens{
        separator_block(p, ", "){
         if(is_bn){ p < "name"; separator(p); };
         for_i32(im,0,variant.struct_members.count){
          M_Struct_Member &member = variant.struct_members[im];
          if(member.type.kind == Parsed_Type_Array){
           for_i32(array_index, 0, member.type.count){
            p < member.name < array_index; separator(p);
           }
          }else{
           p < member.name; separator(p);
          }
         }
         p<"..."; separator(p);
        }
       }
       p<"\\\n";
       p<"send_bez_"<variant.name_lower;
       m_parens{
        separator_block(p, ", "){
         p < (is_bn?"strlit(#name)" : "strlit(\"l\")"); separator(p);
         for_i32(im,0,variant.struct_members.count){
          auto &member = variant.struct_members[im]; 
          if(member_is_ref(member)){
           if(member.type.kind == Parsed_Type_Array){
            for_i32(array_index,0,member.type.count){
             print_format(p, "strlit(#%.*s%d)", string_expand(member.name), array_index);
             separator(p);
            }
           }else{
            p<"strlit(#"<member.name<")";
           }
          }else{
           p<member.name;
          }
          separator(p);
         }
         p < "__VA_ARGS__"; separator(p);
        }
       }
       p<"\n";
      }
     }
     if(entity_variant_is_curve(variant))
     {//-bb_ba
      for_i32(is_ba,0,2){
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
      }
     }
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
    if(variant.name != "Fill3"){
     print_prototype(p, variant, false); m_braces_newline{
      p<"Modeler &m = *painter.modeler;\n";
      //-Fetch all the references
      for_i32(member_index,0,variant.struct_members.count){
       M_Struct_Member &member = variant.struct_members[member_index];
       b32 is_vertex = member.type.name == strlit("Vertex_Index");
       b32 is_curve  = member.type.name == strlit("Curve_Index");
       if(is_vertex || is_curve){
        p<member.type.name<" "<member.name<"_index = "<
        (is_vertex ? "get_vertex_from_var" : "get_curve_from_var")<
         "(m, "<member.name<", linum).index;\n";
        p<"kv_assert("<member.name<"_index.v);\n";
       }
      }
      p<"Curve_Union data = "; m_braces_sm{
       p<"."<variant.name_lower<"= "; m_braces{
        for_i32(member_index,0,variant.struct_members.count){
         M_Struct_Member &member = variant.struct_members[member_index];
         if(member_is_ref(member)){
          p<"."<member.name<"="<member.name<"_index"<", ";
         }else{
          p<"."<member.name<"="<member.name<", ";
         }
        }
       }
      }
      p<"\n";
      p<"send_bez_func"; m_comma_list{
       p<"name, "<variant.enum_name<", data, params, linum";
      }
      p<";\n";
     }
    }
   }
   close_file(p);
  }
 }
}
//-