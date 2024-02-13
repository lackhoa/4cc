struct FUI
{
    App *app;
    Buffer_ID buffer;
    Token_Iterator_Array tk;
};

internal String8
fui_push_slider_value(Arena *arena, Fui_Slider_Type type, Fui_Slider_Value value)
{
    String8 result = {};
    switch (type)
    {
        case Fui_Slider_Type_v1:
        {
            result = push_stringf(arena, "%f", value.v1);
        }break;
        case Fui_Slider_Type_v2:
        {
            v2 v = value.v2;
            result = push_stringf(arena, "v2{ %f, %f }", v.x, v.y);
        }break;
        case Fui_Slider_Type_v3:
        {
            v3 v = value.v3;
            result = push_stringf(arena, "v3{ %f, %f, %f }", v.x, v.y, v.z);
        }break;
        case Fui_Slider_Type_v4:
        {
            v4 v = value.v4;
            result = push_stringf(arena, "v4{ %f, %f, %f, %f }", v.x, v.y, v.z, v.w);
        }break;
        default: todo_incomplete;
    }
    return result;
}

global v4  fui_slider_direction;
global i32 fui_active_slider_index;

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
fui_draw_slider(App *app, Buffer_ID buffer, rect2 region)
{
    FUI f_value; 
    auto *f = &f_value;
    f->app    = app;
    f->buffer = buffer;
    
    b32 at_slider = fui__at_slider_p(f);
    if (at_slider)
    {
        v2 slider_radius = v2{50,50};
        v2 slider_dim    = 2 * slider_radius;
        v2 slider_origin = region.max - slider_radius;
        {// NOTE: the whole slider outline
            rect2 rect = rect2_center_dim(slider_origin, slider_dim);
            v4 color = v4{1,1,1,0.5f};
            if (fui_active_slider_index)  color.a = 1.0f;
            draw_rect_outline(app, rect, 2, color);
        }
        if (fui_active_slider_index)
        {// NOTE: the slider cursor
            Fui_Slider *slider = &fui_slider_store[fui_active_slider_index];
            v2 v = slider->value.v2;
            v.y = -v.y;  // NOTE: invert y to fit math coordinates
            v2 center = slider_origin + hadamard(v, slider_radius);
            rect2 rect = rect2_center_radius(center, v2{5,5});
            draw_rect(app, rect, pack_argb(v4{0,0.5f,0,1}));
        }
    }
}

internal b32
fui_handle_slider(App *app, Buffer_ID buffer)
{
    FUI  f_value; 
    FUI *f = &f_value;
    f->app    = app;
    f->buffer = buffer;
   
    Fui_Slider *&store = fui_slider_store;
    b32 at_slider = fui__at_slider_p(f);
    
    if (at_slider)
    {
        Token_Iterator_Array *tk = &f->tk;
        
        Token *token = tk->ptr;
        i64 slider_begin = token->pos;
        
        b32 parse_ok = false;
        i32 slider_index = 0;
        // NOTE(kv): Parsing and find slider
        if (require_token_kind(tk, TokenBaseKind_ParentheticalOpen) &&
            require_token_kind(tk, TokenBaseKind_Identifier       ))
        {
            Temp_Block temp(app);
            String8 name = push_token_lexeme(app, temp, buffer, tk->ptr);
            Fui_Slider *slider = fui_get_slider_by_name(name);
            if (slider)
            {
                slider_index = slider - store;
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
        
        if (parse_ok && slider_index)
        {
            i64 slider_end = tk->ptr->pos + 1;
            Fui_Slider_Value slider_value_save = store[slider_index].value;
            b32 write_back = false;
            
            fui_active_slider_index = slider_index;
            defer(fui_active_slider_index = 0);
            
            for (;;)
            { // note: ui loop
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
                    else if (Match(O)) direction.z = keydown ? +1 : 0;
                    else if (Match(I)) direction.z = keydown ? -1 : 0;
#undef Match
                }
                else leave_current_input_unhandled(app);
            }
            
            Fui_Slider *slider = &store[slider_index];
            if (write_back)
            { //note(kv): save the results back
                Scratch_Block temp(app);
                String8 value_string = fui_push_slider_value(temp, slider->type, slider->value);
                String8 update_string = push_stringf(temp, "fslider( %.*s, %.*s )", string_expand(slider->name), string_expand(value_string));
                buffer_replace_range(app, buffer, slider_begin, slider_end, update_string);
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
fui_tick(App *app, Frame_Info frame_info)
{
    if (fui_active_slider_index)
    {
        Fui_Slider *slider = &fui_slider_store[fui_active_slider_index];
        
        if ( fui_slider_direction != v4{} )
        {
            f32 speed = 1.0f;
            f32 dt = frame_info.animation_dt;  // NOTE(kv): using actual literal_dt would trigger a big jump when the user initially presses a key
            v4 *value = &slider->value.v4;
            *value += dt * speed * fui_slider_direction;
            animate_in_n_milliseconds(app, 0);
         
            Temp_Block temp(app);
            String8 slider_value = fui_push_slider_value(temp, slider->type, slider->value);
            vim_set_bottom_text(slider_value);
        }
    }
}
