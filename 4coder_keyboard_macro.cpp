/*
4coder_keyboard_macro.cpp - Keyboard macro recording and replaying commands.
*/

// TOP

function Buffer_ID
get_keyboard_log_buffer(App *app){
 Models *models = (Models *)app->cmd_context;
 return models->keyboard_buffer->id;
}

function void
keyboard_macro_play_single_line(App *app, String macro_line)
{
 Scratch_Block scratch(app);
 Input_Event event = parse_keyboard_event(scratch, macro_line);
 if(event.kind != InputEventKind_None)
 {
  enqueue_virtual_event(app, &event);
 }
}

function void
keyboard_macro_play(App *app0, String macro)
{
 App_Cmd app_value = app_cmd_automated(app0);
 App_Cmd *app      = &app_value;
 
 View_ID   view = get_active_view(app, Access_ReadVisible);
 Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
 Scratch_Block scratch;
 
 History_Group history_group = history_group_begin(app, buffer);
 Input_Event history_merge = {};
 history_merge.kind           = InputEventKind_HistoryMerge;
 history_merge.history_buffer = buffer;
 history_merge.history_first  = 1 + buffer_history_get_current_state_index(app, buffer);
 
 List_String lines = string_split(scratch, macro, (u8*)"\n", 1);
 for(Node_String *node = lines.first;
     node != 0;
     node = node->next)
 {
  String line = string_skip_chop_whitespace(node->string);
  keyboard_macro_play_single_line(app0, line);
 }
 enqueue_virtual_event(app, &history_merge);
 history_group_end(history_group);
 
 human_has_edited_after_macro = false;
}

function b32
get_current_input_is_virtual(App *app)
{
 User_Input input = get_current_input(app);
 return(input.event.is_virtual);
}

////////////////////////////////

function void
keyboard_macro_start_recording(App_Cmd *app)
{
    if (global_keyboard_macro_is_recording ||
        get_current_input_is_virtual(app)){
        return;
 }
 
 Buffer_ID buffer = get_keyboard_log_buffer(app);
 global_keyboard_macro_is_recording = true;
 global_keyboard_macro_range.first = buffer_get_size(app, buffer);
}

function void
keyboard_macro_finish_recording(App_Cmd *app)
{
 if (!global_keyboard_macro_is_recording ||
     get_current_input_is_virtual(app)){
  return;
 }
 
 Buffer_ID buffer = get_keyboard_log_buffer(app);
 global_keyboard_macro_is_recording = false;
 i64 end = buffer_get_size(app, buffer);
 Buffer_Cursor cursor = buffer_compute_cursor(app, buffer, seek_pos(end));
 Buffer_Cursor back_cursor = buffer_compute_cursor(app, buffer, seek_line_col(cursor.line - 1, 1));
 global_keyboard_macro_range.opl = back_cursor.pos;
 human_has_edited_after_macro = false;
}

function void
keyboard_macro_replay(App_Cmd *app)
{
 if (global_keyboard_macro_is_recording or
     get_current_input_is_virtual(app))
 {
  return;
 }
 
 Buffer_ID buffer = get_keyboard_log_buffer(app);
 Scratch_Block scratch(app);
 String macro = push_buffer_range(app, scratch, buffer, global_keyboard_macro_range);
 keyboard_macro_play(app, macro);
}

// BOTTOM

