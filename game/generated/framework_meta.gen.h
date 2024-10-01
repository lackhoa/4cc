//NOTE Generated at C:\Users\vodan\4ed\code/meta_klang.cpp:27:
#pragma once
//  C:\Users\vodan\4ed\code/meta_print.cpp:158:
struct Vertex_Data;
function Type_Info
get_type_info_Vertex_Data();//  C:\Users\vodan\4ed\code/meta_print.cpp:89:
global_decl Type_Info Type_Info_Vertex_Data;
//  C:\Users\vodan\4ed\code/meta_print.cpp:210:
function void
read_Vertex_Data(Data_Reader &r, Vertex_Data &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:370:
function Type_Info
get_type_info_Bezier_Type();//  C:\Users\vodan\4ed\code/meta_print.cpp:89:
global_decl Type_Info Type_Info_Bezier_Type;
//  C:\Users\vodan\4ed\code/meta_print.cpp:400:
function void
read_Bezier_Type(Data_Reader &r, Bezier_Type &pointer);
static_assert( sizeof(Bezier_Type) <= sizeof(i32) );

//  C:\Users\vodan\4ed\code/meta_print.cpp:158:
struct Bezier_v3v2;
function Type_Info
get_type_info_Bezier_v3v2();//  C:\Users\vodan\4ed\code/meta_print.cpp:89:
global_decl Type_Info Type_Info_Bezier_v3v2;
//  C:\Users\vodan\4ed\code/meta_print.cpp:210:
function void
read_Bezier_v3v2(Data_Reader &r, Bezier_v3v2 &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:158:
struct Bezier_Parabola;
function Type_Info
get_type_info_Bezier_Parabola();//  C:\Users\vodan\4ed\code/meta_print.cpp:89:
global_decl Type_Info Type_Info_Bezier_Parabola;
//  C:\Users\vodan\4ed\code/meta_print.cpp:210:
function void
read_Bezier_Parabola(Data_Reader &r, Bezier_Parabola &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:158:
struct Bezier_Offsets;
function Type_Info
get_type_info_Bezier_Offsets();//  C:\Users\vodan\4ed\code/meta_print.cpp:89:
global_decl Type_Info Type_Info_Bezier_Offsets;
//  C:\Users\vodan\4ed\code/meta_print.cpp:210:
function void
read_Bezier_Offsets(Data_Reader &r, Bezier_Offsets &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:158:
struct Bezier_Planar_Vec;
function Type_Info
get_type_info_Bezier_Planar_Vec();//  C:\Users\vodan\4ed\code/meta_print.cpp:89:
global_decl Type_Info Type_Info_Bezier_Planar_Vec;
//  C:\Users\vodan\4ed\code/meta_print.cpp:210:
function void
read_Bezier_Planar_Vec(Data_Reader &r, Bezier_Planar_Vec &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:158:
struct Bezier_C2;
function Type_Info
get_type_info_Bezier_C2();//  C:\Users\vodan\4ed\code/meta_print.cpp:89:
global_decl Type_Info Type_Info_Bezier_C2;
//  C:\Users\vodan\4ed\code/meta_print.cpp:210:
function void
read_Bezier_C2(Data_Reader &r, Bezier_C2 &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:283:
union Bezier_Union;
function Type_Info
get_type_info_Bezier_Union();
//  C:\Users\vodan\4ed\code/meta_print.cpp:89:
global_decl Type_Info Type_Info_Bezier_Union;
//  C:\Users\vodan\4ed\code/meta_print.cpp:320:
function void
read_Bezier_Union(Data_Reader &r, Bezier_Union &pointer, Bezier_Type variant);
//  C:\Users\vodan\4ed\code/meta_print.cpp:158:
struct Bezier_Data;
function Type_Info
get_type_info_Bezier_Data();//  C:\Users\vodan\4ed\code/meta_print.cpp:89:
global_decl Type_Info Type_Info_Bezier_Data;
//  C:\Users\vodan\4ed\code/meta_print.cpp:210:
function void
read_Bezier_Data(Data_Reader &r, Bezier_Data &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:158:
struct Keyboard_Cursor;
function Type_Info
get_type_info_Keyboard_Cursor();//  C:\Users\vodan\4ed\code/meta_print.cpp:89:
global_decl Type_Info Type_Info_Keyboard_Cursor;
//  C:\Users\vodan\4ed\code/meta_print.cpp:210:
function void
read_Keyboard_Cursor(Data_Reader &r, Keyboard_Cursor &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:158:
struct Serialized_State;
function Type_Info
get_type_info_Serialized_State();//  C:\Users\vodan\4ed\code/meta_print.cpp:89:
global_decl Type_Info Type_Info_Serialized_State;
//  C:\Users\vodan\4ed\code/meta_print.cpp:210:
function void
read_Serialized_State(Data_Reader &r, Serialized_State &pointer);
