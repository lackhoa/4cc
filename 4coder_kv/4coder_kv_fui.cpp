struct FUI
{
  Application_Links *app;
  Buffer_ID buffer;
  Token_Iterator_Array it;
};

global v3  fui_slider_direction;
global b32 fui_slider_active;

internal b32
fui__at_slider_p(FUI *f)
{
  Application_Links *app = f->app;
  Token_Iterator_Array it_value = kv_token_it_at_cursor(app); 
  auto *it = &it_value;
  f->it = *it;
  if ( !it->tokens ) return false;
 
  Token *token = it->ptr;
  Scratch_Block scratch(app);
  bool at_slider = token_equal_cstring(app, f->buffer, token, "fslider");
  return at_slider;
}

internal void
fui_draw_slider(Application_Links *app, Buffer_ID buffer, rect2 region)
{
  FUI f_value; 
  auto *f = &f_value;
  f->app    = app;
  f->buffer = buffer;
  
  b32 at_slider = fui__at_slider_p(f);
  if (!at_slider) return;

  v2 slider_radius = v2{50,50};
  v2 slider_dim    = 2 * slider_radius;
  v2 slider_origin = region.max - slider_radius;
  { //note: the whole slider outline
    rect2 rect = rect_center_dim(slider_origin, slider_dim);
    v4 color = v4{1,1,1,1};
    if (!fui_slider_active) color.a = 0.5f;
    draw_rect_outline(app, rect, 2, color);
  }
  { //note: the slider cursor
    v2 v = fui_slider_value.xy;
    v.y = -v.y;  // note: invert y to fit math coordinates
    v2 center = slider_origin + hadamard(v, slider_radius);
    rect2 rect = rect2_center_radius(center, v2{5,5});
    draw_rect(app, rect, v4{0,0.5f,0,1});
  }
}

// TODO(kv): the parser is SO bad
internal b32
parse_number(FApp *app, Buffer_ID buffer, Token_Iterator_Array *it, f32 *out)
{
    *out = 0;
    Scratch_Block temp(app);
    if ( !token_it_inc(it) ) return false;
    
    // note(kv): parse the sign
    f32 sign = 1;
    if (it->ptr->kind == TokenBaseKind_Operator)
    {
        String8 string = push_token_lexeme(app, temp, buffer, it->ptr);
        if (string_equal(string, "-"))      sign = -1;
        else if (string_equal(string, "+")) {}
        else                                return false;
        
        if ( !token_it_inc(it)) return false;
    }
   
    if (it->ptr->kind != TokenBaseKind_LiteralFloat &&
        it->ptr->kind != TokenBaseKind_LiteralInteger )
    {
        return false;
    }
    
    String8 string = push_token_lexeme(app, temp, buffer, it->ptr);
    char *cstring = to_c_string(temp, string);
    *out = sign * gb_str_to_f32(cstring, 0);
    
    return true;
}

internal b32
fui_handle_slider(FApp *app, Buffer_ID buffer)
{
    FUI f_value; 
    auto *f = &f_value;
    f->app    = app;
    f->buffer = buffer;
    
    b32 at_slider = fui__at_slider_p(f);
    if (!at_slider) return false;
    
    Token_Iterator_Array *it = &f->it;
    
    Token *token = it->ptr;
    i64 slider_begin = token->pos;
    
    breakable_block
    {
        f32 value_x = 0;
        f32 value_y = 0;
        if ( !require_token_kind(it, TokenBaseKind_ParentheticalOpen) ) break;
        if ( !parse_number(app, buffer, it, &value_x) ) break;
        if ( !require_token_kind(it, TokenBaseKind_StatementClose) ) break;
        if ( !parse_number(app, buffer, it, &value_y) ) break;
        if ( !require_token_kind(it, TokenBaseKind_ParentheticalClose ) ) break;
        i64 slider_end = it->ptr->pos + 1;
        
        fui_slider_value = v3{.x=value_x, .y=value_y};
        
#if 0
        {
            printf_message(app, temp, "value f32: %f\n", value);
            String8 string = push_buffer_range(app, temp, buffer, slider_begin, slider_end);
            printf_message(app, temp, "slider range: %.*s\n", string_expand(string));
        }
#endif
        
        b32 save_result = false;
        v3 fui_slider_save = fui_slider_value;
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
                v3 &direction = fui_slider_direction;
#define Match(CODE)  in.event.key.code == KeyCode_##CODE
                
                if ( Match(Return) && keydown )
                {
                    save_result = true; 
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
        
        if (save_result)
        { //note(kv): save the results back
            Scratch_Block temp(app);
            String8 save = push_stringf(temp, "fslider( %f, %f )", fui_slider_value.x, fui_slider_value.y);
            buffer_replace_range(app, buffer, slider_begin, slider_end, save);
        }
        else
        {
            fui_slider_value = fui_slider_save;
        }
    }
    
    return true;
}

internal void 
fui_tick(Application_Links *app, Frame_Info frame_info)
{
  DEBUG_clear;
  f32 speed = 1.0f;
  f32 dt = frame_info.animation_dt;  // using actual literal_dt would trigger a big jump when the user initially presses a key
  fui_slider_value += dt * speed * fui_slider_direction;
  DEBUG_text(fui_slider_value, fui_slider_value);
  DEBUG_text(fui_slider_direction, fui_slider_direction);
  if( fui_slider_direction != v3{0,0,0} )
  {
    animate_in_n_milliseconds(app, 0);
  }
}
