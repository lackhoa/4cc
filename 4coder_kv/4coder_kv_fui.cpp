struct FUI
{
  Application_Links *app;
  Buffer_ID buffer;
  Token_Iterator_Array it;
};

global v3  fui_slider_value;
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

  v2 slider_dim = v2{100,100};
  v2 slider_origin = region.max - slider_dim;
  { //note: the whole slider outline
    rect2 rect = rect_min_dim(slider_origin, slider_dim);
    v4 color = fui_slider_active ? v4{1,1,1,1} : v4{0.2f,0.2f,0.2f,1};
    draw_rect_outline(app, rect, 2, color);
  }
  { //note: the dot
    auto &v = fui_slider_value;
    v2 center = slider_origin + hadamard(fui_slider_value.xy, slider_dim);
    rect2 rect = rect_center_dim(center, v2{10,10});
    draw_rect(app, rect, v4{0,0.5f,0,1});
  }
}

internal b32
fui_handle_slider(Application_Links *app, Buffer_ID buffer)
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
  
  Scratch_Block scratch(app);
  breakable_block
  {
    if ( !require_token_kind(it, TokenBaseKind_ParentheticalOpen) ) break;
    if ( !require_token_kind(it, TokenBaseKind_LiteralFloat) ) break;
    
    String8 string = push_token_lexeme(app, scratch, buffer, token);
    {
      char *cstring = to_c_string(scratch, string);
      f32 value = gb_str_to_f32(cstring, 0);
      printf_message(app, scratch, "value f32: %f\n", value);
    }
    
    if ( !require_token_kind(it, TokenBaseKind_ParentheticalClose ) ) break;
    i64 slider_end = it->ptr->pos + 1;
    
    string = push_buffer_range(app, scratch, buffer, Ii64(slider_begin, slider_end));
    printf_message(app, scratch, "buffer range: %.*s\n", string_expand(string));
  
    for (;;)
    { // note: ui loop
      fui_slider_active = true;
      defer(fui_slider_active = false);
      
      User_Input in = get_next_input(app, EventPropertyGroup_Any, EventProperty_Escape);
      if (in.abort) break; 
     
      b32 keydown = (in.event.kind == InputEventKind_KeyStroke);
      b32 keyup   = (in.event.kind == InputEventKind_KeyRelease);
      if (keydown || keyup)
      {
        v3 &direction = fui_slider_direction;
#define Match(CODE)  in.event.key.code == KeyCode_##CODE
        if      (Match(L)) direction.x = keydown ? +1 : 0;
        else if (Match(H)) direction.x = keydown ? -1 : 0;
        else if (Match(K)) direction.y = keydown ? +1 : 0;
        else if (Match(J)) direction.y = keydown ? -1 : 0;
        else if (Match(I)) direction.z = keydown ? +1 : 0;
        else if (Match(O)) direction.z = keydown ? -1 : 0;
#undef Match
      }
      else leave_current_input_unhandled(app);
    }
  }
  
  return true;
}

internal void 
fui_tick(Application_Links *app, Frame_Info frame_info)
{
  DEBUG_clear;
  f32 speed = 0.5f;
  f32 dt = frame_info.animation_dt;  // using actual literal_dt would trigger a big jump when the user initially presses a key
  fui_slider_value += dt * speed * fui_slider_direction;
  DEBUG_text(fui_slider_value, fui_slider_value);
  DEBUG_text(fui_slider_direction, fui_slider_direction);
  if( fui_slider_direction != v3{0,0,0} )
  {
    animate_in_n_milliseconds(app, 0);
  }
}
