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

global Type_Info Vertex_Data_Type_Info = get_Type_Info_Vertex_Data();