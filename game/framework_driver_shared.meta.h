//C:\Users\vodan\4ed\code/meta_introspect.cpp:163:
function Type_Info
get_type_info_Vertex_Data()
{
Type_Info result = {};
result.name = strlit("Vertex_Data");
result.size = sizeof(Vertex_Data);
result.kind = Type_Kind_Struct;
result.members.set_count(5);
result.members[0] = {.type=&Type_Info_String, .name=strlit("name"), .offset=offsetof(Vertex_Data, name)};
result.members[1] = {.type=&Type_Info_i1, .name=strlit("bone_index"), .offset=offsetof(Vertex_Data, bone_index)};
result.members[2] = {.type=&Type_Info_i1, .name=strlit("symx"), .offset=offsetof(Vertex_Data, symx)};
result.members[3] = {.type=&Type_Info_v3, .name=strlit("pos"), .offset=offsetof(Vertex_Data, pos)};
result.members[4] = {.type=&Type_Info_i1, .name=strlit("basis_index"), .offset=offsetof(Vertex_Data, basis_index)};
return result;
}
//C:\Users\vodan\4ed\code/meta_introspect.cpp:138:
global Type_Info Type_Info_Vertex_Data = get_type_info_Vertex_Data();

function Type_Info &type_info_from_pointer(Vertex_Data*pointer)
{
return Type_Info_Vertex_Data;
}
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

i1 m_object_index = {};
if ( in_range_exclusive(0, r.read_version, Version_Rename_Object_Index2) )
{
eat_id(p, strlit("object_index"));
read_i1(r, m_object_index);
}
i1 m_bone_index = m_object_index;
if ( in_range_exclusive(Version_Rename_Object_Index2, r.read_version, Version_Inf) )
{
eat_id(p, strlit("bone_index"));
read_i1(r, m_bone_index);
}
pointer.bone_index = m_bone_index;

i1 m_symx = {};

{
eat_id(p, strlit("symx"));
read_i1(r, m_symx);
}
pointer.symx = m_symx;

v3 m_pos = {};

{
eat_id(p, strlit("pos"));
read_v3(r, m_pos);
}
pointer.pos = m_pos;

i1 m_basis_index = {};

{
eat_id(p, strlit("basis_index"));
read_i1(r, m_basis_index);
}
pointer.basis_index = m_basis_index;

eat_char(p, '}');
}
//C:\Users\vodan\4ed\code/meta_introspect.cpp:342:
function Type_Info
get_type_info_Bezier_Type()
{
Type_Info result = {};
result.name = strlit("Bezier_Type");
result.size = sizeof(Bezier_Type);
result.kind = Type_Kind_Enum;
result.enum_members.set_count(3);
result.enum_members[0] = {.name=strlit("Bezier_Type_v3v2"), .value=Bezier_Type_v3v2};
result.enum_members[1] = {.name=strlit("Bezier_Type_Offsets"), .value=Bezier_Type_Offsets};
result.enum_members[2] = {.name=strlit("Bezier_Type_Planar_Vec"), .value=Bezier_Type_Planar_Vec};
return result;
}
//C:\Users\vodan\4ed\code/meta_introspect.cpp:138:
global Type_Info Type_Info_Bezier_Type = get_type_info_Bezier_Type();

function Type_Info &type_info_from_pointer(Bezier_Type*pointer)
{
return Type_Info_Bezier_Type;
}
function void
read_Bezier_Type(Data_Reader &r, Bezier_Type &pointer)
{
STB_Parser *p = r.parser;
i32 integer = eat_i1(p);
pointer = *(Bezier_Type*)(&integer);
}
static_assert( sizeof(Bezier_Type) <= sizeof(i32) );

//C:\Users\vodan\4ed\code/meta_introspect.cpp:163:
function Type_Info
get_type_info_Bezier_Data_v3v2()
{
Type_Info result = {};
result.name = strlit("Bezier_Data_v3v2");
result.size = sizeof(Bezier_Data_v3v2);
result.kind = Type_Kind_Struct;
result.members.set_count(2);
result.members[0] = {.type=&Type_Info_v3, .name=strlit("d0"), .offset=offsetof(Bezier_Data_v3v2, d0)};
result.members[1] = {.type=&Type_Info_v2, .name=strlit("d3"), .offset=offsetof(Bezier_Data_v3v2, d3)};
return result;
}
//C:\Users\vodan\4ed\code/meta_introspect.cpp:138:
global Type_Info Type_Info_Bezier_Data_v3v2 = get_type_info_Bezier_Data_v3v2();

function Type_Info &type_info_from_pointer(Bezier_Data_v3v2*pointer)
{
return Type_Info_Bezier_Data_v3v2;
}
function void
read_Bezier_Data_v3v2(Data_Reader &r, Bezier_Data_v3v2 &pointer)
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
//C:\Users\vodan\4ed\code/meta_introspect.cpp:163:
function Type_Info
get_type_info_Bezier_Data_Offsets()
{
Type_Info result = {};
result.name = strlit("Bezier_Data_Offsets");
result.size = sizeof(Bezier_Data_Offsets);
result.kind = Type_Kind_Struct;
result.members.set_count(2);
result.members[0] = {.type=&Type_Info_v3, .name=strlit("d0"), .offset=offsetof(Bezier_Data_Offsets, d0)};
result.members[1] = {.type=&Type_Info_v3, .name=strlit("d3"), .offset=offsetof(Bezier_Data_Offsets, d3)};
return result;
}
//C:\Users\vodan\4ed\code/meta_introspect.cpp:138:
global Type_Info Type_Info_Bezier_Data_Offsets = get_type_info_Bezier_Data_Offsets();

function Type_Info &type_info_from_pointer(Bezier_Data_Offsets*pointer)
{
return Type_Info_Bezier_Data_Offsets;
}
function void
read_Bezier_Data_Offsets(Data_Reader &r, Bezier_Data_Offsets &pointer)
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
//C:\Users\vodan\4ed\code/meta_introspect.cpp:163:
function Type_Info
get_type_info_Bezier_Data_Planar_Vec()
{
Type_Info result = {};
result.name = strlit("Bezier_Data_Planar_Vec");
result.size = sizeof(Bezier_Data_Planar_Vec);
result.kind = Type_Kind_Struct;
result.members.set_count(3);
result.members[0] = {.type=&Type_Info_v2, .name=strlit("d0"), .offset=offsetof(Bezier_Data_Planar_Vec, d0)};
result.members[1] = {.type=&Type_Info_v2, .name=strlit("d3"), .offset=offsetof(Bezier_Data_Planar_Vec, d3)};
result.members[2] = {.type=&Type_Info_v3, .name=strlit("unit_y"), .offset=offsetof(Bezier_Data_Planar_Vec, unit_y)};
return result;
}
//C:\Users\vodan\4ed\code/meta_introspect.cpp:138:
global Type_Info Type_Info_Bezier_Data_Planar_Vec = get_type_info_Bezier_Data_Planar_Vec();

function Type_Info &type_info_from_pointer(Bezier_Data_Planar_Vec*pointer)
{
return Type_Info_Bezier_Data_Planar_Vec;
}
function void
read_Bezier_Data_Planar_Vec(Data_Reader &r, Bezier_Data_Planar_Vec &pointer)
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
//C:\Users\vodan\4ed\code/meta_introspect.cpp:276:
function Type_Info
get_type_info_Bezier_Union()
{
Type_Info result = {};
result.name = strlit("Bezier_Union");
result.size = sizeof(Bezier_Union);
result.kind = Type_Kind_Union;
result.discriminator_type = &Type_Info_Bezier_Type;
result.union_members.set_count(3);
result.union_members[0] = {.type=&Type_Info_Bezier_Data_v3v2, .name=strlit("v3v2"), .variant=Bezier_Type_v3v2};
result.union_members[1] = {.type=&Type_Info_Bezier_Data_Offsets, .name=strlit("offsets"), .variant=Bezier_Type_Offsets};
result.union_members[2] = {.type=&Type_Info_Bezier_Data_Planar_Vec, .name=strlit("planar_vec"), .variant=Bezier_Type_Planar_Vec};
return result;
}
//C:\Users\vodan\4ed\code/meta_introspect.cpp:138:
global Type_Info Type_Info_Bezier_Union = get_type_info_Bezier_Union();

function Type_Info &type_info_from_pointer(Bezier_Union*pointer)
{
return Type_Info_Bezier_Union;
}
//C:\Users\vodan\4ed\code/meta_introspect.cpp:304:
function void
read_Bezier_Union(Data_Reader &r, Bezier_Union &pointer, Bezier_Type variant)
{
STB_Parser *p = r.parser;
switch (variant)
{
case Bezier_Type_v3v2:
{
read_Bezier_Data_v3v2(r, pointer.v3v2);
break;
}
case Bezier_Type_Offsets:
{
read_Bezier_Data_Offsets(r, pointer.offsets);
break;
}
case Bezier_Type_Planar_Vec:
{
read_Bezier_Data_Planar_Vec(r, pointer.planar_vec);
break;
}

}

}
//C:\Users\vodan\4ed\code/meta_introspect.cpp:163:
function Type_Info
get_type_info_Bezier_Data()
{
Type_Info result = {};
result.name = strlit("Bezier_Data");
result.size = sizeof(Bezier_Data);
result.kind = Type_Kind_Struct;
result.members.set_count(8);
result.members[0] = {.type=&Type_Info_String, .name=strlit("name"), .offset=offsetof(Bezier_Data, name)};
result.members[1] = {.type=&Type_Info_i1, .name=strlit("bone_index"), .offset=offsetof(Bezier_Data, bone_index)};
result.members[2] = {.type=&Type_Info_i1, .name=strlit("symx"), .offset=offsetof(Bezier_Data, symx)};
result.members[3] = {.type=&Type_Info_Bezier_Type, .name=strlit("type"), .offset=offsetof(Bezier_Data, type)};
result.members[4] = {.type=&Type_Info_i1, .name=strlit("p0_index"), .offset=offsetof(Bezier_Data, p0_index)};
result.members[5] = {.type=&Type_Info_Bezier_Union, .name=strlit("data"), .offset=offsetof(Bezier_Data, data), .discriminator_offset=offsetof(Bezier_Data, type)};
result.members[6] = {.type=&Type_Info_i1, .name=strlit("p3_index"), .offset=offsetof(Bezier_Data, p3_index)};
result.members[7] = {.type=&Type_Info_v4, .name=strlit("radii"), .offset=offsetof(Bezier_Data, radii)};
return result;
}
//C:\Users\vodan\4ed\code/meta_introspect.cpp:138:
global Type_Info Type_Info_Bezier_Data = get_type_info_Bezier_Data();

function Type_Info &type_info_from_pointer(Bezier_Data*pointer)
{
return Type_Info_Bezier_Data;
}
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

i1 m_bone_index = {};

{
eat_id(p, strlit("bone_index"));
read_i1(r, m_bone_index);
}
pointer.bone_index = m_bone_index;

i1 m_symx = {};

{
eat_id(p, strlit("symx"));
read_i1(r, m_symx);
}
pointer.symx = m_symx;

Bezier_Type m_type = {};

{
eat_id(p, strlit("type"));
read_Bezier_Type(r, m_type);
}
pointer.type = m_type;

i1 m_p0_index = {};

{
eat_id(p, strlit("p0_index"));
read_i1(r, m_p0_index);
}
pointer.p0_index = m_p0_index;

Bezier_Union m_data = {};

{
eat_id(p, strlit("data"));
read_Bezier_Union(r, m_data, pointer.type);
}
pointer.data = m_data;

i1 m_p3_index = {};

{
eat_id(p, strlit("p3_index"));
read_i1(r, m_p3_index);
}
pointer.p3_index = m_p3_index;

v4 m_radii = {};

{
eat_id(p, strlit("radii"));
read_v4(r, m_radii);
}
pointer.radii = m_radii;

eat_char(p, '}');
}
