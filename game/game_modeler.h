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

introspect(info)
enum Bezier_Type{
 Bezier_Type_v3v2       = 0,
 Bezier_Type_Parabola   = 1,
 Bezier_Type_Offsets    = 2,
 Bezier_Type_Planar_Vec = 3,
};

introspect(info) struct Bezier_v3v2        { v3 d0; v2 d3; };
introspect(info) struct Bezier_Parabola    { v3 d; };
introspect(info) struct Bezier_Offsets     { v3 d0; v3 d3; };
introspect(info) struct Bezier_Planar_Vec  { v2 d0; v2 d3; v3 unit_y; };

introspect(info)
union Bezier_Union{
 tagged_by(Bezier_Type);
 m_variant(Bezier_Type_v3v2)       Bezier_v3v2       v3v2;
 m_variant(Bezier_Type_Parabola)   Bezier_Parabola   parabola;
 m_variant(Bezier_Type_Offsets)    Bezier_Offsets    offsets;
 m_variant(Bezier_Type_Planar_Vec) Bezier_Planar_Vec planar_vec;
};

introspect(info)
struct Bezier_Data{
 String name;
 Bone_ID bone_id;
 i1 symx;
 Bezier_Type type;
 Vertex_Index p0_index;
 Vertex_Index p3_index;
 tagged_by(type) Bezier_Union data;
 v4 radii;
 
 meta_unserialized i1 linum;
};

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

b32 send_bez_v3v2_func(String name, String p0_name, v3 d0, v2 d3, String p3_name);

inline u32 selected_prim_id(Modeler *m) { return m->selected_prim_ro; }

inline Prim_Type
get_selected_type(Modeler *m) {
 return prim_id_type(selected_prim_id(m));
}

inline i1
index_from_prim_id_(u32 id){
 if(prim_id_is_data(id)){ return (id & 0xFFFF); }
 return 0;
}

inline Vertex_Index
vertex_index_from_prim_id(u32 id){
 Vertex_Index result = {};
 if ( prim_id_type(id) == Prim_Vertex ) {
  result = { index_from_prim_id_(id) };
 }
 return result;
}

inline Curve_Index
curve_index_from_prim_id(u32 id){
 Curve_Index result = {};
 if ( prim_id_type(id) == Prim_Curve ) {
  result = { index_from_prim_id_(id) };
 }
 return result;
}

inline Bezier_Data
get_selected_curve(Modeler *m) {
 u32 id = selected_prim_id(m);
 Curve_Index index = curve_index_from_prim_id(id);
 Bezier_Data result = m->curves[index.v];
 return result;
}

xfunction b32 is_prim_id_active(Modeler *m, u32 prim_id);

//~