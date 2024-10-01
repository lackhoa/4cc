struct Vertex_Data
{
String name;
Bone_ID bone_id;
i1 symx;
v3 pos;
i1 basic_index;
i1 linum;
};
//  C:\Users\vodan\4ed\code/meta_print.cpp:354:
enum Bezier_Type
{
Bezier_Type_v3v2 = 0,
Bezier_Type_Parabola = 1,
Bezier_Type_Offsets = 2,
Bezier_Type_Planar_Vec = 3,
Bezier_Type_C2 = 4,
};
//  C:\Users\vodan\4ed\code/meta_main.cpp:114:
struct Bezier_v3v2
{
v3 d0;
v2 d3;
};
//  C:\Users\vodan\4ed\code/meta_main.cpp:114:
struct Bezier_Parabola
{
v3 d;
};
//  C:\Users\vodan\4ed\code/meta_main.cpp:114:
struct Bezier_Offsets
{
v3 d0;
v3 d3;
};
//  C:\Users\vodan\4ed\code/meta_main.cpp:114:
struct Bezier_Planar_Vec
{
v2 d0;
v2 d3;
v3 unit_y;
};
//  C:\Users\vodan\4ed\code/meta_main.cpp:114:
struct Bezier_C2
{
Curve_Index ref;
v3 d3;
};
//  C:\Users\vodan\4ed\code/meta_main.cpp:122:
union Bezier_Union
{
Bezier_v3v2 v3v2;
Bezier_Parabola Parabola;
Bezier_Offsets Offsets;
Bezier_Planar_Vec Planar_Vec;
Bezier_C2 C2;
};
struct Bezier_Data
{
String name;
Bone_ID bone_id;
i1 symx;
Bezier_Type type;
Vertex_Index p0_index;
Vertex_Index p3_index;
Bezier_Union data;
Line_Params params;
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
//  C:\Users\vodan\4ed\code/meta_print.cpp:467:
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
