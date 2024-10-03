//NOTE Generated at C:\Users\vodan\4ed\code/meta_print.cpp:34:
#pragma once
//  C:\Users\vodan\4ed\code/meta_print.cpp:434:
function Type_Info
get_type_info_argb();//  C:\Users\vodan\4ed\code/meta_print.cpp:86:
global_decl Type_Info Type_Info_argb;
//  C:\Users\vodan\4ed\code/meta_print.cpp:455:
function void
read_argb(Data_Reader &r, argb &pointer)//  C:\Users\vodan\4ed\code/meta_print.cpp:162:
struct Vertex_Index;
function Type_Info
get_type_info_Vertex_Index();//  C:\Users\vodan\4ed\code/meta_print.cpp:86:
global_decl Type_Info Type_Info_Vertex_Index;
//  C:\Users\vodan\4ed\code/meta_print.cpp:214:
function void
read_Vertex_Index(Data_Reader &r, Vertex_Index &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:162:
struct Curve_Index;
function Type_Info
get_type_info_Curve_Index();//  C:\Users\vodan\4ed\code/meta_print.cpp:86:
global_decl Type_Info Type_Info_Curve_Index;
//  C:\Users\vodan\4ed\code/meta_print.cpp:214:
function void
read_Curve_Index(Data_Reader &r, Curve_Index &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:374:
function Type_Info
get_type_info_Bone_Type();//  C:\Users\vodan\4ed\code/meta_print.cpp:86:
global_decl Type_Info Type_Info_Bone_Type;
//  C:\Users\vodan\4ed\code/meta_print.cpp:404:
function void
read_Bone_Type(Data_Reader &r, Bone_Type &pointer);
static_assert( sizeof(Bone_Type) <= sizeof(i32) );

//  C:\Users\vodan\4ed\code/meta_print.cpp:162:
struct Bone_ID;
function Type_Info
get_type_info_Bone_ID();//  C:\Users\vodan\4ed\code/meta_print.cpp:86:
global_decl Type_Info Type_Info_Bone_ID;
//  C:\Users\vodan\4ed\code/meta_print.cpp:214:
function void
read_Bone_ID(Data_Reader &r, Bone_ID &pointer);
//  C:\Users\vodan\4ed\code/meta_print.cpp:434:
function Type_Info
get_type_info_Line_Flags();//  C:\Users\vodan\4ed\code/meta_print.cpp:86:
global_decl Type_Info Type_Info_Line_Flags;
//  C:\Users\vodan\4ed\code/meta_print.cpp:455:
function void
read_Line_Flags(Data_Reader &r, Line_Flags &pointer)//  C:\Users\vodan\4ed\code/meta_print.cpp:162:
struct Line_Params;
function Type_Info
get_type_info_Line_Params();//  C:\Users\vodan\4ed\code/meta_print.cpp:86:
global_decl Type_Info Type_Info_Line_Params;
//  C:\Users\vodan\4ed\code/meta_print.cpp:214:
function void
read_Line_Params(Data_Reader &r, Line_Params &pointer);
