
#include "4coder_fleury_bindings.cpp"
#include "4coder_fleury_audio.h"
#include "4coder_fleury_cursor.cpp"

Audio_Clip PowerWAV = {};
Audio_Clip HitWAV = {};
Audio_Control PowerWAVControl = {};

function void
casey_demo_audio(App_Cmd *app)
{
    F4_RequireWAV(app, &PowerWAV, "sounds/hit.wav");
    PowerWAV.channel_volume[0] = 0.5f;
 PowerWAV.channel_volume[1] = 0.25f;
	if(!def_audio_is_playing(&PowerWAVControl))
	{
  def_audio_play_clip(PowerWAV, &PowerWAVControl);
	}
}

function void
casey_demo_audio_switch_panel(App_Cmd *app)
{
 f32 Temp = PowerWAVControl.channel_volume[0];
	PowerWAVControl.channel_volume[0] = PowerWAVControl.channel_volume[1];
 PowerWAVControl.channel_volume[1] = Temp;
	change_active_primary_view(app);
}

function void
casey_demo_audio_one_shot(App_Cmd *app)
{
 F4_RequireWAV(app, &HitWAV, "sounds/hit.wav");
 HitWAV.channel_volume[0] = 0.5f;
 HitWAV.channel_volume[1] = 0.5f;
 def_audio_play_clip(HitWAV, 0);
}

function void
casey_seek_beginning_of_line_and_tab(App_Cmd *app)
{
 seek_beginning_of_line(app);
 auto_indent_line_at_cursor(app);
}

function void
casey_clean_file_and_save(App_Cmd *app)
{
	View_ID view = get_active_view(app, Access_ReadWriteVisible);
 Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
	clean_all_lines_buffer(app, buffer, CleanAllLinesMode_RemoveBlankLines);
 rewrite_lines_to_lf(app, buffer);
 
 save(app);
}

function void
casey_switch_to_keybinding_0(App_Cmd *app)
{
	switch_to_keybinding_0(app);
 global_hide_region_boundary = true;
}

function void
casey_switch_to_keybinding_1(App_Cmd *app)
{
	switch_to_keybinding_1(app);
	global_hide_region_boundary = false;
}

function void
casey_newline_and_indent(App_Cmd *app)
{
    // NOTE(allen): The idea here is that if the current buffer is
    // read-only, it cannot be edited anyway.  So instead let the return
    // key indicate an attempt to interpret the line as a location to jump to.
    
	View_ID view = get_active_view(app, Access_Always);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    
    if (buffer_get_access_flags(app, buffer) & Access_Write)
	{
		write_text(app, strlit("\n"));
        auto_indent_line_at_cursor(app);
 }
 else
	{
  goto_jump_at_cursor(app);
 }
}

function void
casey_delete_to_end_of_line(App_Cmd *app)
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    i64 pos = view_get_cursor_pos(app, view);
    i64 line = get_line_number_from_pos(app, buffer, pos);
    Range_i64 range = get_line_pos_range(app, buffer, line);
	if(pos == range.end)
	{
		range.end = pos + 1;
		range.start = pos;
	}
	else
	{
		range.start = pos + 1;
	}
    
    i1 size = (i1)buffer_get_size(app, buffer);
    range.end = clamp_max(range.end, size);
    if (range_size(range) == 0 ||
     buffer_get_char(app, buffer, range.end - 1) != '\n'){
  range.start -= 1;
  range.first = clamp_min(0, range.first);
 }
 buffer_replace_range(app, buffer, range, strlit(""));
}

function void
casey_find_matching_file(App_Cmd *app)
{
    View_ID view = get_active_view(app, Access_Always);
 Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
 Buffer_ID new_buffer = 0;
 if (get_cpp_matching_file(app, buffer, &new_buffer)){
  view_set_buffer(app, view, new_buffer, 0);
 }
}

function void
casey_go_to_code_peek(App_Cmd *app)
{
 fleury_go_to_definition(app);
}
//