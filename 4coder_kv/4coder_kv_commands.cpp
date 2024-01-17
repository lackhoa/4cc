/* NOTE(kv): This file is for miscellaneous commands */

#include "4coder_kv_utils.cpp"
#include "4coder_kv_input.cpp"

Table_u64_u64 shifted_version_of_characters;

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
	History_Group history_group = history_group_begin(app, buffer);
	for(i64 l=line0; l<line1; l++){
		i64 line_start = get_pos_past_lead_whitespace_from_line_number(app, buffer, l);
		b32 has_comment = c_line_comment_starts_at_position(app, buffer, line_start);
		if(!has_comment){
			buffer_replace_range(app, buffer, Ii64(line_start), string_u8_litexpr("//"));
			buffer_post_fade(app, buffer, 0.667f, Ii64_size(line_start,2), fcolor_resolve(fcolor_id(defcolor_paste)));
		}
	}
	history_group_end(history_group);
}

VIM_REQUEST_SIG(byp_apply_uncomment){
	i64 line0 = get_line_number_from_pos(app, buffer, range.min);
	i64 line1 = get_line_number_from_pos(app, buffer, range.max);
	line1 += (line0 == line1);
	History_Group history_group = history_group_begin(app, buffer);
	for(i64 l=line0; l<line1; l++){
		i64 line_start = get_pos_past_lead_whitespace_from_line_number(app, buffer, l);
		b32 has_comment = c_line_comment_starts_at_position(app, buffer, line_start);
		if(has_comment){
			buffer_replace_range(app, buffer, Ii64_size(line_start,2), string_u8_empty);
		}
	}
	history_group_end(history_group);
}


inline void byp_make_vim_request(Application_Links *app, BYP_Vim_Request request){
	vim_make_request(app, Vim_Request_Type(VIM_REQUEST_COUNT + request));
}

VIM_COMMAND_SIG(byp_request_title){ byp_make_vim_request(app, BYP_REQUEST_Title); }
VIM_COMMAND_SIG(byp_request_comment){ byp_make_vim_request(app, BYP_REQUEST_Comment); }
VIM_COMMAND_SIG(byp_request_uncomment){ byp_make_vim_request(app, BYP_REQUEST_UnComment); }
VIM_COMMAND_SIG(byp_visual_comment){
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

CUSTOM_COMMAND_SIG(vim_goto_definition_other_panel)
{
  vim_push_jump(app, get_active_view(app, Access_ReadVisible));
  view_buffer_other_panel(app);
  jump_to_definition_at_cursor(app);
}

VIM_COMMAND_SIG(kv_newline_above)
{
  vim_newline_above(app);
  vim_normal_mode(app);
}

VIM_COMMAND_SIG(kv_newline_below)
{
  vim_newline_below(app);
  vim_normal_mode(app);
}

CUSTOM_COMMAND_SIG(byp_reset_face_size)
CUSTOM_DOC("Resets face size to default")
{
  GET_VIEW_AND_BUFFER;
  Face_ID face_id = get_face_id(app, buffer);
  Face_Description description = get_face_description(app, face_id);
  description.parameters.pt_size = (i32)def_get_config_u64(app, vars_save_string_lit("default_font_size"));
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

void kv_vim_bounce(Application_Links *app)
{
  GET_VIEW_AND_BUFFER;
  Vim_Motion_Block vim_motion_block(app);
  i64 pos = view_get_cursor_pos(app, view);
  pos = vim_scan_bounce(app, buffer, pos, Scan_Forward);
  view_set_cursor_and_preferred_x(app, view, seek_pos(pos));
}

// todo(kv): support fallback path that works on characters instead of tokens
function b32
kv_find_current_nest(Application_Links *app, Buffer_ID buffer, i64 pos, Range_i64 *out)
{
  Token *token = kv_token_at_cursor(app);
  if (token == 0) return false;

  if (token->kind == TokenBaseKind_LiteralString)
  {
    *out = token_range(token);
    return true;
  }
  else
  {
    u8 current_char = buffer_get_char(app, buffer, pos);
    if ( kv_is_group_opener(current_char) )
      pos++;
    
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
    else
    {
      return false;
    }
  }
}

VIM_COMMAND_SIG(kv_sexpr_up)
{
  GET_VIEW_AND_BUFFER;
  vim_push_jump(app, view);
  Vim_Motion_Block vim_motion_block(app);
  Token_Array tokens = get_token_array_from_buffer(app, buffer);

  i64 pos = view_correct_cursor(app, view);
  if ( kv_is_group_opener(buffer_get_char(app, buffer, pos)) ) 
  {
    pos--;
  }

  Find_Nest_Flag flags = FindNest_Scope | FindNest_Paren | FindNest_Balanced;
  Range_i64 range = {pos, pos};
  kv_find_current_nest(app, buffer, pos, &range);
  kv_goto_pos(app, view, range.start);
}

VIM_COMMAND_SIG(kv_sexpr_down)
{
  GET_VIEW_AND_BUFFER;
  vim_push_jump(app, view);
  Token_Iterator_Array token_it = kv_token_it_at_cursor(app);
  do
  {
    Token *token = token_it_read(&token_it);
    if (kv_is_group_opener(token))
    {
      kv_goto_token(app, token);
      move_right(app);
      break;
    }
  } while (token_it_inc(&token_it));
}

VIM_COMMAND_SIG(kv_sexpr_right)
{
  GET_VIEW_AND_BUFFER;
  Token_Iterator_Array token_it = kv_token_it_at_cursor(app);
  Token *token = token_it_read(&token_it);
  if (!token) return;
  do
  {
    token = token_it_read(&token_it);
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

VIM_COMMAND_SIG(kv_sexpr_left)
{
  GET_VIEW_AND_BUFFER;
  Token_Iterator_Array token_it = kv_token_it_at_cursor(app, -1);
  Token *token = token_it_read(&token_it);
  if (!token) return;
  do
  {
    Token *token = token_it_read(&token_it);
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

VIM_COMMAND_SIG(kv_sexpr_end)
{
  kv_sexpr_up(app);
  kv_sexpr_right(app);
  move_left(app);
  move_left(app);
}

void kv_surround_with(Application_Links *app, char *opener, char *closer)
{
  GET_VIEW_AND_BUFFER;
  History_Group group = history_group_begin(app, buffer);
  defer(history_group_end(group));

  i64 min = view_get_cursor_pos(app, view);
  i64 max = view_get_mark_pos(app, view);
  if (max < min) SWAP(min, max);
  max += 1;

  buffer_replace_range(app, buffer, Ii64(max), SCu8(closer));
  buffer_replace_range(app, buffer, Ii64(min), SCu8(opener));

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
            case KeyCode_Y:
            {
              View_ID view = get_active_view(app, Access_ReadVisible);
              Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
              buffer_reopen(app, buffer, 0);
              cancelled = true;
            }break;
                        
            case KeyCode_Shift:
            case KeyCode_Control:
            case KeyCode_Alt:
            case KeyCode_Command:
            case KeyCode_CapsLock:
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
VIM_COMMAND_SIG(kv_surround_bracket)        {kv_surround_with(app, "[", "]");}
VIM_COMMAND_SIG(kv_surround_bracket_spaced) {kv_surround_with(app, "[ ", " ]");}
VIM_COMMAND_SIG(kv_surround_brace)          {kv_surround_with(app, "{", "}");}
VIM_COMMAND_SIG(kv_surround_brace_spaced)   {kv_surround_with(app, "{ ", " }");}
VIM_COMMAND_SIG(kv_surround_double_quote)   {kv_surround_with(app, "\"", "\"");}

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
    case '/': case '~': case '.': case '\\':  case '-':
      return true;
    default:
      return character_is_alpha_numeric(character);
  }
}

// todo: support relative path maybe
CUSTOM_COMMAND_SIG(kv_open_file_ultimate)
CUSTOM_DOC("The one-stop-shop for all your file-opening need")
{
  GET_VIEW_AND_BUFFER;
  Scratch_Block temp(app);

  i64 curpos = view_get_cursor_pos(app, view);

  i64 buffer_size = buffer_get_size(app, buffer);
  i64 min = curpos;
  i64 max = curpos;
  //
  for (i64 pos=curpos; pos < buffer_size; pos++)
  {
    u8 character = buffer_get_char(app, buffer, pos);
    if (character_is_path(character))
      max = pos+1;
    else
      break;
  }
  for (i64 pos=curpos; pos >= 0; pos--)
  {
    u8 character = buffer_get_char(app, buffer, pos);
    if (character_is_path(character))
      min = pos;
    else
      break;
  }

  b32 looking_at_file = false;
  if (max > min)
  {
    String_Const_u8 path = push_buffer_range(app, temp, buffer, Range_i64{min, max});
    if ( view_open_file(app, view, path, true) )
    {
      looking_at_file = true;
    }
    else
    { // todo debug this codepath
      File_Attributes attributes = system_quick_file_attributes(temp, path);
      if(attributes.flags & FileAttribute_IsDirectory)
      {
        set_hot_directory(app, path);  // note: influences vim_interactive_open_or_new
      }
    }
  }

  if ( !looking_at_file )
  {
    String_Const_u8 dirname = push_buffer_dir_name(app, temp, buffer);
    set_hot_directory(app, dirname);
    vim_interactive_open_or_new(app);
  }
}

VIM_COMMAND_SIG(kv_delete_surrounding_groupers)
{
  GET_VIEW_AND_BUFFER;
  History_Group group = history_group_begin(app, buffer);
  defer(history_group_end(group));

  i64 pos = view_get_cursor_pos(app, view);
  Range_i64 range = {};
  if ( kv_find_current_nest(app, buffer, pos, &range) )
  {
    kv_buffer_delete_pos(app, buffer, range.max-1);
    kv_buffer_delete_pos(app, buffer, range.min);
  }
}

function void kv_do_t_internal(Application_Links *app, b32 shiftp)
{
  GET_VIEW_AND_BUFFER;
  History_Group group = history_group_begin(app, buffer);
  defer(history_group_end(group));

  i64 pos = view_get_cursor_pos(app, view);
  u8 current_char = buffer_get_char(app, buffer, pos);
  // 1. optionally delete space
  if (current_char == ' ')
  {
    kv_buffer_delete_pos(app, buffer, pos);
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
    i64 alpha_max = scan_any_boundary(app, boundary_alpha_numeric, buffer, Scan_Forward, pos);
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
    kv_buffer_replace_range(app, buffer, pos, max, replacement);

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

  String_Const_u8 dir = push_hot_directory(app, temp);
  String_Const_u8 cmd = push_buffer_file_name(app, temp, buffer);
  standard_build_exec_command(app, view, dir, cmd);
}

CUSTOM_COMMAND_SIG(note)
CUSTOM_DOC("run the current script")
{
  View_ID   view = get_active_view(app, Access_ReadVisible);
  Buffer_ID buffer = create_buffer(app, SCu8("~/notes/note.skm"), 0);
  view_set_buffer(app, view, buffer, 0);
}

CUSTOM_COMMAND_SIG(file)
CUSTOM_DOC("kv copy file name")
{
  GET_VIEW_AND_BUFFER;
  Scratch_Block temp(app);
  String_Const_u8 filename = push_buffer_file_name(app, temp, buffer);
  clipboard_post(0, filename);
}

CUSTOM_COMMAND_SIG(dir)
CUSTOM_DOC("kv copy dir name")
{
  GET_VIEW_AND_BUFFER;
  Scratch_Block temp(app);
  String_Const_u8 dirname = push_buffer_dir_name(app, temp, buffer);
  clipboard_post(0, dirname);
}

VIM_COMMAND_SIG(kv_newline_and_indent)
{
  write_text(app, string_u8_litexpr("\n"));
  auto_indent_line_at_cursor(app);
}

VIM_COMMAND_SIG(kv_vim_visual_line_mode)
{
	if(vim_state.mode != VIM_Visual)
  {
		set_mark(app);
		vim_state.mode = VIM_Visual;
	}
	vim_state.params.edit_type = EDIT_LineWise;
}

CUSTOM_COMMAND_SIG(build)
CUSTOM_DOC("kv goto build file (todo rename me)")
{
  GET_VIEW_AND_BUFFER;
  Scratch_Block temp(app);
  String_Const_u8 dirname = push_buffer_dir_name(app, temp, buffer);
  String_Const_u8 build_file = kv_search_build_file_from_dir(app, temp, dirname);
  if (build_file.size)
  {
    view_open_file(app, view, build_file, true);
  }
}

function void
kv_list_all_locations_from_string(Application_Links *app, String_Const_u8 needle_str)
{
  Scratch_Block temp(app);
 
  View_ID default_target_view = get_next_view_after_active(app, Access_Always);
  Buffer_ID search_buffer = create_or_switch_to_buffer_and_clear_by_name(app, search_name, default_target_view);
  
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
        pos = kv_fuzzy_search_forward(app, buffer, pos, needle_str);
        if (pos < range.end)
        {
          // note(kv): just a dummy range, not sure if it's even used
          Range_i64 range = {pos, pos+1};
          string_match_list_push(temp, &buffer_matches, buffer, 0, 0, range);
        }
        kv_assert_defend(pos > original_pos, break;);
      }
    }
    all_matches = string_match_list_join(&all_matches, &buffer_matches);
  }

  string_match_list_filter_remove_buffer(&all_matches, search_buffer);
  string_match_list_filter_remove_buffer_predicate(app, &all_matches, buffer_has_name_with_star);

  print_string_match_list_to_buffer(app, search_buffer, all_matches);
}

u8 kv_get_current_char(Application_Links *app)
{
  GET_VIEW_AND_BUFFER;
  i64 pos = view_get_cursor_pos(app, view);
  return buffer_get_char(app, buffer, pos);
}

CUSTOM_COMMAND_SIG(kv_list_all_locations)
CUSTOM_DOC("adapted from list_all_locations for fuzzy search, if cursor at identifier then search for that instead")
{
  GET_VIEW_AND_BUFFER;
  if ( character_is_alpha(kv_get_current_char(app)) )
  {
    list_all_locations_of_identifier(app);
  }
  else
  {
    Scratch_Block temp(app);
    u8 *space = push_array(temp, u8, KB(1));
    String_Const_u8 needle_str = get_query_string(app, "List Locations For: ", space, KB(1));
    if (!needle_str.size) return;
    kv_list_all_locations_from_string(app, needle_str); 
  }
}

VIM_COMMAND_SIG(kv_handle_return)
{
  // note(kv): The behavior mimicks "if_read_only_goto_position", doesn't make too much sense for us.
  View_ID view = get_active_view(app, Access_ReadVisible);
  Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
  if (buffer)
  {
    save_all_dirty_buffers(app);
  }
  else
  {
    buffer = view_get_buffer(app, view, Access_ReadVisible);
    if (buffer)
    {
      vim_push_jump(app, get_active_view(app, Access_ReadVisible));
      goto_jump_at_cursor(app);
      lock_jump_buffer(app, buffer);
    }
  }
}