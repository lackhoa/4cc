internal String
get_identifier_at_cursor(Arena *arena, App *app, Buffer_ID buffer)
{
 Token_Iterator_Array tk_value = get_token_it_at_cursor(app);
 auto *tk = &tk_value;
 if ( tk->tokens )
 {
  Token *token = tk->ptr;
  return push_token_lexeme(app, arena, buffer, token);
 }
 else { return {}; }
}

internal void
fui_tick(App *app, Frame_Info frame_info)
{
 Game_API *game = get_game_code();
 if (game)
 {
  if ( game->fui_is_active() )
  {
   animate_next_frame(app);
   Scratch_Block scratch(app);
   {// NOTE: Printing
    String value_string = game->fui_push_active_slider_value(scratch);
    vim_set_bottom_text(value_string);
   }
  }
 }
}

internal void
fui_draw_slider(App *app, Buffer_ID buffer, rect2 region)
{
 Game_API *game = get_game_code();
 if ( game )
 {
  if ( game->fui_at_slider_p(app, buffer, 0) )
  {
   v2 slider_radius = v2{50,50};
   v2 slider_dim    = 2 * slider_radius;
   v2 slider_origin = region.max - slider_radius;
   {// NOTE: the whole slider outline
    rect2 rect = rect2_center_dim(slider_origin, slider_dim);
    v4 color = v4{1,1,1,0.5f};
    if ( game->fui_is_active() )  
     color.a = 1.0f;
    draw_rect_outline2(app, rect, 2, argb_pack(color));
   }
  }
 }
}

internal void update_game_key_states(Input_Event *event);


//~