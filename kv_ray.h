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

inline v2
screenProject(Screen screen, v3 p)
{
  v3 d = p - screen.center;
  return v2{dot(screen.x_axis, d),
            dot(screen.y_axis, d)};
}


/*
{
    v3  &screen_N = camera_z;
    f32  screen_d = -dot(camera_p, camera_z) + d_camera_screen;
    
    auto pO_hit = raycastPlane(camera_p, pO - camera_p, screen_N, screen_d);
    auto px_hit = raycastPlane(camera_p, px - camera_p, screen_N, screen_d);
    auto py_hit = raycastPlane(camera_p, py - camera_p, screen_N, screen_d);
    auto pz_hit = raycastPlane(camera_p, pz - camera_p, screen_N, screen_d);
    
    Screen screen = { screen_center, camera_x, camera_y };
    pO_screen = screenProject(screen, pO_hit.p);
    px_screen = screenProject(screen, px_hit.p);
    py_screen = screenProject(screen, py_hit.p);
    pz_screen = screenProject(screen, pz_hit.p);
}
*/
