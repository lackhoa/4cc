/*
 * Mr. 4th Dimention - Allen Webster
 *
 * 10.11.2017
 *
 * OpenGL render implementation
 *
 */

// TOP

internal void
gl__bind_texture(Render_Target *t, Texture_ID tex)
{
    if (t->bound_texture.v == tex.v) return;
    
    glBindTexture(GL_TEXTURE_2D_ARRAY, tex.v);
    t->bound_texture = tex;
}

internal void
gl__bind_fallback_texture(Render_Target *t)
{
    if (t->bound_texture.v) return;
    
    Assert(t->fallback_texture_id.v != 0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, t->fallback_texture_id.v);
    t->bound_texture = t->fallback_texture_id;
}

internal Texture_ID
gl__get_texture(Vec3_i32 dim, Texture_Kind texture_kind)
{
    Texture_ID tex = {};
    glGenTextures(1, &tex.v);
    glBindTexture(GL_TEXTURE_2D_ARRAY, tex.v);
    
    GLint internal_format;
    GLenum pixel_format = 0;
    GLenum data_type = 0;
    if (texture_kind == TextureKind_Mono)
    {
        internal_format = GL_R8;
        pixel_format = GL_RED;
        data_type = GL_UNSIGNED_BYTE;
    }
    else if (texture_kind == TextureKind_ARGB)
    {
        internal_format = GL_RGBA8;
        pixel_format = GL_BGRA;
        data_type = GL_UNSIGNED_INT_8_8_8_8_REV;
    }
    else invalid_code_path;
    //
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internal_format, v3_expand(dim), 0, pixel_format, data_type, 0);
    
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 0);
    return(tex);
}

internal b32
gl__fill_texture(Texture_Kind texture_kind, Texture_ID texture, Vec3_i32 p, Vec3_i32 dim, void *data)
{
    b32 result = false;
    if (texture.v != 0)
    {
        glBindTexture(GL_TEXTURE_2D_ARRAY, texture.v);
    }
    
    GLenum pixel_format;
    GLenum data_type;
    if (dim.x > 0 && dim.y > 0 && dim.z > 0)
    {
        if (texture_kind == TextureKind_Mono)
        {
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            pixel_format = GL_RED;
            data_type = GL_UNSIGNED_BYTE;
        }
        else if (texture_kind == TextureKind_ARGB)
        {
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
            pixel_format = GL_BGRA;
            data_type = GL_UNSIGNED_INT_8_8_8_8_REV;
        }
        else invalid_code_path;
        
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, v3_expand(p), v3_expand(dim), pixel_format, data_type, data);
        result = true;
    }
    
    return(result);
}

internal void
gl__error_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char *message, const void *userParam)
{
    switch (id)
    {
        case 131218:
        {
            // NOTE(allen): performance warning, do nothing.
        }break;
        
        invalid_default_case;
    }
}

global char *gl__header = R"foo(#version 130
        )foo";

global char *gl__vertex_shader =
R"foo(
   uniform vec2   view_t;
   uniform mat2x2 view_m;

   in vec2 vertex_xy;
   in vec3 vertex_uvw;
   in uint vertex_color;
   in float vertex_half_thickness;

   smooth out vec4 fragment_color;
   smooth out vec3 uvw;
   smooth out vec2 xy;
   smooth out vec2 adjusted_half_dim;
   smooth out float half_thickness;

   void main(void)
   {
       vec2 position_xy = view_m * (vertex_xy + view_t);
       gl_Position = vec4(position_xy, 0.0, 1.0);
       fragment_color.b = (float((vertex_color     )&0xFFu))/255.0;
       fragment_color.g = (float((vertex_color>> 8u)&0xFFu))/255.0;
       fragment_color.r = (float((vertex_color>>16u)&0xFFu))/255.0;
       fragment_color.a = (float((vertex_color>>24u)&0xFFu))/255.0;
       uvw = vertex_uvw;
       vec2 center = vertex_uvw.xy;
       vec2 half_dim = abs(vertex_xy - center);
       adjusted_half_dim = half_dim - vertex_uvw.zz + vec2(0.5, 0.5);
       half_thickness = vertex_half_thickness;
       xy = vertex_xy;
   }
   )foo";

global char *gl__fragment_shader = 
R"foo(
   smooth in vec4 fragment_color;
   smooth in vec3 uvw;
   smooth in vec2 xy;
   smooth in vec2 adjusted_half_dim;
   smooth in float half_thickness;
   uniform sampler2DArray sampler;
   out vec4 out_color;
        
   float rectangle_sd(vec2 p, vec2 b){
       vec2 d = abs(p) - b;
       return(length(max(d, vec2(0.0, 0.0))) + min(max(d.x, d.y), 0.0));
   }
        
   void main(void)
   {
       float has_thickness = (step(0.49, half_thickness));
       float does_not_have_thickness = 1.0 - has_thickness;
       
       float sample_value = texture(sampler, uvw).r;
       sample_value *= does_not_have_thickness;
       
       vec2 center = uvw.xy;
       float roundness = uvw.z;
       float sd = rectangle_sd(xy - center, adjusted_half_dim);
       sd = sd - roundness;
       sd = abs(sd + half_thickness) - half_thickness;
       float shape_value = 1.0 - smoothstep(-1.0, 0.0, sd);
       shape_value *= has_thickness;
        
       out_color = vec4(fragment_color.xyz, fragment_color.a*(sample_value + shape_value));
   }
   )foo";

// TODO(kv): use macro to avoid duplications?
global char *gl__vertex_shader_game =
R"foo(
    uniform vec2   view_t;
    uniform mat2x2 view_m;

    in vec2 vertex_xy;
    in vec3 vertex_uvw;
    in uint vertex_color;
    in float vertex_half_thickness;

    smooth out vec4 fragment_color;
    smooth out vec3 uvw;

    void main(void)
    {
        vec2 position_xy = view_m * (vertex_xy + view_t);
        gl_Position = vec4(position_xy, 0.0, 1.0);
        fragment_color.b = (float((vertex_color     )&0xFFu))/255.0;
        fragment_color.g = (float((vertex_color>> 8u)&0xFFu))/255.0;
        fragment_color.r = (float((vertex_color>>16u)&0xFFu))/255.0;
        fragment_color.a = (float((vertex_color>>24u)&0xFFu))/255.0;
        uvw = vertex_uvw;
    }
    )foo";

global char *gl__fragment_shader_game = 
R"foo(
   smooth in vec4 fragment_color;
   smooth in vec3 uvw;
   uniform sampler2DArray sampler;
   out vec4 out_color;
        
   void main(void)
   {
        vec4 sample_value_v4 = texture(sampler, uvw);
        out_color = fragment_color * sample_value_v4;
    }
   )foo";

#define XAttribute(X) \
    X(xy,    2, GL_FLOAT) \
    X(uvw,   3, GL_FLOAT) \
    X(color, 1, GL_UNSIGNED_INT) \
    X(half_thickness, 1, GL_FLOAT)

#define XUniform(X) \
    X(view_t) \
    X(view_m) \
    X(sampler)

struct GL_Program
{
    u32 program;
    
#define X(N,...) i32 vertex_##N;
    XAttribute(X)
#undef X
    
#define X(N) i32 N;
    XUniform(X)
#undef X
};

internal GL_Program
gl__make_program(char *header, char *vertex, char *fragment)
{
    if (header == 0)
    {
        header = "";
    }
    
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLchar *vertex_source_array[] = { header, vertex };
    glShaderSource(vertex_shader, arlen(vertex_source_array), vertex_source_array, 0);
    glCompileShader(vertex_shader);
    
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    GLchar *fragment_source_array[] = { header, fragment };
    glShaderSource(fragment_shader, arlen(fragment_source_array), fragment_source_array, 0);
    glCompileShader(fragment_shader);
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    glValidateProgram(program);
   
    // NOTE(kv): validation
    GLint success = false;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        GLsizei ignore = 0;
        char vertex_errors[KB(4)];
        char fragment_errors[KB(4)];
        char program_errors[KB(4)];
        glGetShaderInfoLog(vertex_shader, sizeof(vertex_errors), &ignore, vertex_errors);
        glGetShaderInfoLog(fragment_shader, sizeof(fragment_errors), &ignore, fragment_errors);
        glGetProgramInfoLog(program, sizeof(program_errors), &ignore, program_errors);
#if SHIP_MODE
        os_popup_error("Error", "Shader compilation failed.");
#endif
        InvalidPath;
    }
    
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    GL_Program result = {};
    result.program = program;
    
#define X(N,...) result.vertex_##N = glGetAttribLocation(program, "vertex_" #N);
    XAttribute(X);
#undef X
    
#define X(N) result.N = glGetUniformLocation(program, #N);
    XUniform(X);
#undef X
    
    return(result);
}

#define GLOffsetStruct(p,m) ((void*)(OffsetOfMemberStruct(p,m)))
#define GLOffset(S,m)       ((void*)(gb_offset_of(S,m)))

Texture_ID global_game_texture;

internal void
gl_vertex_attrib_pointer(GLuint index, GLint size, GLenum type, const void *pointer)
{
    GLsizei stride = sizeof(Render_Vertex);
    if (type == GL_FLOAT)
    {
        glVertexAttribPointer(index, size, GL_FLOAT, true, stride, pointer);
    }
    else if (type == GL_UNSIGNED_INT)
    {
        glVertexAttribIPointer(index, size, GL_UNSIGNED_INT, stride, pointer);
    }
    else
    {
        invalid_code_path;
    }
}

internal void
gl_render(Render_Target *t)
{
    Font_Set *font_set = (Font_Set*)t->font_set;
    
    local_persist b32 first_opengl_call = true;
    local_persist u32 attribute_buffer = 0;
    local_persist GL_Program gl_program      = {};
    local_persist GL_Program gl_program_game = {};
    
    if (first_opengl_call)
    {
        first_opengl_call = false;
        
#if !SHIP_MODE
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        if (glDebugMessageControl)
        {
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, 0, false);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, 0, false);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, 0, true);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, 0, true);
        }
        
        if (glDebugMessageCallback)
        {
            glDebugMessageCallback(gl__error_callback, 0);
        }
#endif
        
        ////////////////////////////////
        
        GLuint dummy_vao = 0;
        glGenVertexArrays(1, &dummy_vao);
        glBindVertexArray(dummy_vao);
        
        ////////////////////////////////
        
        glGenBuffers(1, &attribute_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, attribute_buffer);
        
        ////////////////////////////////
        
        glEnable(GL_SCISSOR_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        ////////////////////////////////
        
        gl_program      = gl__make_program(gl__header, gl__vertex_shader, gl__fragment_shader);
        gl_program_game = gl__make_program(gl__header, gl__vertex_shader_game, gl__fragment_shader_game);
        glUseProgram(gl_program.program);
        
        ////////////////////////////////
        
        t->fallback_texture_id = gl__get_texture(V3i32(2, 2, 1), TextureKind_Mono);
        u8 white_block[] = { 0xFF, 0xFF, 0xFF, 0xFF, };
        gl__fill_texture(TextureKind_Mono, Texture_ID{}, V3i32(0, 0, 0), V3i32(2, 2, 1), white_block);
    }
    
    i32 width  = t->width;
    i32 height = t->height;
    
    glViewport(0, 0, width, height);
    glScissor(0, 0, width, height);
    glClearColor(1.f, 0.f, 1.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    t->bound_texture = {};
    
    for (Render_Free_Texture *free_texture = t->free_texture_first;
         free_texture != 0;
         free_texture = free_texture->next)
    {
        glDeleteTextures(1, &free_texture->tex_id);
    }
    t->free_texture_first = 0;
    t->free_texture_last = 0;
    
    for (Render_Group *group = t->group_first;
         group != 0;
         group = group->next)
    {
        i32 vertex_count = group->vertex_list.count;
        if (vertex_count <= 0) continue;
        
        b32 game_mode = (group->face_id == FACE_ID_GAME);
        GL_Program *program = game_mode ? &gl_program_game : &gl_program;
        
        {
            Rect_i32 box = Ri32(group->clip_box);
            GLint   scissor_x = box.x0;
            GLint   scissor_y = (group->y_is_up?
                                 box.y0 :
                                 height - box.y1);
            GLsizei scissor_w = box.x1 - box.x0;
            GLsizei scissor_h = box.y1 - box.y0;
            
            kv_clamp_bot(scissor_x, 0);
            kv_clamp_bot(scissor_y, 0);
            kv_clamp_bot(scissor_w, 0);
            kv_clamp_bot(scissor_h, 0);
            
            glScissor(scissor_x, scissor_y, scissor_w, scissor_h);
        }
       
        if (game_mode)
        {
            gl__bind_texture(t, global_game_texture);
        }
        else 
        {
            Face *face = font_set_face_from_id(font_set, group->face_id);
            if (face)
            {
                gl__bind_texture(t, face->texture);
            }
            else
            {
                gl__bind_fallback_texture(t);
            }
        }
        
        if (game_mode)
        {// NOTE(kv): game_mode is the rare case so we'll just temporary swap out the program
            glUseProgram(gl_program_game.program);
        }
        
        glBufferData(GL_ARRAY_BUFFER, vertex_count*sizeof(Render_Vertex), 0, GL_STREAM_DRAW);
        i32 cursor = 0;
        for (Render_Vertex_Array_Node *node = group->vertex_list.first;
             node != 0;
             node = node->next)
        {
            i32 size = node->vertex_count*sizeof(*node->vertices);
            glBufferSubData(GL_ARRAY_BUFFER, cursor, size, node->vertices);
            cursor += size;
        }
        
        // NOTE: glEnableVertexAttribArray
#define X(N,...) { glEnableVertexAttribArray(program->vertex_##N); }
        XAttribute(X);
#undef X
        
        // NOTE: tell opengl how to read vertex attributes
#define X(N,S,T) { gl_vertex_attrib_pointer(program->vertex_##N, S, T, GLOffset(Render_Vertex, N)); }
        XAttribute(X);
#undef X
        
        // NOTE: Transforms
        // NOTE(kv): offset to opengl's bilateral normalized space, as well as render group offset
        glUniform2f(program->view_t, 
                    group->offset.x - (f32)width/2.f, 
                    group->offset.y - (f32)height/2.f);
        f32 view_m_y_sign = ((group->y_is_up) ? +1 : -1);
        f32 view_m[4] = 
        {
            2.f/width, 0.f,
            0.f, 2.f/height * view_m_y_sign,
        };
        glUniformMatrix2fv(program->view_m, 1, GL_FALSE, view_m);
        glUniform1i(program->sampler, 0);  // NOTE(kv): idk if this has a meaning?
        
        // NOTE
        glDrawArrays(GL_TRIANGLES, 0, vertex_count);
        
        // NOTE: disable
#define X(N,...) { glDisableVertexAttribArray(program->vertex_##N); }
        XAttribute(X);
#undef X
       
        if (game_mode)
        {// NOTE: switch back to the normal program
            glUseProgram(gl_program.program);
        }
    } // for (Render_Group *group = t->group_first;
    
    glFlush();
}
