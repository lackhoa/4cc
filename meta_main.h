#pragma once

global char *meta_command_name;
global i1 meta_logging_level = 0;
global Arena meta_permanent_arena = make_arena();
struct Meta_Directories{
 Stringz code;
 Stringz game;
 Stringz game_gen;
};
global Meta_Directories meta_dirs;  //@meta_dirs_init
struct Lexed_File{
 String name;
 String data;
 Token_List token_list;
 b32 ok;
};
#define meta_logf(...) if(meta_logging_level){ printf(__VA_ARGS__); }
function void meta_process_ast(Statement_Root *root);
//-
