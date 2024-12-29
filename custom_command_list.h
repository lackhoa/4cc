//-
// NOTE(kv) All of the custom commands, gathered in one place
// ...also not really an h file.
//-

//;normal_commands
normal_commands
{
 F4_Index_Reset "Reset the index"
  delete_char "Deletes the character to the right of the cursor."
  backspace_char "Deletes the character to the left of the cursor."
  click_set_cursor_and_mark "Sets the cursor position and mark to the mouse position."
  click_set_cursor "Sets the cursor position to the mouse position." 
  click_set_cursor_if_lbutton "If the mouse left button is pressed, sets the cursor position to the mouse position." 
  click_set_mark "Sets the mark position to the mouse position." 
  mouse_wheel_scroll "Reads the scroll wheel value from the mouse state and scrolls accordingly." 
  move_up "Moves the cursor up one line." 
  move_down "Moves the cursor down one line." 
  move_up_10 "Moves the cursor up ten lines." 
  move_down_10 "Moves the cursor down ten lines." 
  move_down_textual "Moves down to the next line of actual text, regardless of line wrapping." 
  page_up "Scrolls the view up one view height and moves the cursor up one view height." 
  page_down "Scrolls the view down one view height and moves the cursor down one view height."
  move_up_to_blank_line "Seeks the cursor up to the next blank line." 
  move_down_to_blank_line "Seeks the cursor down to the next blank line." 
  move_up_to_blank_line_skip_whitespace "Seeks the cursor up to the next blank line and places it at the end of the line." 
  move_down_to_blank_line_skip_whitespace "Seeks the cursor down to the next blank line and places it at the end of the line." 
  move_up_to_blank_line_end "Seeks the cursor up to the next blank line and places it at the end of the line." 
  move_down_to_blank_line_end "Seeks the cursor down to the next blank line and places it at the end of the line." 
  select_all "Puts the cursor at the top of the file, and the mark at the bottom of the file." 
  clean_all_lines "Removes trailing whitespace from all lines and removes all blank lines in the current buffer." 
  clean_trailing_whitespace "Removes trailing whitespace from all lines in the current buffer." 
  show_scrollbar "Sets the current view to show it's scrollbar."
  hide_scrollbar "Sets the current view to hide it's scrollbar."
  show_filebar "Sets the current view to show it's filebar."
  hide_filebar "Sets the current view to hide it's filebar."
  toggle_filebar "Toggles the visibility status of the current view's filebar."
  toggle_fps_meter "Toggles the visibility of the FPS performance meter"
  set_face_size "Set face size of the face used by the current buffer."
  increase_face_size "Increase the size of the face used by the current buffer."
  decrease_face_size "Decrease the size of the face used by the current buffer."
  set_face_size_this_buffer "Set face size of the face used by the current buffer; if any other buffers are using the same face a new face is created so that only this buffer is effected"
  mouse_wheel_change_face_size "Reads the state of the mouse wheel and uses it to either increase or decrease the face size."
  toggle_show_whitespace "Toggles the current buffer's whitespace visibility status."
  toggle_line_numbers "Toggles the left margin line numbers."
  exit_4coder "Attempts to close 4coder."
  goto_line "Queries the user for a number, and jumps the cursor to the corresponding line."
  goto_pos "jump to byte position"
  search "Begins an incremental search down through the current buffer for a user specified string."
  reverse_search "Begins an incremental search up through the current buffer for a user specified string."
  search_identifier "Begins an incremental search down through the current buffer for the word or token under the cursor."
  reverse_search_identifier "Begins an incremental search up through the current buffer for the word or token under the cursor."
  replace_in_range "Queries the user for a needle and string. Replaces all occurences of needle with string in the range between cursor and the mark in the active buffer."
  toggle_undo_global_mode "undos will be done globally, or in single buffer"
  replace_in_buffer "Replace (current buffer only)"
  reopen "Reopen the current buffer from the hard drive."
  save_current_buffer "Saves the current buffer."
  kill_buffer "Kills the current buffer."
  quick_swap_buffer "Change to the most recently used buffer in this view - or to the top of the buffer stack if the most recent doesn't exist anymore"
  swap_panels "Swaps the active panel with it's sibling."
  open_matching_file_cpp_other_panel "If the current file is a *.cpp or *.h, attempts to open the corresponding *.h or *.cpp file in the other view."
  open_matching_file_cpp "If the current file is a *.cpp or *.h, attempts to open the corresponding *.h or *.cpp file."
  open_file_in_quotes "Reads a filename from surrounding '\"' characters and attempts to open the corresponding file."
  delete_line "Delete the line the on which the cursor sits."
  duplicate_line "Create a copy of the line on which the cursor sits."
  move_line_down "Swaps the line under the cursor with the line below it, and moves the cursor down with it."
  move_line_up "Swaps the line under the cursor with the line above it, and moves the cursor up with it."
  make_directory_query "Queries the user for a name and creates a new directory with the given name."
  rename_file_query "Queries the user for a new name and renames the file of the current buffer, altering the buffer's name too."
  save_to_query "Queries the user for a file name and saves the contents of the current buffer, altering the buffer's name too."
  delete_file_query "Deletes the file of the current buffer if 4coder has the appropriate access rights. Will ask the user for confirmation first."
  jump_to_last_point "Read from the top of the point stack and jump there; if already there pop the top and go to the next option"
  query_replace_selection "Queries the user for a string, and incrementally replace every occurence of the string found in the selected range with the specified string."
  query_replace_identifier "Queries the user for a string, and incrementally replace every occurence of the word or token found at the cursor with the specified string."
  query_replace "Queries the user for two strings, and incrementally replaces every occurence of the first string with the second string."
  replace_in_all_buffers "Replace (in all editable buffers)"
  are_we_in_debug_build ""
  messages "switch to messages buffer"
  dir "kv copy dir name"
  scratch "switch to scratch buffer"
  set_current_dir_as_hot "set current dir as hot"
  init "configure your editor!"
  kv_miscellaneous_debug_command "just a placeholder command so I can test stuff"
  file "open file"
  copy_filename "kv copy file name"
  switch_to_game_panel "switch to game panel"
  kv_open_note_file "switch to my note file"
  kv_run "run the current script"
  kv_reopen_with_confirmation "Like reopen, but asks for confirmation"
  byp_reset_face_size "Resets face size to default"
  project_command_F16 "Run the command with index 16"
  project_command_F15 "Run the command with index 15"
  project_command_F14 "Run the command with index 14"
  project_command_F13 "Run the command with index 13"
  project_command_F12 "Run the command with index 12"
  project_command_F11 "Run the command with index 11"
  project_command_F10 "Run the command with index 10"
  project_command_F9 "Run the command with index 9"
  project_command_F8 "Run the command with index 8"
  project_command_F7 "Run the command with index 7"
  project_command_F6 "Run the command with index 6"
  project_command_F5 "Run the command with index 5"
  project_command_F4 "Run the command with index 4"
  project_command_F3 "Run the command with index 3"
  project_command_F2 "Run the command with index 2"
  project_command_F1 "Run the command with index 1"
  project_reprint "Prints the current project to the file it was loaded from; prints in the most recent project file version"
  project_command_lister "Open a lister of all commands in the currently loaded project."
  project_go_to_root_directory "Changes 4coder's hot directory to the root directory of the currently loaded project. With no loaded project nothing hapepns."
  project_fkey_command "Run an 'fkey command' configured in a project.4coder file.  Determines the index of the 'fkey command' by which function key or numeric key was pressed to trigger the command."
  load_project_current_dir "Looks for a project.4coder file in the current directory and tries to load it.  Looks in parent directories until a project file is found or there are no more parents."
  open_all_code_recursive "Works as open_all_code but also runs in all subdirectories."
  open_all_code "Open all code in the current directory. File types are determined by extensions. An extension is considered code based on the extensions specified in 4coder.config."
  close_all_code "Closes any buffer with a filename ending with an extension configured to be recognized as a code file type."
  delete_current_scope "Deletes the braces surrounding the currently selected scope.  Leaves the contents within the scope."
  place_in_scope "Wraps the code contained in the range between cursor and mark with a new curly brace scope."
  select_prev_top_most_scope "Finds the first scope that starts before the cursor, then finds the top most scope that contains that scope."
  select_prev_scope_absolute "Finds the first scope started by '{' before the cursor and puts the cursor and mark on the '{' and '}'."
  select_next_scope_after_current "If a scope is selected, find first scope that starts after the selected scope. Otherwise find the first scope that starts after the cursor."
  select_next_scope_absolute "Finds the first scope started by '{' after the cursor and puts the cursor and mark on the '{' and '}'."
  select_surrounding_scope_maximal "Selects the top-most scope that surrounds the cursor."
  select_surrounding_scope "Finds the scope enclosed by '{' '}' surrounding the cursor and puts the cursor and mark on the '{' and '}'."
  clear_all_themes "Clear the theme list"
  load_themes_hot_directory "Loads all the theme files in the current hot directory."
  toggle_fullscreen "Toggle fullscreen mode on or off.  The change(s) do not take effect until the next frame."
  set_mode_to_original "Sets the edit mode to 4coder original."
  toggle_mouse "Toggles the mouse suppression mode, see suppress_mouse and allow_mouse."
  allow_mouse "Shows the mouse and causes all mouse input to be processed normally."
  suppress_mouse "Hides the mouse and causes all mosue input (clicks, position, wheel) to be ignored."
  debug_camera_on ""
  game_disable ""
  game_enable ""
  goto_first_jump_same_panel_sticky "If a buffer containing jump locations has been locked in, goes to the first jump in the buffer and views the buffer in the panel where the jump list was."
  goto_first_jump "If a buffer containing jump locations has been locked in, goes to the first jump in the buffer."
  goto_prev_jump_no_skips "If a buffer containing jump locations has been locked in, goes to the previous jump in the buffer, and does not skip sub jump locations."
  goto_next_jump_no_skips "If a buffer containing jump locations has been locked in, goes to the next jump in the buffer, and does not skip sub jump locations."
  comment_line_toggle "Turns uncommented lines into commented lines and vice versa for comments starting with '//'."
  uncomment_line "If present, delete '//' at the beginning of the line after leading whitespace."
  comment_line "Insert '//' at the beginning of the line after leading whitespace."
  if0_off "Surround the range between the cursor and mark with an '#if 0' and an '#endif'"
  open_long_braces_break "At the cursor, insert a '{' and '}break;' separated by a blank line."
  open_long_braces_semicolon "At the cursor, insert a '{' and '};' separated by a blank line."
  open_long_braces "At the cursor, insert a '{' and '}' separated by a blank line."
  seek_end_of_textual_line "Seeks the cursor to the end of the line across all text."
  seek_beginning_of_textual_line "Seeks the cursor to the beginning of the line across all text."
  keyboard_macro_replay "Replay the most recently recorded keyboard macro"
  keyboard_macro_finish_recording "Stop macro recording, do nothing if macro recording is not already started"
  keyboard_macro_start_recording "Start macro recording, do nothing if macro recording is already started"
  word_complete_drop_down "Word complete with drop down menu."
  word_complete "Iteratively tries completing the word to the left of the cursor with other words in open buffers that have the same prefix string."
  list_all_substring_locations_case_insensitive "Queries the user for a string and lists all case-insensitive substring matches found in all open buffers."
  list_all_locations_case_insensitive "Queries the user for a string and lists all exact case-insensitive matches found in all open buffers."
  list_all_substring_locations "Queries the user for a string and lists all case-sensitive substring matches found in all open buffers."
  list_all_locations "Queries the user for a string and lists all exact case-sensitive matches found in all open buffers."
  set_eol_mode_from_contents "Sets the buffer's line ending mode to match the contents of the buffer."
  set_eol_mode_to_binary "Puts the buffer in bin line ending mode."
  set_eol_mode_to_lf "Puts the buffer in lf line ending mode."
  set_eol_mode_to_crlf "Puts the buffer in crlf line ending mode."
  multi_paste_interactive_quick "Paste multiple lines from the clipboard history, controlled by inputing the number of lines to paste"
  multi_paste_interactive "Paste multiple lines from the clipboard history, controlled with arrow keys"
  multi_paste "Paste multiple entries from the clipboard at once"
  paste_next "If the previous command was paste or paste_next, replaces the paste range with the next text down on the clipboard, otherwise operates as the paste command."
  paste "At the cursor, insert the text at the top of the clipboard."
  cut "Cut the text in the range from the cursor to the mark onto the clipboard."
  copy "Copy the text in the range from the cursor to the mark onto the clipboard."
  clipboard_record_clip "In response to a new clipboard contents events, saves the new clip onto the clipboard history"
 /*hit_sfx "Play the hit sound effect"
 music_stop "Stops the music."
 music_start "Starts the music."
 display_text_input "Example of to_writable and leave_current_input_unhandled"
 display_key_codes "Example of input handling loop"
 play_with_a_counter "Example of query bar"
 double_backspace "Example of history group helpers"*/
  kv_profile_disable_and_inspect "disable and inspect profile"
  profile_clear "Clear all profiling information from 4coder's self profiler."
  profile_enable "Allow 4coder's self profiler to gather new profiling information."
  execute_any_cli "Queries for an output buffer name and system command, runs the system command as a CLI and prints the output to the specified buffer."
  execute_previous_cli "If the command execute_any_cli has already been used, this will execute a CLI reusing the most recent buffer name and command."
  go_to_user_directory "Go to the 4coder user directory"
  load_theme_current_buffer "Parse the current buffer as a theme file and add the theme to the theme list. If the buffer has a .4coder postfix in it's name, it is removed when the name is saved."
  DEBUG_draw_hud_toggle "toggle debug hud"
  view_jump_list_with_lister "When executed on a buffer with jumps, creates a persistent lister for all the jumps"
  no_op "no op for binding keybinds to resolve without side effect"
  default_view_input_handler "Input consumption loop for default view behavior"
  default_try_exit "Default command for responding to a try-exit event"
  write_text_and_auto_indent "Inserts text and auto-indents the line on which the cursor sits if any of the text contains 'layout punctuation' such as ;:{}()[]# and new lines."
  goto_end_of_file "Sets the cursor to the end of the file."
  goto_beginning_of_file "Sets the cursor to the beginning of the file."
  load_themes_default_folder "Loads all the theme files in the default theme folder."
  save_all_dirty_buffers "Saves all buffers marked dirty (showing the '*' indicator)."
  open_panel_hsplit "Create a new panel by horizontally splitting the active panel."
  open_panel_vsplit "Create a new panel by vertically splitting the active panel."
  set_mark "Sets the mark to the current position of the cursor."
  center_view "Centers the view vertically on the line on which the cursor sits."
  left_adjust_view "Sets the left size of the view near the x position of the cursor."
  move_left "Moves the cursor one character to the left."
  move_right "Moves the cursor one character to the right."
  //list_all_functions_current_buffer "Creates a jump list of lines of the current buffer that appear to define or declare functions."
  vim_try_exit "Vim command for responding to a try-exit event"
  cmd_goto_random_position "go to random file position"
  cmd_switch_dot_arrow ""
}

ui_commands
{
 profile_inspect "Inspect all currently collected profiling information in 4coder's self profiler."
  vim_interactive_open_or_new "Interactively open a file out of the file system."
  vim_theme_lister "Opens an interactive list of all registered themes."
  //vim_switch_lister "Opens an interactive list of all loaded buffers."
  //vim_list_all_functions_current_buffer_lister "Creates a lister of locations that look like function definitions and declarations in the buffer."
  vim_proj_cmd_lister "Opens an interactive list of all project commands."
  vim_jump_lister "Opens an interactive lists of the views jumps"
  interactive_switch_buffer "Interactively switch to an open buffer."
  interactive_kill_buffer "Interactively kill an open buffer."
  interactive_open_or_new "Interactively open a file out of the file system."
  interactive_new "Interactively creates a new file."
  interactive_open "Interactively opens a file."
  command_lister "Opens an interactive list of all registered commands."
  theme_lister "Opens an interactive list of all registered themes."
 /*f4_search_for_definition__project_wide "List all definitions in the index and jump to the one selected by the user."
 f4_search_for_definition__current_file "List all definitions in the current file and jump to the one selected by the user."
 f4_open_project "Open a project by navigating to the project file."
 f4_interactive_open_or_new_in_project "Interactively open a file out of the file system, filtered to files only in the project."*/
  //f4_recent_files_menu "Lists the recent files used in the current panel."
  //list_all_functions_current_buffer_lister "Creates a lister of locations that look like function definitions and declarations in the buffer."
  //list_all_functions_all_buffers_lister "Creates a lister of locations that look like function definitions and declarations all buffers."
  //list_all_functions_all_buffers "Creates a jump list of lines from all buffers that appear to define or declare functions."
  show_the_log_graph "Parses *log* and displays the 'log graph' UI"
}

custom_ids
{
 defcolor_bar colors,
 defcolor_base colors,
 defcolor_pop1 colors,
 defcolor_pop2 colors,
 defcolor_back colors,
 defcolor_margin colors,
 defcolor_margin_hover colors,
 defcolor_margin_active colors,
 defcolor_list_item colors,
 defcolor_list_item_hover colors,
 defcolor_list_item_active colors,
 defcolor_cursor colors,
 defcolor_at_cursor colors,
 defcolor_highlight_cursor_line colors,
 defcolor_highlight colors,
 defcolor_at_highlight colors,
 defcolor_mark colors,
 defcolor_text_default colors,
 defcolor_comment colors,
 defcolor_comment_pop colors,
 defcolor_keyword colors,
 defcolor_str_constant colors,
 defcolor_char_constant colors,
 defcolor_int_constant colors,
 defcolor_float_constant colors,
 defcolor_bool_constant colors,
 defcolor_preproc colors,
 defcolor_include colors,
 defcolor_special_character colors,
 defcolor_ghost_character colors,
 defcolor_highlight_junk colors,
 defcolor_highlight_white colors,
 defcolor_paste colors,
 defcolor_undo colors,
 defcolor_back_cycle colors,
 defcolor_text_cycle colors,
 defcolor_line_numbers_back colors,
 defcolor_line_numbers_text colors,
 view_rewrite_loc attachment,
 view_next_rewrite_loc attachment,
 view_paste_index_loc attachment,
 view_is_passive_loc attachment,
 view_snap_mark_to_cursor attachment,
 view_ui_data attachment,
 view_highlight_range attachment,
 view_highlight_buffer attachment,
 view_render_hook attachment,
 view_word_complete_menu attachment,
 view_lister_loc attachment,
 view_previous_buffer attachment,
 buffer_map_id attachment,
 buffer_eol_setting attachment,
 buffer_lex_task attachment,
 sticky_jump_positions_handle attachment,
 attachment_tokens attachment,
 defcolor_vim_filebar_pop colors,
 defcolor_vim_chord_text colors,
 defcolor_vim_chord_unresolved colors,
 defcolor_vim_chord_error colors,
 vim_buffer_prev_visual attachment,
 vim_buffer_marks attachment,
 vim_view_jumps attachment,
 defcolor_function colors,
 defcolor_type colors,
 defcolor_primitive colors,
 defcolor_macro colors,
 defcolor_control colors,
 defcolor_struct colors,
 defcolor_non_text colors,
 defcolor_broken_link colors,
}

//~eof
