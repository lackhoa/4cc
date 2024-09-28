//  C:\Users\vodan\4ed\code/meta_print.cpp:88:
global Type_Info Type_Info_Vertex_Data = get_type_info_Vertex_Data();

function Type_Info &type_info_from_pointer(Vertex_Data*pointer)
{
return Type_Info_Vertex_Data;
}
//  C:\Users\vodan\4ed\code/meta_print.cpp:88:
global Type_Info Type_Info_Bezier_Data = get_type_info_Bezier_Data();

function Type_Info &type_info_from_pointer(Bezier_Data*pointer)
{
return Type_Info_Bezier_Data;
}
