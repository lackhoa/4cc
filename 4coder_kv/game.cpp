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
image_set(Bitmap *image, i32 x, i32 y, u32 color)
{
    image->data[y*image->pitch + x] = color;
}

internal void 
tr_line(Bitmap *image, v2i p0, v2i p1, v4 color_v4)
{
    ARGB_Color color = pack_argb(color_v4);
    i32 x0 = p0.x;
    i32 y0 = p0.y;
    i32 x1 = p1.x;
    i32 y1 = p1.y;
    for (f32 t=0.; t < 1.0f; t+=0.01f)
    {
        i32 x = x0 + (x1-x0)*t; 
        i32 y = y0 + (y1-y0)*t; 
        image_set(image, x, y, color); 
    } 
}

internal void
tiny_renderer_main(v2i dim, u32 *data)
{
    Bitmap image_value = {.data=data, .dim=dim, .pitch=4*dim.x};
    Bitmap *image = &image_value;
    v4 red = {1,0,0,1};
    fslider( 0 );
    f32 xf = lerp((f32)0, fui_slider_value.x, (f32)(dim.x-1));
    tr_line(image, {0,0}, {(i32)xf, 50}, red);
}

internal void
game_update_and_render(FApp *app, View_ID view, rect2 region)
{
    v4 white = {1, 1, 1, 1};
    v4 gray = {.5, .5, .5, 1};
    
    v2 screen_dim      = rect_get_dim(region);
    v2 screen_half_dim = 0.5f * screen_dim;
    
    {// push backdrop
        v4 bg_color = v4{0,0,0,1};
        draw_rect(app, rect_min_max(v2{0, 0}, screen_dim), bg_color);
    }
    
    { // NOTE(kv): test draw texture
        v3i game_dim = {100, 100, 1};
        local_persist b32 initial = true;
        if (initial)
        {
            auto &dim = game_dim;
            Texture_ID game_texture = graphics_get_texture(dim, TextureKind_ARGB);
            graphics_set_game_texture(game_texture);
           
            umm size = 4 * dim.x * dim.y * dim.z;
            u32 *image = (u32 *)malloc(size);
            block_zero(image, size);
            tiny_renderer_main(dim.xy, image);
            graphics_fill_texture(TextureKind_ARGB, game_texture, v3i{}, dim, image);
            free(image);
            
            initial = false;
        }
        
        v2 position = {0,0};
        v2 draw_dim = V2f32(game_dim.x, game_dim.y);
        draw_textured_rect(app, rect_min_dim(position, draw_dim), white);
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
