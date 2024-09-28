#pragma once

#include "4coder_game_shared.h"
#include "game_colors.cpp"
#include "ad_debug.h"

// @distance_level_nonsense
// TODO: NOT HAPPY with storing redundant data!
struct Camera_Data  // IMPORTANT: @Serialized
{
#define X_Camera_Data(X) \
X(v1,distance) \
X(v1,phi)      \
X(v1,theta)    \
X(v1,roll)     \
X(v3,pan)      \
X(v3,pivot)    \
 
 X_Camera_Data(X_Field_Type_Name)
};


struct Bezier {
 v3 e[4];
 force_inline operator v3 *() { return e; };
};
typedef Bezier Bez;

//~ id system
// NOTE(kv): Primitives are either drawn by code or data.
introspect(info) struct Vertex_Index { i1 v; };
inline b32 operator ==(Vertex_Index a, Vertex_Index b){ return a.v == b.v; }
introspect(info) struct Curve_Index  { i1 v; };
inline b32 operator ==(Curve_Index a, Curve_Index b){ return a.v == b.v; }

enum Prim_Type : u8 {
 Prim_Null     = 0,
 Prim_Vertex   = 1,
 Prim_Curve    = 2,
 Prim_Triangle = 3,
};

inline Prim_Type prim_type_from_id(u32 id) { return Prim_Type(id >> 24); }
inline b32 prim_id_is_data(u32 id) { return prim_type_from_id(id) != 0; }

struct Prim_XID{
 u32       id;
 Prim_Type type;
 i1        index;
};
inline i1
prim_index_from_id(u32 id){
 if(prim_id_is_data(id)){ return (id & 0xFFFF); }
 return 0;
}
inline Prim_XID
prim_xid_from_id(u32 id){
 Prim_XID result = {};
 result.id    = id;
 result.type  = prim_type_from_id(id);
 result.index = prim_index_from_id(id);
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
//

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
 
#define X(type,name) type name;
 X_Camera_Data(X)
#undef X
 
 v1 focal_length;
 v1 near_clip;
 v1 far_clip;
};

function void
setup_camera(Camera *camera, Camera_Data *data) {
 *camera = {};
 
 camera->near_clip    = 1*centimeter;
 camera->far_clip     = 10.f;
 camera->focal_length = 0.6169f;
 
 // TODO We can just block copy here
#define X(type,name)   camera->name = data->name;
 X_Camera_Data(X)
#undef X
 
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

//-
#if AD_IS_FRAMEWORK
#    define framework_storage xglobal
#else
#    define framework_storage extern
#endif

//~;game_config
framework_storage i1  bezier_poly_nslice;
framework_storage v1  DEFAULT_NSLICE_PER_METER;
framework_storage b32 debug_frame_time_on;
//~

framework_storage u32 draw_cycle_counter;
framework_storage v1  default_fvert_delta_scale;

introspect(info)
typedef u32 Line_Flags;
enum{
 Line_Overlay  = 0x1,
 Line_Straight = 0x2,
 Line_No_SymX  = 0x4,
};

introspect(info)
typedef u32 argb;

introspect(info)
struct Line_Params{
 v4 radii;
 v1 nslice_per_meter;
 v1 visibility;
 v1 alignment_threshold;
 argb color;
 Line_Flags flags;
};

typedef u32 Poly_Flags;
enum{
 Poly_Shaded  = 0x1,  //NOTE: Has to be 1, for compatibility with old code.
 Poly_Line    = 0x2,
 Poly_Overlay = 0x4,
};

struct Fill_Params{
 //NOTE(kv) Might need to make a different set of flags for fill versus polygons.
 // because the polygon routine is shared by both fills and lines.
 b32 non_default;
 Poly_Flags flags;
};

struct Viewport {
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

introspect(info)
enum Bone_Type{
 Bone_None,
 //-
 Bone_Head          =1,
 Bone_Arm           =2,
 Bone_Forearm       =3,
 Bone_Bottom_Phalanx=4,
 Bone_Mid_Phalanx   =5,
 Bone_Top_Phalanx   =6,
 Bone_Torso         =7,
 Bone_References    =8,
 Bone_Hand          =9,
 Bone_Thumb         =10,
 Bone_Pelvis        =11,
};
introspect(info)
struct Bone_ID{
 Bone_Type type;
 i1        id;
};
inline b32 operator==(Bone_ID &a, Bone_ID &b){
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

struct Viewport;
struct Modeler;
//NOTE(kv) This is a convenient global store.
//NOTE(kv) See @init_painter
struct Painter{
 //-misc
 Render_Target *target;
 Viewport *viewport;
 //v2  mousep;  //NOTE(kv) In view space
 v3 cursorp;
 u32 selected_obj_id;
 Camera camera;
 mat4  view_from_world;  // see @get_view_from_worldT
 mat4i cam_from_boneT;
 v1    meter_to_pixel_;
 v1   profile_score;  //TODO: @Cleanup axe this?
 b32  symx;
 argb fill_color;
 v1 fill_depth_offset;
 v1 line_depth_offset;
 v1 line_radius_unit;
 Line_Params line_params;
 Fill_Params fill_params;
 i32 viz_level;
 b32 ignore_radii;
 b32 ignore_alignment_threshold;
 b32 painting_disabled;
 u32 draw_prim_id;
 Modeler *modeler;
 b32 show_grid;
 argb shade_color;
 
 union{ b32 is_right; b32 lr_index; };
 arrayof<Bone*> bone_stack;
 
 i32 view_vector_count;
 v3  view_vector_stack[16];
};

global_const argb hot_color      = argb_lightness(argb_silver, 0.5f);
global_const argb hot_color2     = argb_yellow;
global_const argb selected_color = argb_red;

framework_storage Painter painter;

//-
inline Bone *current_bone(Painter *p){ return p->bone_stack.last(); }
inline mat4i& current_world_from_bone(Painter *p) {
 return current_bone(p)->xform;
}

inline b32 is_right(Painter *p=&painter) { return p->is_right; }
inline b32 is_left(Painter *p=&painter)  { return !p->is_right; }
//-

#if AD_IS_FRAMEWORK
#include "game_modeler.h"
#endif

#include "game_draw.cpp"

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

struct Pose
{
#define X(NAME,...)   v1 NAME;
 X_Pose_Fields(X);
#undef X
};

//~NOTE Atrocity Alert! Transitional code to convert code to data.
#define line_params_defp Line_Params params=painter.line_params
xfunction void
send_bez_v3v2(String name, String p0_name, v3 d0, v2 d3, String p3_name, line_params_defp, linum_defparam);
#define bn_v3v2(name, p0,d0,d3,p3, ...) \
send_bez_v3v2(strlit(#name), strlit(#p0), d0, d3, strlit(#p3), __VA_ARGS__)

#define bs_v3v2(p0,d0,d3,p3, ...) \
send_bez_v3v2(strlit("l"), strlit(#p0), d0, d3, strlit(#p3), __VA_ARGS__)

#define bb_v3v2(name, p0,d0,d3,p3, ...) \
bn_v3v2(name, p0,d0,d3,p3, __VA_ARGS__); \
Bez name = bez_v3v2(p0,d0,d3,p3);

#define ba_v3v2(name, p0,d0,d3,p3, ...) \
bn_v3v2(name, p0,d0,d3,p3, __VA_ARGS__); \
name = bez_v3v2(p0,d0,d3,p3);
//-
xfunction void
send_bez_parabola(String name, String p0, v3 d, String p3, line_params_defp, linum_defparam);
#define bn_parabola(name, p0,d,p3, ...) \
send_bez_parabola(strlit(#name), strlit(#p0), d, strlit(#p3), __VA_ARGS__)

#define bs_parabola(p0,d,p3, ...) \
send_bez_parabola(strlit("l"), strlit(#p0), d, strlit(#p3), __VA_ARGS__)

#define bb_parabola(name, p0,d,p3, ...) \
bn_parabola(name, p0,d,p3, __VA_ARGS__); \
Bez name = bez_parabola(p0,d,p3);

#define ba_parabola(name, p0,d,p3, ...) \
bn_parabola(name, p0,d,p3, __VA_ARGS__); \
name = bez_parabola(p0,d,p3);
//-
xfunction void
send_vert_func(Painter *p, String name, v3 pos, i32 linum=__builtin_LINE());
//NOTE(kv) You can only send one vert on one line
#define send_vert(NAME)  send_vert_func(&painter, strlit(#NAME), NAME)

#define driver_animate_params Modeler *m, Arena *scratch, v1 anime_time
xfunction Pose driver_animate(driver_animate_params);

//TODO(kv) Maybe put the majority of this in the painter?
#define render_movie_params \
Arena *arena, Arena *scratch,  Render_Config *render_config, \
Viewport *viewport, b32 references_full_alpha, \
App *app, Render_Target *render_target, Modeler *modeler, \
Camera *camera, struct Pose *pose, v1 anime_time

xfunction void render_movie(render_movie_params);
xfunction void driver_update(Viewport *viewports);

function void
set_bone_transform(mat4i const&transform) {
 Painter *p = &painter;
 p->cam_from_boneT = invert(p->camera.transform) * transform;
 push_object_transform_to_target(p->target, cast(mat4*)&transform.m);
}

function v3
camera_object_position(Painter *p) {
 v3 result = (current_world_from_bone(p).inv *
              camera_world_position(&painter.camera));
 return result;
}

function void
push_view_vector(Painter *p, v3 object_center){
 v3 camera_obj = camera_object_position(p);
 v3 view_vector = noz(camera_obj - object_center);
 p->view_vector_stack[p->view_vector_count++] = view_vector;
 kv_assert(p->view_vector_count < alen(p->view_vector_stack));
}
//-
inline v3
get_view_vector(){
 return painter.view_vector_stack[painter.view_vector_count-1];
}
inline void
pop_view_vector(Painter *p){
 p->view_vector_count--;
 kv_assert(p->view_vector_count > 0);
}
#define view_vector_block(center) \
push_view_vector(&painter, center); \
defer(pop_view_vector(&painter));

//NOTE(kv)  Don't you dare return a reference here!
#if AD_IS_DRIVER
xfunction arrayof<Bone> *get_bones(Modeler *m);
#else
xfunction arrayof<Bone> *get_bones(Modeler *m) { return &m->bones; }
#endif

function Bone *
get_bone(Modeler *m, Bone_ID id, b32 is_right) {
 auto bones = get_bones(m);
 for_i32(index,0,bones->count){
  Bone *bone = &bones->get(index);
  if(bone->id       == id &&
     bone->is_right == is_right){
   return bone;
  }
 }
 return 0;
}
inline Bone *
get_bone(Modeler *m, Bone_Type type, b32 is_right) {
 return get_bone(m, make_bone_id(type), is_right);
}

function void
push_bone_inner(Modeler *m, arrayof<Bone *> *stack,
                b32 is_right,
                Bone_ID id, mat4i const&mom_from_kid)
{
 mat4i &mom = stack->last()->xform;
 Bone *bone = get_bone(m, id, is_right);
 if (!bone){
  auto bones = get_bones(m);
  bone = &bones->push(Bone{.id=id, .is_right=is_right});
 }
 
 mat4i xform = mom * mom_from_kid;
 bone->xform = xform;
 
 stack->push(bone);
}
inline void
push_bone_inner(Modeler *m, arrayof<Bone *> *stack, b32 is_right,
                Bone_Type type, mat4i const&mom_from_kid)
{
 push_bone_inner(m,stack,is_right,make_bone_id(type),mom_from_kid);
}

function void
push_bone(Painter *p, Bone_ID id, v3 center={})
{
 auto m  = p->modeler;
 Bone *bone = get_bone(m, id, p->is_right);
 p->bone_stack.push(bone);
 set_bone_transform(bone->xform);
}
inline void
push_bone(Painter *p, Bone_Type type, v3 center={}) {
 return push_bone(p, make_bone_id(type), center);
}
function void
pop_bone(Painter *p)
{
 p->bone_stack.pop();
 mat4i &parent = current_world_from_bone(p);
 set_bone_transform(parent);
}
#define bone_block(id) push_bone(&painter, id); defer(pop_bone(&painter););



//-
#undef framework_storage

//~