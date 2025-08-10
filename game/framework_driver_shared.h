//-
// NOTE: Name,Denom
#define X_Pose_Fields(X) \
X(thead_phi2, 6) \
X(thead_theta2  , 6)  \
X(thead_roll , 6)  \
X(tblink     , 6)  \
X(teye_phi2, 6)  \
X(teye_theta2   , 6)  \
X(tarm_bend  , 18)  \
X(tarm_abduct, 36)  \

struct Pose
{
#define X(NAME,...)   v1 NAME;
 X_Pose_Fields(X);
#undef X
};

typedef v3 tvec;

#include "game_colors.cpp"
#include "game_debug.h"
#include "ad_file_formats.gen.h"
#include "framework_driver_shared.gen.h"
#include "4coder_kv_debug.h"
#include "meta_game_shared.h"
//-
myinline tdim mkdim(v1 x){ return {x}; }

myinline tvert mkvert(v1 x, v1 y, v1 z){ return {v3{x,y,z}}; }

// TODO(kv) This function can be auto-generated.
myinline tvert mkvert(v3 v){ return *(tvert*)&v; }

myinline tvert mkvert(){ return {}; }
myinline tvert mkvertx(v1 x){ tvert result = {}; result.x=x; return result; }
myinline tvert mkverty(v1 y){ tvert result = {}; result.y=y; return result;  }
myinline tvert mkvertz(v1 z){ tvert result = {}; result.z=z; return result; }

// TODO(kv) deprecate these, just use "V3"
myinline tvec mkvec(v1 x, v1 y, v1 z){ return v3{x,y,z}; }
myinline tvec mkvec(v3 v){ return tvec(v); }
myinline tvec mkvecx(v1 x){ return tvec{x,0,0}; }
myinline tvec mkvecy(v1 y){ return tvec{0,y,0}; }
myinline tvec mkvecz(v1 z){ return tvec{0,0,z}; }

// NOTE(kv) Having different types has *some* benefit...
myinline tvert
operator *(mat4 const&mat, tvert vert)
{
 return mkvert(mat4vert(mat, vert.v));
}
myinline tvert
operator +(tvert vert, tvec vec)
{
 return mkvert(vert.v + vec);
}
myinline void
operator +=(tvert &vert, v3 offset)
{
 vert.v += offset;
}
myinline tvert
operator -(tvert vert, tvec vec)
{
 return mkvert(vert.v - vec);
}
myinline tvec
operator -(tvert a, tvert b)
{// NOTE(kv) Delta vector between two vertices
 return mkvec(a.v - b.v);
}
myinline tvert
lerp(tvert a, v1 t, tvert b)
{// NOTE(kv) Lerp between vertices
 return mkvert(lerp(a.v, t, b.v));
}
myinline tvert
negateX(tvert vert)
{
 vert.x = -vert.x;
 return vert;
}
// NOTE(kv) You have to normalize it first, but ya know...
// trying to be flexible here...
myinline tnormal mk_normal(v3 v){ return {v}; };
global tnormal normal_x = {V3x(1.f)};
global tnormal normal_y = {V3y(1.f)};
global tnormal normal_z = {V3z(1.f)};

#define TypeInfoPointerList(X) \
X(v1)  X(v2)  X(v3)  X(v4) \
X(i1)  X(i2)  X(i3)  X(i4) \
X(FUI_Line_Params) X(tdim) X(tvert) X(tnormal) \

struct Type_Info_Pointers
{
#define X(T)      struct Type_Info *T;
 TypeInfoPointerList(X)
#undef X
};
//-
struct Viewport;
struct Modeler;
struct Render_Target;
struct Render_Config;

struct Poly_Flags{ u32 v; };
enum Poly_Flag
{
 Poly_Line     = 0x2,
 Poly_Overlay  = 0x4,
};

struct Fill_Flags{ u32 v; };
enum Fill_Flag
{
 Fill_Culled   = 0x1,
 Fill_Inverted = 0x2,
 Fill_Overlay  = 0x4,
};
myinline Poly_Flags
to_poly_flags(Fill_Flags flags)
{
 Poly_Flags result = {};
 if(flags.v & Fill_Overlay){ result.v |= Poly_Overlay; }
 return result;
}

struct Fill_Params
{
 // NOTE(kv) Singular color fills still makes sense for debugging/highlighting.
 argb color;
 Fill_Flags flags;
};
struct Line_Params
{
 v4 radii;
 // NOTE(kv) I want lightness changes to adapt to line color.
 // so we store the "x" in "rgb + x*rgb".
 v4 lightness_additions;
 Line_Flags flags;
 
 // NOTE(kv) Most of the time we can specify normal-alignment by group,
 // but sometimes we want to infer the view vector automatically from the curve.
 v1 alignment_min;
};
struct Paint_Params
{
 // NOTE(kv) "painting" is used f.ex for alignment checks,
 // it MUST NOT disable sending data, because cursor selection still needs them.
 b32 painting;
 
 Line_Params line;
 Fill_Params fill;
 
 v1 radius_mult;
 v1 nslice_per_meter;
 argb line_color;
 v1 line_depth_offset;
 v1 fill_depth_offset;
};

struct Bezier
{
 tvert e[4];
 myinline operator tvert *(){ return e; };
 myinline tvert &operator[](i32 index){ return e[index]; }
};
typedef Bezier Bez;

template<class TYPE>
function TYPE
bezier_sample(TYPE P[4], v1 t)
{
 v1 T = 1-t;
 return (1*cubed(T)      *P[0] +
         3*(t)*squared(T)*P[1] +
         3*squared(t)*(T)*P[2] +
         1*cubed(t)      *P[3]);
}
// NOTE(kv) By the brilliance of C++, we have to do crap like this.
myinline tvert
bezier_sample(tvert *bez, v1 u)
{
 v3 result = bezier_sample((v3 *)bez, u);
 return mkvert(result);
}
myinline v1
bezier_sample(v4 vec, v1 u)
{
 return bezier_sample(vec.e, u);
}

// NOTE: Actually bernstein basis
function v1
cubic_bernstein(i32 index, v1 t)
{
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
myinline Bezier
negateX(Bezier line)
{
 for_i32(i,0,4){ line[i].x = -line[i].x; }
 return line;
}
myinline Bezier
bez_negateX(Bezier line){ return negateX(line); }

function Bezier
mat4bez(mat4 const &mat, Bezier const &bez)
{
 Bezier result;
 for_i32(i, 0, 4)
 {
  result[i] = mat * bez.e[i];
 }
 return result;
}
function void
mat4bez(mat4 const &mat, Bezier *bez)
{
 *bez = mat4bez(mat, *bez);
}
myinline Bezier
operator *(mat4 const &mat, Bezier const &bez)
{
 return mat4bez(mat, bez);
}

struct Patch{
 tvert e[4][4];
 typedef tvert Array4x4[4][4];  // #stroustrup
 operator Array4x4&() { return e; }
};
function Bezier
get_column(Patch const&surface, i32 col)
{
 Bezier result;
 for_i32(index,0,4)
 {
  result[index] = surface.e[index][col];
 }
 return result;
}

//~ id system
// NOTE(kv): Entities are either drawn by code or data.
enum Prim_Type : u8{
 Prim_Null     = 0,
 Prim_Vertex   = 1,  //NOTE(kv) Idk if vertices are really entities? Maybe they're like vertex visualizer?
 Prim_Curve    = 2,
 Prim_Triangle = 3,
};

inline Prim_Type type_from_prim_id(u32 id){ return Prim_Type(id >> 24); }
inline b32 prim_id_is_data(u32 id){ return type_from_prim_id(id) != 0; }

/*struct Prim_XID{
 u32       id;
 Prim_Type type;
 i32       index;
};
inline u32
index_from_prim_id(u32 id){
 if(prim_id_is_data(id)){ return (id & 0xFFFF); }
 return 0;
}
inline Prim_XID
prim_xid_from_id(u32 id){
 Prim_XID result = {};
 result.id    = id;
 result.type  = type_from_prim_id(id);
 result.index = index_from_prim_id(id);
 return result;
}*/
//~
global char *global_debug_scope;
#define vertex_block(NAME) SetInBlock(global_debug_scope, NAME)

#define DEBUG_NAME(NAME, VALUE)  DEBUG_VALUE_inner(global_debug_scope, NAME, VALUE, 0)
#define DEBUG_NAME_COLOR(NAME, VALUE, COLOR)  DEBUG_VALUE_inner(global_debug_scope, NAME, VALUE, COLOR)
#define DEBUG_VALUE(VALUE)       DEBUG_VALUE_inner(global_debug_scope, #VALUE, VALUE)
#define DEBUG_TEXT(TEXT)         DEBUG_VALUE_inner(global_debug_scope, TEXT, 0.f)
//~

struct Camera
{
 union {
  mat4i world_from_camera;
  // union
  struct {
   mat4 world_from_cam;   // NOTE: 3x3 Columns are camera axes
   union {
    mat4 cam_from_world;  // NOTE: 3x3 Rows are camera axes
    struct {
     union {v4 x_; v3 x;};
     union {v4 y_; v3 y;};
     union {v4 z_; v3 z;};
    };
   };
  };
 };
 
 // NOTE(kv) Really don't care about these...
 // This whole struct doesn't need to exist.
 v1 focal_length;
 v1 near_clip;
 v1 far_clip;
};
myinline v3
get_world_pos(Camera const&cam)
{
 v3 pos = get_column(cam.world_from_cam, 3).xyz;
 return pos;
}
//~;game_config
global i32 bezier_poly_nslice = 16;

myinline bool
operator==(Bone_ID a, Bone_ID b)
{
 return (a.type==b.type) && (a.id==b.id);
}

myinline Bone_ID mk_bone_id(Bone_ID id) { return id; }
myinline Bone_ID mk_bone_id(Bone_Type type, i32 id=0){ return Bone_ID{type, id}; }

struct Bone
{
 Bone_ID id;
 mat4i   world_from_bone;
 // TODO(kv) What if we put this in the id, it'd make bone id actual ID.
 b32     is_right;
 v3      center;
};

struct Range_i16
{
 union{ i16 min,begin; };
 union{ i16 max,end; };
};
myinline bool
operator==(Range_i16 a, Range_i16 b)
{
 return block_match(&a, &b, sizeof(a));
}
struct FUI_File
{
 i16 is_driver;
 i16 index;
};
struct Location
{
 FUI_File file;
 Range_i16 range;
};
// NOTE(kv) Take care of padding, so we can compare values with block comparison.
static_assert(sizeof(Location) == 8);

myinline b32
is_valid(Location location)
{
 return location.file.index != 0;
}

// NOTE(kv) The reason why we have @Unresolved_Location,
// is because we don't know that the marker indexes are
// when we have to emit @set_draw_location_unresolved.
struct Unresolved_Location
{
 i16 file;
 i16 text_object_index_in_file;
};
myinline b32
is_valid(Unresolved_Location a)
{
 return a.file != 0;
}

myinline bool
operator==(Location a, Location b)
{
 return block_match(&a, &b, sizeof(a));
}
struct Vertex
{
 i32 ninfo_index;
 Bone_ID bone_id;
 v3 pos;
};
typedef Static_Array2<Vertex> Vertices;

struct Poly3
{
 v3 points[3];
 myinline operator v3 *() { return points;}
};
myinline Poly3
mk_poly3(v3 points[3])
{
 return Poly3{expand3(points)};
}
struct Dual_Bezier
{
 Bezier P;
 Bezier Q;
};
enum Primitive_Type
{
 Primitive_Type_None,
 Primitive_Type_Curve,
 Primitive_Type_Poly3,
 Primitive_Type_Dual_Bezier,
};
struct Recorded_Primitive
{
 Primitive_Type type;
 Location location;
 Bone_ID bone_id;
 
 union
 {
  Bezier curve;
  Poly3  poly3;
  Dual_Bezier *dual_bezier;
 };
};
struct Model_Persistent
{
 darray(Vertex) vertices;
};
struct Model
{
 // NOTE(kv) I tried getting "is_right" from the bone,
 // but we don't know what "is_right" would be at the start -> that's bad!
 b32 is_right;
 
 b32 primitives_are_in_camera_space;
 Arena *frame_arena;
 
 darray(Bone) bones;
 darray(Bone *) bone_stack;
 
 darray(Recorded_Primitive) primitives;
 darray(Vertex) vertices;
 
 Model_Persistent persistent;
};
struct Viewport
{// NOTE For init code, view @game_init
 i32 index;  // NOTE(kv) Redundant data
 Camera_Data camera;  // NOTE(kv) Current camera, as opposed to the target camera, which is serialized.
 Arena render_arena;
 v1 previous_phi_snap;
 v1 current_phi_snap;
 
 union
 {
  Saved_Viewport_Embed;
  Saved_Viewport saved;
 };
};

myinline b32 is_main_viewport(Viewport *viewport){ return viewport->index==0; }

struct Painter
{// NOTE(kv) This is a convenient global store.
 // IMPORTANT See @init_painter and @init_painter_2
 v1 looping_time;
 v1 anim_time;
 
 sarray(Location) hot_locations;
 Location current_draw_location;
 b32 current_location_is_hot;
 
 Camera camera;
 mat4  clip_from_world;
 mat4  clip_from_bone; // see @set_bone_transform
 mat4i cam_from_bone;  // see @set_bone_transform
 
 i32 view_vector_count;
 v3  view_vector_stack[16];
 
 //-Debug collection
 i32 clipped_curve_count;
 i32 total_curve_count;
 
 Paint_Params params;
 
 //-misc
 b32 shading_on;
 b32 previous_draw_result;  // view @draw_bezier
 Render_Target *target;
 v1 profile_score;  // TODO: @Cleanup axe this?
 i32 viz_level;
 b32 ignore_radii;
 b32 ignore_alignment_min;
 Viewport *viewport;
 b32 show_grid;
 argb shade_color;  // NOTE(kv) Useful enough to keep I guess.
 b32 sending_data;
 b32 references_full_alpha;
 argb background_color;
 u32 render_cycles;
};

global Painter *painter;  // see @init_painter
global Model *the_model;  // see @clear_model

//myinline u32 get_hot_prim_id(){ return painter->hot_prim_id; }
myinline Line_Params
get_line_params(){
 return painter->params.line;
}
myinline Fill_Params
get_fill_params(argb color=0){
 Fill_Params result = painter->params.fill;
 if(color){result.color = color;}
 return result;
}
myinline Line_Params
get_line_params(v4 radii)
{
 Line_Params result = get_line_params();
 result.radii = radii;
 return result;
}
myinline Line_Params
get_line_params(i4 radii)
{
 Line_Params result = get_line_params();
 result.radii = i2f6(radii);
 return result;
}
myinline Line_Params
lp_invisible()
{
 Line_Params result = get_line_params();
 result.flags |= Line_Invisible;
 return result;
}

global argb hot_color  = argb_lightness(linear_argb_red, 0.75f);
global argb hot_color2 = linear_argb_yellow;
//global argb selected_color = argb_red;
global v1 default_line_radius_unit = 1.728125f * millimeter;

//-
function Bone *
current_bone()
{
 Bone *result = the_model->bone_stack.items[the_model->bone_stack.count - 1];
 return result;
}
myinline mat4i &
current_world_from_bone()
{
 Bone *bone = current_bone();
 return bone->world_from_bone;
}

myinline b32 is_right(){ return the_model->is_right; }
myinline b32 is_left(){ return not is_right(); }
//-
//~

//-See @driver_update_tweaks
struct Tweak_Variables
{
 // For @draw_bezier_inner
 b32 force_stamp_rendering;  // NOTE(kv) Just turn this on for fun
 b32 ignore_lightness_additions;
 v1 stamp_density_mult;
 v1 stamp_rotation_speed;
 b32 cull_curve;
 
 //-Misc
 v1 focal_length;
 v3 background_rgb;
};
global Tweak_Variables *tweaks;

struct Framework_API
{
 b32 valid;
#define X(N) wrap_function_pointer(N);
 framework_api_xlist(X)
#undef X
 
 Tweak_Variables *tweaks;
 Type_Info_Pointers types;
};
//-

struct Slider;
typedef sarray(Slider) Sliders;

struct Bez_v2
{
 v2 e[3];
 myinline operator v2 *(){ return e; }
};
enum Image_Marker_Type
{
 Image_Marker_None,
 Image_Marker_Point,
 Image_Marker_Bezier,
};
struct Image_Marker
{
 Image_Marker_Type type;
 union{
  v2 point;
  Bez_v2 bezier;  // NOTE(kv) Being real economical here...
 };
};
myinline Image_Marker
mk_image_marker(v2 point){
 return {.type=Image_Marker_Point, .point=point};
}
myinline Image_Marker
mk_image_marker(v2 bezier[3])
{
 Image_Marker result = {.type=Image_Marker_Bezier};
 for_i32(i, 0, 3)
 {
  result.bezier[i] = bezier[i];
 }
 return result;
}
struct Image_Info
{
 Stringz filename;
 Image_Marker marker;
};
struct Text_Object
{
 Range_i16 location;
 Text_Object_Kind kind;
 
 union {
  Image_Info image;
  // or
  Reference_Preset preset;
 };
};
struct Vertex_Info
{
 Location location;
 i32 indicator_level;
 b32 overlay;
};
struct Driver_DLL
{
 DLL_Handle handle;
 u64 mtime;
};

typedef Range_i16 Marker_Pair;

struct FUI_File_Data
{
 i32 index;
 String name;
 
 sarray(i32) marked_positions;
 
 sarray(Text_Object) text_objects;
 sarray(Slider) sliders;
 sarray(Vertex_Info) vertices_info;
};

struct Driver_Data
{// NOTE See @driver_dll_entry and @do_work_after_loading_driver
 b32 valid;
 sarray(FUI_File_Data) files;
 sarray(Vertex_Info) vertices_info;
};

global Driver_Data driver_data;  // see @driver_dll_entry

struct Driver_API
{// NOTE see @driver_dll_entry
 Driver_DLL dll;
 
#define X(N) wrap_function_pointer(N);
 driver_api_xlist(X)
#undef X
 
 Driver_Data *data;
};
myinline b32
is_valid(Driver_API *driver)
{
 return driver->dll.handle != 0;
}

//~
#if AD_IS_DRIVER
#define X(N)  global wrap_function_pointer(N);
framework_api_xlist(X)
#undef X
#endif

#if AD_IS_FRAMEWORK
#define X(N)  function wrap_function(N);
framework_api_xlist_1(X)
#undef X
#endif
//-
function void
set_bone_transform(mat4i const&world_from_bone)
{
 Painter *p = painter;
 p->cam_from_bone  = invert(p->camera.world_from_camera) * world_from_bone;
 p->clip_from_bone = p->clip_from_world * world_from_bone.m;
 push_object_transform_to_target(p->target, cast(mat4*)&world_from_bone.m);
}

function tvert
camera_world_position(Camera const &camera)
{// NOTE(kv) Don't even think about trying to get ahead by fetching the last row
 // of "cam_from_world", the 4x4 transform isn't invertible via transpose!
 tvert result = mkvert(get_column(camera.world_from_cam, 3).xyz);
 return result;
}
function v3
camera_object_position()
{
 v3 result = (current_world_from_bone().inv *
              camera_world_position(painter->camera));
 return result;
}
function void
push_view_vector(v3 object_center)
{
 Painter *p = painter;
 v3 camera_obj = camera_object_position();
 v3 view_vector = noz(camera_obj - object_center);
 p->view_vector_stack[p->view_vector_count++] = view_vector;
 kv_assert(p->view_vector_count < alen(p->view_vector_stack));
}
myinline v3
get_view_vector()
{
 return painter->view_vector_stack[painter->view_vector_count-1];
}
myinline void
pop_view_vector()
{
 Painter *p = painter;
 p->view_vector_count--;
 kv_assert(p->view_vector_count > 0);
}
#define ViewCenterBlock(center) \
push_view_vector(center); \
defer(pop_view_vector());

function Bone *
get_bone(Bone_ID id, b32 is_right_var=is_right())
{
 sarray(Bone) &bones = the_model->bones;
 for_i32(index, 0, bones.count)
 {
  Bone *bone = &bones.items[index];
  if(bone->id       == id and
     bone->is_right == is_right_var)
  {
   return bone;
  }
 }
 return &bones.items[0];
}
function Bone *
make_bone(Bone_ID id, mat4i const&mom_from_kid)
{
 sarray(Bone *) stack = the_model->bone_stack;
 mat4i &mom = stack[stack.count-1]->world_from_bone;
 Bone *bone = push(&the_model->bones);
 bone->id       = id;
 bone->is_right = is_right();
 bone->world_from_bone = matmul(mom, mom_from_kid);
 return bone;
}
function void
push_bone_painter(Bone_ID id)
{
 Bone *bone = get_bone(id);
 push(&the_model->bone_stack, bone);
 set_bone_transform(bone->world_from_bone);
}
myinline void
push_bone_painter(Bone_Type bone_type)
{
 push_bone_painter(mk_bone_id(bone_type));
}
function void
pop_bone_painter()
{
 kv_assert(the_model->bone_stack.count > 0);
 the_model->bone_stack.count--;
 mat4i &parent = current_world_from_bone();
 set_bone_transform(parent);
}
#define BoneBlock(id)  push_bone_painter(id); defer(pop_bone_painter();)
#define BoneBlockApplied(from)    BoneBlock(from)
//-

#include "4coder_debug_value.h"
#include "game_draw.cpp"
//-