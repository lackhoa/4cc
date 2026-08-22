//~
#define WANT_TYPE_INFO 1
#define AD_IS_FRAMEWORK 1

#define KV_H_NO_GLOBAL_ARENA_CHUNK_STORE
#define AD_STB_SPRINTF_IMPLEMENTATION 0
#include "kv.h"

#define IMGUI_USER_CONFIG "ad_imgui_config.h"
#include "imgui/imgui.h"

#define AD_IS_GAME 1
#define ED_API_USER 1
#define ED_API_USER_STORE_GLOBAL 1
#define AD_IS_DRIVER 0
#include "4coder_game_shared.h"
#include "runtime_type_info.h"

#include "ad_data.h"
#include "basic_types_read.gen.h"

#include "framework_driver_shared.h"
#include "framework.gen.h"
#include "ad_serialize.h"
#include "game_fui.h"
#include "game_fui_data.gen.h"
#include "game_fui_data.gen.cpp"
#include "framework_fui.h"

#include "framework_draw.cpp"

#include "4ed_kv_parser.cpp"
#include "framework.h"
#include "game_replay.cpp"

#include "framework_fui.cpp"

#define DYNAMIC_LINK_API
#include "ed_api.gen.cpp"

#include "game_commands.cpp"
#include "game_anime.cpp"

#include "ad_serialize.cpp"
#include "meta_all.gen.cpp"

#if NOTEBOOK_MODE
#  include "notebook_main.cpp"
#endif

#include "game_config.gen.cpp"

global const v1 vertex_indicator_radius = 3*millimeter;

//#include "test_image.cpp"
/*
IMPORTANT Rule for the renderer
1. Colors are in linear space (todo precision loss if passed as u32)
*/

#define X(N) function wrap_function(N);
// Note: Forward declare
game_api_xlist(X);
//
#undef X

#define X(N) global wrap_function_pointer(N);
memory_functions_xlist(X);
#undef X

// NOTE(kv) temporary
/*#define fv(value, ...) value
#define fbool fv*/

global v1 default_meter_to_pixel = 4050.6329f;

function b32
just_pressed(Game_Input *input, Key_Code keycode, Key_Mods modifiers=0)
{
 return ((input->key_states       [keycode])     &&
         (input->key_state_changes[keycode] > 0) &&
         (input->active_mods == modifiers));
}
myinline b32
key_is_down(Game_Input *input, Key_Code keycode, Key_Mods modifiers=0)
{
 return ((input->key_states[keycode]) &&
         (input->active_mods == modifiers));
}
function v4
key_direction(Game_Input *input, Key_Mods wanted_mods,
              b32 want_new_keypress, b32 *optional_shift=0)
{
 v4 result = {};
 if(implies(want_new_keypress, input->direction.new_keypress))
 {
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
dll_export void
game_api_export(Game_API *api)
{
 api->is_valid = true;
#define X(N) api->N = N;
 game_api_xlist(X)
#undef X
}

global i32 MAIN_VIEWPORT_INDEX = MAIN_VIEWPORT_ID - 1;

myinline i32
get_viewport_index(Viewport_ID viewport_id)
{
 kv_assert(viewport_id <= GAME_VIEWPORT_COUNT);
 return (viewport_id - 1);
}
myinline b32
camera_data_equal(Camera_Data *a, Camera_Data *b)
{
 return block_match(a, b, sizeof(Camera_Data));
}
function v1
animate_value(v1 start, v1 end, v1 dt, v1 difference_multiplier, v1 min_speed)
{
 ClampBot(min_speed, 0.f);
 v1 abs_difference = absolute(end-start);
 v1 abs_delta = abs_difference * difference_multiplier;
 ClampBot(abs_delta, min_speed*dt);
 ClampTop(abs_delta, abs_difference);
 
 v1 result = (end > start) ? (start+abs_delta) : (start-abs_delta);
 return result;
}

global Vertices global_vertices_p;

global v1 CAMERA_DISTANCE_STEP         = 5.f * centimeter;
global v1 CAMERA_PAN_STEP_PER_DISTANCE = 2.f * centimeter;

function b32
animate_camera(Camera_Data *current, Camera_Data *saved, v1 dt)
{
 b32 animation_ended = camera_data_equal(current, saved);
 if(animation_ended)
 {
  saved->phi   = cycle01(saved->phi);
  current->phi = cycle01(current->phi);
 }
 else
 {
#define ANIMATE(FIELD, MIN_SPEED) \
current->FIELD = animate_value(current->FIELD, saved->FIELD, dt, 0.15f, MIN_SPEED)
  ANIMATE(phi,    0.004f);
  ANIMATE(theta,  0.004f);
  ANIMATE(distance, CAMERA_DISTANCE_STEP/3.0f);
  ANIMATE(pivot.x,  CAMERA_PAN_STEP_PER_DISTANCE/3.0f);
  ANIMATE(pivot.y,  CAMERA_PAN_STEP_PER_DISTANCE/3.0f);
  ANIMATE(pivot.z,  CAMERA_PAN_STEP_PER_DISTANCE/3.0f);
#undef ANIMATE
  
  current->roll = saved->roll; // #Hack
 }
 
 return animation_ended;
}

inline v1
round_to_multiple_of(v1 value, v1 n) {
 v1 result = roundv1(value / n) * n;
 return result;
}
//-
function void
print_data_func(Printer &p, Type_Info *type, void *void_pointer);

function void
print_data_union(Printer &p, Type_Info *type,
                 void *pointer0, void *pvariant0)
{
 kv_assert(type->kind == I_Type_Kind_Union);
 u8 *pointer = (u8*)pointer0;
 u8 *pvariant = (u8*)pvariant0;
 
 i32 variant = read_enum(*type->discriminator_type, pvariant);
 
 auto &union_members = type->union_members;
 for_i32(index,0,union_members.count){
  auto &union_member = union_members[index];
  if (union_member.variant == variant) {
   //NOTE(kv) pointer of member is the same as pointer to the union.
   print_data_func(p, union_member.type, pointer);
   break;
  }
 }
}
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
  
  InvalidDefaultCase;
 }
}
function void
print_data_func(Printer &p, Type_Info *type, void *void_pointer)
{
 char newline = '\n';
 u8 *pointer = cast(u8 *)void_pointer;
 switch(type->kind){
  case I_Type_Kind_Basic:{
   write_basic_type(p, type->Basic_Type, pointer);
  }break;
  case I_Type_Kind_Struct:{
   p < "{\n";
   for_i32(member_index, 0, type->members.count){
    I_Struct_Member &member = type->members[member_index];
    if(!member.unserialized){
     p < member.name < " ";
     u8 *member_pointer = pointer+member.offset;
     if(member.type->kind == I_Type_Kind_Union){
      print_data_union(p, member.type, member_pointer,
                       pointer+member.discriminator_offset);
     }else{
      print_data_func(p, member.type, member_pointer);
     }
     p<newline;
    }
   }
   p < "}\n";
  }break;
  case I_Type_Kind_Union:{
   p < "<can't write union without variant info>";
  }break;
  case I_Type_Kind_Array:{
   Type_Info *item_type = type->array_item_type;
   p<"{\n";
   for_i32(item_index,0,type->count){
    print_data_func(p, item_type, pointer + item_type->size*item_index);
    p < newline;
   }
   p < "}\n";
  }break;
  case I_Type_Kind_Enum:{
   i32 enum_value;
   block_copy(&enum_value, pointer, type->size);
   p < enum_value;
  }break;
  InvalidDefaultCase;
 }
}
#define print_data(PRINTER, POINTER) \
print_data_func(PRINTER, &type_info_from_pointer(POINTER), POINTER)
//~
function i32
enum_index_from_pointer(Type_Info *type, void *pointer0)
{
 u8* pointer = (u8*)pointer0;
 i32 value;
 block_copy(&value, pointer, type->size);
 i32 result = {};
 for_i32(index, 0, type->enum_members.count) {
  I_Enum_Member enum_it = type->enum_members[index];
  if (enum_it.value == value) {
   result = index;
   break;
  }
 }
 return result;
}
function String
enum_name_from_pointer(Type_Info *type, void *pointer0)
{
 i32 enum_index = enum_index_from_pointer(type, pointer0);
 return type->enum_members[enum_index].name;
}
#define enum_index_from_value(value) \
enum_index_from_pointer(type_info_from_pointer(&value), &value)

#define enum_name_from_value(value) \
enum_name_from_value(type_info_from_pointer(&value), &value)

function void
pretty_print_func(Printer &p, Type_Info *type, void *void_pointer)
{
 char newline = '\n';
 u8 *pointer = cast(u8 *)void_pointer;
 switch(type->kind)
 {
  case I_Type_Kind_Basic:{
   write_basic_type(p, type->Basic_Type, pointer);
  }break;
  
  case I_Type_Kind_Struct:{
   p << "{\n";
   for_i32(member_index, 0, type->members.count) {
    I_Struct_Member &member = type->members[member_index];
    p << member.name << " ";
    u8 *member_pointer = pointer+member.offset;
    pretty_print_func(p, member.type, member_pointer);
    p << newline;
   }
   p << "}\n";
  }break;
  
  case I_Type_Kind_Union:{
   p<<"<enum requires knowledge of the variant>";
  }break;
  
  case I_Type_Kind_Enum:{
   p << enum_name_from_pointer(type, pointer);
  }break;
  
  InvalidDefaultCase;
 }
}
#define pretty_print(PRINTER, POINTER) \
pretty_print_func(PRINTER, type_info_from_pointer(POINTER), POINTER)

myinline Camera_Data *
get_target_camera(Game_State *state, i32 viewport_index)
{
 return &state->viewports[viewport_index].target_camera;
}
function void
read_debug_string(Binary_Reader *r, Stringz string)
{
 usize size = string.size+1;
 if(r->end_pos - r->pos < isize(size)){
  r->ok = false;
 }
 if(r->ok){
  r->ok = block_match(r->pos, string.str, size);
  r->pos += size;
 }
}
#include "ad_serialize_recording.cpp"
#include "ad_serialize_slider_values.cpp"

function b32
game_load(Game_State *state, App *app, Stringz filename)
{// IMPORTANT(kv) This function overwrites edit history.
 b32 ok = true;
 
 Arena *load_arena = &state->data_load_arena;
 arena_free(load_arena);
 
 String file_data = {};
 {//NOTE(kv) Read the whole file into memory, because we won't have large files.
  //  Plus it makes string handling more convenient.
  file_data = read_entire_file(load_arena, filename);
  ok = file_data.len > 0;
  if(not ok){
   log_error(strlit("Game load: can't read the file!"));
  }
 }
 
 if(ok)
 {//-;deserialize
  Binary_Reader reader = make_binary_reader(file_data.data, file_data.size);
  Binary_Reader *r = &reader;
  
  {
   u32 magic = read_binary_u32(r);
   if(magic != autodraw_data_magic){
    r->ok = false;
   }
   r->read_version = read_binary_u32(r);
   log_string("read version: %u", r->read_version);
   u64 timestamp = read_binary_u64(r);
   
   if(r->read_version < Version_AddViewport)
   {
    read_debug_string(r, strlit("cameras"));
    
    i32 camera_count;
    read_binary_i1(r, &camera_count);
    ClampTop(camera_count, GAME_VIEWPORT_COUNT);
    
    for_i32(cam_index, 0, camera_count)
    {
     Camera_Data *cam = &state->viewports[cam_index].target_camera;
     read_binary_Camera_Data(r, cam);
    }
   }
   
   {
    read_debug_string(r, strlit("Serialized_State"));
    read_binary_Serialized_State(r, &state->serialized);
    if(r->read_version >= Version_AddViewport)
    {
     for_i32(viewport_index, 0, GAME_VIEWPORT_COUNT)
     {
      Saved_Viewport &saved = state->serialized.saved_viewports[viewport_index];
      state->viewports[viewport_index].saved = saved;
     }
    }
   }
   
   read_debug_string(r, strlit("EOF"));
  }
  
  ok = r->ok;
  if(!ok){
   log_error(strlit("Game load: deserialization failed"));
  }
 }
 
 if(ok){
  log_string(strlit("Game load succeeded"));
 }
 
 state->load_failed = !ok;
 return ok;
}
function void
revert_from_autosave(Game_State *state, App *app){
 game_load(state, app, state->autosave_path);
}
//~
function Camera
setup_camera(Camera_Data const &data)
{
 Camera camera = {};
 
 camera.near_clip    = 1*centimeter;
 camera.far_clip     = 20.f;
 camera.focal_length = tweaks->focal_length;
 camera.world_from_camera = (mat4i_rotate_tpr(data.phi, data.theta, data.roll, data.pivot) *
                             mat4i_translate(data.pivot+V3z(data.distance)));
 return camera;
}
function mat4
get_clip_from_camera(Camera const &camera, v2 clip_radius, b32 orthographic)
{// NOTE(kv) We call this "clip space" by D3D terminology, opengl is probably the same
 // https://learn.microsoft.com/en-us/windows/win32/dxtecharts/the-direct3d-transformation-pipeline
 
 // NOTE(kv) Revserse z, to get the depth
 mat4 result = mat4{{
   1,0, 0,0,
   0,1, 0,0,
   0,0,-1,0,
   0,0, 0,1,
  }};
 
 v1 focal = camera.focal_length;
 v1 n = camera.near_clip;
 v1 f = camera.far_clip;
 
 v1 a = focal/clip_radius.x;
 v1 b = focal/clip_radius.y;
 if (orthographic)
 {//-View all objects as if they're at the origin
  // NOTE(kv) We normalize the z dimension here, why?
  v1 d = lengthof(camera_world_position(camera));
  mat4 ortho = {{
    a,0,0,0,
    0,b,0,0,
    0,0,2*d/(f-n), -d*(f+n)/(f-n),
    0,0,0,d,
   }};
  result = ortho*result;
 }
 else
 {//-Perspective
  mat4 perspectiveT = {{
    a,     0,  0,            0,
    0,     b,  0,            0,
    0,     0,  (n+f)/(f-n), -2*f*n/(f-n),
    0,     0,  1,            0,
   }};
  result = perspectiveT*result;
 }
 return result;
}
function mat4
get_clip_from_world(Camera const &camera, v2 clip_radius, b32 orthographic)
{
 mat4 clip_from_cam = get_clip_from_camera(camera, clip_radius, orthographic);
 mat4 result = clip_from_cam * camera.cam_from_world;
 return result;
}
function void
convert_primitives_to_camera_space(Camera &camera)
{
 Model *m = the_model;
 if(not m->primitives_are_in_camera_space)
 {
  m->primitives_are_in_camera_space = 1;
  
  Bone_ID cur_bone = mk_bone_id(Bone_Invalid);
  mat4 camera_from_bone = {};
  mat4 camera_from_world = camera.cam_from_world;
  
  auto update_current_bone = [&](Bone_ID new_bone_id) -> void
  {
   if(new_bone_id != cur_bone)
   {
    // NOTE(kv) We only send primitives on the left (ref @should_send_model_data)
    Bone *bone = get_bone(new_bone_id, /*left*/0);
    cur_bone = bone->id;
    camera_from_bone = matmul(camera_from_world, bone->world_from_bone);
   }
  };
  
  update_current_bone(cur_bone);
  
  for_i32(vi, 0, m->vertices.count)
  {
   Vertex &vertex = m->vertices[vi];
   update_current_bone(vertex.bone_id);
   mat4vert(camera_from_bone, &vertex.pos);
  }
  
  // NOTE(kv) Transform a copy into camera_primitives -- the recording itself
  // stays bone-space (it is the source of truth, never mutated by a camera).
  init_dynamic(m->camera_primitives, m->frame_arena, m->primitives.count);
  for_i32(iprim, 0, m->primitives.count)
  {
   Recorded_Primitive primitive = m->primitives[iprim];
   apply_shape_key(primitive);  // NOTE(kv) picking/hot-test against what's on screen
   update_current_bone(m->groups[primitive.group_index].bone_id);
   switch(primitive.type)
   {
    case Primitive_Type_Curve:
    {
     mat4bez(camera_from_bone, &primitive.curve.bezier);
    }break;

    case Primitive_Type_Poly3:
    {
     for_i32(i,0,3)
     {
      mat4vert(camera_from_bone, &primitive.poly3[i]);
     }
    }break;

    case Primitive_Type_Dual_Bezier:
    {
     mat4bez(camera_from_bone, &primitive.dual_bezier.P);
     mat4bez(camera_from_bone, &primitive.dual_bezier.Q);
    }break;

    case Primitive_Type_Patch:
    {
     for_i32(i,0,4)
     {
      for_i32(j,0,4)
      {
       mat4vert(camera_from_bone, &primitive.patch.e[i][j].v);
      }
     }
    }break;

    case Primitive_Type_Disk:
    {
     mat4vert(camera_from_bone, &primitive.disk.center);
    }break;
   }
   push(&m->camera_primitives, primitive);
  }
 }
}
function void
call_driver_render(Game_State *state, App *app, Render_Target *target,
                   i32 viewport_id, Mouse_State mouse, v2 clip_radius)
{
 Driver_API *driver = &state->driver_api;
 if(is_valid(driver))
 {// ;init_painter
  Painter painter_value = {};
  painter = &painter_value;
  draw_cycle_counter = 0;
  slider_cycle_counter = 0;
  
  Scratch_Block tmp;
  Viewport *viewport = &state->viewports[viewport_id-1];
  
  Camera camera = setup_camera(viewport->camera);
  
  painter->looping_time = state->looping_time;
  painter->anim_time    = game_update_result.anim_time;
  
  painter->show_grid = state->model.recordings.preset_settings[viewport->preset].show_grid;
  {
   b32 camera_frontal = almost_equal(absolute(camera.z.z), 1.f, 1e-2f);
   b32 camera_profile = almost_equal(absolute(camera.z.x), 1.f, 1e-2f);
   b32 orthographic = painter->show_grid and (camera_frontal or camera_profile);
   painter->clip_from_world = get_clip_from_world(camera, clip_radius, orthographic);
  }
  painter->target       = target;
  painter->camera       = camera;
  painter->sending_data = state->sending_data;
  painter->references_full_alpha = state->references_full_alpha;
  painter->hot_locations = state->transient->hot_locations;
  painter->viewport = viewport;
  {
   v4 background_v4;
   background_v4.rgb = tweaks->background_rgb;
   background_v4.a   = 1;
   painter->background_color = argb_pack(background_v4);
  }
  Render_Config *config = draw_new_group(target);
  {
   set_y_up(target, config);
   config->meter_to_pixel  = default_meter_to_pixel;
   config->viewport_id     = viewport->index+1;
   config->clip_from_world = painter->clip_from_world;
   config->world_from_cam  = camera.world_from_camera;
   config->focal_length    = camera.focal_length;
   config->near_clip       = camera.near_clip;
   config->far_clip        = camera.far_clip;
   config->background      = painter->background_color;
  }
  push_view_vector(tvert());
  
  {//-Drawing the movie (+ replay, draw-as-data step 3)
   Replay_State &replay = state->replay;
   b32 do_diff = replay.diff_requested and viewport_id == 1;

   if(viewport_id == 1)
   {// NOTE(kv) Two editor panels can show the same viewport, so this renders more
    // than once per frame -- without the reset the recording accumulates one
    // bitwise-identical copy per pass (caught by Diff-now: replay = 2x code).
    reset_capture();
   }

   Vertex_Tee tee_code = {};
   if(do_diff)
   {// NOTE(kv) Buffer A: the code path's vertex stream (rendered normally).
    init_vertex_tee(&tee_code, tmp);
    global_vertex_tee = &tee_code;
   }
   // NOTE(kv) Mode B mute only applies to the code path's recorded scope --
   // scoped tightly around driver_render so cursor/indicator drawing stays live.
   global_replay_display = replay.display_replay;
   driver->driver_render(tmp, painter);
   global_replay_display = false;
   global_vertex_tee = 0;

   if(viewport_id == 1 and replay.recapture)
   {// NOTE(kv) Q36: snapshot this frame's capture into the one recording.
    // Mode B doesn't stop the capture (mute suppresses rendering, not recording),
    // so it stays fresh either way (unless the Q52 recapture gate is off).
    store_recording();
   }

   Recording &replay_rec = the_model->recordings.recording;
   if(viewport_id == 1 and replay_rec.captured
      and (do_diff or replay.display_replay))
   {
    Vertex_Tee tee_replay = {};
    if(do_diff)
    {// NOTE(kv) Buffer B: the replay's vertex stream.
     init_vertex_tee(&tee_replay, tmp);
     global_vertex_tee = &tee_replay;
    }
    // NOTE(kv) In mode A the replay is diff-only: mute its pushes so it doesn't
    // draw over the code path (Q24). In mode B the replay IS the display.
    global_rendering_suppressed = not replay.display_replay;
    replay_recording(replay_rec);
    global_rendering_suppressed = false;
    global_vertex_tee = 0;

    if(do_diff)
    {
     replay.last_diff = diff_vertex_tees(&tee_code, &tee_replay);
     replay.diff_requested = false;
    }
   }
  }

  if(viewport_id == 1)
  {// NOTE Highlighted vertices
   convert_primitives_to_camera_space(camera);
   v3 cursor_camera = mat4vert(camera.cam_from_world, state->kb_cursor.pos);
   Bone *camera_bone = make_bone(mk_bone_id(Bone_Camera), camera.world_from_camera);
   BoneBlock(camera_bone->id);
   
   sarray(Vertex) vertex_array = the_model->vertices;
   for_i32(array_index, 0, 1)
   {
    /*if(array_index == 1)
    {
     vertex_array = the_model->persistent.vertices;
    }*/
    
    for_i32(vi, 0, vertex_array.count)
    {// NOTE(kv) Having to loop through vertices here because
     // there are vertices that weren't submitted while rendering.
     Vertex &vertex = vertex_array[vi];
     Vertex_Info info = get_vertex_info(vertex);
     argb color = linear_argb_yellow;
     v3 pos = vertex.pos;
     {// TODO(kv) I'm NOT happy with overlays, I'd rather just have depth offset.
      b32 draw_all_near_cursor = false;
      b32 cursor_near = false;
      if(draw_all_near_cursor)
      {
       v1 cursor_dist = length_squared(pos - cursor_camera);
       cursor_near = cursor_dist < squared(3*centimeter);
      }
      
      set_draw_location(info.location);
      b32 is_hot = current_location_is_hot();
      
      b32 should_draw = (is_hot or
                         painter->viz_level >= info.indicator_level or
                         cursor_near);
      if(should_draw)
      {// NOTE Draw
       Paint_Params &cparams = painter->params;
       v1 depth_offset = cparams.line_depth_offset - 1*centimeter;
       Poly_Flags flags = {};
       // NOTE: If lines are overlayed, so are indicators
       if(info.overlay or is_hot)
       {
        flags.v |= Poly_Overlay;
       }
       
       if(is_hot)
       {
        color = (color == hot_color) ? hot_color2 : hot_color;
       }
       fill_disk_camera_space(pos, vertex_indicator_radius,
                              color, flags);
      }
      clear_draw_location();
     }
    }// vertex loop
   }// array loop
  }
  
  i32 active_viewport_id = get_active_game_viewport_id(app);
  if(viewport_id == 1 and
     active_viewport_id == 1 and
     state->kb_cursor.on)
  {// NOTE(kv) ;draw_cursor
   v1 cursor_dist = lengthof(mat4vert(camera.cam_from_world, state->kb_cursor.pos));
   v1 radius = 4*millimeter;
   radius *= cursor_dist / camera.focal_length;
   if(0){ radius *= 10.f; }
   v3 points[3] = { v3{}, V3(-1,-1,0), V3(+1,-1,0), };
   for_i32(i,0,3)
   {
    points[i] = (state->kb_cursor.pos +
                 radius*(points[i].x*camera.x +
                         points[i].y*camera.y));
   }
   poly3_inner(mk_poly3(points), repeat3(linear_argb_blue), {Poly_Overlay});
  }
  
  if(state->is_dev_editor)
  {
   im_text("Render cycles: %_$I64u", painter->render_cycles);
   im_text("Clipped curve / total: %d / %d",
           painter->clipped_curve_count, painter->total_curve_count);
  }
 }
 painter = 0;
}
function void
import_api_from_editor(API_VTable_ed *ed_api, API_VTable_ed_new *ed_api_new)
{
 ed_api_read_vtable(ed_api);
 ed_api_read_vtable_new(ed_api_new);
}
function Game_State *
game_init(Arena *bootstrap_arena, API_VTable_ed *ed_api, API_VTable_ed_new *ed_api_new,
          App *app, Game_ImGui_State &imgui_state, b32 is_dev_editor)
{
 import_api_from_editor(ed_api, ed_api_new);
 Game_State *state = push_struct0(bootstrap_arena, Game_State);
 state->is_dev_editor   = is_dev_editor;
 state->permanent_arena = *bootstrap_arena;
 state->replay.recapture = true;
 thread_permanent_arena = make_arena(MB(1));
 
 {// NOTE: Save/Load business load_game
  Arena *arena = &state->permanent_arena;
  String code_dir = get_code_directory(app);
  state->code_dir         = push_string(arena, code_dir);
  state->save_dir         = pjoin(arena, code_dir, strlit("data"));
  state->backup_dir       = pjoin(arena, state->save_dir, strlit("backups"));
  state->autosave_path    = pjoin(arena, state->save_dir, strlit("autosave.ad"));
  state->manual_save_path = pjoin(arena, state->save_dir, strlit("manual.ad"));
  
  {// NOTE: Load state
   state->data_load_arena = make_arena();
   seed_preset_settings(state->model.recordings.preset_settings);
   game_load(state, app, state->autosave_path);
   load_recording_file(state);
  }
 }
 
 for_i32(viewport_index,0,GAME_VIEWPORT_COUNT)
 {// ;frame_arena_init
  Viewport *viewport = &state->viewports[viewport_index];
  viewport->render_arena = make_arena();
  viewport->index = viewport_index;
 }
 
 //-NOTE: Dear Imgui init
 state->imgui_state = imgui_state;
 
 // NOTE(kv) IMPORTANT: Reload is a part of init
 game_reload(state, ed_api, ed_api_new, true);
 
 init_dynamic(the_model->persistent.vertices, &state->permanent_arena, 128);
 
 return state;
}

global Type_Info_Pointers type_info_pointers = {
#define X(T)   .T = type_info_of(T),
 TypeInfoPointerList(X)
#undef X
};

function void
game_reload(Game_State *state, API_VTable_ed *ed_api, API_VTable_ed_new *ed_api_new, b32 first_time)
{
 the_model = &state->model;
 import_api_from_editor(ed_api, ed_api_new);
 state->sending_data = 1;
 
 if(not first_time)
 {
  thread_permanent_arena = make_arena(MB(1));
 }
 Arena *dll_arena = &thread_permanent_arena;
 
 {
  Game_Transient_State *transient = push_struct0(dll_arena, Game_Transient_State);
  state->transient = transient;
  init_dynamic(transient->pinned_locations, dll_arena);
  init_dynamic(transient->hot_locations, dll_arena);
 }
 
 tweaks = push_struct(dll_arena, Tweak_Variables);
 
 make_all_type_info();
 
 {//-NOTE: Dear ImGui reload
  IMGUI_CHECKVERSION();
  auto &imgui = state->imgui_state;
  ImGui::SetCurrentContext(imgui.ctx);
  ImGui::SetAllocatorFunctions(imgui.alloc_func, imgui.free_func, imgui.user_data);
 }
 
 init_sliders(type_info_pointers);
 build_location_maps(dll_arena, 0);
 load_slider_values_file(state, /*is_driver*/0);
}
function void
game_shutdown(Game_State *state)
{
 // NOTE(kv) Just unload the driver, so it doesn't get messy next time.
 Driver_API *driver = &state->driver_api;
 if(is_valid(driver))
 {
  driver->driver_shutdown();
  gb_dll_unload(driver->dll.handle);
  driver->dll = {};
 }
 arena_free(&thread_permanent_arena);
}
//~
function b32
game_save(Game_State *state, App *app, b32 is_manual)
{// NOTE Save and backup logic
 Scratch_Scope tmp;
 Stringz outpath = (is_manual ? state->manual_save_path :
                    state->autosave_path);
 String backup_dir = state->backup_dir;
 
 b32 ok = true;
 if(!state->has_done_backup &&
    gb_file_exists(to_cstring(outpath)))
 {//-Backup situation
  String time_string = time_format(tmp, "%d_%m_%Y_%H_%M_%S");
  if(time_string.len == 0)
  {
   log_error("strftime failed... go figure that out!");
   ok = false;
  }
  else
  {
   const char *filename_base = is_manual ? "manual" : "auto";
   Stringz backup_path = push_stringf(tmp, "%S/%s_%S.ad",
                                      backup_dir, filename_base, time_string);
   ok = copy_file(outpath, backup_path, true);
   state->has_done_backup = ok;
  }
  
  if(ok)
  {// NOTE Cycle out old backup files
   // TODO Maybe treat manual backups differently? idk man!
   File_List backup_files = system_get_file_list(tmp, backup_dir);
   u32 max_backup = 128;
   if (backup_files.count > max_backup)
   {
    u64 oldest_mtime = u64_max;
    Stringz file_to_delete = empty_string;
    File_Info **opl = backup_files.infos + backup_files.count;
    for (File_Info **backup = backup_files.infos;
         backup < opl;
         backup++)
    {
     File_Attributes attr = (*backup)->attributes;
     if(attr.last_write_time < oldest_mtime)
     {
      oldest_mtime   = attr.last_write_time;
      file_to_delete = pjoin(tmp, backup_dir, (*backup)->filename);
     }
    }
    b32 delete_ok = remove_file(file_to_delete);
    if(delete_ok){
     log_string("deleted backup file %S because it's too old", file_to_delete);
    }else{
     log_error("failed to delete backup file %S", file_to_delete);
    }
   }
  }
 }
 {
  Stringz temp_path = pjoin(tmp, state->save_dir, strlit("temp_file.ad"));
  Stringz old_path  = pjoin(tmp, state->save_dir, strlit("temp_old_file.ad"));
  if(ok)
  {//-serialize to temp file
   FILE *temp_outfile = open_file(temp_path, "wb");
   ok = serialize_state(temp_outfile, state);
   if(not ok){
    log_error("Failed to write to %.*s", strexpand(outpath));
   }
   close_file(temp_outfile);
  }
  b32 moved_to_old_path = false;
  //TODO(kv) are we overdoing this? we already have backup logic, why do we care if this fails?
  if(ok)
  {//-fail-safe setup
   if(file_exists(outpath)){
    ok = move_file(outpath, old_path);
    moved_to_old_path = ok;
   }
  }
  if(ok)
  {//-rename the file
   ok = move_file(temp_path, outpath);
  }
  if(not ok and moved_to_old_path)
  {
   //-fail-safe recover
   move_file(old_path, outpath);
  }
  remove_file(old_path);
  if(not ok){
   vim_set_bottom_text(strlit("failed to save state"));
  }
 }
 if(ok){
  vim_set_bottom_text(strlit("Saved game state!"));
  // NOTE(kv) Q51: recording.ad rides the same cadence as autosave.ad; its own
  // failure only logs -- the state save above already succeeded.
  save_recording_file(state);
 }
 state->save_failed = not ok;
 return ok;
}

// NOTE(kv): Can you believe we used to have complicated crap like "distance_level"?
// There is no "distance_level", fool! There's only distance!
function v1
update_camera_distance(v1 distance, i1 delta_level){
 const v1 mult = 1.3f;
 distance *= integer_power(mult, delta_level);
 return distance;
}
function void
compute_direction_helper(Game_Input *input, Key_Code key_code, i32 component, v1 value)
{
 if(input->key_states[key_code] != 0)
 {
  input->direction.dir.e[component] = value;
  input->direction.new_keypress = (input->key_state_changes[key_code] > 0);
 }
}
function void
update_orbit(Camera_Data *cam, Key_Direction key_dir)
{
 if(key_dir.new_keypress)
 {
  v3 dir = key_dir.dir.xyz;
  {//NOTE(kv) Zoom update
   i1 delta_distance_level = cast(i1)signof(dir.z);
   cam->distance = update_camera_distance(cam->distance, delta_distance_level);
  }
  {//NOTE(kv) Orbit update
   v1 interval = 1.0f / 24.f;
   v1 theta = roundv1(cam->phi / interval);
   v1 phi   = roundv1(cam->theta   / interval);
   {
    v2 delta = dir.xy;
    theta += delta.x;
    phi   += delta.y;  // NOTE: pitch up when we go up
   }
   
   cam->phi = theta * interval;
   cam->theta   = phi   * interval;
   macro_clamp(-0.25f, cam->theta, 0.25f);
  }
 }
}
myinline void
update_orbit(Camera_Data *cam, Game_Input *input) {
 update_orbit(cam, input->direction);
}
function void
update_pan(Camera_Data *cam, Game_Input *input)
{
 v1 step = CAMERA_PAN_STEP_PER_DISTANCE * cam->distance;
 v2 delta_pan = input->direction.dir.xy;
 Camera computed_cam = setup_camera(*cam);
 cam->pivot += step*(delta_pan.x * computed_cam.x + 
                     delta_pan.y * computed_cam.y);
}
//-
#define V2_CASES \
case Key_Code_L: case Key_Code_H: \
case Key_Code_K: case Key_Code_J:
#define V3_CASES  V2_CASES case Key_Code_O: case Key_Code_I:
#define V4_CASES  V3_CASES case Key_Code_Period: case Key_Code_Comma:

inline b32 is_v2_key(Key_Code code){ switch(code){ V2_CASES return true; } return false; }
inline b32 is_v3_key(Key_Code code){ switch(code){ V3_CASES return true; } return false; }
inline b32 is_v4_key(Key_Code code){ switch(code){ V4_CASES return true; } return false; }

#undef V2_CASES
#undef V3_CASES
#undef V4_CASES
//-

global_const b32 transitioning_from_code = true;

function void
g_jump_to_pos(App *app, i64 pos)
{
 // NOTE: Don't switch to the game panel, because the cursor should be in the code panel.
 View_ID view = get_active_view(app,0);
 if(!is_view_to_the_right(app, view))
 {// NOTE(kv) Switch to the right view
  view = get_other_primary_view(app, view, Access_Always, true);
 }
 view_set_buffer_named(app, view, DRIVER_FILE_NAME);
 view_set_cursor(app, view, seek_pos(pos));
}

function void
snap_camera(Camera_Data *cam, Viewport *viewport)
{
 v1 &prev    = viewport->previous_phi_snap;
 v1 &current = viewport->current_phi_snap;
 
 if(prev == 0.f and current == 0.f)
 {// NOTE(kv) Initialize state
  prev = 0.25f;
 }
 
 v1 phi4 = roundv1(cam->phi * 4.f);
 // NOTE Initial snapping effort, would be so simple if it was this easy!
 v1 new_phi = cycle01(phi4 * 0.25f);
 
 if(cam->phi == new_phi)
 {// NOTE We're already at a snap point
  // NOTE -1.f is the sentinel written in @game_init
  new_phi = prev;
 } 
 else if(new_phi == current)
 {// NOTE Snapping to the current snap -> find another snap point,
  // in the direction of the user movement.
  v1 dir = signof(cam->phi - new_phi);
  new_phi = cycle01(new_phi + 0.25f * dir);
 }
 
 prev = current;
 current = new_phi;
 cam->phi = new_phi;
 cam->theta = 0;
}

function void
do_work_after_loading_driver(Game_State *state, Driver_API *driver)
{
 driver_data = *driver->data;
 
 arena_clear(&state->driver_arena);
 build_location_maps(&state->driver_arena, 1);
 // NOTE(kv) The driver DLL's slider table is freshly zeroed: bring the values back.
 load_slider_values_file(state, /*is_driver*/1);
 fui_set_active_slider(0);
}
function b32
load_latest_driver_code(Game_State *state, App *app, Driver_API *driver,
                        b32 *oloaded)
{
 Driver_DLL *dll = &driver->dll;
 local_persist Framework_API framework_api;
 if(not framework_api.valid)
 {
#define X(N) framework_api.N = N;
  framework_api_xlist(X);
#undef X
  framework_api.valid = true;
  kv_assert(tweaks);
  framework_api.tweaks = tweaks;
  framework_api.types = type_info_pointers;
 }
 
 b32 ok = true;
 b32 loaded = false;
 Scratch_Block tmp; 
#define PJOIN(a, b) pjoin(tmp, a, b)
 String binary_dir = system_get_path(tmp, SystemPath_BinaryDirectory);
 
 Stringz lock_file = PJOIN(binary_dir, strlit("driver.lock"));
 b32 lock_file_exists = file_exists(lock_file);
 if(not lock_file_exists)
 {
  Stringz DRIVER_DLL_PATH = PJOIN(binary_dir, strlit("driver.dll"));
  u64 mtime_on_disk = file_mtime(DRIVER_DLL_PATH);
  ok = ok and (mtime_on_disk != 0);
  if(dll->mtime < mtime_on_disk)
  {//-We have new dll
   b32 copied = false;
   Stringz temp_path = {};
   String prefix = {};
   if(state->is_dev_editor){ prefix = strlit("dev_"); }
   for_i32(temp_index, 0, 8)
   {//NOTE(kv) Smooth-brained "retry until it works", because Windows sucks
    temp_path = push_stringf(tmp, "%S/%Sdriver%d.dll", binary_dir, prefix, temp_index);
    copied = copy_file(DRIVER_DLL_PATH, temp_path, false);
    if(copied)
    {
     break;
    }
   }
   if(not copied)
   {
    ok = false;
    DWORD error = GetLastError();
    String message = push_stringf(tmp, "failed to copy driver dll to temp file %S", temp_path);
    log_error(message);
   }
   
   DLL_Handle new_library = gb_dll_load(to_cstring(temp_path));
   ok = ok and (new_library != 0);
   if(not ok){ log_error(strlit("failed to load dll")); }
   
   if(ok)
   {
    if(dll->handle)
    {//NOTE Shutdown running DLL
     driver->driver_shutdown();
     b32 unload_ok = gb_dll_unload(dll->handle);
     if(not unload_ok){ log_error(strlit("WARN: failed to unload old dll")); }
    }
    
    typedef void Entry_Type(Driver_API *, Framework_API *);
    Entry_Type *driver_dll_entry = (Entry_Type *)gb_dll_proc_address(new_library, "driver_dll_entry");
    driver_dll_entry(driver, &framework_api);
    
    *dll = {};
    dll->handle     = new_library;
    dll->mtime      = mtime_on_disk;
    loaded = true;
   }
  }
 }
 
 if(loaded) 
 {
  do_work_after_loading_driver(state, driver);
 }
 
 *oloaded = loaded;
 return ok;
#undef PJOIN
}
function ImVec2
get_imgui_image_position_from_uv(ImVec2 image_pos, v2 image_size, v2 uv)
{
 ImVec2 pos = ImVec2(image_pos.x + uv.x*image_size.x,
                     image_pos.y + (1.0f - uv.y)*image_size.y);
 return pos;
}
function void
show_image_preview(Image_Info &image)
{
 v2 image_size;
 Texture_Handle texture = ed_load_image(image.filename, &image_size);
 if(is_valid(texture))
 {
  ImGuiWindowFlags window_flags = (ImGuiWindowFlags_AlwaysAutoResize |
                                   ImGuiWindowFlags_NoFocusOnAppearing);
  im_begin("ImagePreview", 0, window_flags);
  
  u64 texture_u64 = texture.v;  // NOTE(kv) pedantic compiler
  ImTextureID user_texture_id = ImTextureID(texture_u64);
  ImVec2 im_image_size(image_size.x, image_size.y);
  ImVec2 image_pos = ImGui::GetCursorScreenPos();
  b32 has_marker = image.marker.type != 0;
  ImVec4 tint = has_marker ? ImVec4(1,1,0,0.5f) : ImVec4(1,1,1,1);
  ImGui::ImageWithBg(user_texture_id, im_image_size, ImVec2(0,1), ImVec2(1,0), V4(), tint);
  
  if(has_marker)
  {
   ImDrawList* draw_list = ImGui::GetWindowDrawList();
   ImGuiCol marker_color = ImGui::GetColorU32(ImGuiCol_CheckMark);
   
   switch(image.marker.type)
   {
    case Image_Marker_Point:
    {// NOTE Draw marked uv
     v1 radius = 4.0f;
     v2 marked_uv = image.marker.point;
     ImVec2 marked_pos = get_imgui_image_position_from_uv(image_pos, image_size, marked_uv);
     draw_list->AddCircleFilled(marked_pos, radius, marker_color);
    }break;
    
    case Image_Marker_Bezier:
    {
     Bez_v2 curve = image.marker.bezier;
     const i32 npoints = 3;
     ImVec2 points[npoints];
     for_i32(i, 0, npoints){
      points[i] = get_imgui_image_position_from_uv(image_pos, image_size, curve.e[i]);
     }
     float thickness = 2.f;
     int num_segments = 16;
     draw_list->AddBezierQuadratic(expand3(points), marker_color, thickness, num_segments);
    }break;
   }
  }
  
  im_end();
 }
}
struct Plane
{
 v3 n;
 v1 d;
};

function v1
hit_test_ray_triangle(v3 ray_P, v3 ray_dir,
                      v3 O, v3 A, v3 B)
{
 v1 result = INFINITY;
 
 A -= O;
 B -= O;
 
 v3 h = cross(ray_dir, B);
 v1 det = dot(A, h);
 
 if(absolute(det) < 1e-5f)
 {// NOTE(kv) This happens when "h" is perpendicular to "A",
  // which happens when "ray_dir" is perpendicular to the
  // normal of the plane containing the triangle "(O,A,B)".
 }
 else
 {
  ray_P -= O;
  v1 det_inv = 1.f / det;
  v1 u = det_inv * dot(ray_P, h);
  
  if(u < 0.f or u > 1.f)
  {
   // NOTE Outside
  }
  else
  {
   v3 q = cross(ray_P,A);
   v1 v = det_inv * dot(ray_dir, q);
   
   if (v < 0.0 or u + v > 1.0)
   {
    // NOTE outside
   }
   else
   {// NOTE At this stage we can compute "t" to find out where
    // the intersection point is on the line.
    v1 t = det_inv * dot(B,q);
    
    if(t > 1e-5f)  // ray intersection
    {
     result = t;
    }
    else
    {// NOTE This means that there is a line intersection
     // but not a ray intersection
    }
   }
  }
 }
 return result;
}

function v1
get_distance_squared_point_to_triangle(v3 test_point, Poly3 poly3)
{
 v1 dsq = f32_max;
 //-Projection onto the curve
 // View @project_point_onto_plane for more details of the projection math
 v3 v01 = poly3[1]-poly3[0];
 v3 v02 = poly3[2]-poly3[0];
 v3 n = noz(cross(v01, v02));
 if(n != v3{})
 {
  v1 d = -dot(poly3[0], n);
  v1 t = -(d + dot(n, test_point));
  v3 projection = test_point + t*n;  // NOTE this point is on the plane
  
  // TODO(kv) #speed this math sucks tremendously
  v3 proj0 = projection - poly3[0];
  v3 proj1 = projection - poly3[1];
  v3 proj2 = projection - poly3[2];
  v3 c01 = cross(proj0, proj1);
  v3 c12 = cross(proj1, proj2);
  v3 c20 = cross(proj2, proj0);
  b32 projection_in_triangle = (dot(c01, c12) >= 0 and
                                dot(c01, c20) >= 0);
  if(projection_in_triangle)
  {
   dsq = squared(d + dot(test_point, n));
  }
 }
 return dsq;
}


function sarray(Poly3)
poly4_to_poly3(Arena *arena, v3 p[4])
{
 sarray(Poly3) result;
 init_static(result, arena, 2);
 result[0] = Poly3{p[0], p[1], p[2]};
 result[1] = Poly3{p[0], p[2], p[3]};
 return result;
}

function sarray(Poly3)
get_hit_triangles_from_vertex(Arena *arena, v3 pos)
{// NOTE(kv) Just place a square around the vertex,
 // #Hack we're working in camera space for now, so it's easier.
 const v1 radius = vertex_indicator_radius;
 v2 radius_v2 = V2(radius, radius);
 v2 min = pos.xy - radius_v2;
 v2 max = pos.xy + radius_v2;
 
 v2 pv2[4];
 pv2[0] = V2(min.x, min.y);
 pv2[1] = V2(max.x, min.y);
 pv2[2] = V2(max.x, max.y);
 pv2[3] = V2(min.x, max.y);
 
 v3 pv3[4];
 for_i32(i, 0, 4)
 {// NOTE Copy the z of the vertex
  pv3[i] = V3(pv2[i], pos.z);
 }
 
 sarray(Poly3) result = poly4_to_poly3(arena, pv3);
 
 return result;
}

function Location
get_primitive_hit_by_mouse(Game_State *state, Live_Viewport *mouse_viewport,
                           i2 params_mouse_p)
{
 Location hot_location = {};
 
 if(mouse_viewport)
 {
  v2 center = get_center(mouse_viewport->clip_box);
  v2 mouse_px = V2(params_mouse_p) - center;
  v2 mouse_meter_ = mouse_px / default_meter_to_pixel;
  v1 mouse_z = -tweaks->focal_length;  // TODO #Hack
  v3 mouse_cam = V3(mouse_meter_, mouse_z);
  mouse_cam.y *= -1.f;
  
  // TODO(kv) There are multiple cameras, so this is a no-win, huh?
  Camera camera = setup_camera(state->viewports[0].camera);
  convert_primitives_to_camera_space(camera);
  b32 fill_only = state->model.recordings.preset_settings[state->viewports[0].preset].fill_only_picking;
  
  v1 min_t = INFINITY;
  
  // NOTE(kv) in camera space
  v3 ray_P = V3();
  v3 ray_dir = noz(mouse_cam);
  
  if(not fill_only)
  {// NOTE(kv) Vertices
   Scratch_Block tmp;
   Vertices vertices = the_model->vertices;
   for_i32(vi, 0, vertices.count)
   {
    arena_clear(tmp);
    
    Vertex &vertex = vertices.items[vi];
    sarray(Poly3) triangles = get_hit_triangles_from_vertex(tmp, vertex.pos);
    
    for_i32(ti, 0, triangles.count)
    {// NOTE(kv) Hit test
     Poly3 triangle = triangles[ti];
     v1 hit_t = hit_test_ray_triangle(ray_P, ray_dir, expand3(triangle));
     if(hit_t < min_t)
     {
      // TODO(kv) ...
      Vertex_Info info = get_vertex_info(vertex);
      hot_location = info.location;
      min_t = hit_t;
     }
    }
   }
  }
  
  Scratch_Block tmp;
  for_i32(pi, 0, the_model->camera_primitives.count)
  {// NOTE(kv) Closest primitive
   arena_clear(tmp);
   Recorded_Primitive &primitive = the_model->camera_primitives[pi];
   darray(Poly3) triangles;
   init_dynamic(triangles, tmp);
   
   switch(primitive.type)
   {
    case Primitive_Type_Curve:
    {
     if(not fill_only)
     {
      tvert *curve = primitive.curve.bezier;
      const i32 test_segment_count = 8;
      // NOTE(kv) We know the triangle count,
      // But just in case we mess up the code...
      set_cap_min(&triangles, 2*test_segment_count);
      
      v1 test_t_interval = 1.0f / v1(test_segment_count);
      v3 A = curve[0];
      
      for_i32(si, 0, test_segment_count)
      {
       v1 B_t = test_t_interval * v1(si+1);
       v3 B = bezier_sample(curve, B_t);
       
       // NOTE(kv) We just assume that these two points are on the same plane.
       v2 u = B.xy - A.xy;
       v2 v_ = vertex_indicator_radius * noz(perp(u));
       v3 v = V3(v_);
       
       v3 R[4];
       R[0] = A - v;
       R[1] = B - v;
       R[2] = B + v;
       R[3] = A + v;
       
       sarray(Poly3) segment_triangles = poly4_to_poly3(tmp, R);
       for_i32(i, 0, 2)
       {
        push(&triangles, segment_triangles[i]);
       }
       
       A = B;
      }
     }
    }break;
    
    case Primitive_Type_Poly3:
    {//-Projection onto the curve
     push(&triangles, primitive.poly3);
    }break;
    
    case Primitive_Type_Dual_Bezier:
    {
     i32 const nslices = 8;
     set_cap_min(&triangles, nslices*2);
     
     Bezier P = primitive.dual_bezier.P;
     Bezier Q = primitive.dual_bezier.Q;
     v1 inv_nslices = 1.f / (v1)nslices;
     v3 A0 = P[0];
     v3 B0 = Q[0];
     for_i32(sample_index, 0, nslices)
     {
      v1 u = inv_nslices * (v1)(sample_index+1);
      v3 A = bezier_sample(P,u);
      v3 B = bezier_sample(Q,u);
      
      push(&triangles, {A0, A, B0});
      push(&triangles, {A, B, B0});
      
      A0 = A;
      B0 = B;
     }
    }break;
   }
   
   for_i32(ti, 0, triangles.count)
   {// NOTE Hit test #copypasta
    Poly3 triangle = triangles[ti];
    v1 hit_t = hit_test_ray_triangle(ray_P, ray_dir, expand3(triangle));
    if(hit_t < min_t)
    {
     hot_location = primitive.location;
     min_t = hit_t;
    }
   }
  }// NOTE Loop over primitives
 }
 
 return hot_location;
}


function Live_Viewport *
get_live_viewport_by_id(sarray(Live_Viewport) viewports, Viewport_ID id)
{
 Live_Viewport *result = 0;
 for_i32(index, 0, viewports.count)
 {
  if(viewports[index].id == id)
  {
   return &viewports[index];
  }
 }
 return 0;
}

#include "game_debug_channel.cpp"

function Game_Update_Return
game_update(Game_Update_Params params)
{// @game_api, see also @maybe_update_game
 Scratch_Block tmp;
 update_game_config();
 Game_State *state = params.state;
 App *app = params.app;
 debug_channel_update(state);
 b32 should_animate_next_frame = false;
 arena_clear(&state->frame_arena);
 //-
 darray(String) game_commands = {};
 init_dynamic(game_commands, &state->frame_arena);
 
 Game_Input input_value = {};
 (Game_Input_0 &) input_value = params.input;
 Game_Input *input = &input_value;
 v1 dt = params.frame.animation_dt;
 v1 literal_dt = params.frame.literal_dt;
 
 {
  state->looping_time += dt;
  if(state->looping_time >= 1000.0f){ state->looping_time -= 1000.0f; }
 }
 
 {//-Compute key direction
  compute_direction_helper(input, Key_Code_L, 0, +1);
  compute_direction_helper(input, Key_Code_H, 0, -1);
  compute_direction_helper(input, Key_Code_K, 1, +1);
  compute_direction_helper(input, Key_Code_J, 1, -1);
  compute_direction_helper(input, Key_Code_O, 2, +1);
  compute_direction_helper(input, Key_Code_I, 2, -1);
  compute_direction_helper(input, Key_Code_Period, 3, +1);
  compute_direction_helper(input, Key_Code_Comma,  3, -1);
 }
 v4 input_dir = input->direction.dir;
 
 // NOTE(kv) Cheesy single keyboard event per-frame,
 // since we're not a fighting game, it'd probably work ok anyway.
 // but it's very dumb because we already had events.
 darray(Key_Code) key_strokes;
 init_dynamic(key_strokes, tmp);
 for_i32(code, 1, Key_Code_COUNT)
 {
  if(input->key_states[code] &&
     input->key_state_changes[code] > 0)
  {
   push(&key_strokes, (Key_Code)code);
  }
 }
 
 Driver_API *driver = &state->driver_api;
 if(DRIVER_ENABLED)
 {
  b32 loaded;
  load_latest_driver_code(state, app, driver, &loaded);
 }
 b32 driver_on = DRIVER_ENABLED and is_valid(driver);
 
 {
  if(driver_on)
  {
   driver->driver_update_tweaks();
   
   if(params.game_was_turned_on_this_frame)
   {
    View_ID view = get_active_view(app, Access_Always);
    if(is_view_to_the_right(app, view))
    {// NOTE: switch to the left
     view = get_other_primary_view(app, view, Access_Always, true);
    }
    view_set_buffer(app, view, get_game_buffer(app, 1), 0);
   }
  }
  
  b32 cursor_on = state->kb_cursor.on;
  i32 active_viewport_id = get_active_game_viewport_id(app);
  b32 viewport_focused = driver_on and active_viewport_id != 0;
  if(viewport_focused or fui_is_active())
  {
   should_animate_next_frame = true;
  }
  
  i32 update_viewport_id = (active_viewport_id ? active_viewport_id : 1);
  kv_assert(active_viewport_id <= GAME_VIEWPORT_COUNT);
  i32 update_viewport_index = update_viewport_id - 1;
  Viewport *update_viewport = &state->viewports[update_viewport_index];
  Camera_Data *update_target_camera_data = get_target_camera(state, update_viewport_index);
  // NOTE(kv) Let's just base all our calculation on the target camera,
  // because most of the time the camera isn't moving.
  // Diligently distinguishing *current* and *target* cameras
  // wouldn't bring much benefit, it only complicates thing.
  Camera update_target_camera = setup_camera(*update_target_camera_data);
  if(0)
  {
   v3 camera_world_pos = get_world_pos(update_target_camera);
   DEBUG_VALUE(camera_world_pos);
  }
  
  // TODO(kv) Should we have like a "state diff"?
  // If we did, we could autosave much more confidently.
  v1 AUTOSAVE_PERIOD_SECONDS = 60.0f;
  local_persist v1 seconds_since_last_keystroke_2 = 0;
  {// NOTE autosave
   local_persist v1 seconds_since_last_autosave = 0.001f;
   seconds_since_last_autosave += literal_dt;
   seconds_since_last_keystroke_2 += literal_dt;
   if(seconds_since_last_keystroke_2 > AUTOSAVE_PERIOD_SECONDS and
      seconds_since_last_autosave > AUTOSAVE_PERIOD_SECONDS)
   {
    seconds_since_last_autosave = 0;
   }
   b32 should_autosave = seconds_since_last_autosave == 0;
   if(should_autosave)
   {
    game_save(state, app, false);
    vim_set_bottom_text(strlit("game auto-saved!"));
   }
  }
  
  Location hot_location = {};
  if(0)
  {// NOTE(kv) Mouse cursor disabled
   if(cursor_on and viewport_focused)
   {
    // hot_location = find_primitive_closest_to_keyboard_cursor(state);
   }
  }
  
  Live_Viewport *mouse_viewport = 0;
  {// NOTE Get mouse viewport
   v2 mouse_px = V2(params.mouse.p);
   sarray(Live_Viewport) viewports = params.live_viewports;
   for_i32(index, 0, viewports.count)
   {
    Live_Viewport *viewport = &viewports[index];
    rect2 box = viewport->clip_box;
    if(contains(box, mouse_px))
    {
     mouse_viewport = viewport;
     break;
    }
   }
  }
  
  hot_location = get_primitive_hit_by_mouse(state, mouse_viewport, params.mouse.p);
  
  if(params.mouse.press_left)
  {// NOTE(kv) Jump to code location
   if(is_valid(hot_location))
   {
    g_jump_to_pos(app, resolve_location(hot_location).min);
   }
  }
  
  {// NOTE(kv) Right-click menu
   if(params.mouse.press_right)
   {
    ImGui::OpenPopup("right_click_popup");
   }
   
   if(ImGui::BeginPopup("right_click_popup"))
   {
    // NOTE(kv) Drawing the menu
    i32 selected = 0;
    const char *menu_items[] = { "NONE", "Add Vertex" };
    for_i32(i, 1, alen(menu_items))
    {
     if(ImGui::Selectable(menu_items[i]))
     {
      selected = i;
      break;
     }
    }
    
    // NOTE(kv) Handling selected items
    const i32 add_vertex_index = 1; // TODO #Hack
    switch(selected)
    {
     case add_vertex_index:
     {
      if(selected == add_vertex_index)
      {
       darray(Vertex) *vertices = &the_model->persistent.vertices;
       
       Vertex vertex = {};
       vertex.ninfo_index = -1;
       vertex.bone_id = mk_bone_id(Bone_None);
       {// NOTE(kv) Hacking the vertex position
        v3 pos = {};
        if(vertices->count == 1)
        {
         pos = V3(0.1f,0,0);
        }
        vertex.pos = pos;
       }
       
       push(vertices, vertex);
      }
     }break;
    }
    
    ImGui::EndPopup();
   }
  }
  
  {//-Work based on editor cursor position
   View_ID view = get_active_view(app, Access_Always);
   Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
   FUI_File file = get_fui_file_by_buffer(app, buffer);
   {//;do_stuff_based_on_cursor_position
    i64 curpos = view_get_cursor_pos(app, view);
    Location_Map map = get_location_map(file);
    for(Location_Iterator it = iterate_touched_locations(file, {curpos, curpos+1});
        it.entry;
        advance(&it))
    {
     Location_Map_Entry entry = *it.entry;
     b32 maybe_make_it_hot = 0;
     switch(entry.type)
     {
      case Location_Type_Vertex:
      {//-Maybe make it hot
       maybe_make_it_hot = 1;
      }break;
      
      case Location_Type_Text_Object:
      {//-images preview
       Text_Object &object = get_fui_file(file).text_objects[entry.index_in_file];
       switch(object.kind)
       {
        case Text_Object_Drawn:
        {
         maybe_make_it_hot = 1;
        }break;
        
        case Text_Object_Image:
        {
         show_image_preview(object.image);
        }break;
       }
      }break;
     }
     
     if(maybe_make_it_hot)
     {
      if(not is_valid(hot_location))
      {
       hot_location = {file, entry.range};
      }
     }
    }
   }
  }
  
  Game_Transient_State *transient = state->transient;
  transient->hot_locations.count = 0;
  for_i32(i, 0, transient->pinned_locations.count)
  {
   push(&transient->hot_locations, transient->pinned_locations[i]);
  }
  push_unique(&transient->hot_locations, hot_location);
  
  {//-Game commands
   // NOTE(kv) Commands are things that the editor send to the game.
   
   {//-Serve commands
    darray(Game_Command) &queue = state->command_queue;
    for_i32(command_index,0,queue.count)
    {
     Game_Command command = queue[command_index];
#define MATCH(NAME)    command.name == strlit(NAME)
     if(0);
     else if(MATCH("save_manual"))
     {
      b32 ok = game_save(state, app, true);
      if(ok)
      {
       copy_file(state->manual_save_path, state->autosave_path, false);
      }
     }
     else if(MATCH("load_manual"))
     {
      game_load(state, app, state->manual_save_path);
     }
     else if(MATCH("revert"))
     {
      revert_from_autosave(state, app);
     }
     else if(MATCH("pin"))
     {
      if(is_valid(hot_location))
      {
       push_unique(&transient->pinned_locations, hot_location);
      }
     }
     else if(MATCH("clear_pin"))
     {// TODO(kv) I suspect we're gonna need UI to pin/clear specific things. Oh boy...
      transient->pinned_locations.count = 0;
     }
     else if(MATCH("clear_preset"))
     {
      update_viewport->reference_preset = Preset_None;
     }
     else
     {
      log_error("game: cannot serve command");
     }
#undef MATCH
    }
    queue.count = 0;
   }
   
   if(driver_on)
   {//-Fill command lister
    darray(String) &cmds = game_commands;
    cmds.count = 0;
    push(&cmds, strlit("save_manual"));
    push(&cmds, strlit("load_manual"));  
    push(&cmds, strlit("revert"));  
    push(&cmds, strlit("pin"));  
    push(&cmds, strlit("clear_pin"));  
    push(&cmds, strlit("clear_preset"));
   }
  }
  
  if(key_strokes.count and viewport_focused)
  {
   seconds_since_last_keystroke_2 = 0;
  }
  
  u32 mods = input->active_mods;
  for_i32(key_stroke_index, 0, key_strokes.count)
  {//-NOTE(kv) Key bindings
   Camera &cam = update_target_camera;
   Camera_Data *cam_data = update_target_camera_data;
   Key_Code keycode = key_strokes[key_stroke_index];
   u32 code = mods|keycode;
   const u32 S = Key_Mod_Sft;
   const u32 C = Key_Mod_Ctl;
   const u32 M = Key_Mod_Alt;
   if(viewport_focused)
   {
    if(mods==Key_Mod_Ctl and is_v3_key(keycode))
    {
     update_orbit(cam_data, input);
    }
    else if(mods==Key_Mod_Alt and is_v2_key(keycode))
    {
     update_pan(cam_data, input); 
    }
    else
    {//-Other keys
     switch(code)
     {
      case Key_Code_L: case Key_Code_H:
      case Key_Code_K: case Key_Code_J:
      {
       if(not cursor_on)
       {// NOTE(kv) Cursor is updated separately, because it's continuous input.
        update_orbit(cam_data, input);
       }
      }break;
      
      case Key_Code_0: case Key_Code_1: case Key_Code_2: case Key_Code_3: case Key_Code_4:
      case Key_Code_5: case Key_Code_6: case Key_Code_7: case Key_Code_8: case Key_Code_9:
      {
       i32 preset = code - Key_Code_0;
       game_set_preset(state, update_viewport_id, preset);
      }break;
      
      case Key_Code_Space: { game_last_preset(state, update_viewport_id); }break;
      case Key_Code_M:     { state->kb_cursor.on = true; } break;
      case Key_Code_Escape:{ state->kb_cursor.on = false; }break;
      
      case C|Key_Code_Return:{ game_save(state, app, false); }break;
      case Key_Code_A:
      {
       snap_camera(cam_data, update_viewport);
      }break;
      case Key_Code_Q:{ toggle_boolean(state->references_full_alpha); }break;
      case Key_Code_X:{ cam_data->phi *= -1.f; }break;
      case S|Key_Code_Z:{ cam_data->phi = .5f - cam_data->phi; }break;
      case S|Key_Code_0:{ cam_data->roll = {}; }break;
      //NOTE(kv) Reverting is just.so.useful for debugging!
      //TODO(kv) Pushing strings is dangerous! Since the game might restart.
      case S|Key_Code_U:
      {
       revert_from_autosave(state, app);
      }break;
      
      //NOTE(kv) Set camera to the left
      case C|M|Key_Code_H:{ cam_data->phi=-.25f; cam_data->theta=0; }break;
      
      case Key_Code_I:
      case Key_Code_O:
      {
       update_orbit(cam_data, input);
      }break;
      
      case Key_Code_Return:
      {
       if(0)
       {// NOTE(kv) OLD mouse cursor code
        if(is_valid(hot_location))
        {
         g_jump_to_pos(app, resolve_location(hot_location).min);
        }
       }
      }break;
      
      case Key_Code_Z:{
       v2 cursor_camera_xy = mat4vert(cam.cam_from_world, state->kb_cursor.pos).xy;
       cam_data->pivot += (cursor_camera_xy.x * cam.x +
                           cursor_camera_xy.y * cam.y);
      }break;
     }
    }
   }
   else if(fui_is_active())
   {//-fui
    if(mods==C && is_v3_key(keycode))
    {
     update_orbit(cam_data, input);
    }
    else if(mods==M && is_v2_key(keycode))
    {
     update_pan(cam_data, input);
    }
    else if(mods==0 && is_v4_key(keycode))
    {//-Update discrete slider
     v1 float_increment;
     if(active_slider_is_discrete(&float_increment))
     {
      Active_Slider slider = fui_active_slider;
      i4 value_int;
      v4 value_float;
      b32 is_float = float_increment != 0.f;
      Data_And_Size data = active_slider_data();
      block_copy(&value_int, data.data, data.size);
      block_copy(&value_float, data.data, data.size);
      for_i32(index,0,4)
      {
       value_int.e[index]   += i32(input_dir[index]);
       value_float.e[index] += i32(input_dir[index]) * float_increment;
      }
      if(slider.data->flags & Slider_Clamp_01)
      {
       for_i32(index,0,4)
       {
        macro_clamp01i(value_int.e[index]);
        macro_clamp01(value_float.e[index]);
       }
      }
      if(is_float) {
       block_copy(data.data, &value_float, data.size);
      } else {
       block_copy(data.data, &value_int, data.size);
      }
     }
    }
    else
    {//-fui: Other keys
     switch(code)
     {
      case Key_Code_Tab:
      {
       Type_Info *type = get_slider_type_info(*fui_active_slider.data);
       if(equal(type, type_info_of(FUI_Line_Params)))
       {
        i32 radii_index = member_index_of(FUI_Line_Params, radii);
        i32 lightness_index = member_index_of(FUI_Line_Params, lightness_additions);
        i32 active_index = fui_active_slider.active_member_index;
        i32 new_index;
        if(active_index == radii_index){
         new_index = lightness_index;
        }else{
         new_index = radii_index;
        }
        fui_active_slider.active_member_index = new_index;
       }
      }break;
      
      case Key_Code_Return:
      {// NOTE Commit: the value is already live in the slider table; persist the
       // table to the values file (the source text only holds the id).
       save_slider_values_file(state, fui_active_slider.data->location.file.is_driver);
       fui_set_active_slider(0);
      }break;
      
      case Key_Code_Escape:
      {
       fui_restore_value(fui_active_slider.data);
       fui_set_active_slider(0);
      }break;
     }
    }
   }
   else
   {//-We're somewhere in the editor
    switch(code)
    {
     case Key_Code_Return:
     {
      View_ID view = get_active_view(app, Access_Always);
      Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
      Slider *slider = get_hot_slider_under_cursor(app);
      if(slider)
      {//-Activate slider
       fui_save_value(slider);
       fui_set_active_slider(slider);
      }
      else if(driver_on)
      {//-Button?
       FUI_File file = get_fui_file_by_buffer(app, buffer);
       i64 curpos = view_get_cursor_pos(app, view);
       for(Location_Iterator it = iterate_touched_locations(file, { curpos, curpos+1 });
           it.entry;
           advance(&it))
       {
        if(it.entry->type == Location_Type_Text_Object)
        {
         Text_Object &object = get_fui_file(file).text_objects[it.entry->index_in_file];
         if(object.kind == Text_Object_Preset)
         {
          Viewport &main_viewport = state->viewports[0];
          main_viewport.reference_preset = object.preset;
          
          Reference_Preset_Data preset_data =
          driver->driver_get_reference_preset_data(object.preset);
          
          // NOTE(kv) We update the camera *once*, but still let it fly afterwards.
          update_target_camera_data->theta = preset_data.camera_theta2;
          update_target_camera_data->phi   = preset_data.camera_phi2;
          break;
         }
        }
       }
      }
     }break;
    }
   }
  }
  
  if(input_dir != v4{} and
     fui_is_active() and active_slider_is_continuous())
  {//-Update continuous sliders
   Active_Slider slider = fui_active_slider;
   Type_Info *active_type = active_slider_member_type_info();
   Data_And_Size data = active_slider_data();
   // NOTE(kv) Pretend slider value it's a v4
   v4 value;
   block_copy(&value, data.data, data.size);
   if(mods == 0 or mods == Key_Mod_Sft)
   {
    if(input_dir.y != 0.f and
       type_info_equals(strip_to_basic_type(active_type), v1))
    {//-Special handling j-k to toggle values to 0 and 1
     value.x = (input_dir.y > 0) ? 1.f : 0.f; 
    }
    else
    {
     Slider_Flags flags = slider.data->flags;
     b32 is_camera_aligned = (type_info_equals(active_type, tvert) or
                              type_info_equals(active_type, tnormal) or
                              type_info_equals(active_type, v3));
     if(is_camera_aligned)
     {
      input_dir.xyz = mat4vec(update_target_camera.world_from_cam, input_dir.xyz);
     }
     v1 delta_scale = slider.data->delta_scale;
     if(delta_scale == 0){ delta_scale = 0.2f; }
     v4 delta = delta_scale * dt * input_dir;
     if(mods == Key_Mod_Sft){ delta *= 10.f; }
     value += delta;
     
     if(flags & Slider_Clamp_X){value.x = 0;}
     if(flags & Slider_Clamp_Y){value.y = 0;}
     if(flags & Slider_Clamp_Z){value.z = 0;}
     if(type_info_equals(active_type, tnormal)){ value.xyz = noz(value.xyz); }
     if(flags & Slider_Clamp_01)
     {
      for_i32(index,0,4){ macro_clamp01(value.v[index]); }
     }
    }
   }
   block_copy(data.data, &value, data.size);
  }
  
  if(mouse_viewport)
  {
   i32 wheel = signof(params.mouse.wheel);  // NOTE(kv) We have WEIRD +/-100 mouse wheel values!
   if(wheel)
   {
    Viewport &viewport = state->viewports[mouse_viewport->id-1];
    v1 &distance = viewport.target_camera.distance;
    distance = update_camera_distance(distance, wheel);
   }
  }
  
  if(viewport_focused and cursor_on)
  {//-NOTE(kv) update cursor
   Camera &cam = update_target_camera;
   Camera_Data *cam_data = update_target_camera_data;
   Keyboard_Cursor &cursor = state->kb_cursor;
   b32 shifted = 0;
   v2 dir_v2 = key_direction(input, 0, false, &shifted).xy;
   b32 cursor_moved = false;
   if(dir_v2 == v2{})
   {
    cursor.vel = {};  //NOTE(kv) stop immediately, we don't want skating
   }
   else
   {//NOTE Moving
    cursor_moved = true;
    v3 dir = noz( mat4vec(cam.world_from_cam, V3(dir_v2)) );
    v1 cursor_camz = mat4vert(cam.cam_from_world, cursor.pos).z;
    v1 zoom = absolute(cursor_camz / cam.focal_length);
    v1 acc = zoom * 0.1f * (3.f);
    v1 boost = 4.0f;
    if(shifted){ acc *= boost; }
    v1 new_vel = cursor.vel + dt*acc;
    
    v1 max_vel = zoom*0.1f*2.f;
    if(shifted){ max_vel *= boost; }
    ClampTop(new_vel, max_vel);
    
    v3 delta = 0.5f*(cursor.vel+new_vel)*dt*dir;
    cursor.pos += delta;
    cursor.vel = new_vel;
   }
   
   {//NOTE(kv) Clamping screen and cursor position (Pretty Involved)
    //TODO(kv) #bug We assume orthographic mode
    v3 cursor_cam = mat4vert(cam.cam_from_world, cursor.pos);
    
    Live_Viewport *update_viewport2 = &params.live_viewports[update_viewport_index];
    
    v1 meter_to_pixel = default_meter_to_pixel;
    v1 pixel_to_meter = 1.f / meter_to_pixel;
    v2 clip_radius = pixel_to_meter*get_radius(update_viewport2->clip_box);
    
    v2 radius_on_cam = 0.875f * clip_radius;
    v1 zoom_ratio = absolute(cursor_cam.z / cam.focal_length);
    v2 radius_at_cursor_z = zoom_ratio*radius_on_cam;
    
    if(cursor_moved)
    {//-cursor dictates camera
     v2 delta_in_cam = {};
     for_i32(i,0,2)
     {
      v1 diff = absolute(cursor_cam[i]) - absolute(radius_at_cursor_z[i]);
      if(diff > 0)
      {
       delta_in_cam[i] = signof(cursor_cam[i]) * diff;
      }
     }
     
     cam_data->pivot += mat4vec(cam.world_from_cam, V3(delta_in_cam, 0));
    }
   }
  }
  //~
  
  if(params.debug_camera_on)
  {
   Camera_Data &cam = *update_target_camera_data;
   DEBUG_NAME("camera(theta,phi,distance)", V3(cam.phi, cam.theta, cam.distance));
  }
  
  for_i32(index, 0, GAME_VIEWPORT_COUNT)
  {// NOTE Set viewport presets to useful values
   Viewport *viewport = &state->viewports[index];
   
   if(viewport->preset == viewport->last_preset){
    if(viewport->preset == 0){ viewport->last_preset = 2; }
    else{ viewport->last_preset = 0; }
   }
  }
  
  {// TODO: Have a better error reporting story
   // Like, how do we turn these off? With a clear command?
   if (state->load_failed) { DEBUG_TEXT("Load failed!"); }
   if (state->save_failed) { DEBUG_TEXT("Save failed!"); }
  }
  
  if(0)
  {
   DEBUG_NAME("work cycles", params.frame.work_cycles);
   DEBUG_NAME("slider_cycle_counter", slider_cycle_counter);
   DEBUG_NAME("work us", params.frame.work_useconds);
  }
  
  //show_image_preview(strlit("G:/My Drive/Art/arm medial.jpg"));
  
  if(fui_is_active())
  {//-Show GUI for FUI (rolls right off the tongue)
   ImGuiWindowFlags flags = ImGuiWindowFlags_NoFocusOnAppearing;
   im_begin("FUI", 0, flags);
   
   Slider &slider = *fui_active_slider.data;
   Type_Info *type = get_slider_type_info(slider);
   if(equal(type, type_info_of(FUI_Line_Params)))
   {//-line params
    const char* items[] = { "radii", "lightness" };
    
    if(0)
    {//-Select attribute
     static int current_item = 0;
     ImGui::Combo("line attribute", &current_item, items, IM_ARRAYSIZE(items));
     i32 member_index = get_member_index_by_name(type, SCu8(items[current_item]));
     fui_active_slider.active_member_index = member_index;
    }
    
    {//-General info
     I_Struct_Member &active_member =
     type->members[fui_active_slider.active_member_index];
     im_text("Editing: %S", active_member.name);
    }
   }
   else
   {//-Other types
    String value_string = fui_push_active_slider_value(tmp);
    im_text("%S", value_string);
   }
   im_end();
  }

  {//-Replay panel (draw-as-data step 3, Q23)
   Replay_State &replay = state->replay;
   im_begin("Replay", 0, ImGuiWindowFlags_NoFocusOnAppearing);

   int mode = replay.display_replay ? 1 : 0;
   ImGui::RadioButton("code path", &mode, 0);
   ImGui::SameLine();
   ImGui::RadioButton("replay", &mode, 1);
   replay.display_replay = (mode == 1);

   if(ImGui::Button("Diff now")){ replay.diff_requested = true; }

   Replay_Diff_Result &diff = replay.last_diff;
   if(diff.valid)
   {
    if(diff.match)
    {
     im_text("match (%d vertices)", diff.code_vertex_count);
    }
    else
    {
     im_text("MISMATCH: code %d vs replay %d vertices",
             diff.code_vertex_count, diff.replay_vertex_count);
     if(diff.first_diff_vertex != -1)
     {
      im_text("first diff at vertex %d", diff.first_diff_vertex);
     }
     im_text("code loc: file %d [%d,%d)",
             diff.code_location.file.index,
             diff.code_location.range.min, diff.code_location.range.max);
     im_text("replay loc: file %d [%d,%d)",
             diff.replay_location.file.index,
             diff.replay_location.range.min, diff.replay_location.range.max);
    }
   }
   im_end();
  }

  {//-Preset settings panel (preset-rethink step 6): edits the ACTIVE preset's row.
   Preset_Settings &row = state->model.recordings.preset_settings[state->viewports[0].preset];
   im_begin("Preset settings", 0, ImGuiWindowFlags_NoFocusOnAppearing);
   im_text("preset %d", state->viewports[0].preset);
   ImGui::SliderInt("viz_level", &row.viz_level, 0, 2);
   ImGui::SliderInt("reference_image", &row.reference_image, -1, 4);
#define X(field) \
   { bool value = row.field; ImGui::Checkbox(#field, &value); row.field = value; }
   PRESET_BOOL_FIELDS(X)
#undef X
   im_end();
  }
 }

 if(driver_on)
 {
  {// NOTE(kv) Driver update
   {// ;clear_model
    Model *m = the_model;
    
    Model_Persistent persistent = m->persistent;
    Model_Recordings recordings = m->recordings;
    zero_struct(m);
    m->persistent = persistent;
    m->recordings = recordings;
    
    Arena *frame_arena = &state->frame_arena;
    arena_clear(frame_arena);
    //-
    m->frame_arena = frame_arena;
    
    init_dynamic(m->bones, frame_arena, 128);
    Bone null_bone = {.world_from_bone=mat4i_identity};
    push(&m->bones, null_bone);
    init_dynamic(m->bone_stack, frame_arena, 16);
    push(&m->bone_stack, m->bones.items+0);
    
    i32 vertex_cap = maximum(256, m->vertices.count);
    i32 entity_cap = maximum(256, m->primitives.count);
    init_dynamic(m->vertices, frame_arena, vertex_cap);

    // NOTE(kv) The recording lives on its own arena, cleared per capture run
    // (still every frame while the driver re-records each frame).
    Arena *recording_arena = &state->recording_arena;
    arena_clear(recording_arena);
    init_dynamic(m->primitives, recording_arena, entity_cap);
    init_dynamic(m->groups, recording_arena, 64);
    init_dynamic(m->group_stack.slots, recording_arena, 16);
    reset_capture();  // pushes the root group + root scope slot
    // NOTE(kv) Untagged groups always pass the live-visibility AND; the driver
    // re-publishes tagged slots (e.g. Vis_Skeleton) during its render below.
    m->vis_live[Vis_None] = true;
    {// NOTE(kv) Preset-toggle live visibility (plan-preset-rethink): published from
     // the main viewport's settings row, consumed by replay's vis_live re-AND.
     Preset_Settings &row = m->recordings.preset_settings[state->viewports[0].preset];
     m->vis_live[Vis_Eyeball]              = row.show_eyeball;
     m->vis_live[Vis_Loomis_Ball]          = row.show_loomis_ball;
     m->vis_live[Vis_Ref_Arm_Medial_Right] = row.show_arm_medial_right;
     m->vis_live[Vis_Ref_Arm_Back_Bone]    = row.show_arm_back_bone;
     m->vis_live[Vis_Ref_Arm_Profile_Left] = row.show_arm_profile_left;
     for_i32(ref_index, 0, Vis_Ref_Front_Last - Vis_Ref_Front_0 + 1)
     {
      m->vis_live[Vis_Ref_Front_0 + ref_index] = (row.reference_image == ref_index);
     }
    }
    
    {// NOTE(kv) Add persistent primitives to primitive list.
     // TODO(kv) We'll have to change this to support multiple viewports.
     set_count(&m->vertices, m->persistent.vertices.count);
     Vertex *src = m->persistent.vertices.items;
     isize size = sizeof(*src) * m->persistent.vertices.count;
     block_copy(m->vertices.items, src, size);
    }
   }
   
   v1 anim_time = state->looping_time;
   game_update_result.anim_time = anim_time;
   driver->driver_update(the_model, anim_time);
  }
  
  for_i32(index, 0, params.live_viewports.count)
  {//-Rendering
   Live_Viewport live_viewport = params.live_viewports[index];
   {//-Animate viewport
    i32 viewport_index = get_viewport_index(live_viewport.id);
    Viewport *viewport = &state->viewports[viewport_index];
    {// NOTE Camera animation
     Camera_Data *target  = &viewport->target_camera;
     Camera_Data *current = &viewport->camera;
     b32 animation_ended = animate_camera(current, target, dt);
     if(!animation_ended){ should_animate_next_frame = true; }
    }
   }
   
   {
    rect2 clip_box = live_viewport.clip_box;
    
    Render_Config *old_config = target_last_config(live_viewport.target);
    draw_set_clip(app, clip_box);
    
    v1 meter_to_pixel = default_meter_to_pixel;
    v1 pixel_to_meter = 1.f / meter_to_pixel;
    v2 clip_radius = pixel_to_meter*get_radius(clip_box);
    
    call_driver_render(state, app, live_viewport.target, live_viewport.id,
                       params.mouse, clip_radius);
    {
     Render_Config *config = draw_new_group(live_viewport.target);
     *config = *old_config; 
    }
   }
  }
 }
 
#if NOTEBOOK_MODE
 notebook_update(0);
#endif
 
 return{
  .should_animate_next_frame = should_animate_next_frame or state->replay.force_animate
                               or debug_channel_wants_animate,
  .game_commands             = game_commands,
 };
}
//~EOF
