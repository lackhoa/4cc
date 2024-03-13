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

internal void
draw__begin_new_group(Render_Target *target)
{
    Render_Group *group = 0;
    Render_Group *last = target->group_last;
    if (last &&
        (last->vertex_list.count == 0))
    {// NOTE(kv): Use existing empty group
        group = last;
    }
    else
    {
        group = push_array_zero(&target->arena, Render_Group, 1);
        sll_queue_push(target->group_first, target->group_last, group);
    }
    // NOTE: copy the render config over
    group->render_config = target->render_config;
}

internal Render_Vertex_Array_Node*
draw__extend_group_vertex_memory(Arena *arena, Render_Vertex_List *list, i32 size){
    Render_Vertex_Array_Node *node = push_array_zero(arena, Render_Vertex_Array_Node, 1);
    sll_queue_push(list->first, list->last, node);
    node->vertices = push_array(arena, Render_Vertex, size);
    node->vertex_max = size;
    return(node);
}

internal void
draw__write_vertices_in_current_group(Render_Target *target, Render_Vertex *vertices, i32 count)
{
    if (count > 0)
    {
        Render_Group *group = target->group_last;
        if (group == 0)
        {
            draw__begin_new_group(target);
            group = target->group_last;
        }
        
        Render_Vertex_List *list = &group->vertex_list;
        
        Render_Vertex_Array_Node *last = list->last;
        
        Render_Vertex *tail_vertex = 0;
        i32 tail_count = 0;
        if (last != 0)
        {
            tail_vertex = last->vertices + last->vertex_count;
            tail_count = last->vertex_max - last->vertex_count;
        }
        
        i32 base_vertex_max = 64;
        i32 transfer_count = clamp_top(count, tail_count);
        if (transfer_count > 0)
        {
            block_copy_dynamic_array(tail_vertex, vertices, transfer_count);
            last->vertex_count += transfer_count;
            list->count += transfer_count;
            base_vertex_max = last->vertex_max;
        }
        
        i32 count_left_over = count - transfer_count;
        if (count_left_over > 0)
        {
            Render_Vertex *vertices_left_over = vertices + transfer_count;
            
            i32 next_node_size = (base_vertex_max + count_left_over)*2;
            Render_Vertex_Array_Node *memory = draw__extend_group_vertex_memory(&target->arena, list, next_node_size);
            block_copy_dynamic_array(memory->vertices, vertices_left_over, count_left_over);
            memory->vertex_count += count_left_over;
            list->count += count_left_over;
        }
    }
}

internal void
draw__set_face_id(Render_Target *target, Face_ID face_id)
{
    if (target->face_id != face_id)
    {
        if (target->face_id != 0)
        {// NOTE: Has existing face id
            target->face_id = face_id;
            draw__begin_new_group(target);
        }
        else
        {// NOTE: No current face is set
            target->face_id = face_id;
            for (Render_Group *group = target->group_first;
                 group != 0;
                 group = group->next)
            {
                group->face_id = face_id;
            }
        }
    }
}

// TODO: Not sure if I love the abstraction over Render_Target
// TODO @Cleanup PLEASE tell me you're gonna clean up the transform situation
internal void
draw_configure(App *app, Render_Config *config)
{
    Render_Target *target = draw_get_target(app);
    if (target->y_is_up != config->y_is_up)
    {
        target->y_is_up = config->y_is_up;
        rect2 new_clip_box = target->clip_box;
        // NOTE: Fun times changing the clip box
        new_clip_box.y0 = (f32)target->height - target->clip_box.y1;
        new_clip_box.y1 = (f32)target->height - target->clip_box.y0;
        target->clip_box = new_clip_box;
    }
    
    if (config->clip_box.min==v2{} &&  config->clip_box.max==v2{})
    {
        config->clip_box = target->clip_box;
    }
    
    target->render_config = *config;
    
    draw__begin_new_group(target);
}

internal draw_triangle_return
draw_triangle(draw_triangle_params)
{
    Render_Target *target = draw_get_target(app);
    Render_Vertex vertices[3] = {};
    vertices[0].xyz = p0;
    vertices[1].xyz = p1;
    vertices[2].xyz = p2;
    
    for_u32 (i,0,alen(vertices))
    {
        Render_Vertex *vertex = vertices+i;
        vertex->uvw   = V3(0,0,0);
        vertex->color = color;
    }
    draw__write_vertices_in_current_group(target, vertices, alen(vertices));
}

force_inline void
draw_quad(App *app, v3 p0, v3 p1, v3 p2, v3 p3, ARGB_Color color)
{
    draw_triangle(app, p0,p1,p2, color);
    draw_triangle(app, p1,p2,p3, color);
}

internal void
draw_rect_outline_to_target(Render_Target *target, rect2 rect, v1 roundness, v1 thickness, u32 color, v1 depth=0)
{
    kv_clamp_bot(roundness, epsilon_f32);
    kv_clamp_bot(thickness, 1.f);
    
    Render_Vertex vertices[6] = {};
    vertices[0].xyz = V3(rect.x0, rect.y0, depth);
    vertices[1].xyz = V3(rect.x1, rect.y0, depth);
    vertices[2].xyz = V3(rect.x0, rect.y1, depth);
    vertices[3]    = vertices[1];
    vertices[4]    = vertices[2];
    vertices[5].xyz = V3(rect.x1, rect.y1, depth);
    
    v2 center         = rect_center(rect);
    v1 half_thickness = thickness*0.5f;
    for_u32 (i,0,alen(vertices))
    {
        Render_Vertex *vertex = vertices+i;
        vertex->center         = center;
        vertex->roundness      = roundness;
        vertex->color          = color;
        vertex->half_thickness = half_thickness;
    }
    draw__write_vertices_in_current_group(target, vertices, alen(vertices));
}

api(custom) internal draw_rect_outline_return
draw_rect_outline(draw_rect_outline_params)
{
    draw_rect_outline_to_target(draw_get_target(app), rect, roundness, thickness, color, depth);
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
    draw_rect_outline(app, square, radius, thickness, color, 0);
}

internal void
draw_rect_to_target(Render_Target *target, rect2 rect, v1 roundness, u32 color, v1 depth=0)
{
    v2 dim = rect_dim(rect);
    v1 thickness = Max(dim.x, dim.y);
    draw_rect_outline_to_target(target, rect, roundness, thickness, color, depth);
}

force_inline void
draw_rect(App *app, rect2 rect, v1 roundness, ARGB_Color color, v1 depth)
{
    v2 dim = rect_dim(rect);
    v1 thickness = Max(dim.x, dim.y);
    draw_rect_outline(app, rect, roundness, thickness, color, depth);
}

force_inline void
draw_rect2(App *app, rect2 rect, ARGB_Color color)
{
    draw_rect(app, rect, 0, color, 0);
}

force_inline void
draw_disk(App *app, v3 center, v1 radius, ARGB_Color color)
{
    rect2 square = rect2_center_radius(center.xy, V2(radius, radius));
    draw_rect(app, square, radius, color, center.z);
}

internal void
draw_line(App *app, v3 p0, v3 p1, v1 thickness, ARGB_Color color)
{
    f32 half_thickness = clamp_bot(0.5f*thickness, 1.0f);
    //Models *models = (Models*)app->cmd_context;
    //if (models->in_render_mode)
    {
        Render_Target *target = draw_get_target(app);
        b32 steep = false;
        v1 x0 = p0.x; 
        v1 y0 = p0.y;
        v1 z0 = p0.z;
        v1 x1 = p1.x; 
        v1 y1 = p1.y;
        v1 z1 = p1.z;
        
        if (absolute(x0-x1) < 
            absolute(y0-y1))
        {// if the line is steep, we transpose the image 
            macro_swap(x0, y0);
            macro_swap(x1, y1);
            steep = true;
        }
        
        if (x0 > x1)
        {// make it "left to right"
            macro_swap(x0, x1); 
            macro_swap(y0, y1); 
            macro_swap(z0, z1);
        }
        
        if ((x1-x0) > 0.0001f)
        {
            v1 dy = (y1-y0) / (x1-x0);
            v1 dz = (z1-z0) / (x1-x0);
             
            // NOTE: Clipping
            f32 x_start = x0;
            f32 x_end   = x1;
            {
                f32 x_bot = (target->clip_box.x0 - target->offset.x);
                f32 x_top = (target->clip_box.x1 - target->offset.x);
                if (steep)
                {
                    x_bot = (target->clip_box.y0 - target->offset.y);
                    x_top = (target->clip_box.y1 - target->offset.y);
                }
                kv_clamp_bot(x_start, x_bot);
                kv_clamp_top(x_end,   x_top);
            }
            
            i32 nsamples = 16;
            f32 interval = (x_end - x_start) / (f32)nsamples;
            for (i32 index = 0; 
                 index <= nsamples; 
                 index++)
            {
                v1 x = x_start + (v1)index * interval;
                v1 y = y0 + dy*(x-x0);
                v1 z = z0 + dz*(x-x0);
                v2 center = (steep ? 
                             v2{y,x} :
                             v2{x,y});
                rect2 square = rect2_center_radius(center, V2All(half_thickness));
                draw_rect_to_target(target, square, half_thickness, color, z);
            }
        }
    }
}

force_inline void
draw_circle(App *app, v2 center, v1 radius, ARGB_Color color, v1 thickness)
{
    draw_circle(app, V3(center,0), radius, color, thickness);
}

force_inline rect2
draw_get_clip(App *app)
{
    Render_Target *target = draw_get_target(app);
    return target->clip_box;
}


////////////////////////////////
#if !AD_IS_COMPILING_GAME

internal draw_get_target_return
draw_get_target(draw_get_target_params)
{
    Models *models = (Models*)app->cmd_context;
    return models->target;
}

internal Rect_f32
draw_set_clip(Render_Target *target, Rect_f32 clip_box)
{
    Rect_f32 result = target->clip_box;
    if (target->clip_box != clip_box)
    {
        target->clip_box = clip_box;
        draw__begin_new_group(target);
    }
    return(result);
}

internal void
begin_frame(Render_Target *target, void *font_set)
{
    linalloc_clear(&target->arena);
    target->group_first = 0;
    target->group_last = 0;
    target->face_id = 0;
    target->clip_box = Rf32(0, 0, (f32)target->width, (f32)target->height);
    target->font_set = font_set;
}

internal void
begin_render_section(Render_Target *target, i32 frame_index, f32 literal_dt, f32 animation_dt){
    target->frame_index = frame_index;
    target->literal_dt = literal_dt;
    target->animation_dt = animation_dt;
}

internal void
end_render_section(Render_Target *target){
}

////////////////////////////////

internal void
draw_font_glyph(Render_Target *target, Face *face, u32 codepoint, Vec2_f32 p,
                ARGB_Color color, Glyph_Flag flags, Vec2_f32 x_axis)
{
    draw__set_face_id(target, face->id);
    
    u16 glyph_index = 0;
    if (!codepoint_index_map_read(&face->advance_map.codepoint_to_index,
                                  codepoint, &glyph_index))
    {
        glyph_index = 0;
    }
    // NOTE(kv): I guess this is a mega-texture situation?
    Glyph_Bounds bounds = face->bounds[glyph_index];
    alert_if_false(bounds.w == 0);
    
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
    
    draw__write_vertices_in_current_group(target, vertices, alen(vertices));
}

internal void
draw_textured_rect_to_target(Render_Target *target, rect2 rect, ARGB_Color color)
{
    // NOTE(kv): We still don't know what a group is, so for now let's juts monkey around with the FaceID for now
    Face_ID old_face_id = target->face_id;
    draw__set_face_id(target, FACE_ID_GAME);
    
    Render_Vertex vertices[6] = {};
    
    vertices[0].uvw = V3(0, 0, 0);
    vertices[1].uvw = V3(1, 0, 0);
    vertices[2].uvw = V3(0, 1, 0);
    vertices[5].uvw = V3(1, 1, 0);
   
    auto &min = rect.min;
    auto &max = rect.max;
    vertices[0].xy = min;
    vertices[1].xy = v2{max.x, min.y};
    vertices[2].xy = v2{min.x, max.y};
    vertices[5].xy = max;
    
    for_u32(i, 0, alen(vertices))
    {
        vertices[i].color = color;
        vertices[i].half_thickness = 0.f;
    }
    
    vertices[3]    = vertices[1];
    vertices[4]    = vertices[2];
    
    draw__write_vertices_in_current_group(target, vertices, alen(vertices));
    
    // NOTE(kv): Revert back to the "normal" rendering (maybe wonky, but seems like it works).
    draw__set_face_id(target, old_face_id);
}

////////////////////////////////

internal Vec2_f32
floor_v2(Vec2_f32 point)
{
    point.x = f32_floor32(point.x);
    point.y = f32_floor32(point.y);
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
            translating_fully_process_byte(&tran, *str, i, (i32)string.size, &emits);
            
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
draw_string(Render_Target *target, Face *face, String_Const_u8 string, Vec2_f32 point, u32 color){
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

#endif // AD_IS_COMPILING_GAME
