function Type_Info get_Type_Info_Vertex_Data()
{
Type_Info result = {};
result.name = strlit("Vertex_Data");
result.size = sizeof(Vertex_Data);
result.members.set_count(5);
result.members[0] = Struct_Member{.type=&Type_Info_String, .name=strlit("name"), .offset=offsetof(Vertex_Data, name)};
result.members[1] = Struct_Member{.type=&Type_Info_i1, .name=strlit("object_index"), .offset=offsetof(Vertex_Data, object_index)};
result.members[2] = Struct_Member{.type=&Type_Info_i1, .name=strlit("symx"), .offset=offsetof(Vertex_Data, symx)};
result.members[3] = Struct_Member{.type=&Type_Info_v3, .name=strlit("pos"), .offset=offsetof(Vertex_Data, pos)};
result.members[4] = Struct_Member{.type=&Type_Info_i1, .name=strlit("basis_index"), .offset=offsetof(Vertex_Data, basis_index)};
return result;
}
global Type_Info Type_Info_Vertex_Data = get_Type_Info_Vertex_Data();

function Type_Info &type_info_of_pointer(Vertex_Data*pointer)
{
return Type_Info_Vertex_Data;
}
function void
read_Vertex_Data(Data_Reader &r, Vertex_Data &pointer)
{
STB_Parser *p = r.parser;
eat_char(p, '{');

{
eat_id(p, strlit("name"));
read_String(r, pointer.name);
}

{
eat_id(p, strlit("object_index"));
read_i1(r, pointer.object_index);
}

{
eat_id(p, strlit("symx"));
read_i1(r, pointer.symx);
}

{
eat_id(p, strlit("pos"));
read_v3(r, pointer.pos);
}

{
eat_id(p, strlit("basis_index"));
read_i1(r, pointer.basis_index);
}
eat_char(p, '}');
}
function Type_Info get_Type_Info_Bezier_Type()
{
Type_Info result = {};
result.name = strlit("Bezier_Type");
result.size = sizeof(Bezier_Type);
return result;
}
global Type_Info Type_Info_Bezier_Type = get_Type_Info_Bezier_Type();

function Type_Info &type_info_of_pointer(Bezier_Type*pointer)
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

function Type_Info get_Type_Info_Bezier_Data()
{
Type_Info result = {};
result.name = strlit("Bezier_Data");
result.size = sizeof(Bezier_Data);
result.members.set_count(9);
result.members[0] = Struct_Member{.type=&Type_Info_String, .name=strlit("name"), .offset=offsetof(Bezier_Data, name)};
result.members[1] = Struct_Member{.type=&Type_Info_i1, .name=strlit("object_index"), .offset=offsetof(Bezier_Data, object_index)};
result.members[2] = Struct_Member{.type=&Type_Info_i1, .name=strlit("symx"), .offset=offsetof(Bezier_Data, symx)};
result.members[3] = Struct_Member{.type=&Type_Info_Bezier_Type, .name=strlit("type"), .offset=offsetof(Bezier_Data, type)};
result.members[4] = Struct_Member{.type=&Type_Info_i1, .name=strlit("p0_index"), .offset=offsetof(Bezier_Data, p0_index)};
result.members[5] = Struct_Member{.type=&Type_Info_v3, .name=strlit("p1"), .offset=offsetof(Bezier_Data, p1)};
result.members[6] = Struct_Member{.type=&Type_Info_v3, .name=strlit("p2"), .offset=offsetof(Bezier_Data, p2)};
result.members[7] = Struct_Member{.type=&Type_Info_i1, .name=strlit("p3_index"), .offset=offsetof(Bezier_Data, p3_index)};
result.members[8] = Struct_Member{.type=&Type_Info_v4, .name=strlit("radii"), .offset=offsetof(Bezier_Data, radii)};
return result;
}
global Type_Info Type_Info_Bezier_Data = get_Type_Info_Bezier_Data();

function Type_Info &type_info_of_pointer(Bezier_Data*pointer)
{
return Type_Info_Bezier_Data;
}
function void
read_Bezier_Data(Data_Reader &r, Bezier_Data &pointer)
{
STB_Parser *p = r.parser;
eat_char(p, '{');

{
eat_id(p, strlit("name"));
read_String(r, pointer.name);
}

{
eat_id(p, strlit("object_index"));
read_i1(r, pointer.object_index);
}

{
eat_id(p, strlit("symx"));
read_i1(r, pointer.symx);
}
if (r.read_version >= Version_Add_Bezier_Type)
{
eat_id(p, strlit("type"));
read_Bezier_Type(r, pointer.type);
}
else
{
pointer.type = Bezier_Type_Raw;
}

{
eat_id(p, strlit("p0_index"));
read_i1(r, pointer.p0_index);
}

{
eat_id(p, strlit("p1"));
read_v3(r, pointer.p1);
}

{
eat_id(p, strlit("p2"));
read_v3(r, pointer.p2);
}

{
eat_id(p, strlit("p3_index"));
read_i1(r, pointer.p3_index);
}

{
eat_id(p, strlit("radii"));
read_v4(r, pointer.radii);
}
eat_char(p, '}');
}
