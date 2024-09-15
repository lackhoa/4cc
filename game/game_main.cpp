#define IMGUI_USER_CONFIG "ad_imgui_config.h"
#include "imgui/imgui.h"

#define AD_IS_FRAMEWORK 1
#include "kv.h"
#define AD_IS_GAME 1
#define ED_API_USER 1
#define ED_API_USER_STORE_GLOBAL 1
#define AD_IS_DRIVER 0
#include "4coder_game_shared.h"
#define ED_PARSER_BUFFER 1
#include "4ed_kv_parser.cpp"
#include "ad_stb_parser.cpp"

#include "framework.h"
#include "framework_driver_shared.h"
#include "framework_driver_shared.meta.h"
#define FUI_FAST_PATH 0
#include "game_fui.cpp"
#include "game_api.cpp"

#define DYNAMIC_LINK_API
#include "custom/generated/ed_api.cpp"
#include "game_modeler.cpp"

/*
  IMPORTANT Rule for the renderer
  1. Colors are in linear space (todo precision loss if passed as u32)
 */

#define X(N) function N##_return N(N##_params);
// Note: forward declare
X_GAME_API_FUNCTIONS(X)
//
#undef X



function b32
just_pressed(Game_Input *input, Key_Code keycode, Key_Mods modifiers=0) {
 return ((input->key_states       [keycode])     &&
         (input->key_state_changes[keycode] > 0) &&
         (input->active_mods == modifiers));
}

force_inline b32
key_is_down(Game_Input *input, Key_Code keycode, Key_Mods modifiers=0) {
 return ((input->key_states[keycode]) &&
         (input->active_mods == modifiers));
}

function v4
key_direction_v4(Game_Input *input, Key_Mods wanted_mods, b32 new_keypress,
                 b32 *optional_shift=0) {
 v4 result = {};
 if (new_keypress == input->direction.new_keypress) {
  b32 mods_matched = (input->active_mods == wanted_mods);
  if (optional_shift) {
   *optional_shift = (input->active_mods == (wanted_mods|Key_Mod_Sft));
   if (*optional_shift){
    mods_matched = true;
   }
  }
  if (mods_matched) { result=input->direction.dir; }
 }
 return result;
}

force_inline v3
key_direction(Game_Input *input, Key_Mods wanted_mods, b32 new_keypress) {
 return key_direction_v4(input, wanted_mods, new_keypress).xyz;
}

DLL_EXPORT game_api_export_return 
game_api_export(game_api_export_params) {
 api.is_valid = true;
#define X(N) api.N = N;
 X_GAME_API_FUNCTIONS(X)
#undef X
}

global i32 MAIN_VIEWPORT_INDEX = MAIN_VIEWPORT_ID - 1;

inline i32
get_viewport_index(i32 viewport_id) {
 kv_assert(viewport_id <= GAME_VIEWPORT_COUNT);
 return (viewport_id - 1);
}

force_inline b32
camera_data_equal(Camera_Data *a, Camera_Data *b) {
 return block_match(a, b, sizeof(Camera_Data));
}

function v1
animate_value(v1 start, v1 end, v1 dt, v1 difference_multiplier, v1 min_speed)
{
 macro_clamp_min(min_speed, 0.f);
 v1 abs_difference = absolute(end-start);
 v1 abs_delta = abs_difference * difference_multiplier;
 macro_clamp_min(abs_delta, min_speed*dt);
 macro_clamp_max(abs_delta, abs_difference);
 
 v1 result = (end > start) ? (start+abs_delta) : (start-abs_delta);
 return result;
}

global v1 CAMERA_DISTANCE_STEP         = 5.f  *centimeter;
global v1 CAMERA_PAN_STEP_PER_DISTANCE = 3.75f*centimeter;

function b32
animate_camera(Camera_Data *current, Camera_Data *saved, v1 dt) {
 b32 animation_ended = camera_data_equal(current, saved);
 if ( animation_ended ) {
  saved->theta   = cycle01(saved->theta);
  current->theta = cycle01(current->theta);
 } else {
  
#define ANIMATE(FIELD, MIN_SPEED) \
current->FIELD = animate_value(current->FIELD, saved->FIELD, dt, 0.1f, MIN_SPEED)
  ANIMATE(theta,    0.004f);
  ANIMATE(phi,      0.004f);
  ANIMATE(distance, CAMERA_DISTANCE_STEP/3.0f);
  ANIMATE(pan.x,  CAMERA_PAN_STEP_PER_DISTANCE/3.0f);
  ANIMATE(pan.y,  CAMERA_PAN_STEP_PER_DISTANCE/3.0f);
  ANIMATE(pan.z,  CAMERA_PAN_STEP_PER_DISTANCE/3.0f);
#undef ANIMATE
  
  current->roll  = saved->roll;    // @Hack
  saved->pivot   = current->pivot; // @Hack
 }
 
 return animation_ended;
}

// TODO: Merge this with game_update, come on why are there two calls instead of one?
function game_viewport_update_return
game_viewport_update(game_viewport_update_params)
{
 b32 should_animate_next_frame = false;
 i32 viewport_index = get_viewport_index(viewport_id);
 Viewport *viewport = &state->viewports[viewport_index];
 {// NOTE: camera animtion
  Camera_Data *target  = &viewport->target_camera;
  Camera_Data *current = &viewport->camera;
  b32 animation_ended = animate_camera(current, target, dt);
  if ( !animation_ended ) { should_animate_next_frame = true; }
 }
 
 return should_animate_next_frame;
}

inline v1 
round_to_multiple_of(v1 value, v1 n) {
 v1 result = roundv1(value / n) * n;
 return result;
}

function void
clear_modeler_data(Modeler &m) {
 m.vertices.count = 1;
 m.curves.count   = 1;
}

//-
function void
write_data_func(Printer &p, Type_Info &type, void *void_pointer);

function void
read_enum(Type_Info &type, void *pointer, i32 *dst) {
 kv_assert(type.kind == Type_Kind_Enum);
 *dst = 0;
 block_copy(dst, pointer, type.size);
}

function void
write_data_union(Printer &p, Type_Info &type,
                 void *pointer0, void *pvariant0)
{
 kv_assert(type.kind == Type_Kind_Union);
 u8 *pointer = (u8*)pointer0;
 u8 *pvariant = (u8*)pvariant0;
 
 i32 variant;
 read_enum(*type.discriminator_type, pvariant, &variant);
 
 auto &union_members = type.union_members;
 for_i32(index,0,union_members.count){
  auto &union_member = union_members[index];
  if (union_member.variant == variant) {
   //NOTE(kv) pointer of member is the same as pointer to the union.
   write_data_func(p, *union_member.type, pointer);
   break;
  }
 }
}

function void
write_data_func(Printer &p, Type_Info &type, void *void_pointer)
{
 char newline = '\n';
 u8 *pointer = cast(u8 *)void_pointer;
 switch(type.kind){
  case Type_Kind_Basic:{
   write_basic_type(p, type.Basic_Type, pointer);
  }break;
  
  case Type_Kind_Struct:{
   // NOTE: struct
   p << "{\n";
   for_i1(member_index, 0, type.members.count){
    Struct_Member &member = type.members[member_index];
    p << member.name << " ";
    u8 *member_pointer = pointer+member.offset;
    if(member.type->kind == Type_Kind_Union){
     write_data_union(p, *member.type, member_pointer,
                      (pointer+member.discriminator_offset));
    }else{
     write_data_func(p, *member.type, member_pointer);
    }
    p << newline;
   }
   p << "}\n";
  }break;
  
  case Type_Kind_Union:{
   p<<"<can't write union without variant info>";
  }break;
  
  case Type_Kind_Enum:{
   // NOTE: enum
   i32 enum_value;
   block_copy(&enum_value, pointer, type.size);
   p << enum_value;
  }break;
  
  invalid_default_case;
 }
}
//
#define write_data(PRINTER, POINTER) \
write_data_func(PRINTER, type_info_from_pointer(POINTER), POINTER)

function i32
enum_index_from_pointer(Type_Info &type, void *pointer0)
{
 u8* pointer = (u8*)pointer0;
 i32 value;
 block_copy(&value, pointer, type.size);
 i32 result = {};
 for_i32 (index, 0, type.enum_members.count) {
  Enum_Member enum_it = type.enum_members[index];
  if (enum_it.value == value) {
   result = index;
   break;
  }
 }
 return result;
}
function String
enum_name_from_pointer(Type_Info &type, void *pointer0)
{
 i32 enum_index = enum_index_from_pointer(type, pointer0);
 return type.enum_members[enum_index].name;
}
#define enum_index_from_value(value) \
enum_index_from_pointer(type_info_from_pointer(&value), &value)
#define enum_name_from_value(value) \
enum_name_from_value(type_info_from_pointer(&value), &value)

function void
pretty_print_func(Printer &p, Type_Info &type, void *void_pointer)
{
 char newline = '\n';
 u8 *pointer = cast(u8 *)void_pointer;
 switch(type.kind){
  case Type_Kind_Basic:{
   write_basic_type(p, type.Basic_Type, pointer);
  }break;
  
  case Type_Kind_Struct:{
   p << "{\n";
   for_i1(member_index, 0, type.members.count) {
    Struct_Member &member = type.members[member_index];
    p << member.name << " ";
    u8 *member_pointer = pointer+member.offset;
    pretty_print_func(p, *member.type, member_pointer);
    p << newline;
   }
   p << "}\n";
  }break;
  
  case Type_Kind_Union:{
   p<<"<enum requires knowledge of the variant>";
  }break;
  
  case Type_Kind_Enum:{
   p << enum_name_from_pointer(type, pointer);
  }break;
  
  invalid_default_case;
 }
}
//
#define pretty_print(PRINTER, POINTER) \
pretty_print_func(PRINTER, type_info_from_pointer(POINTER), POINTER)

function b32
game_load(Game_State *state, App *app, String filename,
          b32 ask_confirmation=true)
{// IMPORTANT: This function overwrites edit history.
 b32 ok = true;
 Scratch_Block scratch(app);
 
 if (ask_confirmation) {
  // TODO(kv): Well, I wish confirmation dialog would be *this* easy
  // ok = get_confirmation_from_user(app, strlit("Really reload game? (Will overwrite edit history)"));
 }
 
 String read_string = {};
 if (ok)
 {
  read_string = read_entire_file(scratch, filename);
  ok = read_string.len > 0;
  if ( !ok ) {
   print_message(app, strlit("Game load: can't read the file!\n"));
  }
 }
 
 if( ok )
 {// NOTE: ;deserialize
  Arena *load_arena = &state->data_load_arena;
  arena_clear(load_arena);
  STB_Parser parser = new_parser(read_string, load_arena, 128);
  STB_Parser *p = &parser;
  Data_Reader r = {.parser = p};
  
  {
#define brace_begin  eat_char(p, '{')
#define brace_end    eat_char(p, '}')
#define brace_block  brace_begin; defer( brace_end; )
   eat_id(p, "version");
   r.read_version = cast(Data_Version)eat_i1(p);
   {
    eat_id(p, "cameras");
    {
     brace_block;
     Camera_Data cameras[GAME_VIEWPORT_COUNT];
     for_i32(cam_index,0,GAME_VIEWPORT_COUNT)
     {
      Camera_Data *cam = &state->viewports[cam_index].target_camera;
      brace_block;
#define X(TYPE,NAME)  eat_id(p, #NAME); cam->NAME = eat_##TYPE(p);
      X_Camera_Data(X)
#undef X 
     }
    }
   }
   {
    eat_id(p, "references_full_alpha");
    state->references_full_alpha = eat_i1(p);
   }
   {
    Modeler &m = state->modeler;
    clear_modeler_data(m);
    
    {//-NOTE Vertices
     eat_id(p, "vertices");
     brace_begin;
     while(p->ok_)
     {
      if ( maybe_char(p, '}') ) {
       break;
      }
      Vertex_Data &v = m.vertices.push2();
      read_Vertex_Data(r,v);
     }
    }
    
    {//-NOTE Beziers
     eat_id(p, "beziers");
     brace_begin;
     while(p->ok_)
     {
      if ( maybe_char(p, '}') ) {
       break;
      }
      Bezier_Data &b = m.curves.push2();
      read_Bezier_Data(r,b);
     }
    }
   }
#undef brace_begin
#undef brace_end
#undef brace_block
  }
  
  ok = p->ok_;
  if (!ok) {
   printf_message(app, "Game load: deserialization failed at %d:%d!\n",
                  p->fail_location.line_number, p->fail_location.line_offset);
  }
 }
 
 state->load_failed = !ok; 
 if (ok) { clear_edit_history(state->modeler.history); }
 
 return ok;
}

//~

function void
render_data(Game_State &state)
{
 hl_block;
 painter.is_right = 0;
 auto &m = state.modeler;
 auto &p = painter;
 argb inactive_color = argb_dark_green;
 for_i32(vindex,1,m.vertices.count)
 {
  Vertex_Data &vert = m.vertices[vindex];
  Bone &bone = p.bone_list[vert.bone_index];
  u32 prim_id = vertex_prim_id(vindex);
  {
   mat4 &mat = bone.transform;
   v3 pos = mat4vert(mat, vert.pos);
   indicate_vertex("data", pos, 0, true, inactive_color, prim_id);
  }
  if (vert.symx)
  {//NOTE: Draw the right side (TODO @speed stupid object lookup)
   set_in_block(painter.is_right, 1);
   auto &boneR = get_right_bone(bone);
   mat4 &mat = boneR.transform;
   v3 pos = mat4vert(mat, vert.pos);
   indicate_vertex("data", pos, 0, true, inactive_color, prim_id);
  }
 }
 
 for_i32(curve_index,1,m.curves.count)
 {
  Bezier_Data &curve = m.curves[curve_index];
  auto &bone = p.bone_list[curve.bone_index];
  u32 prim_id = curve_prim_id(curve_index);
  {
   v3 p0 = m.vertices[curve.p0_index].pos;
   v3 p3 = m.vertices[curve.p3_index].pos;
   Bez draw_bez;
   switch(curve.type){
    case Bezier_Type_v3v2:{
     draw_bez = bez(p0, curve.data.v3v2.d0,
                    curve.data.v3v2.d3, p3);
    }break;
    
    invalid_default_case;
   }
   
   {
    draw(bone.transform*draw_bez, prim_id);
   }
   if (curve.symx)
   {
    set_in_block(painter.is_right, 1);
    auto &boneR = get_right_bone(bone);
    draw(boneR.transform*draw_bez, prim_id);
   }
  }
 }
}

function game_render_return
game_render(game_render_params)
{//;switch_game_or_physics
 slider_cycle_counter = 0;
 Scratch_Block scratch;
 {
  Scratch_Block render_scratch;
  // TODO(kv): Why can't the game understand scratch blocks?
  render_movie(scratch, render_scratch,
               state->viewports[viewport_id-1],
               state->time, state->references_full_alpha,
               app, target, viewport_id, mouse); 
 }
 render_data(*state);
}

function game_init_return
game_init(game_init_params)
{// API import
 ed_api_read_vtable(ed_api);
 
 //@game_bootstrap_arena_zero_initialized
 Game_State *state = push_struct(bootstrap_arena, Game_State);
 state->malloc_base_allocator = malloc_base_allocator;  // NOTE(kv): Stupid: can't use global vars on reloaded code!
 state->permanent_arena = *bootstrap_arena;
 Arena *arena = &state->permanent_arena;
 state->dll_arena = make_arena(&state->malloc_base_allocator, MB(1));
 
 {//-;init_modeler
  Modeler &m = state->modeler;
  m.permanent_arena = &state->permanent_arena;
  init_static(m.vertices, m.permanent_arena, 4096);
  init_static(m.curves,   m.permanent_arena, 512);
  {
   init_dynamic(m.active_prims, &state->malloc_base_allocator, 16);
  }
  {
   Modeler_Edit_History &h = m.history;
   // NOTE(kv): We might want to erase edit history, so put it in an arena.
   h.arena     = make_arena(&state->malloc_base_allocator);
   h.inited    = true;
   h.allocator = make_arena_base_allocator(&h.arena);
   clear_edit_history(m.history);
  }
  
  clear_modeler_data(m);
 }
 
 {// NOTE: Save/Load business load_game
  Scratch_Block scratch(app);
  String binary_dir = system_get_path(scratch, SystemPath_BinaryDirectory);
  state->save_dir         = pjoin(arena, binary_dir, "data");
  state->backup_dir       = pjoin(arena, state->save_dir, "backups");
  state->autosave_path    = pjoin(arena, state->save_dir, "text.kv");
  state->manual_save_path = pjoin(arena, state->save_dir, "manual.kv");
  
  {// NOTE: Load state
   state->data_load_arena = make_arena(&state->malloc_base_allocator);
   game_load(state, app, state->autosave_path, false);
  }
 }
 
 for_i32(viewport_index,0,GAME_VIEWPORT_COUNT)
 {//;frame_arena_init
  state->viewports[viewport_index].render_arena = 
   make_arena(&state->malloc_base_allocator);
 }
 
 {
  state->line_cap = 8192;
  state->line_map = push_array(arena, Line_Map_Entry, state->line_cap);
 }
 
 {
  i32 cap = 128;
  state->slow_line_map = {
   .cap = cap,
   .map = push_array(arena, Slow_Line_Map_Entry, cap)
  };
 }
 
 default_fvert_delta_scale = 0.1f;
 
 {//-NOTE: Dear Imgui init
  state->imgui_state = imgui_state;
 }
 
 //-IMPORTANT: Reload is a part of init
 game_reload(state, ed_api, true);
 
 return state;
}

function game_reload_return
game_reload(game_reload_params)
{// Game_State
 // API import
 ed_api_read_vtable(ed_api);
 
 {//NOTE: ;FUI_reload
  dll_arena = &state->dll_arena;
  state->dll_temp_memory = begin_temp(dll_arena);
  // TODO(kv): This trickery means that we do need "static arena".
  push_struct(dll_arena, u8);  // @fui_ensure_nonzero_offset ;fui_ensure_arena_cursor_exists
  
  line_map = state->line_map;
  block_zero(line_map, state->line_cap*sizeof(Line_Map_Entry));
  //-
  slow_line_map       =  state->slow_line_map;
  slow_line_map.count = 0;
 }
 
 {//-NOTE: Modeling data
  global_modeler = &state->modeler;
 }
 
 {//-NOTE: Dear ImGui reload
  IMGUI_CHECKVERSION();
  
  {
   auto &imgui = state->imgui_state;
   ImGui::SetCurrentContext(imgui.ctx);
   ImGui::SetAllocatorFunctions(imgui.alloc_func,
                                imgui.free_func,
                                imgui.user_data);
  }
 }
}

function game_shutdown_return
game_shutdown(game_shutdown_params)
{
 end_temp(state->dll_temp_memory);
}

#include "time.h"

function String
time_format(char *buf, i32 bufsize, char *format)
{
 String result = {};
 time_t rawtime;
 time(&rawtime);
 struct tm *timeinfo = localtime(&rawtime);
 size_t strftime_result = strftime(buf, bufsize, format, timeinfo);
 if (strftime_result != 0)
 {
  result = SCu8(buf);
 }
 return result;
}

inline Camera_Data *
get_target_camera(Game_State *state, i32 index)
{
 return &state->viewports[index].target_camera;
}

function String
serialize_state(Arena *arena, Game_State *state)
{
 // NOTE: lovely MSVC doesn't pass __VA_ARGS__ correctly between macros
 //TODO: Cleanup this macro hell
#define indent                print_nspaces(p, indentation)
 //#define println(VALUE, ...)   print(p, VALUE "\n", ##__VA_ARGS__); indent
#define newline               print(p, "\n"); indent
#define brace_begin           indentation+=1; print(p, "{"); newline;
#define brace_end             indentation-=1; print(p, "}"); newline;
#define brace_block           brace_begin; defer(brace_end);
#define macro_print_field(STRUCT, TYPE, NAME) \
print(p, #NAME " "); write_basic_type(p, Basic_Type_##TYPE, &STRUCT.NAME); newline
 
 const i32 MAX_SAVE_SIZE = KB(16);  // Wastes @Memory
 Printer p = make_printer_buffer(arena, MAX_SAVE_SIZE);
 
 i32 indentation = 0;
 {// NOTE: Content
  {// NOTE: preamble
   print(p, "// autodraw data file (DO NOT EDIT)" ); newline;
   {
    char buf[64];
    String timestamp = time_format(buf, alen(buf), "%d.%m.%Y %H:%M:%S");
    printn2(p, "// Written at ", timestamp); newline;
   }
   printn2(p, "version ", Version_Current); newline;
  }
  {
   print(p, "cameras"); newline;
   {
    brace_block;
    for_i32(camera_index, 0, GAME_VIEWPORT_COUNT)
    {
     brace_block;
     Camera_Data &cam = state->viewports[camera_index].target_camera;
#define X(TYPE,NAME)  macro_print_field(cam, TYPE, NAME);
     X_Camera_Data(X);
#undef X
    }
   }
  }
  newline;
  printn2(p, "references_full_alpha ", state->references_full_alpha); newline;
  newline;
  {// NOTE: Modeler data
   Modeler &modeler = state->modeler;
   {//NOTE vertices
    arrayof<Vertex_Data> &vertices = modeler.vertices;
    print(p, "vertices"); newline;
    {
     brace_block;
     for_i32(vi,1,vertices.count)
     {//NOTE Tight loop
      auto &v = vertices[vi];
      write_data(p, &v);
     }
    }
    newline;
   }
   
   {//NOTE Beziers
    arrayof<Bezier_Data> &curves = modeler.curves;
    print(p, "beziers"); newline;
    {
     brace_block;
     for_i32(bi,1,curves.count)
     {//NOTE tight loop
      auto &b = curves[bi];
      write_data(p, &b);
     }
    }
    newline;
   }
  }
  newline;
  print(p, "//~EOF"); newline;
 }
 
 String result = printer_get_string(p);
 return result;
 
#undef indent
#undef brace_begin
#undef brace_end
#undef brace_block
#undef newline
#undef macro_print_field
}

function b32
game_save(Game_State *state, App *app, b32 is_manual)
{// NOTE: save and backup logic
 Scratch_Block scratch(app);
 String path = (is_manual ? state->manual_save_path :
                state->autosave_path);
 char *pathz = to_cstring(scratch, path);
 String backup_dir = state->backup_dir;
 
 b32 ok = true;
 if (!state->has_done_backup &&
     gb_file_exists(pathz))
 {//-NOTE: Backup situation
  char buf[64];
  String time_string = time_format(buf, alen(buf), "%d_%m_%Y_%H_%M_%S");
  if (time_string.len == 0)
  {
   print_message(app, str8lit("strftime failed... go figure that out!\n"));
   ok = false;
  }
  else
  {
   const char *filename_base = is_manual ? "manual" : "auto";
   String backup_path = push_stringfz(scratch, "%.*s/%s_%.*s.kv",
                                      string_expand(backup_dir),
                                      filename_base,
                                      string_expand(time_string));
   ok = copy_file(path, backup_path, true);
   state->has_done_backup = ok;
  }
  
  if (ok)
  {// NOTE: cycle out old backup files
   // TODO: Maybe treat manual backups differently? idk man!
   File_List backup_files = system_get_file_list(scratch, backup_dir);
   u32 max_backup = 128;
   if (backup_files.count > max_backup)
   {
    u64 oldest_mtime = U64_MAX;
    String file_to_delete = {};
    File_Info **opl = backup_files.infos + backup_files.count;
    for (File_Info **backup = backup_files.infos;
         backup < opl;
         backup++)
    {
     File_Attributes attr = (*backup)->attributes;
     if (attr.last_write_time < oldest_mtime)
     {
      oldest_mtime   = attr.last_write_time;
      file_to_delete = pjoin(scratch, backup_dir, (*backup)->filename);
     }
    }
    b32 delete_ok = remove_file(file_to_delete);
    if (delete_ok) printf_message(app, "INFO: deleted backup file %.*s because it's too old", string_expand(file_to_delete));
    else           printf_message(app, "ERROR: failed to delete backup file %.*s", string_expand(file_to_delete));
   }
  }
 }
 
 if (ok)
 {//-NOTE: Save the file
  String text = serialize_state(scratch, state);
  ok = write_entire_file(path, text.str, text.size);
  if (ok) {vim_set_bottom_text(str8lit("Saved game state!"));}
  else    {printf_message(app, "Failed to write to %.*s", string_expand(path));}
 }
 
 if (!ok) { state->save_failed = true; }
 
 return ok;
}

// NOTE(kv): Can you believe we used to have complicated crap like "distance_level"?
// There is no "distance_level", fool! There's only distance!
inline v1
update_camera_distance(v1 distance, i1 delta_level) {
 const v1 mult = 1.3f;
 distance *= integer_power(mult, delta_level);
 return distance;
}


global arrayof<String> command_queue;

function game_send_command_return
game_send_command(game_send_command_params) {
 state->command_queue.push(command_name);
}

function void
compute_direction_helper(Game_Input *input, Key_Code key_code, i32 component, v1 value)
{
 if (input->key_states[key_code] != 0){
  input->direction.dir.e[component] = value;
  input->direction.new_keypress = (input->key_state_changes[key_code] > 0);
 }
}

function void
update_camera(Camera_Data *cam, Key_Direction key_dir)
{
 if (key_dir.new_keypress) {
  v3 dir = key_dir.dir.xyz;
  {//NOTE(kv) Zoom update
   i1 delta_distance_level = cast(i1)signof(dir.z);
   cam->distance = update_camera_distance(cam->distance, delta_distance_level);
  }
  {//NOTE(kv) Orbit update
   v1 interval = 1.0f / 24.f;
   v1 theta = roundv1(cam->theta / interval);
   v1 phi   = roundv1(cam->phi   / interval);
   {
    v2 delta = dir.xy;
    theta += delta.x;
    phi   += delta.y;  // NOTE: pitch up when we go up
   }
   
   cam->theta = theta * interval;
   cam->phi   = phi   * interval;
   macro_clamp(-0.25f, cam->phi, 0.25f);
  }
 }
}

function void
update_camera_pan(Game_Input *input, Camera_Data *cam)
{
 v1 step = CAMERA_PAN_STEP_PER_DISTANCE * cam->distance;
 v2 delta_pan = key_direction(input, Key_Mod_Alt, true).xy;
 Camera computed_cam; setup_camera(&computed_cam, cam);
 cam->pan += step*(delta_pan.x * computed_cam.x.xyz + 
                   delta_pan.y * computed_cam.y.xyz);
}

function b32
is_v2_key(Key_Code code)
{
 switch(code){
  case Key_Code_L: case Key_Code_H:
  case Key_Code_K: case Key_Code_J:
  return true;
 }
 return false;
}

function b32
is_v3_key(Key_Code code)
{
 switch(code){
  case Key_Code_L: case Key_Code_H:
  case Key_Code_K: case Key_Code_J:
  case Key_Code_O: case Key_Code_I:
  return true;
 }
 return false;
}


// TODO: Input handling: how about we add a callback to look at all the events and report to the game if we would process them or not?
function game_update_return
game_update(game_update_params)
{
 Scratch_Block scratch(app);
 Base_Allocator scratch_allocator = make_arena_base_allocator(scratch);
 
 auto &modeler = state->modeler;
 b32 game_active = active_viewport_id != 0;
 b32 game_or_fui_active = (game_active != 0 || fui_is_active());
 
 b32 should_animate_next_frame = false;
 if (game_or_fui_active) {
  should_animate_next_frame = true;
 }
 
 i32 update_viewport_id = (active_viewport_id ? active_viewport_id : 1);
 kv_assert(active_viewport_id <= GAME_VIEWPORT_COUNT);
 i32 update_viewport_index = update_viewport_id - 1;
 Viewport *update_viewport = &state->viewports[update_viewport_index];
 Camera_Data *update_target_cam = get_target_camera(state, update_viewport_index);
 
 Game_Input input_value = {
  .Game_Input_Public = input_public,
 };
 Game_Input *input = &input_value;
 hot_prim_id = input->frame.hot_prim_id;
 {//NOTE(kv) Compute key direction
  compute_direction_helper(input, Key_Code_L, 0, +1);
  compute_direction_helper(input, Key_Code_H, 0, -1);
  compute_direction_helper(input, Key_Code_K, 1, +1);
  compute_direction_helper(input, Key_Code_J, 1, -1);
  compute_direction_helper(input, Key_Code_O, 2, +1);
  compute_direction_helper(input, Key_Code_I, 2, -1);
  compute_direction_helper(input, Key_Code_Period, 3, +1);
  compute_direction_helper(input, Key_Code_Comma,  3, -1);
 }
 
 //NOTE(kv) Cheesy single keyboard event per-frame,
 // since we're not a fighting game, it'd probably work ok anyway.
 // but it's very dumb because we already had events.
 arrayof<Key_Code> key_strokes;
 {
  init_dynamic(key_strokes, &scratch_allocator);
  for_i32(code, 1, Key_Code_COUNT){
   if (input->key_states[code] &&
       input->key_state_changes[code] > 0){
    key_strokes.push((Key_Code) code);
   }
  }
 }
 
 {//NOTE(kv) Compute active objects
  auto &m = state->modeler;
  m.active_prims.count = 0;
  u32 sel_obj = selected_prim_id(m);
  // NOTE(kv): selected object is always active.
  push_unique(m.active_prims, sel_obj);
  if (m.selection_spanning) {
   Prim_Type sel_type = prim_id_type(sel_obj);
   if (sel_type == Prim_Vertex){
    i32 sel_index = prim_id_to_index(sel_obj);
    Vertex_Data &sel = m.vertices[sel_index];
    for_i32(cindex,1,m.curves.count) {
     Bezier_Data &curve = m.curves[cindex];
     if (sel_index == curve.p0_index) {
      push_unique(m.active_prims, vertex_prim_id(curve.p3_index));
     } else if (sel_index == curve.p3_index) {
      push_unique(m.active_prims, vertex_prim_id(curve.p0_index));
     }
    }
   }
  }
 }
 
#define V2_CASES \
case Key_Code_L: case Key_Code_H: \
case Key_Code_K: case Key_Code_J:
 
#define V3_CASES \
V2_CASES \
case Key_Code_O: case Key_Code_I:
 
 for_i32(key_stroke_index, 0, key_strokes.count){
  //NOTE(kv) Handle keystroke event
  Key_Code key_code = key_strokes[key_stroke_index];
  u32 mods = input->active_mods;
  if (game_active){
   if (mods == 0){
    switch (key_code){
     V2_CASES
     {
      if (state->keyboard_selection_mode){
       //...
      }
      else if (selected_prim_id()){
       //...
      }else{
       update_camera(update_target_cam, input->direction);
      }
     }break;
     
     case Key_Code_O:
     case Key_Code_I:
     {
      update_camera(update_target_cam, input->direction);
     }break;
     
     case Key_Code_Return:{
      if (selected_prim_id()){
       modeler_exit_edit(modeler);
      }else{
       //TODO(kv) Just trigger autosave every 10s or so, and remove this
       game_save(state, app, false);
      }
     }break;
     
     case Key_Code_U:{ modeler_undo(modeler); }break;
     case Key_Code_R:{ modeler_redo(modeler); }break;
     case Key_Code_Escape:{ modeler_exit_edit_undo(modeler); }break;
     
     case Key_Code_S:{
      //TODO(kv) This is not how I envision the flow control, man!
      // The vertex should handle the input.
      if (selecting_vertex(modeler)){
       toggle_boolean(modeler.selection_spanning);
      }
     }break;
     
     case Key_Code_X:{
      update_target_cam->theta *= -1.f;
     }break;
     case Key_Code_Z:{
      update_target_cam->theta = .5f - update_target_cam->theta;
     }break;
    }
   }else if (mods == Key_Mod_Sft){
    switch(key_code){
     case Key_Code_U:{
      game_load(state, app, state->autosave_path);
     }break;
     
     case Key_Code_0: { update_target_cam->roll = {}; }break;
    }
   }else if (mods == Key_Mod_Ctl){
    if (is_v3_key(key_code)){
     update_camera(update_target_cam, input->direction);
    }
   }else if (mods == Key_Mod_Alt){
    switch (key_code){
     V2_CASES {
      update_camera_pan(input, update_target_cam);
     }break;
     case Key_Code_0: { update_target_cam->pan = {}; }break;
    }
   }
  }else if(fui_is_active()){
   if (mods == Key_Mod_Ctl){
    switch(key_code){
     V3_CASES{
      update_camera(update_target_cam, input->direction);
     }break;
    }
   }else if (mods == Key_Mod_Alt){
    switch(key_code){
     V2_CASES{
      update_camera_pan(input, update_target_cam);
     }break;
    }
   }
  }
 }
 
 v4 input_dir = input->direction.dir;
 
 if (input->mouse.press_l){
  // NOTE: Left click handling
  u32 hot_prim = get_hot_prim_id();
  if (hot_prim){
   if (prim_id_is_data(hot_prim)){
    // NOTE: Drawn by data -> change selected obj id
    auto &m = modeler;
    m.selected_prim_id = hot_prim;
    m.change_uncommitted = false;
    switch_to_mouse_panel(app);
   }else{
    // NOTE: Drawn by code -> jump to code
    // NOTE: Don't switch to the game panel, because the cursor should be in the code panel.
    View_ID view = get_active_view(app,0);
    if (!is_view_to_the_right(app, view)){
     //NOTE(kv) Switch to the right view
     view = get_other_primary_view(app, view, Access_Always, true);
    }
    view_set_buffer_named(app, view, GAME_FILE_NAME);
    view_set_cursor(app, view, seek_line_col(hot_prim, 0));
   }
  }
 }
 
 {
  i1 wheel = signof(input->mouse.wheel);  // NOTE(kv): We have WEIRD +/-100 mouse wheel values, dunno whawt that means.
  if (wheel){
   i1 viewport_id = mouse_viewport_id(app);
   if (viewport_id) {
    i1 viewport_index = viewport_id-1;
    Viewport &viewport = state->viewports[viewport_index];
    v1 &distance = viewport.target_camera.distance;
    distance = update_camera_distance(distance, wheel);
   }
  }
 }
 
 if(0){
  i32 selected_index = prim_id_to_index(selected_prim_id(modeler));
  DEBUG_VALUE(selected_index);
 }
 
 v1 dt = input->frame.animation_dt;
 {
  state->time += dt;
  if (state->time >= 1000.0f) { state->time -= 1000.0f; }
 }
 
 Arena *permanent_arena = &state->permanent_arena;
 
 Camera update_camera_value = {};
 Camera *update_cam = &update_camera_value;
 {// NOTE: camera init
  setup_camera(update_cam, update_target_cam);
 }
 
 {//-NOTE: Presets
  if( just_pressed(input, Key_Code_Space) ) {
   game_last_preset(state, update_viewport_id);
  }
  
  if ( just_pressed(input, Key_Code_Q) ) {
   state->references_full_alpha = !state->references_full_alpha;
  }
 }
 
 if ( fui_is_active() )
 {// NOTE: ;UpdateFuislider
  if (just_pressed(input, Key_Code_Tab)) {
   toggle_boolean(fui_v4_zw_active);
  }
  
  Fui_Slider *slider = fui_active_slider;
  if (slider->type == Basic_Type_i1 || 
      slider->type == Basic_Type_i2 ||
      slider->type == Basic_Type_i3 ||
      slider->type == Basic_Type_i4)
  {
   i4 value;
   block_copy(&value, slider+1, slider_value_size(slider));
   v4 direction = key_direction_v4(input, 0, true);
   for_i32(index,0,4) {
    value.e[index] += i32(direction[index]);
   }
   
   if (slider->flags & Slider_Clamp_01) {
    for_i32(index,0,4) {
     macro_clamp01i(value.e[index])
    }
   }
   
   block_copy(slider+1, &value, slider_value_size(slider));
  } else {
   v4 value;
   block_copy(&value, slider+1, slider_value_size(slider));
   
   b32 j_pressed = just_pressed(input, Key_Code_J);
   b32 k_pressed = just_pressed(input, Key_Code_K);
   if (slider->type == Basic_Type_v1 && (j_pressed || k_pressed)) {
    if (j_pressed) { value.x = 0.f; }
    if (k_pressed) { value.x = 1.f; }
   } else {
    b32 shifted;
    v4 direction = key_direction_v4(input, 0, false, &shifted);
    // NOTE: Switch active pair
    if (fui_v4_zw_active &&
        slider->type == Basic_Type_v4) {
     direction.zw = direction.xy;
     direction.xy = {};
    }
    if (slider->flags & Slider_Camera_Aligned) {
     direction = noz( update_cam->forward*direction );
    }
    v1 delta_scale = slider->delta_scale;
    if (delta_scale == 0) { delta_scale = 0.2f; }
    v4 delta = delta_scale * dt * direction;
    if (shifted) { delta *= 10.f; }
    value += delta;
    
    if (slider->flags & Slider_Clamp_X)  {value.x = 0;}
    if (slider->flags & Slider_Clamp_Y)  {value.y = 0;}
    if (slider->flags & Slider_Clamp_Z)  {value.z = 0;}
    
    if (slider->flags & Slider_NOZ) {
     value.xyz = noz(value.xyz);
    }
    
    if (slider->flags & Slider_Clamp_01) {
     for_i32(index,0,4) {
      macro_clamp01(value.v[index])
     }
    }
   }
   
   block_copy(slider+1, &value, slider_value_size(slider));
  }
 }
 
 {
  auto cam = update_target_cam;
  if (input->debug_camera_on) {
   DEBUG_NAME("camera(theta,phi,distance)", V3(cam->theta, cam->phi, cam->distance));
   DEBUG_NAME("camera(pan)", cam->pan);
  }
 }
 
 for_i32(index, 0, GAME_VIEWPORT_COUNT)
 {// NOTE: Set viewport presets to useful values
  Viewport *viewport = &state->viewports[index];
  
  if (viewport->preset == viewport->last_preset)
  {
   if (viewport->preset == 0) { viewport->last_preset = 2; }
   else { viewport->last_preset = 0; }
  }
 }
 
 driver_update(state->viewports);
 
 b32 kb_handled = false;  // TODO(kv): Maybe we automatically set this when you call "just_pressed"?
 
 {//;update_modeler
  auto &m = state->modeler;
  auto &h = m.history;
  
  if ( game_or_fui_active && !kb_handled )
  {// NOTE: Handling input
   Prim_Type sel_type = prim_id_type(selected_prim_id(m));
   if ( sel_type ) {
    i32 sel_index0 = prim_id_to_index(selected_prim_id(m));
    if( sel_type == Prim_Vertex )
    {// NOTE: Selecting a vertex
     Vert_Index sel_index = Vert_Index{sel_index0};
     if( just_pressed(input, Key_Code_S) ) {
      toggle_boolean(modeler.selection_spanning);
     } else {
      v3 direction = key_direction(input, 0, false);
      if (direction != v3{}) {
       // NOTE(kv): Update vertex position
       direction.z = 0.f;
       direction = noz( mat4vec(update_cam->forward, direction) );
       v1 delta_scale = 0.2f;
       v3 delta = delta_scale * dt * direction;
       
       arrayof<Vert_Index> influenced_verts;
       init_static(influenced_verts, scratch, m.active_prims.count);
       for_i32(index,0,m.active_prims.count) {
        Vert_Index vi = vertex_index_from_prim_id(m.active_prims[index]);
        influenced_verts.push(vi);
       }
       
       Modeler_Edit edit = Modeler_Edit{
        .type      = ME_Vert_Move,
        .Vert_Move = {
         .verts=influenced_verts,
         .delta=delta,
        },
       };
       {
        b32 merged = false;
        Modeler_Edit *current_edit = get_current_edit(h);
        // NOTE(kv): edits_can_be_merged is a guardrail for now
        if (current_edit &&
            m.change_uncommitted &&
            edits_can_be_merged(*current_edit, edit))
        {
         merged = true;
         modeler_undo(m);
         edit = *current_edit;
         edit.Vert_Move.delta += delta;
        }
        if (!merged) {
         edit.Vert_Move.verts = influenced_verts.copy(&h.arena);
        }
        apply_new_edit(m, edit);
       }
      }
     }
     DEBUG_VALUE(modeler.selection_spanning);
    }
    else if( sel_type == Prim_Curve )
    {// NOTE: curve update
     Bezier_Data &sel = modeler.curves[sel_index0];
     {
      // TODO @incomplete
      i1 direction = i1( key_direction(input, 0, true).x );
      v1 delta = i2f6(direction);
      if (delta != 0) {
       for_i32(index,0,4) {
        sel.radii[index] += delta;
       }
      }
      DEBUG_VALUE(sel.radii);
     }
    }
   }
  }
  
  {//-NOTE: Drawing GUI
   if ( is_selecting_type(m, Prim_Curve) ) {
    //;draw_curve_info
    ImGui::Begin("curve data", 0);
    Bezier_Data curve = get_selected_curve(m);
    Type_Info type = Type_Info_Bezier_Type;
    i32 curve_type_index = enum_index_from_value(curve.type);
    
    const char* combo_preview = to_cstring(scratch, type.enum_members[curve_type_index].name);
    if ( ImGui::BeginCombo("combo", combo_preview, 0) ) {
     for_i32(enum_index, 0, type.enum_members.count) {
      b32 is_selected = (enum_index == curve_type_index);
      char *name = to_cstring(scratch, type.enum_members[enum_index].name);
      if ( ImGui::Selectable(name, is_selected) ) {
      }
      if (is_selected) {
       ImGui::SetItemDefaultFocus();
      }
     }
     ImGui::EndCombo();
    }
    ImGui::End();
   } else {
    //ImGui::ShowDemoWindow(0);
   }
  }
 }
 
 {
  // TODO: Have a better error reporting story
  // Like, how do we turn these off? With a clear command?
  if (state->load_failed) { DEBUG_TEXT("Load failed!"); }
  if (state->save_failed) { DEBUG_TEXT("Save failed!"); }
 }
 
 {
  if (debug_frame_time_on) {
   DEBUG_NAME("work cycles", input->frame.work_cycles);
   DEBUG_NAME("slider_cycle_counter", slider_cycle_counter);
   DEBUG_NAME("work ms", input->frame.work_seconds * 1e3f);
  }
  
  if ( fbool(0) ) {
   DEBUG_VALUE(image_load_info.image_count);
   DEBUG_VALUE(image_load_info.failure_count);
  }
 }
 
 arrayof<String> game_commands = {};
 {//~ NOTE: Game commands
  {// NOTE: Process commands
   auto &queue = state->command_queue;
   for_i1(ci,0,queue.count)
   {
#define MATCH(NAME)    queue[ci] == strlit(NAME)
    if (0) {}
    else if (MATCH("save_manual")) {
     game_save(state, app, true);
    } else if (MATCH("load_manual")) {
     game_load(state, app, state->manual_save_path);
    } else {
     vim_set_bottom_text(strlit("game: cannot serve command"));
    }
#undef MATCH
   }
   
   queue.count = 0;
  }
  {// NOTE: Fill command lister
   game_commands.count = 0;
   game_commands.push(strlit("save_manual"));
   game_commands.push(strlit("load_manual"));  
  }
 }
 
 return {
  .should_animate_next_frame=should_animate_next_frame,
  .game_commands            =game_commands,
 };
}

//~EOF
