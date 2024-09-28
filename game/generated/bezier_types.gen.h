#pragma once
//  C:\Users\vodan\4ed\code/meta_main.cpp:174:
enum Bezier_Type
{
 Bezier_Type_v3v2 = 0,
 Bezier_Type_Parabola = 1,
 Bezier_Type_Offsets = 2,
 Bezier_Type_Planar_Vec = 3,
 Bezier_Type_C2 = 4,
 
};
//  C:\Users\vodan\4ed\code/meta_print.cpp:280:
function Type_Info
get_type_info_Bezier_Type()
{
 Type_Info result = {};
 result.name = strlit("Bezier_Type");
 result.size = sizeof(Bezier_Type);
 result.kind = Type_Kind_Enum;
 result.enum_members.set_count(5);
 result.enum_members[0] = {.name=strlit("Bezier_Type_v3v2"), .value=Bezier_Type_v3v2};
 result.enum_members[1] = {.name=strlit("Bezier_Type_Parabola"), .value=Bezier_Type_Parabola};
 result.enum_members[2] = {.name=strlit("Bezier_Type_Offsets"), .value=Bezier_Type_Offsets};
 result.enum_members[3] = {.name=strlit("Bezier_Type_Planar_Vec"), .value=Bezier_Type_Planar_Vec};
 result.enum_members[4] = {.name=strlit("Bezier_Type_C2"), .value=Bezier_Type_C2};
 return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:81:
global_decl Type_Info Type_Info_Bezier_Type;
//  C:\Users\vodan\4ed\code/meta_print.cpp:303:
function void
read_Bezier_Type(Data_Reader &r, Bezier_Type &pointer)
{
 STB_Parser *p = r.parser;
 i32 integer = eat_i1(p);
 pointer = *(Bezier_Type*)(&integer);
}
static_assert( sizeof(Bezier_Type) <= sizeof(i32) );

//  C:\Users\vodan\4ed\code/meta_main.cpp:190:
struct Bezier_v3v2
{
 v3 d0;
 v2 d3;
};
//  C:\Users\vodan\4ed\code/meta_print.cpp:109:
function Type_Info
get_type_info_Bezier_v3v2()
{
 Type_Info result = {};
 result.name = strlit("Bezier_v3v2");
 result.size = sizeof(Bezier_v3v2);
 result.kind = Type_Kind_Struct;
 result.members.set_count(2);
 result.members[0] = {.type=&Type_Info_v3, .name=strlit("d0"), .offset=offsetof(Bezier_v3v2, d0)};
 result.members[1] = {.type=&Type_Info_v2, .name=strlit("d3"), .offset=offsetof(Bezier_v3v2, d3)};
 return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:81:
global_decl Type_Info Type_Info_Bezier_v3v2;
function void
read_Bezier_v3v2(Data_Reader &r, Bezier_v3v2 &pointer)
{
 STB_Parser *p = r.parser;
 eat_char(p, '{');
 v3 m_d0 = {};
 
 {
  eat_id(p, strlit("d0"));
  read_v3(r, m_d0);
 }
 pointer.d0 = m_d0;
 
 v2 m_d3 = {};
 
 {
  eat_id(p, strlit("d3"));
  read_v2(r, m_d3);
 }
 pointer.d3 = m_d3;
 
 eat_char(p, '}');
}
//  C:\Users\vodan\4ed\code/meta_main.cpp:190:
struct Bezier_Parabola
{
 v3 d;
};
//  C:\Users\vodan\4ed\code/meta_print.cpp:109:
function Type_Info
get_type_info_Bezier_Parabola()
{
 Type_Info result = {};
 result.name = strlit("Bezier_Parabola");
 result.size = sizeof(Bezier_Parabola);
 result.kind = Type_Kind_Struct;
 result.members.set_count(1);
 result.members[0] = {.type=&Type_Info_v3, .name=strlit("d"), .offset=offsetof(Bezier_Parabola, d)};
 return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:81:
global_decl Type_Info Type_Info_Bezier_Parabola;
function void
read_Bezier_Parabola(Data_Reader &r, Bezier_Parabola &pointer)
{
 STB_Parser *p = r.parser;
 eat_char(p, '{');
 v3 m_d = {};
 
 {
  eat_id(p, strlit("d"));
  read_v3(r, m_d);
 }
 pointer.d = m_d;
 
 eat_char(p, '}');
}
//  C:\Users\vodan\4ed\code/meta_main.cpp:190:
struct Bezier_Offsets
{
 v3 d0;
 v3 d3;
};
//  C:\Users\vodan\4ed\code/meta_print.cpp:109:
function Type_Info
get_type_info_Bezier_Offsets()
{
 Type_Info result = {};
 result.name = strlit("Bezier_Offsets");
 result.size = sizeof(Bezier_Offsets);
 result.kind = Type_Kind_Struct;
 result.members.set_count(2);
 result.members[0] = {.type=&Type_Info_v3, .name=strlit("d0"), .offset=offsetof(Bezier_Offsets, d0)};
 result.members[1] = {.type=&Type_Info_v3, .name=strlit("d3"), .offset=offsetof(Bezier_Offsets, d3)};
 return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:81:
global_decl Type_Info Type_Info_Bezier_Offsets;
function void
read_Bezier_Offsets(Data_Reader &r, Bezier_Offsets &pointer)
{
 STB_Parser *p = r.parser;
 eat_char(p, '{');
 v3 m_d0 = {};
 
 {
  eat_id(p, strlit("d0"));
  read_v3(r, m_d0);
 }
 pointer.d0 = m_d0;
 
 v3 m_d3 = {};
 
 {
  eat_id(p, strlit("d3"));
  read_v3(r, m_d3);
 }
 pointer.d3 = m_d3;
 
 eat_char(p, '}');
}
//  C:\Users\vodan\4ed\code/meta_main.cpp:190:
struct Bezier_Planar_Vec
{
 v2 d0;
 v2 d3;
 v3 unit_y;
};
//  C:\Users\vodan\4ed\code/meta_print.cpp:109:
function Type_Info
get_type_info_Bezier_Planar_Vec()
{
 Type_Info result = {};
 result.name = strlit("Bezier_Planar_Vec");
 result.size = sizeof(Bezier_Planar_Vec);
 result.kind = Type_Kind_Struct;
 result.members.set_count(3);
 result.members[0] = {.type=&Type_Info_v2, .name=strlit("d0"), .offset=offsetof(Bezier_Planar_Vec, d0)};
 result.members[1] = {.type=&Type_Info_v2, .name=strlit("d3"), .offset=offsetof(Bezier_Planar_Vec, d3)};
 result.members[2] = {.type=&Type_Info_v3, .name=strlit("unit_y"), .offset=offsetof(Bezier_Planar_Vec, unit_y)};
 return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:81:
global_decl Type_Info Type_Info_Bezier_Planar_Vec;
function void
read_Bezier_Planar_Vec(Data_Reader &r, Bezier_Planar_Vec &pointer)
{
 STB_Parser *p = r.parser;
 eat_char(p, '{');
 v2 m_d0 = {};
 
 {
  eat_id(p, strlit("d0"));
  read_v2(r, m_d0);
 }
 pointer.d0 = m_d0;
 
 v2 m_d3 = {};
 
 {
  eat_id(p, strlit("d3"));
  read_v2(r, m_d3);
 }
 pointer.d3 = m_d3;
 
 v3 m_unit_y = {};
 
 {
  eat_id(p, strlit("unit_y"));
  read_v3(r, m_unit_y);
 }
 pointer.unit_y = m_unit_y;
 
 eat_char(p, '}');
}
//  C:\Users\vodan\4ed\code/meta_main.cpp:190:
struct Bezier_C2
{
 Curve_Index ref;
 v3 d3;
};
//  C:\Users\vodan\4ed\code/meta_print.cpp:109:
function Type_Info
get_type_info_Bezier_C2()
{
 Type_Info result = {};
 result.name = strlit("Bezier_C2");
 result.size = sizeof(Bezier_C2);
 result.kind = Type_Kind_Struct;
 result.members.set_count(2);
 result.members[0] = {.type=&Type_Info_Curve_Index, .name=strlit("ref"), .offset=offsetof(Bezier_C2, ref)};
 result.members[1] = {.type=&Type_Info_v3, .name=strlit("d3"), .offset=offsetof(Bezier_C2, d3)};
 return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:81:
global_decl Type_Info Type_Info_Bezier_C2;
function void
read_Bezier_C2(Data_Reader &r, Bezier_C2 &pointer)
{
 STB_Parser *p = r.parser;
 eat_char(p, '{');
 Curve_Index m_ref = {};
 
 {
  eat_id(p, strlit("ref"));
  read_Curve_Index(r, m_ref);
 }
 pointer.ref = m_ref;
 
 v3 m_d3 = {};
 
 {
  eat_id(p, strlit("d3"));
  read_v3(r, m_d3);
 }
 pointer.d3 = m_d3;
 
 eat_char(p, '}');
}
//  C:\Users\vodan\4ed\code/meta_main.cpp:198:
union Bezier_Union
{
 Bezier_v3v2 v3v2;
 Bezier_Parabola Parabola;
 Bezier_Offsets Offsets;
 Bezier_Planar_Vec Planar_Vec;
 Bezier_C2 C2;
};
//  C:\Users\vodan\4ed\code/meta_print.cpp:219:
function Type_Info
get_type_info_Bezier_Union()
{
 Type_Info result = {};
 result.name = strlit("Bezier_Union");
 result.size = sizeof(Bezier_Union);
 result.kind = Type_Kind_Union;
 result.discriminator_type = &Type_Info_Bezier_Type;
 result.union_members.set_count(5);
 result.union_members[0] = {.type=&Type_Info_Bezier_v3v2, .name=strlit("v3v2"), .variant=Bezier_Type_v3v2};
 result.union_members[1] = {.type=&Type_Info_Bezier_Parabola, .name=strlit("Parabola"), .variant=Bezier_Type_Parabola};
 result.union_members[2] = {.type=&Type_Info_Bezier_Offsets, .name=strlit("Offsets"), .variant=Bezier_Type_Offsets};
 result.union_members[3] = {.type=&Type_Info_Bezier_Planar_Vec, .name=strlit("Planar_Vec"), .variant=Bezier_Type_Planar_Vec};
 result.union_members[4] = {.type=&Type_Info_Bezier_C2, .name=strlit("C2"), .variant=Bezier_Type_C2};
 return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:81:
global_decl Type_Info Type_Info_Bezier_Union;
//  C:\Users\vodan\4ed\code/meta_print.cpp:247:
function void
read_Bezier_Union(Data_Reader &r, Bezier_Union &pointer, Bezier_Type variant)
{
 STB_Parser *p = r.parser;
 switch(variant)
 {
  case Bezier_Type_v3v2:
  {
   read_Bezier_v3v2(r, pointer.v3v2);
   break;
  }
  case Bezier_Type_Parabola:
  {
   read_Bezier_Parabola(r, pointer.Parabola);
   break;
  }
  case Bezier_Type_Offsets:
  {
   read_Bezier_Offsets(r, pointer.Offsets);
   break;
  }
  case Bezier_Type_Planar_Vec:
  {
   read_Bezier_Planar_Vec(r, pointer.Planar_Vec);
   break;
  }
  case Bezier_Type_C2:
  {
   read_Bezier_C2(r, pointer.C2);
   break;
  }
  
 }
 
}
