//-
xfunction b32
is_prim_id_active(Modeler *m, u32 prim_id){
 b32 result = false;
 auto &active_ids = m->active_prims;
 for_i1(index, 0, active_ids.count){
  if(prim_id == active_ids[index]){
   result = true;
   break;
  }
 }
 return result;
}
inline Vertex_Index
vertex_index_from_pointer(Modeler &m, Vertex_Data *pointer){
 if(pointer){
  return {i32(pointer - m.vertices.items)};
 }
 return {};
}
inline Curve_Index
curve_index_from_pointer(Modeler &m, Curve_Data *pointer) {
 if(pointer){
  return {i32(pointer - m.curves.items)};
 }
 return {};
}
function Vertex_Ref
get_vertex_by_name(Modeler &m, String name){
 Vertex_Ref result = {};
 //NOTE(kv) We can rely on the fact that vertices are added in "code order" for now.
 //  so the latest always wins.
 for(auto vi=m.vertices.count-1;
     vi >= 1;
     vi--){
  Vertex_Data *vertex = &m.vertices[vi];
  if(vertex->name == name){
   result.index = {vi};
   result.vertex = vertex;
   break;
  }
 }
 return result;
}
function Vertex_Ref
get_vertex_by_linum(Modeler &m, i1 linum){
 Vertex_Ref result = {};
 for_i32(vi, 1, m.vertices.count){
  Vertex_Data *vertex = &m.vertices[vi];
  if(vertex->linum == linum){
   result.index = {vi};
   result.vertex = vertex;
   break;
  }
 }
 return result;
}
function Curve_Ref
get_curve_by_linum(Modeler &m, i1 linum){
 Curve_Ref result = {};
 for_i32(i,1,m.curves.count){
  Curve_Data *it = &m.curves[i];
  if(it->linum == linum){
   result.index = {i};
   result.curve = it;
   break;
  }
 }
 return result;
}
function Curve_Ref
get_curve_by_name(Modeler &m, String name){
 Curve_Ref result = {};
 for(i1 ci=m.curves.count-1;
     ci >= 1;
     ci--){
  Curve_Data *curve = &m.curves[ci];
  if(curve->name == name){
   result.index = {ci};
   result.curve = curve;
   break;
  }
 }
 return result;
}
function Fill_Ref
get_fill_by_linum(Modeler &m, i1 linum){
 Fill_Ref result = {};
 for(i1 i=m.fills.count-1;
     i >= 1;
     i--){
  Fill_Data &tri = m.fills[i];
  if(tri.linum == linum){
   result.index = {i};
   result.fill  = &tri;
   break;
  }
 }
 return result;
}
//-
function void
send_vert_func(Painter &p, String name, v3 pos, i1 linum){
 if(p.sending_data){
  Modeler &m = *p.modeler;
  if(is_left(p)){
   Vertex_Data &vertex = m.vertices.push_zero();
   vertex.name    = name;
   vertex.pos     = pos;
   vertex.bone_id = current_bone(p)->id;
   vertex.linum   = linum;
  }
 }
}
//-
function void
send_bez_func(String name, Curve_Type type, Curve_Union data,
              Line_Params params, i1 linum){
 Painter &p = painter;
 if(p.sending_data){
  Modeler &m = *p.modeler;
  if(is_left(p)){
   Curve_Data &curve = m.curves.push_zero();
   curve.cparams  = current_line_cparams_index();
   kv_assert(curve.cparams.v >= 0 and
             curve.cparams.v <  m.line_cparams.count);
   curve.type     = type;
   curve.name     = name;
   curve.data     = data;
   curve.params   = params;
   curve.linum    = linum;
   curve.bone_id  = current_bone(p)->id;
  }else{
   if(p.is_right and p.symx){
    Curve_Data *curve = get_curve_by_linum(m, linum).curve;
    curve->symx = true;
   }
  }
 }
}
//-NOTE(kv) Prototypes at send_bez.gen.h
/*
#if 0
function void
send_bez_v3v2(String name, String p0, v3 d0, v2 d3, String p3, Line_Params params, i1 linum){
 Curve_Union data = {.v3v2={ .d0=d0, .d3=d3 }};
 send_bez_func(name, p0, p3, Curve_Type_v3v2, data, params, linum);
}
function void
send_bez_parabola(String name, String p0, v3 d, String p3, Line_Params params, i1 linum){
 Curve_Union data = {.parabola={.d=d}};
 send_bez_func(name, p0, p3, Curve_Type_Parabola, data, params, linum);
}
function void
send_bez_c2(String name, String ref, v3 d3, String p3, Line_Params params, i1 linum){
 Modeler &m = *painter.modeler;
 Curve_Union data = {.c2={.d3=d3}};
 Curve_Ref ref2 = get_curve_by_name(m, ref);
 kv_assert(ref2.curve);
 data.c2.ref = ref2.index;
 send_bez_func(name, strlit(""), p3, Curve_Type_C2, data, params, linum);
}
function void
send_bez_unit(String name, String p0, v2 d0, v2 d3, v3 unit_y, String p3, Line_Params params, i32 linum){
 Curve_Union data = {.unit={d0,d3,unit_y}};
 send_bez_func(name, p0, p3, Curve_Type_Unit, data, params, linum);
}
function void
send_bez_unit2(String name, String p0, v4 d0d3, v3 unit_y, String p3, Line_Params params, i32 linum){
 send_bez_unit(name, p0, d0d3.xy, d0d3.zw, unit_y, p3, params, linum);
}
function void
send_bez_line(String name, String p0, String p3, Line_Params params, i32 linum){
 Curve_Union data = {};
 send_bez_func(name, p0, p3, Curve_Type_Line, data, params, linum);
}
function void
send_bez_bez_bezd_old(String name, String p0, v3 d0, v2 d3, String p3, Line_Params params, i32 linum){
 Curve_Union data = {.bez_bezd_old={.d0=d0, .d3=d3,}};
 send_bez_func(name, p0, p3, Curve_Type_Bezd_Old, data, params, linum);
}
function void
send_bez_offset(String name, String p0, v3 d0, v3 d3, String p3, Line_Params params, i32 linum){
 Curve_Union data = {.offset={.d0=d0, .d3=d3,}};
 send_bez_func(name, p0, p3, Curve_Type_Offset, data, params, linum);
}
function void
send_bez_negateX(String name, String ref, Line_Params params, i32 linum){
 Modeler &m = *painter.modeler;
 Curve_Union data;
 Curve_Ref ref2 = get_curve_by_name(m, ref);
 kv_assert(ref2.curve);
 data.negateX.ref = ref2.index;
 send_bez_func(name, empty_string, empty_string, Curve_Type_NegateX, data, params, linum);
}
#endif
*/
//-NOTE(kv) Fill situation
function void
send_fill_func(Fill_Type type, Fill_Union &data, Fill_Params params, i1 linum){
 Painter &p = painter;
 if(p.sending_data){
  Modeler &m = *p.modeler;
  if(is_left(p)){
   Fill_Data &fill = m.fills.push_zero();
   fill.type = type;
   fill.params = params;
   fill.linum = linum;
   fill.data = data;
  }else{
   if(p.symx){
    get_fill_by_linum(m, linum).fill->symx = true;
   }
  }
 }
}
function void
dfill3_func(String vert_names[3], Fill_Params params, i1 linum){
 Painter &p = painter;
 Modeler &m = *p.modeler;
 Fill_Union data = {};
 for_i32(i,0,3){
  Vertex_Index vi = get_vertex_by_name(m,vert_names[i]).index;
  kv_assert(vi.v);
  data.fill3.verts[i] = vi;
 }
 send_fill_func(Fill_Type_Fill3, data, params, linum);
}
function void
dfill_bez_func(String curve_name, Fill_Params params, i1 linum){
 Painter &p = painter;
 Modeler &m = *p.modeler;
 Fill_Union data = {};
 {
  data.bez.curve = get_curve_by_name(m, curve_name).index;
  kv_assert(data.bez.curve.v);
 }
 send_fill_func(Fill_Type_Bez, data, params, linum);
}
function void
dfill_dbez_func(String b1n, String b2n, Fill_Params params, i1 linum){
 Painter &p = painter;
 Modeler &m = *p.modeler;
 Fill_Union data0 = {};
 Fill_DBez &data = data0.dbez;
 {
  data.curve1 = get_curve_by_name(m, b1n).index;
  data.curve2 = get_curve_by_name(m, b2n).index;
  kv_assert(data.curve1.v);
  kv_assert(data.curve2.v);
 }
 send_fill_func(Fill_Type_DBez, data0, params, linum);
}
//-NOTE: Edit history
function void
clear_edit_history(Modeler_History *h){
 // NOTE(kv): We wanna clear all history when we want to.
 // NOTE(kv): so when we clear, the plan is to just wipe out the memory, and re-initialize everything.
 // NOTE(kv): Later we can make multiple arena pools, and free those.
 if(h->inited){
  arena_clear(&h->arena);
  init_dynamic(h->edit_stack, &h->allocator, 128);
 }
}
function void
apply_edit_no_history(Modeler *m, Modeler_Edit &edit0, b32 redo) {
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
get_undo_index(Modeler_History &h) {
 return h.redo_index-1;
}
inline b32
can_undo(Modeler_History &h) {
 b32 ok = (get_undo_index(h) >= 0);
 return ok;
}
function b32
modeler_undo(Modeler *m) {
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
modeler_exit_edit_undo(Modeler *m){
 if(m->change_uncommitted){
  modeler__reset_edit(m);
  modeler_undo(m);
  m->change_uncommitted = false;
 }
}
inline b32
selecting_vertex(Modeler *m){
 return type_from_prim_id(selected_prim_id(m)) == Prim_Vertex;
}
function void
clear_modeling_data(Modeler &m){
 m.vertices.set_count(1);
 m.curves.  set_count(1);
 m.fills.   set_count(1);
 m.bones.   set_count(1);
 clear_edit_history(&m.history);
}
function void
compute_active_prims(Modeler *m)
{
 u32 sel_prim = m->selected_prim_ro;
 m->active_prims.count = 0;
 // NOTE(kv): selected object is always active.
 push_unique(m->active_prims, sel_prim);
 if(m->selection_spanning){
  Prim_Type sel_type = type_from_prim_id(sel_prim);
  if (sel_type == Prim_Vertex){
   Vertex_Index sel_index = vertex_index_from_prim_id(sel_prim);
   Vertex_Data &sel = m->vertices[sel_index.v];
   for_i32(cindex,1,m->curves.count){
    Curve_Data &curve = m->curves[cindex];
    //TODO(kv) If the curve doesn't store its endpoint,
    //  do we need to trace the endpoint down?
    Vertex_Index p0_index = get_p0_index_or_zero(curve);
    Vertex_Index p3_index = get_p3_index_or_zero(curve);
    if(sel_index == p0_index){
     push_unique(m->active_prims, prim_id_from_vertex_index(p0_index));
    }else if(sel_index == p3_index){
     push_unique(m->active_prims, prim_id_from_vertex_index(p3_index));
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
clear_selection(Modeler *m){
 m->selected_prim_ro   = 0;
 m->active_prims.count = 0;
}
xfunction arrayof<Bone> &get_bones(Modeler &m){ return m.bones; }
//-
xfunction arrayof<Common_Line_Params> &
get_line_cparams_list(Modeler &m){
 return m.line_cparams;
}
//~EOF
