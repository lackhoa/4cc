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
 darray(Location) selected_locations;  // NOTE(kv) document_selection as locations (both sides), drawn in selection_color
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
 // NOTE(kv) The reference skull (@Reference_Mesh_Placement, plan-reference-skull Q7):
 // same body/corner gestures on a camera-facing square around the mesh, plus Shift-drag
 // for yaw/pitch. Roll is a right-click menu item.
 Reference_Drag_Mesh_Body   = 3,
 Reference_Drag_Mesh_Corner = 4,
 Reference_Drag_Mesh_Rotate = 5,
};
struct Reference_Edit_State
{// NOTE(kv) Reference edit mode (game_reference_gizmo.cpp): while it's on, the active
 // preset's reference image and the reference skull are draggable and nothing else is
 // pickable.
 b32 active;
 Reference_Drag_Kind drag;
 // NOTE(kv) Drag anchors, in the grabbed quad's own (u,v) frame -- see @Reference_Plane.
 v2 grab_offset;   // body: mouse-to-center offset, held constant for the drag
 v1 grab_u;        // corner: u where the drag started
 v3 grab_x_axis;   // corner (image): x_axis at drag start
 v1 grab_scale;    // corner (mesh): effective scale at drag start
 v2 grab_px;       // rotate (mesh): mouse pixel where the drag started
 v3 grab_rotation; // rotate (mesh): rotation at drag start
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
 b32 middle;         // started by the middle button (ends on its release, not left's)
 i32 viewport_index;
 v2  last_px;
 v2  remainder_px;   // sub-step drag carried to the next frame
 // NOTE(kv) plan-selection-followups Q1: a plain click on empty space deselects only
 // if it stays a click -- the release checks `moved` (set once the mouse strays
 // camera_drag_tap_px from `press_px`), so an orbit/pan keeps the selection.
 v2  press_px;
 b32 moved;
 b32 deselect_on_tap;  // plain left press (no shift/alt/middle)
};
// NOTE(kv) Agent instance (`-debug-cmd`, game_debug_channel.cpp). Declared here, not
// in the channel file, because document_file_path (ad_serialize_recording.cpp, included
// earlier) picks the agent's own document off it (plan-selection-followups Q3).
global b32 debug_channel_enabled;
global i32 const Document_Selection_Cap = 4;  // at most this many primitives selected at once
struct Document_Selection
{// NOTE(kv) What commands act on (plan-active-primitive-delete-key.md): curves AND
 // patches, by primitive index. Plain click = sole selection, shift-click toggles,
 // Delete/Backspace deletes the lot. Also the input of "Make patch from selection".
 i32 count;
 i32 prim_index[Document_Selection_Cap];
 Location menu_hot;
};
struct Document_Edit_State
{// NOTE(kv) A live drag of one document control point (plan-document-mouse-editing).
 b32 active;
 b32 moved;           // anything written since press -> save on release
 // NOTE(kv) plan-selection-followups Q4 (tablet behavior): a press on an already
 // selected curve away from its control points drags the WHOLE stroke -- both table
 // vertices (with every handle attached to them, on any curve). `pick` then names
 // vertex slot 0 of that curve: the depth/offset reference of the drag.
 b32 whole_stroke;
 Document_Pick pick;
 Location location;   // the hot document location being dragged (stays hot)
 v1 grab_cam_z;       // camera-space depth of the point at press: the drag plane
 v2 grab_offset_px;   // mouse minus projected point at press, held constant
};
// NOTE(kv) Document undo/redo (game_document_history.cpp, plan-document-undo-redo.md).
enum Document_Action_Kind
{
 Document_Action_None = 0,  // entry 0: the state before the first edit
 Document_Action_Move_Vertex,
 Document_Action_Move_Handle,
 Document_Action_Move_Stroke,  // whole curve: both vertices + attached handles
 Document_Action_Make_Patch,
 Document_Action_Delete_Patch,
 Document_Action_Export_Group,
 Document_Action_Add_Line,
 Document_Action_Delete_Curve,
 Document_Action_Delete_Selection,  // Delete key: `count` primitives in `indices`
 // NOTE(kv) plan-focus-radii-midline: Selection panel edits, applied to every selected
 // curve (`count` curves in `indices`, one entry per slider drag / toggle).
 Document_Action_Set_Radii,
 Document_Action_Set_Midline,  // `index` = the new flag value
};
// NOTE(kv) Line tool (game_document_line_tool.cpp, port of tablet line_tool.ts): armed
// from the right-click menu; the next left-drag places one cubic curve on the
// camera-facing plane through the pivot, fitted live to the pen path. Endpoints snap
// to existing table vertices. A click without a drag disarms.
#define LINE_TOOL_PATH_CAP 1024
struct Line_Tool_State
{
 b32 armed;
 b32 active;          // pen is down
 b32 created;         // the curve primitive exists (pushed on the first real move)
 i32 prim_index;      // the curve being drawn (== primitives.count-1 while active)
 i32 group_index;     // group the curve was added to
 i32 start_snap;      // existing vertex index snapped at press, -1 = new vertex
 i32 temp_end_vertex; // table index of the end vertex that follows the pen (last in the table)
 v3 start_world;
 v3 end_world;
 v1 plane_cam_z;      // camera-space depth of the drawing plane (the snapped start vertex's, else the pivot's)
 v2 press_px;
 v3 path[LINE_TOOL_PATH_CAP];  // raw (unsnapped) plane samples, the fit's input
 i32 path_count;
};
struct Document_Action
{// NOTE(kv) Display only: the snapshot is what restores.
 Document_Action_Kind kind;
 i32 prim_index;   // the primitive edited (handle owner, deleted patch)
 i32 index;        // vertex index (Move_Vertex) or handle slot (Move_Handle)
 i32 count;        // Make_Patch: curves in `indices`
 i32 indices[4];
 Group_Vis tag;    // Export_Group
};
struct Document_Snapshot
{// NOTE(kv) The document as it was after one edit; owns its arena.
 Arena arena;
 Recorded_Primitive *primitives;
 Recorded_Group *groups;
 Recorded_Vertex *vertices;
 i32 primitive_count;
 i32 group_count;
 i32 vertex_count;
 Document_Action action;
};
#define DOCUMENT_HISTORY_CAP 100
struct Document_History
{
 Document_Snapshot entries[DOCUMENT_HISTORY_CAP];  // oldest first
 i32 count;
 i32 position;      // entry the document currently equals; -1 when empty
 b32 pending;       // history_begin called, commit/discard not yet
 Document_Action pending_action;
 char status[160];  // "undo: move vertex 12 (nose)", shown on screen for status_frames
 i32 status_frames;
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
 
 String code_dir;
 String save_dir;  // NOTE(kv) state.txt + recording.ad + driver.*.ad live here
 darray(Slider_Value_Row) orphan_slider_rows[2];  // NOTE(kv) [is_driver]
 u64 slider_values_mtime[2];  // NOTE(kv) [is_driver] mtime of the values file last loaded

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
 // NOTE(kv) File present but rejected (version/size/corrupt) -- shown on screen, a
 // silently-dropped driver.document.ad cost a nose (2026-09-06). Missing file != failed.
 b32 recording_load_failed;
 b32 document_load_failed;
 Game_ImGui_State imgui_state;
 Replay_State replay;
 Reference_Edit_State reference_edit;
 // NOTE(kv) Copy of Painter::reference_mesh_obj_radius from the last driver render (the
 // painter only lives during call_driver_render). 0 = the skull has never been drawn.
 v1 reference_mesh_obj_radius;
 v3 reference_mesh_obj_center;
 Document_Edit_State document_edit;
 Document_Selection document_selection;
 Camera_Drag camera_drag;
 Document_History document_history;
 Line_Tool_State line_tool;
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