//NOTE Generated at C:\Users\vodan\4ed\code/meta_klang.cpp:28:
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
enum Curve_Type
{
Curve_Type_v3v2 = 0,
Curve_Type_Parabola = 1,
Curve_Type_Offset = 2,
Curve_Type_Unit = 3,
Curve_Type_Unit2 = 4,
Curve_Type_C2 = 5,
Curve_Type_Line = 6,
Curve_Type_Bezd_Old = 7,
Curve_Type_NegateX = 8,
Curve_Type_Lerp = 9,
};
//  C:\Users\vodan\4ed\code/meta_main.cpp:162:
struct Curve_v3v2
{
Vertex_Index p0;
v3 d0;
v2 d3;
Vertex_Index p3;
};
//  C:\Users\vodan\4ed\code/meta_main.cpp:162:
struct Curve_Parabola
{
Vertex_Index p0;
v3 d;
Vertex_Index p3;
};
//  C:\Users\vodan\4ed\code/meta_main.cpp:162:
struct Curve_Offset
{
Vertex_Index p0;
v3 d0;
v3 d3;
Vertex_Index p3;
};
//  C:\Users\vodan\4ed\code/meta_main.cpp:162:
struct Curve_Unit
{
Vertex_Index p0;
v2 d0;
v2 d3;
v3 unit_y;
Vertex_Index p3;
};
//  C:\Users\vodan\4ed\code/meta_main.cpp:162:
struct Curve_Unit2
{
Vertex_Index p0;
v4 d0d3;
v3 unit_y;
Vertex_Index p3;
};
//  C:\Users\vodan\4ed\code/meta_main.cpp:162:
struct Curve_C2
{
Curve_Index ref;
v3 d3;
Vertex_Index p3;
};
//  C:\Users\vodan\4ed\code/meta_main.cpp:162:
struct Curve_Line
{
Vertex_Index p0;
Vertex_Index p3;
};
//  C:\Users\vodan\4ed\code/meta_main.cpp:162:
struct Curve_Bezd_Old
{
Vertex_Index p0;
v3 d0;
v2 d3;
Vertex_Index p3;
};
//  C:\Users\vodan\4ed\code/meta_main.cpp:162:
struct Curve_NegateX
{
Curve_Index ref;
};
//  C:\Users\vodan\4ed\code/meta_main.cpp:162:
struct Curve_Lerp
{
Curve_Index begin;
Curve_Index end;
};
//  C:\Users\vodan\4ed\code/meta_main.cpp:171:
union Curve_Union
{
Curve_v3v2 v3v2;
Curve_Parabola parabola;
Curve_Offset offset;
Curve_Unit unit;
Curve_Unit2 unit2;
Curve_C2 c2;
Curve_Line line;
Curve_Bezd_Old bezd_old;
Curve_NegateX negateX;
Curve_Lerp lerp;
};
struct Curve_Data
{
String name;
Bone_ID bone_id;
b32 symx;
Curve_Type type;
Curve_Union data;
Line_Params params;
Common_Line_Params_Index cparams;
i1 linum;
};
//  C:\Users\vodan\4ed\code/meta_print.cpp:384:
enum Fill_Type
{
Fill_Type_Fill3 = 1,
Fill_Type_Bez = 2,
Fill_Type_DBez = 3,
};
//  C:\Users\vodan\4ed\code/meta_main.cpp:447:
struct Fill_Fill3
{
Vertex_Index verts[3];
};
//  C:\Users\vodan\4ed\code/meta_main.cpp:447:
struct Fill_Bez
{
Curve_Index curve;
};
//  C:\Users\vodan\4ed\code/meta_main.cpp:447:
struct Fill_DBez
{
Curve_Index curve1;
Curve_Index curve2;
};
//  C:\Users\vodan\4ed\code/meta_main.cpp:456:
union Fill_Union
{
Fill_Fill3 fill3;
Fill_Bez bez;
Fill_DBez dbez;
};
struct Fill_Data
{
Fill_Type type;
Fill_Union data;
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
