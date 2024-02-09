struct FUI
{
  Application_Links *app;
  Buffer_ID buffer;
  Token_Iterator_Array tk;
};

global v4  fui_slider_direction;
global b32 fui_slider_active;

internal b32
fui__at_slider_p(FUI *f)
{
  App *app = f->app;
  Token_Iterator_Array tk_value = kv_token_it_at_cursor(app); 
  auto *tk = &tk_value;
  f->tk = *tk;
  if ( !tk->tokens ) return false;
 
  Token *token = tk->ptr;
  Scratch_Block scratch(app);
  b32 at_slider = token_equal_cstring(app, f->buffer, token, "fslider");
  return at_slider;
}

internal void
fui_draw_slider(FApp *app, Buffer_ID buffer, rect2 region)
{
    FUI f_value; 
    auto *f = &f_value;
    f->app    = app;
    f->buffer = buffer;
    
    b32 at_slider = fui__at_slider_p(f);
    i32 slider_index = 0;
    Fui_Slider *slider = fui_slider_get(slider_index);
    
    if (at_slider && slider)
    {
        v2 slider_radius = v2{50,50};
        v2 slider_dim    = 2 * slider_radius;
        v2 slider_origin = region.max - slider_radius;
        {// NOTE: the whole slider outline
            rect2 rect = rect_center_dim(slider_origin, slider_dim);
            v4 color = v4{1,1,1,1};
            if (!fui_slider_active) color.a = 0.5f;
            draw_rect_outline(app, rect, 2, color);
        }
        {// NOTE: the slider cursor
            i32 index = 0;
            v2 v = slider->value.xy;
            v.y = -v.y;  // NOTE: invert y to fit math coordinates
            v2 center = slider_origin + hadamard(v, slider_radius);
            rect2 rect = rect2_center_radius(center, v2{5,5});
            draw_rect(app, rect, v4{0,0.5f,0,1});
        }
    }
}

internal b32
require_number(FApp *app, Buffer_ID buffer, Token_Iterator_Array *tk, f32 *out)
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

// TODO(kv): don't need the parser yo!
internal b32
maybe_parse_number(FApp *app, Buffer_ID buffer, Token_Iterator_Array *tk, f32 *out)
{
    Token_Iterator_Array tk_save = *tk;
    b32 result = require_number(app, buffer, tk, out);
    if (!result) *tk = tk_save;

    return result;
}

internal b32
fui_handle_slider(FApp *app, Buffer_ID buffer)
{
    FUI  f_value; 
    FUI *f = &f_value;
    f->app    = app;
    f->buffer = buffer;
    
    b32 at_slider = fui__at_slider_p(f);
    
    i32 slider_index = 0;
    Fui_Slider *slider = fui_slider_get(slider_index);
    
    if (at_slider && slider)
    {
        Token_Iterator_Array *tk = &f->tk;
        
        Token *token = tk->ptr;
        i64 slider_begin = token->pos;
        
        b32 parse_ok = false;
        {// Parsing
            if ( require_token_kind(tk, TokenBaseKind_ParentheticalOpen) ) 
            {
                b32 found_paren_close = false;
                do
                {
                    if (tk->ptr->kind == TokenBaseKind_ParentheticalClose)
                    {
                        found_paren_close = true;
                        break;
                    }
                }
                while ( token_it_inc(tk) );
                
                if (found_paren_close) 
                    parse_ok = true;
            }
        }
        
        if (parse_ok)
        {
            i64 slider_end = tk->ptr->pos + 1;
            v4 slider_value_save = slider->value;
            b32 write_back = false;
            for (;;)
            { // note: ui loop
                Scratch_Block temp(app);
                
                fui_slider_active = true; defer(fui_slider_active = false);
                
                User_Input in = get_next_input(app, EventPropertyGroup_Any, EventProperty_Escape);
                if (in.abort) break; 
                
                b32 keydown = (in.event.kind == InputEventKind_KeyStroke);
                b32 keyup   = (in.event.kind == InputEventKind_KeyRelease);
                if (keydown || keyup)
                {
                    v4 &direction = fui_slider_direction;
#define Match(CODE)  in.event.key.code == KeyCode_##CODE
                    
                    if ( Match(Return) && keydown )
                    {
                        write_back = true; 
                        break;
                    }
                    else if (Match(L)) direction.x = keydown ? +1 : 0;
                    else if (Match(H)) direction.x = keydown ? -1 : 0;
                    else if (Match(K)) direction.y = keydown ? +1 : 0;
                    else if (Match(J)) direction.y = keydown ? -1 : 0;
                    else if (Match(I)) direction.z = keydown ? +1 : 0;
                    else if (Match(O)) direction.z = keydown ? -1 : 0;
#undef Match
                }
                else leave_current_input_unhandled(app);
            }
            
            if (write_back)
            { //note(kv): save the results back
                Scratch_Block temp(app);
                String8 save = {};
                switch (slider->type)
                {
                    case Fui_Slider_Type_v1:
                    {
                        save = push_stringf(temp, "fslider( %f )", slider->value.x);
                    }break;
                    
                    case Fui_Slider_Type_v2:
                    {
                        save = push_stringf(temp, "fslider( %f, %f )", slider->value.xy);
                    }break;
                    
                    default: todo_incomplete; 
                }
                
                buffer_replace_range(app, buffer, slider_begin, slider_end, save);
            }
            else 
                slider->value = slider_value_save;
            
            return true;
        }
        else return false;
    }
    else return false;
}

internal void
fui_tick(FApp *app, Frame_Info frame_info)
{
    i32 slider_index = 0;
    Fui_Slider *slider = fui_slider_get(slider_index);
    
    // DEBUG_text("fui_slider_value", fui_slider_value);
    // DEBUG_text("fui_slider_direction", fui_slider_direction);
    if (slider && 
        fui_slider_direction != v4{} )
    {
        f32 speed = 1.0f;
        f32 dt = frame_info.animation_dt;  // NOTE(kv): using actual literal_dt would trigger a big jump when the user initially presses a key
        slider->value += dt * speed * fui_slider_direction;
        animate_in_n_milliseconds(app, 0);
    }
}
