layout(location=0) in v3  vattr_pos;
layout(location=1) in v3  vattr_uvw;
layout(location=2) in v4  vattr_color;
layout(location=3) in v1  vattr_half_thickness;
layout(location=4) in v1  vattr_depth_offset;
layout(location=5) in u32 vattr_prim_id;

out Fragment_Data {
#if IS_FIRST_PASS || IS_SECOND_PASS
 
# if ACTUALLY_RENDERING
 v4 color;
# else
 u32 prim_id;
# endif
 
#else
 v3 uvw;
#endif
} vs_out;

void main(void)
{
#if IS_FIRST_PASS || IS_SECOND_PASS
 v4 world_pos = vec4(vattr_pos, 1.0);
 world_pos    = uniform_object_transform*world_pos;
 mat4 viewT = uniform_view_transform;
 
 {
  gl_Position = viewT * world_pos;
  if (uniform_overlay)
  {
   gl_Position.z = -gl_Position.w;  //NOTE: z=-1 IS the near-clip plane, so the shader really is the right place to do this.
  }
  else
  {
   v4 offsetted = world_pos;
   v3 camz = uniform_camera_axes[2];
   offsetted.xyz -= vattr_depth_offset * camz;
   offsetted = viewT * offsetted;
   gl_Position.z = offsetted.z * gl_Position.w / offsetted.w;
  }
 }
 
# if ACTUALLY_RENDERING
 vs_out.color.rgba = vattr_color.bgra;
# else
 vs_out.prim_id    = vattr_prim_id;
# endif
 
#else
 // NOTE: The csg software-rendering case
 gl_Position = vec4(vattr_pos, 1);
 vs_out.uvw = vattr_uvw;
#endif
}
