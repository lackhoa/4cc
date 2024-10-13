internal v3
fvert_function(v3 init_value, Fui_Options opts, i32 line=__builtin_LINE()){
 opts = fopts_add_flags(opts, Slider_Vertex);
 v3 result = fval(init_value, opts, line);
 return result;
}

#define fvert(value, ...)   fvert_function( (value), fopts(__VA_ARGS__) )

//-NOTE: Annotations
#define fvertx(value)    V3x(fval(value))
#define fverty(value)    V3y(fval(value))
#define fvertz(value)    V3z(fval(value))

#define fvec(value, ...) \
fval( (value), fopts_add_flags(fopts(__VA_ARGS__), Slider_Vector) )
#define fvecx(value)      V3x(fval(value))
#define fvecy(value)      V3y(fval(value))
#define fvecz(value)      V3z(fval(value))

#define fdir fvec  // NOTE: Not influenced by scale.
//- End annotation
#define fvert3(x,y,z,...) fvert(V3(x,y,z), __VA_ARGS__)
#define funit(value)      fvert(value, Slider_Camera_Aligned|Slider_NOZ)

#define fkeyframe(nframes, value)  add_keyframe(ani, fval2i(nframes, value))
#define fhsv(h,s,v) argb_pack(srgb_to_linear(hsv_to_srgb(fval3(h,s,v))))

#define radius_scale_block(multiplier) \
Common_Line_Params line_unique_var = current_line_cparams(); \
line_unique_var.radius_mult *= multiplier; \
push_line_cparams(line_unique_var); \
defer(pop_line_cparams());

#define line_color_lightness(scale) \
Common_Line_Params line_unique_var = current_line_cparams(); \
line_unique_var.color = argb_lightness(line_unique_var.color, scale); \
push_line_cparams(line_unique_var); \
defer(pop_line_cparams());

#define lp_block(FIELD, value) \
Common_Line_Params line_unique_var = current_line_cparams(); \
line_unique_var.FIELD = value; \
push_line_cparams(line_unique_var); \
defer(pop_line_cparams());

#define indicate(vertex,...)  indicate_vertex(#vertex, vertex, __VA_ARGS__)
#define indicate0(vertex,...) indicate(vertex,0,__VA_ARGS__)

global_const Slider clampx = Slider_Clamp_X;
global_const Slider clampy = Slider_Clamp_Y;
global_const Slider clampz = Slider_Clamp_Z;

inline void
circular_arc_helper(mat4 const&transform, v3 dst[4], v2 src[4]){
 for_i32(index,0,4){
  dst[index] = mat4vert(transform, V3(src[index].x, src[index].y, 0.f));
 }
}
function void
draw_bezier_circle(mat4 const&transform, v4 radii={}){
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
  draw(bez_raw(arc1), params);
  draw(bez_raw(arc2), params);
  draw(bez_raw(arc3), params);
  draw(bez_raw(arc4), params);
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
function v3
perspective_project_non_hyperbolic(Camera *camera, v3 worldP){
 v3 result = (camera->cam_from_world * V4(worldP, 1.f)).xyz;
 v1 depth = camera->distance-result.z;
 result.xy *= (camera->focal_length / depth);
 result.z   = depth;
 return result;
}
force_inline Line_Params
lp_alignment_min(v1 min){
 Line_Params result = painter.line_params;
 result.alignment_min = min;
 return result;
}
function void
debug_view_vector(i1 linum=__builtin_LINE()){
 v3 camera_obj = camera_object_position(&painter);
 v3 view_vector = get_view_vector();
 v3 object_center = camera_obj + view_vector;
 indicate_vertex("view_center", object_center, 0, linum);
 DEBUG_VALUE(view_vector);
}

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

function v4
plane_transform(mat4 const&mat, v3 n, v1 d)
{
 v3 p0 = -d*n;
 v3_pair uv = invent_uv(n);
 v3 n1 = noz( cross(mat4vec(mat,uv.u), mat4vec(mat,uv.v)) );
 v1 d1 = -dot(n1, mat4vert(mat, p0));
 return V4(n1,d1);
}

//~

function mat4i &
mom_bone_xform(Painter *p){
 auto &stack = p->bone_stack;
 return stack[stack.count-2]->xform;
}
inline mat4i& p_mom_bone_xform(){ return mom_bone_xform(&painter); }
function mat4
from_parent(){
 //NOTE(kv) If we just stored the relative offset, we wouldn't need this.
 auto p = &painter;
 return current_world_from_bone(*p).inverse * mom_bone_xform(p);
}

//~

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
fscale(v1 delta_scale){
 Fui_Options result = {};
 result.delta_scale = delta_scale;
 return result;
}
force_inline Bezier
reverse(Bezier B){
 return {B[3], B[2], B[1], B[0]};
}
force_inline v3
reflect_origin(v3 origin, v3 point){
 return origin-(point-origin);
}
inline i1
get_preset(){
 return painter.viewport->preset;
}
inline v3 get_camz() { return painter.camera.z; }
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

#define test_speed_block(NAME, ITERATIONS, CODE) \
u64 cy_begin = ad_rdtsc(); \
for_i32(it,0,ITERATIONS) { CODE } \
u64 cy_end   = ad_rdtsc(); \
v1 NAME = v1(f64(cy_end-cy_begin) / f64(ITERATIONS)); \
DEBUG_VALUE(NAME) ;

function Bezier
get_uline(Patch const&pat, v1 u) {
 Bezier result;
 for_i32(index,0,4) {
  result[index] = bezier_sample(get_column(pat, index), u);
 }
 return result;
}

function Bezier
get_vline(Patch const&pat, v1 v) {
 Bezier result;
 for_i32(index,0,4)
 {
  result[index] = bezier_sample(pat.e[index], v);
 }
 return result;
}

// IMPORTANT: this is the key to skeletal transformation
function mat4i
trs_pivot_transform(v3 translate, mat4i const&rotate, v1 scale,
                    v3 object_space_pivot)
{
 // NOTE: The key to making this work is that the first translation
 // will make the pivot zero, which means scale and rotate will work as expected,
 // then you translate back
 // Finally tack on to that whatever translation the user supplied.
 mat4i result = (mat4i_translate(translate + object_space_pivot)
                 * rotate
                 * mat4i_scale(scale)
                 * mat4i_translate(-object_space_pivot));
 return result;
}
inline mat4i& p_current_world_from_bone(){
 return current_world_from_bone(painter); }

inline void
push_hl(argb color=0, i1 linum=__builtin_LINE()){
 if(color == 0){color = srgb_to_linear(0XFFDBA50F);}
 Common_Line_Params cparams = current_line_cparams();
 cparams.flags |= Line_Overlay|Line_No_SymX;
 cparams.color = color;
 cparams.radii = i2f6(I4(3,3,3,3));
 push_line_cparams(cparams, linum);
}
inline void pop_hl(){ pop_line_cparams(); }

inline Line_Params
profile_visible(v1 min, i1 linum=__builtin_LINE()){
 Line_Params result = painter.line_params;
 result.unit_normal = V3x(1.0f);
 result.alignment_min = min;
 return result;
}
inline Line_Params
profile_visible_transition(v1 min){
 return profile_visible(cosine(min*0.25f));
}
//~ EOF;