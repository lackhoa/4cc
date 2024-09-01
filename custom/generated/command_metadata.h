#if !defined(META_PASS)
#define command_id(c) (fcoder_metacmd_ID_##c)
#define command_metadata(c) (&fcoder_metacmd_table[command_id(c)])
#define command_metadata_by_id(id) (&fcoder_metacmd_table[id])
#define command_one_past_last_id 233
#if defined(CUSTOM_COMMAND_SIG)
#define PROC_LINKS(x,y) x
#else
#define PROC_LINKS(x,y) y
#endif
#if defined(CUSTOM_COMMAND_SIG)
CUSTOM_COMMAND_SIG(DEBUG_draw_hud_toggle);
CUSTOM_COMMAND_SIG(allow_mouse);
CUSTOM_COMMAND_SIG(are_we_in_debug_build);
CUSTOM_COMMAND_SIG(backspace_char);
CUSTOM_COMMAND_SIG(byp_reset_face_size);
CUSTOM_COMMAND_SIG(center_view);
CUSTOM_COMMAND_SIG(clean_all_lines);
CUSTOM_COMMAND_SIG(clean_trailing_whitespace);
CUSTOM_COMMAND_SIG(clear_all_themes);
CUSTOM_COMMAND_SIG(click_set_cursor);
CUSTOM_COMMAND_SIG(click_set_cursor_and_mark);
CUSTOM_COMMAND_SIG(click_set_cursor_if_lbutton);
CUSTOM_COMMAND_SIG(click_set_mark);
CUSTOM_COMMAND_SIG(clipboard_record_clip);
CUSTOM_COMMAND_SIG(close_all_code);
CUSTOM_COMMAND_SIG(command_lister);
CUSTOM_COMMAND_SIG(comment_line);
CUSTOM_COMMAND_SIG(comment_line_toggle);
CUSTOM_COMMAND_SIG(copy);
CUSTOM_COMMAND_SIG(cursor_mark_swap);
CUSTOM_COMMAND_SIG(cut);
CUSTOM_COMMAND_SIG(debug_camera_on);
CUSTOM_COMMAND_SIG(decrease_face_size);
CUSTOM_COMMAND_SIG(default_try_exit);
CUSTOM_COMMAND_SIG(default_view_input_handler);
CUSTOM_COMMAND_SIG(delete_char);
CUSTOM_COMMAND_SIG(delete_current_scope);
CUSTOM_COMMAND_SIG(delete_file_query);
CUSTOM_COMMAND_SIG(delete_line);
CUSTOM_COMMAND_SIG(dir);
CUSTOM_COMMAND_SIG(display_key_codes);
CUSTOM_COMMAND_SIG(display_text_input);
CUSTOM_COMMAND_SIG(double_backspace);
CUSTOM_COMMAND_SIG(dump_history_to_file);
CUSTOM_COMMAND_SIG(duplicate_line);
CUSTOM_COMMAND_SIG(execute_any_cli);
CUSTOM_COMMAND_SIG(execute_previous_cli);
CUSTOM_COMMAND_SIG(exit_4coder);
CUSTOM_COMMAND_SIG(file);
CUSTOM_COMMAND_SIG(fold_clear);
CUSTOM_COMMAND_SIG(fold_pop_cursor);
CUSTOM_COMMAND_SIG(fold_range);
CUSTOM_COMMAND_SIG(fold_toggle_cursor);
CUSTOM_COMMAND_SIG(game_disable);
CUSTOM_COMMAND_SIG(game_enable);
CUSTOM_COMMAND_SIG(go_to_user_directory);
CUSTOM_COMMAND_SIG(goto_beginning_of_file);
CUSTOM_COMMAND_SIG(goto_end_of_file);
CUSTOM_COMMAND_SIG(goto_first_jump);
CUSTOM_COMMAND_SIG(goto_first_jump_same_panel_sticky);
CUSTOM_COMMAND_SIG(goto_line);
CUSTOM_COMMAND_SIG(goto_next_jump_no_skips);
CUSTOM_COMMAND_SIG(goto_prev_jump_no_skips);
CUSTOM_COMMAND_SIG(hide_filebar);
CUSTOM_COMMAND_SIG(hide_scrollbar);
CUSTOM_COMMAND_SIG(if0_off);
CUSTOM_COMMAND_SIG(increase_face_size);
CUSTOM_COMMAND_SIG(init);
CUSTOM_COMMAND_SIG(interactive_kill_buffer);
CUSTOM_COMMAND_SIG(interactive_new);
CUSTOM_COMMAND_SIG(interactive_open);
CUSTOM_COMMAND_SIG(interactive_open_or_new);
CUSTOM_COMMAND_SIG(interactive_switch_buffer);
CUSTOM_COMMAND_SIG(jump_to_last_point);
CUSTOM_COMMAND_SIG(keyboard_macro_finish_recording);
CUSTOM_COMMAND_SIG(keyboard_macro_replay);
CUSTOM_COMMAND_SIG(keyboard_macro_start_recording);
CUSTOM_COMMAND_SIG(kill_buffer);
CUSTOM_COMMAND_SIG(kv_miscellaneous_debug_command);
CUSTOM_COMMAND_SIG(kv_open_note_file);
CUSTOM_COMMAND_SIG(kv_profile_disable_and_inspect);
CUSTOM_COMMAND_SIG(kv_reopen_with_confirmation);
CUSTOM_COMMAND_SIG(kv_run);
CUSTOM_COMMAND_SIG(left_adjust_view);
CUSTOM_COMMAND_SIG(list_all_functions_all_buffers);
CUSTOM_COMMAND_SIG(list_all_functions_all_buffers_lister);
CUSTOM_COMMAND_SIG(list_all_functions_current_buffer);
CUSTOM_COMMAND_SIG(list_all_functions_current_buffer_lister);
CUSTOM_COMMAND_SIG(list_all_locations);
CUSTOM_COMMAND_SIG(list_all_locations_case_insensitive);
CUSTOM_COMMAND_SIG(list_all_substring_locations);
CUSTOM_COMMAND_SIG(list_all_substring_locations_case_insensitive);
CUSTOM_COMMAND_SIG(load_project);
CUSTOM_COMMAND_SIG(load_project_current_dir);
CUSTOM_COMMAND_SIG(load_theme_current_buffer);
CUSTOM_COMMAND_SIG(load_themes_default_folder);
CUSTOM_COMMAND_SIG(load_themes_hot_directory);
CUSTOM_COMMAND_SIG(make_directory_query);
CUSTOM_COMMAND_SIG(messages);
CUSTOM_COMMAND_SIG(miblo_decrement_basic);
CUSTOM_COMMAND_SIG(miblo_decrement_time_stamp);
CUSTOM_COMMAND_SIG(miblo_decrement_time_stamp_minute);
CUSTOM_COMMAND_SIG(miblo_increment_basic);
CUSTOM_COMMAND_SIG(miblo_increment_time_stamp);
CUSTOM_COMMAND_SIG(miblo_increment_time_stamp_minute);
CUSTOM_COMMAND_SIG(mouse_wheel_change_face_size);
CUSTOM_COMMAND_SIG(mouse_wheel_scroll);
CUSTOM_COMMAND_SIG(move_down);
CUSTOM_COMMAND_SIG(move_down_10);
CUSTOM_COMMAND_SIG(move_down_textual);
CUSTOM_COMMAND_SIG(move_down_to_blank_line);
CUSTOM_COMMAND_SIG(move_down_to_blank_line_end);
CUSTOM_COMMAND_SIG(move_down_to_blank_line_skip_whitespace);
CUSTOM_COMMAND_SIG(move_left);
CUSTOM_COMMAND_SIG(move_line_down);
CUSTOM_COMMAND_SIG(move_line_up);
CUSTOM_COMMAND_SIG(move_right);
CUSTOM_COMMAND_SIG(move_up);
CUSTOM_COMMAND_SIG(move_up_10);
CUSTOM_COMMAND_SIG(move_up_to_blank_line);
CUSTOM_COMMAND_SIG(move_up_to_blank_line_end);
CUSTOM_COMMAND_SIG(move_up_to_blank_line_skip_whitespace);
CUSTOM_COMMAND_SIG(multi_paste);
CUSTOM_COMMAND_SIG(multi_paste_interactive);
CUSTOM_COMMAND_SIG(multi_paste_interactive_quick);
CUSTOM_COMMAND_SIG(no_op);
CUSTOM_COMMAND_SIG(open_all_code);
CUSTOM_COMMAND_SIG(open_all_code_recursive);
CUSTOM_COMMAND_SIG(open_file_in_quotes);
CUSTOM_COMMAND_SIG(open_long_braces);
CUSTOM_COMMAND_SIG(open_long_braces_break);
CUSTOM_COMMAND_SIG(open_long_braces_semicolon);
CUSTOM_COMMAND_SIG(open_matching_file_cpp);
CUSTOM_COMMAND_SIG(open_matching_file_cpp_other_panel);
CUSTOM_COMMAND_SIG(page_down);
CUSTOM_COMMAND_SIG(page_up);
CUSTOM_COMMAND_SIG(paste);
CUSTOM_COMMAND_SIG(paste_next);
CUSTOM_COMMAND_SIG(place_in_scope);
CUSTOM_COMMAND_SIG(play_with_a_counter);
CUSTOM_COMMAND_SIG(profile_clear);
CUSTOM_COMMAND_SIG(profile_enable);
CUSTOM_COMMAND_SIG(profile_inspect);
CUSTOM_COMMAND_SIG(project_command_F1);
CUSTOM_COMMAND_SIG(project_command_F10);
CUSTOM_COMMAND_SIG(project_command_F11);
CUSTOM_COMMAND_SIG(project_command_F12);
CUSTOM_COMMAND_SIG(project_command_F13);
CUSTOM_COMMAND_SIG(project_command_F14);
CUSTOM_COMMAND_SIG(project_command_F15);
CUSTOM_COMMAND_SIG(project_command_F16);
CUSTOM_COMMAND_SIG(project_command_F2);
CUSTOM_COMMAND_SIG(project_command_F3);
CUSTOM_COMMAND_SIG(project_command_F4);
CUSTOM_COMMAND_SIG(project_command_F5);
CUSTOM_COMMAND_SIG(project_command_F6);
CUSTOM_COMMAND_SIG(project_command_F7);
CUSTOM_COMMAND_SIG(project_command_F8);
CUSTOM_COMMAND_SIG(project_command_F9);
CUSTOM_COMMAND_SIG(project_command_lister);
CUSTOM_COMMAND_SIG(project_fkey_command);
CUSTOM_COMMAND_SIG(project_go_to_root_directory);
CUSTOM_COMMAND_SIG(project_reprint);
CUSTOM_COMMAND_SIG(query_replace);
CUSTOM_COMMAND_SIG(query_replace_identifier);
CUSTOM_COMMAND_SIG(query_replace_selection);
CUSTOM_COMMAND_SIG(quick_swap_buffer);
CUSTOM_COMMAND_SIG(redo_all_buffers);
CUSTOM_COMMAND_SIG(reg);
CUSTOM_COMMAND_SIG(rename_file_query);
CUSTOM_COMMAND_SIG(reopen);
CUSTOM_COMMAND_SIG(replace_in_all_buffers);
CUSTOM_COMMAND_SIG(replace_in_buffer);
CUSTOM_COMMAND_SIG(replace_in_range);
CUSTOM_COMMAND_SIG(reverse_search);
CUSTOM_COMMAND_SIG(reverse_search_identifier);
CUSTOM_COMMAND_SIG(right_adjust_view);
CUSTOM_COMMAND_SIG(save_all_dirty_buffers);
CUSTOM_COMMAND_SIG(save_to_query);
CUSTOM_COMMAND_SIG(scratch);
CUSTOM_COMMAND_SIG(search);
CUSTOM_COMMAND_SIG(search_identifier);
CUSTOM_COMMAND_SIG(seek_beginning_of_textual_line);
CUSTOM_COMMAND_SIG(seek_end_of_textual_line);
CUSTOM_COMMAND_SIG(select_all);
CUSTOM_COMMAND_SIG(select_next_scope_absolute);
CUSTOM_COMMAND_SIG(select_next_scope_after_current);
CUSTOM_COMMAND_SIG(select_prev_scope_absolute);
CUSTOM_COMMAND_SIG(select_prev_top_most_scope);
CUSTOM_COMMAND_SIG(select_surrounding_scope);
CUSTOM_COMMAND_SIG(select_surrounding_scope_maximal);
CUSTOM_COMMAND_SIG(set_current_dir_as_hot);
CUSTOM_COMMAND_SIG(set_eol_mode_from_contents);
CUSTOM_COMMAND_SIG(set_eol_mode_to_binary);
CUSTOM_COMMAND_SIG(set_eol_mode_to_crlf);
CUSTOM_COMMAND_SIG(set_eol_mode_to_lf);
CUSTOM_COMMAND_SIG(set_face_size);
CUSTOM_COMMAND_SIG(set_face_size_this_buffer);
CUSTOM_COMMAND_SIG(set_mark);
CUSTOM_COMMAND_SIG(set_mode_to_notepad_like);
CUSTOM_COMMAND_SIG(set_mode_to_original);
CUSTOM_COMMAND_SIG(show_filebar);
CUSTOM_COMMAND_SIG(show_scrollbar);
CUSTOM_COMMAND_SIG(show_the_log_graph);
CUSTOM_COMMAND_SIG(snippet_lister);
CUSTOM_COMMAND_SIG(suppress_mouse);
CUSTOM_COMMAND_SIG(swap_panels);
CUSTOM_COMMAND_SIG(switch_to_game_panel);
CUSTOM_COMMAND_SIG(theme_lister);
CUSTOM_COMMAND_SIG(toggle_filebar);
CUSTOM_COMMAND_SIG(toggle_fps_meter);
CUSTOM_COMMAND_SIG(toggle_fullscreen);
CUSTOM_COMMAND_SIG(toggle_highlight_enclosing_scopes);
CUSTOM_COMMAND_SIG(toggle_highlight_line_at_cursor);
CUSTOM_COMMAND_SIG(toggle_line_numbers);
CUSTOM_COMMAND_SIG(toggle_line_wrap);
CUSTOM_COMMAND_SIG(toggle_mouse);
CUSTOM_COMMAND_SIG(toggle_paren_matching_helper);
CUSTOM_COMMAND_SIG(toggle_show_whitespace);
CUSTOM_COMMAND_SIG(toggle_virtual_whitespace);
CUSTOM_COMMAND_SIG(uncomment_line);
CUSTOM_COMMAND_SIG(undo_all_buffers);
CUSTOM_COMMAND_SIG(view_jump_list_with_lister);
CUSTOM_COMMAND_SIG(vim_dec_buffer_peek);
CUSTOM_COMMAND_SIG(vim_inc_buffer_peek);
CUSTOM_COMMAND_SIG(vim_interactive_open_or_new);
CUSTOM_COMMAND_SIG(vim_jump_lister);
CUSTOM_COMMAND_SIG(vim_list_all_functions_current_buffer_lister);
CUSTOM_COMMAND_SIG(vim_proj_cmd_lister);
CUSTOM_COMMAND_SIG(vim_scoll_buffer_peek_down);
CUSTOM_COMMAND_SIG(vim_scoll_buffer_peek_up);
CUSTOM_COMMAND_SIG(vim_switch_lister);
CUSTOM_COMMAND_SIG(vim_theme_lister);
CUSTOM_COMMAND_SIG(vim_toggle_show_buffer_peek);
CUSTOM_COMMAND_SIG(vim_try_exit);
CUSTOM_COMMAND_SIG(word_complete);
CUSTOM_COMMAND_SIG(word_complete_drop_down);
CUSTOM_COMMAND_SIG(write_block);
CUSTOM_COMMAND_SIG(write_hack);
CUSTOM_COMMAND_SIG(write_note);
CUSTOM_COMMAND_SIG(write_text_and_auto_indent);
CUSTOM_COMMAND_SIG(write_todo);
CUSTOM_COMMAND_SIG(write_zero_struct);
#endif
struct Command_Metadata{
PROC_LINKS(Custom_Command_Function, void) *proc;
b32 is_ui;
char *name;
i1 name_len;
char *description;
i1 description_len;
char *source_name;
i1 source_name_len;
i1 line_number;
};
static Command_Metadata fcoder_metacmd_table[233] = {
{ PROC_LINKS(DEBUG_draw_hud_toggle, 0), false, "DEBUG_draw_hud_toggle", 21, "toggle debug hud", 16, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4coder_kv_debug.cpp", 53, 47 },
{ PROC_LINKS(allow_mouse, 0), false, "allow_mouse", 11, "Shows the mouse and causes all mouse input to be processed normally.", 68, "C:\\Users\\vodan\\4ed/code/custom/4coder_default_framework.cpp", 59, 526 },
{ PROC_LINKS(are_we_in_debug_build, 0), false, "are_we_in_debug_build", 21, "", 0, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4coder_kv_commands.cpp", 56, 1225 },
{ PROC_LINKS(backspace_char, 0), false, "backspace_char", 14, "Deletes the character to the left of the cursor.", 48, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 98 },
{ PROC_LINKS(byp_reset_face_size, 0), false, "byp_reset_face_size", 19, "Resets face size to default", 27, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4coder_kv_commands.cpp", 56, 110 },
{ PROC_LINKS(center_view, 0), false, "center_view", 11, "Centers the view vertically on the line on which the cursor sits.", 65, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 139 },
{ PROC_LINKS(clean_all_lines, 0), false, "clean_all_lines", 15, "Removes trailing whitespace from all lines and removes all blank lines in the current buffer.", 93, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 499 },
{ PROC_LINKS(clean_trailing_whitespace, 0), false, "clean_trailing_whitespace", 25, "Removes trailing whitespace from all lines in the current buffer.", 65, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 508 },
{ PROC_LINKS(clear_all_themes, 0), false, "clear_all_themes", 16, "Clear the theme list", 20, "C:\\Users\\vodan\\4ed/code/custom/4coder_default_framework.cpp", 59, 610 },
{ PROC_LINKS(click_set_cursor, 0), false, "click_set_cursor", 16, "Sets the cursor position to the mouse position.", 47, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 193 },
{ PROC_LINKS(click_set_cursor_and_mark, 0), false, "click_set_cursor_and_mark", 25, "Sets the cursor position and mark to the mouse position.", 56, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 179 },
{ PROC_LINKS(click_set_cursor_if_lbutton, 0), false, "click_set_cursor_if_lbutton", 27, "If the mouse left button is pressed, sets the cursor position to the mouse position.", 84, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 203 },
{ PROC_LINKS(click_set_mark, 0), false, "click_set_mark", 14, "Sets the mark position to the mouse position.", 45, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 216 },
{ PROC_LINKS(clipboard_record_clip, 0), false, "clipboard_record_clip", 21, "In response to a new clipboard contents events, saves the new clip onto the clipboard history", 93, "C:\\Users\\vodan\\4ed/code/custom/4coder_clipboard.cpp", 51, 7 },
{ PROC_LINKS(close_all_code, 0), false, "close_all_code", 14, "Closes any buffer with a filename ending with an extension configured to be recognized as a code file type.", 107, "C:\\Users\\vodan\\4ed/code/custom/4coder_project_commands.cpp", 58, 544 },
{ PROC_LINKS(command_lister, 0), true, "command_lister", 14, "Opens an interactive list of all registered commands.", 53, "C:\\Users\\vodan\\4ed/code/custom/4coder_lists.cpp", 47, 773 },
{ PROC_LINKS(comment_line, 0), false, "comment_line", 12, "Insert '//' at the beginning of the line after leading whitespace.", 66, "C:\\Users\\vodan\\4ed/code/custom/4coder_combined_write_commands.cpp", 65, 125 },
{ PROC_LINKS(comment_line_toggle, 0), false, "comment_line_toggle", 19, "Turns uncommented lines into commented lines and vice versa for comments starting with '//'.", 92, "C:\\Users\\vodan\\4ed/code/custom/4coder_combined_write_commands.cpp", 65, 149 },
{ PROC_LINKS(copy, 0), false, "copy", 4, "Copy the text in the range from the cursor to the mark onto the clipboard.", 74, "C:\\Users\\vodan\\4ed/code/custom/4coder_clipboard.cpp", 51, 49 },
{ PROC_LINKS(cursor_mark_swap, 0), false, "cursor_mark_swap", 16, "Swaps the position of the cursor and the mark.", 46, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 126 },
{ PROC_LINKS(cut, 0), false, "cut", 3, "Cut the text in the range from the cursor to the mark onto the clipboard.", 73, "C:\\Users\\vodan\\4ed/code/custom/4coder_clipboard.cpp", 51, 58 },
{ PROC_LINKS(debug_camera_on, 0), false, "debug_camera_on", 15, "", 0, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4ed_kv_game_.cpp", 50, 76 },
{ PROC_LINKS(decrease_face_size, 0), false, "decrease_face_size", 18, "Decrease the size of the face used by the current buffer.", 57, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 605 },
{ PROC_LINKS(default_try_exit, 0), false, "default_try_exit", 16, "Default command for responding to a try-exit event", 50, "C:\\Users\\vodan\\4ed/code/custom/4coder_default_hooks.cpp", 55, 37 },
{ PROC_LINKS(default_view_input_handler, 0), false, "default_view_input_handler", 26, "Input consumption loop for default view behavior", 48, "C:\\Users\\vodan\\4ed/code/custom/4coder_default_hooks.cpp", 55, 82 },
{ PROC_LINKS(delete_char, 0), false, "delete_char", 11, "Deletes the character to the right of the cursor.", 49, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 81 },
{ PROC_LINKS(delete_current_scope, 0), false, "delete_current_scope", 20, "Deletes the braces surrounding the currently selected scope.  Leaves the contents within the scope.", 99, "C:\\Users\\vodan\\4ed/code/custom/4coder_scope_commands.cpp", 56, 112 },
{ PROC_LINKS(delete_file_query, 0), false, "delete_file_query", 17, "Deletes the file of the current buffer if 4coder has the appropriate access rights. Will ask the user for confirmation first.", 125, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 1247 },
{ PROC_LINKS(delete_line, 0), false, "delete_line", 11, "Delete the line the on which the cursor sits.", 45, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 1419 },
{ PROC_LINKS(dir, 0), false, "dir", 3, "kv copy dir name", 16, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4coder_kv_commands.cpp", 56, 871 },
{ PROC_LINKS(display_key_codes, 0), false, "display_key_codes", 17, "Example of input handling loop", 30, "C:\\Users\\vodan\\4ed/code/custom/4coder_examples.cpp", 50, 90 },
{ PROC_LINKS(display_text_input, 0), false, "display_text_input", 18, "Example of to_writable and leave_current_input_unhandled", 56, "C:\\Users\\vodan\\4ed/code/custom/4coder_examples.cpp", 50, 137 },
{ PROC_LINKS(double_backspace, 0), false, "double_backspace", 16, "Example of history group helpers", 32, "C:\\Users\\vodan\\4ed/code/custom/4coder_examples.cpp", 50, 10 },
{ PROC_LINKS(dump_history_to_file, 0), false, "dump_history_to_file", 20, "a utility to combat file corruption", 35, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4coder_kv_commands.cpp", 56, 1352 },
{ PROC_LINKS(duplicate_line, 0), false, "duplicate_line", 14, "Create a copy of the line on which the cursor sits.", 51, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 1405 },
{ PROC_LINKS(execute_any_cli, 0), false, "execute_any_cli", 15, "Queries for an output buffer name and system command, runs the system command as a CLI and prints the output to the specified buffer.", 133, "C:\\Users\\vodan\\4ed/code/custom/4coder_cli_command.cpp", 53, 22 },
{ PROC_LINKS(execute_previous_cli, 0), false, "execute_previous_cli", 20, "If the command execute_any_cli has already been used, this will execute a CLI reusing the most recent buffer name and command.", 126, "C:\\Users\\vodan\\4ed/code/custom/4coder_cli_command.cpp", 53, 7 },
{ PROC_LINKS(exit_4coder, 0), false, "exit_4coder", 11, "Attempts to close 4coder.", 25, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 694 },
{ PROC_LINKS(file, 0), false, "file", 4, "kv copy file name", 17, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4coder_kv_commands.cpp", 56, 865 },
{ PROC_LINKS(fold_clear, 0), false, "fold_clear", 10, "Clears all folds in buffer", 26, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4coder_vim/4coder_folds.hpp", 61, 132 },
{ PROC_LINKS(fold_pop_cursor, 0), false, "fold_pop_cursor", 15, "Pops fold at cursor", 19, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4coder_vim/4coder_folds.hpp", 61, 145 },
{ PROC_LINKS(fold_range, 0), false, "fold_range", 10, "Folds cursor mark range", 23, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4coder_vim/4coder_folds.hpp", 61, 161 },
{ PROC_LINKS(fold_toggle_cursor, 0), false, "fold_toggle_cursor", 18, "Toggles fold at cursor", 22, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4coder_vim/4coder_folds.hpp", 61, 153 },
{ PROC_LINKS(game_disable, 0), false, "game_disable", 12, "", 0, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4ed_kv_game_.cpp", 50, 68 },
{ PROC_LINKS(game_enable, 0), false, "game_enable", 11, "", 0, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4ed_kv_game_.cpp", 50, 62 },
{ PROC_LINKS(go_to_user_directory, 0), false, "go_to_user_directory", 20, "Go to the 4coder user directory", 31, "C:\\Users\\vodan\\4ed/code/custom/4coder_config.cpp", 48, 1721 },
{ PROC_LINKS(goto_beginning_of_file, 0), false, "goto_beginning_of_file", 22, "Sets the cursor to the beginning of the file.", 45, "C:\\Users\\vodan\\4ed/code/custom/4coder_helper.cpp", 48, 2363 },
{ PROC_LINKS(goto_end_of_file, 0), false, "goto_end_of_file", 16, "Sets the cursor to the end of the file.", 39, "C:\\Users\\vodan\\4ed/code/custom/4coder_helper.cpp", 48, 2371 },
{ PROC_LINKS(goto_first_jump, 0), false, "goto_first_jump", 15, "If a buffer containing jump locations has been locked in, goes to the first jump in the buffer.", 95, "C:\\Users\\vodan\\4ed/code/custom/4coder_jump_sticky.cpp", 53, 577 },
{ PROC_LINKS(goto_first_jump_same_panel_sticky, 0), false, "goto_first_jump_same_panel_sticky", 33, "If a buffer containing jump locations has been locked in, goes to the first jump in the buffer and views the buffer in the panel where the jump list was.", 153, "C:\\Users\\vodan\\4ed/code/custom/4coder_jump_sticky.cpp", 53, 594 },
{ PROC_LINKS(goto_line, 0), false, "goto_line", 9, "Queries the user for a number, and jumps the cursor to the corresponding line.", 78, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 702 },
{ PROC_LINKS(goto_next_jump_no_skips, 0), false, "goto_next_jump_no_skips", 23, "If a buffer containing jump locations has been locked in, goes to the next jump in the buffer, and does not skip sub jump locations.", 132, "C:\\Users\\vodan\\4ed/code/custom/4coder_jump_sticky.cpp", 53, 546 },
{ PROC_LINKS(goto_prev_jump_no_skips, 0), false, "goto_prev_jump_no_skips", 23, "If a buffer containing jump locations has been locked in, goes to the previous jump in the buffer, and does not skip sub jump locations.", 136, "C:\\Users\\vodan\\4ed/code/custom/4coder_jump_sticky.cpp", 53, 563 },
{ PROC_LINKS(hide_filebar, 0), false, "hide_filebar", 12, "Sets the current view to hide it's filebar.", 43, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 552 },
{ PROC_LINKS(hide_scrollbar, 0), false, "hide_scrollbar", 14, "Sets the current view to hide it's scrollbar.", 45, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 538 },
{ PROC_LINKS(if0_off, 0), false, "if0_off", 7, "Surround the range between the cursor and mark with an '#if 0' and an '#endif'", 78, "C:\\Users\\vodan\\4ed/code/custom/4coder_combined_write_commands.cpp", 65, 70 },
{ PROC_LINKS(increase_face_size, 0), false, "increase_face_size", 18, "Increase the size of the face used by the current buffer.", 57, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 594 },
{ PROC_LINKS(init, 0), false, "init", 4, "configure your editor!", 22, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4coder_kv_commands.cpp", 56, 999 },
{ PROC_LINKS(interactive_kill_buffer, 0), true, "interactive_kill_buffer", 23, "Interactively kill an open buffer.", 34, "C:\\Users\\vodan\\4ed/code/custom/4coder_lists.cpp", 47, 533 },
{ PROC_LINKS(interactive_new, 0), true, "interactive_new", 15, "Interactively creates a new file.", 33, "C:\\Users\\vodan\\4ed/code/custom/4coder_lists.cpp", 47, 673 },
{ PROC_LINKS(interactive_open, 0), true, "interactive_open", 16, "Interactively opens a file.", 27, "C:\\Users\\vodan\\4ed/code/custom/4coder_lists.cpp", 47, 727 },
{ PROC_LINKS(interactive_open_or_new, 0), true, "interactive_open_or_new", 23, "Interactively open a file out of the file system.", 49, "C:\\Users\\vodan\\4ed/code/custom/4coder_lists.cpp", 47, 624 },
{ PROC_LINKS(interactive_switch_buffer, 0), true, "interactive_switch_buffer", 25, "Interactively switch to an open buffer.", 39, "C:\\Users\\vodan\\4ed/code/custom/4coder_lists.cpp", 47, 523 },
{ PROC_LINKS(jump_to_last_point, 0), false, "jump_to_last_point", 18, "Read from the top of the point stack and jump there; if already there pop the top and go to the next option", 107, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 1199 },
{ PROC_LINKS(keyboard_macro_finish_recording, 0), false, "keyboard_macro_finish_recording", 31, "Stop macro recording, do nothing if macro recording is not already started", 74, "C:\\Users\\vodan\\4ed/code/custom/4coder_keyboard_macro.cpp", 56, 54 },
{ PROC_LINKS(keyboard_macro_replay, 0), false, "keyboard_macro_replay", 21, "Replay the most recently recorded keyboard macro", 48, "C:\\Users\\vodan\\4ed/code/custom/4coder_keyboard_macro.cpp", 56, 77 },
{ PROC_LINKS(keyboard_macro_start_recording, 0), false, "keyboard_macro_start_recording", 30, "Start macro recording, do nothing if macro recording is already started", 71, "C:\\Users\\vodan\\4ed/code/custom/4coder_keyboard_macro.cpp", 56, 41 },
{ PROC_LINKS(kill_buffer, 0), false, "kill_buffer", 11, "Kills the current buffer.", 25, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 1606 },
{ PROC_LINKS(kv_miscellaneous_debug_command, 0), false, "kv_miscellaneous_debug_command", 30, "just a placeholder command so I can test stuff", 46, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4coder_kv_commands.cpp", 56, 994 },
{ PROC_LINKS(kv_open_note_file, 0), false, "kv_open_note_file", 17, "switch to my note file", 22, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4coder_kv_commands.cpp", 56, 853 },
{ PROC_LINKS(kv_profile_disable_and_inspect, 0), false, "kv_profile_disable_and_inspect", 30, "disable and inspect profile", 27, "C:\\Users\\vodan\\4ed/code/custom/4coder_profile_commands.cpp", 58, 24 },
{ PROC_LINKS(kv_reopen_with_confirmation, 0), false, "kv_reopen_with_confirmation", 27, "Like reopen, but asks for confirmation", 38, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4coder_kv_commands.cpp", 56, 475 },
{ PROC_LINKS(kv_run, 0), false, "kv_run", 6, "run the current script", 22, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4coder_kv_commands.cpp", 56, 842 },
{ PROC_LINKS(left_adjust_view, 0), false, "left_adjust_view", 16, "Sets the left size of the view near the x position of the cursor.", 65, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 154 },
{ PROC_LINKS(list_all_functions_all_buffers, 0), false, "list_all_functions_all_buffers", 30, "Creates a jump list of lines from all buffers that appear to define or declare functions.", 89, "C:\\Users\\vodan\\4ed/code/custom/4coder_function_list.cpp", 55, 296 },
{ PROC_LINKS(list_all_functions_all_buffers_lister, 0), true, "list_all_functions_all_buffers_lister", 37, "Creates a lister of locations that look like function definitions and declarations all buffers.", 95, "C:\\Users\\vodan\\4ed/code/custom/4coder_function_list.cpp", 55, 302 },
{ PROC_LINKS(list_all_functions_current_buffer, 0), false, "list_all_functions_current_buffer", 33, "Creates a jump list of lines of the current buffer that appear to define or declare functions.", 94, "C:\\Users\\vodan\\4ed/code/custom/4coder_function_list.cpp", 55, 268 },
{ PROC_LINKS(list_all_functions_current_buffer_lister, 0), true, "list_all_functions_current_buffer_lister", 40, "Creates a lister of locations that look like function definitions and declarations in the buffer.", 97, "C:\\Users\\vodan\\4ed/code/custom/4coder_function_list.cpp", 55, 278 },
{ PROC_LINKS(list_all_locations, 0), false, "list_all_locations", 18, "Queries the user for a string and lists all exact case-sensitive matches found in all open buffers.", 99, "C:\\Users\\vodan\\4ed/code/custom/4coder_search.cpp", 48, 330 },
{ PROC_LINKS(list_all_locations_case_insensitive, 0), false, "list_all_locations_case_insensitive", 35, "Queries the user for a string and lists all exact case-insensitive matches found in all open buffers.", 101, "C:\\Users\\vodan\\4ed/code/custom/4coder_search.cpp", 48, 342 },
{ PROC_LINKS(list_all_substring_locations, 0), false, "list_all_substring_locations", 28, "Queries the user for a string and lists all case-sensitive substring matches found in all open buffers.", 103, "C:\\Users\\vodan\\4ed/code/custom/4coder_search.cpp", 48, 336 },
{ PROC_LINKS(list_all_substring_locations_case_insensitive, 0), false, "list_all_substring_locations_case_insensitive", 45, "Queries the user for a string and lists all case-insensitive substring matches found in all open buffers.", 105, "C:\\Users\\vodan\\4ed/code/custom/4coder_search.cpp", 48, 348 },
{ PROC_LINKS(load_project, 0), false, "load_project", 12, "Looks for a project.4coder file in the hot directory and tries to load it.  Looks in parent directories until a project file is found or there are no more parents.", 163, "C:\\Users\\vodan\\4ed/code/custom/4coder_project_commands.cpp", 58, 582 },
{ PROC_LINKS(load_project_current_dir, 0), false, "load_project_current_dir", 24, "Looks for a project.4coder file in the current directory and tries to load it.  Looks in parent directories until a project file is found or there are no more parents.", 167, "C:\\Users\\vodan\\4ed/code/custom/4coder_project_commands.cpp", 58, 732 },
{ PROC_LINKS(load_theme_current_buffer, 0), false, "load_theme_current_buffer", 25, "Parse the current buffer as a theme file and add the theme to the theme list. If the buffer has a .4coder postfix in it's name, it is removed when the name is saved.", 165, "C:\\Users\\vodan\\4ed/code/custom/4coder_config.cpp", 48, 1676 },
{ PROC_LINKS(load_themes_default_folder, 0), false, "load_themes_default_folder", 26, "Loads all the theme files in the default theme folder.", 54, "C:\\Users\\vodan\\4ed/code/custom/4coder_default_framework.cpp", 59, 580 },
{ PROC_LINKS(load_themes_hot_directory, 0), false, "load_themes_hot_directory", 25, "Loads all the theme files in the current hot directory.", 55, "C:\\Users\\vodan\\4ed/code/custom/4coder_default_framework.cpp", 59, 599 },
{ PROC_LINKS(make_directory_query, 0), false, "make_directory_query", 20, "Queries the user for a name and creates a new directory with the given name.", 76, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 1359 },
{ PROC_LINKS(messages, 0), false, "messages", 8, "switch to messages buffer", 25, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4coder_kv_commands.cpp", 56, 1097 },
{ PROC_LINKS(miblo_decrement_basic, 0), false, "miblo_decrement_basic", 21, "Decrement an integer under the cursor by one.", 45, "C:\\Users\\vodan\\4ed/code/custom/4coder_miblo_numbers.cpp", 55, 44 },
{ PROC_LINKS(miblo_decrement_time_stamp, 0), false, "miblo_decrement_time_stamp", 26, "Decrement a time stamp under the cursor by one second. (format [m]m:ss or h:mm:ss", 81, "C:\\Users\\vodan\\4ed/code/custom/4coder_miblo_numbers.cpp", 55, 237 },
{ PROC_LINKS(miblo_decrement_time_stamp_minute, 0), false, "miblo_decrement_time_stamp_minute", 33, "Decrement a time stamp under the cursor by one minute. (format [m]m:ss or h:mm:ss", 81, "C:\\Users\\vodan\\4ed/code/custom/4coder_miblo_numbers.cpp", 55, 249 },
{ PROC_LINKS(miblo_increment_basic, 0), false, "miblo_increment_basic", 21, "Increment an integer under the cursor by one.", 45, "C:\\Users\\vodan\\4ed/code/custom/4coder_miblo_numbers.cpp", 55, 29 },
{ PROC_LINKS(miblo_increment_time_stamp, 0), false, "miblo_increment_time_stamp", 26, "Increment a time stamp under the cursor by one second. (format [m]m:ss or h:mm:ss", 81, "C:\\Users\\vodan\\4ed/code/custom/4coder_miblo_numbers.cpp", 55, 231 },
{ PROC_LINKS(miblo_increment_time_stamp_minute, 0), false, "miblo_increment_time_stamp_minute", 33, "Increment a time stamp under the cursor by one minute. (format [m]m:ss or h:mm:ss", 81, "C:\\Users\\vodan\\4ed/code/custom/4coder_miblo_numbers.cpp", 55, 243 },
{ PROC_LINKS(mouse_wheel_change_face_size, 0), false, "mouse_wheel_change_face_size", 28, "Reads the state of the mouse wheel and uses it to either increase or decrease the face size.", 92, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 647 },
{ PROC_LINKS(mouse_wheel_scroll, 0), false, "mouse_wheel_scroll", 18, "Reads the scroll wheel value from the mouse state and scrolls accordingly.", 74, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 226 },
{ PROC_LINKS(move_down, 0), false, "move_down", 9, "Moves the cursor down one line.", 31, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 301 },
{ PROC_LINKS(move_down_10, 0), false, "move_down_10", 12, "Moves the cursor down ten lines.", 32, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 313 },
{ PROC_LINKS(move_down_textual, 0), false, "move_down_textual", 17, "Moves down to the next line of actual text, regardless of line wrapping.", 72, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 319 },
{ PROC_LINKS(move_down_to_blank_line, 0), false, "move_down_to_blank_line", 23, "Seeks the cursor down to the next blank line.", 45, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 372 },
{ PROC_LINKS(move_down_to_blank_line_end, 0), false, "move_down_to_blank_line_end", 27, "Seeks the cursor down to the next blank line and places it at the end of the line.", 82, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 396 },
{ PROC_LINKS(move_down_to_blank_line_skip_whitespace, 0), false, "move_down_to_blank_line_skip_whitespace", 39, "Seeks the cursor down to the next blank line and places it at the end of the line.", 82, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 384 },
{ PROC_LINKS(move_left, 0), false, "move_left", 9, "Moves the cursor one character to the left.", 43, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 402 },
{ PROC_LINKS(move_line_down, 0), false, "move_line_down", 14, "Swaps the line under the cursor with the line below it, and moves the cursor down with it.", 90, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 1399 },
{ PROC_LINKS(move_line_up, 0), false, "move_line_up", 12, "Swaps the line under the cursor with the line above it, and moves the cursor up with it.", 88, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 1393 },
{ PROC_LINKS(move_right, 0), false, "move_right", 10, "Moves the cursor one character to the right.", 44, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 410 },
{ PROC_LINKS(move_up, 0), false, "move_up", 7, "Moves the cursor up one line.", 29, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 295 },
{ PROC_LINKS(move_up_10, 0), false, "move_up_10", 10, "Moves the cursor up ten lines.", 30, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 307 },
{ PROC_LINKS(move_up_to_blank_line, 0), false, "move_up_to_blank_line", 21, "Seeks the cursor up to the next blank line.", 43, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 366 },
{ PROC_LINKS(move_up_to_blank_line_end, 0), false, "move_up_to_blank_line_end", 25, "Seeks the cursor up to the next blank line and places it at the end of the line.", 80, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 390 },
{ PROC_LINKS(move_up_to_blank_line_skip_whitespace, 0), false, "move_up_to_blank_line_skip_whitespace", 37, "Seeks the cursor up to the next blank line and places it at the end of the line.", 80, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 378 },
{ PROC_LINKS(multi_paste, 0), false, "multi_paste", 11, "Paste multiple entries from the clipboard at once", 49, "C:\\Users\\vodan\\4ed/code/custom/4coder_clipboard.cpp", 51, 153 },
{ PROC_LINKS(multi_paste_interactive, 0), false, "multi_paste_interactive", 23, "Paste multiple lines from the clipboard history, controlled with arrow keys", 75, "C:\\Users\\vodan\\4ed/code/custom/4coder_clipboard.cpp", 51, 297 },
{ PROC_LINKS(multi_paste_interactive_quick, 0), false, "multi_paste_interactive_quick", 29, "Paste multiple lines from the clipboard history, controlled by inputing the number of lines to paste", 100, "C:\\Users\\vodan\\4ed/code/custom/4coder_clipboard.cpp", 51, 306 },
{ PROC_LINKS(no_op, 0), false, "no_op", 5, "no op for binding keybinds to resolve without side effect", 57, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4coder_vim/4coder_vim_helper.cpp", 66, 5 },
{ PROC_LINKS(open_all_code, 0), false, "open_all_code", 13, "Open all code in the current directory. File types are determined by extensions. An extension is considered code based on the extensions specified in 4coder.config.", 164, "C:\\Users\\vodan\\4ed/code/custom/4coder_project_commands.cpp", 58, 553 },
{ PROC_LINKS(open_all_code_recursive, 0), false, "open_all_code_recursive", 23, "Works as open_all_code but also runs in all subdirectories.", 59, "C:\\Users\\vodan\\4ed/code/custom/4coder_project_commands.cpp", 58, 562 },
{ PROC_LINKS(open_file_in_quotes, 0), false, "open_file_in_quotes", 19, "Reads a filename from surrounding '\"' characters and attempts to open the corresponding file.", 94, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 1440 },
{ PROC_LINKS(open_long_braces, 0), false, "open_long_braces", 16, "At the cursor, insert a '{' and '}' separated by a blank line.", 62, "C:\\Users\\vodan\\4ed/code/custom/4coder_combined_write_commands.cpp", 65, 46 },
{ PROC_LINKS(open_long_braces_break, 0), false, "open_long_braces_break", 22, "At the cursor, insert a '{' and '}break;' separated by a blank line.", 68, "C:\\Users\\vodan\\4ed/code/custom/4coder_combined_write_commands.cpp", 65, 62 },
{ PROC_LINKS(open_long_braces_semicolon, 0), false, "open_long_braces_semicolon", 26, "At the cursor, insert a '{' and '};' separated by a blank line.", 63, "C:\\Users\\vodan\\4ed/code/custom/4coder_combined_write_commands.cpp", 65, 54 },
{ PROC_LINKS(open_matching_file_cpp, 0), false, "open_matching_file_cpp", 22, "If the current file is a *.cpp or *.h, attempts to open the corresponding *.h or *.cpp file.", 92, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 1527 },
{ PROC_LINKS(open_matching_file_cpp_other_panel, 0), false, "open_matching_file_cpp_other_panel", 34, "If the current file is a *.cpp or *.h, attempts to open the corresponding *.h or *.cpp file in the other view.", 110, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 1539 },
{ PROC_LINKS(page_down, 0), false, "page_down", 9, "Scrolls the view down one view height and moves the cursor down one view height.", 80, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 337 },
{ PROC_LINKS(page_up, 0), false, "page_up", 7, "Scrolls the view up one view height and moves the cursor up one view height.", 76, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 329 },
{ PROC_LINKS(paste, 0), false, "paste", 5, "At the cursor, insert the text at the top of the clipboard.", 59, "C:\\Users\\vodan\\4ed/code/custom/4coder_clipboard.cpp", 51, 69 },
{ PROC_LINKS(paste_next, 0), false, "paste_next", 10, "If the previous command was paste or paste_next, replaces the paste range with the next text down on the clipboard, otherwise operates as the paste command.", 156, "C:\\Users\\vodan\\4ed/code/custom/4coder_clipboard.cpp", 51, 106 },
{ PROC_LINKS(place_in_scope, 0), false, "place_in_scope", 14, "Wraps the code contained in the range between cursor and mark with a new curly brace scope.", 91, "C:\\Users\\vodan\\4ed/code/custom/4coder_scope_commands.cpp", 56, 106 },
{ PROC_LINKS(play_with_a_counter, 0), false, "play_with_a_counter", 19, "Example of query bar", 20, "C:\\Users\\vodan\\4ed/code/custom/4coder_examples.cpp", 50, 29 },
{ PROC_LINKS(profile_clear, 0), false, "profile_clear", 13, "Clear all profiling information from 4coder's self profiler.", 60, "C:\\Users\\vodan\\4ed/code/custom/4coder_profile_commands.cpp", 58, 17 },
{ PROC_LINKS(profile_enable, 0), false, "profile_enable", 14, "Allow 4coder's self profiler to gather new profiling information.", 65, "C:\\Users\\vodan\\4ed/code/custom/4coder_profile_commands.cpp", 58, 3 },
{ PROC_LINKS(profile_inspect, 0), true, "profile_inspect", 15, "Inspect all currently collected profiling information in 4coder's self profiler.", 80, "C:\\Users\\vodan\\4ed/code/custom/4coder_profile_inspect.cpp", 57, 904 },
{ PROC_LINKS(project_command_F1, 0), false, "project_command_F1", 18, "Run the command with index 1", 28, "C:\\Users\\vodan\\4ed/code/custom/4coder_project_commands.cpp", 58, 827 },
{ PROC_LINKS(project_command_F10, 0), false, "project_command_F10", 19, "Run the command with index 10", 29, "C:\\Users\\vodan\\4ed/code/custom/4coder_project_commands.cpp", 58, 881 },
{ PROC_LINKS(project_command_F11, 0), false, "project_command_F11", 19, "Run the command with index 11", 29, "C:\\Users\\vodan\\4ed/code/custom/4coder_project_commands.cpp", 58, 887 },
{ PROC_LINKS(project_command_F12, 0), false, "project_command_F12", 19, "Run the command with index 12", 29, "C:\\Users\\vodan\\4ed/code/custom/4coder_project_commands.cpp", 58, 893 },
{ PROC_LINKS(project_command_F13, 0), false, "project_command_F13", 19, "Run the command with index 13", 29, "C:\\Users\\vodan\\4ed/code/custom/4coder_project_commands.cpp", 58, 899 },
{ PROC_LINKS(project_command_F14, 0), false, "project_command_F14", 19, "Run the command with index 14", 29, "C:\\Users\\vodan\\4ed/code/custom/4coder_project_commands.cpp", 58, 905 },
{ PROC_LINKS(project_command_F15, 0), false, "project_command_F15", 19, "Run the command with index 15", 29, "C:\\Users\\vodan\\4ed/code/custom/4coder_project_commands.cpp", 58, 911 },
{ PROC_LINKS(project_command_F16, 0), false, "project_command_F16", 19, "Run the command with index 16", 29, "C:\\Users\\vodan\\4ed/code/custom/4coder_project_commands.cpp", 58, 917 },
{ PROC_LINKS(project_command_F2, 0), false, "project_command_F2", 18, "Run the command with index 2", 28, "C:\\Users\\vodan\\4ed/code/custom/4coder_project_commands.cpp", 58, 833 },
{ PROC_LINKS(project_command_F3, 0), false, "project_command_F3", 18, "Run the command with index 3", 28, "C:\\Users\\vodan\\4ed/code/custom/4coder_project_commands.cpp", 58, 839 },
{ PROC_LINKS(project_command_F4, 0), false, "project_command_F4", 18, "Run the command with index 4", 28, "C:\\Users\\vodan\\4ed/code/custom/4coder_project_commands.cpp", 58, 845 },
{ PROC_LINKS(project_command_F5, 0), false, "project_command_F5", 18, "Run the command with index 5", 28, "C:\\Users\\vodan\\4ed/code/custom/4coder_project_commands.cpp", 58, 851 },
{ PROC_LINKS(project_command_F6, 0), false, "project_command_F6", 18, "Run the command with index 6", 28, "C:\\Users\\vodan\\4ed/code/custom/4coder_project_commands.cpp", 58, 857 },
{ PROC_LINKS(project_command_F7, 0), false, "project_command_F7", 18, "Run the command with index 7", 28, "C:\\Users\\vodan\\4ed/code/custom/4coder_project_commands.cpp", 58, 863 },
{ PROC_LINKS(project_command_F8, 0), false, "project_command_F8", 18, "Run the command with index 8", 28, "C:\\Users\\vodan\\4ed/code/custom/4coder_project_commands.cpp", 58, 869 },
{ PROC_LINKS(project_command_F9, 0), false, "project_command_F9", 18, "Run the command with index 9", 28, "C:\\Users\\vodan\\4ed/code/custom/4coder_project_commands.cpp", 58, 875 },
{ PROC_LINKS(project_command_lister, 0), false, "project_command_lister", 22, "Open a lister of all commands in the currently loaded project.", 62, "C:\\Users\\vodan\\4ed/code/custom/4coder_project_commands.cpp", 58, 779 },
{ PROC_LINKS(project_fkey_command, 0), false, "project_fkey_command", 20, "Run an 'fkey command' configured in a project.4coder file.  Determines the index of the 'fkey command' by which function key or numeric key was pressed to trigger the command.", 175, "C:\\Users\\vodan\\4ed/code/custom/4coder_project_commands.cpp", 58, 742 },
{ PROC_LINKS(project_go_to_root_directory, 0), false, "project_go_to_root_directory", 28, "Changes 4coder's hot directory to the root directory of the currently loaded project. With no loaded project nothing hapepns.", 125, "C:\\Users\\vodan\\4ed/code/custom/4coder_project_commands.cpp", 58, 768 },
{ PROC_LINKS(project_reprint, 0), false, "project_reprint", 15, "Prints the current project to the file it was loaded from; prints in the most recent project file version", 105, "C:\\Users\\vodan\\4ed/code/custom/4coder_project_commands.cpp", 58, 789 },
{ PROC_LINKS(query_replace, 0), false, "query_replace", 13, "Queries the user for two strings, and incrementally replaces every occurence of the first string with the second string.", 120, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 1145 },
{ PROC_LINKS(query_replace_identifier, 0), false, "query_replace_identifier", 24, "Queries the user for a string, and incrementally replace every occurence of the word or token found at the cursor with the specified string.", 140, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 1166 },
{ PROC_LINKS(query_replace_selection, 0), false, "query_replace_selection", 23, "Queries the user for a string, and incrementally replace every occurence of the string found in the selected range with the specified string.", 141, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 1182 },
{ PROC_LINKS(quick_swap_buffer, 0), false, "quick_swap_buffer", 17, "Change to the most recently used buffer in this view - or to the top of the buffer stack if the most recent doesn't exist anymore", 129, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 1586 },
{ PROC_LINKS(redo_all_buffers, 0), false, "redo_all_buffers", 16, "Advances forward through the undo history in the buffer containing the most recent regular edit.", 96, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 1871 },
{ PROC_LINKS(reg, 0), false, "reg", 3, "Vim: Display registers", 22, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4coder_vim/4coder_vim_commands.cpp", 68, 76 },
{ PROC_LINKS(rename_file_query, 0), false, "rename_file_query", 17, "Queries the user for a new name and renames the file of the current buffer, altering the buffer's name too.", 107, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 1324 },
{ PROC_LINKS(reopen, 0), false, "reopen", 6, "Reopen the current buffer from the hard drive.", 46, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 1625 },
{ PROC_LINKS(replace_in_all_buffers, 0), false, "replace_in_all_buffers", 22, "Replace (in all editable buffers)", 33, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4coder_kv_commands.cpp", 56, 1275 },
{ PROC_LINKS(replace_in_buffer, 0), false, "replace_in_buffer", 17, "Replace (current buffer only)", 29, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 1063 },
{ PROC_LINKS(replace_in_range, 0), false, "replace_in_range", 16, "Queries the user for a needle and string. Replaces all occurences of needle with string in the range between cursor and the mark in the active buffer.", 150, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 1054 },
{ PROC_LINKS(reverse_search, 0), false, "reverse_search", 14, "Begins an incremental search up through the current buffer for a user specified string.", 87, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 988 },
{ PROC_LINKS(reverse_search_identifier, 0), false, "reverse_search_identifier", 25, "Begins an incremental search up through the current buffer for the word or token under the cursor.", 98, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 1000 },
{ PROC_LINKS(right_adjust_view, 0), false, "right_adjust_view", 17, "Sets the right size of the view near the x position of the cursor.", 66, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4coder_vim/4coder_vim_commands.cpp", 68, 57 },
{ PROC_LINKS(save_all_dirty_buffers, 0), false, "save_all_dirty_buffers", 22, "Saves all buffers marked dirty (showing the '*' indicator).", 59, "C:\\Users\\vodan\\4ed/code/custom/4coder_default_framework.cpp", 59, 499 },
{ PROC_LINKS(save_to_query, 0), false, "save_to_query", 13, "Queries the user for a file name and saves the contents of the current buffer, altering the buffer's name too.", 110, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 1291 },
{ PROC_LINKS(scratch, 0), false, "scratch", 7, "switch to scratch buffer", 24, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4coder_kv_commands.cpp", 56, 1091 },
{ PROC_LINKS(search, 0), false, "search", 6, "Begins an incremental search down through the current buffer for a user specified string.", 89, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 982 },
{ PROC_LINKS(search_identifier, 0), false, "search_identifier", 17, "Begins an incremental search down through the current buffer for the word or token under the cursor.", 100, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 994 },
{ PROC_LINKS(seek_beginning_of_textual_line, 0), false, "seek_beginning_of_textual_line", 30, "Seeks the cursor to the beginning of the line across all text.", 62, "C:\\Users\\vodan\\4ed/code/custom/4coder_helper.cpp", 48, 2339 },
{ PROC_LINKS(seek_end_of_textual_line, 0), false, "seek_end_of_textual_line", 24, "Seeks the cursor to the end of the line across all text.", 56, "C:\\Users\\vodan\\4ed/code/custom/4coder_helper.cpp", 48, 2345 },
{ PROC_LINKS(select_all, 0), false, "select_all", 10, "Puts the cursor at the top of the file, and the mark at the bottom of the file.", 79, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 420 },
{ PROC_LINKS(select_next_scope_absolute, 0), false, "select_next_scope_absolute", 26, "Finds the first scope started by '{' after the cursor and puts the cursor and mark on the '{' and '}'.", 102, "C:\\Users\\vodan\\4ed/code/custom/4coder_scope_commands.cpp", 56, 57 },
{ PROC_LINKS(select_next_scope_after_current, 0), false, "select_next_scope_after_current", 31, "If a scope is selected, find first scope that starts after the selected scope. Otherwise find the first scope that starts after the cursor.", 139, "C:\\Users\\vodan\\4ed/code/custom/4coder_scope_commands.cpp", 56, 66 },
{ PROC_LINKS(select_prev_scope_absolute, 0), false, "select_prev_scope_absolute", 26, "Finds the first scope started by '{' before the cursor and puts the cursor and mark on the '{' and '}'.", 103, "C:\\Users\\vodan\\4ed/code/custom/4coder_scope_commands.cpp", 56, 82 },
{ PROC_LINKS(select_prev_top_most_scope, 0), false, "select_prev_top_most_scope", 26, "Finds the first scope that starts before the cursor, then finds the top most scope that contains that scope.", 108, "C:\\Users\\vodan\\4ed/code/custom/4coder_scope_commands.cpp", 56, 99 },
{ PROC_LINKS(select_surrounding_scope, 0), false, "select_surrounding_scope", 24, "Finds the scope enclosed by '{' '}' surrounding the cursor and puts the cursor and mark on the '{' and '}'.", 107, "C:\\Users\\vodan\\4ed/code/custom/4coder_scope_commands.cpp", 56, 27 },
{ PROC_LINKS(select_surrounding_scope_maximal, 0), false, "select_surrounding_scope_maximal", 32, "Selects the top-most scope that surrounds the cursor.", 53, "C:\\Users\\vodan\\4ed/code/custom/4coder_scope_commands.cpp", 56, 39 },
{ PROC_LINKS(set_current_dir_as_hot, 0), false, "set_current_dir_as_hot", 22, "set current dir as hot", 22, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4coder_kv_commands.cpp", 56, 1082 },
{ PROC_LINKS(set_eol_mode_from_contents, 0), false, "set_eol_mode_from_contents", 26, "Sets the buffer's line ending mode to match the contents of the buffer.", 71, "C:\\Users\\vodan\\4ed/code/custom/4coder_eol.cpp", 45, 125 },
{ PROC_LINKS(set_eol_mode_to_binary, 0), false, "set_eol_mode_to_binary", 22, "Puts the buffer in bin line ending mode.", 40, "C:\\Users\\vodan\\4ed/code/custom/4coder_eol.cpp", 45, 112 },
{ PROC_LINKS(set_eol_mode_to_crlf, 0), false, "set_eol_mode_to_crlf", 20, "Puts the buffer in crlf line ending mode.", 41, "C:\\Users\\vodan\\4ed/code/custom/4coder_eol.cpp", 45, 86 },
{ PROC_LINKS(set_eol_mode_to_lf, 0), false, "set_eol_mode_to_lf", 18, "Puts the buffer in lf line ending mode.", 39, "C:\\Users\\vodan\\4ed/code/custom/4coder_eol.cpp", 45, 99 },
{ PROC_LINKS(set_face_size, 0), false, "set_face_size", 13, "Set face size of the face used by the current buffer.", 53, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 574 },
{ PROC_LINKS(set_face_size_this_buffer, 0), false, "set_face_size_this_buffer", 25, "Set face size of the face used by the current buffer; if any other buffers are using the same face a new face is created so that only this buffer is effected", 157, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 616 },
{ PROC_LINKS(set_mark, 0), false, "set_mark", 8, "Sets the mark to the current position of the cursor.", 52, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 117 },
{ PROC_LINKS(set_mode_to_notepad_like, 0), false, "set_mode_to_notepad_like", 24, "Sets the edit mode to Notepad like.", 35, "C:\\Users\\vodan\\4ed/code/custom/4coder_default_framework.cpp", 59, 544 },
{ PROC_LINKS(set_mode_to_original, 0), false, "set_mode_to_original", 20, "Sets the edit mode to 4coder original.", 38, "C:\\Users\\vodan\\4ed/code/custom/4coder_default_framework.cpp", 59, 538 },
{ PROC_LINKS(show_filebar, 0), false, "show_filebar", 12, "Sets the current view to show it's filebar.", 43, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 545 },
{ PROC_LINKS(show_scrollbar, 0), false, "show_scrollbar", 14, "Sets the current view to show it's scrollbar.", 45, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 531 },
{ PROC_LINKS(show_the_log_graph, 0), true, "show_the_log_graph", 18, "Parses *log* and displays the 'log graph' UI", 44, "C:\\Users\\vodan\\4ed/code/custom/4coder_log_parser.cpp", 52, 992 },
{ PROC_LINKS(snippet_lister, 0), true, "snippet_lister", 14, "Opens a snippet lister for inserting whole pre-written snippets of text.", 72, "C:\\Users\\vodan\\4ed/code/custom/4coder_combined_write_commands.cpp", 65, 237 },
{ PROC_LINKS(suppress_mouse, 0), false, "suppress_mouse", 14, "Hides the mouse and causes all mosue input (clicks, position, wheel) to be ignored.", 83, "C:\\Users\\vodan\\4ed/code/custom/4coder_default_framework.cpp", 59, 520 },
{ PROC_LINKS(swap_panels, 0), false, "swap_panels", 11, "Swaps the active panel with it's sibling.", 41, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 1564 },
{ PROC_LINKS(switch_to_game_panel, 0), false, "switch_to_game_panel", 20, "switch to game panel", 20, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4coder_kv_commands.cpp", 56, 859 },
{ PROC_LINKS(theme_lister, 0), true, "theme_lister", 12, "Opens an interactive list of all registered themes.", 51, "C:\\Users\\vodan\\4ed/code/custom/4coder_lists.cpp", 47, 797 },
{ PROC_LINKS(toggle_filebar, 0), false, "toggle_filebar", 14, "Toggles the visibility status of the current view's filebar.", 60, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 559 },
{ PROC_LINKS(toggle_fps_meter, 0), false, "toggle_fps_meter", 16, "Toggles the visibility of the FPS performance meter", 51, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 568 },
{ PROC_LINKS(toggle_fullscreen, 0), false, "toggle_fullscreen", 17, "Toggle fullscreen mode on or off.  The change(s) do not take effect until the next frame.", 89, "C:\\Users\\vodan\\4ed/code/custom/4coder_default_framework.cpp", 59, 574 },
{ PROC_LINKS(toggle_highlight_enclosing_scopes, 0), false, "toggle_highlight_enclosing_scopes", 33, "In code files scopes surrounding the cursor are highlighted with distinguishing colors.", 87, "C:\\Users\\vodan\\4ed/code/custom/4coder_default_framework.cpp", 59, 558 },
{ PROC_LINKS(toggle_highlight_line_at_cursor, 0), false, "toggle_highlight_line_at_cursor", 31, "Toggles the line highlight at the cursor.", 41, "C:\\Users\\vodan\\4ed/code/custom/4coder_default_framework.cpp", 59, 550 },
{ PROC_LINKS(toggle_line_numbers, 0), false, "toggle_line_numbers", 19, "Toggles the left margin line numbers.", 37, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 673 },
{ PROC_LINKS(toggle_line_wrap, 0), false, "toggle_line_wrap", 16, "Toggles the line wrap setting on this buffer.", 45, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 681 },
{ PROC_LINKS(toggle_mouse, 0), false, "toggle_mouse", 12, "Toggles the mouse suppression mode, see suppress_mouse and allow_mouse.", 71, "C:\\Users\\vodan\\4ed/code/custom/4coder_default_framework.cpp", 59, 532 },
{ PROC_LINKS(toggle_paren_matching_helper, 0), false, "toggle_paren_matching_helper", 28, "In code files matching parentheses pairs are colored with distinguishing colors.", 80, "C:\\Users\\vodan\\4ed/code/custom/4coder_default_framework.cpp", 59, 566 },
{ PROC_LINKS(toggle_show_whitespace, 0), false, "toggle_show_whitespace", 22, "Toggles the current buffer's whitespace visibility status.", 58, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 664 },
{ PROC_LINKS(toggle_virtual_whitespace, 0), false, "toggle_virtual_whitespace", 25, "Toggles virtual whitespace for all files.", 41, "C:\\Users\\vodan\\4ed/code/custom/4coder_code_index.cpp", 52, 1239 },
{ PROC_LINKS(uncomment_line, 0), false, "uncomment_line", 14, "If present, delete '//' at the beginning of the line after leading whitespace.", 78, "C:\\Users\\vodan\\4ed/code/custom/4coder_combined_write_commands.cpp", 65, 137 },
{ PROC_LINKS(undo_all_buffers, 0), false, "undo_all_buffers", 16, "Advances backward through the undo history in the buffer containing the most recent regular edit.", 97, "C:\\Users\\vodan\\4ed/code/custom/4coder_base_commands.cpp", 55, 1800 },
{ PROC_LINKS(view_jump_list_with_lister, 0), false, "view_jump_list_with_lister", 26, "When executed on a buffer with jumps, creates a persistent lister for all the jumps", 83, "C:\\Users\\vodan\\4ed/code/custom/4coder_jump_lister.cpp", 53, 60 },
{ PROC_LINKS(vim_dec_buffer_peek, 0), false, "vim_dec_buffer_peek", 19, "Decrements buffer peek index", 28, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4coder_vim/4coder_vim_commands.cpp", 68, 25 },
{ PROC_LINKS(vim_inc_buffer_peek, 0), false, "vim_inc_buffer_peek", 19, "Incremenets buffer peek index", 29, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4coder_vim/4coder_vim_commands.cpp", 68, 17 },
{ PROC_LINKS(vim_interactive_open_or_new, 0), true, "vim_interactive_open_or_new", 27, "Interactively open a file out of the file system.", 49, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4coder_vim/4coder_vim_lists.cpp", 65, 238 },
{ PROC_LINKS(vim_jump_lister, 0), true, "vim_jump_lister", 15, "Opens an interactive lists of the views jumps", 45, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4coder_vim/4coder_vim_lists.cpp", 65, 455 },
{ PROC_LINKS(vim_list_all_functions_current_buffer_lister, 0), true, "vim_list_all_functions_current_buffer_lister", 44, "Creates a lister of locations that look like function definitions and declarations in the buffer.", 97, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4coder_vim/4coder_vim_lists.cpp", 65, 378 },
{ PROC_LINKS(vim_proj_cmd_lister, 0), true, "vim_proj_cmd_lister", 19, "Opens an interactive list of all project commands.", 50, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4coder_vim/4coder_vim_lists.cpp", 65, 396 },
{ PROC_LINKS(vim_scoll_buffer_peek_down, 0), false, "vim_scoll_buffer_peek_down", 26, "Scrolls buffer peek down", 24, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4coder_vim/4coder_vim_commands.cpp", 68, 53 },
{ PROC_LINKS(vim_scoll_buffer_peek_up, 0), false, "vim_scoll_buffer_peek_up", 24, "Scrolls buffer peek up", 22, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4coder_vim/4coder_vim_commands.cpp", 68, 49 },
{ PROC_LINKS(vim_switch_lister, 0), true, "vim_switch_lister", 17, "Opens an interactive list of all loaded buffers.", 48, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4coder_vim/4coder_vim_lists.cpp", 65, 315 },
{ PROC_LINKS(vim_theme_lister, 0), true, "vim_theme_lister", 16, "Opens an interactive list of all registered themes.", 51, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4coder_vim/4coder_vim_lists.cpp", 65, 287 },
{ PROC_LINKS(vim_toggle_show_buffer_peek, 0), false, "vim_toggle_show_buffer_peek", 27, "Toggles buffer peek", 19, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4coder_vim/4coder_vim_commands.cpp", 68, 1 },
{ PROC_LINKS(vim_try_exit, 0), false, "vim_try_exit", 12, "Vim command for responding to a try-exit event", 46, "C:\\Users\\vodan\\4ed\\code\\4coder_kv/4coder_vim/4coder_vim_hooks.cpp", 65, 128 },
{ PROC_LINKS(word_complete, 0), false, "word_complete", 13, "Iteratively tries completing the word to the left of the cursor with other words in open buffers that have the same prefix string.", 130, "C:\\Users\\vodan\\4ed/code/custom/4coder_search.cpp", 48, 561 },
{ PROC_LINKS(word_complete_drop_down, 0), false, "word_complete_drop_down", 23, "Word complete with drop down menu.", 34, "C:\\Users\\vodan\\4ed/code/custom/4coder_search.cpp", 48, 807 },
{ PROC_LINKS(write_block, 0), false, "write_block", 11, "At the cursor, insert a block comment.", 38, "C:\\Users\\vodan\\4ed/code/custom/4coder_combined_write_commands.cpp", 65, 94 },
{ PROC_LINKS(write_hack, 0), false, "write_hack", 10, "At the cursor, insert a '// HACK' comment, includes user name if it was specified in config.4coder.", 99, "C:\\Users\\vodan\\4ed/code/custom/4coder_combined_write_commands.cpp", 65, 82 },
{ PROC_LINKS(write_note, 0), false, "write_note", 10, "At the cursor, insert a '// NOTE' comment, includes user name if it was specified in config.4coder.", 99, "C:\\Users\\vodan\\4ed/code/custom/4coder_combined_write_commands.cpp", 65, 88 },
{ PROC_LINKS(write_text_and_auto_indent, 0), false, "write_text_and_auto_indent", 26, "Inserts text and auto-indents the line on which the cursor sits if any of the text contains 'layout punctuation' such as ;:{}()[]# and new lines.", 145, "C:\\Users\\vodan\\4ed/code/custom/4coder_auto_indent.cpp", 53, 480 },
{ PROC_LINKS(write_todo, 0), false, "write_todo", 10, "At the cursor, insert a '// TODO' comment, includes user name if it was specified in config.4coder.", 99, "C:\\Users\\vodan\\4ed/code/custom/4coder_combined_write_commands.cpp", 65, 76 },
{ PROC_LINKS(write_zero_struct, 0), false, "write_zero_struct", 17, "At the cursor, insert a ' = {};'.", 33, "C:\\Users\\vodan\\4ed/code/custom/4coder_combined_write_commands.cpp", 65, 100 },
};
static i1 fcoder_metacmd_ID_DEBUG_draw_hud_toggle = 0;
static i1 fcoder_metacmd_ID_allow_mouse = 1;
static i1 fcoder_metacmd_ID_are_we_in_debug_build = 2;
static i1 fcoder_metacmd_ID_backspace_char = 3;
static i1 fcoder_metacmd_ID_byp_reset_face_size = 4;
static i1 fcoder_metacmd_ID_center_view = 5;
static i1 fcoder_metacmd_ID_clean_all_lines = 6;
static i1 fcoder_metacmd_ID_clean_trailing_whitespace = 7;
static i1 fcoder_metacmd_ID_clear_all_themes = 8;
static i1 fcoder_metacmd_ID_click_set_cursor = 9;
static i1 fcoder_metacmd_ID_click_set_cursor_and_mark = 10;
static i1 fcoder_metacmd_ID_click_set_cursor_if_lbutton = 11;
static i1 fcoder_metacmd_ID_click_set_mark = 12;
static i1 fcoder_metacmd_ID_clipboard_record_clip = 13;
static i1 fcoder_metacmd_ID_close_all_code = 14;
static i1 fcoder_metacmd_ID_command_lister = 15;
static i1 fcoder_metacmd_ID_comment_line = 16;
static i1 fcoder_metacmd_ID_comment_line_toggle = 17;
static i1 fcoder_metacmd_ID_copy = 18;
static i1 fcoder_metacmd_ID_cursor_mark_swap = 19;
static i1 fcoder_metacmd_ID_cut = 20;
static i1 fcoder_metacmd_ID_debug_camera_on = 21;
static i1 fcoder_metacmd_ID_decrease_face_size = 22;
static i1 fcoder_metacmd_ID_default_try_exit = 23;
static i1 fcoder_metacmd_ID_default_view_input_handler = 24;
static i1 fcoder_metacmd_ID_delete_char = 25;
static i1 fcoder_metacmd_ID_delete_current_scope = 26;
static i1 fcoder_metacmd_ID_delete_file_query = 27;
static i1 fcoder_metacmd_ID_delete_line = 28;
static i1 fcoder_metacmd_ID_dir = 29;
static i1 fcoder_metacmd_ID_display_key_codes = 30;
static i1 fcoder_metacmd_ID_display_text_input = 31;
static i1 fcoder_metacmd_ID_double_backspace = 32;
static i1 fcoder_metacmd_ID_dump_history_to_file = 33;
static i1 fcoder_metacmd_ID_duplicate_line = 34;
static i1 fcoder_metacmd_ID_execute_any_cli = 35;
static i1 fcoder_metacmd_ID_execute_previous_cli = 36;
static i1 fcoder_metacmd_ID_exit_4coder = 37;
static i1 fcoder_metacmd_ID_file = 38;
static i1 fcoder_metacmd_ID_fold_clear = 39;
static i1 fcoder_metacmd_ID_fold_pop_cursor = 40;
static i1 fcoder_metacmd_ID_fold_range = 41;
static i1 fcoder_metacmd_ID_fold_toggle_cursor = 42;
static i1 fcoder_metacmd_ID_game_disable = 43;
static i1 fcoder_metacmd_ID_game_enable = 44;
static i1 fcoder_metacmd_ID_go_to_user_directory = 45;
static i1 fcoder_metacmd_ID_goto_beginning_of_file = 46;
static i1 fcoder_metacmd_ID_goto_end_of_file = 47;
static i1 fcoder_metacmd_ID_goto_first_jump = 48;
static i1 fcoder_metacmd_ID_goto_first_jump_same_panel_sticky = 49;
static i1 fcoder_metacmd_ID_goto_line = 50;
static i1 fcoder_metacmd_ID_goto_next_jump_no_skips = 51;
static i1 fcoder_metacmd_ID_goto_prev_jump_no_skips = 52;
static i1 fcoder_metacmd_ID_hide_filebar = 53;
static i1 fcoder_metacmd_ID_hide_scrollbar = 54;
static i1 fcoder_metacmd_ID_if0_off = 55;
static i1 fcoder_metacmd_ID_increase_face_size = 56;
static i1 fcoder_metacmd_ID_init = 57;
static i1 fcoder_metacmd_ID_interactive_kill_buffer = 58;
static i1 fcoder_metacmd_ID_interactive_new = 59;
static i1 fcoder_metacmd_ID_interactive_open = 60;
static i1 fcoder_metacmd_ID_interactive_open_or_new = 61;
static i1 fcoder_metacmd_ID_interactive_switch_buffer = 62;
static i1 fcoder_metacmd_ID_jump_to_last_point = 63;
static i1 fcoder_metacmd_ID_keyboard_macro_finish_recording = 64;
static i1 fcoder_metacmd_ID_keyboard_macro_replay = 65;
static i1 fcoder_metacmd_ID_keyboard_macro_start_recording = 66;
static i1 fcoder_metacmd_ID_kill_buffer = 67;
static i1 fcoder_metacmd_ID_kv_miscellaneous_debug_command = 68;
static i1 fcoder_metacmd_ID_kv_open_note_file = 69;
static i1 fcoder_metacmd_ID_kv_profile_disable_and_inspect = 70;
static i1 fcoder_metacmd_ID_kv_reopen_with_confirmation = 71;
static i1 fcoder_metacmd_ID_kv_run = 72;
static i1 fcoder_metacmd_ID_left_adjust_view = 73;
static i1 fcoder_metacmd_ID_list_all_functions_all_buffers = 74;
static i1 fcoder_metacmd_ID_list_all_functions_all_buffers_lister = 75;
static i1 fcoder_metacmd_ID_list_all_functions_current_buffer = 76;
static i1 fcoder_metacmd_ID_list_all_functions_current_buffer_lister = 77;
static i1 fcoder_metacmd_ID_list_all_locations = 78;
static i1 fcoder_metacmd_ID_list_all_locations_case_insensitive = 79;
static i1 fcoder_metacmd_ID_list_all_substring_locations = 80;
static i1 fcoder_metacmd_ID_list_all_substring_locations_case_insensitive = 81;
static i1 fcoder_metacmd_ID_load_project = 82;
static i1 fcoder_metacmd_ID_load_project_current_dir = 83;
static i1 fcoder_metacmd_ID_load_theme_current_buffer = 84;
static i1 fcoder_metacmd_ID_load_themes_default_folder = 85;
static i1 fcoder_metacmd_ID_load_themes_hot_directory = 86;
static i1 fcoder_metacmd_ID_make_directory_query = 87;
static i1 fcoder_metacmd_ID_messages = 88;
static i1 fcoder_metacmd_ID_miblo_decrement_basic = 89;
static i1 fcoder_metacmd_ID_miblo_decrement_time_stamp = 90;
static i1 fcoder_metacmd_ID_miblo_decrement_time_stamp_minute = 91;
static i1 fcoder_metacmd_ID_miblo_increment_basic = 92;
static i1 fcoder_metacmd_ID_miblo_increment_time_stamp = 93;
static i1 fcoder_metacmd_ID_miblo_increment_time_stamp_minute = 94;
static i1 fcoder_metacmd_ID_mouse_wheel_change_face_size = 95;
static i1 fcoder_metacmd_ID_mouse_wheel_scroll = 96;
static i1 fcoder_metacmd_ID_move_down = 97;
static i1 fcoder_metacmd_ID_move_down_10 = 98;
static i1 fcoder_metacmd_ID_move_down_textual = 99;
static i1 fcoder_metacmd_ID_move_down_to_blank_line = 100;
static i1 fcoder_metacmd_ID_move_down_to_blank_line_end = 101;
static i1 fcoder_metacmd_ID_move_down_to_blank_line_skip_whitespace = 102;
static i1 fcoder_metacmd_ID_move_left = 103;
static i1 fcoder_metacmd_ID_move_line_down = 104;
static i1 fcoder_metacmd_ID_move_line_up = 105;
static i1 fcoder_metacmd_ID_move_right = 106;
static i1 fcoder_metacmd_ID_move_up = 107;
static i1 fcoder_metacmd_ID_move_up_10 = 108;
static i1 fcoder_metacmd_ID_move_up_to_blank_line = 109;
static i1 fcoder_metacmd_ID_move_up_to_blank_line_end = 110;
static i1 fcoder_metacmd_ID_move_up_to_blank_line_skip_whitespace = 111;
static i1 fcoder_metacmd_ID_multi_paste = 112;
static i1 fcoder_metacmd_ID_multi_paste_interactive = 113;
static i1 fcoder_metacmd_ID_multi_paste_interactive_quick = 114;
static i1 fcoder_metacmd_ID_no_op = 115;
static i1 fcoder_metacmd_ID_open_all_code = 116;
static i1 fcoder_metacmd_ID_open_all_code_recursive = 117;
static i1 fcoder_metacmd_ID_open_file_in_quotes = 118;
static i1 fcoder_metacmd_ID_open_long_braces = 119;
static i1 fcoder_metacmd_ID_open_long_braces_break = 120;
static i1 fcoder_metacmd_ID_open_long_braces_semicolon = 121;
static i1 fcoder_metacmd_ID_open_matching_file_cpp = 122;
static i1 fcoder_metacmd_ID_open_matching_file_cpp_other_panel = 123;
static i1 fcoder_metacmd_ID_page_down = 124;
static i1 fcoder_metacmd_ID_page_up = 125;
static i1 fcoder_metacmd_ID_paste = 126;
static i1 fcoder_metacmd_ID_paste_next = 127;
static i1 fcoder_metacmd_ID_place_in_scope = 128;
static i1 fcoder_metacmd_ID_play_with_a_counter = 129;
static i1 fcoder_metacmd_ID_profile_clear = 130;
static i1 fcoder_metacmd_ID_profile_enable = 131;
static i1 fcoder_metacmd_ID_profile_inspect = 132;
static i1 fcoder_metacmd_ID_project_command_F1 = 133;
static i1 fcoder_metacmd_ID_project_command_F10 = 134;
static i1 fcoder_metacmd_ID_project_command_F11 = 135;
static i1 fcoder_metacmd_ID_project_command_F12 = 136;
static i1 fcoder_metacmd_ID_project_command_F13 = 137;
static i1 fcoder_metacmd_ID_project_command_F14 = 138;
static i1 fcoder_metacmd_ID_project_command_F15 = 139;
static i1 fcoder_metacmd_ID_project_command_F16 = 140;
static i1 fcoder_metacmd_ID_project_command_F2 = 141;
static i1 fcoder_metacmd_ID_project_command_F3 = 142;
static i1 fcoder_metacmd_ID_project_command_F4 = 143;
static i1 fcoder_metacmd_ID_project_command_F5 = 144;
static i1 fcoder_metacmd_ID_project_command_F6 = 145;
static i1 fcoder_metacmd_ID_project_command_F7 = 146;
static i1 fcoder_metacmd_ID_project_command_F8 = 147;
static i1 fcoder_metacmd_ID_project_command_F9 = 148;
static i1 fcoder_metacmd_ID_project_command_lister = 149;
static i1 fcoder_metacmd_ID_project_fkey_command = 150;
static i1 fcoder_metacmd_ID_project_go_to_root_directory = 151;
static i1 fcoder_metacmd_ID_project_reprint = 152;
static i1 fcoder_metacmd_ID_query_replace = 153;
static i1 fcoder_metacmd_ID_query_replace_identifier = 154;
static i1 fcoder_metacmd_ID_query_replace_selection = 155;
static i1 fcoder_metacmd_ID_quick_swap_buffer = 156;
static i1 fcoder_metacmd_ID_redo_all_buffers = 157;
static i1 fcoder_metacmd_ID_reg = 158;
static i1 fcoder_metacmd_ID_rename_file_query = 159;
static i1 fcoder_metacmd_ID_reopen = 160;
static i1 fcoder_metacmd_ID_replace_in_all_buffers = 161;
static i1 fcoder_metacmd_ID_replace_in_buffer = 162;
static i1 fcoder_metacmd_ID_replace_in_range = 163;
static i1 fcoder_metacmd_ID_reverse_search = 164;
static i1 fcoder_metacmd_ID_reverse_search_identifier = 165;
static i1 fcoder_metacmd_ID_right_adjust_view = 166;
static i1 fcoder_metacmd_ID_save_all_dirty_buffers = 167;
static i1 fcoder_metacmd_ID_save_to_query = 168;
static i1 fcoder_metacmd_ID_scratch = 169;
static i1 fcoder_metacmd_ID_search = 170;
static i1 fcoder_metacmd_ID_search_identifier = 171;
static i1 fcoder_metacmd_ID_seek_beginning_of_textual_line = 172;
static i1 fcoder_metacmd_ID_seek_end_of_textual_line = 173;
static i1 fcoder_metacmd_ID_select_all = 174;
static i1 fcoder_metacmd_ID_select_next_scope_absolute = 175;
static i1 fcoder_metacmd_ID_select_next_scope_after_current = 176;
static i1 fcoder_metacmd_ID_select_prev_scope_absolute = 177;
static i1 fcoder_metacmd_ID_select_prev_top_most_scope = 178;
static i1 fcoder_metacmd_ID_select_surrounding_scope = 179;
static i1 fcoder_metacmd_ID_select_surrounding_scope_maximal = 180;
static i1 fcoder_metacmd_ID_set_current_dir_as_hot = 181;
static i1 fcoder_metacmd_ID_set_eol_mode_from_contents = 182;
static i1 fcoder_metacmd_ID_set_eol_mode_to_binary = 183;
static i1 fcoder_metacmd_ID_set_eol_mode_to_crlf = 184;
static i1 fcoder_metacmd_ID_set_eol_mode_to_lf = 185;
static i1 fcoder_metacmd_ID_set_face_size = 186;
static i1 fcoder_metacmd_ID_set_face_size_this_buffer = 187;
static i1 fcoder_metacmd_ID_set_mark = 188;
static i1 fcoder_metacmd_ID_set_mode_to_notepad_like = 189;
static i1 fcoder_metacmd_ID_set_mode_to_original = 190;
static i1 fcoder_metacmd_ID_show_filebar = 191;
static i1 fcoder_metacmd_ID_show_scrollbar = 192;
static i1 fcoder_metacmd_ID_show_the_log_graph = 193;
static i1 fcoder_metacmd_ID_snippet_lister = 194;
static i1 fcoder_metacmd_ID_suppress_mouse = 195;
static i1 fcoder_metacmd_ID_swap_panels = 196;
static i1 fcoder_metacmd_ID_switch_to_game_panel = 197;
static i1 fcoder_metacmd_ID_theme_lister = 198;
static i1 fcoder_metacmd_ID_toggle_filebar = 199;
static i1 fcoder_metacmd_ID_toggle_fps_meter = 200;
static i1 fcoder_metacmd_ID_toggle_fullscreen = 201;
static i1 fcoder_metacmd_ID_toggle_highlight_enclosing_scopes = 202;
static i1 fcoder_metacmd_ID_toggle_highlight_line_at_cursor = 203;
static i1 fcoder_metacmd_ID_toggle_line_numbers = 204;
static i1 fcoder_metacmd_ID_toggle_line_wrap = 205;
static i1 fcoder_metacmd_ID_toggle_mouse = 206;
static i1 fcoder_metacmd_ID_toggle_paren_matching_helper = 207;
static i1 fcoder_metacmd_ID_toggle_show_whitespace = 208;
static i1 fcoder_metacmd_ID_toggle_virtual_whitespace = 209;
static i1 fcoder_metacmd_ID_uncomment_line = 210;
static i1 fcoder_metacmd_ID_undo_all_buffers = 211;
static i1 fcoder_metacmd_ID_view_jump_list_with_lister = 212;
static i1 fcoder_metacmd_ID_vim_dec_buffer_peek = 213;
static i1 fcoder_metacmd_ID_vim_inc_buffer_peek = 214;
static i1 fcoder_metacmd_ID_vim_interactive_open_or_new = 215;
static i1 fcoder_metacmd_ID_vim_jump_lister = 216;
static i1 fcoder_metacmd_ID_vim_list_all_functions_current_buffer_lister = 217;
static i1 fcoder_metacmd_ID_vim_proj_cmd_lister = 218;
static i1 fcoder_metacmd_ID_vim_scoll_buffer_peek_down = 219;
static i1 fcoder_metacmd_ID_vim_scoll_buffer_peek_up = 220;
static i1 fcoder_metacmd_ID_vim_switch_lister = 221;
static i1 fcoder_metacmd_ID_vim_theme_lister = 222;
static i1 fcoder_metacmd_ID_vim_toggle_show_buffer_peek = 223;
static i1 fcoder_metacmd_ID_vim_try_exit = 224;
static i1 fcoder_metacmd_ID_word_complete = 225;
static i1 fcoder_metacmd_ID_word_complete_drop_down = 226;
static i1 fcoder_metacmd_ID_write_block = 227;
static i1 fcoder_metacmd_ID_write_hack = 228;
static i1 fcoder_metacmd_ID_write_note = 229;
static i1 fcoder_metacmd_ID_write_text_and_auto_indent = 230;
static i1 fcoder_metacmd_ID_write_todo = 231;
static i1 fcoder_metacmd_ID_write_zero_struct = 232;
#endif
