/*
  Rules for the renderer:
  - When you push render entries, dimensions are specified in "meter"
  - Textures and bitmaps are srgb premultiplied-alpha
  - v4 colors are in linear space, alpha=1 (so pma doesn't matter)
  - Packed colors are rgba in memory order (i.e abgr in u32 register order)
  pma = pre-multiplied alpha
 */

struct Scene {
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

gb_internal void
draw_line(Application_Links *app, v2 p0, v2 p1, f32 thickness, v4 color)
{
  if (thickness < 2) thickness = 2;
  v2 d = noz(p1 - p0);
  v2 perpendicular = 0.5f * thickness * perp(d);
  draw_rectangle(app, p0 - perpendicular, p1 + perpendicular, color);
}

gb_internal void game_update_and_render(Application_Links *app)
{
  v2 screen_dim      = {100,100};
  v2 screen_half_dim = 0.5f * screen_dim;

  f32 E3 = 1000.f;

  {// push backdrop
    v4 bg_color = v4{0,0,0,1};
    draw_rectangle(app, -screen_half_dim, screen_half_dim, bg_color);
  }

  {
    // push canvas rect //
    v2 layout_center = {-350, 0};
    
    // push coordinate system
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
      draw_line(app, p0_screen+O, px_screen+O, 2.f, {.2,  0,  0, 1});
      draw_line(app, p0_screen+O, py_screen+O, 2.f, { 0, .2,  0, 1});
      draw_line(app, p0_screen+O, pz_screen+O, 2.f, { 0,  0, .2, 1});
      
      // pushDebugText("eye_p: %2.f, %2.f, %2.f", eye_p.x, eye_p.y, eye_p.z);
      // v3 px = px_hit.p;
      // pushDebugText("p0_hit: %.1f, %.1f, %.1f", p0_hit.p.x, p0_hit.p.y, p0_hit.p.z);
      // pushDebugText("p0_screen: %.1f, %.1f", p0_screen.x, p0_screen.y);
    }
  }
}