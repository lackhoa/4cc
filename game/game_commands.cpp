function void
game_set_preset(Game_State *state, i32 viewport_id, i32 preset)
{
 if(viewport_id <= 0){ viewport_id = 1; }
 i32 viewport_index = viewport_id - 1;
 Viewport *viewport = &state->viewports[viewport_index];
 if(preset < 0 or preset >= state->model.recordings.preset_count){ return; }  // NOTE(kv) digit key past the list
 viewport->last_preset = viewport->preset;
 viewport->preset      = preset;
}
//-NOTE(kv) Preset list editing (plan-settings-ui). All viewports index the same
// list, so every reorder/delete fixes up preset/last_preset on all of them.
function void
preset_index_moved(Game_State *state, i32 from, i32 to)
{
 for_i32(viewport_index, 0, GAME_VIEWPORT_COUNT)
 {
  Viewport &viewport = state->viewports[viewport_index];
  if(viewport.preset      == from){ viewport.preset      = to; }
  else if(viewport.preset == to)  { viewport.preset      = from; }
  if(viewport.last_preset      == from){ viewport.last_preset = to; }
  else if(viewport.last_preset == to)  { viewport.last_preset = from; }
 }
}
function b32
preset_add(Game_State *state, i32 copy_from)
{// NOTE(kv) New preset = copy of `copy_from`, appended; becomes the active one.
 Model_Recordings &rec = state->model.recordings;
 if(rec.preset_count >= PRESET_CAP){ return false; }
 i32 index = rec.preset_count++;
 rec.preset_settings[index] = rec.preset_settings[copy_from];
 Preset_Settings &row = rec.preset_settings[index];
 snprintf(row.name, sizeof(row.name), "%.*s copy", (int)(sizeof(row.name)-6), rec.preset_settings[copy_from].name);
 game_set_preset(state, 1, index);
 return true;
}
function b32
preset_delete(Game_State *state, i32 index)
{
 Model_Recordings &rec = state->model.recordings;
 if(rec.preset_count <= 1 or index < 0 or index >= rec.preset_count){ return false; }
 for_i32(i, index, rec.preset_count-1){ rec.preset_settings[i] = rec.preset_settings[i+1]; }
 rec.preset_count--;
 for_i32(viewport_index, 0, GAME_VIEWPORT_COUNT)
 {
  Viewport &viewport = state->viewports[viewport_index];
  if(viewport.preset      > index){ viewport.preset--; }
  if(viewport.last_preset > index){ viewport.last_preset--; }
  viewport.preset      = clamp_between(0, viewport.preset,      rec.preset_count-1);
  viewport.last_preset = clamp_between(0, viewport.last_preset, rec.preset_count-1);
 }
 return true;
}
function b32
preset_swap(Game_State *state, i32 a, i32 b)
{// NOTE(kv) Move up/down = swap with the neighbor.
 Model_Recordings &rec = state->model.recordings;
 if(a < 0 or b < 0 or a >= rec.preset_count or b >= rec.preset_count or a == b){ return false; }
 macro_swap(rec.preset_settings[a], rec.preset_settings[b]);
 preset_index_moved(state, a, b);
 return true;
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
                         Input_Event *event, b32 is_game_buffer)
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
  {// IMPORTANT(kv) @Brittle Be careful not to let the editor "handle" keys
   // in the game buffer (like entering a Vim mode).
   // It's gonna crash, which is dumb but... We're only hacking here!
   handled = not (MATCH(Tab) or MATCH(Semicolon) or
                  MATCH_MOD(Alt, Q) or
                  MATCH_MOD(Ctl, Tab) or MATCH_MOD(Ctl, W) or
                  MATCH_MOD(Alt, Comma) or MATCH_MOD(Alt, Period) or
                  0);
  }
  else
  {
   // NOTE(kv) If this function is called, then the game is already on.
   // so we allow intercepting the return key.
   
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