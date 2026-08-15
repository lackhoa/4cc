//-NOTE(kv) Replay path (draw-as-data step 3): re-issue draw calls from the recording
// (Model.primitives/groups) through the SAME outer draw functions the code path uses,
// so culling / hot-highlight / tessellation logic is shared by construction (Q22).
// Plan + decisions: ~/notes/tasks/autodraw_draw_as_data/plan-replay-path.md

function void
store_recording(i32 preset)
{// NOTE(kv) Snapshot the per-frame capture (Model.primitives/groups) into the
 // preset's slot (Q36). Runs right after viewport 0's driver_render, so the
 // painter still holds that render's viz/grid state for the header.
 Model *m = the_model;
 kv_assert(0 <= preset and preset < Game_Preset_Count);
 Recording &rec = m->recordings.slots[preset];
 arena_clear(&rec.arena);
 init_dynamic(rec.primitives, &rec.arena, maximum(1, m->primitives.count));
 init_dynamic(rec.groups,     &rec.arena, maximum(1, m->groups.count));
 set_count(&rec.primitives, m->primitives.count);
 block_copy(rec.primitives.items, m->primitives.items,
            sizeof(Recorded_Primitive) * m->primitives.count);
 set_count(&rec.groups, m->groups.count);
 block_copy(rec.groups.items, m->groups.items,
            sizeof(Recorded_Group) * m->groups.count);
 rec.viz_level            = painter->viz_level;
 rec.ignore_radii         = painter->ignore_radii;
 rec.ignore_alignment_min = painter->ignore_alignment_min;
 rec.show_grid            = painter->show_grid;
 rec.captured = true;
}

function void
replay_recording(Recording &rec)
{// NOTE(kv) Second consumer of a captured recording: bones/camera are live, the
 // primitives/groups come from the preset's slot. Correctness bar (same-frame,
 // same-preset): the vertex stream is bit-identical to the code path's (Diff-now).
 Model *m = the_model;
 Painter *p = painter;

 // NOTE(kv) Painter-level capture state (Q36): these alter tessellation/culling
 // inside the re-issued draws but are NOT Paint_Params, so the group freeze never
 // sees them. Restore the recording's values for the duration of the replay.
 i32 saved_viz_level            = p->viz_level;
 b32 saved_ignore_radii         = p->ignore_radii;
 b32 saved_ignore_alignment_min = p->ignore_alignment_min;
 b32 saved_show_grid            = p->show_grid;
 p->viz_level            = rec.viz_level;
 p->ignore_radii         = rec.ignore_radii;
 p->ignore_alignment_min = rec.ignore_alignment_min;
 p->show_grid            = rec.show_grid;

 global_replaying = true;  // NOTE(kv) suppress send_primitive during replay
 Paint_Params saved_params = p->params;
 b32 saved_is_right = m->is_right;
 m->is_right = false;  // NOTE(kv) the recording is left-side only (should_send_model_data)

 // NOTE(kv) Own bone-stack slot: draw_bezier reads the stack top for its alignment
 // check, so set_bone_transform alone isn't enough.
 push(&m->bone_stack, m->bone_stack.items[0]);
 Bone *cur_bone = 0;

 for_i32(iprim, 0, rec.primitives.count)
 {
  Recorded_Primitive &prim = rec.primitives.items[iprim];
  Recorded_Group &group = rec.groups.items[prim.group_index];

  if(cur_bone == 0 or not (group.bone_id == cur_bone->id))
  {// NOTE(kv) Bone is a group property (Q43a): one bone per group by construction.
   cur_bone = get_bone(group.bone_id, /*is_right*/false);
   m->bone_stack.items[m->bone_stack.count-1] = cur_bone;
   set_bone_transform(cur_bone->world_from_bone);
  }

  clear_draw_location();  // NOTE(kv) set_draw_location only ever raises the hot flag
  set_draw_location(prim.location);
  p->params = group.params;
  // NOTE(kv) Q32 live visibility: re-AND the frozen `painting` with the tag's live
  // value (driver publishes vis_live each frame). AND -- not overwrite -- so nested
  // conditions baked into the freeze (e.g. front_back_aligned) survive; a capture
  // taken with the toggle ON decomposes as painting = other_conditions AND toggle.
  p->params.painting = (p->params.painting and m->vis_live[group.vis_tag]);

  switch(prim.type)
  {
   case Primitive_Type_Curve:
   {// NOTE(kv) Style from the group fold + per-curve shape (Q44/Q47c).
    Line_Params line_params = p->params.line;
    line_params.radii = prim.curve.radii;
    line_params.lightness_additions = prim.curve.lightness_additions;
    if(prim.curve.straight){ line_params.flags |= Line_Straight; }
    draw_bezier(prim.curve.bezier.e, line_params);
   }break;

   case Primitive_Type_Poly3:
   {
    fill3(prim.poly3[0], prim.poly3[1], prim.poly3[2], get_fill_params());
   }break;

   case Primitive_Type_Dual_Bezier:
   {// NOTE(kv) Culling reads the view-scope stack; derive the view vector LIVE from
    // the group's recorded {view_center, view_bone} and the current camera (Q43b) --
    // same arithmetic as push_view_vector, so a same-frame diff is bit-identical.
    View_Scope scope = {};
    scope.center = group.view_center;
    scope.bone   = group.view_bone;
    v3 camera_obj = (get_bone(group.view_bone, /*is_right*/false)->world_from_bone.inv *
                     camera_world_position(p->camera));
    scope.vector = noz(camera_obj - group.view_center);
    p->view_scope_stack[p->view_scope_count++] = scope;
    fill_dual_bez(prim.dual_bezier.P.e, prim.dual_bezier.Q.e, get_fill_params());
    p->view_scope_count--;
   }break;

   case Primitive_Type_Patch:
   {
    fill_patch(prim.patch.e, get_fill_params());
   }break;

   case Primitive_Type_Disk:
   {
    fill_disk(prim.disk.center, {prim.disk.radius}, get_fill_params());
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
 p->viz_level            = saved_viz_level;
 p->ignore_radii         = saved_ignore_radii;
 p->ignore_alignment_min = saved_ignore_alignment_min;
 p->show_grid            = saved_show_grid;
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
