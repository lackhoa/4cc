#pragma once
//  C:\Users\vodan\4ed\code/meta_print.cpp:109:
function Type_Info
get_type_info_Vertex_Data()
{
Type_Info result = {};
result.name = strlit("Vertex_Data");
result.size = sizeof(Vertex_Data);
result.kind = Type_Kind_Struct;
result.members.set_count(6);
result.members[0] = {.type=&Type_Info_String, .name=strlit("name"), .offset=offsetof(Vertex_Data, name)};
result.members[1] = {.type=&Type_Info_Bone_ID, .name=strlit("bone_id"), .offset=offsetof(Vertex_Data, bone_id)};
result.members[2] = {.type=&Type_Info_i1, .name=strlit("symx"), .offset=offsetof(Vertex_Data, symx)};
result.members[3] = {.type=&Type_Info_v3, .name=strlit("pos"), .offset=offsetof(Vertex_Data, pos)};
result.members[4] = {.type=&Type_Info_i1, .name=strlit("basis_index"), .offset=offsetof(Vertex_Data, basis_index)};
result.members[5] = {.type=&Type_Info_i1, .name=strlit("linum"), .offset=offsetof(Vertex_Data, linum), .unserialized=true};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:81:
global_decl Type_Info Type_Info_Vertex_Data;
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
//  C:\Users\vodan\4ed\code/meta_print.cpp:109:
function Type_Info
get_type_info_Bezier_Data()
{
Type_Info result = {};
result.name = strlit("Bezier_Data");
result.size = sizeof(Bezier_Data);
result.kind = Type_Kind_Struct;
result.members.set_count(9);
result.members[0] = {.type=&Type_Info_String, .name=strlit("name"), .offset=offsetof(Bezier_Data, name)};
result.members[1] = {.type=&Type_Info_Bone_ID, .name=strlit("bone_id"), .offset=offsetof(Bezier_Data, bone_id)};
result.members[2] = {.type=&Type_Info_i1, .name=strlit("symx"), .offset=offsetof(Bezier_Data, symx)};
result.members[3] = {.type=&Type_Info_Bezier_Type, .name=strlit("type"), .offset=offsetof(Bezier_Data, type)};
result.members[4] = {.type=&Type_Info_Vertex_Index, .name=strlit("p0_index"), .offset=offsetof(Bezier_Data, p0_index)};
result.members[5] = {.type=&Type_Info_Vertex_Index, .name=strlit("p3_index"), .offset=offsetof(Bezier_Data, p3_index)};
result.members[6] = {.type=&Type_Info_Bezier_Union, .name=strlit("data"), .offset=offsetof(Bezier_Data, data), .discriminator_offset=offsetof(Bezier_Data, type)};
result.members[7] = {.type=&Type_Info_Line_Params, .name=strlit("params"), .offset=offsetof(Bezier_Data, params)};
result.members[8] = {.type=&Type_Info_i1, .name=strlit("linum"), .offset=offsetof(Bezier_Data, linum), .unserialized=true};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:81:
global_decl Type_Info Type_Info_Bezier_Data;
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

eat_char(p, '}');
}
