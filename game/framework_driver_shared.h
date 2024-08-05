#pragma once

#include "4coder_game_shared.h"
#include "game_colors.cpp"
#include "ad_debug.h"

#define X_Field_Type_Name(type,name) type name;

// @distance_level_nonsense
// TODO: NOT HAPPY with storing redundant data!
struct Camera_Data  // IMPORTANT: @Serialized
{
#define X_Camera_Data(X) \
X(v1,distance) \
X(v1,phi)      \
X(v1,theta)    \
X(i1,distance_level) \
X(v1,roll)     \
X(v3,pan)      \
X(v3,pivot)    \
 
 X_Camera_Data(X_Field_Type_Name)
};

global u32 game_save_magic_number;
global u32 game_save_version;

struct Viewport
{
 i1 preset;
 i1 last_preset;
 Camera_Data camera;
 Camera_Data target_camera;
 // ;viewport_frame_arena @frame_arena_init @frame_arena_clear
 Arena frame_arena;
};

struct Bezier
{
 v3 e[4];
 force_inline operator v3 *() { return e; };
};
typedef Bezier Bez;

// TODO(kv): we'll need to migrate off the X macro,
// can't use b32 because C++ doesn't like it when you define function overloads
// for both "b32" and "i1". No way to work around that afaict.
struct Vertex_Data
{
#define X_Vertex_Data(X) \
X(String, name) \
X(i1, object_index) \
X(i1, symx) \
X(v3, pos)  \
X(i1, basis_index) \
//
 X_Vertex_Data(X_Field_Type_Name)
};

struct Bezier_Data
{
#define X_Bezier_Data(X) \
X(String, name) \
X(i1, object_index) \
X(i1, symx) \
X(i1, p0_index) \
X(v3, p1) \
X(v3, p2) \
X(i1, p3_index) \
X(v4, radii) \
//
 X_Bezier_Data(X_Field_Type_Name)
};

struct Slow_Line_Map
{
 i32 cap;
 i32 count;
 struct Slow_Line_Map_Entry *map;
};

//~ id system
enum Primitive_Type : u8
{
 Prim_Null     = 0,
 Prim_Vertex   = 1,
 Prim_Curve    = 2,
 Prim_Triangle = 3,
};

force_inline Primitive_Type
prim_id_type(u32 id)
{
 return Primitive_Type(id >> 24);
}

force_inline b32
prim_id_is_obj(u32 prim_id)
{
 return prim_id_type(prim_id) != 0;
}

force_inline u32
prim_id_from_vertex_index(i1 index)
{
 u32 type = u32(Prim_Vertex) << 24;
 u32 result = (u32)(index) | type;
 return result;
}
//
force_inline u32
index_from_prim_id(u32 id)
{
 u32 result = 0;
 if ( prim_id_is_obj(id) )
 {
  result = (id & 0xFFFF);
 }
 return result;
}

force_inline u32
prim_id_from_curve_index(i1 index)
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

internal void
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
 
 camera->transform =  (mat4i_translate(data->pan) *
                       mat4i_rotate_tpr(data->theta, data->phi, data->roll, data->pivot) *
                       mat4i_translate(data->pivot+V3z(data->distance)));
}


//-
#if AD_IS_DRIVER
#    define framework_storage  global_extern
#else
#    define framework_storage  extern
#endif

//~;game_config
framework_storage i1 BEZIER_POLY_NSLICE;
framework_storage v1 DEFAULT_NSLICE_PER_METER;
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

struct Object
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
 kv_array<Object> object_list;
 kv_array<i1>     object_stack;  // NOTE: offset into the object list
 
 i32 view_vector_count;
 v3  view_vector_stack[16];
};

global_const argb hot_color      = argb_silver;
global_const argb hot_color2     = argb_yellow;
global_const argb selected_color = argb_red;

framework_storage Painter painter;

//-
inline i1
current_object_index()
{
 return painter.object_stack.last();
}

inline Object&
current_object()
{
 i1 index = painter.object_stack.last();
 return painter.object_list[index];
}
//
inline mat4i&
get_object_transform()
{
 return current_object().transform;
}
//
inline String
get_object_name()
{
 return current_object().name;
}

inline b32 is_left() { return painter.is_right == 0; }
//-

#include "game_ed.cpp"

#define fill3_inner_params v3 p0, v3 p1, v3 p2, \
argb c0, argb c1, argb c2, \
v1 depth_offset, Poly_Flags flags
#define fill3_inner_return void

#define draw_bezier_inner_return void
#define draw_bezier_inner_params v3 P[4], Line_Params *params

#define draw_disk_inner_return void
#define draw_disk_inner_params v3 center, v1 radius, argb color, v1 depth_offset, Poly_Flags flags

internal v3
bezier_sample(const v3 P[4], v1 u)
{
 v1 U = 1-u;
 return (1*cubed(U)      *P[0] +
         3*(u)*squared(U)*P[1] +
         3*squared(u)*(U)*P[2] +
         1*cubed(u)      *P[3]);
}

internal v1
bezier_sample(v4 P, v1 u)
{
 v1 U = 1-u;
 return (1*cubed(U)      *P.v[0] +
         3*(u)*squared(U)*P.v[1] +
         3*squared(u)*(U)*P.v[2] +
         1*cubed(u)      *P.v[3]);
}


#if AD_IS_DRIVER

draw_bezier_inner_return draw_bezier_inner(draw_bezier_inner_params);
fill3_inner_return fill3_inner(fill3_inner_params);
draw_disk_inner_return draw_disk_inner(draw_disk_inner_params);

#else

fill3_inner_return
fill3_inner(fill3_inner_params)
{// NOTE: Triangle
 TIMED_BLOCK(draw_cycle_counter);
 
 b32 is_viz     = (flags & Poly_Viz);
 b32 is_line    = (flags & Poly_Line);
 b32 is_overlay = (flags & Poly_Overlay);
 
 Render_Vertex vertices[3] = {};
 vertices[0].pos = p0;
 vertices[1].pos = p1;
 vertices[2].pos = p2;
 
 argb colors[3] = {c0,c1,c2};
 b32 is_hot      = (painter.draw_prim_id == get_hot_prim_id());
 b32 is_selected = (painter.draw_prim_id == global_modeler->selected_obj_id);
 if (is_hot || is_selected)
 {
  argb hl_color;
  if (is_hot)
  {
   hl_color = (c0 == hot_color) ? hot_color2 : hot_color;
  }
  else
  {
   kv_assert(is_selected);
   hl_color = argb_red;
  }
  for_i32(index,0,3) { colors[index] = hl_color; };
 }
 else if (is_viz && !is_line)
 {// ;poly_shading
  v3 normal = noz(cross(p1-p2, p0-p2));
  for_i32(index,0,3)
  {
   v4 colorv = argb_unpack(colors[index]);
   v1 darkness_fuzz = 0.3f;
   colorv.rgb *= lerp(darkness_fuzz,absolute(normal.z),1.f);
   colors[index] = argb_pack(colorv);
  }
 }
 
 for_i32(index,0,3)
 {
  vertices[index].color = colors[index];
 }
 
 for_u32(index,0,alen(vertices))
 {
  Render_Vertex *vertex = vertices+index;
  vertex->uvw          = V3();
  vertex->depth_offset = depth_offset;
  vertex->prim_id      = painter.draw_prim_id;
 }
 
 Vertex_Type type = Vertex_Poly;
 if (is_overlay) { type = Vertex_Overlay; }
 draw__push_vertices(painter.target, vertices, alen(vertices), type);
}

draw_disk_inner_return
draw_disk_inner(draw_disk_inner_params)
{
 // TODO: @Speed We can cull the circle if it's too far away, or just reduce sample count
 if (radius > 0.f)
 {
  const i32 CIRCLE_NSLICE = 8;
  i32 nslices = CIRCLE_NSLICE;  //NOTE: @Tested Minimum should be 8
  v1 interval = 1.f / v1(nslices);
  v3 last_sample;
  for_i32(index, 0, nslices+1)
  {
   v1 angle = interval * v1(index);
   v2 arm = radius*arm2(angle);
   v3 sample = center + mat4vec(painter.obj_to_camera.inverse, V3(arm, 0.f));//@slow
   if (index!=0)
   {
    fill3_inner(center, last_sample, sample,
                repeat3(color), depth_offset, flags);
   }
   last_sample = sample;
  }
 }
}

draw_bezier_inner_return
draw_bezier_inner(draw_bezier_inner_params)
{
 if (params->visibility > 0.f)
 {
  i32 nslices;
  v4 radii = params->radii;
  if (params->flags & Line_Straight) {
   radii = V4(radii[1]);  //note: don't wanna take radius from the tip
   nslices = 1;
  }
  else
  {//~NOTE: pre-pass
   // NOTE: Working in 2D is no good, because we don't take into account that samples are moving in 3D,
   // so they might be traveling longer distances due to the depth, 
   // and spaced out more when we look at them in 3D -> you'd underestimate the density in 2D
   Bezier P_transformed;
   {
    const mat4 &transform = get_object_transform().forward;
    for_i32(index,0,4)
    {
     P_transformed[index] = mat4vert_div(transform, P[index]);
    }
   }
   v1 the_length = 0.f;
   {
    i32 pre_nslices = 8;
    v1 interval = 1.0f / (v1)pre_nslices;
    v3 last_sample_transformed;
    for (i32 sample_index=0;
         sample_index < pre_nslices+1;
         sample_index++)
    {
     v1 t = interval * (v1)sample_index;
     v3 sample_transformed = bezier_sample(P_transformed,t);
     if (sample_index != 0)
     {
      the_length += lengthof(sample_transformed - last_sample_transformed);
     }
     last_sample_transformed = sample_transformed;
    }
   }
   
   nslices = i32(params->nslice_per_meter * the_length)+1;
   
   i32 MAX_NSLICES = 128;
   if (nslices > MAX_NSLICES) {
    DEBUG_VALUE(nslices);
   }
   macro_clamp_max(nslices, MAX_NSLICES);
  }
  
  Poly_Flags poly_flags = Poly_Line;
  if (params->flags & Line_Overlay) { poly_flags |= Poly_Overlay; }
  
  v1 radius_threshold = 0.1065f*millimeter;
  macro_clamp_min(radius_threshold, 0.f);
  
  v1 interval = 1.0f / (v1)nslices;
  
  Vertex_Type vertex_type = Vertex_Poly; //@temp
  
  argb color = params->color;
  //NOTE: the hot color is deferred to the triangle filling routine
  
  v3 last_sample = bezier_sample(P,-interval);  //NOTE: so we can calculate C and D at t=0
  v1 last_radius;
  v3 A,B;
  for (i32 sample_index=0;
       sample_index < nslices+1;
       sample_index++)
  {
   v1 t = interval * (v1)sample_index;
   v3 sample = bezier_sample(P,t);
   v1 radius = bezier_sample(radii,t);
   
   v3 C,D;
   {
    v3 perp1;
    {
     v3 d_camera_space = mat4vec(painter.obj_to_camera, (sample - last_sample));
     v2 perp_unit = noz( perp(d_camera_space.xy) );
     perp1 = mat4vec(painter.obj_to_camera.inverse, V3(radius*perp_unit, 0.f));
    }
    C = sample+perp1;
    D = sample-perp1;
   }
   
   if (sample_index!=0)
   {
    // NOTE: Clean up ugly tiny lines (and zeros), possibly unnecessary?
    if (0.5f*(radius+last_radius) >= radius_threshold)
    {
     fill3_inner(A,C,D, repeat3(color), params->depth_offset, poly_flags);
     fill3_inner(A,B,D, repeat3(color), params->depth_offset, poly_flags);
    }
   }
   
   A = C;
   B = D;
   last_sample = sample;
   last_radius = radius;
  }
  
  for_i32(index,0,2)
  {//NOTE: Draw the circular endpoints
   v1 t = 0.f;
   if (index==1) { t=1.f; }
   v3 center = bezier_sample(P,t);
   v1 radius = bezier_sample(radii,t);
   if (radius > radius_threshold)
   {
    draw_disk_inner(center, radius, color, params->depth_offset, poly_flags);
   }
  }
 }
}

#endif

// NOTE: Actually bernstein basis
internal v1
cubic_bernstein(i32 index, v1 t)
{
 v1 factor = v1((index == 1 || index == 2) ? 3 : 1);
 v1 result = factor * integer_power(t,index) * integer_power(1.f-t, 3-index);
 return result;
}
// NOTE: Actually bernstein basis
internal v1
quad_bernstein(i32 index, v1 t)
{
 v1 result = (index==0 ? squared(1-t) :
              index==1 ? 2*(1-t)*t :
              squared(t));
 return result;
}

force_inline void
poly4_inner(v3 p0, v3 p1, v3 p2, v3 p3, 
            argb c0, argb c1, argb c2, argb c3,
            v1 depth_offset, u32 flags)
{
 fill3_inner(p0,p1,p2, c0,c1,c2, depth_offset, flags);
 fill3_inner(p0,p2,p3, c0,c2,c3, depth_offset, flags);
}

#if 0
jump fill_patch_inner();
#endif
//
extern "C" void
fill_patch_inner(const v3 P[4][4],
                 argb color, v1 depth_offset,
                 b32 viz)
#if AD_IS_DRIVER
;
#else
{
 const i32 nslice = 16;
 v1 inv_nslice = 1.0f / (v1)nslice;
 v3 prev_v[nslice+1];
 v3 this_v[nslice+1];
 
 for_i32(v_index, 0, nslice+1) 
 {
  for_i32(u_index, 0, nslice+1)
  {
   v1 u = inv_nslice * (v1)u_index;
   v1 v = inv_nslice * (v1)v_index;
   v3 world_p = {};
   for_i32(i,0,4)
   {
    for_i32(j,0,4)
    {
     world_p += (cubic_bernstein(i,v) *
                 cubic_bernstein(j,u) *
                 P[i][j]);
    }
   }
   this_v[u_index] = world_p;
  }
  
  if (v_index != 0)
  {// NOTE: Draw triangles between last_v and this_v
   for_i32(u_index, 0, nslice)
   {
    poly4_inner(prev_v[u_index],   this_v[u_index],
                this_v[u_index+1], prev_v[u_index+1],
                color,color,color,color,
                depth_offset, viz);
   }
  }
  
  block_copy_array(prev_v, this_v);
 }
} 
#endif

#define render_movie_return void
#define render_movie_params Game_State *state, App *app, Render_Target *render_target, i1 viewport_id, Mouse_State mouse

render_movie_return render_movie(render_movie_params);
void driver_update(Game_State *state);

// NOTE: Planar curve (with v3 control point)
Bezier bez(v3 p0, v3 d0, v2 d3, v3 p3)
#if AD_IS_DRIVER
;
#else
{
 v3 w, p1;
 {
  v3 u = p3 - p0;
  p1 = (2.f*p0 + p3)/3.f + d0;
  w = noz( cross(u,d0) );
 }
 v3 p2;
 {
  v3 u = p3-p1;
  v3 v = cross(w, u);  // NOTE: u and v has the same magnitude
  p2 = 0.5f*(p1+p3) + (d3.x*u + d3.y*v);
 }
 return Bezier{p0, p1, p2, p3};
}
#endif

//NOTE: The state is saved between reloads.
struct Game_State
{
 Arena permanent_arena;
 Arena load_arena;  //NOTE: cleared on load
 
 b32 has_done_backup;
 String save_dir;
 String save_path_text;
 
 //-NOTE: Misc
 v1 time;
 b32 indicator_level;
 b32 references_full_alpha;
 Viewport viewports[GAME_VIEWPORT_COUNT];
 b32 save_failed;
 b32 load_failed;
 
 //-FUI
 // NOTE: We store things in the state to allow reload (reusing memory).
 // see @FUI_reload
 i32                    line_cap;
 struct Line_Map_Entry *line_map;
 Arena slider_store;
 //-Slow Slider Path
 Arena slow_slider_store;
 Slow_Line_Map slow_line_map;
 
 Modeler modeler;
 
 b8 __padding[64];
};

force_inline u32
selected_object_id()
{
 return global_modeler->selected_obj_id;
}

force_inline i1
selected_vertex_index(Game_State *state)
{
 i1 result = 0;
 u32 id = selected_object_id();
 if ( prim_id_type(id) == Prim_Vertex )
 {
  result = index_from_prim_id(id);
 }
 return result;
}

internal void
set_object_transform(mat4i const&transform)
{
 Painter *p = &painter;
 p->obj_to_camera = invert(p->camera->transform) * transform;
 push_object_transform_to_target(p->target, &transform.m);
}

inline v3
camera_world_position(Camera *camera)
{
 v3 result = mat4vert(camera->forward, V3());  //camera->distance * camera->z.xyz + camera->pan;
 return result;
}

internal v3
camera_object_position()
{
 v3 result = get_object_transform().inv * camera_world_position(painter.camera);
 return result;
}

internal void
push_view_vector(v3 object_center)
{
 auto &p = painter;
 v3 camera_obj = camera_object_position();
 v3 view_vector = noz(camera_obj - object_center);
 p.view_vector_stack[p.view_vector_count++] = view_vector;
 kv_assert(p.view_vector_count < alen(p.view_vector_stack));
}
//-
inline v3
get_view_vector()
{
 return painter.view_vector_stack[painter.view_vector_count-1];
}

inline void
pop_view_vector()
{
 auto &p = painter;
 p.view_vector_count--;
 kv_assert(p.view_vector_count > 0);
}

void
push_object(mat4i const&child_to_parent, v3 center, String name)
#if AD_IS_DRIVER
;
#else
{
 auto &p = painter;
 
 i1 object_index = -1;
 {
  auto &list = p.object_list;
  if( name != String{} )
  {
   for_i32(index,0,list.count)
   {
    Object &object = list[index];
    if(object.name     == name &&
       object.is_right == p.is_right)
    {
     object_index = index;
     break;
    }
   }
  }
  
  if (object_index == -1)
  {
   object_index = list.count;
   mat4i &parent = get_object_transform();
   mat4i new_transform = parent * child_to_parent;
   list.push(Object{.name=name, .transform=new_transform, .is_right=p.is_right});
  }
 }
 
 p.object_stack.push(object_index);
 push_view_vector(center);
 
 set_object_transform( get_object_transform() );
}
#endif
// @Overload
inline void
push_object(mat4i const&child_to_parent, v3 center={}, char *name="")
{
 push_object(child_to_parent, center, SCu8(name));
}
// @Overload
inline void
push_object(mat4i const&child_to_parent, char *name)
{
 return push_object(child_to_parent, V3(), SCu8(name));
}

internal void
pop_object()
{
 Painter &p = painter;
 p.object_stack.pop();
 mat4i &parent = get_object_transform();
 set_object_transform(parent);
 pop_view_vector();
}

#define symx_off set_in_block(painter.symx, false)
#define symx_on  set_in_block(painter.symx, true)

force_inline b32 
is_poly_enabled()
{
 return (painter.painting_disabled == false);
}

internal void
draw_disk(v3 center, v1 radius,
          argb color, v1 depth_offset, Poly_Flags flags,
          i32 linum = __builtin_LINE())
{
 if ( is_poly_enabled() )
 {
  painter.draw_prim_id = linum;
  draw_disk_inner(center, radius, color, depth_offset, flags);
 }
}

void
indicate_vertex(char *vertex_name, v3 pos,
                i32 force_draw_level=9000,
                b32 force_overlay=false,
                argb color=argb_yellow,
                u32 prim_id = __builtin_LINE())
#if AD_IS_DRIVER
;
#else
{
 Painter *p = &painter;
 painter.draw_prim_id = prim_id;
 b32 mouse_near;
 const v1 radius = 3*millimeter;
 {//NOTE: above
  mat4 object_to_view = p->view_transform * get_object_transform();
  v3 vertex_viewp = mat4vert_div(object_to_view, pos);
  v2 delta = painter.mouse_viewp - vertex_viewp.xy;
  mouse_near = (absolute(delta.x) < 1*centimeter && 
                absolute(delta.y) < 1*centimeter);
 }
 
 b32 mouse_on_top = (prim_id == get_hot_prim_id());
 
 b32 should_draw = ((painter.viz_level >= force_draw_level) || mouse_near);
 if (should_draw)
 {//NOTE: Draw
  symx_off;
  v1 depth_offset = painter.line_depth_offset - 1*centimeter;
  u32 flags = 0;
  // NOTE: If lines are overlayed, so should indicators (I guess?)
  b32 line_overlay_on = (painter.line_params.flags & Line_Overlay);
  if (line_overlay_on || force_overlay) {
   flags = Poly_Overlay;
  }
  draw_disk(pos, radius, color, depth_offset, flags, prim_id);
 }
 
 if ( mouse_on_top )
 {// NOTE: debug
  DEBUG_NAME(vertex_name, prim_id);
 }
}
#endif

#define linum_param     i32 linum = __builtin_LINE()
#define set_linum       if (linum!=0) { painter.draw_prim_id = linum; }

force_inline b32 is_line_enabled()
{
 return painter.painting_disabled == false;
}

internal v1
get_curve_view_alignment(const v3 P[4])
{
 Painter *p = &painter;
 v3 A = P[0];
 v3 B = P[1];
 v3 C = P[2];
 v3 D = P[3];
 v3 normal = noz( cross(B-A, D-A) );  // NOTE: the normal is only defined when the curve is planar; choosing ABD or ACD is arbitrary
 v3 centroid = 0.5f*(A+D);  // NOTE: our curves are kinda straight most of the time, so I guess this works
 v3 camera_obj = (get_object_transform().inv * camera_world_position(p->camera));  // TODO @speed
 v3 view_vector = noz(camera_obj - centroid);
 v1 visibility = absolute( dot(normal,view_vector) );
 return visibility;
}

force_inline Bezier 
negateX(Bezier line) 
{
 for_i32(i,0,4) { line[i].x = -line[i].x; }
 return line;
}

b32
draw(const v3 P0[4], Line_Params params, linum_param)
#if AD_IS_DRIVER
;
#else
{
 set_linum;
 Painter *p = &painter;
 b32 ok = is_line_enabled();
 if (ok && !p->ignore_alignment_threshold &&
     (params.alignment_threshold > 0.f))
 {
  ok = get_curve_view_alignment(P0) > params.alignment_threshold;
 }
 
 if (ok)
 {
  b32 symx = p->symx && !(params.flags & Line_No_SymX);
  
  const i32 npoints = 4;
  v3 points[npoints];
  block_copy_array(points,P0);
  
  // NOTE: reflect
  v3 reflects[npoints];
  if (symx) { for_i32 (ipoint,0,npoints) { reflects[ipoint] = negateX(points[ipoint]); } }
  
  // NOTE: Processing parameters
  if (p->ignore_radii || (params.radii == v4{})) {
   params.radii = p->line_params.radii;
  }
  if (params.nslice_per_meter <= 0.f) { params.nslice_per_meter = DEFAULT_NSLICE_PER_METER; }
  params.radii *= p->line_radius_unit;
  
  draw_bezier_inner(points, &params);
  if (symx) {
   draw_bezier_inner(reflects, &params);
  }
 }
 
 return ok;
}
#endif

// NOTE: Line
force_inline Bezier
bez(v3 a, v3 b)
{
 return Bezier{
  a,
  (2.f*a+b)/3.f,
  (a+2.f*b)/3.f,
  b
 };
}

//NOTE: radii in fractions
force_inline void
draw(Bezier b, v4 radii, linum_param)
{
 Line_Params params = painter.line_params;
 params.radii = radii;
 draw(b, params, linum);
}
//NOTE radii in sixths
force_inline void
draw(Bezier b, i4 radii, linum_param)
{
 Line_Params params = painter.line_params;
 params.radii = i2f6(radii);
 draw(b, params, linum);
}
// NOTE: omit params
force_inline void
draw(Bezier b, linum_param)
{
 draw(b, painter.line_params, linum);
}
// NOTE: straight line
force_inline void
draw(v3 a, v3 b, Line_Params params=painter.line_params, linum_param)
{
 draw(bez(a,b), params, linum);
}

inline Line_Params
hl_line_params(argb color=0)
{
 if (color == 0) { color = srgb_to_linear(0xffDBA50F); }
 Line_Params params = painter.line_params;
 params.flags |= Line_Overlay|Line_No_SymX;
 params.color = color;
 params.radii = i2f6(I4(3,3,3,3));
 return params;
}

#define hl_block_inner(color) \
set_in_block(painter.line_params, hl_line_params(color));

#define hl_block hl_block_inner(0)
#define hl_block_color hl_block_inner

#if AD_IS_FRAMEWORK
internal Object &
get_right_object(Object &object)
{
 auto &p = painter;
 Object *result = &object;
 for_i32(object_index, 0, p.object_list.count)
 {
  auto &objectR = p.object_list[object_index];
  if (objectR.is_right &&
      (objectR.name == object.name))
  {
   result = &objectR;
   break;
  }
 }
 
 return *result;
}
#endif

internal Bez
operator*(mat4 transform, Bez bezier)
{
 Bez result;
 for_i32(index,0,4)
 {
  result[index] = transform*bezier[index];
 }
 return result;
}

void
render_data(Game_State &state)
#if AD_IS_DRIVER
;
#else
{
 hl_block;
 auto &m = state.modeler;
 auto &p = painter;
 argb inactive_color = argb_dark_green;
 for_i32(vindex,1,m.vertices.count)
 {
  Vertex_Data &vert = m.vertices[vindex];
  Object &object = p.object_list[vert.object_index];
  u32 prim_id = prim_id_from_vertex_index(vindex);
  {
   mat4 &mat = object.transform;
   v3 pos = mat4vert(mat, vert.pos);
   indicate_vertex("data", pos, 0, true, inactive_color, prim_id);
  }
  if (vert.symx)
  {//NOTE: Draw the right side (TODO @speed stupid object lookup)
   auto &objectR = get_right_object(object);
   mat4 &mat = objectR.transform;
   v3 pos = mat4vert(mat, vert.pos);
   indicate_vertex("data", pos, 0, true, inactive_color, prim_id);
  }
 }
 
 for_i32(curve_index,1,m.beziers.count)
 {
  Bezier_Data &bez = m.beziers[curve_index];
  auto &object = p.object_list[bez.object_index];
  {
   v3 p0 = m.vertices[bez.p0_index].pos;
   v3 p1 = bez.p1;
   v3 p2 = bez.p2;
   v3 p3 = m.vertices[bez.p3_index].pos;
   
   Bez draw_bez = Bez{p0,p1,p2,p3};
   {
    draw(object.transform*draw_bez);
   }
   if (bez.symx)
   {
    auto &objectR = get_right_object(object);
    draw(objectR.transform*draw_bez);
   }
  }
 }
}
#endif


//-
#undef framework_storage

//~