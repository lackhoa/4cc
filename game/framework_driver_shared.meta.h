// C:\Users\vodan\4ed\code/meta_introspect.cpp:344:
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
// C:\Users\vodan\4ed\code/meta_introspect.cpp:140:
global Type_Info Type_Info_Bone_Type = get_type_info_Bone_Type();

function Type_Info &type_info_from_pointer(Bone_Type*pointer)
{
return Type_Info_Bone_Type;
}
function void
read_Bone_Type(Data_Reader &r, Bone_Type &pointer)
{
STB_Parser *p = r.parser;
i32 integer = eat_i1(p);
pointer = *(Bone_Type*)(&integer);
}
static_assert( sizeof(Bone_Type) <= sizeof(i32) );

// C:\Users\vodan\4ed\code/meta_introspect.cpp:165:
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
// C:\Users\vodan\4ed\code/meta_introspect.cpp:140:
global Type_Info Type_Info_Bone_ID = get_type_info_Bone_ID();

function Type_Info &type_info_from_pointer(Bone_ID*pointer)
{
return Type_Info_Bone_ID;
}
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
