#pragma once

//-NOTE: Editing
enum Modeler_Edit_Type{
 ME_None,
 ME_Edit_Group,
 ME_Vert_Move,
 ME_Count,
};
struct Modeler_Edit;
typedef arrayof<Modeler_Edit> Edit_Group;

struct Vert_Move{
 arrayof<Vertex_Index> verts;
 v3 delta;
};
struct Modeler_Edit{
 Modeler_Edit_Type type;
 union {
  Vert_Move  Vert_Move;
  Edit_Group Edit_Group;
 };
};
struct Modeler_History{
 Arena arena;
 b32 inited;
 Base_Allocator allocator;
 arrayof<Modeler_Edit> edit_stack;
 i1 redo_index;
};
//-
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

inline u32 selected_prim_id(Modeler *m){ return m->selected_prim_ro; }

inline Prim_Type
get_selected_type(Modeler *m){
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