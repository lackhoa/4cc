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
// NOTE(kv) Perf probe (rdtsc cycles of the last frame), printed by dump_state.
struct Debug_Cycles
{
 u32 frame;             // whole game_update
 u32 driver_render;     // one driver_render call (last viewport rendered)
 u32 render_character;  // painter->render_cycles
 u32 reference_mesh;    // painter->reference_mesh_cycles
 u32 driver_render_calls;  // renders per frame
};
global Debug_Cycles debug_cycles;

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

// NOTE(kv) Group_Vis names through Type_Info (replaced the GroupVisList X-macro and
// the group_vis_names[] table on 2026-09-12). Files store tags by name; the debug
// channel takes them by name (`export_group Vis_Nose`).
function String
group_vis_name(Group_Vis tag)
{// NOTE(kv) The real member comes first in Type_Info_Group_Vis, aliases
 // (Vis_Ref_Front_Last...) later, so the first value match is the canonical name.
 Type_Info &type = Type_Info_Group_Vis;
 for_i32(index, 0, type.enum_members.count)
 {
  I_Enum_Member &member = type.enum_members[index];
  if(member.value == tag){ return member.name; }
 }
 return strlit("Vis_None");
}
function b32
group_vis_from_name(String name, Group_Vis *out)
{// NOTE(kv) Only real tags resolve ([0, Group_Vis_Count)); Group_Vis_Count itself does not.
 Type_Info &type = Type_Info_Group_Vis;
 for_i32(index, 0, type.enum_members.count)
 {
  I_Enum_Member &member = type.enum_members[index];
  if(member.name == name and member.value >= 0 and member.value < Group_Vis_Count)
  {
   *out = cast(Group_Vis)member.value;
   return true;
  }
 }
 return false;
}

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
#include "ad_serialize_schema.cpp"
#include "ad_serialize_state.cpp"
#include "game_document_history.cpp"
#include "game_document.cpp"
#include "ad_serialize_slider_values.cpp"

function void
revert_from_autosave(Game_State *state, App *app){
 // IMPORTANT(kv) Overwrites edit history (state.txt is the periodic save).
 load_state_file(state);
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
 // NOTE(kv) First frame with the mouse already over a viewport: the driver hasn't
 // built the bones yet (init_dynamic(m->bones) runs at render), but the document is
 // loaded, so get_bone would deref an empty array. Nothing to pick yet anyway.
 if(m->bones.count == 0){ return; }
 if(not m->primitives_are_in_camera_space)
 {
  m->primitives_are_in_camera_space = 1;

  Bone_ID cur_bone = mk_bone_id(Bone_Invalid);
  mat4 camera_from_bone = {};
  mat4 camera_from_world = camera.cam_from_world;

  b32 cur_is_right = false;
  auto update_current_bone = [&](Bone_ID new_bone_id, b32 is_right=false) -> void
  {
   if(new_bone_id != cur_bone or is_right != cur_is_right)
   {
    // NOTE(kv) We only send primitives on the left (ref @should_send_model_data);
    // the document is replayed on both sides.
    Bone *bone = get_bone(new_bone_id, is_right);
    cur_bone = bone->id;
    cur_is_right = is_right;
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
  Recording &document = m->recordings.document;
  i32 document_count = document.captured ? 2*document.primitives.count : 0;
  init_dynamic(m->camera_primitives, m->frame_arena, m->primitives.count + document_count);
  // NOTE(kv) Don't pick what isn't drawn (Khoa 2026-09-11, the nose bridge line lost
  // its hover to the Line_Invisible side curve next to it: closest-hit wins in the pick
  // and the invisible strip sat nearer the camera). draw_bezier_rec keeps invisible
  // curves *selectable* on purpose, but that is for the cursor-on-code path, not the
  // mouse. Not covered: alignment_min culling (camera-dependent) and cam_vis groups.
  auto is_pickable = [&](Recorded_Primitive const &primitive, Recorded_Group const &group) -> b32
  {
   if(not group.params.painting or not m->vis_live[group.vis_tag]){ return false; }
   if(primitive.type == Primitive_Type_Curve and
      HasFlag(group.params.line.flags, Line_Invisible)){ return false; }
   return true;
  };
  auto convert_primitive = [&](Recorded_Primitive primitive, Bone_ID bone_id, b32 is_right)
  {
   apply_shape_key(primitive);  // NOTE(kv) picking/hot-test against what's on screen
   update_current_bone(bone_id, is_right);
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
      mat4vert(camera_from_bone, &primitive.poly3.points[i].v);
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
     mat4vert(camera_from_bone, &primitive.disk.center.v);
    }break;
   }
   push(&m->camera_primitives, primitive);
  };
  for_i32(iprim, 0, m->primitives.count)
  {
   Recorded_Primitive &primitive = m->primitives[iprim];
   Recorded_Group &group = m->groups[primitive.group_index];
   if(not is_pickable(primitive, group)){ continue; }
   convert_primitive(primitive, group.bone_id, false);
  }
  // NOTE(kv) Document primitives are hit-testable too, on both sides (they're
  // replayed left+right); their locations are the document variant so a hit
  // highlights the replayed draw instead of jumping to code.
  for_i32(iprim, 0, document.primitives.count * (document.captured ? 1 : 0))
  {
   Recorded_Primitive primitive = document.primitives[iprim];
   Recorded_Group &group = document.groups[primitive.group_index];
   if(not is_pickable(primitive, group)){ continue; }
   resolve_vertices(document, primitive);
   primitive.location = document_location(iprim, false);
   convert_primitive(primitive, group.bone_id, false);
   b32 midline = (primitive.type == Primitive_Type_Curve and primitive.curve.midline);
   if(not group.one_sided and not midline)
   {
    primitive.location = document_location(iprim, true);
    convert_primitive(primitive, group.bone_id, true);
   }
  }
 }
}
#include "game_reference_gizmo.cpp"
#include "game_document_edit.cpp"
#include "game_curve_patch.cpp"
#include "game_document_line_tool.cpp"

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
   b32 orthographic = camera_is_orthographic(state->orthographic, painter->show_grid, camera);
   painter->clip_from_world = get_clip_from_world(camera, clip_radius, orthographic);
  }
  painter->target       = target;
  painter->camera       = camera;
  painter->sending_data = state->sending_data;
  painter->reference_mode = state->reference_mode;
  painter->hot_locations = state->transient->hot_locations;
  painter->selected_locations = state->transient->selected_locations;
  painter->active_shape_location = {};
  if(fui_is_active())
  {// NOTE(kv) Curve handles show only while d0/d3 is the active member (@draw_curve).
   Slider &slider = *fui_active_slider.data;
   Type_Info *type = get_slider_type_info(slider);
   if(is_struct(type))
   {
    String member_name = type->members[fui_active_slider.active_member_index].name;
    if(member_name == strcode(d0) or member_name == strcode(d3))
    {
     painter->active_shape_location = slider.location;
    }
   }
  }
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
   {
    u64 cycle_start = __rdtsc();
    driver->driver_render(tmp, painter);
    debug_cycles.driver_render    = u32(__rdtsc() - cycle_start);
    debug_cycles.render_character = painter->render_cycles;
    debug_cycles.reference_mesh   = painter->reference_mesh_cycles;
    debug_cycles.driver_render_calls++;
    // NOTE(kv) The painter is a stack local of this function; the gizmo's hit test and
    // the debug channel run outside it, so the skull radius is kept on the state.
    if(painter->reference_mesh_obj_radius > 0.f)
    {
     state->reference_mesh_obj_radius = painter->reference_mesh_obj_radius;
     state->reference_mesh_obj_center = painter->reference_mesh_obj_center;
    }
   }
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

   Recording &document = the_model->recordings.document;
   if(document.captured)
   {// NOTE(kv) The document is drawing, not debug state: replayed in every viewport,
    // rendering on regardless of mode, outside the diff tees (the diff compares the
    // code path against ITS recording only). Drawn after driver_render; the depth
    // test settles pixel order. Left then right (Q94 mirror).
    replay_recording(document, /*is_right*/false);
    replay_recording(document, /*is_right*/true);
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

  document_hover_draw(state, camera);  // NOTE(kv) control points of the hovered document item (every viewport)

  if(viewport_id == 1)
  {
   draw_reference_edit_gizmo(state, camera);
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

  {// NOTE: Load state
   state->data_load_arena = make_arena();
   // NOTE(kv) The text state reader walks Type_Info, which game_reload (below) only
   // sets up AFTER this point; the tables live in thread_permanent_arena (made above).
   make_all_type_info();
   seed_preset_settings(&state->model.recordings);
   if(not load_state_file(state))
   {// TODO(kv) One-off: pull autosave.ad + recording.ad presets into state.txt
    migrate_state_from_binary_files(state);
   }
   load_recording_file(state);
   load_document_file(state);
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
  init_dynamic(transient->selected_locations, dll_arena);
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
game_save(Game_State *state, App *app)
{// NOTE(kv) state.txt (text, git-friendly -- no backup ring anymore) + recording.ad.
 b32 ok = save_state_file(state);
 if(ok){
  vim_set_bottom_text(strlit("Saved game state!"));
  // NOTE(kv) Q51: recording.ad rides the same cadence as state.txt; its own
  // failure only logs -- the state save above already succeeded.
  save_recording_file(state);
 }else{
  vim_set_bottom_text(strlit("failed to save state"));
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
function void update_pan(Camera_Data *cam, Game_Input *input);
global v1 CAMERA_DRAG_ORBIT_PX_PER_STEP = 80.f;  // NOTE(kv) one 1/24-turn cell per this many px
global v1 CAMERA_DRAG_PAN_PX_PER_STEP   = 40.f;  // NOTE(kv) one (scaled) pan step per this many px
global v1 CAMERA_DRAG_PAN_STEP_SCALE = 0.5f;  // NOTE(kv) a drag pan step is this fraction of a keyboard pan step

global v1 CAMERA_DRAG_TAP_PX = 4.f;  // NOTE(kv) a press that strays less than this is a click, not a drag

function void
camera_drag_press(Game_State *state, i32 viewport_index, v2 mouse_px, b32 pan, b32 middle,
                  b32 deselect_on_tap)
{
 Camera_Drag &drag = state->camera_drag;
 drag = {};
 drag.active = true;
 drag.pan = pan;
 drag.middle = middle;
 drag.viewport_index = viewport_index;
 drag.last_px = mouse_px;
 drag.press_px = mouse_px;
 drag.deselect_on_tap = deselect_on_tap;
}

// NOTE(kv) Called every frame while the left button is held: turn the pixel delta
// into whole orbit cells / pan steps on the target camera (same math as the keys).
function void
camera_drag_move(Game_State *state, v2 mouse_px)
{
 Camera_Drag &drag = state->camera_drag;
 if(not drag.active){ return; }
 Camera_Data *cam = get_target_camera(state, drag.viewport_index);
 if(lengthof(V3(mouse_px - drag.press_px, 0)) > CAMERA_DRAG_TAP_PX)
 {
  drag.moved = true;
 }
 v2 acc = drag.remainder_px + (mouse_px - drag.last_px);
 drag.last_px = mouse_px;
 // NOTE(kv) Truncate, not round: a step fires only after a full px_per_step.
 v1 px_per_step = drag.pan ? CAMERA_DRAG_PAN_PX_PER_STEP : CAMERA_DRAG_ORBIT_PX_PER_STEP;
 v2 steps = {v1(i32(acc.x / px_per_step)), v1(i32(acc.y / px_per_step))};
 drag.remainder_px = acc - px_per_step * steps;
 if(steps == v2{}){ return; }
 // NOTE(kv) Same convention as the tablet (gestures.ts): the content follows the mouse.
 // Screen y grows downward. Pan: drag right = world moves right = pivot moves left.
 // Orbit: drag right = yaw so the near side moves right (opposite of the L key),
 // drag down = pitch so the near side moves down.
 Key_Direction dir = {};
 dir.new_keypress = true;
 if(drag.pan)
 {
  dir.dir.xy = CAMERA_DRAG_PAN_STEP_SCALE * V2(-steps.x, steps.y);
  Game_Input input = {};
  input.direction = dir;
  update_pan(cam, &input);
 }
 else
 {
  dir.dir.xy = V2(-steps.x, steps.y);
  update_orbit(cam, dir);
 }
}

function void
camera_drag_release(Game_State *state)
{
 Camera_Drag &drag = state->camera_drag;
 if(drag.deselect_on_tap and not drag.moved)
 {// NOTE(kv) Q5 of plan-active-primitive-delete-key + plan-selection-followups Q1: a
  // click on nothing deselects, a drag (orbit/pan) keeps the selection.
  state->document_selection.count = 0;
 }
 drag.active = false;
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

 // NOTE(kv) Relative epsilon: det scales with |A||B| (ray_dir is unit), so an
 // absolute 1e-5 threw away every triangle of a millimeter-sized fill (curve patches).
 if(absolute(det) < 1e-5f * lengthof(A) * lengthof(B))
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

// NOTE(kv) plan-pick-curves-over-patches: a curve is picked by SCREEN distance (px from
// the cursor to the projected polyline), a fill (patch, poly3, dual bezier) by ray depth,
// and a curve within `curve_pick_radius_px` always beats a fill. The old 3 mm world-space
// ribbon lost to any patch bulging toward the camera, and was hair-thin zoomed out.
global const v1 curve_pick_radius_px = 8.f;

function v1
distance_squared_point_to_segment_2d(v2 p, v2 a, v2 b)
{
 v2 ab = b - a;
 v1 len_sq = dot(ab, ab);
 v1 t = (len_sq > 0.f) ? clamp01(dot(p - a, ab) / len_sq) : 0.f;
 v2 q = a + t*ab;
 return dot(p - q, p - q);
}

function Location
get_primitive_hit_by_mouse(Game_State *state, Live_Viewport *mouse_viewport,
                           i2 params_mouse_p)
{
 Location hot_location = {};

 if(mouse_viewport)
 {
  // NOTE(kv) plan-screen-projection-unification: the one view mapping, built once. Camera
  // from viewports[0] (TODO(kv) multi-camera is a no-win, Q9), primitives cached in its
  // camera space.
  Screen_Projection_Data proj = mk_screen_projection_data(state, mouse_viewport);
  convert_primitives_to_camera_space(proj.camera);
  v2 mouse_px = V2(params_mouse_p) - proj.center;
  b32 fill_only = state->model.recordings.preset_settings[state->viewports[0].preset].fill_only_picking;

  v1 min_t = INFINITY;

  // NOTE(kv) plan-curve-selection-precision: picking MUST use the same projection the
  // renderer uses. In orthographic mode the render draws a parallel projection; a
  // perspective pick ray would grab a primitive other than the one under the visual cursor
  // (selecting a specific curve was near-impossible). The mouse ray and the curve-point
  // projection below are the two halves of the one shared screen mapping.
  Screen_Ray ray = screen_ray(proj, mouse_px);
  Location  curve_hit = {};
  v1        curve_hit_dsq = curve_pick_radius_px * curve_pick_radius_px;
  b32       vertex_hit = false;

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
     v1 hit_t = hit_test_ray_triangle(ray.P, ray.dir, expand3(triangle));
     if(hit_t < min_t)
     {
      // TODO(kv) ...
      Vertex_Info info = get_vertex_info(vertex);
      hot_location = info.location;
      min_t = hit_t;
      vertex_hit = true;
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
     {// NOTE(kv) Screen-distance pick (plan-pick-curves-over-patches Q2): nearest
      // projected polyline within `curve_pick_radius_px` wins, no triangles involved.
      tvert *curve = primitive.curve.bezier;
      const i32 test_segment_count = 24;
      v1 test_t_interval = 1.0f / v1(test_segment_count);
      v2 A_px;
      b32 A_ok = px_from_camera(proj, curve[0].v, &A_px);
      for_i32(si, 0, test_segment_count)
      {
       v1 B_t = test_t_interval * v1(si+1);
       v2 B_px;
       b32 B_ok = px_from_camera(proj, bezier_sample(curve, B_t), &B_px);
       if(A_ok and B_ok)
       {
        v1 dsq = distance_squared_point_to_segment_2d(mouse_px, A_px, B_px);
        if(dsq < curve_hit_dsq)
        {
         curve_hit = primitive.location;
         curve_hit_dsq = dsq;
        }
       }
       A_px = B_px;
       A_ok = B_ok;
      }
     }
    }break;

    case Primitive_Type_Poly3:
    {//-Projection onto the curve
     tvert const (&p)[3] = primitive.poly3.points;
     push(&triangles, Poly3{p[0].v, p[1].v, p[2].v});
    }break;

    case Primitive_Type_Curve_Patch:
    {// NOTE(kv) Document-only: evaluate the surface from the referenced curves.
     if(is_document_location(primitive.location))
     {
      Recording &document = state->model.recordings.document;
      push_curve_patch_hit_triangles(tmp, &triangles, document,
                                     document.primitives[document_primitive_index(primitive.location)],
                                     document_location_is_right(primitive.location),
                                     proj.camera.cam_from_world);
     }
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
    v1 hit_t = hit_test_ray_triangle(ray.P, ray.dir, expand3(triangle));
    if(hit_t < min_t)
    {
     hot_location = primitive.location;
     min_t = hit_t;
     vertex_hit = false;
    }
   }
  }// NOTE Loop over primitives

  // NOTE(kv) Priority (plan-pick-curves-over-patches Q1): a vertex hit by the ray keeps
  // its win (it is a screen-facing quad, already a "line-like" pick); else a curve within
  // the pixel radius beats every fill hit regardless of depth; else the nearest fill.
  if(not vertex_hit and curve_hit_dsq < curve_pick_radius_px * curve_pick_radius_px)
  {
   hot_location = curve_hit;
  }
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
 u64 frame_cycle_start = __rdtsc();
 debug_cycles.driver_render_calls = 0;
 Scratch_Block tmp;
 update_game_config();
 Game_State *state = params.state;
 App *app = params.app;
 debug_channel_update(state, app);
 maybe_reload_slider_values_files(state);
 b32 should_animate_next_frame = false;
 arena_clear(&state->frame_arena);
 //-
 // NOTE(kv) Never the frame arena: the editor stashes this in received_game_commands
 // and only walks it when the command lister opens, a frame or more later.
 sarray(String) game_commands = {};

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
  // NOTE(kv) Agent mode (-debug-cmd): the lone viewport is always "focused", which
  // would redraw every frame forever; it polls on a timer instead (poll_again_in_ms).
  if((viewport_focused and not debug_channel_enabled) or fui_is_active())
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
   // NOTE(kv) The agent instance (-debug-cmd) shares autosave.ad with the live
   // editor; set_camera etc. must not clobber the user's saved view.
   b32 should_autosave = seconds_since_last_autosave == 0 and not debug_channel_enabled;
   if(should_autosave)
   {
    game_save(state, app);
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

  if(debug_channel_mouse_active)
  {// NOTE(kv) Agent mode: the channel's virtual mouse replaces the real one (position
   // and left button), so picking and document editing run the same code.
   params.mouse.p = debug_channel_mouse_p;
   params.mouse.left = b8(debug_channel_mouse_left);
   params.mouse.press_left = b8(debug_channel_mouse_press_pending);
   params.mouse.release_left = b8(debug_channel_mouse_release_pending);
   params.mouse.middle = b8(debug_channel_mouse_middle);
   debug_channel_mouse_press_pending = false;
   debug_channel_mouse_release_pending = false;
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

  {
   b32 shift = ((params.input.active_mods & Key_Mod_Sft) != 0 or debug_channel_mouse_shift);
   update_reference_edit(state, params.mouse, shift, mouse_viewport);
  }

  // NOTE(kv) Agent mode (-debug-cmd): the mouse sits wherever the user left it, so
  // hover-highlighting would just paint random red fills into every screenshot.
  // Reference edit mode owns the mouse: the image is the only pickable thing (plan Q5).
  if(state->line_tool.active)
  {// NOTE(kv) A line-tool drag owns the mouse: nothing is hot, the curve follows the pen.
   if(params.mouse.left)
   {
    line_tool_move(state, mouse_viewport, V2(params.mouse.p));
    should_animate_next_frame = true;
   }
   if(params.mouse.release_left or not params.mouse.left)
   {
    line_tool_release(state);
   }
  }
  else if(state->document_edit.active)
  {// NOTE(kv) A document drag owns the mouse: keep the grabbed item hot, no re-picking.
   hot_location = state->document_edit.location;
   if(params.mouse.left)
   {
    document_edit_move(state, mouse_viewport, V2(params.mouse.p));
    should_animate_next_frame = true;
   }
   if(params.mouse.release_left or not params.mouse.left)
   {
    document_edit_release(state);
   }
  }
  else if((not debug_channel_enabled or debug_channel_mouse_active) and not state->reference_edit.active)
  {
   hot_location = get_primitive_hit_by_mouse(state, mouse_viewport, params.mouse.p);
  }
  debug_channel_last_hot = hot_location;
  document_hover_update(state, mouse_viewport, V2(params.mouse.p));
  if(mouse_viewport){ debug_channel_mouse_viewport_box = mouse_viewport->clip_box; }

  if(state->camera_drag.active)
  {// NOTE(kv) Mouse camera drag (orbit / alt-pan / middle-pan) owns the mouse until release.
   camera_drag_move(state, V2(params.mouse.p));
   b32 released = (state->camera_drag.middle ?
                   not params.mouse.middle :
                   (params.mouse.release_left or not params.mouse.left));
   if(released){ camera_drag_release(state); }
  }
  else if(params.mouse.middle and mouse_viewport and not state->document_edit.active and not state->line_tool.active)
  {// NOTE(kv) Middle button held: pan drag, regardless of what is hot.
   camera_drag_press(state, mouse_viewport->id - 1, V2(params.mouse.p), true, true, false);
  }

  if(params.mouse.press_left and not state->document_edit.active and not state->camera_drag.active)
  {// NOTE(kv) Hot code item: jump to code. Near a control point of a SELECTED document
   // primitive: start a drag (Q6, explicit selection since 2026-09-13). Hot unselected
   // document item: select it, no drag. Nothing hot: camera drag (orbit, alt = pan).
   // Line tool armed: the press starts a new curve whatever is hot (a hot vertex is a
   // snap target, not a drag).
   b32 shift = ((params.input.active_mods & Key_Mod_Sft) != 0 or debug_channel_mouse_shift);
   Document_Pick pick = {};
   v1 pick_dist = INFINITY;
   b32 near_selected_point = (not shift and
                              document_pick_nearest(state, mouse_viewport, V2(params.mouse.p), &pick, &pick_dist) and
                              pick_dist <= document_pick_radius_px);
   if(state->line_tool.armed and mouse_viewport)
   {
    line_tool_press(state, mouse_viewport, V2(params.mouse.p));
   }
   else if(near_selected_point)
   {// NOTE(kv) Control points of the selection win over whatever is hot: a shared
    // vertex of two chained curves is edited through the curve you selected, and a
    // handle floating off the surface is grabbable with nothing hot under it.
    // Ctrl at press = free handle drag (plan-handle-drag-modes Q2), sampled once here
    // like shift/alt.
    b32 ctrl = ((params.input.active_mods & Key_Mod_Ctl) != 0 or debug_channel_mouse_ctrl);
    document_edit_press(state, mouse_viewport, V2(params.mouse.p), pick, ctrl);
   }
   else if(not is_valid(hot_location) and mouse_viewport and
           state->reference_edit.drag == Reference_Drag_None)
   {// NOTE(kv) A reference drag (image or skull) that started this frame owns the press;
    // without this check the orbit ran under the gizmo drag (found 2026-09-12).
    b32 alt = ((params.input.active_mods & Key_Mod_Alt) != 0 or debug_channel_mouse_alt);
    // NOTE(kv) Q5: a click on nothing deselects -- decided on release, once we know it
    // stayed a click (plan-selection-followups Q1). Shift-click on nothing adds
    // nothing and keeps the selection (Q2).
    b32 deselect_on_tap = (not shift and not alt);
    camera_drag_press(state, mouse_viewport->id - 1, V2(params.mouse.p), alt, false,
                      deselect_on_tap);
   }
   else if(is_document_location(hot_location))
   {
    i32 hot_prim = document_primitive_index(hot_location);
    if(shift)
    {// NOTE(kv) Q8: shift-click toggles the hot curve in the patch selection, no drag.
     document_selection_toggle(state, hot_prim);
    }
    else if(document_selection_contains(state, hot_prim) and
            state->model.recordings.document.primitives[hot_prim].type == Primitive_Type_Curve)
    {// NOTE(kv) plan-selection-followups Q4, after the tablet: pressing the already
     // selected curve away from its control points drags the whole stroke.
     document_edit_press_stroke(state, mouse_viewport, V2(params.mouse.p),
                                hot_prim, document_location_is_right(hot_location));
    }
    else
    {// NOTE(kv) Q1 (revised 2026-09-13): the clicked curve/patch becomes the sole
     // selection; its control points become grabbable on the NEXT press.
     document_selection_set(state, hot_prim);
    }
   }
   else if(is_valid(hot_location))
   {
    g_jump_to_pos(app, resolve_location(hot_location).min);
   }
  }

  {// NOTE(kv) Right-click menu
   if(params.mouse.press_right)
   {
    state->document_selection.menu_hot = hot_location;
    ImGui::OpenPopup("right_click_popup");
   }

   if(ImGui::BeginPopup("right_click_popup"))
   {
    {// NOTE(kv) Document patches (Q8): the selection was built by shift-clicks.
     Document_Selection &sel = state->document_selection;
     Recording &doc = state->model.recordings.document;
     b32 menu_hot_is_patch = (is_document_location(sel.menu_hot) and
                              document_primitive_index(sel.menu_hot) < doc.primitives.count and
                              doc.primitives[document_primitive_index(sel.menu_hot)].type == Primitive_Type_Curve_Patch);
     if(sel.count >= 2 and document_selection_all_curves(state))
     {
      char label[64];
      snprintf(label, sizeof(label), "Make patch from selection (%d curves)", sel.count);
      if(ImGui::Selectable(label)){ document_make_patch(state, sel.prim_index, sel.count); }
     }
     b32 menu_hot_is_curve = (is_document_location(sel.menu_hot) and
                              document_primitive_index(sel.menu_hot) < doc.primitives.count and
                              doc.primitives[document_primitive_index(sel.menu_hot)].type == Primitive_Type_Curve);
     if(menu_hot_is_patch)
     {
      if(ImGui::Selectable("Delete patch")){ document_delete_patch(state, document_primitive_index(sel.menu_hot)); }
     }
     if(menu_hot_is_curve)
     {
      if(ImGui::Selectable("Delete curve")){ document_delete_curve(state, document_primitive_index(sel.menu_hot)); }
     }
     if(sel.count >= 2 or menu_hot_is_patch or menu_hot_is_curve){ ImGui::Separator(); }
    }
    {// NOTE(kv) Line tool (game_document_line_tool.cpp): arm, then drag a curve; a
     // click without a drag disarms.
     Line_Tool_State &tool = state->line_tool;
     if(ImGui::Selectable(tool.armed ? "Cancel line tool" : "Add line"))
     {
      b32 arm = not tool.armed;
      line_tool_reset(state);
      tool.armed = arm;
     }
     ImGui::Separator();
    }
    {// NOTE(kv) Reference edit mode lives here rather than on a key: placing a
     // reference is rare enough that a binding would never be remembered (plan Q6).
     Reference_Edit_State &edit = state->reference_edit;
     Stringz reference_filename = {};
     Reference_Placement *placement = get_reference_placement(state, &reference_filename);
     Reference_Mesh_Placement *mesh_placement = get_reference_mesh_placement(state);
     if(placement or mesh_placement)
     {
      if(ImGui::Selectable(edit.active ? "Stop editing reference" : "Edit reference"))
      {
       edit.active = not edit.active;
       edit.drag = Reference_Drag_None;
      }
      if(edit.active and placement)
      {
       if(ImGui::Selectable("Mirror reference"))
       {
        placement->x_axis = -placement->x_axis;
        save_slider_values_file(state, /*is_driver*/1);
       }
       ImGui::SetNextItemWidth(120.f);
       if(ImGui::SliderFloat("Alpha", &placement->alpha, 0.f, 1.f))
       {
        save_slider_values_file(state, /*is_driver*/1);
       }
      }
      if(edit.active and mesh_placement)
      {// NOTE(kv) Skull roll (plan-reference-skull Q7): no drag gesture left for it, so
       // it's a pair of menu nudges. rotation is in turns.
       const v1 roll_step = 5.f / 360.f;
       if(ImGui::Selectable("Roll skull +5 deg"))
       {
        v3 rotation = mesh_placement->rotation;
        rotation.z += roll_step;
        set_reference_mesh_scale_rotation(state, *mesh_placement, mesh_placement->scale, rotation);
        save_slider_values_file(state, /*is_driver*/1);
       }
       if(ImGui::Selectable("Roll skull -5 deg"))
       {
        v3 rotation = mesh_placement->rotation;
        rotation.z -= roll_step;
        set_reference_mesh_scale_rotation(state, *mesh_placement, mesh_placement->scale, rotation);
        save_slider_values_file(state, /*is_driver*/1);
       }
      }
      ImGui::Separator();
     }
    }

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
  {// NOTE(kv) Selected curves count as hot on both sides, like pins (visible, control
   // points), and also go to selected_locations so they draw in selection_color.
   Document_Selection &sel = state->document_selection;
   transient->selected_locations.count = 0;
   for_i32(i, 0, sel.count)
   {
    for_i32(side, 0, 2)
    {
     Location location = document_location(sel.prim_index[i], side == 1);
     push_unique(&transient->hot_locations, location);
     push_unique(&transient->selected_locations, location);
    }
   }
  }

  {//-Game commands
   // NOTE(kv) Commands are things that the editor send to the game.

   {//-Serve commands
    darray(Game_Command) &queue = state->command_queue;
    for_i32(command_index,0,queue.count)
    {
     Game_Command command = queue[command_index];
#define MATCH(NAME)    command.name == strlit(NAME)
     if(0);
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
     {// NOTE(kv) Clears the active preset's reference scene (the preset itself stays).
      state->model.recordings.preset_settings[update_viewport->preset].scene = Scene_None;
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
    local_persist String names[] = {
     strlit("revert"),
     strlit("pin"),
     strlit("clear_pin"),
     strlit("clear_preset"),
    };
    game_commands = to_sarray(names);
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
      // NOTE(kv) Numpad digits too (the platform only reports them as NumPad codes
      // with Num Lock on; with it off they arrive as Home/End/arrows).
      case Key_Code_NumPad0: case Key_Code_NumPad1: case Key_Code_NumPad2: case Key_Code_NumPad3: case Key_Code_NumPad4:
      case Key_Code_NumPad5: case Key_Code_NumPad6: case Key_Code_NumPad7: case Key_Code_NumPad8: case Key_Code_NumPad9:
      {
       i32 preset = code - Key_Code_NumPad0;
       game_set_preset(state, update_viewport_id, preset);
      }break;

      case Key_Code_Space: { game_last_preset(state, update_viewport_id); }break;
      case Key_Code_M:     { state->kb_cursor.on = true; } break;
      case Key_Code_Escape:{ state->kb_cursor.on = false; line_tool_reset(state); state->document_selection.count = 0; }break;
      // NOTE(kv) Delete the selection (plan-active-primitive-delete-key.md Q2/Q3).
      case Key_Code_Delete: case Key_Code_Backspace:{ document_delete_selection(state); }break;

      case C|Key_Code_Return:{ game_save(state, app); }break;
      // NOTE(kv) Document undo/redo (plan-document-undo-redo Q5).
      case C|Key_Code_Z:{ history_undo(state); }break;
      case C|S|Key_Code_Z: case C|Key_Code_Y:{ history_redo(state); }break;
      case Key_Code_A:
      {
       snap_camera(cam_data, update_viewport);
      }break;
      // NOTE(kv) Independent of edit modes on purpose: Khoa flips it mid-drag to judge a
      // skull placement against the drawing (plan-reference-toggle-two-states Q4).
      case Key_Code_Q:{ state->reference_mode = (state->reference_mode == Reference_On ? Reference_Off : Reference_On); }break;
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
        if(is_valid(hot_location) and not is_document_location(hot_location))
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
       fui_cycle_active_member();
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
     case Key_Code_Escape:
     {
      state->reference_edit.active = false;
      state->reference_edit.drag = Reference_Drag_None;
     }break;

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
          active_preset_row(state).scene = object.preset;

          Reference_Scene_Data preset_data =
          driver->driver_get_scene_data(object.preset);

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
   if (state->load_failed) { DEBUG_TEXT("state.txt REJECTED (syntax) -- see log"); }
   if (state->save_failed) { DEBUG_TEXT("Save failed!"); }
   if (state->recording_load_failed) { DEBUG_TEXT("recording.ad REJECTED (version/corrupt) -- see log"); }
   if (state->document_load_failed)  { DEBUG_TEXT("driver.document.ad REJECTED (version/corrupt) -- see log"); }
   if (state->document_history.status_frames > 0)
   {// NOTE(kv) "undo: move vertex 12 (nose)" for a couple of seconds after Ctrl+Z.
    state->document_history.status_frames--;
    DEBUG_TEXT(state->document_history.status);
   }
   {// NOTE(kv) Which document control point the mouse is on: "vertex 43 (Vis_Cheek) ..."
    local_persist char hover_label[128];
    if(document_hover_label(hover_label, sizeof(hover_label), state->model.recordings.document) > 0)
    {
     DEBUG_TEXT(hover_label);
    }
   }
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
   if(is_struct(type))
   {//-Struct sliders (FUI_Line_Params, Curve): pick a member, show its value
    i32 member_count = type->members.count;
    const char **items = push_array(tmp, const char *, member_count);
    for_i32(index, 0, member_count)
    {
     items[index] = (const char *)push_stringf(tmp, "%S", type->members[index].name).str;
    }
    i32 &active_index = fui_active_slider.active_member_index;
    ImGui::Combo("member", &active_index, items, member_count);

    I_Struct_Member &active_member = type->members[active_index];
    Printer printer = make_printer_buffer(tmp, 128);
    print_code(printer, active_member.type, (u8 *)slider.value + active_member.offset, true);
    im_text("%s = %S", items[active_index], printer_get_string(printer));
   }
   else
   {//-Other types
    String value_string = fui_push_active_slider_value(tmp);
    im_text("%S", value_string);
   }
   im_end();
  }

  // NOTE(kv) Agent mode has no mouse user; the debug channel sets these knobs and
  // the panels would only clutter screenshots.
  if(not debug_channel_enabled)
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

  if(not debug_channel_enabled)
  {//-Presets panel (plan-settings-ui): list on the left, the ACTIVE preset's fields on the right.
   Model_Recordings &rec = state->model.recordings;
   i32 active = state->viewports[0].preset;
   im_begin("Presets", 0, ImGuiWindowFlags_NoFocusOnAppearing);
   {//-Global (state.txt flags that aren't per-preset)
    ImGui::SeparatorText("Global");
    { bool value = state->orthographic;         ImGui::Checkbox("orthographic",         &value); state->orthographic = value; }
    ImGui::SameLine();
    {
     bool value = (state->reference_mode == Reference_On);
     ImGui::Checkbox("reference on top (Q)", &value);
     state->reference_mode = (value ? Reference_On : Reference_Off);
    }
    ImGui::SeparatorText("Presets");
   }
   {//-List
    ImGui::BeginChild("preset_list", ImVec2(180, 260), true);
    for_i32(index, 0, rec.preset_count)
    {
     Preset_Settings &it = rec.preset_settings[index];
     char label[PRESET_NAME_CAP+16];
     snprintf(label, sizeof(label), "%d %s##preset%d", index, it.name, index);
     if(ImGui::Selectable(label, index == active)){ game_set_preset(state, 1, index); }
    }
    ImGui::EndChild();
    if(ImGui::Button("add")){ preset_add(state, active); }
    ImGui::SameLine();
    if(ImGui::Button("delete")){ preset_delete(state, active); }
    ImGui::SameLine();
    if(ImGui::Button("up")){ preset_swap(state, active, active-1); }
    ImGui::SameLine();
    if(ImGui::Button("down")){ preset_swap(state, active, active+1); }
   }
   ImGui::SameLine();
   {//-Active preset
    active = state->viewports[0].preset;  // the buttons above may have moved it
    Preset_Settings &row = rec.preset_settings[active];
    ImGui::BeginGroup();
    ImGui::InputText("name", row.name, sizeof(row.name));
    ImGui::SeparatorText("Display");
    ImGui::SliderInt("viz_level", &row.viz_level, 0, 2);
    // NOTE(kv) Checkboxes come from Preset_Settings' reflection: the groups below name
    // the b32 members they want; whatever b32 member is left over lands in "Other", so
    // a new flag in the .kh shows up here without touching this panel.
    Type_Info *preset_type = &Type_Info_Preset_Settings;
    b32 shown[64] = {};
    kv_assert(preset_type->members.count <= alen(shown));
    auto preset_checkbox = [&](i32 member_index)
    {
     I_Struct_Member &member = preset_type->members[member_index];
     b32 *field = cast(b32 *)(cast(u8 *)&row + member.offset);
     bool value = *field;
     ImGui::Checkbox((const char *)member.name.data, &value);
     *field = value;
     shown[member_index] = true;
    };
#define X(field) \
{ preset_checkbox(get_member_index_by_name(preset_type, strlit(#field))); }
    X(show_eyeball) X(show_loomis_ball) X(show_grid) X(hide_hair) X(ignore_radii) X(ignore_alignment_min)
    ImGui::SeparatorText("Reference images");
    {// NOTE(kv) Scene combo from the enum's reflection, so new scenes show up for free.
     Type_Info *scene_type = &Type_Info_Reference_Scene;
     const char *scene_name = "?";
     for_i32(i, 0, scene_type->enum_members.count)
     {
      if(scene_type->enum_members[i].value == (i32)row.scene){ scene_name = (const char *)scene_type->enum_members[i].name.data; }
     }
     if(ImGui::BeginCombo("scene", scene_name))
     {
      for_i32(i, 0, scene_type->enum_members.count)
      {
       I_Enum_Member &member = scene_type->enum_members[i];
       if(ImGui::Selectable((const char *)member.name.data, member.value == (i32)row.scene))
       {
        row.scene = cast(Reference_Scene)member.value;
       }
      }
      ImGui::EndCombo();
     }
    }
    ImGui::SliderInt("reference_image", &row.reference_image, -1, 4);
    X(show_arm_medial_right) X(show_arm_back_bone) X(show_arm_profile_left)
    ImGui::SeparatorText("Picking");
    X(fill_only_picking)
#undef X
    {//-Other: b32 members no group above claimed
     b32 header_done = false;
     for_i32(mi, 0, preset_type->members.count)
     {
      I_Struct_Member &member = preset_type->members[mi];
      if(member.type == &Type_Info_b32 and not shown[mi])
      {
       if(not header_done){ ImGui::SeparatorText("Other"); header_done = true; }
       preset_checkbox(mi);
      }
     }
    }
    ImGui::EndGroup();
   }
   im_end();

   {//-History panel (plan-document-undo-redo Q2): one row per document edit, oldest
    // first, `>` marks the state the document equals, rows past it are undone (grey);
    // click a row = jump there.
    Document_History &history = state->document_history;
    im_begin("History", 0, ImGuiWindowFlags_NoFocusOnAppearing);
    ImGui::BeginDisabled(history.position <= 0);
    if(ImGui::Button("undo (^Z)")){ history_undo(state); }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(history.position >= history.count - 1);
    if(ImGui::Button("redo (^Y)")){ history_redo(state); }
    ImGui::EndDisabled();
    ImGui::BeginChild("history_list", ImVec2(300, 300), true);
    i32 jump_to = -1;
    for_i32(index, 0, history.count)
    {
     char text[128];
     document_action_text(text, sizeof(text), history.entries[index].action,
                          state->model.recordings.document);
     char label[160];
     snprintf(label, sizeof(label), "%c %s##history%d", (index == history.position) ? '>' : ' ', text, index);
     bool undone = (index > history.position);
     if(undone){ ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]); }
     if(ImGui::Selectable(label, index == history.position)){ jump_to = index; }
     if(undone){ ImGui::PopStyleColor(); }
    }
    ImGui::EndChild();
    if(jump_to != -1){ history_jump(state, jump_to); }
    im_end();
   }

   {//-Selection panel (plan-focus-radii-midline Q3/Q4/Q9): the selected curves' width
    // profile (uniform width + end taper, or the raw v4) and the midline flag. The
    // first selected curve supplies the displayed values; edits go to all of them.
    Recording &doc = state->model.recordings.document;
    i32 curves[Document_Selection_Cap];
    i32 curve_count = document_selection_curve_indices(state, curves);
    im_begin("Selection", 0, ImGuiWindowFlags_NoFocusOnAppearing);
    {// NOTE(kv) plan-curve-selection-precision: live pick readout so a counter-example is
     // reproducible in the agent instance. Mouse px is RELATIVE TO THE VIEWPORT CENTER --
     // that is window-size independent (px -> camera ray goes through the global
     // default_meter_to_pixel, not the window size), so feeding the same rel px at the
     // agent viewport's center reproduces the exact ray. Camera is viewport 0, same args
     // as the channel `set_camera <theta> <phi> [distance [pivot...]]`. `hot` is what
     // picking chose this frame (name + group tag), so Khoa can read all three to me.
     v2 center = get_center(debug_channel_mouse_viewport_box);
     v2 rel = V2(params.mouse.p) - center;
     Camera_Data &cam = state->viewports[0].camera;
     ImGui::Text("mouse rel-center (%.0f %.0f)  raw (%d %d)", rel.x, rel.y,
                 params.mouse.p.x, params.mouse.p.y);
     ImGui::Text("camera theta %.4f phi %.4f dist %.4f pivot (%.3f %.3f %.3f)",
                 cam.theta, cam.phi, cam.distance, cam.pivot.x, cam.pivot.y, cam.pivot.z);
     Location hot = debug_channel_last_hot;
     if(is_document_location(hot))
     {
      i32 hi = document_primitive_index(hot);
      Recorded_Primitive &hp = doc.primitives[hi];
      String tname = enum_name_from_pointer(&Type_Info_Primitive_Type, &hp.type);
      String gname = group_vis_name(doc.groups[hp.group_index].vis_tag);
      ImGui::Text("hot: %.*s #%d (%.*s) %s", strexpand(tname), hi, strexpand(gname),
                  document_location_is_right(hot) ? "right" : "left");
     }
     else if(is_valid(hot))
     {
      ImGui::Text("hot: code %d:%d %d..%d", hot.file.is_driver, hot.file.index,
                  hot.range.min, hot.range.max);
     }
     else { ImGui::TextDisabled("hot: none"); }
     ImGui::Separator();
    }
    if(curve_count == 0)
    {
     ImGui::TextDisabled("no curve selected (click one, shift-click adds)");
    }
    else
    {
     Recorded_Curve &first = doc.primitives[curves[0]].curve;
     ImGui::Text("%d curve%s:", curve_count, curve_count == 1 ? "" : "s");
     for_i32(i, 0, curve_count){ ImGui::SameLine(); ImGui::Text("%d", curves[i]); }
     if(state->model.recordings.preset_settings[state->viewports[0].preset].ignore_radii)
     {// NOTE(kv) Q5: the knob forces uniform radii, so edits below don't show.
      ImGui::TextColored(ImVec4(1,.6f,.2f,1), "ignore_radii is on: radii edits are invisible");
     }
     // NOTE(kv) One history entry per slider drag: begin on activation, commit on the
     // release that followed an edit, discard a release without one.
     auto bracket_drag = [&]()
     {
      if(ImGui::IsItemActivated()){ document_set_radii_begin(state); }
      if(ImGui::IsItemDeactivatedAfterEdit()){ document_edit_commit_and_save(state); }
      else if(ImGui::IsItemDeactivated()){ history_discard(state); }
     };
     v4 radii = first.radii;
     v1 width = maximum(maximum(radii.x, radii.y), maximum(radii.z, radii.w));
     {//-width: scales every selected curve's own profile
      v1 new_width = width;
      ImGui::SliderFloat("width", &new_width, 0.1f, 4.f, "%.2f", ImGuiSliderFlags_Logarithmic);
      bracket_drag();
      if(new_width != width and width > 0){ document_set_radii_scale(state, new_width / width); }
     }
     {//-taper: the end shape at the current width
      const char *taper_names[] = {"flat", "default (.25 1 1 .25)", "tip in (.25 1 1 1)", "tip out (1 1 1 .25)", "custom"};
      v4 tapers[] = {V4(1,1,1,1), V4(.25f,1,1,.25f), V4(.25f,1,1,1), V4(1,1,1,.25f)};
      i32 taper = alen(tapers);  // custom
      for_i32(i, 0, alen(tapers))
      {
       if(width > 0 and radii == width * tapers[i]){ taper = i; break; }
      }
      i32 new_taper = taper;
      if(ImGui::Combo("taper", &new_taper, taper_names, alen(taper_names)) and
         new_taper != taper and new_taper < alen(tapers))
      {
       document_set_radii_begin(state);
       document_set_radii_apply(state, width * tapers[new_taper]);
       document_edit_commit_and_save(state);
      }
     }
     if(ImGui::TreeNode("raw radii"))
     {// NOTE(kv) Q3: the four taper multipliers as stored; sets all selected curves.
      v4 raw = radii;
      ImGui::DragFloat4("radii", &raw.x, 0.01f, 0.f, 8.f, "%.2f");
      bracket_drag();
      if(not (raw == radii)){ document_set_radii_apply(state, raw); }
      ImGui::TreePop();
     }
     {//-midline (Q9): pins x=0 and draws the curve once
      bool midline = first.midline;
      if(ImGui::Checkbox("midline (x=0, not mirrored)", &midline))
      {
       document_set_midline(state, midline);
      }
     }
    }
    im_end();
   }
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
    init_dynamic(m->recorded_vertices, recording_arena, recorded_vertex_cap*entity_cap);
    init_dynamic(m->group_stack.slots, recording_arena, 16);
    reset_capture();  // pushes the root group + root scope slot
    // NOTE(kv) Untagged groups always pass the live-visibility AND; the driver
    // re-publishes tagged slots (e.g. Vis_Skeleton) during its render below.
    m->vis_live[Vis_None] = true;
    for_i32(vis, Vis_Region_First, Group_Vis_Count){ m->vis_live[vis] = true; }
    {// NOTE(kv) Preset-toggle live visibility (plan-preset-rethink): published from
     // the main viewport's settings row, consumed by replay's vis_live re-AND.
     Preset_Settings &row = m->recordings.preset_settings[state->viewports[0].preset];
     m->vis_live[Vis_Level1]               = (row.viz_level >= 1);
     m->vis_live[Vis_Eyeball]              = row.show_eyeball;
     m->vis_live[Vis_Loomis_Ball]          = row.show_loomis_ball;
     m->vis_live[Vis_Hair]                 = !row.hide_hair;
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

 // NOTE(kv) One-shot: re-sending the exit signal every poll nested the exe's
 // "are you sure?" lister inside itself until the stack overflowed (2026-09-06).
 b32 request_exit = debug_channel_request_exit;
 debug_channel_request_exit = false;
 debug_cycles.frame = u32(__rdtsc() - frame_cycle_start);
 return{
  .should_animate_next_frame = should_animate_next_frame or state->replay.force_animate
                               or debug_channel_wants_animate,
  .poll_again_in_ms          = debug_channel_enabled ? DEBUG_CHANNEL_POLL_MS : 0u,
  .request_exit              = request_exit,
  .game_commands             = game_commands,
 };
}
//~EOF
