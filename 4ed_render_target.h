#pragma once

#define WINDOW_COUNT 2

struct Render_Free_Texture
{
 Render_Free_Texture *next;
 u32 tex_id;
};

typedef i1 Object_ID;

enum Vertex_Type {
 Vertex_Poly,
 Vertex_Overlay,
};

struct Render_Vertex
{
 union { v3 pos; struct { v2 xy; v1 z; }; };
 union
 {
  union  { v3 uvw;    v2 uv; };
  struct { v2 center; v1 roundness; };
 };
 argb color;
 v1 half_thickness;
 v1 depth_offset;
 u32 prim_id;
};

struct Render_Vertex_Array_Node
{
    Render_Vertex_Array_Node *next;
    Render_Vertex *vertices;
    i1 vertex_count;
    i1 vertex_max;
};

struct Render_Vertex_List
{
 Render_Vertex_Array_Node *first;
 Render_Vertex_Array_Node *last;
 i1 count;
};

struct Render_Config
{
#define RENDER_CONFIG_FIELDS \
i1      viewport_id;      \
rect2   clip_box;         \
b32     y_is_up;          \
argb    background;       \
mat4    view_transform;   \
mat4i   camera_transform; \
v1      meter_to_pixel;   \
v1      focal_length;     \
v1      near_clip;        \
v1      far_clip;         \
i1      scale_down_pow2;  \
 
 RENDER_CONFIG_FIELDS
};

enum Render_Entry_Type
{
 RET_Null,
 RET_Poly,
 RET_Object_Transform,
 RET_Image,
};

enum CSG_Type
{
 CSG_NULL     = 0,
 
 CSG_Union    = 1,
 CSG_Subtract = 2,
 
 CSG_Sphere    = 128,
 CSG_Plane     = 129,
 CSG_Box       = 130,
};

//NOTE: We bin vertices like this because they may require different programs, uniforms, etc.
struct Render_Entry_Poly
{
 Render_Vertex_List vertex_list;
 Render_Vertex_List vertex_list_overlay;
};

struct Render_Entry_Image
{
 char *filename;
 //mat4 *object_transform;
 v3 o, x, y_info;  // "y_info" encodes both direction and scale
 argb  color;
 u32 prim_id;
};

//
struct Render_Entry
{
 Render_Entry *next;
 
 Render_Entry_Type type;
 union
 {
  Render_Entry_Poly   poly;
  Render_Entry_Image *image;
  mat4                object_transform;
 };
};

struct Render_Group
{
 i32 window_id;
 Render_Group *next;
 
 i1 entry_count;
 Render_Entry *entry_first;
 Render_Entry *entry_last;
 
 Face_ID face_id;
 union
 {
  Render_Config config;
  struct { RENDER_CONFIG_FIELDS };
 };
};

force_inline b32
render_group_is_game(Render_Group *group)
{
 return group->viewport_id != 0;
}


// NOTE(kv): Render targets correspond to windows,
// since we have multiple windows now.
struct Render_Target
{
 i1 window_id;
 i1 width;
 i1 height;
};
//
struct Render_State
{
 Render_Target targets[WINDOW_COUNT];
 
 u32 texture_bound_at_unit0;
 Face_ID  face_id;
 
 Render_Free_Texture *free_texture_first;
 Render_Free_Texture *free_texture_last;
 
 Arena arena;
 Render_Group *group_first;
 Render_Group *group_last;
 i1 group_count;
 
 void *font_set;
 u32 fallback_texture_id;
};

#define draw_get_target_return Render_Target *
#define draw_get_target_params App *app

#define draw__push_vertices_return void
#define draw__push_vertices_params Render_Target *target, Render_Vertex *vertices, i1 count, Vertex_Type type

#define push_image_return void
#define push_image_params Render_Target *target, char *filename, v3 o, v3 x, v3 y, argb color, u32 prim_id

#define push_object_transform_to_target_return void
#define push_object_transform_to_target_params Render_Target *target, mat4 const*transform

#define draw_configure_return void
#define draw_configure_params Render_Target *target, Render_Config *config

//~