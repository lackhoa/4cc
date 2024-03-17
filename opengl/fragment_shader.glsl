// NOTE: Input
in Fragment_Data
{
#if IS_FIRST_PASS || IS_SECOND_PASS
 
#if ACTUALLY_RENDERING
 v4 color;
#else
 flat u32 prim_id;
#endif
 
#else
 v3 uvw;
#endif
} fs_in;

#if !(IS_FIRST_PASS || IS_SECOND_PASS)
layout(binding=0) uniform sampler2D csg_texture;
#endif

// NOTE: Ouput
#if ACTUALLY_RENDERING
layout(location=0) out vec4 out_color;
#else
layout(location=0) out u32  out_prim_id;
#endif

void main(void)
{
#if IS_FIRST_PASS || IS_SECOND_PASS
 
# if ACTUALLY_RENDERING
 //NOTE: Alpha testing I don't think we even use alpha
 if (fs_in.color.a == 0.0f) { discard; } // NOTE(kv): alpha testing
 out_color   = fs_in.color;
# else
 out_prim_id = fs_in.prim_id;
# endif
 
#else
 // NOTE: csg rendering
 gl_FragDepth = 0.999f;
 out_color = texture(csg_texture, fs_in.uvw.xy);
 
#endif
}
