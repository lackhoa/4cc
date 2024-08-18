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
 Base_Allocator *allocator;
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
 u32 selected_obj_id;  // todo: there could be multiple selected obj?
 b32 change_uncommitted;
 b32 selection_spanning;
 arrayof<u32> active_objects;
 Modeler_Edit_History history;
};

//-NOTE: Vertex
framework_storage Modeler *global_modeler;

function i32
vertex_index_from_pointer(Vertex_Data *pointer)
{
 i32 result = i32(pointer - global_modeler->vertices.items);
 return result;
}

void send_vert_func(String name, v3 pos);
b32 send_bez_func(String name, String p0_name, v3 d0, v2 d3, String p3_name);


//~