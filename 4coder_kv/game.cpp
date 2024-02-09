/*
  NOTE(kv): OLD Rules for the renderer:
  - When you push render entries, dimensions are specified in "meter"
  - Textures and bitmaps are srgb premultiplied-alpha
  - v4 colors are in linear space, alpha=1 (so pma doesn't matter)
  - Packed colors are rgba in memory order (i.e abgr in u32 register order)
  pma = pre-multiplied alpha
 */

#include "tr_model.cpp"

v4 warm_yellow = v4{.5, .5, .3, 1};

struct RaycastOutput
{
  b32 hit;
  v3  p;
};

RaycastOutput
raycastPlane(v3 ray_origin, v3 ray_direction, v3 plane_N, f32 plane_d)
{
    RaycastOutput out = {};
    f32 denom = dot(plane_N, ray_direction);
    if (absolute(denom) >= .0001f) 
    {
        f32 t = -(plane_d + dot(plane_N, ray_origin)) / denom;
        if (t >= 0)
        {
            out.hit = true;
            out.p = ray_origin + t*ray_direction;
        }
    }
    return out;
}

RaycastOutput
screencast(v3 ray_origin, v3 ray_direction, v3 plane_N, f32 plane_d)
{
  RaycastOutput out = {};
  f32 denom = dot(plane_N, ray_direction);
  if (absolute(denom) >= .0001f) {
    f32 t = -(plane_d + dot(plane_N, ray_origin)) / denom;
    if (t >= 0) {
      out.p = ray_origin + t*ray_direction;
    }
  }
  return out;
}

struct Screen {
  v3 center;
  v3 x_axis;
  v3 y_axis;
};

inline v2
screenProject(Screen screen, v3 p)
{
  v3 d = p - screen.center;
  return v2{dot(screen.x_axis, d),
            dot(screen.y_axis, d)};
}

internal void
bitmap_set(Bitmap *bitmap, i32 x, i32 y, u32 color)
{
    y = bitmap->dim.y-1 - y; // inverting y
    kv_assert(in_between(0,y,bitmap->dim.y-1));
    
    i32 index = y * bitmap->dim.x + x;
    bitmap->data[index] = color;
}

inline u64
get_bitmap_size(Bitmap *bitmap)
{
    return bitmap->dim.y * bitmap->pitch;
}

internal void
tr_rectangle2i(Bitmap *bitmap, rect2i rect, v4 color)
{
    v2i dim = rect2i_get_dim(rect);
    for_i32 (y, rect.min.x, dim.y)
    {
        for_i32 (x, rect.min.x, dim.x)
        {
            bitmap_set(bitmap, x, y, pack_argb(color));
        }
    }
}

internal void 
tr_line(Bitmap *bitmap, 
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
    {// make it left to right
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

inline void 
tr_linev(Bitmap *bitmap,  v2i p0, v2i p1,  ARGB_Color color)
{
    tr_line(bitmap, p0.x, p0.y, p1.x, p1.y, color);
}

inline void 
tr_linev2(Bitmap *bitmap,  v2 p0, v2 p1,  ARGB_Color color)
{
    tr_line(bitmap, (i32)p0.x, (i32)p0.y, (i32)p1.x, (i32)p1.y, color);
}

global ARGB_Color red   = pack_argb({1,0,0,1});
global ARGB_Color green = pack_argb({0,1,0,1});
global ARGB_Color blue  = pack_argb({0,0,1,1});
global ARGB_Color black = pack_argb({0,0,0,1});
global ARGB_Color white = pack_argb({1,1,1,1});

internal void
tr_triangle(Bitmap *bitmap, v2i p0i, v2i p1i, v2i p2i, ARGB_Color color) 
{
    v2 p0 = castV2(p0i);
    v2 p1 = castV2(p1i);
    v2 p2 = castV2(p2i);
    
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
        for_i32 ( x, (i32)Ax, (i32)Bx+1 )
        { 
            bitmap_set(bitmap, x, y, color); // attention, due to int casts t0.y+i != A.y 
        } 
 
    }
    
    b32 draw_outline = 0;
    if (draw_outline)
    {
        tr_linev2(bitmap, (p1), (p2), red); 
        tr_linev2(bitmap, (p0), (p2), green); 
        tr_linev2(bitmap, (p0), (p1), blue); 
    }
}

internal void
tiny_renderer_main(v2i dim, u32 *data, Model *model)
{
    Bitmap bitmap_value = {.data=data, .dim=dim, .pitch=4*dim.x};
    Bitmap *bitmap = &bitmap_value;
    
    {// NOTE(kv): outline
        i32 X = dim.x-1;
        i32 Y = dim.y-1;
        tr_linev(bitmap, {0,0}, {X,0}, red);
        tr_linev(bitmap, {0,0}, {0,Y}, red);
        tr_linev(bitmap, {X,0}, {X,Y}, red);
        tr_linev(bitmap, {0,Y}, {X,Y}, red);
    }
   
    b32 draw_mesh = def_get_config_b32(vars_save_string_lit("draw_mesh"));
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
                // NOTE: the model coordinates are in clip space?
                i32 x0 = i32(unilateral(v0.x)*(f32)dim.x);
                i32 y0 = i32(unilateral(v0.y)*(f32)dim.y);
                i32 x1 = i32(unilateral(v1.x)*(f32)dim.x);
                i32 y1 = i32(unilateral(v1.y)*(f32)dim.y);
                tr_linev(bitmap, {x0, y0}, {x1, y1}, white);
            } 
        }
    }
    
    v2i t0[3] = {V2i(10, 70),   V2i(50, 160),  V2i(70, 80)};
    v2i t1[3] = {V2i(180, 50),  V2i(150, 1),   V2i(70, 180)};
    v2i t2[3] = {V2i(180, 150), V2i(120, 160), V2i(130, 180)};
    tr_triangle(bitmap, t0[0], t0[1], t0[2], red); 
    tr_triangle(bitmap, t1[0], t1[1], t1[2], white); 
    tr_triangle(bitmap, t2[0], t2[1], t2[2], green);
}

internal void
game_update_and_render(FApp *app, View_ID view, rect2 region)
{
    v4 whitev4 = {1, 1, 1, 1};
    v4 grayv4  = {.5, .5, .5, 1};
    v4 blackv4 = {0,0,0,1};
    
    v2 screen_dim      = rect2_dim(region);
    v2 screen_half_dim = 0.5f * screen_dim;
    
    {// push backdrop
        v4 bg_color = blackv4;
        draw_rect(app, rect2_min_max(v2{0, 0}, screen_dim), bg_color);
    }
   
    {// NOTE(kv): tiny renderer
        v3i dim = {256, 256, 1};
        local_persist b32 initial = true;
        local_persist u32 *data = 0;
        local_persist Texture_ID game_texture = {};
        Model model = {};
        b32 draw_mesh = def_get_config_b32(vars_save_string_lit("draw_mesh"));
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
        
        v2 position = {10,10};
        f32 scale = fslider( 0.819921 );
        v2 draw_dim = scale * castV2(dim.x, dim.y);
        draw_textured_rect(app, rect2_min_dim(position, draw_dim));
    }
    
    f32 E3 = 1000.f;
    
    { // push coordinate system
        v2 layout_center = screen_half_dim;
        v3 eye_p = E3 * v3{-1, -.2f, 3};
        v3 eye_z = noz(eye_p);  // NOTE: z comes at you
        v3 eye_x = cross(eye_z, v3{0,0,1});
        v3 eye_y = cross(eye_z, eye_x);
        
        f32 d_eye_screen = 2000.f;  // NOTE: based on real life distance
        v3 screen_center = eye_p - d_eye_screen * eye_z;
        
        v3 p0 = E3 * v3{0,0,0};
        v3 px = E3 * v3{1,0,0};
        v3 py = E3 * v3{0,1,0};
        v3 pz = E3 * v3{0,0,1};
        
        v3  &screen_N = eye_z;
        f32  screen_d = -dot(eye_p, eye_z) + d_eye_screen;
        
        auto p0_hit = raycastPlane(eye_p, p0 - eye_p, screen_N, screen_d);
        auto px_hit = raycastPlane(eye_p, px - eye_p, screen_N, screen_d);
        auto py_hit = raycastPlane(eye_p, py - eye_p, screen_N, screen_d);
        auto pz_hit = raycastPlane(eye_p, pz - eye_p, screen_N, screen_d);
        
        if (p0_hit.hit && px_hit.hit && py_hit.hit && pz_hit.hit)
        {
            Screen screen = {screen_center, eye_x, eye_y};
            v2 p0_screen = screenProject(screen, p0_hit.p);
            v2 px_screen = screenProject(screen, px_hit.p);
            v2 py_screen = screenProject(screen, py_hit.p);
            v2 pz_screen = screenProject(screen, pz_hit.p);
            auto &O = layout_center;
            draw_line(app, p0_screen+O, px_screen+O, 2.f, {.5,  0,  0, 1});
            draw_line(app, p0_screen+O, py_screen+O, 2.f, { 0, .5,  0, 1});
            draw_line(app, p0_screen+O, pz_screen+O, 2.f, { 0,  0, .5, 1});
        }
    }
}
