#pragma once

global Game_State *ed_game_state_pointer;
global b32 global_game_on_readonly;
global arrayof<String> received_game_commands;
global Game_API global_game_code_;

inline Game_API *
get_game_code()
{// NOTE: whether we wanna allow the game to receive call when it is off
 Game_API *result = 0;
 if (global_game_code_.is_valid && global_game_on_readonly) {
  result = &global_game_code_;
 }
 return result;
}

//~eof