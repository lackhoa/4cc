#include "4coder_kv_build.cpp"
#include "4coder_kv_commands.cpp"
#include "4coder_kv_hooks.cpp"
#include "4coder_kv_draw.cpp"
#include "4coder_kv_vim_stuff.cpp"
#include "4coder_kv_lang_list.h"
// #include "ad_editor.h"

// note: Custom layer swapping for testing and trying out.
// note: Please enable only one layer, or else it explodes!
#if KV_INTERNAL
#    define USE_LAYER_kv               1
#    define USE_LAYER_fleury_lite      0
#    define USE_LAYER_fleury           0
#    define USE_LAYER_default_bindings 0
#else
#    define USE_LAYER_kv               1
#endif

#if    USE_LAYER_fleury
#    include "4coder_fleury/4coder_fleury.cpp"  // because including this will bring in weird commands I don't need / can't use
#elif  USE_LAYER_fleury_lite
#    include "4coder_kv_fleury_lite.cpp"
#endif

#if !defined(META_PASS)
#    include "generated/managed_id_metadata.cpp"  // from Mr. Allen 4th
#endif

function void kvInitShiftedTable()
{
  Base_Allocator *base = get_base_allocator_system();
  shifted_version_of_characters = make_table_u64_u64(base, 128);
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

function void kv_open_startup_files(Application_Links *app)
{
  set_hot_directory(app, SCu8("/Users/khoa/AutoDraw/4coder_kv/"));
  load_project(app);
  View_ID view = get_this_ctx_view(app, Access_Always);

#if USE_LAYER_fleury || USE_LAYER_fleury_lite
  char *startup_file = "~/AutoDraw/4coder_kv/4coder_fleury/4coder_fleury_plots_demo.cpp";
#else
  char *startup_file = "~/notes/note.skm";
#endif

  Buffer_ID buffer = create_buffer(app, SCu8(startup_file), 0);
  if (view && buffer)
  {
    view_set_buffer(app, view, buffer, 0);
  }
}

// NOTE(kv): shared between custom layers
function void
kv_essential_mapping(Mapping *mapping)
{
  String_ID global_id = vars_save_string_lit("keys_global");
  String_ID file_id   = vars_save_string_lit("keys_file");
  String_ID code_id   = vars_save_string_lit("keys_code");

  MappingScope();
  SelectMapping(mapping);

  SelectMap(global_id);
  BindCore(vim_try_exit, CoreCode_TryExit);
  BindCore(clipboard_record_clip, CoreCode_NewClipboardContents);
  BindMouseWheel(mouse_wheel_scroll);
  BindMouseWheel(mouse_wheel_change_face_size, KeyCode_Control);
  // BindCore(vim_file_externally_modified, CoreCode_FileExternallyModified);  NOTE(kv): cool idea but there are auto-generated files that we don't care about

  SelectMap(file_id);
  ParentMap(global_id);
  BindTextInput(write_text_input);
  BindMouse(click_set_cursor_and_mark, MouseCode_Left);
  BindMouseRelease(click_set_cursor, MouseCode_Left);
  BindCore(click_set_cursor_and_mark, CoreCode_ClickActivateView);
  BindMouseMove(click_set_cursor_if_lbutton);

  SelectMap(code_id);
  ParentMap(file_id);
}

// NOTE(kv): shared between custom layers
function void 
kv_default_bindings(Mapping *mapping)
{
  String_ID global_id = vars_save_string_lit("keys_global");
  String_ID file_id   = vars_save_string_lit("keys_file");
  String_ID code_id   = vars_save_string_lit("keys_code");

  MappingScope();
  SelectMapping(mapping);

  SelectMap(global_id);

  Bind(toggle_fullscreen,   KeyCode_F11);
  Bind(increase_face_size,  KeyCode_Equal, KeyCode_Control);
  Bind(decrease_face_size,  KeyCode_Minus, KeyCode_Control);
  Bind(byp_reset_face_size, KeyCode_0, KeyCode_Control);
  Bind(exit_4coder,         KeyCode_Q, KeyCode_Control);
  Bind(exit_4coder,         KeyCode_Q, KeyCode_Command);

  SelectMap(file_id);
  ParentMap(global_id);

  Bind(delete_char,        KeyCode_Delete);
  Bind(backspace_char,     KeyCode_Backspace);
  Bind(move_up,            KeyCode_Up);
  Bind(move_down,          KeyCode_Down);
  Bind(move_left,          KeyCode_Left);
  Bind(move_right,         KeyCode_Right);
  Bind(seek_end_of_line,   KeyCode_End);
  Bind(right_adjust_view,  KeyCode_Home);

  SelectMap(code_id);
  ParentMap(file_id);
}

CUSTOM_COMMAND_SIG(kv_startup)
{
  default_startup(app);  // NOTE(kv): this thing stomps over your binding
  kv_essential_mapping(&framework_mapping);
  kv_default_bindings(&framework_mapping);
  kv_open_startup_files(app);
	set_window_title(app, string_u8_litexpr("4coder kv"));
}

function void kvInitQuailTable(Application_Links *app)
{
  arrsetcap(kv_quail_table, 64);
  
#define QUAIL_DEFRULE(KEY, VALUE) kv_quail_defrule(app, KEY, VALUE, strlen(KEY)-1, 0, strlen(VALUE))

  QUAIL_DEFRULE(",,", "_");
  QUAIL_DEFRULE(",,.", "=>");
  //
  QUAIL_DEFRULE(",.", "->");
  QUAIL_DEFRULE(",..", "<>");
  //
  QUAIL_DEFRULE(".,", "<-");
  //
  QUAIL_DEFRULE(";;", ":");
  QUAIL_DEFRULE(";;;", ";;");
  //
  QUAIL_DEFRULE("11", "!");
  QUAIL_DEFRULE("22", "@");
  QUAIL_DEFRULE("33", "#");
  QUAIL_DEFRULE("44", "$");
  QUAIL_DEFRULE("55", "%");
  QUAIL_DEFRULE("77", "&");
  QUAIL_DEFRULE("88", "*");
  kv_quail_defrule(app, "99", "()", 1,0,1);
  // QUAIL_DEFRULE("00", /*(*/")");  // I wanna type "0"
  //
  kv_quail_defrule(app, "[", "[]", 0,0,1);
  // {
  kv_quail_defrule(app, "[[", "{}", 1,1,1);
  QUAIL_DEFRULE("]]", "}");
  //
  kv_quail_defrule(app, "''", "\"\"", 1,0,1);
  QUAIL_DEFRULE("leq", "<=");
  QUAIL_DEFRULE("geq", ">=");
  QUAIL_DEFRULE("gtt", ">");
  QUAIL_DEFRULE("ltt", "<");
  QUAIL_DEFRULE("neq", "!=");

#undef QUAIL_DEFRULE
}

// ;binding
function void kv_vim_bindings(Application_Links *app)
{
  u32 N = bit_1;
  u32 I = bit_2;
  u32 V = bit_3;
  u32 MAP = 0;
#define BIND(...) if (!VimBind(__VA_ARGS__)) { printf("Keymap conflict at line %d!!!\n", __LINE__); }

  u32 C  = KeyMod_Ctl;
  u32 S  = KeyMod_Sft;
  u32 M = OS_MAC ? KeyMod_Cmd : KeyMod_Alt;
  Key_Code leader = KeyCode_BackwardSlash;

  BIND(MAP, kv_vim_normal_mode, KeyCode_Escape);
  BIND(MAP, kv_void_command,    KeyCode_Menu);  // todo(kv) this key inserts some random crap and I still can't turn it off.

  /// Rebinds
  BIND(N|MAP, undo,                                 KeyCode_U);
  BIND(N|MAP, undo,                              (C|KeyCode_Z));
  BIND(N|MAP, redo,                              (C|KeyCode_R));
  BIND(N|MAP, vim_interactive_open_or_new,    SUB_G,  KeyCode_F);
  BIND(N|MAP, kv_open_file_ultimate,             M|KeyCode_F);
  BIND(N|MAP, vim_next_4coder_jump,              M|KeyCode_N);
  BIND(N|MAP, vim_prev_4coder_jump,              M|KeyCode_P);
  BIND(N|MAP, view_buffer_other_panel,           M|KeyCode_D);
  BIND(N|MAP, vim_switch_lister,                 M|KeyCode_B);

  /// Mode Binds
  BIND(N|V|MAP, vim_modal_i,                        KeyCode_I);
  BIND(N|0|MAP, goto_line,                       (S|KeyCode_Semicolon));
  // BIND(N|MAP,   vim_insert_begin,                (S|KeyCode_I));
  BIND(N|MAP,   vim_replace_mode,                (S|KeyCode_R));
  BIND(N|0|MAP, vim_visual_mode,                    KeyCode_V);
  BIND(0|V|MAP, kv_vim_visual_line_mode,            KeyCode_V);
  BIND(N|V|MAP, vim_visual_mode,                  S|KeyCode_V);
  BIND(N|V|MAP, vim_visual_mode,                  C|KeyCode_V);
  BIND(N|0|MAP, vim_prev_visual,          SUB_G,    KeyCode_V);
  BIND(N|0|MAP, vim_newline_below,                  KeyCode_O);
  BIND(N|0|MAP, vim_newline_above,                S|KeyCode_O);
  BIND(N|0|MAP, kv_newline_above,                 C|KeyCode_K);
  BIND(N|0|MAP, kv_newline_below,                 C|KeyCode_J);

  /// Sub Mode Binds
  BIND(N|V|MAP, vim_submode_g,                        KeyCode_G);
  BIND(N|V|MAP, vim_submode_z,                        KeyCode_Z);
  BIND(N|V|MAP, vim_submode_leader,                   leader);

  /// Request Binds
  BIND(N|V|MAP, vim_request_yank,                     KeyCode_Y);
  BIND(N|V|MAP, vim_request_delete,                   KeyCode_D);
  BIND(N|V|MAP, vim_request_change,                   KeyCode_C);
  BIND(N|V|MAP, vim_delete_end,                     S|KeyCode_D);
  BIND(N|V|MAP, vim_change_end,                     S|KeyCode_C);
  BIND(N|V|MAP, vim_yank_end,                       S|KeyCode_Y);
  BIND(N|0|MAP, auto_indent_line_at_cursor,           KeyCode_Tab);
  BIND(0|V|MAP, auto_indent_range,                    KeyCode_Tab);
  BIND(N|V|MAP, vim_lowercase,            SUB_G,      KeyCode_U);
  BIND(  V|MAP, vim_toggle_case,                      KeyCode_Comma);
  BIND(N|V|MAP, vim_request_indent,                 S|KeyCode_Period);
  BIND(N|V|MAP, vim_request_outdent,                S|KeyCode_Comma);
  BIND(V|MAP,   vim_replace_range_next,               KeyCode_R);

  /// Edit Binds
  BIND(N|V|MAP,   vim_paste_before,                KeyCode_P);
  BIND(N|MAP,     vim_delete_char,                 KeyCode_X);
  BIND(N|MAP,     vim_replace_next_char,           KeyCode_R);
  BIND(N|V|MAP,   vim_combine_line,             (S|KeyCode_J));
  BIND(N|V|MAP,   vim_combine_line,      SUB_G, (S|KeyCode_J));
  BIND(N|MAP,     vim_last_command,                KeyCode_Period);
  // BIND(N|MAP,     vim_backspace_char,              KeyCode_Backspace);
  // BIND(N|MAP,     vim_delete_char,                 KeyCode_Delete);
  BIND(I|MAP,     word_complete,                   KeyCode_Tab);
  BIND(I|MAP,     vim_paste_before,              M|KeyCode_V);
  BIND(I|MAP,     kv_newline_and_indent,           KeyCode_Return)
  BIND(N|MAP,     kv_newline_and_indent,         S|KeyCode_K); 

  /// Movement Binds
  BIND(N|V|MAP, vim_left,                           KeyCode_H);
  BIND(N|V|MAP, vim_down,                           KeyCode_J);
  BIND(N|V|MAP, vim_up,                             KeyCode_K);
  BIND(N|V|MAP, vim_right,                          KeyCode_L);
  BIND(N|V|MAP, vim_end_line,                    (S|KeyCode_4));
  BIND(N|V|MAP, vim_begin_line,                   M|KeyCode_I);
  BIND(N|V|MAP, vim_forward_WORD,                (S|KeyCode_W));
  BIND(N|V|MAP, vim_backward_WORD,               (S|KeyCode_B));
  BIND(N|V|MAP, vim_forward_end,                    KeyCode_E);
  BIND(N|V|MAP, vim_forward_END,                 (S|KeyCode_E));
  BIND(N|V|MAP, vim_modal_0,                        KeyCode_0);

  BIND(N|V|MAP, vim_forward_word,  KeyCode_W);
  BIND(N|V|MAP, vim_backward_word, KeyCode_B);

  BIND(N|V|MAP, vim_file_top,             SUB_G,     KeyCode_G);
  BIND(N|V|MAP, vim_goto_line,                   (S|KeyCode_G));
  BIND(N|V|MAP, vim_goto_column,                 (S|KeyCode_BackwardSlash));
  BIND(N|V|MAP, vim_modal_percent,               (S|KeyCode_5));
  BIND(N|V|MAP, vim_bounce,                      (C|KeyCode_5));
  BIND(N|V|MAP, vim_set_seek_char,                    KeyCode_F);
  BIND(N|0|MAP, vim_paragraph_up,                     KeyCode_LeftBracket);
  BIND(N|0|MAP, vim_paragraph_down,                   KeyCode_RightBracket);
  BIND(N|V|MAP, vim_screen_top,                  (S|KeyCode_H));
  BIND(N|V|MAP, vim_screen_bot,                  (S|KeyCode_L));
  BIND(N|V|MAP, vim_screen_mid,                  (S|KeyCode_M));
  BIND(V|MAP,   cursor_mark_swap,                   KeyCode_O);
  BIND(V|MAP,   vim_block_swap,                  (S|KeyCode_O));

  BIND(N|MAP, vim_search_identifier,           (C|S|KeyCode_8));
  BIND(N|MAP, vim_search_identifier,             (S|KeyCode_8));
  BIND(N|MAP, vim_clear_search,          SUB_Leader,KeyCode_Space);
  BIND(N|MAP, vim_start_search_forward,             KeyCode_ForwardSlash);
  BIND(N|MAP, vim_start_search_backward,         (S|KeyCode_ForwardSlash));
  BIND(N|MAP, vim_to_next_pattern,                  KeyCode_N);
  BIND(N|MAP, vim_to_prev_pattern,               (S|KeyCode_N));
  BIND(N|MAP, vim_in_next_pattern,        SUB_G,    KeyCode_N);
  BIND(N|MAP, vim_in_prev_pattern,        SUB_G, (S|KeyCode_N));

  BIND(N|MAP, vim_prev_jump,                     (C|KeyCode_O));
  BIND(N|MAP, vim_next_jump,                     (C|KeyCode_I));

  /// Screen Adjust Binds
  BIND(N|V|MAP, vim_whole_page_up,                 (C|KeyCode_B));
  BIND(N|V|MAP, vim_whole_page_down,               (C|KeyCode_F));
  BIND(N|V|MAP, vim_line_down,               (C|S|KeyCode_Y));
  BIND(N|V|MAP, vim_line_down,                     (C|KeyCode_Y));
  BIND(N|V|MAP, vim_line_up,                 (C|S|KeyCode_E));
  BIND(N|V|MAP, vim_line_up,                       (C|KeyCode_E));
  BIND(N|V|MAP, vim_scroll_screen_top,         SUB_Z,   KeyCode_T);
  BIND(N|V|MAP, vim_scroll_screen_mid,         SUB_Z,   KeyCode_Z);
  BIND(N|V|MAP, vim_scroll_screen_bot,         SUB_Z,   KeyCode_B);

  /// Miscellaneous Binds
  BIND(N|V|MAP, vim_set_mark,                         KeyCode_M);
  BIND(N|0|MAP, vim_goto_mark,                        KeyCode_Quote);
  BIND(N|V|MAP, vim_toggle_macro,                   S|KeyCode_Q);
  BIND(N|V|MAP, vim_play_macro,                     S|KeyCode_2);
  BIND(N|MAP,   open_matching_file_cpp,               KeyCode_F12);

  /// Panel
  BIND(N|MAP, change_active_panel,         C|KeyCode_Tab);
  BIND(N|MAP, close_panel,                 M|KeyCode_W);

  // Sub modes
  BIND(N|V|MAP, vim_leader_d, SUB_Leader,       KeyCode_D);
  BIND(N|V|MAP, vim_leader_c, SUB_Leader,       KeyCode_C);
  BIND(N|V|MAP, vim_leader_D, SUB_Leader,  (S|KeyCode_D));
  BIND(N|V|MAP, vim_leader_C, SUB_Leader,  (S|KeyCode_C));

  // Project keys
  BIND(N|MAP,  kv_build_normal,               M|KeyCode_M);
  BIND(N|MAP,  kv_build_run_only,           C|M|KeyCode_M);
  BIND(N|MAP,  kv_build_full_rebuild,       S|M|KeyCode_M);
  // Language support
  BIND(N|MAP,  vim_goto_definition,                 KeyCode_F1);
  BIND(N|MAP,  vim_goto_definition_other_panel,   M|KeyCode_F1);
  BIND(N|MAP,  kv_list_all_locations,               KeyCode_F2);
  //
  BIND(N|MAP,   byp_request_comment,   SUB_G,     KeyCode_ForwardSlash);
  BIND(N|MAP,   byp_request_uncomment, SUB_G,   S|KeyCode_ForwardSlash);
  // sexpr movement
  BIND(N|V|MAP,   kv_sexpr_up,     M|KeyCode_K);
  BIND(N|V|MAP,   kv_sexpr_down,   M|KeyCode_J);
  BIND(N|V|MAP,   kv_sexpr_right,  M|KeyCode_L);
  BIND(N|V|MAP,   kv_sexpr_left,   M|KeyCode_H);
  BIND(N|V|MAP,   kv_sexpr_end,    M|KeyCode_Semicolon);
  BIND(N|MAP,     kv_sexpr_select_whole, KeyCode_Q);
  // surround paren
  BIND(V|MAP,   kv_surround_paren,                 KeyCode_0);
  BIND(V|MAP,   kv_surround_paren_spaced,          KeyCode_9);
  BIND(V|MAP,   kv_surround_bracket,               KeyCode_RightBracket);
  BIND(V|MAP,   kv_surround_bracket_spaced,        KeyCode_LeftBracket);
  BIND(V|MAP,   kv_surround_brace,               S|KeyCode_RightBracket);
  BIND(V|MAP,   kv_surround_brace_spaced,        S|KeyCode_LeftBracket);
  BIND(V|MAP,   kv_surround_double_quote,          KeyCode_Quote);
  BIND(N|MAP,   kv_delete_surrounding_groupers,  M|KeyCode_RightBracket);

  // NOTE(kv) KV miscellaneous binds
  BIND(N|  MAP,  kv_handle_return,                        KeyCode_Return);
  BIND(N|  MAP,  if_read_only_goto_position_same_panel, S|KeyCode_Return);
  //
  BIND(N|  MAP,  write_space,                KeyCode_Space);
  BIND(N|  MAP,  vim_insert_end,             KeyCode_A);
  BIND(  V|MAP,  vim_end_line,               KeyCode_A);
  BIND(N  |MAP,  vim_end_line,             S|KeyCode_A);
  BIND(N|  MAP,  kv_shift_character,         KeyCode_Comma);
  BIND(N|  MAP,  exit_4coder,              M|KeyCode_Q);
  BIND(N|V|MAP,  vim_command_mode,           KeyCode_Semicolon);
  BIND(N|  MAP,  kv_reopen_with_confirmation,                     S|KeyCode_U);
  BIND(N|  MAP,  quick_swap_buffer,        M|KeyCode_Comma);
  BIND(N|0|MAP,  kv_do_t,                    KeyCode_T);
  BIND(N|0|MAP,  kv_do_T,                  S|KeyCode_T);
  BIND(N|0|MAP,  open_panel_vsplit,        M|KeyCode_V);

#undef BIND
}

function void 
default_bindings_custom_layer_init(Application_Links *app)
{
    Thread_Context *tctx = get_thread_context(app);
    
    // NOTE(allen): setup for default framework
    default_framework_init(app);
    
    // NOTE(allen): default hooks and command maps
    set_all_default_hooks(app);
    mapping_init(tctx, &framework_mapping);
    String_ID global_map_id = vars_save_string_lit("keys_global");
    String_ID file_map_id = vars_save_string_lit("keys_file");
    String_ID code_map_id = vars_save_string_lit("keys_code");
#if OS_MAC
    setup_mac_mapping(&framework_mapping, global_map_id, file_map_id, code_map_id);
#else
    setup_default_mapping(&framework_mapping, global_map_id, file_map_id, code_map_id);
#endif
	  setup_essential_mapping(&framework_mapping, global_map_id, file_map_id, code_map_id);
}

function void 
kv_custom_layer_init(Application_Links *app)
{
  default_framework_init(app);
  set_all_default_hooks(app);
  
  // fleury
  global_frame_arena = make_arena(get_base_allocator_system());
  permanent_arena = make_arena(get_base_allocator_system());

  vim_buffer_peek_list[ArrayCount(vim_default_peek_list) + 0] = { buffer_identifier(string_u8_litexpr("*scratch*")), 1.f, 1.f };
  vim_buffer_peek_list[ArrayCount(vim_default_peek_list) + 1] = { buffer_identifier(string_u8_litexpr("todo.txt")),  1.f, 1.f };
  vim_request_vtable[VIM_REQUEST_COUNT + BYP_REQUEST_Title]     = byp_apply_title;
  vim_request_vtable[VIM_REQUEST_COUNT + BYP_REQUEST_Comment]   = byp_apply_comment;
  vim_request_vtable[VIM_REQUEST_COUNT + BYP_REQUEST_UnComment] = byp_apply_uncomment;

  vim_text_object_vtable[VIM_TEXT_OBJECT_COUNT + BYP_OBJECT_param0] = {',', (Vim_Text_Object_Func *)byp_object_param};
  vim_text_object_vtable[VIM_TEXT_OBJECT_COUNT + BYP_OBJECT_param1] = {';', (Vim_Text_Object_Func *)byp_object_param};
  vim_text_object_vtable[VIM_TEXT_OBJECT_COUNT + BYP_OBJECT_camel0] = {'_', (Vim_Text_Object_Func *)byp_object_camel};
  vim_text_object_vtable[VIM_TEXT_OBJECT_COUNT + BYP_OBJECT_camel1] = {'-', (Vim_Text_Object_Func *)byp_object_camel};
  kv_vim_init(app);

  set_custom_hook(app, HookID_SaveFile,                kv_file_save);
  // set_custom_hook(app, HookID_BufferRegion,            byp_buffer_region);
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
  set_custom_hook_memory_size(app, HookID_DeltaRule, delta_ctx_size(sizeof(Vec2_f32)));

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
 
  /*
  {// AutoDraw code, which has to be run in a main thread unfortunately because it creates a window
    char *todo_autodraw_path = (char *)"/Users/khoa/AutoDraw/build";
    adMainFcoder(todo_autodraw_path);
  }

  // gb_mutex_init(&var_mutex);
  */
}

/*
CUSTOM_COMMAND_SIG(ad_toggle_test)
CUSTOM_DOC("test ad integration")
{
  gb_mutex_lock(&var_mutex);
  ad_test_boolean = !ad_test_boolean;
  gb_mutex_unlock(&var_mutex);
}
*/

extern "C" void
custom_layer_init(Application_Links *app)
{
#if USE_LAYER_kv
  kv_custom_layer_init(app);
#elif USE_LAYER_fleury
  fleury_custom_layer_init(app);
#elif USE_LAYER_fleury_lite
  fleury_lite_custom_layer_init(app);
#elif USE_LAYER_default_bindings
  default_bindings_custom_layer_init(app);
#endif

  {// note(kv): shared startup code
    MappingScope();
    SelectMapping(&framework_mapping);
  
    String_ID global_id = vars_save_string_lit("keys_global");
    SelectMap(global_id);
    BindCore(kv_startup, CoreCode_Startup);
  }
}
