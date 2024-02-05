#if 0
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

#endif  // if0

struct Model
{
    v3   *vertices;
    i32 **faces;
};

// NOTE(kv): assume that the stream is nil-terminated.
struct Lexer
{
    b32 ok;
    // NOTE(kv): "at" never goes past the nil-terminator
    // NOTE(kv): "at" never point at a space character? (point in contention)
    char *at;
    b32 newline_encountered;
    
    i64 int_value;
    f64 float_value;
    
    char *string_value;
    u32 string_len;
    u32 string_buffer_size;
};

inline b32
lexer_ok(Lexer *lex)
{
    return lex->ok && *lex->at;
}

internal void
eat_all_spaces(Lexer *lex)
{
    while ( gb_char_is_space(*lex->at) )
    {
        if (*lex->at == '\n')  lex->newline_encountered = true;
        ++lex->at;
    }
}

internal Lexer
new_lexer(char *at, char *string_buffer, u32 string_buffer_size)
{
    Lexer lex = {};
    lex.ok = true;
    lex.at = at;
    lex.string_buffer_size = string_buffer_size;
    eat_all_spaces(&lex);
    return lex;
}

// NOTE(kv): also eats the sign.
internal void
eat_integer(Lexer *lex)
{
    char *end = 0;
    i64 value = gb_str_to_i64(lex->at, &end, 0);
    
    lex->int_value = value;
    lex->ok = ( end != lex->at );
    lex->at = end;
    
    eat_all_spaces(lex);
}

// NOTE(kv): also eats the sign.
internal void
eat_float(Lexer *lex)
{
    char *end = 0;
    i64 value = gb_str_to_f64(lex->at, &end);
    
    lex->float_value = value;
    lex->ok = ( end != lex->at );
    lex->at = end;
    
    eat_all_spaces(lex);
}

internal void
eat_character(Lexer *lex, char character)
{
    if ( *lex->at == character )  ++lex->at;
}

internal b32
maybe_eat_character(Lexer *lex, char character)
{
    if (*lex->at == character)
    {
        ++lex->at;
        eat_all_spaces(lex);
        return true;
    }
    else return false;
}

internal b32
maybe_eat_string(Lexer *lex, char *string)
{
    char *at_save = lex->at;
    char *string_cursor = string;
    while (*string_cursor && *lex->at)
    {
        if (*string_cursor == *lex->at)
        {
            ++string_cursor;
            ++lex->at;
        }
        else break;
    }
    if (*string_cursor == 0)
    {// successful
        eat_all_spaces(lex);
        return true;
    }
    else
    {
        lex->at = at_save;
        return false;
    }
}

internal void
eat_line(Lexer *lex)
{
    while ( lexer_ok(lex) )
    {
        if (*lex->at++ == '\n') break;
    }
    eat_all_spaces(lex);
}

inline b32
lexer_newline_encountered(Lexer *lex)
{
    b32 result = lex->newline_encountered;
    lex->newline_encountered = false;
    return result;
}

internal Model
new_model(const char *filepath)
{
    Model model = {};
    usize expected_nvertices = 2519;
    usize expected_nfaces    = 5022;
    arrsetcap(model.vertices, expected_nvertices);
    arrsetcap(model.faces,    expected_nfaces);
   
    gbAllocator a = gb_heap_allocator();
    gbFileContents contents = gb_file_read_contents(a, true, filepath);
    kv_assert(contents.data);
    char lexer_string_buffer[64];
    Lexer  lex_value = new_lexer((char *)contents.data, lexer_string_buffer, 64);
    Lexer *lex = &lex_value;
    while ( lexer_ok(lex) )
    {
        if ( maybe_eat_string(lex, "v ") )
        {// NOTE(kv): Vertex
            v3 vertex;
            for_i32 (i,0,3)
            {
                eat_float(lex);
                vertex.v[i] = lex->float_value;
            }
            arrput(model.vertices, vertex);
        }
        else if ( maybe_eat_string(lex, "f ") )
        {// NOTE(kv): Face
            i32 *face = 0;
            lex->newline_encountered = false;
            while ( lexer_ok(lex) && !lexer_newline_encountered(lex) )
            {// NOTE(kv): Parse vertex indexes;
                eat_integer(lex);
                i32 idx = lex->int_value;
                eat_character(lex, '/');
                eat_integer(lex);
                eat_character(lex, '/');
                eat_integer(lex);
                arrput(face, idx-1);
            }
            arrput(model.faces, face);
        }
        else 
        {
            eat_line(lex);
        }
    }
    
    usize nvertices = arrlen(model.vertices);
    usize nfaces    = arrlen(model.faces);
    kv_assert( nvertices == expected_nvertices );
    kv_assert( nfaces    == expected_nfaces );
    
    gb_file_free_contents(&contents);
    return model;
}
