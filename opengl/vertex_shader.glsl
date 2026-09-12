layout(location=0) in v3  vattr_pos;
layout(location=1) in v3  vattr_uvw;
layout(location=2) in v4  vattr_color;
layout(location=3) in v1  vattr_half_thickness;
layout(location=4) in v1  vattr_depth_offset;
layout(location=5) in u32 vattr_prim_id;

out Fragment_Data
{
#if WRITE_PRIM_ID
 u32 prim_id;
#else
#  if IS_FIRST_PASS || IS_SECOND_PASS
 v4 color;
#  else
 flat v4 color;
 v3 uvw;
#  endif
#endif
} vs_out;

void main(void)
{
 v4 world_pos = uniform_object_transform*V4(vattr_pos, 1.0);
 
 gl_Position = uniform_clip_from_world * world_pos;
 
#if IS_FIRST_PASS || IS_SECOND_PASS
 if(uniform_overlay)
 {
  gl_Position.z = -gl_Position.w;  //NOTE: z=-1 IS the near-clip plane, so the shader really is the right place to do this.
 }
 else
 {
  v4 offsetted = world_pos;
  v3 camz = uniform_camera_axes[2];
  offsetted.xyz -= vattr_depth_offset * camz;
  offsetted = uniform_clip_from_world * offsetted;
  gl_Position.z = offsetted.z * gl_Position.w / offsetted.w;
 }
#  if WRITE_PRIM_ID
 vs_out.prim_id    = vattr_prim_id;
#  else
 vs_out.color.rgba = vattr_color.bgra;
#  endif
 
#else
 // NOTE: Blit image
 // NOTE(kv) Was pinned to the near plane (always on top, indicators shining through via
 // a 1e-3 offset). Since plan-reference-toggle-two-states (2026-09-12) images are
 // depth-tested like polys, offset along the camera z by vattr_depth_offset, so the
 // reference z-order can put them behind or in front of the drawing. Clamped to the
 // near plane so a big "in front" offset never gets clipped away.
 {
  v4 offsetted = world_pos;
  v3 camz = uniform_camera_axes[2];
  offsetted.xyz -= vattr_depth_offset * camz;
  offsetted = uniform_clip_from_world * offsetted;
  gl_Position.z = max(offsetted.z * gl_Position.w / offsetted.w, -gl_Position.w+1e-3);
 }
#  if WRITE_PRIM_ID
 vs_out.prim_id = vattr_prim_id;
#  else
 vs_out.uvw        = vattr_uvw;
 vs_out.color.rgba = vattr_color.bgra;
#  endif
#endif
}
