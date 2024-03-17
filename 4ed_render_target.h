#pragma once

#include "4coder_types.h"

struct Render_Free_Texture
{
 Render_Free_Texture *next;
 u32 tex_id;
};

typedef i32 Object_ID;

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
    i32 vertex_count;
    i32 vertex_max;
};

struct Render_Vertex_List
{
 Render_Vertex_Array_Node *first;
 Render_Vertex_Array_Node *last;
 i32 count;
};

struct Render_Config
{
#define RENDER_CONFIG_FIELDS \
i32      viewport_id;\
rect2    clip_box;       \
b32      y_is_up;        \
argb     background;     \
mat4     view_transform; \
mat4i    camera_transform; \
v1       meter_to_pixel; \
v1       focal_length;   \
v1       near_clip;      \
v1       far_clip;       \
i32      scale_down_pow2;\
 
 RENDER_CONFIG_FIELDS
};

enum Render_Entry_Type
{
 Render_Null,
 Render_Poly,
 Render_Object_Transform,
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

struct CSG_Tree;

//NOTE: Kinda like a list of primitives for now?
struct CSG_Group
{
 CSG_Tree *first;
 CSG_Tree *last;
 i32 count;
};

struct CSG_Tree
{
 CSG_Tree *next;
 CSG_Type type;
 
 b32 negated;
 //b32 symx;
 
 union
 {
  struct { CSG_Tree *l; CSG_Tree *r; };
  struct { v3 center; v1 radius; };  // sphere
  struct { v3 n; v1 d; };            // plane
  // box: @speed This thing is way too big
  struct { 
   mat4  to_aabb;
   v3    box_radius; 
  };
 };
};

//NOTE: We bin vertices like this because they may require different programs, uniforms, etc.
struct Render_Entry_Poly
{
 Render_Vertex_List vertex_list;
 Render_Vertex_List vertex_list_overlay;
 CSG_Tree           csg;
};

//
typedef mat4 Render_Entry_Object_Transform;

struct Render_Entry
{
 Render_Entry *next;
 
 Render_Entry_Type type;
 union {
  Render_Entry_Poly poly;
  Render_Entry_Object_Transform object_transform;
 };
};

struct Render_Group
{
 Render_Group *next;
 
 i32 entry_count;
 Render_Entry *entry_first;
 Render_Entry *entry_last;
 
 Face_ID face_id;
 union
 {
  Render_Config config;
  struct { RENDER_CONFIG_FIELDS };
 };
};

force_inline b32 render_group_is_game(Render_Group *group)
{
 return group->viewport_id != 0;
}

struct Render_Target
{
 i32 width;
 i32 height;
 u32 texture_bound_at_unit0;
 Face_ID  face_id;
 
 Render_Free_Texture *free_texture_first;
 Render_Free_Texture *free_texture_last;
 
 Arena arena;
 Render_Group *group_first;
 Render_Group *group_last;
 i32 group_count;
 
 void *font_set;
 u32 fallback_texture_id;
};

#define draw_get_target_return Render_Target *
#define draw_get_target_params App *app
ED_API_FUNCTION(draw_get_target);
