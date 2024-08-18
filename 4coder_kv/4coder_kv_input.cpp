struct KvQuailEntry
{
  char *key;
  char *insert;
  i1   delete_before;
  i1   delete_after;
  i1   cursor_index;
};

global KvQuailEntry *kv_quail_table;
global char         *kv_quail_keystroke_buffer;

// NOTE(kv): If keys are overlapping, you have to push the shorter key first in
// order to for the quail rule to work.
function void
kv_quail_defrule(App *app, char *key, char *insert,
                 i1 delete_before, i1 delete_after, i1 cursor_index)
{
  i1 entry_index = arrlen(kv_quail_table);
  // note: we keep the table sorted by key length, largest first, for overlapping keys.
  for (i1 table_index=0;
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
kv_handle_text_insert(App *app, u8 character)
{
 assert_defend(arrlen(kv_quail_keystroke_buffer) < 1024, 
               {
                print_message(app, SCu8("ERROR: 'kv_quail_keystroke_buffer' grown too big!"));
                return false;
               });
 
 GET_VIEW_AND_BUFFER;
 
 char* &keybuf = kv_quail_keystroke_buffer;
 b32 substituted = false;
 arrput(keybuf, character);
 
 // loop to find a match in quail table
 for (i1 quail_index=0;
      ( quail_index < arrlen(kv_quail_table) ) && ( !substituted );
      quail_index++)
 {
  KvQuailEntry entry = kv_quail_table[quail_index];
  i1 keylen = strlen(entry.key);
  
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
   
   // NOTE: @Hack to indent the line open brace.
   if (strncmp(keys, "[[", 2) == 0)
   {
    auto_indent_line_at_cursor(app);
   }
  }
 }
 
 return substituted;
}


function b32
kv_handle_vim_keyboard_input(App *app, Input_Event *event)
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
        String8 in_string = to_writable(event);
        if ((vim_state.mode == VIM_Insert) &&
            (in_string.size == 1))
        {
            return kv_handle_text_insert(app, in_string.str[0]);
        }
        else return false;
    }
    else if (event->kind == InputEventKind_KeyStroke)
    {
        ProfileScope(app, "InputEventKind_KeyStroke");
        Key_Code code = event->key.code;
        if ( is_modifier_key(code) )
        {
            return false;
        }
       
        {
            Input_Modifier_Set mods = event->key.modifiers;
            Key_Code modifiers = cast(Key_Code)pack_modifiers(mods.mods, mods.count);
            code = (Key_Code)(code|modifiers);
        }
        
        bool handled = false;
        
        // NOTE: Translate the Key_Code
        if (vim_state.chord_resolved) { vim_keystroke_text.size=0; vim_state.chord_resolved=false; }
        
        b32 was_in_sub_mode = (vim_state.sub_mode != SUB_None);
        u64 function_data = 0;
        if ( table_read(vim_maps + vim_state.mode + vim_state.sub_mode*VIM_MODE_COUNT, code, &function_data) )
        {
            ProfileScope(app, "execute vim_func from vim_maps");
            Custom_Command_Function *vim_func = (Custom_Command_Function *)IntAsPtr(function_data);
            if (vim_func)
            {
                // Pre command stuff
                View_ID view = get_active_view(app, Access_ReadVisible);
                Managed_Scope scope = view_get_managed_scope(app, view);
                default_pre_command(app, scope);
                vim_pre_keystroke_size = vim_keystroke_text.size;
                vim_append_keycode(code);
                vim_state.active_command = vim_func;
                vim_state.chord_resolved = true;
                if (vim_func == no_op) { vim_state.chord_resolved = bitmask_2; }
                
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
            String_ID map_id = vars_intern_lit("keys_global");
            Command_Binding command_binding = map_get_binding_non_recursive(&framework_mapping, map_id, event);
            if (command_binding.custom) 
            {
                vim_reset_state();
                command_binding.custom(app);
                vim_keystroke_text.size = 0;
            } 
            else 
            {
                vim_append_keycode(code);
                vim_state.chord_resolved = bitmask_2;
            }
            handled = true;
        }
        
        if (was_in_sub_mode) { vim_state.sub_mode = SUB_None; }
        
        if (vim_keystroke_text.size >= vim_keystroke_text.cap) { vim_keystroke_text.size = 0; }
        
        return handled;
    }
    else return false;
}

// TODO(kv): This key tracking is inaccurate at least in the case when you alt+tab out of the app
internal void
update_game_key_states(Input_Event *event)
{
 b32 keydown = (event->kind == InputEventKind_KeyStroke);
 b32 keyup   = (event->kind == InputEventKind_KeyRelease);
 if (keydown || keyup)
 {
  Key_Code keycode = event->key.code;
  // NOTE: We have system_get_keyboard_modifiers to track modifier keys already
  if ( !is_modifier_key(keycode) )
  {
   global_game_key_states       [keycode] = keydown;
   global_game_key_state_changes[keycode]++;
  }
 }
}

internal void
kv_view_input_handler(App *app)
{
 Scratch_Block scratch(app);
 default_input_handler_init(app, scratch);
 
 View_ID view = get_this_ctx_view(app, Access_Always);
 Managed_Scope scope = view_get_managed_scope(app, view);
 
 for (User_Input input = get_next_input(app, EventPropertyGroup_Any, 0);
      !input.abort;
      input  = get_next_input(app, EventPropertyGroup_Any, 0))
 {
  Temp_Memory_Block temp(scratch);
  
  if (input.event.kind == InputEventKind_KeyStroke)
   seconds_since_last_keystroke = 0;
  
#if VIM_USE_BOTTOM_LISTER
  // Clicking on lister items outside of original view panel is a hack
  if ((vim_lister_view_id != 0) && 
      (view != vim_lister_view_id))
  {
   view_set_active(app, vim_lister_view_id);
   leave_current_input_unhandled(app);
   continue;
  }
#endif
  
  ProfileScopeNamed(app, "before view input", view_input_profile);
  
  // NOTE(allen): Mouse Suppression
  Event_Property event_properties = get_event_properties(&input.event);
  b32 is_mouse_event = event_properties & EventPropertyGroup_AnyMouseEvent;
  if (!(is_mouse_event && suppressing_mouse))
  {
   if (!is_mouse_event && (input.event.kind != InputEventKind_None))
   {
    vim_keystroke_text.size = 0;
    vim_cursor_blink = 0;
   }
   
   update_game_key_states(&input.event);
   
   b32 is_game_buffer;
   {
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    is_game_buffer = buffer_viewport_id(app, buffer);
   }
   
   b32 handled = false;
   
   auto game = get_game_code();
   if ( game )
   {
    handled = game->is_event_handled_by_game(app, &input.event, is_game_buffer);
   }
   
   if (!handled)
   {// NOTE: The normal text editor
    handled = kv_handle_vim_keyboard_input(app, &input.event);
   }
   
   if ( !handled )
   {
    // NOTE(allen): Get binding
    if (implicit_map_function == 0)
     implicit_map_function = default_implicit_map;
    
    Implicit_Map_Result map_result = implicit_map_function(app, 0, 0, &input.event);
    if ( map_result.command )
    {
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
    else
     leave_current_input_unhandled(app);
   }
  }
 }
}

internal void 
kv_newline_and_indent(App *app)
{
    GET_VIEW_AND_BUFFER;
    HISTORY_GROUP_SCOPE;
    write_text(app, str8lit("\n"), true);
    
    i64 curpos = view_get_cursor_pos(app, view);
    u8 character = buffer_get_char(app, buffer, curpos);
    if (character == /*{*/'}')
    {// NOTE: Handling for brace
        write_text(app, str8lit("\n"), true);
        auto_indent_line_at_cursor(app);
        move_vertical_lines(app, -1);
    }
    
    auto_indent_line_at_cursor(app);
}
