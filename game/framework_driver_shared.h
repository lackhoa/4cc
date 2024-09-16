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

struct Viewport {
 i1 preset;
 i1 last_preset;
 Camera_Data camera;
 Camera_Data target_camera;
 // ;viewport_frame_arena @frame_arena_init
 Arena render_arena;
};

struct Bezier {
 v3 e[4];
 force_inline operator v3 *() { return e; };
};
typedef Bezier Bez;

//~ id system
// NOTE(kv): Primitives are either drawn by code or data.
enum Prim_Type : u8
{
 Prim_Null     = 0,
 Prim_Vertex   = 1,
 Prim_Curve    = 2,
 Prim_Triangle = 3,
};

force_inline Prim_Type
prim_id_type(u32 id) {
 return Prim_Type(id >> 24);
}

force_inline b32
prim_id_is_data(u32 prim_id) {
 return prim_id_type(prim_id) != 0;
}

inline u32
vertex_prim_id(i1 index)
{
 u32 type = u32(Prim_Vertex) << 24;
 u32 result = (u32)(index) | type;
 return result;
}
//
inline i1
prim_id_to_index(u32 id)
{
 u32 result = 0;
 if( prim_id_is_data(id) ) {
  result = (id & 0xFFFF);
 }
 return result;
}
//
force_inline u32
curve_prim_id(i1 index)
{
 u32 type = u32(Prim_Curve) << 24;
 u32 result = (u32)(index) | type;
 return result;
}

//~

global char *global_debug_scope;
#define vertex_block(NAME) set_in_block(global_debug_scope, NAME)

#define DEBUG_NAME(NAME, VALUE)  DEBUG_VALUE_inner(global_debug_scope, NAME, VALUE, 0)
#define DEBUG_NAME_COLOR(NAME, VALUE, COLOR)  DEBUG_VALUE_inner(global_debug_scope, NAME, VALUE, COLOR)
#define DEBUG_VALUE(VALUE)       DEBUG_VALUE_inner(global_debug_scope, #VALUE, VALUE)
#define DEBUG_TEXT(TEXT)         DEBUG_VALUE_inner(global_debug_scope, TEXT, 0.f)
//~

struct Camera
{
 union {
  mat4i transform;
  struct {
   mat4 forward;   // NOTE: 3x3 Columns are camera axes
   union {
    mat4 inverse;  // NOTE: 3x3 Rows are camera axes
    struct {v4 x,y,z,w;};
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
setup_camera(Camera *camera, Camera_Data *data)
{
 *camera = {};
 
 camera->near_clip    = 1*centimeter;
 camera->far_clip     = 10.f;
 camera->focal_length = 0.6169f;
 
 // TODO We can just block copy here
#define X(type,name)   camera->name = data->name;
 X_Camera_Data(X)
#undef X
 
 camera->transform = (mat4i_translate(data->pan) *
                      mat4i_rotate_tpr(data->theta, data->phi, data->roll, data->pivot) *
                      mat4i_translate(data->pivot+V3z(data->distance)));
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

enum Poly_Flag
{
 Poly_Viz     = 0x1,  //NOTE: this has to be 1 for compatibility with old code
 Poly_Line    = 0x2,
 Poly_Overlay = 0x4,
};
typedef u32 Poly_Flags;

enum Line_Flag
{
 Line_Overlay  = 0x1,
 Line_Straight = 0x2,
 Line_No_SymX  = 0x4,
 
 Line_Null     = 0x80,
};
typedef u32 Line_Flags;

struct Line_Params
{
 v1 nslice_per_meter;
 v4 radii;
 argb color;
 v1 visibility;
 v1 alignment_threshold;
 v1 depth_offset;
 Line_Flags flags;
};

struct Bone
{
 String name;
 mat4i transform;
 b32 is_right;
};

//NOTE: This is a convenient global store. See @init_painter
struct Painter  
{
 Render_Target *target;
 Viewport *viewport;
 v2 mouse_viewp;
 u32 selected_obj_id;
 Camera *camera;
 mat4  view_transform;  // see @camera_view_matrix
 mat4i obj_to_camera;
 v1    meter_to_pixel;
 v1   profile_score;  //TODO: @Cleanup axe this?
 b32  symx;
 argb fill_color;
 v1 fill_depth_offset;
 v1 line_depth_offset;
 v1 line_radius_unit;
 Line_Params line_params;
 i32 viz_level;
 b32 ignore_radii;
 b32 ignore_alignment_threshold;
 b32 painting_disabled;
 u32 draw_prim_id;
 
 b32 is_right;
 arrayof<Bone>   bone_list;
 arrayof<i1>     bone_stack;  // NOTE: Offset into the coframe list
 
 i32 view_vector_count;
 v3  view_vector_stack[16];
};

global_const argb hot_color      = argb_lightness(argb_silver, 0.5f);
global_const argb hot_color2     = argb_yellow;
global_const argb selected_color = argb_red;

framework_storage Painter painter;

//-
inline i1
current_bone_index() {
 return painter.bone_stack.last();
}

inline Bone&
current_bone() {
 i1 index = painter.bone_stack.last();
 return painter.bone_list[index];
}
//
inline mat4i& get_bone_transform() { return current_bone().transform; }
inline String get_bone_name()      { return current_bone().name; }

inline b32 is_left() { return painter.is_right == 0; }
//-

#if AD_IS_FRAMEWORK
#include "game_modeler.h"
#endif

#include "game_draw.cpp"

xfunction b32
send_bez_v3v2_func(String name, String p0_name, v3 d0, v2 d3, String p3_name);
//
#define send_bez_v3v2(name, p0_name, d0, d3, p3_name) \
send_bez_v3v2_func(strlit(#name), strlit(#p0_name), d0, d3, strlit(#p3_name))

#define render_movie_return void
#define render_movie_params \
Arena *arena, Arena *scratch, \
Viewport &viewport, v1 state_time, b32 references_full_alpha, \
App *app, Render_Target *render_target, i1 viewport_id, Mouse_State mouse

render_movie_return render_movie(render_movie_params);
void driver_update(Viewport *viewports);

function void
set_bone_transform(mat4i const&transform) {
 Painter *p = &painter;
 p->obj_to_camera = invert(p->camera->transform) * transform;
 push_object_transform_to_target(p->target, cast(mat4*)&transform.m);
}

function v3
camera_object_position() {
 v3 result = get_bone_transform().inv * camera_world_position(painter.camera);
 return result;
}

function void
push_view_vector(v3 object_center) {
 auto &p = painter;
 v3 camera_obj = camera_object_position();
 v3 view_vector = noz(camera_obj - object_center);
 p.view_vector_stack[p.view_vector_count++] = view_vector;
 kv_assert(p.view_vector_count < alen(p.view_vector_stack));
}
//-
inline v3
get_view_vector() {
 return painter.view_vector_stack[painter.view_vector_count-1];
}

inline void
pop_view_vector() {
 auto &p = painter;
 p.view_vector_count--;
 kv_assert(p.view_vector_count > 0);
}

xfunction void
push_object(mat4i const&child_to_parent, v3 center, String name)
#if AD_IS_DRIVER
;
#else
{
 auto &p = painter;
 
 i1 bone_index = -1;
 {
  auto &list = p.bone_list;
  if( name != String{} )
  {
   for_i32(index,0,list.count)
   {
    Bone &bone = list[index];
    if(bone.name     == name &&
       bone.is_right == p.is_right)
    {
     bone_index = index;
     break;
    }
   }
  }
  
  if (bone_index == -1)
  {
   bone_index = list.count;
   mat4i &parent = get_bone_transform();
   mat4i new_transform = parent * child_to_parent;
   list.push(Bone{.name=name, .transform=new_transform, .is_right=p.is_right});
  }
 }
 
 p.bone_stack.push(bone_index);
 push_view_vector(center);
 
 set_bone_transform( get_bone_transform() );
}
#endif
// @Overload
inline void
push_object(mat4i const&child_to_parent, v3 center={}, char *name="") {
 push_object(child_to_parent, center, SCu8(name));
}
// @Overload
inline void
push_object(mat4i const&child_to_parent, char *name) {
 return push_object(child_to_parent, V3(), SCu8(name));
}

function void
pop_object() {
 Painter &p = painter;
 p.bone_stack.pop();
 mat4i &parent = get_bone_transform();
 set_bone_transform(parent);
 pop_view_vector();
}

#undef framework_storage

//~