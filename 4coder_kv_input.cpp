//-#processed
struct KvQuailEntry
{
 char *key;
 char *insert;
 i1   delete_before;
 i1   delete_after;
 i1   cursor_index;
};
global darray(KvQuailEntry) kv_quail_table;

// NOTE(kv): If keys are overlapping, you have to push the shorter key first in
// order to for the quail rule to work.
function void
kv_quail_defrule(App *app, char *key, char *insert,
                 i1 delete_before, i1 delete_after, i1 cursor_index)
{
 darray(KvQuailEntry) *table = &kv_quail_table;
 i1 entry_index = (i1)table->count;
 // note: We keep the table sorted by key length, largest first, for overlapping keys.
 for (i32 table_index=0;
      table_index < table->count;
      table_index++)
 {
  char *existing_key = table->items[table_index].key;
  if(starts_with(SCu8(key), SCu8(existing_key))){
   entry_index = table_index;  // change insertion index so this rule matches first
   break;
  }
 }
 
 KvQuailEntry entry = {key, insert, delete_before, delete_after, cursor_index};
 table->push();
 for(i32 i=table->count-1;
     i >= entry_index+1;
     i--)
 {
  table->items[i] = table->items[i-1];
 }
 table->items[entry_index] = entry;
}

function void 
kvInitQuailTable(App *app)
{
 init_dynamic(kv_quail_table, &thread_permanent_arena, 64);
 
#define QUAIL_DEFRULE(KEY, VALUE) \
kv_quail_defrule(app, KEY, VALUE, (i1)strlen(KEY)-1, 0, (i1)strlen(VALUE))
 
 QUAIL_DEFRULE(",,", "_");
 kv_quail_defrule(app, ",,,", "__", 1,0,2);
 
 //
 QUAIL_DEFRULE(",.", "->");
 kv_quail_defrule(app, ",.,", "<>", 2,0,1);
 //
 kv_quail_defrule(app, "9", "()", 0,0,1);
 kv_quail_defrule(app, "99", "9", 1,1,1);  // NOTE escape
 //
 kv_quail_defrule(app, "[", "[]", 0,0,1);
 // {
 kv_quail_defrule(app, "[[", "{}", 1,1,1);
 QUAIL_DEFRULE("]]", "}");
 //
 kv_quail_defrule(app, "''", "\"\"", 1,0,1);
 QUAIL_DEFRULE("leq", "<=");
 QUAIL_DEFRULE("geq", ">=");
 QUAIL_DEFRULE("neq", "!=");
 
#undef QUAIL_DEFRULE
}

function b32
kv_handle_text_insert(App_Cmd *app, u8 character)
{
 darray(char) &keybuf = kv_quail_keystroke_buffer;
 kv_assert(keybuf.count < 1024);
 
 GET_VIEW_AND_BUFFER;
 
 b32 substituted = false;
 push(&kv_quail_keystroke_buffer, char(character));
 
 // NOTE loop to find a match in quail table
 for(i32 quail_index=0;
     ( quail_index < kv_quail_table.count ) and ( !substituted );
     quail_index++)
 {
  KvQuailEntry entry = kv_quail_table[quail_index];
  i1 keylen = (i1)strlen(entry.key);
  
  i32 keybuf_start_index = keybuf.count - keylen;
  if(keybuf_start_index >= 0)
  {
   char *keys = keybuf.items + keybuf_start_index;
   substituted = ( strncmp(keys, entry.key, keylen) == 0 );
   if(substituted)
   {
    // NOTE(kv): Edit buffer content
    i64 pos = view_get_cursor_pos(app, view);
    
    Range_i64 range = { pos-entry.delete_before, pos + entry.delete_after };
    buffer_replace_range(app, buffer, range, SCu8(entry.insert));
    
    // NOTE(kv): move cursor
    move_horizontal_lines(app, entry.cursor_index);
    
    // NOTE #Hack to indent the line open brace.
    if(strncmp(keys, "[[", 2) == 0)
    {
     auto_indent_line_at_cursor(app);
    }
   }
  }
 }
 
 return substituted;
}

function b32
kv_handle_vim_keyboard_input(App *app0, Input_Event *event)
{
 App_Cmd app_value = app_cmd_automated(app0);
 App_Cmd *app      = &app_value;
 ProfileBlock( "kv_handle_keyboard_input");
 
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
  ProfileBlock( "InputEventKind_TextInsert");
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
  ProfileBlock( "InputEventKind_KeyStroke");
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
  if ( table_read(vim_maps + vim_state.mode + (u32)vim_state.sub_mode*(u32)VIM_MODE_COUNT, code, &function_data) )
  {
   ProfileBlock( "execute vim_func from vim_maps");
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
function void
register_keyboard_event_to_the_game(Input_Event &event)
{
 if(event.kind == InputEventKind_KeyStroke or
    event.kind == InputEventKind_KeyRelease)
 {
  // NOTE(kv) kinda messed up but ok...
  Key_Code key = event.key.code;
  global_game_key_state_changes[key]++;
 }
}
function void
kv_view_input_handler(App *app0)
{
 Scratch_Block tmp;
 default_input_handler_init(app0, tmp);
 
 View_ID view = get_this_ctx_view(app0, Access_Always);
 Managed_Scope scope = view_get_managed_scope(app0, view);
 
 for(User_Input input = get_next_input(app0, EventPropertyGroup_Any, 0);
     !input.abort;
     input  = get_next_input(app0, EventPropertyGroup_Any, 0))
 {
  // NOTE(kv) IMPORTANT Guys, we need to fetch the view's buffer every time.
  // Because the buffer CHANGES!
  Buffer_ID buffer = view_get_buffer(app0, view, Access_Always);
  b32 is_game_buffer = buffer_viewport_id(app0, buffer) != 0;
  
  App_Cmd app_value = app_cmd_event(app0, &input.event);
  App_Cmd *app = &app_value;
  Game_API *game = get_game_code(Game_On);
  
  Temp_Memory_Block temp(tmp);
  
  if(input.event.kind == InputEventKind_KeyStroke)
  {
   seconds_since_last_keystroke = 0;
  }
  
#if VIM_USE_BOTTOM_LISTER
  // Clicking on lister items outside of original view panel is a hack
  if((vim_lister_view_id != 0) and
     (view != vim_lister_view_id))
  {
   view_set_active(app, vim_lister_view_id);
   leave_current_input_unhandled(app);
   continue;
  }
#endif
  
  ProfileBlockNamed("before view input", before_view_input_block);
  
  // NOTE(allen): Mouse Suppression
  Event_Property event_properties = get_event_properties(&input.event);
  b32 is_mouse_event = event_properties & EventPropertyGroup_AnyMouseEvent;
  b32 suppressed = is_mouse_event && suppressing_mouse;
  
  if(not suppressed)
  {
   if(not is_mouse_event and (input.event.kind != InputEventKind_None))
   {
    vim_keystroke_text.size = 0;
    vim_cursor_blink = 0;
   }
   
   b32 handled = 0;
   
   if(game)
   {
    if(not is_game_buffer and (vim_state.mode != VIM_Normal))
    {
     handled = 0;
    }
    else
    {
     handled = game->is_event_handled_by_game(ed_game_state_pointer, app, &input.event, is_game_buffer);
    }
    
    if(handled)
    {
     register_keyboard_event_to_the_game(input.event);
    }
   }
   
   if(not handled)
   {// NOTE: The normal text editor
    handled = kv_handle_vim_keyboard_input(app, &input.event);
   }
   
   if(not handled)
   {
    // NOTE(allen): Get binding
    if(implicit_map_function == 0)
    {
     implicit_map_function = default_implicit_map;
    }
    
    Implicit_Map_Result map_result = implicit_map_function(app, 0, 0, &input.event);
    if(map_result.command)
    {
     // NOTE(allen): Run the command and pre/post command stuff
     default_pre_command(app, scope);
     ProfileBlockEnd(&before_view_input_block);
     
     {
      ProfileBlock("map_result_command_profile");
      map_result.command(app);
     }
     
     ProfileBlock("after view input");
     default_post_command(app, scope);
     handled = 1;
    }
   }
   
   if(not handled)
   {
    leave_current_input_unhandled(app);
   }
  }
 }
}

function void 
kv_newline_and_indent(App_Cmd *app)
{
 GET_VIEW_AND_BUFFER;
 HISTORY_GROUP_SCOPE;
 write_text(app, str8lit("\n"), true);
 kv_quail_keystroke_buffer.count = 0;  // #Hack
 
 i64 curpos = view_get_cursor_pos(app, view);
 u8 character = buffer_get_char(app, buffer, curpos);
 if (character == '}')
 {// NOTE: Handling for brace
  write_text(app, str8lit("\n"), true);
  auto_indent_line_at_cursor(app);
  move_vertical_lines(app, -1);
 }
 
 auto_indent_line_at_cursor(app);
}
//-