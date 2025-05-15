/* NOTE(kv): This file is for miscellaneous commands */
// see also 4coder_kv.cpp

global Table_u64_u64 shifted_version_of_characters;

function void 
kv_shift_character(App_Cmd *app)
{
 GET_VIEW_AND_BUFFER;
 i64 pos = view_get_cursor_pos(app, view);
 
 u8 current_character = 0;
 buffer_read_range(app, buffer, Ii64(pos, pos+1), &current_character);
 
 u64 replacement_char = 0;
 if (character_is_upper(current_character)) {
  replacement_char = character_to_lower(current_character);
 } else if (character_is_lower(current_character)) {
  replacement_char = character_to_upper(current_character);
 } else {
  table_read(&shifted_version_of_characters, (u64)current_character, &replacement_char);
 }
 //
 if (replacement_char) {
  buffer_replace_range(app, buffer, Ii64(pos, pos+1), SCu8((u8 *)&replacement_char, 1));
 }
 
 move_right(app);
}

function void 
byp_apply_comment(App_Cmd *app, View_ID view, Buffer_ID buffer, Range_i64 range){
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

function void 
byp_apply_uncomment(App_Cmd *app, View_ID view, Buffer_ID buffer, Range_i64 range){
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
byp_make_vim_request(App_Cmd *app, BYP_Vim_Request request)
{
 vim_make_request(app, Vim_Request_Type(VIM_REQUEST_COUNT + request));
}

function void 
byp_request_title(App_Cmd *app){ byp_make_vim_request(app, BYP_REQUEST_Title); }
function void 
byp_request_comment(App_Cmd *app) { byp_make_vim_request(app, BYP_REQUEST_Comment); }
function void 
byp_request_uncomment(App_Cmd *app){ byp_make_vim_request(app, BYP_REQUEST_UnComment); }
function void 
byp_visual_comment(App_Cmd *app)
{
	if(vim_state.mode == VIM_Visual){
		Vim_Edit_Type edit = vim_state.params.edit_type;
		byp_request_comment(app);
		vim_state.mode = VIM_Visual;
		vim_state.params.edit_type = edit;
	}
}
function void 
byp_visual_uncomment(App_Cmd *app){
	if(vim_state.mode == VIM_Visual){
		Vim_Edit_Type edit = vim_state.params.edit_type;
		byp_request_uncomment(app);
		vim_state.mode = VIM_Visual;
		vim_state.params.edit_type = edit;
	}
}

function void 
kv_newline_above(App_Cmd *app)
{
 GET_VIEW_AND_BUFFER;
 HISTORY_GROUP_SCOPE;
 vim_newline_above(app);
 vim_down(app);
 vim_normal_mode(app);
}

function void
kv_newline_below(App_Cmd *app)
{
 GET_VIEW_AND_BUFFER;
 HISTORY_GROUP_SCOPE;
 vim_newline_below(app);
 vim_up(app);
 vim_normal_mode(app);
}

function void 
byp_reset_face_size(App_Cmd *app)
{
 GET_VIEW_AND_BUFFER;
 Face_ID face_id = get_face_id(app, buffer);
 Face_Description description = get_face_description(app, face_id);
 description.parameters.pt_size = (i1)def_get_config_u64(app, vars_intern_lit("default_font_size"));
 try_modify_face(app, face_id, &description);
}


inline b32 token_is_group_opener(Token *token) {
  return (token->kind == TokenBaseKind_ParenOpen ||
         token->kind == TokenBaseKind_ScopeOpen);
}
inline b32 token_is_group_closer(Token *token) {
 return (token->kind == TokenBaseKind_ParenClose ||
         token->kind == TokenBaseKind_ScopeClose);
}

function u8
kv_is_group_opener(u8 c)
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
function u8
kv_is_group_closer(u8 c)
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
kv_vim_bounce(App_Cmd *app) {
 GET_VIEW_AND_BUFFER;
 Vim_Motion_Block vim_motion_block(app);
 i64 pos = view_get_cursor_pos(app, view);
 pos = vim_scan_bounce(app, buffer, pos, Scan_Forward);
 view_set_cursor_and_preferred_x(app, view, seek_pos(pos));
}

function Range_i64
kv_find_current_nest(App *app, Buffer_ID buffer, i64 pos)
{
 Scratch_Block scratch(app);
 Range_i64 result = {};
 Token_Array tokens = get_token_array_from_buffer(app, buffer);
 if(Token *cur_token = token_from_pos(&tokens, pos)){
  if(cur_token->kind == TokenBaseKind_LiteralString){
   //-String
   result = get_token_range(cur_token);
  }else{
   //-Normal groups (surrounded by group characters)
   Ed_Parser parser_value = make_ed_parser_at_cursor(app);
   Ed_Parser *parser = &parser_value;
   char closer_char = 0;
   i64 closer_pos = 0;
   i64 opener_pos = 0;
   b32 found_nest = false;
   {//-forward
    if(token_is_group_opener(cur_token)){
     //move inside the group, if we're at the start of it
     ep_eat(parser);
    }
    closer_char = ep_eat_until_char(parser, strlit("}])"));
    closer_pos  = ep_get_pos(parser);
   }
   if(closer_char)
   {//-backward
    //NOTE(kv) If the forward succeeded, then the parser should be at
    //  the group closer, so we'll avoid it on the way back.
    parser->direction = Scan_Backward;
    ep_eat(parser);
    char opener_char = get_matching_group_opener(closer_char);
    found_nest = ep_eat_until_char(parser, String{(u8 *)&opener_char, 1});
    opener_pos = ep_get_pos(parser);
   }
   if(found_nest){
    result = Ii64(opener_pos, closer_pos+1);
   }
  }
 }
 return result;
}

function Buffer_AST *
get_buffer_ast(App *app, Buffer_ID buffer)
{
 Managed_Scope scope = buffer_get_managed_scope(app, buffer);
 Buffer_AST *ast = scope_attachment(app, scope, buffer_attachment_ast, Buffer_AST);
 return ast;
}
function void
kv_sexpr_up(App_Cmd *app)
{
 Scratch_Block scratch(app);
 GET_VIEW_AND_BUFFER;
 vim_push_jump(app, view);
 Vim_Motion_Block vim_motion_block(app);
 Ed_Parser parser = make_ed_parser_at_cursor(app, Scan_Backward);
 b32 done = 0;
 
 {
  Token *token = ep_get_token(&parser);
  if(token_is_group_closer(token))
  {
   //-If we're at group end, jump to the beginning
   Buffer_AST *ast = get_buffer_ast(app, buffer);
   
   String closer_string = ep_print_token(scratch, &parser);
   u8 closer = closer_string.data[0];
   u8 opener = get_matching_group_opener(closer);
   ep_eat(&parser);  //NOTE eat past the group closer
   char do_jump = ep_eat_until_char(&parser, opener);
   if(do_jump)
   {
    i64 goto_pos = ep_get_pos(&parser);
    kv_goto_pos(app, view, goto_pos);
   }
   done = 1;
  }
 }
 
 if(not done)
 {
  {//-Detect if we're right at the start of a group
   Token *token = ep_get_token(&parser);
   if(token->kind == TokenBaseKind_LiteralString)
   {
    if(view_get_cursor_pos(app, view) == token->pos)
    {
     //-We're standing right at the string -> back up
     ep_eat(&parser);
    }
   }
   else if(token_is_group_opener(token))
   {
    ep_eat(&parser);
   }
  }
  {//-jump
   Token *token = ep_get_token(&parser);
   if(token->kind == TokenBaseKind_LiteralString)
   {
    kv_goto_pos(app, view, token->pos);
   }
   else
   {
    char do_jump = ep_eat_until_char(&parser, strlit("{[("));
    if(do_jump)
    {
     i64 goto_pos = ep_get_pos(&parser);
     kv_goto_pos(app, view, goto_pos);
    }
   }
  }
 } 
}
function void
kv_sexpr_down(App_Cmd *app)
{
 View_ID   view = get_active_view(app, Access_ReadVisible);
 vim_push_jump(app, view);
 Token_Iterator_Array token_it = get_token_it_at_cursor(app);
 if ( !token_it.tokens ) return;
 
 do {
  Token *token = tkarr_read(&token_it);
  if (token_is_group_opener(token))
  {
   kv_goto_token(app, token);
   move_right(app);
   break;
  }
 }
 while ( tkarr_inc(&token_it) );
}

function b32
if_preprocessor_movement(App *app, Scan_Direction scan_direction)
{
 b32 result = false;
 GET_VIEW_AND_BUFFER;
 Token_Iterator_Array token_it = get_token_it_at_cursor(app);
 if(token_it.tokens)
 {
  Scratch_Block scratch(app);
  Ed_Parser p_value = ed_parser_from_buffer(app, buffer, token_it, 0, scan_direction);
  Ed_Parser *p = &p_value;
  i1 nest_level = 0;
  if(!ep_maybe_preprocessor(p, str8lit("else")) &&
     !ep_maybe_preprocessor(p, str8lit("elif")))
  {
   if(scan_direction == Scan_Forward){
    p->set_ok(ep_maybe_preprocessor(p, str8lit("if")) or
              ep_maybe_preprocessor(p, str8lit("ifdef")) or
              ep_maybe_preprocessor(p, str8lit("ifndef")));
   }else{
    ep_eat_preprocessor(p, str8lit("endif"));
   }
  }
  while(p->ok_){
   b32 is_ifs = (ep_test_preprocessor(p, str8lit("if")) or
                 ep_test_preprocessor(p, str8lit("ifdef")) or
                 ep_test_preprocessor(p, str8lit("ifndef")));
   
   b32 is_endif = ep_test_preprocessor(p, str8lit("endif"));
   
   b32 is_el  = (ep_test_preprocessor(p, str8lit("else")) or
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
   
   ep_eat(p);
  }
  if(p->ok_){
   kv_goto_token(app, ep_get_token(p));
   result = true;
  }
 }
 return result;
}
function void
kv_sexpr_right(App *app)
{
 Token_Iterator_Array token_it = get_token_it_at_cursor(app);
 if(token_it.tokens)
 {
  View_ID view = get_active_view(app, Access_ReadVisible);
  vim_push_jump(app, view);
  b32 jumped = if_preprocessor_movement(app, Scan_Forward);
  if(not jumped)
  {
   i32 nest = 0;
   i64 curpos = view_get_cursor_pos(app, view);
   do{
    Token *token = tkarr_read(&token_it);
    if(token_is_group_opener(token)){
     nest += 1;
    }else if(token_is_group_closer(token)){
     nest -= 1;
     if(nest <= 0){
      if(token->pos != curpos){
       i64 pos = token->pos;
       if(nest < 0){ pos -= 1; }
       kv_goto_pos(app, view, pos);
       break;
      }
      nest = 0;
     }
    }else if(token->kind != TokenBaseKind_Whitespace){
     if(nest == 0){
      i64 token_max = get_token_range(token).end - 1;
      if(token_max != curpos){
       // NOTE: Goto end of token
       kv_goto_pos(app, view, token_max);
       break;
      }
     }
    }
   }while(tkarr_inc(&token_it));
  }
 }
}
function void
kv_sexpr_left(App *app)
{
 b32 jumped = if_preprocessor_movement(app, Scan_Backward);
 if(not jumped)
 {
  Token_Iterator_Array token_it = get_token_it_at_cursor(app);
  Token *token = tkarr_read(&token_it);
  if(token)
  {
   View_ID view = get_active_view(app, Access_ReadVisible);
   vim_push_jump(app, view);
   i32 nest = 0;
   i64 curpos = view_get_cursor_pos(app, view);
   do{
    token = tkarr_read(&token_it);
    if(token_is_group_opener(token)){
     nest -= 1;
     if(nest <= 0){
      if(token->pos != curpos){
       i64 pos = token->pos;
       if(nest < 0){ pos += 1; }
       kv_goto_pos(app, view, pos);
       break;
      }
      nest = 0;
     }
    }else if(token_is_group_closer(token)) {
     nest += 1;
    }else if(token->kind != TokenBaseKind_Whitespace){
     if(nest == 0){
      if(token->pos != curpos){
       // NOTE: Goto begin of token
       kv_goto_pos(app, view, token->pos);
       break;
      }
     }
    }
   }while(tkarr_dec(&token_it));
  }
 }
}
function void 
kv_sexpr_end(App_Cmd *app)
{
 kv_sexpr_up(app);
 kv_sexpr_right(app);
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
buffer_delete_pos(App_Cmd *app, Buffer_ID buffer, i64 min)
{
 buffer_replace_range(app, buffer, Ii64(min, min+1), empty_string);
}

inline void
buffer_insert_pos(App_Cmd *app, Buffer_ID buffer, i64 pos, String string)
{
 buffer_replace_range(app, buffer, Ii64(pos), string);
}

function void 
kv_surround_with(App_Cmd *app, char *opener, char *closer)
{
 GET_VIEW_AND_BUFFER;
 HISTORY_GROUP_SCOPE;
 
 Range_i64 selected = view_get_selected_range(app, view);
 buffer_insert_pos(app, buffer, selected.max, SCu8(closer));
 buffer_insert_pos(app, buffer, selected.min, SCu8(opener));
 
 vim_normal_mode(app);
}

function void
kv_surround_brace_special(App_Cmd *app)
{
 GET_VIEW_AND_BUFFER;
 HISTORY_GROUP_SCOPE;
 
 b32 is_visual = (vim_state.mode == VIM_Visual);
 Range_i64 range = {};
 if(is_visual){
  range = view_get_selected_range(app, view);
 }else{
  i64 curpos = view_get_cursor_pos(app, view);
  range = {curpos, curpos};
 }
 
 range.min = get_line_start_pos_from_pos(app, buffer, range.min);
 range.max = get_line_end_pos_from_pos  (app, buffer, range.max);
 
 buffer_insert_pos(app, buffer, range.max, strlit("\n}"));
 buffer_insert_pos(app, buffer, range.min, strlit("{\n"));
 
 auto_indent_buffer(app, buffer, Ii64(range.min, range.max+4));
 
 vim_normal_mode(app);
}

function void 
kv_reopen_with_confirmation(App_Cmd *app)
{
 if (get_confirmation_from_user(app, strlit("Actually revert?")))
 {
  View_ID view = get_active_view(app, Access_ReadVisible);
  Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
  buffer_reopen(app, buffer, 0);
 }
}

function void 
kv_surround_paren(App_Cmd *app)          {kv_surround_with(app, "(", ")");}
function void 
kv_surround_paren_spaced(App_Cmd *app)   {kv_surround_with(app, "( ", " )");}
function void 
kv_surround_brace(App_Cmd *app)          {kv_surround_with(app, "{", "}");}
function void 
kv_surround_brace_spaced(App_Cmd *app)   {kv_surround_with(app, "{ ", " }");}
function void 
kv_surround_double_quote(App_Cmd *app)   {kv_surround_with(app, "\"", "\"");}

function void
cmd_closing_bracket_in_visual_mode(App_Cmd *app)
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

function void 
kv_void_command(App_Cmd *app) { return; }

function void 
kv_vim_normal_mode(App_Cmd *app) {
 vim_normal_mode(app);
 kv_quail_keystroke_buffer.count = 0;
}
function void
vim_enter_visual_mode(){
 vim_state.mode = VIM_Visual;
 vim_state.params.edit_type = EDIT_CharWise;
}
function Range_i64
kv_sexpr_select_whole(App *app)
{
 GET_VIEW_AND_BUFFER;
 Scratch_Block tmp;
 Range_i64 range = {};
 {
  i64 pos = view_get_cursor_pos(app, view);
  Token_Array tokens = get_token_array_from_buffer(app, buffer);
  Token *token = token_from_pos(&tokens, pos);
  if(token->kind == TokenBaseKind_ScopeOpen or
     token->kind == TokenBaseKind_ScopeClose or
     token->kind == TokenBaseKind_ParenOpen or
     token->kind == TokenBaseKind_ParenClose)
  {//-Select nest
   range = kv_find_current_nest(app, buffer, pos);
  }else{
   //-Select token
   range = get_token_range(token);
  }
 }
 return range;
}
function b32
range_a_contained_in_range_b(Range_i64 a, Range_i64 b)
{
 return (a.min >= b.min) and (a.max <= b.max);
}
function void
cmd_handle_q(App *app)
{
 GET_VIEW_AND_BUFFER;
 Scratch_Block tmp;
 
 i64 cursor_pos = view_get_cursor_pos(app, view);
 Range_i64 selected = {cursor_pos, cursor_pos};
 if(vim_state.mode == VIM_Visual)
 {
  selected = get_selected_range(app);
 }
 
 Range_i64 range = kv_sexpr_select_whole(app);
 
 if(range_a_contained_in_range_b(range, selected))
 {
  auto scan_function = [&](Scan_Direction direction) -> Token *{
   Token *end_token = 0;
   Ed_Parser parser_value = make_ed_parser_at_cursor(app, direction);
   Ed_Parser *parser = &parser_value;
   Token *init_token = ep_get_token(parser);
   b32 expecting_identifier = (init_token->kind == TokenBaseKind_Identifier);
   while(true){
    Token *test_token = ep_get_token(parser);
    b32 passes = false;
    if(expecting_identifier){
     passes = (test_token->kind == TokenBaseKind_Identifier);
     expecting_identifier = false;
    }else{
     if(test_token->kind == TokenBaseKind_Operator){
      String test_string = ep_print_token(tmp, parser, test_token);
      passes = (test_string == "." or test_string == "->");
     }
     expecting_identifier = true;
    }
    if(passes){
     end_token = test_token;
     ep_eat(parser);
    }else{
     break;
    }
   }
   return end_token;
  };
  
  Token *begin_token = scan_function(Scan_Backward);
  Token *end_token   = scan_function(Scan_Forward);
  if(begin_token and end_token){
   range = {begin_token->pos, end_token->pos + end_token->size};
  }
 }
 
 if(range_a_contained_in_range_b(range, selected))
 {
  b32 hit_comma = false;
  {
   Ed_Parser parserv = make_ed_parser_at_cursor(app, Scan_Forward);
   Ed_Parser *parser = &parserv;
   char term = ep_eat_until_char(parser, strlit(")]},"));
   range.max = ep_get_token(parser)->pos;
   hit_comma = term == ',';
   if(hit_comma){ range.max++ ; }
  }
  
  {
   Ed_Parser parserv = make_ed_parser_at_cursor(app, Scan_Backward);
   Ed_Parser *parser = &parserv;
   char term = ep_eat_until_char(parser, strlit("([{,"));
   range.min = ep_get_token(parser)->pos + 1;
   // NOTE(kv) If we haven't eaten a comma, and we see a comma,
   // then we eat the comma.
   if(not hit_comma and term == ','){ range.min -= 1; }
  }
 }
 
 if(range.max > 0 and
    not range_a_contained_in_range_b(range, selected))
 {//-Select
  range.min = minimum(range.min, selected.min);
  range.max = maximum(range.max, selected.max);
  view_set_cursor_pos(app, view, range.min);
  view_set_mark_pos(app, view, range.max-1);
  vim_enter_visual_mode();
 }
}

inline b32
character_is_path(char character)
{
 switch (character) {
  case '/': case '~': case '.': case '\\':  case '-': case '+': case ':':
  return true;
  default:
  return character_is_alnum(character);
 }
}

function void 
copy_filename(App_Cmd *app)
{// normal_commands
 GET_VIEW_AND_BUFFER;
 Scratch_Block temp(app);
 String8 filename = push_buffer_filepath(app, temp, buffer);
 if (filename.size)
 {
  string_mod_replace_character(filename, '\\', '/');
  clipboard_post(0, filename);
 }
}

function void 
file(App_Cmd *app)
{
 GET_VIEW_AND_BUFFER;
 Scratch_Block temp(app);
 String8 dirname = push_buffer_dirname(app, temp, buffer);
 set_hot_directory(app, dirname);
 vim_interactive_open_or_new(app);
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
{
 i32 note_same_buffer        = 1;
 i32 note_implementation     = 2;
 i32 note_not_generated_file = 4;
 
 i32 best_score = 0;
 F4_Index_Note *result = note;
 if(note != 0)
 {
  F4_Index_Note *best_note = 0;
  for(F4_Index_Note *candidate = note;
      candidate;
      candidate = candidate->next)
  {
   F4_Index_File *file = candidate->file;
   if(file != 0)
   {
    i32 score = 0;
    if(file->buffer == cursor_buffer)
    {
     score += note_same_buffer;
    }
    if(not(candidate->flags & F4_Index_NoteFlag_Prototype))
    {
     score += note_implementation;
    }
    if(not file->is_generated)
    {
     score += note_not_generated_file;
    }
    
    if(score > best_score)
    {
     best_score = score;
     result = candidate;
    }
   }
  }
 }
 return result;
}

function b32
f4_goto_definition(App *app)
{
 View_ID view = get_active_view(app, Access_Always);
 Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
 Scratch_Block tmp;
 String string = push_token_or_word_under_active_cursor(app, tmp);
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
{// NOTE(kv) Used to jumping to various places in code. Use sparingly!
 b32 jumped = false;
 String_Match match = {};
 
 Scratch_Block scratch(app);
 String tag;
 {
  GET_VIEW_AND_BUFFER;
  Range_i64 range = get_surrounding_characters(app, character_is_tag);
  tag = push_buffer_range(app, scratch, buffer, range);
 }
 if(tag.size >= 2 and
    starts_with(tag, strlit("@")))
 {
  String identifier = string_skip(tag, 1);
  F4_Index_Note *note = F4_Index_LookupNote(identifier);
  jumped = F4_GoToDefinition(app, note, true);
#if 0
  String_u8 needle = string_u8_push(scratch, identifier.size+1);
  string_concat_character(&needle, ';');
  string_concat(&needle, identifier);
  for (Buffer_ID buffer = get_buffer_next(app, 0, Access_Always);
       buffer != 0;
       buffer = get_buffer_next(app, buffer, Access_Always))
  {
   match = buffer_seek_string(app, buffer, needle.string, Scan_Forward, /*start pos*/0, /*case sensitive*/true);
   if(match.buffer){ break; }
  }
  
  if(match.buffer)
  {
   View_ID view = get_active_view(app, Access_Always);
   view_set_buffer(app, view, match.buffer, 0);
   view_set_cursor(app, view, seek_pos(match.range.min));
   jumped = true;
  }
#endif
 }
 
 return jumped;
}

function void
kv_jump_ultimate(App_Cmd *app)
{
 GET_VIEW_AND_BUFFER;
 Scratch_Block tmp;
 vim_push_jump(app, view);
 b32 jumped = false;
 {//-File path (and maybe line+column)
  Range_i64 loc_range = get_surrounding_characters(app, character_is_path);
  if(loc_range.max > 0){
   String loc = push_buffer_range(app, tmp, buffer, loc_range);
   if(view_open_file(app, view, loc, true)){
    jumped = true;
   }else{
    Parsed_Jump jump = parse_jump_location(loc);
    if(jump.success){
     jump_to_location(app, view, jump.location);
     jumped = true;
    }
   }
  }
  
  if(not jumped)
  {//-Looking at quotes? It might be a path -> just jump to it!
   i64 curpos = view_get_cursor_pos(app, view);
   i64 line_min = get_line_side_pos_from_pos(app, buffer, curpos, Side_Min);
   i64 line_max = get_line_side_pos_from_pos(app, buffer, curpos, Side_Max);
   String needle = strlit("\"");
   i64 quote_start = seek_string_backward(app, buffer, curpos, line_min, needle);
   if(quote_start >= 0){
    i64 quote_end = seek_string_forward(app, buffer, curpos, line_max, needle);
    if(quote_end <= line_max){
     Range_i64 range = Ii64(quote_start+1, quote_end);
     String path = push_buffer_range(app, tmp, buffer, range);
     jumped = view_open_file(app, view, path, true);
     if(not jumped){
      //-Maybe it's some relative path?
      String filename = path_filename(path);
      Buffer_ID existing_buffer = create_buffer(app, filename, BufferCreate_NeverNew);
      if(existing_buffer){
       view_set_buffer(app, view, existing_buffer, 0);
       jumped = true;
      }
     }
    }
   }
  }
 }
 
 jumped = jumped or f4_goto_definition(app);
 jumped = jumped or goto_comment_identifier(app);
 
 if(not jumped)
 {// NOTE(kv): go to the "file" file
  // yank_current_filename_(app);  // In case we wanna paste add the current filename in
  set_buffer_named(app, KV_FILE_FILENAME);
 }
}

function void
kv_jump_ultimate_other_panel(App_Cmd *app) {
 view_buffer_other_panel(app);
 kv_jump_ultimate(app);
}

function void 
kv_delete_surrounding_groupers(App_Cmd *app)
{
 GET_VIEW_AND_BUFFER;
 HISTORY_GROUP_SCOPE;
 i64 pos = view_get_cursor_pos(app, view);
 Range_i64 range = kv_find_current_nest(app, buffer, pos);
 if(range.max)
 {
  buffer_delete_pos(app, buffer, range.max-1);
  buffer_delete_pos(app, buffer, range.min);
  auto_indent_buffer(app, buffer, range);
 }
}

function void 
kv_do_t_function(App_Cmd *app, b32 shiftp)
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
function void 
kv_do_t(App_Cmd *app) {kv_do_t_function(app, false);}
function void 
kv_do_T(App_Cmd *app) {kv_do_t_function(app, true);}

function void 
kv_do_underscore_function(App_Cmd *app, b32 shiftp)
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
function void 
kv_do_underscore(App_Cmd *app)         {kv_do_underscore_function(app, false);}

function void 
kv_do_underscore_shifted(App_Cmd *app) {kv_do_underscore_function(app, true);}

function void 
kv_run(App_Cmd *app)
{
  GET_VIEW_AND_BUFFER;
 Scratch_Block temp(app);
 
 String8 dir = push_hot_directory(app, temp);
 String8 cmd = push_buffer_filepath(app, temp, buffer);
 standard_build_exec_command(app, view, dir, cmd);
}

function void 
kv_open_note_file(App_Cmd *app)
{
 set_buffer_named(app, strlit("~/notes/note.skm"));
}

function void 
switch_to_game_panel(App_Cmd *app)
{
 View_ID view = get_active_view(app, 0);
 view_set_buffer(app, view, get_game_buffer(app, 1), 0);
}

function void 
dir(App_Cmd *app)
{
 GET_VIEW_AND_BUFFER;
 Scratch_Block temp(app);
 String8 dirname = push_buffer_dirname(app, temp, buffer);
 clipboard_post(0, dirname);
}

function void 
kv_vim_visual_line_mode(App_Cmd *app)
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
 print_string_match_list_to_buffer(app, needle, out_buffer, all_matches);
 
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
kv_list_all_locations(App_Cmd *app)
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

function void 
open_build_script(App_Cmd *app)
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

function void 
vim_select_all(App_Cmd *app)
{
 vim_visual_char_mode(app);
 select_all(app);
}

function void 
kv_miscellaneous_debug_command(App_Cmd *app)
{
 vim_set_bottom_text(strlit("one two three"));
}

function void 
init(App_Cmd *app)
{
    set_buffer_named(app, strlit("~/4ed/code/4coder_kv/4coder_kv.cpp"));
}

// TODO(kv): We don't wanna bind to a buffer for some command.
function void kv_system_command(App *app, String8 cmd)
{
 GET_VIEW_AND_BUFFER;
 Scratch_Block temp(app);
 String8 dir = push_buffer_dirname(app, temp, buffer);
 exec_system_command(app, global_bottom_view, standard_build_compilation_buffer_identifier,
                     dir, cmd, standard_build_exec_flags);
}
//~
function void
remedy_add_breakpoint(App *app) {
 GET_VIEW_AND_BUFFER;
 Scratch_Block temp(app);
 String filename = push_buffer_filepath(app, temp, buffer);
 i64 linum = get_current_line_number(app);
 String cmd = push_stringf(temp, "remedybg.exe add-breakpoint-at-file %.*s %lld",
                             string_expand(filename), linum);
 kv_system_command(app, cmd);
}
function void
remedy_stop_debugging(App *app) {
 String cmd = str8_lit("remedybg.exe stop-debugging");
 kv_system_command(app, cmd);
}
function void
remedy_run_to_cursor(App *app) {
 GET_VIEW_AND_BUFFER;
 Scratch_Block temp(app);
 String filename = push_buffer_filepath(app, temp, buffer);
 i64 linum = get_current_line_number(app);
 String cmd = push_stringf(temp, "remedybg.exe run-to-cursor %.*s %lld",
                            string_expand(filename), linum);
 kv_system_command(app, cmd);
}
//-
function void
raddbg_add_breakpoint(App *app) {
 GET_VIEW_AND_BUFFER;
 Scratch_Block temp(app);
 String filename = push_buffer_filepath(app, temp, buffer);
 i64 linum = get_current_line_number(app);
 String cmd = push_stringf(temp, "raddbg --ipc add_breakpoint %.*s:%lld",
                            string_expand(filename), linum);
 kv_system_command(app, cmd);
}
function void
raddbg_stop_debugging(App *app) {
 String cmd = str8_lit("raddbg --ipc kill_all");
 kv_system_command(app, cmd);
}
function void
raddbg_run_to_cursor(App *app) {
 GET_VIEW_AND_BUFFER;
 Scratch_Block temp(app);
 String filename = push_buffer_filepath(app, temp, buffer);
 i64 linum = get_current_line_number(app);
 String cmd = push_stringf(temp, "raddbg --ipc run_to_cursor %.*s:%lld",
                            string_expand(filename), linum);
 kv_system_command(app, cmd);
}
//~
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
 change_active_primary_view(app);
 toggle_split_panel(app);
}

function void 
set_current_dir_as_hot(App_Cmd *app)
{
 GET_VIEW_AND_BUFFER;
 Scratch_Block scratch(app);
 String8 dirname = push_buffer_dirname(app, scratch, buffer);
 set_hot_directory(app, dirname);
}

function void 
scratch(App_Cmd *app)
{
 set_buffer_named(app, strlit("~/notes/scratch.skm"));
}

function void 
messages(App_Cmd *app)
{
 set_buffer_named(app, strlit("*messages*"));
}

function void
quick_align_command(App_Cmd *app)
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
 darray(Line) lines = {};
 init_dynamic(lines, scratch, 32);
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
   push(&lines, line);
  }
 }
 
 i64 rightmost_equal_sign = 0;
 // NOTE: find the right-most equal sign
 for_i32(index, 0, lines.count)
 {
  ClampBot(rightmost_equal_sign, lines[index].equal_sign_pos);
 }
 // NOTE: Then go back and fix up our lines from end to beginning
 u8 space_buffer[256];
 block_fill_u8(space_buffer, 256, ' ');
 for (i64 index=lines.count-1;
      index >= 0;
      index--)
 {
  Line *line = lines.items+index;
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

#if 0
function b32
maybe_handle_fui(App_Cmd *app)
{
 b32 result = false;
 if(is_game_on())
 {
  // NOTE: When the tick doesn't run, we don't load the game code.
  // so we update the game code here so that it doesn't reach in the wrong slider.
  load_latest_game_code(app, 0);
  Game_API *game = get_game_code(Game_On);
  if(game)
  {
   global_game_dll_lock = true;
   result = game->fui_handle_enter(ed_game_state_pointer, app);
   global_game_dll_lock = false;
  }
 }
 return result;
}
#endif

function void
kv_handle_return_normal_mode(App_Cmd *app)
{
 View_ID   view   = get_active_view(app, Access_ReadVisible);
 Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
 if(buffer)
 {// NOTE(kv) Still not sure why "save_all_dirty_buffers" sometimes fails to save?
  Scratch_Block scratch;
  String filename = push_buffer_filepath(app, scratch, buffer);
  buffer_save(app, buffer, filename, 0);
  save_all_dirty_buffers(app);
 }
 else
 {
  buffer = view_get_buffer(app, view, Access_ReadVisible);
  if(buffer) 
  {// NOTE Readonly buffer
   vim_push_jump(app, get_active_view(app, Access_ReadVisible));
   goto_jump_at_cursor(app);
   lock_jump_buffer(app, buffer);
  }
 }
}

function void
cmd_insert_ampersand(App_Cmd *app) {
 write_text(app, strlit("&"), false);
}
function void
cmd_insert_asterisk(App_Cmd *app) {
 write_text(app, strlit("*"), false);
}

function void
cmd_insert_caret(App_Cmd *app)
{
 write_text(app, strlit("^"), false);
}

function void 
are_we_in_debug_build(App_Cmd *app)
{
#if KV_INTERNAL
 vim_set_bottom_text_lit("yes, we are in DEBUG build!");
#else
 vim_set_bottom_text_lit("no, we are in RELEASE build!");
#endif
}

function void
handle_tab_normal_mode(App_Cmd *app)
{
 GET_VIEW_AND_BUFFER;
 
 b32 handled = false;
 
 Game_API *game = get_game_code(Game_On);
 if(game){
  handled = game->game_handle_tab_normal_mode(app);
 }
 
 if(not handled)
 {
  b32 is_limited = is_buffer_limited_edit(app, buffer);
  b32 is_dirty = buffer_get_dirty_state(app, buffer) == DirtyState_UnsavedChanges;
  if(!is_limited and is_dirty){
   handled = auto_indent_line_at_cursor(app);
  }
 }
 
 if(not handled){
  change_active_primary_view(app);
 }
}

function void
handle_space_command(App_Cmd *app)
{
 write_space_command(app);
}

function void
kv_handle_left_click(App_Cmd *app)
{
 click_set_cursor_and_mark(app);
 switch_to_mouse_panel(app);
}

function void 
replace_in_all_buffers(App_Cmd *app)
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
kv_toggle_cpp_comment(App_Cmd *app){
 if (vim_state.mode == VIM_Visual) {
  kv_surround_with(app, "/*", "*/");
 }else{
  Ed_Parser p_value = make_ed_parser_at_cursor(app);
  Ed_Parser *p = &p_value;
  Token *token = ep_get_token(p);
  if (token){
   if (token->kind == TokenBaseKind_Comment){
    Scratch_Block scratch(app);
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
move_parameter_left_or_right(App_Cmd* app, b32 move_rightp)
{
 Scratch_Block scratch(app);
 GET_VIEW_AND_BUFFER;
 i64 curpos = view_get_cursor_pos(app, view);
 //TODO(kv) Uh oh, this wouldn't work if the parameter is a group?
 Range_i64 nest = kv_find_current_nest(app, buffer, curpos);
 if(nest.max)
 {
  Token_Iterator_Array token_it = get_token_it_at_pos(app, buffer, nest.min);
  Ed_Parser parser = ed_parser_from_buffer(app, buffer, token_it);
  Ed_Parser *p = &parser;
  darray(Range_i64) list; init_dynamic(list, scratch);
  push_buffer_range(app, scratch, buffer, nest);
  ep_eat(p);  //NOTE group opener
  //Range_i64 sentinel_range = {};
  while(p->ok_){
   if(Token *token = ep_get_token(p)){
    if(token->pos >= nest.max){
     break;
    }else{
     Range_i64 *item = list.push();
     item->min = token->pos;
     ep_eat_until_char(p, strlit(",)]}"));
     if(Token *end_token = ep_get_token(p)){
      item->max = end_token->pos;
     }
     ep_eat(p);
    }
   }else{ break; }
  }
  if(p->ok_){
   if(list.count){
    i32 curindex = -1;
    for_i32(i,0,list.count)
    {
     if(curpos >= list[i].min &&
        curpos <  list[i].max){
      curindex = i;
      break;
     }
    }
    kv_assert(curindex != -1);
    if(move_rightp){
     //NOTE Move right
     if(curindex < i32(list.count-1)){
      macro_swap(list[curindex], list[curindex+1]);
      i64 new_curpos;
      String replacement;
      {
       Printer pr = make_printer_buffer(scratch, nest.max-nest.min+list.count);
       for_i32(i,0,list.count)
       {
        if(i){ pr<", "; }
        if(i32(i) == curindex+1){
         new_curpos = nest.min+1+pr.byte_pos;
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
     if(curindex > 0)
     {
      macro_swap(list[curindex], list[curindex-1]);
      i64 new_curpos;
      String replacement;
      {
       Printer pr = make_printer_buffer(scratch, nest.max-nest.min+list.count);
       for_i32(i,0,list.count)
       {
        if(i){ pr < ", "; }
        if(i32(i) == curindex-1){
         new_curpos = nest.min+1+pr.byte_pos;
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
move_parameter_right(App_Cmd* app){ move_parameter_left_or_right(app, 1); }
function void
move_parameter_left (App_Cmd* app){ move_parameter_left_or_right(app, 0); }
//-
#include "meta_file_formats.h"

struct Source_Map_Entry
{
 i32 source_pos;
 i32 gen_pos;
};
function b32
vet_map_file(String string)
{//NOTE(kv) Don't vet 100% because oh my god.
 b32 ok = true;
 char *magic = "kmap";
 ok = ok and string.size >= sizeof(Meta_Map_File_Header);
 ok = ok and string.data != 0;
 Meta_Map_File_Header *map = (Meta_Map_File_Header*)string.data;
 ok = ok and map->magic == *(u32 *)magic;
 return ok;
}
function Stringz
get_map_path_from_gen_path(Arena *arena, String gen_path)
{//NOTE(kv) This filename untangling code is stupid,
 //  but it beats "the right thing", for now.
 String stem = path_stem(path_stem(gen_path));
 b32 is_h = (path_extension(gen_path) == strlit("h"));
 Printer p = make_printer_buffer(arena, 256);
 p < path_dir(gen_path) < OS_SLASH < "generated" < OS_SLASH <
  stem < (is_h ? ".kh" : ".kc") < ".map";
 Stringz map_path = printer_get_string(p);
 return map_path;
}
function void
jump_between_meta_and_generated_code(App_Cmd *app)
{
 //TODO(kv) I have a "better" idea how to organize the mess
 //  of map files and generated files and how do we go from one to another.
 //  Just have common functions that link source file to map file and generated file.
 //  Then annotate the generated file with the same metadata -> done!
 //  It is more complicated since you have to peek at the file.
 
 GET_VIEW_AND_BUFFER;
 Scratch_Block tmp(app);
 vim_push_jump(app, view);
 String buffer_path = push_buffer_filepath(app, tmp, buffer);
 String buffer_dir = path_dir(buffer_path);
 
 i64 curpos = view_get_cursor_pos(app, view);
 if(char_is_whitespace(buffer_get_char(app, buffer, curpos))){
  //-Scan first non whitespace
  i64 line_min = get_line_side_pos_from_pos(app, buffer, curpos, Side_Min);
  i64 line_max = get_line_side_pos_from_pos(app, buffer, curpos, Side_Max);
  i64 first_non_white = buffer_seek_character_class_change_1_0(app, buffer, &character_predicate_whitespace, Scan_Forward, line_min);
  if(first_non_white <= line_max){
   ClampBot(curpos, first_non_white);
  }
 }
 
 auto open_file_at_pos = [&](String path, i64 pos) -> void{
  if(view_open_file(app, view, path, true)){
   view_goto_pos(app, view, pos);
   vim_scroll_screen_mid(app);
  }else{
   String message = push_stringf(tmp, "Can't open file '%S'", path);
   vim_set_bottom_text(message);
  }
 };
 
 b32 is_generated = is_generated_file_name(buffer_path);
 
 String map_string = {};
 b32 ok = true;
 {
  Stringz map_path = {};
  {
   if(is_generated){
    map_path = get_map_path_from_gen_path(tmp, buffer_path);
   }else{
    map_path = get_map_path_from_source_path(tmp, buffer_path);
   }
  }
  map_string = read_entire_file(tmp, map_path);
  
  ok = vet_map_file(map_string);
  if(not ok){
   String message = push_stringf(tmp, "Map file '%s' is corrupted or we can't find it.",
                                 map_path);
   vim_set_bottom_text(message);
  }
 }
 
 if(ok)
 {
  Meta_Map_File_Header *map = (Meta_Map_File_Header *)map_string.data;
  Source_Map_Entry *entries = (Source_Map_Entry *)(map + 1);
  if(is_generated)
  {//-Generated -> Generator
   //NOTE(kv) In finding the best match, we favor positions that are less than
   //  the current position (because you can read from there and find the actual location).
   i32 begin = 0; //NOTE(kv) stores the best match entry
   i32 end = map->count;
   while(end-begin > 1)
   {
    i32 entry_index = begin+(end-begin)/2;
    i32 entry_pos = entries[entry_index].gen_pos;
    i32 delta = entry_pos - (i32)curpos;
    if(delta <= 0){
     begin = entry_index;
     if(delta == 0){
      break;
     }
    }else /*(delta > 0)*/{
     end = entry_index;
    }
   }
   {//-Jump!
    String source_path;
    source_path.str   = (u8 *)map + map->source_name_offset;
    source_path.count = map->source_name_count;
    
    i64 pos = 0;
    if(begin < map->count){
     pos = entries[begin].source_pos;
    }
    open_file_at_pos(source_path, pos);
   }
  }else{
   //-Generator -> Generated
   //NOTE(kv) In finding the best match, we favor positions that are less than
   //  the current position (because you can read from there and find the actual location).
   i32 begin = 0; //NOTE(kv) stores the best match entry
   i32 end = map->count;
   while(end-begin > 1)
   {
    i32 entry_index = begin+(end-begin)/2;
    i32 entry_pos = entries[entry_index].source_pos;
    i32 delta = entry_pos - (i32)curpos;
    if(delta <= 0){
     begin = entry_index;
     if(delta == 0){
      break;
     }
    }else /*if(delta > 0)*/{
     end = entry_index;
    }
   }
   {//-Jump!
    String gen_path;
    gen_path.str   = (u8 *)map + map->gen_name_offset;
    gen_path.count = map->gen_name_count;
    
    i64 pos = 0;
    if(begin < map->count){
     pos = entries[begin].gen_pos;
    }
    open_file_at_pos(gen_path, pos);
   }
  }
 }
}
function void
jump_between_meta_and_generated_code_other_panel(App_Cmd *app)
{
 view_buffer_other_panel(app);
 jump_between_meta_and_generated_code(app);
}
function void
cmd_handle_8_normal(App_Cmd *app){
 //View_ID view = get_active_view(app, Access_ReadVisible);
 write_text(app, strlit("*"), false);
}
function void
cmd_insert_parens(App_Cmd *app)
{
 write_text(app, strlit("()"), false);
}
function void
kv_insert_at_sign(App_Cmd *app)
{
 write_text(app, strlit("@"), false);
}
function void
kv_insert_hash_tag(App_Cmd *app)
{
 write_text(app, strlit("#"), false);
}
function void
cmd_open_log_buffer(App_Cmd *app)
{
 set_buffer_named(app, strlit("*log*"));
}
function void
cmd_expand_snippet(App_Cmd *app)
{
 GET_VIEW_AND_BUFFER;
 Scratch_Block scratch;
 Ed_Parser parser_value = make_ed_parser_at_cursor(app);
 Ed_Parser *parser = &parser_value;
 parser->string_arena = scratch;
 
 b32 done = false;
 
 if(not done){
  //-Editor
  Token *token0 = ep_get_token(parser);
  if(ep_maybe_id(parser, strlit("funwrap"))){
   Token *last_token = ep_get_token(parser);
   String function_name = ep_id(parser);
   if(parser->ok_){
    String replacement = push_stringf(scratch, "%.*s__return\n%.*s(%.*s__params)",
                                      strexpand(function_name),
                                      strexpand(function_name),
                                      strexpand(function_name));
    Range_i64 range = {token0->pos, last_token->pos + last_token->size};
    buffer_replace_range(app, buffer, range, replacement);
   }
  }
 }
}
function void
kv_handle_c_normal(App_Cmd *app)
{
 View_ID   view   = get_active_view(app, Access_ReadVisible);
 Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
 if(buffer){
  //writable
  vim_request_change(app);
 }else{
  buffer = view_get_buffer(app, view, Access_ReadVisible);
  if(buffer){
   //readonly
   set_buffer_named(app, compilation_buffer_name);
  }
 }
}
//-
global b32 undo_global_mode;

function void
cmd_undo(App_Cmd *app)
{
 if(undo_global_mode){
  undo_all_buffers(app);
 }else{
  undo(app);
 }
}
function void
cmd_redo(App_Cmd *app)
{
 if(undo_global_mode){
  redo_all_buffers(app);
 }else{
  redo(app);
 }
}
function void
toggle_undo_global_mode(App_Cmd *app)
{
 toggle_boolean(undo_global_mode);
}
//-
function void
cmd_goto_random_position(App_Cmd *app)
{
 GET_VIEW_AND_BUFFER;
 local_persist b32 inited = 0;
 if(not inited){
  inited = 1;
  srand(0xC0FFEE);
 }
 i64 buffer_size = buffer_get_size(app, buffer);
 // NOTE(kv) This is never gonna cover the whole range... but whatever man!
 f64 ratio = f64(rand()) / f64(RAND_MAX);
 i64 random_pos = i64(ratio * f64(buffer_size));
 view_set_cursor_and_preferred_x(app, view, seek_pos(random_pos));
}
function void
cmd_switch_dot_arrow(App_Cmd *app)
{
 Scratch_Block tmp;
 GET_VIEW_AND_BUFFER;
 Ed_Parser parserv = make_ed_parser_at_cursor(app);
 Ed_Parser *parser = &parserv;
 ep_eat(parser);
 Token *op = ep_get_token(parser);
 String op_str = ep_print_token(tmp, parser, op);
 String dot = strlit(".");
 String arrow = strlit("->");
 if(op_str == dot){
  buffer_replace_range(app, buffer, get_token_range(op), arrow);
 }else if(op_str == arrow){
  buffer_replace_range(app, buffer, get_token_range(op), dot);
 }
}
/*function void
delete_comma_separated_item(App_Cmd *app)
{
 Scratch_Block tmp;
 GET_VIEW_AND_BUFFER;
 Ed_Parser parserv = make_ed_parser_at_cursor(app);
 Ed_Parser *parser = &parserv;
 i32 nesting = 0;
 while
 {
 }
}*/
//~EOF
