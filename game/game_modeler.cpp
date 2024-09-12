//-File begin

global Modeler *global_modeler;

function u32
selected_prim_id()
{
 return global_modeler->selected_prim_id;
}

b32
is_prim_id_active(u32 prim_id)
{
 b32 result = false;
 auto &active_ids = global_modeler->active_prims;
 for_i1(index, 0, active_ids.count) {
  if (prim_id == active_ids[index]) {
   result = true;
   break;
  }
 }
 return result;
}

inline i32
vertex_index_from_pointer(Vertex_Data *pointer)
{
 i32 result = i32(pointer - global_modeler->vertices.items);
 return result;
}

function Vertex_Data *
get_vertex_by_name(String name)
{
 Vertex_Data *result = 0;
 auto &modeler = *global_modeler;
 for_i32(vert_index, 1, modeler.vertices.count)
 {
  Vertex_Data *vertex = &modeler.vertices[vert_index];
  if( vertex->name == name )
  {
   result = vertex;
  }
 }
 return result;
}

void send_vert_func(String name, v3 pos)
{
 b32 is_new = false;
 auto &modeler = *global_modeler;
 Vertex_Data *result = get_vertex_by_name(name);
 
 if (result == 0)
 {
  is_new = true;
  result = &modeler.vertices.push2();
  *result = {
   .name = push_string(modeler.permanent_arena, name)
  };
 }
 
 result->pos = pos;
 if ( is_left() ) {
  // NOTE: weird and arbitrary logic
  result->bone_index = current_bone_index();
 } else {
  result->symx = true;
 }
}
//
#define send_vert(NAME)  send_vert_func(strlit(#NAME), NAME)

Bezier bez(v3 p0, v3 d0, v2 d3, v3 p3);

b32 send_bez_func(String name, String p0_name, v3 d0, v2 d3, String p3_name)
{
 b32 ok = false;
 Bezier_Data *curve = 0;
 auto &modeler = *global_modeler;
 
 for_i32(curve_index, 1, modeler.curves.count) {
  Bezier_Data *it = &modeler.curves[curve_index];
  if ( string_match(it->name, name) ) {
   curve = it;
  }
 }
 
 if (curve == 0) {
  curve = &modeler.curves.push2();
  *curve = {};
  curve->name = push_string(modeler.permanent_arena, name);
 }
 
 Vertex_Data *vert0 = get_vertex_by_name(p0_name);
 Vertex_Data *vert3 = get_vertex_by_name(p3_name);
 if (vert0 && vert3) {
  ok = true;
  Bez data = bez(vert0->pos, d0, d3, vert3->pos);
  curve->p0_index = vertex_index_from_pointer(vert0);
  curve->p1       = data[1];
  curve->p2       = data[2];
  curve->p3_index = vertex_index_from_pointer(vert3);
 } else {
  DEBUG_TEXT("bezier error: cannot find vertex");
 }
 
 if ( is_left() ) {
  curve->bone_index = current_bone_index();
 } else {
  curve->symx = true;
 }
 
 return ok;
}
//
#define send_bez(name, p0_name, d0, d3, p3_name) \
send_bez_func(strlit(#name), strlit(#p0_name), d0, d3, strlit(#p3_name))

//-NOTE: Edit history

function void
clear_edit_history(Modeler_Edit_History &h)
{
 // NOTE(kv): We wanna clear all history when we want to.
 // NOTE(kv): so when we clear, the plan is to just wipe out the memory, and re-initialize everything.
 // NOTE(kv): Later we can make multiple arena pools, and free those.
 if (h.inited)
 {
  arena_clear(&h.arena);
  init_dynamic(h.stack, &h.allocator, 128);
 }
}

function void
apply_edit_no_history(Modeler &m, Modeler_Edit &edit0, b32 redo)
{
 switch(edit0.type)
 {
  case ME_Vert_Move:
  {
   auto &edit = edit0.Vert_Move;
   v3 delta = redo ? edit.delta : -edit.delta;
   for_i1(index,0,edit.verts.count){
    i1 vi = edit.verts[index].v;
    m.vertices[vi].pos += delta;
   }
  }break;
  
  case ME_Edit_Group:
  {
   auto &edit = edit0.Edit_Group;
   for_i1(index,0,edit.count){
    apply_edit_no_history(m, edit[index], redo);
   }
  }break;
  
  invalid_default_case;
 }
}

inline i1
get_undo_index(Modeler_Edit_History &h)
{
 return h.redo_index-1;
}

inline b32
can_undo(Modeler_Edit_History &h)
{
 b32 ok = (get_undo_index(h) >= 0);
 return ok;
}
//
function b32
modeler_undo(Modeler &m)
{
 auto &h = m.history;
 i1 undo_index = h.redo_index - 1;
 b32 ok = (undo_index >= 0);
 if (ok) {
  apply_edit_no_history(m, h.stack[undo_index], false);
  h.redo_index -= 1;
 }
 return ok;
}

inline b32
can_redo(Modeler_Edit_History &h)
{
 return (h.redo_index < h.stack.count);
}
//
function void
modeler_redo(Modeler &m)
{
 auto &h = m.history;
 apply_edit_no_history(m, h.stack[h.redo_index], true);
 h.redo_index += 1;
}

function void
apply_new_edit(Modeler &m, Modeler_Edit &edit)
{
 auto &h = m.history;
 h.stack.count = h.redo_index;
 h.stack.set_count(h.redo_index+1);  // NOTE(kv): overwrite everything after the redo
 h.stack[h.redo_index] = edit;  // NOTE(kv): push the edit on top of the stack
 modeler_redo(m);
 m.change_uncommitted = true;
}

inline Modeler_Edit *
get_current_edit(Modeler_Edit_History &h)
{// NOTE: Sometimes the "current edit" doesn't exist yet
 Modeler_Edit *result = 0;
 if (can_undo(h)) {
  result = &h.stack[get_undo_index(h)];
 }
 return result;
}

function b32
edits_can_be_merged(Modeler_Edit &edit1, Modeler_Edit &edit2)
{
 b32 result = false;
 if (edit1.type == edit2.type)
 {
  switch (edit1.type)
  {
   case ME_Vert_Move:
   {
    Vert_Move &e1 = edit1.Vert_Move;
    Vert_Move &e2 = edit2.Vert_Move;
    if (e1.verts.count == e2.verts.count)
    {
     result = true;
     for_i1(vi,0,e1.verts.count)
     {
      if (e1.verts[vi].v != e2.verts[vi].v)
      {
       result = false;
       break;
      }
     }
    }
   } break;
  }
 }
 return result;
}

inline Bezier_Data
get_selected_curve(Modeler &m)
{
 u32 id = selected_prim_id(m);
 Curve_Index index = curve_index_from_prim_id(id);
 Bezier_Data result = m.curves[index.v];
 return result;
}

//~ EOF
