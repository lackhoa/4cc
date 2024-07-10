internal get_indicator_level_return
get_indicator_level(get_indicator_level_params)
{
 return state->indicator_level;
}

internal set_indicator_level_return
set_indicator_level(set_indicator_level_params)
{
 state->indicator_level = level;
}

internal game_set_preset_return
game_set_preset(game_set_preset_params)
{
 if (viewport_id <= 0) { viewport_id = 1; }
 i1 viewport_index = viewport_id - 1;
 Viewport *viewport = &state->viewports[viewport_index];
 viewport->last_preset = viewport->preset;
 viewport->preset      = preset;
}

internal game_last_preset_return
game_last_preset(game_last_preset_params)
{
 if (viewport_id <= 0) { viewport_id = 1; }
 i1 viewport_index = viewport_id - 1;
 Viewport *viewport = &state->viewports[viewport_index];
 macro_swap(viewport->preset, viewport->last_preset);
}

internal b32 
is_key_handled_by_game(is_key_handled_by_game_params)
{
 if (event->kind == InputEventKind_KeyStroke)
 {
  Key_Mods mods = pack_modifiers(event->key.modifiers.mods, event->key.modifiers.count);
  Key_Code code = event->key.code;
  //
#define MATCH(CODE) (mods == 0 && code == Key_Code_##CODE)
#define MATCH_MOD(MOD, CODE)  \
( (mods == Key_Mod_##MOD) && (code == Key_Code_##CODE) )
  //
  if (MATCH_MOD(Alt, Q) ||
      MATCH_MOD(Ctl, Tab)   || MATCH_MOD(Ctl, W) ||
      MATCH_MOD(Alt, Comma) || MATCH_MOD(Alt, Period) ||
      MATCH(Tab) ||
      MATCH(Semicolon) ||
      (mods == (Key_Mod_Ctl|Key_Mod_Sft) && code == Key_Code_Tab) ||
      (mods == 0 && (Key_Code_0 <= code) && (code <= Key_Code_9)) ||
      false)
  {
   return false;
  }
  else { return true; }
#undef MATCH_MOD
#undef MATCH
 }
 else { return false; }
}
