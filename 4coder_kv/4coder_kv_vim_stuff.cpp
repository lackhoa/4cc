#include "4coder_kv_utils.cpp"

VIM_REQUEST_SIG(byp_apply_title)
{
	Scratch_Block scratch(app);
	String_Const_u8 text = push_buffer_range(app, scratch, buffer, range);
	u8 prev = buffer_get_char(app, buffer, range.min-1);
	foreach(i, text.size){
		text.str[i] += u8(i32('A' - 'a')*((!character_is_alpha(prev) || prev == '_') &&
										  character_is_lower(text.str[i])));
		prev = text.str[i];
	}
	buffer_replace_range(app, buffer, range, text);
	buffer_post_fade(app, buffer, 0.667f, range, fcolor_resolve(fcolor_id(defcolor_paste)));
}

VIM_TEXT_OBJECT_SIG(byp_object_param)
{
	u8 c = buffer_get_char(app, buffer, cursor_pos);
	Range_i64 range = Ii64(cursor_pos + (c == ',' || c == ';'));

	for(; range.min>0; range.min--){
		c = buffer_get_char(app, buffer, range.min);
		if(c == ',' || c == ';'){ break; }
		Scan_Direction bounce = vim_bounce_direction(c);
		if(bounce == Scan_Forward){ break; }
		if(bounce == Scan_Backward){
			range.min = vim_bounce_pair(app, buffer, range.min, c)-1;
			continue;
		}
	}

	i64 buffer_size = buffer_get_size(app, buffer);
	for(; range.max < buffer_size; range.max++){
		c = buffer_get_char(app, buffer, range.max);
		if(c == ',' || c == ';'){ break; }
		Scan_Direction bounce = vim_bounce_direction(c);
		if(bounce == Scan_Backward){ break; }
		if(bounce == Scan_Forward){
			range.max = vim_bounce_pair(app, buffer, range.max, c);
			continue;
		}
	}
	range.min++; range.max--;
	if(range.min >= range.max){ range = {}; }

	return range;
}

VIM_TEXT_OBJECT_SIG(byp_object_camel)
{
	Range_i64 range = {};
	Scratch_Block scratch(app);
	Range_i64 line_range = get_line_range_from_pos(app, buffer, cursor_pos);
	i64 s = line_range.min;
	u8 *line_text = push_buffer_range(app, scratch, buffer, line_range).str;
	u8 c = line_text[cursor_pos-s];
	if(!character_is_alpha_numeric(c)){ return {}; }
	cursor_pos += line_text[cursor_pos-s] == '_';
	range.min = range.max = cursor_pos;

	b32 valid=false;
	for(; range.min>0; range.min--){
		c = line_text[range.min-s];
		if(!character_is_alpha_numeric(c) || c == '_'){ valid = true; break; }
	}
	if(!valid){ return {}; }

	valid=false;
	for(; range.max>0; range.max++){
		c = line_text[range.max-s];
		if(!character_is_alpha_numeric(c) || c == '_'){ valid = true; break; }
	}
	if(!valid){ return {}; }

	range.min += (vim_state.params.clusivity != VIM_Inclusive || line_text[range.min-s] != '_');
	range.max -= (vim_state.params.clusivity != VIM_Inclusive || line_text[range.max-s] != '_');
	if(range.min >= range.max){ range = {}; }

	return range;
}

inline void
kv_vim_init(Application_Links *app)
{
  vim_init(app);
}

