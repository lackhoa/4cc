global GLuint DEFAULT_FRAMEBUFFER = 0;

global GLint uniform_screen_from_world      = 0;
global GLint uniform_camera_axes         = 1;
global GLint uniform_unused              = 2;
global GLint uniform_object_transform    = 3;
global GLint uniform_meter_to_pixel      = 4;
global GLint uniform_overlay             = 5;

//~Vertex attributes
// TODO: @Speed Still a hodge-podge of old vertex attributes.
// TODO: Do we even want "depth_offset" as an attribute?

global i32 VATTR_COUNT;

#define X_VERTEX_ATTRIBUTES(X) \
X(pos,           0) \
X(uvw,           1) \
X(color,         2) \
X(half_thickness,3) \
X(depth_offset,  4) \
X(prim_id,       5) \

#define X(name, location) global GLint vattr_##name = location;
X_VERTEX_ATTRIBUTES(X)
#undef X

//~

struct Loaded_Image
{
 char  *filename;
 i2     dim;
 GLuint texture;
};
//
global Loaded_Image *loaded_images;
global i32 image_load_failure_count;

function Image_Load_Info
get_image_load_info(void) 
{
 return Image_Load_Info
 {
  i32(arrlen(loaded_images)),
  image_load_failure_count,
 };
}

//~

inline void
ogl__check_error()
{// NOTE(kv): Don't need this anymore if debug output works, this is just for sanity
#if !SHIP_MODE
    GLuint error = glGetError();
    kv_assert(error == GL_NO_ERROR);
#endif
}

force_inline b32
ogl__index_valid(GLuint index)
{
 return index != (GLuint)-1;
}

inline void
ogl__clamp_to_edge()
{
 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

inline void
ogl__filter_nearest()
{
 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

function GLuint
ogl__gen_ed_texture(i3 dim, Texture_Kind texture_kind)
{
 GLuint tex = {};
 glGenTextures(1, &tex);
 glBindTexture(GL_TEXTURE_2D_ARRAY, tex);
 
 GLint function_format = 0;
 GLenum submit_format = 0;
 GLenum submit_type = GL_UNSIGNED_BYTE;
 if (texture_kind == TextureKind_Mono)
 {
  function_format = GL_R8;
  submit_format   = GL_RED;
 }
 else if (texture_kind == TextureKind_ARGB)
 {
  function_format = GL_RGBA8;
  submit_format   = GL_BGRA;//NOTE: this is how it's stored in memory, GL_UNSIGNED_BYTE basically says "read the value byte-by-byte in memory order"
 }
 else { invalid_code_path; }
 //
 glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, function_format, v3_expand(dim), 0, submit_format, submit_type, 0);
 
 glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
 glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
 glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
 glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
 glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 0);
 return(tex);
}

function b32
ogl__fill_texture(Texture_Kind texture_kind, u32 texture, Vec3_i32 p, Vec3_i32 dim, void *data)
{
 b32 result = false;
 if (texture != 0 &&
     (dim.x > 0 && dim.y > 0 && dim.z > 0))
 {
  glBindTexture(GL_TEXTURE_2D_ARRAY, texture);
  GLenum submit_format = 0;
  GLenum submit_type   = 0;
  if (texture_kind == TextureKind_Mono)
  {
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   submit_format = GL_RED;
   submit_type   = GL_UNSIGNED_BYTE;
  }
  else if (texture_kind == TextureKind_ARGB)
  {
   glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
   submit_format = GL_BGRA;
   submit_type   = GL_UNSIGNED_BYTE;
  }
  else { invalid_code_path; }
  
  glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, v3_expand(p), v3_expand(dim), submit_format, submit_type, data);
  result = true;
 }
 
 return(result);
}

function void
ogl__error_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, 
                    const char *message, const void *userParam)
{
 //Severity: GL_DEBUG_SEVERITY_HIGH, GL_DEBUG_SEVERITY_MEDIUM, GL_DEBUG_SEVERITY_LOW
 switch (id)
 {
  case 131218:
  {
   // NOTE(allen): performance warning, do nothing.
  }break;
  
  invalid_default_case;
 }
}

global char *ogl_shared_header = 
R"foo(
#define v1 float
#define v2 V2
#define v3 V3
#define v4 V4
#define i32 int
#define b32 int
#define u32 uint
#define V2 vec2
#define V3 vec3
#define V4 vec4

layout(location=0) uniform mat4 uniform_screen_from_world;
layout(location=1) uniform mat3 uniform_camera_axes;
layout(location=2) uniform v1   uniform_unused;
layout(location=3) uniform mat4 uniform_object_transform;
layout(location=4) uniform v1   uniform_meter_to_pixel;
layout(location=5) uniform b32  uniform_overlay;
)foo";

//-NOTE: ;editor_shaders
global char *ed_vertex_shader =
R"foo(
layout(location=0) in v2 vertex_pos;
layout(location=1) in v3 vertex_uvw;
layout(location=2) in v4 vertex_color;
layout(location=3) in v1 vertex_half_thickness;

out v4 color;
out v3 uvw;
out v2 xy;
out v2 adjusted_half_dim;
out v1 half_thickness;

void main(void)
{
    gl_Position = uniform_screen_from_world * V4(vertex_pos, 1, 1);
    color.rgba = vertex_color.bgra;

    uvw = vertex_uvw;
    V2 center = vertex_uvw.xy;
    xy = vertex_pos;
    v2 half_dim = abs(xy - center);
    v2 roundness = vertex_uvw.zz;
    adjusted_half_dim = half_dim - roundness + V2(0.5f,0.5f);
    half_thickness = vertex_half_thickness;
}
)foo";

global char *ed_fragment_shader =
R"foo(
layout(binding=0) uniform sampler2DArray sampler;
//
smooth in V4 color;
smooth in V3 uvw;
smooth in V2 xy;
smooth in V2 adjusted_half_dim;
smooth in float half_thickness;
//
out V4 out_color;

// NOTE: negative = inside, positive = outside
v1 rectangle_sd(v2 p, v2 b)
{
    V2 d = abs(p) - b;
    return(length(max(d, V2(0,0))) + 
           min(max(d.x, d.y), 0.0));
}
    
void main(void)
{
    v1 value;
    {
        bool has_thickness = half_thickness > 0.49;
        if (has_thickness)
        {
            V2 center = uvw.xy;
            float sd = rectangle_sd(xy - center, adjusted_half_dim);
            float roundness = uvw.z;
            sd = sd - roundness;
            sd = abs(sd + half_thickness) - half_thickness;
            value = smoothstep(0.0, -1.0, sd);
        }
        else
        {
            value = texture(sampler, uvw).r;
        }
    }
    v1 out_alpha = color.a*value;
    out_color = V4(color.rgb, out_alpha);
}
)foo";

enum OGL_Program_Type
{
 OGL_First_Pass  = 1,
 OGL_Second_Pass = 2,
 OGL_Editor      = 3,
 OGL_Image       = 4,
};

enum OGL_Program_Flag
{
 OGL_Overlay            = 0x1,
 OGL_Write_Primitive_ID = 0x2,
};
typedef u32 OGL_Program_Flags;

function u32
ogl__create_program(OGL_Program_Type type, OGL_Program_Flags flags)
{
 // const GLubyte *version_confirm = glGetString(GL_VERSION);
 b32 is_game       = type != OGL_Editor;
 b32 is_first_pass = type == OGL_First_Pass;
 
 GLuint vertex_shader   = glCreateShader(GL_VERTEX_SHADER);
 GLuint geometry_shader = 0;
 if (is_first_pass) { geometry_shader = glCreateShader(GL_GEOMETRY_SHADER); }
 GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
 {
  Arena arena_value = make_arena_malloc();
  Arena *arena = &arena_value;
  
  char *header = ogl_shared_header;
  char defines[256];
  snprintf(defines,
           alen(defines),
           "#define IS_FIRST_PASS %d\n"
           "#define IS_SECOND_PASS %d\n"
           "#define WRITE_PRIM_ID %d\n",
           is_first_pass,
           (type == OGL_Second_Pass),
           (flags & OGL_Write_Primitive_ID));
  char *version_str = "#version 460\n";
    
#if SHIP_MODE
# define SHADER_DIR "C:/Users/vodan/4coder/opengl/"
#else
# define SHADER_DIR "C:/Users/vodan/4ed/code/opengl/"
#endif
  {
   char *vertex;
   //
   if(is_game){ vertex = to_cstring(read_entire_file(arena, strlit(SHADER_DIR "vertex_shader.glsl"))); }
   else       { vertex = ed_vertex_shader; }
   GLchar *array[] = { version_str, defines, header, vertex };
   glShaderSource(vertex_shader, alen(array), array, 0);
   glCompileShader(vertex_shader);
  }
  
  if(is_first_pass){
   char *geometry = to_cstring(read_entire_file(arena, strlit(SHADER_DIR "geometry_shader.glsl")));
   GLchar *array[] = { version_str, defines, header, geometry };
   glShaderSource(geometry_shader, alen(array), array, 0);
   glCompileShader(geometry_shader);
  } 
  
  {
   char *fragment;
   if(is_game){ fragment = to_cstring(read_entire_file(arena, strlit(SHADER_DIR "fragment_shader.glsl"))); }
   else       { fragment = ed_fragment_shader; }
   GLchar *array[] = { version_str, defines, header, fragment };
   glShaderSource(fragment_shader, alen(array), array, 0);
   glCompileShader(fragment_shader);
  }
#undef SHADER_DIR
 }
 
 GLuint program = glCreateProgram();
 {
  glAttachShader(program, vertex_shader);
  if (is_first_pass)
  {
   glAttachShader(program, geometry_shader);
  }
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);
  glValidateProgram(program);
 }
 
 {// NOTE(kv): Validation
  GLint success = false;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if ( !success )
  {
   GLsizei ignore = 0;
   char vertex_errors  [KB(4)];
   char geometry_errors[KB(4)];
   char fragment_errors[KB(4)];
   char program_errors [KB(4)];
   glGetShaderInfoLog(vertex_shader, sizeof(vertex_errors), &ignore, vertex_errors);
   if (is_first_pass)
   {
    glGetShaderInfoLog(geometry_shader, sizeof(geometry_errors), &ignore, geometry_errors);
   }
   glGetShaderInfoLog(fragment_shader, sizeof(fragment_errors), &ignore, fragment_errors);
   glGetProgramInfoLog(program, sizeof(program_errors), &ignore, program_errors);
#if SHIP_MODE
   os_popup_error("Error", "Shader compilation failed.");
#endif
   InvalidPath;
  }
 }
 
 glDeleteShader(vertex_shader);
 glDeleteShader(geometry_shader);
 glDeleteShader(fragment_shader);
 
 return(program);
}

#define GLOffset(S,m)       ((void*)(offsetof(S,m)))

function b32
ogl__is_uniform_active(GLuint program, char *name)
{
 return (glGetUniformLocation(program, name) != -1);
}

// NOTE: Would it be better to just have one program?
// Yeah but I wanna have a tiny primitive id framebuffer.
// Which can't be done in OpenGL (not even mentioning MSAA).
global u32 ogl_program_2;
global u32 ogl_program_2_prim_id;
global u32 ogl_program_image;
global u32 ogl_program_image_prim_id;
global u32 ogl_program_editor;

function v1 * ogl_cast_mat4(mat4 *matrix) { return cast(v1*)matrix; }
function v1 * ogl_cast_mat3(mat3 *matrix) { return cast(v1*)matrix; }

struct OGL_Program_State {
 Render_Group *group;
 mat3 camera_axes;
 b32 is_overlay;
};

function void
ogl__vertex_attributes()
{
 for_i32 (attr,0,VATTR_COUNT) { glEnableVertexAttribArray(attr); }
 
 {// NOTE: Tell opengl how to read vertex attributes
#define OFFSET(name)  GLOffset(Render_Vertex, name)
  GLsizei stride = sizeof(Render_Vertex);
  glVertexAttribPointer(vattr_pos,            3, GL_FLOAT, GL_FALSE, stride, OFFSET(pos));
  glVertexAttribPointer(vattr_uvw,            3, GL_FLOAT, GL_FALSE, stride, OFFSET(uvw));
  glVertexAttribPointer(vattr_color, 4, GL_UNSIGNED_BYTE, GL_TRUE,  stride, OFFSET(color));
  glVertexAttribPointer(vattr_half_thickness, 1, GL_FLOAT, GL_FALSE, stride, OFFSET(half_thickness));
  glVertexAttribPointer(vattr_depth_offset,   1, GL_FLOAT, GL_FALSE, stride, OFFSET(depth_offset));
  glVertexAttribIPointer(vattr_prim_id, 1, GL_UNSIGNED_INT, stride, OFFSET(prim_id));
#undef OFFSET
 }
}

function void
ogl__uniform_mat4(GLint uniform, mat4 *mat) {
 glUniformMatrix4fv(uniform, 1, GL_TRUE, ogl_cast_mat4(mat));
}

// @Ugh The only reason why this function exist is because
// every time you switch program, you gotta send it uniforms...
function void
ogl__begin_program(OGL_Program_State *s, u32 program, mat4 *screen_from_world)
{
 glUseProgram(program);
 Render_Group *group = s->group;
 
 {// NOTE: Uniforms
#define ACTIVE(uniform)  ogl__is_uniform_active(program, #uniform)
  if( ACTIVE(uniform_screen_from_world) ) {
   ogl__uniform_mat4(uniform_screen_from_world, screen_from_world);
  }
  if (program == ogl_program_image)
  {
   GLint image_texture_binding = 0;
   if ( ACTIVE(image_texture_binding) ) {
    glUniform1i(image_texture_binding, GL_TEXTURE0);
   }
  }
  if( ACTIVE(uniform_camera_axes) ) {
   glUniformMatrix3fv(uniform_camera_axes, 1, GL_TRUE, ogl_cast_mat3(&s->camera_axes));
  }
  if( ACTIVE(uniform_object_transform) ) {
   //glUniformMatrix4fv(uniform_object_transform, 1, GL_TRUE, ogl_cast_mat4(s->object_transform));
  }
  if( ACTIVE(uniform_meter_to_pixel) ) {
   glUniform1f(uniform_meter_to_pixel, group->meter_to_pixel);
  }
  if( ACTIVE(uniform_overlay) ) {
   glUniform1i(uniform_overlay, false);
  }
#undef ACTIVE
 }
 
 ogl__vertex_attributes();
}

function void
ogl__begin_program_editor(Render_Target *target, Render_Group *group)
{
 glUseProgram(ogl_program_editor);
 
 {// NOTE: Uniforms
  mat4 screen_transform;
  {
   v1 y_sign = (group->y_up) ? +1.f : -1.f;
   v2 dst_dim = V2(v1(target->width), v1(target->height));
   // NOTE(kv): Scale to normalized device coordinate
   v1 a = 2.f/dst_dim.x;
   v1 b = 2.f/dst_dim.y * y_sign;
   v1 c = -1.0f;
   v1 d = -1.0f*y_sign;
   
   screen_transform = {{
     a,0,0,c,
     0,b,0,d,
     0,0,1,0,
     0,0,0,1,
    }};
  }
  
  glUniformMatrix4fv(uniform_screen_from_world, 1, GL_TRUE, ogl_cast_mat4(&screen_transform));
 }
 
 ogl__vertex_attributes();
}

function void
ogl__end_program()
{
 // NOTE: Disable Vertex Attrib Array
 for_i32(location,0,VATTR_COUNT)
 {
  glDisableVertexAttribArray(location);
 }
}

function void
ogl__send_buffer_data(Render_Vertex_List *list)
{
 glBufferData(GL_ARRAY_BUFFER, list->count*sizeof(Render_Vertex), 0, GL_STREAM_DRAW);
 i32 cursor = 0;
 for (Render_Vertex_Array_Node *node = list->first;
      node != 0;
      node = node->next)
 {
  i32 size = node->vertex_count*sizeof(*node->vertices);
  glBufferSubData(GL_ARRAY_BUFFER, cursor, size, node->vertices);
  cursor += size;
 }
}

// NOTE: Originally used glTexImage2D, which was awful because of the unnecessary submission parameters.
// glTexStorage2D is less verbose, so idk.
// IMPORTANT: you CANNOT call glTexImage2D after calling glTexStorage2D
// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexStorage2D.xhtml
function void
glTexStorage2D_wrapper(GLint function_format, GLsizei width, GLsizei height)
{
 // TODO: we can switch to glTextureStorage2D
 glTexStorage2D(GL_TEXTURE_2D, /*level*/1, function_format, width, height);
 // NOTE: Some texture voodoo that may or may not be needed
 ogl__clamp_to_edge();
 ogl__filter_nearest();
}

function void
glTexImage2D_wrapper(GLint function_format, GLsizei width, GLsizei height,
                     GLenum submission_format, GLenum type,
                     const void * data)
{
 glTexImage2D(GL_TEXTURE_2D, /*level*/0, function_format, width, height,
              /*border*/0, submission_format, type, data);
}

inline void
assert_framebuffer_status()
{
 GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
 kv_assert(status == GL_FRAMEBUFFER_COMPLETE);
} 

global GLuint prim_id_framebuffer;

function u32
ogl_read_primitive_id()
{// NOTE: We have to read after rendering, because reading while you're rendering 
 // causes flicker, idk why because it shouldn't change the draw order.
 // also probably bad for perforamnce
 
 u32 result = 0;
 
 for (Render_Group *group = render_state.group_first;
      group;
      group = group->next)
 {//NOTE: searching for main viewport (stupid!)
  if (group->viewport_id == 1)
  {
   glBindFramebuffer(GL_READ_FRAMEBUFFER, prim_id_framebuffer);
   glReadBuffer(GL_COLOR_ATTACHMENT0);
   rect2i box = Ri32(group->clip_box);
   glReadPixels(0,0,
                1,1,
                GL_RED_INTEGER, GL_UNSIGNED_INT,
                &result);
   glBindFramebuffer(GL_FRAMEBUFFER,DEFAULT_FRAMEBUFFER);
   break;
  }
 }
 
 return result;
}

function void
ogl__stream_draw(Render_Vertex_List *list)
{
 if (list->count)
 {
  ogl__send_buffer_data(list);
  glDrawArrays(GL_TRIANGLES, 0, list->count);
 }
}

function void
ogl__stream_draw(Render_Vertex *vertices, i32 count)
{
 if (count)
 {
  glBufferData(GL_ARRAY_BUFFER, count*sizeof(Render_Vertex), vertices, GL_STREAM_DRAW);
  glDrawArrays(GL_TRIANGLES, 0, count);
 }
}


function void
ogl__render_entries(Render_Group *group)
{
 for (Render_Entry *entry = group->entry_first;
      entry; 
      entry = entry->next)
 {
  if (entry->type == RET_Poly)
  {
   {// normal stuff
    ogl__stream_draw(&entry->poly.vertex_list);
   }
   {// overlay
    glUniform1i(uniform_overlay, true);
    ogl__stream_draw(&entry->poly.vertex_list_overlay);
    glUniform1i(uniform_overlay, false);
   }
  }
  else if (entry->type == RET_Object_Transform)
  {
   ogl__uniform_mat4(uniform_object_transform, &entry->object_transform);
  }
 }
}

function void
ogl__render_images(Render_Group *group, b32 render_primitive_id)
{
 mat4 *current_object_transform = &mat4_identity;
 // @Speed We only have at most a few images on the screen at a time.
 // sifting through all the entries might not be efficient.
 for(Render_Entry *entry0 = group->entry_first;
     entry0; 
     entry0 = entry0->next)
 {
  if (entry0->type == RET_Image)
  {
   Render_Entry_Image *entry = entry0->image;
   b32 bound_texture = false;
   i2 dim = {};
   for_i32(image_index, 0, arrlen(loaded_images))
   {
    Loaded_Image *loaded_image = &loaded_images[image_index];
    if ( string_match(loaded_image->filename, entry->filename) )
    {
     glBindTextureUnit(0, loaded_image->texture);
     bound_texture = true;
     dim = loaded_image->dim;
     break;
    }
   }
   
   if (bound_texture == false)
   {// NOTE: Image not loaded yet
    u8 *data = 0;
    {
     int ncomp = 3;
     data = stbi_load(entry->filename, &dim.x,&dim.y, 0,ncomp);
    }
    if (data)
    {
     char *filename = (char*)malloc(strlen(entry->filename)+1);  // NOTE: no free, I don't care where these strings are
     strcpy(filename, entry->filename);
     
     Loaded_Image new_image = { .filename=filename, .dim=dim };
     glGenTextures(1, &new_image.texture);
     glBindTexture(GL_TEXTURE_2D, new_image.texture);
     bound_texture = true;
     glTexStorage2D_wrapper(GL_RGB8, dim.x, dim.y);
     glTextureSubImage2D(new_image.texture,
                         /*level*/0,
                         /*offset*/0,0,
                         dim.x, dim.y,
                         GL_RGB, GL_UNSIGNED_BYTE,
                         data);
     free(data);  // NOTE: multiple sources saying this alright
     arrpush(loaded_images, new_image);
     kv_assert(arrlen(loaded_images) < 128);
    }
   }
   
   if (bound_texture)
   {
    ogl__uniform_mat4(uniform_object_transform, current_object_transform);
    
    v1 hw_ratio = v1(dim.y) / v1(dim.x);
    v3 x = entry->x;
    v3 y = hw_ratio*lengthof(x)*entry->y_info;
    v3 a = entry->o-x-y;
    v3 b = a + 2.f*x;
    v3 c = b + 2.f*y;
    v3 d = c - 2.f*x;
    Render_Vertex A = { .pos=a, .uv=V2(0,0) };
    Render_Vertex B = { .pos=b, .uv=V2(1,0) };
    Render_Vertex C = { .pos=c, .uv=V2(1,1) };
    Render_Vertex D = { .pos=d, .uv=V2(0,1) };
    Render_Vertex vertices[6] = { A,B,C, A,C,D };
    for_i32(index,0,alen(vertices)) {
     vertices[index].color   = entry->color; 
     vertices[index].prim_id = entry->prim_id; 
    }
    ogl__stream_draw(vertices, alen(vertices));
   }
  } 
  else if (entry0->type == RET_Object_Transform)
  {
   current_object_transform = &entry0->object_transform;
  }
 }
}

function void
ogl_render(i2 mousep_ydown, i32 window_id)
{
 //@HardCoded Allow one 1080p panel maximum
 const i32 MAX_PANEL_DIMX = 1920;
 const i32 MAX_PANEL_DIMY = 1080;
 
 local_persist GLuint game_framebuffer;
 // NOTE: We use a different framebuffer because the default one doesn't let us attach any texture.
 local_persist GLuint color0_texture;
 local_persist GLuint depth_texture;
 
 local_persist GLuint prim_id_texture;
 local_persist GLuint prim_id_depth_texture;
 
 local_persist b32 first_opengl_call = true;
 if (first_opengl_call)
 {
  first_opengl_call = false;
  
#define X(...) VATTR_COUNT++;
  X_VERTEX_ATTRIBUTES(X)
#undef X
  
  {
#if !SHIP_MODE
   {
    i32 context_flags = 0;
    glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
    kv_assert(context_flags & GL_CONTEXT_FLAG_DEBUG_BIT);
   }
   glEnable(GL_DEBUG_OUTPUT);
   glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
   if(1)
   {
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, 0, true);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, 0, true);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, 0, true);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, 0, true);
   }
   glDebugMessageCallback(ogl__error_callback, 0);
#endif
  }
  
  GLuint dummy_vao = 0;
  glGenVertexArrays(1, &dummy_vao);
  glBindVertexArray(dummy_vao);
  
  // TODO(kv): What is this? Remove?
  local_persist u32 attribute_buffer = 0;
  glGenBuffers(1, &attribute_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, attribute_buffer);
  
  glEnable(GL_MULTISAMPLE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  
  glDepthFunc(GL_LEQUAL);  // NOTE: "equal" because we want things drawn later to be on top, prolly doesn't matter
  glClearDepth(1.0f);
  
  ogl_program_2             = ogl__create_program(OGL_Second_Pass, 0);
  ogl_program_2_prim_id     = ogl__create_program(OGL_Second_Pass, OGL_Write_Primitive_ID);
  ogl_program_image         = ogl__create_program(OGL_Image, 0);
  ogl_program_image_prim_id = ogl__create_program(OGL_Image, OGL_Write_Primitive_ID);
  ogl_program_editor        = ogl__create_program(OGL_Editor, 0);
  
  render_state.fallback_texture_id = ogl__gen_ed_texture(I3(2,2,1), TextureKind_Mono);
  u8 white_block[] = { 0xFF, 0xFF, 0xFF, 0xFF };
  ogl__fill_texture(TextureKind_Mono, GLuint{}, I3(0,0,0), I3(2,2,1), white_block);
  
  {//NOTE: game frame buffer
   i32 W = MAX_PANEL_DIMX;
   i32 H = MAX_PANEL_DIMY;
   
   glGenFramebuffers(1, &game_framebuffer);
   glBindFramebuffer(GL_FRAMEBUFFER, game_framebuffer);
   
   GLenum texture_target = GL_TEXTURE_2D_MULTISAMPLE;
   GLsizei num_samples = 4;  //@Tweak: I literally don't see any difference between 4x and 8x
   {//-Game texture
    glGenTextures(1, &color0_texture);
    glBindTexture(texture_target, color0_texture);
    
    glTexImage2DMultisample(texture_target, num_samples, GL_SRGB8_ALPHA8, W,H, GL_FALSE);
    ogl__clamp_to_edge();
    ogl__filter_nearest();
    
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture_target, color0_texture, /*level*/0);
   }
   
   {//-Depth texture: Because we can't use the default one for some reason
    glGenTextures(1, &depth_texture);
    glBindTexture(texture_target, depth_texture);
    ogl__clamp_to_edge();
    ogl__filter_nearest();
    glTexImage2DMultisample(texture_target/*dummy*/, num_samples, GL_DEPTH_COMPONENT24, W,H, GL_FALSE);
    glFramebufferTexture2D(GL_FRAMEBUFFER/*dummy param*/, GL_DEPTH_ATTACHMENT, texture_target, depth_texture, /*level*/0);
   }
   
   assert_framebuffer_status();
   
   glBindFramebuffer(GL_FRAMEBUFFER,DEFAULT_FRAMEBUFFER);
  }
  
  {//NOTE: ;init_prim_id_framebuffer
   i32 W = 1;
   i32 H = 1;
   glGenFramebuffers(1, &prim_id_framebuffer);
   glBindFramebuffer(GL_FRAMEBUFFER, prim_id_framebuffer);
   
   GLenum texture_target = GL_TEXTURE_2D;
   {
    glGenTextures(1, &prim_id_texture);
    glBindTexture(texture_target, prim_id_texture);
    glTexStorage2D_wrapper(GL_R32UI, W,H);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture_target, prim_id_texture, /*level*/0);
   }
   
   {//-NOTE: Depth texture
    glGenTextures(1, &prim_id_depth_texture);
    glBindTexture(texture_target, prim_id_depth_texture);
    ogl__clamp_to_edge();
    ogl__filter_nearest();
    glTexStorage2D_wrapper(GL_DEPTH_COMPONENT24, W,H);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture_target, prim_id_depth_texture, /*level*/0);
   }
   
   assert_framebuffer_status();
   
   glBindFramebuffer(GL_FRAMEBUFFER,DEFAULT_FRAMEBUFFER);
  }
 }
 
 {// NOTE: Clear default buffer
  glBindFramebuffer(GL_FRAMEBUFFER, DEFAULT_FRAMEBUFFER);
#if KV_INTERNAL  // NOTE: Irritating clear in debug mode only
  glClearColor(.2f,0.f,.2f,1.f);
#else
  glClearColor(0.f,0.f,0.f,1.f);
#endif
  glClear(GL_COLOR_BUFFER_BIT);
 }
 
 if (window_id == 0)
 {// NOTE: Clear primitive id buffer
  // IMPORTANT: For some fucking reason, you just can't clear this before you clear the default framebuffer.
  // If you do that, texture lookup *magically* breaks
  glBindFramebuffer(GL_FRAMEBUFFER, prim_id_framebuffer);
  glClearColor(0,0,0,0);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  glBindFramebuffer(GL_FRAMEBUFFER, DEFAULT_FRAMEBUFFER);
 }
 
 glBindTexture(GL_TEXTURE_2D, 0);
 render_state.texture_bound_at_unit0 = {};
 
 for (Render_Free_Texture *free_texture = render_state.free_texture_first;
      free_texture != 0;
      free_texture = free_texture->next)
 {
  glDeleteTextures(1, &free_texture->tex_id);
 }
 render_state.free_texture_first = 0;
 render_state.free_texture_last = 0;
 
 Font_Set *font_set = (Font_Set*)render_state.font_set;
 for (Render_Group *group = render_state.group_first;
      group != 0;
      group = group->next)
 {//-NOTE: Each render group corresponds to one view/panel (for the game at least)
  Render_Target *target = get_render_target(group);
  b32 is_game = render_group_is_game(group);
  b32 has_poly = 0;
  if(group->entry_first){
   has_poly = (group->entry_first->poly.vertex_list.count > 0);
  };
  if (target->window_id == window_id &&
      (is_game || has_poly))
  {
   i32 dstx, dsty;
   i32 dst_dimx, dst_dimy;
   v2 dst_dim;
   {// NOTE: get render dimensions
    rect2i box = Ri32(group->clip_box);
    dstx = box.x0;
    dsty = box.y0; 
    if (!group->y_up) { 
     dsty = target->height - box.y1;
    };  // NOTE; For editor
    dst_dim = V2(rect2i_dim(box));
    
    {// NOTE: Handle scale down (TODO: Should've just let the game change the render surface, that would save us the trouble of computing render location at both places)
     for_i32(it,0,group->scale_down_pow2) {
      dst_dim *= 0.5f;
     }
     
     // @dim_round_down
     dst_dimx = cast(i32)dst_dim.x;
     dst_dimy = cast(i32)dst_dim.y;
    }
    
    macro_clamp_min(dstx, 0);     macro_clamp_min(dsty, 0);
    macro_clamp_min(dst_dimx, 0); macro_clamp_min(dst_dimy, 0);
   }
   
   if( is_game )
   {//~NOTE: Render for the game!
    b32 should_draw_prim_id = false;
    v2 view_mousep = {};
    if (group->viewport_id == 1 &&
        window_id == 0)
    {
     i2 mousep = I2(mousep_ydown.x,
                    target->height - mousep_ydown.y);
     view_mousep = V2(mousep)-cast_V2(dstx,dsty);
     b32 mouse_in_view = (view_mousep.x >= v1(dstx) &&
                          view_mousep.y >= v1(dsty) &&
                          view_mousep.x < dst_dim.x &&
                          view_mousep.y < dst_dim.y);
     should_draw_prim_id = mouse_in_view;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, game_framebuffer);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glScissor(0,0, dst_dimx,dst_dimy);
    glViewport(0, 0, dst_dimx,dst_dimy);
    
    {//NOTE: Clears
     v4 bg_color = argb_unpack(group->background);
     glClearColor( v4_expand(bg_color) );
     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    
    //
    mat4 object_transform = mat4_identity;
    mat4 screen_from_world;
    OGL_Program_State state_value = {.group = group};
    auto state = &state_value;
    //state->object_transform = &object_transform;
    {
     state->camera_axes = to_mat3(group->world_from_cam);
     {
      mat4 screen_from_view;
      {
       // :viewport_alignment The center of the game viewport lines up with the center of the opengl viewport
       v1 sx = 2.f*group->meter_to_pixel/dst_dim.x;
       v1 sy = 2.f*group->meter_to_pixel/dst_dim.y;
       screen_from_view = mat4_scales(sx,sy,1.f);
      }
      
      screen_from_world = screen_from_view*group->view_from_world;  //NOTE: this view transform is different from the camera view transform, which is confusing?
     }
    }
    mat4 view_transform_offsetted_by_mousep;
    if (should_draw_prim_id) {
     //NOTE: We have to translate in NDC space
     mat4 translation = mat4_translate( -V3((2.f*view_mousep) / dst_dim, 0.f) );
     view_transform_offsetted_by_mousep = translation*screen_from_world;
    }
    
    {// NOTE: second pass
     {// NOTE: main rendering
      ogl__begin_program(state, ogl_program_2, &screen_from_world);
      ogl__render_entries(group);
      ogl__end_program();
     }
     
     if (should_draw_prim_id)
     {// NOTE: (prim id)
      glBindFramebuffer(GL_FRAMEBUFFER, prim_id_framebuffer);
      // TODO(kv): Am I dumb? Why modifying this state when it's always the same for this framebuffer?
      glDisable(GL_FRAMEBUFFER_SRGB);
      ogl__begin_program(state, ogl_program_2_prim_id,
                         &view_transform_offsetted_by_mousep);
      ogl__render_entries(group);
      ogl__end_program();
      glEnable(GL_FRAMEBUFFER_SRGB); 
      glBindFramebuffer(GL_FRAMEBUFFER, game_framebuffer);
     }
    }
    
    {//-NOTE: Render @ReferenceImage
     {// NOTE: Actually rendering
      ogl__begin_program(state, ogl_program_image, &screen_from_world);
      ogl__render_images(group, false);
      ogl__end_program();
     }
     
     if (should_draw_prim_id)
     {// NOTE: prim id
      glBindFramebuffer(GL_FRAMEBUFFER, prim_id_framebuffer);
      glDisable(GL_FRAMEBUFFER_SRGB); 
      ogl__begin_program(state, ogl_program_image_prim_id,
                         &view_transform_offsetted_by_mousep);
      ogl__render_images(group, true);
      ogl__end_program();
      glEnable(GL_FRAMEBUFFER_SRGB); 
      glBindFramebuffer(GL_FRAMEBUFFER, game_framebuffer);
     }
     
     glBindTextureUnit(0, render_state.texture_bound_at_unit0); //NOTE: Set it back for Mr. Editor
    }
    
    {// NOTE: Blit game_texture -> default framebuffer :game_blit
     glBindFramebuffer(GL_READ_FRAMEBUFFER, game_framebuffer);
     glBindFramebuffer(GL_DRAW_FRAMEBUFFER, DEFAULT_FRAMEBUFFER);
     glDisable(GL_SCISSOR_TEST); // NOTE: damn, this is global state
     glBlitFramebuffer(0,0, dst_dimx,dst_dimy,
                       dstx,dsty, dstx+dst_dimx,dsty+dst_dimy,
                       GL_COLOR_BUFFER_BIT, GL_NEAREST);
     
     glBindFramebuffer(GL_FRAMEBUFFER,DEFAULT_FRAMEBUFFER);
    }
   }
   else
   {//-NOTE: Editor
    kv_assert(group->entry_first->type == RET_Poly);
    Render_Entry_Poly *entry = &group->entry_first->poly;
    
    {
     Face *face = 0;
     if (group->face_id) {
      face = font_set_face_from_id(font_set, group->face_id);
     }
     if (face)  {
      GLuint tex = face->texture;
      if (render_state.texture_bound_at_unit0 != tex) {
       glBindTextureUnit(0,tex);
       render_state.texture_bound_at_unit0 = tex;
      }
     } else if (!render_state.texture_bound_at_unit0) {
      Assert(render_state.fallback_texture_id != 0);
      glBindTextureUnit(0, render_state.fallback_texture_id);
      render_state.texture_bound_at_unit0 = render_state.fallback_texture_id;
     }
    }
    
    glViewport(0, 0, target->width, target->height);
    glEnable(GL_SCISSOR_TEST);
    glScissor(dstx, dsty, dst_dimx, dst_dimy);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FRAMEBUFFER_SRGB); 
    
    ogl__begin_program_editor(target, group);
    Render_Vertex_List *list = &entry->vertex_list;
    ogl__send_buffer_data(list);
    glDrawArrays(GL_TRIANGLES, 0, list->count);
    ogl__end_program();
    
    glDisable(GL_SCISSOR_TEST);
   }
  }
 }
}

//~ EOF
