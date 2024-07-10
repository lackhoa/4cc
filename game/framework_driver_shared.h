#pragma once

#if AD_COMPILING_FRAMEWORK
global_extern u32 draw_cycle_counter;
#else
extern u32 draw_cycle_counter;
#endif

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

//NOTE: This is kinda like a convenient global store. See
//TODO: Maybe we should just axe this concept and use globals?
struct Painter  // @init_painter
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
 b32 is_viz;
 i32 viz_level;
 b32 disable_radii;
 b32 painting_disabled;
 u32 draw_prim_id;
 
 i32 transform_count;
 mat4i transform_stack[16];
 
 i32 view_vector_count;
 v3 view_vector_stack[16];
 
 // @Verified @Clang
 force_inline operator Line_Params const&() { return line_params; }
};

global_const argb hot_color      = argb_silver;
global_const argb hot_color2     = argb_yellow;
global_const argb selected_color = argb_red;


#if AD_COMPILING_DRIVER

extern Painter painter;

#else

global_extern Painter painter;  // @init_painter

#endif

inline u32
get_hot_prim_id()
{
 return painter.target->current_prim_id;
}

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

internal mat4i &
get_object_transform()
{
 kv_assert(painter.transform_count > 0);
 return painter.transform_stack[painter.transform_count-1];
}


#if AD_COMPILING_DRIVER

extern "C" draw_bezier_inner_return draw_bezier_inner(draw_bezier_inner_params);
extern "C" fill3_inner_return fill3_inner(fill3_inner_params);
extern "C" draw_disk_inner_return draw_disk_inner(draw_disk_inner_params);

#else

extern "C" fill3_inner_return
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
 b32 is_selected = (painter.draw_prim_id == selected_obj_id);
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

extern "C" draw_disk_inner_return
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

extern "C" draw_bezier_inner_return
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
     P_transformed[index] = mat4vert(transform, P[index]);
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
#if AD_COMPILING_DRIVER
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
void movie_update(Game_State *state);

#undef DECL