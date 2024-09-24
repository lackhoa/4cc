global String8 strong_divider_comment_signifier = SCu8("//~");
global String8 weak_divider_comment_signifier   = SCu8("//-");

// NOTE(kv): Patch because the original one doesn't terminate early (and buggy!)
function void
kv_draw_paren_highlight(App *app, Buffer_ID buffer, Text_Layout_ID text_layout_id,
                        i64 pos, ARGB_Color *colors, i1 color_count)
{
  if (!(colors && color_count)) return;

  Token_Array tokens = get_token_array_from_buffer(app, buffer);
  if (!(tokens.tokens && tokens.count)) return;

  {// Nudge the cursor in case we're near parentheses.
    Token_Iterator_Array it = tkarr_at_pos(0, &tokens, pos);
    Token *token = tkarr_read(&it);
    if (token && (token->kind == TokenBaseKind_ParenOpen))
    {
      pos = token->pos + token->size;
    }
    else if ( tkarr_dec_all(&it) )
    {
      token = tkarr_read(&it);
      if (token &&
          token->kind == TokenBaseKind_ParenClose &&
          pos == token->pos + token->size)
      {
        pos = token->pos;
      }
    }
  }

  {// draw_enclosures(app, buffer, pos);
    Scratch_Block scratch(app);
    Range_i64_Array ranges = {};

    {// get_enclosure_ranges(app, scratch, buffer, pos);
      i1 max = 16;
      ranges.ranges = push_array(scratch, Range_i64, max);
      while ((ranges.count < max) && (pos >= 1))
      {
        // NOTE(kv): this algorithm is weird and inefficient: just keep two
        // cursors and scan for parentheses, then we'd be done!
        Range_i64 range = {};
        // find_surrounding_nest(app, buffer, pos, FindNest_Paren, &range)
        // NOTE(kv): "pos" has to be positive for this to work, Allen!
        b32 find_nest_backward = kv_find_nest_side_paren(app, &tokens, pos-1,
                                                         Scan_Backward, NestDelim_Open, &range.start);
        b32 find_nest_forward = kv_find_nest_side_paren(app, &tokens, pos,
                                                        Scan_Forward, NestDelim_Close, &range.end);
        if (find_nest_backward && find_nest_forward)
        {
          ranges.ranges[ranges.count] = range;
          ranges.count += 1;
          pos = range.first;
        }
        else
        {
          break;
        }
      }
    }

    // Draw those parens!
    i1 color_index = 0;
    for (i1 range_i = ranges.count-1; range_i >= 0; range_i--)
    {
      Range_i64 range = ranges.ranges[range_i];
      i1 fore_index = color_index % color_count;
   paint_text_color_pos(app, text_layout_id, range.min, colors[fore_index]);
   paint_text_color_pos(app, text_layout_id, range.max - 1, colors[fore_index]);
   color_index += 1;
  }
 }
}

function void
F4_RenderDividerComments(App *app, Buffer_ID buffer, View_ID view,
                         Text_Layout_ID text_layout_id)
{
 if(!def_get_config_b32(vars_intern_lit("f4_disable_divider_comments")))
 {
  ProfileScope(app, "[F4] Divider Comments");
  
  Token_Array token_array = get_token_array_from_buffer(app, buffer);
  Range_i64 visible_range = text_layout_get_visible_range_(app, text_layout_id);
  Scratch_Block scratch(app);
  
  if(token_array.tokens != 0)
  {
   i64 first_index = token_index_from_pos(&token_array, visible_range.first);
   Token_Iterator_Array it = token_iterator_index(0, &token_array, first_index);
   
   Token *token = 0;
   for(;;)
   {
    token = tkarr_read(&it);
    
    if(token->pos >= visible_range.opl || !token || !tkarr_inc_non_whitespace(&it))
    {
     break;
    }
    
    if(token->kind == TokenBaseKind_Comment)
    {
     Rect_f32 comment_first_char_rect = text_layout_character_on_screen(app, text_layout_id, token->pos);
     Rect_f32 comment_last_char_rect = text_layout_character_on_screen(app, text_layout_id, token->pos+token->size-1);
     String token_string = push_buffer_range(app, scratch, buffer, Ii64(token));
     String signifier_substring = string_substring(token_string, Ii64(0,clamp_max(3,token_string.size)));
     f32 roundness = 4.f;
     
     // NOTE(rjf): Strong dividers.
     if(string_match(signifier_substring, strong_divider_comment_signifier))
     {
      Rect_f32 rect =
      {
       comment_first_char_rect.x0,
       comment_first_char_rect.y0-2,
       10000,
       comment_first_char_rect.y0,
      };
      draw_rect(app, rect, roundness, fcolor_resolve(fcolor_id(defcolor_comment)), 0);
     }
     
     // NOTE(rjf): Weak dividers.
     else if(string_match(signifier_substring, weak_divider_comment_signifier))
     {
      f32 dash_size = 8;
      Rect_f32 rect =
      {
       comment_last_char_rect.x1,
       (comment_last_char_rect.y0 + comment_last_char_rect.y1)/2 - 1,
       comment_last_char_rect.x1 + dash_size,
       (comment_last_char_rect.y0 + comment_last_char_rect.y1)/2 + 1,
      };
      
      for(int i = 0; i < 1000; i += 1)
      {
       draw_rect(app, rect, roundness, fcolor_resolve(fcolor_id(defcolor_comment)), 0);
       rect.x0 += dash_size*1.5f;
       rect.x1 += dash_size*1.5f;
      }
     }
    }
   }
  }
 }
}

function Render_Caller_Function kv_render_caller;
//
function void
kv_render_caller(App *app, Frame_Info frame, View_ID view)
{
 ProfileScope(app, "render caller");
 b32 view_active = view_is_active(app, view);
 i1 monitor = hax_guess_which_monitor_the_view_is_in(app, view);
 
 rect2 clip      = view_get_screen_rect(app, view);
 rect2 prev_clip = draw_set_clip(app, clip);
 
 Buffer_ID buffer = view_get_buffer(app, view, 0);
 Face_ID face_id = get_face_id(app, buffer);
 Face_Metrics face_metrics = get_face_metrics(app, face_id);
 v1 line_height = face_metrics.line_height;
 v1 digit_advance = face_metrics.decimal_digit_advance;
 
 if (vim_lister_running)
 {// NOTE(kv) Watch out for the vim bottom lister!
  clip.y1 -= clamp_min(vim_cur_lister_offset,0);
 }
 
 //NOTE(kv) If the bottom view is expanded,
 //  the bottom text wouldn't eat into our buffer.
 if (!global_bottom_view_expanded){
  //NOTE @vim_bottom_text_height
  clip.y1 -= 2.f*line_height;
 }
 
 {// NOTE(kv) Clear
  draw_rect_fcolor(app, clip, 0.f, fcolor_id(defcolor_back));
 }
 
 clip = vim_draw_query_bars(app, clip, view, face_id);
 
 if ( view != global_bottom_view )
 {// Draw file bar
  rect2_Pair pair = layout_file_bar_on_bot(clip, line_height);
  vim_draw_filebar(app, view, buffer, frame, face_id, pair.b);
  clip = pair.a;
 }
 
 if (view_active)
 {// Draw borders
  rect2 global_rect = global_get_screen_rectangle(app);
  FColor border_color = fcolor_id(defcolor_margin);
  if(clip.x0 > global_rect.x0) {
   rect2_Pair border_pair = rect_split_left_right(clip, 2.f);
   draw_rect_fcolor(app, border_pair.a, 0.f, border_color);
   clip = border_pair.b;
  }
  if(clip.x1 < global_rect.x1) {
   rect2_Pair border_pair = rect_split_left_right_neg(clip, 2.f);
   draw_rect_fcolor(app, border_pair.b, 0.f, border_color);
   clip = border_pair.a;
  }
  clip.y0 += 3.f;
 }
 
 if (show_fps_hud && view_active)
 {
  rect2_Pair pair = layout_fps_hud_on_bottom(clip, line_height);
  draw_fps_hud(app, frame, face_id, pair.max);
  clip = pair.min;
  animate_in_n_milliseconds(app, 1000);
 }
 
 Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
 Buffer_Point_Delta_Result delta = delta_apply(app, view, frame.animation_dt, scroll);
 if(!block_match_struct(&scroll.position, &delta.point))
 {
  block_copy_struct(&scroll.position, &delta.point);
  view_set_buffer_scroll(app, view, scroll, SetBufferScroll_NoCursorChange);
 }
 if(delta.still_animating)
 {
  animate_in_n_milliseconds(app, 0);
 }
 Buffer_Point buffer_point = scroll.position;
 Text_Layout_ID text_layout_id = text_layout_create(app, buffer, clip, buffer_point);
 
 {// NOTE(kv): kv_render_buffer(app, frame, view, face_id, buffer, text_layout_id, clip);
  // NOTE(kv): originally from "byp_render_buffer"
  ProfileScope(app, "render buffer");
  rect2 prev_clip2 = draw_set_clip(app, clip);
  defer( draw_set_clip(app, prev_clip2); );
  
  Scratch_Block scratch(app);
  if (i1 viewport = buffer_viewport_id(app, buffer)){
   Render_Target *target = get_view_render_target(app, view);
   render_game(app, target, viewport, frame, clip);
  }else{
   Range_i64 visible_range = text_layout_get_visible_range_(app, text_layout_id);
   
   u64 cursor_roundness_100 = def_get_config_u64(app, vars_intern_lit("cursor_roundness"));
   f32 cursor_roundness = face_metrics.normal_advance*(f32)cursor_roundness_100*0.01f;
   f32 mark_thickness = (f32)def_get_config_u64(app, vars_intern_lit("mark_thickness"));
   
   i64 cursor_pos = view_correct_cursor(app, view);
   view_correct_mark(app, view);
   
   b32 use_scope_highlight = def_get_config_b32(vars_intern_lit("use_scope_highlight"));
   if(use_scope_highlight)
   {
    Color_Array colors = finalize_color_array(defcolor_back_cycle);
    draw_scope_highlight(app, buffer, text_layout_id, cursor_pos, colors.vals, colors.count);
   }
   
   b32 highlight_line_at_cursor = def_get_config_b32(vars_intern_lit("highlight_line_at_cursor"));
   if(highlight_line_at_cursor && view_active){
    i64 line_number = get_line_number_from_pos(app, buffer, cursor_pos);
    draw_line_highlight(app, text_layout_id, line_number, fcolor_id(defcolor_highlight_cursor_line));
   }
   
   Token_Array token_array = get_token_array_from_buffer(app, buffer);
   if(token_array.tokens)
   {
    byp_draw_token_colors(app, view, buffer, text_layout_id);
   }
   else
   {
    paint_text_color_fcolor(app, text_layout_id, visible_range, fcolor_id(defcolor_text_default));
   }
   
   {// Error, jump (search) highlightss
    Buffer_ID comp_buffer = get_buffer_by_name(app, compilation_buffer_name, Access_Always);
    draw_jump_highlights(app, buffer, text_layout_id, comp_buffer, fcolor_id(defcolor_highlight_junk));
    // TODO(BYP): Draw error messsage annotations
    Buffer_ID jump_buffer = get_locked_jump_buffer(app);
    if (jump_buffer != comp_buffer)
    {
     draw_jump_highlights(app, buffer, text_layout_id, jump_buffer, fcolor_id(defcolor_highlight_white));
    }
   }
   
   { // draw paren highlight
    b32 is_skm = false;
    {
     F4_Language *language = F4_LanguageFromBuffer(app, buffer);
     F4_Language *skm_lang = F4_LanguageFromString(SCu8("skm"));
     is_skm = (language == skm_lang);
    }
    Color_Array colors = finalize_color_array(defcolor_text_cycle);
    if (is_skm)
     kv_draw_paren_highlight(app, buffer, text_layout_id, cursor_pos, colors.vals, colors.count);
    else
     draw_paren_highlight(app, buffer, text_layout_id, cursor_pos, colors.vals, colors.count);
   }
   
   b64 show_whitespace = false;
   view_get_setting(app, view, ViewSetting_ShowWhitespace, &show_whitespace);
   if(show_whitespace)
   {
    if(token_array.tokens == 0)
     draw_whitespace_highlight(app, buffer, text_layout_id, cursor_roundness);
    else
     draw_whitespace_highlight(app, text_layout_id, &token_array, cursor_roundness);
   }
   
   if(view_active && vim_state.mode == VIM_Visual)
   {
    vim_draw_visual_mode(app, view, buffer, face_id, text_layout_id);
   }
   
   fold_draw(app, buffer, text_layout_id);
   
   vim_draw_search_highlight(app, view, buffer, text_layout_id, cursor_roundness);
   
   vim_draw_cursor(app, view, view_active, buffer, text_layout_id, cursor_roundness, mark_thickness);
   
   paint_fade_ranges(app, text_layout_id, buffer);
   
   draw_text_layout_default(app, text_layout_id);  // NOTE: this highlights the @Notes
   
   // NOTE(kv): fui
   fui_draw_slider(app, buffer, clip);
   
   F4_RenderDividerComments(app, buffer, view, text_layout_id);
  }
 }
 
 text_layout_free(app, text_layout_id);
 draw_set_clip(app, prev_clip);
}
