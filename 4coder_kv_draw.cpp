//-
enum
{
 byp_TokenKind_Primitive = 16,
 byp_TokenKind_ControlFlow = 17,
 byp_TokenKind_Struct = 18,
};

global String8 strong_divider_comment_signifier = strlit("//~");
global String8 weak_divider_comment_signifier   = strlit("//-");

// NOTE(kv): Patch because the original one doesn't terminate early (and buggy!)
function void
kv_draw_paren_highlight(App *app, Buffer_ID buffer, Text_Layout_ID text_layout_id,
                        i64 pos, ARGB_Color *colors, i32 color_count)
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

function void
F4_RenderDividerComments(App *app, Buffer_ID buffer, View_ID view,
                         Text_Layout_ID text_layout_id)
{
 if(!def_get_config_b32(vars_intern_lit("f4_disable_divider_comments")))
 {
  ProfileBlock( "[F4] Divider Comments");
  
  Token_Array token_array = get_token_array_from_buffer(app, buffer);
  Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
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
     if(string_match(signifier_substring, strong_divider_comment_signifier) or
        string_match(signifier_substring, strlit("_")))
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
// NOTE see custom_command_list.h
global Managed_ID defcolor_function; //colors
global Managed_ID defcolor_type; //colors
global Managed_ID defcolor_primitive; //colors
global Managed_ID defcolor_macro; //colors
global Managed_ID defcolor_control; //colors
global Managed_ID defcolor_struct; //colors
global Managed_ID defcolor_non_text; //colors
global Managed_ID defcolor_broken_link; //colors

function b32
byp_highlight_token(Token_Base_Kind kind)
{
	switch(kind)
 {
		case TokenBaseKind_Keyword:
  case TokenBaseKind_Identifier:
		case byp_TokenKind_Primitive:
		case byp_TokenKind_ControlFlow:
		case byp_TokenKind_Struct:
		return true;
	}
	return false;
}
function ARGB_Color
get_identifier_color_default_to_zero(String id)
{
 ARGB_Color resolved_color = 0;
 Managed_ID color = 0;
 F4_Index_Note *f4_note = F4_Index_LookupNote(id);
 if(f4_note)
 {
  switch(f4_note->kind)
  {
   case F4_Index_NoteKind_Decl:     color = defcolor_int_constant; break;
   case F4_Index_NoteKind_Constant: color = defcolor_int_constant; break;
   case F4_Index_NoteKind_Type:     color = defcolor_type;     break;
   case F4_Index_NoteKind_Function: color = defcolor_function; break;
   case F4_Index_NoteKind_Macro:    color = defcolor_macro;    break;
   
   case F4_Index_NoteKind_CommentIdentifier:
   color = defcolor_int_constant; break;
  }
 }
 
 if(resolved_color == 0)
 {
  resolved_color = fcolor_resolve(fcolor_id(color));
 }
 return resolved_color;
}
function ARGB_Color
get_identifier_color(String id)
{
 ARGB_Color color = get_identifier_color_default_to_zero(id);
 if(color == 0)
 {
  color = fcolor_resolve(fcolor_id(defcolor_text_default));
 }
 return color;
}
function ARGB_Color
byp_get_token_color_cpp(App *app, Buffer_ID buffer, Token *token)
{//NOTE(kv) Not just cpp, but oh well...
 Scratch_Scope tmp;
	Managed_ID color = defcolor_text_default;
 ARGB_Color resolved_color = 0;
	switch (token->kind)
 {
		case TokenBaseKind_Preprocessor:{ color = defcolor_preproc; }break;
		case TokenBaseKind_Keyword:{ color = defcolor_keyword; }break;
		case TokenBaseKind_Comment:{ color = defcolor_comment; }break;
		case TokenBaseKind_LiteralString:{ color = defcolor_str_constant; }break;
		case TokenBaseKind_LiteralInteger:{ color = defcolor_int_constant; }break;
		case TokenBaseKind_LiteralFloat:{ color = defcolor_float_constant; }break;
  
		case TokenBaseKind_Operator:
		case TokenBaseKind_ScopeOpen:
		case TokenBaseKind_ScopeClose:
		case TokenBaseKind_ParenOpen:
		case TokenBaseKind_ParenClose:
		case TokenBaseKind_StatementClose:{ color = defcolor_non_text; } break;
  
		case byp_TokenKind_ControlFlow:{ color = defcolor_control; }break;
		case byp_TokenKind_Primitive:{ color = defcolor_primitive; }break;
		case byp_TokenKind_Struct:{ color = defcolor_struct; }break;
  
  case TokenBaseKind_Identifier:
  {
   String id = push_token_lexeme(app, tmp, buffer, token);
   resolved_color = get_identifier_color(id);
  }break;
	}
	// specifics override generals
	switch (token->sub_kind)
 {
		case TokenCppKind_LiteralTrue:
		case TokenCppKind_LiteralFalse:
		{ color = defcolor_bool_constant; }break;
  
		case TokenCppKind_LiteralCharacter:
		case TokenCppKind_LiteralCharacterWide:
		case TokenCppKind_LiteralCharacterUTF8:
		case TokenCppKind_LiteralCharacterUTF16:
		case TokenCppKind_LiteralCharacterUTF32:
		{ color = defcolor_char_constant; }break;
  
		case TokenCppKind_PPIncludeFile:
		{ color = defcolor_include; }break;
	}
 
 if(resolved_color == 0)
 {
  resolved_color = fcolor_resolve(fcolor_id(color));
 }
	return resolved_color;
}

function void
byp_draw_cpp_token_colors(App *app, Buffer_ID buffer, Text_Layout_ID text_layout_id, Token_Array *array)
{
	Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
	i64 first_index = token_index_from_pos(array, visible_range.first);
	Token_Iterator_Array it = token_iterator_index(0, array, first_index);
	for(;;)
 {
		Token *token = tkarr_read(&it);
		if(token->pos >= visible_range.opl){ break; }
		ARGB_Color argb = byp_get_token_color_cpp(app, buffer, token);
		paint_text_color(app, text_layout_id, Ii64_size(token->pos, token->size), argb);
		if(!tkarr_inc_all(&it)){ break; }
	}
}
function void 
byp_draw_token_colors(App *app, View_ID view, Buffer_ID buffer, Text_Layout_ID layout)
{
 Token_Array token_array = get_token_array_from_buffer(app, buffer);
 Range_i64 visible_range = text_layout_get_visible_range(app, layout);
 
	Scratch_Block tmp(app);
	byp_draw_cpp_token_colors(app, buffer, layout, &token_array);
 
 i64 cursor_index = token_index_from_pos(&token_array, view_get_cursor_pos(app, view));
 Token_Iterator_Array it = token_iterator_index(0, token_array.tokens, token_array.count, cursor_index);
 Token *cursor_token = tkarr_read(&it);
 b32 do_cursor_tok_highlight = byp_highlight_token(cursor_token->kind);
 
 String token_string = {};
 Rect_f32 cursor_tok_rect = {};
 v2 tok_rect_dim = {};
 
 v1 highlight_thick = 2.0f;
 if (do_cursor_tok_highlight) {
  token_string = push_token_lexeme(app, tmp, buffer, cursor_token);
  cursor_tok_rect = text_layout_character_on_screen(app, layout, cursor_token->pos);
  f32 tok_rect_dimx = f32(cursor_token->size) * rect_width(cursor_tok_rect);
  tok_rect_dim = V2(tok_rect_dimx, highlight_thick);
  cursor_tok_rect = Rf32_xy_wh(V2(cursor_tok_rect.x0, cursor_tok_rect.y1 - highlight_thick), tok_rect_dim);
	}
 
	ARGB_Color function_color = fcolor_resolve(fcolor_id(defcolor_function));
	ARGB_Color type_color     = fcolor_resolve(fcolor_id(defcolor_type));
	ARGB_Color macro_color    = fcolor_resolve(fcolor_id(defcolor_macro));
 ARGB_Color constant_color = fcolor_resolve(fcolor_id(defcolor_int_constant));
	ARGB_Color cursor_tok_color = byp_get_token_color_cpp(app, buffer, cursor_token);
 
 ARGB_Color comment_pop_0 = finalize_color(defcolor_comment_pop, 0);
 ARGB_Color comment_pop_1 = finalize_color(defcolor_comment_pop, 1);
 ARGB_Color comment_pop_2 = finalize_color(defcolor_comment_pop, 2);
 ARGB_Color broken_link_color = finalize_color(defcolor_broken_link, 2);
 
	{// NOTE(BYP): Highlight #Annotations
		i64 first_index = token_index_from_pos(&token_array, visible_range.first);
		Token_Iterator_Array comment_it = token_iterator_index(buffer, &token_array, first_index);
		for(;;)
  {
   Token *token = tkarr_read(&comment_it);
   if(token->pos >= visible_range.max){ break; }
   String tail = {};
   if(token_it_check_and_get_lexeme(app, tmp, &comment_it, TokenBaseKind_Comment, &tail))
   {
    foreach(i, token->size)
    {
     b32 is_ref = tail.str[i] == '@';
     b32 is_tag = tail.str[i] == '#';
     b32 is_id  = tail.str[i] == ';';
     if(is_ref or is_tag or is_id)
     {
      Range_i64 annot_range = Ii64(i);
      {
       i1 j=i+1;
       for(; j < token->size; j++)
       {
        if(not character_is_alnum(tail.str[j]))
        {
         break;
        }
       }
       annot_range.max = j;
      }
      if(annot_range.min < annot_range.max)
      {
       Range_i64 id_range = annot_range + token->pos;
       id_range.min += 1;
       
       ARGB_Color annot_color = comment_pop_0;
       if(is_ref)
       {
        String annot_string = push_buffer_range(app, tmp, buffer, id_range);
        annot_color = get_identifier_color_default_to_zero(annot_string);
        if(annot_color == 0){ annot_color = broken_link_color; }
       }
       else if(is_id)
       {
        annot_color = constant_color;
       }
       
       paint_text_color(app, layout, id_range, annot_color);
      }
     }
    }
   }
   if(!tkarr_inc_non_whitespace(&comment_it)){ break; }
  }
 }
 
 {// NOTE(allen): Scan for TODOs and NOTEs
  Comment_Highlight_Pair pairs[] = {
   {str8lit("note"), comment_pop_0},
   {str8lit("todo"), comment_pop_1},
   {str8lit("important"), comment_pop_2},
   {str8lit("no""no"),    comment_pop_2},
   {str8lit("bookmark"),  comment_pop_2}
  };
  draw_comment_highlights(app, buffer, layout, &token_array, ArrayAndCount(pairs));
 }
 
 it = tkarr_at_pos(0, &token_array, Max(0, visible_range.first-1));
 
 if(cursor_token->kind == TokenBaseKind_Identifier)
 {//-Matching identifiers highlight 
  for(;;)
  {
   Token *token = tkarr_read(&it);
   if(token->pos > visible_range.max) {
    break;
   }
   
   String lexeme = push_token_lexeme(app, tmp, buffer, token);
   
   if(string_match(lexeme, token_string))
   {
    Rect_f32 cur_tok_rect = text_layout_character_on_screen(app, layout, token->pos);
    cur_tok_rect = Rf32_xy_wh(V2(cur_tok_rect.x0, cur_tok_rect.y1 - highlight_thick), tok_rect_dim);
    draw_rect(app, cur_tok_rect, 5.f, cursor_tok_color, 0);
   }
   
   if (!tkarr_inc_non_whitespace(&it)) {
    break;
   }
  }
 }
 
 String buffer_path = push_buffer_filepath(app, tmp, buffer);
 b32 is_note_file = (path_extension(buffer_path) == "skm");
 if(is_note_file)
 {//-Hacked note highlighting
  for(;;)
  {
   Token *token = tkarr_read(&it);
   if(token->pos > visible_range.max) {
    break;
   }
   
   String id = push_token_lexeme(app, tmp, buffer, token);
   ARGB_Color color = get_identifier_color_default_to_zero(id);
   if(color)
   {
    paint_text_color(app, layout, Ii64_size(token->pos, token->size), color);
   }
   
   if (!tkarr_inc_non_whitespace(&it)) {
    break;
   }
  }
 }
 
 if (do_cursor_tok_highlight) { draw_rect(app, cursor_tok_rect, 5.f, cursor_tok_color, 0); }
}

function Render_Caller_Function kv_render_caller;
//
function void
kv_render_caller(App *app, Frame_Info frame, View_ID view)
{
 ProfileBlock( "render caller");
 b32 view_active = view_is_active(app, view);
 
 rect2 clip      = view_get_screen_rect(app, view);
 rect2 prev_clip = draw_set_clip(app, clip);
 
 Buffer_ID buffer = view_get_buffer(app, view, 0);
 i32 viewport = buffer_viewport_id(app, buffer);
 b32 is_game = viewport != 0;
 Face_ID face_id = get_face_id(app, buffer);
 Face_Metrics face_metrics = get_face_metrics(app, face_id);
 v1 line_height = face_metrics.line_height;
 
 if(vim_lister_running)
 {// NOTE(kv) Watch out for the vim bottom lister!
  clip.y1 -= clamp_min(vim_cur_lister_offset,0);
 }
 
 // NOTE(kv) If the bottom view is expanded,
 // the bottom text wouldn't eat into our buffer.
 if(!global_bottom_view_expanded)
 {//NOTE @vim_bottom_text_height
  clip.y1 -= 2.f*line_height;
 }
 
 if(not is_game)
 {// NOTE Clear
  draw_rect_fcolor(app, clip, 0.f, fcolor_id(defcolor_back));
 }
 
 clip = vim_draw_query_bars(app, clip, view, face_id);
 
 if( view != global_bottom_view )
 {// NOTE Draw file bar
  rect2_Pair pair = layout_file_bar_on_bot(clip, line_height);
  vim_draw_filebar(app, view, buffer, frame, face_id, pair.b);
  clip = pair.a;
 }
 
 if(view_active)
 {// Draw borders
  rect2 global_rect = global_get_screen_rectangle(app);
  FColor border_color = fcolor_id(defcolor_margin);
  if(clip.x0 > global_rect.x0)
  {
   rect2_Pair border_pair = rect_split_left_right(clip, 2.f);
   draw_rect_fcolor(app, border_pair.a, 0.f, border_color);
   clip = border_pair.b;
  }
  if(clip.x1 < global_rect.x1)
  {
   rect2_Pair border_pair = rect_split_left_right_neg(clip, 2.f);
   draw_rect_fcolor(app, border_pair.b, 0.f, border_color);
   clip = border_pair.a;
  }
  clip.y0 += 3.f;
 }
 
 if(show_fps_hud && view_active)
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
 
 {// NOTE(kv): kv_render_buffer(app, frame, view, face_id, buffer, text_layout_id, clip);
  // NOTE(kv): originally from "byp_render_buffer"
  ProfileBlock( "render buffer");
  rect2 pre_buffer_clip = draw_set_clip(app, clip);
  defer( draw_set_clip(app, pre_buffer_clip); );
  
  if(not is_game)
  {
   Buffer_Point buffer_point = scroll.position;
   Text_Layout_ID text_layout_id = text_layout_create(app, buffer, clip, buffer_point);
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
   
   Game_API *game = get_game_code(Game_On);
   if(game)
   {
    game->fui_draw_over_text_buffer(app, buffer, text_layout_id);
   }
   
   {// Error, jump (search) highlightss
    Buffer_ID comp_buffer = get_buffer_by_name(app, compilation_buffer_name, Access_Always);
    draw_jump_highlights(app, buffer, text_layout_id, comp_buffer, fcolor_id(defcolor_highlight_junk));
    // TODO(BYP): Draw error messsage annotations
    Buffer_ID jump_buffer = get_locked_jump_buffer(app);
    if (jump_buffer != comp_buffer) {
     draw_jump_highlights(app, buffer, text_layout_id, jump_buffer, fcolor_id(defcolor_highlight_white));
    }
   }
   
   {//-paren highlight
    b32 is_skm = false;
    {
     F4_Language *language = F4_LanguageFromBuffer(app, buffer);
     F4_Language *skm_lang = F4_LanguageFromExtension(SCu8("skm"));
     is_skm = (language == skm_lang);
    }
    Color_Array colors = finalize_color_array(defcolor_text_cycle);
    if(is_skm)
    {
     kv_draw_paren_highlight(app, buffer, text_layout_id, cursor_pos, colors.vals, colors.count);
    }
    else
    {
     draw_paren_highlight(app, buffer, text_layout_id, cursor_pos, colors.vals, colors.count);
    }
   }
   
   {
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
   }
   
   if(view_active && vim_state.mode == VIM_Visual) {
    vim_draw_visual_mode(app, view, buffer, face_id, text_layout_id);
   }
   
   vim_draw_search_highlight(app, view, buffer, text_layout_id, cursor_roundness);
   vim_draw_cursor(app, view, view_active, buffer, text_layout_id, cursor_roundness, mark_thickness);
   paint_fade_ranges(app, text_layout_id, buffer);
   draw_text_layout_default(app, text_layout_id);  // NOTE: this highlights the @Notes
   F4_RenderDividerComments(app, buffer, view, text_layout_id);
   
   text_layout_free(app, text_layout_id);
  }
 }
 
 draw_set_clip(app, prev_clip);
}
//-