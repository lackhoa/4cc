#include "4coder_kv_utils.cpp"
#include "4coder_vim/4coder_vim_include.h"

struct KvQuailEntry
{
  char *key;
  char *insert;
  i32   delete_before;
  i32   delete_after;
  i32   cursor_index;
};

global KvQuailEntry *kv_quail_table;
global char         *kv_quail_keystroke_buffer;

// NOTE(kv): If keys are overlapping, you have to push the shorter key first in
// order to for the quail rule to work.
function void
kv_quail_defrule(Application_Links *app, char *key, char *insert,
                 i32 delete_before, i32 delete_after, i32 cursor_index)
{
  i32 entry_index = arrlen(kv_quail_table);
  // note: we keep the table sorted by key length, largest first, for overlapping keys.
  for (i32 table_index=0;
       table_index < arrlen(kv_quail_table);
       table_index++)
  {
    char *existing_key = kv_quail_table[table_index].key;
    if (gb_str_has_suffix(key, existing_key))
    {
      entry_index = table_index;  // change insertion index so this rule matches first
      break;
    }
  }
  KvQuailEntry entry = {key, insert, delete_before, delete_after, cursor_index};
  arrins(kv_quail_table, entry_index, entry);
}

function b32
kv_handle_quail(Application_Links *app, u8 character)
{
  kv_assert_defend(arrlen(kv_quail_keystroke_buffer) < 1024, 
                   {
                       print_message(app, SCu8("ERROR: kv_quail_keystroke_buffer grown too big!"));
                       return false;
                     });
  

  GET_VIEW_AND_BUFFER;

  char* &keybuf = kv_quail_keystroke_buffer;
  b32 substituted = false;
  arrput(keybuf, character);

  // loop to find a match in quail table
  for (i32 quail_index=0;
       ( quail_index < arrlen(kv_quail_table) ) && ( !substituted );
       quail_index++)
  {
    KvQuailEntry entry = kv_quail_table[quail_index];
    i32 keylen = strlen(entry.key);

    char *keys = keybuf + arrlen(keybuf) - keylen;
    substituted = ( strncmp(keys, entry.key, keylen) == 0 );
    if (substituted)
    {
      // NOTE(kv): Edit buffer content
      i64 pos = view_get_cursor_pos(app, view);

      Range_i64 range = { pos-entry.delete_before, pos + entry.delete_after };
      buffer_replace_range(app, buffer, range, SCu8(entry.insert));

      // NOTE(kv): move cursor
      move_horizontal_lines(app, entry.cursor_index);
    }
  }

  return substituted;
}

// NOTE(kv): re-appropriated from the vim layer
function b32
kv_handle_keyboard_input(Application_Links *app, Input_Event *event)
{
  ProfileScope(app, "kv_handle_keyboard_input");

  if (vim_state.mode == VIM_Replace)
  {
    return vim_handle_replace_mode(app, event);
  }
  else if (vim_state.mode == VIM_Replace)
  {
    return vim_handle_visual_insert_mode(app, event);
  }
  else if (event->kind == InputEventKind_TextInsert)
  {
    ProfileScope(app, "InputEventKind_TextInsert");
    String_Const_u8 in_string = to_writable(event);
    if ((vim_state.mode == VIM_Insert) &&
        (in_string.size == 1))
    {
      return kv_handle_quail(app, in_string.str[0]);
    }
    else return false;
  }
  else if (event->kind == InputEventKind_KeyStroke)
  {
    ProfileScope(app, "InputEventKind_KeyStroke");
    Key_Code code = event->key.code;
    if (code == KeyCode_Control ||
        code == KeyCode_Shift   ||
        code == KeyCode_Alt     ||
        code == KeyCode_Command ||
        code == KeyCode_Menu)
    {
      return false;
    }

    bool handled = false;

    /// Translate the KeyCode
    if (vim_state.chord_resolved) { vim_keystroke_text.size=0; vim_state.chord_resolved=false; }
    foreach(i, event->key.modifiers.count)
    {
      Key_Code mod = event->key.modifiers.mods[i];
      if(0){}
      else if(mod == KeyCode_Control){ code |= KeyMod_Ctl; }
      else if(mod == KeyCode_Shift){   code |= KeyMod_Sft; }
      else if(mod == KeyCode_Alt){     code |= KeyMod_Alt; }
      else if(mod == KeyCode_Command){ code |= KeyMod_Cmd; }
      else if(mod == KeyCode_Menu){    code |= KeyMod_Mnu; }
    }

    b32 was_in_sub_mode = (vim_state.sub_mode != SUB_None);
    u64 function_data=0;
    if (table_read(vim_maps + vim_state.mode + vim_state.sub_mode*VIM_MODE_COUNT, code, &function_data))
    {
      ProfileScope(app, "execute vim_func from vim_maps");
      Custom_Command_Function *vim_func = (Custom_Command_Function *)IntAsPtr(function_data);
      if(vim_func)
      {
        // Pre command stuff
        View_ID view = get_active_view(app, Access_ReadVisible);
        Managed_Scope scope = view_get_managed_scope(app, view);
        default_pre_command(app, scope);
        vim_pre_keystroke_size = vim_keystroke_text.size;
        vim_append_keycode(code);
        vim_state.active_command = vim_func;
        vim_state.chord_resolved = true;
        if(vim_func == no_op){ vim_state.chord_resolved = bitmask_2; }

        vim_func(app);

        // Post command stuff
        default_post_command(app, scope);
        vim_state.active_command = 0;

        handled = true;
      }
    }
    else if (vim_state.mode == VIM_Insert)
    {
      // passthrough to do text insertion
    }
    else 
    { // global keymap passthrough
      String_ID map_id = vars_save_string_lit("keys_global");
      Command_Binding command_binding = map_get_binding_non_recursive(&framework_mapping, map_id, event);
      if (command_binding.custom) {
        vim_reset_state();
        command_binding.custom(app);
        vim_keystroke_text.size = 0;
      } else {
        vim_append_keycode(code);
        vim_state.chord_resolved = bitmask_2;
      }
      handled = true;
    }

    if(was_in_sub_mode){ vim_state.sub_mode = SUB_None; }

    if(vim_keystroke_text.size >= vim_keystroke_text.cap){ vim_keystroke_text.size = 0; }

    return handled;
  }
  else return false;
}

CUSTOM_COMMAND_SIG(kv_view_input_handler)
CUSTOM_DOC("Input consumption loop for view behavior (why is this a command?)")
{
	Scratch_Block scratch(app);
	default_input_handler_init(app, scratch);

	View_ID view = get_this_ctx_view(app, Access_Always);
	Managed_Scope scope = view_get_managed_scope(app, view);

	for(User_Input input = get_next_input(app, EventPropertyGroup_Any, 0);
        !input.abort;
        input  = get_next_input(app, EventPropertyGroup_Any, 0))
    {
        if (input.event.kind == InputEventKind_KeyStroke) seconds_since_last_keystroke = 0;

#if VIM_USE_BOTTOM_LISTER
		// Clicking on lister items outside of original view panel is a hack
		if(vim_lister_view_id != 0 && view != vim_lister_view_id){
			view_set_active(app, vim_lister_view_id);
			leave_current_input_unhandled(app);
			continue;
		}
#endif

		ProfileScopeNamed(app, "before view input", view_input_profile);

        // NOTE(allen): Mouse Suppression
        Event_Property event_properties = get_event_properties(&input.event);
        b32 is_mouse_event = event_properties & EventPropertyGroup_AnyMouseEvent;
        if(suppressing_mouse && is_mouse_event) {
          continue;
        }

        if (!is_mouse_event && input.event.kind != InputEventKind_None)
        {
        	vim_keystroke_text.size = 0;
        	vim_cursor_blink = 0;
        }

        if ( kv_handle_keyboard_input(app, &input.event) )
        {
          continue;
        }

        // NOTE(allen): Get binding
        Implicit_Map_Result map_result;
        {
          ProfileScope(app, "implicit_map_function");
          if(implicit_map_function == 0){
            implicit_map_function = default_implicit_map;
          }
          map_result = implicit_map_function(app, 0, 0, &input.event);
          if(map_result.command == 0){
            leave_current_input_unhandled(app);
            continue;
          }
        }

        // NOTE(allen): Run the command and pre/post command stuff
        default_pre_command(app, scope);
		ProfileCloseNow(view_input_profile);

        {
          ProfileScope(app, "map_result_command_profile");
          map_result.command(app);
        }

		ProfileScope(app, "after view input");
		default_post_command(app, scope);
	}
}
