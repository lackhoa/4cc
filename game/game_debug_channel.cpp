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
global b32  debug_channel_enabled;
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
// NOTE(kv) How often the agent instance wakes up to poll cmd.txt when nothing animates.
// Every poll runs a full game_update + render (~150 ms at -Od), so 200 ms was ~15% CPU.
#define DEBUG_CHANNEL_POLL_MS 500
// NOTE(kv) Fixed window size in agent mode so screenshot crops land on the same pixels
// across launches (1200x900 outer -> 1174x829 png).
#define DEBUG_CHANNEL_WINDOW_W 1200
#define DEBUG_CHANNEL_WINDOW_H 900

function BOOL CALLBACK
debug_channel_find_own_window(HWND hwnd, LPARAM out_hwnd)
{
 DWORD pid = 0;
 GetWindowThreadProcessId(hwnd, &pid);
 if(pid == GetCurrentProcessId() && IsWindowVisible(hwnd))
 {
  *(HWND *)out_hwnd = hwnd;
  return FALSE;
 }
 return TRUE;
}

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

 HWND window = 0;
 EnumWindows(debug_channel_find_own_window, (LPARAM)&window);
 if(window)
 {
  ShowWindow(window, SW_SHOWNORMAL);  // un-minimize if needed, else a no-op
  SetWindowPos(window, 0, 0, 0, DEBUG_CHANNEL_WINDOW_W, DEBUG_CHANNEL_WINDOW_H,
               SWP_NOMOVE | SWP_NOZORDER);
 }
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
 fprintf(out, "recorded_vertices: %d\n", m->recorded_vertices.count);
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
  fprintf(out, "group %d: parent %d, tag %s, bone %d:%d, loc file %d range %d..%d%s\n",
          igroup, g.parent_index, group_vis_names[g.vis_tag],
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
  fprintf(out, "  params: painting %d, line_color %08x, fill_color %08x, line_depth_offset %g, fill_depth_offset %g, radius_mult %g\n",
          pp.painting, pp.line_color, pp.fill.color, pp.line_depth_offset,
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
    fprintf(out, "curve%s: ", c.straight ? " (straight)" : "");
    for_i32(i, 0, 4){ debug_channel_print_tvert(out, c.bezier.e[i]); fprintf(out, " "); }
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
  fprintf(out, "vertex %d: (%g %g %g) bone %d:%d\n", ivert, v.p.x, v.p.y, v.p.z, v.bone.type, v.bone.id);
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
  fprintf(out, "quit: exiting\n");
 }
 else if(strcmp(cmd, "reload_autosave") == 0)
 {// NOTE(kv) See what the user sees: reload data/autosave.ad (the live instance's
  // periodic save), camera included. Same as the revert command; overwrites this
  // instance's edit history, which an agent instance does not care about.
  b32 ok = game_load(state, app, state->autosave_path);
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
 else if(strncmp(cmd, "export_group ", 13) == 0)
 {// NOTE(kv) `export_group Vis_Nose`: move that tagged region from the live capture
  // into the document (game_document.cpp), weld, save driver.document.ad.
  char const *name = cmd + 13;
  Group_Vis tag = Vis_None;
  for_i32(vis, 1, Group_Vis_Count)
  {
   if(strcmp(name, group_vis_names[vis]) == 0){ tag = cast(Group_Vis)vis; break; }
  }
  if(tag == Vis_None)
  {
   fprintf(out, "export_group: unknown tag '%s'\n", name);
  }
  else
  {
   Document_Export_Result r = export_group_to_document(state, tag);
   fprintf(out, "export_group %s: %s, %d groups, %d primitives, %d vertices (%d welded)\n",
           group_vis_names[tag], r.ok ? "ok" : "FAILED",
           r.group_count, r.primitive_count, r.vertex_count, r.welded_count);
   debug_channel_wants_animate = true;
  }
 }
 else if(strcmp(cmd, "document_dump") == 0)
 {
  debug_channel_document_dump(out, state);
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
  // (viewport 0). Persisted via the settings table in recording.ad.
  char field[64] = {};
  int value = 0;
  if(sscanf(cmd+7, "%63s %d", field, &value) == 2)
  {
   Preset_Settings &row = state->model.recordings.preset_settings[state->viewports[0].preset];
   b32 *target = 0;
#define X(name) else if(strcmp(field, #name) == 0){ target = &row.name; }
   if(0);
   PRESET_BOOL_FIELDS(X)
#undef X
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
   else if(strcmp(field, "reference_preset") == 0)
   {
    state->viewports[0].reference_preset = cast(Reference_Preset)value;
    fprintf(out, "set reference_preset: %d\n", value);
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
