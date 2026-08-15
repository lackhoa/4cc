//-NOTE(kv) Replay path (draw-as-data step 3): re-issue draw calls from the recording
// (Model.primitives/groups) through the SAME outer draw functions the code path uses,
// so culling / hot-highlight / tessellation logic is shared by construction (Q22).
// Plan + decisions: ~/notes/tasks/autodraw_draw_as_data/plan-replay-path.md

// NOTE(kv) Q52: gates the per-frame store_recording below. Default on = today's
// behavior; debug-channel `recapture 0` freezes the recording so a loaded one
// survives frames (frozen document mode, for testing cross-frame behavior).
global b32 global_debug_recapture = true;

function void
seed_preset_settings(Preset_Settings settings[Game_Preset_Count])
{// NOTE(kv) Hardcoded defaults reproducing the digit-key behavior the presets had
 // when they were code branches (plan-preset-rethink Q57 inventory). First-run seed;
 // recording.ad overwrites these rows once the settings table persists.
 block_zero(settings, sizeof(Preset_Settings) * Game_Preset_Count);
 for_i32(preset, 0, Game_Preset_Count)
 {
  Preset_Settings &row = settings[preset];
  row.reference_image = -1;
  switch(preset)
  {
   case 1:
   {
    row.viz_level = 1;
    row.fill_only_picking = true;
   }break;
   case 2:
   {
    row.viz_level = 2;
    row.show_eyeball = true;
    row.show_loomis_ball = true;  // was the level2 default in render_head
   }break;
   case 3:
   {
    row.show_loomis_ball = true;
   }break;
   default: break;
  }
  if(preset >= 3)
  {// NOTE(kv) The old `render_preset >= 3` reference-image block.
   row.show_grid = true;
   row.show_arm_medial_right = true;
   i32 front_ref = preset - 4;
   if(0 <= front_ref and front_ref < 5){ row.reference_image = front_ref; }
   row.show_arm_back_bone    = (preset == 4);
   row.show_arm_profile_left = (preset >= 4);
  }
  row.ignore_radii         = (row.viz_level != 0);
  row.ignore_alignment_min = (row.viz_level != 0);
 }
}

function void
store_recording()
{// NOTE(kv) Snapshot the per-frame capture (Model.primitives/groups) into the one
 // recording (Q36/Q57). Runs right after viewport 0's driver_render.
 Model *m = the_model;
 Recording &rec = m->recordings.recording;
 arena_clear(&rec.arena);
 init_dynamic(rec.primitives, &rec.arena, maximum(1, m->primitives.count));
 init_dynamic(rec.groups,     &rec.arena, maximum(1, m->groups.count));
 set_count(&rec.primitives, m->primitives.count);
 block_copy(rec.primitives.items, m->primitives.items,
            sizeof(Recorded_Primitive) * m->primitives.count);
 set_count(&rec.groups, m->groups.count);
 block_copy(rec.groups.items, m->groups.items,
            sizeof(Recorded_Group) * m->groups.count);
 rec.captured = true;
}

function void
replay_recording(Recording &rec)
{// NOTE(kv) Second consumer of a captured recording: bones/camera are live, the
 // primitives/groups come from the preset's slot. Correctness bar (same-frame,
 // same-preset): the vertex stream is bit-identical to the code path's (Diff-now).
 Model *m = the_model;
 Painter *p = painter;

 // NOTE(kv) Painter-level display state (viz_level/ignore/show_grid) is applied
 // LIVE from the active preset's settings row: the painter already holds those
 // values from this frame's driver_render, so nothing to save/restore (Q57).

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
  if(group.cam_vis.active)
  {// NOTE(kv) Q38 camera-bound visibility: re-evaluate the recorded condition against
   // the LIVE view vector (derived from the group's view scope + current camera, Q42)
   // and AND it in -- same decomposition argument as the vis_live re-AND above.
   v3 live_view = view_vector_from(get_bone(group.view_bone, /*is_right*/false)->world_from_bone,
                                   group.view_center);
   v1 alignment = dot(group.cam_vis.normal, live_view);
   if(group.cam_vis.symmetric){ alignment = absolute(alignment); }
   p->params.painting = (p->params.painting and (alignment > group.cam_vis.min_alignment));
  }

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
    // the group's recorded {view_center, view_bone} and the current camera (Q43b).
    View_Scope scope = {};
    scope.center = group.view_center;
    scope.bone   = group.view_bone;
    scope.vector = view_vector_from(get_bone(group.view_bone, /*is_right*/false)->world_from_bone,
                                    group.view_center);
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
