//NOTE Generated at C:\Users\vodan\4ed\code/meta_klang.cpp:254:
// NOTE: source: C:\Users\vodan\4ed\code\game\framework.kh
#pragma once
//-
struct Entity_Type_Info{b32 is_curve;};;
//-
struct Vertex_Data{String name;
Bone_ID bone_id;
v3 pos;
i1 linum;};//  C:\Users\vodan\4ed\code/meta_print.cpp:169:
struct Vertex_Data;
function Type_Info
get_type_info_Vertex_Data();//  C:\Users\vodan\4ed\code/meta_print.cpp:174:
function Type_Info
get_type_info_Vertex_Data()
{
Type_Info result = {};
result.name = strlit("Vertex_Data");
result.size = sizeof(Vertex_Data);
result.kind = I_Type_Kind_Struct;
result.members.set_count(4);
result.members[0] = {.type=&Type_Info_String, .name=strlit("name"), .offset=offsetof(Vertex_Data, name)};
result.members[1] = {.type=&Type_Info_Bone_ID, .name=strlit("bone_id"), .offset=offsetof(Vertex_Data, bone_id)};
result.members[2] = {.type=&Type_Info_v3, .name=strlit("pos"), .offset=offsetof(Vertex_Data, pos)};
result.members[3] = {.type=&Type_Info_i1, .name=strlit("linum"), .offset=offsetof(Vertex_Data, linum), .unserialized=true};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:85:
global_decl Type_Info Type_Info_Vertex_Data;
//  C:\Users\vodan\4ed\code/meta_print.cpp:92:
xglobal Type_Info Type_Info_Vertex_Data = get_type_info_Vertex_Data();

function Type_Info &type_info_from_pointer(Vertex_Data*pointer){return Type_Info_Vertex_Data;}//  C:\Users\vodan\4ed\code/meta_print.cpp:220:
function void
read_Vertex_Data(Data_Reader &r, Vertex_Data &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:225:
function void
read_Vertex_Data(Data_Reader &r, Vertex_Data &pointer)
{
STB_Parser *p = r.parser;
eat_char(p, '{');
String m_name = {};

{
eat_id(p, strlit("name"));
read_String(r, m_name);
}
pointer.name = m_name;

Bone_ID m_bone_id = {};

{
eat_id(p, strlit("bone_id"));
read_Bone_ID(r, m_bone_id);
}
pointer.bone_id = m_bone_id;

v3 m_pos = {};

{
eat_id(p, strlit("pos"));
read_v3(r, m_pos);
}
pointer.pos = m_pos;

eat_char(p, '}');
}
;
//  C:\Users\vodan\4ed\code/meta_print.cpp:374:
enum Curve_Type{Curve_Type_v3v2 = 11,
Curve_Type_Parabola = 1,
Curve_Type_Offset = 2,
Curve_Type_Unit = 3,
Curve_Type_Unit2 = 4,
Curve_Type_Line = 6,
Curve_Type_Bezd_Old = 7,
Curve_Type_Raw = 10,
Curve_Type_C2 = 5,
Curve_Type_NegateX = 8,
Curve_Type_Lerp = 9,
Curve_Type_Circle = 32,
Curve_Type_Fill3 = 33,
Curve_Type_Fill_Bez = 34,
Curve_Type_Fill_DBez = 35,};//  C:\Users\vodan\4ed\code/meta_print.cpp:389:
function Type_Info
get_type_info_Curve_Type();//  C:\Users\vodan\4ed\code/meta_print.cpp:393:
function Type_Info
get_type_info_Curve_Type(){Type_Info result = {};
result.name = strlit("Curve_Type");
result.size = sizeof(Curve_Type);
result.kind = I_Type_Kind_Enum;
result.enum_members.set_count(15);
result.enum_members[0] = {.name=strlit("Curve_Type_v3v2"), .value=Curve_Type_v3v2};
result.enum_members[1] = {.name=strlit("Curve_Type_Parabola"), .value=Curve_Type_Parabola};
result.enum_members[2] = {.name=strlit("Curve_Type_Offset"), .value=Curve_Type_Offset};
result.enum_members[3] = {.name=strlit("Curve_Type_Unit"), .value=Curve_Type_Unit};
result.enum_members[4] = {.name=strlit("Curve_Type_Unit2"), .value=Curve_Type_Unit2};
result.enum_members[5] = {.name=strlit("Curve_Type_Line"), .value=Curve_Type_Line};
result.enum_members[6] = {.name=strlit("Curve_Type_Bezd_Old"), .value=Curve_Type_Bezd_Old};
result.enum_members[7] = {.name=strlit("Curve_Type_Raw"), .value=Curve_Type_Raw};
result.enum_members[8] = {.name=strlit("Curve_Type_C2"), .value=Curve_Type_C2};
result.enum_members[9] = {.name=strlit("Curve_Type_NegateX"), .value=Curve_Type_NegateX};
result.enum_members[10] = {.name=strlit("Curve_Type_Lerp"), .value=Curve_Type_Lerp};
result.enum_members[11] = {.name=strlit("Curve_Type_Circle"), .value=Curve_Type_Circle};
result.enum_members[12] = {.name=strlit("Curve_Type_Fill3"), .value=Curve_Type_Fill3};
result.enum_members[13] = {.name=strlit("Curve_Type_Fill_Bez"), .value=Curve_Type_Fill_Bez};
result.enum_members[14] = {.name=strlit("Curve_Type_Fill_DBez"), .value=Curve_Type_Fill_DBez};
return result;}//  C:\Users\vodan\4ed\code/meta_print.cpp:85:
global_decl Type_Info Type_Info_Curve_Type;
//  C:\Users\vodan\4ed\code/meta_print.cpp:92:
xglobal Type_Info Type_Info_Curve_Type = get_type_info_Curve_Type();

function Type_Info &type_info_from_pointer(Curve_Type*pointer){return Type_Info_Curve_Type;}//  C:\Users\vodan\4ed\code/meta_print.cpp:417:
function void
read_Curve_Type(Data_Reader &r, Curve_Type &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:422:
function void
read_Curve_Type(Data_Reader &r, Curve_Type &pointer){STB_Parser *p = r.parser;
i32 integer = eat_i1(p);
pointer = *(Curve_Type*)(&integer);}static_assert( sizeof(Curve_Type) <= sizeof(i32) );

//  C:\Users\vodan\4ed\code/meta_entity.cpp:109:
struct Curve_v3v2{Vertex_Index p0;
v3 d0;
v2 d3;
Vertex_Index p3;};//  C:\Users\vodan\4ed\code/meta_print.cpp:169:
struct Curve_v3v2;
function Type_Info
get_type_info_Curve_v3v2();//  C:\Users\vodan\4ed\code/meta_print.cpp:174:
function Type_Info
get_type_info_Curve_v3v2()
{
Type_Info result = {};
result.name = strlit("Curve_v3v2");
result.size = sizeof(Curve_v3v2);
result.kind = I_Type_Kind_Struct;
result.members.set_count(4);
result.members[0] = {.type=&Type_Info_Vertex_Index, .name=strlit("p0"), .offset=offsetof(Curve_v3v2, p0)};
result.members[1] = {.type=&Type_Info_v3, .name=strlit("d0"), .offset=offsetof(Curve_v3v2, d0)};
result.members[2] = {.type=&Type_Info_v2, .name=strlit("d3"), .offset=offsetof(Curve_v3v2, d3)};
result.members[3] = {.type=&Type_Info_Vertex_Index, .name=strlit("p3"), .offset=offsetof(Curve_v3v2, p3)};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:85:
global_decl Type_Info Type_Info_Curve_v3v2;
//  C:\Users\vodan\4ed\code/meta_print.cpp:92:
xglobal Type_Info Type_Info_Curve_v3v2 = get_type_info_Curve_v3v2();

function Type_Info &type_info_from_pointer(Curve_v3v2*pointer){return Type_Info_Curve_v3v2;}//  C:\Users\vodan\4ed\code/meta_print.cpp:220:
function void
read_Curve_v3v2(Data_Reader &r, Curve_v3v2 &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:225:
function void
read_Curve_v3v2(Data_Reader &r, Curve_v3v2 &pointer)
{
STB_Parser *p = r.parser;
eat_char(p, '{');
Vertex_Index m_p0 = {};

{
eat_id(p, strlit("p0"));
read_Vertex_Index(r, m_p0);
}
pointer.p0 = m_p0;

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

Vertex_Index m_p3 = {};

{
eat_id(p, strlit("p3"));
read_Vertex_Index(r, m_p3);
}
pointer.p3 = m_p3;

eat_char(p, '}');
}
//  C:\Users\vodan\4ed\code/meta_entity.cpp:109:
struct Curve_Parabola{Vertex_Index p0;
v3 d;
Vertex_Index p3;};//  C:\Users\vodan\4ed\code/meta_print.cpp:169:
struct Curve_Parabola;
function Type_Info
get_type_info_Curve_Parabola();//  C:\Users\vodan\4ed\code/meta_print.cpp:174:
function Type_Info
get_type_info_Curve_Parabola()
{
Type_Info result = {};
result.name = strlit("Curve_Parabola");
result.size = sizeof(Curve_Parabola);
result.kind = I_Type_Kind_Struct;
result.members.set_count(3);
result.members[0] = {.type=&Type_Info_Vertex_Index, .name=strlit("p0"), .offset=offsetof(Curve_Parabola, p0)};
result.members[1] = {.type=&Type_Info_v3, .name=strlit("d"), .offset=offsetof(Curve_Parabola, d)};
result.members[2] = {.type=&Type_Info_Vertex_Index, .name=strlit("p3"), .offset=offsetof(Curve_Parabola, p3)};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:85:
global_decl Type_Info Type_Info_Curve_Parabola;
//  C:\Users\vodan\4ed\code/meta_print.cpp:92:
xglobal Type_Info Type_Info_Curve_Parabola = get_type_info_Curve_Parabola();

function Type_Info &type_info_from_pointer(Curve_Parabola*pointer){return Type_Info_Curve_Parabola;}//  C:\Users\vodan\4ed\code/meta_print.cpp:220:
function void
read_Curve_Parabola(Data_Reader &r, Curve_Parabola &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:225:
function void
read_Curve_Parabola(Data_Reader &r, Curve_Parabola &pointer)
{
STB_Parser *p = r.parser;
eat_char(p, '{');
Vertex_Index m_p0 = {};

{
eat_id(p, strlit("p0"));
read_Vertex_Index(r, m_p0);
}
pointer.p0 = m_p0;

v3 m_d = {};

{
eat_id(p, strlit("d"));
read_v3(r, m_d);
}
pointer.d = m_d;

Vertex_Index m_p3 = {};

{
eat_id(p, strlit("p3"));
read_Vertex_Index(r, m_p3);
}
pointer.p3 = m_p3;

eat_char(p, '}');
}
//  C:\Users\vodan\4ed\code/meta_entity.cpp:109:
struct Curve_Offset{Vertex_Index p0;
v3 d0;
v3 d3;
Vertex_Index p3;};//  C:\Users\vodan\4ed\code/meta_print.cpp:169:
struct Curve_Offset;
function Type_Info
get_type_info_Curve_Offset();//  C:\Users\vodan\4ed\code/meta_print.cpp:174:
function Type_Info
get_type_info_Curve_Offset()
{
Type_Info result = {};
result.name = strlit("Curve_Offset");
result.size = sizeof(Curve_Offset);
result.kind = I_Type_Kind_Struct;
result.members.set_count(4);
result.members[0] = {.type=&Type_Info_Vertex_Index, .name=strlit("p0"), .offset=offsetof(Curve_Offset, p0)};
result.members[1] = {.type=&Type_Info_v3, .name=strlit("d0"), .offset=offsetof(Curve_Offset, d0)};
result.members[2] = {.type=&Type_Info_v3, .name=strlit("d3"), .offset=offsetof(Curve_Offset, d3)};
result.members[3] = {.type=&Type_Info_Vertex_Index, .name=strlit("p3"), .offset=offsetof(Curve_Offset, p3)};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:85:
global_decl Type_Info Type_Info_Curve_Offset;
//  C:\Users\vodan\4ed\code/meta_print.cpp:92:
xglobal Type_Info Type_Info_Curve_Offset = get_type_info_Curve_Offset();

function Type_Info &type_info_from_pointer(Curve_Offset*pointer){return Type_Info_Curve_Offset;}//  C:\Users\vodan\4ed\code/meta_print.cpp:220:
function void
read_Curve_Offset(Data_Reader &r, Curve_Offset &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:225:
function void
read_Curve_Offset(Data_Reader &r, Curve_Offset &pointer)
{
STB_Parser *p = r.parser;
eat_char(p, '{');
Vertex_Index m_p0 = {};

{
eat_id(p, strlit("p0"));
read_Vertex_Index(r, m_p0);
}
pointer.p0 = m_p0;

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

Vertex_Index m_p3 = {};

{
eat_id(p, strlit("p3"));
read_Vertex_Index(r, m_p3);
}
pointer.p3 = m_p3;

eat_char(p, '}');
}
//  C:\Users\vodan\4ed\code/meta_entity.cpp:109:
struct Curve_Unit{Vertex_Index p0;
v2 d0;
v2 d3;
v3 unit_y;
Vertex_Index p3;};//  C:\Users\vodan\4ed\code/meta_print.cpp:169:
struct Curve_Unit;
function Type_Info
get_type_info_Curve_Unit();//  C:\Users\vodan\4ed\code/meta_print.cpp:174:
function Type_Info
get_type_info_Curve_Unit()
{
Type_Info result = {};
result.name = strlit("Curve_Unit");
result.size = sizeof(Curve_Unit);
result.kind = I_Type_Kind_Struct;
result.members.set_count(5);
result.members[0] = {.type=&Type_Info_Vertex_Index, .name=strlit("p0"), .offset=offsetof(Curve_Unit, p0)};
result.members[1] = {.type=&Type_Info_v2, .name=strlit("d0"), .offset=offsetof(Curve_Unit, d0)};
result.members[2] = {.type=&Type_Info_v2, .name=strlit("d3"), .offset=offsetof(Curve_Unit, d3)};
result.members[3] = {.type=&Type_Info_v3, .name=strlit("unit_y"), .offset=offsetof(Curve_Unit, unit_y)};
result.members[4] = {.type=&Type_Info_Vertex_Index, .name=strlit("p3"), .offset=offsetof(Curve_Unit, p3)};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:85:
global_decl Type_Info Type_Info_Curve_Unit;
//  C:\Users\vodan\4ed\code/meta_print.cpp:92:
xglobal Type_Info Type_Info_Curve_Unit = get_type_info_Curve_Unit();

function Type_Info &type_info_from_pointer(Curve_Unit*pointer){return Type_Info_Curve_Unit;}//  C:\Users\vodan\4ed\code/meta_print.cpp:220:
function void
read_Curve_Unit(Data_Reader &r, Curve_Unit &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:225:
function void
read_Curve_Unit(Data_Reader &r, Curve_Unit &pointer)
{
STB_Parser *p = r.parser;
eat_char(p, '{');
Vertex_Index m_p0 = {};

{
eat_id(p, strlit("p0"));
read_Vertex_Index(r, m_p0);
}
pointer.p0 = m_p0;

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

Vertex_Index m_p3 = {};

{
eat_id(p, strlit("p3"));
read_Vertex_Index(r, m_p3);
}
pointer.p3 = m_p3;

eat_char(p, '}');
}
//  C:\Users\vodan\4ed\code/meta_entity.cpp:109:
struct Curve_Unit2{Vertex_Index p0;
v4 d0d3;
v3 unit_y;
Vertex_Index p3;};//  C:\Users\vodan\4ed\code/meta_print.cpp:169:
struct Curve_Unit2;
function Type_Info
get_type_info_Curve_Unit2();//  C:\Users\vodan\4ed\code/meta_print.cpp:174:
function Type_Info
get_type_info_Curve_Unit2()
{
Type_Info result = {};
result.name = strlit("Curve_Unit2");
result.size = sizeof(Curve_Unit2);
result.kind = I_Type_Kind_Struct;
result.members.set_count(4);
result.members[0] = {.type=&Type_Info_Vertex_Index, .name=strlit("p0"), .offset=offsetof(Curve_Unit2, p0)};
result.members[1] = {.type=&Type_Info_v4, .name=strlit("d0d3"), .offset=offsetof(Curve_Unit2, d0d3)};
result.members[2] = {.type=&Type_Info_v3, .name=strlit("unit_y"), .offset=offsetof(Curve_Unit2, unit_y)};
result.members[3] = {.type=&Type_Info_Vertex_Index, .name=strlit("p3"), .offset=offsetof(Curve_Unit2, p3)};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:85:
global_decl Type_Info Type_Info_Curve_Unit2;
//  C:\Users\vodan\4ed\code/meta_print.cpp:92:
xglobal Type_Info Type_Info_Curve_Unit2 = get_type_info_Curve_Unit2();

function Type_Info &type_info_from_pointer(Curve_Unit2*pointer){return Type_Info_Curve_Unit2;}//  C:\Users\vodan\4ed\code/meta_print.cpp:220:
function void
read_Curve_Unit2(Data_Reader &r, Curve_Unit2 &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:225:
function void
read_Curve_Unit2(Data_Reader &r, Curve_Unit2 &pointer)
{
STB_Parser *p = r.parser;
eat_char(p, '{');
Vertex_Index m_p0 = {};

{
eat_id(p, strlit("p0"));
read_Vertex_Index(r, m_p0);
}
pointer.p0 = m_p0;

v4 m_d0d3 = {};

{
eat_id(p, strlit("d0d3"));
read_v4(r, m_d0d3);
}
pointer.d0d3 = m_d0d3;

v3 m_unit_y = {};

{
eat_id(p, strlit("unit_y"));
read_v3(r, m_unit_y);
}
pointer.unit_y = m_unit_y;

Vertex_Index m_p3 = {};

{
eat_id(p, strlit("p3"));
read_Vertex_Index(r, m_p3);
}
pointer.p3 = m_p3;

eat_char(p, '}');
}
//  C:\Users\vodan\4ed\code/meta_entity.cpp:109:
struct Curve_Line{Vertex_Index p0;
Vertex_Index p3;};//  C:\Users\vodan\4ed\code/meta_print.cpp:169:
struct Curve_Line;
function Type_Info
get_type_info_Curve_Line();//  C:\Users\vodan\4ed\code/meta_print.cpp:174:
function Type_Info
get_type_info_Curve_Line()
{
Type_Info result = {};
result.name = strlit("Curve_Line");
result.size = sizeof(Curve_Line);
result.kind = I_Type_Kind_Struct;
result.members.set_count(2);
result.members[0] = {.type=&Type_Info_Vertex_Index, .name=strlit("p0"), .offset=offsetof(Curve_Line, p0)};
result.members[1] = {.type=&Type_Info_Vertex_Index, .name=strlit("p3"), .offset=offsetof(Curve_Line, p3)};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:85:
global_decl Type_Info Type_Info_Curve_Line;
//  C:\Users\vodan\4ed\code/meta_print.cpp:92:
xglobal Type_Info Type_Info_Curve_Line = get_type_info_Curve_Line();

function Type_Info &type_info_from_pointer(Curve_Line*pointer){return Type_Info_Curve_Line;}//  C:\Users\vodan\4ed\code/meta_print.cpp:220:
function void
read_Curve_Line(Data_Reader &r, Curve_Line &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:225:
function void
read_Curve_Line(Data_Reader &r, Curve_Line &pointer)
{
STB_Parser *p = r.parser;
eat_char(p, '{');
Vertex_Index m_p0 = {};

{
eat_id(p, strlit("p0"));
read_Vertex_Index(r, m_p0);
}
pointer.p0 = m_p0;

Vertex_Index m_p3 = {};

{
eat_id(p, strlit("p3"));
read_Vertex_Index(r, m_p3);
}
pointer.p3 = m_p3;

eat_char(p, '}');
}
//  C:\Users\vodan\4ed\code/meta_entity.cpp:109:
struct Curve_Bezd_Old{Vertex_Index p0;
v3 d0;
v2 d3;
Vertex_Index p3;};//  C:\Users\vodan\4ed\code/meta_print.cpp:169:
struct Curve_Bezd_Old;
function Type_Info
get_type_info_Curve_Bezd_Old();//  C:\Users\vodan\4ed\code/meta_print.cpp:174:
function Type_Info
get_type_info_Curve_Bezd_Old()
{
Type_Info result = {};
result.name = strlit("Curve_Bezd_Old");
result.size = sizeof(Curve_Bezd_Old);
result.kind = I_Type_Kind_Struct;
result.members.set_count(4);
result.members[0] = {.type=&Type_Info_Vertex_Index, .name=strlit("p0"), .offset=offsetof(Curve_Bezd_Old, p0)};
result.members[1] = {.type=&Type_Info_v3, .name=strlit("d0"), .offset=offsetof(Curve_Bezd_Old, d0)};
result.members[2] = {.type=&Type_Info_v2, .name=strlit("d3"), .offset=offsetof(Curve_Bezd_Old, d3)};
result.members[3] = {.type=&Type_Info_Vertex_Index, .name=strlit("p3"), .offset=offsetof(Curve_Bezd_Old, p3)};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:85:
global_decl Type_Info Type_Info_Curve_Bezd_Old;
//  C:\Users\vodan\4ed\code/meta_print.cpp:92:
xglobal Type_Info Type_Info_Curve_Bezd_Old = get_type_info_Curve_Bezd_Old();

function Type_Info &type_info_from_pointer(Curve_Bezd_Old*pointer){return Type_Info_Curve_Bezd_Old;}//  C:\Users\vodan\4ed\code/meta_print.cpp:220:
function void
read_Curve_Bezd_Old(Data_Reader &r, Curve_Bezd_Old &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:225:
function void
read_Curve_Bezd_Old(Data_Reader &r, Curve_Bezd_Old &pointer)
{
STB_Parser *p = r.parser;
eat_char(p, '{');
Vertex_Index m_p0 = {};

{
eat_id(p, strlit("p0"));
read_Vertex_Index(r, m_p0);
}
pointer.p0 = m_p0;

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

Vertex_Index m_p3 = {};

{
eat_id(p, strlit("p3"));
read_Vertex_Index(r, m_p3);
}
pointer.p3 = m_p3;

eat_char(p, '}');
}
//  C:\Users\vodan\4ed\code/meta_entity.cpp:109:
struct Curve_Raw{Vertex_Index p0;
v3 p1;
v3 p2;
Vertex_Index p3;};//  C:\Users\vodan\4ed\code/meta_print.cpp:169:
struct Curve_Raw;
function Type_Info
get_type_info_Curve_Raw();//  C:\Users\vodan\4ed\code/meta_print.cpp:174:
function Type_Info
get_type_info_Curve_Raw()
{
Type_Info result = {};
result.name = strlit("Curve_Raw");
result.size = sizeof(Curve_Raw);
result.kind = I_Type_Kind_Struct;
result.members.set_count(4);
result.members[0] = {.type=&Type_Info_Vertex_Index, .name=strlit("p0"), .offset=offsetof(Curve_Raw, p0)};
result.members[1] = {.type=&Type_Info_v3, .name=strlit("p1"), .offset=offsetof(Curve_Raw, p1)};
result.members[2] = {.type=&Type_Info_v3, .name=strlit("p2"), .offset=offsetof(Curve_Raw, p2)};
result.members[3] = {.type=&Type_Info_Vertex_Index, .name=strlit("p3"), .offset=offsetof(Curve_Raw, p3)};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:85:
global_decl Type_Info Type_Info_Curve_Raw;
//  C:\Users\vodan\4ed\code/meta_print.cpp:92:
xglobal Type_Info Type_Info_Curve_Raw = get_type_info_Curve_Raw();

function Type_Info &type_info_from_pointer(Curve_Raw*pointer){return Type_Info_Curve_Raw;}//  C:\Users\vodan\4ed\code/meta_print.cpp:220:
function void
read_Curve_Raw(Data_Reader &r, Curve_Raw &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:225:
function void
read_Curve_Raw(Data_Reader &r, Curve_Raw &pointer)
{
STB_Parser *p = r.parser;
eat_char(p, '{');
Vertex_Index m_p0 = {};

{
eat_id(p, strlit("p0"));
read_Vertex_Index(r, m_p0);
}
pointer.p0 = m_p0;

v3 m_p1 = {};

{
eat_id(p, strlit("p1"));
read_v3(r, m_p1);
}
pointer.p1 = m_p1;

v3 m_p2 = {};

{
eat_id(p, strlit("p2"));
read_v3(r, m_p2);
}
pointer.p2 = m_p2;

Vertex_Index m_p3 = {};

{
eat_id(p, strlit("p3"));
read_Vertex_Index(r, m_p3);
}
pointer.p3 = m_p3;

eat_char(p, '}');
}
//  C:\Users\vodan\4ed\code/meta_entity.cpp:109:
struct Curve_C2{Curve_Index ref;
v3 d3;
Vertex_Index p3;};//  C:\Users\vodan\4ed\code/meta_print.cpp:169:
struct Curve_C2;
function Type_Info
get_type_info_Curve_C2();//  C:\Users\vodan\4ed\code/meta_print.cpp:174:
function Type_Info
get_type_info_Curve_C2()
{
Type_Info result = {};
result.name = strlit("Curve_C2");
result.size = sizeof(Curve_C2);
result.kind = I_Type_Kind_Struct;
result.members.set_count(3);
result.members[0] = {.type=&Type_Info_Curve_Index, .name=strlit("ref"), .offset=offsetof(Curve_C2, ref)};
result.members[1] = {.type=&Type_Info_v3, .name=strlit("d3"), .offset=offsetof(Curve_C2, d3)};
result.members[2] = {.type=&Type_Info_Vertex_Index, .name=strlit("p3"), .offset=offsetof(Curve_C2, p3)};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:85:
global_decl Type_Info Type_Info_Curve_C2;
//  C:\Users\vodan\4ed\code/meta_print.cpp:92:
xglobal Type_Info Type_Info_Curve_C2 = get_type_info_Curve_C2();

function Type_Info &type_info_from_pointer(Curve_C2*pointer){return Type_Info_Curve_C2;}//  C:\Users\vodan\4ed\code/meta_print.cpp:220:
function void
read_Curve_C2(Data_Reader &r, Curve_C2 &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:225:
function void
read_Curve_C2(Data_Reader &r, Curve_C2 &pointer)
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

Vertex_Index m_p3 = {};

{
eat_id(p, strlit("p3"));
read_Vertex_Index(r, m_p3);
}
pointer.p3 = m_p3;

eat_char(p, '}');
}
//  C:\Users\vodan\4ed\code/meta_entity.cpp:109:
struct Curve_NegateX{Curve_Index ref;};//  C:\Users\vodan\4ed\code/meta_print.cpp:169:
struct Curve_NegateX;
function Type_Info
get_type_info_Curve_NegateX();//  C:\Users\vodan\4ed\code/meta_print.cpp:174:
function Type_Info
get_type_info_Curve_NegateX()
{
Type_Info result = {};
result.name = strlit("Curve_NegateX");
result.size = sizeof(Curve_NegateX);
result.kind = I_Type_Kind_Struct;
result.members.set_count(1);
result.members[0] = {.type=&Type_Info_Curve_Index, .name=strlit("ref"), .offset=offsetof(Curve_NegateX, ref)};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:85:
global_decl Type_Info Type_Info_Curve_NegateX;
//  C:\Users\vodan\4ed\code/meta_print.cpp:92:
xglobal Type_Info Type_Info_Curve_NegateX = get_type_info_Curve_NegateX();

function Type_Info &type_info_from_pointer(Curve_NegateX*pointer){return Type_Info_Curve_NegateX;}//  C:\Users\vodan\4ed\code/meta_print.cpp:220:
function void
read_Curve_NegateX(Data_Reader &r, Curve_NegateX &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:225:
function void
read_Curve_NegateX(Data_Reader &r, Curve_NegateX &pointer)
{
STB_Parser *p = r.parser;
eat_char(p, '{');
Curve_Index m_ref = {};

{
eat_id(p, strlit("ref"));
read_Curve_Index(r, m_ref);
}
pointer.ref = m_ref;

eat_char(p, '}');
}
//  C:\Users\vodan\4ed\code/meta_entity.cpp:109:
struct Curve_Lerp{Curve_Index begin;
Curve_Index end;};//  C:\Users\vodan\4ed\code/meta_print.cpp:169:
struct Curve_Lerp;
function Type_Info
get_type_info_Curve_Lerp();//  C:\Users\vodan\4ed\code/meta_print.cpp:174:
function Type_Info
get_type_info_Curve_Lerp()
{
Type_Info result = {};
result.name = strlit("Curve_Lerp");
result.size = sizeof(Curve_Lerp);
result.kind = I_Type_Kind_Struct;
result.members.set_count(2);
result.members[0] = {.type=&Type_Info_Curve_Index, .name=strlit("begin"), .offset=offsetof(Curve_Lerp, begin)};
result.members[1] = {.type=&Type_Info_Curve_Index, .name=strlit("end"), .offset=offsetof(Curve_Lerp, end)};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:85:
global_decl Type_Info Type_Info_Curve_Lerp;
//  C:\Users\vodan\4ed\code/meta_print.cpp:92:
xglobal Type_Info Type_Info_Curve_Lerp = get_type_info_Curve_Lerp();

function Type_Info &type_info_from_pointer(Curve_Lerp*pointer){return Type_Info_Curve_Lerp;}//  C:\Users\vodan\4ed\code/meta_print.cpp:220:
function void
read_Curve_Lerp(Data_Reader &r, Curve_Lerp &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:225:
function void
read_Curve_Lerp(Data_Reader &r, Curve_Lerp &pointer)
{
STB_Parser *p = r.parser;
eat_char(p, '{');
Curve_Index m_begin = {};

{
eat_id(p, strlit("begin"));
read_Curve_Index(r, m_begin);
}
pointer.begin = m_begin;

Curve_Index m_end = {};

{
eat_id(p, strlit("end"));
read_Curve_Index(r, m_end);
}
pointer.end = m_end;

eat_char(p, '}');
}
//  C:\Users\vodan\4ed\code/meta_entity.cpp:109:
struct Curve_Circle{v3 center;
v3 normal;};//  C:\Users\vodan\4ed\code/meta_print.cpp:169:
struct Curve_Circle;
function Type_Info
get_type_info_Curve_Circle();//  C:\Users\vodan\4ed\code/meta_print.cpp:174:
function Type_Info
get_type_info_Curve_Circle()
{
Type_Info result = {};
result.name = strlit("Curve_Circle");
result.size = sizeof(Curve_Circle);
result.kind = I_Type_Kind_Struct;
result.members.set_count(2);
result.members[0] = {.type=&Type_Info_v3, .name=strlit("center"), .offset=offsetof(Curve_Circle, center)};
result.members[1] = {.type=&Type_Info_v3, .name=strlit("normal"), .offset=offsetof(Curve_Circle, normal)};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:85:
global_decl Type_Info Type_Info_Curve_Circle;
//  C:\Users\vodan\4ed\code/meta_print.cpp:92:
xglobal Type_Info Type_Info_Curve_Circle = get_type_info_Curve_Circle();

function Type_Info &type_info_from_pointer(Curve_Circle*pointer){return Type_Info_Curve_Circle;}//  C:\Users\vodan\4ed\code/meta_print.cpp:220:
function void
read_Curve_Circle(Data_Reader &r, Curve_Circle &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:225:
function void
read_Curve_Circle(Data_Reader &r, Curve_Circle &pointer)
{
STB_Parser *p = r.parser;
eat_char(p, '{');
v3 m_center = {};

{
eat_id(p, strlit("center"));
read_v3(r, m_center);
}
pointer.center = m_center;

v3 m_normal = {};

{
eat_id(p, strlit("normal"));
read_v3(r, m_normal);
}
pointer.normal = m_normal;

eat_char(p, '}');
}
//  C:\Users\vodan\4ed\code/meta_entity.cpp:109:
struct Curve_Fill3{Vertex_Index verts[3];};//  C:\Users\vodan\4ed\code/meta_print.cpp:169:
struct Curve_Fill3;
function Type_Info
get_type_info_Curve_Fill3();//  C:\Users\vodan\4ed\code/meta_print.cpp:174:
function Type_Info
get_type_info_Curve_Fill3()
{
Type_Info result = {};
result.name = strlit("Curve_Fill3");
result.size = sizeof(Curve_Fill3);
result.kind = I_Type_Kind_Struct;
result.members.set_count(1);
result.members[0] = {.type=&Type_Info_Vertex_Index, .name=strlit("verts"), .offset=offsetof(Curve_Fill3, verts)};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:85:
global_decl Type_Info Type_Info_Curve_Fill3;
//  C:\Users\vodan\4ed\code/meta_print.cpp:92:
xglobal Type_Info Type_Info_Curve_Fill3 = get_type_info_Curve_Fill3();

function Type_Info &type_info_from_pointer(Curve_Fill3*pointer){return Type_Info_Curve_Fill3;}//  C:\Users\vodan\4ed\code/meta_print.cpp:220:
function void
read_Curve_Fill3(Data_Reader &r, Curve_Fill3 &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:225:
function void
read_Curve_Fill3(Data_Reader &r, Curve_Fill3 &pointer)
{
STB_Parser *p = r.parser;
eat_char(p, '{');
Vertex_Index m_verts[3] = {};

{
eat_id(p, strlit("verts"));
eat_char(p, '{');read_Vertex_Index(r, m_verts[0]);read_Vertex_Index(r, m_verts[1]);read_Vertex_Index(r, m_verts[2]);eat_char(p, '}');
}
copy_array_dst(pointer.verts, m_verts);

eat_char(p, '}');
}
//  C:\Users\vodan\4ed\code/meta_entity.cpp:109:
struct Curve_Fill_Bez{Curve_Index curve;};//  C:\Users\vodan\4ed\code/meta_print.cpp:169:
struct Curve_Fill_Bez;
function Type_Info
get_type_info_Curve_Fill_Bez();//  C:\Users\vodan\4ed\code/meta_print.cpp:174:
function Type_Info
get_type_info_Curve_Fill_Bez()
{
Type_Info result = {};
result.name = strlit("Curve_Fill_Bez");
result.size = sizeof(Curve_Fill_Bez);
result.kind = I_Type_Kind_Struct;
result.members.set_count(1);
result.members[0] = {.type=&Type_Info_Curve_Index, .name=strlit("curve"), .offset=offsetof(Curve_Fill_Bez, curve)};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:85:
global_decl Type_Info Type_Info_Curve_Fill_Bez;
//  C:\Users\vodan\4ed\code/meta_print.cpp:92:
xglobal Type_Info Type_Info_Curve_Fill_Bez = get_type_info_Curve_Fill_Bez();

function Type_Info &type_info_from_pointer(Curve_Fill_Bez*pointer){return Type_Info_Curve_Fill_Bez;}//  C:\Users\vodan\4ed\code/meta_print.cpp:220:
function void
read_Curve_Fill_Bez(Data_Reader &r, Curve_Fill_Bez &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:225:
function void
read_Curve_Fill_Bez(Data_Reader &r, Curve_Fill_Bez &pointer)
{
STB_Parser *p = r.parser;
eat_char(p, '{');
Curve_Index m_curve = {};

{
eat_id(p, strlit("curve"));
read_Curve_Index(r, m_curve);
}
pointer.curve = m_curve;

eat_char(p, '}');
}
//  C:\Users\vodan\4ed\code/meta_entity.cpp:109:
struct Curve_Fill_DBez{Curve_Index curve1;
Curve_Index curve2;};//  C:\Users\vodan\4ed\code/meta_print.cpp:169:
struct Curve_Fill_DBez;
function Type_Info
get_type_info_Curve_Fill_DBez();//  C:\Users\vodan\4ed\code/meta_print.cpp:174:
function Type_Info
get_type_info_Curve_Fill_DBez()
{
Type_Info result = {};
result.name = strlit("Curve_Fill_DBez");
result.size = sizeof(Curve_Fill_DBez);
result.kind = I_Type_Kind_Struct;
result.members.set_count(2);
result.members[0] = {.type=&Type_Info_Curve_Index, .name=strlit("curve1"), .offset=offsetof(Curve_Fill_DBez, curve1)};
result.members[1] = {.type=&Type_Info_Curve_Index, .name=strlit("curve2"), .offset=offsetof(Curve_Fill_DBez, curve2)};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:85:
global_decl Type_Info Type_Info_Curve_Fill_DBez;
//  C:\Users\vodan\4ed\code/meta_print.cpp:92:
xglobal Type_Info Type_Info_Curve_Fill_DBez = get_type_info_Curve_Fill_DBez();

function Type_Info &type_info_from_pointer(Curve_Fill_DBez*pointer){return Type_Info_Curve_Fill_DBez;}//  C:\Users\vodan\4ed\code/meta_print.cpp:220:
function void
read_Curve_Fill_DBez(Data_Reader &r, Curve_Fill_DBez &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:225:
function void
read_Curve_Fill_DBez(Data_Reader &r, Curve_Fill_DBez &pointer)
{
STB_Parser *p = r.parser;
eat_char(p, '{');
Curve_Index m_curve1 = {};

{
eat_id(p, strlit("curve1"));
read_Curve_Index(r, m_curve1);
}
pointer.curve1 = m_curve1;

Curve_Index m_curve2 = {};

{
eat_id(p, strlit("curve2"));
read_Curve_Index(r, m_curve2);
}
pointer.curve2 = m_curve2;

eat_char(p, '}');
}
//  C:\Users\vodan\4ed\code/meta_entity.cpp:118:
union Curve_Union{Curve_v3v2 v3v2;
Curve_Parabola parabola;
Curve_Offset offset;
Curve_Unit unit;
Curve_Unit2 unit2;
Curve_Line line;
Curve_Bezd_Old bezd_old;
Curve_Raw raw;
Curve_C2 c2;
Curve_NegateX negateX;
Curve_Lerp lerp;
Curve_Circle circle;
Curve_Fill3 fill3;
Curve_Fill_Bez fill_bez;
Curve_Fill_DBez fill_dbez;};//  C:\Users\vodan\4ed\code/meta_print.cpp:306:
union Curve_Union;
function Type_Info
get_type_info_Curve_Union();
//  C:\Users\vodan\4ed\code/meta_print.cpp:312:
function Type_Info
get_type_info_Curve_Union()
{
Type_Info result = {};
result.name = strlit("Curve_Union");
result.size = sizeof(Curve_Union);
result.kind = I_Type_Kind_Union;
result.discriminator_type = &Type_Info_Curve_Type;
result.union_members.set_count(15);
result.union_members[0] = {.type=&Type_Info_Curve_v3v2, .name=strlit("v3v2"), .variant=Curve_Type_v3v2};
result.union_members[1] = {.type=&Type_Info_Curve_Parabola, .name=strlit("Parabola"), .variant=Curve_Type_Parabola};
result.union_members[2] = {.type=&Type_Info_Curve_Offset, .name=strlit("Offset"), .variant=Curve_Type_Offset};
result.union_members[3] = {.type=&Type_Info_Curve_Unit, .name=strlit("Unit"), .variant=Curve_Type_Unit};
result.union_members[4] = {.type=&Type_Info_Curve_Unit2, .name=strlit("Unit2"), .variant=Curve_Type_Unit2};
result.union_members[5] = {.type=&Type_Info_Curve_Line, .name=strlit("Line"), .variant=Curve_Type_Line};
result.union_members[6] = {.type=&Type_Info_Curve_Bezd_Old, .name=strlit("Bezd_Old"), .variant=Curve_Type_Bezd_Old};
result.union_members[7] = {.type=&Type_Info_Curve_Raw, .name=strlit("Raw"), .variant=Curve_Type_Raw};
result.union_members[8] = {.type=&Type_Info_Curve_C2, .name=strlit("C2"), .variant=Curve_Type_C2};
result.union_members[9] = {.type=&Type_Info_Curve_NegateX, .name=strlit("NegateX"), .variant=Curve_Type_NegateX};
result.union_members[10] = {.type=&Type_Info_Curve_Lerp, .name=strlit("Lerp"), .variant=Curve_Type_Lerp};
result.union_members[11] = {.type=&Type_Info_Curve_Circle, .name=strlit("Circle"), .variant=Curve_Type_Circle};
result.union_members[12] = {.type=&Type_Info_Curve_Fill3, .name=strlit("Fill3"), .variant=Curve_Type_Fill3};
result.union_members[13] = {.type=&Type_Info_Curve_Fill_Bez, .name=strlit("Fill_Bez"), .variant=Curve_Type_Fill_Bez};
result.union_members[14] = {.type=&Type_Info_Curve_Fill_DBez, .name=strlit("Fill_DBez"), .variant=Curve_Type_Fill_DBez};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:85:
global_decl Type_Info Type_Info_Curve_Union;
//  C:\Users\vodan\4ed\code/meta_print.cpp:92:
xglobal Type_Info Type_Info_Curve_Union = get_type_info_Curve_Union();

function Type_Info &type_info_from_pointer(Curve_Union*pointer){return Type_Info_Curve_Union;}//  C:\Users\vodan\4ed\code/meta_print.cpp:341:
function void
read_Curve_Union(Data_Reader &r, Curve_Union &pointer, Curve_Type variant);
//  C:\Users\vodan\4ed\code/meta_print.cpp:346:
function void
read_Curve_Union(Data_Reader &r, Curve_Union &pointer, Curve_Type variant)
{
STB_Parser *p = r.parser;
switch(variant)
{
case Curve_Type_v3v2:
{
read_Curve_v3v2(r, pointer.v3v2);
break;
}
case Curve_Type_Parabola:
{
read_Curve_Parabola(r, pointer.parabola);
break;
}
case Curve_Type_Offset:
{
read_Curve_Offset(r, pointer.offset);
break;
}
case Curve_Type_Unit:
{
read_Curve_Unit(r, pointer.unit);
break;
}
case Curve_Type_Unit2:
{
read_Curve_Unit2(r, pointer.unit2);
break;
}
case Curve_Type_Line:
{
read_Curve_Line(r, pointer.line);
break;
}
case Curve_Type_Bezd_Old:
{
read_Curve_Bezd_Old(r, pointer.bezd_old);
break;
}
case Curve_Type_Raw:
{
read_Curve_Raw(r, pointer.raw);
break;
}
case Curve_Type_C2:
{
read_Curve_C2(r, pointer.c2);
break;
}
case Curve_Type_NegateX:
{
read_Curve_NegateX(r, pointer.negateX);
break;
}
case Curve_Type_Lerp:
{
read_Curve_Lerp(r, pointer.lerp);
break;
}
case Curve_Type_Circle:
{
read_Curve_Circle(r, pointer.circle);
break;
}
case Curve_Type_Fill3:
{
read_Curve_Fill3(r, pointer.fill3);
break;
}
case Curve_Type_Fill_Bez:
{
read_Curve_Fill_Bez(r, pointer.fill_bez);
break;
}
case Curve_Type_Fill_DBez:
{
read_Curve_Fill_DBez(r, pointer.fill_dbez);
break;
}

}

}
//  C:\Users\vodan\4ed\code/meta_entity.cpp:137:
global Entity_Type_Info entity_variant_info_table[35];
function void
init_entity_type_info_table(){
Entity_Type_Info *table = entity_variant_info_table;
table[11].is_curve = 1;
table[1].is_curve = 1;
table[2].is_curve = 1;
table[3].is_curve = 1;
table[4].is_curve = 1;
table[6].is_curve = 1;
table[7].is_curve = 1;
table[10].is_curve = 1;
table[5].is_curve = 1;
table[8].is_curve = 1;
table[9].is_curve = 1;
table[32].is_curve = 0;
table[33].is_curve = 0;
table[34].is_curve = 0;
table[35].is_curve = 0;
};
struct Curve_Data{String name;
Bone_ID bone_id;
b32 symx;
Curve_Type type;
Curve_Union data;
Line_Params params;
Common_Line_Params_Index cparams;
argb fill_color;
Fill_Params fill_params;
i1 linum;};//  C:\Users\vodan\4ed\code/meta_print.cpp:169:
struct Curve_Data;
function Type_Info
get_type_info_Curve_Data();//  C:\Users\vodan\4ed\code/meta_print.cpp:174:
function Type_Info
get_type_info_Curve_Data()
{
Type_Info result = {};
result.name = strlit("Curve_Data");
result.size = sizeof(Curve_Data);
result.kind = I_Type_Kind_Struct;
result.members.set_count(10);
result.members[0] = {.type=&Type_Info_String, .name=strlit("name"), .offset=offsetof(Curve_Data, name)};
result.members[1] = {.type=&Type_Info_Bone_ID, .name=strlit("bone_id"), .offset=offsetof(Curve_Data, bone_id)};
result.members[2] = {.type=&Type_Info_b32, .name=strlit("symx"), .offset=offsetof(Curve_Data, symx)};
result.members[3] = {.type=&Type_Info_Curve_Type, .name=strlit("type"), .offset=offsetof(Curve_Data, type)};
result.members[4] = {.type=&Type_Info_Curve_Union, .name=strlit("data"), .offset=offsetof(Curve_Data, data), .discriminator_offset=offsetof(Curve_Data, type)};
result.members[5] = {.type=&Type_Info_Line_Params, .name=strlit("params"), .offset=offsetof(Curve_Data, params)};
result.members[6] = {.type=&Type_Info_Common_Line_Params_Index, .name=strlit("cparams"), .offset=offsetof(Curve_Data, cparams)};
result.members[7] = {.type=&Type_Info_argb, .name=strlit("fill_color"), .offset=offsetof(Curve_Data, fill_color)};
result.members[8] = {.type=&Type_Info_Fill_Params, .name=strlit("fill_params"), .offset=offsetof(Curve_Data, fill_params)};
result.members[9] = {.type=&Type_Info_i1, .name=strlit("linum"), .offset=offsetof(Curve_Data, linum), .unserialized=true};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:85:
global_decl Type_Info Type_Info_Curve_Data;
//  C:\Users\vodan\4ed\code/meta_print.cpp:92:
xglobal Type_Info Type_Info_Curve_Data = get_type_info_Curve_Data();

function Type_Info &type_info_from_pointer(Curve_Data*pointer){return Type_Info_Curve_Data;}//  C:\Users\vodan\4ed\code/meta_print.cpp:220:
function void
read_Curve_Data(Data_Reader &r, Curve_Data &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:225:
function void
read_Curve_Data(Data_Reader &r, Curve_Data &pointer)
{
STB_Parser *p = r.parser;
eat_char(p, '{');
String m_name = {};

{
eat_id(p, strlit("name"));
read_String(r, m_name);
}
pointer.name = m_name;

Bone_ID m_bone_id = {};

{
eat_id(p, strlit("bone_id"));
read_Bone_ID(r, m_bone_id);
}
pointer.bone_id = m_bone_id;

b32 m_symx = {};

{
eat_id(p, strlit("symx"));
read_b32(r, m_symx);
}
pointer.symx = m_symx;

Curve_Type m_type = {};

{
eat_id(p, strlit("type"));
read_Curve_Type(r, m_type);
}
pointer.type = m_type;

Curve_Union m_data = {};

{
eat_id(p, strlit("data"));
read_Curve_Union(r, m_data, pointer.type);
}
pointer.data = m_data;

Line_Params m_params = {};

{
eat_id(p, strlit("params"));
read_Line_Params(r, m_params);
}
pointer.params = m_params;

Common_Line_Params_Index m_cparams = {};

{
eat_id(p, strlit("cparams"));
read_Common_Line_Params_Index(r, m_cparams);
}
pointer.cparams = m_cparams;

argb m_fill_color = {};

{
eat_id(p, strlit("fill_color"));
read_argb(r, m_fill_color);
}
pointer.fill_color = m_fill_color;

Fill_Params m_fill_params = {};

{
eat_id(p, strlit("fill_params"));
read_Fill_Params(r, m_fill_params);
}
pointer.fill_params = m_fill_params;

eat_char(p, '}');
}
;
//-
struct Keyboard_Cursor{v3 pos;
v1 vel;};//  C:\Users\vodan\4ed\code/meta_print.cpp:169:
struct Keyboard_Cursor;
function Type_Info
get_type_info_Keyboard_Cursor();//  C:\Users\vodan\4ed\code/meta_print.cpp:174:
function Type_Info
get_type_info_Keyboard_Cursor()
{
Type_Info result = {};
result.name = strlit("Keyboard_Cursor");
result.size = sizeof(Keyboard_Cursor);
result.kind = I_Type_Kind_Struct;
result.members.set_count(2);
result.members[0] = {.type=&Type_Info_v3, .name=strlit("pos"), .offset=offsetof(Keyboard_Cursor, pos)};
result.members[1] = {.type=&Type_Info_v1, .name=strlit("vel"), .offset=offsetof(Keyboard_Cursor, vel)};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:85:
global_decl Type_Info Type_Info_Keyboard_Cursor;
//  C:\Users\vodan\4ed\code/meta_print.cpp:92:
xglobal Type_Info Type_Info_Keyboard_Cursor = get_type_info_Keyboard_Cursor();

function Type_Info &type_info_from_pointer(Keyboard_Cursor*pointer){return Type_Info_Keyboard_Cursor;}//  C:\Users\vodan\4ed\code/meta_print.cpp:220:
function void
read_Keyboard_Cursor(Data_Reader &r, Keyboard_Cursor &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:225:
function void
read_Keyboard_Cursor(Data_Reader &r, Keyboard_Cursor &pointer)
{
STB_Parser *p = r.parser;
eat_char(p, '{');
v3 m_pos = {};

{
eat_id(p, strlit("pos"));
read_v3(r, m_pos);
}
pointer.pos = m_pos;

v1 m_vel = {};

{
eat_id(p, strlit("vel"));
read_v1(r, m_vel);
}
pointer.vel = m_vel;

eat_char(p, '}');
}
;
struct Serialized_State{Keyboard_Cursor kb_cursor;};//  C:\Users\vodan\4ed\code/meta_print.cpp:169:
struct Serialized_State;
function Type_Info
get_type_info_Serialized_State();//  C:\Users\vodan\4ed\code/meta_print.cpp:174:
function Type_Info
get_type_info_Serialized_State()
{
Type_Info result = {};
result.name = strlit("Serialized_State");
result.size = sizeof(Serialized_State);
result.kind = I_Type_Kind_Struct;
result.members.set_count(1);
result.members[0] = {.type=&Type_Info_Keyboard_Cursor, .name=strlit("kb_cursor"), .offset=offsetof(Serialized_State, kb_cursor)};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:85:
global_decl Type_Info Type_Info_Serialized_State;
//  C:\Users\vodan\4ed\code/meta_print.cpp:92:
xglobal Type_Info Type_Info_Serialized_State = get_type_info_Serialized_State();

function Type_Info &type_info_from_pointer(Serialized_State*pointer){return Type_Info_Serialized_State;}//  C:\Users\vodan\4ed\code/meta_print.cpp:220:
function void
read_Serialized_State(Data_Reader &r, Serialized_State &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:225:
function void
read_Serialized_State(Data_Reader &r, Serialized_State &pointer)
{
STB_Parser *p = r.parser;
eat_char(p, '{');
Keyboard_Cursor m_kb_cursor = {};

{
eat_id(p, strlit("kb_cursor"));
read_Keyboard_Cursor(r, m_kb_cursor);
}
pointer.kb_cursor = m_kb_cursor;

eat_char(p, '}');
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:477:
#define Serialized_State_Embed \
 union\
{\
struct\
{\
Keyboard_Cursor kb_cursor;\
\
};\
Serialized_State Serialized_State;\
};\
;
//-