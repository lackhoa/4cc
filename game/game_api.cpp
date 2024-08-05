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
  View_ID view = get_active_view(app,0);
  u32 hot_prim = get_hot_prim_id();
  if( hot_prim && !(hot_prim & bit_31) )
  {// NOTE: Jump to primitive
   if( !is_view_to_the_right(app, view) )
   {//NOTE: switch to the right view
    view = get_other_primary_view(app, view, Access_Always, true);
   }
   view_set_buffer_named(app, view, GAME_FILE_NAME);
   view_set_cursor(app, view, seek_line_col(hot_prim, 0));
   result = true;
  }
 }
 
 return result;
}

//~