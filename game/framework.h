#pragma once

struct Key_Direction{
 v4  dir;
 b32 new_keypress;
};

struct Game_Input : Game_Input_Public{
 Key_Direction direction;
};

//NOTE: The state is saved between reloads.
struct Game_State{
 Base_Allocator malloc;
 Arena permanent_arena;
 Arena data_load_arena;  // NOTE: cleared on data load
 Arena dll_arena;        // NOTE: cleared on dll reload
 Temp_Memory dll_temp_memory;
 
 b32 has_done_backup;
 String save_dir;
 String backup_dir;
 Stringz autosave_path;
 Stringz manual_save_path;
 
 //-FUI
 // NOTE: We store things in the state to allow reload (reusing memory).
 // see @FUI_reload
 i32                    line_cap;
 struct Line_Map_Entry *line_map;
 Arena slider_store;
 //-NOTE: Slow Slider Path
 Arena slow_slider_store;
 Slow_Line_Map slow_line_map;
 
 //-NOTE: Misc
 Modeler modeler;
 v1 time;
 b32 indicator_level;
 b32 references_full_alpha;
 Viewport viewports[GAME_VIEWPORT_COUNT];
 b32 save_failed;
 b32 load_failed;
 arrayof<String> command_queue;
 Game_ImGui_State imgui_state;
 b32 kb_cursor_mode;
 Serialized_State_Embed;
 Pose pose;
 v1 anime_time;
 b32 sending_data;
 
 b8 __padding[64];
};

//-EOF