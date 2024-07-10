 {// TODO: wtf does any of this mean?
  glGenBuffers(1, &uniform_buffer);
  glBindBuffer(GL_UNIFORM_BUFFER, uniform_buffer);
  GLuint slot     = 0; // similar stuff to glActiveTexture(GL_TEXTURE0)
  GLuint location = 0; // this is value from binding attribute in GLSL shader
  glBindBufferBase(GL_UNIFORM_BUFFER, location, uniform_buffer);
 }
 
#if 0
{
 glUniformBlockBinding(program, /*buffer location*/0, /*slot*/0);
 glBindBufferBase(GL_UNIFORM_BUFFER, /*buffer location*/0, uniform_buffer);
 glBufferData(GL_UNIFORM_BUFFER, sizeof(uniforms), &uniforms, GL_DYNAMIC_DRAW); // adjust GL_DYNAMIC_DRAW as needed
}
#endif

#if IS_FIRST_PASS
vs_out.half_thickness = uniform_meter_to_pixel*vattr_half_thickness;

{// NOTE: computing radius_clip to be added to the center in geometry shader
 v1 radius = vattr_uvw.z;
 v3 camx = uniform_camera_axes[0];
 v3 camy = uniform_camera_axes[1];
 radius_clip = (viewT * V4(radius*(camx+camy), 0.f)).xy;
}
#endif

{
 // ref: How to read from multisample framebufer https://www.khronos.org/opengl/wiki/GL_EXT_framebuffer_multisample
 const i1 W = 1920;
 const i1 H = 1080;
 {
  i2 mouse = {0,0};
  glBindFramebuffer(GL_READ_FRAMEBUFFER, game_framebuffer);
  glReadBuffer(GL_COLOR_ATTACHMENT1);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, readback_framebuffer);
  glDrawBuffer(GL_COLOR_ATTACHMENT0);
  // https://www.khronos.org/opengl/wiki/Framebuffer#Blitting
  // If you perform a blit operation and and at least one of the framebuffers is multisampled, then the source and destination sizes must be the same. That is, you cannot do multisampled blits and rescaling at the same time.
  glBlitFramebuffer(0,0,W,H,
                    //mouse.x,mouse.y, mouse.x+W,mouse.y+H,
                    0,0,W,H,
                    GL_COLOR_BUFFER_BIT, GL_NEAREST);
 }
 
}