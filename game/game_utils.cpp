global v1 default_fvert_delta_scale = 0.1f;
#define fvert(value, ...) \
fval( (value), fopts_add_delta_scale(fopts_add_flags(fopts_maybe(__VA_ARGS__), Fui_Flag_Camera_Aligned), default_fvert_delta_scale) )

//-NOTE: Annotations
#define fvertx(value)    V3x(fval(value))
#define fverty(value)    V3y(fval(value))
#define fvertz(value)    V3z(fval(value))

#define fvec  fvert
#define fvecx(value)      V3x(fval(value))
#define fvecy(value)      V3y(fval(value))
#define fvecz(value)      V3z(fval(value))

#define fdir fvec  // NOTE: Not influenced by scale.
//- End annotation
#define fvert3(x,y,z,...) fvert(V3(x,y,z), __VA_ARGS__)
#define funit(value)      fvert(value, Fui_Flag_Camera_Aligned|Fui_Flag_NOZ)

#define fkeyframe(nframes, value)  add_keyframe(ani, fval2i(nframes, value))
#define fhsv(h,s,v) argb_pack(srgb_to_linear(hsv_to_srgb(fval3(h,s,v))))

#define radius_scale_block(multiplier)  \
scale_in_block(painter.line_radius_unit, multiplier)

#define line_radius_fine_tip  scale_in_block(painter.line_radius_unit, 0.25f)
#define line_radius_medium    scale_in_block(painter.line_radius_unit, 0.5f)
#define line_color_lightness(scale)  set_in_block(painter.line_params.color, argb_lightness(painter.line_params.color, scale))

internal void
indicate_vertex(char *vertex_name, v3 pos, i32 level,
                u32 prim_id = __builtin_LINE())
{
 Painter *p = &painter;
 painter.draw_prim_id = prim_id;
 b32 mouse_near;
 v1 radius = 3*millimeter;
 {//NOTE: above
  mat4 object_to_view = p->view_transform * get_object_transform();
  v3 vertex_viewp = mat4vert(object_to_view, pos);
  v2 delta = painter.mouse_viewp - vertex_viewp.xy;
  mouse_near = (absolute(delta.x) < 1*centimeter && 
                absolute(delta.y) < 1*centimeter);
 }
 
 b32 mouse_on_top = (prim_id == get_hot_prim_id());
 
 b32 should_draw = ((painter.viz_level >= level) || mouse_near);
 if (should_draw)
 {//NOTE: Draw
  symx_off;
  v1 depth_offset = painter.line_depth_offset - 1*centimeter;
  u32 flags = 0;
  if (level == 0) { flags = Poly_Overlay; }
  draw_disk(pos, radius, argb_yellow, depth_offset, flags, prim_id);
 }
 
 if ( mouse_on_top )
 {// NOTE: debug
  DEBUG_NAME(vertex_name, prim_id);
 }
}
#define indicate_level(vertex,level)     indicate_vertex(#vertex, vertex, level)
#define indicate(vertex)                 indicate_level(vertex,9000)
#define indicate0(vertex)                indicate_level(vertex,0)

global_const Fui_Flag clampx = Fui_Flag_Clamp_X;
global_const Fui_Flag clampy = Fui_Flag_Clamp_Y;
global_const Fui_Flag clampz = Fui_Flag_Clamp_Z;

inline void
circular_arc_helper(mat4 const&transform, v3 dst[4], v2 src[4])
{
 for_i32 (index,0,4) 
 {
  dst[index] = mat4vert(transform, V3(src[index].x, src[index].y, 0.f));
 }
}

internal void
draw_bezier_circle(mat4 const&transform, v4 radii={})
{
 v1 a = 1.00005519f;
 v1 b = 0.55342686f;
 v1 c = 0.99873585f;
 v2 arc[4] = { V2(a,0.f),V2(c,b),V2(b,c),V2(0.f,a) };
 
 v3 arc1[4], arc2[4], arc3[4], arc4[4];
 circular_arc_helper(transform, arc1, arc);
 for_i32(index,0,4) {arc[index].x *= -1;}
 circular_arc_helper(transform, arc2, arc);
 for_i32(index,0,4) {arc[index].y *= -1;}
 circular_arc_helper(transform, arc3, arc);
 for_i32(index,0,4) {arc[index].x *= -1;}
 circular_arc_helper(transform, arc4, arc);
 
 {
  Line_Params params = painter.line_params;
  params.radii = radii;
  draw(bez(arc1), params);
  draw(bez(arc2), params);
  draw(bez(arc3), params);
  draw(bez(arc4), params);
 }
}

#define WARN_DELTA(a, b, EPSILON) \
if (absolute(a-b) > EPSILON) { DEBUG_NAME("WARN " #a "-" #b, (a-b)); }

struct Eye_Min_Distance_Result
{
 v1 min_distance;
 v3 closest_point;
};
//
internal Eye_Min_Distance_Result
get_eye_min_distance(v3 center, v1 radius, Bezier line)
{
 Eye_Min_Distance_Result result;
 v1 min_lensq = 10000.f;
 i32 sample_count = 8;
 for_i32(index,0,sample_count+1)
 {
  v1 t = v1(index) * (1.f / v1(sample_count));
  v3 sample = bezier_sample(line, t);
  v1 lengthsq = lensq(sample - center);
  if ( lengthsq < min_lensq )
  {
   result.closest_point = sample;
   min_lensq = lengthsq;
  }
 }
 result.min_distance = square_root(min_lensq) - radius;
 return result;
}

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
internal v3
perspective_project_non_hyperbolic(Camera *camera, v3 worldP)
{
 v3 result = (camera->inverse * V4(worldP, 1.f)).xyz;
 v1 depth = camera->distance-result.z;
 result.xy *= (camera->focal_length / depth);
 result.z   = depth;
 return result;
}

force_inline Line_Params
lp_alignment_threshold(v1 threshold)
{
 Line_Params result = painter.line_params;
 result.alignment_threshold = threshold;
 return result;
}

inline v3
camera_object_position()
{
 v3 result = get_object_transform().inv * camera_world_position(painter.camera);
 return result;
}

//-
inline v3
get_view_vector()
{
 return painter.view_vector_stack[painter.view_vector_count-1];
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

inline void
pop_view_vector()
{
 auto &p = painter;
 p.view_vector_count--;
 kv_assert(p.view_vector_count > 0);
}

internal void
debug_view_vector(i1 linum=__builtin_LINE())
{
 Painter &p = painter;
 v3 camera_obj = camera_object_position();
 v3 view_vector = get_view_vector();
 v3 object_center = camera_obj + view_vector;
 indicate_vertex("view_center", object_center, 0, linum);
 DEBUG_VALUE(view_vector);
}
//-

struct v3_pair {
 union{v3 u; v3 a;};
 union{v3 v; v3 b;};
};
typedef v3_pair v6;

inline v3_pair
invent_uv(v3 n)
{
 v3 u = (almost_equal(absolute(n.x), 1.f, 1e-4f) ?
         cross(n, V3y(1.f)) :
         cross(n, V3x(1.f)));
 u = noz(u);
 v3 v = cross(n,u);
 return v3_pair{u,v};
}

internal v4
plane_transform(mat4 const&mat, v3 n, v1 d)
{
 v3 p0 = -d*n;
 v3_pair uv = invent_uv(n);
 v3 n1 = noz( cross(mat4vec(mat,uv.u), mat4vec(mat,uv.v)) );
 v1 d1 = -dot(n1, mat4vert(mat, p0));
 return V4(n1,d1);
}

//~ begin

internal void
set_object_transform(mat4i const&transform)
{
 Painter *p = &painter;
 p->obj_to_camera = invert(p->camera->transform) * transform;
 push_object_transform_to_target(p->target, &transform.m);
}

internal void
push_object_transform(mat4i const&child_to_parent)
{
 Painter *p = &painter;
 mat4i &parent = get_object_transform();
 mat4i new_transform = parent * child_to_parent;
 p->transform_stack[p->transform_count++] = new_transform;
 kv_assert(p->transform_count <= alen(p->transform_stack));
 set_object_transform(new_transform);
}
//
internal void
pop_object_transform()
{
 Painter *p = &painter;
 p->transform_count--;
 kv_assert(p->transform_count > 0);
 mat4i &parent = get_object_transform();
 set_object_transform( parent );
}

#define transform_block(transform) \
push_object_transform(transform); \
defer( pop_object_transform(); );

#define object_block(transform, center) \
push_object(transform, center); \
defer( pop_object(); );

internal void
push_object(mat4i const&child_to_parent, v3 object_center)
{
 push_object_transform(child_to_parent);
 push_view_vector(object_center);
}
//
internal void
pop_object()
{
 pop_object_transform();
 pop_view_vector();
}

internal mat4i &
get_parent_transform()
{
 auto &p = painter;
 kv_assert(p.transform_count > 1);
 return   p.transform_stack[p.transform_count-2] ;
}

internal mat4
from_parent()
{
 return   get_object_transform().inverse * get_parent_transform() ;
}

//~ end

internal v3
bezier_deriv_div3(Bezier bezier, v1 t)
{//NOTE: The real derivative is multiplied by 3, for some reason
 v3 result = {};
 for_i32(index,0,3)
 {
  result += quad_bernstein(index, t) * (bezier[index+1] - bezier[index]);
 }
 return result;
}

internal v1
bezier_curvature(v2 p[4], v1 t)
{
 v2 deriv = {};
 for_i32(index,0,3)
 {
  deriv += quad_bernstein(index, t) * (p[index+1] - p[index]);
 }
 deriv *= 3.f;
 
 v2 second_deriv = 6.f*((1.f-t)*(p[0] - 2.f*p[1] + p[2]) +
                        t      *(p[1] - 2.f*p[2] + p[3]));
 
 v1 result = cross2d(deriv, second_deriv);
 return result;
}

#if 0
// NOTE: A line is still a v3
internal v3
bezier_tangent(Bezier bezier, v1 t)
{
 v3 point = bezier_sample(bezier,t);
 //NOTE: Grounding the bezier curve
 for_i32(index,0,4)
 {
  bezier[index] -= point;
 }
}
#endif

global_const Fui_Options f20th = Fui_Options{0, 0.05f};
global_const Fui_Options f10th = Fui_Options{0, 0.1f};
global_const Fui_Options f10s  = Fui_Options{0, 10.f};

force_inline Fui_Options
fscale(v1 delta_scale)
{
 Fui_Options result = {};
 result.delta_scale = delta_scale;
 return result;
}

force_inline void
profile_visible(Line_Params *params, v1 edge1)
{
 if (painter.profile_score > edge1) { params->visibility = 1.f; }
 else { params->visibility = 0.f; }
}
force_inline Line_Params
profile_visible(v1 edge1)
{
 Line_Params params = painter.line_params;
 profile_visible(&params, edge1);
 return params;
}

force_inline Line_Params
line_color(argb color)
{
 Line_Params result = painter.line_params;
 result.color = color;
 return result;
}

force_inline Bezier
reverse(Bezier B)
{
 return {B[3], B[2], B[1], B[0]};
}

force_inline v3
reflect_origin(v3 origin, v3 point)
{
 return origin-(point-origin);
}

inline Line_Params
lp(i4 radii)
{
 Line_Params result = painter.line_params;
 result.radii = i2f6(radii);
 return result;
}

inline Line_Params
lp(v1 alignment_threshold, i4 radii)
{
 Line_Params result = painter.line_params;
 result.alignment_threshold = alignment_threshold;
 result.radii               = i2f6(radii);
 return result;
}

inline i1
get_preset()
{
 return painter.viewport->preset;
}

inline v3 get_camz() { return painter.camera->z.xyz; }

inline b32 camera_is_right() {
 return(almost_equal(get_camz().x, -1.f, 1e-2f));
}
//
inline b32 camera_is_left() {
 return(almost_equal(get_camz().x, +1.f, 1e-2f));
}
//
inline b32 camera_is_front() {
 v3 camz = get_camz();
 return(almost_equal(camz.z, +1.f, 1e-2f));
}
//
inline b32 camera_is_back() {
 v3 camz = get_camz();
 return(almost_equal(camz.z, -1.f, 1e-2f));
}

//~ EOF;