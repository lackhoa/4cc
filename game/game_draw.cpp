global char* GAME_DRAW_FILENAME = __FILE_NAME__;
global u32 g_draw_linum;


#define symx_off set_in_block(p->symx, false)
#define symx_on  set_in_block(p->symx, true)
#define viz_block_inner(color) \
set_in_block(p->line_params, viz_line_params(p, color)); \

#define viz_block viz_block_inner(0)
#define viz_block_color viz_block_inner

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

struct Poly_Params { };

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
 CAMERA_DATA_FIELDS(X)
#undef X
 
 v1 focal_length;
 v1 near_clip;
 v1 far_clip;
};

typedef i32 Edge_ID;
struct Bezier
{
 v3 e[4];
 force_inline operator v3 *() { return e; };
};
typedef Bezier Bez;

struct Patch
{
 v3 e[4][4];
 typedef v3 Array4x4[4][4];  // @stroustrup
 operator Array4x4&() { return e; } 
};

internal Bezier
get_column(Patch const&surface, i32 col)
{
 Bezier result;
 for_i32(index,0,4)
 {
  result[index] = surface.e[index][col];
 }
 return result;
}

internal Bezier
get_uline(Patch const&pat, v1 u)
{
 Bezier result;
 for_i32(index,0,4)
 {
  result[index] = bezier_sample(get_column(pat, index), u);
 }
 return result;
}

internal Bezier
get_vline(Patch const&pat, v1 v)
{
 Bezier result;
 for_i32(index,0,4)
 {
  result[index] = bezier_sample(pat.e[index], v);
 }
 return result;
}

enum Line_Flag
{
 Line_Overlay  = 0x1,
 Line_Straight = 0x2,
 Line_No_SymX  = 0x4,
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
 range2 range;
};

//NOTE: This is kinda like a convenient global store. See
//TODO: Maybe we should just axe this concept and use globals?
struct Painter  // @init_painter
{
 b32 painting_disabled;
 Render_Target *target;
 Camera *camera;
 mat4  view_transform;  // see @camera_view_matrix
 //mat4i object_transform;  //nono nah, group these in the painter, don't scatter them around -> we can later multi-thread it if needed
 v1    meter_to_pixel;
 v1   profile_score;  //TODO: @Cleanup axe this?
 v3   view_vector;
 b32  symx;
 b32  csg_negated;
 argb fill_color;
 v1 fill_depth_offset;
 v1 line_depth_offset;
 v1 line_radius_unit;
 Line_Params line_params;
 b32 poly_viz;
 i32 viz_level;
 
 // @Verified @Clang
 force_inline operator Line_Params const&() { return line_params; }
};

global Painter global_painter;  // game_render
global b32 csg_subtract_on;

force_inline b32 
poly_enabled(Painter *p)
{
 return (p->painting_disabled == false);
}

force_inline b32 line_enabled(Painter *p)
{
 return p->painting_disabled == false;
}

internal mat4
camera_view_matrix(Camera *camera, b32 orthographic)
{
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
  }} * camera->inverse;
 
 if (orthographic)
 {// NOTE
  //NOTE: The divide is based off the camera distance, not the z.
  v1 d = camera->distance;
  mat4 ortho = {{
    focal,0,0,0,
    0,focal,0,0,
    0,0,2*d/(f-n),-d*(f+n)/(f-n),
    0,0,0,d,
   }};
  result = ortho*result;
 }
 else
 {//NOTE
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

enum Poly_Flag
{
 Poly_Viz     = 0x1,  //NOTE: this has to be 1 for compatibility with old code
 Poly_Line    = 0x2,
 Poly_Overlay = 0x4,
};
typedef u32 Poly_Flags;

internal void
fill3_inner(Render_Target *target,
            v3 p0, v3 p1, v3 p2,
            argb c0, argb c1, argb c2,
            v1 depth_offset, Poly_Flags flags)
{// NOTE: Triangle
 b32 is_viz     = (flags & Poly_Viz);
 b32 is_line    = (flags & Poly_Line);
 b32 is_overlay = (flags & Poly_Overlay);
 
 Render_Vertex vertices[3] = {};
 vertices[0].pos = p0;
 vertices[1].pos = p1;
 vertices[2].pos = p2;
 
 argb colors[3] = {c0,c1,c2};
 if (is_viz && !is_line)
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
  vertex->uvw          = vec3();
  vertex->depth_offset = depth_offset;
  vertex->prim_id      = g_draw_linum;
 }
 
 Vertex_Type type = Vertex_Poly;
 if (is_overlay) { type = Vertex_Overlay; }
 draw__push_vertices(target, vertices, alen(vertices), type);
}

force_inline void
poly4_inner(Render_Target *target, 
            v3 p0, v3 p1, v3 p2, v3 p3, 
            argb c0, argb c1, argb c2, argb c3,
            v1 depth_offset, u32 flags)
{
 fill3_inner(target, p0,p1,p2, c0,c1,c2, depth_offset, flags);
 fill3_inner(target, p0,p2,p3, c0,c2,c3, depth_offset, flags);
}

inline v1
cubic_bernstein(i32 index, v1 u)
{
 v1 factor = (index == 1 || index == 2) ? 3 : 1;
 v1 result = factor * pow(u,index) * pow(1-u, 3-index);
 return result;
}

global Fui_Options ftens = {.delta_scale=10.f};

force_inline Fui_Options
fscale(v1 delta_scale)
{
 Fui_Options result = {};
 result.delta_scale = delta_scale;
 return result;
}

//@TransformAnnoyance
internal void
draw_diskf(Render_Target *target, mat4 const&camera_to_obj,
           v3 center, v1 radius,
           argb color, v1 depth_offset, Poly_Flags flags)
{
 // TODO: @Speed We can cull the circle if it's too far away, or just reduce sample count
 if (radius > 0.f)
 {
  i32 nslices = CIRCLE_NSLICE;  //NOTE: @Tested Minimum should be 8
  v1 interval = 1.f / v1(nslices);
  v3 last_sample;
  for_i32(index, 0, nslices+1)
  {
   v1 angle = interval * v1(index);
   v2 arm = radius*arm2(angle);
   v3 sample = center + mat4vec(camera_to_obj, vec3(arm, 0.f));//@slow
   if (index!=0)
   {
    fill3_inner(target, center, last_sample, sample,
                repeat3(color), depth_offset, flags);
   }
   last_sample = sample;
  }
 }
}

//@TransformAnnoyance
internal void
draw_bezier_inner(Render_Target *target, Camera *camera,
                  mat4i const&objectT, mat4 const&view_objectT,
                  v3 P[4], Line_Params *params)
{
 if (params->visibility > 0.f)
 {
  b32 new_triangle_method = fbool(1);
  if (new_triangle_method)
  {
   i32 nslices;
   if (params->flags & Line_Straight) {nslices = 1;}
   else
   {//~NOTE: pre-pass
    // NOTE: Working in 2D is no good, because we don't take into account that samples are moving in 3D,
    // so they might be traveling longer distances due to the depth, 
    // and spaced out more when we look at them in 3D -> you'd underestimate the density in 2D
    Bezier P_transformed;
    {
     const mat4 &transform = objectT.forward;
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
    if (nslices > MAX_NSLICES)
    {
     DEBUG_VALUE(nslices);
    }
    macro_clamp_max(nslices, MAX_NSLICES);
   }
   
   Poly_Flags poly_flags = Poly_Line;
   if (params->flags & Line_Overlay) { poly_flags |= Poly_Overlay; }
   
   v1 radius_threshold = fval(0.1065f)*millimeter;
   macro_clamp_min(radius_threshold, 0.f);
   
   mat4 obj_to_camera = camera->inverse * objectT.forward;
   mat4 camera_to_obj = objectT.inverse * camera->forward;
   
   v1 interval = 1.0f / (v1)nslices;
   
   Vertex_Type vertex_type = Vertex_Poly; //@temp
   
   argb color = params->color;
   v3 last_sample = bezier_sample(P,-interval);  //NOTE: so we can calculate C and D at t=0
   v1 last_radius;
   v3 A,B;
   for (i32 sample_index=0;
        sample_index < nslices+1;
        sample_index++)
   {
    v1 t = interval * (v1)sample_index;
    v3 sample = bezier_sample(P,t);
    v1 radius = bezier_sample(params->radii,t);
    
    v3 C,D;
    {
     v3 perp1;
     {
      v3 d_camera_space = mat4vec(obj_to_camera, (sample - last_sample));
      v2 perp_unit = noz( perp(d_camera_space.xy) );
      perp1 = mat4vec(camera_to_obj, vec3(radius*perp_unit, 0.f));
     }
     C = sample+perp1;
     D = sample-perp1;
    }
    
    if (sample_index!=0)
    {
     // NOTE: Clean up ugly tiny lines, possibly unnecessary?
     if (0.5f*(radius+last_radius) >= radius_threshold)
     {
      fill3_inner(target, A,C,D, repeat3(color), params->depth_offset, poly_flags);
      fill3_inner(target, A,B,D, repeat3(color), params->depth_offset, poly_flags);
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
    v1 radius = bezier_sample(params->radii,t);
    if (radius > radius_threshold)
    {
     draw_diskf(target, camera_to_obj,
                center, radius,
                color, params->depth_offset, poly_flags);
    }
   }
  }
 }
}

internal void
fill_bezier_inner_2(Render_Target *target, v3 P[4], v3 Q[4], argb color, v1 depth_offset, b32 viz)
{
 i32 nslices = BEZIER_POLY_NSLICE;
 v1 inv_nslices = 1.0f / (v1)nslices;
 v3 previous_worldP;
 v3 previous_worldQ;
 for_i32(sample_index, 0, nslices+1)
 {
  v1 u = inv_nslices * (v1)sample_index;
  v3 worldP = bezier_sample(P,u);
  v3 worldQ = bezier_sample(Q,u);
  //
  if (sample_index != 0)
  {
   poly4_inner(target, previous_worldP, previous_worldQ, worldQ, worldP, 
               repeat4(color), depth_offset, viz);
  }
  previous_worldP = worldP;
  previous_worldQ = worldQ;
 }
}

force_inline v3 
negateX(v3 vert) 
{
 return vec3(-vert.x, vert.y, vert.z);
}

force_inline Bezier 
negateX(Bezier line) 
{
 for_i32(i,0,4) { line[i].x = -line[i].x; }
 return line;
}

force_inline void
bez_bake(Bezier &line)
{
 line[1] += line[0];
 line[2] += line[3];
}

internal void
bezier_poly3_inner(Render_Target *target, 
                   v3 A, v3 P[4], 
                   argb c0, argb c1, argb c2, 
                   v1 depth_offset, b32 viz)
{
 i32 nslices = BEZIER_POLY_NSLICE;
 v1 inv_nslices = 1.f / (v1)nslices;
 v4 color1 = argb_unpack(c1);
 v4 color2 = argb_unpack(c2);
 v3 previous_worldP;
 v4 previous_color;
 for_i32(sample_index,0,nslices+1)
 {
  v1 u = inv_nslices * (v1)sample_index;
  v3 worldP = bezier_sample(P,u);
  v4 color  = lerp(color1, u, color2);
  if (sample_index != 0)
  {
   fill3_inner(target, A, previous_worldP, worldP, c0,argb_pack(previous_color),argb_pack(color), depth_offset, viz);
  }
  previous_worldP = worldP;
  previous_color  = color;
 }
}

inline v3
camera_world_position(Camera *camera)
{
 v3 result = camera->distance * camera->z.xyz + camera->pan;
 return result;
}

internal v1
curve_visibility(Painter *p, const v3 P[4])
{
 v3 A = P[0];
 v3 B = P[1];
 v3 C = P[2];
 v3 D = P[3];
 v3 normal = noz( cross(B-A, D-A) );  // NOTE: the normal is only defined when the curve is planar; choosing ABD or ACD is arbitrary
 v3 centroid = 0.5f*(A+D);
 v3 camera_local = object_transform.inv * camera_world_position(p->camera);
 v3 view_vector = noz(camera_local - centroid);
 v1 visibility = absolute( dot(normal,view_vector) );
 return visibility;
}

internal void
draw_bezier_painter(Painter *p, const v3 P0[4], Line_Params params)
{
 b32 ok = line_enabled(p);
 if (ok && params.alignment_threshold > 0.f)
 {
  ok = curve_visibility(p, P0) > params.alignment_threshold;
 }
 
 if (ok)
 {
  Render_Target *target = p->target;
  b32 symx = p->symx && !(params.flags & Line_No_SymX);
  
  i32 npoints = 4;
  v3 points[npoints];
  block_copy_array(points,P0);
  
  // NOTE: reflect
  v3 reflects[npoints];
  if (symx) { for_i32 (ipoint,0,npoints) { reflects[ipoint] = negateX(points[ipoint]); } }
  
  // NOTE: "Processing" parameters
  if (params.radii == v4{}) { params.radii = p->line_params.radii; }
  if (params.nslice_per_meter <= 0.f) { params.nslice_per_meter = DEFAULT_NSLICE_PER_METER; }
  params.radii *= p->line_radius_unit;
  
  mat4 view_objectT = p->view_transform * object_transform;
  draw_bezier_inner(target, p->camera, object_transform, view_objectT, points, &params);
  if (symx)
  {
   draw_bezier_inner(target, p->camera, object_transform, view_objectT, reflects, &params);
  }
 }
}

#define macro_control_points(p0,d0, d3,p3)  p0,p0+d0, p3+d3,p3

force_inline Bezier
bez_raw(v3 p0, v3 p1, v3 p2, v3 p3)
{
 return Bezier{ p0,p1,p2,p3 };
}

force_inline Bezier
bez(v3 P[4])
{
 return Bezier{ P[0],P[1],P[2],P[3] };
}

//@Deprecated
force_inline Bezier
bez2(v3 p0, v3 d0, v3 d3, v3 p3)
{
 return bez_raw(macro_control_points(p0,d0, d3,p3) );
}

inline Bezier
bez(v3 p0, v3 d0, v3 d3, v3 p3)
{
 v1 length = lengthof(p3-p0);
 v3 p1 = (2.f*p0 + p3)/3.f + length*d0;
 v3 p2 = (p0 + 2.f*p3)/3.f + length*d3;
 return bez_raw(p0,p1,p2,p3);
}

// NOTE: parabola
internal Bezier
bez(v3 p0, v3 d, v3 p3)
{// NOTE: Next-gen parabola: length-dependent
 v1 length = lengthof(p3-p0);
 v3 q = 0.5f*(p0 + p3) + length*d;
 v3 p1 = (p0 + 2.f*q) / 3.f;
 v3 p2 = (2.f*q  + p3)/ 3.f;
 return bez_raw(p0,p1,p2,p3);
}

// NOTE: Line
force_inline Bezier
bez(v3 a, v3 b)
{
 return bez(a,vec3(),vec3(),b);
}

// NOTE: Planar curve with unit vector guide
internal Bezier
bez(v3 p0, v2 d0, v2 d3, v3 p3, v3 unit_y)
{
 v3 p1;
 v3 unit_z;
 {
  v3 u = p3 - p0;
  unit_z = cross(u, unit_y);
  v3 v = cross(unit_z, u);  // NOTE: u and v has the same magnitude
  p1 = (2.f*p0 + p3)/3.f + (d0.x*u + d0.y*v);
 }
 
 v3 p2;
 {
  v3 u = p3-p1;
  v3 v = cross(unit_z, u);
  p2 = 0.5f*(p1+p3) + (d3.x*u + d3.y*v);
 }
 
 return bez_raw(p0, p1, p2, p3);
}
// NOTE: see above
force_inline Bezier
bez(v3 p0, v4 d, v3 p3, v3 unit_y)
{
 return bez(p0, d.xy, d.zw, p3, unit_y);
}

//NOTE: Planar curve @deprecated
internal Bezier
bez_old(v3 p0, v3 d0, v2 d3, v3 p3)
{
 v3 u = p3 - p0;
 v1 length = lengthof(u);
 v3 p1 = (2.f*p0 + p3)/3.f + length*d0;
 
 v3 z = noz( cross(u,d0) );
 v3 v = cross(z, u);  // NOTE: u and v has the same magnitude
 
 v3 p2 = (p0 + 2.f*p3)/3.f + (d3.x*u + d3.y*v);//NOTE: we don't want this
 return bez_raw(p0, p1, p2, p3);
}

// NOTE: Planar curve (with v3 control point)
internal Bezier
bez(v3 p0, v3 d0, v2 d3, v3 p3)
{
 v3 z, p1;
 {
  v3 u = p3 - p0;
  v1 length = lengthof(u);
  p1 = (2.f*p0 + p3)/3.f + length*d0;
  z = noz( cross(u,d0) );
 }
 v3 p2;
 {
  v3 u = p3-p1;
  v3 v = cross(z, u);  // NOTE: u and v has the same magnitude
  p2 = 0.5f*(p1+p3) + (d3.x*u + d3.y*v);
 }
 return bez_raw(p0, p1, p2, p3);
}

// NOTE: Planar join (to differentiate from C2 join)
// if we just use planar bezier curve, not sure if needed?
internal Bezier 
bez_join(Bez const& ref, v2 d0, v2 d3, v3 p3)
{
 const v3 &p0 = ref.e[3];
 v3 p1,z;
 {
  v3 u = p3-p0;
  z = noz( cross(u, p0-ref.e[2]) );
  v3 v = cross(z,u);
  p1 = (2.f*p0 + p3)/3.f + d0.x*u + d0.y*v;
 }
 v3 p2;
 {
  v3 p13 = p3-p1;
  v3 p13_perp = cross(z, p13);
  p2 = 0.5f*(p1+p3) + d3.x*p13 + d3.y*p13_perp;
 }
 return bez_raw( p0, p1, p2, p3 );
}

force_inline void
draw_bezier_6(Painter *p, 
              v3 p0, v3 d0,  v3 d3, v3 p3,  v3 d7, v3 p7,
              v4 input_radii1, v2 input_radii2,
              Line_Params *input_params=0)
{// TODO: @Perf passing in all these values are NUTS
 // TODO: @Incomplete Things like visibility wouldn't work
 Bezier line1 = bez2(p0,d0, d3,p3);
 Bezier line2 = bez2(p3,-d3, d7,p7);
 
 auto &r1 = input_radii1;
 auto &r2 = input_radii2;
 v4 radii1 = vec4(r1[0], r1[0]+r1[1], r1[3]+r1[2], r1[3]);
 v4 radii2 = vec4(r1[3], r1[3]-r1[2], r2[1]+r2[0], r2[1]);
 
 Line_Params params = input_params ? *input_params : p->line_params;
 
 params.radii = radii1;
 draw_bezier_painter(p, line1, params);
 params.radii = radii2;
 draw_bezier_painter(p, line2, params);
}

force_inline void
drawf(Painter *p, Bezier b, Line_Params const&params)
{
 draw_bezier_painter(p,b,params);
}

//NOTE: radii in fractions
force_inline void
drawf(Painter *p, Bezier b, v4 radii)
{
 Line_Params params = p->line_params;
 params.radii = radii;
 draw_bezier_painter(p,b,params);
}
//NOTE radii in sixths
force_inline void
drawf(Painter *p, Bezier b, v4i radii)
{
 Line_Params params = p->line_params;
 params.radii = i2f6(radii);
 draw_bezier_painter(p,b,params);
}
// NOTE: omit params
force_inline void
drawf(Painter *p, Bezier b)
{
 draw_bezier_painter(p,b,p->line_params);
}

force_inline void
draw_linef(Painter *p, v3 a, v3 b, Line_Params in_params)
{
 Line_Params params = in_params;
 params.flags |= Line_Straight;
 drawf( p, bez(a,b), params);
}
//
force_inline void
draw_linef(Painter *p, v3 a, v3 b)
{
 draw_linef( p, a,b, p->line_params);
}

force_inline ARGB_Color
argb_gray(v1 value)
{
 macro_clamp_min(value,0.0f);
 macro_clamp_max(value,1.0f);
 return argb_pack(v4{repeat3(value),1});
}

force_inline v4
v4_gray(v1 value)
{
 return v4{repeat3(value),1};
}

internal argb
compute_fill_color(Painter *p, v1 color_lerp)
{
 // @Slow Maybe we should just store the color in v4 form?
 argb color;
 if (color_lerp != 0.0f)
 {
  v4 color_v4 = argb_unpack(p->fill_color);
  // NOTE(kv): 0 is the lerp target
  color_v4.xyz *= (1.f-color_lerp);
  color = argb_pack(color_v4);
 }
 else { color = p->fill_color; }
 
 return color;
}

internal void
fill3f(Painter *p, 
               v3 a, v3 b, v3 c, 
               argb ac, argb bc, argb cc,
               Poly_Params params={})
{
 if (poly_enabled(p))
 {
  b32 symx = p->symx;
  i32 npoints=3;
  v3 points[3] = {a,b,c};
  
  v3 reflects[npoints];
  if (symx) 
  { 
   for_i32 (ipoint,0,npoints)
   {
    reflects[ipoint] = negateX(points[ipoint]);
   }
  }
  
  if (ac == 0) { ac = p->fill_color; }
  if (bc == 0) { bc = p->fill_color; }
  if (cc == 0) { cc = p->fill_color; }
  
  fill3_inner( p->target, expand3(points), ac,bc,cc, p->fill_depth_offset, p->poly_viz);
  if (symx) {
   fill3_inner( p->target, expand3(reflects), ac,bc,cc, p->fill_depth_offset, p->poly_viz);
  }
 }
}
//
internal void
fill3f(Painter *p, v3 a, v3 b, v3 c)
{
 fill3f(p,a,b,c,0,0,0);
}
//
internal void
fill3f(Painter *p, v3 a, v3 b, v3 c, argb color)
{
 fill3f(p,a,b,c,repeat3(color));
}
//
internal void
fill3f(Painter *p, v3 P[3])
{
 fill3f(p,P[0],P[1],P[2]);
}

force_inline void
fill4f(Painter *p, 
       v3 p0, v3 p1, v3 p2, v3 p3,
       argb c0, argb c1, argb c2, argb c3)
{
 fill3f( p, p0,p1,p2, c0,c1,c2);
 fill3f( p, p0,p2,p3, c0,c2,c3);
}
//@Cleanup
force_inline void
fill4f(Painter *p, v3 p0, v3 p1, v3 p2, v3 p3, argb c)
{
 fill4f( p, p0,p1,p2,p3, repeat4(c));
}
force_inline void
fill4f(Painter *p, v3 p0, v3 p1, v3 p2, v3 p3)
{
 fill4f( p, p0,p1,p2,p3,0,0,0,0);
}

internal void
fillf(Painter *p, v3 A, Bezier &bezier, 
     argb c0=0, argb c1=0, argb c2=0,
     u32 flags=0)
{
 if ( poly_enabled(p) )
 {
  if (c0 == 0) { c0 = p->fill_color; }
  if (c1 == 0) { c1 = p->fill_color; }
  if (c2 == 0) { c2 = p->fill_color; }
  
  v3 *P = bezier;
  b32 symx = p->symx;
  
  const i32 npoints = 5;
  v3 points[npoints];
  points[0] = A;
  block_copy(points+1, P, 4*sizeof(*points));
  
  // NOTE: Reflect
  
  // TODO: Something fishy with this flag vs painter logic
  if (p->poly_viz) { flags |= Poly_Viz; }
  
  bezier_poly3_inner(p->target, points[0], points+1, c0,c1,c2, p->fill_depth_offset, flags);
  if (symx) 
  {
   for_i32 (ipoint,0,npoints) { points[ipoint] = negateX(points[ipoint]); }
   bezier_poly3_inner(p->target, points[0], points+1, c0,c1,c2, p->fill_depth_offset, flags);
  }
 }
}

internal void
fill_dbez_inner(Painter *p, const v3 P[4], const v3 Q[4], argb color)
{
 if ( poly_enabled(p) )
 {
  if (color == 0) { color = p->fill_color; }
  b32 symx = p->symx;
  
  const i32 npoints = 8;
  v3 points[npoints];
  for_i32(ipoint,0,4) { points[ipoint+0] = P[ipoint]; }
  for_i32(ipoint,0,4) { points[ipoint+4] = Q[ipoint]; }
  
  // NOTE: reflect
  v3 reflects[npoints];
  if (symx) { for_i32 (ipoint,0,npoints) { reflects[ipoint] = negateX(points[ipoint]); } }
  
  fill_bezier_inner_2(p->target, points, points+4, color, p->fill_depth_offset, p->poly_viz);
  if (symx) { 
   fill_bezier_inner_2(p->target, reflects, reflects+4, color, p->fill_depth_offset, p->poly_viz);
  }
 }
}

force_inline void
fill_dbezf(Painter *p, Bezier const&b1, Bezier const&b2, argb color=0)
{
 fill_dbez_inner(p, b1.e, b2.e, color);
}

internal void
fill_dbezf(Painter *p, v3 a, v3 b, Bezier const&bezier, argb color=0)
{
 v3 ab[4] = {
  a,
  a+(b-a)/3.f,
  a+(b-a)/3.f,
  b,
 };
 fill_dbez_inner(p, ab, bezier.e, color);
}

force_inline void
fill_bezf(Painter *p, Bezier const&bezier, argb color=0)
{
 fill_dbezf(p, bezier.e[0], bezier.e[3], bezier, color);
}

internal void
fill_patch_inner(Render_Target *target,
                 const v3 P[4][4],
                 argb color, v1 depth_offset,
                 b32 viz)
{
 i32 nslice = PATCH_NSLICE;
 v1 inv_nslice = 1.0f / (v1)nslice;
 v3 prev_v [nslice+1];
 v3 this_v [nslice+1];
 
 for_i32  (v_index, 0, nslice+1) 
 {
  for_i32 (u_index, 0, nslice+1)
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
    poly4_inner(target,
                prev_v[u_index],   this_v[u_index],
                this_v[u_index+1], prev_v[u_index+1],
                color,color,color,color,
                depth_offset, viz);
   }
  }
  
  block_copy_array(prev_v, this_v);
 }
}

internal void
fill_patch(Painter *p, const v3 P[4][4], argb color=0)
{
 if (color == 0) { color = p->fill_color; }
 fill_patch_inner(p->target, P, color, p->fill_depth_offset, p->poly_viz);
}

internal void
fill_patch(Painter *p,
           v3 P0[4], v3 P1[4],
           v3 P2[4], v3 P3[4],
           argb color=0)
{
 v3 P[4][4];
 block_copy_array(P[0], P0);
 block_copy_array(P[1], P1);
 block_copy_array(P[2], P2);
 block_copy_array(P[3], P3);
 fill_patch(p, P, color);
}

force_inline void
fill3_symx(Painter *p, v3 a, v3 b)
{
 symx_off;
 fill3f(p, a, b, negateX(b));
}

force_inline void
fill4_symxf(Painter *p, v3 a, v3 b)
{
 symx_off;
 fill4f(p, a, b, negateX(b), negateX(a));
}

internal void 
fill_stripf(Painter *p, v3 verts[], i32 vert_count)
{
 for_i32(i,0,vert_count-2)
 {
  fill3f( p, verts[i], verts[i+1], verts[i+2]);
 }
}

internal void 
fill_fanf(Painter *p, v3 A, v3 verts[], i32 vert_count, argb color=0)
{
 for_i32(index, 0, vert_count-1)
 {
  fill3f( p, A, verts[index], verts[index+1], repeat3(color));
 }
}

internal void
draw_boxf(Painter *p, mat4 const&transform)
{
 v3 x = mat4vec(transform, vec3x(2));
 v3 y = mat4vec(transform, vec3y(2));
 v3 z = mat4vec(transform, vec3z(2));
 v3 O = transform*vec3() - 0.5f*(x+y+z);
 v3 X = O+x;
 drawf(p,bez(X,X+y));
 drawf(p,bez(X,X+z));
 v3 Y = O+y;
 drawf(p,bez(Y,Y+x));
 drawf(p,bez(Y,Y+z));
 v3 Z = O+z;
 drawf(p,bez(Z,Z+x));
 drawf(p,bez(Z,Z+y));
 
 drawf(p,bez(O,X)); drawf(p,bez(O,Y)); drawf(p,bez(O,Z));
 
 v3 P = O+x+y+z;
 drawf(p,bez(P,P-x)); drawf(p,bez(P,P-y)); drawf(p,bez(P,P-z));
}

force_inline Patch
patch(Bezier const&p0, Bezier const&p1, Bezier const&p2, Bezier const&p3)
{
 Patch result;
 block_copy_array(result.e[0], p0.e);
 block_copy_array(result.e[1], p1.e);
 block_copy_array(result.e[2], p2.e);
 block_copy_array(result.e[3], p3.e);
 return result;
}

force_inline Patch
patch_symx(Bezier const&P0, Bezier const&P1)
{
 Bezier N0 = negateX(P0);
 Bezier N1 = negateX(P1);
 return patch(P0, P1, N1, N0);
}

internal void
draw_meshf(Mesh const&mesh)
{
 for_i32(triangle_index,0,mesh.triangle_count)
 {
  v3i triangle = mesh.triangles[triangle_index];
  fill3f(&global_painter, 
         mesh.vertices[triangle[0]],
         mesh.vertices[triangle[1]],
         mesh.vertices[triangle[2]]);
 }
}

//~NOTE: Absolutely abhorrent macros so we can instrument stuff
#define set_draw_linum  g_draw_linum = __LINE__

#define draw(...)      set_draw_linum; drawf(__VA_ARGS__);
#define draw_line(...) set_draw_linum; draw_linef(__VA_ARGS__);
#define draw_disk(...) set_draw_linum; draw_diskf(__VA_ARGS__);
#define draw_mesh(...) set_draw_linum; draw_meshf(__VA_ARGS__);
#define fill(...)  set_draw_linum; fillf(__VA_ARGS__);
#define fill_bez(...)  set_draw_linum; fill_bezf(__VA_ARGS__);
#define fill_dbez(...)  set_draw_linum; fill_dbezf(__VA_ARGS__);
#define fill3(...) set_draw_linum; fill3f(__VA_ARGS__);
#define fill4(...) set_draw_linum; fill4f(__VA_ARGS__);
#define fill_fan(...)   set_draw_linum; fill_fanf(__VA_ARGS__);
#define fill_strip(...) set_draw_linum; fill_stripf(__VA_ARGS__);

//~EOF
