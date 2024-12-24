#pragma once

struct Key_Direction{
 v4  dir;
 b32 new_keypress;
};

struct Game_Input : Game_Input_Public{
 Key_Direction direction;
};

struct Driver_DLL
{
 u64 mtime;
 u32 temp_index;
 DLL_Handle handle;
};
//NOTE: The state is saved between reloads.
struct Game_State
{
 Base_Allocator malloc;
 Arena permanent_arena;
 Arena data_load_arena;  // NOTE: cleared on data load
 Arena dll_arena;        // NOTE: cleared on dll reload
 
 b32 has_done_backup;
 String save_dir;
 String backup_dir;
 Stringz autosave_path;
 Stringz manual_save_path;
 
 //-NOTE: Misc
 Modeler modeler;
 v1 time;
 b32 indicator_level;
 Viewport viewports[GAME_VIEWPORT_COUNT];
 b32 save_failed;
 b32 load_failed;
 darray(String )command_queue;
 Game_ImGui_State imgui_state;
 b32 kb_cursor_mode;
 Serialized_State_Embed;
 Pose pose;//todo(kv) What is this?
 v1 anime_time;
 b32 sending_data;
 darray(String )unsynced_files;
 Driver_API driver_api;
 u32 hot_prim_id;
 Driver_DLL driver_dll;
 
 b8 __padding[64];
};

myinline void print_nspaces(Printer &p, i1 n){ for_repeat(n) { print(p, " "); } }
//-EOF