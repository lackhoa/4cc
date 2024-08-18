/*
 * Mr. 4th Dimention - Allen Webster
 *
 * 10.11.2017
 *
 * Render target function implementations.
 *
 * NOTE(kv): Currently we leave the clipping, offset, upside-down y transform to the GPU.
 * When you change those values, we'll make a new render group.
 *
 */

#pragma once

#include "4ed_render_target.h"

global Render_State render_state;

inline Render_Config *
target_last_config()
{
 auto &state = render_state;
 if (state.group_last) { return &state.group_last->config; }
 else { return 0; }
}

inline Render_Target *
get_render_target(i32 window_id)
{
 return &render_state.targets[window_id];
}
//
inline Render_Target *
get_render_target(Render_Group *group)
{
 return &render_state.targets[group->window_id];
}

inline Render_Entry *
new_render_entry(Render_Entry_Type type)
{
 auto &state = render_state;
 Render_Group *group = state.group_last;
 if ( !render_group_is_game(group) )
 {
  kv_assert(group->entry_count == 0);
 }
 if (group)
 {
  Render_Entry *entry = push_struct(&state.arena, Render_Entry, true);
  entry->type = type;
  sll_queue_push(group->entry_first, group->entry_last, entry);
  group->entry_count++;
  return entry;
 }
 else { return 0; }
}

internal void
draw__new_group(Render_Target *target, Render_Config *config)
{
 auto &state = render_state;
 Render_Group *group = push_struct(&state.arena, Render_Group, true);
 sll_queue_push(state.group_first, state.group_last, group);
 state.group_count++;
 
 if (config) { group->config = *config; }
 group->face_id = state.face_id;
 group->window_id = target->window_id;
 
 new_render_entry(RET_Poly);
}

internal void
draw__maybe_new_group(Render_Target *target, Render_Config *config)
{
 Render_Group *last = render_state.group_last;
 if (last &&
     !render_group_is_game(last) && 
     last->entry_first->poly.vertex_list.count == 0 &&
     config == 0)
 {
  //pass
 }
 else
 {
  draw__new_group(target, config);
 }
}

internal Render_Vertex_Array_Node*
draw__extend_group_vertex_memory(Arena *arena, Render_Vertex_List *list, i1 size){
 Render_Vertex_Array_Node *node = push_array_zero(arena, Render_Vertex_Array_Node, 1);
 sll_queue_push(list->first, list->last, node);
 node->vertices = push_array(arena, Render_Vertex, size);
 node->vertex_max = size;
 return(node);
}

internal void
draw__set_face_id(Face_ID face_id)
{
 auto &state = render_state;
 if (state.face_id != face_id)
 {
  if (state.face_id != 0)
  {// NOTE: Has existing face id
   state.face_id = face_id;
   draw__new_group( get_render_target(0), target_last_config() );
  }
  else
  {// NOTE: No current face is set
   state.face_id = face_id;
   for (Render_Group *group = state.group_first;
        group != 0;
        group = group->next)
   {
    group->face_id = face_id;
   }
  }
 }
}

function void
draw_rect_outline_to_target(Render_Target *target, rect2 rect, v1 roundness, v1 thickness, u32 color,
                            v1 depth=0, Vertex_Type type=Vertex_Poly, v1 depth_offset=0.f)
{
 Render_Vertex vertices[6] = {};
 vertices[0].pos = V3(rect.x0, rect.y0, depth);
 vertices[1].pos = V3(rect.x1, rect.y0, depth);
 vertices[2].pos = V3(rect.x0, rect.y1, depth);
 vertices[3]     = vertices[1];
 vertices[4]     = vertices[2];
 vertices[5].pos = V3(rect.x1, rect.y1, depth);
 
 v2 center         = rect_center(rect);
 v1 half_thickness = thickness*0.5f;
 
 for_u32 ( index, 0, alen(vertices) )
 {
  Render_Vertex *vertex = vertices+index;
  vertex->center         = center;
  vertex->roundness      = roundness;
  vertex->color          = color;
  vertex->half_thickness = half_thickness;
  vertex->depth_offset   = depth_offset;
 }
 draw__push_vertices(target, vertices, alen(vertices), type);
}

api(custom) function void
draw_rect_outline(App *app, rect2 rect, v1 roundness, v1 thickness, ARGB_Color color, v1 depth)
{
 Render_Target *target = draw_get_target(app);
 draw_rect_outline_to_target(target, rect, roundness, thickness, color, depth);
}

force_inline void
draw_rect_outline2(App *app, rect2 rect, v1 thickness, ARGB_Color color)
{
    draw_rect_outline(app, rect, 0, thickness, color, 0);
}

force_inline void
draw_circle(App *app, v3 center, v1 radius, ARGB_Color color, v1 thickness)
{
    rect2 square = rect2_center_radius(center.xy, V2(radius, radius));
    draw_rect_outline(app, square, radius, thickness, color, center.z);
}

internal void
draw_rect_to_target(Render_Target *target, rect2 rect, v1 roundness, u32 color, v1 depth=0)
{
    v2 dim = rect2_dim(rect);
    v1 thickness = Max(dim.x, dim.y);
    draw_rect_outline_to_target(target, rect, roundness, thickness, color, depth);
}

force_inline void
draw_rect(App *app, rect2 rect, v1 roundness, ARGB_Color color, v1 depth)
{
    v2 dim = rect2_dim(rect);
    v1 thickness = Max(dim.x, dim.y);
    draw_rect_outline(app, rect, roundness, thickness, color, depth);
}

force_inline void
draw_rect2(App *app, rect2 rect, ARGB_Color color)
{
    draw_rect(app, rect, 0, color, 0);
}

force_inline void
ed_draw_disk(App *app, v3 center, v1 radius, ARGB_Color color)
{
    rect2 square = rect2_center_radius(center.xy, V2(radius, radius));
    draw_rect(app, square, radius, color, center.z);
}

force_inline void
draw_circle(App *app, v2 center, v1 radius, ARGB_Color color, v1 thickness)
{
 draw_circle(app, V3(center,0), radius, color, thickness);
}

//~
function rect2
draw_set_clip(Render_Target *target, rect2 clip_box)
{
 rect2 prev_clip;
 Render_Group *group = render_state.group_last;
 if (group)
 {
  if (group->clip_box != clip_box)
  {
   Render_Config config = group->config;
   prev_clip = config.clip_box;
   config.clip_box = clip_box;
   draw__maybe_new_group(target, &config);
  }
 }
 else
 {
  Render_Config config = {};
  draw__maybe_new_group(target, &config);
  prev_clip = {};
 }
 return(prev_clip);
}

internal void
begin_frame(void *font_set)
{
 auto &state = render_state;
 arena_clear(&render_state.arena);
 state.group_first = 0;
 state.group_last  = 0;
 state.face_id     = 0;
 state.font_set = font_set;
 
 for_i32(window_index,0,WINDOW_COUNT)
 {
  Render_Target *target = &state.targets[window_index];
  Render_Config config =
  {
   .clip_box = rect2{0, 0, (v1)target->width, (v1)target->height}
  };
  draw__new_group(target, &config);
 }
}

////////////////////////////////

internal void
draw_font_glyph(Render_Target *target, Face *face, u32 codepoint, Vec2_f32 p,
                ARGB_Color color, Glyph_Flag flags, Vec2_f32 x_axis)
{
    draw__set_face_id(face->id);
    
    u16 glyph_index = 0;
    if (!codepoint_index_map_read(&face->advance_map.codepoint_to_index,
                                  codepoint, &glyph_index))
    {
        glyph_index = 0;
    }
    // NOTE(kv): I guess this is a mega-texture situation?
    Glyph_Bounds bounds = face->bounds[glyph_index];
    kv_assert(bounds.w == 0);
    
    Render_Vertex vertices[6] = {};
    
    Rect_f32 uv = bounds.uv;
    vertices[0].uvw = V3(uv.x0, uv.y0, bounds.w);
    vertices[1].uvw = V3(uv.x1, uv.y0, bounds.w);
    vertices[2].uvw = V3(uv.x0, uv.y1, bounds.w);
    vertices[5].uvw = V3(uv.x1, uv.y1, bounds.w);
    
    Vec2_f32 y_axis = V2(-x_axis.y, x_axis.x);
    Vec2_f32 x_min = bounds.xy_off.x0*x_axis;
    Vec2_f32 x_max = bounds.xy_off.x1*x_axis;
    Vec2_f32 y_min = bounds.xy_off.y0*y_axis;
    Vec2_f32 y_max = bounds.xy_off.y1*y_axis;
    Vec2_f32 p_x_min = p + x_min;
    Vec2_f32 p_x_max = p + x_max;
    vertices[0].xy = p_x_min + y_min;
    vertices[1].xy = p_x_max + y_min;
    vertices[2].xy = p_x_min + y_max;
    vertices[3]    = vertices[1];
    vertices[4]    = vertices[2];
    vertices[5].xy = p_x_max + y_max;
    
    for (u32 i = 0; i < alen(vertices); i += 1)
    {
        vertices[i].color = color;
        vertices[i].half_thickness = 0.f;
    }
    
    draw__push_vertices(target, vertices, alen(vertices), Vertex_Poly);
}

////////////////////////////////

internal Vec2_f32
floor_v2(Vec2_f32 point)
{
    point.x = floorv1(point.x);
    point.y = floorv1(point.y);
    return(point);
}

internal f32
draw_string_inner(Render_Target *target, Face *face, String8 string, v2 point,
                  ARGB_Color color, u32 flags, v2 delta)
{
    f32 total_delta = 0.f;
    if (face != 0)
    {
        point = floor_v2(point);
        
        f32 byte_advance = face->metrics.byte_advance;
        f32 *byte_sub_advances = face->metrics.byte_sub_advances;
        
        u8 *str = (u8*)string.str;
        u8 *str_end = str + string.size;
        
        Translation_State tran = {};
        Translation_Emits emits = {};
        
        for (u32 i = 0; str < str_end; ++str, ++i){
            translating_fully_process_byte(&tran, *str, i, (i1)string.size, &emits);
            
            for (TRANSLATION_DECL_EMIT_LOOP(J, emits))
            {
                TRANSLATION_DECL_GET_STEP(step, behavior, J, emits);
                
                if (behavior.do_codepoint_advance)
                {
                    u32 codepoint = step.value;
                    if (color != 0){
                        u32 draw_codepoint = step.value;
                        if (draw_codepoint == '\t'){
                            draw_codepoint = ' ';
                        }
                        draw_font_glyph(target, face, draw_codepoint, point, color, flags, delta);
                    }
                    local_const f32 internal_tab_width = 4.f;
                    f32 d = font_get_glyph_advance(&face->advance_map, &face->metrics, codepoint, internal_tab_width);
                    point += d*delta;
                    total_delta += d;
                }
                else if (behavior.do_number_advance)
                {
                    u8 n = (u8)(step.value);
                    if (color != 0){
                        u8 cs[3];
                        cs[0] = '\\';
                        u8 nh = (n >> 4);
                        u8 nl = (n & 0xF);
                        u8 ch = '0' + nh;
                        u8 cl = '0' + nl;
                        if (nh > 0x9){
                            ch = ('A' - 0xA) + nh;
                        }
                        if (nl > 0x9){
                            cl = ('A' - 0xA) + nl;
                        }
                        cs[1] = ch;
                        cs[2] = cl;
                        
                        Vec2_f32 pp = point;
                        for (u32 j = 0; j < 3; ++j){
                            draw_font_glyph(target, face, cs[j], pp, color, flags, delta);
                            pp += delta*byte_sub_advances[j];
                        }
                    }
                    point += byte_advance*delta;
                    total_delta += byte_advance;
                }
            }
        }
    }
    
    return(total_delta);
}

internal f32
draw_string(Render_Target *target, Face *face, String string, Vec2_f32 point, u32 color){
    return(draw_string_inner(target, face, string, point, color, 0, V2(1.f, 0.f)));
}

internal f32
draw_string(Render_Target *target, Face *face, u8 *str, Vec2_f32 point, u32 color, u32 flags, Vec2_f32 delta){
    return(draw_string_inner(target, face, SCu8(str), point, color, flags, delta));
}

internal f32
draw_string(Render_Target *target, Face *face, u8 *str, Vec2_f32 point, u32 color){
    return(draw_string_inner(target, face, SCu8(str), point, color, 0, V2(1.f, 0.f)));
}

internal f32
font_string_width(Render_Target *target, Face *face, String str){
 return(draw_string_inner(target, face, str, V2(0, 0), 0, 0, V2(0, 0)));
}

internal f32
font_string_width(Render_Target *target, Face *face, u8 *str){
 return(draw_string_inner(target, face, SCu8(str), V2(0, 0), 0, 0, V2(0, 0)));
}

//~