/*
4coder_default_framework_variables.cpp - Declares the global variables used by the framework for
the default 4coder behavior.
*/

// TOP

global Managed_ID view_rewrite_loc; //attachment
global Managed_ID view_next_rewrite_loc; //attachment
global Managed_ID view_paste_index_loc; //attachment
global Managed_ID view_is_passive_loc; //attachment
global Managed_ID view_snap_mark_to_cursor; //attachment
global Managed_ID view_ui_data; //attachment
global Managed_ID view_highlight_range; //attachment
global Managed_ID view_highlight_buffer; //attachment
global Managed_ID view_render_hook; //attachment
global Managed_ID view_word_complete_menu; //attachment
global Managed_ID view_lister_loc; //attachment
global Managed_ID view_previous_buffer; //attachment

global Managed_ID buffer_map_id; //attachment
global Managed_ID buffer_eol_setting; //attachment
global Managed_ID buffer_lex_task; //attachment

global Managed_ID sticky_jump_positions_handle; //attachment
global Managed_ID attachment_tokens; //attachment

////////////////////////////////

#if 0
global Managed_ID mapid_global; //command_map
global Managed_ID mapid_file; //command_map
global Managed_ID mapid_code; //command_map
#endif

////////////////////////////////

global b32 allow_immediate_close_without_checking_for_changes = false;

global char *default_extensions[] = {
    "cpp",
    "hpp",
    "c",
    "h",
    "cc",
    "cs",
    "java",
    "rs",
    "glsl",
    "m",
};

#if !defined(AUTO_CENTER_AFTER_JUMPS)
#define AUTO_CENTER_AFTER_JUMPS true
#endif
global b32 auto_center_after_jumps = AUTO_CENTER_AFTER_JUMPS;
global u8 locked_buffer_space[256];
global String locked_buffer = {};

global u8 out_buffer_space[1024];
global u8 command_space[1024];
global char hot_directory_space[1024];

global b32 suppressing_mouse = false;

global b32 show_fps_hud = false;

// TODO(allen): REMOVE THIS!
global Heap global_heap;

enum{
    FCoderMode_Original = 0,
    FCoderMode_NotepadLike = 1,
};
global i1 fcoder_mode = FCoderMode_Original;

global ID_Pos_Jump_Location prev_location = {};

global View_ID global_bottom_view;
global b32     global_bottom_view_expanded;

global Arena global_permanent_arena = {};
global Arena global_frame_arena     = {};


global Arena global_config_arena = {};

global char previous_isearch_query[256] = {};

global Mapping framework_mapping = {};

global Buffer_Modified_Set global_buffer_modified_set = {};

////////////////////////////////

global b32 global_keyboard_macro_is_recording = false;
global Range_i64 global_keyboard_macro_range = {};

////////////////////////////////

global Fade_Range_List buffer_fade_ranges = {};
global Arena fade_range_arena = {};
global Fade_Range *free_fade_ranges = 0;

////////////////////////////////

global Point_Stack point_stack = {};

////////////////////////////////

global Clipboard global_clipboard0 = {};

// BOTTOM

