//NOTE Generated at C:\Users\vodan\4ed\code/meta_print.cpp:35:
//  C:\Users\vodan\4ed\code/meta_print.cpp:465:
function Type_Info
get_type_info_argb()
{
Type_Info result = Type_Info_u32;
result.name = strlit("argb");
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:93:
global Type_Info Type_Info_argb = get_type_info_argb();

//  C:\Users\vodan\4ed\code/meta_print.cpp:486:
function void
read_argb(Data_Reader &r, argb &pointer)
{
read_u32(r, pointer);
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:465:
function Type_Info
get_type_info_b32()
{
Type_Info result = Type_Info_i1;
result.name = strlit("b32");
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:93:
global Type_Info Type_Info_b32 = get_type_info_b32();

//  C:\Users\vodan\4ed\code/meta_print.cpp:486:
function void
read_b32(Data_Reader &r, b32 &pointer)
{
read_i1(r, pointer);
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:177:
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
//  C:\Users\vodan\4ed\code/meta_print.cpp:230:
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
//  C:\Users\vodan\4ed\code/meta_print.cpp:177:
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
//  C:\Users\vodan\4ed\code/meta_print.cpp:230:
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
//  C:\Users\vodan\4ed\code/meta_print.cpp:177:
function Type_Info
get_type_info_Fill_Index()
{
Type_Info result = {};
result.name = strlit("Fill_Index");
result.size = sizeof(Fill_Index);
result.kind = Type_Kind_Struct;
result.members.set_count(1);
result.members[0] = {.type=&Type_Info_i1, .name=strlit("v"), .offset=offsetof(Fill_Index, v)};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:93:
global Type_Info Type_Info_Fill_Index = get_type_info_Fill_Index();

function Type_Info &type_info_from_pointer(Fill_Index*pointer)
{
return Type_Info_Fill_Index;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:230:
function void
read_Fill_Index(Data_Reader &r, Fill_Index &pointer)
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
//  C:\Users\vodan\4ed\code/meta_print.cpp:405:
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
//  C:\Users\vodan\4ed\code/meta_print.cpp:436:
function void
read_Bone_Type(Data_Reader &r, Bone_Type &pointer)
{
STB_Parser *p = r.parser;
i32 integer = eat_i1(p);
pointer = *(Bone_Type*)(&integer);
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:177:
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
//  C:\Users\vodan\4ed\code/meta_print.cpp:230:
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
//  C:\Users\vodan\4ed\code/meta_print.cpp:465:
function Type_Info
get_type_info_Line_Flags()
{
Type_Info result = Type_Info_u32;
result.name = strlit("Line_Flags");
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:93:
global Type_Info Type_Info_Line_Flags = get_type_info_Line_Flags();

//  C:\Users\vodan\4ed\code/meta_print.cpp:486:
function void
read_Line_Flags(Data_Reader &r, Line_Flags &pointer)
{
read_u32(r, pointer);
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:177:
function Type_Info
get_type_info_Common_Line_Params()
{
Type_Info result = {};
result.name = strlit("Common_Line_Params");
result.size = sizeof(Common_Line_Params);
result.kind = Type_Kind_Struct;
result.members.set_count(7);
result.members[0] = {.type=&Type_Info_v1, .name=strlit("radius_mult"), .offset=offsetof(Common_Line_Params, radius_mult)};
result.members[1] = {.type=&Type_Info_v1, .name=strlit("nslice_per_meter"), .offset=offsetof(Common_Line_Params, nslice_per_meter)};
result.members[2] = {.type=&Type_Info_argb, .name=strlit("color"), .offset=offsetof(Common_Line_Params, color)};
result.members[3] = {.type=&Type_Info_v1, .name=strlit("depth_offset"), .offset=offsetof(Common_Line_Params, depth_offset)};
result.members[4] = {.type=&Type_Info_Line_Flags, .name=strlit("flags"), .offset=offsetof(Common_Line_Params, flags)};
result.members[5] = {.type=&Type_Info_v4, .name=strlit("radii"), .offset=offsetof(Common_Line_Params, radii)};
result.members[6] = {.type=&Type_Info_i1, .name=strlit("linum"), .offset=offsetof(Common_Line_Params, linum), .unserialized=true};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:93:
global Type_Info Type_Info_Common_Line_Params = get_type_info_Common_Line_Params();

function Type_Info &type_info_from_pointer(Common_Line_Params*pointer)
{
return Type_Info_Common_Line_Params;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:230:
function void
read_Common_Line_Params(Data_Reader &r, Common_Line_Params &pointer)
{
STB_Parser *p = r.parser;
eat_char(p, '{');
v1 m_radius_mult = {};

{
eat_id(p, strlit("radius_mult"));
read_v1(r, m_radius_mult);
}
pointer.radius_mult = m_radius_mult;

v1 m_nslice_per_meter = {};

{
eat_id(p, strlit("nslice_per_meter"));
read_v1(r, m_nslice_per_meter);
}
pointer.nslice_per_meter = m_nslice_per_meter;

argb m_color = {};

{
eat_id(p, strlit("color"));
read_argb(r, m_color);
}
pointer.color = m_color;

v1 m_depth_offset = {};

{
eat_id(p, strlit("depth_offset"));
read_v1(r, m_depth_offset);
}
pointer.depth_offset = m_depth_offset;

Line_Flags m_flags = {};

{
eat_id(p, strlit("flags"));
read_Line_Flags(r, m_flags);
}
pointer.flags = m_flags;

v4 m_radii = {};

{
eat_id(p, strlit("radii"));
read_v4(r, m_radii);
}
pointer.radii = m_radii;

eat_char(p, '}');
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:177:
function Type_Info
get_type_info_Line_Params()
{
Type_Info result = {};
result.name = strlit("Line_Params");
result.size = sizeof(Line_Params);
result.kind = Type_Kind_Struct;
result.members.set_count(5);
result.members[0] = {.type=&Type_Info_v4, .name=strlit("radii"), .offset=offsetof(Line_Params, radii)};
result.members[1] = {.type=&Type_Info_v1, .name=strlit("visibility"), .offset=offsetof(Line_Params, visibility)};
result.members[2] = {.type=&Type_Info_Line_Flags, .name=strlit("flags"), .offset=offsetof(Line_Params, flags)};
result.members[3] = {.type=&Type_Info_v3, .name=strlit("unit_normal"), .offset=offsetof(Line_Params, unit_normal)};
result.members[4] = {.type=&Type_Info_v1, .name=strlit("alignment_min"), .offset=offsetof(Line_Params, alignment_min)};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:93:
global Type_Info Type_Info_Line_Params = get_type_info_Line_Params();

function Type_Info &type_info_from_pointer(Line_Params*pointer)
{
return Type_Info_Line_Params;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:230:
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

v1 m_visibility = {};

{
eat_id(p, strlit("visibility"));
read_v1(r, m_visibility);
}
pointer.visibility = m_visibility;

Line_Flags m_flags = {};

{
eat_id(p, strlit("flags"));
read_Line_Flags(r, m_flags);
}
pointer.flags = m_flags;

v3 m_unit_normal = {};

{
eat_id(p, strlit("unit_normal"));
read_v3(r, m_unit_normal);
}
pointer.unit_normal = m_unit_normal;

v1 m_alignment_min = {};

{
eat_id(p, strlit("alignment_min"));
read_v1(r, m_alignment_min);
}
pointer.alignment_min = m_alignment_min;

eat_char(p, '}');
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:465:
function Type_Info
get_type_info_Poly_Flags()
{
Type_Info result = Type_Info_u32;
result.name = strlit("Poly_Flags");
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:93:
global Type_Info Type_Info_Poly_Flags = get_type_info_Poly_Flags();

//  C:\Users\vodan\4ed\code/meta_print.cpp:486:
function void
read_Poly_Flags(Data_Reader &r, Poly_Flags &pointer)
{
read_u32(r, pointer);
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:177:
function Type_Info
get_type_info_Fill_Params()
{
Type_Info result = {};
result.name = strlit("Fill_Params");
result.size = sizeof(Fill_Params);
result.kind = Type_Kind_Struct;
result.members.set_count(2);
result.members[0] = {.type=&Type_Info_argb, .name=strlit("color"), .offset=offsetof(Fill_Params, color)};
result.members[1] = {.type=&Type_Info_Poly_Flags, .name=strlit("flags"), .offset=offsetof(Fill_Params, flags)};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:93:
global Type_Info Type_Info_Fill_Params = get_type_info_Fill_Params();

function Type_Info &type_info_from_pointer(Fill_Params*pointer)
{
return Type_Info_Fill_Params;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:230:
function void
read_Fill_Params(Data_Reader &r, Fill_Params &pointer)
{
STB_Parser *p = r.parser;
eat_char(p, '{');
argb m_color = {};

{
eat_id(p, strlit("color"));
read_argb(r, m_color);
}
pointer.color = m_color;

Poly_Flags m_flags = {};

{
eat_id(p, strlit("flags"));
read_Poly_Flags(r, m_flags);
}
pointer.flags = m_flags;

eat_char(p, '}');
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:177:
function Type_Info
get_type_info_Common_Line_Params_Index()
{
Type_Info result = {};
result.name = strlit("Common_Line_Params_Index");
result.size = sizeof(Common_Line_Params_Index);
result.kind = Type_Kind_Struct;
result.members.set_count(1);
result.members[0] = {.type=&Type_Info_i1, .name=strlit("v"), .offset=offsetof(Common_Line_Params_Index, v)};
return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:93:
global Type_Info Type_Info_Common_Line_Params_Index = get_type_info_Common_Line_Params_Index();

function Type_Info &type_info_from_pointer(Common_Line_Params_Index*pointer)
{
return Type_Info_Common_Line_Params_Index;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:230:
function void
read_Common_Line_Params_Index(Data_Reader &r, Common_Line_Params_Index &pointer)
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
