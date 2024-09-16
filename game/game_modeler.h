#pragma once

//-NOTE: Editing
enum Modeler_Edit_Type
{
 ME_None,
 ME_Edit_Group,
 ME_Vert_Move,
 ME_Count,
};

struct Modeler_Edit;
typedef arrayof<Modeler_Edit> Edit_Group;

struct Vertex_Index   { i1 v; };
struct Curve_Index  { i1 v; };
struct Object_Index { i1 v; };

struct Vert_Move {
 arrayof<Vertex_Index> verts;
 v3 delta;
};

struct Modeler_Edit
{
 Modeler_Edit_Type type;
 union {
  Vert_Move  Vert_Move;
  Edit_Group Edit_Group;
 };
};

struct Modeler_History
{
 Arena arena;
 b32 inited;
 Base_Allocator allocator;
 arrayof<Modeler_Edit> stack;
 i1 redo_index;
};

//-

introspect(info)
struct Vertex_Data {
 String name;
 i1 bone_index;
 i1 symx;
 v3 pos;
 i1 basis_index;
};

introspect(info)
enum Bezier_Type {
 Bezier_Type_v3v2       = 0,
 Bezier_Type_Offsets    = 2,
 Bezier_Type_Planar_Vec = 3,
};

introspect(info) struct Bezier_Data_v3v2        { v3 d0; v2 d3; };
introspect(info) struct Bezier_Data_Offsets     { v3 d0; v3 d3; };
introspect(info) struct Bezier_Data_Planar_Vec  { v2 d0; v2 d3; v3 unit_y; };

introspect(info)
union Bezier_Union {
 tagged_by(Bezier_Type);
 m_variant(Bezier_Type_v3v2)       Bezier_Data_v3v2       v3v2;
 m_variant(Bezier_Type_Offsets)    Bezier_Data_Offsets    offsets;
 m_variant(Bezier_Type_Planar_Vec) Bezier_Data_Planar_Vec planar_vec;
};

introspect(info)
struct Bezier_Data {
 String name;
 i1 bone_index;
 i1 symx;
 Bezier_Type type;
 i1 p0_index;
 tagged_by(type) Bezier_Union data;
 i1 p3_index;
 v4 radii;
};

struct Modeler  // see @init_modeler
{
 //-NOTE: Data
 Arena      *permanent_arena;
 arrayof<Vertex_Data> vertices;
 arrayof<Bezier_Data> curves;
 
 //-NOTE: Editor
 u32 selected_prim_id;  // todo: There could be multiple selected obj?
 b32 change_uncommitted;
 b32 selection_spanning;
 arrayof<u32> active_prims;
 Modeler_History history;
};

//-NOTE: Vertex

void send_vert_func(String name, v3 pos);
b32 send_bez_v3v2_func(String name, String p0_name, v3 d0, v2 d3, String p3_name);

inline u32
selected_prim_id(Modeler &m) {
 return m.selected_prim_id;
}

inline Prim_Type
get_selected_type(Modeler &m) {
 return prim_id_type(selected_prim_id(m));
}

inline Vertex_Index
vertex_index_from_prim_id(u32 id){
 Vertex_Index result = {};
 if ( prim_id_type(id) == Prim_Vertex ) {
  result = { prim_id_to_index(id) };
 }
 return result;
}

inline Curve_Index
curve_index_from_prim_id(u32 id){
 Curve_Index result = {};
 if ( prim_id_type(id) == Prim_Curve ) {
  result = { prim_id_to_index(id) };
 }
 return result;
}

inline Bezier_Data
get_selected_curve(Modeler &m) {
 u32 id = selected_prim_id(m);
 Curve_Index index = curve_index_from_prim_id(id);
 Bezier_Data result = m.curves[index.v];
 return result;
}

xfunction b32 is_prim_id_active(u32 prim_id);
function u32 selected_prim_id(void);

//~