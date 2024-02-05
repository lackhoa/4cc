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
