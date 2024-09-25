//-File begin

//global Modeler *global_modeler;
global b32 sending_draw_data = true;

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

inline Vertex_Index
vertex_index_from_pointer(Modeler *m, Vertex_Data *pointer) {
 return {i32(pointer - m->vertices.items)};
}
inline Curve_Index
curve_index_from_pointer(Modeler *m, Bezier_Data *pointer) {
 return {i32(pointer - m->curves.items)};
}

function Vertex_Data *
get_vertex_by_name(Modeler *m, String name, i1 max_linum){
 Vertex_Data *result = 0;
 i1 max_linum_seen = 0;
 for_i32(vert_index,1,m->vertices.count){
  Vertex_Data *vertex = &m->vertices[vert_index];
  if(vertex->linum >= max_linum_seen &&
     vertex->linum < max_linum &&
     vertex->name == name){
   result = vertex;
   max_linum_seen = vertex->linum;
  }
 }
 kv_assert(result);
 return result;
}
function Vertex_Data *
get_vertex_by_linum(Modeler *m, i1 linum){
 for_i32(vert_index, 1, m->vertices.count){
  Vertex_Data *vertex = &m->vertices[vert_index];
  if(vertex->linum == linum){
   return vertex;
  }
 }
 return 0;
}

xfunction void
send_vert_func(String name, v3 pos, i1 linum)
{
 if(sending_draw_data){
  Painter *p = &painter;
  auto m = p->modeler;
  Vertex_Data *result = 0;
  result = get_vertex_by_linum(m, linum);
  if(result == 0){
   result = &m->vertices.push2();
   *result = {
    .name = push_string(m->permanent_arena, name)
   };
  }
  
  if(is_left()){
   //NOTE(kv) Only record vertex on the left
   result->pos     = pos;
   result->bone_id = current_bone(p)->id;
   result->linum   = linum;
  }else{
   result->symx = true;
  }
 }
}

xfunction Bezier bez_v3v2(v3 p0, v3 d0, v2 d3, v3 p3);

function Bezier_Data *
get_curve_by_linum(Modeler *m, i1 linum){
 for_i32(i,1,m->curves.count){
  Bezier_Data *it = &m->curves[i];
  if(it->linum == linum){
   return it;
  }
 }
 return 0;
}

xfunction void
send_bez_func(String name,
              String p0_name, String p3_name,
              Bezier_Type type, Bezier_Union data,
              i1 linum)
{
 if(sending_draw_data){
  Painter *p = &painter;
  Modeler *m = p->modeler;
  Bezier_Data *curve = get_curve_by_linum(m, linum);
  if(curve == 0){
   curve = &m->curves.push2();
   *curve = {};
  }
  //NOTE(kv) Only get vertex above this line number
  Vertex_Data *vert0 = get_vertex_by_name(m, p0_name, linum);
  Vertex_Data *vert3 = get_vertex_by_name(m, p3_name, linum);
  kv_assert(vert0 && vert3);
  
  curve->type     = type;
  curve->name     = name;
  curve->p0_index = vertex_index_from_pointer(m, vert0);
  curve->p3_index = vertex_index_from_pointer(m, vert3);
  curve->data     = data;
  curve->linum    = linum;
  
  if(is_left()){
   curve->bone_id = current_bone(p)->id;
  }else{
   curve->symx = true;
  }
 }
}
xfunction void
send_bez_v3v2_func(String name, String p0, String p3, v3 d0, v2 d3, i1 linum) {
 Bezier_Union data = {.v3v2={ .d0=d0, .d3=d3 }};
 send_bez_func(name, p0, p3, Bezier_Type_v3v2, data, linum);
}
xfunction void
send_bez_parabola_func(String name, String p0, String p3, v3 d, i1 linum) {
 Bezier_Union data = {.parabola={ .d=d }};
 send_bez_func(name, p0, p3, Bezier_Type_Parabola, data, linum);
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
 switch(edit0.type) {
  case ME_Vert_Move: {
   auto &edit = edit0.Vert_Move;
   v3 delta = redo ? edit.delta : -edit.delta;
   for_i1(index,0,edit.verts.count){
    i1 vi = edit.verts[index].v;
    m->vertices[vi].pos += delta;
   }
  }break;
  
  case ME_Edit_Group: {
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
 if (edit1.type == edit2.type) {
  switch (edit1.type) {
   case ME_Vert_Move: {
    Vert_Move &e1 = edit1.Vert_Move;
    Vert_Move &e2 = edit2.Vert_Move;
    if (e1.verts.count == e2.verts.count) {
     result = true;
     for_i1(vi,0,e1.verts.count) {
      if (e1.verts[vi].v != e2.verts[vi].v) {
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
 //NOTE(kv) If you wanna unselect vertices, this is your chance!
}
function void
modeler_exit_edit(Modeler *m) {
 if(m->change_uncommitted) {
  modeler__reset_edit(m);
  m->change_uncommitted = false;
 }
}
function void
modeler_exit_edit_undo(Modeler *m) {
 if(m->change_uncommitted){
  modeler__reset_edit(m);
  modeler_undo(m);
  m->change_uncommitted = false;
 }
}

inline b32
selecting_vertex(Modeler *m){
 return prim_id_type(selected_prim_id(m)) == Prim_Vertex;
}

function void
clear_modeling_data(Modeler *m){
 m->vertices.set_count(1);
 m->curves.  set_count(1);
 m->bones.   set_count(1);
 clear_edit_history(&m->history);
}

function void
compute_active_prims(Modeler *m)
{
 u32 sel_prim = m->selected_prim_ro;
 m->active_prims.count = 0;
 // NOTE(kv): selected object is always active.
 push_unique(m->active_prims, sel_prim);
 if (m->selection_spanning) {
  Prim_Type sel_type = prim_id_type(sel_prim);
  if (sel_type == Prim_Vertex){
   Vertex_Index sel_index = vertex_index_from_prim_id(sel_prim);
   Vertex_Data &sel = m->vertices[sel_index.v];
   for_i32(cindex,1,m->curves.count) {
    Bezier_Data &curve = m->curves[cindex];
    if (sel_index == curve.p0_index) {
     push_unique(m->active_prims, prim_id_from_vertex_index(curve.p3_index));
    } else if (sel_index == curve.p3_index) {
     push_unique(m->active_prims, prim_id_from_vertex_index(curve.p0_index));
    }
   }
  }
 }
}
inline void
select_primitive(Modeler *m, u32 prim){
 m->selected_prim_ro = prim;
 compute_active_prims(m);
}
inline void
clear_selection(Modeler *m) {
 m->selected_prim_ro = 0;
 m->active_prims.count = 0;
}


//~ EOF
