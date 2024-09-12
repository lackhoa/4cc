function Type_Info
get_Type_Info_Vertex_Data()
{
Type_Info result = {};
result.name = strlit("Vertex_Data");
result.size = sizeof(Vertex_Data);
result.members.set_count(5);
result.members[0] = {.type=&Type_Info_String, .name=strlit("name"), .offset=offsetof(Vertex_Data, name)};
result.members[1] = {.type=&Type_Info_i1, .name=strlit("bone_index"), .offset=offsetof(Vertex_Data, bone_index)};
result.members[2] = {.type=&Type_Info_i1, .name=strlit("symx"), .offset=offsetof(Vertex_Data, symx)};
result.members[3] = {.type=&Type_Info_v3, .name=strlit("pos"), .offset=offsetof(Vertex_Data, pos)};
result.members[4] = {.type=&Type_Info_i1, .name=strlit("basis_index"), .offset=offsetof(Vertex_Data, basis_index)};
return result;
}
global Type_Info Type_Info_Vertex_Data = get_Type_Info_Vertex_Data();

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
function Type_Info
get_Type_Info_Bezier_Type()
{
 Type_Info result = {};
 result.name = strlit("Bezier_Type");
 result.size = sizeof(Bezier_Type);
 result.enum_values.set_count(4);
 result.enum_values[0] = {.name=strlit("Bezier_Type_Planar_v3_v2"), .value=Bezier_Type_Planar_v3_v2};
 result.enum_values[1] = {.name=strlit("Bezier_Type_Raw"), .value=Bezier_Type_Raw};
 result.enum_values[2] = {.name=strlit("Bezier_Type_Offsets"), .value=Bezier_Type_Offsets};
 result.enum_values[3] = {.name=strlit("Bezier_Type_Planar_Unit_Vector"), .value=Bezier_Type_Planar_Unit_Vector};
 return result;
}
global Type_Info Type_Info_Bezier_Type = get_Type_Info_Bezier_Type();

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

function Type_Info
get_Type_Info_Bezier_Data()
{
Type_Info result = {};
result.name = strlit("Bezier_Data");
result.size = sizeof(Bezier_Data);
result.members.set_count(9);
result.members[0] = {.type=&Type_Info_String, .name=strlit("name"), .offset=offsetof(Bezier_Data, name)};
result.members[1] = {.type=&Type_Info_i1, .name=strlit("bone_index"), .offset=offsetof(Bezier_Data, bone_index)};
result.members[2] = {.type=&Type_Info_i1, .name=strlit("symx"), .offset=offsetof(Bezier_Data, symx)};
result.members[3] = {.type=&Type_Info_Bezier_Type, .name=strlit("type"), .offset=offsetof(Bezier_Data, type)};
result.members[4] = {.type=&Type_Info_i1, .name=strlit("p0_index"), .offset=offsetof(Bezier_Data, p0_index)};
result.members[5] = {.type=&Type_Info_v3, .name=strlit("p1"), .offset=offsetof(Bezier_Data, p1)};
result.members[6] = {.type=&Type_Info_v3, .name=strlit("p2"), .offset=offsetof(Bezier_Data, p2)};
result.members[7] = {.type=&Type_Info_i1, .name=strlit("p3_index"), .offset=offsetof(Bezier_Data, p3_index)};
result.members[8] = {.type=&Type_Info_v4, .name=strlit("radii"), .offset=offsetof(Bezier_Data, radii)};
return result;
}
global Type_Info Type_Info_Bezier_Data = get_Type_Info_Bezier_Data();

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

i1 m_object_index = {};
if ( in_range_exclusive(0, r.read_version, Version_Rename_Object_Index) )
{
eat_id(p, strlit("object_index"));
read_i1(r, m_object_index);
}
i1 m_bone_index = m_object_index;
if ( in_range_exclusive(Version_Rename_Object_Index, r.read_version, Version_Inf) )
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

Bezier_Type m_type = Bezier_Type_Raw;
if ( in_range_exclusive(Version_Add_Bezier_Type, r.read_version, Version_Inf) )
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
