struct Model
{
    v3   *vertices;
    i1 **faces;
};

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
    while ( lex->ok )
    {
        if ( maybe_eat_string(lex, "v ") )
        {// NOTE(kv): Vertex
            v3 vertex;
            for_i1 (i,0,3)
            {
                eat_float(lex);
                vertex.v[i] = lex->float_value;
            }
            arrput(model.vertices, vertex);
        }
        else if ( maybe_eat_string(lex, "f ") )
        {// NOTE(kv): Face
            i1 *face = 0;
            lex->newline_encountered = false;
            while ( lex->ok && !lexer_newline_encountered(lex) )
            {// NOTE(kv): Parse vertex indexes;
                eat_integer(lex);
                i1 idx = lex->int_value;
                eat_character(lex, '/');
                eat_integer(lex);
                eat_character(lex, '/');
                eat_integer(lex);
                arrput(face, idx-1);
            }
            arrput(model.faces, face);
        }
        else 
            eat_line(lex);
    }
    
    usize nvertices = arrlen(model.vertices);
    usize nfaces    = arrlen(model.faces);
    kv_assert( nvertices == expected_nvertices );
    kv_assert( nfaces    == expected_nfaces );
    
    gb_file_free_contents(&contents);
    return model;
}
