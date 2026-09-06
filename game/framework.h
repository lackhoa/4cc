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
 // NOTE(kv) Debug-channel knobs live here, not in globals, so a DLL hot reload
 // mid-test doesn't silently reset them.
 b32 recapture;      // Q52: per-frame store_recording gate (default on)
 b32 force_animate;  // keep frames flowing while idle/unfocused
};
enum Reference_Drag_Kind
{
 Reference_Drag_None   = 0,
 Reference_Drag_Body   = 1,
 Reference_Drag_Corner = 2,
};
struct Reference_Edit_State
{// NOTE(kv) Reference edit mode (game_reference_gizmo.cpp): while it's on, the active
 // preset's reference image is draggable and nothing else is pickable.
 b32 active;
 Reference_Drag_Kind drag;
 // NOTE(kv) Drag anchors, in the image quad's own (u,v) frame -- see @Reference_Plane.
 v2 grab_offset;   // body: mouse-to-center offset, held constant for the drag
 v1 grab_u;        // corner: u where the drag started
 v3 grab_x_axis;   // corner: x_axis at drag start
};
struct Document_Pick
{// NOTE(kv) One control point of a document primitive, addressed for editing
 // (game_document_edit.cpp).
 i32 prim_index;
 b32 is_right;      // which mirror pass it was picked on (bones differ)
 b32 is_handle;     // false: table vertex `vertex_index[slot]`; true: curve handle e[slot]
 i32 slot;
};
// NOTE(kv) Shift-click curve selection for "Make patch from selection" (Q8), plus the
// hot document item captured when the right-click menu opened (hot moves with the mouse).
// NOTE(kv) Mouse camera drag (plan-mouse-camera-control.md): left-drag on empty space
// orbits, alt+left-drag pans. Pixels accumulate and whole steps go to the target
// camera, so the existing target->current animation does the smoothing.
struct Camera_Drag
{
 b32 active;
 b32 pan;            // else orbit; decided at press time
 i32 viewport_index;
 v2  last_px;
 v2  remainder_px;   // sub-step drag carried to the next frame
};
struct Document_Selection
{
 i32 count;
 i32 prim_index[4];
 Location menu_hot;
};
struct Document_Edit_State
{// NOTE(kv) A live drag of one document control point (plan-document-mouse-editing).
 b32 active;
 b32 moved;           // anything written since press -> save on release
 Document_Pick pick;
 Location location;   // the hot document location being dragged (stays hot)
 v1 grab_cam_z;       // camera-space depth of the point at press: the drag plane
 v2 grab_offset_px;   // mouse minus projected point at press, held constant
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
 String code_dir;
 String save_dir;
 String backup_dir;
 darray(Slider_Value_Row) orphan_slider_rows[2];  // NOTE(kv) [is_driver]
 u64 slider_values_mtime[2];  // NOTE(kv) [is_driver] mtime of the values file last loaded
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
 Reference_Edit_State reference_edit;
 Document_Edit_State document_edit;
 Document_Selection document_selection;
 Camera_Drag camera_drag;
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