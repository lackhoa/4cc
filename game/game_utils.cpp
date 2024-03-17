#define fvert(value, ...) \
fval( (value), fopts_add_delta_scale(fopts_add_flags(fopts_maybe(__VA_ARGS__), Fui_Flag_Camera_Aligned), default_fvert_delta_scale) )
#define fvec fvert  // NOTE: This is just an annotation
#define fvert3(x,y,z,...) fvert(vec3 (x,y,z), __VA_ARGS__)
#define fvertx(value,...) fvert(vec3x(value), fopts_add_flags(fopts_maybe(__VA_ARGS__), clampy|clampz))
#define fverty(value,...) fvert(vec3y(value), fopts_add_flags(fopts_maybe(__VA_ARGS__), clampx|clampz))
#define fvertz(value,...) fvert(vec3z(value), fopts_add_flags(fopts_maybe(__VA_ARGS__), clampx|clampy))
#define fvert0(...)       fvert3(0,0,0, __VA_ARGS__)
#define fkeyframe(nframes, value)  add_keyframe(ani, fval2i(nframes, value))
#define funit(value)          fvert(value, Fui_Flag_Camera_Aligned|Fui_Flag_NOZ)
#define fhsv(h,s,v) argb_pack(srgb_to_linear(hsv_to_srgb(fval3(h,s,v))))

#define radius_scale_block(multiplier)  \
scale_in_block(p->line_radius_unit, multiplier)

#define line_radius_fine_tip  scale_in_block(p->line_radius_unit, 0.25f)
#define line_radius_medium    scale_in_block(p->line_radius_unit, 0.5f)
#define line_color_lightness(scale)  set_in_block(p->line_params.color, argb_lightness(p->line_params.color, scale))

internal void
indicate_vertex(Painter *p, v2 mouse_viewp, 
                char *vertex_name, i32 line_number, 
                v3 vertex_worldp, i32 viz_level, i32 indicate_level)
{
 b32 mouse_near;
 b32 mouse_on_top;
 v1 radius = 3*millimeter;
 {//NOTE: above
  mat4 object_to_view = p->view_transform * object_transform;
  v3 vertex_viewp = mat4vert(object_to_view, vertex_worldp);
  v2 delta = mouse_viewp - vertex_viewp.xy;
  mouse_near = (absolute(delta.x) < 1*centimeter && 
                absolute(delta.y) < 1*centimeter);
  mouse_on_top = (absolute(delta.x) < radius && 
                  absolute(delta.y) < radius);
 }
 
 argb draw_color = argb_yellow;
 argb debug_color = 0xffffff00;
 if (mouse_on_top) { draw_color = argb_red; debug_color = 0xffff0000; }
 
 b32 should_draw = (indicate_level == 0 || mouse_near);
 if (should_draw)
 {//NOTE: draw
  symx_off;
  //TODO: What if we only draw if the mouse is nearby?
  mat4 camera_to_obj = object_transform.inverse * p->camera->forward;
  draw_disk(p->target, camera_to_obj, vertex_worldp, radius, draw_color, 0.f, Poly_Overlay);
 }
 
 if (mouse_near)
 {// NOTE: debug
  DEBUG_NAME_COLOR(vertex_name, line_number, debug_color);
 }
}

global Fui_Flag clampx = Fui_Flag_Clamp_X;
global Fui_Flag clampy = Fui_Flag_Clamp_Y;
global Fui_Flag clampz = Fui_Flag_Clamp_Z;

inline void
circular_arc_helper(mat4 const&transform, v3 dst[4], v2 src[4])
{
 for_i32 (index,0,4) 
 {
  dst[index] = mat4vert(transform, vec3(src[index].x, src[index].y, 0.f));
 }
}

internal void
draw_bezier_circle(Painter *p, mat4 const&transform, v4 radii={})
{
 v1 a = 1.00005519f;
 v1 b = 0.55342686f;
 v1 c = 0.99873585f;
 v2 arc[4] = { vec2(a,0.f),vec2(c,b),vec2(b,c),vec2(0.f,a) };
 
 v3 arc1[4], arc2[4], arc3[4], arc4[4];
 circular_arc_helper(transform, arc1, arc);
 for_i32(index,0,4) {arc[index].x *= -1;}
 circular_arc_helper(transform, arc2, arc);
 for_i32(index,0,4) {arc[index].y *= -1;}
 circular_arc_helper(transform, arc3, arc);
 for_i32(index,0,4) {arc[index].x *= -1;}
 circular_arc_helper(transform, arc4, arc);
 
 {
  Line_Params params = p->line_params;
  params.radii = radii;
  draw(p, bez(arc1), params);
  draw(p, bez(arc2), params);
  draw(p, bez(arc3), params);
  draw(p, bez(arc4), params);
 }
}

#define WARN_DELTA(a, b, EPSILON) \
if (absolute(a-b) > EPSILON) { DEBUG_NAME("WARN " #a "-" #b, a-b); }

union Head
{
 struct {
  v3 back_out;
  v3 jaw;
  v3 behind_ear;
  v3 behind_earR;
  v3 neck_junction;
  v3 noseY;
  v3 trapezius_head;
  v3 trapezius_headR;
  v3 chin_middle;
  v3 top;
 };
 v3 verts[];
};

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
 v3 result = (camera->inverse * vec4(worldP, 1.f)).xyz;
 v1 depth = camera->distance-result.z;
 result.xy *= (camera->focal_length / depth);
 result.z   = depth;
 return result;
}

//@Depressing
force_inline Line_Params
lp_alignment_threshold(v1 threshold)
{
 Line_Params result = global_painter.line_params;
 result.alignment_threshold = threshold;
 return result;
}

internal void
update_view_vector(Painter *p, v3 view_center)
{
 v3 camera_local = object_transform.inv * camera_world_position(p->camera);
 p->view_vector = noz(camera_local-view_center);
}

force_inline v4
big_to_small(Painter *p)
{
 v1 big   = 1.f;
 v1 small = 0.5f;
 return vec4(big, big, small, small);
}

force_inline v4
small_to_big(Painter *p)
{
 v1 big   = 1.f;
 v1 small = 0.5f;
 return vec4(small, small, big, big);
}

force_inline v4
big_endian(Painter *p)
{
 v1 big   = 1.f;
 v1 small = 0.5f;
 return vec4_symmetric(big, small);
}

force_inline void
duo_line(Painter *p, v3 a, v3 b, v3 c)
{
 draw(p, bez(a,b), small_to_big(p));
 draw(p, bez(b,c), big_to_small(p));
}
force_inline void
duo_line(Painter *p, v3 array[3])
{
 duo_line(p, array[0], array[1], array[2]);
}

struct v3_pair {
 union{v3 u; v3 a;};
 union{v3 v; v3 b;};
};
typedef v3_pair v6;

inline v3_pair
invent_uv(v3 n)
{
 v3 u = (almost_equal(absolute(n.x), 1.f, 1e-4) ?
         cross(n, vec3y(1)) :
         cross(n, vec3x(1)));
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
 return vec4(n1,d1);
}

internal void
push_object_transform(Painter *p, mat4i *transform)
{
 object_transform = *transform;
 object_to_camera = inverse(p->camera->transform) * object_transform;
 push_object_transform_to_target(p->target, &transform->m);
}
