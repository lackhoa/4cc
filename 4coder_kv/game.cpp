/*
  Rules for the renderer:
  - When you push render entries, dimensions are specified in "meter"
  - Textures and bitmaps are srgb premultiplied-alpha
  - v4 colors are in linear space, alpha=1 (so pma doesn't matter)
  - Packed colors are rgba in memory order (i.e abgr in u32 register order)
  pma = pre-multiplied alpha
 */

struct Scene 
{
  v3 eye_position;
};

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
  if (absolute(denom) >= .0001f) {
    f32 t = -(plane_d + dot(plane_N, ray_origin)) / denom;
    if (t >= 0) {
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
bitmap_write(Bitmap *bitmap, i32 x, i32 y, u32 color)
{
    i32 index = y * bitmap->dim.x + x;
    bitmap->data[index] = color;
}

internal void 
tr_line(Bitmap *bitmap, v2i p0, v2i p1, v4 color_v4)
{
    ARGB_Color color = pack_argb(color_v4);
    for (f32 t=0.f; 
         t < 1.0f; 
         t += 0.01f)
    {
        i32 x = p0.x + (p1.x - p0.x)*t; 
        i32 y = p0.y + (p1.y - p0.y)*t; 
        bitmap_write(bitmap, x, y, color); 
    } 
}

inline umm
get_bitmap_size(Bitmap *bitmap)
{
    return bitmap->dim.y * bitmap->pitch;
}

internal void
tiny_renderer_main(v2i dim, u32 *data)
{
    Bitmap bitmap_value = {.data=data, .dim=dim, .pitch=4*dim.x};
    Bitmap *bitmap = &bitmap_value;
    v4 black = {0,0,0,1};
    v4 red   = {1,0,0,1};
    fslider( 0.579974, 0.210499 );
    v4 color_v4 = lerp(black, fui_slider_value.x, red);
#if 0
    tr_line(bitmap, {0,0}, {99, 99}, color_v4);
#else
    for_i32 (y, 0, dim.y)
    {
        for_i32 (x, 0, dim.x)
        {
            bitmap_write(bitmap, x, y, pack_argb(red));
        }
    }
#endif
}

internal void
game_update_and_render(FApp *app, View_ID view, rect2 region)
{
    v4 white = {1, 1, 1, 1};
    v4 gray  = {.5, .5, .5, 1};
    v4 black = {0,0,0,1};
    
    v2 screen_dim      = rect_get_dim(region);
    v2 screen_half_dim = 0.5f * screen_dim;
    
    {// push backdrop
        v4 bg_color = black;
        draw_rect(app, rect_min_max(v2{0, 0}, screen_dim), bg_color);
    }
   
    { // NOTE(kv): tiny renderer
        v3i dim = {100, 100, 1};
        local_persist b32 initial = true;
        local_persist u32 *data = 0;
        local_persist Texture_ID game_texture = {};
        if (initial)
        {
            initial = false;
           
            // create game texture
            game_texture = graphics_get_texture(dim, TextureKind_ARGB);
            graphics_set_game_texture(game_texture);
           
            // allocate memory
            umm size = 4 * dim.x * dim.y * dim.z;
            data = (u32 *)malloc(size);
            block_zero(data, size);
        }
        
        tiny_renderer_main(dim.xy, data);
        graphics_fill_texture(TextureKind_ARGB, game_texture, v3i{}, dim, data);
        
        v2 position = {0,0};
        v2 draw_dim = V2f32(dim.x, dim.y);
        draw_textured_rect(app, rect_min_dim(position, draw_dim));
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
