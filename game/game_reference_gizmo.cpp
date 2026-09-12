//-NOTE(kv) Reference edit mode: drag the active preset's reference image and the
// reference skull directly in the viewport instead of poking their sliders through the
// FUI or the debug channel. Entered from the right-click menu, left with Esc. Body drag
// translates, the +u+v corner handle scales uniformly; mirror and alpha (image) and roll
// (skull) live in the right-click menu (no keybindings). Skull only: Shift-drag on the
// body = yaw/pitch. Drags mutate the @Reference_Placement / @Reference_Mesh_Placement
// slider in place; the values file is written once on release, not per mouse-move.
// Plans: ~/notes/tasks/autodraw_draw_as_data/plan-reference-image-adjust.md,
//        ~/notes/tasks/autodraw_draw_as_data/plan-reference-skull.md (Q7)

struct Reference_Plane
{// NOTE(kv) The image quad's own frame, in WORLD space: origin at center, u along x_axis
 // (signed -- a mirrored image has u pointing the other way), v along y. half_v comes from
 // the texture's aspect ratio, the same way the renderer builds the quad (search RET_Image).
 // For the skull it's a camera-facing square around the bounding sphere (u,v = camera
 // x,y; half_u = half_v = world radius), so the same hit test and gestures apply.
 v3 center;
 v3 u_axis;
 v3 v_axis;
 v1 half_u;
 v1 half_v;
 // NOTE(kv) A placement is authored in bone space (Bone_References for the image,
 // Bone_Head for the skull), not world space. Kept here so drags can push world-space
 // deltas back into the placement's own space.
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
 Reference_Scene scene = active_preset_row(state).scene;
 if(scene == Scene_None){ return 0; }
 Driver_API *driver = &state->driver_api;
 if(not is_valid(driver)){ return 0; }
 Slider *slider = find_reference_placement_slider();
 if(slider == 0){ return 0; }
 Reference_Scene_Data data = driver->driver_get_scene_data(scene);
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

//-Reference skull

function Reference_Mesh_Placement *
get_reference_mesh_placement(Game_State *state)
{// NOTE(kv) The sole driver-side Reference_Mesh_Placement slider (the skull), null until
 // the driver has drawn the mesh once (that's when its bounding radius gets published).
 // Reads state, not the painter: the painter is null outside call_driver_render.
 if(state->reference_mesh_obj_radius <= 0.f){ return 0; }
 if(state->reference_mode == Reference_Off){ return 0; }
 sarray(FUI_File_Data) files = get_file_array({1, 0});
 for_i32(file_index, 1, files.count)
 {
  for_each(slider, files[file_index].sliders)
  {
   if(type_info_equals(slider->type, Reference_Mesh_Placement))
   {
    return (Reference_Mesh_Placement *)slider->value;
   }
  }
 }
 return 0;
}

myinline v1
reference_mesh_effective_scale(Reference_Mesh_Placement &placement)
{// NOTE(kv) Mirrors draw_reference_mesh: a fresh slider (scale 0) draws at 1.
 return (placement.scale <= 0.f) ? 1.f : placement.scale;
}

function v3
reference_mesh_bone_center(Game_State *state, Reference_Mesh_Placement &placement)
{// NOTE(kv) The skull's bbox center in bone space: draw_reference_mesh's transform
 // (center + scale*rotate(obj)) applied to the obj-space bbox center.
 mat4i bone_from_obj = (mat4i_translate(placement.center) *
                        mat4i_scale(reference_mesh_effective_scale(placement)) *
                        mat4i_rotate_tpr(placement.rotation.x, placement.rotation.y,
                                         placement.rotation.z));
 return mat4vert(bone_from_obj, state->reference_mesh_obj_center);
}

function void
set_reference_mesh_scale_rotation(Game_State *state, Reference_Mesh_Placement &placement,
                                  v1 scale, v3 rotation)
{// NOTE(kv) The placement scales and rotates about the OBJ ORIGIN, which is ~250 obj
 // units away from the skull; a naive scale/rotation change flings the skull across the
 // screen (found 2026-09-12: a 1.3x corner drag moved the center 1300 px). So every
 // scale/rotation edit goes through here and re-anchors `center` so the bbox center
 // stays where it was.
 v3 anchor = reference_mesh_bone_center(state, placement);
 placement.scale    = scale;
 placement.rotation = rotation;
 placement.center  += anchor - reference_mesh_bone_center(state, placement);
}

function b32
get_reference_mesh_plane(Game_State *state, Reference_Mesh_Placement &placement,
                         Camera &camera, Reference_Plane *out)
{// NOTE(kv) Camera-facing square around the skull's bounding sphere. The skull is drawn
 // under the LEFT Bone_Head (show_reference_images), so that's the bone to lift through.
 mat4i world_from_bone = get_bone(mk_bone_id(Bone_Head), /*is_right*/false)->world_from_bone;
 v1 bone_scale = get_xscale(world_from_bone.m);
 v1 radius = bone_scale * reference_mesh_effective_scale(placement) *
             state->reference_mesh_obj_radius;
 if(radius < 1e-5f){ return false; }
 v3 bone_center = reference_mesh_bone_center(state, placement);
 *out = {
  .center = mat4vert(world_from_bone.m, bone_center),
  .u_axis = camera.x,
  .v_axis = camera.y,
  .half_u = radius,
  .half_v = radius,
  .world_from_bone = world_from_bone,
 };
 return true;
}

//-

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

myinline b32
reference_plane_on_corner(Reference_Plane &plane, v2 uv)
{
 v1 grab_radius = reference_corner_grab_radius(plane);
 return (absolute(uv.x - plane.half_u) < grab_radius and
         absolute(uv.y - plane.half_v) < grab_radius);
}

myinline b32
reference_plane_on_body(Reference_Plane &plane, v2 uv)
{
 return (absolute(uv.x) <= plane.half_u and absolute(uv.y) <= plane.half_v);
}

// NOTE(kv) Shift-drag yaw/pitch: this many pixels of mouse travel = one full turn.
global v1 reference_mesh_px_per_turn = 800.f;

function void
update_reference_edit(Game_State *state, Mouse_State mouse, b32 shift,
                      Live_Viewport *mouse_viewport)
{
 Reference_Edit_State &edit = state->reference_edit;
 if(not edit.active or mouse_viewport == 0)
 {
  edit.drag = Reference_Drag_None;
  return;
 }

 Camera camera = setup_camera(state->viewports[0].camera);

 Stringz filename = {};
 Reference_Placement *placement = get_reference_placement(state, &filename);
 Reference_Plane plane = {};
 b32 has_plane = (placement and get_reference_plane(*placement, filename, &plane));
 v2 uv = {};
 b32 on_plane = (has_plane and hit_reference_plane(plane, camera, mouse_viewport, mouse.p, &uv));

 Reference_Mesh_Placement *mesh = get_reference_mesh_placement(state);
 Reference_Plane mesh_plane = {};
 b32 has_mesh = (mesh and get_reference_mesh_plane(state, *mesh, camera, &mesh_plane));
 v2 mesh_uv = {};
 b32 on_mesh_plane = (has_mesh and
                      hit_reference_plane(mesh_plane, camera, mouse_viewport, mouse.p, &mesh_uv));

 if(not has_plane and not has_mesh)
 {
  edit.drag = Reference_Drag_None;
  return;
 }

 if(mouse.release_left and edit.drag != Reference_Drag_None)
 {
  edit.drag = Reference_Drag_None;
  save_slider_values_file(state, /*is_driver*/1);
 }

 if(mouse.press_left)
 {// NOTE(kv) The skull wins over the image: it's the smaller target and sits in front
  // of the reference plane from every useful angle.
  if(on_mesh_plane and reference_plane_on_corner(mesh_plane, mesh_uv))
  {
   edit.drag       = Reference_Drag_Mesh_Corner;
   edit.grab_u     = mesh_uv.x;
   edit.grab_scale = reference_mesh_effective_scale(*mesh);
  }
  else if(on_mesh_plane and reference_plane_on_body(mesh_plane, mesh_uv))
  {
   if(shift)
   {
    edit.drag          = Reference_Drag_Mesh_Rotate;
    edit.grab_px       = V2(mouse.p);
    edit.grab_rotation = mesh->rotation;
   }
   else
   {
    edit.drag        = Reference_Drag_Mesh_Body;
    edit.grab_offset = mesh_uv;
   }
  }
  else if(on_plane and reference_plane_on_corner(plane, uv))
  {
   edit.drag        = Reference_Drag_Corner;
   edit.grab_u      = uv.x;
   edit.grab_x_axis = placement->x_axis;
  }
  else if(on_plane and reference_plane_on_body(plane, uv))
  {
   edit.drag        = Reference_Drag_Body;
   edit.grab_offset = uv;
  }
 }

 if(edit.drag != Reference_Drag_None and mouse.left)
 {
  switch(edit.drag)
  {
   case Reference_Drag_Body:
   {// NOTE(kv) Keep the grabbed point under the cursor: the center moves by however
    // much the (u,v) hit drifted from where the grab started.
    if(not on_plane){ break; }
    v2 delta = uv - edit.grab_offset;
    v3 world_delta = delta.x*plane.u_axis + delta.y*plane.v_axis;
    placement->center += mat4vec(plane.world_from_bone.inv, world_delta);
   }break;

   case Reference_Drag_Corner:
   {// NOTE(kv) Uniform scale: the corner follows the cursor's u. Sign is untouched, so
    // dragging past the center doesn't flip the image (that's the Mirror menu item).
    if(not on_plane){ break; }
    if(absolute(edit.grab_u) > 1e-4f)
    {
     v1 scale = uv.x / edit.grab_u;
     if(scale > 1e-2f){ placement->x_axis = scale * edit.grab_x_axis; }
    }
   }break;

   case Reference_Drag_Mesh_Body:
   {// NOTE(kv) Same as the image, in the camera-facing plane through the skull center.
    // The plane's center moves with the drag, so the grab offset stays constant by
    // construction: each frame the cursor's drift from the offset is the new delta.
    if(not on_mesh_plane){ break; }
    v2 delta = mesh_uv - edit.grab_offset;
    v3 world_delta = delta.x*mesh_plane.u_axis + delta.y*mesh_plane.v_axis;
    mesh->center += mat4vec(mesh_plane.world_from_bone.inv, world_delta);
   }break;

   case Reference_Drag_Mesh_Corner:
   {// NOTE(kv) The square's half-extent is the world radius, so the plane grows with
    // the scale and the corner tracks the cursor exactly like the image corner does.
    if(not on_mesh_plane){ break; }
    if(absolute(edit.grab_u) > 1e-4f)
    {
     v1 factor = mesh_uv.x / edit.grab_u;
     if(factor > 1e-2f)
     {
      set_reference_mesh_scale_rotation(state, *mesh, factor * edit.grab_scale, mesh->rotation);
     }
    }
   }break;

   case Reference_Drag_Mesh_Rotate:
   {// NOTE(kv) Mouse x -> yaw (phi), mouse y -> pitch (theta), in turns; the drag
    // keeps going off the square. Not an arcball (plan Q7: only if this bites).
    v2 delta_px = V2(mouse.p) - edit.grab_px;
    v3 rotation = edit.grab_rotation;
    rotation.x += delta_px.x / reference_mesh_px_per_turn;
    rotation.y -= delta_px.y / reference_mesh_px_per_turn;
    set_reference_mesh_scale_rotation(state, *mesh, mesh->scale, rotation);
   }break;

   case Reference_Drag_None: break;
  }
 }
}

function void
draw_reference_plane_gizmo(Reference_Plane &plane, Camera &camera, argb outline_color)
{// NOTE(kv) Overlay only -- poly3_inner, not draw_line, so the gizmo never enters the
 // recorded primitive stream (it isn't drawing, it's UI).
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
  draw_edge(corners[index], corners[(index+1) % 4], outline_color);
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

function void
draw_reference_edit_gizmo(Game_State *state, Camera &camera)
{
 if(not state->reference_edit.active){ return; }

 Stringz filename = {};
 Reference_Placement *placement = get_reference_placement(state, &filename);
 Reference_Plane plane = {};
 if(placement and get_reference_plane(*placement, filename, &plane))
 {
  draw_reference_plane_gizmo(plane, camera, linear_argb_blue);
 }

 Reference_Mesh_Placement *mesh = get_reference_mesh_placement(state);
 Reference_Plane mesh_plane = {};
 if(mesh and get_reference_mesh_plane(state, *mesh, camera, &mesh_plane))
 {
  draw_reference_plane_gizmo(mesh_plane, camera, linear_argb_red);
 }
}
