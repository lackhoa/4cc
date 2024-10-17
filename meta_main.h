#pragma once

global char *command_name;
global i1 meta_logging_level = 0;
global Arena meta_permanent_arena = make_arena_malloc();
struct Meta_Directories{
 String code;
 String game;
 String game_gen;
};
global Meta_Directories meta_dirs;  //@meta_dirs_init
struct Meta_Parsed_File{
 String name;
 String data;
 Token_List token_list;
};
#define meta_logf(...) if(meta_logging_level){ printf(__VA_ARGS__); }
//-
