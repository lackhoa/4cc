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
struct Modeler  // see @init_modeler
{//-Data
 Arena data_arena;
 //NOTE(kv) Do we need really need these arrays?
 //  For transition it does have some utility: referring to the last vertex with this name.
 arrayof<Bone> bones;
 arrayof<Common_Line_Params> line_cparams;
 
 arrayof<Vertex_Data> vertices;
 arrayof<Curve_Data> curves;
 arrayof<Fill_Data>   fills;
 
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
 return type_from_prim_id(selected_prim_id(m));
}
inline Vertex_Index
vertex_index_from_prim_id(u32 id){
 Vertex_Index result = {};
 if(type_from_prim_id(id) == Prim_Vertex){
  result = { index_from_prim_id(id) };
 }
 return result;
}
inline Curve_Index
curve_index_from_prim_id(u32 id){
 Curve_Index result = {};
 if(type_from_prim_id(id) == Prim_Curve){
  result = {index_from_prim_id(id)};
 }
 return result;
}
inline Vertex_Data *
get_vertex_from_id(Modeler *m, u32 id){
 Vertex_Index index = vertex_index_from_prim_id(id);
 return &m->vertices[index.v];
}
inline Curve_Data *
get_curve_from_id(Modeler *m, u32 id){
 Curve_Index index = curve_index_from_prim_id(id);
 return &m->curves[index.v];
}
inline Vertex_Data *
get_selected_vertex(Modeler *m){
 return get_vertex_from_id(m, selected_prim_id(m));
}
inline Curve_Data *
get_selected_curve(Modeler *m){
 return get_curve_from_id(m, selected_prim_id(m));
}

xfunction b32 is_prim_id_active(Modeler *m, u32 prim_id);
struct Vertex_Ref{
 Vertex_Index index;
 Vertex_Data *vertex;
};
struct Curve_Ref{
 Curve_Index index;
 Curve_Data *curve;
};
struct Fill_Ref{
 Fill_Index index;
 Fill_Data *fill;
};
struct Prim_Ref{
 Prim_Type type;
 union{
  Vertex_Index vertex_index;
  Curve_Index  curve_index;
  Fill_Index   fill_index;
 };
 union{
  Vertex_Data *vertex;
  Curve_Data *curve;
  Fill_Data   *fill;
 };
};
//-
function void
send_vert_func(Painter &p, String name, v3 pos, i1 linum=__builtin_LINE());
function void
dfill3_func(String vert_names[3], Fill_Params params=fp(), i1 linum=__builtin_LINE());
function void
dfill_bez_func(String curve_name, Fill_Params params=fp(), i1 linum=__builtin_LINE());
function void
dfill_dbez_func(String b1, String b2, Fill_Params params=fp(), i1 linum=__builtin_LINE());

//~