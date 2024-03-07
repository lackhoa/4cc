/*
 * Mr. 4th Dimention - Allen Webster
 *
 * 10.11.2017
 *
 * OpenGL render implementation
 *
 */

// NOTE(kv): ARGB colors flowing through the system are in srgb space,
// which incurs penalty in shaders, since we wanna blend in linear space.

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
            pixel_format = GL_SRGB8_ALPHA8;  // todo: maybe default to GL_BGRA?
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
    uniform mat4x4 view_m;
    uniform bool linear_alpha_blend;

    in vec3 vertex_xyz;
    in vec3 vertex_uvw;
    in vec4 vertex_color;
    in float vertex_half_thickness;

    smooth out vec4 fragment_color;
    smooth out vec3 uvw;
    smooth out vec2 xy;
    smooth out vec2 adjusted_half_dim;
    smooth out float half_thickness;

    void main(void)
    {
        gl_Position = view_m * vec4(vertex_xyz, 1);
        fragment_color.rgba = vertex_color.bgra;
        if (linear_alpha_blend)
        {
            fragment_color.rgb *= (fragment_color.rgb);
        }
        uvw = vertex_uvw;
        vec2 center = vertex_uvw.xy;
        xy = vertex_xyz.xy;
        vec2 half_dim = abs(xy - center);
        adjusted_half_dim = half_dim - vertex_uvw.zz + vec2(0.5, 0.5);
        half_thickness = vertex_half_thickness;
    }
    )foo";

global char *gl__fragment_shader =
R"foo(
    uniform sampler2DArray sampler;
    //
    smooth in vec4 fragment_color;
    smooth in vec3 uvw;
    smooth in vec2 xy;
    smooth in vec2 adjusted_half_dim;
    smooth in float half_thickness;
    //
    out vec4 out_color;
    
    float rectangle_sd(vec2 p, vec2 b)
    {
        vec2 d = abs(p) - b;
        return(length(max(d, vec2(0,0))) + 
               min(max(d.x, d.y), 0.0));
    }
    
    void main(void)
    {
        float value;
        {
            bool has_thickness = half_thickness > 0.49;
            if (has_thickness)
            {
                vec2 center = uvw.xy;
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
        
        out_color = vec4(fragment_color.rgb, fragment_color.a*value);
    }
   )foo";

// IMPORTANT(kv): Unmaintained
global char *gl__vertex_shader_game =
R"foo(
    uniform mat4x4 view_m;

    in vec2 vertex_xy;
    in vec3 vertex_uvw;
    in vec4 vertex_color;
    in float vertex_half_thickness;

    smooth out vec4 fragment_color;
    smooth out vec3 uvw;

    void main(void)
    {
        gl_Position = view_m * vec4(vertex_xy, 0,1);
        fragment_color.rgba = vertex_color.bgra;
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

#define X_VERTEX_ATTRIBUTES(X)    \
    X(xyz)          \
    X(uvw)         \
    X(color)       \
    X(half_thickness)

#define X_UNIFORMS(X) \
    X(linear_alpha_blend) \
    X(view_m) \
    X(sampler)

struct GL_Program
{
    u32 program;
    
#define X(N,...) i32 vertex_##N;
    X_VERTEX_ATTRIBUTES(X)
#undef X
    
#define X(N) i32 N;
    X_UNIFORMS(X)
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
    X_VERTEX_ATTRIBUTES(X);
#undef X
    
#define X(N) result.N = glGetUniformLocation(program, #N);
    X_UNIFORMS(X);
#undef X
    
    return(result);
}

#define GLOffsetStruct(p,m) ((void*)(OffsetOfMemberStruct(p,m)))
#define GLOffset(S,m)       ((void*)(gb_offset_of(S,m)))

global Texture_ID global_game_texture;

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
        
        glEnable(GL_FRAMEBUFFER_SRGB);  // NOTE(kv): Used to not have this, so our blending was "wrong", but actually it was right for some fonts
       
        glDepthFunc(GL_LEQUAL);  // NOTE: "equal" because we want things drawn later to be on top, prolly doesn't matter
        
        ////////////////////////////////
        
        gl_program      = gl__make_program(gl__header, gl__vertex_shader, gl__fragment_shader);
        gl_program_game = gl__make_program(gl__header, gl__vertex_shader_game, gl__fragment_shader_game);
        glUseProgram(gl_program.program);
        
        ////////////////////////////////
        
        t->fallback_texture_id = gl__get_texture(V3i32(2, 2, 1), TextureKind_Mono);
        u8 white_block[] = { 0xFF, 0xFF, 0xFF, 0xFF, };
        gl__fill_texture(TextureKind_Mono, Texture_ID{}, V3i32(0, 0, 0), V3i32(2, 2, 1), white_block);
    }
    
    glViewport(0, 0, t->width, t->height);
    glScissor (0, 0, t->width, t->height);
    glClearColor(1.f, 0.f, 1.f, 1.f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    
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
   
    b8 depth_test = 0;
    for (Render_Group *group = t->group_first;
         group != 0;
         group = group->next)
    {
        i32 vertex_count = group->vertex_list.count;
        if (vertex_count > 0)
        {
            b32 game_mode = (group->face_id == FACE_ID_GAME);
            GL_Program *program = game_mode ? &gl_program_game : &gl_program;
           
            if ( group->depth_test != depth_test )
            {
                depth_test = group->depth_test;
                if (depth_test)
                    glEnable (GL_DEPTH_TEST);
                else
                    glDisable(GL_DEPTH_TEST);
            }
            
            {
                Rect_i32 box = Ri32(group->clip_box);
                GLint   scissor_x = box.x0;
                GLint   scissor_y = (group->y_is_up?
                                     box.y0 :
                                     t->height - box.y1);
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
            
            // NOTE(kv): In the editor, rectangles and fonts are mixed together in one Render_Group.
            // So we can't quite say "enable srgb for fonts"
            if (group->linear_alpha_blend)
            {
                glEnable (GL_FRAMEBUFFER_SRGB);
            }
            else
            {
                glDisable(GL_FRAMEBUFFER_SRGB);
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
            X_VERTEX_ATTRIBUTES(X);
#undef X
            
            // NOTE: tell opengl how to read vertex attributes
#define INDEX(N)  program->vertex_##N
            {
                GLsizei stride = sizeof(Render_Vertex);
                glVertexAttribPointer(INDEX(xyz),            3, GL_FLOAT,         GL_TRUE, stride, GLOffset(Render_Vertex, xyz));
                glVertexAttribPointer(INDEX(uvw),            3, GL_FLOAT,         GL_TRUE, stride, GLOffset(Render_Vertex, uvw));
                glVertexAttribPointer(INDEX(color),          4, GL_UNSIGNED_BYTE, GL_TRUE, stride, GLOffset(Render_Vertex, color));
                glVertexAttribPointer(INDEX(half_thickness), 1, GL_FLOAT,         GL_TRUE, stride, GLOffset(Render_Vertex, half_thickness));
            }
#undef INDEX
            
            {// NOTE: Transforms
                v1 width  = (v1)t->width;
                v1 height = (v1)t->height;
                v1 y_sign = ((group->y_is_up) ? +1 : -1);
                // NOTE(kv): Scale
                v1 a = 2.f/width;
                v1 b = 2.f/height * y_sign;
                // NOTE(kv): Offset to opengl's bilateral normalized space, as well as render group offset
                v1 c = a * group->offset.x - 1.0f;
                v1 d = b * group->offset.y - 1.0f*y_sign;
                m4x4 view_m = 
                {
                    {
                        a,0,0,c,
                        0,b,0,d,
                        0,0,1,0,
                        0,0,0,1,
                    }
                };
               
                if (group->is_perspective)
                {
                    Camera *camera = &group->camera;
                   
                    m4x4 camera_project =
                    {
                        {
                            V4(camera->px,0),
                            V4(camera->py,0),
                            V4(camera->pz,0),
                            V4(0,0,0,1),
                        }
                    };
                    
                    m4x4 z_to_depth =
                    {
                        {
                           V4(1,0,0,0),
                           V4(0,1,0,0),
                           V4(0,0,-1,camera->distance),
                           V4(0,0,0,1),
                        } 
                    };
                    
                    v1 focal = camera->focal_length;
                    v1 n  = 10.0f;         // NOTE: in depth
                    v1 f  = 100.0f * 1E3;  // NOTE: in depth
                   
                    m4x4 perspective_project =
                    {
                        {
                            focal, 0,     0,            0,
                            0,     focal, 0,            0,
                            0,     0,     (-n-f)/(n-f), 2*f*n/(n-f),
                            0,     0,     1,            0,
                        },
                    };
                    
                    m4x4 camera_project_and_z_to_depth = (z_to_depth * camera_project);
                    m4x4 camera_project_and_z_to_depth_and_perspective = perspective_project * camera_project_and_z_to_depth;
                    view_m = view_m * camera_project_and_z_to_depth_and_perspective;
                    
                    if (0)
                    {
                        v1 U = 1e3;
                        v4 O = v4{0,0,0,1};
                        v4 O1 = camera_project * O;
                        v4 O2 = z_to_depth * O1;
                        v4 O3 = perspective_project * O2;
                        O3 /= O3.w;
                            
                        v4 point = U * v4{0,0,1,1};
                        v4 point1 = camera_project * point;
                        v4 point2 = z_to_depth * point1;
                        v4 point3 = perspective_project * point2;
                        point3 /= point3.w;
                       
                        breakhere;
                    }
                }
                
                glUniformMatrix4fv(program->view_m, 1, GL_TRUE, (v1*)view_m.v);
                glUniform1i       (program->linear_alpha_blend, group->linear_alpha_blend);
                glUniform1i       (program->sampler, 0);  // NOTE(kv): idk if this has a meaning?
            }
            
            // NOTE: draw
            glDrawArrays(GL_TRIANGLES, 0, vertex_count);
            
            // NOTE: Disable Vertex Attrib Array
#define X(N,...) { glDisableVertexAttribArray(program->vertex_##N); }
            X_VERTEX_ATTRIBUTES(X);
#undef X
            
            if (game_mode)
            {// NOTE: switch back to the normal program
                glUseProgram(gl_program.program);
            }
        }
    }
    
    glFlush();
}
