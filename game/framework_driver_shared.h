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
#define Game_Preset_Count 10  // presets seeded on first run; digit keys select presets 0-9 (game_main.cpp key handler)
#define PRESET_CAP 32         // fixed storage cap for the preset rows (panel enforces it)
#define PRESET_NAME_CAP 32    // Preset_Settings.name, in framework_driver_shared.kh
#define DOCUMENT_NAME_CAP 64  // a document's file name without ".ad" (game_document_file.cpp)
// NOTE(kv) Recorded_Primitive.vertex_index cap (ad_file_formats.kh); see primitive_vertex_count.
global i32 const recorded_vertex_cap = 4;
// NOTE(kv) Reference_Scene_Data.mesh_layers cap (framework_driver_shared.kh): the five head layers.
global i32 const reference_mesh_layer_cap = 8;  // NOTE(kv) Scene_Head_Layers uses 6 since the jaw split (plan-reference-landmarks)
// NOTE(kv) plan-reference-landmarks: labeled mesh-space points stored beside each reference
// .obj (Reference_Landmark_File in framework_driver_shared.kh).
#define REFERENCE_LANDMARK_NAME_CAP 32
#define REFERENCE_LANDMARK_CAP 32
struct Reference_Mesh_Triangles
{// NOTE(kv) A reference mesh's triangles for game-side picking (landmark placement), in
 // .obj units. Borrowed from the driver's mesh cache: use within the frame, never keep
 // (the cache is rebuilt on driver reload).
 v3  *vertices;
 i32  vertex_count;
 i32 *indices;      // 3 per triangle, 0-based
 i32  index_count;
 b32  ok;
};
// NOTE(kv) A b32 flag of Preset_Settings, by pointer-to-member (Reference_Mesh_Layer.show_flag
// in the .kh). Typedef'd here because klang doesn't parse `b32 Preset_Settings::*`; a
// pointer-to-member of a not-yet-defined class is fine, the class comes from the gen.h.
struct Preset_Settings;
typedef b32 Preset_Settings::*Preset_Flag;
// NOTE(kv) Order matters: ad_file_formats.kh holds the recorded-drawing structs, which
// use tvert/Bezier/Bone_ID/Location from framework_driver_shared.kh.
#include "framework_driver_shared.gen.h"
#include "ad_file_formats.gen.h"
#include "4coder_kv_debug.h"
#include "meta_game_shared.h"
//-
myinline b32
scene_has_mesh(Reference_Scene_Data &data)
{// NOTE(kv) A scene carries reference meshes iff it lists at least one layer.
 return data.mesh_layer_count > 0;
}
myinline tdim mkdim(v1 x){ return {x}; }

myinline tvert mkvert(v1 x, v1 y, v1 z){ return {v3{x,y,z}}; }

// TODO(kv) This function can be auto-generated.
myinline tvert mkvert(v3 v){ tvert result = {}; result.v = v; return result; }

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
// NOTE bone_id propagation: vert-with-offset keeps the vert's bone; a blend of
// two verts on DIFFERENT bones drops to Bone_None ("use the group's bone").
myinline tvert
operator *(mat4 const&mat, tvert vert)
{
 vert.v = mat4vert(mat, vert.v);
 return vert;
}
myinline tvert
operator +(tvert vert, tvec vec)
{
 vert.v += vec;
 return vert;
}
myinline void
operator +=(tvert &vert, v3 offset)
{
 vert.v += offset;
}
myinline tvert
operator -(tvert vert, tvec vec)
{
 vert.v -= vec;
 return vert;
}
myinline tvec
operator -(tvert a, tvert b)
{// NOTE(kv) Delta vector between two vertices
 return mkvec(a.v - b.v);
}
myinline tvert
lerp(tvert a, v1 t, tvert b)
{// NOTE(kv) Lerp between vertices
 tvert result = mkvert(lerp(a.v, t, b.v));
 if(a.bone_id.type == b.bone_id.type && a.bone_id.id == b.bone_id.id)
 {
  result.bone_id = a.bone_id;
 }
 return result;
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
X(FUI_Line_Params) X(Curve) X(Reference_Placement) X(Reference_Mesh_Placement) X(tdim) X(tvert) X(tnormal) \

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

// NOTE(kv) Fill_Flags{u32 v} is an [info] struct in framework_driver_shared.kh.
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

// NOTE(kv) Fill_Params / Line_Params / Paint_Params are [info] structs in
// framework_driver_shared.kh (moved 2026-09-12).
// NOTE(kv) Field table for Paint_Params: X(enum_suffix, member_path).
// Single source for the PaintField_* bits and paint_params_diff_mask
// (game_draw.cpp) -- extend HERE when Paint_Params grows.
#define PaintFieldList(X) \
X(painting,          painting)                 \
X(line_radii,        line.radii)               \
X(line_lightness,    line.lightness_additions) \
X(line_flags,        line.flags)               \
X(alignment_min,     line.alignment_min)       \
X(fill_color,        fill.color)               \
X(fill_flags,        fill.flags)               \
X(radius_mult,       radius_mult)              \
X(nslice_per_meter,  nslice_per_meter)         \
X(line_color,        line_color)               \
X(line_depth_offset, line_depth_offset)        \
X(fill_depth_offset, fill_depth_offset)        \

// NOTE(kv) Bezier{tvert e[4]} is an [info] struct in framework_driver_shared.kh.
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
{// NOTE sampled point is a blend -> bone_id stays Bone_None (group default)
 v3 P[4] = {bez[0].v, bez[1].v, bez[2].v, bez[3].v};
 v3 result = bezier_sample(P, u);
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

// NOTE(kv) Patch{tvert e[4][4]} is an [info] struct in framework_driver_shared.kh.
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
myinline tvert
tag_bone(Bone_ID bone_id, tvert vert)
{// NOTE Bone funnel for inline conversions (e.g. arm_local*fvert): the point keeps
 // its converted coords but remembers which bone it rides.
 vert.bone_id = bone_id;
 return vert;
}

struct Bone
{
 Bone_ID id;
 mat4i   world_from_bone;
 // TODO(kv) What if we put this in the id, it'd make bone id actual ID.
 b32     is_right;
 v3      center;
};

// NOTE(kv) Range_i16, FUI_File, Location are [info] structs in framework_driver_shared.kh.
myinline bool
operator==(Range_i16 a, Range_i16 b)
{
 return block_match(&a, &b, sizeof(a));
}
// NOTE(kv) Take care of padding, so we can compare values with block comparison.
static_assert(sizeof(Location) == 8);

myinline b32
is_valid(Location location)
{
 return location.file.index != 0;
}
// NOTE(kv) Document variant (plan-document-mouse-editing Q1): a replayed document
// primitive has no code range, so `file.is_driver` carries this sentinel and
// `range.min` the primitive's index in recordings.document, `range.max` the mirror
// side (Q94: the document is replayed left+right on different bones, and editing
// needs to know which one was grabbed). Same 8 bytes, same block compare -> the
// hot/active/handle plumbing works unchanged; only the code-jump sites must check for
// it (resolve_location would index driver_data.files with -1).
enum{ Location_File_Document = -1 };
myinline Location
document_location(i32 primitive_index, b32 is_right)
{
 return {{i16(Location_File_Document), 1}, {i16(primitive_index), i16(is_right ? 1 : 0)}};
}
myinline b32
document_location_is_right(Location location)
{
 return location.range.max != 0;
}
myinline b32
is_document_location(Location location)
{
 return location.file.is_driver == Location_File_Document;
}
myinline i32
document_primitive_index(Location location)
{
 return location.range.min;
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
// NOTE(kv) The recorded-drawing structs (Weight_Key, Dual_Bezier, Recorded_*, Disk,
// Primitive_Type, Group_Vis, Group_Cam_Vis, Document_File) are [info] types in
// ad_file_formats.kh (moved 2026-09-12).

// NOTE(kv) Which Paint_Params fields a group overrides vs its parent (the "delta" view).
// The group's "params" is always the FULL effective state; the mask is metadata.
// Generated from PaintFieldList (next to Paint_Params).
enum
{
#define X(name, path) PaintFieldIndex_##name,
 PaintFieldList(X)
#undef X
};
enum
{
#define X(name, path) PaintField_##name = (1 << PaintFieldIndex_##name),
 PaintFieldList(X)
#undef X
};
struct Model_Persistent
{
 darray(Vertex) vertices;
};
struct Group_Scope_Slot
{
 i32 group_index;
 // NOTE(kv) Tag lives on the SCOPE, not just the group row: a mid-scope param
 // mutation sibling-splits the group (current_recorded_group_index), and the sibling
 // must keep the tag.
 Group_Vis vis_tag;
 Group_Cam_Vis cam_vis;
 b32 one_sided;
};
struct Group_Scope_Stack
{// NOTE(kv) One slot per open Paint_Params_Block scope, holding that scope's group
 // index. Eager: every scope gets a group at entry (slot 0 = the root group), so
 // slots always hold valid indices. Operations live in game_draw.cpp.
 darray(Group_Scope_Slot) slots;
};
// NOTE(kv) Preset_Settings itself is an [info] struct in framework_driver_shared.kh
// (reflected: the text state file, the panel checkboxes and the channel `toggle` walk
// its Type_Info). The caps live above the .gen.h include at the top of this file.
struct Recording
{// NOTE(kv) The ONE captured tree over the one model (Q57: presets are settings
 // rows, not separate captures). The arena owns primitives+groups; recapture
 // clears and refills it. Painter-level display state (viz_level etc.) lives in
 // Preset_Settings, applied live at replay -- not baked into the capture.
 // Persisted raw-block by ad_serialize_recording.cpp -- bump Data_Version when
 // any recorded struct changes (see ad_data.h).
 Arena arena;
 darray(Recorded_Primitive) primitives;
 darray(Recorded_Group) groups;
 darray(Recorded_Vertex) vertices;  // indexed by Recorded_Primitive.vertex_index
 b32 captured;
};
struct Model_Recordings
{
 Recording recording;  // live capture of the code path, recaptured every frame (debug/diff)
 // NOTE(kv) The *document*: regions already migrated out of code (plan-data-only-region-poc
 // Q92). Loaded from game/driver/documents/<current_document_name>.ad at startup, never
 // recaptured, replayed every frame with rendering on. "Is it data?" == "is it in here".
 Recording document;
 i32 preset_count;  // rows in use, 1..PRESET_CAP
 Preset_Settings preset_settings[PRESET_CAP];
 // NOTE(kv) plan-eye-to-document step 1: the Recording drawn as the document THIS frame --
 // `document`, or the compare document while flipped. Set by the game right before
 // driver_update, so driver code that reads the document (eye_origin) flips with it.
 // Never kept across frames (the compare document can be reloaded or cleared).
 Recording *displayed_document;
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
 
 // NOTE(kv) The recording: bone-space, immutable after capture (recording_arena).
 darray(Recorded_Primitive) primitives;
 darray(Recorded_Group) groups;
 darray(Recorded_Vertex) recorded_vertices;  // one per vertex slot pushed by send_primitive
 Group_Scope_Stack group_stack;
 // NOTE(kv) Live visibility per Group_Vis tag, published by the driver each frame
 // (before replay runs). vis_live[Vis_None] is always true.
 b32 vis_live[Group_Vis_Count];
 // NOTE(kv) Live shape-key weights per Weight_Key, published by the driver each frame
 // (before replay runs). weight_live[Weight_None] is always 0.
 v1 weight_live[Weight_Count];
 // NOTE(kv) Camera-space copy of `primitives` (frame_arena), rebuilt per frame on
 // demand -- consumers needing camera space read this, never transform the recording.
 darray(Recorded_Primitive) camera_primitives;
 darray(Vertex) vertices;

 // NOTE(kv) Per-preset recordings (Q36). Survives the per-frame model reset
 // (preserved across the clear-model block like `persistent`).
 Model_Recordings recordings;

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

struct View_Scope
{// NOTE(kv) One ViewCenterBlock entry: the derived view vector plus the center it
 // was computed from and the bone that center is expressed in (recorded onto
 // groups so replay can re-derive the vector from the live camera, Q43b).
 v3 vector;
 v3 center;
 Bone_ID bone;
};

struct Painter
{// NOTE(kv) This is a convenient global store.
 // IMPORTANT See @init_painter and @init_painter_2
 v1 looping_time;
 v1 anim_time;
 
 sarray(Location) hot_locations;
 // NOTE(kv) The document selection (Khoa, 2026-09-13: "highlight the selected thing"):
 // selected items also count as hot (visible, control points), but draw in
 // selection_color so they read apart from the item under the mouse.
 sarray(Location) selected_locations;
 Location current_draw_location;
 b32 current_location_is_hot;
 b32 current_location_is_selected;
 // NOTE(kv) Slider whose shape (Curve d0/d3) is being edited this frame, else invalid.
 // Set by the game per frame, read by @draw_curve to show the control handles.
 Location active_shape_location;
 b32 current_location_is_active_shape;
 
 Camera camera;
 mat4  clip_from_world;
 mat4  clip_from_bone; // see @set_bone_transform
 mat4i cam_from_bone;  // see @set_bone_transform
 
 i32 view_scope_count;
 View_Scope view_scope_stack[16];
 
 //-Debug collection
 i32 clipped_curve_count;
 i32 total_curve_count;
 
 Paint_Params params;
 // NOTE(kv) Live visibility bindings for the current scope (Q7 preset-followups):
 // drawing evaluates these against live state (vis_live table / current view vector)
 // exactly the way replay evaluates the recorded group's tags -- one visibility
 // mechanism for both paths, and nothing toggle-shaped enters the frozen params.
 // Scoped via SetInBlock in ShowGroup/ShowAlignedIfEx; the innermost binding wins,
 // same as replay's re-AND (so don't nest tagged scopes).
 Group_Vis     live_vis_tag;  // zero-init = Vis_None (vis_live[Vis_None] is true)
 Group_Cam_Vis live_cam_vis;  // zero-init = inactive

 //-misc
 b32 shading_on;
 b32 previous_draw_result;  // view @draw_bezier
 Render_Target *target;
 v1 profile_score;  // TODO: @Cleanup axe this?
 i32 viz_level;
 b32 show_all_lines;  // view Preset_Settings
 Viewport *viewport;
 b32 show_grid;
 argb shade_color;  // NOTE(kv) Useful enough to keep I guess.
 b32 sending_data;
 Reference_Mode reference_mode;
 argb background_color;
 u32 render_cycles;
 u32 reference_mesh_cycles;  // NOTE(kv) time spent in draw_reference_mesh this render (perf probe)
 // NOTE(kv) Bounding radius of the reference skull in its own .obj units (max vertex
 // distance from the obj origin), published by draw_reference_mesh so the gizmo
 // (game_reference_gizmo.cpp) can size its handle without seeing the mesh. 0 = no mesh
 // drawn yet.
 v1 reference_mesh_obj_radius;
 v3 reference_mesh_obj_center;  // NOTE(kv) bbox center in obj units, the sphere's center
 // NOTE(kv) plan-reference-landmarks step 4: per-layer mesh-space transform applied BEFORE
 // the shared placement (the mandible's hinge). Solved game-side from the landmarks
 // (reference_jaw_hinge, game_reference_landmarks.cpp) and written here before each
 // driver_render; the driver only applies it to layers flagged `hinged`.
 mat4i reference_layer_hinge[reference_mesh_layer_cap];
 b32   reference_layer_hinge_valid[reference_mesh_layer_cap];
};

global Painter *painter;  // see @init_painter
global Model *the_model;  // see @clear_model

myinline Preset_Settings &
active_preset_settings()
{// NOTE(kv) Settings row of the active preset for the viewport being rendered.
 return the_model->recordings.preset_settings[painter->viewport->preset];
}
myinline Reference_Scene
get_reference_scene()
{
 return active_preset_settings().scene;
}

//myinline u32 get_hot_prim_id(){ return painter->hot_prim_id; }
myinline Line_Params
get_line_params(){
 return painter->params.line;
}
myinline Fill_Params
get_fill_params(){
 return painter->params.fill;
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

global argb hot_color  = argb_lightness(linear_argb_red, 0.75f);
global argb hot_color2 = linear_argb_yellow;
//global argb selected_color = argb_red;
global argb selection_color = argb_lightness(linear_argb_green, 0.75f);  // document selection (see Painter.selected_locations)
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
  Reference_Scene preset;  // NOTE(kv) fpreset(Scene_x) text object; sets the active preset's scene
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
// NOTE(kv) Shared by capture (push_view_vector) and replay (dual-bezier culling)
// so the two derivations stay bit-identical.
function v3
view_vector_from(mat4i const&world_from_bone, v3 object_center)
{
 v3 camera_obj = world_from_bone.inv * camera_world_position(painter->camera);
 return noz(camera_obj - object_center);
}
function void
push_view_vector(v3 object_center)
{
 Painter *p = painter;
 v3 view_vector = view_vector_from(current_world_from_bone(), object_center);
 View_Scope scope = {};
 scope.vector = view_vector;
 scope.center = object_center;
 scope.bone   = current_bone()->id;
 p->view_scope_stack[p->view_scope_count++] = scope;
 kv_assert(p->view_scope_count < alen(p->view_scope_stack));
}
myinline v3
get_view_vector()
{
 return painter->view_scope_stack[painter->view_scope_count-1].vector;
}
myinline View_Scope
get_view_scope()
{
 return painter->view_scope_stack[painter->view_scope_count-1];
}
myinline void
pop_view_vector()
{
 Painter *p = painter;
 p->view_scope_count--;
 kv_assert(p->view_scope_count > 0);
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
 Bone *bone = push_zero(&the_model->bones);
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
//-

#include "4coder_debug_value.h"

//-NOTE(kv) Draw-mute state. Defined here (before game_draw.cpp) so BOTH the game_main
// translation unit and the driver_precompiled PCH unit -- which pulls in game_draw.cpp
// via this header but NOT framework_draw.cpp -- can see them. (The Vertex_Tee itself
// lives in framework_draw.cpp; draw_is_muted doesn't need it.)
// Q24: mutes the platform push -- lets the diff re-run a path without double-drawing.
global b32 global_rendering_suppressed;
// Rendering mode B: recorded-scope pushes from the CODE path are muted and the replay
// draws that scope instead. Only raised around driver_render (game_main.cpp).
global b32 global_replay_display;
// Shared mute predicate for BOTH funnels (poly3_inner + draw_image): a push is muted in
// mode-A replay (suppressed, diff-only) or when the code path is replaced by the replay
// in mode B.
function b32
draw_is_muted(b32 in_recorded_scope)
{
 return (global_rendering_suppressed or
         (global_replay_display and in_recorded_scope));
}
//-

#include "game_draw.cpp"
//-