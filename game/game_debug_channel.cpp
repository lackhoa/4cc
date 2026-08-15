// NOTE(kv) File-based debug command channel (see
// ~/notes/tasks/autodraw_draw_as_data/plan-self-debug-visibility.md).
// Gated by the `-debug-cmd` launch arg (off by default). Each frame we poll
// <exe_dir>/debug/cmd.txt; if present: execute the command, write ack/results to
// <exe_dir>/debug/out.txt (overwritten per command), delete cmd.txt.
//
// v1 commands (one per file):
//   screenshot        -> request exe-side capture -> debug/screenshot_<n>.png
//   diff              -> trigger Diff-now, result written on the NEXT update
//   force_animate 0|1 -> set global_debug_force_animate
//   dump_state        -> key counts + replay/diff state as text
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
 fprintf(out, "force_animate: %d\n",  global_debug_force_animate);
 debug_channel_write_diff_result(out, replay.last_diff);
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
  global_debug_force_animate = (atoi(cmd+13) != 0);
  fprintf(out, "force_animate: %d\n", global_debug_force_animate);
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
  global_debug_recapture = (atoi(cmd+9) != 0);
  fprintf(out, "recapture: %d\n", global_debug_recapture);
 }
 else if(strncmp(cmd, "display_replay", 14) == 0)
 {
  state->replay.display_replay = (atoi(cmd+14) != 0);
  fprintf(out, "display_replay: %d\n", state->replay.display_replay);
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
