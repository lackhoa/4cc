/*
 * Mr. 4th Dimention - Allen Webster
 *
 * 10.11.2017
 *
 * Render target type definition
 *
 */

// TOP

#pragma once

struct Render_Free_Texture
{
    Render_Free_Texture *next;
    u32 tex_id;
};

struct Render_Vertex
{
    union 
    {
        v3 xyz;
        v2 xy;
    };
    union 
    {
        v3 uvw;
        struct { v2 center; v1 roundness; };
    };
    u32 color;
    v1  half_thickness;
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
    Face_ID face_id; \
    Rect_f32 clip_box; \
    v2 offset; \
    b8 y_is_up; \
    b8     is_perspective; \
    Camera camera; \
    b8 depth_test; \
    b8 linear_alpha_blend;
    
    RENDER_CONFIG_FIELDS
};

struct Render_Group
{
    Render_Group *next;
    Render_Vertex_List vertex_list;
    union
    {
        Render_Config render_config;
        struct { RENDER_CONFIG_FIELDS };
    };
};

global const Face_ID FACE_ID_GAME = U32_MAX;

struct Render_Target
{
    union
    {
        Render_Config render_config;
        struct { RENDER_CONFIG_FIELDS; };
    };
    
    b8 clip_all;
    i32 width;
    i32 height;
    Texture_ID bound_texture;
    u32 color;
    
    i32 frame_index;
    f32 literal_dt;
    f32 animation_dt;
    
    Render_Free_Texture *free_texture_first;
    Render_Free_Texture *free_texture_last;
    
    Arena arena;
    Render_Group *group_first;
    Render_Group *group_last;
    i32 group_count;
    
    void *font_set;
    Texture_ID fallback_texture_id;
};

// BOTTOM
