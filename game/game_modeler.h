#pragma once

//-NOTE: Editing
enum Modeler_Edit_Type {
 ME_None,
 ME_Edit_Group,
 ME_Vert_Move,
 ME_Count,
};

struct Modeler_Edit;
typedef arrayof<Modeler_Edit> Edit_Group;

struct Vert_Move {
 arrayof<Vertex_Index> verts;
 v3 delta;
};

struct Modeler_Edit {
 Modeler_Edit_Type type;
 union {
  Vert_Move  Vert_Move;
  Edit_Group Edit_Group;
 };
};

struct Modeler_History {
 Arena arena;
 b32 inited;
 Base_Allocator allocator;
 arrayof<Modeler_Edit> edit_stack;
 i1 redo_index;
};

//-

introspect(info)
struct Vertex_Data{
 String name;
 Bone_ID bone_id;
 i1 symx;
 v3 pos;
 i1 basis_index;
 
 meta_unserialized i1 linum;
};

#if 0
/*enum Bezier_Type{
 Bezier_Type_v3v2       = 0,
 Bezier_Type_Parabola   = 1,
 Bezier_Type_Offsets    = 2,
 Bezier_Type_Planar_Vec = 3,
 Bezier_Type_C2         = 4,
};

struct Bezier_v3v2        { v3 d0; v2 d3; };
struct Bezier_Parabola    { v3 d; };
struct Bezier_Offsets     { v3 d0; v3 d3; };
struct Bezier_Planar_Vec  { v2 d0; v2 d3; v3 unit_y; };
struct Bezier_C2          { Curve_Index ref; };

union Bezier_Union{
 tagged_by(Bezier_Type);
 m_variant(v3v2)       Bezier_v3v2       v3v2;
 m_variant(Parabola)   Bezier_Parabola   Parabola;
 m_variant(Offsets)    Bezier_Offsets    Offsets;
 m_variant(Planar_Vec) Bezier_Planar_Vec Planar_Vec;
 m_variant(C2)         Bezier_C2         C2;
};*/
#endif
#include "generated/framework_driver_shared.gen.h"
#include "generated/bezier_types.gen.h"

introspect(info)
struct Bezier_Data{
 String name;
 Bone_ID bone_id;
 i1 symx;
 Bezier_Type type;
 Vertex_Index p0_index;
 Vertex_Index p3_index;
 tagged_by(type) Bezier_Union data;
 Line_Params params;
 meta_unserialized i1 linum;
};
typedef Bezier_Data Curve_Data;

struct Modeler  // see @init_modeler
{
 //-NOTE: Data
 Arena      *permanent_arena;
 arrayof<Vertex_Data> vertices;
 arrayof<Bezier_Data> curves;
 arrayof<Bone>        bones;
 
 //-NOTE: Editor
 u32 selected_prim_ro;  // todo: There could be multiple selected obj?
 b32 change_uncommitted;
 b32 selection_spanning;
 arrayof<u32> active_prims;
 Modeler_History history;
};

//-NOTE: Vertex

inline u32 selected_prim_id(Modeler *m) { return m->selected_prim_ro; }

inline Prim_Type
get_selected_type(Modeler *m) {
 return prim_type_from_id(selected_prim_id(m));
}
inline Vertex_Index
vertex_prim_index_from_id(u32 id){
 Vertex_Index result = {};
 if(prim_type_from_id(id) == Prim_Vertex){
  result = { prim_index_from_id(id) };
 }
 return result;
}

inline Curve_Index
curve_prim_index_from_id(u32 id){
 Curve_Index result = {};
 if(prim_type_from_id(id) == Prim_Curve){
  result = { prim_index_from_id(id) };
 }
 return result;
}

inline Vertex_Data *
get_vertex_from_id(Modeler *m, u32 id){
 Vertex_Index index = vertex_prim_index_from_id(id);
 return &m->vertices[index.v];
}
inline Curve_Data *
get_curve_from_id(Modeler *m, u32 id){
 Curve_Index index = curve_prim_index_from_id(id);
 return &m->curves[index.v];
}
inline Vertex_Data *
get_selected_vertex(Modeler *m){
 return get_vertex_from_id(m, selected_prim_id(m));
}
inline Bezier_Data *
get_selected_curve(Modeler *m){
 return get_curve_from_id(m, selected_prim_id(m));
}

xfunction b32 is_prim_id_active(Modeler *m, u32 prim_id);

//~