// Tiny Lexer ////////////////////////////////////////////////

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
    f64 value = gb_str_to_f64(lex->at, &end);
    
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
