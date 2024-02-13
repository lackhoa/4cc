internal b32
require_number(App *app, Buffer_ID buffer, Token_Iterator_Array *tk, f32 *out)
{
    *out = 0;
    Scratch_Block temp(app);
    if ( !token_it_inc(tk) ) return false;
    
    // note(kv): parse the sign
    f32 sign = 1;
    if (tk->ptr->kind == TokenBaseKind_Operator)
    {
        String8 string = push_token_lexeme(app, temp, buffer, tk->ptr);
        if (string_equal(string, "-"))      sign = -1;
        else if (string_equal(string, "+")) {}
        else                                return false;
        
        if ( !token_it_inc(tk)) return false;
    }
   
    if (tk->ptr->kind != TokenBaseKind_LiteralFloat &&
        tk->ptr->kind != TokenBaseKind_LiteralInteger )
    {
        return false;
    }
    
    String8 string = push_token_lexeme(app, temp, buffer, tk->ptr);
    char *cstring = to_c_string(temp, string);
    *out = sign * gb_str_to_f32(cstring, 0);
    
    return true;
}

internal b32
maybe_parse_number(App *app, Buffer_ID buffer, Token_Iterator_Array *tk, f32 *out)
{
    Token_Iterator_Array tk_save = *tk;
    b32 result = require_number(app, buffer, tk, out);
    if (!result) *tk = tk_save;

    return result;
}

