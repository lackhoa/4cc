function game_set_preset_return
game_set_preset(game_set_preset_params){
 if (viewport_id <= 0) { viewport_id = 1; }
 i1 viewport_index = viewport_id - 1;
 Viewport *viewport = &state->viewports[viewport_index];
 viewport->last_preset = viewport->preset;
 viewport->preset      = preset;
}

function game_last_preset_return
game_last_preset(game_last_preset_params){
 if (viewport_id <= 0) { viewport_id = 1; }
 i1 viewport_index = viewport_id - 1;
 Viewport *viewport = &state->viewports[viewport_index];
 macro_swap(viewport->preset, viewport->last_preset);
}

function is_event_handled_by_game_return
is_event_handled_by_game(is_event_handled_by_game_params)
{
 b32 result = false;
 if (event->kind == InputEventKind_KeyStroke)
 {
  Key_Mods mods = pack_modifiers(event->key.modifiers.mods,
                                 event->key.modifiers.count);
  Key_Code code = event->key.code;
#define MATCH(CODE) (mods == 0 && code == Key_Code_##CODE)
#define MATCH_MOD(MOD, CODE)  \
( (mods == Key_Mod_##MOD) && (code == Key_Code_##CODE) )
  if (game_hot)
  {
   result = !(MATCH(Tab) ||
              MATCH(Semicolon) ||
              MATCH_MOD(Alt, Q) ||
              MATCH_MOD(Ctl, Tab)   || MATCH_MOD(Ctl, W) ||
              MATCH_MOD(Alt, Comma) || MATCH_MOD(Alt, Period) ||
              //(mods == (Key_Mod_Ctl|Key_Mod_Sft) && code == Key_Code_Tab) ||
              (mods == 0 && (Key_Code_0 <= code) && (code <= Key_Code_9)) ||
              false);
  }
  else
  {// NOTE: control when game is NOT hot (i.e game view not active)
   result = (MATCH(Space) ||
             MATCH(Q));
  }
#undef MATCH_MOD
#undef MATCH
 }
 else if (event->kind == InputEventKind_MouseButton)
 {
  //NOTE: We're supposed to detect if the mouse is within the game view,
  // and handle if we're inside, but I'm too lazy ;>
  u32 hot_prim = get_hot_prim_id();
  if( hot_prim ) {
   result = true;
  }
 }
 
 return result;
}

//~