/* NOTE(kv): This file is for miscellaneous commands */

global Table_u64_u64 shifted_version_of_characters;

VIM_COMMAND_SIG(kv_shift_character)
{
 GET_VIEW_AND_BUFFER;
 i64 pos = view_get_cursor_pos(app, view);
 
 u8 current_character = 0;
 buffer_read_range(app, buffer, Ii64(pos, pos+1), &current_character);
 
 u64 replacement_char = 0;
 if (character_is_upper(current_character))
 {
  replacement_char = character_to_lower(current_character);
 }
 else if (character_is_lower(current_character))
 {
  replacement_char = character_to_upper(current_character);
 }
 else
 {
  table_read(&shifted_version_of_characters, (u64)current_character, &replacement_char);
 }
 //
 if (replacement_char) {
  buffer_replace_range(app, buffer, Ii64(pos, pos+1), SCu8((u8 *)&replacement_char, 1));
 }
 
 move_right(app);
}

VIM_REQUEST_SIG(byp_apply_comment){
	i64 line0 = get_line_number_from_pos(app, buffer, range.min);
	i64 line1 = get_line_number_from_pos(app, buffer, range.max);
	line1 += (line0 == line1);
	HISTORY_GROUP_SCOPE;
	for(i64 l=line0; l<line1; l++)
    {
		i64 line_start = get_pos_past_lead_whitespace_from_line_number(app, buffer, l);
		b32 has_comment = c_line_comment_starts_at_position(app, buffer, line_start);
		if(!has_comment){
			buffer_replace_range(app, buffer, Ii64(line_start), strlit("//"));
			buffer_post_fade(app, buffer, 0.667f, Ii64_size(line_start,2), fcolor_resolve(fcolor_id(defcolor_paste)));
		}
	}
}

VIM_REQUEST_SIG(byp_apply_uncomment){
	i64 line0 = get_line_number_from_pos(app, buffer, range.min);
	i64 line1 = get_line_number_from_pos(app, buffer, range.max);
	line1 += (line0 == line1);
	HISTORY_GROUP_SCOPE;
	for(i64 l=line0; l<line1; l++){
		i64 line_start = get_pos_past_lead_whitespace_from_line_number(app, buffer, l);
		b32 has_comment = c_line_comment_starts_at_position(app, buffer, line_start);
		if(has_comment){
			buffer_replace_range(app, buffer, Ii64_size(line_start,2), empty_string);
		}
	}
}


inline void 
byp_make_vim_request(App *app, BYP_Vim_Request request)
{
 vim_make_request(app, Vim_Request_Type(VIM_REQUEST_COUNT + request));
}

VIM_COMMAND_SIG(byp_request_title){ byp_make_vim_request(app, BYP_REQUEST_Title); }
VIM_COMMAND_SIG(byp_request_comment) { byp_make_vim_request(app, BYP_REQUEST_Comment); }
VIM_COMMAND_SIG(byp_request_uncomment){ byp_make_vim_request(app, BYP_REQUEST_UnComment); }
VIM_COMMAND_SIG(byp_visual_comment)
{
	if(vim_state.mode == VIM_Visual){
		Vim_Edit_Type edit = vim_state.params.edit_type;
		byp_request_comment(app);
		vim_state.mode = VIM_Visual;
		vim_state.params.edit_type = edit;
	}
}
VIM_COMMAND_SIG(byp_visual_uncomment){
	if(vim_state.mode == VIM_Visual){
		Vim_Edit_Type edit = vim_state.params.edit_type;
		byp_request_uncomment(app);
		vim_state.mode = VIM_Visual;
		vim_state.params.edit_type = edit;
	}
}

VIM_COMMAND_SIG(kv_newline_above)
{
  GET_VIEW_AND_BUFFER;
  HISTORY_GROUP_SCOPE;
  vim_newline_above(app);
  vim_down(app);
  vim_normal_mode(app);
}

function void kv_newline_below(App *app)
{
    GET_VIEW_AND_BUFFER;
    HISTORY_GROUP_SCOPE;
    vim_newline_below(app);
    vim_up(app);
    vim_normal_mode(app);
}

CUSTOM_COMMAND_SIG(byp_reset_face_size)
CUSTOM_DOC("Resets face size to default")
{
  GET_VIEW_AND_BUFFER;
  Face_ID face_id = get_face_id(app, buffer);
  Face_Description description = get_face_description(app, face_id);
  description.parameters.pt_size = (i1)def_get_config_u64(app, vars_intern_lit("default_font_size"));
  try_modify_face(app, face_id, &description);
}


force_inline b32 kv_is_group_opener(Token *token) {
  return (token->kind == TokenBaseKind_ParenOpen ||
          token->kind == TokenBaseKind_ScopeOpen);
}

force_inline b32 kv_is_group_closer(Token *token) {
  return (token->kind == TokenBaseKind_ParenClose ||
          token->kind == TokenBaseKind_ScopeClose);
}

inline u8 kv_is_group_opener(u8 c)
{
  switch (c) {
    case '(':  return ')';
    case '[':  return ']';
    case '{':  return '}';
    case '\"': return '\"';
    case '\'': return '\'';
    default:   return 0;
  }
}

inline u8 kv_is_group_closer(u8 c)
{
  switch (c) {
    case ')':  return '(';
    case ']':  return '[';
    case '}':  return '{';
    case '\"': return '\"';
    case '\'': return '\'';
    default:   return 0;
  }
}

function void
kv_vim_bounce(App *app) {
 GET_VIEW_AND_BUFFER;
 Vim_Motion_Block vim_motion_block(app);
 i64 pos = view_get_cursor_pos(app, view);
 pos = vim_scan_bounce(app, buffer, pos, Scan_Forward);
 view_set_cursor_and_preferred_x(app, view, seek_pos(pos));
}

function b32
kv_find_current_nest(App *app, Buffer_ID buffer, i64 pos, Range_i64 *out){
 Token_Array tokens = get_token_array_from_buffer(app, buffer);
 if(Token *token = token_from_pos(&tokens, pos)){
  if(token->kind == TokenBaseKind_LiteralString){
   Range_i64 range = get_token_range(token);
   *out = range;
   return true;
  }else{
   u8 current_char = buffer_get_char(app, buffer, pos);
   if(kv_is_group_opener(current_char)){ pos++; }
   
   Find_Nest_Flag flags = FindNest_Scope | FindNest_Paren | FindNest_Balanced;
   Range_i64 range = {};
   if(find_nest_side(app, buffer, pos-1, flags,
                     Scan_Backward, NestDelim_Open, &range.start) &&
      find_nest_side(app, buffer, pos, flags|FindNest_EndOfToken,
                     Scan_Forward, NestDelim_Close, &range.end)){
    *out = range;
    return true;
   }else{ return false; }
  }
 }else{ return false; }
}

function void kv_sexpr_up(App *app){
 GET_VIEW_AND_BUFFER;
 vim_push_jump(app, view);
 Vim_Motion_Block vim_motion_block(app);
 
 i64 pos = view_correct_cursor(app, view);
 if(kv_is_group_opener(buffer_get_char(app, buffer, pos))){
  pos--;
 }
 
 Range_i64 range = {pos, pos};
 kv_find_current_nest(app, buffer, pos, &range);
 kv_goto_pos(app, view, range.start);
}

VIM_COMMAND_SIG(kv_sexpr_down)
{
  View_ID   view = get_active_view(app, Access_ReadVisible);
  vim_push_jump(app, view);
  Token_Iterator_Array token_it = get_token_it_at_cursor(app);
  if ( !token_it.tokens ) return;
  
  do
  {
    Token *token = tkarr_read(&token_it);
    if (kv_is_group_opener(token))
  {
   kv_goto_token(app, token);
   move_right(app);
   break;
  }
 }
 while ( tkarr_inc(&token_it) );
}

function Ed_Parser
make_ed_parser_at_cursor(App *app){
 GET_VIEW_AND_BUFFER;
 Token_Iterator_Array token_it = get_token_it_at_cursor(app);
 Ed_Parser result = make_ep_from_buffer(app, buffer, token_it);
 return result;
}

function b32
if_preprocessor_movement(App *app, Scan_Direction scan_direction){
 b32 result = false;
 GET_VIEW_AND_BUFFER;
 Token_Iterator_Array token_it = get_token_it_at_cursor(app);
 if(token_it.tokens){
  Scratch_Block scratch(app);
  Ed_Parser p_value = make_ep_from_buffer(app, buffer, token_it, 0, scan_direction);
  Ed_Parser *p = &p_value;
  i1 nest_level = 0;
  if(!ep_maybe_preprocessor(p, str8lit("else")) &&
     !ep_maybe_preprocessor(p, str8lit("elif"))){
   if(scan_direction == Scan_Forward){
    p->set_ok(ep_maybe_preprocessor(p, str8lit("if")) ||
              ep_maybe_preprocessor(p, str8lit("ifdef")) ||
              ep_maybe_preprocessor(p, str8lit("ifndef")));
   }else{
    ep_eat_preprocessor(p, str8lit("endif"));
   }
  }
  while(p->ok_){
   b32 is_ifs = (ep_test_preprocessor(p, str8lit("if")) ||
                 ep_test_preprocessor(p, str8lit("ifdef")) ||
                 ep_test_preprocessor(p, str8lit("ifndef")));
   
   b32 is_endif = ep_test_preprocessor(p, str8lit("endif"));
   
   b32 is_el  = (ep_test_preprocessor(p, str8lit("else")) ||
                 ep_test_preprocessor(p, str8lit("elif")));
   
   if(is_ifs){
    nest_level += scan_direction;
   }
   if(is_endif){
    nest_level -= scan_direction;
   }
   
   if(is_el && nest_level == 0){
    break;
   }
   if((is_ifs || is_endif) &&  (nest_level == -1)){
    break;
   }
   
   ep_eat_token(p);
  }
  if(p->ok_){
   kv_goto_token(app, ep_get_token(p));
   result = true;
  }
 }
 return result;
}

function void 
kv_sexpr_right(App *app) {
 Token_Iterator_Array token_it = get_token_it_at_cursor(app);
 if ( token_it.tokens ) {
  View_ID view = get_active_view(app, Access_ReadVisible);
  vim_push_jump(app, view);
  if(!if_preprocessor_movement(app, Scan_Forward)){
   i32 nest = 0;
   do{
    Token *token = tkarr_read(&token_it);
    if(token->kind == TokenBaseKind_LiteralString)
    {// NOTE: goto end of string
     if(nest == 0){
      i64 token_end = get_token_range(token).end;
      kv_goto_pos(app, view, token_end);
      break;
     }
    }else if(kv_is_group_opener(token)){
     nest += 1;
    }else if(kv_is_group_closer(token)){
     nest -= 1;
     if(nest <= 0){
      kv_goto_token(app, token);
      if(nest == 0){ move_right(app); }
      break;
     }
    }
   }while(tkarr_inc(&token_it));
  }
 }
}

function void
kv_sexpr_left(App *app)
{
 if ( if_preprocessor_movement(app, Scan_Backward) == 0 )
 {
  Token_Iterator_Array token_it = get_token_it_at_cursor(app, -1);
  Token *token = tkarr_read(&token_it);
  if (token)
  {
   View_ID view = get_active_view(app, Access_ReadVisible);
   vim_push_jump(app, view);
   i32 nest = 0;
   do
   {
    token = tkarr_read(&token_it);
    if (token->kind == TokenBaseKind_LiteralString)
    {
     if (nest == 0)
     {
      kv_goto_token(app, token);
      break;
     }
    }
    else if (kv_is_group_opener(token))
    {
     nest -= 1;
     if (nest <= 0)
     {
      kv_goto_token(app, token);
      if (nest < 0) { move_right(app); }
      break;
     }
    }
    else if (kv_is_group_closer(token))
    {
     nest += 1;
    }
   } while ( tkarr_dec(&token_it) );
  }
 }
}

VIM_COMMAND_SIG(kv_sexpr_end)
{
 kv_sexpr_up(app);
 kv_sexpr_right(app);
 move_left(app);
 move_left(app);
}

function Range_i64
view_get_selected_range(App *app, View_ID view){
 Buffer_ID buffer = view_get_buffer(app,view,0);
 i64 begin = view_get_cursor_pos(app, view);
 i64 end   = view_get_mark_pos  (app, view);
 swap_minmax(begin,end);
 
 if(vim_is_editing_linewise()){
  begin = get_line_start_pos_from_pos(app, buffer, begin);
  end   = line_last_nonwhite         (app, buffer, end);
 }
 
 Range_i64 range = Ii64(begin, end);
 range.max += 1;
 return range;
}

inline void 
buffer_delete_pos(App *app, Buffer_ID buffer, i64 min)
{
 buffer_replace_range(app, buffer, Ii64(min, min+1), empty_string);
}

inline void
buffer_insert_pos(App *app, Buffer_ID buffer, i64 pos, String string)
{
 buffer_replace_range(app, buffer, Ii64(pos), string);
}

function void 
kv_surround_with(App *app, char *opener, char *closer)
{
 GET_VIEW_AND_BUFFER;
 HISTORY_GROUP_SCOPE;
 
 Range_i64 selected = view_get_selected_range(app, view);
 buffer_insert_pos(app, buffer, selected.max, SCu8(closer));
 buffer_insert_pos(app, buffer, selected.min, SCu8(opener));
 
 vim_normal_mode(app);
}

function void
kv_surround_brace_special(App *app)
{
    GET_VIEW_AND_BUFFER;
    HISTORY_GROUP_SCOPE;
    
    Range_i64 range;
    if (vim_state.mode == VIM_Visual)
    {
        range = view_get_selected_range(app, view);
    }
    else
    {
        range = Ii64(view_get_cursor_pos(app, view));
    }
    
    range.min = get_line_start_pos_from_pos(app, buffer, range.min);
    range.max = get_line_end_pos_from_pos  (app, buffer, range.max);
 buffer_insert_pos(app, buffer, range.max, /*{*/str8lit("\n}"));
 buffer_insert_pos(app, buffer, range.min, str8lit("{\n")/*}*/);
 
 auto_indent_buffer(app, buffer, Ii64(range.min, range.max+4));
 
 if (vim_state.mode == VIM_Visual)
  vim_normal_mode(app);
}

CUSTOM_COMMAND_SIG(kv_reopen_with_confirmation)
CUSTOM_DOC("Like reopen, but asks for confirmation")
{
 if (get_confirmation_from_user(app, strlit("Actually revert?")))
 {
  View_ID view = get_active_view(app, Access_ReadVisible);
  Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
  buffer_reopen(app, buffer, 0);
 }
}

VIM_COMMAND_SIG(kv_surround_paren)          {kv_surround_with(app, "(", ")");}
VIM_COMMAND_SIG(kv_surround_paren_spaced)   {kv_surround_with(app, "( ", " )");}
VIM_COMMAND_SIG(kv_surround_brace)          {kv_surround_with(app, "{", "}");}
VIM_COMMAND_SIG(kv_surround_brace_spaced)   {kv_surround_with(app, "{ ", " }");}
VIM_COMMAND_SIG(kv_surround_double_quote)   {kv_surround_with(app, "\"", "\"");}

function void
cmd_closing_bracket_in_visual_mode(App *app)
{
    GET_VIEW_AND_BUFFER;
    b32 decide_to_move_the_cursor = false;
    b32 visual_line = (vim_state.params.edit_type == EDIT_LineWise);
    if (visual_line)
    {
        decide_to_move_the_cursor = true;
    }
#if 0
    else
    {
        Range_i64 selected = get_selected_range(app);
        i64 line0 = get_line_number_from_pos(app, buffer, selected.min);
        i64 line1 = get_line_number_from_pos(app, buffer, selected.max);;
        if (line0 != line1)
        {
            decide_to_move_the_cursor = true;
        }
    }
#endif
    
    if (decide_to_move_the_cursor)
    {
        vim_paragraph_down(app);
 }
 else
 {
  kv_surround_with(app, "[", "]");
 }
}

VIM_COMMAND_SIG(kv_void_command) { return; }

VIM_COMMAND_SIG(kv_vim_normal_mode) {
 vim_normal_mode(app);
 arrsetlen(kv_quail_keystroke_buffer, 0);
}
function void
vim_enter_visual_mode(){
 vim_state.mode = VIM_Visual;
 vim_state.params.edit_type = EDIT_CharWise;
}
VIM_COMMAND_SIG(kv_sexpr_select_whole){
 GET_VIEW_AND_BUFFER;
 
 i64 pos = view_get_cursor_pos(app, view);
 Range_i64 nest = {};
 if(kv_find_current_nest(app, buffer, pos, &nest)){
  view_set_cursor_and_preferred_x(app, view, seek_pos(nest.min));
  view_set_mark(app, view, seek_pos(nest.max-1));
  vim_enter_visual_mode();
 }
}

inline b32
character_is_path(char character) {
 switch (character) {
  case '/': case '~': case '.': case '\\':  case '-': case ':':
  return true;
  default:
  return character_is_alnum(character);
 }
}

function void 
yank_current_filename(App *app)
{
 GET_VIEW_AND_BUFFER;
 Scratch_Block temp(app);
 String8 filename = push_buffer_filename(app, temp, buffer);
 if (filename.size)
 {
  string_mod_replace_character(filename, '\\', '/');
  clipboard_post(0, filename);
 }
}

function void 
open_file_from_current_dir(App *app)
{
    GET_VIEW_AND_BUFFER;
    Scratch_Block temp(app);
    String8 dirname = push_buffer_dirname(app, temp, buffer);
    set_hot_directory(app, dirname);
 vim_interactive_open_or_new(app);
}

function void 
kv_handle_g_f(App *app)
{
 open_file_from_current_dir(app);
}


global const String KV_FILE_FILENAME = str8lit("~/notes/file.skm");

function b32
F4_GoToDefinition(App *app, F4_Index_Note *note, b32 same_panel)
{
 b32 result = false;
 if(note != 0 && note->file != 0)
 {
  View_ID view = get_active_view(app, Access_Always);
  Rect_f32 region = view_get_buffer_region(app, view);
  f32 view_height = rect_height(region);
  Buffer_ID buffer = note->file->buffer;
  if(!same_panel)
  {
   view = get_other_primary_view(app, view, Access_Always, true);
  }
  point_stack_push_view_cursor(app, view);
  view_set_buffer(app, view, buffer, 0);
  i64 line_number = get_line_number_from_pos(app, buffer, note->range.min);
  Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
  scroll.position.line_number = line_number;
  scroll.target.line_number = line_number;
  scroll.position.pixel_shift.y = scroll.target.pixel_shift.y = -view_height*0.5f;
  view_set_buffer_scroll(app, view, scroll, SetBufferScroll_SnapCursorIntoView);
  view_set_cursor(app, view, seek_pos(note->range.min));
  view_set_mark(app, view, seek_pos(note->range.min));
  result = true;
 }
 return result;
}

function F4_Index_Note *
F4_FindMostIntuitiveNoteInDuplicateChain(F4_Index_Note *note, Buffer_ID cursor_buffer, i64 cursor_pos)
{//NOTE(kv) You can keep cycling through different definitions, which is awesome!
 F4_Index_Note *result = note;
 if(note != 0) {
  F4_Index_Note *best_note_based_on_cursor = 0;
  for(F4_Index_Note *candidate = note; candidate; candidate = candidate->next) {
   F4_Index_File *file = candidate->file;
   if(file != 0) {
    if(cursor_buffer == file->buffer &&
       candidate->range.min <= cursor_pos &&
       cursor_pos <= candidate->range.max) {
     if(candidate->next) {
      best_note_based_on_cursor = candidate->next;
      break;
     } else {
      best_note_based_on_cursor = note;
      break;
     }
    }
   }
  }
  
  if(best_note_based_on_cursor) {
   result = best_note_based_on_cursor;
  } else if(note->flags & F4_Index_NoteFlag_Prototype) {
   for(F4_Index_Note *candidate = note; candidate; candidate = candidate->next) {
    if(!(candidate->flags & F4_Index_NoteFlag_Prototype)) {
     result = candidate;
     break;
    }
   }
  }
 }
 return result;
}

// CUSTOM_DOC("Goes to the definition of the identifier under the cursor.")
function b32
f4_goto_definition(App *app)
{
 View_ID view = get_active_view(app, Access_Always);
 Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
 Scratch_Block scratch(app);
 String string = push_token_or_word_under_active_cursor(app, scratch);
 F4_Index_Note *note = F4_Index_LookupNote(string);
 note = F4_FindMostIntuitiveNoteInDuplicateChain(note, buffer, view_get_cursor_pos(app, view));
 b32 jumped = F4_GoToDefinition(app, note, true);
 return jumped;
}

function b32
character_is_tag(char c)
{
 return (c == '@' || character_is_alnum(c));
}

function b32
goto_comment_identifier(App *app)
{
 b32 jumped = false;
 String_Match match = {};
 {
  Scratch_Block scratch(app);
  String tag;
  {
   GET_VIEW_AND_BUFFER;
   Range_i64 range = get_surrounding_characters(app, character_is_tag);
   tag = push_buffer_range(app, scratch, buffer, range);
  }
  if (tag.size >= 2)
  {
   String identifier = tag;
   if( starts_with(tag, strlit("@")) )
   {
    identifier = string_skip(tag, 1);
   }
   
   String_u8 needle = string_u8_push(scratch, identifier.size+1);
   string_concat_character(&needle, ';');
   string_concat(&needle, identifier);
   for (Buffer_ID buffer = get_buffer_next(app, 0, Access_Always);
        buffer != 0;
        buffer = get_buffer_next(app, buffer, Access_Always))
   {
    match = buffer_seek_string(app, buffer, needle.string, Scan_Forward, /*start pos*/0, /*case sensitive*/true);
    if (match.buffer)  {  break;  }
   }
   
   if (match.buffer)
   {
    View_ID view = get_active_view(app, Access_Always);
    view_set_buffer(app, view, match.buffer, 0);
    view_set_cursor(app, view, seek_pos(match.range.min));
    jumped = true;
   }
  }
 }
 
 return jumped;
}

function void
kv_jump_ultimate(App *app)
{
 GET_VIEW_AND_BUFFER;
 Scratch_Block scratch(app);
 vim_push_jump(app, view);
 b32 jumped = false;
 {//NOTE(kv) File path and/or line+column
  Range_i64 loc_range = get_surrounding_characters(app, character_is_path);
  if (loc_range.max > 0) {
   String loc = push_buffer_range(app, scratch, buffer, loc_range);
   if (view_open_file(app, view, loc, true)){
    //NOTE(kv) The parse_jump doesn't support file-only path
    jumped = true;
   }else{
    Parsed_Jump jump = parse_jump_location(loc);
    if (jump.success){
     jump_to_location(app, view, jump.location);
     jumped = true;
    }
   }
  }
 }
 
 jumped = jumped || f4_goto_definition(app);
 jumped = jumped || goto_comment_identifier(app);
 
 if (!jumped)
 {// NOTE(kv): go to the "file" file
  // yank_current_filename_(app);  // In case we wanna paste add the current filename in
  set_buffer_named(app, KV_FILE_FILENAME);
 }
}

function void kv_jump_ultimate_other_panel(App *app) {
 view_buffer_other_panel(app);
 kv_jump_ultimate(app);
}

VIM_COMMAND_SIG(kv_delete_surrounding_groupers){
 GET_VIEW_AND_BUFFER;
 HISTORY_GROUP_SCOPE;
 i64 pos = view_get_cursor_pos(app, view);
 Range_i64 range = {};
 if(kv_find_current_nest(app, buffer, pos, &range)){
  buffer_delete_pos(app, buffer, range.max-1);
  buffer_delete_pos(app, buffer, range.min);
  auto_indent_buffer(app, buffer, range);
 }
}

function void 
kv_do_t_function(App *app, b32 shiftp)
{
 GET_VIEW_AND_BUFFER;
 HISTORY_GROUP_SCOPE;
 
 i64 pos = view_get_cursor_pos(app, view);
 u8 current_char = buffer_get_char(app, buffer, pos);
 
 // 1. optionally delete space
 if (current_char == ' ') {
  buffer_delete_pos(app, buffer, pos);
  current_char = buffer_get_char(app, buffer, pos);
 } else if (current_char == '_') {
  pos++;
  current_char = buffer_get_char(app, buffer, pos);
 }
 
 if ( character_is_alpha(current_char) ) {
  // 2. upcase character/word
  Scratch_Block temp(app);
  i64 max = 0;
  String replacement = {};
  i64 alpha_max = scan_any_boundary(app, boundary_alnum, buffer, Scan_Forward, pos);
  if (shiftp) {
   max = alpha_max;
   Range_i64 range = {pos, alpha_max};
   replacement = push_buffer_range(app, temp, buffer, range);
   string_mod_upper(replacement);
  } else {
   max = pos+1;
   u8 upper = character_to_upper(current_char);
   replacement = push_string_const_u8(temp, 1);
   replacement.str[0] = upper;
  }
  buffer_replace_range(app, buffer, Ii64(pos,max), replacement);
  
  // 3. move
  view_set_cursor_and_preferred_x(app, view, seek_pos(alpha_max));
 } else {
  move_right(app);
 }
}
VIM_COMMAND_SIG(kv_do_t) {kv_do_t_function(app, false);}
VIM_COMMAND_SIG(kv_do_T) {kv_do_t_function(app, true);}

function void 
kv_do_underscore_function(App *app, b32 shiftp)
{
 GET_VIEW_AND_BUFFER;
 HISTORY_GROUP_SCOPE;
 
 i64 pos = view_get_cursor_pos(app, view);
 u8 current_char = buffer_get_char(app, buffer, pos);
 
 // 1. Replace space with underscore
 if (current_char == ' '){
  buffer_replace_range(app, buffer, Ii64(pos, pos+1), strlit("_"));
  pos++;
  current_char = buffer_get_char(app, buffer, pos);
 } else if (current_char == '_'){
  pos++;
  current_char = buffer_get_char(app, buffer, pos);
 }
 
 if (character_is_alpha(current_char)){
  // 2. Upcase character/word
  Scratch_Block temp(app);
  i64 max = 0;
  i64 alpha_max = scan_any_boundary(app, boundary_alnum, buffer, Scan_Forward, pos);
  if (shiftp) {
   max = pos+1;
   u8 upper = character_to_upper(current_char);
   String replacement = push_string_const_u8(temp, 1);
   replacement.str[0] = upper;
   buffer_replace_range(app, buffer, Ii64(pos,max), replacement);
  }
  
  // 3. move
  view_set_cursor_and_preferred_x(app, view, seek_pos(alpha_max));
 } else {
  move_right(app);
 }
}
VIM_COMMAND_SIG(kv_do_underscore)         {kv_do_underscore_function(app, false);}
VIM_COMMAND_SIG(kv_do_underscore_shifted) {kv_do_underscore_function(app, true);}



CUSTOM_COMMAND_SIG(kv_run)
CUSTOM_DOC("run the current script")
{
  GET_VIEW_AND_BUFFER;
  Scratch_Block temp(app);

  String8 dir = push_hot_directory(app, temp);
  String8 cmd = push_buffer_filename(app, temp, buffer);
  standard_build_exec_command(app, view, dir, cmd);
}

CUSTOM_COMMAND_SIG(kv_open_note_file)
CUSTOM_DOC("switch to my note file")
{
 set_buffer_named(app, strlit("~/notes/note.skm"));
}

CUSTOM_COMMAND_SIG(switch_to_game_panel)
CUSTOM_DOC("switch to game panel")
{
 set_buffer_named(app, GAME_BUFFER_NAMES[0]);
}

CUSTOM_COMMAND_SIG(file)
CUSTOM_DOC("kv copy file name")
{
 yank_current_filename(app);
}

CUSTOM_COMMAND_SIG(dir)
CUSTOM_DOC("kv copy dir name")
{
  GET_VIEW_AND_BUFFER;
  Scratch_Block temp(app);
  String8 dirname = push_buffer_dirname(app, temp, buffer);
  clipboard_post(0, dirname);
}

function void 
kv_vim_visual_line_mode(App *app)
{
 if (vim_state.mode != VIM_Visual)
 {
  set_mark(app);
 }
 vim_state.mode = VIM_Visual;
 vim_state.params.edit_type = EDIT_LineWise;
}

function void
kv_list_all_locations_from_string(App *app, String needle)
{
 Scratch_Block temp(app);
 
 String_Match_List all_matches = {};
 for (Buffer_ID buffer = get_buffer_next(app, 0, Access_Always);
      buffer != 0;
      buffer = get_buffer_next(app, buffer, Access_Always))
 {
  String_Match_List buffer_matches = {};
  {
   i64 pos = 0;
   while (true)
   {
    i64 original_pos = pos;
    pos = kv_fuzzy_search_forward(app, buffer, pos, needle).min;
    if (pos)
    {
     // note(kv): just a dummy range, not sure if it's even used
     Range_i64 range2 = {pos, pos+1};
     string_match_list_push(temp, &buffer_matches, buffer, 0, 0, range2);
    }
    else { break; }
    assert_defend(pos > original_pos, break;);
   }
  }
  all_matches = string_match_list_join(&all_matches, &buffer_matches);
 }
 
 //
 Buffer_ID out_buffer = maybe_create_buffer_and_clear_by_name(app, search_buffer_name, global_bottom_view);
 kv_filter_match_list(app, &all_matches, out_buffer);
 print_string_match_list_to_buffer(app, out_buffer, all_matches);
 
 lock_jump_buffer(app, out_buffer);
}

function u8 
kv_get_current_char(App *app)
{
 GET_VIEW_AND_BUFFER;
 i64 pos = view_get_cursor_pos(app, view);
 return buffer_get_char(app, buffer, pos);
}

// CUSTOM_DOC("adapted from list_all_locations for fuzzy search, if cursor at identifier then search for that instead")
function void 
kv_list_all_locations(App *app)
{
 b32 at_identifier = false;
 b32 is_visual = (vim_state.mode == VIM_Visual);
 
 if (!is_visual)
 {
  if ( character_is_alnum(kv_get_current_char(app)) )
  {
   at_identifier = true;
   list_all_locations_of_identifier(app);
  }
 }
 
 if ( !at_identifier )
 {
  String needle_str = {};
  Scratch_Block temp(app);
  if (is_visual)
  {// note select range
   needle_str = get_selected_string(app, temp);
   vim_normal_mode(app);
  }
  else
  {// note prompt
   u8 *space = push_array(temp, u8, KB(1));
   needle_str = get_query_string(app, "List Locations For: ", space, KB(1));
  }
  
  if (needle_str.size)
  {
   kv_list_all_locations_from_string(app, needle_str); 
  }
 }
}

VIM_COMMAND_SIG(open_build_script)
{
  GET_VIEW_AND_BUFFER;
  Scratch_Block scratch(app);
  String8 dirname = push_buffer_dirname(app, scratch, buffer);
  String8 build_file = kv_search_build_file_from_dir(scratch, dirname);
  if (build_file.size)
  {
    view_open_file(app, view, build_file, true);
  }
}

VIM_COMMAND_SIG(vim_select_all)
{
  vim_visual_char_mode(app);
  select_all(app);
}


CUSTOM_COMMAND_SIG(kv_miscellaneous_debug_command)
CUSTOM_DOC("just a placeholder command so I can test stuff")
{
}

CUSTOM_COMMAND_SIG(init)
CUSTOM_DOC("configure your editor!")
{
    set_buffer_named(app, strlit("~/4ed/code/4coder_kv/4coder_kv.cpp"));
}

// todo: We don't wanna bind to a buffer, maybe?
function void kv_system_command(App *app, String8 cmd)
{
 GET_VIEW_AND_BUFFER;
 Scratch_Block temp(app);
 String8 dir = push_buffer_dirname(app, temp, buffer);
 exec_system_command(app, global_bottom_view, standard_build_compilation_buffer_identifier,
                     dir, cmd, standard_build_exec_flags);
}

VIM_COMMAND_SIG(remedy_add_breakpoint)
{
 GET_VIEW_AND_BUFFER;
 Scratch_Block temp(app);
 String8 filename = push_buffer_filename(app, temp, buffer);
 i64 linum = get_current_line_number(app);
 String8 cmd = push_stringfz(temp, "remedybg.exe add-breakpoint-at-file %.*s %lld",
                             string_expand(filename), linum);
 kv_system_command(app, cmd);
}

VIM_COMMAND_SIG(remedy_start_debugging)
{
    String8 cmd = str8_lit("remedybg.exe start-debugging");
    kv_system_command(app, cmd);
}

VIM_COMMAND_SIG(remedy_stop_debugging)
{
    String8 cmd = str8_lit("remedybg.exe stop-debugging");
    kv_system_command(app, cmd);
}

VIM_COMMAND_SIG(remedy_run_to_cursor)
{
    GET_VIEW_AND_BUFFER;
    Scratch_Block temp(app);
    String8 filename = push_buffer_filename(app, temp, buffer);
    i64 linum = get_current_line_number(app);
    String8 cmd = push_stringfz(temp, "remedybg.exe run-to-cursor %.*s %lld",
                               string_expand(filename), linum);
    kv_system_command(app, cmd);
}

function void
clipboard_pop_command(App *app)
{
    clipboard_pop(app, 0);
    Scratch_Block scratch(app);
    String8 current_item = push_clipboard_index(app, scratch, 0, 0);
   
    // NOTE(kv): hack to make vim paste this thing (don't understand it)
    // Managed_Scope scope = view_get_managed_scope(app, view);
    // i1 *paste_index_ptr = scope_attachment(app, scope, view_paste_index_loc, i1);
    // *paste_index_ptr = 0;
   
    // NOTE(kv): print it
    vim_set_bottom_text(current_item);
}

function void
view_goto_first_search_position(App *app, View_ID view, String8 needle)
{
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    i64 pos = kv_fuzzy_search_forward(app, buffer, 0, needle).min;
    view_goto_pos(app, view, pos);
}


function void 
close_panel(App *app)
{
    View_ID view = get_active_view(app, Access_Always);
    change_active_primary_view(app);
    toggle_split_panel(app);
}

CUSTOM_COMMAND_SIG(set_current_dir_as_hot)
CUSTOM_DOC("set current dir as hot")
{
    GET_VIEW_AND_BUFFER;
    Scratch_Block scratch(app);
    String8 dirname = push_buffer_dirname(app, scratch, buffer);
    set_hot_directory(app, dirname);
}

CUSTOM_COMMAND_SIG(scratch)
CUSTOM_DOC("switch to scratch buffer")
{
    set_buffer_named(app, strlit("~/notes/scratch.skm"));
}

CUSTOM_COMMAND_SIG(messages)
CUSTOM_DOC("switch to messages buffer")
{
 set_buffer_named(app, strlit("*messages*"));
}

function void
quick_align_command(App *app)
{
 GET_VIEW_AND_BUFFER;
 HISTORY_GROUP_SCOPE;
 Range_i64 the_range = view_get_selected_range(app, view);
    Scratch_Block scratch(app);
    the_range.min = get_line_start_pos_from_pos(app, buffer, the_range.min);
    the_range.max = get_line_end_pos_from_pos(app, buffer, the_range.max);
    String selected = push_buffer_range(app, scratch, buffer, the_range);
    // NOTE: Figure out the lines
    struct Line
    {
        i64 start;
        i64 equal_sign_pos;
    };
    Line *lines = 0;
    {
        i64 pos = 0;
        while (pos < range_size(the_range))
        {
            Line line = {};
            line.start = pos;
            line.equal_sign_pos = -1;
            while (pos < range_size(the_range))
            {
                u8 chr = selected.str[pos];
                if (chr == '=' && 
                    line.equal_sign_pos == -1)
                {
                    line.equal_sign_pos = pos - line.start;
                }
                pos++;
                
                if (chr == '\n')  break; 
            }
            arrpush(lines, line);
        }
    }
    
    i64 rightmost_equal_sign = 0;
    // NOTE: find the right-most equal sign
    for_i64 (index, 0, arrlen(lines))
    {
        macro_clamp_min(rightmost_equal_sign, lines[index].equal_sign_pos);
    }
    // NOTE: Then go back and fix up our lines from end to beginning
    u8 space_buffer[256];
    block_fill_u8(space_buffer, 256, ' ');
    for (i64 index=arrlen(lines)-1;
         index >= 0;
         index--)
    {
        Line *line = lines+index;
        i64 nspaces = rightmost_equal_sign - line->equal_sign_pos;
        if (line->equal_sign_pos >= 0 && 
            nspaces > 0)
        {
            String spaces = { space_buffer, (u64)clamp_max(nspaces,256) };
            i64 pos = the_range.start + line->start + line->equal_sign_pos;
   buffer_replace_range(app, buffer, Ii64(pos,pos), spaces);
  }
 }
}

function b32
maybe_handle_fui(App *app, Buffer_ID buffer)
{
 b32 result = false;
 if (game_on_ro)
 {
  // NOTE: When the tick doesn't run, we don't load the game code.
  // so we update the game code here so that it doesn't reach in the wrong part slider.
  // TODO: I still the handling is very wonky, idk what else to do.
  load_latest_game_code(app, 0);
  
  Game_API *game = get_game_code();
  if ( game )
  {
   global_game_dll_lock = true;
   Scratch_Block scratch(app);
   String filename = push_buffer_filename(app, scratch, buffer);
   i32 line_number = (i32)get_current_line_number(app);
   result = game->fui_handle_slider(app, buffer, filename, line_number);
   global_game_dll_lock = false;
  }
 }
 return result;
}

function void
kv_handle_return_normal_mode(App *app)
{
 View_ID view = get_active_view(app, Access_ReadVisible);
 Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
 if (buffer) { // Writable buffer
  if ( maybe_handle_fui(app, buffer) ) { // pass
  } else {
   save_all_dirty_buffers(app);
  }
 } else { // Readonly buffer
  buffer = view_get_buffer(app, view, Access_ReadVisible);
  if (buffer) {
   vim_push_jump(app, get_active_view(app, Access_ReadVisible));
   goto_jump_at_cursor(app);
   lock_jump_buffer(app, buffer);
  }
 }
}

function void
cmd_insert_ampersand(App *app) {
 write_text(app, strlit("&"), false);
}
function void
cmd_insert_asterisk(App *app) {
 write_text(app, strlit("*"), false);
}

#if 0
jump are_we_in_debug_build();
#endif
//
CUSTOM_COMMAND_SIG(are_we_in_debug_build)
CUSTOM_DOC("")
{
#if KV_INTERNAL
 vim_set_bottom_text_lit("yes, we are in DEBUG build!");
#else
 vim_set_bottom_text_lit("no, we are in RELEASE build!");
#endif
}

function void
handle_tab_normal_mode(App *app)
{
 GET_VIEW_AND_BUFFER;
 
 b32 try_indent = false;
 b32 is_limited = is_buffer_limited_edit(app, buffer);
 if (!is_limited)
 {
  try_indent = auto_indent_line_at_cursor(app);
 }
 
 if ( !try_indent )
 {
  change_active_primary_view(app);
 }
}

function void
handle_space_command(App *app)
{
#if 0
 if (game_on_ro)
 {
  i1 viewport_id = get_active_game_viewport_id(app);
  global_game_code.game_last_preset(ed_game_state_pointer, viewport_id);
 }
 //else if ( get_active_game_viewport_id(app) ) { turn_game_on(); }
 else
#endif
  write_space_command(app);
}

function void
kv_handle_left_click(App *app)
{
 click_set_cursor_and_mark(app);
 switch_to_mouse_panel(app);
}

CUSTOM_COMMAND_SIG(replace_in_all_buffers)
CUSTOM_DOC("Replace (in all editable buffers)")
{
 global_history_edit_group_begin(app);
 Scratch_Block scratch(app);
 
 b32 ok = true;
 Query_Bar_Group group(app);
 String_Pair pair;
 b32 is_visual = (vim_state.mode == VIM_Visual);
 if (is_visual)
 {
  pair.a = get_selected_string(app, scratch);
  Query_Bar query = make_query_bar(scratch, "Replace selected with: ");
  ok = query_user_string(app, &query);
  pair.b = query.string;
  if (ok) { pair.valid = true; }
 }
 else
 {
  pair = query_user_replace_pair(app, scratch);
  if (!pair.valid) { ok = false; }
 }
 
 if (ok)
 {
  for (Buffer_ID buffer = get_buffer_next(app, 0, Access_ReadWriteVisible);
       buffer != 0;
       buffer = get_buffer_next(app, buffer, Access_ReadWriteVisible))
  {
   Range_i64 range = buffer_range(app, buffer);
   replace_in_range(app, buffer, range, pair.a, pair.b);
  }
 }
 
 if(is_visual){
  vim_normal_mode(app);
 }
 global_history_edit_group_end(app);
}

function void
kv_toggle_cpp_comment(App *app) {
 if (vim_state.mode == VIM_Visual) {
  kv_surround_with(app, "/*", "*/");
 }else{
  Ed_Parser p_value = make_ed_parser_at_cursor(app);
  Ed_Parser *p = &p_value;
  Token *token = ep_get_token(p);
  if (token){
   if (token->kind == TokenBaseKind_Comment){
    Scratch_Block scratch;
    GET_VIEW_AND_BUFFER;
    String token_string = ep_print_token(scratch, p);
    Range_i64 token_range = get_token_range(token);
    if (token_string.str[0] == '/' &&
        token_string.str[1] == '*')
    {// NOTE(kv): We're in cpp comment -> Delete
     HISTORY_GROUP_SCOPE;
     buffer_delete_range(app, buffer, Ii64(token_range.max-2, token_range.max));
     buffer_delete_range(app, buffer, Ii64(token_range.min,   token_range.min+2));
    }
    else if (token_string.str[0] == '/' &&
             token_string.str[0] == '/')
    {
     buffer_delete_range(app, buffer, Ii64(token_range.min, token_range.min+2));
    }
    else { /*should never reach here*/ }
   }
  }
 }
}
function void
move_parameter_left_or_right(App* app, b32 move_rightp){
 Scratch_Block scratch(app);
 GET_VIEW_AND_BUFFER;
 i64 curpos = view_get_cursor_pos(app, view);
 Range_i64 nest;
 if(kv_find_current_nest(app, buffer, curpos, &nest)){
  Token_Iterator_Array token_it = get_token_it_at_pos(app, buffer, nest.min);
  Ed_Parser parser = make_ep_from_buffer(app, buffer, token_it);
  Ed_Parser *p = &parser;
  arrayof<Range_i64> list; init_dynamic(list, scratch);
  push_buffer_range(app, scratch, buffer, nest);
  ep_eat_token(p);  //NOTE group opener
  Range_i64 sentinel_range = {};
  Range_i64 *current_range = &sentinel_range;
  while(p->ok_){
   if(Token *token = ep_get_token(p)){
    if(token->pos >= nest.max){
     break;
    }else{
     Range_i64 &item = list.push_zero();
     item.min = token->pos;
     ep_eat_until_char(p, strlit(",)]}"));
     if(Token *end_token = ep_get_token(p)){
      item.max = end_token->pos;
     }
     ep_eat_token(p);
    }
   }else{ break; }
  }
  if(p->ok_){
   if(list.count){
    i1 curindex = -1;
    for_i32(i,0,list.count){
     if(curpos >= list[i].min &&
        curpos <  list[i].max){
      curindex = i;
      break;
     }
    }
    kv_assert(curindex != -1);
    if(move_rightp){
     //NOTE Move right
     if(curindex < list.count-1){
      macro_swap(list[curindex], list[curindex+1]);
      i64 new_curpos;
      String replacement;
      {
       Printer pr = make_printer_buffer(scratch, nest.max-nest.min);
       for_i1(i,0,list.count){
        if(i){ pr<", "; }
        if(i == curindex+1){
         new_curpos = nest.min+1+pr.used;
        }
        pr<push_buffer_range(app, scratch, buffer, list[i]);
       }
       replacement = printer_get_string(pr);
      }
      buffer_replace_range(app, buffer, Ii64(nest.min+1, nest.max-1), replacement);
      view_set_cursor_pos(app, view, new_curpos);
     }
    }else{
     //NOTE Move left
     if(curindex > 0){
      macro_swap(list[curindex], list[curindex-1]);
      i64 new_curpos;
      String replacement;
      {
       Printer pr = make_printer_buffer(scratch, nest.max-nest.min);
       for_i1(i,0,list.count){
        if(i){ pr<", "; }
        if(i == curindex-1){
         new_curpos = nest.min+1+pr.used;
        }
        pr<push_buffer_range(app, scratch, buffer, list[i]);
       }
       replacement = printer_get_string(pr);
      }
      buffer_replace_range(app, buffer, Ii64(nest.min+1, nest.max-1), replacement);
      view_set_cursor_pos(app, view, new_curpos);
     }
    }
   }
  }
 }
}
function void
move_parameter_right(App* app){ move_parameter_left_or_right(app, 1); }
function void
move_parameter_left (App* app){ move_parameter_left_or_right(app, 0); }

//~EOF
