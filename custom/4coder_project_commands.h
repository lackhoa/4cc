/*
4coder_project_commands.h - type header paired with 4coder_project_commands.cpp
*/

// TOP

#if !defined(FCODER_PROJECT_COMMANDS_H)
#define FCODER_PROJECT_COMMANDS_H

////////////////////////////////
// NOTE(allen): Match Pattern Types

struct Prj_Pattern{
 String8List absolutes;
};

struct Prj_Pattern_Node{
 Prj_Pattern_Node *next;
 Prj_Pattern pattern;
};

struct Prj_Pattern_List{
 Prj_Pattern_Node *first;
 Prj_Pattern_Node *last;
 i1 count;
};

typedef u32 Prj_Open_File_Flags;
enum{
 PrjOpenFileFlag_Recursive = 1,
};

///////////////////////////////
// NOTE(allen): Project Files

struct Prj_Setup_Status{
    b32 bat_exists;
    b32 sh_exists;
    b32 project_exists;
    b32 everything_exists;
};

struct Prj_Key_Strings{
    b32 success;
    String8 script_file;
    String8 code_file;
    String8 output_dir;
    String8 binary_file;
};

typedef u32 Prj_Setup_Script_Flags;
enum{
 PrjSetupScriptFlag_Project = 0x1,
 PrjSetupScriptFlag_Bat     = 0x2,
 PrjSetupScriptFlag_Sh      = 0x4,
};

struct Prj_Open_File_Config
{
 Prj_Pattern_List whitelist;
 Prj_Pattern_List blacklist;
 arrayof<String>  limited_edit_list;
 Prj_Open_File_Flags flags;
};

////////////////////////////////
// NOTE(allen): File Pattern Operators

function Prj_Pattern_List prj_pattern_list_from_extension_array(Arena *arena, String8Array list);
function Prj_Pattern_List prj_pattern_list_from_var(Arena *arena, Variable_Handle var);
function Prj_Pattern_List prj_get_standard_blacklist(Arena *arena);

function b32  prj_match_in_pattern_list(String8 string, Prj_Pattern_List list);

function void prj_close_files_with_ext(App *app, String8Array extension_array);
function void prj_open_files_pattern_filter(App *app, String8 dir, Prj_Pattern_List whitelist, Prj_Pattern_List blacklist, Prj_Pattern_List limited_edit_list, Prj_Open_File_Flags flags);
function void prj_open_all_files_with_ext_in_hot(App *app, String8Array array, Prj_Open_File_Flags flags);

////////////////////////////////
// NOTE(allen): Project Files

function void prj_stringize_project(App *app, Arena *arena, Variable_Handle project, String8List *out);

////////////////////////////////
// NOTE(allen): Project Operations

function void            prj_exec_command(App *app, Variable_Handle cmd_var);
function Variable_Handle prj_command_from_name(App *app, String8 cmd_name);
function void            prj_exec_command_name(App *app, String8 cmd_name);
function void            prj_exec_command_fkey_index(App *app, i1 fkey_index);

function String8         prj_full_file_path_from_project(Arena *arena, Variable_Handle project);
function String8         prj_path_from_project(Arena *arena, Variable_Handle project);
function Variable_Handle prj_cmd_from_user(App *app, Variable_Handle prj_var, String8 query);

#endif

// BOTTOM

