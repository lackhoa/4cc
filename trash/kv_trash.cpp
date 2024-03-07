// internal Model
// new_model(const char *filename)
{
    Model result = {};
    
    FILE *file = fopen(filename, "r");
    if (!file) return result;
    
    char *line = 0;
    umm line_len = 0;
    b32 success = true;
    while (true)
    {
        imm read_size = getline(&line, &line_len, file);
        if (read_size == -1) break;
      
        if (line_len < 2) continue;
        
        char c0 = line[0];
        char c1 = line[1];
        
        if (c0 == 'v' && c1 == ' ')
        {
            arrput(model.verts);
            v3 v = arrlen(model.verts)-1;
            if ( sscanf(line, "v %f %f %f", v, v+1, v+2) != 3 )
            {
                success = false;
                break;
            }
        }
        else if (c0 == 'f' && c1 == ' ')
        {
            i32 itrash;
            i32 output = scanf(line, "f %i/%i/%i %i/%i/%i %i/%i/%i %i/%i/%i",
                               i, &itrash, &itrash,
                               );
            if (output != 3) invalid_code_path;
            {
            }
            while (iss >> idx >> trash >> itrash >> trash >> itrash) 
            {
                idx--; // in wavefront obj all indices start at 1, not zero
                f.push_back(idx);
            }
            faces_.push_back(f);
        }
        else { /* passthrough for other */ }
    }
    
    if (!success) invalid_code_path;
    
    return model;
}

// NOTE(kv): very hacky stuff!
internal b32
optional_char(stb_lexer *tk)
{
    stb_lexer tk_save = *tk;
    b32 result = false;
    if (*tk->parse_point && tk->parse_point != tk->eof)
    {
        tk->parse_point++;
        if (tk->parse_point)
    }
    
    if (!result)  *tk = tk_save;
        
    return result;
}

// NOTE(kv): "geometry.h"

template <class t> struct Vec2 
{
	union 
    {
		struct {t u, v;};
		struct {t x, y;};
		t raw[2];
	};
	Vec2() : u(0), v(0) {}
	Vec2(t _u, t _v) : u(_u),v(_v) {}
};

template <class t> struct Vec3 
{
	union 
    {
		struct {t x, y, z;};
		struct { t ivert, iuv, inorm; };
		t raw[3];
	};
	Vec3() : x(0), y(0), z(0) {}
	inline Vec3<t> operator ^(const Vec3<t> &v) const { return Vec3<t>(y*v.z-z*v.y, z*v.x-x*v.z, x*v.y-y*v.x); }
	Vec3<t> & normalize(t l=1) { *this = (*this)*(l/norm()); return *this; }
};

    if (1)
    {// test srgb
        // background
        draw_rect(app, rect2_min_dim(V2(0,0), V2(400,400)), 0xff000000);
        // 50%-blended white
        draw_rect(app, rect2_min_dim(V2(0,0), V2(200,200)), 0x80ffffff);
        // midpoint gray in srgb value (which isn't even the correct blending)
        // draw_rect(app, rect2_min_dim(V2(200,0), V2(200,200)), 0xff808080);
        // linear blend
        draw_rect(app, rect2_min_dim(V2(0,200), V2(200,200)), 0xffbababa);
        
        //c 10 + 11*16
        
        // defcolor_base
        //draw_rect(app, rect2_min_dim(V2(200,200), V2(200,200)), 0xff777700);
        
        // a*x / (1-a)*y
        // (x^2.2) / (1-a)*(y^2.2))^0.4545
        
        // x/(foreground) should be 1, y/(background) should be 0
        // blended color in linear space is: 0.501961
        // blended color in srgb space is:   0.731062 -> 186 -> 0xBA
        // blended color in clip space 
        
        // if done wrong: 0.5 -> 0x80
        //c 0.5*100
    }

    fslider(software_rendering_slider, 0.0f);
    if (software_rendering_slider > 0)
    {// NOTE: software rendering experiments
        v3i dim = {768, 768, 1};
        local_persist b32 initial = true;
        local_persist u32 *data = 0;
        local_persist Texture_ID game_texture = {};
        Model model = {};
        b32 draw_mesh = def_get_config_b32(vars_intern_lit("draw_mesh"));
        if (initial)
        {
            initial = false;
            if (draw_mesh)
            {
                char *filename = "C:/Users/vodan/4ed/4coder-non-source/tiny_renderer/diablo3_pose.obj";
                model = new_model(filename);
            }
            // create game texture
            game_texture = graphics_get_texture(dim, TextureKind_ARGB);
            graphics_set_game_texture(game_texture);
            // allocate memory
            umm size = 4 * dim.x * dim.y * dim.z;
            data = (u32 *)malloc(size);
            block_zero(data, size);
        }
        
        tiny_renderer_main(dim.xy, data, &model);
        graphics_fill_texture(TextureKind_ARGB, game_texture, v3i{}, dim, data);
        
        v2 position = v2{10,10};
        fslider( scale, 0.338275f );
        v2 draw_dim = scale * castV2(dim.x, dim.y);
        draw_textured_rect(app, rect2_min_dim(position, draw_dim));
    }
