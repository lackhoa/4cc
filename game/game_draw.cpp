//-NOTE(kv) Highest-level drawing functions
//  We wanna share most of these functions with the driver.
#pragma once

framework_storage u32 bs_cycle_counter;

struct Patch{
 v3 e[4][4];
 typedef v3 Array4x4[4][4];  // @stroustrup
 operator Array4x4&() { return e; } 
};

#define set_linum       if (linum!=0) { painter.draw_prim_id = linum; }
#define macro_control_points(p0,d0, d3,p3)  p0,p0+d0, p3+d3,p3
#define symx_off set_in_block(painter.symx, false)
#define symx_on  set_in_block(painter.symx, true)
#define hl_block_color(...)  push_hl(__VA_ARGS__); defer(pop_hl(););
#define hl_block  hl_block_color(0)

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
inline Bezier
bez_negateX(Bezier line){ return negateX(line); }
function Bez
operator*(mat4 transform, Bez bezier){
 Bez result;
 for_i32(index,0,4) { result[index] = transform*bezier[index]; }
 return result;
}
framework_storage u32 hot_prim_id;
inline u32 get_hot_prim_id(){ return hot_prim_id; }

inline b32 is_poly_enabled(){
 return (painter.painting_disabled == false);
}
inline b32 is_line_enabled(){
 return painter.painting_disabled == false;
}

function Bezier
get_column(Patch const&surface, i32 col){
 Bezier result;
 for_i32(index,0,4) {
  result[index] = surface.e[index][col];
 }
 return result;
}
xfunction void
poly3_inner(v3 points[3], argb color,
            v1 depth_offset, Poly_Flags flags)
{// NOTE: Triangle
 TIMED_BLOCK(draw_cycle_counter);
 auto &p = painter;
 b32 shaded     = (flags & Poly_Shaded);
 b32 is_line    = (flags & Poly_Line);
 b32 is_overlay = (flags & Poly_Overlay);
 Render_Vertex vertices[3] = {};
 for_i32(i,0,3){ vertices[i].pos = points[i]; }
 // TODO(kv): @Speed The caller should be in charge of passing the color in!
 u32 prim_id = p.draw_prim_id;
 b32 is_hot      = prim_id == get_hot_prim_id();
 b32 is_selected = prim_id == selected_prim_id(p.modeler);
 b32 is_active   = is_prim_id_active(p.modeler, prim_id);
 // TODO(kv): @cleanup wtf is this code?
 if(is_hot || is_selected || is_active){
  argb hl_color = hot_color;
  if(is_hot){
   hl_color = (color == hot_color) ? hot_color2 : hot_color;
  }else if(is_selected){
   hl_color = argb_red;
  }else if(is_active){
   //TODO: pick color
   hl_color = hot_color;
  }
  color = hl_color;
 }else if(shaded && !is_line){
  //NOTE(kv) Shading logic doesn't apply to line, because we can see them just fine.
  // ;poly_shading
  v3 normal = noz(cross(points[1]-points[2], points[0]-points[2]));
  {
   v4 colorv = argb_unpack(color);
   v1 darkness_fuzz = 0.3f;
   colorv.rgb *= lerp(darkness_fuzz,absolute(normal.z),1.f);
   color = argb_pack(colorv);
  }
 }
 
 for_i32(index,0,3){
  vertices[index].color = color;
 }
 
 for_u32(index,0,alen(vertices)){
  Render_Vertex *vertex = vertices+index;
  vertex->uvw          = V3();
  vertex->depth_offset = depth_offset;
  vertex->prim_id      = p.draw_prim_id;
 }
 
 Vertex_Type type = Vertex_Poly;
 if(is_overlay){type = Vertex_Overlay;}
 draw__push_vertices(p.target, vertices, alen(vertices), type);
}
function void
poly3_inner(v3 p0, v3 p1, v3 p2,
            argb color,
            v1 depth_offset, Poly_Flags flags) {
 v3 points[3] = {p0,p1,p2};
 poly3_inner(points, color, depth_offset, flags);
}
function void
fill3_inner2(v3 points[3],
             Fill_Params const&params,
             linum_defparam){
 if(is_poly_enabled()){
  set_linum;
  const i32 npoints=3;
  Poly_Flags flags = params.flags;
  v1 depth_offset = painter.fill_depth_offset;
  poly3_inner(points, params.color, depth_offset, flags);
 }
}
inline void
fill3(v3 a, v3 b, v3 c, Fill_Params params=fp(), linum_defparam){
 v3 points[3] = {a,b,c};
 fill3_inner2(points,params,linum);
}
inline void
dfill3_func(String a, String b, String c, Fill_Params params=fp(), linum_defparam){
 String points[3] = {a,b,c};
 dfill3_func(points,params,linum);
}
#define dfill3(a,b,c,...) \
dfill3_func(strlit(#a), strlit(#b), strlit(#c), __VA_ARGS__)
#define dfill_bez(b,...)  dfill_bez_func(strlit(#b),__VA_ARGS__)
inline void
fill3(v3 points[3], Fill_Params params=fp(), linum_defparam){
 fill3_inner2(points,params,linum);
}
#define dfill_dbez(b1,b2,...) dfill_dbez_func(strlit(#b1), strlit(#b2), __VA_ARGS__)
//-
inline Bezier bez_raw(v3 p0, v3 p1, v3 p2, v3 p3){
 return Bezier{ p0,p1,p2,p3 };
}
inline Bezier bez_raw(v3 P[4]){
 return Bezier{ P[0],P[1],P[2],P[3] };
}
inline Bezier
bez_offset(v3 p0, v3 d0, v3 d3, v3 p3){
 return bez_raw(macro_control_points(p0,d0,d3,p3) );
}

//TODO(kv) This is length dependent so fix it?
//  Why don't we want length dependence?
//  Because look, if the length changes, so would the curve.
//  There just isn't much point to specifying curves that look the same
//  when the distance between the endpoints change.
function Bezier
bez_v3v3(v3 p0, v3 d0, v3 d3, v3 p3){
 TIMED_BLOCK(bs_cycle_counter);
 v1 length = lengthof(p3-p0);
 v3 p1 = (2.f*p0 + p3)/3.f + length*d0;
 v3 p2 = (p0 + 2.f*p3)/3.f + length*d3;
 return bez_raw(p0,p1,p2,p3);
}
// NOTE: Parabola (no length dependence)
function Bezier
bez_parabola(v3 p0, v3 d, v3 p3){
 TIMED_BLOCK(bs_cycle_counter);
 v3 q = 0.5f*(p0 + p3) + d;
 v3 p1 = (p0 + 2.f*q) / 3.f;
 v3 p2 = (2.f*q + p3) / 3.f;
 return bez_raw(p0,p1,p2,p3);
}

// NOTE: Planar curve (with v3 control point)
function Bezier
bez_v3v2(v3 p0, v3 d0, v2 d3, v3 p3) {
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

// NOTE: Planar curve with unit vector guide
function Bezier
bez_unit(v3 p0, v2 d0, v2 d3, v3 unit_y, v3 p3){
 TIMED_BLOCK(bs_cycle_counter);
 v3 p1, unit_z;
 {
  v3 u = p3-p0;
  unit_z = cross(u, unit_y); // TODO(kv) Need to normalize! This is a bug that can't be fixed right now because our data depends on it
  v3 v = cross(unit_z, u);  // NOTE(kv) u and v has the same magnitude, because "unit_z" and "u" are orthogonal
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
force_inline Bezier
bez_unit2(v3 p0, v4 d0d3, v3 unit_y, v3 p3){
 return bez_unit(p0, d0d3.xy, d0d3.zw, unit_y, p3);
}
// NOTE: Planar curve (with v3 control point)
// NOTE: Control point is length adjusted, which is wasteful.
//       the shape of the curve is independent on the length, but so what?
inline Bezier
bezd_len(v3 p0, v3 d0, v2 d3, v3 p3) {
 d0 *= lengthof(p3-p0);
 return bez_v3v2(p0, d0, d3, p3);
}
//NOTE: Planar curve (with v3 control point, BUT it doesn't automatically adjust d3)
// @deprecated
function Bezier
bez_bezd_old(v3 p0, v3 d0, v2 d3, v3 p3){
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
function Bezier 
bez_c2(Bez const&ref, v3 d3, v3 p3){
 TIMED_BLOCK(bs_cycle_counter);
 v3 p0 = ref.e[3];
 v3 p1 = p0 + (ref.e[3] - ref.e[2]);
 v3 p2 = 0.5f*(p3+p1) + d3;
 return bez_raw(p0,p1,p2,p3);
}
function v4
radii_c2(v4 ref, v2 d_p3){
 TIMED_BLOCK(bs_cycle_counter);
 v1 d  = d_p3[0];
 v1 p3 = d_p3[1];
 v1 p0 = ref[3];
 v1 p1 = p0 + (ref[3] - ref[2]);
 v1 len = absolute(p3 - p0);
 v1 p2 = 0.5f*(p3+p1) + len*d;
 return V4(p0,p1,p2,p3);
}
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
function void
draw_disk_inner(v3 center, v1 radius, argb color,
                v1 depth_offset, Poly_Flags flags){
 // TODO: @Speed We can reduce level of detail if it's too far away
 if (radius > 0.f) {
  const i32 CIRCLE_NSLICE = 8;
  i32 nslices = CIRCLE_NSLICE;  //NOTE: @Tested Minimum should be 8
  v1 interval = 1.f / v1(nslices);
  v3 last_sample;
  for_i32(index, 0, nslices+1) {
   v1 angle = interval * v1(index);
   v2 arm = radius*arm2(angle);
   mat4 &bone_from_camT = painter.cam_from_boneT.inverse;
   v3 sample = center + mat4vec(bone_from_camT, V3(arm, 0.f));//@slow
   if (index!=0) {
    poly3_inner(center, last_sample, sample,
                color, depth_offset, flags);
   }
   last_sample = sample;
  }
 }
}
function void
draw_bezier_inner(const v3 P[4], Common_Line_Params &cparams, Line_Params &params){
 if(params.visibility > 0.f){
  v1 depth_offset = cparams.depth_offset;
  v4 radii = params.radii;
  if(radii == v4{}){ radii = cparams.radii; }
  radii *= cparams.radius_mult * default_line_radius_unit;
  //NOTE(kv) we view zero as default, so whoever turns gets to stick it
  Line_Flags flags = params.flags | cparams.flags;
  i32 nslices;
  if(flags & Line_Straight){
   radii = V4(radii[1]);  //note: don't wanna take radius from the tip
   nslices = 1;
  }else{
   //~NOTE: pre-pass
   // NOTE: Working in 2D is no good, because we don't take into account that samples are moving in 3D,
   // so they might be traveling longer distances due to the depth, 
   // and spaced out more when we look at them in 3D -> you'd underestimate the density in 2D
   Bezier P_transformed;
   {
    const mat4 &transform = current_world_from_bone(painter);
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
   
   nslices = i32(cparams.nslice_per_meter * the_length)+1;
   
   i32 MAX_NSLICES = 128;
   if(nslices > MAX_NSLICES){ DEBUG_VALUE(nslices); }
   macro_clamp_max(nslices, MAX_NSLICES);
  }
  
  Poly_Flags poly_flags = Poly_Line;
  if (flags & Line_Overlay) { poly_flags |= Poly_Overlay; }
  
  v1 radius_threshold = 0.1065f*millimeter;
  macro_clamp_min(radius_threshold, 0.f);
  
  v1 interval = 1.0f / (v1)nslices;
  
  Vertex_Type vertex_type = Vertex_Poly; //@temp
  
  argb color = cparams.color;
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
     v3 d_camera_space = mat4vec(painter.cam_from_boneT, (sample - last_sample));
     v2 perp_unit = noz( perp(d_camera_space.xy) );
     perp1 = mat4vec(painter.cam_from_boneT.inverse, V3(radius*perp_unit, 0.f));
    }
    C = sample+perp1;
    D = sample-perp1;
   }
   
   if(sample_index!=0){
    // NOTE: Clean up ugly tiny lines (and zeros), possibly unnecessary?
    if(0.5f*(radius+last_radius) >= radius_threshold){
     poly3_inner(A,C,D, color, depth_offset, poly_flags);
     poly3_inner(A,B,D, color, depth_offset, poly_flags);
    }
   }
   
   A = C;
   B = D;
   last_sample = sample;
   last_radius = radius;
  }
  
  for_i32(index,0,2){
   //NOTE: Draw the circular endpoints
   v1 t = 0.f;
   if(index==1){t=1.f;}
   v3 center = bezier_sample(P,t);
   v1 radius = bezier_sample(radii,t);
   if(radius > radius_threshold){
    draw_disk_inner(center, radius, color, depth_offset, poly_flags);
   }
  }
 }
}
function b32
draw_cparams(const v3 P[4], Common_Line_Params &cparams, Line_Params params, linum_defparam){
 set_linum;
 Painter &p = painter;
 b32 ok = is_line_enabled();
 if(ok and
    not p.ignore_alignment_min and
    u32(linum) != get_hot_prim_id()  and
    params.alignment_min > 0.f)
 {//-NOTE(kv) Alignment business
  v3 A = P[0];
  v3 B = P[1];
  v3 C = P[2];
  v3 D = P[3];
  v3 normal = params.unit_normal;
  if(normal == V3()){
   //NOTE(kv) The normal is only defined when the curve is planar; choosing ABD or ACD is arbitrary
   normal = noz(cross(B-A, D-A));
  }
  v1 alignment;
  {
   v3 centroid = 0.5f*(A+D);  // NOTE: our curves are kinda straight most of the time, so I guess this works
   mat4 &bone_from_world = current_world_from_bone(p).inv;
   v3 camera_obj = bone_from_world * camera_world_position(&p.camera);
   v3 view_vector = noz(camera_obj - centroid);
   alignment = absolute(dot(normal,view_vector));
  }
  ok = alignment > params.alignment_min;
 }
 if(ok){
  const i32 npoints = 4;
  // NOTE: Processing parameters
  draw_bezier_inner(P, cparams, params);
 }
 return ok;
}
function b32
draw(const v3 P0[4], Line_Params params, linum_defparam){
 return draw_cparams(P0,current_line_cparams(),params,linum);
}
// NOTE: Line
force_inline Bezier
bez_line(v3 a, v3 b){
 return Bezier{
  a,
  (2.f*a+b)/3.f,
  (a+2.f*b)/3.f,
  b
 };
}
force_inline void
draw(Bezier b, v4 radii, linum_defparam){
 Line_Params params = painter.line_params;
 params.radii = radii;
 draw(b, params, linum);
}
force_inline void
draw(Bezier b, i4 radii, linum_defparam){
 Line_Params params = painter.line_params;
 params.radii = i2f6(radii);
 draw(b, params, linum);
}
// NOTE: omit params
force_inline void
draw(Bezier b, linum_defparam){
 draw(b, painter.line_params, linum);
}
// NOTE: straight line
force_inline void
draw(v3 a, v3 b, Line_Params params=painter.line_params, linum_defparam){
 draw(bez_line(a,b), params, linum);
}
force_inline void
draw_line(v3 a, v3 b, Line_Params params=painter.line_params, linum_defparam){
 params.flags |= Line_Straight;
 draw(bez_line(a,b), params, linum);
}
force_inline ARGB_Color
argb_gray(v1 value){
 macro_clamp_min(value,0.0f);
 macro_clamp_max(value,1.0f);
 return argb_pack(v4{repeat3(value),1});
}

force_inline v4
v4_gray(v1 value) {
 return v4{repeat3(value),1};
}

function argb
compute_fill_color(v1 color_lerp){
 Painter &p = painter;
 argb color = p.fill_params.color;
 if(color_lerp != 0.0f){
  v4 color_v4 = argb_unpack(color);
  // NOTE(kv): 0 is the lerp target
  color_v4.xyz *= (1.f-color_lerp);
  color = argb_pack(color_v4);
 }
 return color;
}

inline void
fill4(v3 p0, v3 p1, v3 p2, v3 p3,
      Fill_Params params=fp(), linum_defparam){
 fill3(p0,p1,p2,params,linum);
 fill3(p0,p2,p3,params,linum);
}
inline void
poly4_inner(v3 p0, v3 p1, v3 p2, v3 p3, 
            argb c0, v1 depth_offset, Poly_Flags flags){
 poly3_inner(p0,p1,p2, c0, depth_offset, flags);
 poly3_inner(p0,p2,p3, c0, depth_offset, flags);
}
xfunction void
fill_bezier_inner_2(v3 P[4], v3 Q[4], argb color, v1 depth_offset, b32 viz)
{
 i32 nslices = bezier_poly_nslice;
 v1 inv_nslices = 1.0f / (v1)nslices;
 v3 previous_worldP;
 v3 previous_worldQ;
 for_i32(sample_index, 0, nslices+1) {
  v1 u = inv_nslices * (v1)sample_index;
  v3 worldP = bezier_sample(P,u);
  v3 worldQ = bezier_sample(Q,u);
  //
  if (sample_index != 0)
  {
   poly4_inner(previous_worldP, previous_worldQ, worldQ, worldP, 
               color, depth_offset, viz);
  }
  previous_worldP = worldP;
  previous_worldQ = worldQ;
 }
}

xfunction void
bezier_poly3_inner(v3 A, v3 P[4], 
                   argb color,
                   v1 depth_offset, Poly_Flags flags)
{
 i32 nslices = bezier_poly_nslice;
 v1 inv_nslices = 1.f / (v1)nslices;
 v3 previous_worldP;
 for_i32(sample_index,0,nslices+1) {
  v1 u = inv_nslices * (v1)sample_index;
  v3 worldP = bezier_sample(P,u);
  if (sample_index != 0) {
   poly3_inner(A, previous_worldP, worldP,
               color,
               depth_offset, flags);
  }
  previous_worldP = worldP;
 }
}
function void
fill(v3 A, Bezier &bezier, 
     argb c0=0,
     Poly_Flags flags=0, linum_defparam){
 set_linum;
 auto p = &painter;
 if(is_poly_enabled()){
  if(c0 == 0){c0 = p->fill_params.color;}
  v3 *P = bezier;
  const i32 npoints = 5;
  v3 points[npoints];
  points[0] = A;
  block_copy(points+1, P, 4*sizeof(*points));
  // TODO: Something fishy with this flag vs painter logic
  if (p->viz_level) { flags |= Poly_Shaded; }
  bezier_poly3_inner(points[0], points+1, c0, p->fill_depth_offset, flags);
 }
}
function void
fill_dbez_inner(const v3 P[4], const v3 Q[4], argb color){
 Painter *p = &painter;
 if(is_poly_enabled()){
  if(color == 0){color = p->fill_params.color;}
  const i32 npoints = 8;
  v3 points[npoints];
  for_i32(ipoint,0,4){points[ipoint+0] = P[ipoint];}
  for_i32(ipoint,0,4){points[ipoint+4] = Q[ipoint];}
  fill_bezier_inner_2(points, points+4, color, p->fill_depth_offset, p->viz_level);
 }
}
force_inline void
fill_dbez(Bezier const&b1, Bezier const&b2, argb color=0, linum_defparam){
 set_linum;
 fill_dbez_inner(b1.e, b2.e, color);
}
function void
fill_dbez(v3 a, v3 b, Bezier const&bezier, argb color=0, linum_defparam){
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
fill_bez(Bezier const&bezier, argb color=0, linum_defparam){
 fill_dbez(bezier.e[0], bezier.e[3], bezier, color, linum);
}
xfunction void
fill_patch_inner(const v3 P[4][4],
                 argb color, v1 depth_offset,
                 b32 viz)
{
 if (is_poly_enabled()){
  const i32 nslice = 16;
  v1 inv_nslice = 1.0f / (v1)nslice;
  v3 prev_v[nslice+1];
  v3 this_v[nslice+1];
  
  for_i32(v_index, 0, nslice+1)  {
   for_i32(u_index, 0, nslice+1) {
    v1 u = inv_nslice * (v1)u_index;
    v1 v = inv_nslice * (v1)v_index;
    v3 world_p = {};
    for_i32(i,0,4) {
     for_i32(j,0,4) {
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
                 color,
                 depth_offset, viz);
    }
   }
   
   copy_array_dst(prev_v, this_v);
  }
 }
} 
function void
fill_patch(const v3 P[4][4], argb color=0) {
 auto p = &painter;
 if(color == 0){color = p->fill_params.color;}
 fill_patch_inner(P, color, p->fill_depth_offset, p->viz_level);
}
function void
fill_patch(v3 P0[4], v3 P1[4],
           v3 P2[4], v3 P3[4],
           argb color=0)
{
 v3 P[4][4];
 copy_array_dst(P[0], P0);
 copy_array_dst(P[1], P1);
 copy_array_dst(P[2], P2);
 copy_array_dst(P[3], P3);
 fill_patch(P, color);
}
force_inline void
fill3_symx(v3 a, v3 b, linum_defparam){
 symx_off;
 fill3(a,b,negateX(b),fp(),linum);
}
function void 
fill_fan(v3 A, v3 verts[], i32 vert_count,
         Fill_Params params=fp(), linum_defparam){
 for_i32(index, 0, vert_count-1) {
  fill3(A, verts[index], verts[index+1], params, linum);
 }
}
function void
draw_box(mat4 const&transform, linum_defparam) {
 set_linum;
 v3 x = mat4vec(transform, V3x(2));
 v3 y = mat4vec(transform, V3y(2));
 v3 z = mat4vec(transform, V3z(2));
 v3 O = transform*V3() - 0.5f*(x+y+z);
 v3 X = O+x;
 draw(bez_line(X,X+y), 0);
 draw(bez_line(X,X+z), 0);
 v3 Y = O+y;
 draw(bez_line(Y,Y+x), 0);
 draw(bez_line(Y,Y+z), 0);
 v3 Z = O+z;
 draw(bez_line(Z,Z+x), 0);
 draw(bez_line(Z,Z+y), 0);
 
 draw(bez_line(O,X), 0); draw(bez_line(O,Y), 0); draw(bez_line(O,Z), 0);
 
 v3 P = O+x+y+z;
 draw(bez_line(P,P-x), 0); draw(bez_line(P,P-y), 0); draw(bez_line(P,P-z), 0);
}
force_inline Patch
patch(Bezier const&p0, Bezier const&p1, Bezier const&p2, Bezier const&p3) {
 Patch result;
 copy_array_dst(result.e[0], p0.e);
 copy_array_dst(result.e[1], p1.e);
 copy_array_dst(result.e[2], p2.e);
 copy_array_dst(result.e[3], p3.e);
 return result;
}

force_inline Patch
patch_symx(Bezier const&P0, Bezier const&P1) {
 Bezier N0 = negateX(P0);
 Bezier N1 = negateX(P1);
 return patch(P0, P1, N1, N0);
}
force_inline v4
big_to_small() {
 v1 big   = 1.f;
 v1 small = 0.5f;
 return V4(big, big, small, small);
}
force_inline v4
small_to_big() {
 v1 big   = 1.f;
 v1 small = 0.5f;
 return V4(small, small, big, big);
}
inline Line_Params
lp(v1 alignment_min, i4 radii={}){
 Line_Params result = painter.line_params;
 result.alignment_min = alignment_min;
 result.radii         = i2f6(radii);
 return result;
}
force_inline void
duo_line(v3 a, v3 b, v3 c, linum_defparam){
 set_linum;
 draw(bez_line(a,b), lp(small_to_big()), 0);
 draw(bez_line(b,c), lp(big_to_small()), 0);
}
force_inline void
duo_line(v3 array[3], linum_defparam){
 set_linum;
 duo_line(array[0], array[1], array[2], 0);
}
function void
draw_image(char *filename, v3 o, v3 x, v3 y, v1 alpha=1.f, v3 color={1,1,1},
           linum_defparam) {
 if(get_hot_prim_id() == u32(linum))
 {// NOTE: tint it
  color = V3(1.f, 1.f, 0.f)*color;
 }
 argb argb_color = argb_pack( V4(color,alpha) );
 push_image(painter.target, filename,o,x,y,argb_color, linum);
}
function void
draw_disk(v3 center, v1 radius,
          argb color, v1 depth_offset, Poly_Flags flags,
          linum_defparam) {
 if(is_poly_enabled()){
  painter.draw_prim_id = linum;
  draw_disk_inner(center, radius, color, depth_offset, flags);
 }
}
xfunction void
indicate_vertex(char *vertex_name, v3 pos,
                i32  force_draw_level=9000,
                b32  force_overlay   =false,
                argb color           =argb_yellow,
                u32  prim_id         =__builtin_LINE())
{
 auto &p = painter;
 if(is_left()){
  p.draw_prim_id = prim_id;
  const v1 radius = 3*millimeter;
  b32 mouse_near = false;
  if(p.cursor_on)
  {
   v1 dist = lensq(pos-p.cursorp);
   mouse_near = dist < squared(3*centimeter);
  }
  
  b32 mouse_on_top = (prim_id == get_hot_prim_id());
  
  b32 should_draw = ((p.viz_level >= force_draw_level) || mouse_near);
  if(should_draw){//NOTE: Draw
   symx_off;
   auto &cparams = current_line_cparams();
   v1 depth_offset = cparams.depth_offset - 1*centimeter;
   u32 flags = 0;
   // NOTE: If lines are overlayed, so should indicators (I guess?)
   b32 line_overlay_on = (p.line_params.flags & Line_Overlay);
   if(line_overlay_on || force_overlay){
    flags = Poly_Overlay;
   }
   draw_disk(pos, radius, color, depth_offset, flags, prim_id);
  }
  
  if(mouse_on_top) {// NOTE: debug
   DEBUG_NAME(vertex_name, prim_id);
  }
 }
}
function Bez
bez_lerp(Bez &begin, v1 t, Bez &end){
 Bez result;
 for_i32(index,0,4){
  result[index] = lerp(begin[index], t, end[index]);
 }
 return result;
}
//-