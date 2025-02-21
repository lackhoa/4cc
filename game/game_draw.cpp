//-Location business and stuff
function Location
resolve_range(Unresolved_Location location)
{// NOTE(kv) Resolving range at runtime isn't great,
 // but maybe we don't care, since it's only done per-primitive.
 Location result = {};
 if(is_valid(location))
 {// NOTE(kv) We might wanna pass null location in,
  // to test, or when we don't have instrumentation.
  result.file = location.file;
  sarray(Marker_Pair) ranges = driver_data.marker_pairs[location.file];
  result.range = ranges[location.range_index];
 }
 return result;
}
function void
set_draw_location(Location location)
{
 painter->current_draw_location = location;
 for_i32(i, 0, painter->hot_locations.count)
 {//-Set hot state
  if(painter->current_draw_location == painter->hot_locations[i])
  {
   painter->current_location_is_hot = true;
   break;
  }
 }
}
myinline void
set_draw_location(Unresolved_Location location0)
{
 set_draw_location(resolve_range(location0));
}
myinline void
clear_draw_location()
{
 painter->current_draw_location = {};
 painter->current_location_is_hot = false;
}
myinline b32
current_location_is_hot()
{
 b32 result = painter->current_location_is_hot;
 return result;
}
myinline b32
is_painting_enabled()
{
 return (painter->params.painting or
         (current_location_is_hot() and is_left()));
}
myinline b32 is_fill_enabled(){ return is_painting_enabled(); }
myinline b32 is_line_enabled(){ return is_painting_enabled(); }
//-
// NOTE used in @draw_bezier_inner
function void
draw_disk_camera_space(v3 center, v1 radius_camera_space,
                       argb color, Poly_Flags flags, i32 nslice=8)
{
 // @Slow LOD
 if(radius_camera_space > 0.f)
 {
  v1 interval = 1.f / v1(nslice);
  v3 last_sample;
  for_i32(index, 0, nslice+1)
  {
   v1 angle = interval * v1(index);
   v2 arm = radius_camera_space*arm2(angle);
   mat4 &bone_from_cam = painter->cam_from_bone.inverse;
   v3 sample = center + mat4vec(bone_from_cam, V3(arm, 0.f));  // @Slow
   if(index!=0)
   {
    poly3_inner({center, last_sample, sample},
                repeat3(color), flags);
   }
   last_sample = sample;
  }
 }
}
function void
fill_disk(v3 center, tdim radius_bone_space, Fill_Params params=get_fill_params())
{
 if(is_fill_enabled())
 {
  Poly_Flags flags = to_poly_flags(params.flags);
  i32 nslice = 16;  // note(kv) just testing man
  {
   v1 interval = 1.f / v1(nslice);
   v3 last_sample;
   for_i32(index, 0, nslice+1)
   {
    v1 angle = interval * v1(index);
    v2 arm = arm2(angle);
    mat4 &bone_from_cam = painter->cam_from_bone.inverse;
    v3 vector = mat4vec(bone_from_cam, V3(arm, 0.f));
    vector = radius_bone_space.v * noz(vector);
    v3 sample = center + vector;  // @Slow
    if(index!=0)
    {
     poly3_inner({center, last_sample, sample},
                 repeat3(params.color), flags);
    }
    last_sample = sample;
   }
  }
 }
}
myinline void
poly4_inner(v3 p0, v3 p1, v3 p2, v3 p3, 
            argb c0, Poly_Flags flags)
{
 poly3_inner({p0,p1,p2}, repeat3(c0), flags);
 poly3_inner({p0,p2,p3}, repeat3(c0), flags);
}
function void
fill_patch(tvert P[4][4], Fill_Params params=get_fill_params())
{
 if(is_fill_enabled())
 {
  Poly_Flags flags = to_poly_flags(params.flags);
  i32 const nslice = 16;
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
                 params.color, flags);
    }
   }
   
   copy_array_dst(prev_v, this_v);
  }
 }
}

//-NOTE(kv) Highest-level drawing functions
//  We wanna share most of these functions with the driver.
global u32 bs_cycle_counter;

#define set_linum       if (linum!=0) { painter->draw_prim_id = linum; }
#define macro_control_points(p0,d0, d3,p3)  p0,p0+d0, p3+d3,p3
// NOTE(kv) IMPORTANT You can't merge this with painting_disabled, because
// sometimes we set symx off, turn it back on inside that block...
// Also, we can't just do an "if", because we set variables inside that block.
// (I mean, maybe we shouldn't, I'm just never sure)
/*#define symx_off SetInBlock(painter->symx, false)
#define symx_on  SetInBlock(painter->symx, true)*/

struct Paint_Params_Block
{
 Paint_Params saved_params;
 myinline Paint_Params_Block() { this->saved_params = painter->params; }
 myinline ~Paint_Params_Block(){ painter->params = this->saved_params; }
};
#define PaintBlock Paint_Params_Block line_unique_var

#define hl_block_color(...) \
PaintBlock; \
painter->params = mk_highlight_params(__VA_ARGS__)

#define hl_block  hl_block_color(0)

function b32
should_send_model_data()
{
 b32 left = not is_right();
 b32 main_viewport = implies(painter != 0, is_main_viewport(painter->viewport));
 return left and main_viewport;
}
function void
send_primitive(Recorded_Primitive &primitive)
{
 Location location = painter->current_draw_location;
 if(should_send_model_data() and is_valid(location))
 {
  primitive.location = location;
  primitive.bone_id = current_bone()->id;
  push(&the_model->primitives, primitive);
 }
}

function void
send_poly3(Poly3 points)
{
 Location location = painter->current_draw_location;
 Recorded_Primitive fill = {.type = Primitive_Type_Poly3};
 fill.poly3 = points;
 send_primitive(fill);
}
function void
fill3_no_send(Poly3 points, Fill_Params params=get_fill_params())
{
 // todo Implement
}
function void
fill3(v3 p0, v3 p1, v3 p2,
      Fill_Params params=get_fill_params())
{
 Poly3 points = {p0,p1,p2};
 send_poly3(points);
 
 if(is_fill_enabled())
 {
  v1 depth_offset = painter->params.fill_depth_offset;
  Poly_Flags flags = to_poly_flags(params.flags);
  poly3_inner(points, repeat3(params.color), flags);
 }
}
//-
myinline Bezier
bez_raw(v3 p0, v3 p1, v3 p2, v3 p3)
{
 return Bezier{ p0,p1,p2,p3 };
}
myinline Bezier
bez_raw(v3 P[4])
{
 return Bezier{ P[0],P[1],P[2],P[3] };
}
myinline Bezier
bez_offset(v3 p0, v3 d0, v3 d3, v3 p3)
{
 return bez_raw(macro_control_points(p0,d0,d3,p3) );
}

// TODO(kv) I have plan to change all clean up all the cruft:
//  we're gonna have composite curve sliders.
//  Then we can change slider type -> win!

// NOTE(kv) On length dependency: we don't want it
// What is it? It means that we should specify things so the curve
//   scale with the distance between the endpoints.
// Look, if the length changes, so would the curve.
// There just isn't much point to specifying curves that look the same
//   when the distance between the endpoints change.
// And it's slower and more complicated.
function Bezier
bez_v3v3(v3 p0, v3 d0, v3 d3, v3 p3)
{
 TIMED_BLOCK(bs_cycle_counter);
 v1 length = lengthof(p3-p0);
 v3 p1 = (2.f*p0 + p3)/3.f + length*d0;
 v3 p2 = (p0 + 2.f*p3)/3.f + length*d3;
 return bez_raw(p0,p1,p2,p3);
}
// NOTE: Parabola (no length dependence)
function Bezier
bez_parabola(v3 p0, v3 d, v3 p3)
{
 TIMED_BLOCK(bs_cycle_counter);
 v3 q = 0.5f*(p0 + p3) + d;
 v3 p1 = (p0 + 2.f*q) / 3.f;
 v3 p2 = (2.f*q + p3) / 3.f;
 return bez_raw(p0,p1,p2,p3);
}

// NOTE: Planar curve (with v3 control point)
function Bezier
bez_v3v2(v3 p0, v3 d0, v2 d3, v3 p3)
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

// NOTE: Planar curve with unit vector guide
function Bezier
bez_unit(v3 p0, v2 d0, v2 d3, v3 unit_y, v3 p3)
{
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
myinline Bezier
bez_unit2(v3 p0, v4 d0d3, v3 unit_y, v3 p3){
 return bez_unit(p0, d0d3.xy, d0d3.zw, unit_y, p3);
}
// NOTE: Planar curve (with v3 control point)
myinline Bezier
bezd_len(v3 p0, v3 d0, v2 d3, v3 p3)
{
 d0 *= lengthof(p3-p0);
 return bez_v3v2(p0, d0, d3, p3);
}
//NOTE: Planar curve (with v3 control point, BUT it doesn't automatically adjust d3)
// #deprecated
function Bezier
bez_bezd_old(v3 p0, v3 d0, v2 d3, v3 p3)
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
function Bezier 
bez_c2(Bez const&ref, tvec d3, tvert p3)
{
 TIMED_BLOCK(bs_cycle_counter);
 tvert p0 = ref.e[3];
 tvert p1 = p0 + (ref.e[3] - ref.e[2]);
 tvert p2 = mkvert(0.5f*(v3(p3)+v3(p1))) + d3;
 return bez_raw(p0,p1,p2,p3);
}
function v4
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
function void
draw_bezier(tvert P[4], Line_Params params)
{
 {//-send data
  Recorded_Primitive primitive = {.type = Primitive_Type_Curve};
  for_i32(i,0,4)
  {
   primitive.curve[i] = P[i];
  }
  send_primitive(primitive);
 }
 
 if(is_line_enabled())
 {
  b32 do_draw = true;
  
  //NOTE(kv) We allow *selecting* the curve, whether or not we *actually* draw it.
  Location location = painter->current_draw_location;
  b32 is_hot = current_location_is_hot();
  
  mat4i &world_from_bone = current_world_from_bone();
  
  if(HasFlag(params.flags, Line_Invisible))
  {
   do_draw = false;
  }
  
  b32 is_straight = params.flags & Line_Straight;
  b32 do_check_alignment = (do_draw and
                            not is_straight and
                            not painter->ignore_alignment_min and
                            not is_hot and
                            params.alignment_min > 0.f);
  if(do_check_alignment)
  {//-NOTE(kv) Alignment business
   tvert A = P[0];
   tvert B = P[1];
   tvert C = P[2];
   tvert D = P[3];
   // NOTE(kv) The normal is only defined when the curve is planar; choosing ABD or ACD is arbitrary
   v3 normal = noz(cross(B-A, D-A));
   if(normal != v3{})
   {// NOTE If it's a line, I mean probably the user hasn't made the curve yet
    v1 alignment;
    {
     v3 centroid = 0.5f*(A+D);  // NOTE: our curves are kinda straight most of the time, so I guess this works
     mat4 &bone_from_world = world_from_bone.inv;
     v3 camera_obj = mat4vert(bone_from_world, camera_world_position(painter->camera));
     v3 view_vector = noz(camera_obj - centroid);
     alignment = absolute(dot(normal,view_vector));
    }
    do_draw = alignment > params.alignment_min;
   }
  }
  
  if(do_draw)
  {
   argb color = (is_hot ? hot_color
                 : painter->params.line_color);
   draw_bezier_inner(P, params, color);
  }
  
  // NOTE(kv) We want "draw" to be like a command, so don't return anything.
  painter->previous_draw_result = do_draw;
 }
}
myinline void
draw(Bezier P, Line_Params params=get_line_params())
{
 draw_bezier(P, params);
}
// NOTE: Line
myinline Bezier
bez_line(v3 a, v3 b)
{
 return Bezier{
  a,
  (2.f*a+b)/3.f,
  (a+2.f*b)/3.f,
  b
 };
}
myinline void
draw(Bezier b, v4 radii){
 Line_Params params = get_line_params();
 params.radii = radii;
 draw(b, params);
}
myinline void
draw(Bezier b, i4 radii){
 Line_Params params = get_line_params();
 params.radii = i2f6(radii);
 draw( b, params);
}
// NOTE: Straight line
function void
draw_line(v3 a, v3 b, Line_Params params=get_line_params())
{// NOTE(kv) Don't do fancy footwork in here,
 // since the bezier logic is so crazy!
 params.flags |= Line_Straight;
 Bezier bez = bez_line(a,b);
 draw_bezier(bez, params);
}
//~
myinline ARGB_Color
argb_gray(v1 value){
 ClampBot(value,0.0f);
 ClampTop(value,1.0f);
 return argb_pack(v4{repeat3(value),1});
}

myinline v4
v4_gray(v1 value) {
 return v4{repeat3(value),1};
}

function argb
compute_fill_color(v1 color_lerp)
{
 argb color = get_fill_params().color;
 if(color_lerp != 0.0f){
  v4 color_v4 = argb_unpack(color);
  // NOTE(kv): 0 is the lerp target
  color_v4.xyz *= (1.f-color_lerp);
  color = argb_pack(color_v4);
 }
 return color;
}

myinline void
fill4(v3 p0, v3 p1, v3 p2, v3 p3,
      Fill_Params params=get_fill_params())
{
 fill3(p0,p1,p2,params);
 fill3(p0,p2,p3,params);
}
myinline v3
get_triangle_normal(v3 a, v3 b, v3 c)
{// NOTE No normalization ,because I'm not a weirdo
 return cross(b-a, c-a);
}

function void
fill_dual_bez(tvert P[4], tvert Q[4], Fill_Params params=get_fill_params())
{
 {//-Sending data
  Recorded_Primitive primitive = {.type=Primitive_Type_Dual_Bezier};
  primitive.dual_bezier = push_struct(the_model->frame_arena, Dual_Bezier);
  block_copy(primitive.dual_bezier->P, P, 4*sizeof(v3));
  block_copy(primitive.dual_bezier->Q, Q, 4*sizeof(v3));
  send_primitive(primitive);
 }
 
 if(is_fill_enabled())
 {
  b32 do_fill = true;
  
  if(do_fill and
     (params.flags.v & Fill_Culled) and
     not current_location_is_hot())
  {//-Culling
   v3 normal = get_triangle_normal(P[0], Q[0], Q[3]);
   v1 dot_result = dot(normal, get_view_vector());
   do_fill = ((params.flags.v & Fill_Inverted) ?
              dot_result < 0 :
              dot_result > 0);
  }
  
  if(do_fill)
  {
   Poly_Flags flags = to_poly_flags(params.flags);
   i32 nslices = bezier_poly_nslice;
   v1 inv_nslices = 1.0f / (v1)nslices;
   v3 A0;
   v3 B0;
   for_i32(sample_index, 0, nslices+1)
   {
    v1 u = inv_nslices * (v1)sample_index;
    tvert A = bezier_sample(P,u);
    tvert B = bezier_sample(Q,u);
    if(sample_index > 0)
    {
     poly4_inner(A0,B0,B,A, params.color, flags);
    }
    A0 = A;
    B0 = B;
   }
  }
 }
}
function void
fill_point_bez(tvert O, Bezier const&bezier, Fill_Params params=get_fill_params())
{
 Bezier dummy;
 for_i32(i,0,4){ dummy[i] = O; }
 fill_dual_bez(dummy.e, (tvert *)bezier.e, params);
}
myinline void
fill_line_bez(tvert a, tvert b, Bezier const&bezier, Fill_Params params=get_fill_params())
{
 Bez ab = bez_line(a,b);
 fill_dual_bez(ab.e, (tvert *)bezier.e, params);
}
myinline void
fill_bez(Bezier const&bezier, Fill_Params params=get_fill_params())
{
 fill_line_bez(bezier.e[0], bezier.e[3], bezier, params);
}
function void
fill_patch(tvert P0[4], tvert P1[4],
           tvert P2[4], tvert P3[4],
           Fill_Params params=get_fill_params())
{
 tvert P[4][4];
 copy_array_dst(P[0], P0);
 copy_array_dst(P[1], P1);
 copy_array_dst(P[2], P2);
 copy_array_dst(P[3], P3);
 fill_patch(P, params);
}
myinline void
fill3_symx(v3 a, v3 b)
{
 if(is_left())
 {
  fill3(a,b,negateX(b));
 }
}
myinline void
fill4_symx(v3 a, v3 b)
{
 if(is_left())
 {
  fill4(a, b, negateX(b), negateX(a));
 }
}
function void 
fill_fan(tvert A, tvert verts[], i32 vert_count,
         Fill_Params params=get_fill_params())
{
 for_i32(index, 0, vert_count-1)
 {
  fill3(A, verts[index], verts[index+1], params);
 }
}

function b32
fill4_culled(v3 p0, v3 p1, v3 p2, v3 p3,
             Fill_Params params=get_fill_params())
{
 b32 filled = false;
 v3 p01 = p1-p0;
 v3 p02 = p2-p0;
 v3 normal = cross(p01, p02);
 if(dot(normal, get_view_vector()) > 0)
 {
  fill4(p0,p1,p2,p3, params);
  filled = true;
 }
 return filled;
}
function void
fill_sphere(v3 center, tdim radius, Fill_Params params=get_fill_params())
{// NOTE Wait, is a sphere, or just a disk?
 fill_disk(center, radius, params);
}
#if 0
function void
fill_hiding_box(v3 P000, v3 x, v3 y, v3 z)
{// NOTE(kv) Filling a box, with the normals inverted
 v3 P100 = P000+x;
 v3 P010 = P000+y;
 v3 P001 = P000+z;
 v3 P110 = P100 + y;
 v3 P011 = P010 + z;
 v3 P101 = P001 + x;
 v3 P111 = P110 + z;
 
 if(not fill4_culled(P000, P100, P110, P010))
 {// NOTE z is the normal
  fill4(P001, P011, P111, P101);
 }
 if(not fill4_culled(P000, P010, P011, P001))
 {// NOTE x is the normal
  fill4(P100, P101, P111, P110);
 }
 if(not fill4_culled(P000, P001, P101, P100))
 {// NOTE y is the normal
  fill4(P010, P110, P111, P011);
 }
}
function void
draw_box(mat4 const&transform)
{
 v3 x = mat4vec(transform, V3x(2));
 v3 y = mat4vec(transform, V3y(2));
 v3 z = mat4vec(transform, V3z(2));
 v3 O = transform*V3() - 0.5f*(x+y+z);
 v3 X = O+x;
 draw( bez_line(X,X+y), 0);
 draw( bez_line(X,X+z), 0);
 v3 Y = O+y;
 draw( bez_line(Y,Y+x), 0);
 draw( bez_line(Y,Y+z), 0);
 v3 Z = O+z;
 draw( bez_line(Z,Z+x), 0);
 draw( bez_line(Z,Z+y), 0);
 
 draw( bez_line(O,X), 0);
 draw( bez_line(O,Y), 0);
 draw( bez_line(O,Z), 0);
 
 v3 P = O+x+y+z;
 draw( bez_line(P,P-x), 0);
 draw( bez_line(P,P-y), 0);
 draw( bez_line(P,P-z), 0);
}
#endif
myinline Patch
patch(Bezier const&p0, Bezier const&p1, Bezier const&p2, Bezier const&p3) {
 Patch result;
 copy_array_dst(result.e[0], p0.e);
 copy_array_dst(result.e[1], p1.e);
 copy_array_dst(result.e[2], p2.e);
 copy_array_dst(result.e[3], p3.e);
 return result;
}

myinline Patch
patch_symx(Bezier const&P0, Bezier const&P1) {
 Bezier N0 = negateX(P0);
 Bezier N1 = negateX(P1);
 return patch(P0, P1, N1, N0);
}
myinline v4
big_to_small() {
 v1 big   = 1.f;
 v1 small = 0.5f;
 return V4(big, big, small, small);
}
myinline v4
small_to_big() {
 v1 big   = 1.f;
 v1 small = 0.5f;
 return V4(small, small, big, big);
}
myinline i4
I4_sym(i2 v)
{
 return i4{v[0], v[1], v[1], v[0]};
}
function Line_Params
get_line_params(v1 alignment_min, i4 radii={})
{
 Line_Params result = get_line_params();
 result.alignment_min = alignment_min;
 result.radii         = i2f6(radii);
 return result;
}
myinline void
nduo_line( v3 a, v3 b, v3 c)
{
 draw( bez_line(a,b), get_line_params(small_to_big()));
 draw( bez_line(b,c), get_line_params(big_to_small()));
}
myinline void
nduo_line( v3 array[3])
{
 nduo_line( array[0], array[1], array[2]);
}
function void
draw_image(Stringz image_file,
           v3 o, v3 x, v3 y,
           v1 alpha=1.f, v3 color={1,1,1})
{
 if(current_location_is_hot())
 {// NOTE: tint it
  color = V3(1.f, 1.f, 0.f)*color;
 }
 argb argb_color = argb_pack( V4(color,alpha) );
 push_image(painter->target, image_file, o,x,y,argb_color);
}
function Bezier
bez_lerp(Bezier &begin, v1 t, Bezier &end)
{// NOTE(kv) I guess we can do this?
 Bezier result;
 for_i32(index,0,4)
 {
  result[index] = mkvert(lerp(begin[index], t, end[index]));
 }
 return result;
}
function void
send_vert(i32 info_index, v3 pos)
{
 if(should_send_model_data())
 {
  Vertex vertex = {};
  vertex.info_index = info_index;
  vertex.pos        = pos;
  vertex.bone_id    = current_bone()->id;
  push(&the_model->vertices, vertex);
 }
}
function Bez_v2
bez_v2_offset(v2 a, v2 a_offset, v2 b)
{
 Bez_v2 result;
 result.e[0] = a;
 result.e[1] = a + a_offset;
 result.e[2] = b;
 return result;
}
struct v3_pair
{
 union{v3 u,a,x;};
 union{v3 v,b,y;};
};
function v3_pair
invent_xy(v3 z)
{
 v3 x;
 b32 z_is_close_to_y = almost_equal(absolute(z.y), 1.f, 1e-8f);
 if(z_is_close_to_y)
 {
  x = V3x(1.f);
 }
 else
 {// NOTE y is the up vector
  v3 up_vector = V3y(1);
  x = noz(cross(up_vector, z));
 }
 
 v3 y = cross(z,x);
 return v3_pair{x,y};
}
function void
split_bezier(v4 p, v1 tsplit, v4 *left, v4 *right)
{// NOTE Return the curve from t=0 to t="split"
 // NOTE(kv) The way it works is explained by the de Casteljau algorithm
 // (which works by literally splitting the curve).
 // but I don't understand why the de Casteljau algorithm works either so :>
 v1 P01 = lerp(p[0], tsplit, p[1]);
 v1 P12 = lerp(p[1], tsplit, p[2]);
 v1 L2 = lerp(P01, tsplit, P12);
 v1 P = bezier_sample(p, tsplit);  // NOTE The last point is obviously on the original curve
 *left = {p[0], P01, L2, P};
 
 v1 P23 = lerp(p[2], tsplit, p[3]);
 v1 R1 = lerp(P12, tsplit, P23);
 *right = {P, R1, P23, p[3]};
}
function void
draw_circle(v3 center, tnormal unit_normal, tdim radius,
            Line_Params params=get_line_params())
{
 v1 a = 1.00005519f;
 v1 b = 0.55342686f;
 v1 c = 0.99873585f;
 v2 arc0v2[4] = { {a,0}, {c,b}, {b,c}, {0,a} };
 v2 arc1v2[4], arc2v2[4], arc3v2[4];
 for_i32(i,0,4)
 {
  arc1v2[i] = perp(arc0v2[i]);
  arc2v2[i] = perp(arc1v2[i]);
  arc3v2[i] = perp(arc2v2[i]);
 }
 
 v3_pair xy = invent_xy(unit_normal);
 mat4 transform = mat4_columns(radius*xy.x, radius*xy.y, radius*unit_normal, center);
 
 auto circular_arc_helper = [&]( v3 dst[4], v2 source[4])
 {
  for_i32(index,0,4)
  {
   v2 src = source[index];
   dst[index] = mat4vert(transform, V3(src.x, src.y, 0.f));
  }
 };
 
 v3 arc0[4], arc1[4], arc2[4], arc3[4];
 circular_arc_helper(arc0, arc0v2);
 
 circular_arc_helper(arc1, arc1v2);
 circular_arc_helper(arc2, arc2v2);
 circular_arc_helper(arc3, arc3v2);
 
 v4 radii_list[4];
 {
  v4 radii_first_half, radii_second_half;
  split_bezier(params.radii, 0.5f, &radii_first_half, &radii_second_half);
  split_bezier(radii_first_half,  0.5f, &radii_list[0], &radii_list[1]);
  split_bezier(radii_second_half, 0.5f, &radii_list[2], &radii_list[3]);
 }
 
 params.radii = radii_list[0];
 draw(bez_raw(arc0), params);
 
 params.radii = radii_list[1];
 draw(bez_raw(arc1), params);
 
 params.radii = radii_list[2];
 draw(bez_raw(arc2), params);
 
 params.radii = radii_list[3];
 draw(bez_raw(arc3), params);
}
function void
draw_circle(v3 center, tnormal unit_normal, tdim radius, v4 radii)
{
 draw_circle(center, unit_normal, radius, get_line_params(radii));
}
//-