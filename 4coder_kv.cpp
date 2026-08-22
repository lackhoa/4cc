// ~/4ed/code/4coder_kv/config.4coder
// ~/4ed/code/4coder_kv/4coder_kv_commands.cpp

#define ED_PARSER_BUFFER 1
#include "4ed_kv_parser.cpp"
#include "4coder_custom_include.cpp"

#include "4coder_kv_debug.cpp"
#include "4coder_fleury/4coder_fleury_kv.cpp"

global darray(char) kv_quail_keystroke_buffer;  // TODO(kv) Why is this thing even dynamic?

#include "4coder_vim/4coder_vim_include.h"

#include "4coder_game.cpp"
#include "4coder_kv_input.cpp"
#include "4coder_kv_commands.cpp"
#include "4coder_kv_hooks.cpp"
#include "4coder_kv_draw.cpp"
#include "4coder_kv_vim_stuff.cpp"
#include "4coder_kv_lang_list.h"

// NOTE: Custom layer swapping for testing and trying out.
// NOTE: Please enable only one layer, or else it explodes!
#if KV_INTERNAL
#    define USE_LAYER_kv               1
#    define USE_LAYER_fleury_lite      0
#    define USE_LAYER_fleury           0
#    define USE_LAYER_default          0
#else
#    define USE_LAYER_kv               1
#endif

#if    USE_LAYER_fleury
#    include "4coder_fleury/4coder_fleury.cpp"  // because including this will bring in weird commands I don't need / can't use
#elif  USE_LAYER_fleury_lite
#    include "4coder_kv_fleury_lite.cpp"
#endif

#include "init_custom_id.gen.cpp"

function void
kvInitShiftedTable(){
 Base_Allocator *base = get_default_allocator();
 shifted_version_of_characters = make_table_u64_u64(base, 32);
 
#define INSERT(CHAR1, CHAR2) table_insert(&shifted_version_of_characters, CHAR1, CHAR2)
 {
  INSERT('a', 'A');
  INSERT('1', '!');
  INSERT('2', '@');
  INSERT('3', '#');
  INSERT('4', '$');
  INSERT('5', '%');
  INSERT('6', '^');
  INSERT('7', '&');
  INSERT('8', '*');
  INSERT('`', '~');
  INSERT('-', '_');
  INSERT(',', '<');
  INSERT('.', '>');
  INSERT(';', ':');
  INSERT('=', '+');
  INSERT('/', '?');
  INSERT('\\', '|');
 }
#undef INSERT
}

// NOTE(kv): shared between custom layers
function void
kv_essential_mapping(Mapping *mapping) {
 String_ID global_id = vars_intern_lit("keys_global");
 String_ID file_id   = vars_intern_lit("keys_file");
 String_ID code_id   = vars_intern_lit("keys_code");
 
 MappingScope();
 SelectMapping(mapping);
 
 SelectMap(global_id);
 BindCore(vim_try_exit, CoreCode_TryExit);
 BindCore(clipboard_record_clip, CoreCode_NewClipboardContents);
 BindMouseWheel(mouse_wheel_scroll);
 BindMouseWheel(mouse_wheel_change_face_size, Key_Code_Control);
 // BindCore(vim_file_externally_modified, CoreCode_FileExternallyModified);  NOTE(kv): cool idea but there are auto-generated files that we don't care about
 
 SelectMap(file_id);
 ParentMap(global_id);
 BindTextInput(write_text_input);
 BindMouse       (kv_handle_left_click, MouseCode_Left);
 //BindMouseRelease(click_set_cursor,          MouseCode_Left);
 BindCore        (click_set_cursor_and_mark, CoreCode_ClickActivateView);
 BindMouseMove   (click_set_cursor_if_lbutton);
 
 SelectMap(code_id);
 ParentMap(file_id);
}

// NOTE(kv): shared between custom layers
function void 
kv_default_bindings(Mapping *mapping) {
 String_ID global_id = vars_intern_lit("keys_global");
 String_ID file_id   = vars_intern_lit("keys_file");
 String_ID code_id   = vars_intern_lit("keys_code");
 
 MappingScope();
 SelectMapping(mapping);
 
 SelectMap(global_id);
 
 Bind(toggle_fullscreen,   Key_Code_F11);
 Bind(increase_face_size,  Key_Code_Equal, Key_Code_Control);
 Bind(decrease_face_size,  Key_Code_Minus, Key_Code_Control);
 Bind(byp_reset_face_size, Key_Code_0, Key_Code_Control);
 Bind(exit_4coder,         Key_Code_Q, Key_Code_Control);
 Bind(exit_4coder,         Key_Code_Q, Key_Code_Command);
 
 SelectMap(file_id);
 ParentMap(global_id);
 
 Bind(delete_char,        Key_Code_Delete);
 Bind(backspace_char,     Key_Code_Backspace);
 Bind(move_up,            Key_Code_Up);
 Bind(move_down,          Key_Code_Down);
 Bind(move_left,          Key_Code_Left);
 Bind(move_right,         Key_Code_Right);
 Bind(seek_end_of_line,   Key_Code_End);
 Bind(right_adjust_view,  Key_Code_Home);
 
 SelectMap(code_id);
 ParentMap(file_id);
}

function Buffer_ID 
create_special_buffer(App *app, String name, Buffer_Create_Flag create_flags, Buffer_Setting_ID buffer_flags)
{
 Buffer_ID buffer = create_buffer(app, name, create_flags);
 buffer_set_setting(app, buffer, buffer_flags, true);
 return buffer;
}

function void
kv_4coder_initialize(App *app)
{
 Face_Description description = get_face_description(app, 0);
 i32 override_font_size = description.parameters.pt_size;
 b32 override_hinting = description.parameters.hinting;
 load_config_and_apply(app, &global_config_arena, override_font_size, override_hinting);
}

function void
startup_panels_and_files(App_Cmd *app)
{
 Scratch_Block scratch(app);
 
#define FILE_VAR "startup_file"
 String file1 = def_get_config_string(scratch, FILE_VAR "1");
 String file2 = def_get_config_string(scratch, FILE_VAR "2");
 //String file3 = def_get_config_string(scratch, FILE_VAR "3");
 String file4 = def_get_config_string(scratch, FILE_VAR "4");
#undef FILE_VAR
 
 b32 dev_single_panel = false;
#if KV_INTERNAL
 if ( def_get_config_b32(vars_intern_lit("dev_single_panel")) )
 {
  dev_single_panel = true;
 }
#endif
 // NOTE(kv) Agent mode (-debug-cmd, see game_debug_channel.cpp): one big game
 // viewport, nothing else to look at.
 if(strstr(GetCommandLineA(), "-debug-cmd")){ dev_single_panel = true; }
 
 if ( dev_single_panel )
 {
  View_ID view4 = get_active_view(app, Access_Always);
  Buffer_ID buffer4 = create_buffer(app, file4, 0);
  view_set_buffer(app, view4, buffer4, 0);
 }
 else
 {
  View_ID view1 = get_active_view(app, Access_Always);
  {// NOTE: 1
   Buffer_ID buffer1 = create_buffer(app, file1, 0);
   view_set_buffer(app, view1, buffer1, 0);
  }
  
  // NOTE(kv): Bottom view
  Buffer_Identifier comp = buffer_identifier(compilation_buffer_name);
  Buffer_ID comp_id = buffer_identifier_to_id(app, comp);
  View_ID bottom_view = 0;
  {
   bottom_view = open_view(app, view1, ViewSplit_Bottom);
   new_view_settings(app, bottom_view);
   //Buffer_ID buffer = view_get_buffer(app, bottom_view, Access_Always);
   //Face_ID face_id = get_face_id(app, buffer);
   //Face_Metrics metrics = get_face_metrics(app, face_id);
   //view_set_split_pixel_size(app, bottom_view, (i32)(metrics.line_height*4.f));
   view_set_passive(app, bottom_view, true);
   global_bottom_view = bottom_view;
   view_set_buffer(app, bottom_view, comp_id, 0);
   collapse_bottom_view(app);
  }
  
  {// NOTE: 2
   view_set_active(app, view1);
   open_panel_vsplit(app);
   View_ID view2     = get_active_view(app, Access_Always);
   Buffer_ID buffer2 = create_buffer(app, file2, 0);
   view_set_buffer(app, view2, buffer2, 0);
  }
 }
}

function void
initialize_stylist_fonts(App *app)
{
 Scratch_Block scratch(app);
 String bin_path = system_get_path(scratch, SystemPath_BinaryDirectory);
 
 // NOTE(rjf): Fallback font.
 Face_ID face_that_should_totally_be_there = get_face_id(app, 0);
 
 // NOTE(rjf): Title font.
 {
  Face_Description desc = {};
  {
   desc.font.filename =  push_stringf(scratch, "%.*sfonts/RobotoCondensed-Regular.ttf", string_expand(bin_path));
   desc.parameters.pt_size = 18;
   desc.parameters.bold = 0;
   desc.parameters.italic = 0;
   desc.parameters.hinting = 0;
  }
  
  if(IsFileReadable(desc.font.filename))
  {
   global_styled_title_face = try_create_new_face(app, &desc);
  }
  else
  {
   global_styled_title_face = face_that_should_totally_be_there;
  }
 }
 
 // NOTE(rjf): Label font.
 {
  Face_Description desc = {};
  {
   desc.font.filename =  push_stringf(scratch, "%.*sfonts/RobotoCondensed-Regular.ttf", string_expand(bin_path));
   desc.parameters.pt_size = 10;
   desc.parameters.bold = 1;
   desc.parameters.italic = 1;
   desc.parameters.hinting = 0;
  }
  
  if(IsFileReadable(desc.font.filename))
  {
   global_styled_label_face = try_create_new_face(app, &desc);
  }
  else
  {
   global_styled_label_face = face_that_should_totally_be_there;
  }
 }
 
 // NOTE(rjf): Small code font.
 {
  Face_Description normal_code_desc = get_face_description(app, get_face_id(app, 0));
  
  Face_Description desc = {};
  {
   desc.font.filename =  push_stringf(scratch, "%.*sfonts/Inconsolata-Regular.ttf", string_expand(bin_path));
   desc.parameters.pt_size = normal_code_desc.parameters.pt_size - 1;
   desc.parameters.bold = 1;
   desc.parameters.italic = 1;
   desc.parameters.hinting = 0;
  }
  
  if(IsFileReadable(desc.font.filename))
  {
   global_small_code_face = try_create_new_face(app, &desc);
  }
  else
  {
   global_small_code_face = face_that_should_totally_be_there;
  }
 }
}

function void
kv_startup(App_Cmd *app)
{
 ProfileBlock( "kv_startup");
 Scratch_Block temp(app);
 
 set_window_title(app, str8lit("4coder kv"));
 load_themes_default_folder(app);
 kv_4coder_initialize(app);
 
 String startup_hot_directory = def_get_config_string(temp, vars_intern_lit("startup_hot_directory"));
 set_hot_directory(app, startup_hot_directory);
 
#if !KV_INTERNAL
 load_project(app);
#endif
 
 //def_audio_init();
 
 clear_all_layouts(app);
 
 kv_essential_mapping(&framework_mapping);
 kv_default_bindings(&framework_mapping);
 
 {// NOTE(kv): Create special buffers.
  Buffer_Create_Flag create_flags = BufferCreate_NeverAttachToFile|BufferCreate_AlwaysNew;
  create_special_buffer(app, compilation_buffer_name, create_flags, (Buffer_Setting_ID)(BufferSetting_Unimportant|BufferSetting_ReadOnly));
  String GAME_BUFFER_NAMES[GAME_BUFFER_COUNT] = {
   strlit("*game*"),
   strlit("*game2*"),
   strlit("*game3*"),
  };
  Models *models = app_get_models(app);
  for_i32(index,0,GAME_BUFFER_COUNT)
  {
   Buffer_Setting_ID setting = (BufferSetting_Unimportant |
                                BufferSetting_ReadOnly    |
                                BufferSetting_IsGame);
   Buffer_ID buffer = create_special_buffer(app, GAME_BUFFER_NAMES[index], create_flags, setting);
   models->game_buffers[index] = buffer;
   Editing_File *file = imp_get_file(models, buffer);
   file->game_viewport_id = index+1;
   breakhere;
  }
 }
 
 startup_panels_and_files(app);
 
 initialize_stylist_fonts(app);
 
 if(!def_get_config_b32(vars_intern_lit("dev_disable_game_on_startup"))){
  turn_game_on();
  switch_to_game_panel(app);  // NOTE(kv) show the game right away (skip the g,o dance)
 }
}

// ;binding
function void 
kv_vim_bindings(App *app)
{
 u32 N = bit_1;
 u32 I = bit_2;
 u32 V = bit_3;
 
 // todo(kv): tons of ifs :<
#define BIND(...) if (!VimBind(__VA_ARGS__)) { printf("Keymap conflict at line %d!!!\n", __LINE__); }
 
 u32 C = Key_Mod_Ctl;
 u32 S = Key_Mod_Sft;
 u32 M = OS_MAC ? Key_Mod_Cmd : Key_Mod_Alt;
 Key_Code leader = Key_Code_BackwardSlash;
 
 BIND(0, kv_vim_normal_mode, Key_Code_Escape);
 BIND(0, kv_void_command,    Key_Code_Menu);  // NOTE(kv): On Macos, this key inserts some random crap and I still can't turn it off.
 
 //-NOTE: SUB_G
 BIND(N, cmd_open_log_buffer,     SUB_G,   Key_Code_M);
 BIND(N, kv_open_note_file,       SUB_G,   Key_Code_N);
 BIND(N, vim_switch_buffer_lister,SUB_G,   Key_Code_F);
 BIND(N, toggle_game_cmd,         SUB_G,   Key_Code_O);
 BIND(N, toggle_game_auxiliary_viewports, SUB_G, S|Key_Code_O);
 BIND(N|V, vim_file_top,          SUB_G,   Key_Code_G);
 BIND(N|V, kv_toggle_cpp_comment, SUB_G,   Key_Code_C);
 
 BIND(N, cmd_undo,                    Key_Code_U);
 BIND(N, cmd_redo,                  C|Key_Code_R);
 BIND(N, swap_panels,                       C|Key_Code_S);
 BIND(N, vim_next_4coder_jump,              M|Key_Code_N);
 BIND(N, vim_prev_4coder_jump,              M|Key_Code_P);
 BIND(N, view_buffer_other_panel,           C|Key_Code_D);
 
 /// Mode Binds
 BIND(N|V, vim_modal_i,                        Key_Code_I);
 BIND(N|0, goto_line,                       (S|Key_Code_Semicolon));
 // BIND(N,   vim_insert_begin,                (S|Key_Code_I));
 BIND(N,   vim_replace_mode,                (S|Key_Code_R));
 BIND(N|0, vim_visual_mode,                    Key_Code_V);
 BIND(0|V, kv_vim_visual_line_mode,            Key_Code_V);
 BIND(N|V, vim_visual_mode,                  S|Key_Code_V);
 BIND(N|V, vim_visual_mode,                  C|Key_Code_V);
 BIND(N|0, vim_newline_below,                  Key_Code_O);
 BIND(N|0, vim_newline_above,                S|Key_Code_O);
 BIND(N|0, kv_newline_above,                 C|Key_Code_K);
 BIND(N|0, kv_newline_below,                 C|Key_Code_J);
 
 /// Sub Mode Binds
 BIND(N|V, vim_submode_g,       Key_Code_G);
 BIND(N|V, vim_submode_z,       Key_Code_Z);
 BIND(N|V, vim_submode_leader,  leader);
 
 BIND(N,   vim_repeat_last_command,       Key_Code_Period);
 BIND(N|V, vim_request_yank,              Key_Code_Y);
 BIND(N|V, vim_request_delete,            Key_Code_D);
 BIND(N|0, kv_handle_c_normal,            Key_Code_C);
 BIND(0|V, vim_request_change,            Key_Code_C);
 BIND(N|V, vim_delete_end,              S|Key_Code_D);
 BIND(N|V, vim_change_end,              S|Key_Code_C);
 BIND(N|V, vim_yank_end,                S|Key_Code_Y);
 BIND(N|0, handle_tab_normal_mode,        Key_Code_Tab);
 BIND(0|V, auto_indent_range,             Key_Code_Tab);
 BIND(  V, vim_uppercase,                 Key_Code_Comma);
 BIND(N|V, vim_request_indent,          S|Key_Code_Period);
 BIND(N|V, vim_request_outdent,         S|Key_Code_Comma);
 BIND(N,   vim_replace_next_char,         Key_Code_R);
 BIND(V,   vim_replace_range_next,        Key_Code_R);
 //BIND(N,       delete_comma_separated_item, C|Key_Code_Comma);
 
 /// Edit Binds
 BIND(N|V,   vim_paste_before,         Key_Code_P);
 BIND(N,     vim_delete_char,          Key_Code_X);
 BIND(N,     kill_buffer,            S|Key_Code_X);
 BIND(N|V,   vim_combine_line,      (S|Key_Code_J));
 BIND(N,     vim_backspace_char,       Key_Code_Backspace);
 BIND(I,     word_complete,            Key_Code_Tab);
 BIND(I,     vim_paste_before,       M|Key_Code_V);
 BIND(I,     vim_paste_before,       C|Key_Code_V);
 BIND(I,     kv_newline_and_indent,    Key_Code_Return);
 BIND(N,     kv_newline_and_indent,  S|Key_Code_K); 
 
 /// Movement Binds
 BIND(N|V, vim_left,                Key_Code_H);
 BIND(N|V, vim_down,                Key_Code_J);
 BIND(N|V, vim_up,                  Key_Code_K);
 BIND(N|V, vim_right,               Key_Code_L);
 BIND(N|V, vim_end_line,         (S|Key_Code_4));
 BIND(N|V, vim_begin_line,        M|Key_Code_I);
 BIND(0|V, vim_begin_line,          Key_Code_I);
 BIND(N|V, vim_w_cmd,               Key_Code_W);
 BIND(N|V, vim_b_cmd,               Key_Code_B);
 BIND(N|V, vim_forward_WORD,     (S|Key_Code_W));
 BIND(N|V, vim_backward_WORD,    (S|Key_Code_B));
 BIND(N|V, vim_forward_end,         Key_Code_E);
 BIND(N|V, vim_forward_END,      (S|Key_Code_E));
 
 /* for(i32 code = cast(i32)Key_Code_0;
 code <= cast(i32)Key_Code_9;
 code++)
 {
 BIND(N, command_game_set_preset, cast(Key_Code)code);
 }*/
 
 BIND(N|V, vim_goto_line,                   (S|Key_Code_G));
 BIND(N|V, vim_goto_column,                 (S|Key_Code_BackwardSlash));
 BIND(N|V, vim_modal_percent,               (S|Key_Code_5));
 BIND(N|V, vim_bounce,                      (C|Key_Code_5));
 BIND(N|0, kv_jump_ultimate,                   Key_Code_F);
 BIND(N|0, kv_jump_ultimate_other_panel,     M|Key_Code_F);
 BIND(0|V, vim_set_seek_char,                  Key_Code_F);
 BIND(N|0, vim_half_page_up,                   Key_Code_LeftBracket);
 BIND(N|0, vim_half_page_down,                 Key_Code_RightBracket);
 BIND(V,   cursor_mark_swap,                   Key_Code_O);
 BIND(V,   vim_block_swap,                  (S|Key_Code_O));
 
 BIND(N, vim_search_identifier,              C|Key_Code_N);
 BIND(N, vim_clear_search,          SUB_Leader,Key_Code_Space);
 BIND(N, vim_start_search_forward,             Key_Code_ForwardSlash);
 BIND(N, vim_start_search_backward,         (S|Key_Code_ForwardSlash));
 BIND(N, vim_to_next_pattern,                  Key_Code_N);
 BIND(N, vim_to_prev_pattern,               (S|Key_Code_N));
 
 BIND(N, vim_prev_jump,                     (C|Key_Code_O));
 BIND(N, vim_next_jump,                     (C|Key_Code_I));
 
 // Screen Adjust Binds
 BIND(N|V, vim_half_page_up,                 (C|Key_Code_B));
 BIND(N|V, vim_half_page_down,               (C|Key_Code_F));
 BIND(N,   cmd_expand_snippet,                  (C|Key_Code_E));
 BIND(N|V, vim_scroll_screen_top,         SUB_Z,   Key_Code_T);
 BIND(N|V, vim_scroll_screen_mid,         SUB_Z,   Key_Code_Z);
 BIND(N|V, vim_scroll_screen_bot,         SUB_Z,   Key_Code_B);
 
 // Miscellaneous Binds
 BIND(N|V, vim_set_mark,                         Key_Code_M);
 BIND(N|0, vim_goto_mark,                        Key_Code_Quote);
 BIND(N|V, vim_toggle_macro,                   S|Key_Code_Q);
 BIND(N|V, keyboard_macro_replay,              S|Key_Code_2);
 BIND(N,   open_matching_file_cpp,     SUB_G,    Key_Code_H);
 BIND(N,   open_matching_file_cpp,               Key_Code_F12);
 BIND(N,   open_matching_file_cpp_other_panel, M|Key_Code_F12);
 BIND(N,       jump_between_meta_and_generated_code, Key_Code_4);
 BIND(N,      jump_between_meta_and_generated_code_other_panel, M|Key_Code_4)
 
 /// Panel
 BIND(N|V|I, change_active_primary_view,   C|Key_Code_Tab);
 BIND(N|V|I, change_active_monitor,      C|S|Key_Code_Tab);
 BIND(N,     close_panel,                  M|Key_Code_W);
 BIND(N,     toggle_split_panel,           C|Key_Code_W);
 
 // Sub modes
 BIND(N|V, vim_leader_d, SUB_Leader,       Key_Code_D);
 BIND(N|V, vim_leader_c, SUB_Leader,       Key_Code_C);
 BIND(N|V, vim_leader_D, SUB_Leader,  (S|Key_Code_D));
 BIND(N|V, vim_leader_C, SUB_Leader,  (S|Key_Code_C));
 
 // Language support
 BIND(N|0,  kv_list_all_locations,     S|Key_Code_S);
 
 // sexpr movement
 BIND(N|V,   kv_sexpr_up,     M|Key_Code_K);
 BIND(N|V,   kv_sexpr_down,   M|Key_Code_J);
 BIND(N|V,   kv_sexpr_right,  M|Key_Code_L);
 BIND(N|V,   kv_sexpr_left,   M|Key_Code_H);
 BIND(N|V,   kv_sexpr_end,    M|Key_Code_Semicolon);
 //BIND(N,     kv_sexpr_select_whole, Key_Code_Q);
 BIND(N|V,     cmd_handle_q,   Key_Code_Q);
 // surround paren
 BIND(V,   kv_surround_paren,                 Key_Code_0);
 BIND(V,   kv_surround_paren_spaced,          Key_Code_9);
 BIND(V,   cmd_closing_bracket_in_visual_mode,Key_Code_RightBracket);
 BIND(V,   kv_surround_brace,                 Key_Code_LeftBracket);
 BIND(V,   kv_surround_double_quote,          Key_Code_Quote);
 BIND(N|V,     kv_surround_brace_special,       M|Key_Code_LeftBracket)
 BIND(N,   kv_delete_surrounding_groupers,  M|Key_Code_RightBracket);
 
 //
 BIND(N,  handle_space_command,       Key_Code_Space);
 BIND(N,  vim_insert_end,             Key_Code_A);
 BIND(  V,  vim_end_line,               Key_Code_A);
 BIND(N  ,  vim_select_all,           C|Key_Code_A);
 BIND(N,  kv_shift_character,         Key_Code_Comma);
 BIND(N,  exit_4coder,              M|Key_Code_Q);
 BIND(N|V,  vim_command_mode,           Key_Code_Semicolon);
 BIND(N,  kv_reopen_with_confirmation, S|Key_Code_U);
 BIND(N,        quick_swap_buffer,           M|Key_Code_Comma);
 BIND(N|0,  kv_do_t,                    Key_Code_T);
 BIND(N|0,  kv_do_T,                  S|Key_Code_T);
 // NOTE(kv): debugger
#if 0
 BIND(N|0,  raddbg_add_breakpoint,      Key_Code_F9);
 BIND(N|0,  raddbg_stop_debugging,    S|Key_Code_F5);
 BIND(N|0,  raddbg_run_to_cursor,     C|Key_Code_F10);
#else
 BIND(N|0,  remedy_add_breakpoint,      Key_Code_F9);
 BIND(N|0,  remedy_stop_debugging,    S|Key_Code_F5);
 BIND(N|0,  remedy_run_to_cursor,     C|Key_Code_F10);
#endif
 // NOTE(kv): build
 BIND(N,  kv_build_normal,               M|Key_Code_M);
 BIND(N,  kv_build_run_only,           C|M|Key_Code_M);
 BIND(N,  kv_build_full_rebuild,       S|M|Key_Code_M);
 BIND(N|V,  replace_in_all_buffers,        Key_Code_F2);
 BIND(N|0,  open_build_script,             Key_Code_F3);
 BIND(N,        toggle_bottom_view_command,  M|Key_Code_Period);
 BIND(N,        toggle_bottom_view_command,  C|Key_Code_Period);
 //
 BIND(N|0,  clipboard_pop_command,  S|Key_Code_P);
 BIND(V,    quick_align_command,    M|Key_Code_A);
 
 //-NOTE(kv) KV miscellaneous binds
 BIND(N, kv_handle_return_normal_mode, Key_Code_Return);
 BIND(N, cmd_insert_ampersand,       Key_Code_S);
 BIND(N, cmd_insert_caret,           Key_Code_6);
 BIND(N, cmd_handle_8_normal,        Key_Code_8);
 BIND(N, cmd_insert_parens,          Key_Code_9);
 BIND(N, kv_insert_at_sign,          Key_Code_2);
 BIND(N, kv_insert_hash_tag,         Key_Code_3);
 BIND(N, kv_do_underscore,           Key_Code_Minus);
 BIND(N, kv_do_underscore_shifted, S|Key_Code_Minus);
 BIND(N, move_parameter_left,      C|Key_Code_H);
 BIND(N, move_parameter_right,     C|Key_Code_L);
 
#undef BIND
}

function void 
default_custom_layer_init(App *app)
{
 Thread_Context *tctx = get_thread_context(app);
 
 // NOTE(allen): setup for default framework
 default_framework_init(app);
 
 // NOTE(allen): default hooks and command maps
 set_all_default_hooks(app);
 mapping_init(tctx, &framework_mapping);
 String_ID global_map_id = vars_intern_lit("keys_global");
 String_ID file_map_id   = vars_intern_lit("keys_file");
 String_ID code_map_id   = vars_intern_lit("keys_code");
 setup_essential_mapping(&framework_mapping, global_map_id, file_map_id, code_map_id);
}

function Tick_Function kv_tick;
//

function void 
kv_custom_layer_init(App *app)
{
 Scratch_Block scratch;
 
 default_framework_init(app);
 set_all_default_hooks(app);
 
 vim_buffer_peek_list[alen(vim_default_peek_list) + 0] = { buffer_identifier(strlit("*scratch*")), 1.f, 1.f };
 vim_buffer_peek_list[alen(vim_default_peek_list) + 1] = { buffer_identifier(strlit("todo.txt")),  1.f, 1.f };
 vim_request_vtable[(u32)VIM_REQUEST_COUNT + (u32)BYP_REQUEST_Title]     = byp_apply_title;
 vim_request_vtable[(u32)VIM_REQUEST_COUNT + (u32)BYP_REQUEST_Comment]   = byp_apply_comment;
 vim_request_vtable[(u32)VIM_REQUEST_COUNT + (u32)BYP_REQUEST_UnComment] = byp_apply_uncomment;
 
 vim_text_object_vtable[(u32)VIM_TEXT_OBJECT_COUNT + (u32)BYP_OBJECT_param0] = {',', (Vim_Text_Object_Func *)byp_object_param};
 vim_text_object_vtable[(u32)VIM_TEXT_OBJECT_COUNT + (u32)BYP_OBJECT_param1] = {';', (Vim_Text_Object_Func *)byp_object_param};
 vim_text_object_vtable[(u32)VIM_TEXT_OBJECT_COUNT + (u32)BYP_OBJECT_camel0] = {'_', (Vim_Text_Object_Func *)byp_object_camel};
 vim_text_object_vtable[(u32)VIM_TEXT_OBJECT_COUNT + (u32)BYP_OBJECT_camel1] = {'-', (Vim_Text_Object_Func *)byp_object_camel};
 kv_vim_init(app);
 
 set_custom_hook(app, HookID_SaveFile,                kv_file_save);
 set_custom_hook(app, HookID_RenderCaller,            kv_render_caller);
 set_custom_hook(app, HookID_WholeScreenRenderCaller, vim_draw_whole_screen);
 //
 set_custom_hook(app, HookID_Tick,             kv_tick);
 set_custom_hook(app, HookID_NewFile,          kv_new_file);
 set_custom_hook(app, HookID_BeginBuffer,      kv_begin_buffer);
 set_custom_hook(app, HookID_BufferEditRange,  kv_buffer_edit_range);
 set_custom_hook(app, HookID_ViewChangeBuffer, vim_view_change_buffer);
 set_custom_hook(app, HookID_ViewEventHandler, kv_view_input_handler);
 set_custom_hook(app, HookID_DeltaRule,        F4_DeltaRule_lite);
 set_custom_hook(app, HookID_BufferRegion,     kv_buffer_region_hook);
 set_custom_hook_memory_size(app, HookID_DeltaRule, delta_ctx_size(sizeof(v2)));
 
 Thread_Context *tctx = get_thread_context(app);
 mapping_init(tctx, &framework_mapping);
 kv_essential_mapping(&framework_mapping);
 //
 kvInitShiftedTable();
 kvInitQuailTable(app);
 //
 kv_vim_bindings(app);
 
 // NOTE(rjf): Set up custom code index.
 F4_Index_Initialize();
 // NOTE(rjf): Register languages.
 F4_RegisterLanguages();
 
 {// NOTE: Game stuff
  // Export ed_api
  ed_api_fill_vtable(&const_ed_api);
  ed_api_fill_vtable_new(&const_ed_api_new);
  String binary_dir = system_get_path(scratch, SystemPath_BinaryDirectory);
  GAME_DLL_PATH = pjoin(&thread_permanent_arena, binary_dir, strlit("game_main.dll"));
 }
}

function void
custom_layer_init(App *app)
{
#if USE_LAYER_kv
 kv_custom_layer_init(app);
 
 {// note(kv): shared startup code
  MappingScope();
  SelectMapping(&framework_mapping);
  
  String_ID global_id = vars_intern_lit("keys_global");
  SelectMap(global_id);
  BindCore(kv_startup, CoreCode_Startup);
 }
 
#elif USE_LAYER_fleury
 fleury_custom_layer_init(app);
#elif USE_LAYER_fleury_lite
 fleury_lite_custom_layer_init(app);
#elif USE_LAYER_default
 default_custom_layer_init(app);
#endif
}
//-