//-
// NOTE(kv) Shorthands, helper functions for the scripts.
//-
#if 0
//#define fvec(value) fval( (value), {.flags=Slider_Vector} )
//#define fdir fvec  // NOTE: Not influenced by scale.
//#define funit(value)      fvec(value, Slider_Camera_Aligned|Slider_NOZ)
//#define fhsv(h,s,v) argb_pack(srgb_to_linear(hsv_to_srgb(fval3(h,s,v))))
#define draw_image draw_image_func
#endif
#define fimage(STRING, ...) STRING

// TODO(kv) Just save/restore the whole params structure,
// then we wouldn't have to do these ad-hoc macros anymore.
function void
scale_line_radius(v1 multiplier)
{
 painter->params.radius_mult *= multiplier;
}
function void
set_line_color_lightness(v1 lightness)
{
 argb &color = painter->params.line_color;
 color = argb_lightness(color, lightness);
}

#define clampx Slider_Clamp_X
#define clampy Slider_Clamp_Y
#define clampz Slider_Clamp_Z

typedef v3_pair v6;
//

#define WARN_DELTA(a, b, EPSILON) \
if (absolute(a-b) > EPSILON) { DEBUG_NAME("WARN " #a "-" #b, (a-b)); }

struct Eye_Min_Distance_Result {
 v1 min_distance;
 v3 closest_point;
};
function Eye_Min_Distance_Result
get_eye_min_distance(v3 center, v1 radius, Bezier line)
{
 Eye_Min_Distance_Result result;
 v1 min_lensq = 10000.f;
 i32 sample_count = 8;
 for_i32(index,0,sample_count+1)
 {
  v1 t = v1(index) * (1.f / v1(sample_count));
  v3 sample = bezier_sample(line, t);
  v1 lengthsq = length_squared(sample - center);
  if ( lengthsq < min_lensq )
  {
   result.closest_point = sample;
   min_lensq = lengthsq;
  }
 }
 result.min_distance = square_root(min_lensq) - radius;
 return result;
}

global v1 const cos_45_degree = 1.f / square_root(2.f);

function b32
aligned(v3 normal, v1 min_alignment=cos_45_degree)
{
 v1 alignment = dot(normal, get_view_vector());
 return alignment > min_alignment;
}
function b32
aligned_symmetric(v3 normal, v1 min_alignment=cos_45_degree)
{
 v3 view_vector = get_view_vector();
 v1 alignment = absolute(dot(normal, view_vector));
 return alignment > min_alignment;
}
myinline b32
front_back_aligned(v1 min_alignment=cos_45_degree)
{
 return aligned_symmetric(V3z(1.f), min_alignment);
}
myinline b32
profile_aligned(v1 min_alignment=cos_45_degree)
{
 return aligned_symmetric(V3x(1.f), min_alignment);
}
myinline Line_Params
lp_alignment_min(v1 min)
{
 Line_Params result = get_line_params();
 result.alignment_min = min;
 return result;
}
function void
debug_view_vector(i32 location)
{
 v3 camera_obj = camera_object_position();
 v3 view_vector = get_view_vector();
 v3 object_center = camera_obj + view_vector;
 //indicate_vertex(object_center, 0);
 DEBUG_VALUE(view_vector);
}

function v4
plane_transform(mat4 const&mat, v3 n, v1 d)
{
 v3 p0 = -d*n;
 v3_pair xy = invent_xy(n);
 v3 n1 = noz( cross(mat4vec(mat,xy.u), mat4vec(mat,xy.v)) );
 v1 d1 = -dot(n1, mat4vert(mat, p0));
 return V4(n1,d1);
}
//~
function mat4i &
mom_bone_xform()
{
 darray(Bone *) &stack = the_model->bone_stack;
 kv_assert(stack.count >= 2);
 return stack.items[stack.count-2]->world_from_bone;
}

function mat4
from_parent()
{
 //NOTE(kv) If we just stored the relative offset, we wouldn't need this.
 mat4 &mom_world_from_bone = mom_bone_xform().forward;
 mat4 &bone_from_world = current_world_from_bone().inverse;
 return matmul(bone_from_world, mom_world_from_bone);
}

//~

function v3
bezier_deriv_div3(Bezier bezier, v1 t)
{//NOTE: The real derivative is multiplied by 3, for some reason
 v3 result = {};
 for_i32(index,0,3)
 {
  result += quad_bernstein(index, t) * (bezier[index+1] - bezier[index]);
 }
 return result;
}

function v1
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
function v3
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

myinline Fui_Options
fscale(v1 delta_scale)
{
 Fui_Options result = {};
 result.delta_scale = delta_scale;
 return result;
}
myinline Bezier
reverse(Bezier B){
 return {B[3], B[2], B[1], B[0]};
}
myinline v3
reflect_origin(v3 origin, v3 point){
 return origin-(point-origin);
}
myinline i1
get_preset()
{
 return painter->viewport->preset;
}
myinline Reference_Preset
get_reference_preset()
{
 return painter->viewport->reference_preset;
}
myinline v3 get_camz() { return painter->camera.z; }

function b32 camera_is_right() {
 return(almost_equal(get_camz().x, -1.f, 1e-2f));
}
//
function b32 camera_is_left() {
 return(almost_equal(get_camz().x, +1.f, 1e-2f));
}
//
function b32 camera_is_front() {
 v3 camz = get_camz();
 return(almost_equal(camz.z, +1.f, 1e-2f));
}
//
function b32 camera_is_back() {
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
get_vline(Patch const&pat, v1 v)
{
 Bezier result;
 for_i32(index,0,4)
 {
  Bezier *b = (Bezier *)pat.e[index];
  result[index] = bezier_sample(*b, v);
 }
 return result;
}

// IMPORTANT: this is the key to skeletal transformation
function mat4i
trs_pivot_transform(v3 translate, mat4i const&rotate, v1 scale,
                    v3 object_space_pivot)
{
 // NOTE(kv) The key to understanding this is that the first translation
 // will make the pivot zero, which means scale and rotate will work as expected,
 // then you translate back.
 // NOTE(kv) A bad consequence of defining the pivot in "object space"
 // is that "object_space_pivot" cannot be visualized,
 // because you don't have the transform yet.
 // But I can't think of any other scheme that allows that, either
 // (unless you wanna complicate things).
 mat4i result = (mat4i_translate(translate + object_space_pivot)
                 * rotate
                 * mat4i_scale(scale)
                 * mat4i_translate(-object_space_pivot));
 return result;
}

function Paint_Params
mk_highlight_params(argb color=0)
{
 if(color == 0){
  //color = srgb_to_linear(0XFFDBA50F);
  color = argb_yellow;
 }
 Paint_Params cparams = painter->params;
 cparams.line.flags |= Line_Overlay|Line_No_SymX;
 cparams.line_color = color;
 cparams.fill.flags.v |= Fill_Overlay;
 cparams.fill.color = color;
 return cparams;
}
myinline void
push_hl(argb color=0)
{
 painter->params = mk_highlight_params(color);
}

myinline mat4i &
get_world_from_bone(Bone_ID id, i32 lr_index=is_right())
{
 return get_bone(id, lr_index)->world_from_bone;
}
myinline mat4i &
get_world_from_bone(Bone_Type type, i32 lr_index=is_right())
{
 return get_world_from_bone(mk_bone_id(type), lr_index);
}
#define rebase(o, e) e

// TODO(kv) Fold this into paint params block, too!
#define ShowIf(condition) \
SetInBlock(painter->params.painting, painter->params.painting and condition)

#define ShowIf2(condition) \
SetInBlock(painter->params.painting, condition)

#define lp get_line_params
#define fp get_fill_params

function Line_Params
line_params_from_fui(FUI_Line_Params src)
{
 Line_Params result = get_line_params();
 if(src.radii != v4{})
 {
  result.radii = src.radii;
 }
 if(src.lightness_additions != v4{})
 {
  result.lightness_additions = src.lightness_additions;
 }
 
 // TODO(kv) IDK what to do about flags...
 // should we say that every flag will have its own... flag?
 // I really don't know and don't care rn.
 result.flags |= src.line_flags;
 
 return result;
}
function void
draw_reference_image_from_data(Reference_Image ref)
{
 v1 alpha = ref.alpha;
 if(painter->references_full_alpha) { alpha = 1.0f; }
 draw_image(ref.filename, ref.center, ref.x_axis, V3y(1.f), alpha);
}

//~ EOF