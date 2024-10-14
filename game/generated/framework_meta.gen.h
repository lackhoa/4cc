//NOTE Generated at C:\Users\vodan\4ed\code/meta_print.cpp:34:
#pragma once
//  C:\Users\vodan\4ed\code/meta_print.cpp:171:
struct Vertex_Data;
function Type_Info
get_type_info_Vertex_Data();//  C:\Users\vodan\4ed\code/meta_print.cpp:86:
global_decl Type_Info Type_Info_Vertex_Data;
//  C:\Users\vodan\4ed\code/meta_print.cpp:224:
function void
read_Vertex_Data(Data_Reader &r, Vertex_Data &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:400:
function Type_Info
get_type_info_Curve_Type();//  C:\Users\vodan\4ed\code/meta_print.cpp:86:
global_decl Type_Info Type_Info_Curve_Type;
//  C:\Users\vodan\4ed\code/meta_print.cpp:430:
function void
read_Curve_Type(Data_Reader &r, Curve_Type &pointer);
static_assert( sizeof(Curve_Type) <= sizeof(i32) );

//  C:\Users\vodan\4ed\code/meta_print.cpp:171:
struct Curve_v3v2;
function Type_Info
get_type_info_Curve_v3v2();//  C:\Users\vodan\4ed\code/meta_print.cpp:86:
global_decl Type_Info Type_Info_Curve_v3v2;
//  C:\Users\vodan\4ed\code/meta_print.cpp:224:
function void
read_Curve_v3v2(Data_Reader &r, Curve_v3v2 &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:171:
struct Curve_Parabola;
function Type_Info
get_type_info_Curve_Parabola();//  C:\Users\vodan\4ed\code/meta_print.cpp:86:
global_decl Type_Info Type_Info_Curve_Parabola;
//  C:\Users\vodan\4ed\code/meta_print.cpp:224:
function void
read_Curve_Parabola(Data_Reader &r, Curve_Parabola &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:171:
struct Curve_Offset;
function Type_Info
get_type_info_Curve_Offset();//  C:\Users\vodan\4ed\code/meta_print.cpp:86:
global_decl Type_Info Type_Info_Curve_Offset;
//  C:\Users\vodan\4ed\code/meta_print.cpp:224:
function void
read_Curve_Offset(Data_Reader &r, Curve_Offset &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:171:
struct Curve_Unit;
function Type_Info
get_type_info_Curve_Unit();//  C:\Users\vodan\4ed\code/meta_print.cpp:86:
global_decl Type_Info Type_Info_Curve_Unit;
//  C:\Users\vodan\4ed\code/meta_print.cpp:224:
function void
read_Curve_Unit(Data_Reader &r, Curve_Unit &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:171:
struct Curve_Unit2;
function Type_Info
get_type_info_Curve_Unit2();//  C:\Users\vodan\4ed\code/meta_print.cpp:86:
global_decl Type_Info Type_Info_Curve_Unit2;
//  C:\Users\vodan\4ed\code/meta_print.cpp:224:
function void
read_Curve_Unit2(Data_Reader &r, Curve_Unit2 &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:171:
struct Curve_C2;
function Type_Info
get_type_info_Curve_C2();//  C:\Users\vodan\4ed\code/meta_print.cpp:86:
global_decl Type_Info Type_Info_Curve_C2;
//  C:\Users\vodan\4ed\code/meta_print.cpp:224:
function void
read_Curve_C2(Data_Reader &r, Curve_C2 &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:171:
struct Curve_Line;
function Type_Info
get_type_info_Curve_Line();//  C:\Users\vodan\4ed\code/meta_print.cpp:86:
global_decl Type_Info Type_Info_Curve_Line;
//  C:\Users\vodan\4ed\code/meta_print.cpp:224:
function void
read_Curve_Line(Data_Reader &r, Curve_Line &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:171:
struct Curve_Bezd_Old;
function Type_Info
get_type_info_Curve_Bezd_Old();//  C:\Users\vodan\4ed\code/meta_print.cpp:86:
global_decl Type_Info Type_Info_Curve_Bezd_Old;
//  C:\Users\vodan\4ed\code/meta_print.cpp:224:
function void
read_Curve_Bezd_Old(Data_Reader &r, Curve_Bezd_Old &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:171:
struct Curve_NegateX;
function Type_Info
get_type_info_Curve_NegateX();//  C:\Users\vodan\4ed\code/meta_print.cpp:86:
global_decl Type_Info Type_Info_Curve_NegateX;
//  C:\Users\vodan\4ed\code/meta_print.cpp:224:
function void
read_Curve_NegateX(Data_Reader &r, Curve_NegateX &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:171:
struct Curve_Lerp;
function Type_Info
get_type_info_Curve_Lerp();//  C:\Users\vodan\4ed\code/meta_print.cpp:86:
global_decl Type_Info Type_Info_Curve_Lerp;
//  C:\Users\vodan\4ed\code/meta_print.cpp:224:
function void
read_Curve_Lerp(Data_Reader &r, Curve_Lerp &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:313:
union Curve_Union;
function Type_Info
get_type_info_Curve_Union();
//  C:\Users\vodan\4ed\code/meta_print.cpp:86:
global_decl Type_Info Type_Info_Curve_Union;
//  C:\Users\vodan\4ed\code/meta_print.cpp:350:
function void
read_Curve_Union(Data_Reader &r, Curve_Union &pointer, Curve_Type variant);
//  C:\Users\vodan\4ed\code/meta_print.cpp:171:
struct Curve_Data;
function Type_Info
get_type_info_Curve_Data();//  C:\Users\vodan\4ed\code/meta_print.cpp:86:
global_decl Type_Info Type_Info_Curve_Data;
//  C:\Users\vodan\4ed\code/meta_print.cpp:224:
function void
read_Curve_Data(Data_Reader &r, Curve_Data &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:400:
function Type_Info
get_type_info_Fill_Type();//  C:\Users\vodan\4ed\code/meta_print.cpp:86:
global_decl Type_Info Type_Info_Fill_Type;
//  C:\Users\vodan\4ed\code/meta_print.cpp:430:
function void
read_Fill_Type(Data_Reader &r, Fill_Type &pointer);
static_assert( sizeof(Fill_Type) <= sizeof(i32) );

//  C:\Users\vodan\4ed\code/meta_print.cpp:171:
struct Fill_Fill3;
function Type_Info
get_type_info_Fill_Fill3();//  C:\Users\vodan\4ed\code/meta_print.cpp:86:
global_decl Type_Info Type_Info_Fill_Fill3;
//  C:\Users\vodan\4ed\code/meta_print.cpp:224:
function void
read_Fill_Fill3(Data_Reader &r, Fill_Fill3 &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:171:
struct Fill_Bez;
function Type_Info
get_type_info_Fill_Bez();//  C:\Users\vodan\4ed\code/meta_print.cpp:86:
global_decl Type_Info Type_Info_Fill_Bez;
//  C:\Users\vodan\4ed\code/meta_print.cpp:224:
function void
read_Fill_Bez(Data_Reader &r, Fill_Bez &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:171:
struct Fill_DBez;
function Type_Info
get_type_info_Fill_DBez();//  C:\Users\vodan\4ed\code/meta_print.cpp:86:
global_decl Type_Info Type_Info_Fill_DBez;
//  C:\Users\vodan\4ed\code/meta_print.cpp:224:
function void
read_Fill_DBez(Data_Reader &r, Fill_DBez &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:313:
union Fill_Union;
function Type_Info
get_type_info_Fill_Union();
//  C:\Users\vodan\4ed\code/meta_print.cpp:86:
global_decl Type_Info Type_Info_Fill_Union;
//  C:\Users\vodan\4ed\code/meta_print.cpp:350:
function void
read_Fill_Union(Data_Reader &r, Fill_Union &pointer, Fill_Type variant);
//  C:\Users\vodan\4ed\code/meta_print.cpp:171:
struct Fill_Data;
function Type_Info
get_type_info_Fill_Data();//  C:\Users\vodan\4ed\code/meta_print.cpp:86:
global_decl Type_Info Type_Info_Fill_Data;
//  C:\Users\vodan\4ed\code/meta_print.cpp:224:
function void
read_Fill_Data(Data_Reader &r, Fill_Data &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:171:
struct Keyboard_Cursor;
function Type_Info
get_type_info_Keyboard_Cursor();//  C:\Users\vodan\4ed\code/meta_print.cpp:86:
global_decl Type_Info Type_Info_Keyboard_Cursor;
//  C:\Users\vodan\4ed\code/meta_print.cpp:224:
function void
read_Keyboard_Cursor(Data_Reader &r, Keyboard_Cursor &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:171:
struct Serialized_State;
function Type_Info
get_type_info_Serialized_State();//  C:\Users\vodan\4ed\code/meta_print.cpp:86:
global_decl Type_Info Type_Info_Serialized_State;
//  C:\Users\vodan\4ed\code/meta_print.cpp:224:
function void
read_Serialized_State(Data_Reader &r, Serialized_State &pointer);
