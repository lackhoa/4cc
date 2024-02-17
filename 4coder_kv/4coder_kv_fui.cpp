struct FUI
{
    App *app;
    Buffer_ID buffer;
    Token_Iterator_Array tk;
};

internal String8
fui_push_slider_value(Arena *arena, Fui_Type type, Fui_Value value)
{
    String8 result = {};
    switch (type)
    {
        case Fui_Type_v1:
        {
            result = push_stringf(arena, "%f", value.v1);
        }break;
        case Fui_Type_v2:
        {
            v2 v = value.v2;
            result = push_stringf(arena, "v2{ %f, %f }", v.x, v.y);
        }break;
        case Fui_Type_v3:
        {
            v3 v = value.v3;
            result = push_stringf(arena, "v3{ %f, %f, %f }", v.x, v.y, v.z);
        }break;
        case Fui_Type_v4:
        {
            v4 v = value.v4;
            result = push_stringf(arena, "v4{ %f, %f, %f, %f }", v.x, v.y, v.z, v.w);
        }break;
        default: todo_incomplete;
    }
    return result;
}

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
    b32 result = false;
    FUI  f_value; 
    FUI *f = &f_value;
    f->app    = app;
    f->buffer = buffer;
    
    Fui_Slider *&store = fui_slider_store;
    b32 at_slider = fui__at_slider_p(f);
    
    if (at_slider)
    {
        X_Block xblock(app);
       
        Range_i64 slider_value_range = {};
        String8 slider_name;
        b32 parser_ok = false;
        {// NOTE(kv): Parsing
            Quick_Parser parser_value = qp_new(app, buffer, &f->tk);
            Quick_Parser *parser = &parser_value;
            qp_eat_token_kind(parser, TokenBaseKind_ParentheticalOpen);
            qp_eat_token_kind(parser, TokenBaseKind_Identifier);
            // NOTE: Slider name
            slider_name = qp_push_token(parser, xblock);
            // NOTE: Find the slider value range (so we can write back later)
            qp_eat_comma(parser);
            // NOTE: at value
            slider_value_range.min = get_next_token_pos(parser);
            u32 eat_index = qp_eat_until_lit(parser, ",)");
            slider_value_range.max = qp_get_pos(parser);
            if (eat_index == 1)
            {
                qp_eat_until_lit(parser, ")");
            }
            parser_ok = parser->ok;
        }
        
        // NOTE: find slider
        i32 slider_index = 0;
        if (parser_ok)
        {
            Fui_Slider *slider = fui_get_slider_by_name(slider_name);
            if (slider)
            {
                slider_index = slider - store;
            }
        }
        
        if (slider_index)
        {
            Fui_Value slider_value_save = store[slider_index].value;
            b32 write_back = false;
            
            fui_active_slider_index = slider_index;
            defer(fui_active_slider_index = 0);
            
            Fui_Slider *slider = &store[slider_index];
            
            block_zero_array(global_fui_key_states);
           
            for (;;)
            {// NOTE: ui loop
                User_Input in = get_next_input(app, EventPropertyGroup_Any, EventProperty_Escape);
                if (in.abort) break; 
                
                b32 keydown = (in.event.kind == InputEventKind_KeyStroke);
                b32 keyup   = (in.event.kind == InputEventKind_KeyRelease);
                if (keydown || keyup)
                {
                    Key_Code keycode = in.event.key.code;
#define Match(CODE) keycode == KeyCode_##CODE
                    //
                    if ( Match(Return) && keydown )
                    {
                        write_back = true; 
                        break;
                    }
                    else
                    {
                        global_fui_key_states[keycode] = keydown;
                    }
                    //
#undef Match
                }
                else leave_current_input_unhandled(app);
            }
            
            if (write_back)
            { // NOTE(kv): save the results back
                String8 value_string = fui_push_slider_value(xblock, slider->type, slider->value);
                buffer_replace_range(app, buffer, slider_value_range, value_string);
            }
            else 
                slider->value = slider_value_save;
            
            result = true;
        }
    }
    
    return result;
}

internal void
fui_tick(App *app, Frame_Info frame_info)
{
    if (fui_active_slider_index)
    {
        animate_next_frame(app);
        
        f32 dt = frame_info.animation_dt;  // NOTE(kv): using actual literal_dt would trigger a big jump when the user initially presses a key.
        Fui_Slider *slider = &fui_slider_store[fui_active_slider_index];
        
        v4 new_value = slider->update(slider, dt);
        if (new_value != slider->value.v4)
        {
            slider->value.v4 = new_value;
            X_Block x(app);
            String8 slider_value = fui_push_slider_value(x, slider->type, slider->value);
            vim_set_bottom_text(slider_value);  // todo Allow customizing this too?
        }
    }
}
