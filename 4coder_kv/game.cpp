/*
  NOTE(kv): OLD Rules for the renderer:
  - When you push render entries, dimensions are specified in "meter"
  - Textures and bitmaps are srgb premultiplied-alpha
  - v4 colors are in linear space, alpha=1 (so pma doesn't matter)
  - Packed colors are rgba in memory order (i.e abgr in u32 register order)
  pma = pre-multiplied alpha

  NOTE(kv): NEW rules for 3D
  - Matrices are arrays of column vectors
 */

#include "tr_model.cpp"

struct Camera
{
    v3   world_p;
    union
    {
        m3x3 axes;    // transform to project the object onto the camera axes
        struct
        {
            v3 x;
            v3 y;
            v3 z;
        };
    };
    m3x3 project;
    v1   focal_length;
};

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

global ARGB_Color red   = pack_argb({1,0,0,1});
global ARGB_Color green = pack_argb({0,1,0,1});
global ARGB_Color blue  = pack_argb({0,0,1,1});
global ARGB_Color black = pack_argb({0,0,0,1});
global ARGB_Color white = pack_argb({1,1,1,1});

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
        tr_line(bitmap, (p1), (p2), red); 
        tr_line(bitmap, (p0), (p2), green); 
        tr_line(bitmap, (p0), (p1), blue); 
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
    tr_rectangle2i(bitmap, rect2i{.p0={0,0}, .p1=dim}, black);
    
    {// NOTE(kv): outline
        f32 X = (f32)dim.x-1;
        f32 Y = (f32)dim.y-1;
        tr_line(bitmap, 0,0, X,0, red);
        tr_line(bitmap, 0,0, 0,Y, red);
        tr_line(bitmap, X,0, X,Y, red);
        tr_line(bitmap, 0,Y, X,Y, red);
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
                tr_line(bitmap, x0,y0, x1,y1, white);
            } 
        }
    }
  
    // NOTE: triangle nonsense who gives a *
    if (0)
    {
        v2 t0[3] = {V2(10, 70),   V2(50, 160),  V2(70, 80)};
        v2 t1[3] = {V2(180, 50),  V2(150, 1),   V2(70, 180)};
        v2 t2[3] = {V2(180, 150), V2(120, 160), V2(130, 180)};
        tr_triangle(bitmap, t0, red); 
        tr_triangle(bitmap, t1, white); 
        tr_triangle(bitmap, t2, green);
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
            bitmap_set(bitmap, (i32)pos.x, (i32)pos.y, white);
        }
    }
}

internal v2 
perspective_project(Camera *camera, v3 point)
{
    v3 projected = matvmul3(&camera->project, point);
    // NOTE: distance is in z, because d_camera_screen is also in z, and the screen_ratio is the ratio between those two distances.
    f32 dz_point_camera = absolute(camera->world_p.z - projected.z);
    v3 result_v3 = (camera->focal_length / dz_point_camera) * projected;
    return result_v3.xy;
}

internal void
draw_cubic_bezier(App *app, Camera *camera, v3 P[4])
{
    v4 grayv4  = {.5, .5, .5, 1};
    u32 gray   = pack_argb(grayv4);
    
    i32 nslices = 16;
    f32 du = 1.0f / (f32)nslices;
    for_i32 (sample_index, 1, nslices)
    {
        f32 u = du * (f32)sample_index;
        f32 U = 1.0f-u;
        v3 world_p = (pow(U,3)*    P[0] + 
                      3*pow(U,2)*u*P[1] +
                      3*U*pow(u,2)*P[2] +
                      pow(u,3)*    P[3]);
        //
        v2 screen_p = perspective_project(camera, world_p);
        draw_circle(app, screen_p, 10, gray);
    }
}

internal void
setup_camera_from_data(Camera *camera, v3 camz, v1 distance)
{
    // NOTE: "camz" is normalized.
    camera->world_p = distance * camz;
   
    camera->x = noz( v3{-camz.y, camz.x, 0} );  // NOTE: z axis' projection should point up on the screen
    if (camera->x == v3{})
    {// NOTE: Happens when we look STRAIGHT DOWN on the target.
        camera->x = noz( v3{camz.z, 0, -camz.x} );  // NOTE: we want y axis to point up in this case
    }
    camera->y = noz( cross(camz, camera->x) );
    
#define t camera->axes.columns
    camera->project =
    {{
            {t[0][0], t[1][0], t[2][0]},
            {t[0][1], t[1][1], t[2][1]},
            {t[0][2], t[1][2], t[2][2]},
    }};
#undef t
}

struct Game_Data
{
    i32 the_number;
};

internal void
game_update_and_render(App *app, View_ID view, v1 dt, rect2 clip)
{
    v4 whitev4 = {1,1,1,1};
    v4 grayv4  = {0.5,0.5,0.5,1};
    u32 gray   = pack_argb(grayv4);
    v4 blackv4 = {0,0,0,1};
    
    v2 screen_dim      = rect2_dim(clip);
    v2 screen_half_dim = 0.5f * screen_dim;
    v2 layout_center = clip.min + screen_half_dim;
    
    draw_set_y_up(app);
    draw_set_offset(app, layout_center);
   
    v1 U = 1e3;  // distance multiplier (@Cleanup push this scale down to the renderer also)
    local_persist Game_Data game_data;
    
    local_persist b32 inited = false;
    if ( !inited )
    {// NOTE: Initialization
        // BOOKMARK
        Scratch_Block scratch(app);
        // String8 current_dir = push_exe_directory(app, scratch);
        String8 binary_dir = system_get_path(scratch, SystemPath_BinaryDirectory);
        // String8 binary_dir = path_dirname(binary_path);
        // It's not a string, surprisingly!
        String8 test_data = str8lit("new data for me to write");
        String8 filename = str8lit("data.kv");
        String8 filepath = push_stringf(scratch, "%.*s/data/%.*s", string_expand(binary_dir), string_expand(filename));
        write_entire_file(scratch, filepath, test_data.str, test_data.size);
        // File_Name_Data file_data = read_entire_file_search_up_path(scratch, hotdir, str8lit("test_data.kv"));
        inited = true;
    }
    
    Camera camera_value = {};
    Camera *camera = &camera_value;
    {// NOTE: Camera setup
        camera->focal_length = 2000.f;
        
        Fui_Item_Index camera_input_index;
        // BOOKMARK NOTE: The input combines both z and distance (distance in w)
        fslider(camera_input, v4{ 0.245445, 0.871441, 0.424672, 6.564896 }, Fui_Options{.update=fui_update_null}, &camera_input_index);
        v3 camz = camera_input.xyz;
        v1 cam_distance = U * camera_input.w;
        
        setup_camera_from_data(camera, camz, cam_distance);
       
        v3 cam_delta = {};
        if ( fui_is_active(camera_input_index) )
        {
            cam_delta = U * dt * fui_direction_from_key_states().xyz;
            
            // NOTE: align movement to the camera
            v3 adjusted_delta = matvmul3(&camera->axes, cam_delta);
            camz = noz(camera->world_p + adjusted_delta);
            cam_distance = length(camera->world_p) + cam_delta.z;
            
            setup_camera_from_data(camera, camz, cam_distance);
            
            fui_post_value(camera_input_index, V4(camz, cam_distance / U));
        }
    }

    { // push coordinate system
        v2 pO_screen, px_screen, py_screen, pz_screen;
        {
            v3 pO = v3{0,0,0};
            v3 px = U * v3{1,0,0};
            v3 py = U * v3{0,1,0};
            v3 pz = U * v3{0,0,1};
            //
            pO_screen = perspective_project(camera, pO);
            px_screen = perspective_project(camera, px);
            py_screen = perspective_project(camera, py);
            pz_screen = perspective_project(camera, pz);
        }
        
        v2 O = {};
        f32 line_thickness = 8.0f;
        draw_line(app, O+pO_screen, O+px_screen, line_thickness, pack_argb({.5,  0,  0, 1}));
        draw_line(app, O+pO_screen, O+py_screen, line_thickness, pack_argb({ 0, .5,  0, 1}));
        draw_line(app, O+pO_screen, O+pz_screen, line_thickness, pack_argb({ 0,  .5, 1, 1}));
    }    
    
    {// NOTE: bezier curve experiment!
        // BOOKMARK
        fslider( c0, v3{ -0.637700, 0.176406, 0.768833 });
        fslider( c1, v3{ -0.411237, 0.887374, 1.143159 });
        fslider( c2, v3{ 1.666014, -0.706654, 0.000000 });
        fslider( c3, v3{ 0.344152, 0.116179, 0.000000 } );
        
        f32 pos_unit = 500;
        v3 control_points[] = {c0, c1, c2, c3};
        for_i32 (index, 0, alen(control_points))
        {
            control_points[index] *= pos_unit;
        }
       
        // NOTE: Draw the control points
        fslider( radius, v1{1.0f} );
        radius *= 10.0f;
        //
        for_i32 (index, 0, 4)
        {
            v2 screen_p = perspective_project(camera, control_points[index]);
            draw_circle(app, screen_p, radius, gray);
        }
        
        draw_cubic_bezier(app, camera, control_points);
    }
     
    fslider(software_rendering_slider, 0.0f);
    if (software_rendering_slider > 0)
    {// NOTE: software rendering experiments
        v3i dim = {768, 768, 1};
        local_persist b32 initial = true;
        local_persist u32 *data = 0;
        local_persist Texture_ID game_texture = {};
        Model model = {};
        b32 draw_mesh = def_get_config_b32(vars_intern_lit("draw_mesh"));
        if (initial)
        {
            initial = false;
            if (draw_mesh)
            {
                char *filename = "C:/Users/vodan/4ed/4coder-non-source/tiny_renderer/diablo3_pose.obj";
                model = new_model(filename);
            }
            // create game texture
            game_texture = graphics_get_texture(dim, TextureKind_ARGB);
            graphics_set_game_texture(game_texture);
            // allocate memory
            umm size = 4 * dim.x * dim.y * dim.z;
            data = (u32 *)malloc(size);
            block_zero(data, size);
        }
        
        tiny_renderer_main(dim.xy, data, &model);
        graphics_fill_texture(TextureKind_ARGB, game_texture, v3i{}, dim, data);
        
        v2 position = clip.min + v2{10,10};
        fslider( scale, 0.338275f );
        v2 draw_dim = scale * castV2(dim.x, dim.y);
        draw_textured_rect(app, rect2_min_dim(position, draw_dim));
    }
    
    draw_set_y_down(app);
    draw_set_offset(app, v2{});
}
