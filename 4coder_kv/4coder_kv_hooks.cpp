#include "4coder_kv_utils.cpp"
#include "4coder_fleury/4coder_fleury_kv.cpp"
#include "4coder_fleury/4coder_fleury_calc.cpp"

BUFFER_HOOK_SIG(kv_file_save)
{
  default_file_save(app, buffer_id);
  vim_file_save(app, buffer_id);
  return 0;
}

BUFFER_HOOK_SIG(kv_new_file)
{
	Scratch_Block scratch(app);
	String_Const_u8 file_name = push_buffer_base_name(app, scratch, buffer_id);
	if(string_match(string_postfix(file_name, 4), string_u8_litexpr(".bat"))){
		Buffer_Insertion insert = begin_buffer_insertion_at_buffered(app, buffer_id, 0, scratch, KB(16));
		insertf(&insert, "@echo off" "\n");
		end_buffer_insertion(&insert);
		return 0;
	}

    // todo bash shell here

	return 0;
}

#if KV_INTERNAL
inline void
kv_fiddle(Application_Links *app)
{
  f32 linum = get_current_line(app);
  // kv_set_fiddle_value(linum);
}
#endif

function Tick_Function kv_tick;
function void kv_tick(Application_Links *app, Frame_Info frame_info)
{
  // NOTE(kv): F4
  linalloc_clear(&global_frame_arena);
  F4_Index_Tick(app);
  F4_CLC_Tick(frame_info);

  // NOTE(kv): Default tick stuff from the 4th dimension:
  default_tick(app, frame_info);

  // NOTE(kv): vim
  vim_animate_filebar(app, frame_info);
  vim_animate_cursor(app, frame_info);
  vim_cursor_blink++;

  // NOTE(kv): autosave
  f32 AUTOSAVE_PERIOD_SECONDS = 5.0f;
  seconds_since_last_keystroke += frame_info.literal_dt;
  if (seconds_since_last_keystroke > AUTOSAVE_PERIOD_SECONDS)
  {
    seconds_since_last_keystroke = 0;
    b32 saved_at_least_one_buffer = false;
    {
      ProfileScope(app, "save all dirty buffers");
      Scratch_Block scratch(app);
      for (Buffer_ID buffer = get_buffer_next(app, 0, Access_ReadWriteVisible);
           buffer != 0;
           buffer = get_buffer_next(app, buffer, Access_ReadWriteVisible))
      {
        switch(buffer_get_dirty_state(app, buffer))
        {
          case DirtyState_UnsavedChanges:
          {
            saved_at_least_one_buffer = true;
            String_Const_u8 file_name = push_buffer_file_name(app, scratch, buffer);
            buffer_save(app, buffer, file_name, 0);
          }
          break;

          case DirtyState_UnloadedChanges:
          {
            buffer_reopen(app, buffer, 0);
            String_Const_u8 file_name = push_buffer_file_name(app, scratch, buffer);
            printf_message(app, scratch, "automatically reloaded file %.*s\n", string_expand(file_name));
          }break;
        }
        
      }
    }
    if (saved_at_least_one_buffer) {
      kv_print_message(app, "auto-saved all dirty buffers\n");
    }
  }
 
#if KV_INTERNAL
 
  kv_fiddle(app);
  
#endif
}

BUFFER_HOOK_SIG(kv_begin_buffer)
{
  ProfileScope(app, "[kv] Begin Buffer");
  vim_begin_buffer(app, buffer_id);
  
  Scratch_Block scratch(app);
  b32 treat_as_code = false;
  String_Const_u8 file_name = push_buffer_file_name(app, scratch, buffer_id);
  String_Const_u8 buffer_name = push_buffer_base_name(app, scratch, buffer_id);
  
  // NOTE(rjf): Treat as code if the config tells us to.
  if(treat_as_code == false)
  {
    if(file_name.size > 0)
    {
      String_Const_u8 treat_as_code_string = def_get_config_string(scratch, vars_save_string_lit("treat_as_code"));
      String_Const_u8_Array extensions = parse_extension_line_to_extension_list(app, scratch, treat_as_code_string);
      String_Const_u8 ext = string_file_extension(file_name);
      for(i32 i = 0; i < extensions.count; ++i)
      {
        if(string_match(ext, extensions.strings[i]))
        {
          treat_as_code = true;
          break;
        }
      }
    }
  }
 
  // NOTE(rjf): Treat as code for *calc* buffers.
  if(treat_as_code == false)
  {
    if(string_match(buffer_name, string_u8_litexpr("*calc*")))
    {
      treat_as_code = true;
    }
  }
  
  // NOTE(rjf): Treat as code if we've identified the language of a file.
  if(treat_as_code == false)
  {
    F4_Language *language = F4_LanguageFromBuffer(app, buffer_id);
    if (language)
    {
      treat_as_code = true;
    }
  }
  
  String_ID file_map_id = vars_save_string_lit("keys_file");
  String_ID code_map_id = vars_save_string_lit("keys_code");
  
  Command_Map_ID map_id = (treat_as_code) ? (code_map_id) : (file_map_id);
  Managed_Scope scope = buffer_get_managed_scope(app, buffer_id);
  Command_Map_ID *map_id_ptr = scope_attachment(app, scope, buffer_map_id, Command_Map_ID);
  *map_id_ptr = map_id;
  
  Line_Ending_Kind setting = guess_line_ending_kind_from_buffer(app, buffer_id);
  Line_Ending_Kind *eol_setting = scope_attachment(app, scope, buffer_eol_setting, Line_Ending_Kind);
  *eol_setting = setting;
  
  // NOTE(allen): Decide buffer settings
  b32 wrap_lines = false;  // NOTE(kv): vim doesn't allow line wrap (in "vim_begin_buffer_inner")
  b32 use_lexer = false;
  if(treat_as_code)
  {
    use_lexer = true;
  }
  
  if (use_lexer)
  {
    ProfileBlock(app, "begin buffer kick off lexer");
    Async_Task *lex_task_ptr = scope_attachment(app, scope, buffer_lex_task, Async_Task);
    *lex_task_ptr = async_task_no_dep(&global_async_system, F4_DoFullLex_ASYNC, make_data_struct(&buffer_id));
  }
  
  {
    b32 *wrap_lines_ptr = scope_attachment(app, scope, buffer_wrap_lines, b32);
    *wrap_lines_ptr = wrap_lines;
  }
  
  if (use_lexer)
  {
    buffer_set_layout(app, buffer_id, layout_virt_indent_index_generic);
  }
  else
  {
    if (treat_as_code)
    {
      buffer_set_layout(app, buffer_id, layout_virt_indent_literal_generic);
    }
    else{
      buffer_set_layout(app, buffer_id, layout_generic);
    }
  }
  
  // no meaning for return
  return(0);
}

BUFFER_EDIT_RANGE_SIG(kv_buffer_edit_range)
{
  // NOTE(kv): Fleury
  F4_BufferEditRange(app, buffer_id, new_range, old_cursor_range);
  // NOTE(kv): vim
  fold_buffer_edit_range_inner(app, buffer_id, new_range, old_cursor_range);
  return 0;
}
