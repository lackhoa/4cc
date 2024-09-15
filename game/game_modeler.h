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

struct Vert_Index   { i1 v; };
struct Curve_Index  { i1 v; };
struct Object_Index { i1 v; };

struct Vert_Move {
 arrayof<Vert_Index> verts;
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

struct Modeler_Edit_History
{
 Arena arena;
 b32 inited;
 Base_Allocator allocator;
 arrayof<Modeler_Edit> stack;
 i1 redo_index;
};

//-

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
 Modeler_Edit_History history;
};

//-NOTE: Vertex

void send_vert_func(String name, v3 pos);
b32 send_bez_v3v2_func(String name, String p0_name, v3 d0, v2 d3, String p3_name);

force_inline u32
selected_prim_id(Modeler &m) {
 return m.selected_prim_id;
}

inline b32
is_selecting_type(Modeler &m, Prim_Type type) {
 Prim_Type sel_type = prim_id_type(selected_prim_id(m));
 return sel_type == type;
}

inline Vert_Index
vertex_index_from_prim_id(u32 id)
{
 Vert_Index result = {};
 if ( prim_id_type(id) == Prim_Vertex ) {
  result = { prim_id_to_index(id) };
 }
 return result;
}
inline Curve_Index
curve_index_from_prim_id(u32 id)
{
 Curve_Index result = {};
 if ( prim_id_type(id) == Prim_Curve ) {
  result = { prim_id_to_index(id) };
 }
 return result;
}

xfunction b32 is_prim_id_active(u32 prim_id);
function u32 selected_prim_id(void);


//~