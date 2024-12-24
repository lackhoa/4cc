/*
4coder_combined_write_commands.cpp - Commands for writing text specialized for particular contexts.
*/

// TOP

function void
write_string(App_Cmd *app, View_ID view, Buffer_ID buffer, String string){
 i64 pos = view_get_cursor_pos(app, view);
 buffer_replace_range(app, buffer, Ii64(pos), string);
 view_set_cursor_and_preferred_x(app, view, seek_pos(pos + string.size));
}

function void
write_string(App_Cmd *app, String string){
 View_ID view = get_active_view(app, Access_ReadWriteVisible);
 Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
 write_string(app, view, buffer, string);
}

function void
write_named_comment_string(App_Cmd *app, char *type_string){
 Scratch_Block scratch(app);
 String name = def_get_config_string(scratch, vars_intern_lit("user_name"));
 String str = {};
 if (name.size > 0){
  str = push_stringfz(scratch, "// %s(%.*s): ", type_string, string_expand(name));
 }
 else{
  str = push_stringfz(scratch, "// %s: ", type_string);
 }
 write_string(app, str);
}

function void
long_braces(App_Cmd *app, char *text, i1 size){
 View_ID view = get_active_view(app, Access_ReadWriteVisible);
 Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
 i64 pos = view_get_cursor_pos(app, view);
 buffer_replace_range(app, buffer, Ii64(pos), SCu8(text, size));
 view_set_cursor_and_preferred_x(app, view, seek_pos(pos + 2));
 auto_indent_buffer(app, buffer, Ii64_size(pos, size));
 move_past_lead_whitespace(app, view, buffer);
}

function void
open_long_braces(App_Cmd *app)
{
 char text[] = "{\n\n}";
 i1 size = sizeof(text) - 1;
 long_braces(app, text, size);
}

function void
open_long_braces_semicolon(App_Cmd *app)
{
 char text[] = "{\n\n};";
 i1 size = sizeof(text) - 1;
 long_braces(app, text, size);
}

function void
open_long_braces_break(App_Cmd *app)
{
 char text[] = "{\n\n}break;";
 i1 size = sizeof(text) - 1;
 long_braces(app, text, size);
}

function void
if0_off(App_Cmd *app)
{
 place_begin_and_end_on_own_lines(app, "#if 0", "#endif");
}

function i64
get_start_of_line_at_cursor(App *app, View_ID view, Buffer_ID buffer){
    i64 pos = view_get_cursor_pos(app, view);
    i64 line = get_line_number_from_pos(app, buffer, pos);
    return(get_pos_past_lead_whitespace_from_line_number(app, buffer, line));
}

function b32
c_line_comment_starts_at_position(App *app, Buffer_ID buffer, i64 pos){
    b32 alread_has_comment = false;
    u8 check_buffer[2];
    if (buffer_read_range(app, buffer, Ii64(pos, pos + 2), check_buffer)){
  if (check_buffer[0] == '/' && check_buffer[1] == '/'){
   alread_has_comment = true;
  }
 }
 return(alread_has_comment);
}

function void
comment_line(App_Cmd *app)
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
 i64 pos = get_start_of_line_at_cursor(app, view, buffer);
 b32 alread_has_comment = c_line_comment_starts_at_position(app, buffer, pos);
 if (!alread_has_comment){
  buffer_replace_range(app, buffer, Ii64(pos), strlit("//"));
 }
}

function void
uncomment_line(App_Cmd *app)
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
 i64 pos = get_start_of_line_at_cursor(app, view, buffer);
 b32 alread_has_comment = c_line_comment_starts_at_position(app, buffer, pos);
 if (alread_has_comment){
  buffer_replace_range(app, buffer, Ii64(pos, pos + 2), empty_string);
 }
}

function void
comment_line_toggle(App_Cmd *app)
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    i64 pos = get_start_of_line_at_cursor(app, view, buffer);
    b32 alread_has_comment = c_line_comment_starts_at_position(app, buffer, pos);
    if (alread_has_comment){
        buffer_replace_range(app, buffer, Ii64(pos, pos + 2), empty_string);
    }
    else{
        buffer_replace_range(app, buffer, Ii64(pos), strlit("//"));
    }
}

// BOTTOM

