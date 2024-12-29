function void
fui_tick(App *app, Frame_Info frame_info)
{
 Game_API *game = get_game_code(Game_On);
 if(game)
 {
  if(game->fui_is_active())
  {
   animate_next_frame(app);
   Scratch_Block scratch;
   String value_string = game->fui_push_active_slider_value(scratch);
   vim_set_bottom_text(value_string);
  }
 }
}
function void
fui_draw_slider(App *app, rect2 region)
{
 Game_API *game = get_game_code(Game_On);
 if(game){
  if(game->fui_at_slider_p(app)){
   v2 slider_radius = v2{50,50};
   v2 slider_dim    = 2 * slider_radius;
   v2 slider_origin = region.max - slider_radius;
   {// NOTE: the whole slider outline
    rect2 rect = rect2_center_dim(slider_origin, slider_dim);
    v4 color = v4{1,1,1,0.5f};
    if ( game->fui_is_active() ) {
     color.a = 1.0f;
    }
    draw_rect_outline2(app, rect, 2, argb_pack(color));
   }
  }
 }
}
//~