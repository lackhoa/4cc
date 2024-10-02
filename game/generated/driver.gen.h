//NOTE Generated at C:\Users\vodan\4ed\code/meta_klang.cpp:26:
#pragma once
// NOTE: source: C:\Users\vodan\4ed\code\game\driver.kh
typedef u32 argb;
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
//  C:\Users\vodan\4ed\code/meta_print.cpp:354:
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
//  C:\Users\vodan\4ed\code/meta_print.cpp:354:
enum 
{
Line_Overlay = 1,
Line_Straight = 2,
Line_No_SymX = 4,
};
struct Line_Params
{
v4 radii;
v1 nslice_per_meter;
v1 visibility;
v1 alignment_threshold;
argb color;
Line_Flags flags;
};
