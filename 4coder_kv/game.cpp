/*
  NOTE(kv): OLD Rules for the renderer:

  - Textures and bitmaps are srgb premultiplied-alpha
  - v4 colors are in linear space, alpha=1 (so pma doesn't matter)
  - Packed colors are rgba in memory order (i.e abgr in u32 register order)
  pma = Pre-multiplied alpha
 */

#include "tr_model.cpp"

// TODO: @Cleanup: Rename. (also with the fcolor)
global v4  yellow_v4   = {.5, .5, 0, 1.0};
global u32 yellow_argb = pack_argb(yellow_v4);
global v4  gray_v4     = {.5,.5,.5,1};
global u32 gray_argb   = pack_argb(gray_v4);
global u32 red_argb    = pack_argb({1,0,0,1});
global u32 green_argb  = pack_argb({0,1,0,1});
global u32 blue_argb   = pack_argb({0,0,1,1});
global v4  black_v4    = {0,0,0,1};
global u32 black_argb  = pack_argb(black_v4);
global v4  white_v4    = {1,1,1,1};
global u32 white_argb  = pack_argb(white_v4);

// NOTE: This is a dummy buffer, so we can use the same commands to switch to the rendered game
global String GAME_BUFFER_NAME = str8lit("*game*");


struct Screen
{
  v3 center;
  v3 x_axis;
  v3 y_axis;
};

internal void
bitmap_set(Bitmap *bitmap, i32 x, i32 y, u32 color)
{
    y = bitmap->dim.y-1 - y; // inverting y
    
    if (in_range(0, y, bitmap->dim.y) &&
        in_range(0, x, bitmap->dim.x))
    {
        i32 index = y * bitmap->dim.x + x;
        bitmap->data[index] = color;
    }
}

inline u64
get_bitmap_size(Bitmap *bitmap)
{
    return bitmap->dim.y * bitmap->pitch;
}

internal void
tr_rectangle2i(Bitmap *bitmap, rect2i rect, ARGB_Color color)
{
    v2i dim = rect2i_get_dim(rect);
    for_i32 (y, rect.min.x, dim.y)
    {
        for_i32 (x, rect.min.x, dim.x)
        {
            bitmap_set(bitmap, x, y, color);
        }
    }
}

internal void 
tr_linei(Bitmap *bitmap, 
         i32 x0, i32 y0, i32 x1, i32 y1,
         ARGB_Color color)
{
    bool steep = false; 
   
    if (absolute(x0-x1) < absolute(y0-y1))
    {// if the line is steep, we transpose the image 
        macro_swap(x0, y0); 
        macro_swap(x1, y1); 
        steep = true; 
    }
    
    if (x0 > x1)
    {// make it "left to right"
        macro_swap(x0, x1); 
        macro_swap(y0, y1); 
    }
    
    i32 dx = x1-x0;
    i32 dy = y1-y0;
    i32 derror = absolute(dy) * 2; 
    i32 error = 0;
    i32 y = y0; 
    
    for (i32 x=x0;
         x <= x1; 
         x++)
    { 
        if (steep)
            bitmap_set(bitmap, y, x, color);   // if transposed, detranspose
        else
            bitmap_set(bitmap, x, y, color);
       
        error += derror;
        if (error > dx)
        {
            if (y1 > y0) y += 1;
            else         y -= 1;
            error -= 2 * dx;
        }
    }
}

internal void
tr_line(Bitmap *bitmap, 
        f32 x0, f32 y0, f32 x1, f32 y1,
        ARGB_Color color)
{
    return tr_linei(bitmap, (i32)x0, (i32)y0, (i32)x1, (i32)y1, color);
}

inline void
tr_line(Bitmap *bitmap, v2 p0, v2 p1, ARGB_Color color)
{
    tr_line(bitmap, p0.x, p0.y, p1.x, p1.y, color);
}

internal void
tr_triangle_old_school(Bitmap *bitmap, v2 p0, v2 p1, v2 p2, ARGB_Color color) 
{
    if (p0.y > p1.y) macro_swap(p0, p1); 
    if (p0.y > p2.y) macro_swap(p0, p2); 
    if (p1.y > p2.y) macro_swap(p1, p2); 
    
    f32 dy02 = p2.y - p0.y; 
    f32 dy01 = p1.y - p0.y;
    if (dy01 < 0.0001f) return;
    
    for_i32 (y, (i32)p0.y, (i32)p1.y+1)
    {
        f32 dy0 = (f32)y - p0.y;
        f32 alpha = dy0 / dy02;
        f32 beta  = dy0 / dy01;
        f32 Ax = lerp(p0.x, alpha, p2.x);
        f32 Bx = lerp(p0.x, beta,  p1.x);
        if (Ax > Bx)  macro_swap(Ax, Bx);
        for_i32 (x, (i32)Ax, (i32)Bx+1 )
        { 
            bitmap_set(bitmap, x, y, color);
        } 
    }
    
    f32 dy12 = p2.y - p1.y;
    if (dy12 > 0.0001f)
    {
        for_i32 (y, (i32)p1.y, (i32)p2.y+1)
        { 
            f32 dy0 = (f32)y - p0.y;
            f32 dy1 = (f32)y - p1.y;
            f32 alpha = dy0 / dy02; 
            f32 beta  = dy1 / dy12; // be careful with divisions by zero 
            f32 Ax = lerp(p0.x, alpha, p2.x);
            f32 Bx = lerp(p1.x, beta,  p2.x);
            if (Ax > Bx) macro_swap(Ax, Bx); 
            for_i32 (x, (i32)Ax, (i32)Bx+1)
            { 
                bitmap_set(bitmap, x, y, color);
            } 
        }    
    }
    
    fslider( draw_outline, v1{-0.298632f} );
    if (draw_outline > 0)
    {
        tr_line(bitmap, (p1), (p2), red_argb); 
        tr_line(bitmap, (p0), (p2), green_argb); 
        tr_line(bitmap, (p0), (p1), blue_argb); 
    }
}

// TODO: idk why this returns a v3, but rn I don't care
internal v3 barycentric(v2 *p, v2 P)
{
    v2 p0 = p[0];
    v2 p1 = p[1];
    v2 p2 = p[2];
    v3 u = cross(v3{ p2.x-p0.x, p1.x-p0.x, p0.x-P.x },
                 v3{ p2.y-p0.y, p1.y-p0.y, p0.y-P.y });
    
    if ( absolute(u.z) < 0.0001f )
        return v3{-1,1,1};
    else
        return v3{1.0f - (u.x+u.y)/u.z, u.y/u.z, u.x/u.z};
}

internal void 
tr_triangle(Bitmap *bitmap, v2 *p, ARGB_Color color)
{ 
    v2i max = bitmap->dim;
    v2i bboxmin = max; 
    v2i bboxmax = {0,0}; 
    for_i32 (i,0,3)
    { 
        bboxmin.x = clamp_bot(macro_min(bboxmin.x, (i32)p[i].x), 0);
        bboxmin.y = clamp_bot(macro_min(bboxmin.y, (i32)p[i].y), 0);
        
        bboxmax.x = clamp_top(macro_max(bboxmax.x, (i32)p[i].x+1), max.x);
        bboxmax.y = clamp_top(macro_max(bboxmax.y, (i32)p[i].y+1), max.y);
    } 
    for_i32 (x, bboxmin.x, bboxmax.x)
    { 
        for_i32 (y, bboxmin.y, bboxmax.y)
        {
            v3 bc_screen  = barycentric(p, castV2(x,y));
            if (bc_screen.x >= 0 && 
                bc_screen.y >= 0 && 
                bc_screen.z >= 0)
            {
                bitmap_set(bitmap, x, y, color);
            }
        } 
    }
}

internal void
tiny_renderer_main(v2i dim, u32 *data, Model *model)
{
    Bitmap bitmap_value = {.data=data, .dim=dim, .pitch=4*dim.x};
    Bitmap *bitmap = &bitmap_value;
   
    // NOTE(kv): clear screen
    tr_rectangle2i(bitmap, rect2i{.p0={0,0}, .p1=dim}, black_argb);
    
    {// NOTE(kv): outline
        f32 X = (f32)dim.x-1;
        f32 Y = (f32)dim.y-1;
        tr_line(bitmap, 0,0, X,0, red_argb);
        tr_line(bitmap, 0,0, 0,Y, red_argb);
        tr_line(bitmap, X,0, X,Y, red_argb);
        tr_line(bitmap, 0,Y, X,Y, red_argb);
    }
   
    b32 draw_mesh = def_get_config_b32(vars_intern_lit("draw_mesh"));
    if (draw_mesh)
    {
        for (int facei=0; 
             facei < arrlen(model->faces);
             facei++) 
        { 
            i32 *face = model->faces[facei]; 
            for_i32 (verti,0,3)
            { 
                v3 v0 = model->vertices[face[verti]];
                v3 v1 = model->vertices[face[(verti+1) % 3]];
                // NOTE: the model coordinates are in clip space
                f32 x0 = unilateral(v0.x)*(f32)dim.x;
                f32 y0 = unilateral(v0.y)*(f32)dim.y;
                f32 x1 = unilateral(v1.x)*(f32)dim.x;
                f32 y1 = unilateral(v1.y)*(f32)dim.y;
                tr_line(bitmap, x0,y0, x1,y1, white_argb);
            } 
        }
    }
  
    // NOTE: triangle nonsense who gives a *
    if (0)
    {
        v2 t0[3] = {V2(10, 70),   V2(50, 160),  V2(70, 80)};
        v2 t1[3] = {V2(180, 50),  V2(150, 1),   V2(70, 180)};
        v2 t2[3] = {V2(180, 150), V2(120, 160), V2(130, 180)};
        tr_triangle(bitmap, t0, red_argb); 
        tr_triangle(bitmap, t1, white_argb); 
        tr_triangle(bitmap, t2, green_argb);
    }
   
    f32 dim_x = (f32)dim.x;
    f32 dim_y = (f32)dim.y;
   
    if (1)
    {
        v2 beg = v2{0,0};
        v2 mid = v2{0, dim_y-1};
        v2 end = V2(dim_x-1, dim_y-1);
        v1 step = {1.0f / dim_x};  // NOTE: I don't know what I'm doing!
        for (f32 t = 0;
             t <= 1.0f;
             t += step)
        {
            v2 p1 = lerp(beg, t, mid);
            v2 p2 = lerp(mid, t, end);
            v2 pos = lerp(p1, t, p2);
            bitmap_set(bitmap, (i32)pos.x, (i32)pos.y, white_argb);
        }
    }
}

internal v3
perspective_project_(Camera *camera, v3 point)
{
    v3 projected = matvmul3(&camera->project, point);
    // NOTE: distance is in camera_z, because d_camera_screen is also in z, and the screen_ratio is the ratio between those two distances.
    v1 depth = camera->distance - projected.z;
    v1 near_clip = 10;
    v1 far_clip  = 100 * 1E3;
    if ( in_between(near_clip, depth, far_clip) )
    {
        v2 xy = (camera->focal_length / depth) * projected.xy;
        v1 depth_clip = bilateral( unlerp(near_clip,depth,far_clip) );
        return V3(xy,depth_clip);
    }
    else
        return V3All(F32_MAX);
}

internal v3
perspective_project_nono(Camera *camera, v3 point)
{
#if 0
    return perspective_project_(camera, point);
#else
    return point;
#endif
}

internal v1
cubic_bernstein(u32 index, v1 u)
{
    v1 factor = (index == 1 || index == 2) ? 3 : 1;
    v1 result = factor * pow(u,index) * pow(1-u, 3-index);
    return result;
}

internal void
draw_cubic_bezier(App *app, Camera *camera, v3 P[4])
{
    fslider(radius_a, 23.380762);
    fslider(radius_b, 3.923108);
    
    if (0)
    {// NOTE: Control points
        for_i32 (index, 1, 3)
        {
            v3 screen_p = perspective_project_nono(camera, P[index]);
            draw_circle(app, screen_p.xy, radius_a, yellow_argb, screen_p.z);
        }
    }
    
    // NOTE: Sample points
    i32 nslices = 16;
    f32 inv_nslices = 1.0f / (f32)nslices;
    for_i32 (sample_index, 0, nslices+1)
    {
        f32 t_radius = (f32)sample_index * inv_nslices;
        f32 radius = lerp(radius_a, t_radius, radius_b);
        
        f32 u = inv_nslices * (f32)sample_index;
        f32 U = 1.0f-u;
        v3 world_p = (1*cubed(U)*P[0] + 
                      3*(u)*squared(U)*P[1] +
                      3*squared(u)*(U)*P[2] +
                      1*cubed(u)*P[3]);
        //
        v3 screen_p = perspective_project_nono(camera, world_p);
        
        v1 draw_radius;
        {// NOTE: Having to calculate the focal_length/dz twice :<
            v1 projected_z = dot(camera->axes.z, world_p);
            v1 dz = camera->distance - projected_z;
            draw_radius = absolute((camera->focal_length / dz) * radius);
        }
        
        draw_circle(app, screen_p.xy, draw_radius, gray_argb, screen_p.z);
    }
}

internal void
draw_bezier_surface(App *app, Camera *camera, v3 P[4][4], 
                    u32 active_i, u32 active_j)
{
    if (0)
    {// NOTE: draw control points
        for_u32 (i,0,4)
        {
            for_u32 (j,0,4)
            {
                v3 screen_p = perspective_project_nono(camera, P[i][j]);
                draw_circle(app, screen_p.xy, 6.0f, gray_argb, screen_p.z);
            }
        }
    }
    
    u32 nslices = 16;
    f32 inv_nslices = 1.0f / (f32)nslices;
    for_u32 (u_index, 0, nslices+1)
    {
        for_u32 (v_index, 0, nslices+1)
        {
            v1 u = inv_nslices * (v1)u_index;
            v1 v = inv_nslices * (v1)v_index;
            v3 world_p = {};
            for_u32 (i,0,4)
            {
                for_u32 (j,0,4)
                {
                    world_p += (cubic_bernstein(i,u) * cubic_bernstein(j,v) *
                                P[i][j]);
                }
            }
            v3 screen_p = perspective_project_nono(camera, world_p);
            
            v1 radius = 12.0f;
            v1 draw_radius;
            {// NOTE: calculate draw radius @Slow
                v1 projected_z = dot(camera->axes.z, world_p);
                v1 dz = camera->distance - projected_z;
                draw_radius = absolute((camera->focal_length / dz) * radius);
            }
        
            v1 lightness = 0.0f;
            {
                v1 quadrance = (squared((v1)active_i - 3*u) +
                                squared((v1)active_j - 3*v));
                if (quadrance < 1.0f)
                {
                    lightness = 1.0f - quadrance;  // so it's kinda like Lambert's light law
                }
            }
            v4 color = lerp(gray_v4, lightness, white_v4);
            draw_circle(app, screen_p.xy, draw_radius, pack_argb(color), screen_p.z);
        }
    }
}

u32 data_current_version = 5;
u32 data_magic_number = *(u32 *)"kvda";

struct Bezier
{
    v3 control_points[4];
};

struct Game_Save_Old
{
    u32 magic_number;
    u32 version;
    
    v3 camera_z;
    v1 camera_distance;
    
    Bezier bezier_curves[3];
};

struct Game_Save
{
    u32 magic_number;
    u32 version;
    
    v3 camera_z;  // @Old
    v1 camera_distance;
    
    Bezier bezier_curves[3];
    
    v1 camera_phi;
    v1 camera_theta;
    
    v3 bezier_surface[4][4];
};

internal b32
save_game(App *app, String8 save_dir, String8 save_path, Game_Save *save)
{
    Scratch_Block scratch(app);
    local_persist b32 has_done_backup = false;
    String8 backup_dir = pjoin(scratch, save_dir, "backups");
    
    b32 ok = true;
    if (!has_done_backup)
    {
        // NOTE: backup game if is_first_write_since_launched
        time_t rawtime;
        time(&rawtime);
        struct tm *timeinfo = localtime(&rawtime);
        char time_string[256];
        size_t strftime_result = strftime(time_string, sizeof(time_string), "%d_%m_%Y_%H_%M_%S", timeinfo);
        if (strftime_result == 0)
        {
            print_message(app, "strftime failed... go figure that out!\n");
            ok = false;
        }
        else
        {
            String8 backup_path = push_stringf(scratch, "%.*s/data_%s.kv", string_expand(backup_dir), time_string);
            ok = move_file(save_path, backup_path);
           
            if (ok) has_done_backup = true;
        }
        
        if (ok)
        {// NOTE: cycle out old backup files
            File_List backup_files = system_get_file_list(scratch, backup_dir);
            if (backup_files.count > 100)
            {
                u64 oldest_mtime = U64_MAX;
                String8 file_to_delete = {};
                File_Info **opl = backup_files.infos + backup_files.count;
                for (File_Info **backup = backup_files.infos;
                     backup < opl;
                     backup++)
                {
                    File_Attributes attr = (*backup)->attributes;
                    if (attr.last_write_time < oldest_mtime)
                    {
                        oldest_mtime   = attr.last_write_time;
                        file_to_delete = pjoin(scratch, backup_dir, (*backup)->filename);
                    }
                }
                b32 delete_ok = remove_file(scratch, file_to_delete);
                if (delete_ok) printf_message(app, "INFO: deleted backup file %.*s because it's too old", string_expand(file_to_delete));
                else           printf_message(app, "ERROR: failed to delete backup file %.*s", string_expand(file_to_delete));
            }
        }
    }
    
    if (ok)
    {// note: save the file
        ok = write_entire_file(scratch, save_path, save, sizeof(*save));
        if (ok)
            vim_set_bottom_text(str8lit("Saved game state!"));
        else
            printf_message(app, "Failed to write to file %.*s", string_expand(save_path));
    }
    
    return ok;
}

internal b32
is_key_newly_pressed(Key_Mod active_modifiers, Key_Mod modifiers, Key_Code keycode)
{
    if (active_modifiers == modifiers)
    {
        return (key_is_down      (keycode) &&
                key_state_changes(keycode) > 0);
    }
    else return false;
}

typedef u32 Widget_ID;

struct Widget
{
    // Widget_ID id;
    String8 name;
    
    Widget *parent;
    
    u32     children_count;
    Widget *children;
};

struct Widget_State
{
    Widget root;
    Widget *hot_item;
    b32 is_editing;
};

global const v1 widget_margin = 5.0f;

internal rect2 draw_single_widget(App *app, Widget_State *state, Widget *tree, v2 top_left);

internal rect2
draw_sibling_widgets(App *app, Widget_State *state, Widget *siblings, u32 sibling_count, v2 top_left)
{
    v2 min = top_left;
    v2 max = top_left;
    for_u32 (index,0,sibling_count)
    {
        Widget *widget = siblings + index;
        rect2 rect = draw_single_widget(app, state, widget, min);
        
        max.x = macro_max(rect.max.x, max.x);
        min.y = rect.min.y - widget_margin;
    }
    return rect2_min_max(min, max);
}

inline rect2 
draw_widget_children(App *app, Widget_State *state, Widget *parent, v2 top_left)
{
    return draw_sibling_widgets(app, state, parent->children, parent->children_count, top_left);
}

internal rect2
draw_single_widget(App *app, Widget_State *state, Widget *widget, v2 top_left)
{
    Scratch_Block scratch(app);
   
#if 0
    Face_ID face = get_face_id(app,0);
    Face_Metrics face_metrics = get_face_metrics(app, face);
    
    Fancy_Line line_value = {}; 
    Fancy_Line *line = &line_value;
    push_fancy_string(scratch, line, f_white, widget->name);
    // 
    v2 name_dim = V2(get_fancy_line_width(app, face, line),
                     face_metrics.line_height);
    // 
    draw_fancy_line(app, face, fcolor_zero(), line, widget_min);
#else
    v2 name_dim = V2(50.0f, 50.0f);
    v2 widget_min = V2(top_left.x, top_left.y-name_dim.y);
    draw_rect(app, rect2_min_dim(widget_min, name_dim), gray_argb);
#endif
   
    fslider(widget_indentation, v1{10.0f});
    v2 children_top_left = V2(widget_min.x + widget_indentation,
                              widget_min.y);
    rect2 children = draw_widget_children(app, state, widget, children_top_left);
    //
    v2 whole_max = V2(macro_max(widget_min.x + name_dim.x, children.max.x),
                      widget_min.y + name_dim.y + widget_margin);
    //
    widget_min.y = children.min.y - widget_margin;
   
    rect2 whole_box = rect2_min_max(widget_min, whole_max);
    if (widget == state->hot_item)
    {
        u32 color = yellow_argb;
        if (state->is_editing)  color = red_argb;
        v1 thickness = 2.0f;
        draw_rect_outline(app, whole_box, thickness, color);
    }
    
    return whole_box;
}

struct Game_State
{
    Arena permanent_arena;
    Widget_State widget_state;
    
    Widget *surface_widget;
    Widget *curve_widgets;
};

internal v3
point_on_sphere(i32 nsegment, v1 radius, i32 itheta, i32 iphi)
{
    v1 segment = 1/(v1)nsegment;
    v1 phi   = segment*(v1)iphi;
    v1 theta = segment*(v1)itheta;
    v2 xy = cos_turn(phi) * arm2(theta);
    return radius * V3(xy, sin_turn(phi));
}

// TODO: Input handling: how about we add a callback to look at all the events and report to the game if we would process them or not?
// TODO: If there are two panels, this function will be called twice!
internal void
game_update_and_render(App *app, View_ID view, v1 dt)
{
    local_persist b32 is_initial_frame = true;
    b32 view_active = view_is_active(app, view);
    
    
    for_i32 (keycode, 1, KeyCode_COUNT)
    {// NOTE: Enable animation if any key is down
        if ( key_is_down(keycode) )
        {
            animate_in_n_milliseconds(app, 0);
            break;
        }
    }
   
    local_persist Game_State state_value = {};
    Game_State *state = &state_value;
    if (is_initial_frame)
    {
        state->permanent_arena = make_arena_system();
    }
    Arena *permanent_arena = &state->permanent_arena;
    Widget_State *widget_state = &state->widget_state;
    
    Scratch_Block scratch(app);
    
    if (is_initial_frame)
    {// TODO: We should make this a parsed data structure, because this is torture!
        u32 WIDGET_COUNT = 4;
        
        Widget *widgets = push_array(permanent_arena, Widget, WIDGET_COUNT);
       
        Widget *root = &widget_state->root;
        *root = {str8lit("root"),0, WIDGET_COUNT,widgets};
        
        widget_state->hot_item = root->children;
        
        // NOTE: surface
        Widget *surface = widgets + 0;
        *surface        = {str8lit("surface"),root, 0,0};
        state->surface_widget = surface;
        
        // NOTE: Bezier curve
        Widget *curves = widgets + 1;
        for_u32 (curve_index,0,3)
        {
            curves[curve_index] = {str8lit("curve"),root, 0,0};
        }
        state->curve_widgets = curves;
    }
    
    Key_Mod active_mods;
    {// NOTE: Input shenanigans
        Input_Modifier_Set set = system_get_keyboard_modifiers(scratch);
        active_mods = pack_modifiers(set.mods, set.count);
    }
    
    v1 U = 1e3;  // render scale multiplier (@Cleanup push this scale down to the renderer)
    
    local_persist Game_Save save = {};
    
    local_persist String save_dir  = {};
    local_persist String save_path = {};
    
    if ( is_initial_frame )
    {// NOTE: Save data business
        String binary_dir = system_get_path(scratch, SystemPath_BinaryDirectory);
        save_dir  = pjoin(&global_permanent_arena, binary_dir, "data");
        save_path = pjoin(&global_permanent_arena, save_dir, "data.kv");
        
        String8 read_string = read_entire_file(scratch, save_path);
        Game_Save *read = (Game_Save *)read_string.str;
        if (read->magic_number == data_magic_number)
        {
            if (read->version == data_current_version)
            {
                save = *read;
            }
            else if ( read->version == (data_current_version-1) )
            {
                print_message(app, "Game data load: Converting old version to new version!\n");
                Game_Save_Old *old = (Game_Save_Old *)read;
                // NOTE(kv): Conversion code!
                save = *(Game_Save *)old;
                
                save.version = data_current_version;
                // NOTE(kv): Write back the converted file.
                save_game(app, save_dir, save_path, &save);
            }
            else print_message(app, "Game data load: Unknown version\n");
        }
        else print_message(app, "Game data load: Wrong magic number!\n");
    }
    
    if ( is_key_newly_pressed(active_mods, 0, KeyCode_Return) )
    {
        save_game(app, save_dir, save_path, &save);
    }
    
    Camera camera_value = {};
    Camera *camera = &camera_value;
    {// NOTE: Camera handling
        camera->focal_length = 2000.f;
        
        if (view_active)
        {// NOTE: Input handling
            v3 delta = dt * fui_direction_from_key_states(active_mods, KeyMod_Ctl).xyz;
            
            v1 phi   = save.camera_phi   + 0.1*delta.y;
            v1 theta = save.camera_theta + 0.1*delta.x;
            
            save.camera_phi       = clamp_between(-0.25f, phi, 0.25f);
            save.camera_theta     = cycle01(theta);
            save.camera_distance += delta.z;
            
            DEBUG_VALUE(save.camera_phi);
            DEBUG_VALUE(save.camera_theta);
        }
        
        v3 camz;
        camz.y = sin_turn(save.camera_phi);
        v1 cos_phi = cos_turn(save.camera_phi);
        v1 z_point = cos_turn(save.camera_theta);
        v1 x_point = sin_turn(save.camera_theta);
        camz.z = cos_phi * z_point;
        camz.x = cos_phi * x_point;
        
        camera->distance = U * save.camera_distance;
        
        //nono
#if 0
        z_point = 1.f;
        x_point = 0.f;
        camz = V3(0,0,1);
#endif
        
        {//setup_camera_from_data(camera, camz, distance);
            // NOTE: "camz" is normalized.
            camera->x = v3{z_point, 0, -x_point};
            camera->y = noz( cross(camz, camera->x) );
            camera->z = camz;
            
#define t camera->axes.columns
            camera->project =
            {{
                    {t[0][0], t[1][0], t[2][0]},
                    {t[0][1], t[1][1], t[2][1]},
                    {t[0][2], t[1][2], t[2][2]},
            }};
#undef t
        }
        
        DEBUG_VALUE(camera->distance);
        DEBUG_VALUE(camera->z);
    }
  
    {
        // NOTE: Setup sane math coordinate system.
        rect2 clip_box = draw_get_clip(app);
        v2 clip_dim = rect_dim(clip_box);
        v2 clip_half_dim = 0.5f * clip_dim;
        v2 layout_center = clip_box.min + clip_half_dim;
        Render_Config config = 
        {
            .offset             = layout_center,
            .y_is_up            = true,
            .is_perspective     = true,
            .camera             = *camera,
            .depth_test         = true,
            .linear_alpha_blend = true,
        };
        draw_configure(app, &config);
    }
    
    {// NOTE: Coordinate system
        v3 pO, px, py, pz;
        {
            v3 world_pO = v3{0,0,0};
            v3 world_px = U * v3{1,0,0};
            v3 world_py = U * v3{0,1,0};
            v3 world_pz = U * v3{0,0,1};
            //
            pO = perspective_project_nono(camera, world_pO);
            px = perspective_project_nono(camera, world_px);
            py = perspective_project_nono(camera, world_py);
            pz = perspective_project_nono(camera, world_pz);
        }
        
        v1 thickness = 8.0f;
        draw_line(app, pO, px, thickness, pack_argb({.5,  0,  0, 1}));
        draw_line(app, pO, py, thickness, pack_argb({ 0, .5,  0, 1}));
        draw_line(app, pO, pz, thickness, pack_argb({ 0,  .5, 1, 1}));
        
        //draw_triangle(app, pO, px, py, gray_argb);
        draw_triangle(app, pO, py, pz, gray_argb);
        //draw_triangle(app, pO, pz, px, gray_argb);
    }
    
    const u32 bezier_count = 3;
    
    if (view_active)
    {
        if ( is_key_newly_pressed(active_mods, 0, KeyCode_E) )
        {
            widget_state->is_editing = !widget_state->is_editing;
        }
        
        if (!widget_state->is_editing)
        {// NOTE: Change hot item
            Widget *parent = widget_state->hot_item->parent;
            Widget *last_child = parent->children + parent->children_count-1;
            if ( is_key_newly_pressed(active_mods, 0, KeyCode_L) )
            {
                widget_state->hot_item++;
                if (widget_state->hot_item > last_child)
                {
                    widget_state->hot_item = parent->children;
                }
            }
            if ( is_key_newly_pressed(active_mods, 0, KeyCode_H) )
            {
                widget_state->hot_item--;
                if (widget_state->hot_item < parent->children)
                {
                    widget_state->hot_item = last_child;
                }
            }
        }
    }
    
    b32 editing_active = view_active && widget_state->is_editing;
    {// NOTE: bezier curve experiment!
        for_u32 (curve_index, 0, bezier_count)
        {
            Bezier *curve = save.bezier_curves + curve_index;
            if (editing_active &&
                widget_state->hot_item == state->curve_widgets + curve_index)
            {
                v3 direction = fui_direction_from_key_states(active_mods,0).xyz;
                v3 delta = matvmul3(&camera->axes, direction) * dt;
                
                local_persist i32 active_cp_index = 0;
#define Down(N) global_key_states[KeyCode_##N] != 0
                if (Down(0)) active_cp_index = 0;
                if (Down(1)) active_cp_index = 1;
                if (Down(2)) active_cp_index = 2;
                if (Down(3)) active_cp_index = 3;
#undef Down
                curve->control_points[active_cp_index] += delta;
            }
           
            v3 control_points[4];
            {
                block_copy_array(control_points, curve->control_points);
                f32 pos_unit = 500;
                for_i32 ( index, 0, alen(control_points) )
                {
                    control_points[index] *= pos_unit;
                }
            }
            
            if(0)
            {
                // NOTE: Draw
                draw_cubic_bezier(app, camera, control_points);
            }
        }
    }
    
#if 0
    {// NOTE: Drawing our widgets
        v2 dim = V2(100,100);
        v2 at  = V2(-clip_half_dim.x, clip_half_dim.y-dim.y);
        draw_widget_children(app, widget_state, &widget_state->root, at);
    } 
#endif
    
    if (0)
    {// NOTE: Bezier surface experiment
        local_persist v3 init_control_points[4][4];
        if ( is_initial_frame )
        {
            for_u32 (i,0,4)
            {
                for_u32 (j,0,4)
                {
                    init_control_points[i][j] = U*0.4f*V3((v1)i, (v1)j, 0);
                }
            }
        }
        
        local_persist u32 active_i = 0;
        local_persist u32 active_j = 0;
        
        b32 editing_surface = (editing_active &&
                               widget_state->hot_item == state->surface_widget);
        if (editing_surface)
        {// NOTE: pick control points
            if (is_key_newly_pressed(active_mods,0,KeyCode_Tab))
            {
                active_j++;
                if (active_j > 3)
                {
                    active_j = 0;
                    active_i++;
                    if (active_i > 3) active_i = 0;
                }
            }
            else if (is_key_newly_pressed(active_mods,KeyMod_Sft,KeyCode_Tab))
            {
                active_j--;
                if (active_j > 3)
                {
                    active_j = 3;
                    active_i--;
                    if (active_i > 3) active_i = 3;
                }
            }
        }
        
        if (editing_surface)
        {// NOTE: edit direction
            v3 direction = fui_direction_from_key_states(active_mods, 0).xyz;
            v3 delta = matvmul3(&camera->axes, direction) * dt * U;
            save.bezier_surface[active_i][active_j] += delta;
        }
        
        v3 control_points[4][4];
        for_u32 (i,0,4)
        {
            for_u32 (j,0,4)
            {
                control_points[i][j] = (init_control_points[i][j] +
                                        save.bezier_surface[i][j]);
            }
        }
        draw_bezier_surface(app, camera, control_points, active_i, active_j);
    }
    
    {// NOTE: The sphere game!
        local_persist v1 active_phi   = 0.f;
        local_persist v1 active_theta = 0.f;
        
        i32 nsegment = 16;
        v1 segment = 1/(v1)nsegment;
        v1 radius = U;
#if 1
        for_i32 (iphi, -nsegment/4, nsegment/4 + 1)
        {// NOTE: Draw points
            u32 nloop = nsegment;
            if (iphi == -nsegment/4 || iphi == nsegment/4)
            {
                nloop = 1;
            }
            for_u32 (itheta, 0, nloop)
            {
                v3 world_pos = point_on_sphere(nsegment, radius, itheta, iphi);
                v3 screen_pos = perspective_project_nono(camera, world_pos);
                u32 color = gray_argb;
                draw_circle(app, screen_pos.xy, 8.f, color, screen_pos.z);
            }
        }
#endif
        
#if 1
        for_i32 (iphi, -nsegment/4, nsegment/4)
        {// NOTE: Draw quads on the surface of the sphere
            for_i32 (itheta, 0, nsegment)
            {
#define SPHERE(ITHETA, IPHI) perspective_project_nono(camera, point_on_sphere(nsegment, radius, ITHETA, IPHI))
                //
                v3 p0 = SPHERE(itheta+0, iphi+0);
                v3 p1 = SPHERE(itheta+1, iphi+0);
                v3 p2 = SPHERE(itheta+0, iphi+1);
                v3 p3 = SPHERE(itheta+1, iphi+1);
#undef SPHERE
                {
                    draw_quad(app, p0, p1, p2, p3, gray_argb);
                }
            }
        }
        v3 midpoint = perspective_project_nono(camera, U*v3{0,0,1});
        draw_point(app, midpoint, 8.0f, yellow_argb);
#endif
    }
    
    ////////////////////////////////////////////////////////////////////
    
    {
        Render_Config config = 
        {
            .offset             = v2{},
            .y_is_up            = false,
            .is_perspective     = false,
            .depth_test         = false,
            .linear_alpha_blend = false,
        };
        draw_configure(app, &config);
    }
    
    block_zero_array(global_game_key_state_changes);
    is_initial_frame = false;
}
