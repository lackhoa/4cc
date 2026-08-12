//-NOTE(kv) Replay path (draw-as-data step 3): re-issue draw calls from the recording
// (Model.primitives/groups) through the SAME outer draw functions the code path uses,
// so culling / hot-highlight / tessellation logic is shared by construction (Q22).
// Plan + decisions: ~/notes/tasks/autodraw_draw_as_data/plan-replay-path.md

function void
replay_recording()
{// NOTE(kv) Same-frame second consumer of the recording: bones/camera/painter are the
 // ones the capture ran with. Correctness bar: the vertex stream is bit-identical to
 // the code path's (verified by the Diff-now button).
 Model *m = the_model;
 Painter *p = painter;

 global_replaying = true;  // NOTE(kv) suppress send_primitive during replay
 Paint_Params saved_params = p->params;
 b32 saved_is_right = m->is_right;
 m->is_right = false;  // NOTE(kv) the recording is left-side only (should_send_model_data)

 // NOTE(kv) Own bone-stack slot: draw_bezier reads the stack top for its alignment
 // check, so set_bone_transform alone isn't enough.
 push(&m->bone_stack, m->bone_stack.items[0]);
 Bone *cur_bone = 0;

 for_i32(iprim, 0, m->primitives.count)
 {
  Recorded_Primitive &prim = m->primitives.items[iprim];

  if(cur_bone == 0 or not (prim.bone_id == cur_bone->id))
  {
   cur_bone = get_bone(prim.bone_id, /*is_right*/false);
   m->bone_stack.items[m->bone_stack.count-1] = cur_bone;
   set_bone_transform(cur_bone->world_from_bone);
  }

  clear_draw_location();  // NOTE(kv) set_draw_location only ever raises the hot flag
  set_draw_location(prim.location);
  p->params = m->groups.items[prim.group_index].params;

  switch(prim.type)
  {
   case Primitive_Type_Curve:
   {
    draw_bezier(prim.curve.e, prim.line_params);
   }break;

   case Primitive_Type_Poly3:
   {
    fill3(prim.poly3[0], prim.poly3[1], prim.poly3[2], prim.fill_params);
   }break;

   case Primitive_Type_Dual_Bezier:
   {// NOTE(kv) Culling reads the view-vector stack; restore the recorded vector.
    p->view_vector_stack[p->view_vector_count++] = prim.view_vector;
    fill_dual_bez(prim.dual_bezier.P.e, prim.dual_bezier.Q.e, prim.fill_params);
    p->view_vector_count--;
   }break;

   case Primitive_Type_Patch:
   {
    fill_patch(prim.patch.e, prim.fill_params);
   }break;

   case Primitive_Type_Disk:
   {
    fill_disk(prim.disk.center, {prim.disk.radius}, prim.fill_params);
   }break;

   case Primitive_Type_Image:
   {
    draw_image(prim.image.filename,
               prim.image.o, prim.image.x, prim.image.y,
               prim.image.alpha, prim.image.color);
   }break;
  }
 }

 clear_draw_location();
 m->bone_stack.count--;
 set_bone_transform(current_world_from_bone());
 m->is_right = saved_is_right;
 p->params = saved_params;
 global_replaying = false;
}

//-NOTE(kv) Vertex-stream diff (Q23/Q25)

function void
init_vertex_tee(Vertex_Tee *tee, Arena *arena)
{
 init_dynamic(tee->vertices, arena, 4096);
 init_dynamic(tee->entries, arena, 1024);
}
function Location
tee_location_of_vertex(Vertex_Tee *tee, i32 vertex_index)
{
 i32 base = 0;
 for_i32(i, 0, tee->entries.count)
 {
  base += tee->entries.items[i].vertex_count;
  if(vertex_index < base){ return tee->entries.items[i].location; }
 }
 return {};
}
function Replay_Diff_Result
diff_vertex_tees(Vertex_Tee *code, Vertex_Tee *replay)
{
 Replay_Diff_Result r = {};
 r.valid = true;
 r.code_vertex_count   = code->vertices.count;
 r.replay_vertex_count = replay->vertices.count;
 r.first_diff_vertex = -1;

 i32 n = minimum(code->vertices.count, replay->vertices.count);
 for_i32(i, 0, n)
 {// NOTE(kv) Bitwise compare is correct: identical inputs through identical code
  // must produce identical bits (vertices are zero-initialized, so padding matches).
  if(not block_match_struct(&code->vertices.items[i], &replay->vertices.items[i]))
  {
   r.first_diff_vertex = i;
   break;
  }
 }

 r.match = (r.first_diff_vertex == -1 and
            code->vertices.count == replay->vertices.count);
 if(not r.match)
 {// NOTE(kv) Map the divergence back to the owning draw's Location in each stream.
  i32 index = (r.first_diff_vertex != -1) ? r.first_diff_vertex : n;
  r.code_location   = tee_location_of_vertex(code, index);
  r.replay_location = tee_location_of_vertex(replay, index);
 }
 return r;
}
//-EOF
