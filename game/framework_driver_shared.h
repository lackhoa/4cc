//-
#include "game_colors.cpp"
#include "game_debug.h"
#include "generated/ad_file_formats.gen.h"
#include "generated/driver.gen.h"
#include "4coder_kv_debug.h"

//-
struct Viewport;
struct Modeler;
struct Render_Target;
struct Render_Config;
//-

struct Bezier{
 v3 e[4];
 myinline operator v3 *() { return e; };
};
typedef Bezier Bez;

function v3
bezier_sample(const v3 P[4], v1 u){
 v1 U = 1-u;
 return (1*cubed(U)      *P[0] +
         3*(u)*squared(U)*P[1] +
         3*squared(u)*(U)*P[2] +
         1*cubed(u)      *P[3]);
}
function v1
bezier_sample(v4 P, v1 u){
 v1 U = 1-u;
 return (1*cubed(U)      *P.v[0] +
         3*(u)*squared(U)*P.v[1] +
         3*squared(u)*(U)*P.v[2] +
         1*cubed(u)      *P.v[3]);
}
// NOTE: Actually bernstein basis
function v1
cubic_bernstein(i32 index, v1 t){
 v1 factor = v1((index == 1 || index == 2) ? 3 : 1);
 v1 result = factor * integer_power(t,index) * integer_power(1.f-t, 3-index);
 return result;
}
// NOTE: Actually bernstein basis
function v1
quad_bernstein(i32 index, v1 t){
 v1 result = (index==0 ? squared(1-t) :
              index==1 ? 2*(1-t)*t :
              squared(t));
 return result;
}
inline Bezier
negateX(Bezier line){
 for_i32(i,0,4) { line[i].x = -line[i].x; }
 return line;
}
myinline Bezier
bez_negateX(Bezier line){ return negateX(line); }

function Bez
operator*(mat4 transform, Bez bezier)
{
 Bez result;
 for_i32(index,0,4) { result[index] = transform*bezier[index]; }
 return result;
}

struct Patch{
 v3 e[4][4];
 typedef v3 Array4x4[4][4];  // @stroustrup
 operator Array4x4&() { return e; }
};
function Bezier
get_column(Patch const&surface, i32 col){
 Bezier result;
 for_i32(index,0,4) {
  result[index] = surface.e[index][col];
 }
 return result;
}

//~ id system
// NOTE(kv): Entities are either drawn by code or data.
enum Prim_Type : u8{
 Prim_Null     = 0,
 Prim_Vertex   = 1,  //NOTE(kv) Idk if vertices are really entities? Maybe they're like vertex visualizer?
 Prim_Curve    = 2,
 Prim_Triangle = 3,
};

inline Prim_Type type_from_prim_id(u32 id){ return Prim_Type(id >> 24); }
inline b32 prim_id_is_data(u32 id){ return type_from_prim_id(id) != 0; }

struct Prim_XID{
 u32       id;
 Prim_Type type;
 i1        index;
};
inline u32
index_from_prim_id(u32 id){
 if(prim_id_is_data(id)){ return (id & 0xFFFF); }
 return 0;
}
inline Prim_XID
prim_xid_from_id(u32 id){
 Prim_XID result = {};
 result.id    = id;
 result.type  = type_from_prim_id(id);
 result.index = index_from_prim_id(id);
 return result;
}
inline u32
prim_id_from_vertex_index(Vertex_Index i){
 u32 type = u32(Prim_Vertex) << 24;
 return (u32)(i.v) | type;
}
inline u32
prim_id_from_curve_index(Curve_Index i){
 u32 type = u32(Prim_Curve) << 24;
 return (u32)(i.v) | type;
}
#if 0
inline u32
prim_id_from_triangle_index(Fill_Index i){
 u32 type = u32(Prim_Triangle) << 24;
 return (u32)(i.v) | type;
}
#endif
//~
global char *global_debug_scope;
#define vertex_block(NAME) set_in_block(global_debug_scope, NAME)

#define DEBUG_NAME(NAME, VALUE)  DEBUG_VALUE_inner(global_debug_scope, NAME, VALUE, 0)
#define DEBUG_NAME_COLOR(NAME, VALUE, COLOR)  DEBUG_VALUE_inner(global_debug_scope, NAME, VALUE, COLOR)
#define DEBUG_VALUE(VALUE)       DEBUG_VALUE_inner(global_debug_scope, #VALUE, VALUE)
#define DEBUG_TEXT(TEXT)         DEBUG_VALUE_inner(global_debug_scope, TEXT, 0.f)
//~

struct Camera {
 union {
  mat4i transform;
  struct {
   mat4 world_from_cam;   // NOTE: 3x3 Columns are camera axes
   union {
    mat4 cam_from_world;  // NOTE: 3x3 Rows are camera axes
    struct {
     union {v4 x_; v3 x;};
     union {v4 y_; v3 y;};
     union {v4 z_; v3 z;};
    };
   };
  };
 };
 
 Camera_Data_Embed;
 
 v1 focal_length;
 v1 near_clip;
 v1 far_clip;
};

function void
setup_camera(Camera *camera, Camera_Data *data) {
 *camera = {};
 camera->Camera_Data = *data;
 
 camera->near_clip    = 1*centimeter;
 camera->far_clip     = 10.f;
 camera->focal_length = 0.6169f;
 camera->transform = (/*mat4i_translate(data->pan) * */
                      mat4i_rotate_tpr(data->theta, data->phi, data->roll, data->pivot) *
                      mat4i_translate(data->pivot+V3z(data->distance)));
}

function mat4
get_view_from_world(Camera *camera, b32 orthographic) {
 mat4 result;
 v1 focal = camera->focal_length;
 v1 n = camera->near_clip;
 v1 f = camera->far_clip;
 
 //NOTE: Compute the depth from z
 //NOTE: revserse z
 result = mat4{{
   1,0, 0,0,
   0,1, 0,0,
   0,0,-1,0,
   0,0, 0,1,
  }} * camera->cam_from_world;
 
 if (orthographic) {
  //NOTE All objects depth are at the origin's depth
  v1 d = camera->distance;
  mat4 ortho = {{
    focal,0,0,0,
    0,focal,0,0,
    0,0,2*d/(f-n),-d*(f+n)/(f-n),
    0,0,0,d,
   }};
  result = ortho*result;
 } else {
  //NOTE perspective
  mat4 perspectiveT = {{
    focal, 0,     0,            0,
    0,     focal, 0,            0,
    0,     0,     (n+f)/(f-n), -2*f*n/(f-n),
    0,     0,     1,            0,
   }};
  result = perspectiveT*result;
 }
 return result;
}

//~;game_config
global i1  bezier_poly_nslice = 16;

struct Viewport{
 i1 index;  //NOTE(kv) Redundant data
 i1 preset;
 i1 last_preset;
 Camera_Data camera;
 Camera_Data target_camera;
 // ;viewport_frame_arena @frame_arena_init
 Arena render_arena;
 v2 clip_radius;
};
inline b32 is_main_viewport(Viewport *viewport){ return viewport->index==0; }

inline bool operator==(Bone_ID &a, Bone_ID &b){
 return (a.type==b.type) && (a.id==b.id);
}
inline Bone_ID
make_bone_id(Bone_Type type, i1 id=0){
 return Bone_ID{type, id};
}

struct Bone{
 Bone_ID id;
 mat4i   xform;
 b32     is_right;  //TODO(kv) What if this was in the id, hm?
 v3      center;
};

struct Painter
{
 //NOTE(kv) This is a convenient global store.
 //NOTE(kv) See @init_painter
 //-misc
 u32 hot_prim_id;
 Render_Target *target;
 Viewport *viewport;
 v3 cursorp;
 b32 cursor_on;
 u32 selected_obj_id;
 Camera camera;
 mat4  view_from_world;  // see @get_view_from_worldT
 mat4i cam_from_boneT;
 v1    meter_to_pixel_;
 v1   profile_score;  //TODO: @Cleanup axe this?
 b32  symx;
 v1 fill_depth_offset;  //TODO(kv) This should be part of the fill params?
 v1 line_radius_unit_mult;
 Line_Params line_params;
 //Fill_Params fill_params;  //NOTE(kv) smuggle this in the line params, temporarily
 i32 viz_level;
 b32 ignore_radii;
 b32 ignore_alignment_min;
 b32 painting_disabled;
 u32 draw_prim_id;
 
 Modeler *modeler;
 Bone *bones;
 Common_Line_Params *line_cparams;
 
 b32 show_grid;
 argb shade_color;
 Common_Line_Params **line_cparams_stack;
 b32 sending_data;
 b32 references_full_alpha;
 argb background_color;
 
 union{ b32 is_right; b32 lr_index; };
 Bone **bone_stack;
 
 i32 view_vector_count;
 v3  view_vector_stack[16];
};

global Painter *painter;  // see @init_painter

//myinline u32 get_hot_prim_id(){ return painter->hot_prim_id; }
myinline b32 is_poly_enabled(){
 return (painter->painting_disabled == false);
}
myinline b32 is_line_enabled(){
 return painter->painting_disabled == false;
}
myinline Line_Params lp(){ return painter->line_params; }
function Fill_Params fp(argb color=0){
 Fill_Params result = painter->line_params.fill;
 if(color){result.color = color;}
 return result;
}
inline Line_Params
lp(v4 radii){
 Line_Params result=painter->line_params;
 result.radii = radii;
 return result;
}
inline Line_Params
lp(i4 radii){
 Line_Params result = painter->line_params;
 result.radii = i2f6(radii);
 return result;
}
inline Line_Params
lp_invisible(){
 Line_Params result = painter->line_params;
 result.visibility = 0.0f;
 return result;
}

global argb hot_color      = argb_lightness(argb_silver, 0.5f);
global argb hot_color2     = argb_yellow;
global argb selected_color = argb_red;
global v1 default_line_radius_unit = 1.728125f * millimeter;

//-
myinline Bone *
current_bone(Painter *p) {
 return array_last(p->bone_stack);
}
myinline mat4i *
current_world_from_bone(Painter *p){
 return &current_bone(p)->xform;
}

myinline b32 is_right(Painter *p=painter){ return p->is_right; }
myinline b32 is_left (Painter *p=painter){ return !p->is_right; }
//-
// NOTE: Name,Denom
#define X_Pose_Fields(X) \
X(thead_theta, 6) \
X(thead_phi  , 6)  \
X(thead_roll , 6)  \
X(tblink     , 6)  \
X(teye_theta , 6)  \
X(teye_phi   , 6)  \
X(tarm_bend  , 18)  \
X(tarm_abduct, 36)  \

struct Pose{
#define X(NAME,...)   v1 NAME;
 X_Pose_Fields(X);
#undef X
};
//~
//NOTE(kv) You can only send one vert on one line
#define sending_vertices 0

#define send_vert(NAME)  //send_vert_func(painter, strlit(#NAME), NAME)

#define old_vv(NAME, VAL, ...) \
v3 NAME = VAL; \
send_vert(NAME)

#define old_va(NAME, VAL, ...) \
NAME = VAL;
send_vert(NAME)

/*
#define data_vv(NAME, ...) \
v3 NAME = vertex_from_name(painter->modeler, strlit(#NAME)).vertex->pos

#define data_va(NAME, ...) \
NAME = vertex_from_name(painter->modeler, strlit(#NAME)).vertex->pos
*/

//-
//~
#include "generated/framework_api.gen.h"

struct Framework_API
{
 b32 valid;
#define X(N) wrap_function_pointer(N);
 framework_api_xlist(X)
#undef X
};
//-
#include "generated/driver_api.gen.h"

struct Driver_API
{
 b32 is_valid;
#define X(N) wrap_function_pointer(N);
 driver_api_xlist(X)
#undef X
};

//~
#if AD_IS_DRIVER
#define X(N)  global wrap_function_pointer(N);
framework_api_xlist(X)
#undef X
#endif

#if AD_IS_FRAMEWORK
#define X(N)  function wrap_function(N);
framework_api_xlist_1(X)
#undef X
#endif

//-
function void
set_bone_transform(mat4i const&transform)
{
 Painter *p = painter;
 p->cam_from_boneT = invert(p->camera.transform) * transform;
 push_object_transform_to_target(p->target, cast(mat4*)&transform.m);
}

myinline v3
camera_world_position(Camera *camera){
 v3 result = mat4vert(camera->world_from_cam, V3());
 return result;
}
function v3
camera_object_position(Painter *p){
 v3 result = (current_world_from_bone(p)->inv *
              camera_world_position(&painter->camera));
 return result;
}
function void
push_view_vector(Painter *p, v3 object_center){
 v3 camera_obj = camera_object_position(p);
 v3 view_vector = noz(camera_obj - object_center);
 p->view_vector_stack[p->view_vector_count++] = view_vector;
 kv_assert(p->view_vector_count < alen(p->view_vector_stack));
}
inline v3
get_view_vector(){
 return painter->view_vector_stack[painter->view_vector_count-1];
}
inline void
pop_view_vector(Painter *p){
 p->view_vector_count--;
 kv_assert(p->view_vector_count > 0);
}
#define view_vector_block(center) \
push_view_vector(painter, center); \
defer(pop_view_vector(painter));

function Bone *
get_bone(Modeler *m, Bone_ID id, b32 is_right)
{
 Bone *bones = get_bones(m);
 for_u32(index, 0, array_count(bones))
 {
  Bone *bone = &bones[index];
  if(bone->id       == id and
     bone->is_right == is_right){
   return bone;
  }
 }
 return &bones[0];
}
myinline Bone *
get_bone(Modeler *m, Bone_Type type, b32 is_right)
{
 return get_bone(m, make_bone_id(type), is_right);
}
function void
push_bone_inner(Modeler *m, Bone **stack,
                b32 is_right,
                Bone_ID id, mat4i const&mom_from_kid)
{
 mat4i mom = array_last(stack)->xform;
 Bone *bone = get_bone(m, id, is_right);
 if(bone->id.type == 0){
  Bone *bones = get_bones(m);
  bone = array_push(bones);
  *bone = {};
  bone->id       = id;
  bone->is_right = is_right;
  bone->xform    = matmul(mom, mom_from_kid);
 }
 *array_push(stack) = bone;
}
inline void
push_bone_inner(Modeler *m, Bone **stack, b32 is_right,
                Bone_Type type, mat4i const&mom_from_kid)
{
 push_bone_inner(m, stack, is_right, make_bone_id(type), mom_from_kid);
}

function void
push_bone(Painter *p, Bone_ID id, v3 center={})
{
 Modeler *m  = p->modeler;
 Bone *bone = get_bone(m, id, p->is_right);
 *array_push(p->bone_stack) = bone;
 set_bone_transform(bone->xform);
}
myinline void
push_bone(Painter *p, Bone_Type type, v3 center={})
{
 return push_bone(p, make_bone_id(type), center);
}
function void
pop_bone(Painter *p){
 array_pop(p->bone_stack);
 mat4i *parent = current_world_from_bone(p);
 set_bone_transform(*parent);
}
#define bone_block(id)  push_bone(painter, id); defer(pop_bone(painter););
//-

function Common_Line_Params&
get_line_cparams(i1 linum)
{
 Common_Line_Params *list = painter->line_cparams;
 for_u32(i,0,array_count(list)){
  auto &cparams = list[i];
  if(cparams.linum == linum){ return cparams; }
 }
 return list[0];
}
function void
use_line_cparams(Common_Line_Params &cparams){
 Line_Params &params = painter->line_params;
 params.flags = cparams.flags;
 params.radii = cparams.radii;
}
inline Common_Line_Params_Index
current_line_cparams_index();

function void
push_line_cparams(Common_Line_Params &value, i1 linum=__builtin_LINE()){
 Modeler *m = painter->modeler;
 Common_Line_Params *list = painter->line_cparams;
 Common_Line_Params *address = &get_line_cparams(linum);
 if(address == &list[0]){// not found
  value.linum = linum;
  array_push_value(list, value);
  address = array_lastp(list);
 }
 array_push_value(painter->line_cparams_stack, address);
 use_line_cparams(value);
}
function void
pop_line_cparams(){
 Painter *p = painter;
 array_pop(p->line_cparams_stack);
 use_line_cparams(*array_last(p->line_cparams_stack));
}
inline Common_Line_Params *
current_line_cparams(){
 return array_last(painter->line_cparams_stack);
}
inline Common_Line_Params_Index
current_line_cparams_index(){
 //NOTE(kv) This "index" business is so ridiculous,... but we gotta pull through
 Common_Line_Params_Index result;
 Common_Line_Params *base = painter->line_cparams;
 result.v = current_line_cparams() - base;
 return result;
}
//-