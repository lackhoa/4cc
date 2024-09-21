// NOTE: Input
in Fragment_Data
{
#if WRITE_PRIM_ID
 u32 prim_id;
#else
#    if IS_FIRST_PASS || IS_SECOND_PASS
 v4 color;
#    else
 flat v4 color;
 v3 uvw;
#    endif
#endif
} fs_in;

#if !(IS_FIRST_PASS || IS_SECOND_PASS)
layout(binding=0) uniform sampler2D image_texture;  // ;image_texture_binding
#endif

// NOTE: Ouput
#if WRITE_PRIM_ID
layout(location=0) out u32  out_prim_id;
#else
layout(location=0) out V4 out_color;
#endif

void main(void)
{
#if IS_FIRST_PASS || IS_SECOND_PASS
 
#    if WRITE_PRIM_ID
 //NOTE: Alpha testing: I don't think we even use alpha
 //if (fs_in.color.a == 0.0f) { discard; } // NOTE(kv): alpha testing
 out_prim_id = fs_in.prim_id;
#    else
 out_color   = fs_in.color;
 if (uniform_overlay){
  //NOTE(kv) We have to set gl_FragDepth because if we don't,
  //  overlays on the same depth level wouldn't sort correctly due to precision error.
  //TODO(kv) @speed I think the right way is to just draw overlays on their own depth layer,
  //  that's the only way to make overlays respect each other's depth.
  gl_FragDepth = -1;
 } else {
  gl_FragDepth = gl_FragCoord.z;
 }
#    endif
 
#else
 // NOTE: blit image
#    if WRITE_PRIM_ID
 out_prim_id = fs_in.prim_id;
#    else
 out_color = fs_in.color * texture(image_texture, fs_in.uvw.xy);
#    endif
#endif
}
