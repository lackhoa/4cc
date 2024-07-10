// @distance_level_nonsense
// TODO: NOT HAPPY with storing redundant data!
struct Camera_Data  // IMPORTANT: @Serialized
{
#define CAMERA_DATA_FIELDS(X) \
X(v1,distance) \
X(v1,phi) \
X(v1,theta) \
X(i1,distance_level) \
X(v1,roll) \
X(v3,pan) \
X(v3, pivot) \
 
#define X(type,name) type name;
 CAMERA_DATA_FIELDS(X)
#undef X
};

struct Game_Save_Old
{
 u32 magic_number;
 u32 version;
};

struct Game_Save
{
 u32 magic_number;
 u32 version;
 
 Camera_Data camera[GAME_VIEWPORT_COUNT];
};

struct Viewport
{
 i1 preset;
 i1 last_preset;
 Camera_Data camera;
 // ;viewport_frame_arena @frame_arena_init @frame_arena_clear
 Arena frame_arena;
};

struct Bezier
{
 v3 e[4];
 force_inline operator v3 *() { return e; };
};
typedef Bezier Bez;

struct Planar_Bezier
{
 v2 d0;
 v2 d3;
 v3 unit_y;
};

struct Vertex_Data
{
 v3 pos;
 String name;
 v3 basis;
};

struct Bezier_Data
{
 Bezier bezier;
 String name;
 v4 radii;
};

struct Game_State
{
 Arena permanent_arena;
 
 Game_Save save;
 b32 has_done_backup;
 String save_dir;
 String save_path_text;
 
 //-NOTE: misc
 v1 time;
 b32 indicator_level;
 Viewport viewports[GAME_VIEWPORT_COUNT];
 
 //-FUI
 //-FUI new
 i1             line_cap;
 struct Line_Map_Entry *line_map;
 Arena           new_slider_store;
 
 //-NOTE: Editor
 u32 selected_obj_id;
 
 Vertex_Data previous_vertex;
 Vertex_Data vertices[4];
 i1 vertex_count;
 
 Bezier_Data previous_curve;
 Bezier_Data curves[128];
 i1 curve_count;
 
 b8 __padding[64];
};

//~ id system
enum Primitive_Type : u8
{
 Prim_Null     = 0,
 Prim_Vertex   = 1,
 Prim_Curve    = 2,
 Prim_Triangle = 3,
};

force_inline Primitive_Type
prim_id_type(u32 id)
{
 return Primitive_Type(id >> 24);
}

force_inline b32
prim_id_is_obj(u32 prim_id)
{
 return prim_id_type(prim_id) != 0;
}

force_inline u32
prim_id_from_vertex_index(i1 index)
{
 u32 type = u32(Prim_Vertex) << 24;
 u32 result = (u32)(index) | type;
 return result;
}
//
force_inline u32
index_from_prim_id(u32 id)
{
 if ( prim_id_is_obj(id) )
 {
  return (id & 0xFFFF);
 }
 else return 0;
}

force_inline u32
prim_id_from_curve_index(i1 index)
{
 u32 type = u32(Prim_Curve) << 24;
 u32 result = (u32)(index) | type;
 return result;
}

force_inline i1
get_selected_vertice_index(Game_State *state)
{
 i1 result = 0;
 u32 id = state->selected_obj_id;
 if ( prim_id_type(id) == Prim_Vertex )
 {
  return index_from_prim_id(id);
 }
 else return result;
}

//~

global char *global_debug_scope;
#define vertex_block(NAME) set_in_block(global_debug_scope, NAME)

#define DEBUG_NAME(NAME, VALUE)  DEBUG_VALUE_inner(global_debug_scope, NAME, VALUE, 0)
#define DEBUG_NAME_COLOR(NAME, VALUE, COLOR)  DEBUG_VALUE_inner(global_debug_scope, NAME, VALUE, COLOR)
#define DEBUG_VALUE(VALUE)       DEBUG_VALUE_inner(global_debug_scope, #VALUE, VALUE)
#define DEBUG_TEXT(TEXT)         DEBUG_VALUE_inner(global_debug_scope, TEXT, 0.f)
//~

global u32 selected_obj_id;

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

internal void
setup_camera(Camera *camera, Camera_Data *data)
{
 *camera = {};
 
 camera->near_clip    = 1*centimeter;
 camera->far_clip     = 10.f;
 camera->focal_length = 0.6169f;
 
 // TODO We can just block copy here
#define X(type,name)   camera->name = data->name;
 CAMERA_DATA_FIELDS(X)
#undef X
 
 camera->transform =  (mat4i_translate(data->pan) *
                      mat4i_rotate_tpr(data->theta, data->phi, data->roll, data->pivot) *
                      mat4i_translate(data->pivot+V3z(data->distance)));
}

//~ EOF
