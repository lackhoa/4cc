//NOTE Generated at C:\Users\vodan\4ed\code/meta_print.cpp:35:
//  C:\Users\vodan\4ed\code/meta_print.cpp:435:
function Type_Info
get_type_info_argb()
{
Type_Info result = Type_Info_u32;
result.name = strlit("argb");
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:93:
global Type_Info Type_Info_argb = get_type_info_argb();

//  C:\Users\vodan\4ed\code/meta_print.cpp:456:
function void
read_argb(Data_Reader &r, argb &pointer)
{
read_u32(r, pointer);
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:164:
function Type_Info
get_type_info_Vertex_Index()
{
Type_Info result = {};
result.name = strlit("Vertex_Index");
result.size = sizeof(Vertex_Index);
result.kind = Type_Kind_Struct;
result.members.set_count(1);
result.members[0] = {.type=&Type_Info_i1, .name=strlit("v"), .offset=offsetof(Vertex_Index, v)};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:93:
global Type_Info Type_Info_Vertex_Index = get_type_info_Vertex_Index();

function Type_Info &type_info_from_pointer(Vertex_Index*pointer)
{
return Type_Info_Vertex_Index;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:216:
function void
read_Vertex_Index(Data_Reader &r, Vertex_Index &pointer)
{
STB_Parser *p = r.parser;
eat_char(p, '{');
i1 m_v = {};

{
eat_id(p, strlit("v"));
read_i1(r, m_v);
}
pointer.v = m_v;

eat_char(p, '}');
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:164:
function Type_Info
get_type_info_Curve_Index()
{
Type_Info result = {};
result.name = strlit("Curve_Index");
result.size = sizeof(Curve_Index);
result.kind = Type_Kind_Struct;
result.members.set_count(1);
result.members[0] = {.type=&Type_Info_i1, .name=strlit("v"), .offset=offsetof(Curve_Index, v)};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:93:
global Type_Info Type_Info_Curve_Index = get_type_info_Curve_Index();

function Type_Info &type_info_from_pointer(Curve_Index*pointer)
{
return Type_Info_Curve_Index;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:216:
function void
read_Curve_Index(Data_Reader &r, Curve_Index &pointer)
{
STB_Parser *p = r.parser;
eat_char(p, '{');
i1 m_v = {};

{
eat_id(p, strlit("v"));
read_i1(r, m_v);
}
pointer.v = m_v;

eat_char(p, '}');
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:375:
function Type_Info
get_type_info_Bone_Type()
{
Type_Info result = {};
result.name = strlit("Bone_Type");
result.size = sizeof(Bone_Type);
result.kind = Type_Kind_Enum;
result.enum_members.set_count(12);
result.enum_members[0] = {.name=strlit("Bone_None"), .value=Bone_None};
result.enum_members[1] = {.name=strlit("Bone_Head"), .value=Bone_Head};
result.enum_members[2] = {.name=strlit("Bone_Arm"), .value=Bone_Arm};
result.enum_members[3] = {.name=strlit("Bone_Forearm"), .value=Bone_Forearm};
result.enum_members[4] = {.name=strlit("Bone_Bottom_Phalanx"), .value=Bone_Bottom_Phalanx};
result.enum_members[5] = {.name=strlit("Bone_Mid_Phalanx"), .value=Bone_Mid_Phalanx};
result.enum_members[6] = {.name=strlit("Bone_Top_Phalanx"), .value=Bone_Top_Phalanx};
result.enum_members[7] = {.name=strlit("Bone_Torso"), .value=Bone_Torso};
result.enum_members[8] = {.name=strlit("Bone_References"), .value=Bone_References};
result.enum_members[9] = {.name=strlit("Bone_Hand"), .value=Bone_Hand};
result.enum_members[10] = {.name=strlit("Bone_Thumb"), .value=Bone_Thumb};
result.enum_members[11] = {.name=strlit("Bone_Pelvis"), .value=Bone_Pelvis};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:93:
global Type_Info Type_Info_Bone_Type = get_type_info_Bone_Type();

function Type_Info &type_info_from_pointer(Bone_Type*pointer)
{
return Type_Info_Bone_Type;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:406:
function void
read_Bone_Type(Data_Reader &r, Bone_Type &pointer)
{
STB_Parser *p = r.parser;
i32 integer = eat_i1(p);
pointer = *(Bone_Type*)(&integer);
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:164:
function Type_Info
get_type_info_Bone_ID()
{
Type_Info result = {};
result.name = strlit("Bone_ID");
result.size = sizeof(Bone_ID);
result.kind = Type_Kind_Struct;
result.members.set_count(2);
result.members[0] = {.type=&Type_Info_Bone_Type, .name=strlit("type"), .offset=offsetof(Bone_ID, type)};
result.members[1] = {.type=&Type_Info_i1, .name=strlit("id"), .offset=offsetof(Bone_ID, id)};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:93:
global Type_Info Type_Info_Bone_ID = get_type_info_Bone_ID();

function Type_Info &type_info_from_pointer(Bone_ID*pointer)
{
return Type_Info_Bone_ID;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:216:
function void
read_Bone_ID(Data_Reader &r, Bone_ID &pointer)
{
STB_Parser *p = r.parser;
eat_char(p, '{');
Bone_Type m_type = {};

{
eat_id(p, strlit("type"));
read_Bone_Type(r, m_type);
}
pointer.type = m_type;

i1 m_id = {};

{
eat_id(p, strlit("id"));
read_i1(r, m_id);
}
pointer.id = m_id;

eat_char(p, '}');
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:435:
function Type_Info
get_type_info_Line_Flags()
{
Type_Info result = Type_Info_u32;
result.name = strlit("Line_Flags");
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:93:
global Type_Info Type_Info_Line_Flags = get_type_info_Line_Flags();

//  C:\Users\vodan\4ed\code/meta_print.cpp:456:
function void
read_Line_Flags(Data_Reader &r, Line_Flags &pointer)
{
read_u32(r, pointer);
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:164:
function Type_Info
get_type_info_Line_Params()
{
Type_Info result = {};
result.name = strlit("Line_Params");
result.size = sizeof(Line_Params);
result.kind = Type_Kind_Struct;
result.members.set_count(6);
result.members[0] = {.type=&Type_Info_v4, .name=strlit("radii"), .offset=offsetof(Line_Params, radii)};
result.members[1] = {.type=&Type_Info_v1, .name=strlit("nslice_per_meter"), .offset=offsetof(Line_Params, nslice_per_meter)};
result.members[2] = {.type=&Type_Info_v1, .name=strlit("visibility"), .offset=offsetof(Line_Params, visibility)};
result.members[3] = {.type=&Type_Info_v1, .name=strlit("alignment_threshold"), .offset=offsetof(Line_Params, alignment_threshold)};
result.members[4] = {.type=&Type_Info_argb, .name=strlit("color"), .offset=offsetof(Line_Params, color)};
result.members[5] = {.type=&Type_Info_Line_Flags, .name=strlit("flags"), .offset=offsetof(Line_Params, flags)};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:93:
global Type_Info Type_Info_Line_Params = get_type_info_Line_Params();

function Type_Info &type_info_from_pointer(Line_Params*pointer)
{
return Type_Info_Line_Params;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:216:
function void
read_Line_Params(Data_Reader &r, Line_Params &pointer)
{
STB_Parser *p = r.parser;
eat_char(p, '{');
v4 m_radii = {};

{
eat_id(p, strlit("radii"));
read_v4(r, m_radii);
}
pointer.radii = m_radii;

v1 m_nslice_per_meter = {};

{
eat_id(p, strlit("nslice_per_meter"));
read_v1(r, m_nslice_per_meter);
}
pointer.nslice_per_meter = m_nslice_per_meter;

v1 m_visibility = {};

{
eat_id(p, strlit("visibility"));
read_v1(r, m_visibility);
}
pointer.visibility = m_visibility;

v1 m_alignment_threshold = {};

{
eat_id(p, strlit("alignment_threshold"));
read_v1(r, m_alignment_threshold);
}
pointer.alignment_threshold = m_alignment_threshold;

argb m_color = {};

{
eat_id(p, strlit("color"));
read_argb(r, m_color);
}
pointer.color = m_color;

Line_Flags m_flags = {};

{
eat_id(p, strlit("flags"));
read_Line_Flags(r, m_flags);
}
pointer.flags = m_flags;

eat_char(p, '}');
}
