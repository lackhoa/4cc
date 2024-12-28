//~ NOTE(rjf): Divider Comments

#include "4coder_fleury_ubiquitous.h"

#if 0
function i64
_F4_Boundary_DividerComment(App *app, Buffer_ID buffer, 
                            Side side, Scan_Direction direction, i64 pos,
                            String signifier)
{
    i64 result = pos;
    Scratch_Block scratch(app);
    
    Token_Array tokens = get_token_array_from_buffer(app, buffer);
    if(tokens.tokens != 0)
    {
        Token_Iterator_Array it = token_it_at_pos(0, &tokens, pos);
        switch(direction)
        {
            case Scan_Forward:
            {
                for(;token_it_inc_non_whitespace(&it);)
                {
                    Token *token = token_it_read(&it);
                    if(token == 0)
                    {
                        break;
                    }
                    if(token->kind == TokenBaseKind_Comment)
                    {
                        String str = push_buffer_range(app, scratch, buffer, Ii64(token));
                        if(str.size >= signifier.size &&
                           string_match(string_substring(str, Ii64(0, signifier.size)), signifier))
                        {
                            result = token->pos;
                            break;
                        }
                    }
                }
            }break;
            
            case Scan_Backward:
            {
                for(;token_it_dec_non_whitespace(&it);)
                {
                    Token *token = token_it_read(&it);
                    if(token == 0)
                    {
                        break;
                    }
                    if(token->kind == TokenBaseKind_Comment)
                    {
                        String str = push_buffer_range(app, scratch, buffer, Ii64(token));
                        if(str.size >= signifier.size &&
                           string_match(string_substring(str, Ii64(0, signifier.size)), signifier))
                        {
                            result = token->pos;
                            break;
                        }
                    }
                }
            }break;
            
        }
    }
    return(result);
}

function i64
F4_Boundary_DividerComment(App *app, Buffer_ID buffer, 
                           Side side, Scan_Direction direction, i64 pos,
                           String signifier)
{
    return _F4_Boundary_DividerComment(app, buffer, side, direction, pos, strong_divider_comment_signifier);
}

CUSTOM_COMMAND_SIG(f4_move_to_next_divider_comment)
CUSTOM_DOC("Seek right for next divider comment in the buffer.")
{
    Scratch_Block scratch(app);
    current_view_scan_move(app, Scan_Forward, push_boundary_list(scratch, F4_Boundary_DividerComment));
}

CUSTOM_COMMAND_SIG(f4_move_to_prev_divider_comment)
CUSTOM_DOC("Seek left for previous divider comment in the buffer.")
{
    Scratch_Block scratch(app);
    current_view_scan_move(app, Scan_Backward, push_boundary_list(scratch, F4_Boundary_DividerComment));
}
#endif
