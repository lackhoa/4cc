//NOTE Generated at C:\Users\vodan\4ed\code/meta_print.cpp:35:
//  C:\Users\vodan\4ed\code/meta_print.cpp:177:
function Type_Info
get_type_info_Vertex_Data()
{
Type_Info result = {};
result.name = strlit("Vertex_Data");
result.size = sizeof(Vertex_Data);
result.kind = Type_Kind_Struct;
result.members.set_count(4);
result.members[0] = {.type=&Type_Info_String, .name=strlit("name"), .offset=offsetof(Vertex_Data, name)};
result.members[1] = {.type=&Type_Info_Bone_ID, .name=strlit("bone_id"), .offset=offsetof(Vertex_Data, bone_id)};
result.members[2] = {.type=&Type_Info_v3, .name=strlit("pos"), .offset=offsetof(Vertex_Data, pos)};
result.members[3] = {.type=&Type_Info_i1, .name=strlit("linum"), .offset=offsetof(Vertex_Data, linum), .unserialized=true};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:93:
global Type_Info Type_Info_Vertex_Data = get_type_info_Vertex_Data();

function Type_Info &type_info_from_pointer(Vertex_Data*pointer)
{
return Type_Info_Vertex_Data;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:230:
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
//  C:\Users\vodan\4ed\code/meta_print.cpp:405:
function Type_Info
get_type_info_Bezier_Type()
{
Type_Info result = {};
result.name = strlit("Bezier_Type");
result.size = sizeof(Bezier_Type);
result.kind = Type_Kind_Enum;
result.enum_members.set_count(8);
result.enum_members[0] = {.name=strlit("Bezier_Type_v3v2"), .value=Bezier_Type_v3v2};
result.enum_members[1] = {.name=strlit("Bezier_Type_Parabola"), .value=Bezier_Type_Parabola};
result.enum_members[2] = {.name=strlit("Bezier_Type_Offsets"), .value=Bezier_Type_Offsets};
result.enum_members[3] = {.name=strlit("Bezier_Type_Unit"), .value=Bezier_Type_Unit};
result.enum_members[4] = {.name=strlit("Bezier_Type_Unit2"), .value=Bezier_Type_Unit2};
result.enum_members[5] = {.name=strlit("Bezier_Type_C2"), .value=Bezier_Type_C2};
result.enum_members[6] = {.name=strlit("Bezier_Type_Line"), .value=Bezier_Type_Line};
result.enum_members[7] = {.name=strlit("Bezier_Type_Bezd_Old"), .value=Bezier_Type_Bezd_Old};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:93:
global Type_Info Type_Info_Bezier_Type = get_type_info_Bezier_Type();

function Type_Info &type_info_from_pointer(Bezier_Type*pointer)
{
return Type_Info_Bezier_Type;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:436:
function void
read_Bezier_Type(Data_Reader &r, Bezier_Type &pointer)
{
STB_Parser *p = r.parser;
i32 integer = eat_i1(p);
pointer = *(Bezier_Type*)(&integer);
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:177:
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
//  C:\Users\vodan\4ed\code/meta_print.cpp:93:
global Type_Info Type_Info_Bezier_v3v2 = get_type_info_Bezier_v3v2();

function Type_Info &type_info_from_pointer(Bezier_v3v2*pointer)
{
return Type_Info_Bezier_v3v2;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:230:
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
//  C:\Users\vodan\4ed\code/meta_print.cpp:177:
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
//  C:\Users\vodan\4ed\code/meta_print.cpp:93:
global Type_Info Type_Info_Bezier_Parabola = get_type_info_Bezier_Parabola();

function Type_Info &type_info_from_pointer(Bezier_Parabola*pointer)
{
return Type_Info_Bezier_Parabola;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:230:
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
//  C:\Users\vodan\4ed\code/meta_print.cpp:177:
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
//  C:\Users\vodan\4ed\code/meta_print.cpp:93:
global Type_Info Type_Info_Bezier_Offsets = get_type_info_Bezier_Offsets();

function Type_Info &type_info_from_pointer(Bezier_Offsets*pointer)
{
return Type_Info_Bezier_Offsets;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:230:
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
//  C:\Users\vodan\4ed\code/meta_print.cpp:177:
function Type_Info
get_type_info_Bezier_Unit()
{
Type_Info result = {};
result.name = strlit("Bezier_Unit");
result.size = sizeof(Bezier_Unit);
result.kind = Type_Kind_Struct;
result.members.set_count(3);
result.members[0] = {.type=&Type_Info_v2, .name=strlit("d0"), .offset=offsetof(Bezier_Unit, d0)};
result.members[1] = {.type=&Type_Info_v2, .name=strlit("d3"), .offset=offsetof(Bezier_Unit, d3)};
result.members[2] = {.type=&Type_Info_v3, .name=strlit("unit_y"), .offset=offsetof(Bezier_Unit, unit_y)};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:93:
global Type_Info Type_Info_Bezier_Unit = get_type_info_Bezier_Unit();

function Type_Info &type_info_from_pointer(Bezier_Unit*pointer)
{
return Type_Info_Bezier_Unit;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:230:
function void
read_Bezier_Unit(Data_Reader &r, Bezier_Unit &pointer)
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
//  C:\Users\vodan\4ed\code/meta_print.cpp:177:
function Type_Info
get_type_info_Bezier_Unit2()
{
Type_Info result = {};
result.name = strlit("Bezier_Unit2");
result.size = sizeof(Bezier_Unit2);
result.kind = Type_Kind_Struct;
result.members.set_count(2);
result.members[0] = {.type=&Type_Info_v4, .name=strlit("d0d3"), .offset=offsetof(Bezier_Unit2, d0d3)};
result.members[1] = {.type=&Type_Info_v3, .name=strlit("unit_y"), .offset=offsetof(Bezier_Unit2, unit_y)};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:93:
global Type_Info Type_Info_Bezier_Unit2 = get_type_info_Bezier_Unit2();

function Type_Info &type_info_from_pointer(Bezier_Unit2*pointer)
{
return Type_Info_Bezier_Unit2;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:230:
function void
read_Bezier_Unit2(Data_Reader &r, Bezier_Unit2 &pointer)
{
STB_Parser *p = r.parser;
eat_char(p, '{');
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

eat_char(p, '}');
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:177:
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
//  C:\Users\vodan\4ed\code/meta_print.cpp:93:
global Type_Info Type_Info_Bezier_C2 = get_type_info_Bezier_C2();

function Type_Info &type_info_from_pointer(Bezier_C2*pointer)
{
return Type_Info_Bezier_C2;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:230:
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
//  C:\Users\vodan\4ed\code/meta_print.cpp:177:
function Type_Info
get_type_info_Bezier_Line()
{
Type_Info result = {};
result.name = strlit("Bezier_Line");
result.size = sizeof(Bezier_Line);
result.kind = Type_Kind_Struct;
result.members.set_count(0);
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:93:
global Type_Info Type_Info_Bezier_Line = get_type_info_Bezier_Line();

function Type_Info &type_info_from_pointer(Bezier_Line*pointer)
{
return Type_Info_Bezier_Line;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:230:
function void
read_Bezier_Line(Data_Reader &r, Bezier_Line &pointer)
{
STB_Parser *p = r.parser;
eat_char(p, '{');
eat_char(p, '}');
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:177:
function Type_Info
get_type_info_Bezier_Bezd_Old()
{
Type_Info result = {};
result.name = strlit("Bezier_Bezd_Old");
result.size = sizeof(Bezier_Bezd_Old);
result.kind = Type_Kind_Struct;
result.members.set_count(2);
result.members[0] = {.type=&Type_Info_v3, .name=strlit("d0"), .offset=offsetof(Bezier_Bezd_Old, d0)};
result.members[1] = {.type=&Type_Info_v2, .name=strlit("d3"), .offset=offsetof(Bezier_Bezd_Old, d3)};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:93:
global Type_Info Type_Info_Bezier_Bezd_Old = get_type_info_Bezier_Bezd_Old();

function Type_Info &type_info_from_pointer(Bezier_Bezd_Old*pointer)
{
return Type_Info_Bezier_Bezd_Old;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:230:
function void
read_Bezier_Bezd_Old(Data_Reader &r, Bezier_Bezd_Old &pointer)
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
//  C:\Users\vodan\4ed\code/meta_print.cpp:320:
function Type_Info
get_type_info_Bezier_Union()
{
Type_Info result = {};
result.name = strlit("Bezier_Union");
result.size = sizeof(Bezier_Union);
result.kind = Type_Kind_Union;
result.discriminator_type = &Type_Info_Bezier_Type;
result.union_members.set_count(8);
result.union_members[0] = {.type=&Type_Info_Bezier_v3v2, .name=strlit("v3v2"), .variant=Bezier_Type_v3v2};
result.union_members[1] = {.type=&Type_Info_Bezier_Parabola, .name=strlit("Parabola"), .variant=Bezier_Type_Parabola};
result.union_members[2] = {.type=&Type_Info_Bezier_Offsets, .name=strlit("Offsets"), .variant=Bezier_Type_Offsets};
result.union_members[3] = {.type=&Type_Info_Bezier_Unit, .name=strlit("Unit"), .variant=Bezier_Type_Unit};
result.union_members[4] = {.type=&Type_Info_Bezier_Unit2, .name=strlit("Unit2"), .variant=Bezier_Type_Unit2};
result.union_members[5] = {.type=&Type_Info_Bezier_C2, .name=strlit("C2"), .variant=Bezier_Type_C2};
result.union_members[6] = {.type=&Type_Info_Bezier_Line, .name=strlit("Line"), .variant=Bezier_Type_Line};
result.union_members[7] = {.type=&Type_Info_Bezier_Bezd_Old, .name=strlit("Bezd_Old"), .variant=Bezier_Type_Bezd_Old};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:93:
global Type_Info Type_Info_Bezier_Union = get_type_info_Bezier_Union();

function Type_Info &type_info_from_pointer(Bezier_Union*pointer)
{
return Type_Info_Bezier_Union;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:356:
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
read_Bezier_Parabola(r, pointer.parabola);
break;
}
case Bezier_Type_Offsets:
{
read_Bezier_Offsets(r, pointer.offsets);
break;
}
case Bezier_Type_Unit:
{
read_Bezier_Unit(r, pointer.unit);
break;
}
case Bezier_Type_Unit2:
{
read_Bezier_Unit2(r, pointer.unit2);
break;
}
case Bezier_Type_C2:
{
read_Bezier_C2(r, pointer.c2);
break;
}
case Bezier_Type_Line:
{
read_Bezier_Line(r, pointer.line);
break;
}
case Bezier_Type_Bezd_Old:
{
read_Bezier_Bezd_Old(r, pointer.bezd_old);
break;
}

}

}
//  C:\Users\vodan\4ed\code/meta_print.cpp:177:
function Type_Info
get_type_info_Bezier_Data()
{
Type_Info result = {};
result.name = strlit("Bezier_Data");
result.size = sizeof(Bezier_Data);
result.kind = Type_Kind_Struct;
result.members.set_count(10);
result.members[0] = {.type=&Type_Info_String, .name=strlit("name"), .offset=offsetof(Bezier_Data, name)};
result.members[1] = {.type=&Type_Info_Bone_ID, .name=strlit("bone_id"), .offset=offsetof(Bezier_Data, bone_id)};
result.members[2] = {.type=&Type_Info_b32, .name=strlit("symx"), .offset=offsetof(Bezier_Data, symx)};
result.members[3] = {.type=&Type_Info_Bezier_Type, .name=strlit("type"), .offset=offsetof(Bezier_Data, type)};
result.members[4] = {.type=&Type_Info_Vertex_Index, .name=strlit("p0_index"), .offset=offsetof(Bezier_Data, p0_index)};
result.members[5] = {.type=&Type_Info_Vertex_Index, .name=strlit("p3_index"), .offset=offsetof(Bezier_Data, p3_index)};
result.members[6] = {.type=&Type_Info_Bezier_Union, .name=strlit("data"), .offset=offsetof(Bezier_Data, data), .discriminator_offset=offsetof(Bezier_Data, type)};
result.members[7] = {.type=&Type_Info_Line_Params, .name=strlit("params"), .offset=offsetof(Bezier_Data, params)};
result.members[8] = {.type=&Type_Info_Common_Line_Params_Index, .name=strlit("cparams"), .offset=offsetof(Bezier_Data, cparams)};
result.members[9] = {.type=&Type_Info_i1, .name=strlit("linum"), .offset=offsetof(Bezier_Data, linum), .unserialized=true};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:93:
global Type_Info Type_Info_Bezier_Data = get_type_info_Bezier_Data();

function Type_Info &type_info_from_pointer(Bezier_Data*pointer)
{
return Type_Info_Bezier_Data;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:230:
function void
read_Bezier_Data(Data_Reader &r, Bezier_Data &pointer)
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

Bezier_Type m_type = {};

{
eat_id(p, strlit("type"));
read_Bezier_Type(r, m_type);
}
pointer.type = m_type;

Vertex_Index m_p0_index = {};

{
eat_id(p, strlit("p0_index"));
read_Vertex_Index(r, m_p0_index);
}
pointer.p0_index = m_p0_index;

Vertex_Index m_p3_index = {};

{
eat_id(p, strlit("p3_index"));
read_Vertex_Index(r, m_p3_index);
}
pointer.p3_index = m_p3_index;

Bezier_Union m_data = {};

{
eat_id(p, strlit("data"));
read_Bezier_Union(r, m_data, pointer.type);
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

eat_char(p, '}');
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:177:
function Type_Info
get_type_info_Triangle_Data()
{
Type_Info result = {};
result.name = strlit("Triangle_Data");
result.size = sizeof(Triangle_Data);
result.kind = Type_Kind_Struct;
result.members.set_count(5);
result.members[0] = {.type=&Type_Info_Vertex_Index, .name=strlit("verts"), .offset=offsetof(Triangle_Data, verts)};
result.members[1] = {.type=&Type_Info_argb, .name=strlit("color"), .offset=offsetof(Triangle_Data, color)};
result.members[2] = {.type=&Type_Info_b32, .name=strlit("symx"), .offset=offsetof(Triangle_Data, symx)};
result.members[3] = {.type=&Type_Info_Fill_Params, .name=strlit("params"), .offset=offsetof(Triangle_Data, params)};
result.members[4] = {.type=&Type_Info_i1, .name=strlit("linum"), .offset=offsetof(Triangle_Data, linum), .unserialized=true};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:93:
global Type_Info Type_Info_Triangle_Data = get_type_info_Triangle_Data();

function Type_Info &type_info_from_pointer(Triangle_Data*pointer)
{
return Type_Info_Triangle_Data;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:230:
function void
read_Triangle_Data(Data_Reader &r, Triangle_Data &pointer)
{
STB_Parser *p = r.parser;
eat_char(p, '{');
Vertex_Index m_verts[3] = {};

{
eat_id(p, strlit("verts"));
eat_char(p, '{');read_Vertex_Index(r, m_verts[0]);read_Vertex_Index(r, m_verts[1]);read_Vertex_Index(r, m_verts[2]);eat_char(p, '}');
}
copy_array_dst(pointer.verts, m_verts);

argb m_color = {};

{
eat_id(p, strlit("color"));
read_argb(r, m_color);
}
pointer.color = m_color;

b32 m_symx = {};

{
eat_id(p, strlit("symx"));
read_b32(r, m_symx);
}
pointer.symx = m_symx;

Fill_Params m_params = {};

{
eat_id(p, strlit("params"));
read_Fill_Params(r, m_params);
}
pointer.params = m_params;

eat_char(p, '}');
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:177:
function Type_Info
get_type_info_Keyboard_Cursor()
{
Type_Info result = {};
result.name = strlit("Keyboard_Cursor");
result.size = sizeof(Keyboard_Cursor);
result.kind = Type_Kind_Struct;
result.members.set_count(2);
result.members[0] = {.type=&Type_Info_v3, .name=strlit("pos"), .offset=offsetof(Keyboard_Cursor, pos)};
result.members[1] = {.type=&Type_Info_v1, .name=strlit("vel"), .offset=offsetof(Keyboard_Cursor, vel)};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:93:
global Type_Info Type_Info_Keyboard_Cursor = get_type_info_Keyboard_Cursor();

function Type_Info &type_info_from_pointer(Keyboard_Cursor*pointer)
{
return Type_Info_Keyboard_Cursor;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:230:
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
//  C:\Users\vodan\4ed\code/meta_print.cpp:177:
function Type_Info
get_type_info_Serialized_State()
{
Type_Info result = {};
result.name = strlit("Serialized_State");
result.size = sizeof(Serialized_State);
result.kind = Type_Kind_Struct;
result.members.set_count(1);
result.members[0] = {.type=&Type_Info_Keyboard_Cursor, .name=strlit("kb_cursor"), .offset=offsetof(Serialized_State, kb_cursor)};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:93:
global Type_Info Type_Info_Serialized_State = get_type_info_Serialized_State();

function Type_Info &type_info_from_pointer(Serialized_State*pointer)
{
return Type_Info_Serialized_State;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:230:
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
