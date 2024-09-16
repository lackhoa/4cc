#pragma once

global u32 game_save_magic_number;
global u32 game_save_version;

enum Data_Version
{
 Version_Init                 = 7,
 Version_Add_Bezier_Type      = 8,
 Version_Rename_Object_Index  = 9,
 Version_Rename_Object_Index2 = 10,
 Version_Bezier_Revamp        = 11,
 //-
 Version_OPL,
 Version_Inf                 = 0xFFFF,
};
global Data_Version Version_Current = Data_Version(Version_OPL-1);

struct Data_Reader{
 Data_Version read_version;
 STB_Parser *parser;
};

struct Key_Direction{
 v4  dir;
 b32 new_keypress;
};

struct Game_Input{
 Game_Input_Public_Embedding;
 Key_Direction direction;
};

struct Slow_Line_Map {
 i32 cap;
 i32 count;
 struct Slow_Line_Map_Entry *map;
};

//NOTE: The state is saved between reloads.
struct Game_State {
 Base_Allocator malloc_base_allocator;
 Arena permanent_arena;
 Arena data_load_arena;  // NOTE: cleared on data load
 Arena dll_arena;        // NOTE: cleared on dll reload
 Temp_Memory dll_temp_memory;
 
 b32 has_done_backup;
 String save_dir;
 String backup_dir;
 String autosave_path;
 String manual_save_path;
 
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
 b32 keyboard_selection_mode;
 
 b8 __padding[64];
};

// NOTE: ;read_basic_types
#define X(T) \
force_inline void \
read_##T(Data_Reader &r, T &DST)  \
{ DST = eat_##T(r.parser) ; }
//
X_Basic_Types(X)
//
#undef X

//-eof