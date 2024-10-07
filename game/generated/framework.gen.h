//NOTE Generated at C:\Users\vodan\4ed\code/meta_klang.cpp:26:
// NOTE: source: C:\Users\vodan\4ed\code\game\framework.kh
#pragma once
struct Vertex_Data
{
String name;
Bone_ID bone_id;
v3 pos;
i1 linum;
};
//  C:\Users\vodan\4ed\code/meta_print.cpp:384:
enum Bezier_Type
{
Bezier_Type_v3v2 = 0,
Bezier_Type_Parabola = 1,
Bezier_Type_Offsets = 2,
Bezier_Type_Unit = 3,
Bezier_Type_Unit2 = 4,
Bezier_Type_C2 = 5,
Bezier_Type_Line = 6,
Bezier_Type_Bezd_Old = 7,
};
//  C:\Users\vodan\4ed\code/meta_main.cpp:137:
struct Bezier_v3v2
{
v3 d0;
v2 d3;
};
//  C:\Users\vodan\4ed\code/meta_main.cpp:137:
struct Bezier_Parabola
{
v3 d;
};
//  C:\Users\vodan\4ed\code/meta_main.cpp:137:
struct Bezier_Offsets
{
v3 d0;
v3 d3;
};
//  C:\Users\vodan\4ed\code/meta_main.cpp:137:
struct Bezier_Unit
{
v2 d0;
v2 d3;
v3 unit_y;
};
//  C:\Users\vodan\4ed\code/meta_main.cpp:137:
struct Bezier_Unit2
{
v4 d0d3;
v3 unit_y;
};
//  C:\Users\vodan\4ed\code/meta_main.cpp:137:
struct Bezier_C2
{
Curve_Index ref;
v3 d3;
};
//  C:\Users\vodan\4ed\code/meta_main.cpp:137:
struct Bezier_Line
{

};
//  C:\Users\vodan\4ed\code/meta_main.cpp:137:
struct Bezier_Bezd_Old
{
v3 d0;
v2 d3;
};
//  C:\Users\vodan\4ed\code/meta_main.cpp:146:
union Bezier_Union
{
Bezier_v3v2 v3v2;
Bezier_Parabola parabola;
Bezier_Offsets offsets;
Bezier_Unit unit;
Bezier_Unit2 unit2;
Bezier_C2 c2;
Bezier_Line line;
Bezier_Bezd_Old bezd_old;
};
struct Bezier_Data
{
String name;
Bone_ID bone_id;
b32 symx;
Bezier_Type type;
Vertex_Index p0_index;
Vertex_Index p3_index;
Bezier_Union data;
Line_Params params;
Common_Line_Params_Index cparams;
i1 linum;
};
struct Triangle_Data
{
Vertex_Index verts[3];
argb color;
b32 symx;
Fill_Params params;
i1 linum;
};
struct Keyboard_Cursor
{
v3 pos;
v1 vel;
};
struct Serialized_State
{
Keyboard_Cursor kb_cursor;
};
//  C:\Users\vodan\4ed\code/meta_print.cpp:497:
#define Serialized_State_Embed \
 union\
{\
struct\
{\
Keyboard_Cursor kb_cursor;\
\
};\
Serialized_State Serialized_State;\
};\
