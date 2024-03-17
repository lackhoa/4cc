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
			buffer_replace_range(app, buffer, Ii64(line_start), string_u8_litexpr("//"));
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
			buffer_replace_range(app, buffer, Ii64_size(line_start,2), string_u8_empty);
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

internal void
vim_goto_definition_other_panel(App *app)
{
    vim_push_jump(app, get_active_view(app, Access_ReadVisible));
    view_buffer_other_panel(app);
    jump_to_definition_at_cursor(app);
}

VIM_COMMAND_SIG(kv_newline_above)
{
  GET_VIEW_AND_BUFFER;
  HISTORY_GROUP_SCOPE;
  vim_newline_above(app);
  vim_down(app);
  vim_normal_mode(app);
}

internal void kv_newline_below(App *app)
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
  description.parameters.pt_size = (i32)def_get_config_u64(app, vars_intern_lit("default_font_size"));
  try_modify_face(app, face_id, &description);
}

CUSTOM_COMMAND_SIG(kv_profile_disable_and_inspect)
CUSTOM_DOC("disable and inspect profile")
{
  profile_disable(app);
  profile_inspect(app);
 }

inline b32 kv_is_group_opener(Token *token)
{
  return (token->kind == TokenBaseKind_ParentheticalOpen ||
          token->kind == TokenBaseKind_ScopeOpen);
}

inline b32 kv_is_group_closer(Token *token)
{
  return (token->kind == TokenBaseKind_ParentheticalClose ||
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

internal void 
kv_vim_bounce(App *app)
{
  GET_VIEW_AND_BUFFER;
  Vim_Motion_Block vim_motion_block(app);
  i64 pos = view_get_cursor_pos(app, view);
  pos = vim_scan_bounce(app, buffer, pos, Scan_Forward);
  view_set_cursor_and_preferred_x(app, view, seek_pos(pos));
}

internal b32
kv_find_current_nest(App *app, Buffer_ID buffer, i64 pos, Range_i64 *out)
{
 Token_Array tokens = get_token_array_from_buffer(app, buffer);
 if (Token *token = token_from_pos(&tokens, pos))
 {
  if (token->kind == TokenBaseKind_LiteralString)
  {
   Range_i64 range = token_range(token);
   *out = range;
   return true;
  }
  else
  {
   u8 current_char = buffer_get_char(app, buffer, pos);
   if ( kv_is_group_opener(current_char) ) { pos++; }
   
   Find_Nest_Flag flags = FindNest_Scope | FindNest_Paren | FindNest_Balanced;
   Range_i64 range = {};
   if (find_nest_side(app, buffer, pos-1, flags,
                      Scan_Backward, NestDelim_Open, &range.start) &&
       find_nest_side(app, buffer, pos, flags|FindNest_EndOfToken,
                      Scan_Forward, NestDelim_Close, &range.end))
   {
    *out = range;
    return true;
   }
   else { return false; }
  }
 }
 else { return false; }
}

internal void kv_sexpr_up(App *app)
{
    GET_VIEW_AND_BUFFER;
    vim_push_jump(app, view);
    Vim_Motion_Block vim_motion_block(app);
    
    i64 pos = view_correct_cursor(app, view);
    if ( kv_is_group_opener(buffer_get_char(app, buffer, pos)) ) 
    {
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
  Token_Iterator_Array token_it = token_it_at_cursor(app);
  if ( !token_it.tokens ) return;
  
  do
  {
    Token *token = token_it_read(&token_it);
    if (kv_is_group_opener(token))
    {
      kv_goto_token(app, token);
      move_right(app);
      break;
    }
  }
  while ( token_it_inc(&token_it) );
}

internal b32
if_preprocessor_movement(App *app, Scan_Direction scan_direction)
{
    b32 result = false;
    GET_VIEW_AND_BUFFER;
    Token_Iterator_Array token_it = token_it_at_cursor(app);
    if (token_it.tokens)
    {
        Quick_Parser p_value = qp_new(app, buffer, &token_it, scan_direction);
        Quick_Parser *p = &p_value;
        i32 nest_level = 0;
        if (!qp_maybe_preprocessor(p, str8lit("else")) &&
            !qp_maybe_preprocessor(p, str8lit("elif")))
        {
            if (scan_direction == Scan_Forward)
                qp_eat_preprocessor(p, str8lit("if"));
            else
                qp_eat_preprocessor(p, str8lit("endif"));
        }
        while ( p->ok )
        {
            b32 is_ifs = (qp_test_preprocessor(p, str8lit("if")) ||
                          qp_test_preprocessor(p, str8lit("ifdef")) ||
                          qp_test_preprocessor(p, str8lit("ifdef")));
            
            b32 is_endif = qp_test_preprocessor(p, str8lit("endif"));
            
            b32 is_el  = (qp_test_preprocessor(p, str8lit("else")) ||
                          qp_test_preprocessor(p, str8lit("elif")));
            
            if (is_ifs)
            {
                nest_level += scan_direction;
            }
            if (is_endif)
            {
                nest_level -= scan_direction;
            }
            
            if (is_el && nest_level == 0)
            {
                break;
            }
            if ((is_ifs || is_endif) &&  (nest_level == -1))
            {
                break;
            }
            
            qp_eat_token(p);
        }
        if ( p->ok )
        {
            kv_goto_token(app, qp_get_token(p));
            result = true;
        }
    }
    return result;
}

internal void 
kv_sexpr_right(App *app)
{
    Token_Iterator_Array token_it = token_it_at_cursor(app);
    if ( token_it.tokens )
    {
        View_ID view = get_active_view(app, Access_ReadVisible);
        vim_push_jump(app, view);
        if ( !if_preprocessor_movement(app, Scan_Forward) )
        {
            do
            {
                Token *token = token_it_read(&token_it);
                if (token->kind == TokenBaseKind_LiteralString)
                {
                    i64 token_end = token_range(token).end;
                    kv_goto_pos(app, view, token_end);
                    break;
                }
                else if (kv_is_group_opener(token))
                {
                    kv_goto_token(app, token);
                    kv_vim_bounce(app);
                    move_right(app);
                    break;
                }
                else if (kv_is_group_closer(token))
                {
                    kv_goto_token(app, token);
                    move_left(app);
                    break;
                }
            } while ( token_it_inc(&token_it) );
        }
    }
}

internal void
kv_sexpr_left(App *app)
{
    if ( if_preprocessor_movement(app, Scan_Backward) == 0 )
    {
        Token_Iterator_Array token_it = token_it_at_cursor_delta(app, -1);
        Token *token = token_it_read(&token_it);
        if (token)
        {
            View_ID view = get_active_view(app, Access_ReadVisible);
            vim_push_jump(app, view);
            do
            {
                token = token_it_read(&token_it);
                if (token->kind == TokenBaseKind_LiteralString)
                {
                    kv_goto_token(app, token);
                    break;
                }
                else if (kv_is_group_opener(token))
                {
                    kv_goto_token(app, token);
                    move_right(app);
                    break;
                }
                else if (kv_is_group_closer(token))
                {
                    kv_goto_token(app, token);
                    kv_vim_bounce(app);
                    break;
                }
            } while (token_it_dec(&token_it));
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

internal void 
kv_surround_with(App *app, char *opener, char *closer)
{
    GET_VIEW_AND_BUFFER;
    HISTORY_GROUP_SCOPE;
   
    Range_i64 selected = view_get_selected_range(app, view);
    buffer_insert_pos(app, buffer, selected.max, SCu8(closer));
    buffer_insert_pos(app, buffer, selected.min, SCu8(opener));
    
    vim_normal_mode(app);
}

internal void
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
    Query_Bar_Group group(app);
    Query_Bar bar = {};
    bar.prompt = SCu8("Reload current buffer from disk?");
    if (start_query_bar(app, & bar, 0))
    {
      b32 cancelled = false;
      for (;!cancelled;){
        User_Input in = get_next_input(app, EventProperty_AnyKey, 0);
        if (in.abort){
          cancelled = true;
        }
        else{
          switch (in.event.key.code){
            case Key_Code_Y:
            {
              View_ID view = get_active_view(app, Access_ReadVisible);
              Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
              buffer_reopen(app, buffer, 0);
              cancelled = true;
            }break;
                        
            case Key_Code_Shift:
            case Key_Code_Control:
            case Key_Code_Alt:
            case Key_Code_Command:
            case Key_Code_CapsLock:
            {}break;
                        
            default:
            {
              cancelled = true;
            }break;
          }
        }
      }
    }
}

VIM_COMMAND_SIG(kv_surround_paren)          {kv_surround_with(app, "(", ")");}
VIM_COMMAND_SIG(kv_surround_paren_spaced)   {kv_surround_with(app, "( ", " )");}
VIM_COMMAND_SIG(kv_surround_brace)          {kv_surround_with(app, "{", "}");}
VIM_COMMAND_SIG(kv_surround_brace_spaced)   {kv_surround_with(app, "{ ", " }");}
VIM_COMMAND_SIG(kv_surround_double_quote)   {kv_surround_with(app, "\"", "\"");}

internal void
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

VIM_COMMAND_SIG(kv_vim_normal_mode)
{
  vim_normal_mode(app);
  arrsetlen(kv_quail_keystroke_buffer, 0);
}

VIM_COMMAND_SIG(kv_sexpr_select_whole)
{
  GET_VIEW_AND_BUFFER;

  i64 pos = view_get_cursor_pos(app, view);
  Range_i64 nest = {};
  if ( kv_find_current_nest(app, buffer, pos, &nest) )
  {
    view_set_cursor_and_preferred_x(app, view, seek_pos(nest.min));
    view_set_mark(app, view, seek_pos(nest.max-1));
    vim_state.mode = VIM_Visual;
    vim_state.params.edit_type = EDIT_CharWise;
  }
}

inline b32
character_is_path(char character)
{
  switch (character)
  {
    case '/': case '~': case '.': case '\\':  case '-': case ':':
      return true;
    default:
      return character_is_alnum(character);
  }
}

internal void 
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

internal void 
open_file_from_current_dir(App *app)
{
    GET_VIEW_AND_BUFFER;
    Scratch_Block temp(app);
    String8 dirname = push_buffer_dirname(app, temp, buffer);
    set_hot_directory(app, dirname);
 vim_interactive_open_or_new(app);
}

internal void 
kv_handle_g_f(App *app)
{
 open_file_from_current_dir(app);
}


global const String KV_FILE_FILENAME = str8lit("~/notes/file.skm");

internal b32
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

internal F4_Index_Note *
F4_FindMostIntuitiveNoteInDuplicateChain(F4_Index_Note *note, Buffer_ID cursor_buffer, i64 cursor_pos)
{
    F4_Index_Note *result = note;
    if(note != 0)
    {
        F4_Index_Note *best_note_based_on_cursor = 0;
        for(F4_Index_Note *candidate = note; candidate; candidate = candidate->next)
        {
            F4_Index_File *file = candidate->file;
            if(file != 0)
            {
                if(cursor_buffer == file->buffer &&
                   candidate->range.min <= cursor_pos && cursor_pos <= candidate->range.max)
                {
                    if(candidate->next)
                    {
                        best_note_based_on_cursor = candidate->next;
                        break;
                    }
                    else
                    {
                        best_note_based_on_cursor = note;
                        break;
                    }
                }
            }
        }
        
        if(best_note_based_on_cursor)
        {
            result = best_note_based_on_cursor;
        }
        else if(note->flags & F4_Index_NoteFlag_Prototype)
        {
            for(F4_Index_Note *candidate = note; candidate; candidate = candidate->next)
            {
                if(!(candidate->flags & F4_Index_NoteFlag_Prototype))
                {
                    result = candidate;
                    break;
                }
            }
  }
 }
 return result;
}

// CUSTOM_DOC("Goes to the definition of the identifier under the cursor.")
internal b32
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

internal b32
character_is_tag(char c)
{
 return (c == '@' || character_is_alnum(c));
}

internal b32
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
  if (tag.size >= 2 && starts_with(tag, strlit("@")))
  {
   String identifier = string_skip(tag, 1);
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

internal void
kv_jump_ultimate(App *app)
{
 GET_VIEW_AND_BUFFER;
 Scratch_Block temp(app);
 
 vim_push_jump(app, view);
 
 b32 jumped = false;
 
 {
  Range_i64 file_range = get_surrounding_characters(app, character_is_path);
  if (file_range.max > 0)
  {
   String path = push_buffer_range(app, temp, buffer, file_range);
   if ( view_open_file(app, view, path, true) )
   {
    jumped = true;
   }
  }
 }
 
 jumped = jumped || f4_goto_definition(app);
 jumped = jumped || goto_comment_identifier(app);
 
 if (!jumped)
 {// NOTE(kv): go to the "file" file
  // yank_current_filename_(app);  // In case we wanna paste add the current filename in
  view_set_buffer_named(app, KV_FILE_FILENAME);
 }
}

internal void
kv_jump_ultimate_other_panel(App *app)
{
    view_buffer_other_panel(app);
    kv_jump_ultimate(app);
}

VIM_COMMAND_SIG(kv_delete_surrounding_groupers)
{
  GET_VIEW_AND_BUFFER;
  HISTORY_GROUP_SCOPE;

  i64 pos = view_get_cursor_pos(app, view);
  Range_i64 range = {};
  if ( kv_find_current_nest(app, buffer, pos, &range) )
  {
    buffer_delete_pos(app, buffer, range.max-1);
    buffer_delete_pos(app, buffer, range.min);
  }
}

function void 
kv_do_t_internal(App *app, b32 shiftp)
{
  GET_VIEW_AND_BUFFER;
  HISTORY_GROUP_SCOPE;
    
  i64 pos = view_get_cursor_pos(app, view);
  u8 current_char = buffer_get_char(app, buffer, pos);
  // 1. optionally delete space
  if (current_char == ' ')
  {
    buffer_delete_pos(app, buffer, pos);
    current_char = buffer_get_char(app, buffer, pos);
  }
  else if (current_char == '_')
  {
    pos++;
    current_char = buffer_get_char(app, buffer, pos);
  }

  if ( character_is_alpha(current_char) )
  {
    // 2. upcase character/word
    Scratch_Block temp(app);
    i64 max = 0;
    String_Const_u8 replacement = {};
    i64 alpha_max = scan_any_boundary(app, boundary_alnum, buffer, Scan_Forward, pos);
    if (shiftp)
    {
      max = alpha_max;
      Range_i64 range = {pos, alpha_max};
      replacement = push_buffer_range(app, temp, buffer, range);
      string_mod_upper(replacement);
    }
    else
    {
      max = pos+1;
      u8 upper = character_to_upper(current_char);
      replacement = push_string_const_u8(temp, 1);
      replacement.str[0] = upper;
    }
    buffer_replace_range(app, buffer, Ii64(pos,max), replacement);

    // 3. move
    view_set_cursor_and_preferred_x(app, view, seek_pos(alpha_max));
  }
  else
  {
    move_right(app);
  }
}

VIM_COMMAND_SIG(kv_do_t) {kv_do_t_internal(app, false);}
VIM_COMMAND_SIG(kv_do_T) {kv_do_t_internal(app, true);}

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
 view_set_buffer_named(app, strlit("~/notes/note.skm"));
}

CUSTOM_COMMAND_SIG(game)
CUSTOM_DOC("switch to game panel")
{
 view_set_buffer_named(app, GAME_BUFFER_NAMES[0]);
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

internal void 
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
  Range_i64 range = buffer_range(app, buffer);
  {
   for (i64 pos = 0; 
        pos < range.end;)
   {
    i64 original_pos = pos;
    pos = kv_fuzzy_search_forward(app, buffer, pos, needle);
    if (pos < range.end)
    {
     // note(kv): just a dummy range, not sure if it's even used
     Range_i64 range2 = {pos, pos+1};
     string_match_list_push(temp, &buffer_matches, buffer, 0, 0, range2);
    }
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

internal u8 
kv_get_current_char(App *app)
{
 GET_VIEW_AND_BUFFER;
 i64 pos = view_get_cursor_pos(app, view);
 return buffer_get_char(app, buffer, pos);
}

// CUSTOM_DOC("adapted from list_all_locations for fuzzy search, if cursor at identifier then search for that instead")
internal void 
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
   GET_VIEW_AND_BUFFER;
   needle_str = push_buffer_selected_range(app, temp, buffer);
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
    view_set_buffer_named(app, strlit("~/4ed/code/4coder_kv/4coder_kv.cpp"));
}

// todo: We don't wanna bind to a buffer, maybe?
internal void kv_system_command(App *app, String8 cmd)
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
    String8 cmd = push_stringf(temp, "remedybg.exe add-breakpoint-at-file %.*s %lld",
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
    String8 cmd = push_stringf(temp, "remedybg.exe run-to-cursor %.*s %lld",
                               string_expand(filename), linum);
    kv_system_command(app, cmd);
}

internal void
clipboard_pop_command(App *app)
{
    clipboard_pop(app, 0);
    Scratch_Block scratch(app);
    String8 current_item = push_clipboard_index(app, scratch, 0, 0);
   
    // NOTE(kv): hack to make vim paste this thing (don't understand it)
    // Managed_Scope scope = view_get_managed_scope(app, view);
    // i32 *paste_index_ptr = scope_attachment(app, scope, view_paste_index_loc, i32);
    // *paste_index_ptr = 0;
   
    // NOTE(kv): print it
    vim_set_bottom_text(current_item);
}

internal void
view_goto_first_search_position(App *app, View_ID view, String8 needle)
{
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    i64 pos = kv_fuzzy_search_forward(app, buffer, 0, needle);
    view_goto_pos(app, view, pos);
}


internal void 
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
    view_set_buffer_named(app, strlit("~/notes/scratch.skm"));
}

CUSTOM_COMMAND_SIG(messages)
CUSTOM_DOC("switch to messages buffer")
{
    view_set_buffer_named(app, strlit("*messages*"));
}

internal void
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

internal b32
maybe_handle_fui(App *app, Buffer_ID buffer)
{
 load_latest_game_code(app, 0);
 Game_API *game = &global_game_code;
 if ( game->is_valid )
 {
  global_game_dll_lock = true;
  Scratch_Block scratch(app);
  String filename = push_buffer_filename(app, scratch, buffer);
  i64 line_number = get_current_line_number(app);
  b32 handled = game->fui_handle_slider(app, buffer, filename, line_number);
  global_game_dll_lock = false;
  return handled;
 }
 else { return false; }
}

internal void
kv_handle_return_normal_mode(App *app)
{
 View_ID view = get_active_view(app, Access_ReadVisible);
 Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
 if (buffer)
 {// Writable buffer
  if ( maybe_handle_fui(app, buffer) )
  {
   // pass
  }
  else
  {
   save_all_dirty_buffers(app);
  }
 }
 else
 {
  buffer = view_get_buffer(app, view, Access_ReadVisible);
  if (buffer)
  {// Readonly buffer
   vim_push_jump(app, get_active_view(app, Access_ReadVisible));
   goto_jump_at_cursor(app);
   lock_jump_buffer(app, buffer);
  }
 }
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

internal void
handle_tab_normal_mode(App *app)
{
 change_active_primary_view(app);
#if 0
 if ( global_game_on_readonly )
 {
  change_active_primary_view(app);
 }
 else
 {
  auto_indent_line_at_cursor(app);
 }
#endif
}

internal void
handle_space_command(App *app)
{
 if (global_game_on_readonly)
 {
  i32 viewport_id = get_active_game_viewport_id(app);
  global_game_code.game_last_preset(global_game_state, viewport_id);
 }
 else if ( get_active_game_viewport_id(app) )
 {
  turn_game_on();
 }
 else
 {
  write_space_command(app);
 }
}

//~EOF
