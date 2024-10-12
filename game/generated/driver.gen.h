//NOTE Generated at C:\Users\vodan\4ed\code/meta_klang.cpp:28:
// NOTE: source: C:\Users\vodan\4ed\code\game\driver.kh
#pragma once
typedef u32 argb;
typedef i1 b32;
struct Vertex_Index
{
i1 v;
};
inline b32 operator==(Vertex_Index a, Vertex_Index b){ return a.v==b.v; }
struct Curve_Index
{
i1 v;
};
inline b32 operator==(Curve_Index a, Curve_Index b){ return a.v==b.v; }
struct Fill_Index
{
i1 v;
};
inline b32 operator==(Fill_Index a, Fill_Index b){ return a.v==b.v; }
//  C:\Users\vodan\4ed\code/meta_print.cpp:384:
enum Bone_Type
{
Bone_None = 0,
Bone_Head = 1,
Bone_Arm = 2,
Bone_Forearm = 3,
Bone_Bottom_Phalanx = 4,
Bone_Mid_Phalanx = 5,
Bone_Top_Phalanx = 6,
Bone_Torso = 7,
Bone_References = 8,
Bone_Hand = 9,
Bone_Thumb = 10,
Bone_Pelvis = 11,
};
struct Bone_ID
{
Bone_Type type;
i1 id;
};
typedef u32 Line_Flags;
//  C:\Users\vodan\4ed\code/meta_print.cpp:384:
enum 
{
Line_Overlay = 1,
Line_Straight = 2,
Line_No_SymX = 4,
};
struct Common_Line_Params
{
v1 radius_mult;
v1 nslice_per_meter;
argb color;
v1 depth_offset;
Line_Flags flags;
v4 radii;
i1 linum;
};
struct Line_Params
{
v4 radii;
v1 visibility;
v1 alignment_threshold;
Line_Flags flags;
};
typedef u32 Poly_Flags;
//  C:\Users\vodan\4ed\code/meta_print.cpp:384:
enum 
{
Poly_Shaded = 1,
Poly_Line = 2,
Poly_Overlay = 4,
};
struct Fill_Params
{
argb color;
Poly_Flags flags;
};
struct Common_Line_Params_Index
{
i1 v;
};
inline b32 operator==(Common_Line_Params_Index a, Common_Line_Params_Index b){ return a.v==b.v; }
