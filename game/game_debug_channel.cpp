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
//   slider_dump       -> every data slider: "<side> <id> <type> <value as code>"
//   slider_write      -> save both values files from the live slider tables
//   slider <id> <n>.. -> set a slider (scalars/vectors by component) and save its file
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
debug_channel_screenshot(FILE *out)
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
debug_channel_slider_dump(FILE *out)
{
 Scratch_Scope tmp;
 for_i32(is_driver, 0, 2)
 {
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
 }
}

function void
debug_channel_slider_set(FILE *out, Game_State *state, char *args)
{// NOTE(kv) `slider <id> <c0> [c1 c2 c3]`: components are floats (ints for i1);
 // wrapper types (tvert/tnormal/tdim) set their wrapped value and keep the rest.
 char id_buffer[64] = {};
 int consumed = 0;
 if(sscanf(args, "%63s%n", id_buffer, &consumed) != 1)
 {
  fprintf(out, "slider: usage: slider <id> <c0> [c1 c2 c3]\n");
  return;
 }
 String id = SCu8(id_buffer);
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
 Type_Info *basic = strip_to_basic_type(slider->type);
 if(is_struct(basic) and not type_info_equals(basic, v2) and
    not type_info_equals(basic, v3) and not type_info_equals(basic, v4))
 {
  fprintf(out, "slider: %s has type %.*s, which this command can't set\n",
          id_buffer, string_expand(slider->type->name));
  return;
 }
 b32 is_int = type_info_equals(basic, i1);
 i32 component_count = basic->size / 4;
 char *cursor = args + consumed;
 i32 parsed = 0;
 for_i32(i, 0, component_count)
 {
  char *end = 0;
  if(is_int)
  {
   long value = strtol(cursor, &end, 10);
   if(end == cursor){ break; }
   ((i32 *)slider->value)[i] = (i32)value;
  }
  else
  {
   float value = strtof(cursor, &end);
   if(end == cursor){ break; }
   ((v1 *)slider->value)[i] = value;
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
debug_channel_update(Game_State *state)
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
 fprintf(out, "ack: %s\n", cmd);

 if(strcmp(cmd, "screenshot") == 0)
 {
  debug_channel_screenshot(out);
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
  debug_channel_slider_dump(out);
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
 {// NOTE(kv) Q55: absolute theta/phi on viewport 0 (the main viewport). Sets both
  // target AND current camera so the effect is immediate, no animation tail.
  float theta = 0, phi = 0;
  if(sscanf(cmd+10, "%f %f", &theta, &phi) == 2)
  {
   Viewport *viewport = &state->viewports[0];
   viewport->target_camera.theta = theta;
   viewport->target_camera.phi   = phi;
   viewport->camera.theta = theta;
   viewport->camera.phi   = phi;
   fprintf(out, "set_camera: theta=%f phi=%f\n", theta, phi);
   debug_channel_wants_animate = true;
  }
  else
  {
   fprintf(out, "error: usage: set_camera <theta> <phi>\n");
  }
 }
 else
 {
  fprintf(out, "error: unknown command\n");
 }
 fclose(out);
}
