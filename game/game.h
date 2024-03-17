// @distance_level_nonsense TODO: NOT HAPPY with storing redundant data!
struct Camera_Data  // IMPORTANT: @Serialized
{
#define CAMERA_DATA_FIELDS(X) \
X(v1,distance) \
X(v1,phi) \
X(v1,theta) \
X(v1i, distance_level) \
X(v2,UNUSED_FIELD) \
X(v1,roll) \
X(v3,pan) \
 
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
 i32 preset;
 i32 last_preset;
 Camera_Data camera;
 // ;viewport_frame_arena @frame_arena_init @frame_arena_clear
 Arena frame_arena;
};

struct Game_State
{
 Arena permanent_arena;
 Arena mesh_arena;
 
 Game_Save save;
 b32 has_done_backup;
 String save_dir;
 String save_path;
 
 //-NOTE: misc
 v1 time;
 b32 indicator_level;
 Viewport viewports[GAME_VIEWPORT_COUNT];
 
 Fui_Store fui_store[1];
 
 b8 __padding[64];
};

global mat4 mat4_negateX = {{
  -1,0,0,0,
  0,1,0,0,
  0,0,1,0,
  0,0,0,1,
 }};

global Arena *viewport_frame_arena;  // @set_viewport_frame_arena
global CSG_Group *global_csg_group;  // @set_global_csg_group
global mat4i object_to_camera;  // set in push_object_transform
global mat4i &raycast_transform = object_to_camera;
global mat4i object_transform;

//~
global char *global_debug_scope;

#define DEBUG_NAME(NAME, VALUE)  DEBUG_VALUE_inner(global_debug_scope, NAME, VALUE, 0)
#define DEBUG_NAME_COLOR(NAME, VALUE, COLOR)  DEBUG_VALUE_inner(global_debug_scope, NAME, VALUE, COLOR)
#define DEBUG_VALUE(VALUE)       DEBUG_VALUE_inner(global_debug_scope, #VALUE, VALUE)
#define DEBUG_TEXT(TEXT)         DEBUG_VALUE_inner(global_debug_scope, TEXT, 0.f)
//~


//~