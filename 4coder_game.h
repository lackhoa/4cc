#pragma once

global Game_State *ed_game_state_pointer;

enum Game_Status
{
 Game_Off,
 Game_On,
 Game_Rendering,
};
global Game_Status game_status;

global sarray(String) received_game_commands;
global Game_API game_code_ro;

myinline b32 is_game_on() { return game_status >= Game_On; }
myinline b32 is_game_rendering() { return game_status >= Game_Rendering; }

function Game_API *
get_game_code(Game_Status min_status)
{// NOTE: whether we wanna allow the game to receive call when it is off
 Game_API *result = 0;
 if(min_status != Game_Off)
 {
  b32 status_ok = game_status >= min_status;
  if(game_code_ro.is_valid and status_ok)
  {
   result = &game_code_ro;
  }
 }
 return result;
}

function void
maybe_update_game(App *app, Frame_Info frame);

function void
render_game(App *app, Render_Target *target, i32 viewport, Frame_Info frame, rect2 clip_box);

//~EOF