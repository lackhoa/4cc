#define SILLY_IMGUI_PARTY 0
#define IMGUI_USER_CONFIG "ad_imgui_config.h"
#if SILLY_IMGUI_PARTY
#  define IMGUI_DEFINE_MATH_OPERATORS // Access to math operators
#endif
#include "imgui/imgui.h"
#if SILLY_IMGUI_PARTY
#  include "imgui_function.h"
#endif
//-
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

#include "framework_data.h"

#include "generated/driver.gen.h"
#include "driver.h"
#include "generated/framework.gen.h"
#include "generated/send_bez.gen.h"
#include "game_modeler.h"

#include "game_draw.cpp"
#define FUI_FAST_PATH 0
#include "game_fui.cpp"

#define DYNAMIC_LINK_API
#include "custom/generated/ed_api.cpp"
#include "game_modeler.cpp"
#include "generated/send_bez.gen.cpp"

#include "framework.h"
#include "game_api.cpp"

#undef fval
#define fval fast_fval
#include "game_anime.cpp"
#include "game_utils.cpp"
#include "game_body.cpp"
#include "generated/driver.gen.cpp"
#undef fval
#define fval slow_fval

/*
  IMPORTANT Rule for the renderer
  1. Colors are in linear space (todo precision loss if passed as u32)
 */

#define X(N) function N##_return N(N##_params);
// Note: forward declare
X_GAME_API_FUNCTIONS(X)
//
#undef X

#if SILLY_IMGUI_PARTY
#include "silly.cpp"
#endif

function b32
just_pressed(Game_Input *input, Key_Code keycode, Key_Mods modifiers=0){
 return ((input->key_states       [keycode])     &&
         (input->key_state_changes[keycode] > 0) &&
         (input->active_mods == modifiers));
}
force_inline b32
key_is_down(Game_Input *input, Key_Code keycode, Key_Mods modifiers=0){
 return ((input->key_states[keycode]) &&
         (input->active_mods == modifiers));
}
function v4
key_direction(Game_Input *input, Key_Mods wanted_mods,
              b32 want_new_keypress, b32 *optional_shift=0) {
 v4 result = {};
 if (implies(want_new_keypress, input->direction.new_keypress)) {
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
  ANIMATE(pivot.x,  CAMERA_PAN_STEP_PER_DISTANCE/3.0f);
  ANIMATE(pivot.y,  CAMERA_PAN_STEP_PER_DISTANCE/3.0f);
  ANIMATE(pivot.z,  CAMERA_PAN_STEP_PER_DISTANCE/3.0f);
#undef ANIMATE
  
  current->roll = saved->roll; // @Hack
  current->pan  = saved->pan; // @Hack
 }
 
 return animation_ended;
}
// TODO: Merge this with game_update, come on why are there two calls instead of one?
function game_viewport_update_return
game_viewport_update(game_viewport_update_params){
 b32 should_animate_next_frame = false;
 i32 viewport_index = get_viewport_index(viewport_id);
 Viewport *viewport = &state->viewports[viewport_index];
 {// NOTE: camera animation
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
//-
function void
write_data_func(Printer &p, Type_Info &type, void *void_pointer);
function void
read_enum(Type_Info &type, void *pointer, i32 *dst){
 kv_assert(type.kind == I_Type_Kind_Enum);
 *dst = 0;
 block_copy(dst, pointer, type.size);
}
function void
write_data_union(Printer &p, Type_Info &type,
                 void *pointer0, void *pvariant0){
 kv_assert(type.kind == I_Type_Kind_Union);
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
write_data_func(Printer &p, Type_Info &type, void *void_pointer){
 char newline = '\n';
 u8 *pointer = cast(u8 *)void_pointer;
 switch(type.kind){
  case I_Type_Kind_Basic:{
   write_basic_type(p, type.Basic_Type, pointer);
  }break;
  case I_Type_Kind_Struct:{
   // NOTE: struct
   p << "{\n";
   for_i1(member_index, 0, type.members.count){
    I_Struct_Member &member = type.members[member_index];
    if(!member.unserialized){
     p << member.name << " ";
     u8 *member_pointer = pointer+member.offset;
     if(member.type->kind == I_Type_Kind_Union){
      write_data_union(p, *member.type, member_pointer,
                       (pointer+member.discriminator_offset));
     }else{
      write_data_func(p, *member.type, member_pointer);
     }
     p << newline;
    }
   }
   p << "}\n";
  }break;
  case I_Type_Kind_Union:{
   p<<"<can't write union without variant info>";
  }break;
  case I_Type_Kind_Enum:{
   // NOTE: enum
   i32 enum_value;
   block_copy(&enum_value, pointer, type.size);
   p << enum_value;
  }break;
  
  invalid_default_case;
 }
}
#define write_data(PRINTER, POINTER) \
write_data_func(PRINTER, type_info_from_pointer(POINTER), POINTER)
function i32
enum_index_from_pointer(Type_Info &type, void *pointer0) {
 u8* pointer = (u8*)pointer0;
 i32 value;
 block_copy(&value, pointer, type.size);
 i32 result = {};
 for_i32 (index, 0, type.enum_members.count) {
  I_Enum_Member enum_it = type.enum_members[index];
  if (enum_it.value == value) {
   result = index;
   break;
  }
 }
 return result;
}
function String
enum_name_from_pointer(Type_Info &type, void *pointer0) {
 i32 enum_index = enum_index_from_pointer(type, pointer0);
 return type.enum_members[enum_index].name;
}
#define enum_index_from_value(value) \
enum_index_from_pointer(type_info_from_pointer(&value), &value)
#define enum_name_from_value(value) \
enum_name_from_value(type_info_from_pointer(&value), &value)
function void
pretty_print_func(Printer &p, Type_Info &type, void *void_pointer) {
 char newline = '\n';
 u8 *pointer = cast(u8 *)void_pointer;
 switch(type.kind){
  case I_Type_Kind_Basic:{
   write_basic_type(p, type.Basic_Type, pointer);
  }break;
  
  case I_Type_Kind_Struct:{
   p << "{\n";
   for_i1(member_index, 0, type.members.count) {
    I_Struct_Member &member = type.members[member_index];
    p << member.name << " ";
    u8 *member_pointer = pointer+member.offset;
    pretty_print_func(p, *member.type, member_pointer);
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
  invalid_default_case;
 }
}
//
#define pretty_print(PRINTER, POINTER) \
pretty_print_func(PRINTER, type_info_from_pointer(POINTER), POINTER)

inline Camera_Data*
get_target_camera(Game_State *state, i32 viewport_index){
 return &state->viewports[viewport_index].target_camera;
}

#define IGNORE_MODELING_DATA 1

function b32
game_load(Game_State *state, App *app, Stringz filename){
 // IMPORTANT: This function overwrites edit history.
 b32 ok = true;
 Scratch_Block scratch(app);
 
 String read_string = {};
 if(ok){
  read_string = read_entire_file(scratch, filename);
  ok = read_string.len > 0;
  if(!ok){
   print_message(app, strlit("Game load: can't read the file!\n"));
  }
 }
 
 if(ok){
  // NOTE: ;deserialize
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
     for_i32(cam_index,0,GAME_VIEWPORT_COUNT){
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
   if (r.read_version >= Version_Add_Cursor){
    eat_id(p, "Serialized_State");
    read_Serialized_State(r,state->Serialized_State);
   }
   {
    Modeler &m = state->modeler;
    clear_modeling_data(m);
    
    {//-NOTE Vertices
     eat_id(p, "vertices");
     brace_begin;
     while(p->ok_){
      if(maybe_char(p, '}')){
       break;
      }
      Vertex_Data &v = m.vertices.push2();
#if IGNORE_MODELING_DATA
      eat_char(p,'{');
      eat_until_char(p, '}');
      eat_char(p,'}');
#else
      read_Vertex_Data(r,v);
#endif
     }
    }
    
    {//-NOTE Beziers
     eat_id(p, "beziers");
     brace_begin;
     while(p->ok_){
      if(maybe_char(p, '}')){
       break;
      }
      Curve_Data &b = m.curves.push2();
#if IGNORE_MODELING_DATA
      eat_char(p,'{');
      eat_until_char(p, '}');
#else
      read_Curve_Data(r,b);
#endif
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
 return ok;
}
function void
revert_from_autosave(Game_State *state, App *app){
 game_load(state, app, state->autosave_path);
}

//~

function Bone &
get_right_bone(Modeler &m, Bone &bone){
 Bone *result = 0;
 auto &p = painter;
 if(bone.is_right){
  result = &bone;
 }else{
  for_i32(bone_index, 0, m.bones.count){
   auto &boneR = m.bones[bone_index];
   if(boneR.is_right &&
      boneR.id == bone.id){
    result = &boneR;
    break;
   }
  }
 }
 return *result;
}
//NOTE(kv) Unfortunately a curve can look differently based on
//  which side of the body it is on (because the endpoints can belong to different bones).
//  It feels wonky because this is an animation problem we have yet to "solve".
function Bez
compute_curve_from_data(Modeler &m, Curve_Data &curve, b32 lr){
 Bez result;
 Bone_ID curve_bone_id = curve.bone_id;
 Bone &curve_bone = get_bone(m,curve_bone_id,lr);
 mat4i &curve_xform = curve_bone.xform;
#define curve_vec(vec)  mat4vec(curve_xform, vec)
 Vertex_Data &ve0 = m.vertices[get_p0_index_or_zero(curve).v];
 Vertex_Data &ve3 = m.vertices[get_p3_index_or_zero(curve).v];
 if(curve.type == Curve_Type_Unit){
  //NOTE(kv) Has to preserve the wrong logic here, omg!
  //  could be wrong in some cases with differing coframes, but I don't care!
  v3 p0 = ve0.pos;
  v3 p3 = ve3.pos;
  auto &data = curve.data.unit;
  result = bez_unit(p0, data.d0, data.d3, data.unit_y, p3);
  result = curve_xform*result;
 }else{
  v3 p0 = get_bone(m,ve0.bone_id,lr).xform * ve0.pos;
  v3 p3 = get_bone(m,ve3.bone_id,lr).xform * ve3.pos;
  switch(curve.type){
   case Curve_Type_v3v2:{
    auto &data = curve.data.v3v2;
    result = bez_v3v2(p0, curve_vec(data.d0), data.d3, p3);
   }break;
   case Curve_Type_Parabola:{
    result = bez_parabola(p0, curve_vec(curve.data.parabola.d), p3);
   }break;
   case Curve_Type_C2:{
    auto &data = curve.data.c2;
    Curve_Data &refd = m.curves[data.ref.v];
    Bez ref = compute_curve_from_data(m, refd, lr);
    result = bez_c2(ref, curve_vec(data.d3), p3);
   }break;
   case Curve_Type_Unit:{
    auto &data = curve.data.unit;
    result = bez_unit(p0, data.d0, data.d3, noz(curve_vec(data.unit_y)), p3);
   }break;
   case Curve_Type_Unit2:{
    auto &data = curve.data.unit2;
    result = bez_unit2(p0, data.d0d3, noz(curve_vec(data.unit_y)), p3);
   }break;
   case Curve_Type_Line:{
    result = bez_line(p0,p3);
   }break;
   case Curve_Type_Bezd_Old:{
    auto &data = curve.data.bezd_old;
    result = bez_bezd_old(p0,data.d0,data.d3,p3);
   }break;
   case Curve_Type_Offset:{
    auto &data = curve.data.offset;
    result = bez_offset(p0,curve_vec(data.d0),curve_vec(data.d3),p3);
   }break;
   case Curve_Type_NegateX:{
    auto &data = curve.data.negateX;
    Curve_Data &refd = m.curves[data.ref.v];
    Bez ref = compute_curve_from_data(m, refd, lr);
    result = negateX(ref);
   }break;
   case Curve_Type_Lerp:{
    auto &data = curve.data.lerp;
    v1 t = 0.0f;//todo(kv) incomplete
    Bez begin_data = compute_curve_from_data(m, m.curves[data.begin.v], lr);
    Bez end_data   = compute_curve_from_data(m, m.curves[data.end.v], lr);
    result = bez_lerp(begin_data, t, end_data);
   }break;
   case Curve_Type_Raw:{
    auto &data = curve.data.raw;
    result = bez_raw(p0, curve_xform*data.p1, curve_xform*data.p2, p3);
   }break;
   case Curve_Type_Circle:{
    auto &data = curve.data.circle;
    result = bez_circle(curve_xform*data.center, curve_vec(data.normal));
   }break;
   invalid_default_case;
  }
 }
 return result;
#undef curve_vec
}
function b32
entity_is_curve(Curve_Data &entity){
 return entity_variant_info_table[entity.type].is_curve;
}
function void
render_data(Modeler &m){
 painter.is_right = 0;
 for_i32(vi,1,m.vertices.count){
  //-Vertices
  Vertex_Data &vert = m.vertices[vi];
  Bone &bone = get_bone(m, vert.bone_id, 0);
  u32 prim_id = prim_id_from_vertex_index({vi});
  v3 pos = mat4vert(bone.xform, vert.pos);
  argb inactive_color = argb_dark_green;
  indicate_vertex("data", pos, 9000, false, inactive_color, prim_id);
 }
 for_i32(ci,1,m.curves.count){
  //-Curves
  Curve_Data &curve = m.curves[ci];
  u32 prim_id = prim_id_from_curve_index({ci});
  Common_Line_Params &cparams = m.line_cparams[curve.cparams.v];
  for_i32(lr_index,0,2){
   if(curve.symx || lr_index==0){
    if(entity_is_curve(curve)){
     //-Curve
     set_in_block(painter.lr_index, lr_index);
     Bez drawn = compute_curve_from_data(m,curve,lr_index);
     draw_cparams(drawn, cparams, curve.params, prim_id);
    }else{
     //-Fill
     switch(curve.type){
      case Curve_Type_Fill3:{
       v3 p[3];
       for_i32(vi,0,3){
        i1 vert_index = curve.data.fill3.verts[vi].v;
        Vertex_Data &vert = m.vertices[vert_index];
        Bone &bone = get_bone(m, vert.bone_id, lr_index);
        p[vi] = bone.xform*vert.pos;
       }
       set_in_block(painter.lr_index, lr_index);
       fill3_inner2(p,curve.fill_params,prim_id);
      }break;
      case Curve_Type_Fill_Bez:{
       auto &data = curve.data.fill_bez;
       Curve_Data &curve_data = m.curves[data.curve.v];
       Bezier curve_ = compute_curve_from_data(m,curve_data,lr_index);
       fill_bez(curve_,0,prim_id);
      }break;
      case Curve_Type_Fill_DBez:{
       auto &data = curve.data.fill_dbez;
       Curve_Data &curve1_data = m.curves[data.curve1.v];
       Curve_Data &curve2_data = m.curves[data.curve2.v];
       Bezier curve1 = compute_curve_from_data(m,curve1_data,lr_index);
       Bezier curve2 = compute_curve_from_data(m,curve2_data,lr_index);
       fill_dbez(curve1,curve2,0,prim_id);
      }break;
     }
    }
   }
  }
 }
#if 0
 for_i32(ti,1,m.fills.count){
  //-Triangles
  Fill_Data &fill = m.fills[ti];
  u32 prim_id = prim_id_from_triangle_index({ti});
  for_i32(lr_index,0,2){
   if(fill.symx || lr_index==0){
    switch(fill.type){
     invalid_default_case;
    }
   }
  }
 }
#else
#endif
}
//TODO(kv) @cleanup We wanna change this from update+render to update_and_render
function game_render_return
game_render(game_render_params){
 Modeler &mo = state->modeler;
 Painter &pa = painter;
 pa = {};
 slider_cycle_counter = 0;
 Scratch_Block scratch;
 Viewport *viewport = &state->viewports[viewport_id-1];
 
 i32 scale_down_pow2 = fval(0); // ;scale_down_slider
 v1 meter_to_pixel;
 {
  macro_clamp_min(scale_down_pow2, 0);
  v1 render_scale = 1.f;
  for_i32 (it,0,scale_down_pow2) { render_scale *= 0.5f; }
  v1 default_meter_to_pixel = 4050.6329f;
  meter_to_pixel = default_meter_to_pixel * render_scale;
 }
 v1 pixel_to_meter = 1.f / meter_to_pixel;
 viewport->clip_radius = pixel_to_meter*get_radius(clip_box);
 
 Camera camera_value;
 Camera *camera = &camera_value;
 {// NOTE: camera
  setup_camera(camera, &viewport->camera);
 }
 {
  b32 camera_frontal=almost_equal(absolute(camera->z.z), 1.f, 1e-2f);
  b32 camera_profile=almost_equal(absolute(camera->z.x), 1.f, 1e-2f);
  b32 orthographic = pa.show_grid and (camera_frontal or camera_profile);
  if(fbool(0)){orthographic = pa.show_grid;}
  if(fbool(0)){orthographic = true;}
  pa.view_from_world = get_view_from_world(camera, orthographic);
 }
 pa.cursorp   = state->kb_cursor.pos;
 pa.cursor_on = state->kb_cursor_mode;
 pa.target    = target;
 pa.viewport  = viewport;
 pa.modeler   = &mo;
 pa.camera    = *camera;
 pa.sending_data = state->sending_data;
 {//-NOTE(kv) Drawing the movie
  Render_Config *config = draw_new_group(target);
  {
   set_y_up(target, config);
   config->scale_down_pow2 = scale_down_pow2;
   config->meter_to_pixel  = meter_to_pixel;
  }
  //TODO(kv) Why can't the game understand scratch blocks?
  Scratch_Block render_scratch;
/*  if(pa.sending_data){
   mo.vertices.set_count(1);
   mo.curves.  set_count(1);
   mo.fills.   set_count(1);
  }*/
  render_movie(scratch, render_scratch,
               config, state->references_full_alpha,
               &state->pose, state->anime_time);
  //state->sending_data = false;  //NOTE(kv) We will be caching computation, so this is not needed anymore I don't think
 }
 //-NOTE
 render_data(mo);
 
 if(state->kb_cursor_mode &&
    viewport_id == 1)
 {//NOTE(kv) ;draw_cursor
  v1 cursor_dist = lengthof(camera->cam_from_world * state->kb_cursor.pos);
  v1 radius = 4*millimeter;
  radius *= cursor_dist / camera->focal_length;
  if (fbool(0)){ radius *= 10.f; }
  v3 points[] = { v3{}, V3(-1,-1,0), V3(+1,-1,0), };
  for_i32(i,0,3){
   points[i] = (state->kb_cursor.pos +
                radius*(points[i].x*camera->x +
                        points[i].y*camera->y));
  }
  {
   Fill_Params params = {.flags=Poly_Overlay};
   params.color = argb_blue;
   fill3(points, params);
  }
 }
}
function game_init_return
game_init(game_init_params)
{// API import
 ed_api_read_vtable(ed_api);
 //@game_bootstrap_arena_zero_initialized
 Game_State *state = push_struct(bootstrap_arena, Game_State);
 state->malloc = malloc_base_allocator;  // NOTE(kv): Stupid: can't use global vars on reloaded code!
 state->permanent_arena = *bootstrap_arena;
 Arena *arena = &state->permanent_arena;
 state->dll_arena = make_arena(&state->malloc, MB(1));
 
 {//-;init_modeler
  Modeler &m = state->modeler;
  //NOTE(kv) when you refer to something make sure it doesn't move!
  init_static(m.vertices,  arena, 4096);
  init_static(m.curves,    arena, 512);
  //init_static(m.fills,     arena, 512);
  init_static(m.bones,     arena, 128);
  init_static(m.bones,     arena, 128);
  init_static(m.line_cparams,arena, 128);
  m.bones.push(Bone{.xform=mat4i_identity});
  {
   Common_Line_Params cp = {};
   cp.radius_mult      = 1.f;
   cp.nslice_per_meter = 2.2988f * 100.f;
   m.line_cparams.push(cp);
  }
  {
   Modeler_History &h = m.history;
   // NOTE(kv): We might want to erase entire edit history (or part of it),
   //  so we'll allocate items from an arena for now (maybe make a heap later).
   h.arena     = make_arena(&state->malloc);
   h.inited    = true;
   h.allocator = make_arena_base_allocator(&h.arena);
  }
  clear_modeling_data(m);
 }
 
 {// NOTE: Save/Load business load_game
  Scratch_Block scratch(app);
  String binary_dir = system_get_path(scratch, SystemPath_BinaryDirectory);
  state->save_dir         = pjoin(arena, binary_dir, "data");
  state->backup_dir       = pjoin(arena, state->save_dir, "backups");
  state->autosave_path    = pjoin(arena, state->save_dir, "text.kv");
  state->manual_save_path = pjoin(arena, state->save_dir, "manual.kv");
  
  {// NOTE: Load state
   state->data_load_arena = make_arena(&state->malloc);
   game_load(state, app, state->autosave_path);
  }
 }
 
 for_i32(viewport_index,0,GAME_VIEWPORT_COUNT)
 {//;frame_arena_init
  Viewport *viewport = &state->viewports[viewport_index];
  viewport->render_arena = make_arena(&state->malloc);
  viewport->index = viewport_index;
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
function void
meta_init(){
 init_entity_type_info_table();
}
function game_reload_return
game_reload(game_reload_params)
{// Game_State
 meta_init();
 {// API import
  ed_api_read_vtable(ed_api);
 }
 {//NOTE: ;FUI_reload
  dll_arena = &state->dll_arena;
  state->dll_temp_memory = begin_temp(dll_arena);
  // TODO(kv): This trickery means that we do need "static arena".
  push_struct(dll_arena, u8);  // @fui_ensure_nonzero_offset ;fui_ensure_arena_cursor_exists
  
  line_map = state->line_map;
  block_zero(line_map, state->line_cap*sizeof(Line_Map_Entry));
  //-
  slow_line_map       = state->slow_line_map;
  slow_line_map.count = 0;
 }
 {//-NOTE: Dear ImGui reload
  IMGUI_CHECKVERSION();
  
  {
   auto &imgui = state->imgui_state;
   ImGui::SetCurrentContext(imgui.ctx);
   ImGui::SetAllocatorFunctions(imgui.alloc_func, imgui.free_func, imgui.user_data);
  }
 }
 state->sending_data = true;
}

function game_shutdown_return
game_shutdown(game_shutdown_params){
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
 
 const i32 MAX_SAVE_SIZE = KB(32);  // Wastes @Memory
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
    for_i32(camera_index, 0, GAME_VIEWPORT_COUNT) {
     brace_block;
     Camera_Data &cam = state->viewports[camera_index].target_camera;
#define X(TYPE,NAME)  macro_print_field(cam, TYPE, NAME);
     X_Camera_Data(X);
#undef X
    }
   }
  }
  newline;
  p<<"references_full_alpha "<<state->references_full_alpha<<"\n\n";
  p<<"Serialized_State\n";
  write_data(p, &state->Serialized_State);
  {// NOTE: Modeler data
   Modeler &modeler = state->modeler;
   {//NOTE vertices
    arrayof<Vertex_Data> &vertices = modeler.vertices;
    print(p, "vertices"); newline;
    {
     brace_block;
     for_i32(vi,1,vertices.count)
     {//NOTE Tight loop
      write_data(p, &vertices[vi]);
     }
    }
    newline;
   }
   
   {//NOTE Beziers
    arrayof<Curve_Data> &curves = modeler.curves;
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
  } else {
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

function game_send_command_return
game_send_command(game_send_command_params) {
 state->command_queue.push(command_name);
}

function void
compute_direction_helper(Game_Input *input, Key_Code key_code, i32 component, v1 value) {
 if (input->key_states[key_code] != 0){
  input->direction.dir.e[component] = value;
  input->direction.new_keypress = (input->key_state_changes[key_code] > 0);
 }
}

function void
update_orbit(Camera_Data *cam, Key_Direction key_dir)
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
inline void
update_orbit(Camera_Data *cam, Game_Input *input) {
 update_orbit(cam, input->direction);
}
function void
update_pan(Camera_Data *cam, Game_Input *input){
 v1 step = CAMERA_PAN_STEP_PER_DISTANCE * cam->distance;
 v2 delta_pan = input->direction.dir.xy;
 Camera computed_cam; setup_camera(&computed_cam, cam);
 cam->pivot += step*(delta_pan.x * computed_cam.x + 
                     delta_pan.y * computed_cam.y);
}
inline b32
fui_slider_is_discrete(Fui_Slider *slider){
 return (slider->type == Basic_Type_i1 || 
         slider->type == Basic_Type_i2 ||
         slider->type == Basic_Type_i3 ||
         slider->type == Basic_Type_i4);
}
inline b32
fui_slider_is_continuous(Fui_Slider *slider){
 return !fui_slider_is_discrete(slider);
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
g_jump_to_line(App *app, i1 linum){
 // NOTE: Don't switch to the game panel, because the cursor should be in the code panel.
 View_ID view = get_active_view(app,0);
 if(!is_view_to_the_right(app, view)){
  //NOTE(kv) Switch to the right view
  view = get_other_primary_view(app, view, Access_Always, true);
 }
 view_set_buffer_named(app, view, DRIVER_FILE_NAME);
 view_set_cursor(app, view, seek_line_col(linum, 0));
}
function void
set_camera_frontal_or_profile(Camera_Data &cam){
 if(cam.theta == 0 and cam.phi == 0){
  // if frontal -> set to profile
  cam.theta = .25f;
  cam.phi   = 0;
 }else{
  // set frontal
  cam.theta = 0;
  cam.phi   = 0;
 }
}
// TODO: Input handling: how about we add a callback to look at all the events and report to the game if we would process them or not?
function game_update_return
game_update(game_update_params){
#if SILLY_IMGUI_PARTY 
 FxTestBed();
#endif
 
 Scratch_Block scratch(app);
 Base_Allocator scratch_allocator = make_arena_base_allocator(scratch);
 
 Modeler *modeler = &state->modeler;
 b32 game_active = active_viewport_id != 0;
 b32 fui_active = fui_is_active();
 b32 game_or_fui_active = (game_active || fui_active);
 
 b32 should_animate_next_frame = false;
 if (game_or_fui_active) {
  should_animate_next_frame = true;
 }
 
 i32 update_viewport_id = (active_viewport_id ? active_viewport_id : 1);
 kv_assert(active_viewport_id <= GAME_VIEWPORT_COUNT);
 i32 update_viewport_index = update_viewport_id - 1;
 Viewport *update_viewport = &state->viewports[update_viewport_index];
 Camera_Data *update_target_cam = get_target_camera(state, update_viewport_index);
 
 Game_Input input_value = {};
 (Game_Input_Public&) input_value = input_public;
 Game_Input *input = &input_value;
 v1 dt = input->frame.animation_dt;
 {
  state->time += dt;
  if (state->time >= 1000.0f) { state->time -= 1000.0f; }
 }
 
 //NOTE(kv) If you wanna load states, then do it first and foremost.
 arrayof<String> game_commands = {};
 {//~ NOTE: Game commands
  {// NOTE: Process commands
   arrayof<String> &queue = state->command_queue;
   for_i1(ci,0,queue.count){
#define MATCH(NAME)    queue[ci] == strlit(NAME)
    if(0){
    }else if(MATCH("save_manual")) {
     game_save(state, app, true);
    }else if(MATCH("load_manual")) {
     game_load(state, app, state->manual_save_path);
    }else if(MATCH("revert")){
     revert_from_autosave(state, app);
    }else{
     vim_set_bottom_text(strlit("game: cannot serve command"));
    }
#undef MATCH
   }
   queue.count = 0;
  }
  {// NOTE: Fill command lister
   auto &cmds = game_commands;
   cmds.count = 0;
   cmds.push(strlit("save_manual"));
   cmds.push(strlit("load_manual"));  
   cmds.push(strlit("revert"));  
  }
 }
 
 {
  state->anime_time = state->time;
  state->pose = driver_animate(modeler, scratch, state->anime_time);
 }
 
 if(state->kb_cursor_mode){
  v3 curpos = state->kb_cursor.pos;
  Modeler &m = *modeler;
  v1 min_lensq = max_f32;
  for_i32(vi,1,m.vertices.count){
   //-Closest vertex
   Vertex_Data &v = m.vertices[vi];
   Bone &bone = get_bone(m, v.bone_id, false);
   v1 l = lensq(curpos - bone.xform*v.pos);
   if(l < min_lensq){
    min_lensq = l;
    hot_prim_id = prim_id_from_vertex_index(Vertex_Index{vi});
   }
  }
  for_i32(ci,1,m.curves.count){
   //-Closest curve
   Curve_Data &c = m.curves[ci];
   Bez computed = compute_curve_from_data(m, c, 0);
   v1 sd_squared;
   {//-Convex box culling
    v3 min_corner = V3(max_f32);
    v3 max_corner = V3(min_f32);
    for_i32(i,0,4){
     min_corner = min(min_corner, computed[i]);
     max_corner = max(max_corner, computed[i]);
    }
    v3 center = lerp(min_corner, 0.5f, max_corner);
    v3 p = curpos-center;
    v3 r = 0.5f*(max_corner-min_corner);
    v3 q = absolute(p)-r;
    sd_squared = lensq(max(q,0.f));  //NOTE zero for anything inside, which is what we want
   }
   if(sd_squared < min_lensq){
    v1 l = max_f32;
    const i1 test_segment_count = 8;
    for_i1(iseg,1,test_segment_count){
     //NOTE(kv) Don't need to test the endpoints, those are vertices.
     v1 t = v1(iseg) / v1(test_segment_count);
     v3 sample = bezier_sample(computed,t);
     l = min(l,lensq(curpos-sample));
    }
    if(l < min_lensq){
     min_lensq = l;
     hot_prim_id = prim_id_from_curve_index(Curve_Index{ci});
    }
   }
  }
 }else{
  hot_prim_id = input->frame.hot_prim_id;
 }
 
 if(!hot_prim_id){
  //NOTE(kv) If there's nothing hot, then we select by editor cursor position
  Buffer_ID buffer = get_active_buffer(app);
  String active_buffer_name = push_buffer_base_name(app, scratch, buffer);
  View_ID view_id = get_active_view(app,0);
  if(active_buffer_name != DRIVER_FILE_NAME){
   //note(kv) Retry with the other view 
   view_id = get_other_primary_view(app, view_id, 0, false);
   buffer = view_get_buffer(app, view_id, 0);
   active_buffer_name = push_buffer_base_name(app, scratch, buffer);
  }
  if(active_buffer_name == DRIVER_FILE_NAME){
   i64 linum = get_current_line_number2(app, view_id, buffer);
   Modeler &m = *modeler;
   Vertex_Ref v = get_vertex_by_linum(m,linum);
   if(v.vertex){
    hot_prim_id = prim_id_from_vertex_index(v.index);
   }else{
    Curve_Ref c = get_curve_by_linum(m,linum);
    if(c.curve){
     hot_prim_id = prim_id_from_curve_index(c.index);
    }
   }
   if(!hot_prim_id){
    //NOTE(kv) If still no hot prim id found, just wing it! (might crash if we're not careful)
    hot_prim_id = linum;
   }
  }
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
 
 b32 modeler_selecting = selected_prim_id(modeler);
 u32 mods = input->active_mods;
 for_i32(key_stroke_index, 0, key_strokes.count){
  //-NOTE(kv) Key bindings
  auto cam = update_target_cam;
  Key_Code keycode = key_strokes[key_stroke_index];
  u32 code = mods|keycode;
  const u32 S = Key_Mod_Sft;
  const u32 C = Key_Mod_Ctl;
  const u32 M = Key_Mod_Alt;
  if(game_active){
   if(modeler_selecting){
    //-NOTE
    auto m = modeler;
    switch(code){
     case Key_Code_Return:{ modeler_exit_edit(m); }break;
     case Key_Code_Escape:{
      if(m->change_uncommitted){
       modeler_exit_edit_undo(m);
      }else{
       clear_selection(m);
      }
     }break;
     case Key_Code_S:{
      toggle_boolean(m->selection_spanning);
      compute_active_prims(m);
     }break;
    }
   }else if(state->kb_cursor_mode){
    //-Cursor mode
    if(mods==Key_Mod_Ctl && is_v3_key(keycode)){
     update_orbit(cam, input);
    }else if(mods==Key_Mod_Alt && is_v2_key(keycode)){
     update_pan(cam, input); 
    }else{
     switch(code){
      case Key_Code_A:{ set_camera_frontal_or_profile(*cam); }break;
      case Key_Code_I: case Key_Code_O:{
       update_orbit(cam, input);
      }break;
      case Key_Code_Escape:
      case Key_Code_M:{
       state->kb_cursor_mode=false;
      }break;
      case Key_Code_Return:{
       auto m = modeler;
       Prim_XID hot_xid = prim_xid_from_id(get_hot_prim_id());
       if(transitioning_from_code){
        if(hot_xid.type){
         //NOTE(kv) drawn by data
         if(hot_xid.type==Prim_Vertex){
          Vertex_Data *vertex = &m->vertices[hot_xid.index];
          g_jump_to_line(app, vertex->linum);
         }else if(hot_xid.type==Prim_Curve){
          Curve_Data *curve  = &m->curves[hot_xid.index];
          g_jump_to_line(app, curve->linum);
         }
        }else{
         g_jump_to_line(app, hot_xid.index);
        }
       }else{
        //NOTE(kv) Normal editor behavior
        state->kb_cursor_mode = false;
        select_primitive(m, hot_xid.id);
       }
      }break;
      //TODO(kv) @cleanup cutnpaste
      //NOTE(kv) Set camera frontal
      case C|M|Key_Code_K:{ cam->theta=0; cam->phi=0; }break;
      //NOTE(kv) Set camera to the right
      case C|M|Key_Code_L:{ cam->theta=.25f; cam->phi=0; }break;
      //NOTE(kv) Set camera to the left
      case C|M|Key_Code_H:{ cam->theta=-.25f; cam->phi=0; }break;
     }
    }
   }else{
    //-Normal mode
    if((mods==C || mods==0) && is_v3_key(keycode)){
     update_orbit(cam, input);
    }else if (mods==M && is_v2_key(keycode)){
     update_pan(cam, input);
    }else{
     switch(code){
      case Key_Code_Return:{ game_save(state, app, false); }break;
      case Key_Code_A:{ set_camera_frontal_or_profile(*cam); }break;
      case Key_Code_M:{ state->kb_cursor_mode = true; }break;
      case Key_Code_Q:{ toggle_boolean(state->references_full_alpha); }break;
      case Key_Code_U:{ modeler_undo(modeler); }break;
      case Key_Code_X:{ cam->theta *= -1.f; }break;
      case Key_Code_Z:{ cam->theta = .5f - cam->theta; }break;
      case S|Key_Code_0:{ cam->roll = {}; }break;
      //NOTE(kv) Reverting is just.so.useful for debugging!
      //TODO(kv) Pushing strings is dangerous! Since the game might restart.
      case S|Key_Code_U:{ state->command_queue.push(strlit("revert")); }break;
      case C|Key_Code_R:{ modeler_redo(modeler); }break;
      case M|Key_Code_0:{ cam->pan = {}; }break;
      //NOTE(kv) Set camera frontal
      case C|M|Key_Code_K:{ cam->theta=0; cam->phi=0; }break;
      //NOTE(kv) Set camera to the right
      case C|M|Key_Code_L:{ cam->theta=.25f; cam->phi=0; }break;
      //NOTE(kv) Set camera to the left
      case C|M|Key_Code_H:{ cam->theta=-.25f; cam->phi=0; }break;
     }
    }
   }
  }else if(fui_is_active()){
   //-NOTE
   if(mods==C && is_v3_key(keycode)){
    update_orbit(cam, input);
   }else if(mods==M && is_v2_key(keycode)){
    update_pan(cam, input);
   }else if(mods==0 && is_v4_key(keycode)){
    //NOTE(kv) Update discrete slider
    Fui_Slider *slider = fui_active_slider;
    if(fui_slider_is_discrete(slider)){
     i4 value;
     block_copy(&value, slider+1, slider_value_size(slider));
     for_i32(index,0,4){
      value.e[index] += i32(input_dir[index]);
     }
     if(slider->flags & Slider_Clamp_01){
      for_i32(index,0,4) {
       macro_clamp01i(value.e[index])
      }
     }
     block_copy(slider+1, &value, slider_value_size(slider));
    }
   }else{
    switch(code){
     case Key_Code_Tab:{ toggle_boolean(fui_v4_zw_active); }break;
    }
   }
  }
 }
 
 Camera update_cam = {};
 setup_camera(&update_cam, update_target_cam);
 
 if(input_dir != v4{}){
  //-NOTE(kv) Handling continuous directional input
  if (fui_active){
   //NOTE(kv) ;UpdateFuislider
   Fui_Slider *slider = fui_active_slider;
   if (fui_slider_is_continuous(slider)){
    // NOTE(kv) we copy the slider value out, pretend it's a v4
    // operate on it, and put it back in again.
    v4 value;
    block_copy(&value, slider+1, slider_value_size(slider));
    if (mods == 0 || mods == Key_Mod_Sft){
     if (input_dir.y != 0.f &&
         slider->type == Basic_Type_v1){
      value.x = (input_dir.y > 0) ? 1.f : 0.f; 
     }else{
      if (fui_v4_zw_active &&
          slider->type == Basic_Type_v4) {
       input_dir.zw = input_dir.xy;
       input_dir.xy = {};
      }
      if (slider->flags & Slider_Camera_Aligned) {
       input_dir = noz( update_cam.world_from_cam*input_dir );
      }
      v1 delta_scale = slider->delta_scale;
      if(delta_scale == 0){ delta_scale = 0.2f; }
      v4 delta = delta_scale * dt * input_dir;
      if(mods == Key_Mod_Sft){ delta *= 10.f; }
      value += delta;
      
      if(slider->flags & Slider_Clamp_X) {value.x = 0;}
      if(slider->flags & Slider_Clamp_Y) {value.y = 0;}
      if(slider->flags & Slider_Clamp_Z) {value.z = 0;}
      if(slider->flags & Slider_NOZ)     { value.xyz = noz(value.xyz); }
      if(slider->flags & Slider_Clamp_01){
       for_i32(index,0,4) { macro_clamp01(value.v[index]); }
      }
     }
    }
    block_copy(slider+1, &value, slider_value_size(slider));
   }
  }else if(game_active){
   if (u32 sel_prim = selected_prim_id(modeler)){
    //-NOTE
    Modeler *m = modeler;
    Modeler_History &h = m->history;
    if (get_selected_type(m) == Prim_Vertex){
     // NOTE: Selecting a vertex
     Vertex_Index sel_index = vertex_index_from_prim_id(sel_prim);
     v3 direction = key_direction(input, 0, false).xyz;
     
     // NOTE(kv): Update vertex position
     direction.z = 0.f;
     direction = noz( mat4vec(update_cam.world_from_cam, direction) );
     v1 velocity = 0.2f;
     v3 delta = velocity * dt * direction;
     
     arrayof<Vertex_Index> influenced_verts;
     init_static(influenced_verts, scratch, m->active_prims.count);
     for_i32(index,0,m->active_prims.count) {
      Vertex_Index vi = vertex_index_from_prim_id(m->active_prims[index]);
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
          m->change_uncommitted &&
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
    }else if(get_selected_type(m) == Prim_Curve){
#if 0
     // NOTE(kv) Curve update stub
     Curve_Data &sel = modeler.curves[sel_index0];
     {
      i1 direction = i1( key_direction(input, 0, true).x );
      v1 delta = i2f6(direction);
      if (delta != 0) { for_i32(index,0,4) { sel.radii[index] += delta; } }
      DEBUG_VALUE(sel.radii);
     }
#endif
    }
   }
  }
 }
 
 if(input->mouse.press_l){
  // NOTE: Left click handling
  u32 hot_prim = get_hot_prim_id();
  if (hot_prim){
   if(prim_id_is_data(hot_prim)){
    // NOTE: Drawn by data -> change selected obj id
    auto &m = modeler;
    select_primitive(m, hot_prim);
    m->change_uncommitted = false;
    switch_to_mouse_panel(app);
   }else{
    // NOTE: Drawn by code -> jump to code
    g_jump_to_line(app, hot_prim);
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
 
 {//-NOTE: Presets
  if( just_pressed(input, Key_Code_Space) ) {
   game_last_preset(state, update_viewport_id);
  }
 }
 
 if(game_active && state->kb_cursor_mode){
  //-NOTE(kv) update cursor
  Keyboard_Cursor &cursor = state->kb_cursor;
  b32 shifted = 0;
  v2 dir_v2 = key_direction(input, 0, false, &shifted).xy;
  if (dir_v2 == v2{}){
   cursor.vel = {};  //NOTE(kv) stop immediately, we don't want skating
  }else{
   Camera cam;
   setup_camera(&cam, &state->viewports[0].camera);
   {//NOTE(kv) Movement
    v3 dir = noz( mat4vec(cam.world_from_cam, V3(dir_v2)) );
    v1 cursor_camz = (cam.cam_from_world * cursor.pos).z;
    v1 zoom = absolute(cursor_camz/cam.focal_length);
    v1 acc = zoom * 0.1f * fval(2.0423f);
    if (shifted){ acc *= 10.f; }
    v1 new_vel = cursor.vel + dt*acc;
    v1 max_vel = zoom*0.1f*fval(1.0625f);
    if (shifted){ max_vel *= 10.f; }
    macro_clamp_max(new_vel, max_vel);
    v3 delta = 0.5f*(cursor.vel+new_vel)*dt*dir;
    cursor.pos += delta;
    cursor.vel = new_vel;
   }
   
   {//NOTE(kv) Clamping (Pretty Involved)
    //TODO(kv) @bug We don't know if we're in orthographic mode
    v2 cursor_view = mat4vert_div(get_view_from_world(&cam, false),
                                  cursor.pos).xy;
    v2 radius_view = state->viewports[0].clip_radius;
    {
     auto &p = cursor_view;
     auto &r = radius_view;
     macro_clamp_min(p.x, -r.x); macro_clamp_min(p.y, -r.y);
     macro_clamp_max(p.x, +r.x); macro_clamp_max(p.y, +r.y);
    }
    v1 cursor_camz = (cam.cam_from_world * cursor.pos).z;
    v3 cursor_cam = V3(absolute(cursor_camz / cam.focal_length) * cursor_view,
                       cursor_camz);
    cursor.pos = (cam.world_from_cam * cursor_cam);
   }
  }
 }
 //~
 if (input->debug_camera_on) {
  auto cam = update_target_cam;
  DEBUG_NAME("camera(theta,phi,distance)", V3(cam->theta, cam->phi, cam->distance));
  DEBUG_NAME("camera(pan)", cam->pan);
 }
 
 for_i32(index, 0, GAME_VIEWPORT_COUNT) {
  // NOTE: Set viewport presets to useful values
  Viewport *viewport = &state->viewports[index];
  
  if(viewport->preset == viewport->last_preset){
   if(viewport->preset == 0){ viewport->last_preset = 2; }
   else{ viewport->last_preset = 0; }
  }
 }
 
 driver_update(state->viewports);
 
 {//;update_modeler
  auto m = &state->modeler;
  auto &h = m->history;
  {//-NOTE: Drawing GUI
   if(get_selected_type(m) == Prim_Curve){
    //;draw_curve_info
    ImGui::Begin("curve data", 0);
    Curve_Data *curve = get_selected_curve(m);
    Type_Info type = Type_Info_Curve_Type;
    i32 curve_type_index = enum_index_from_value(curve->type);
    
    const char* combo_preview = to_cstring(scratch, type.enum_members[curve_type_index].name);
    if ( ImGui::BeginCombo("combo", combo_preview, 0) ) {
     for_i32(enum_index, 0, type.enum_members.count) {
      b32 is_selected = (enum_index == curve_type_index);
      char *name = to_cstring(scratch, type.enum_members[enum_index].name);
      if (ImGui::Selectable(name, is_selected)){
      }
      if (is_selected){
       ImGui::SetItemDefaultFocus();
      }
     }
     ImGui::EndCombo();
    }
    ImGui::End();
   } else {
    if (fbool(0)){
     ImGui::ShowDemoWindow(0);
    }
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
  if(debug_frame_time_on){
   DEBUG_NAME("work cycles", input->frame.work_cycles);
   DEBUG_NAME("slider_cycle_counter", slider_cycle_counter);
   DEBUG_NAME("work ms", input->frame.work_seconds * 1e3f);
  }
  
  if(fbool(0)){
   DEBUG_VALUE(image_load_info.image_count);
   DEBUG_VALUE(image_load_info.failure_count);
  }
 }
 
 return{
  .should_animate_next_frame=should_animate_next_frame,
  .game_commands            =game_commands,
 };
}

//~EOF