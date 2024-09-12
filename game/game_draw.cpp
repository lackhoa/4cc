//~NOTE: Absolutely abhorrent macros so we can get some reasonable UX
//~

global u32 bs_cycle_counter;



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
  //NOTE: All objects depth are at the origin's depth
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

internal void
fill_bezier_inner_2(v3 P[4], v3 Q[4], argb color, v1 depth_offset, b32 viz)
{
 i32 nslices = bezier_poly_nslice;
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
   poly4_inner(previous_worldP, previous_worldQ, worldQ, worldP, 
               repeat4(color), depth_offset, viz);
  }
  previous_worldP = worldP;
  previous_worldQ = worldQ;
 }
}

internal void
bezier_poly3_inner(v3 A, v3 P[4], 
                   argb c0, argb c1, argb c2, 
                   v1 depth_offset, b32 viz)
{
 i32 nslices = bezier_poly_nslice;
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
   fill3_inner(A, previous_worldP, worldP, c0,argb_pack(previous_color),argb_pack(color), depth_offset, viz);
  }
  previous_worldP = worldP;
  previous_color  = color;
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

// @deprecated Vanilla bezier curves Controlled by point (NOT length invariant)
force_inline Bezier
bez_old(v3 p0, v3 d0, v3 d3, v3 p3)
{
 return bez_raw(macro_control_points(p0,d0, d3,p3) );
}

inline Bezier
bez(v3 p0, v3 d0, v3 d3, v3 p3)
{
 TIMED_BLOCK(bs_cycle_counter);
 v1 length = lengthof(p3-p0);
 v3 p1 = (2.f*p0 + p3)/3.f + length*d0;
 v3 p2 = (p0 + 2.f*p3)/3.f + length*d3;
 return bez_raw(p0,p1,p2,p3);
}

// NOTE: Parabola @deprecated since it's length adjusted
internal Bezier
bez_parabola_len(v3 p0, v3 d, v3 p3)
{// NOTE: Next-gen parabola: length-dependent
 TIMED_BLOCK(bs_cycle_counter);
 v1 length = lengthof(p3-p0);
 v3 q = 0.5f*(p0 + p3) + length*d;
 v3 p1 = (p0 + 2.f*q) / 3.f;
 v3 p2 = (2.f*q  + p3)/ 3.f;
 return bez_raw(p0,p1,p2,p3);
}

// NOTE: Parabola (no length dependence)
internal Bezier
bez(v3 p0, v3 d, v3 p3)
{
 TIMED_BLOCK(bs_cycle_counter);
 v3 q = 0.5f*(p0 + p3) + d;
 v3 p1 = (p0 + 2.f*q) / 3.f;
 v3 p2 = (2.f*q + p3) / 3.f;
 return bez_raw(p0,p1,p2,p3);
}

// NOTE: Planar curve with unit vector guide
internal Bezier
bez(v3 p0, v2 d0, v2 d3, v3 p3, v3 unit_y)
{
 TIMED_BLOCK(bs_cycle_counter);
 v3 p1, unit_z;
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
//
force_inline Bezier
bez(v3 p0, v4 d, v3 p3, v3 unit_y)
{
 return bez(p0, d.xy, d.zw, p3, unit_y);
}
//
force_inline Bezier
bez(v3 p0, Planar_Bezier data, v3 p3)
{
 return bez(p0, data.d0, data.d3, p3, data.unit_y);
}

// NOTE: Planar curve (with v3 control point)
// NOTE: Control point is length adjusted, which is wasteful.
//       the shape of the curve is independent on the length, but so what?
force_inline Bezier
bezd_len(v3 p0, v3 d0, v2 d3, v3 p3)
{
 d0 *= lengthof(p3-p0);
 return bez(p0, d0, d3, p3);
}

//NOTE: Planar curve (with v3 control point, BUT it doesn't automatically adjust d3)
// @deprecated
internal Bezier
bezd_old(v3 p0, v3 d0, v2 d3, v3 p3)
{
 TIMED_BLOCK(bs_cycle_counter);
 
 v3 u = p3 - p0;
 v1 length = lengthof(u);
 v3 p1 = (2.f*p0 + p3)/3.f + length*d0;
 
 v3 w = noz( cross(u,d0) );
 v3 v = cross(w, u);  // NOTE: u and v has the same magnitude
 
 v3 p2 = (p0 + 2.f*p3)/3.f + (d3.x*u + d3.y*v);
 return bez_raw(p0, p1, p2, p3);
}

// NOTE: No length adjustment, non-planar
internal Bezier 
bez_c2(Bez const&ref, v3 d3, v3 p3)
{
 TIMED_BLOCK(bs_cycle_counter);
 v3 p0 = ref.e[3];
 v3 p1 = p0 + (ref.e[3] - ref.e[2]);
 v3 p2 = 0.5f*(p3+p1) + d3;
 return bez_raw(p0,p1,p2,p3);
}

internal v4
radii_c2(v4 ref, v2 d_p3)
{
 TIMED_BLOCK(bs_cycle_counter);
 v1 d  = d_p3[0];
 v1 p3 = d_p3[1];
 v1 p0 = ref[3];
 v1 p1 = p0 + (ref[3] - ref[2]);
 v1 len = absolute(p3 - p0);
 v1 p2 = 0.5f*(p3+p1) + len*d;
 return V4(p0,p1,p2,p3);
}


force_inline void
draw_line(v3 a, v3 b, Line_Params in_params, linum_param)
{
 Line_Params params = in_params;
 params.flags |= Line_Straight;
 draw(bez(a,b), params, linum);
}
//
force_inline void
draw_line(v3 a, v3 b, linum_param)
{
 draw_line(a,b, painter.line_params, linum);
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
compute_fill_color(v1 color_lerp)
{
 Painter *p = &painter;
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
fill3(v3 a, v3 b, v3 c, 
      argb ac, argb bc, argb cc,
      linum_param)
{
 set_linum;
 Painter *p = &painter;
 if( is_poly_enabled() )
 {
  b32 symx = p->symx;
  const i32 npoints=3;
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
  
  Poly_Flags flags = (p->viz_level) ? 1 : 0;
  fill3_inner(expand3(points), ac,bc,cc, p->fill_depth_offset, flags);
  if (symx) {
   fill3_inner(expand3(reflects), ac,bc,cc, p->fill_depth_offset, flags);
  }
 }
}
//
internal void
fill3(v3 a, v3 b, v3 c, linum_param)
{
 fill3(a,b,c,0,0,0, linum);
}
//
internal void
fill3(v3 a, v3 b, v3 c, argb color, linum_param)
{
 fill3(a,b,c,repeat3(color), linum);
}
//
internal void
fill3(v3 P[3], linum_param)
{
 fill3(P[0],P[1],P[2], linum);
}

force_inline void
fill4(v3 p0, v3 p1, v3 p2, v3 p3,
      argb c0, argb c1, argb c2, argb c3, linum_param)
{
 set_linum;
 fill3( p0,p1,p2, c0,c1,c2, 0);
 fill3( p0,p2,p3, c0,c2,c3, 0);
}
//@Cleanup
force_inline void
fill4(v3 p0, v3 p1, v3 p2, v3 p3, argb c, linum_param)
{
 fill4(p0,p1,p2,p3, repeat4(c), linum);
}
force_inline void
fill4(v3 p0, v3 p1, v3 p2, v3 p3, linum_param)
{
 fill4(p0,p1,p2,p3,0,0,0,0, linum);
}

internal void
fill(v3 A, Bezier &bezier, 
     argb c0=0, argb c1=0, argb c2=0,
     u32 flags=0, linum_param)
{
 set_linum;
 auto p = &painter;
 if ( is_poly_enabled() )
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
  if (p->viz_level) { flags |= Poly_Viz; }
  
  bezier_poly3_inner(points[0], points+1, c0,c1,c2, p->fill_depth_offset, flags);
  if (symx) 
  {
   for_i32 (ipoint,0,npoints) { points[ipoint] = negateX(points[ipoint]); }
   bezier_poly3_inner(points[0], points+1, c0,c1,c2, p->fill_depth_offset, flags);
  }
 }
}

internal void
fill_dbez_inner(const v3 P[4], const v3 Q[4], argb color)
{
 Painter *p = &painter;
 if ( is_poly_enabled() )
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
  
  fill_bezier_inner_2(points, points+4, color, p->fill_depth_offset, p->viz_level);
  if (symx) { 
   fill_bezier_inner_2(reflects, reflects+4, color, p->fill_depth_offset, p->viz_level);
  }
 }
}

force_inline void
fill_dbez(Bezier const&b1, Bezier const&b2, argb color=0, linum_param)
{
 set_linum;
 fill_dbez_inner(b1.e, b2.e, color);
}

internal void
fill_dbez(v3 a, v3 b, Bezier const&bezier, argb color=0, linum_param)
{
 set_linum;
 v3 ab[4] = {
  a,
  a+(b-a)/3.f,
  a+(b-a)/3.f,
  b,
 };
 fill_dbez_inner(ab, bezier.e, color);
}

force_inline void
fill_bez(Bezier const&bezier, argb color=0, linum_param)
{
 fill_dbez(bezier.e[0], bezier.e[3], bezier, color, linum);
}



internal void
fill_patch(const v3 P[4][4], argb color=0)
{
 auto p = &painter;
 if (color == 0) { color = p->fill_color; }
 fill_patch_inner(P, color, p->fill_depth_offset, p->viz_level);
}

internal void
fill_patch(v3 P0[4], v3 P1[4],
           v3 P2[4], v3 P3[4],
           argb color=0)
{
 v3 P[4][4];
 block_copy_array(P[0], P0);
 block_copy_array(P[1], P1);
 block_copy_array(P[2], P2);
 block_copy_array(P[3], P3);
 fill_patch(P, color);
}

force_inline void
fill3_symx(v3 a, v3 b, linum_param)
{
 symx_off;
 fill3(a, b, negateX(b), linum);
}

force_inline void
fill4_symx(v3 a, v3 b, linum_param)
{
 symx_off;
 fill4(a, b, negateX(b), negateX(a), linum);
}

internal void 
fill_strip(v3 verts[], i32 vert_count, linum_param)
{
 for_i32(i,0,vert_count-2)
 {
  fill3(verts[i], verts[i+1], verts[i+2], linum);
 }
}

internal void 
fill_fan(v3 A, v3 verts[], i32 vert_count, argb color=0, linum_param)
{
 for_i32(index, 0, vert_count-1)
 {
  fill3(A, verts[index], verts[index+1], repeat3(color), linum);
 }
}

internal void
draw_box(mat4 const&transform, linum_param)
{
 set_linum;
 v3 x = mat4vec(transform, V3x(2));
 v3 y = mat4vec(transform, V3y(2));
 v3 z = mat4vec(transform, V3z(2));
 v3 O = transform*V3() - 0.5f*(x+y+z);
 v3 X = O+x;
 draw(bez(X,X+y), 0);
 draw(bez(X,X+z), 0);
 v3 Y = O+y;
 draw(bez(Y,Y+x), 0);
 draw(bez(Y,Y+z), 0);
 v3 Z = O+z;
 draw(bez(Z,Z+x), 0);
 draw(bez(Z,Z+y), 0);
 
 draw(bez(O,X), 0); draw(bez(O,Y), 0); draw(bez(O,Z), 0);
 
 v3 P = O+x+y+z;
 draw(bez(P,P-x), 0); draw(bez(P,P-y), 0); draw(bez(P,P-z), 0);
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

force_inline v4
big_to_small()
{
 v1 big   = 1.f;
 v1 small = 0.5f;
 return V4(big, big, small, small);
}

force_inline v4
small_to_big()
{
 v1 big   = 1.f;
 v1 small = 0.5f;
 return V4(small, small, big, big);
}

force_inline void
duo_line(v3 a, v3 b, v3 c, linum_param)
{
 set_linum;
 draw(bez(a,b), small_to_big(), 0);
 draw(bez(b,c), big_to_small(), 0);
}
//
force_inline void
duo_line(v3 array[3], linum_param)
{
 set_linum;
 duo_line(array[0], array[1], array[2], 0);
}

function void
draw_image(char *filename, v3 o, v3 x, v3 y, v1 alpha=1.f, v3 color={1,1,1},
           linum_param)
{
 if (get_hot_prim_id() == u32(linum))
 {// NOTE: tint it
  color = V3(1.f, 1.f, 0.f)*color;
 }
 argb argb_color = argb_pack( V4(color,alpha) );
 push_image(painter.target, filename,o,x,y,argb_color, linum);
}

//~EOF
