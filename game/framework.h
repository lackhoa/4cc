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

inline void print_nspaces(Printer &p, i1 n){ for_repeat(n) { print(p, " "); } }

function void
write_basic_type(Printer &p, Basic_Type type, void *value0)
{
 switch(type){
  //-Floats
  case Basic_Type_v1:
  case Basic_Type_v2:
  case Basic_Type_v3:
  case Basic_Type_v4:
  {
   v1 *values = cast(v1*)value0;
   i1 count = i1(get_basic_type_size(type) / 4);
   if (count == 1) {
    print_float_trimmed(p, *values);
   } else {
    for_i32(index,0,count) {
     if (index != 0) { print(p, " "); }
     print_float_trimmed(p, values[index]);
    }
   }
  }break;
  
  //-Integers
  case Basic_Type_i1:
  case Basic_Type_i2:
  case Basic_Type_i3:
  case Basic_Type_i4:
  {
   i1 *v = (i1*)value0;
   i1 count = i1(get_basic_type_size(type) / 4);
   
   for_i32(index,0,count) {
    if (index != 0) { print(p, " "); }
    print(p, v[index]);
   }
  }break;
  
  //-
  case Basic_Type_String: { print(p, *(String*)value0); }break;
  case Basic_Type_u32:    { print(p, *(u32*)value0);    }break;
  
  invalid_default_case;
 }
}



//-EOF