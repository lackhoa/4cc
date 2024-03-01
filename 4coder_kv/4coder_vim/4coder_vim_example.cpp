
#include "4coder_vimrc.h"
#include "4coder_vim_include.h"

#if !defined(META_PASS)
#include "generated/managed_id_metadata.cpp"
#endif

VIM_TEXT_OBJECT_SIG(EXAMPLE_object_camel){
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

function void EXAMPLE_make_vim_request(Application_Links *app, EXAMPLE_Vim_Request request){
	vim_make_request(app, Vim_Request_Type(VIM_REQUEST_COUNT + request));
}

VIM_REQUEST_SIG(EXAMPLE_apply_title){
	Scratch_Block scratch(app);
	String_Const_u8 text = push_buffer_range(app, scratch, buffer, range);
	u8 prev = buffer_get_char(app, buffer, range.min-1);
	for(i32 i=0; i<text.size; i++){
		text.str[i] += u8(i32('A' - 'a')*((!character_is_alpha(prev) || prev == '_') &&
										  character_is_lower(text.str[i])));
		prev = text.str[i];
	}
	buffer_replace_range(app, buffer, range, text);
	buffer_post_fade(app, buffer, 0.667f, range, fcolor_resolve(fcolor_id(defcolor_paste)));
}

VIM_REQUEST_SIG(EXAMPLE_apply_rot13){
	Scratch_Block scratch(app);
	String_Const_u8 text = push_buffer_range(app, scratch, buffer, range);
	for(i32 i=0; i<text.size; i++){
		if(0){}
		else if(character_is_lower(text.str[i])){
			text.str[i] = (text.str[i] + 13) - 26*(text.str[i] > ('z'-13));
		}
		else if(character_is_upper(text.str[i])){
			text.str[i] = (text.str[i] + 13) - 26*(text.str[i] > ('Z'-13));
		}
	}
	buffer_replace_range(app, buffer, range, text);
	buffer_post_fade(app, buffer, 0.667f, range, fcolor_resolve(fcolor_id(defcolor_paste)));
}
VIM_COMMAND_SIG(EXAMPLE_request_title){ EXAMPLE_make_vim_request(app, EXAMPLE_REQUEST_Title); }
VIM_COMMAND_SIG(EXAMPLE_request_rot13){ EXAMPLE_make_vim_request(app, EXAMPLE_REQUEST_Rot13); }
