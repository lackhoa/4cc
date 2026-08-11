#pragma once

struct Key_Direction
{
 v4  dir;
 b32 new_keypress;
};

struct Game_Input : Game_Input_0
{
 Key_Direction direction;
};

struct Notebook_State
{
 Texture_Handle texture;
};

struct Game_Transient_State
{// NOTE see @game_reload
 darray(Location) pinned_locations;
 darray(Location) hot_locations;
};
struct Replay_Diff_Result
{// NOTE(kv) Last "Diff now" outcome (game_replay.cpp); shown in the Replay ImGui panel.
 b32 valid;
 b32 match;
 i32 code_vertex_count;
 i32 replay_vertex_count;
 i32 first_diff_vertex;   // -1 when the streams only differ in length
 Location code_location;  // owning push of the first divergent vertex, per stream
 Location replay_location;
};
struct Replay_State
{// NOTE(kv) Draw-as-data step 3 dev UI state (survives DLL reloads via Game_State).
 b32 display_replay;  // rendering mode B: the replay draws the recorded scope
 b32 diff_requested;  // one-shot, consumed by call_driver_render (main viewport)
 Replay_Diff_Result last_diff;
};
struct Game_State
{// NOTE The state that is saved between reloads.
 // NOTE See also @game_init
 Arena permanent_arena;
 Arena data_load_arena;  // NOTE(kv) Cleared on data load
 Arena frame_arena;
 // NOTE(kv) Home of the recording (Model primitives/groups). Cleared per capture run
 // (currently every frame, since the driver still re-records each frame), NOT shared
 // with per-frame scratch -- the recording must outlive frame data once capture is
 // one-time.
 Arena recording_arena;
 //Arena model_frame_arena;
 Arena driver_arena;
 
 b32 has_done_backup;
 String save_dir;
 String backup_dir;
 Stringz autosave_path;
 Stringz manual_save_path;
 
 union
 {
  Serialized_State_Embed;
  Serialized_State serialized;
 };
 
 //-Public state (maybe put it in a different struct)
 darray(Game_Command) command_queue;
 
 //-Misc
 Game_Transient_State *transient;
 Model model;  // NOTE(kv) It's nice to retain some information here.
 b32 sending_data;
 Driver_API driver_api;
 b32 is_dev_editor;
 // NOTE(kv) Source time that is enough to hold the entirety of animation time (plus speedups).
 v1 looping_time;
 b32 indicator_level;
 Viewport viewports[GAME_VIEWPORT_COUNT];
 b32 save_failed;
 b32 load_failed;
 Game_ImGui_State imgui_state;
 Replay_State replay;
};

// TODO(kv) Just hacking around the limitation of update & render being separate
struct Game_Update_Result
{
 Pose pose;
 v1 anim_time;
};
global Game_Update_Result game_update_result;

myinline void print_nspaces(Printer &p, i1 n){ for_repeat(n) { print(p, " "); } }
//-EOF