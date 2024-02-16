// NOTE(kv): This is yet another parser based on 4coder's Token Iterator

struct Quick_Parser
{
    b32 ok;
    App *app;
    Buffer_ID buffer;
    Token_Iterator_Array tk;
};

internal Quick_Parser
qp_new(App *app, Buffer_ID buffer, Token_Iterator_Array *tk)
{
    Quick_Parser result = 
    {
        .ok = true,
        .app = app,
        .buffer = buffer,
        .tk = *tk,
    };
    return result;
}

internal Token *
qp_get_token(Quick_Parser *p)
{
    Token *token = 0;
    if (p->ok)
    {
        token = token_it_read(&p->tk);
    }
    return token;
}

internal String8
qp_push_token(Quick_Parser *p, Arena *arena)
{
    if (Token *token = qp_get_token(p))
    {
        return push_token_lexeme(p->app, arena, p->buffer, token);
    }
    return {};
}

internal i64
qp_get_pos(Quick_Parser *p)
{
    if (Token *token = qp_get_token(p))
    {
        return token->pos;
    }
    else return 0;
}

internal Token *
qp_eat_token(Quick_Parser *p)
{
    if (p->ok)
    {
        p->ok = token_it_inc(&p->tk);
        return token_it_read(&p->tk);
    }
    else return 0;
}

// TODO: @hack
internal i64
get_next_token_pos(Quick_Parser *p)
{
    i64 result = 0;
    if (p->ok)
    {
        Quick_Parser save = *p;
        if (Token *token = qp_eat_token(p))
        {
            result = token->pos;
        }
        *p = save;
    }
    return result;
}

internal b32
qp_eat_number(Quick_Parser *p, f32 *out)
{
    X_Block xblock(p->app);
    
    f32 sign = 1;
    Token *token = qp_eat_token(p);
    if (token)
    {
        if (token->kind == TokenBaseKind_Operator)
        {// NOTE(kv): parse sign
            String8 string = qp_push_token(p, xblock);
            if ( string_equal(string, "-") )
            {
                sign = -1;
            }
            else if (!string_equal(string, "+"))
            {
                p->ok = false;
            }
            
            token = qp_eat_token(p);
        }
    }
   
    if (p->ok)
    {
        if (token->kind != TokenBaseKind_LiteralFloat &&
            token->kind != TokenBaseKind_LiteralInteger )
        {
            p->ok = false;
        }
    }
   
    if (p->ok)
    {
        String8 string = qp_push_token(p, xblock);
        char *cstring = to_c_string(xblock, string);
        *out = sign * gb_str_to_f32(cstring, 0);
    }
    
    return p->ok;
}

internal b32
qp_maybe_eat_number(Quick_Parser *p, f32 *out)
{
    Token_Iterator_Array tk_save = p->tk;
    b32 result = qp_eat_number(p, out);
    if (!result) p->tk = tk_save;

    return result;
}

internal b32
qp_eat_token_kind(Quick_Parser *p, Token_Base_Kind kind)
{
    if ( Token *token = qp_eat_token(p) )
    {
        p->ok = (token->kind == kind);
    }
    return p->ok;
}

internal b32
qp_maybe_eat_token_kind(Quick_Parser *p, Token_Base_Kind kind)
{
    Quick_Parser p_save = *p;
    b32 result = qp_eat_token_kind(p, kind);
    if (!result) *p = p_save;
    return result;
}

internal b32
qp_eat_char(Quick_Parser *p, u8 chr)
{
    if ( qp_eat_token(p) )
    {
        X_Block xblock(p->app);
        String8 string = qp_push_token(p, xblock);
        p->ok = string_equal(string, chr);
    }
    return p->ok;
}

inline b32
qp_eat_comma(Quick_Parser *p)
{
    return qp_eat_char(p, ',');
}

// NOTE returns index + 1
internal u32
char_in_string(String8 chars, char chr)
{
    u32 result = 0;
    for_u32 (index, 0, (u32)chars.size)
    {
        if (chars.str[index] == chr)
        {
            result = index + 1;
            break;
        }
    }
    return result;
}

#define qp_eat_until_lit(p, cstring) qp_eat_until_char(p, str8lit(cstring));

// NOTE returns index + 1
internal u32
qp_eat_until_char(Quick_Parser *p, String8 chars)
{
    u32 result = 0;
    while ( !result && qp_eat_token(p) )
    {
        X_Block xblock(p->app);
        String8 token = qp_push_token(p, xblock);
        if ((token.size == 1) && 
            (result = char_in_string(chars, token.str[0])))
        {
            // break
        }
        // TODO: crappy recursion time!
        else if ( string_equal(token, '(') )
        {
            qp_eat_until_lit(p, ")");
        }
        else if ( string_equal(token, '[') )
        {
            qp_eat_until_lit(p, "]");
        }
        else if ( string_equal(token, '{') )
        {
            qp_eat_until_lit(p, "}");
        }
    }
    return result;
}
