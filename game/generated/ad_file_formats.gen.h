//NOTE File created programmatically by C:\Users\vodan\4ed\code/meta_klang.cpp:513:
// NOTE: source: C:\Users\vodan\4ed\code\game\ad_file_formats.kh
#pragma once
//-
struct Camera_Data{
v1 distance;
v1 phi;
v1 theta;
v1 roll;
v3 pan;
v3 pivot;
};
//  C:\Users\vodan\4ed\code/meta_print.cpp:167:
struct Camera_Data;
function Type_Info
get_type_info_Camera_Data();//  C:\Users\vodan\4ed\code/meta_print.cpp:172:
function Type_Info
get_type_info_Camera_Data()
{
Type_Info result = {};
result.name = strlit("Camera_Data");
result.size = sizeof(Camera_Data);
result.kind = I_Type_Kind_Struct;
result.members.set_count(6);
{Type_Info *member_type = & Type_Info_v1;
result.members[0] = {.type=member_type, .name=strlit("distance"), .offset=offsetof(Camera_Data, distance)};
}{Type_Info *member_type = & Type_Info_v1;
result.members[1] = {.type=member_type, .name=strlit("phi"), .offset=offsetof(Camera_Data, phi)};
}{Type_Info *member_type = & Type_Info_v1;
result.members[2] = {.type=member_type, .name=strlit("theta"), .offset=offsetof(Camera_Data, theta)};
}{Type_Info *member_type = & Type_Info_v1;
result.members[3] = {.type=member_type, .name=strlit("roll"), .offset=offsetof(Camera_Data, roll)};
}{Type_Info *member_type = & Type_Info_v3;
result.members[4] = {.type=member_type, .name=strlit("pan"), .offset=offsetof(Camera_Data, pan)};
}{Type_Info *member_type = & Type_Info_v3;
result.members[5] = {.type=member_type, .name=strlit("pivot"), .offset=offsetof(Camera_Data, pivot)};
}return result;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:88:
global Type_Info Type_Info_Camera_Data;

function Type_Info &type_info_from_pointer(Camera_Data*pointer){return Type_Info_Camera_Data;}//  C:\Users\vodan\4ed\code/meta_print.cpp:238:
function void
read_Camera_Data(Data_Reader &r, Camera_Data &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:243:
function void
read_Camera_Data(Data_Reader &r, Camera_Data &pointer)
{
STB_Parser *p = r.parser;
eat_char(p, '{');
v1 m_distance = {};

{
eat_id(p, strlit("distance"));
read_v1(r, m_distance);
}
pointer.distance = m_distance;

v1 m_phi = {};

{
eat_id(p, strlit("phi"));
read_v1(r, m_phi);
}
pointer.phi = m_phi;

v1 m_theta = {};

{
eat_id(p, strlit("theta"));
read_v1(r, m_theta);
}
pointer.theta = m_theta;

v1 m_roll = {};

{
eat_id(p, strlit("roll"));
read_v1(r, m_roll);
}
pointer.roll = m_roll;

v3 m_pan = {};

{
eat_id(p, strlit("pan"));
read_v3(r, m_pan);
}
pointer.pan = m_pan;

v3 m_pivot = {};

{
eat_id(p, strlit("pivot"));
read_v3(r, m_pivot);
}
pointer.pivot = m_pivot;

eat_char(p, '}');
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:495:
#define Camera_Data_Embed \
 union\
{\
struct\
{\
v1 distance;\
v1 phi;\
v1 theta;\
v1 roll;\
v3 pan;\
v3 pivot;\
\
};\
Camera_Data Camera_Data;\
};\
;
//-