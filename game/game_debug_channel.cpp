// NOTE(kv) File-based debug command channel (see
// ~/notes/tasks/autodraw_draw_as_data/plan-self-debug-visibility.md).
// Gated by the `-debug-cmd` launch arg (off by default). Each frame we poll
// <exe_dir>/debug/cmd.txt; if present: execute the command, write ack/results to
// <exe_dir>/debug/out.txt (overwritten per command), delete cmd.txt.
//
// v1 commands (one per file):
//   screenshot        -> request exe-side capture -> debug/screenshot_<n>.png
//   diff              -> trigger Diff-now, result written on the NEXT update
//   force_animate 0|1 -> set Replay_State.force_animate
//   dump_state        -> key counts + replay/diff state as text
//   slider_dump       -> every data slider: "<side> <id> <type> <value as code>", plus orphan rows
//   slider_prune      -> drop orphan rows (no slider claims them) and rewrite both values files
//   export_group <tag> -> move a Vis_* tagged region of the live capture into the document
//   document_dump     -> the document recording as text: groups, primitives, vertex table
//   slider_write      -> save both values files from the live slider tables
//   slider <id> <n>.. -> set a slider (scalars/vectors by component) and save its file
//   slider_next_id <type> -> first free "<type>_<n>" id across both sides, e.g. v3_227
//   set_camera <theta> <phi> [distance [pivot_x y z]]
//   screenshot [x y w h] -> optional crop in png pixels (top-left origin)
//   reload_autosave   -> load data/autosave.ad (the live instance's view), camera included
//   mouse_move <x> <y> -> park a virtual mouse at window pixels (top-left origin, same
//                        frame as the screenshot png); picking runs against it every frame
//   mouse_down <x> <y> [shift|alt|ctrl|middle] / mouse_up -> press/release the virtual left button there
//                        (drives the same document_edit_* as the real mouse: near a
//                        control point of a SELECTED primitive = drag it, on an unselected
//                        one = select it (no drag); shift = toggle the hot document curve
//                        in the selection, no drag; ctrl = free handle drag, the other
//                        handle swings into the new plane (plan-handle-drag-modes))
//   select <i> [j ...] / select none -> set the document selection (curves or patches)
//   key delete        -> delete the selection, as the Delete/Backspace key does
//   make_patch <i> <j> [k] [l] -> curve patch primitive over those document curves
//   link <v> <v> [v...] / unlink <v> [v...] -> vertex links (table indices, plan-vertex-links)
//   curve_offsets <prim> -> full-precision endpoints + handle offsets of one curve
//   delete_patch <i>  -> remove a curve patch primitive
//   delete_curve <i>  -> remove a curve primitive (patches using it drop the entry; a patch
//                        left with < 2 curves goes too)
//   line_tool 0|1     -> arm/disarm the line tool (game_document_line_tool.cpp); then
//                        mouse_down/mouse_move.../mouse_up draws one curve, `hot` shows the tool state
//   coplanarize <i>   -> swing curve i's d3 into the {chord, d0} plane (one history entry)
//   patch_grid <i>    -> evaluated grid size + corner/center px of a curve patch
//   mouse_off         -> release the virtual mouse
//   hot               -> the hot location picked on the last frame (document prim / code range)
//   prim_px <i>       -> control-point px of document primitive i (same block as `hot`)
//   quit              -> exit this instance
//
// cdb remains the fallback for crashes/breakpoints/ad-hoc struct inspection.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

global b32  debug_channel_initialized;
// NOTE(kv) debug_channel_enabled lives in framework.h (document_file_path needs it).
global char debug_channel_dir[MAX_PATH];      // <exe_dir>/debug
global char debug_channel_cmd_path[MAX_PATH];
global char debug_channel_out_path[MAX_PATH];
// NOTE(kv) Frames left before we report the pending diff result: the diff is computed
// during this frame's render (call_driver_render), so the result is only trustworthy
// at the start of the NEXT game_update.
global i32  debug_channel_pending_diff;
// NOTE(kv) Transient per-frame: keeps frames flowing while a command needs a render.
global b32  debug_channel_wants_animate;
global b32  debug_channel_request_exit;  // `quit` command, consumed by the custom layer
global u32  debug_channel_ack_counter;   // sequence number on every out.txt
// NOTE(kv) Virtual mouse (plan-document-mouse-editing Q9): the real mouse sits wherever
// the user left it, so picking is off in agent mode; the channel parks this one instead
// and game_update substitutes it for params.mouse.p.
global b32  debug_channel_mouse_active;
global i2   debug_channel_mouse_p;
global b32  debug_channel_mouse_left;           // held state
global b32  debug_channel_mouse_press_pending;  // one-frame edges
global b32  debug_channel_mouse_release_pending;
global b32  debug_channel_mouse_shift;           // shift held for the virtual press
global b32  debug_channel_mouse_alt;             // alt held for the virtual press (camera pan drag)
global b32  debug_channel_mouse_ctrl;            // ctrl held for the virtual press (free handle drag)
global b32  debug_channel_mouse_middle;          // virtual middle button held (camera pan drag)
global Location debug_channel_last_hot;  // from the last frame's picking
global rect2 debug_channel_mouse_viewport_box;  // clip box of the viewport under it
// NOTE(kv) How often the agent instance wakes up to poll cmd.txt when nothing animates.
// Every poll runs a full game_update + render (~150 ms at -Od), so 200 ms was ~15% CPU.
#define DEBUG_CHANNEL_POLL_MS 500

function void
debug_channel_init()
{
 debug_channel_initialized = true;
 const char *cmdline = GetCommandLineA();
 debug_channel_enabled = (strstr(cmdline, "-debug-cmd") != 0);
 if(!debug_channel_enabled){ return; }

 char exe_path[MAX_PATH];
 GetModuleFileNameA(0, exe_path, sizeof(exe_path));
 char *last_slash = strrchr(exe_path, '\\');
 if(last_slash){ *last_slash = 0; }
 snprintf(debug_channel_dir,      sizeof(debug_channel_dir),      "%s\\debug", exe_path);
 snprintf(debug_channel_cmd_path, sizeof(debug_channel_cmd_path), "%s\\cmd.txt", debug_channel_dir);
 snprintf(debug_channel_out_path, sizeof(debug_channel_out_path), "%s\\out.txt", debug_channel_dir);
 CreateDirectoryA(debug_channel_dir, 0);

 // NOTE(kv) Window size/placement in agent mode is the exe's job now (win32_4ed.cpp,
 // search agent_mode: work-area sized, shown without activating, bottom of the
 // z-order). The ShowWindow(SW_MAXIMIZE) that used to live here always activated the
 // window and stole the user's foreground on every launch (2026-09-12).
}

function FILE *
debug_channel_open_out()
{
 return fopen(debug_channel_out_path, "wb");
}

function void
debug_channel_write_diff_result(FILE *out, Replay_Diff_Result &diff)
{
 if(!diff.valid)
 {
  fprintf(out, "diff: no valid result (was a render skipped?)\n");
  return;
 }
 if(diff.match)
 {
  fprintf(out, "diff: match (%d vertices)\n", diff.code_vertex_count);
 }
 else
 {
  fprintf(out, "diff: MISMATCH code %d vs replay %d vertices\n",
          diff.code_vertex_count, diff.replay_vertex_count);
  fprintf(out, "first_diff_vertex: %d\n", diff.first_diff_vertex);
  fprintf(out, "code loc: file %d [%d,%d)\n",
          diff.code_location.file.index,
          diff.code_location.range.min, diff.code_location.range.max);
  fprintf(out, "replay loc: file %d [%d,%d)\n",
          diff.replay_location.file.index,
          diff.replay_location.range.min, diff.replay_location.range.max);
 }
}

// NOTE(kv) The game DLL can't read the composed frame itself: at game_update time the
// back buffer is post-swap garbage and GL_FRONT is all-black under DWM (tested
// 2026-08-14). So we drop debug/screenshot_request.txt; the exe captures pre-swap
// (ogl_debug_maybe_screenshot in 4ed_opengl_render.cpp) and writes
// debug/screenshot_result.txt with the png path.
function void
debug_channel_screenshot(FILE *out, const char *crop_args)
{
 char request_path[MAX_PATH];
 snprintf(request_path, sizeof(request_path), "%s\\screenshot_request.txt",
          debug_channel_dir);
 FILE *request = fopen(request_path, "wb");
 if(!request)
 {
  fprintf(out, "error: cannot write %s\n", request_path);
  return;
 }
 // NOTE(kv) Optional "x y w h" crop in png pixels (top-left origin); the render side
 // parses it, empty file = full frame.
 fputs(crop_args, request);
 fclose(request);
 // NOTE(kv) Delete the previous result so the reader can't mistake it for this one.
 char result_path[MAX_PATH];
 snprintf(result_path, sizeof(result_path), "%s\\screenshot_result.txt",
          debug_channel_dir);
 DeleteFileA(result_path);
 debug_channel_wants_animate = true;  // make sure a render happens to fulfill it
 fprintf(out, "screenshot: requested, result in debug/screenshot_result.txt\n");
}

function void
debug_channel_dump_state(FILE *out, Game_State *state)
{
 Model *m = &state->model;
 fprintf(out, "primitives: %d\n", m->primitives.count);
 fprintf(out, "groups: %d\n",     m->groups.count);
 // NOTE(kv) plan-curve-chord-handles Q3: nonzero means a captured curve mixed bone
 // spaces (offsets got converted through bone transforms); see game_draw.cpp.
 fprintf(out, "curve_mixed_bone_captures: %d\n", curve_mixed_bone_capture_count);
 fprintf(out, "recorded_vertices: %d\n", m->recorded_vertices.count);
 {// NOTE(kv) Viewport 0 camera: target (what the keys/mouse drags set) vs current (animated).
  Camera_Data &t = state->viewports[0].target_camera;
  Camera_Data &c = state->viewports[0].camera;
  fprintf(out, "camera target: theta=%.4f phi=%.4f distance=%.4f pivot=(%.4f %.4f %.4f)\n",
          t.theta, t.phi, t.distance, t.pivot.x, t.pivot.y, t.pivot.z);
  fprintf(out, "camera current: theta=%.4f phi=%.4f distance=%.4f pivot=(%.4f %.4f %.4f)\n",
          c.theta, c.phi, c.distance, c.pivot.x, c.pivot.y, c.pivot.z);
  Camera_Drag &d = state->camera_drag;
  fprintf(out, "camera_drag: active %d pan %d middle %d remainder (%.1f %.1f)\n", d.active, d.pan, d.middle, d.remainder_px.x, d.remainder_px.y);
 }
 {
  Recording &doc = m->recordings.document;
  fprintf(out, "document: %s, %d primitives, %d groups, %d vertices\n",
          doc.captured ? "loaded" : "empty",
          doc.primitives.count, doc.groups.count, doc.vertices.count);
 }
 fprintf(out, "vertices: %d\n",   m->vertices.count);
 Replay_State &replay = state->replay;
 fprintf(out, "display_replay: %d\n", replay.display_replay);
 fprintf(out, "diff_requested: %d\n", replay.diff_requested);
 fprintf(out, "force_animate: %d\n",  replay.force_animate);
 fprintf(out, "recapture: %d\n",      replay.recapture);
 fprintf(out, "weight_blink: %f\n",   m->weight_live[Weight_Blink]);
 fprintf(out, "cycles: frame %u, driver_render %u (x%u), render_character %u, reference_mesh %u\n",
         debug_cycles.frame, debug_cycles.driver_render, debug_cycles.driver_render_calls,
         debug_cycles.render_character, debug_cycles.reference_mesh);
 debug_channel_write_diff_result(out, replay.last_diff);
}

function void
debug_channel_slider_dump(FILE *out, Game_State *state)
{
 Scratch_Scope tmp;
 for_i32(is_driver, 0, 2)
 {
  // NOTE(kv) Orphans = rows in the values file no slider claims (call site gone or
  // migrated to the document). `slider_prune` drops them.
  darray(Slider_Value_Row) &orphans = state->orphan_slider_rows[is_driver];
  for_each(row, orphans)
  {
   fprintf(out, "%s orphan %.*s (%d bytes)\n", is_driver ? "driver" : "game",
           string_expand(row->id), cast(i32)row->bytes.size);
  }
  sarray(FUI_File_Data) files = get_file_array({i16(is_driver), 0});
  for_i32(file_index, 1, files.count)
  {
   for_each(slider, files[file_index].sliders)
   {
    if(slider->id.size == 0){ continue; }
    Printer printer = make_printer_buffer(tmp, 256);
    print_code(printer, slider->type, slider->value, /*wrapped*/false);
    String value = printer_get_string(printer);
    fprintf(out, "%s %.*s %.*s %.*s\n", is_driver ? "driver" : "game",
            string_expand(slider->id), string_expand(slider->type->name),
            string_expand(value));
   }
  }
  fprintf(out, "%s orphans: %d\n", is_driver ? "driver" : "game", orphans.count);
 }
}

// NOTE(kv) Which Paint_Params fields a group overrides, by name (PaintFieldList order).
global char const *paint_field_names[] = {
#define X(name, path) #name,
 PaintFieldList(X)
#undef X
};

function void
debug_channel_print_tvert(FILE *out, tvert const &t)
{
 fprintf(out, "(%g %g %g bone %d:%d)", t.x, t.y, t.z, t.bone_id.type, t.bone_id.id);
}

function void
debug_channel_print_primitive_px(FILE *out, Game_State *state, i32 prim_index, b32 is_right)
{// NOTE(kv) Where a primitive's control points are on screen, so a drag can aim at one.
 Recording &doc = state->model.recordings.document;
 Recorded_Primitive &prim = doc.primitives[prim_index];
 Screen_Projection_Data proj = mk_screen_projection_data(state, get_center(debug_channel_mouse_viewport_box));
 auto print_pick = [&](const char *label, Document_Pick pick)
 {
  v3 world = document_pick_world_pos(doc, pick);
  v2 px = project(proj, world);
  // NOTE(kv) cam_z = camera-space depth (negative in front), the line tool's stroke-plane test.
  fprintf(out, "  %s slot %d: px (%.0f %.0f) cam_z %.4f\n", label, pick.slot, px.x, px.y,
          mat4vert(proj.camera.cam_from_world, world).z);
 };
 for_i32(slot, 0, primitive_vertex_count(prim.type))
 {
  print_pick("vertex", {prim_index, is_right, false, slot});
 }
 if(prim.type == Primitive_Type_Curve)
 {
  print_pick("handle", {prim_index, is_right, true, 1});
  print_pick("handle", {prim_index, is_right, true, 2});
  // NOTE(kv) Points ON the curve body, for aiming a pick test at the line itself
  // (plan-pick-curves-over-patches): projected world bezier, not a px-space bezier.
  v3 P[4] = {
   document_pick_world_pos(doc, {prim_index, is_right, false, 0}),
   document_pick_world_pos(doc, {prim_index, is_right, true,  1}),
   document_pick_world_pos(doc, {prim_index, is_right, true,  2}),
   document_pick_world_pos(doc, {prim_index, is_right, false, 1}),
  };
  fprintf(out, "  body px:");
  for(v1 t = 0.25f; t < 0.9f; t += 0.25f)
  {
   v2 px = project(proj, bezier_sample(P, t));
   fprintf(out, " t=%.2f (%.0f %.0f)", t, px.x, px.y);
  }
  fprintf(out, "\n");
 }
 if(prim.type == Primitive_Type_Curve_Patch)
 {
  fprintf(out, "  curve_patch: curves");
  for_i32(i, 0, prim.curve_patch.curve_count){ fprintf(out, " %d", prim.curve_patch.curve_index[i]); }
  fprintf(out, "\n");
 }
}

function void
debug_channel_document_dump(FILE *out, Game_State *state)
{// NOTE(kv) Text view of `recordings.document` for reading, not for loading back.
 // Bones print as Bone_Type:id (see the Bone_Type enum). Locations are the source
 // char range of the ORIGINAL draw call (the code is gone after an export, so it's
 // only a breadcrumb). Vertex refs point into the table at the end; the by-value
 // positions inside each primitive are what the table resolves over at replay.
 Recording &doc = state->model.recordings.document;
 fprintf(out, "document: %s, %d groups, %d primitives, %d vertices\n",
         doc.captured ? "loaded" : "empty",
         doc.groups.count, doc.primitives.count, doc.vertices.count);

 fprintf(out, "\n[groups]\n");
 for_i32(igroup, 0, doc.groups.count)
 {
  Recorded_Group &g = doc.groups.items[igroup];
  fprintf(out, "group %d: parent %d, tag %.*s, bone %d:%d, loc file %d range %d..%d%s\n",
          igroup, g.parent_index, strexpand(group_vis_name(g.vis_tag)),
          g.bone_id.type, g.bone_id.id,
          g.location.file.index, g.location.range.min, g.location.range.max,
          g.one_sided ? ", one_sided" : "");
  if(g.cam_vis.active)
  {
   fprintf(out, "  cam_vis: normal (%g %g %g) min_alignment %g%s, view_center (%g %g %g) in bone %d:%d\n",
           g.cam_vis.normal.x, g.cam_vis.normal.y, g.cam_vis.normal.z,
           g.cam_vis.min_alignment, g.cam_vis.symmetric ? " symmetric" : "",
           g.view_center.x, g.view_center.y, g.view_center.z,
           g.view_bone.type, g.view_bone.id);
  }
  Paint_Params &pp = g.params;
  fprintf(out, "  params: painting %d, line_color %08x, line_flags %x, fill_color %08x, line_depth_offset %g, fill_depth_offset %g, radius_mult %g\n",
          pp.painting, pp.line_color, pp.line.flags, pp.fill.color, pp.line_depth_offset,
          pp.fill_depth_offset, pp.radius_mult);
  fprintf(out, "  changed vs parent:");
  for_i32(ifield, 0, ArrayCount(paint_field_names))
  {
   if(g.changed_mask & (1u << ifield)){ fprintf(out, " %s", paint_field_names[ifield]); }
  }
  fprintf(out, "\n");
 }

 fprintf(out, "\n[primitives]\n");
 for_i32(iprim, 0, doc.primitives.count)
 {
  Recorded_Primitive &prim = doc.primitives.items[iprim];
  fprintf(out, "prim %d: group %d, loc file %d range %d..%d, vertex refs",
          iprim, prim.group_index,
          prim.location.file.index, prim.location.range.min, prim.location.range.max);
  for_i32(i, 0, primitive_vertex_count(prim.type)){ fprintf(out, " %d", prim.vertex_index[i]); }
  fprintf(out, "\n  ");
  switch(prim.type)
  {
   case Primitive_Type_Curve:
   {
    Recorded_Curve &c = prim.curve;
    fprintf(out, "curve%s%s: ", c.straight ? " (straight)" : "", c.midline ? " (midline)" : "");
    // NOTE(kv) plan-curve-table-first: print the truth (table vertices + handle
    // offsets), never the scratch `bezier`, so staleness has nowhere to hide. Order
    // v0 d0 d1 v1, then the BUILT h0 h1 (plan-curve-chord-handles) so a dump compares
    // against the pre-offset dumps.
    for_i32(i, 0, 4)
    {
     tvert point;
     if(i == 0 or i == 3)
     {
      point = document_curve_endpoint(doc, prim, i == 0 ? 0 : 1);
      fprintf(out, "v%d", i == 0 ? 0 : 1);
     }
     else
     {
      point = c.handle_offset[i-1];
      fprintf(out, "d%d", i-1);
     }
     debug_channel_print_tvert(out, point);
     fprintf(out, " ");
    }
    fprintf(out, "\n  built ");
    for_i32(i, 0, 2)
    {
     fprintf(out, "h%d", i);
     debug_channel_print_tvert(out, document_curve_handle_point(doc, prim, i));
     fprintf(out, " ");
    }
    fprintf(out, "\n  radii (%g %g %g %g) lightness (%g %g %g %g)",
            c.radii.x, c.radii.y, c.radii.z, c.radii.w,
            c.lightness_additions.x, c.lightness_additions.y,
            c.lightness_additions.z, c.lightness_additions.w);
   }break;
   case Primitive_Type_Poly3:
   {
    fprintf(out, "poly3: ");
    for_i32(i, 0, 3){ debug_channel_print_tvert(out, prim.poly3.points[i]); fprintf(out, " "); }
   }break;
   case Primitive_Type_Dual_Bezier:
   {
    fprintf(out, "dual_bezier P: ");
    for_i32(i, 0, 4){ debug_channel_print_tvert(out, prim.dual_bezier.P.e[i]); fprintf(out, " "); }
    fprintf(out, "\n  Q: ");
    for_i32(i, 0, 4){ debug_channel_print_tvert(out, prim.dual_bezier.Q.e[i]); fprintf(out, " "); }
   }break;
   case Primitive_Type_Patch:
   {
    fprintf(out, "patch:");
    for_i32(row, 0, 4)
    {
     fprintf(out, "\n   ");
     for_i32(col, 0, 4){ debug_channel_print_tvert(out, prim.patch.e[row][col]); fprintf(out, " "); }
    }
   }break;
   case Primitive_Type_Disk:
   {
    fprintf(out, "disk: center ");
    debug_channel_print_tvert(out, prim.disk.center);
    fprintf(out, " radius %g", prim.disk.radius);
   }break;
   case Primitive_Type_Point:
   {// NOTE(kv) The position is the table vertex (see "vertex refs"); `p` is only a cache.
    fprintf(out, "point");
   }break;
   case Primitive_Type_Image:
   {
    fprintf(out, "image: %s", prim.image.filename.str);
   }break;
   default: { fprintf(out, "type %d", prim.type); }break;
  }
  fprintf(out, "\n");
 }

 fprintf(out, "\n[vertices]\n");
 for_i32(ivert, 0, doc.vertices.count)
 {
  Recorded_Vertex &v = doc.vertices.items[ivert];
  fprintf(out, "vertex %d: (%g %g %g) bone %d:%d", ivert, v.p.x, v.p.y, v.p.z, v.bone.type, v.bone.id);
  if(v.link_id != 0){ fprintf(out, " link %d", v.link_id); }
  fprintf(out, "\n");
 }
}

// NOTE(kv) Data slider ids are "<type>_<n>" (fv(v3_225)); the next free id is max+1
// over both sides, so a new fv() in the driver never collides with a game-side one.
function void
debug_channel_slider_next_id(FILE *out, char *type_name)
{
 Scratch_Scope tmp;
 String prefix = push_stringf(tmp, "%s_", type_name);
 i32 max_n = -1;
 for_i32(is_driver, 0, 2)
 {
  sarray(FUI_File_Data) files = get_file_array({i16(is_driver), 0});
  for_i32(file_index, 1, files.count)
  {
   for_each(slider, files[file_index].sliders)
   {
    if(not starts_with(slider->id, prefix)){ continue; }
    i32 n = 0;  // NOTE(kv) ids aren't null-terminated, so no atoi
    for(u64 i = prefix.size; i < slider->id.size && isdigit((u8)slider->id.data[i]); i++)
    {
     n = n*10 + (slider->id.data[i] - '0');
    }
    max_n = Max(max_n, n);
   }
  }
 }
 fprintf(out, "slider_next_id: %.*s%d\n", string_expand(prefix), max_n+1);
}

function void
debug_channel_slider_set(FILE *out, Game_State *state, char *args)
{// NOTE(kv) `slider <id>[.<member>] <c0> [c1 c2 c3]`: components are floats (ints
 // for i1); wrapper types (tvert/tnormal/tdim) set their wrapped value and keep the
 // rest; struct sliders (Curve, FUI_Line_Params) take one member at a time.
 char id_buffer[64] = {};
 int consumed = 0;
 if(sscanf(args, "%63s%n", id_buffer, &consumed) != 1)
 {
  fprintf(out, "slider: usage: slider <id>[.<member>] <c0> [c1 c2 c3]\n");
  return;
 }
 String id = SCu8(id_buffer);
 String member_name = {};
 {
  i64 dot = string_find_last(id, '.');
  if(dot > 0)
  {
   member_name = string_skip(id, u64(dot+1));
   id = string_prefix(id, u64(dot));
  }
 }
 Slider *slider = 0;
 b32 is_driver = 0;
 for_i32(side, 0, 2)
 {
  slider = find_slider_by_id(side, id);
  if(slider){ is_driver = side; break; }
 }
 if(slider == 0)
 {
  fprintf(out, "slider: no slider with id %s\n", id_buffer);
  return;
 }
 void *value = slider->value;
 Type_Info *type = slider->type;
 if(is_struct(type))
 {
  I_Struct_Member *member = 0;
  for_each(candidate, type->members)
  {
   if(candidate->name == member_name){ member = candidate; break; }
  }
  if(member == 0)
  {
   fprintf(out, "slider: %.*s has type %.*s; usage: slider %.*s.<member> ..., members:",
           string_expand(id), string_expand(type->name), string_expand(id));
   for_each(candidate, type->members){ fprintf(out, " %.*s", string_expand(candidate->name)); }
   fprintf(out, "\n");
   return;
  }
  value = (u8 *)value + member->offset;
  type  = member->type;
 }
 Type_Info *basic = strip_to_basic_type(type);
 b32 is_int = type_info_equals(basic, i1);
 i32 component_count = basic->size / 4;
 char *cursor = args + consumed;
 i32 parsed = 0;
 for_i32(i, 0, component_count)
 {
  char *end = 0;
  if(is_int)
  {
   long parsed_value = strtol(cursor, &end, 10);
   if(end == cursor){ break; }
   ((i32 *)value)[i] = (i32)parsed_value;
  }
  else
  {
   float parsed_value = strtof(cursor, &end);
   if(end == cursor){ break; }
   ((v1 *)value)[i] = parsed_value;
  }
  cursor = end;
  parsed += 1;
 }
 b32 saved = save_slider_values_file(state, is_driver);
 fprintf(out, "slider: %s set %d of %d components, save %s\n",
         id_buffer, parsed, component_count, saved ? "ok" : "FAILED");
}

// NOTE(kv) Called at the top of game_update, every frame.
function void
debug_channel_update(Game_State *state, App *app)
{
 if(!debug_channel_initialized){ debug_channel_init(); }
 if(!debug_channel_enabled){ return; }
 debug_channel_wants_animate = false;

 if(debug_channel_pending_diff > 0)
 {// NOTE(kv) Last frame's `diff` command has rendered by now; report it.
  debug_channel_pending_diff -= 1;
  if(debug_channel_pending_diff == 0)
  {
   FILE *out = debug_channel_open_out();
   if(out)
   {
    debug_channel_write_diff_result(out, state->replay.last_diff);
    fclose(out);
   }
  }
  else
  {
   debug_channel_wants_animate = true;
  }
  return;  // don't read a new command until the pending one is reported
 }

 FILE *cmd_file = fopen(debug_channel_cmd_path, "rb");
 if(!cmd_file){ return; }
 char cmd[256] = {};
 isize cmd_len = fread(cmd, 1, sizeof(cmd)-1, cmd_file);
 fclose(cmd_file);
 // NOTE(kv) A trailing newline is the "write complete" terminator: we poll every
 // frame and can catch the file half-written (hit this in testing). No newline yet
 // -> leave the file alone and retry next frame. `echo cmd > cmd.txt` adds the
 // newline for free, so writers don't need a tmp+rename dance.
 if(cmd_len == 0 || cmd[cmd_len-1] != '\n'){ return; }
 DeleteFileA(debug_channel_cmd_path);
 // strip trailing whitespace/newlines
 for(isize i = strlen(cmd)-1; i >= 0 && isspace((u8)cmd[i]); i--){ cmd[i] = 0; }

 FILE *out = debug_channel_open_out();
 if(!out){ return; }
 debug_channel_ack_counter += 1;
 fprintf(out, "ack #%u: %s\n", debug_channel_ack_counter, cmd);

 if(strncmp(cmd, "screenshot", 10) == 0)
 {
  debug_channel_screenshot(out, cmd+10);
 }
 else if(strcmp(cmd, "quit") == 0)
 {
  debug_channel_request_exit = true;
  // NOTE(kv) The exit signal is only acted on in the NEXT app_step, and an idle agent
  // gets no frames (win32 WM_TIMER skips them while no cmd.txt waits), so ask for one.
  // Without it the instance lived on and answered the next command (2026-09-19).
  debug_channel_wants_animate = true;
  fprintf(out, "quit: exiting\n");
 }
 else if(strcmp(cmd, "reload_autosave") == 0)
 {// NOTE(kv) See what the user sees: reload data/state.txt (the live instance's
  // periodic save), camera + presets included. Same as the revert command; overwrites
  // this instance's edit history, which an agent instance does not care about.
  // (Command name kept from the autosave.ad days.)
  b32 ok = load_state_file(state);
  for_i32(viewport_index, 0, GAME_VIEWPORT_COUNT)
  {
   Viewport *viewport = &state->viewports[viewport_index];
   viewport->camera = viewport->target_camera;  // no animation tail
  }
  Camera_Data camera = state->viewports[0].camera;
  fprintf(out, "reload_autosave: %s; camera theta=%f phi=%f distance=%f pivot=(%f %f %f)\n",
          ok ? "ok" : "FAILED", camera.theta, camera.phi, camera.distance,
          camera.pivot.x, camera.pivot.y, camera.pivot.z);
  debug_channel_wants_animate = true;
 }
 else if(strcmp(cmd, "diff") == 0)
 {
  state->replay.diff_requested = true;
  debug_channel_pending_diff = 2;  // render this frame, report next update
  debug_channel_wants_animate = true;
  fprintf(out, "diff: requested, result on next frame\n");
 }
 else if(strncmp(cmd, "force_animate", 13) == 0)
 {
  state->replay.force_animate = (atoi(cmd+13) != 0);
  fprintf(out, "force_animate: %d\n", state->replay.force_animate);
 }
 else if(strcmp(cmd, "dump_state") == 0)
 {
  debug_channel_dump_state(out, state);
 }
 else if(strcmp(cmd, "preset_dump") == 0)
 {// NOTE(kv) Every preset row; `*` marks the main viewport's active one.
  Model_Recordings &rec = state->model.recordings;
  fprintf(out, "presets: %d (last %d)\n", rec.preset_count, state->viewports[0].last_preset);
  for_i32(index, 0, rec.preset_count)
  {
   Preset_Settings &row = rec.preset_settings[index];
   fprintf(out, "%c%2d \"%s\" viz %d ref_image %d scene %d",
           index == state->viewports[0].preset ? '*' : ' ',
           index, row.name, row.viz_level, row.reference_image, (int)row.scene);
   Type_Info *type = &Type_Info_Preset_Settings;
   for_i32(mi, 0, type->members.count)
   {// NOTE(kv) every b32 member that is on, by reflection
    I_Struct_Member &member = type->members[mi];
    if(member.type == &Type_Info_b32 and *cast(b32 *)(cast(u8 *)&row + member.offset))
    {
     fprintf(out, " %.*s", strexpand(member.name));
    }
   }
   fprintf(out, "\n");
  }
 }
 else if(strncmp(cmd, "preset_add", 10) == 0)
 {
  b32 ok = preset_add(state, state->viewports[0].preset);
  fprintf(out, "preset_add: %s (count %d)\n", ok ? "ok" : "FAILED", state->model.recordings.preset_count);
 }
 else if(strncmp(cmd, "preset_delete", 13) == 0)
 {
  b32 ok = preset_delete(state, state->viewports[0].preset);
  fprintf(out, "preset_delete: %s (count %d)\n", ok ? "ok" : "FAILED", state->model.recordings.preset_count);
 }
 else if(strncmp(cmd, "preset_name ", 12) == 0)
 {
  Preset_Settings &row = active_preset_row(state);
  snprintf(row.name, sizeof(row.name), "%s", cmd+12);
  fprintf(out, "preset_name: \"%s\"\n", row.name);
 }
 else if(strcmp(cmd, "save_recording") == 0)
 {
  b32 ok = save_recording_file(state);
  fprintf(out, "save_recording: %s\n", ok ? "ok" : "FAILED");
 }
 else if(strcmp(cmd, "load_recording") == 0)
 {
  b32 ok = load_recording_file(state);
  fprintf(out, "load_recording: %s\n", ok ? "ok" : "FAILED");
  debug_channel_wants_animate = true;
 }
 else if(strcmp(cmd, "save_document") == 0)
 {
  b32 ok = save_document_file(state);
  fprintf(out, "save_document: %s\n", ok ? "ok" : "FAILED");
 }
 else if(strcmp(cmd, "load_document") == 0)
 {
  b32 ok = load_document_file(state);
  fprintf(out, "load_document: %s\n", ok ? "ok" : "FAILED");
  debug_channel_wants_animate = true;
 }
 else if(strcmp(cmd, "document_copy_from_live") == 0)
 {// NOTE(kv) plan-selection-followups Q3: refresh the agent's own document
  // (driver.document.agent.ad) from the live driver.document.ad, then reload it.
  Scratch_Scope tmp;
  Stringz src = live_document_file_path(tmp, state);
  Stringz dst = document_file_path(tmp, state);
  Stringz data = read_entire_file(tmp, src);
  b32 ok = (data.len > 0);
  if(ok)
  {
   Stringz temp_path = pjoin(tmp, state->save_dir, strlit("document_copy_temp.ad"));
   FILE *file = open_file(temp_path, "wb");
   ok = (file != 0);
   if(ok)
   {
    ok = (fwrite(data.str, 1, data.len, file) == (size_t)data.len);
    close_file(file);
   }
   if(ok)
   {
    if(file_exists(dst)){ remove_file(dst); }
    ok = move_file(temp_path, dst);
   }
  }
  if(ok)
  {
   ok = load_document_file(state);
  }
  fprintf(out, "document_copy_from_live: %s (%s -> %s)\n", ok ? "ok" : "FAILED", to_cstring(src), to_cstring(dst));
  debug_channel_wants_animate = true;
 }
 else if(strncmp(cmd, "export_group ", 13) == 0)
 {// NOTE(kv) `export_group Vis_Nose`: move that tagged region from the live capture
  // into the document (game_document.cpp), weld, save driver.document.ad.
  char const *name = cmd + 13;
  Group_Vis tag = Vis_None;
  group_vis_from_name(SCu8(name), &tag);
  if(tag == Vis_None)
  {
   fprintf(out, "export_group: unknown tag '%s'\n", name);
  }
  else
  {
   Document_Export_Result r = export_group_to_document(state, tag);
   fprintf(out, "export_group %.*s: %s, %d groups, %d primitives, %d vertices (%d welded)\n",
           strexpand(group_vis_name(tag)), r.ok ? "ok" : "FAILED",
           r.group_count, r.primitive_count, r.vertex_count, r.welded_count);
   debug_channel_wants_animate = true;
  }
 }
 else if(strcmp(cmd, "document_dump") == 0)
 {
  debug_channel_document_dump(out, state);
 }
 else if(strcmp(cmd, "undo") == 0)
 {
  fprintf(out, "undo: %s\n", history_undo(state) ? "ok" : "nothing to undo");
  debug_channel_wants_animate = true;
 }
 else if(strcmp(cmd, "redo") == 0)
 {
  fprintf(out, "redo: %s\n", history_redo(state) ? "ok" : "nothing to redo");
  debug_channel_wants_animate = true;
 }
 else if(strcmp(cmd, "history_dump") == 0)
 {
  history_dump(out, state);
 }
 //-NOTE(kv) plan-document-checkpoints. <n> is the checkpoint's NUMBER (file name), not an index.
 else if(strcmp(cmd, "checkpoint_create") == 0)
 {
  i32 number = document_checkpoint_create(state);
  fprintf(out, "checkpoint_create: %s (%d)\n", number ? "ok" : "FAILED", number);
  debug_channel_wants_animate = true;
 }
 else if(strcmp(cmd, "checkpoint_list") == 0)
 {
  document_checkpoint_dump(out, state);
 }
 else if(strncmp(cmd, "checkpoint_compare_with ", 24) == 0)
 {
  i32 index = document_checkpoint_index_from_number(state, atoi(cmd + 24));
  if(index != -1){ state->document_checkpoints.compare_checkpoint_index = index; }
  fprintf(out, "checkpoint_compare_with: %s\n", index != -1 ? "ok" : "no such checkpoint");
  debug_channel_wants_animate = true;
 }
 else if(strncmp(cmd, "checkpoint_flip ", 16) == 0)
 {
  state->document_checkpoints.is_flipped_by_debug_channel = (atoi(cmd + 16) != 0);
  fprintf(out, "checkpoint_flip: %d\n", state->document_checkpoints.is_flipped_by_debug_channel);
  debug_channel_wants_animate = true;
 }
 else if(strncmp(cmd, "checkpoint_go_back_to ", 22) == 0)
 {
  i32 index = document_checkpoint_index_from_number(state, atoi(cmd + 22));
  b32 ok = document_checkpoint_go_back_to(state, index);
  fprintf(out, "checkpoint_go_back_to: %s\n", ok ? "ok" : "no such checkpoint");
  debug_channel_wants_animate = true;
 }
 else if(strcmp(cmd, "document_schema_dump") == 0)
 {// NOTE(kv) The type table stored in driver.document.ad, as text (ad_serialize_schema.cpp).
  dump_document_schema_file(out, state);
 }
 else if(strncmp(cmd, "recapture", 9) == 0)
 {
  state->replay.recapture = (atoi(cmd+9) != 0);
  fprintf(out, "recapture: %d\n", state->replay.recapture);
 }
 else if(strncmp(cmd, "display_replay", 14) == 0)
 {
  state->replay.display_replay = (atoi(cmd+14) != 0);
  fprintf(out, "display_replay: %d\n", state->replay.display_replay);
  debug_channel_wants_animate = true;
 }
 else if(strcmp(cmd, "slider_dump") == 0)
 {
  debug_channel_slider_dump(out, state);
 }
 else if(strcmp(cmd, "slider_prune") == 0)
 {// NOTE(kv) Drop orphan rows on both sides and rewrite the values files. Orphans are
  // the safety net for a commented-out slider (ad_serialize_slider_values.cpp) --
  // run this only once the call sites are gone for good (e.g. after an export).
  i32 dropped = 0;
  for_i32(is_driver, 0, 2)
  {
   dropped += state->orphan_slider_rows[is_driver].count;
   state->orphan_slider_rows[is_driver].count = 0;
  }
  b32 ok_game   = save_slider_values_file(state, 0);
  b32 ok_driver = save_slider_values_file(state, 1);
  fprintf(out, "slider_prune: dropped %d orphans; game %s, driver %s\n", dropped,
          ok_game ? "ok" : "FAILED", ok_driver ? "ok" : "FAILED");
 }
 else if(strncmp(cmd, "slider_next_id ", 15) == 0)
 {
  debug_channel_slider_next_id(out, cmd+15);
 }
 else if(strcmp(cmd, "slider_write") == 0)
 {
  b32 ok_game   = save_slider_values_file(state, 0);
  b32 ok_driver = save_slider_values_file(state, 1);
  fprintf(out, "slider_write: game %s, driver %s\n",
          ok_game ? "ok" : "FAILED", ok_driver ? "ok" : "FAILED");
 }
 else if(strncmp(cmd, "slider ", 7) == 0)
 {
  debug_channel_slider_set(out, state, cmd+7);
  debug_channel_wants_animate = true;
 }
 else if(strncmp(cmd, "toggle ", 7) == 0)
 {// NOTE(kv) Preset-rethink step 6: flip a bool on the ACTIVE preset's settings row
  // (viewport 0). Persisted in state.txt. Field names come from the reflection.
  char field[64] = {};
  int value = 0;
  if(sscanf(cmd+7, "%63s %d", field, &value) == 2)
  {
   Preset_Settings &row = state->model.recordings.preset_settings[state->viewports[0].preset];
   b32 *target = 0;
   Type_Info *type = &Type_Info_Preset_Settings;
   i32 mi = find_member_index_by_name(type, SCu8(field));
   if(mi >= 0 and type->members[mi].type == &Type_Info_b32)
   {
    target = cast(b32 *)(cast(u8 *)&row + type->members[mi].offset);
   }
   if(target)
   {
    *target = (value != 0);
    fprintf(out, "toggle %s: %d\n", field, *target);
    debug_channel_wants_animate = true;
   }
   else
   {
    fprintf(out, "error: unknown field %s\n", field);
   }
  }
  else
  {
   fprintf(out, "error: usage: toggle <field> 0|1\n");
  }
 }
 else if(strncmp(cmd, "set ", 4) == 0)
 {// NOTE(kv) i32 fields on the active row: viz_level 0..2, reference_image -1..4.
  char field[64] = {};
  int value = 0;
  if(sscanf(cmd+4, "%63s %d", field, &value) == 2)
  {
   Preset_Settings &row = state->model.recordings.preset_settings[state->viewports[0].preset];
   if(strcmp(field, "viz_level") == 0)
   {
    row.viz_level = value;
    fprintf(out, "set viz_level: %d\n", row.viz_level);
    debug_channel_wants_animate = true;
   }
   else if(strcmp(field, "scene") == 0)
   {// NOTE(kv) Reference scene of the active preset (Reference_Scene enum value).
    row.scene = cast(Reference_Scene)value;
    fprintf(out, "set scene: %d\n", value);
    debug_channel_wants_animate = true;
   }
   else if(strcmp(field, "orthographic") == 0)
   {// NOTE(kv) Global (state.txt), not per-preset.
    state->user_wants_orthographic = (value != 0);
    fprintf(out, "set orthographic: %d\n", state->user_wants_orthographic);
    debug_channel_wants_animate = true;
   }
   else if(strcmp(field, "reference_mode") == 0)
   {// NOTE(kv) Global (state.txt): 0 = drawing on top, 1 = references on top.
    state->reference_mode = cast(Reference_Mode)clamp_between(0, value, 1);
    fprintf(out, "set reference_mode: %d\n", state->reference_mode);
    debug_channel_wants_animate = true;
   }
   else if(strcmp(field, "preset") == 0)
   {
    if(0 <= value and value < state->model.recordings.preset_count)
    {
     game_set_preset(state, 1, value);
     fprintf(out, "set preset: %d\n", value);
     debug_channel_wants_animate = true;
    }
    else
    {
     fprintf(out, "error: preset %d out of range (count %d)\n", value, state->model.recordings.preset_count);
    }
   }
   else if(strcmp(field, "reference_edit") == 0)
   {
    state->reference_edit.active = (value != 0);
    state->reference_edit.drag = Reference_Drag_None;
    // NOTE(kv) Report what the gizmo will find: without a mouse, a missing outline is
    // otherwise indistinguishable from a placement/plane lookup that silently bailed.
    Stringz reference_filename = {};
    Reference_Placement *placement = get_reference_placement(state, &reference_filename);
    Reference_Plane plane = {};
    b32 has_plane = (placement and
                     get_reference_plane(*placement, reference_filename, &plane));
    fprintf(out, "set reference_edit: %d (placement %s, plane %s)\n", value,
            placement ? "found" : "MISSING", has_plane ? "ok" : "FAILED");
    if(has_plane)
    {// NOTE(kv) World-space plane, so these are directly comparable to the drawn quad.
     fprintf(out, "  center=(%.3f %.3f %.3f) u=(%.3f %.3f %.3f) half=(%.3f %.3f)\n",
             plane.center.x, plane.center.y, plane.center.z,
             plane.u_axis.x, plane.u_axis.y, plane.u_axis.z,
             plane.half_u, plane.half_v);
    }
    {// NOTE(kv) The skull's camera-facing square (game_reference_gizmo.cpp), plus its
     // center in png pixels so mouse_down can aim at it.
     Reference_Mesh_Placement *mesh = get_reference_mesh_placement(state);
     Reference_Plane mesh_plane = {};
     Screen_Projection_Data proj = mk_screen_projection_data(state, get_center(debug_channel_mouse_viewport_box));
     b32 has_mesh = (mesh and get_reference_mesh_plane(state, *mesh, proj.camera, &mesh_plane));
     fprintf(out, "  skull: placement %s, plane %s\n",
             mesh ? "found" : "MISSING", has_mesh ? "ok" : "FAILED");
     if(has_mesh)
     {
      fprintf(out, "  skull center=(%.3f %.3f %.3f) radius=%.3f scale=%.3f rotation=(%.3f %.3f %.3f)\n",
              mesh_plane.center.x, mesh_plane.center.y, mesh_plane.center.z,
              mesh_plane.half_u, mesh->scale, mesh->rotation.x, mesh->rotation.y, mesh->rotation.z);
      v2 center_px = project(proj, mesh_plane.center);
      v3 corner = mesh_plane.center + mesh_plane.half_u*mesh_plane.u_axis
                                    + mesh_plane.half_v*mesh_plane.v_axis;
      v2 corner_px = project(proj, corner);
      fprintf(out, "  skull px: center (%.0f %.0f) +u+v corner (%.0f %.0f)\n",
              center_px.x, center_px.y, corner_px.x, corner_px.y);
     }
    }
    debug_channel_wants_animate = true;
   }
   else if(strcmp(field, "reference_image") == 0)
   {
    row.reference_image = value;
    fprintf(out, "set reference_image: %d\n", row.reference_image);
    debug_channel_wants_animate = true;
   }
   else
   {
    fprintf(out, "error: unknown field %s\n", field);
   }
  }
  else
  {
   fprintf(out, "error: usage: set <field> <n>\n");
  }
 }
 else if(strncmp(cmd, "mouse_move ", 11) == 0)
 {
  i32 x, y;
  if(sscanf(cmd+11, "%d %d", &x, &y) == 2)
  {
   debug_channel_mouse_active = true;
   debug_channel_mouse_p = {x, y};
   debug_channel_wants_animate = true;
   fprintf(out, "mouse_move: virtual mouse at (%d %d); query with `hot` next\n", x, y);
  }
  else
  {
   fprintf(out, "error: usage: mouse_move <x> <y>\n");
  }
 }
 else if(strncmp(cmd, "mouse_down ", 11) == 0)
 {
  i32 x, y;
  char mod[16] = {};
  i32 n = sscanf(cmd+11, "%d %d %15s", &x, &y, mod);
  if(n >= 2)
  {
   debug_channel_mouse_active = true;
   debug_channel_mouse_p = {x, y};
   debug_channel_mouse_middle = (n == 3 and strcmp(mod, "middle") == 0);
   debug_channel_mouse_left = not debug_channel_mouse_middle;
   debug_channel_mouse_press_pending = debug_channel_mouse_left;
   debug_channel_mouse_shift = (n == 3 and strcmp(mod, "shift") == 0);
   debug_channel_mouse_alt   = (n == 3 and strcmp(mod, "alt") == 0);
   debug_channel_mouse_ctrl  = (n == 3 and strcmp(mod, "ctrl") == 0);
   debug_channel_wants_animate = true;
   fprintf(out, "mouse_down: at (%d %d) %s\n", x, y, n == 3 ? mod : "");
  }
  else
  {
   fprintf(out, "error: usage: mouse_down <x> <y> [shift|alt|ctrl|middle]\n");
  }
 }
 else if(strncmp(cmd, "make_patch ", 11) == 0)
 {
  i32 idx[4];
  i32 count = sscanf(cmd+11, "%d %d %d %d", &idx[0], &idx[1], &idx[2], &idx[3]);
  if(count >= 2)
  {
   b32 ok = document_make_patch(state, idx, count);
   fprintf(out, "make_patch: %s, document now %d primitives\n", ok ? "ok" : "FAILED (see log)",
           state->model.recordings.document.primitives.count);
   debug_channel_wants_animate = true;
  }
  else { fprintf(out, "error: usage: make_patch <i> <j> [k] [l]\n"); }
 }
 else if(strncmp(cmd, "link ", 5) == 0 or strncmp(cmd, "unlink ", 7) == 0)
 {// NOTE(kv) plan-vertex-links: `link <v> <v> [v...]` / `unlink <v> [v...]`, table indices.
  b32 is_link = (cmd[0] == 'l');
  i32 indices[Document_Vertex_Selection_Cap];
  i32 count = 0;
  char *at = cmd + (is_link ? 5 : 7);
  i32 value, consumed;
  while(count < alen(indices) and sscanf(at, "%d%n", &value, &consumed) == 1)
  {
   indices[count++] = value;
   at += consumed;
  }
  b32 ok = (is_link ? document_link_vertices(state, indices, count)
            : document_unlink_vertices(state, indices, count));
  fprintf(out, "%s: %s\n", is_link ? "link" : "unlink", ok ? "ok" : "FAILED / nothing to do (see log)");
  debug_channel_wants_animate = true;
 }
 else if(strncmp(cmd, "curve_offsets ", 14) == 0)
 {// NOTE(kv) Full-precision endpoints + handle offsets of one curve (%.9g round-trips a
  // float), for "did this curve only translate" asserts.
  Recording &doc = state->model.recordings.document;
  i32 prim_index = -1;
  if(sscanf(cmd+14, "%d", &prim_index) == 1 and prim_index >= 0 and prim_index < doc.primitives.count and
     doc.primitives[prim_index].type == Primitive_Type_Curve)
  {
   Recorded_Primitive &prim = doc.primitives[prim_index];
   for_i32(i, 0, 2)
   {
    v3 p = doc.vertices[prim.vertex_index[i]].p;
    v3 d = prim.curve.handle_offset[i].v;
    fprintf(out, "v%d #%d (%.9g %.9g %.9g)  d%d (%.9g %.9g %.9g)\n",
            i, prim.vertex_index[i], p.x, p.y, p.z, i, d.x, d.y, d.z);
   }
  }
  else { fprintf(out, "error: usage: curve_offsets <curve prim index>\n"); }
 }
 else if(strncmp(cmd, "patch_grid ", 11) == 0)
 {// NOTE(kv) Evaluate a curve patch and print its grid corners/center in window px.
  i32 idx;
  Recording &doc = state->model.recordings.document;
  if(sscanf(cmd+11, "%d", &idx) == 1 and idx >= 0 and idx < doc.primitives.count and
     doc.primitives[idx].type == Primitive_Type_Curve_Patch)
  {
   Scratch_Block tmp;
   Curve_Patch_Grid grid = {};
   if(curve_patch_world_grid(tmp, doc, doc.primitives[idx], false, &grid))
   {
    Screen_Projection_Data proj = mk_screen_projection_data(state, get_center(debug_channel_mouse_viewport_box));
    i32 stride = grid.rows+1;
    auto print_at = [&](char const *name, i32 i, i32 j)
    {
     v3 world = grid.positions[i*stride+j];
     v2 px = project(proj, world);
     fprintf(out, "  %s: world (%.4f %.4f %.4f) px (%.0f %.0f)\n", name, world.x, world.y, world.z, px.x, px.y);
    };
    fprintf(out, "patch_grid %d: %dx%d (left side)\n", idx, grid.columns, grid.rows);
    print_at("u0v0", 0, 0); print_at("u1v0", grid.columns, 0);
    print_at("u0v1", 0, grid.rows); print_at("u1v1", grid.columns, grid.rows);
    print_at("center", grid.columns/2, grid.rows/2);
    {// NOTE(kv) Ray-test the hit triangles at the center px (same math as
     // get_primitive_hit_by_mouse) so the pick path is verifiable even when
     // another fill occludes the patch on screen.
     v3 world = grid.positions[(grid.columns/2)*stride + grid.rows/2];
     v2 px = project(proj, world);
     Screen_Ray ray = screen_ray(proj, px - proj.center);
     darray(Poly3) triangles; init_dynamic(triangles, tmp);
     push_curve_patch_hit_triangles(tmp, &triangles, doc, doc.primitives[idx], false, proj.camera.cam_from_world);
     v1 min_t = INFINITY;
     for_i32(ti, 0, triangles.count)
     {
      v1 t = hit_test_ray_triangle(ray.P, ray.dir, expand3(triangles[ti]));
      if(t < min_t){ min_t = t; }
     }
     fprintf(out, "  hit_test at center: %d triangles, t=%.4f\n", triangles.count, min_t);
    }
   }
   else { fprintf(out, "patch_grid %d: no surface (curves don't chain into a loop)\n", idx); }
  }
  else { fprintf(out, "error: usage: patch_grid <curve patch index>\n"); }
 }
 else if(strncmp(cmd, "delete_patch ", 13) == 0)
 {
  i32 idx;
  if(sscanf(cmd+13, "%d", &idx) == 1)
  {
   b32 ok = document_delete_patch(state, idx);
   fprintf(out, "delete_patch: %s, document now %d primitives\n", ok ? "ok" : "FAILED (see log)",
           state->model.recordings.document.primitives.count);
   debug_channel_wants_animate = true;
  }
  else { fprintf(out, "error: usage: delete_patch <i>\n"); }
 }
 else if(strncmp(cmd, "add_point ", 10) == 0)
 {// NOTE(kv) plan-point-primitive: `add_point Vis_Nose 1:0 x y z` -- tag by name, bone
  // as type:id (what document_dump prints), position in that bone's space.
  char name[64] = {};
  i32 bone_type, bone_index;
  v3 p;
  Group_Vis tag = Vis_None;
  if(sscanf(cmd+10, "%63s %d:%d %f %f %f", name, &bone_type, &bone_index, &p.x, &p.y, &p.z) == 6 and
     (group_vis_from_name(SCu8(name), &tag), tag != Vis_None))
  {
   Bone_ID bone_id = {};
   bone_id.type = (Bone_Type)bone_type;
   bone_id.id   = bone_index;
   i32 prim_index = document_add_point(state, tag, bone_id, p);
   fprintf(out, "add_point: %s, prim %d, document now %d primitives\n",
           prim_index >= 0 ? "ok" : "FAILED (see log)", prim_index,
           state->model.recordings.document.primitives.count);
   debug_channel_wants_animate = true;
  }
  else { fprintf(out, "error: usage: add_point <Vis_Tag> <bone type:id> x y z\n"); }
 }
 else if(strncmp(cmd, "delete_curve ", 13) == 0)
 {
  i32 idx;
  if(sscanf(cmd+13, "%d", &idx) == 1)
  {
   b32 ok = document_delete_curve(state, idx);
   fprintf(out, "delete_curve: %s, document now %d primitives\n", ok ? "ok" : "FAILED (see log)",
           state->model.recordings.document.primitives.count);
   debug_channel_wants_animate = true;
  }
  else { fprintf(out, "error: usage: delete_curve <i>\n"); }
 }
 else if(strncmp(cmd, "select", 6) == 0 and (cmd[6] == ' ' or cmd[6] == 0))
 {// NOTE(kv) `select none` / `select <i> [j ...]` (curves or patches): replaces the
  // selection, like a click plus shift-clicks. `hot` prints it back.
  Document_Selection &sel = state->document_selection;
  sel.count = 0;
  if(strcmp(cmd+6, " none") != 0)
  {
   char const *p = cmd+6;
   i32 idx, read;
   while(sscanf(p, "%d%n", &idx, &read) == 1)
   {
    document_selection_toggle(state, idx);
    p += read;
   }
  }
  fprintf(out, "select:");
  for_i32(i, 0, sel.count){ fprintf(out, " %d", sel.prim_index[i]); }
  fprintf(out, "%s\n", sel.count ? "" : " (empty)");
  debug_channel_wants_animate = true;
 }
 else if(strncmp(cmd, "set_radii ", 10) == 0)
 {// NOTE(kv) plan-focus-radii-midline: the Selection panel's raw v4 without a mouse.
  v4 radii = {};
  if(sscanf(cmd+10, "%f %f %f %f", &radii.x, &radii.y, &radii.z, &radii.w) == 4)
  {
   document_set_radii_begin(state);
   document_set_radii_apply(state, radii);
   document_edit_commit_and_save(state);
   fprintf(out, "set_radii: (%g %g %g %g) on %d selected\n",
           radii.x, radii.y, radii.z, radii.w, state->document_selection.count);
  }
  else
  {
   fprintf(out, "set_radii: usage set_radii x y z w\n");
  }
  debug_channel_wants_animate = true;
 }
 else if(strncmp(cmd, "set_midline ", 12) == 0)
 {
  b32 ok = document_set_midline(state, atoi(cmd+12) != 0);
  fprintf(out, "set_midline: %s\n", ok ? "ok" : "nothing selected");
  debug_channel_wants_animate = true;
 }
 else if(strcmp(cmd, "key delete") == 0)
 {// NOTE(kv) The Delete/Backspace key path without a keyboard.
  b32 ok = document_delete_selection(state);
  fprintf(out, "key delete: %s, document now %d primitives\n", ok ? "ok" : "nothing selected",
          state->model.recordings.document.primitives.count);
  debug_channel_wants_animate = true;
 }
 else if(strncmp(cmd, "nudge ", 6) == 0)
 {// NOTE(kv) plan-keyboard-vertex-move: `nudge <dx> <dy> <dz> [solo]` -- the h/j/k/l path
  // without a keyboard; the delta is in WORLD units (the keys rotate theirs by the camera).
  v3 delta = {};
  char solo_word[16] = {};
  i32 got = sscanf(cmd+6, "%f %f %f %15s", &delta.x, &delta.y, &delta.z, solo_word);
  b32 ok = (got >= 3) and document_keyboard_nudge(state, delta, strcmp(solo_word, "solo") == 0);
  fprintf(out, "nudge: %s, pending %d\n", ok ? "ok" : "refused", state->document_history.pending_nudge);
  debug_channel_wants_animate = true;
 }
 else if(strncmp(cmd, "select_vertex ", 14) == 0)
 {// NOTE(kv) A plain click on table vertex <i>, without having to hit its pixel.
  i32 vertex_index = atoi(cmd+14);
  b32 ok = (vertex_index >= 0 and vertex_index < state->model.recordings.document.vertices.count);
  if(ok)
  {
   state->document_vertex_selection.count = 1;
   state->document_vertex_selection.vertex_index[0] = vertex_index;
   state->document_selection.count = 0;
  }
  fprintf(out, "select_vertex: %s\n", ok ? "ok" : "out of range");
  debug_channel_wants_animate = true;
 }
 else if(strcmp(cmd, "nudge_commit") == 0)
 {
  fprintf(out, "nudge_commit: %s\n", document_keyboard_nudge_commit(state) ? "ok" : "nothing pending");
  debug_channel_wants_animate = true;
 }
 else if(strcmp(cmd, "nudge_cancel") == 0)
 {
  fprintf(out, "nudge_cancel: %s\n", history_nudge_cancel(state) ? "ok" : "nothing pending");
  debug_channel_wants_animate = true;
 }
 else if(strcmp(cmd, "mouse_up") == 0)
 {
  debug_channel_mouse_left = false;
  debug_channel_mouse_shift = false;
  debug_channel_mouse_alt = false;
  debug_channel_mouse_ctrl = false;
  debug_channel_mouse_middle = false;
  debug_channel_mouse_release_pending = true;
  debug_channel_wants_animate = true;
  fprintf(out, "mouse_up\n");
 }
 else if(strncmp(cmd, "line_tool ", 10) == 0)
 {
  i32 on = atoi(cmd+10);
  line_tool_reset(state);
  state->line_tool.armed = (on != 0);
  fprintf(out, "line_tool: %s\n", on ? "armed" : "disarmed");
 }
 else if(strncmp(cmd, "split_arm ", 10) == 0)
 {// NOTE(kv) Enter split mode for a curve headlessly (the menu item does this live). A
  // following mouse_move rides the marker; mouse_down/up commits through the game_main
  // press branch. `split_report` reads back the preview the update computed.
  i32 idx = atoi(cmd+10);
  line_tool_reset(state);
  split_tool_reset(state);
  state->split_tool.armed      = true;
  state->split_tool.prim_index = idx;
  fprintf(out, "split_arm: armed for curve %d\n", idx);
  debug_channel_wants_animate = true;
 }
 else if(strcmp(cmd, "split_report") == 0)
 {
  Split_Tool_State &split = state->split_tool;
  fprintf(out, "split_report: armed %d prim %d on_curve %d preview_valid %d t %.4f\n",
          split.armed, split.prim_index, split.on_curve, split.preview_valid, split.preview_t);
 }
 else if(strncmp(cmd, "coplanarize ", 12) == 0)
 {
  i32 idx = atoi(cmd+12);
  b32 ok = document_coplanarize_once(state, idx);
  fprintf(out, "coplanarize: %s\n", ok ? "ok" : "nothing changed (already planar, not a curve, or ~0 chord)");
  debug_channel_wants_animate = true;
 }
 else if(strncmp(cmd, "split_curve ", 12) == 0)
 {// NOTE(kv) Cut curve <prim> at parameter <t> without the split-mode UI, so the de
  // Casteljau math + history/save path is testable headless. Mirrors the game_main
  // commit branch: split, then commit + save + clear the selection (Q7).
  i32 idx; v1 t;
  Recording &doc = state->model.recordings.document;
  if(sscanf(cmd+12, "%d %f", &idx, &t) == 2)
  {
   b32 ok = document_split_curve(state, idx, t);
   if(ok)
   {
    history_commit(state);
    save_document_file(state);
    state->document_selection.count = 0;
   }
   fprintf(out, "split_curve %d @ %.4f: %s, document now %d primitives / %d vertices\n",
           idx, t, ok ? "ok" : "refused (near an end, not a curve, or bounds a patch)",
           doc.primitives.count, doc.vertices.count);
   debug_channel_wants_animate = true;
  }
  else { fprintf(out, "error: usage: split_curve <primitive index> <t>\n"); }
 }
 else if(strcmp(cmd, "mouse_off") == 0)
 {
  debug_channel_mouse_active = false;
  debug_channel_mouse_left = false;
  debug_channel_mouse_middle = false;
  debug_channel_wants_animate = true;
  fprintf(out, "mouse_off\n");
 }
 else if(strncmp(cmd, "prim_px ", 8) == 0)
 {// NOTE(kv) Same control-point px block as `hot`, for any primitive index (so a test
  // drag can aim at a vertex without hovering it first).
  i32 idx;
  Recording &doc = state->model.recordings.document;
  if(sscanf(cmd+8, "%d", &idx) == 1 and idx >= 0 and idx < doc.primitives.count)
  {
   fprintf(out, "prim_px: document primitive %d\n", idx);
   debug_channel_print_primitive_px(out, state, idx, false);
  }
  else { fprintf(out, "error: usage: prim_px <primitive index>\n"); }
 }
 else if(strcmp(cmd, "hot") == 0)
 {
  {// NOTE(kv) Viewport center + virtual-mouse rel-center, so a live counter-example read
   // off the "Selection" panel (which prints rel-center) can be reproduced: mouse_move to
   // (center + rel) here reproduces the same camera ray regardless of window size.
   v2 center = get_center(debug_channel_mouse_viewport_box);
   v2 rel = V2(debug_channel_mouse_p) - center;
   fprintf(out, "viewport_center (%.0f %.0f)  mouse rel-center (%.0f %.0f)\n",
           center.x, center.y, rel.x, rel.y);
  }
  Location hot = debug_channel_last_hot;
  if(is_document_location(hot))
  {
   fprintf(out, "hot: document primitive %d (%s)\n", document_primitive_index(hot),
           document_location_is_right(hot) ? "right" : "left");
   debug_channel_print_primitive_px(out, state, document_primitive_index(hot), document_location_is_right(hot));
  }
  else if(is_valid(hot))
  {
   fprintf(out, "hot: code file %d:%d range %d..%d\n",
           hot.file.is_driver, hot.file.index, hot.range.min, hot.range.max);
  }
  else
  {
   fprintf(out, "hot: none\n");
  }
  {
   Line_Tool_State &tool = state->line_tool;
   fprintf(out, "line_tool: %s%s", tool.armed ? "armed" : "off", tool.active ? " active" : "");
   if(tool.created)
   {
    fprintf(out, " prim %d group %d start_snap %d path %d plane_cam_z %.4f",
            tool.prim_index, tool.group_index, tool.start_snap, tool.path_count,
            tool.plane_cam_z);
   }
   fprintf(out, "\n");
  }
  {
   Document_Selection &sel = state->document_selection;
   fprintf(out, "selection:");
   for_i32(i, 0, sel.count){ fprintf(out, " %d", sel.prim_index[i]); }
   fprintf(out, "%s\n", sel.count ? "" : " (empty)");
  }
  Document_Mouse_Drag_State &edit = state->document_mouse_drag;
  fprintf(out, "edit: active %d moved %d prim %d %s slot %d %s\n",
          edit.active, edit.moved, edit.pick.prim_index,
          edit.pick.is_right ? "right" : "left", edit.pick.slot,
          edit.pick.is_handle ? "handle" : "vertex");
  {// NOTE(kv) The same label the debug text overlay shows for the hovered control point.
   char hover_label[128];
   if(document_hover_label(hover_label, sizeof(hover_label), state->model.recordings.document) > 0)
   { fprintf(out, "hover: %s\n", hover_label); }
   else { fprintf(out, "hover: none\n"); }
  }
 }
 else if(strncmp(cmd, "set_camera", 10) == 0)
 {// NOTE(kv) Q55: absolute theta/phi on viewport 0 (the main viewport), optionally
  // distance and pivot too. Sets both target AND current camera so the effect is
  // immediate, no animation tail.
  Viewport *viewport = &state->viewports[0];
  Camera_Data camera = viewport->target_camera;
  i32 parsed = sscanf(cmd+10, "%f %f %f %f %f %f", &camera.theta, &camera.phi,
                      &camera.distance, &camera.pivot.x, &camera.pivot.y, &camera.pivot.z);
  if(parsed == 2 || parsed == 3 || parsed == 6)
  {
   viewport->target_camera = camera;
   viewport->camera        = camera;
   fprintf(out, "set_camera: theta=%f phi=%f distance=%f pivot=(%f %f %f)\n",
           camera.theta, camera.phi, camera.distance,
           camera.pivot.x, camera.pivot.y, camera.pivot.z);
   debug_channel_wants_animate = true;
  }
  else
  {
   fprintf(out, "error: usage: set_camera <theta> <phi> [distance [pivot_x pivot_y pivot_z]]\n");
  }
 }
 else
 {
  fprintf(out, "error: unknown command\n");
 }
 fclose(out);
}
