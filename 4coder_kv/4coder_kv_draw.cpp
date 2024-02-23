function b32
kv_find_nest_side_paren(Application_Links *app, Token_Array *tokens, i64 pos,
                        Scan_Direction scan, Nest_Delimiter_Kind delim,
                        i64 *out)
{
  if (!(tokens && tokens->count)) return false;

  Range_i64 range = {};
  b32 nest_found = false;
  {// b32 result = find_nest_side(app, buffer, pos, flags, scan, delim, &range);
    Token_Iterator_Array it = token_iterator_pos(0, tokens, pos);
    i32 level = 0;
    for (;;)
    {
      Token *token = token_it_read(&it);
      Nest_Delimiter_Kind token_delim_kind = NestDelim_None;
      //
      if (token->kind == TokenBaseKind_ParentheticalOpen)
      {
        token_delim_kind = NestDelim_Open;
      }
      else if (token->kind == TokenBaseKind_ParentheticalClose)
      {
        token_delim_kind = NestDelim_Close;
      }
            
      if (level == 0 && token_delim_kind == delim)
      {
        range = Ii64_size(token->pos, token->size);
        nest_found = true;
        break;
      }
      else
      {
        if (token_delim_kind != NestDelim_None)
        {
          if (token_delim_kind == delim) { level--; }
          else                           { level++; }
        }
            
        if (scan == Scan_Forward)
        {
          if (!token_it_inc(&it))
          {
            break;
          }
        }
        else
        {
          if (!token_it_dec(&it))
          {
            break;
          }
        }
      }
    }
  }
  if (nest_found)
  {
    if (delim == NestDelim_Close) { *out = range.end;   }
    else                          { *out = range.start; }
  }
  return(nest_found);
}

// NOTE(kv): Patch because the original one doesn't terminate early (and buggy!)
function void
kv_draw_paren_highlight(Application_Links *app, Buffer_ID buffer, Text_Layout_ID text_layout_id,
                        i64 pos, ARGB_Color *colors, i32 color_count)
{
  if (!(colors && color_count)) return;

  Token_Array tokens = get_token_array_from_buffer(app, buffer);
  if (!(tokens.tokens && tokens.count)) return;

  {// Nudge the cursor in case we're near parentheses.
    Token_Iterator_Array it = token_iterator_pos(0, &tokens, pos);
    Token *token = token_it_read(&it);
    if (token && (token->kind == TokenBaseKind_ParentheticalOpen))
    {
      pos = token->pos + token->size;
    }
    else if ( token_it_dec_all(&it) )
    {
      token = token_it_read(&it);
      if (token &&
          token->kind == TokenBaseKind_ParentheticalClose &&
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
      i32 max = 16;
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
    i32 color_index = 0;
    for (i32 range_i = ranges.count-1; range_i >= 0; range_i--)
    {
      Range_i64 range = ranges.ranges[range_i];
      i32 fore_index = color_index % color_count;
      paint_text_color_pos(app, text_layout_id, range.min, colors[fore_index]);
      paint_text_color_pos(app, text_layout_id, range.max - 1, colors[fore_index]);
      color_index += 1;
    }
  }
}

function Render_Caller_Function kv_render_caller;
function void
kv_render_caller(FApp *app, Frame_Info frame_info, View_ID view)
{
    ProfileScope(app, "render caller");
    View_ID active_view = get_active_view(app, Access_Always);
    b32 is_active_view = (active_view == view);
    
    // todo(kv): maybe this thing sets the clip to the active panel? so it messes with vim command lister?
    // Rect_f32 region = draw_background_and_margin(app, view, is_active_view);
    
    Rect_f32 region = view_get_screen_rect(app, view);
    Rect_f32 prev_clip = draw_set_clip(app, region);
    
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    Face_ID face_id = get_face_id(app, buffer);
    Face_Metrics face_metrics = get_face_metrics(app, face_id);
    f32 line_height = face_metrics.line_height;
    f32 digit_advance = face_metrics.decimal_digit_advance;
    
    Rect_f32 global_rect = global_get_screen_rectangle(app);
    {
        f32 filebar_y = global_rect.y1 - 2.f*line_height - vim_cur_filebar_offset;
        if(region.y1 >= filebar_y){ region.y1 = filebar_y; }
    }
    
    // clear
    draw_rectangle_fcolor(app, region, 0.f, fcolor_id(defcolor_back));
    
    region = vim_draw_query_bars(app, region, view, face_id);
   
    if ( rect2_dim(region).y > 200 )
    {// Draw file bar
        Rect_f32_Pair pair = layout_file_bar_on_bot(region, line_height);
        vim_draw_filebar(app, view, buffer, frame_info, face_id, pair.b);
        region = pair.a;
    }
    
    {// Draw borders
        if(region.x0 > global_rect.x0){
            Rect_f32_Pair border_pair = rect_split_left_right(region, 2.f);
            draw_rectangle_fcolor(app, border_pair.a, 0.f, fcolor_id(defcolor_margin));
            region = border_pair.b;
        }
        if(region.x1 < global_rect.x1){
            Rect_f32_Pair border_pair = rect_split_left_right_neg(region, 2.f);
            draw_rectangle_fcolor(app, border_pair.b, 0.f, fcolor_id(defcolor_margin));
            region = border_pair.a;
        }
        region.y0 += 3.f;
    }
    
	if(show_fps_hud)
    {
        Rect_f32_Pair pair = layout_fps_hud_on_bottom(region, line_height);
        draw_fps_hud(app, frame_info, face_id, pair.max);
        region = pair.min;
        animate_in_n_milliseconds(app, 1000);
	}
    
    // NOTE(allen): layout line numbers
	b32 show_line_number_margins = def_get_config_b32(vars_intern_lit("show_line_number_margins"));
    Rect_f32 line_number_rect = {};
    if (show_line_number_margins)
    {
        Rect_f32_Pair pair = layout_line_number_margin(app, buffer, region, digit_advance);
        line_number_rect = pair.min;
        region = pair.max;
    }
    
	Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
	Buffer_Point_Delta_Result delta = delta_apply(app, view, frame_info.animation_dt, scroll);
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
	Text_Layout_ID text_layout_id = text_layout_create(app, buffer, region, buffer_point);
    
    if(show_line_number_margins)
        vim_draw_line_number_margin(app, view, buffer, face_id, text_layout_id, line_number_rect);
    // else
    //   draw_rectangle_fcolor(app, line_number_rect, 0.f, fcolor_id(defcolor_back));
   
    { // NOTE(kv): kv_render_buffer(app, frame_info, view, face_id, buffer, text_layout_id, region);
        // NOTE(kv): originally from "byp_render_buffer"
        ProfileScope(app, "render buffer");
        Rect_f32 prev_clip2 = draw_set_clip(app, region);  // todo I don't even think this is necessary?
        defer(draw_set_clip(app, prev_clip2););
        
        {// NOTE(kv): draw test render buffer
            // TODO @Cleanup Why not just check the view buffer name?
            Buffer_ID game_buffer = get_buffer_by_name(app, GAME_BUFFER_NAME, Access_Always);
            if (game_buffer == buffer)
            {
                game_update_and_render(app, view, frame_info.animation_dt, region);
                return;
            }
        }
        
        Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
        
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
        if(highlight_line_at_cursor && is_active_view){
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
            {
                draw_whitespace_highlight(app, buffer, text_layout_id, cursor_roundness);
            }
            else
            {
                draw_whitespace_highlight(app, text_layout_id, &token_array, cursor_roundness);
            }
        }
        
        if(is_active_view && vim_state.mode == VIM_Visual)
        {
            vim_draw_visual_mode(app, view, buffer, face_id, text_layout_id);
        }
        
        fold_draw(app, buffer, text_layout_id);
        
        vim_draw_search_highlight(app, view, buffer, text_layout_id, cursor_roundness);
        
        vim_draw_cursor(app, view, is_active_view, buffer, text_layout_id, cursor_roundness, mark_thickness);
        
        paint_fade_ranges(app, text_layout_id, buffer);
        
        draw_text_layout_default(app, text_layout_id);  // NOTE: this highlights the @Notes
        
        vim_draw_after_text(app, view, is_active_view, buffer, text_layout_id, cursor_roundness, mark_thickness);
        
        Buffer_ID calc_buffer_id = get_buffer_by_name(app, SCu8("*calc*"), AccessFlag_Read);
        if(calc_buffer_id == buffer)
        { // NOTE(rjf): Interpret the calc buffer as calc code.
            F4_CLC_RenderBuffer(app, buffer, view, text_layout_id);
        }
        else
        { // NOTE(rjf): Draw calc comments.
            F4_CLC_RenderComments(app, buffer, view, text_layout_id);
        }
        
        // NOTE(kv): fui
        fui_draw_slider(app, buffer, region);
    }
    
#if KV_INTERNAL
    if (arrlen(DEBUG_entries) > 0 &&
        DEBUG_draw_hud_p)
    {// my test hud
        Rect_f32_Pair pair = rect_split_top_bottom_neg(region, 5*line_height);
		DEBUG_draw_hud(app, face_id, text_layout_id, pair.max);
		region = pair.min;
    }
#endif
    
	text_layout_free(app, text_layout_id);
    draw_set_clip(app, prev_clip);
}
