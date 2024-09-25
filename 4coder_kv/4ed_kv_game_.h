#pragma once

global Game_State *ed_game_state_pointer;
global b32 game_on_ro;
global arrayof<String> received_game_commands;
global Game_API game_code_ro;

inline Game_API *
get_game_code()
{// NOTE: whether we wanna allow the game to receive call when it is off
 Game_API *result = 0;
 if (game_code_ro.is_valid && game_on_ro) {
  result = &game_code_ro;
 }
 return result;
}

//~EOF