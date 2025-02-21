function void
game_set_preset(Game_State *state, i32 viewport_id, i32 preset)
{
 if(viewport_id <= 0){ viewport_id = 1; }
 i32 viewport_index = viewport_id - 1;
 Viewport *viewport = &state->viewports[viewport_index];
 viewport->last_preset = viewport->preset;
 viewport->preset      = preset;
}
function void
game_last_preset(Game_State *state, i32 viewport_id)
{
 if(viewport_id <= 0){ viewport_id = 1; }
 i32 viewport_index = viewport_id - 1;
 Viewport *viewport = &state->viewports[viewport_index];
 macro_swap(viewport->preset, viewport->last_preset);
}
function b32
is_event_handled_by_game(Game_State *state, App *app,
                         Input_Event *event, b32 is_game_buffer, b32 game_rendered)
{// NOTE see @kv_view_input_handler
 b32 handled = false;
 if(event->kind == InputEventKind_KeyStroke)
 {//-Keyboard events
  Key_Mods mods = pack_modifiers(event->key.modifiers.mods,
                                 event->key.modifiers.count);
  Key_Code code = event->key.code;
#define MATCH(CODE) (mods == 0 && code == Key_Code_##CODE)
#define MATCH_MOD(MOD, CODE)  \
( (mods == Key_Mod_##MOD) && (code == Key_Code_##CODE) )
  
  if(fui_is_active())
  {
   handled = 1;
  }
  else if(is_game_buffer)
  {// IMPORTANT(kv) @Brittle Be very careful to not let the editor handle keys
   // in the game buffer (like entering a Vim mode).
   // It's gonna crash, which is dumb but... We're only hacking here!
   handled = not (MATCH(Tab) or MATCH(Semicolon) or
                  MATCH_MOD(Alt, Q) or
                  MATCH_MOD(Ctl, Tab) or MATCH_MOD(Ctl, W) or
                  MATCH_MOD(Alt, Comma) or MATCH_MOD(Alt, Period) or
                  0);
  }
  else if(game_rendered)
  {// NOTE Not a game buffer, but rendering, so there is exception.
   // NOTE(kv) @Brittle Events that are handled by lister, etc. will not flow through
   // this function, so no need to worry about messing with a lister, etc.
   handled = MATCH(Return);
  }
  
#undef MATCH_MOD
#undef MATCH
 }
 else if(event->kind == InputEventKind_MouseButton)
 {// NOTE(kv) We're supposed to detect if the mouse is within the game view,
  // and handle if we're inside, but I'm too lazy :>
  if(state->transient->hot_locations.count > 0)
  {
   handled = 1;
  }
 }
 
 return handled;
}
function b32
game_handle_tab_normal_mode(App_Cmd *app)
{
 b32 handled = false;
 View_ID   view   = get_active_view(app, Access_ReadVisible);
 Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
 
 Ed_Parser parserv = make_ed_parser_at_cursor(app);
 Ed_Parser *parser = &parserv;
 if(ep_maybe_id(parser, strlit("rebase")))
 {
  handled = true;
 }
 
 return handled;
}
//~