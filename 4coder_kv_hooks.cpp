function i32
kv_file_save(App_Cmd *app, Buffer_ID buffer_id)
{
 default_file_save(app, buffer_id);
 vim_file_save(app, buffer_id);
 return 0;
}
function i32
kv_new_file(App_Cmd *app, Buffer_ID buffer_id)
{
	Scratch_Block scratch(app);
	String filename = push_buffer_base_name(app, scratch, buffer_id);
	if(string_match(string_postfix(filename, 4), strlit(".bat")))
 {
		Buffer_Insertion insert = begin_buffer_insertion_at_buffered2(app, buffer_id, 0, scratch, KB(16));
		insertf(&insert, "@echo off" "\n");
		end_buffer_insertion(&insert);
		return 0;
	}
 
 // todo bash shell here
 
	return 0;
}
function i32
kv_begin_buffer(App_Cmd *app, Buffer_ID buffer)
{
 ProfileBlock( "[kv] Begin Buffer");
 
 Scratch_Block scratch(app);
 b32 treat_as_code = false;
 String8 filename = push_buffer_filepath(app, scratch, buffer);
 String8 buffer_name = push_buffer_base_name(app, scratch, buffer);
 
 // NOTE(rjf): Treat as code if the config tells us to.
 if(treat_as_code == false)
 {
  if(filename.size > 0)
  {
   String8 treat_as_code_string = def_get_config_string(scratch, vars_intern_lit("treat_as_code"));
   String8_Array extensions = parse_extension_line_to_extension_list(app, scratch, treat_as_code_string);
   String8 ext = path_extension(filename);
   for(i1 i = 0; i < extensions.count; ++i)
   {
    if(string_match(ext, extensions.strings[i]))
    {
     treat_as_code = true;
     break;
    }
   }
  }
 }
 
 // NOTE(rjf): Treat as code if we've identified the language of a file.
 if(treat_as_code == false)
 {
  F4_Language *language = F4_LanguageFromBuffer(app, buffer);
  if (language)
  {
   treat_as_code = true;
  }
 }
 
 String_ID file_map_id = vars_intern_lit("keys_file");
 String_ID code_map_id = vars_intern_lit("keys_code");
 
 Command_Map_ID map_id = (treat_as_code) ? (code_map_id) : (file_map_id);
 Managed_Scope scope = buffer_get_managed_scope(app, buffer);
 Command_Map_ID *map_id_ptr = scope_attachment(app, scope, buffer_map_id, Command_Map_ID);
 *map_id_ptr = map_id;
 
 Line_Ending_Kind setting = guess_line_ending_kind_from_buffer(app, buffer);
 Line_Ending_Kind *eol_setting = scope_attachment(app, scope, buffer_eol_setting, Line_Ending_Kind);
 *eol_setting = setting;
 
 // NOTE(allen): Decide buffer settings
 b32 use_lexer = false;
 if(treat_as_code)
 {
  use_lexer = true;
 }
 
 if(use_lexer)
 {
  submit_full_lex_work(app, buffer);
 }
 
 buffer_set_layout(app, buffer, layout_basic);
 vim_begin_buffer(app, buffer);
 
 // no meaning for return
 return(0);
}
BUFFER_EDIT_RANGE_SIG(kv_buffer_edit_range)
{
 // NOTE(kv): Fleury
 F4_BufferEditRange(app, buffer_id, new_range, old_cursor_range, automated);
 Game_API *game = get_game_code(Game_On);
 if(game)
 {
  game->game_buffer_edit_range(ed_game_state_pointer, app, buffer_id, new_range, old_cursor_range);
 }
 return 0;
}
function Rect_f32
kv_buffer_region_hook(App *app, View_ID view, Rect_f32 region)
{
 Buffer_ID buffer = view_get_buffer(app, view, 0);
 Face_ID face_id = get_face_id(app, buffer);
 Face_Metrics face_metrics = get_face_metrics(app, face_id);
 v1 line_height = face_metrics.line_height;
 v1 vim_bottom_reserve_height = 4.f*line_height;
 region.y1 -= vim_bottom_reserve_height;
 return region;
}
function void
kv_tick(App *app, Frame_Info frame)
{
 Scratch_Block scratch(app);
 DEBUG_entries.count = 0;
 
 // NOTE(kv): F4
 F4_Index_Tick(app);
 
 // NOTE(kv): Default tick stuff from the 4th dimension:
 default_tick(app, frame);
 
 // NOTE(kv): vim
 vim_animate_filebar(app, frame);
 vim_animate_cursor(app, frame);
 vim_cursor_blink++;
 
 seconds_since_last_keystroke += frame.literal_dt;
 
 {// NOTE(kv): autosave / reload
  local_persist v1 seconds_since_last_autosave = 0;
  seconds_since_last_autosave += frame.literal_dt;
  v1 AUTOSAVE_PERIOD_SECONDS = 5.0f;
  
  if(seconds_since_last_keystroke > AUTOSAVE_PERIOD_SECONDS and
     seconds_since_last_autosave > AUTOSAVE_PERIOD_SECONDS)
  {
   seconds_since_last_autosave = 0;
  }
  b32 should_autosave = seconds_since_last_autosave == 0;
  
  u32 saved_count = 0;
  u32 reloaded_count = 0;
  {
   ProfileBlock( "save all dirty buffers");
   for(Buffer_ID buffer = get_buffer_next(app, 0, Access_ReadWriteVisible);
       buffer != 0;
       buffer = get_buffer_next(app, buffer, Access_ReadWriteVisible))
   {
    switch(buffer_get_dirty_state(app, buffer))
    {
     case DirtyState_UnsavedChanges:
     {
      if(should_autosave){
       String filename = push_buffer_filepath(app, scratch, buffer);
       b32 res = buffer_save(app, buffer, filename, 0);
       if(res){
        saved_count++;
       }
      }
     }break;
     
     case DirtyState_UnloadedChanges:
     {
      Buffer_Reopen_Result res = buffer_reopen(app, buffer, 0);
      if(res == BufferReopenResult_Reopened){
       reloaded_count++;
      }
     }break;
    }
   }
  }
  if(saved_count){
   String msg = push_stringf(scratch, "auto-saved %u buffers", saved_count);
   vim_set_bottom_text(msg);
   
  }else if(reloaded_count){
   String msg = push_stringf(scratch, "auto-reloaded %u buffers", reloaded_count);
   vim_set_bottom_text(msg);
  }
  
  animate_in_n_milliseconds(app, u32(1e3 * AUTOSAVE_PERIOD_SECONDS));
 }
}
//-