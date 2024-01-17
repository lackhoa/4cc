#pragma once

#include "4coder_vim_registers.cpp"
#include "4coder_vim_lister.cpp"

function String_Const_u8_Array
kv_string_split_wildcards(Arena *arena, String_Const_u8 string)
{
  String_Const_u8_Array array = {};
  List_String_Const_u8 list = string_split(arena, string, (u8*)"* ", 2);
  array.count   = list.node_count;
  array.strings = push_array(arena, String_Const_u8, array.count);
  i64 index = 0;
  for (Node_String_Const_u8 *node = list.first;
       node;
       node = node->next)
  {
    kv_assert(index < array.count);
    array.strings[index++] = node->string;
  }
  return(array);
}

function i64
kv_fuzzy_search_forward(Application_Links *app, Buffer_ID buffer, i64 pos, String_Const_u8 needle)
{
  i64 buffer_size = buffer_get_size(app, buffer);
  i64 result = buffer_size;

  Scratch_Block temp(app);
  String_Const_u8_Array splits = kv_string_split_wildcards(temp, needle);
  if ( !splits.count ) { return result; }

  while( pos < buffer_size )
  {
    i64 original_pos = pos;
    String_Match first_match = buffer_seek_string(app, buffer, splits.strings[0], Scan_Forward, pos);
    if ( !first_match.buffer ) break;

    i64 match_start = first_match.range.min;
    i64 line_end    = get_line_end_pos_from_pos(app, buffer, match_start);
    pos = first_match.range.end - 1;
    b32 matched = true;
    for (i64 index = 1;
         index < splits.count;
         index++)
    {
      String_Const_u8 substring = splits.strings[index];
      String_Match match = buffer_seek_string(app, buffer, substring, Scan_Forward, pos);
      if ( match.buffer )
      {
        if ( match.range.max <= line_end )
        {
          pos = match.range.end - 1;
        }
        else
        {
          pos = get_line_start_pos_from_pos(app, buffer, match.range.start) - 1;
          matched = false;
          break;
        }
      }
      else
      {
        return result;
      }
    }
    if ( matched )
    {
      result = match_start;
      break;
    }

    kv_assert_defend(pos > original_pos, return buffer_size;);
  }

  return result;
}

function i64
kv_fuzzy_search_backward(Application_Links *app, Buffer_ID buffer, i64 pos, String_Const_u8 needle)
{
  i64 buffer_size = buffer_get_size(app, buffer);
  i64 result = -1;

  Scratch_Block temp(app);
  String_Const_u8_Array splits = kv_string_split_wildcards(temp, needle);
  if ( !splits.count ) { return result; }

  while( pos > -1 )
  {
    i64 original_pos = pos;
    String_Match first_match = buffer_seek_string(app, buffer, splits.strings[splits.count-1], Scan_Backward, pos);
    if( !first_match.buffer ) break;

    i64 match_start = first_match.range.max;
    i64 line_start   = get_line_start_pos_from_pos(app, buffer, match_start);
    pos = first_match.range.start;
    b32 matched = true;
    for (i64 index = splits.count-2;
         index >= 0;
         index--)
    {
      String_Const_u8 substring = splits.strings[index];
      String_Match match = buffer_seek_string(app, buffer, substring, Scan_Backward, pos);
      if ( match.buffer)
      {
        if ( match.range.min >= line_start )
        {
          pos = match.range.start;
        }
        else
        {
          pos = get_line_end_pos_from_pos(app, buffer, match.range.start);
          matched = false;
          break;
        }
      }
      else
      {
        return result;
      }
    }
    if ( matched )
    {
      result = pos;
      break;
    }

    kv_assert_defend(pos < original_pos, return result;);
  }

  return result;
}

// todo(kv): we don't support case sensitive search, cause don't need it.
function i64 vim_pattern_inner_v(Application_Links *app, Buffer_Seek_String_Flags seek_flags)
{
	String_u8 *pattern = &vim_registers.search.data;
	if(pattern->size == 0){ return -1; }

	View_ID view = get_active_view(app, Access_ReadVisible);
	Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
	i64 pos = view_get_cursor_pos(app, view);
    if (seek_flags & BufferSeekString_Backward)
    {
      return kv_fuzzy_search_backward(app, buffer, pos, pattern->string);
    }
    else
    {
      return kv_fuzzy_search_forward(app, buffer, pos, pattern->string);
    }
}

function void vim_in_pattern_inner(Application_Links *app, Buffer_Seek_String_Flags seek_flags){
	String_u8 *pattern = &vim_registers.search.data;
	i64 new_pos = vim_pattern_inner_v(app, seek_flags);
	View_ID view = get_active_view(app, Access_ReadVisible);
	Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
	i64 buffer_size = buffer_get_size(app, buffer);
	if(new_pos > 0 && new_pos != buffer_size){
		vim_push_jump(app, view);
		Vim_Motion_Block vim_motion_block(app, new_pos + pattern->size-1);
		view_set_cursor_and_preferred_x(app, view, seek_pos(new_pos));
	}
}

function void
vim_to_pattern_inner(Application_Links *app, Buffer_Seek_String_Flags seek_flags)
{
  View_ID view = get_active_view(app, Access_ReadVisible);
  Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
  i64 buffer_size = buffer_get_size(app, buffer);
  i64 new_pos = vim_pattern_inner_v(app, seek_flags);
  if(new_pos > 0 && new_pos != buffer_size){
    vim_push_jump(app, view);
    Vim_Motion_Block vim_motion_block(app);
    view_set_cursor_and_preferred_x(app, view, seek_pos(new_pos));
  }
}

function void
vim_start_search_inner(Application_Links *app, Scan_Direction start_direction){
	View_ID view = get_active_view(app, Access_ReadVisible);
	Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
	if(!buffer_exists(app, buffer)){ return; }

	i64 buffer_size = buffer_get_size(app, buffer);

	Vec2_f32 old_margin = {};
	Vec2_f32 old_push_in = {};
	view_get_camera_bounds(app, view, &old_margin, &old_push_in);

	Vec2_f32 margin = old_margin;
	margin.y = clamp_bot(200.f, margin.y);
	view_set_camera_bounds(app, view, margin, old_push_in);

	Scan_Direction direction = start_direction;
	i64 pos = view_get_cursor_pos(app, view);

	/// TODO(BYP): Need a better system to have multiple types of these drawing
	vim_use_bottom_cursor = true;
	String_Const_u8 prefix = (start_direction == Scan_Forward ? string_u8_litexpr("/") : string_u8_litexpr("?"));
	vim_set_bottom_text(prefix);
	u8 *dest = vim_bot_text.str + vim_bot_text.size;
	u64 base_size, after_size;
	base_size = after_size = vim_bot_text.size;

	Vim_Register *reg = &vim_registers.search;
	if(reg->data.size < 256){ vim_realloc_string(&reg->data, 0); }
	reg->data.size = 0;
	String_u8 *query = &reg->data;

	f32 prev_offset = vim_nxt_filebar_offset;

	i64 match_size = 0;
	User_Input in = {};
	for(;;)
    {
		if(vim_nxt_filebar_offset == 0.f){ vim_nxt_filebar_offset = 0.1f; }
		animate_in_n_milliseconds(app, 0);
		vim_set_bottom_text(prefix);
		block_copy(dest, query->str, query->size);
		vim_bot_text.size = after_size + query->size;

		in = get_next_input(app, EventPropertyGroup_Any, EventProperty_Escape);
		if(in.abort){ query->size = 0; break; }

		String_Const_u8 string = to_writable(&in);

		b32 string_change = false;
		if(match_key_code(&in, KeyCode_Return))
        {
          reg->flags |= (REGISTER_Set|REGISTER_Updated);
          break;
		}
		else if(string.str && string.size)
        {
			string_append(query, string);
			string_change = true;
		}
		else if(match_key_code(&in, KeyCode_Backspace))
        {
          u64 old_size = query->size;
          if(has_modifier(&in.event.key.modifiers, KeyCode_Control)){
            if(has_modifier(&in.event.key.modifiers, KeyCode_Shift)){
              query->string.size = 0;
            }else{
              query->string = ctrl_backspace_utf8(query->string);
            }
          }else{
            query->string = backspace_utf8(query->string);
          }
          string_change = old_size != query->size;
		}
		else { leave_current_input_unhandled(app); }

		if(string_change)
        {
          reg->flags |= (REGISTER_Set|REGISTER_Updated);
          vim_update_registers(app);
          i64 new_pos = 0;
          if (direction == Scan_Forward)
          {
            // seek_string_insensitive_forward(app, buffer, pos - 1, 0, query->string, &new_pos);
            new_pos = kv_fuzzy_search_forward(app, buffer, pos - 1, query->string);
            if(new_pos < buffer_size){
              pos = new_pos;
              match_size = query->string.size;
            }
          }
          else
          {
            // seek_string_insensitive_backward(app, buffer, pos + 1, 0, query->string, &new_pos);
            new_pos = kv_fuzzy_search_backward(app, buffer, pos+1, query->string);
            if(new_pos >= 0){
              pos = new_pos;
              match_size = query->string.size;
            }
          }

          if( in_range(0, new_pos, buffer_size) )
          {
            view_set_cursor_and_preferred_x(app, view, seek_pos(new_pos));
            isearch__update_highlight(app, view, Ii64_size(new_pos, match_size));
          }
		}
	}

	view_disable_highlight_range(app, view);

	vim_reset_bottom_text();
	vim_use_bottom_cursor = false;
	vim_nxt_filebar_offset = prev_offset;
	view_set_camera_bounds(app, view, old_margin, old_push_in);
}

VIM_COMMAND_SIG(vim_clear_search){
	vim_registers.search.data.size = 0;
	vim_registers.search.flags &= (~REGISTER_Set);
	vim_update_registers(app);
}

VIM_COMMAND_SIG(vim_start_search_forward){  vim_start_search_inner(app, Scan_Forward); }
VIM_COMMAND_SIG(vim_start_search_backward){ vim_start_search_inner(app, Scan_Backward); }

VIM_COMMAND_SIG(vim_to_next_pattern){ vim_to_pattern_inner(app, 0); }
VIM_COMMAND_SIG(vim_to_prev_pattern){ vim_to_pattern_inner(app, BufferSeekString_Backward); }

VIM_COMMAND_SIG(vim_in_next_pattern){ vim_in_pattern_inner(app, 0); }
VIM_COMMAND_SIG(vim_in_prev_pattern){ vim_in_pattern_inner(app, BufferSeekString_Backward); }

VIM_COMMAND_SIG(vim_search_identifier){
	View_ID view = get_active_view(app, Access_ReadVisible);
	Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
	i64 pos = view_get_cursor_pos(app, view);
	Range_i64 range = enclose_pos_alpha_numeric_underscore(app, buffer, pos);
	vim_state.params.selected_reg = &vim_registers.search;
	vim_request_vtable[REQUEST_Yank](app, view, buffer, range);
	vim_default_register();
}

