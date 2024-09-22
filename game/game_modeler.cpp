//-File begin

//global Modeler *global_modeler;

xfunction b32
is_prim_id_active(Modeler *m, u32 prim_id) {
 b32 result = false;
 auto &active_ids = m->active_prims;
 for_i1(index, 0, active_ids.count) {
  if (prim_id == active_ids[index]) {
   result = true;
   break;
  }
 }
 return result;
}

inline i32
vertex_index_from_pointer(Modeler *m, Vertex_Data *pointer) {
 return i32(pointer - m->vertices.items);
}

function Vertex_Data *
get_vertex_by_name(Modeler *m, String name) {
 Vertex_Data *result = 0;
 for_i32(vert_index, 1, m->vertices.count) {
  Vertex_Data *vertex = &m->vertices[vert_index];
  if( vertex->name == name ) {
   result = vertex;
  }
 }
 return result;
}

xfunction void
send_vert_func(Painter *p, String name, v3 pos) {
 auto m = p->modeler;
 Vertex_Data *result = get_vertex_by_name(m, name);
 
 if (result == 0){
  result = &m->vertices.push2();
  *result = {
   .name = push_string(m->permanent_arena, name)
  };
 }
 
 if (is_left()){
  //NOTE(kv) Only record vertex on the left
  result->pos     = pos;
  result->bone_id = current_bone(p)->id;
 }else{
  result->symx = true;
 }
}

Bezier bez(v3 p0, v3 d0, v2 d3, v3 p3);

xfunction b32
send_bez_v3v2_func(Painter *p, String name,
                   String p0_name, v3 d0,
                   v2 d3, String p3_name)
{
 Modeler *m = p->modeler;
 b32 ok = false;
 Bezier_Data *curve = 0;
 
 //NOTE(kv) Query the curve
 for_i32(curve_index, 1, m->curves.count) {
  Bezier_Data *it = &m->curves[curve_index];
  if (string_match(it->name, name)){
   curve = it;
  }
 }
 
 if (curve == 0) {
  curve = &m->curves.push2();
  *curve = {};
  curve->name = push_string(m->permanent_arena, name);
 }
 
 Vertex_Data *vert0 = get_vertex_by_name(m, p0_name);
 Vertex_Data *vert3 = get_vertex_by_name(m, p3_name);
 if (vert0 && vert3) {
  ok = true;
  curve->p0_index = vertex_index_from_pointer(m, vert0);
  curve->data.v3v2 = { .d0 = d0, .d3 = d3, };
  curve->p3_index = vertex_index_from_pointer(m, vert3);
 } else {
  DEBUG_TEXT("bezier error: cannot find vertex");
 }
 
 if (is_left()){
  curve->bone_id = current_bone(p)->id;
 }else{
  curve->symx = true;
 }
 
 return ok;
}

//-NOTE: Edit history

function void
clear_edit_history(Modeler_History *h)
{
 // NOTE(kv): We wanna clear all history when we want to.
 // NOTE(kv): so when we clear, the plan is to just wipe out the memory, and re-initialize everything.
 // NOTE(kv): Later we can make multiple arena pools, and free those.
 if (h->inited) {
  arena_clear(&h->arena);
  init_dynamic(h->edit_stack, &h->allocator, 128);
 }
}

function void
apply_edit_no_history(Modeler *m, Modeler_Edit &edit0, b32 redo)
{
 switch(edit0.type)
 {
  case ME_Vert_Move:
  {
   auto &edit = edit0.Vert_Move;
   v3 delta = redo ? edit.delta : -edit.delta;
   for_i1(index,0,edit.verts.count){
    i1 vi = edit.verts[index].v;
    m->vertices[vi].pos += delta;
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
get_undo_index(Modeler_History &h)
{
 return h.redo_index-1;
}

inline b32
can_undo(Modeler_History &h)
{
 b32 ok = (get_undo_index(h) >= 0);
 return ok;
}
//
function b32
modeler_undo(Modeler *m)
{
 auto &h = m->history;
 i1 undo_index = h.redo_index - 1;
 b32 ok = (undo_index >= 0);
 if (ok) {
  apply_edit_no_history(m, h.edit_stack[undo_index], false);
  h.redo_index -= 1;
 }
 return ok;
}

inline b32
can_redo(Modeler_History &h) {
 return (h.redo_index < h.edit_stack.count);
}
//
function void
modeler_redo(Modeler *m) {
 auto &h = m->history;
 apply_edit_no_history(m, h.edit_stack[h.redo_index], true);
 h.redo_index += 1;
}

function void
apply_new_edit(Modeler *m, Modeler_Edit &edit)
{
 auto &h = m->history;
 h.edit_stack.count = h.redo_index;
 h.edit_stack.set_count(h.redo_index+1);  // NOTE(kv): overwrite everything after the redo
 h.edit_stack[h.redo_index] = edit;  // NOTE(kv): push the edit on top of the stack
 modeler_redo(m);
 m->change_uncommitted = true;
}

inline Modeler_Edit *
get_current_edit(Modeler_History &h)
{// NOTE: Sometimes the "current edit" doesn't exist yet
 Modeler_Edit *result = 0;
 if (can_undo(h)) {
  result = &h.edit_stack[get_undo_index(h)];
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

function void
modeler__reset_edit(Modeler *m) {
 m->selected_prim_id   = 0;
 m->active_prims.count = 0;
}
function void
modeler_exit_edit(Modeler *m) {
 modeler__reset_edit(m);
 m->change_uncommitted = false;
}
function void
modeler_exit_edit_undo(Modeler *m) {
 modeler__reset_edit(m);
 modeler_undo(m);
 m->change_uncommitted = false;
}

inline b32
selecting_vertex(Modeler *m) {
 return prim_id_type(selected_prim_id(m)) == Prim_Vertex;
}

function void
modeler_clear_data(Modeler *m) {
 m->vertices.set_count(1);
 m->curves.  set_count(1);
 m->bones.   set_count(1);
 clear_edit_history(&m->history);
}

//~ EOF
