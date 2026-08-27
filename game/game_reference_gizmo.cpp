//-NOTE(kv) Reference edit mode: drag the active preset's reference image directly in the
// viewport instead of poking its slider through the FUI or the debug channel. Entered
// from the right-click menu, left with Esc. Body drag translates, the +u+v corner handle
// scales uniformly; mirror and alpha live in the right-click menu (no keybindings).
// Drags mutate the @Reference_Placement slider in place; the values file is written once
// on release, not per mouse-move.
// Plan: ~/notes/tasks/autodraw_draw_as_data/plan-reference-image-adjust.md

struct Reference_Plane
{// NOTE(kv) The image quad's own frame, in WORLD space: origin at center, u along x_axis
 // (signed -- a mirrored image has u pointing the other way), v along y. half_v comes from
 // the texture's aspect ratio, the same way the renderer builds the quad (search RET_Image).
 v3 center;
 v3 u_axis;
 v3 v_axis;
 v1 half_u;
 v1 half_v;
 // NOTE(kv) A Reference_Placement is authored in Bone_References space, not world space
 // -- that's the bone show_reference_images draws it under. Kept here so drags can push
 // world-space deltas back into the placement's own space.
 mat4i world_from_bone;
};

function Slider *
find_reference_placement_slider()
{// NOTE(kv) One reference is editable at a time (plan Q3): the sole driver-side
 // Reference_Placement slider.
 sarray(FUI_File_Data) files = get_file_array({1, 0});
 for_i32(file_index, 1, files.count)
 {
  for_each(slider, files[file_index].sliders)
  {
   if(type_info_equals(slider->type, Reference_Placement)){ return slider; }
  }
 }
 return 0;
}

function Reference_Placement *
get_reference_placement(Game_State *state, Stringz *out_filename)
{// NOTE(kv) Null unless the active preset actually draws a placement-slider reference.
 Viewport &viewport = state->viewports[0];
 if(viewport.reference_preset == Preset_None){ return 0; }
 Driver_API *driver = &state->driver_api;
 if(not is_valid(driver)){ return 0; }
 Slider *slider = find_reference_placement_slider();
 if(slider == 0){ return 0; }
 Reference_Preset_Data data = driver->driver_get_reference_preset_data(viewport.reference_preset);
 *out_filename = data.image.filename;
 return (Reference_Placement *)slider->value;
}

function b32
get_reference_plane(Reference_Placement &placement, Stringz filename, Reference_Plane *out)
{
 v1 half_u = lengthof(placement.x_axis);
 if(half_u < 1e-5f){ return false; }
 v2 image_size = {};
 if(not is_valid(ed_load_image(filename, &image_size))){ return false; }
 if(image_size.x <= 0.f){ return false; }
 // NOTE(kv) Bone_References is a uniform scale (head_radius_world), so lifting the
 // placement to world space is one matrix apply plus one scalar on the half-extents.
 mat4i world_from_bone = get_bone(mk_bone_id(Bone_References),
                                 /*is_right*/false)->world_from_bone;
 v1 bone_scale = get_xscale(world_from_bone.m);
 *out = {
  .center = mat4vert(world_from_bone.m, placement.center),
  .u_axis = noz(mat4vec(world_from_bone.m, placement.x_axis)),
  .v_axis = noz(mat4vec(world_from_bone.m, V3y(1.f))),
  .half_u = bone_scale * half_u,
  .half_v = bone_scale * half_u * image_size.y / image_size.x,
  .world_from_bone = world_from_bone,
 };
 return true;
}

function b32
hit_reference_plane(Reference_Plane &plane, Camera &camera, Live_Viewport *viewport,
                    i2 mouse_p, v2 *out_uv)
{// NOTE(kv) Same pixel->camera-ray construction as get_primitive_hit_by_mouse, but kept
 // in world space (the plane is world data), then expressed in the quad's (u,v) frame.
 v2 mouse_px = V2(mouse_p) - get_center(viewport->clip_box);
 v3 mouse_cam = V3(mouse_px / default_meter_to_pixel, -tweaks->focal_length);
 mouse_cam.y *= -1.f;

 v3 ray_o = get_world_pos(camera);
 v3 ray_dir = mat4vec(camera.world_from_cam, noz(mouse_cam));
 v3 normal = cross(plane.u_axis, plane.v_axis);
 v1 denominator = dot(ray_dir, normal);
 if(absolute(denominator) < 1e-5f){ return false; }
 v1 t = dot(plane.center - ray_o, normal) / denominator;
 if(t <= 0.f){ return false; }

 v3 hit = ray_o + t*ray_dir;
 *out_uv = V2(dot(hit - plane.center, plane.u_axis),
              dot(hit - plane.center, plane.v_axis));
 return true;
}

myinline v1
reference_corner_grab_radius(Reference_Plane &plane)
{
 return 0.15f * minimum(plane.half_u, plane.half_v);
}

function void
update_reference_edit(Game_State *state, Mouse_State mouse, Live_Viewport *mouse_viewport)
{
 Reference_Edit_State &edit = state->reference_edit;
 if(not edit.active)
 {
  edit.drag = Reference_Drag_None;
  return;
 }

 Stringz filename = {};
 Reference_Placement *placement = get_reference_placement(state, &filename);
 Reference_Plane plane = {};
 if(placement == 0 or mouse_viewport == 0 or
    not get_reference_plane(*placement, filename, &plane))
 {
  edit.drag = Reference_Drag_None;
  return;
 }

 Camera camera = setup_camera(state->viewports[0].camera);
 v2 uv = {};
 b32 on_plane = hit_reference_plane(plane, camera, mouse_viewport, mouse.p, &uv);

 if(mouse.release_left and edit.drag != Reference_Drag_None)
 {
  edit.drag = Reference_Drag_None;
  save_slider_values_file(state, /*is_driver*/1);
 }

 if(mouse.press_left and on_plane)
 {
  v1 grab_radius = reference_corner_grab_radius(plane);
  b32 on_corner = (absolute(uv.x - plane.half_u) < grab_radius and
                   absolute(uv.y - plane.half_v) < grab_radius);
  if(on_corner)
  {
   edit.drag        = Reference_Drag_Corner;
   edit.grab_u      = uv.x;
   edit.grab_x_axis = placement->x_axis;
  }
  else if(absolute(uv.x) <= plane.half_u and absolute(uv.y) <= plane.half_v)
  {
   edit.drag        = Reference_Drag_Body;
   edit.grab_offset = uv;
  }
 }

 if(edit.drag != Reference_Drag_None and mouse.left and on_plane)
 {
  switch(edit.drag)
  {
   case Reference_Drag_Body:
   {// NOTE(kv) Keep the grabbed point under the cursor: the center moves by however
    // much the (u,v) hit drifted from where the grab started.
    v2 delta = uv - edit.grab_offset;
    v3 world_delta = delta.x*plane.u_axis + delta.y*plane.v_axis;
    placement->center += mat4vec(plane.world_from_bone.inv, world_delta);
   }break;

   case Reference_Drag_Corner:
   {// NOTE(kv) Uniform scale: the corner follows the cursor's u. Sign is untouched, so
    // dragging past the center doesn't flip the image (that's the Mirror menu item).
    if(absolute(edit.grab_u) > 1e-4f)
    {
     v1 scale = uv.x / edit.grab_u;
     if(scale > 1e-2f){ placement->x_axis = scale * edit.grab_x_axis; }
    }
   }break;

   case Reference_Drag_None: break;
  }
 }
}

function void
draw_reference_edit_gizmo(Game_State *state, Camera &camera)
{// NOTE(kv) Overlay only -- poly3_inner, not draw_line, so the gizmo never enters the
 // recorded primitive stream (it isn't drawing, it's UI).
 if(not state->reference_edit.active){ return; }

 Stringz filename = {};
 Reference_Placement *placement = get_reference_placement(state, &filename);
 Reference_Plane plane = {};
 if(placement == 0 or not get_reference_plane(*placement, filename, &plane)){ return; }

 v3 u = plane.half_u * plane.u_axis;
 v3 v = plane.half_v * plane.v_axis;
 v3 corners[4] = {
  plane.center - u - v, plane.center + u - v,
  plane.center + u + v, plane.center - u + v,
 };

 // NOTE(kv) Screen-space width, same trick as @draw_cursor: the `distance/focal_length`
 // factor makes the millimeters count on the film plane, not in the world, so the
 // outline keeps its on-screen weight at any zoom. Below ~4mm the edge quads land
 // between pixel centers and vanish entirely.
 v1 distance = lengthof(mat4vert(camera.cam_from_world, plane.center));
 v1 thickness = 4*millimeter * distance / camera.focal_length;

 auto draw_edge = [&](v3 a, v3 b, argb color) -> void
 {
  v3 along = noz(b - a);
  v3 across = thickness * noz(cross(along, camera.z));
  v3 quad[4] = { a - across, b - across, b + across, a + across };
  v3 first[3]  = { quad[0], quad[1], quad[2] };
  v3 second[3] = { quad[0], quad[2], quad[3] };
  poly3_inner(mk_poly3(first),  repeat3(color), {Poly_Overlay});
  poly3_inner(mk_poly3(second), repeat3(color), {Poly_Overlay});
 };

 for_i32(index, 0, 4)
 {
  draw_edge(corners[index], corners[(index+1) % 4], linear_argb_blue);
 }

 {// NOTE(kv) The scale handle, on the +u+v corner (@reference_corner_grab_radius).
  v1 radius = reference_corner_grab_radius(plane);
  v3 handle = corners[2];
  v3 a = handle - radius*plane.u_axis;
  v3 b = handle - radius*plane.v_axis;
  v3 triangle[3] = { handle, a, b };
  poly3_inner(mk_poly3(triangle), repeat3(linear_argb_yellow), {Poly_Overlay});
 }
}
