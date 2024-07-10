#pragma once

#include "4coder_fleury_base_commands.cpp"
#include "4coder_fleury_lego.cpp"

struct Command_Map_ID_Pair
{
	Command_Map_ID From;
	Command_Map_ID To;
};
static Command_Map_ID_Pair GlobalCommandMapReroute[4] = {};

CUSTOM_COMMAND_SIG(switch_to_keybinding_0)
CUSTOM_DOC("Switch the keybindings to mode 0.")
{
	GlobalKeybindingMode = KeyBindingMode_0;
}

CUSTOM_COMMAND_SIG(switch_to_keybinding_1)
CUSTOM_DOC("Switch the keybindings to mode 1.")
{
	GlobalKeybindingMode = KeyBindingMode_1;
}

CUSTOM_COMMAND_SIG(switch_to_keybinding_2)
CUSTOM_DOC("Switch the keybindings to mode 2.")
{
	GlobalKeybindingMode = KeyBindingMode_2;
}

CUSTOM_COMMAND_SIG(switch_to_keybinding_3)
CUSTOM_DOC("Switch the keybindings to mode 3.")
{
	GlobalKeybindingMode = KeyBindingMode_3;
}

function Implicit_Map_Result
F4_ImplicitMap(App *app, String_ID lang, String_ID mode, Input_Event *event)
{
    Implicit_Map_Result result = {};
    
    View_ID view = get_this_ctx_view(app, Access_Always);
    
	Command_Map_ID orig_id = default_get_map_id(app, view);
    Command_Map_ID map_id = orig_id;
	if(GlobalKeybindingMode == KeyBindingMode_1)
	{
		for(u32 PairIndex = 0;
			PairIndex < ArrayCount(GlobalCommandMapReroute);
			++PairIndex)
		{
			if(GlobalCommandMapReroute[PairIndex].From == map_id)
			{
				map_id = GlobalCommandMapReroute[PairIndex].To;
				break;
			}
		}
	}
	
	Command_Binding binding = map_get_binding_recursive(&framework_mapping, map_id, event);
	if(!binding.custom)
	{
		binding = map_get_binding_recursive(&framework_mapping, orig_id, event);
	}
    
    // TODO(allen): map_id <-> map name?
    result.map = 0;
    result.command = binding.custom;
    
    return(result);
}

CUSTOM_COMMAND_SIG(fleury_startup);

//~ NOTE(rjf): Bindings

function void
F4_SetAbsolutelyNecessaryBindings(Mapping *mapping)
{
    String_ID global_map_id = vars_intern_lit("keys_global");
    String_ID file_map_id = vars_intern_lit("keys_file");
    String_ID code_map_id = vars_intern_lit("keys_code");
    
	String_ID global_command_map_id = vars_intern_lit("keys_global_1");
	String_ID file_command_map_id = vars_intern_lit("keys_file_1");
    String_ID code_command_map_id = vars_intern_lit("keys_code_1");
    
	implicit_map_function = F4_ImplicitMap;
	
	MappingScope();
    SelectMapping(mapping);
    
    SelectMap(global_map_id);
    BindCore(fleury_startup, CoreCode_Startup);
    BindCore(default_try_exit, CoreCode_TryExit);
    Bind(exit_4coder,          Key_Code_F4, Key_Code_Alt);
    BindMouseWheel(mouse_wheel_scroll);
    BindMouseWheel(mouse_wheel_change_face_size, Key_Code_Control);
    
    SelectMap(file_map_id);
    ParentMap(global_map_id);
    BindTextInput(fleury_write_text_input);
    BindMouse(click_set_cursor_and_mark, MouseCode_Left);
    BindMouseRelease(click_set_cursor, MouseCode_Left);
    BindCore(click_set_cursor_and_mark, CoreCode_ClickActivateView);
    BindMouseMove(click_set_cursor_if_lbutton);
    
    SelectMap(code_map_id);
    ParentMap(file_map_id);
    BindTextInput(fleury_write_text_and_auto_indent);
    BindMouse(f4_lego_click_store_token_1, MouseCode_Right);
    BindMouse(f4_lego_click_store_token_2, MouseCode_Middle);
    
    SelectMap(global_command_map_id);
	ParentMap(global_map_id);
	GlobalCommandMapReroute[0].From = global_map_id;
	GlobalCommandMapReroute[0].To = global_command_map_id;
	
    SelectMap(file_command_map_id);
	ParentMap(global_command_map_id);
	GlobalCommandMapReroute[1].From = file_map_id;
	GlobalCommandMapReroute[1].To = file_command_map_id;
	
    SelectMap(code_command_map_id);
	ParentMap(file_command_map_id);
	GlobalCommandMapReroute[2].From = code_map_id;
	GlobalCommandMapReroute[2].To = code_command_map_id;
    
}

function void
F4_SetDefaultBindings(Mapping *mapping)
{
    String_ID global_map_id = vars_intern_lit("keys_global");
    String_ID file_map_id = vars_intern_lit("keys_file");
    String_ID code_map_id = vars_intern_lit("keys_code");
    
    MappingScope();
    SelectMapping(mapping);
    SelectMap(global_map_id);
    Bind(keyboard_macro_start_recording , Key_Code_U, Key_Code_Control);
    Bind(keyboard_macro_finish_recording, Key_Code_U, Key_Code_Control, Key_Code_Shift);
    Bind(keyboard_macro_replay,           Key_Code_U, Key_Code_Alt);
    Bind(interactive_new,               Key_Code_N, Key_Code_Control);
    Bind(interactive_open_or_new,       Key_Code_O, Key_Code_Control);
    Bind(open_in_other,                 Key_Code_O, Key_Code_Alt);
    Bind(interactive_kill_buffer,       Key_Code_K, Key_Code_Control);
    Bind(interactive_switch_buffer,     Key_Code_I, Key_Code_Control);
    Bind(project_go_to_root_directory,  Key_Code_H, Key_Code_Control);
    Bind(save_all_dirty_buffers,        Key_Code_S, Key_Code_Control, Key_Code_Shift);
    Bind(goto_next_jump,                Key_Code_N, Key_Code_Alt);
    Bind(goto_prev_jump,                Key_Code_N, Key_Code_Alt, Key_Code_Shift);
    Bind(goto_first_jump,               Key_Code_M, Key_Code_Alt, Key_Code_Shift);
    Bind(toggle_filebar,                Key_Code_B, Key_Code_Alt);
    Bind(execute_any_cli,               Key_Code_Z, Key_Code_Alt);
    Bind(execute_previous_cli,          Key_Code_Z, Key_Code_Alt, Key_Code_Shift);
    Bind(command_lister,                Key_Code_X, Key_Code_Alt);
    Bind(project_command_lister,        Key_Code_X, Key_Code_Alt, Key_Code_Shift);
    Bind(list_all_functions_current_buffer_lister, Key_Code_I, Key_Code_Control, Key_Code_Shift);
    Bind(project_fkey_command, Key_Code_F1);
    Bind(project_fkey_command, Key_Code_F2);
    Bind(project_fkey_command, Key_Code_F3);
    Bind(project_fkey_command, Key_Code_F4);
    Bind(project_fkey_command, Key_Code_F5);
    Bind(project_fkey_command, Key_Code_F6);
    Bind(project_fkey_command, Key_Code_F7);
    Bind(project_fkey_command, Key_Code_F8);
    Bind(project_fkey_command, Key_Code_F9);
    Bind(project_fkey_command, Key_Code_F10);
    Bind(project_fkey_command, Key_Code_F11);
    Bind(project_fkey_command, Key_Code_F12);
    Bind(project_fkey_command, Key_Code_F13);
    Bind(project_fkey_command, Key_Code_F14);
    Bind(project_fkey_command, Key_Code_F15);
    Bind(project_fkey_command, Key_Code_F16);
    
    // NOTE(rjf): Custom bindings.
    {
        Bind(open_panel_vsplit, Key_Code_P, Key_Code_Control);
        Bind(open_panel_hsplit, Key_Code_Minus, Key_Code_Control);
        Bind(close_panel, Key_Code_P, Key_Code_Control, Key_Code_Shift);
        Bind(f4_search_for_definition__project_wide, Key_Code_J, Key_Code_Control);
        Bind(f4_search_for_definition__current_file, Key_Code_J, Key_Code_Control, Key_Code_Shift);
        Bind(fleury_toggle_battery_saver, Key_Code_Tick, Key_Code_Alt);
        Bind(move_right_token_boundary, Key_Code_Right, Key_Code_Shift, Key_Code_Control);
        Bind(move_left_token_boundary, Key_Code_Left, Key_Code_Shift, Key_Code_Control);
    }
    
    SelectMap(file_map_id);
    ParentMap(global_map_id);
    Bind(delete_char,            Key_Code_Delete);
    Bind(backspace_char,         Key_Code_Backspace);
    Bind(move_up,                Key_Code_Up);
    Bind(move_down,              Key_Code_Down);
    Bind(move_left,              Key_Code_Left);
    Bind(move_right,             Key_Code_Right);
    Bind(seek_end_of_line,       Key_Code_End);
    Bind(fleury_home,            Key_Code_Home);
    Bind(page_up,                Key_Code_PageUp);
    Bind(page_down,              Key_Code_PageDown);
    Bind(goto_beginning_of_file, Key_Code_PageUp, Key_Code_Control);
    Bind(goto_end_of_file,       Key_Code_PageDown, Key_Code_Control);
    Bind(move_up_to_blank_line_end,        Key_Code_Up, Key_Code_Control);
    Bind(move_down_to_blank_line_end,      Key_Code_Down, Key_Code_Control);
    Bind(move_left_whitespace_boundary,    Key_Code_Left, Key_Code_Control);
    Bind(move_right_whitespace_boundary,   Key_Code_Right, Key_Code_Control);
    Bind(move_line_up,                     Key_Code_Up, Key_Code_Alt);
    Bind(move_line_down,                   Key_Code_Down, Key_Code_Alt);
    Bind(backspace_alnum_boundary, Key_Code_Backspace, Key_Code_Control);
    Bind(delete_alnum_boundary,    Key_Code_Delete, Key_Code_Control);
    Bind(snipe_backward_whitespace_or_token_boundary, Key_Code_Backspace, Key_Code_Alt);
    Bind(snipe_forward_whitespace_or_token_boundary,  Key_Code_Delete, Key_Code_Alt);
    Bind(set_mark,                    Key_Code_Space, Key_Code_Control);
    Bind(replace_in_range,            Key_Code_A, Key_Code_Control);
    Bind(copy,                        Key_Code_C, Key_Code_Control);
    Bind(delete_range,                Key_Code_D, Key_Code_Control);
    Bind(delete_line,                 Key_Code_D, Key_Code_Control, Key_Code_Shift);
    Bind(center_view,                 Key_Code_E, Key_Code_Control);
    Bind(left_adjust_view,            Key_Code_E, Key_Code_Control, Key_Code_Shift);
    Bind(search,                      Key_Code_F, Key_Code_Control);
    Bind(list_all_locations,          Key_Code_F, Key_Code_Control, Key_Code_Shift);
    Bind(list_all_substring_locations_case_insensitive, Key_Code_F, Key_Code_Alt);
    Bind(goto_line,                   Key_Code_G, Key_Code_Control);
    Bind(list_all_locations_of_selection,  Key_Code_G, Key_Code_Control, Key_Code_Shift);
    Bind(kill_buffer,                 Key_Code_K, Key_Code_Control, Key_Code_Shift);
    Bind(duplicate_line,              Key_Code_L, Key_Code_Control);
    Bind(cursor_mark_swap,            Key_Code_M, Key_Code_Control);
    Bind(reopen,                      Key_Code_O, Key_Code_Control, Key_Code_Shift);
    Bind(query_replace,               Key_Code_Q, Key_Code_Control);
    Bind(query_replace_identifier,    Key_Code_Q, Key_Code_Control, Key_Code_Shift);
    Bind(query_replace_selection,     Key_Code_Q, Key_Code_Alt);
    Bind(reverse_search,              Key_Code_R, Key_Code_Control);
    Bind(save,                        Key_Code_S, Key_Code_Control);
    Bind(save_all_dirty_buffers,      Key_Code_S, Key_Code_Control, Key_Code_Shift);
    Bind(search_identifier,           Key_Code_T, Key_Code_Control);
    Bind(list_all_locations_of_identifier, Key_Code_T, Key_Code_Control, Key_Code_Shift);
    Bind(paste_and_indent,            Key_Code_V, Key_Code_Control);
    Bind(paste_next_and_indent,       Key_Code_V, Key_Code_Control, Key_Code_Shift);
    Bind(cut,                         Key_Code_X, Key_Code_Control);
    Bind(redo,                        Key_Code_Y, Key_Code_Control);
    Bind(undo,                        Key_Code_Z, Key_Code_Control);
    Bind(view_buffer_other_panel,     Key_Code_1, Key_Code_Control);
    Bind(swap_panels,                 Key_Code_2, Key_Code_Control);
    Bind(if_read_only_goto_position,  Key_Code_Return);
    Bind(if_read_only_goto_position_same_panel, Key_Code_Return, Key_Code_Shift);
    Bind(view_jump_list_with_lister,  Key_Code_Period, Key_Code_Control, Key_Code_Shift);
    
    // NOTE(rjf): Custom bindings.
    {
        Bind(fleury_write_zero_struct,  Key_Code_0, Key_Code_Control);
        Bind(move_right_token_boundary, Key_Code_Right, Key_Code_Shift, Key_Code_Control);
        Bind(move_left_token_boundary, Key_Code_Left, Key_Code_Shift, Key_Code_Control);
    }
    
    SelectMap(code_map_id);
    ParentMap(file_map_id);
    BindTextInput(fleury_write_text_and_auto_indent);
    Bind(move_left_alnum_boundary,           Key_Code_Left, Key_Code_Control);
    Bind(move_right_alnum_boundary,          Key_Code_Right, Key_Code_Control);
    Bind(move_left_alnum_or_camel_boundary,  Key_Code_Left, Key_Code_Alt);
    Bind(move_right_alnum_or_camel_boundary, Key_Code_Right, Key_Code_Alt);
    Bind(comment_line_toggle,        Key_Code_Semicolon, Key_Code_Control);
    Bind(word_complete,              Key_Code_Tab);
    Bind(auto_indent_range,          Key_Code_Tab, Key_Code_Control);
    Bind(auto_indent_line_at_cursor, Key_Code_Tab, Key_Code_Shift);
    Bind(word_complete_drop_down,    Key_Code_Tab, Key_Code_Shift, Key_Code_Control);
    Bind(write_block,                Key_Code_R, Key_Code_Alt);
    Bind(write_todo,                 Key_Code_T, Key_Code_Alt);
    Bind(write_note,                 Key_Code_Y, Key_Code_Alt);
    Bind(list_all_locations_of_type_definition,               Key_Code_D, Key_Code_Alt);
    Bind(list_all_locations_of_type_definition_of_identifier, Key_Code_T, Key_Code_Alt, Key_Code_Shift);
    Bind(open_long_braces,           Key_Code_LeftBracket, Key_Code_Control);
    Bind(open_long_braces_semicolon, Key_Code_LeftBracket, Key_Code_Control, Key_Code_Shift);
    Bind(open_long_braces_break,     Key_Code_RightBracket, Key_Code_Control, Key_Code_Shift);
    Bind(select_surrounding_scope,   Key_Code_LeftBracket, Key_Code_Alt);
    Bind(select_surrounding_scope_maximal, Key_Code_LeftBracket, Key_Code_Alt, Key_Code_Shift);
    Bind(select_prev_scope_absolute, Key_Code_RightBracket, Key_Code_Alt);
    Bind(select_prev_top_most_scope, Key_Code_RightBracket, Key_Code_Alt, Key_Code_Shift);
    Bind(select_next_scope_absolute, Key_Code_Quote, Key_Code_Alt);
    Bind(select_next_scope_after_current, Key_Code_Quote, Key_Code_Alt, Key_Code_Shift);
    Bind(place_in_scope,             Key_Code_ForwardSlash, Key_Code_Alt);
    Bind(delete_current_scope,       Key_Code_Minus, Key_Code_Alt);
    Bind(if0_off,                    Key_Code_I, Key_Code_Alt);
    Bind(open_file_in_quotes,        Key_Code_1, Key_Code_Alt);
    Bind(open_matching_file_cpp,     Key_Code_2, Key_Code_Alt);
    
}
