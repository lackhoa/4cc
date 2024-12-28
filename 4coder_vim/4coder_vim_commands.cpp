function void
right_adjust_view(App_Cmd *app)
{
	View_ID view = get_active_view(app, Access_ReadVisible);
	Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
	i64 pos = view_get_cursor_pos(app, view);
	if(!line_is_valid_and_blank(app, buffer, get_line_number_from_pos(app, buffer, pos))){
		i64 new_pos = get_line_side_pos_from_pos(app, buffer, pos, Side_Min);
		if(char_is_whitespace(buffer_get_char(app, buffer, new_pos))){
			new_pos = buffer_seek_character_class_change_1_0(app, buffer, &character_predicate_whitespace, Scan_Forward, new_pos);
		}
		view_set_cursor_and_preferred_x(app, view, seek_pos(new_pos));
	}
	Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
	scroll.target.pixel_shift.x = 0.f;
	view_set_buffer_scroll(app, view, scroll, SetBufferScroll_NoCursorChange);
}

#if VIM_USE_REGISTER_BUFFER
function void
reg(App_Cmd *app)
{
	vim_show_buffer_peek = 0;
	vim_buffer_peek_index = 1;
	view_enqueue_command_function(app, get_active_view(app, Access_ReadVisible), vim_toggle_show_buffer_peek);
}
#endif


function void
vim_normal_mode(App *app)
{
 View_ID view = get_active_view(app, Access_ReadVisible);
	if(vim_state.mode == VIM_Insert)
 {
  Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
  
  //NOTE(kv) I have no idea why we clear the insert register,
  //  because we put it back later...
  vim_registers.insert.data.size = 0;
  
  Scratch_Block scratch(app);
  
  History_Record_Index index = vim_state.insert_index;
  History_Record_Index max_index = buffer_history_get_current_state_index(app, buffer);
  i64 prev_pos = vim_state.insert_cursor.pos;
  for(; index <= max_index; index++)
  {
   Record_Info record = buffer_history_get_record_info(app, buffer, index);
   if(record.error != RecordError_NoError){ continue; }
   if(record.kind == RecordKind_Single)
   {
    vim_process_insert_record(record, &prev_pos);
   }
   else if(record.kind == RecordKind_Group)
   {
    foreach(i, record.group_count)
    {
     Record_Info sub_record = buffer_history_get_group_sub_record(app, buffer, index, i);
     if(sub_record.error != RecordError_NoError){ continue; }
     vim_process_insert_record(sub_record, &prev_pos);
    }
   }
  }
  vim_state.prev_params.do_insert = true;
  vim_registers.insert.flags &= (~REGISTER_Append);
  vim_registers.insert.flags |= (REGISTER_Set|REGISTER_Updated);
  vim_update_registers(app);
  
  history_group_end(vim_history_group);
  
  move_horizontal_lines(app, -1);
	}
	else if(vim_state.mode == VIM_Visual)
 {
		vim_set_prev_visual(app, view);
	}
	vim_reset_state();
}


function void 
vim_insert_mode_after(App_Cmd *app)
{
 View_ID view = get_active_view(app, Access_ReadVisible);
 Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
 i64 pos   = view_get_cursor_pos(app, view);
#if 0
 i64 end   = get_line_end_pos_from_pos(app, buffer, pos);
 i64 start = get_line_start_pos_from_pos(app, buffer, pos);
#endif
 u8 c = buffer_get_char(app, buffer, pos);
 if (c != '\n' && c != '\r')
 {
  move_right(app);
 }
 vim_enter_insert_mode(app);
}
function void 
vim_insert_begin(App_Cmd *app){ vim_begin_line(app); vim_enter_insert_mode(app); }

function void 
vim_insert_end(App_Cmd *app)
{
 vim_end_line(app);
 vim_insert_mode_after(app);
}

function void 
vim_modal_i(App *app)
{
 vim_state.dot_delete_count = 0;
 vim_enter_insert_mode(app);
}

function void 
vim_modal_a(App_Cmd *app)
{
	if(vim_state.mode == VIM_Visual || vim_state.params.request != REQUEST_None){
		vim_state.params.clusivity = VIM_Inclusive;
		u8 key = vim_query_user_key(app, strlit("-- TEXT OBJECT --"));
		if(key){
			vim_state.params.seek.character = key;
			vim_state.active_command = vim_text_object;
			vim_text_object(app);
		}
	}
	else{ vim_insert_mode_after(app); }
}
function void 
vim_newline_below(App_Cmd *app)
{
 vim_insert_end(app);
 vim_state.insert_index++;
 write_text(app, strlit("\n"), true);
 auto_indent_line_at_cursor(app);
}
function void
vim_newline_above(App_Cmd *app)
{
    vim_line_start(app);
 vim_enter_insert_mode(app);
 vim_state.insert_index++;
 write_text(app, strlit("\n"), true);
 move_vertical_lines(app, -1);
 auto_indent_line_at_cursor(app);
}

function void
vim_visual_char_mode(App_Cmd *app)
{
	if(vim_state.mode != VIM_Visual)
 {
		set_mark(app);
  vim_state.mode = VIM_Visual;
	}
	vim_state.params.edit_type = EDIT_CharWise;
}

function void 
vim_visual_mode(App_Cmd *app)
{
	if(vim_state.mode != VIM_Visual){
		set_mark(app);
		vim_state.mode = VIM_Visual;
	}
	vim_state.params.edit_type = EDIT_CharWise;
	Input_Event event = get_current_input(app).event;
	if(event.kind == InputEventKind_KeyStroke){
		if(0){}
		else if(has_modifier(&event, Key_Code_Shift)){   vim_state.params.edit_type = EDIT_LineWise; }
		else if(has_modifier(&event, Key_Code_Control)){ vim_state.params.edit_type = EDIT_Block; }
	}
}

function void 
vim_prev_visual(App_Cmd *app){
	View_ID view = get_active_view(app, Access_ReadVisible);
	Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
	Managed_Scope scope = buffer_get_managed_scope(app, buffer);

	Vim_Prev_Visual *prev_visual = scope_attachment(app, scope, vim_buffer_prev_visual, Vim_Prev_Visual);
	if(prev_visual && prev_visual->cursor_pos != 0 && prev_visual->mark_pos != 0){
		view_set_cursor_and_preferred_x(app, view, seek_pos(prev_visual->cursor_pos));
		view_set_mark(app, view, seek_pos(prev_visual->mark_pos));
		vim_state.params.edit_type = prev_visual->edit_type;
		vim_state.mode = VIM_Visual;
	}
}

function void 
vim_block_swap(App_Cmd *app){
	View_ID view = get_active_view(app, Access_ReadVisible);
	Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
	i64 c = view_get_cursor_pos(app, view);
	i64 m = view_get_mark_pos(app, view);
	i64 line = get_line_number_from_pos(app, buffer, c);
	v2 c_p = view_relative_xy_of_pos(app, view, line, c);
	v2 m_p = view_relative_xy_of_pos(app, view, line, m);
	macro_swap(c_p.x, m_p.x);
	c = view_pos_at_relative_xy(app, view, line, c_p);
	m = view_pos_at_relative_xy(app, view, line, m_p);
	view_set_cursor(app, view, seek_pos(c));
	view_set_mark(app, view, seek_pos(m));
}

function void 
vim_replace_mode(App_Cmd *app){
	vim_state.mode = VIM_Replace;
	set_mark(app);
	View_ID view = get_active_view(app, Access_ReadVisible);
	Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
	vim_history_group = history_group_begin(app, buffer);
}

function void
vim_visual_insert(App_Cmd *app)
{
	if(vim_state.params.edit_type == EDIT_Block){
		View_ID view = get_active_view(app, Access_ReadVisible);
		Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);

		vim_visual_insert_inner(app, view, buffer);

		i64 cursor_pos = view_get_cursor_pos(app, view);
		if(line_is_valid_and_blank(app, buffer, get_line_number_from_pos(app, buffer, cursor_pos))){
			auto_indent_buffer(app, buffer, Ii64(cursor_pos));
			view_set_cursor(app, view, seek_pos(get_pos_past_lead_whitespace(app, buffer, cursor_pos)));
		}
		i64 mark_pos = view_get_mark_pos(app, view);
		if(line_is_valid_and_blank(app, buffer, get_line_number_from_pos(app, buffer, mark_pos))){
			auto_indent_buffer(app, buffer, Ii64(mark_pos));
			view_set_mark(app, view, seek_pos(get_pos_past_lead_whitespace(app, buffer, mark_pos)));
		}

		User_Input input = get_current_input(app);
		if(input.event.kind == InputEventKind_KeyStroke && input.event.key.code == Key_Code_A){
			vim_visual_insert_after = true;
		}
	}
}
function void 
vim_submode_g(App_Cmd *app){ vim_state.sub_mode = SUB_G; vim_state.chord_resolved = false; }
function void 
vim_submode_z(App_Cmd *app){ vim_state.sub_mode = SUB_Z; vim_state.chord_resolved = false; }
function void 
vim_submode_leader(App_Cmd *app){ vim_state.sub_mode = SUB_Leader; vim_state.chord_resolved = false; }

function void 
vim_replace_next_char(App_Cmd *app)
{
 u8 key = vim_query_user_key(app, strlit("-- REPLACE NEXT --"));
 if ( key )
 {
  View_ID view = get_active_view(app, Access_ReadVisible);
  Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
  i64 pos = view_get_cursor_pos(app, view);
  String string = SCu8(&key, 1);
  buffer_replace_range(app, buffer, Ii64(pos, pos+1), string);
  vim_register_copy(&vim_registers.insert, string);
  vim_state.dot_do_insert    = true;
  vim_state.dot_delete_count = 1;
	}
}


function void
vim_replace_range_next(App_Cmd *app)
{
	u8 key = vim_query_user_key(app, strlit("-- RANGE REPLACE NEXT --"));
	if(key)
 {
		vim_state.params.seek.character = key;
		vim_make_request(app, REQUEST_Replace);
	}
}

function void
vim_request_yank(App_Cmd *app){ vim_make_request(app, REQUEST_Yank); }

function void 
vim_request_delete(App_Cmd *app)
{
 vim_state.dot_do_insert = false;
 vim_make_request(app, REQUEST_Delete);
}

function void
vim_request_change(App_Cmd *app){ vim_make_request(app, REQUEST_Change); }

function void vim_uppercase(App_Cmd *app){       vim_make_request(app, REQUEST_Upper); }
function void vim_lowercase(App_Cmd *app){       vim_make_request(app, REQUEST_Lower); }
function void vim_toggle_case(App_Cmd *app){     vim_make_request(app, REQUEST_ToggleCase); }
function void vim_request_indent(App_Cmd *app){  vim_make_request(app, REQUEST_Indent); }
function void vim_request_outdent(App_Cmd *app){ vim_make_request(app, REQUEST_Outdent); }
function void vim_request_auto_indent(App_Cmd *app){ vim_make_request(app, REQUEST_AutoIndent); }

function void 
vim_toggle_char(App_Cmd *app)
{
	View_ID view = get_active_view(app, Access_ReadVisible);
	Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
	i64 pos = view_get_cursor_pos(app, view);
	u8 c = buffer_get_char(app, buffer, pos);
	c = character_toggle_case(c);
	buffer_replace_range(app, buffer, Ii64_size(pos, 1), SCu8(&c, 1));
}

function void 
vim_delete_end(App_Cmd *app){
	View_ID view = get_active_view(app, Access_ReadVisible);
	Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
	i64 pos = view_get_cursor_pos(app, view);
	i64 line_num = get_line_number_from_pos(app, buffer, pos);
	if(line_is_valid_and_blank(app, buffer, line_num)){ vim_reset_state(); return; }
	Vim_Motion_Block vim_motion_block(app);
	vim_state.params.request = REQUEST_Delete;
	vim_state.params.clusivity = VIM_Exclusive;
	seek_end_of_line(app);
}
function void 
vim_delete_to_begin(App_Cmd *app){
	View_ID view = get_active_view(app, Access_ReadVisible);
	Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
	i64 pos = view_get_cursor_pos(app, view);
	i64 line_num = get_line_number_from_pos(app, buffer, pos);
	if(line_is_valid_and_blank(app, buffer, line_num)){ vim_reset_state(); return; }
	Vim_Motion_Block vim_motion_block(app);
	vim_state.params.request = REQUEST_Delete;
	vim_state.params.clusivity = VIM_Exclusive;
	seek_beginning_of_line(app);
}

function void 
vim_change_end(App_Cmd *app){
	Vim_Motion_Block vim_motion_block(app);
	vim_state.params.request = REQUEST_Change;
	vim_state.params.clusivity = VIM_Exclusive;
	seek_end_of_line(app);
}

function void 
vim_yank_end(App_Cmd *app){
	Vim_Motion_Block vim_motion_block(app);
	vim_state.params.request = REQUEST_Yank;
	vim_state.params.clusivity = VIM_Exclusive;
	seek_end_of_line(app);
}


function void 
vim_leader_d(App_Cmd *app){ vim_state.params.selected_reg=0; vim_request_delete(app); }
function void 
vim_leader_D(App_Cmd *app){ vim_state.params.selected_reg=0; vim_delete_end(app); }
function void 
vim_leader_c(App_Cmd *app){ vim_state.params.selected_reg=0; vim_request_change(app); }
function void 
vim_leader_C(App_Cmd *app){ vim_state.params.selected_reg=0; vim_change_end(app); }

function void 
vim_digit(App_Cmd *app){
	User_Input input = get_current_input(app);
	if(input.event.kind == InputEventKind_KeyStroke){
		int digit = input.event.key.code - Key_Code_0;
		if(in_range_exclusive(0, digit, 10)){
			vim_state.number *= 10;
			vim_state.number += digit;
		}
		vim_state.chord_resolved = false;
	}
}

function void 
vim_digit_del(App_Cmd *app){
	if(vim_state.number != 0){
		vim_state.number /= 10;
		vim_keystroke_text.size = vim_pre_keystroke_size-1;
		vim_state.chord_resolved = false;
	}else{
		vim_reset_state();
	}
}

function void 
vim_modal_0(App_Cmd *app){
	if(vim_state.number){ vim_digit(app); }
	else{ vim_begin_line(app); }
	//else{ vim_line_start(app); }
}


function void 
vim_paragraph_up(App_Cmd *app){
	vim_push_jump(app, get_active_view(app, Access_ReadVisible));
	Vim_Motion_Block vim_motion_block(app);
	vim_state.params.edit_type = EDIT_LineWise;
	vim_state.params.clusivity = VIM_Exclusive;
	const i1 N = vim_consume_number();
	foreach(i,N)
		move_up_to_blank_line_end(app);
}

function void 
vim_paragraph_down(App_Cmd *app){
	vim_push_jump(app, get_active_view(app, Access_ReadVisible));
	Vim_Motion_Block vim_motion_block(app);
	vim_state.params.edit_type = EDIT_LineWise;
	vim_state.params.clusivity = VIM_Exclusive;
	const i1 N = vim_consume_number();
	foreach(i,N)
		move_down_to_blank_line_end(app);
}

function void 
vim_whole_page_up(App_Cmd *app){   vim_page_scroll_inner(app, -1.0f); }
function void 
vim_whole_page_down(App_Cmd *app){ vim_page_scroll_inner(app,  1.0f); }
function void 
vim_half_page_up(App_Cmd *app){    vim_page_scroll_inner(app, -0.5f); }
function void 
vim_half_page_down(App_Cmd *app){  vim_page_scroll_inner(app,  0.5f); }

function void 
vim_scroll_screen_top(App_Cmd *app){ vim_scroll_inner(app,  0.0f); }
function void 
vim_scroll_screen_mid(App_Cmd *app){ vim_scroll_inner(app, -0.5f); }
function void 
vim_scroll_screen_bot(App_Cmd *app){ vim_scroll_inner(app, -1.0f); }

function void 
vim_screen_top(App_Cmd *app){ vim_screen_inner(app, 0.0f,  vim_consume_number()); }
function void 
vim_screen_mid(App_Cmd *app){ vim_screen_inner(app, 0.5f,  vim_consume_number()); }
function void 
vim_screen_bot(App_Cmd *app){ vim_screen_inner(app, 1.0f, -vim_consume_number()); }

function void 
vim_line_up(App_Cmd *app){
	View_ID view = get_active_view(app, Access_ReadVisible);
	f32 line_height = get_face_metrics(app, get_face_id(app, 0)).line_height;
	Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
	scroll.target = view_move_buffer_point(app, view, scroll.target, V2(0.f, line_height));
	view_set_buffer_scroll(app, view, scroll, SetBufferScroll_SnapCursorIntoView);
}

function void 
vim_line_down(App_Cmd *app)
{
	View_ID view = get_active_view(app, Access_ReadVisible);
	f32 line_height = get_face_metrics(app, get_face_id(app, 0)).line_height;
	Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
	scroll.target = view_move_buffer_point(app, view, scroll.target, V2(0.f, -line_height));
	view_set_buffer_scroll(app, view, scroll, SetBufferScroll_SnapCursorIntoView);
}

function b32
vim_is_wb_pivot(App *app, Buffer_ID buffer, i64 pos)
{
 u8 c = buffer_get_char(app, buffer, pos);
 if ( char_is_whitespace(c) )
 {
  return false;
 }
 else
 {
  u8 b = buffer_get_char(app, buffer, pos-1);
  if ( char_is_whitespace(b) )
  {
   return true;
  }
  else
  {
   b32 cw = character_predicate_check(character_predicate_word, c);
   b32 bw = character_predicate_check(character_predicate_word, b);
   if (cw != bw)
   {
    return true;
   }
   else if (cw)
   {
    if (character_is_upper(c) && character_is_lower(b))
    {
     return true;
    }
    else { return false; }
   }
   else { return false; }
  }
 }
}

function void 
vim_w_cmd(App_Cmd *app)
{
 Vim_Motion_Block vim_motion_block(app);
 vim_state.params.clusivity = VIM_Exclusive; //NOTE(kv): this is so that "dw" doesn't delete the character at the cursor after movement.
 
 View_ID view = get_active_view(app, Access_ReadVisible);
 i64 pos = view_get_cursor_pos(app, view);
 i64 prev_pos = pos;
 
 Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
 i64 size = buffer_get_size(app, buffer);
 if ( pos < size )
 {
  pos++;
 }
 while ( pos < size && !vim_is_wb_pivot(app, buffer, pos) )
 {
  pos++;
 }
 
 view_set_cursor_and_preferred_x(app, view, seek_pos(pos));
 if (prev_pos != pos)
 {
  i64 line0 = get_line_number_from_pos(app, buffer, prev_pos);
  i64 line1 = get_line_number_from_pos(app, buffer, pos);
  if ( line0 != line1 )
  {//NOTE(kv): this is so that "dw" doesn't delete past the line end
   vim_motion_block.clamp_end = get_line_side_pos(app, buffer, line0, Side_Max);
  }
 }
}

function void
vim_b_cmd(App_Cmd *app)
{
 Vim_Motion_Block vim_motion_block(app);
 View_ID view = get_active_view(app, Access_ReadVisible);
 i64 pos;
 {
  Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
  pos = view_get_cursor_pos(app, view);
  if ( pos > 0 )
  {
   pos--;
  }
  while ( pos > 0 && !vim_is_wb_pivot(app, buffer, pos) )
  {
   pos--;
  }
 }
 view_set_cursor_and_preferred_x(app, view, seek_pos(pos));
}

function void 
vim_forward_WORD(App_Cmd *app){
	Vim_Motion_Block vim_motion_block(app);
	vim_state.params.clusivity = VIM_Exclusive;
	View_ID view = get_active_view(app, Access_ReadVisible);
	i64 prev_pos = -1;
	i64 pos = vim_scan_WORD(app, view, Scan_Forward, &prev_pos, vim_consume_number());
	view_set_cursor_and_preferred_x(app, view, seek_pos(pos));
	if(prev_pos != pos){
		Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
		i64 line0 = get_line_number_from_pos(app, buffer, prev_pos);
		i64 line1 = get_line_number_from_pos(app, buffer, pos);
		if(line0 != line1){
			vim_motion_block.clamp_end = get_line_side_pos(app, buffer, line0, Side_Max);
		}
	}
}

function void 
vim_backward_WORD(App_Cmd *app){
	View_ID view = get_active_view(app, Access_ReadVisible);
	i64 cursor_pos = view_get_cursor_pos(app, view);
	Vim_Motion_Block vim_motion_block(app, cursor_pos-1);
	i64 pos = vim_scan_WORD(app, view, Scan_Backward, 0, vim_consume_number());
	view_set_cursor_and_preferred_x(app, view, seek_pos(pos));
}

function void 
vim_forward_end(App_Cmd *app){
	Vim_Motion_Block vim_motion_block(app);
	View_ID view = get_active_view(app, Access_ReadVisible);
	i64 pos = vim_scan_end(app, view, Scan_Forward, vim_consume_number());
	view_set_cursor_and_preferred_x(app, view, seek_pos(pos));
}

function void 
vim_backward_end(App_Cmd *app){
	Vim_Motion_Block vim_motion_block(app);
	View_ID view = get_active_view(app, Access_ReadVisible);
	i64 pos = vim_scan_end(app, view, Scan_Backward, vim_consume_number());
	view_set_cursor_and_preferred_x(app, view, seek_pos(pos));
}

function void 
vim_forward_END(App_Cmd *app){
	Vim_Motion_Block vim_motion_block(app);
	View_ID view = get_active_view(app, Access_ReadVisible);
	i64 pos = vim_scan_END(app, view, Scan_Forward, vim_consume_number());
	view_set_cursor_and_preferred_x(app, view, seek_pos(pos));
}

function void 
vim_backward_END(App_Cmd *app){
	Vim_Motion_Block vim_motion_block(app);
	View_ID view = get_active_view(app, Access_ReadVisible);
	i64 pos = vim_scan_END(app, view, Scan_Backward, vim_consume_number());
	view_set_cursor_and_preferred_x(app, view, seek_pos(pos));
}

function void 
vim_bounce(App_Cmd *app){
	vim_push_jump(app, get_active_view(app, Access_ReadVisible));
	Vim_Motion_Block vim_motion_block(app);
	Scan_Direction direction = Scan_Forward;
	Input_Event event = get_current_input(app).event;
	if(event.kind == InputEventKind_KeyStroke && has_modifier(&event, Key_Code_Control)){ direction=Scan_Backward; }
	View_ID view = get_active_view(app, Access_ReadVisible);
	Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
	i64 pos = view_get_cursor_pos(app, view);
	pos = vim_scan_bounce(app, buffer, pos, direction);
	view_set_cursor_and_preferred_x(app, view, seek_pos(pos));
}

function void 
vim_modal_percent(App_Cmd *app)
{
	if(vim_state.number){ vim_percent_file(app); }
	else{ vim_bounce(app); }
}

myinline void 
buffer_delete_range(App_Cmd *app, Buffer_ID buffer, Range_i64 range){
 buffer_replace_range(app, buffer, range, empty_string);
}

function void
vim_paste_before(App_Cmd *app)
{
 if(vim_state.params.selected_reg)
 {
  View_ID   view   = get_active_view(app, Access_ReadVisible);
  Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
  HISTORY_GROUP_SCOPE;
  
  b32 edit_linewise = (vim_state.params.edit_type == EDIT_LineWise);
  if(vim_state.params.selected_reg->edit_type == EDIT_LineWise ||
     edit_linewise // @Experiment(kv)
     )
  {
   seek_beginning_of_line(app);
  }
  if( vim_state.mode == VIM_Visual )
  {
   i64 cursor = view_get_cursor_pos(app, view);
   i64 mark = view_get_mark_pos(app, view);
   i64 end = mark;
   if (edit_linewise) { end = line_last_nonwhite(app, buffer, mark); }
   
   Range_i64 selected = Ii64(cursor, end);
   selected.max += 1;
   buffer_delete_range(app, buffer, selected);
   vim_state.dot_delete_count = selected.max-selected.min;
   vim_normal_mode(app);
  }
  // paste
  vim_paste_from_register(app, view, buffer, vim_state.params.selected_reg);
  vim_state.params.command = vim_paste_before;
  
  Vim_Register *prev_reg = vim_state.prev_params.selected_reg;
  vim_state.prev_params              = vim_state.params;
  vim_state.prev_params.selected_reg = prev_reg;
 }
}

// IMPORTANT(kv): the original function is broken and I'm just hacking it
function void
vim_backspace_char_inner(App_Cmd *app, i1 offset)
{
 View_ID view = get_active_view(app, Access_ReadWriteVisible);
 Vim_Register *reg = vim_state.params.selected_reg;
 if (!reg) return;
 
 if(!if_view_has_highlighted_range_delete_range(app, view))
 {
  Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
  i64 pos = view_get_cursor_pos(app, view);
  i64 buffer_size = buffer_get_size(app, buffer);
  if(in_range_exclusive(0, pos, buffer_size))
  {
   Buffer_Cursor cursor = view_compute_cursor(app, view, seek_pos(pos));
   i64 character = view_relative_character_from_pos(app, view, cursor.line, cursor.pos);
   i64 start = view_pos_from_relative_character(app, view, cursor.line, character + offset);
   u8 c = buffer_get_char(app, buffer, start);
   
   vim_register_copy(reg, SCu8(&c, 1));
   reg->edit_type = EDIT_CharWise;
   if (reg == &vim_registers.system) 
   {// NOTE(kv): always?
    clipboard_post(0, reg->data.string);
   }
   
   vim_update_registers(app);
   buffer_replace_range(app, buffer, Ii64(start, start+1), empty_string);
		}
	}
}

function void 
vim_backspace_char(App_Cmd *app){ vim_backspace_char_inner(app, -1); }

function void 
vim_delete_char(App_Cmd *app){    vim_backspace_char_inner(app, 0); }

function void
vim_toggle_macro(App_Cmd *app)
{
 if(global_keyboard_macro_is_recording){
  keyboard_macro_finish_recording(app);
 }else{
  keyboard_macro_start_recording(app);
 }
}
function void
vim_repeat_last_command(App_Cmd* app)
{
 GET_VIEW_AND_BUFFER;
 HISTORY_GROUP_SCOPE;
 b32 should_play_macro = not human_has_edited_after_macro;
 if(should_play_macro)
 {//-replay macro
  keyboard_macro_replay(app);
 }
 else
 {
  // @Hack(kv)
  i64 cursor_pos = view_get_cursor_pos(app, view);
  Range_i64 range = Ii64(cursor_pos, cursor_pos + vim_state.dot_delete_count);
  String insertion = {};
  if(vim_state.dot_do_insert){
   insertion = vim_registers.insert.data.string;
  }
  buffer_replace_range(app, buffer, range, insertion);
 }
}

function b32
vim_combine_line_inner(App_Cmd *app, View_ID view, Buffer_ID buffer, i64 line_num)
{
	if(!is_valid_line(app, buffer, line_num+1)){ return true; }
	i64 pos = get_line_end_pos(app, buffer, line_num);
	Range_i64 range = {};
	range.min = pos;

	i64 new_pos = pos + 1;
	String delimiter = (vim_state.sub_mode == SUB_G ? empty_string : strlit(" "));
	if(!line_is_valid_and_blank(app, buffer, line_num+1)){
		if(char_is_whitespace(buffer_get_char(app, buffer, new_pos))){
			new_pos = buffer_seek_character_class_change_1_0(app, buffer, &character_predicate_whitespace, Scan_Forward, new_pos);
		}
	}else{
		new_pos = get_line_end_pos(app, buffer, line_num+1);
		delimiter.size = 0;
	}
	i64 end_pos = get_line_side_pos_from_pos(app, buffer, pos, Side_Max);
	view_set_cursor_and_preferred_x(app, view, seek_pos(end_pos));
	move_right(app);
 
	range.max = new_pos;
 
	buffer_replace_range(app, buffer, range, delimiter);
 
	return false;
}

function void 
vim_combine_line(App_Cmd *app){
	View_ID view = get_active_view(app, Access_ReadWriteVisible);
	Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
	if(buffer == 0){ return; }
	i64 pos = view_get_cursor_pos(app, view);
	i64 line = buffer_compute_cursor(app, buffer, seek_pos(pos)).line;

	i1 N = vim_consume_number();
	if(vim_state.mode == VIM_Visual){
		Range_i64 range = get_view_range(app, view);
		i64 line_min = get_line_number_from_pos(app, buffer, range.min);
		i64 line_max = get_line_number_from_pos(app, buffer, range.max);
		N = Max(1, i1(line_max-line_min));
		view_set_cursor_and_preferred_x(app, view, seek_pos(range.min));
		view_set_mark(app, view, seek_pos(range.max));
		line = line_min;
	}

	History_Group history_group = history_group_begin(app, buffer);
	foreach(i,N){
		if(vim_combine_line_inner(app, view, buffer, line)){
			break;
		}
	}
	if(N > 1){ history_group_end(history_group); }
}


function void 
vim_set_mark(App_Cmd *app){
	View_ID view = get_active_view(app, Access_ReadVisible);
	Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
	i64 pos = view_get_cursor_pos(app, view);
	Scratch_Block scratch(app);
	u8 character = vim_query_user_key(app, strlit("-- SET MARK NEXT --"));
	if(in_range_exclusive('a', character, 'z'+1)){
		Managed_Scope scope = buffer_get_managed_scope(app, buffer);
		i64 *marks = (i64 *)managed_scope_get_attachment(app, scope, vim_buffer_marks, 26*sizeof(i64));
		if(marks){
			marks[character-'a'] = pos;
			vim_set_bottom_text(push_stringf(scratch, "Mark %c set", character));
		}
	}
	else if(in_range_exclusive('A', character, 'Z'+1)){
		vim_global_marks[character-'A'] = {buffer_identifier(buffer), pos};
		vim_set_bottom_text(push_stringf(scratch, "Global mark %c set", character));
	}
}

function void 
vim_goto_mark(App_Cmd *app){
	User_Input input = get_current_input(app);
	if(input.event.kind == InputEventKind_KeyStroke){
		if(input.event.key.code == Key_Code_Tick){
			vim_state.params.edit_type = EDIT_LineWise;
		}
	}
	View_ID view = get_active_view(app, Access_ReadVisible);
	Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
	u8 c = vim_query_user_key(app, strlit("-- GOTO MARK NEXT --"));
	if(in_range_exclusive('a', c, 'z'+1)){
		Managed_Scope scope = buffer_get_managed_scope(app, buffer);
		i64 *marks = (i64 *)managed_scope_get_attachment(app, scope, vim_buffer_marks, 26*sizeof(i64));
		if(marks){
			i64 pos = marks[c-'a'];
			if(pos > 0){
				vim_push_jump(app, view);
				Vim_Motion_Block vim_motion_block(app);
				view_set_cursor_and_preferred_x(app, view, seek_pos(pos));
			}else{
				Scratch_Block scratch(app);
				vim_set_bottom_text(push_stringf(scratch, "Mark %c not set", c));
			}
		}
	}
	else if(in_range_exclusive('A', c, 'Z'+1)){
		vim_push_jump(app, view);
		Vim_Global_Mark mark = vim_global_marks[c-'A'];
		if(mark.buffer_id.id){
			vim_push_jump(app, view);
			view_set_buffer(app, view, mark.buffer_id.id, 0);
			view_set_cursor_and_preferred_x(app, view, seek_pos(mark.pos));
		}else{
			Scratch_Block scratch(app);
			vim_set_bottom_text(push_stringf(scratch, "Mark %c not set", c));
		}
	}
	else{
		// TODO(BYP): Special marks
		i64 pos = -1;
		switch(c){
			case '\'':{} break;
			case '.': {} break;
			case '`': {} break;
			case '[': {} break;
			case ']': {} break;
			case '<': {} break;
			case '>': {} break;
			//case ' ': { cursor_mark_swap(app); } break;
		}
		if(pos > 0){
			;
		}
	}
}

function void 
vim_next_4coder_jump(App_Cmd *app)
{
 vim_push_jump(app, get_active_view(app, Access_ReadVisible));
 goto_next_jump(app);
}
function void
vim_prev_4coder_jump(App_Cmd *app)
{
 vim_push_jump(app, get_active_view(app, Access_ReadVisible));
 goto_prev_jump(app);
}
function void 
vim_first_4coder_jump(App_Cmd *app){
	vim_push_jump(app, get_active_view(app, Access_ReadVisible));
	goto_first_jump(app);
}

function void
vim_move_selection(App_Cmd *app, Scan_Direction direction){
	View_ID view = get_active_view(app, Access_ReadWriteVisible);
	Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);

	Scratch_Block scratch(app);
	const i64 N = vim_consume_number();
	const b32 forward = direction == Scan_Forward;

	i64 cursor_pos = view_get_cursor_pos(app, view);
	i64 mark_pos = view_get_mark_pos(app, view);
	Range_i64 range = Ii64(cursor_pos, mark_pos);
	i64 min_line = get_line_number_from_pos(app, buffer, range.min);
	i64 max_line = get_line_number_from_pos(app, buffer, range.max);
	i64 line_count = buffer_get_line_count(app, buffer);

	Range_i64 copy_range = range_union(get_line_pos_range(app, buffer, min_line),
									   get_line_pos_range(app, buffer, max_line));
	copy_range.max += buffer_get_char(app, buffer, copy_range.max) == '\r';
	copy_range.max += 1;

	i64 paste_pos = (forward ?
					 get_line_pos_range(app, buffer, Min(max_line + N, line_count)).max + 1 :
					 get_line_pos_range(app, buffer, min_line - N).min);
	i64 buff_size = buffer_get_size(app, buffer);
	paste_pos = Min(paste_pos, buff_size);

	String copy_string = push_buffer_range(app, scratch, buffer, copy_range);

	i64 cursor_offset = cursor_pos - copy_range.min;
	i64 mark_offset = mark_pos - copy_range.min;

	History_Group group = history_group_begin(app, buffer);
	if(forward){
		buffer_replace_range(app, buffer, Ii64(paste_pos), copy_string);

		view_set_cursor(app, view, seek_pos(paste_pos + cursor_offset));
		view_set_mark(app, view, seek_pos(paste_pos + mark_offset));

		buffer_replace_range(app, buffer, copy_range, empty_string);
	}else{
		buffer_replace_range(app, buffer, copy_range, empty_string);
		buffer_replace_range(app, buffer, Ii64(paste_pos), copy_string);
  
		view_set_cursor(app, view, seek_pos(paste_pos + cursor_offset));
		view_set_mark(app, view, seek_pos(paste_pos + mark_offset));
	}
 
	history_group_end(group);
}

function void 
vim_move_selection_up(App_Cmd *app)  { vim_move_selection(app, Scan_Backward); }
function void 
vim_move_selection_down(App_Cmd *app){ vim_move_selection(app, Scan_Forward); }

//-