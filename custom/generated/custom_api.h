#pragma once

#include "4coder_game_shared.h"

#define custom_global_set_setting_sig() b32 custom_global_set_setting(App* app, Global_Setting_ID setting, i64 value)
#define custom_global_get_screen_rectangle_sig() Rect_f32 custom_global_get_screen_rectangle(App* app)
#define custom_get_thread_context_sig() Thread_Context* custom_get_thread_context(App* app)
#define custom_create_child_process_sig() Child_Process_ID custom_create_child_process(App* app, String path, String command)
#define custom_child_process_set_target_buffer_sig() b32 custom_child_process_set_target_buffer(App* app, Child_Process_ID child_process_id, Buffer_ID buffer_id, Child_Process_Set_Target_Flags flags)
#define custom_buffer_get_attached_child_process_sig() Child_Process_ID custom_buffer_get_attached_child_process(App* app, Buffer_ID buffer_id)
#define custom_child_process_get_attached_buffer_sig() Buffer_ID custom_child_process_get_attached_buffer(App* app, Child_Process_ID child_process_id)
#define custom_child_process_get_state_sig() Process_State custom_child_process_get_state(App* app, Child_Process_ID child_process_id)
#define custom_enqueue_virtual_event_sig() b32 custom_enqueue_virtual_event(App* app, Input_Event* event)
#define custom_get_buffer_count_sig() i1 custom_get_buffer_count(App* app)
#define custom_get_buffer_next_sig() Buffer_ID custom_get_buffer_next(App* app, Buffer_ID buffer_id, Access_Flag access)
#define custom_get_buffer_by_name_sig() Buffer_ID custom_get_buffer_by_name(App* app, String name, Access_Flag access)
#define custom_get_buffer_by_filename_sig() Buffer_ID custom_get_buffer_by_filename(App* app, String filename, Access_Flag access)
#define custom_buffer_read_range_sig() b32 custom_buffer_read_range(App* app, Buffer_ID buffer_id, Range_i64 range, u8* out)
#define custom_buffer_replace_range_sig() b32 custom_buffer_replace_range(App* app, Buffer_ID buffer_id, Range_i64 range, String string)
#define custom_buffer_batch_edit_sig() b32 custom_buffer_batch_edit(App* app, Buffer_ID buffer_id, Batch_Edit* batch)
#define custom_buffer_seek_string_sig() String_Match custom_buffer_seek_string(App* app, Buffer_ID buffer, String needle, Scan_Direction direction, i64 start_pos)
#define custom_buffer_seek_character_class_sig() String_Match custom_buffer_seek_character_class(App* app, Buffer_ID buffer, Character_Predicate* predicate, Scan_Direction direction, i64 start_pos)
#define custom_buffer_line_y_difference_sig() f32 custom_buffer_line_y_difference(App* app, Buffer_ID buffer_id, f32 width, Face_ID face_id, i64 line_a, i64 line_b)
#define custom_buffer_line_shift_y_sig() Line_Shift_Vertical custom_buffer_line_shift_y(App* app, Buffer_ID buffer_id, f32 width, Face_ID face_id, i64 line, f32 y_shift)
#define custom_buffer_pos_at_relative_xy_sig() i64 custom_buffer_pos_at_relative_xy(App* app, Buffer_ID buffer_id, f32 width, Face_ID face_id, i64 base_line, Vec2_f32 relative_xy)
#define custom_buffer_relative_box_of_pos_sig() Rect_f32 custom_buffer_relative_box_of_pos(App* app, Buffer_ID buffer_id, f32 width, Face_ID face_id, i64 base_line, i64 pos)
#define custom_buffer_padded_box_of_pos_sig() Rect_f32 custom_buffer_padded_box_of_pos(App* app, Buffer_ID buffer_id, f32 width, Face_ID face_id, i64 base_line, i64 pos)
#define custom_buffer_relative_character_from_pos_sig() i64 custom_buffer_relative_character_from_pos(App* app, Buffer_ID buffer_id, f32 width, Face_ID face_id, i64 base_line, i64 pos)
#define custom_buffer_pos_from_relative_character_sig() i64 custom_buffer_pos_from_relative_character(App* app, Buffer_ID buffer_id, f32 width, Face_ID face_id, i64 base_line, i64 relative_character)
#define custom_view_line_y_difference_sig() f32 custom_view_line_y_difference(App* app, View_ID view_id, i64 line_a, i64 line_b)
#define custom_view_line_shift_y_sig() Line_Shift_Vertical custom_view_line_shift_y(App* app, View_ID view_id, i64 line, f32 y_shift)
#define custom_view_pos_at_relative_xy_sig() i64 custom_view_pos_at_relative_xy(App* app, View_ID view_id, i64 base_line, Vec2_f32 relative_xy)
#define custom_view_relative_box_of_pos_sig() Rect_f32 custom_view_relative_box_of_pos(App* app, View_ID view_id, i64 base_line, i64 pos)
#define custom_view_padded_box_of_pos_sig() Rect_f32 custom_view_padded_box_of_pos(App* app, View_ID view_id, i64 base_line, i64 pos)
#define custom_view_relative_character_from_pos_sig() i64 custom_view_relative_character_from_pos(App* app, View_ID view_id, i64 base_line, i64 pos)
#define custom_view_pos_from_relative_character_sig() i64 custom_view_pos_from_relative_character(App* app, View_ID view_id, i64 base_line, i64 character)
#define custom_buffer_exists_sig() b32 custom_buffer_exists(App* app, Buffer_ID buffer_id)
#define custom_buffer_get_access_flags_sig() Access_Flag custom_buffer_get_access_flags(App* app, Buffer_ID buffer_id)
#define custom_buffer_get_size_sig() i64 custom_buffer_get_size(App* app, Buffer_ID buffer_id)
#define custom_buffer_get_line_count_sig() i64 custom_buffer_get_line_count(App* app, Buffer_ID buffer_id)
#define custom_push_buffer_base_name_sig() String custom_push_buffer_base_name(App* app, Arena* arena, Buffer_ID buffer_id)
#define custom_push_buffer_unique_name_sig() String custom_push_buffer_unique_name(App* app, Arena* out, Buffer_ID buffer_id)
#define custom_push_buffer_filename_sig() String custom_push_buffer_filename(App* app, Arena* arena, Buffer_ID buffer_id)
#define custom_buffer_get_dirty_state_sig() Dirty_State custom_buffer_get_dirty_state(App* app, Buffer_ID buffer_id)
#define custom_buffer_set_dirty_state_sig() b32 custom_buffer_set_dirty_state(App* app, Buffer_ID buffer_id, Dirty_State dirty_state)
#define custom_buffer_set_layout_sig() b32 custom_buffer_set_layout(App* app, Buffer_ID buffer_id, Layout_Function* layout_func)
#define custom_buffer_clear_layout_cache_sig() b32 custom_buffer_clear_layout_cache(App* app, Buffer_ID buffer_id)
#define custom_buffer_get_layout_sig() Layout_Function* custom_buffer_get_layout(App* app, Buffer_ID buffer_id)
#define custom_buffer_get_setting_sig() b32 custom_buffer_get_setting(App* app, Buffer_ID buffer_id, Buffer_Setting_ID setting, i64* value_out)
#define custom_buffer_set_setting_sig() b32 custom_buffer_set_setting(App* app, Buffer_ID buffer_id, Buffer_Setting_ID setting, i64 value)
#define custom_buffer_get_managed_scope_sig() Managed_Scope custom_buffer_get_managed_scope(App* app, Buffer_ID buffer_id)
#define custom_buffer_send_end_signal_sig() b32 custom_buffer_send_end_signal(App* app, Buffer_ID buffer_id)
#define custom_create_buffer_sig() Buffer_ID custom_create_buffer(App* app, String filename, Buffer_Create_Flag flags)
#define custom_buffer_save_sig() b32 custom_buffer_save(App* app, Buffer_ID buffer_id, String filename, Buffer_Save_Flag flags)
#define custom_buffer_kill_sig() Buffer_Kill_Result custom_buffer_kill(App* app, Buffer_ID buffer_id, Buffer_Kill_Flag flags)
#define custom_buffer_reopen_sig() Buffer_Reopen_Result custom_buffer_reopen(App* app, Buffer_ID buffer_id, Buffer_Reopen_Flag flags)
#define custom_buffer_get_file_attributes_sig() File_Attributes custom_buffer_get_file_attributes(App* app, Buffer_ID buffer_id)
#define custom_get_view_next_sig() View_ID custom_get_view_next(App* app, View_ID view_id, Access_Flag access)
#define custom_get_view_prev_sig() View_ID custom_get_view_prev(App* app, View_ID view_id, Access_Flag access)
#define custom_get_this_ctx_view_sig() View_ID custom_get_this_ctx_view(App* app, Access_Flag access)
#define custom_get_active_view_sig() View_ID custom_get_active_view(App* app, Access_Flag access)
#define custom_view_exists_sig() b32 custom_view_exists(App* app, View_ID view_id)
#define custom_view_get_buffer_sig() Buffer_ID custom_view_get_buffer(App* app, View_ID view_id, Access_Flag access)
#define custom_view_get_cursor_pos_sig() i64 custom_view_get_cursor_pos(App* app, View_ID view_id)
#define custom_view_get_mark_pos_sig() i64 custom_view_get_mark_pos(App* app, View_ID view_id)
#define custom_view_get_preferred_x_sig() f32 custom_view_get_preferred_x(App* app, View_ID view_id)
#define custom_view_set_preferred_x_sig() b32 custom_view_set_preferred_x(App* app, View_ID view_id, f32 x)
#define custom_view_get_screen_rect_sig() Rect_f32 custom_view_get_screen_rect(App* app, View_ID view_id)
#define custom_view_get_panel_sig() Panel_ID custom_view_get_panel(App* app, View_ID view_id)
#define custom_panel_get_view_sig() View_ID custom_panel_get_view(App* app, Panel_ID panel_id, Access_Flag access)
#define custom_panel_is_split_sig() b32 custom_panel_is_split(App* app, Panel_ID panel_id)
#define custom_panel_is_leaf_sig() b32 custom_panel_is_leaf(App* app, Panel_ID panel_id)
#define custom_panel_split_sig() b32 custom_panel_split(App* app, Panel_ID panel_id, Dimension split_dim)
#define custom_panel_set_split_sig() b32 custom_panel_set_split(App* app, Panel_ID panel_id, Panel_Split_Kind kind, f32 t)
#define custom_panel_swap_children_sig() b32 custom_panel_swap_children(App* app, Panel_ID panel_id)
#define custom_panel_get_root_sig() Panel_ID custom_panel_get_root(App* app)
#define custom_panel_get_parent_sig() Panel_ID custom_panel_get_parent(App* app, Panel_ID panel_id)
#define custom_panel_get_child_sig() Panel_ID custom_panel_get_child(App* app, Panel_ID panel_id, Side which_child)
#define custom_view_close_sig() b32 custom_view_close(App* app, View_ID view_id)
#define custom_view_get_buffer_region_sig() Rect_f32 custom_view_get_buffer_region(App* app, View_ID view_id)
#define custom_view_get_buffer_scroll_sig() Buffer_Scroll custom_view_get_buffer_scroll(App* app, View_ID view_id)
#define custom_view_set_active_sig() b32 custom_view_set_active(App* app, View_ID view_id)
#define custom_view_enqueue_command_function_sig() b32 custom_view_enqueue_command_function(App* app, View_ID view_id, Custom_Command_Function* custom_func)
#define custom_view_get_setting_sig() b32 custom_view_get_setting(App* app, View_ID view_id, View_Setting_ID setting, i64* value_out)
#define custom_view_set_setting_sig() b32 custom_view_set_setting(App* app, View_ID view_id, View_Setting_ID setting, i64 value)
#define custom_view_get_managed_scope_sig() Managed_Scope custom_view_get_managed_scope(App* app, View_ID view_id)
#define custom_buffer_compute_cursor_sig() Buffer_Cursor custom_buffer_compute_cursor(App* app, Buffer_ID buffer, Buffer_Seek seek)
#define custom_view_compute_cursor_sig() Buffer_Cursor custom_view_compute_cursor(App* app, View_ID view_id, Buffer_Seek seek)
#define custom_view_set_camera_bounds_sig() b32 custom_view_set_camera_bounds(App* app, View_ID view_id, Vec2_f32 margin, Vec2_f32 push_in_multiplier)
#define custom_view_get_camera_bounds_sig() b32 custom_view_get_camera_bounds(App* app, View_ID view_id, Vec2_f32* margin, Vec2_f32* push_in_multiplier)
#define custom_view_set_cursor_sig() b32 custom_view_set_cursor(App* app, View_ID view_id, Buffer_Seek seek)
#define custom_view_set_buffer_scroll_sig() b32 custom_view_set_buffer_scroll(App* app, View_ID view_id, Buffer_Scroll scroll, Set_Buffer_Scroll_Rule rule)
#define custom_view_set_mark_sig() b32 custom_view_set_mark(App* app, View_ID view_id, Buffer_Seek seek)
#define custom_view_quit_ui_sig() b32 custom_view_quit_ui(App* app, View_ID view_id)
#define custom_view_set_buffer_sig() b32 custom_view_set_buffer(App* app, View_ID view_id, Buffer_ID buffer_id, Set_Buffer_Flag flags)
#define custom_view_push_context_sig() b32 custom_view_push_context(App* app, View_ID view_id, View_Context* ctx)
#define custom_view_pop_context_sig() b32 custom_view_pop_context(App* app, View_ID view_id)
#define custom_view_alter_context_sig() b32 custom_view_alter_context(App* app, View_ID view_id, View_Context* ctx)
#define custom_view_current_context_sig() View_Context custom_view_current_context(App* app, View_ID view_id)
#define custom_view_current_context_hook_memory_sig() String custom_view_current_context_hook_memory(App* app, View_ID view_id, Hook_ID hook_id)
#define custom_create_user_managed_scope_sig() Managed_Scope custom_create_user_managed_scope(App* app)
#define custom_destroy_user_managed_scope_sig() b32 custom_destroy_user_managed_scope(App* app, Managed_Scope scope)
#define custom_get_global_managed_scope_sig() Managed_Scope custom_get_global_managed_scope(App* app)
#define custom_get_managed_scope_with_multiple_dependencies_sig() Managed_Scope custom_get_managed_scope_with_multiple_dependencies(App* app, Managed_Scope* scopes, i1 count)
#define custom_managed_scope_clear_contents_sig() b32 custom_managed_scope_clear_contents(App* app, Managed_Scope scope)
#define custom_managed_scope_clear_self_all_dependent_scopes_sig() b32 custom_managed_scope_clear_self_all_dependent_scopes(App* app, Managed_Scope scope)
#define custom_managed_scope_allocator_sig() Base_Allocator* custom_managed_scope_allocator(App* app, Managed_Scope scope)
#define custom_managed_id_group_highest_id_sig() u64 custom_managed_id_group_highest_id(App* app, String group)
#define custom_managed_id_declare_sig() Managed_ID custom_managed_id_declare(App* app, String group, String name)
#define custom_managed_id_get_sig() Managed_ID custom_managed_id_get(App* app, String group, String name)
#define custom_managed_scope_get_attachment_sig() void* custom_managed_scope_get_attachment(App* app, Managed_Scope scope, Managed_ID id, u64 size)
#define custom_managed_scope_attachment_erase_sig() b32 custom_managed_scope_attachment_erase(App* app, Managed_Scope scope, Managed_ID id)
#define custom_alloc_managed_memory_in_scope_sig() Managed_Object custom_alloc_managed_memory_in_scope(App* app, Managed_Scope scope, i1 item_size, i1 count)
#define custom_alloc_buffer_markers_on_buffer_sig() Managed_Object custom_alloc_buffer_markers_on_buffer(App* app, Buffer_ID buffer_id, i1 count, Managed_Scope* optional_extra_scope)
#define custom_managed_object_get_item_size_sig() u32 custom_managed_object_get_item_size(App* app, Managed_Object object)
#define custom_managed_object_get_item_count_sig() u32 custom_managed_object_get_item_count(App* app, Managed_Object object)
#define custom_managed_object_get_pointer_sig() void* custom_managed_object_get_pointer(App* app, Managed_Object object)
#define custom_managed_object_get_type_sig() Managed_Object_Type custom_managed_object_get_type(App* app, Managed_Object object)
#define custom_managed_object_get_containing_scope_sig() Managed_Scope custom_managed_object_get_containing_scope(App* app, Managed_Object object)
#define custom_managed_object_free_sig() b32 custom_managed_object_free(App* app, Managed_Object object)
#define custom_managed_object_store_data_sig() b32 custom_managed_object_store_data(App* app, Managed_Object object, u32 first_index, u32 count, void* mem)
#define custom_managed_object_load_data_sig() b32 custom_managed_object_load_data(App* app, Managed_Object object, u32 first_index, u32 count, void* mem_out)
#define custom_get_next_input_raw_sig() User_Input custom_get_next_input_raw(App* app)
#define custom_get_current_input_sequence_number_sig() i64 custom_get_current_input_sequence_number(App* app)
#define custom_get_current_input_sig() User_Input custom_get_current_input(App* app)
#define custom_set_current_input_sig() void custom_set_current_input(App* app, User_Input* input)
#define custom_leave_current_input_unhandled_sig() void custom_leave_current_input_unhandled(App* app)
#define custom_set_custom_hook_sig() void custom_set_custom_hook(App* app, Hook_ID hook_id, Void_Func* func_ptr)
#define custom_get_custom_hook_sig() Void_Func* custom_get_custom_hook(App* app, Hook_ID hook_id)
#define custom_set_custom_hook_memory_size_sig() b32 custom_set_custom_hook_memory_size(App* app, Hook_ID hook_id, u64 size)
#define custom_get_mouse_state_sig() Mouse_State custom_get_mouse_state(App* app)
#define custom_get_active_query_bars_sig() b32 custom_get_active_query_bars(App* app, View_ID view_id, i1 max_result_count, Query_Bar_Ptr_Array* array_out)
#define custom_start_query_bar_sig() b32 custom_start_query_bar(App* app, Query_Bar* bar, u32 flags)
#define custom_end_query_bar_sig() void custom_end_query_bar(App* app, Query_Bar* bar, u32 flags)
#define custom_clear_all_query_bars_sig() void custom_clear_all_query_bars(App* app, View_ID view_id)
#define custom_print_message_sig() void custom_print_message(App* app, String message)
#define custom_log_string_sig() b32 custom_log_string(App* app, String str)
#define custom_get_largest_face_id_sig() Face_ID custom_get_largest_face_id(App* app)
#define custom_set_global_face_sig() b32 custom_set_global_face(App* app, Face_ID id)
#define custom_buffer_history_get_max_record_index_sig() History_Record_Index custom_buffer_history_get_max_record_index(App* app, Buffer_ID buffer_id)
#define custom_buffer_history_get_record_info_sig() Record_Info custom_buffer_history_get_record_info(App* app, Buffer_ID buffer_id, History_Record_Index index)
#define custom_buffer_history_get_group_sub_record_sig() Record_Info custom_buffer_history_get_group_sub_record(App* app, Buffer_ID buffer_id, History_Record_Index index, i1 sub_index)
#define custom_buffer_history_get_current_state_index_sig() History_Record_Index custom_buffer_history_get_current_state_index(App* app, Buffer_ID buffer_id)
#define custom_buffer_history_set_current_state_index_sig() b32 custom_buffer_history_set_current_state_index(App* app, Buffer_ID buffer_id, History_Record_Index index)
#define custom_buffer_history_merge_record_range_sig() b32 custom_buffer_history_merge_record_range(App* app, Buffer_ID buffer_id, History_Record_Index first_index, History_Record_Index last_index, Record_Merge_Flag flags)
#define custom_buffer_history_clear_after_current_state_sig() b32 custom_buffer_history_clear_after_current_state(App* app, Buffer_ID buffer_id)
#define custom_global_history_edit_group_begin_sig() void custom_global_history_edit_group_begin(App* app)
#define custom_global_history_edit_group_end_sig() void custom_global_history_edit_group_end(App* app)
#define custom_buffer_set_face_sig() b32 custom_buffer_set_face(App* app, Buffer_ID buffer_id, Face_ID id)
#define custom_get_face_description_sig() Face_Description custom_get_face_description(App* app, Face_ID face_id)
#define custom_get_face_metrics_sig() Face_Metrics custom_get_face_metrics(App* app, Face_ID face_id)
#define custom_get_face_advance_map_sig() Face_Advance_Map custom_get_face_advance_map(App* app, Face_ID face_id)
#define custom_get_face_id_sig() Face_ID custom_get_face_id(App* app, Buffer_ID buffer_id)
#define custom_try_create_new_face_sig() Face_ID custom_try_create_new_face(App* app, Face_Description* description)
#define custom_try_modify_face_sig() b32 custom_try_modify_face(App* app, Face_ID id, Face_Description* description)
#define custom_try_release_face_sig() b32 custom_try_release_face(App* app, Face_ID id, Face_ID replacement_id)
#define custom_push_hot_directory_sig() String custom_push_hot_directory(App* app, Arena* arena)
#define custom_set_hot_directory_sig() void custom_set_hot_directory(App* app, String string)
#define custom_send_exit_signal_sig() void custom_send_exit_signal(App* app)
#define custom_hard_exit_sig() void custom_hard_exit(App* app)
#define custom_set_window_title_sig() void custom_set_window_title(App* app, String title)
#define custom_acquire_global_frame_mutex_sig() void custom_acquire_global_frame_mutex(App* app)
#define custom_release_global_frame_mutex_sig() void custom_release_global_frame_mutex(App* app)
#define custom_draw_string_oriented_sig() Vec2_f32 custom_draw_string_oriented(App* app, Face_ID font_id, ARGB_Color color, String str, Vec2_f32 point, u32 flags, Vec2_f32 delta)
#define custom_get_string_advance_sig() f32 custom_get_string_advance(App* app, Face_ID font_id, String str)
#define custom_draw_rect_sig() void custom_draw_rect(App* app, Rect_f32 rect, f32 roundness, ARGB_Color color)
#define custom_draw_rect_outline_sig() void custom_draw_rect_outline(App* app, Rect_f32 rect, f32 roundness, f32 thickness, ARGB_Color color)
#define custom_draw_set_clip_sig() Rect_f32 custom_draw_set_clip(App* app, Rect_f32 new_clip)
#define custom_text_layout_create_sig() Text_Layout_ID custom_text_layout_create(App* app, Buffer_ID buffer_id, Rect_f32 rect, Buffer_Point buffer_point)
#define custom_text_layout_region_sig() Rect_f32 custom_text_layout_region(App* app, Text_Layout_ID text_layout_id)
#define custom_text_layout_get_buffer_sig() Buffer_ID custom_text_layout_get_buffer(App* app, Text_Layout_ID text_layout_id)
#define custom_text_layout_get_visible_range_sig() Range_i64 custom_text_layout_get_visible_range(App* app, Text_Layout_ID text_layout_id)
#define custom_text_layout_line_on_screen_sig() Range_f32 custom_text_layout_line_on_screen(App* app, Text_Layout_ID layout_id, i64 line_number)
#define custom_text_layout_character_on_screen_sig() Rect_f32 custom_text_layout_character_on_screen(App* app, Text_Layout_ID layout_id, i64 pos)
#define custom_paint_text_color_sig() void custom_paint_text_color(App* app, Text_Layout_ID layout_id, Range_i64 range, ARGB_Color color)
#define custom_paint_text_color_blend_sig() void custom_paint_text_color_blend(App* app, Text_Layout_ID layout_id, Range_i64 range, ARGB_Color color, f32 blend)
#define custom_text_layout_free_sig() b32 custom_text_layout_free(App* app, Text_Layout_ID text_layout_id)
#define custom_draw_text_layout_sig() void custom_draw_text_layout(App* app, Text_Layout_ID layout_id, ARGB_Color special_color, ARGB_Color ghost_color)
#define custom_open_color_picker_sig() void custom_open_color_picker(App* app, Color_Picker* picker)
#define custom_animate_in_n_milliseconds_sig() void custom_animate_in_n_milliseconds(App* app, u32 n)
#define custom_buffer_find_all_matches_sig() String_Match_List custom_buffer_find_all_matches(App* app, Arena* arena, Buffer_ID buffer, i1 string_id, Range_i64 range, String needle, Character_Predicate* predicate, Scan_Direction direction)
#define custom_get_core_profile_list_sig() Profile_Global_List* custom_get_core_profile_list(App* app)
#define custom_get_custom_layer_boundary_docs_sig() Doc_Cluster* custom_get_custom_layer_boundary_docs(App* app, Arena* arena)
typedef b32 custom_global_set_setting_type(App* app, Global_Setting_ID setting, i64 value);
typedef Rect_f32 custom_global_get_screen_rectangle_type(App* app);
typedef Thread_Context* custom_get_thread_context_type(App* app);
typedef Child_Process_ID custom_create_child_process_type(App* app, String path, String command);
typedef b32 custom_child_process_set_target_buffer_type(App* app, Child_Process_ID child_process_id, Buffer_ID buffer_id, Child_Process_Set_Target_Flags flags);
typedef Child_Process_ID custom_buffer_get_attached_child_process_type(App* app, Buffer_ID buffer_id);
typedef Buffer_ID custom_child_process_get_attached_buffer_type(App* app, Child_Process_ID child_process_id);
typedef Process_State custom_child_process_get_state_type(App* app, Child_Process_ID child_process_id);
typedef b32 custom_enqueue_virtual_event_type(App* app, Input_Event* event);
typedef i1 custom_get_buffer_count_type(App* app);
typedef Buffer_ID custom_get_buffer_next_type(App* app, Buffer_ID buffer_id, Access_Flag access);
typedef Buffer_ID custom_get_buffer_by_name_type(App* app, String name, Access_Flag access);
typedef Buffer_ID custom_get_buffer_by_filename_type(App* app, String filename, Access_Flag access);
typedef b32 custom_buffer_read_range_type(App* app, Buffer_ID buffer_id, Range_i64 range, u8* out);
typedef b32 custom_buffer_replace_range_type(App* app, Buffer_ID buffer_id, Range_i64 range, String string);
typedef b32 custom_buffer_batch_edit_type(App* app, Buffer_ID buffer_id, Batch_Edit* batch);
typedef String_Match custom_buffer_seek_string_type(App* app, Buffer_ID buffer, String needle, Scan_Direction direction, i64 start_pos);
typedef String_Match custom_buffer_seek_character_class_type(App* app, Buffer_ID buffer, Character_Predicate* predicate, Scan_Direction direction, i64 start_pos);
typedef f32 custom_buffer_line_y_difference_type(App* app, Buffer_ID buffer_id, f32 width, Face_ID face_id, i64 line_a, i64 line_b);
typedef Line_Shift_Vertical custom_buffer_line_shift_y_type(App* app, Buffer_ID buffer_id, f32 width, Face_ID face_id, i64 line, f32 y_shift);
typedef i64 custom_buffer_pos_at_relative_xy_type(App* app, Buffer_ID buffer_id, f32 width, Face_ID face_id, i64 base_line, Vec2_f32 relative_xy);
typedef Rect_f32 custom_buffer_relative_box_of_pos_type(App* app, Buffer_ID buffer_id, f32 width, Face_ID face_id, i64 base_line, i64 pos);
typedef Rect_f32 custom_buffer_padded_box_of_pos_type(App* app, Buffer_ID buffer_id, f32 width, Face_ID face_id, i64 base_line, i64 pos);
typedef i64 custom_buffer_relative_character_from_pos_type(App* app, Buffer_ID buffer_id, f32 width, Face_ID face_id, i64 base_line, i64 pos);
typedef i64 custom_buffer_pos_from_relative_character_type(App* app, Buffer_ID buffer_id, f32 width, Face_ID face_id, i64 base_line, i64 relative_character);
typedef f32 custom_view_line_y_difference_type(App* app, View_ID view_id, i64 line_a, i64 line_b);
typedef Line_Shift_Vertical custom_view_line_shift_y_type(App* app, View_ID view_id, i64 line, f32 y_shift);
typedef i64 custom_view_pos_at_relative_xy_type(App* app, View_ID view_id, i64 base_line, Vec2_f32 relative_xy);
typedef Rect_f32 custom_view_relative_box_of_pos_type(App* app, View_ID view_id, i64 base_line, i64 pos);
typedef Rect_f32 custom_view_padded_box_of_pos_type(App* app, View_ID view_id, i64 base_line, i64 pos);
typedef i64 custom_view_relative_character_from_pos_type(App* app, View_ID view_id, i64 base_line, i64 pos);
typedef i64 custom_view_pos_from_relative_character_type(App* app, View_ID view_id, i64 base_line, i64 character);
typedef b32 custom_buffer_exists_type(App* app, Buffer_ID buffer_id);
typedef Access_Flag custom_buffer_get_access_flags_type(App* app, Buffer_ID buffer_id);
typedef i64 custom_buffer_get_size_type(App* app, Buffer_ID buffer_id);
typedef i64 custom_buffer_get_line_count_type(App* app, Buffer_ID buffer_id);
typedef String custom_push_buffer_base_name_type(App* app, Arena* arena, Buffer_ID buffer_id);
typedef String custom_push_buffer_unique_name_type(App* app, Arena* out, Buffer_ID buffer_id);
typedef String custom_push_buffer_filename_type(App* app, Arena* arena, Buffer_ID buffer_id);
typedef Dirty_State custom_buffer_get_dirty_state_type(App* app, Buffer_ID buffer_id);
typedef b32 custom_buffer_set_dirty_state_type(App* app, Buffer_ID buffer_id, Dirty_State dirty_state);
typedef b32 custom_buffer_set_layout_type(App* app, Buffer_ID buffer_id, Layout_Function* layout_func);
typedef b32 custom_buffer_clear_layout_cache_type(App* app, Buffer_ID buffer_id);
typedef Layout_Function* custom_buffer_get_layout_type(App* app, Buffer_ID buffer_id);
typedef b32 custom_buffer_get_setting_type(App* app, Buffer_ID buffer_id, Buffer_Setting_ID setting, i64* value_out);
typedef b32 custom_buffer_set_setting_type(App* app, Buffer_ID buffer_id, Buffer_Setting_ID setting, i64 value);
typedef Managed_Scope custom_buffer_get_managed_scope_type(App* app, Buffer_ID buffer_id);
typedef b32 custom_buffer_send_end_signal_type(App* app, Buffer_ID buffer_id);
typedef Buffer_ID custom_create_buffer_type(App* app, String filename, Buffer_Create_Flag flags);
typedef b32 custom_buffer_save_type(App* app, Buffer_ID buffer_id, String filename, Buffer_Save_Flag flags);
typedef Buffer_Kill_Result custom_buffer_kill_type(App* app, Buffer_ID buffer_id, Buffer_Kill_Flag flags);
typedef Buffer_Reopen_Result custom_buffer_reopen_type(App* app, Buffer_ID buffer_id, Buffer_Reopen_Flag flags);
typedef File_Attributes custom_buffer_get_file_attributes_type(App* app, Buffer_ID buffer_id);
typedef View_ID custom_get_view_next_type(App* app, View_ID view_id, Access_Flag access);
typedef View_ID custom_get_view_prev_type(App* app, View_ID view_id, Access_Flag access);
typedef View_ID custom_get_this_ctx_view_type(App* app, Access_Flag access);
typedef View_ID custom_get_active_view_type(App* app, Access_Flag access);
typedef b32 custom_view_exists_type(App* app, View_ID view_id);
typedef Buffer_ID custom_view_get_buffer_type(App* app, View_ID view_id, Access_Flag access);
typedef i64 custom_view_get_cursor_pos_type(App* app, View_ID view_id);
typedef i64 custom_view_get_mark_pos_type(App* app, View_ID view_id);
typedef f32 custom_view_get_preferred_x_type(App* app, View_ID view_id);
typedef b32 custom_view_set_preferred_x_type(App* app, View_ID view_id, f32 x);
typedef Rect_f32 custom_view_get_screen_rect_type(App* app, View_ID view_id);
typedef Panel_ID custom_view_get_panel_type(App* app, View_ID view_id);
typedef View_ID custom_panel_get_view_type(App* app, Panel_ID panel_id, Access_Flag access);
typedef b32 custom_panel_is_split_type(App* app, Panel_ID panel_id);
typedef b32 custom_panel_is_leaf_type(App* app, Panel_ID panel_id);
typedef b32 custom_panel_split_type(App* app, Panel_ID panel_id, Dimension split_dim);
typedef b32 custom_panel_set_split_type(App* app, Panel_ID panel_id, Panel_Split_Kind kind, f32 t);
typedef b32 custom_panel_swap_children_type(App* app, Panel_ID panel_id);
typedef Panel_ID custom_panel_get_root_type(App* app);
typedef Panel_ID custom_panel_get_parent_type(App* app, Panel_ID panel_id);
typedef Panel_ID custom_panel_get_child_type(App* app, Panel_ID panel_id, Side which_child);
typedef b32 custom_view_close_type(App* app, View_ID view_id);
typedef Rect_f32 custom_view_get_buffer_region_type(App* app, View_ID view_id);
typedef Buffer_Scroll custom_view_get_buffer_scroll_type(App* app, View_ID view_id);
typedef b32 custom_view_set_active_type(App* app, View_ID view_id);
typedef b32 custom_view_enqueue_command_function_type(App* app, View_ID view_id, Custom_Command_Function* custom_func);
typedef b32 custom_view_get_setting_type(App* app, View_ID view_id, View_Setting_ID setting, i64* value_out);
typedef b32 custom_view_set_setting_type(App* app, View_ID view_id, View_Setting_ID setting, i64 value);
typedef Managed_Scope custom_view_get_managed_scope_type(App* app, View_ID view_id);
typedef Buffer_Cursor custom_buffer_compute_cursor_type(App* app, Buffer_ID buffer, Buffer_Seek seek);
typedef Buffer_Cursor custom_view_compute_cursor_type(App* app, View_ID view_id, Buffer_Seek seek);
typedef b32 custom_view_set_camera_bounds_type(App* app, View_ID view_id, Vec2_f32 margin, Vec2_f32 push_in_multiplier);
typedef b32 custom_view_get_camera_bounds_type(App* app, View_ID view_id, Vec2_f32* margin, Vec2_f32* push_in_multiplier);
typedef b32 custom_view_set_cursor_type(App* app, View_ID view_id, Buffer_Seek seek);
typedef b32 custom_view_set_buffer_scroll_type(App* app, View_ID view_id, Buffer_Scroll scroll, Set_Buffer_Scroll_Rule rule);
typedef b32 custom_view_set_mark_type(App* app, View_ID view_id, Buffer_Seek seek);
typedef b32 custom_view_quit_ui_type(App* app, View_ID view_id);
typedef b32 custom_view_set_buffer_type(App* app, View_ID view_id, Buffer_ID buffer_id, Set_Buffer_Flag flags);
typedef b32 custom_view_push_context_type(App* app, View_ID view_id, View_Context* ctx);
typedef b32 custom_view_pop_context_type(App* app, View_ID view_id);
typedef b32 custom_view_alter_context_type(App* app, View_ID view_id, View_Context* ctx);
typedef View_Context custom_view_current_context_type(App* app, View_ID view_id);
typedef String custom_view_current_context_hook_memory_type(App* app, View_ID view_id, Hook_ID hook_id);
typedef Managed_Scope custom_create_user_managed_scope_type(App* app);
typedef b32 custom_destroy_user_managed_scope_type(App* app, Managed_Scope scope);
typedef Managed_Scope custom_get_global_managed_scope_type(App* app);
typedef Managed_Scope custom_get_managed_scope_with_multiple_dependencies_type(App* app, Managed_Scope* scopes, i1 count);
typedef b32 custom_managed_scope_clear_contents_type(App* app, Managed_Scope scope);
typedef b32 custom_managed_scope_clear_self_all_dependent_scopes_type(App* app, Managed_Scope scope);
typedef Base_Allocator* custom_managed_scope_allocator_type(App* app, Managed_Scope scope);
typedef u64 custom_managed_id_group_highest_id_type(App* app, String group);
typedef Managed_ID custom_managed_id_declare_type(App* app, String group, String name);
typedef Managed_ID custom_managed_id_get_type(App* app, String group, String name);
typedef void* custom_managed_scope_get_attachment_type(App* app, Managed_Scope scope, Managed_ID id, u64 size);
typedef b32 custom_managed_scope_attachment_erase_type(App* app, Managed_Scope scope, Managed_ID id);
typedef Managed_Object custom_alloc_managed_memory_in_scope_type(App* app, Managed_Scope scope, i1 item_size, i1 count);
typedef Managed_Object custom_alloc_buffer_markers_on_buffer_type(App* app, Buffer_ID buffer_id, i1 count, Managed_Scope* optional_extra_scope);
typedef u32 custom_managed_object_get_item_size_type(App* app, Managed_Object object);
typedef u32 custom_managed_object_get_item_count_type(App* app, Managed_Object object);
typedef void* custom_managed_object_get_pointer_type(App* app, Managed_Object object);
typedef Managed_Object_Type custom_managed_object_get_type_type(App* app, Managed_Object object);
typedef Managed_Scope custom_managed_object_get_containing_scope_type(App* app, Managed_Object object);
typedef b32 custom_managed_object_free_type(App* app, Managed_Object object);
typedef b32 custom_managed_object_store_data_type(App* app, Managed_Object object, u32 first_index, u32 count, void* mem);
typedef b32 custom_managed_object_load_data_type(App* app, Managed_Object object, u32 first_index, u32 count, void* mem_out);
typedef User_Input custom_get_next_input_raw_type(App* app);
typedef i64 custom_get_current_input_sequence_number_type(App* app);
typedef User_Input custom_get_current_input_type(App* app);
typedef void custom_set_current_input_type(App* app, User_Input* input);
typedef void custom_leave_current_input_unhandled_type(App* app);
typedef void custom_set_custom_hook_type(App* app, Hook_ID hook_id, Void_Func* func_ptr);
typedef Void_Func* custom_get_custom_hook_type(App* app, Hook_ID hook_id);
typedef b32 custom_set_custom_hook_memory_size_type(App* app, Hook_ID hook_id, u64 size);
typedef Mouse_State custom_get_mouse_state_type(App* app);
typedef b32 custom_get_active_query_bars_type(App* app, View_ID view_id, i1 max_result_count, Query_Bar_Ptr_Array* array_out);
typedef b32 custom_start_query_bar_type(App* app, Query_Bar* bar, u32 flags);
typedef void custom_end_query_bar_type(App* app, Query_Bar* bar, u32 flags);
typedef void custom_clear_all_query_bars_type(App* app, View_ID view_id);
typedef void custom_print_message_type(App* app, String message);
typedef b32 custom_log_string_type(App* app, String str);
typedef Face_ID custom_get_largest_face_id_type(App* app);
typedef b32 custom_set_global_face_type(App* app, Face_ID id);
typedef History_Record_Index custom_buffer_history_get_max_record_index_type(App* app, Buffer_ID buffer_id);
typedef Record_Info custom_buffer_history_get_record_info_type(App* app, Buffer_ID buffer_id, History_Record_Index index);
typedef Record_Info custom_buffer_history_get_group_sub_record_type(App* app, Buffer_ID buffer_id, History_Record_Index index, i1 sub_index);
typedef History_Record_Index custom_buffer_history_get_current_state_index_type(App* app, Buffer_ID buffer_id);
typedef b32 custom_buffer_history_set_current_state_index_type(App* app, Buffer_ID buffer_id, History_Record_Index index);
typedef b32 custom_buffer_history_merge_record_range_type(App* app, Buffer_ID buffer_id, History_Record_Index first_index, History_Record_Index last_index, Record_Merge_Flag flags);
typedef b32 custom_buffer_history_clear_after_current_state_type(App* app, Buffer_ID buffer_id);
typedef void custom_global_history_edit_group_begin_type(App* app);
typedef void custom_global_history_edit_group_end_type(App* app);
typedef b32 custom_buffer_set_face_type(App* app, Buffer_ID buffer_id, Face_ID id);
typedef Face_Description custom_get_face_description_type(App* app, Face_ID face_id);
typedef Face_Metrics custom_get_face_metrics_type(App* app, Face_ID face_id);
typedef Face_Advance_Map custom_get_face_advance_map_type(App* app, Face_ID face_id);
typedef Face_ID custom_get_face_id_type(App* app, Buffer_ID buffer_id);
typedef Face_ID custom_try_create_new_face_type(App* app, Face_Description* description);
typedef b32 custom_try_modify_face_type(App* app, Face_ID id, Face_Description* description);
typedef b32 custom_try_release_face_type(App* app, Face_ID id, Face_ID replacement_id);
typedef String custom_push_hot_directory_type(App* app, Arena* arena);
typedef void custom_set_hot_directory_type(App* app, String string);
typedef void custom_send_exit_signal_type(App* app);
typedef void custom_hard_exit_type(App* app);
typedef void custom_set_window_title_type(App* app, String title);
typedef void custom_acquire_global_frame_mutex_type(App* app);
typedef void custom_release_global_frame_mutex_type(App* app);
typedef Vec2_f32 custom_draw_string_oriented_type(App* app, Face_ID font_id, ARGB_Color color, String str, Vec2_f32 point, u32 flags, Vec2_f32 delta);
typedef f32 custom_get_string_advance_type(App* app, Face_ID font_id, String str);
typedef void custom_draw_rect_type(App* app, Rect_f32 rect, f32 roundness, ARGB_Color color);
typedef void custom_draw_rect_outline_type(App* app, Rect_f32 rect, f32 roundness, f32 thickness, ARGB_Color color);
typedef Rect_f32 custom_draw_set_clip_type(App* app, Rect_f32 new_clip);
typedef Text_Layout_ID custom_text_layout_create_type(App* app, Buffer_ID buffer_id, Rect_f32 rect, Buffer_Point buffer_point);
typedef Rect_f32 custom_text_layout_region_type(App* app, Text_Layout_ID text_layout_id);
typedef Buffer_ID custom_text_layout_get_buffer_type(App* app, Text_Layout_ID text_layout_id);
typedef Range_i64 custom_text_layout_get_visible_range_type(App* app, Text_Layout_ID text_layout_id);
typedef Range_f32 custom_text_layout_line_on_screen_type(App* app, Text_Layout_ID layout_id, i64 line_number);
typedef Rect_f32 custom_text_layout_character_on_screen_type(App* app, Text_Layout_ID layout_id, i64 pos);
typedef void custom_paint_text_color_type(App* app, Text_Layout_ID layout_id, Range_i64 range, ARGB_Color color);
typedef void custom_paint_text_color_blend_type(App* app, Text_Layout_ID layout_id, Range_i64 range, ARGB_Color color, f32 blend);
typedef b32 custom_text_layout_free_type(App* app, Text_Layout_ID text_layout_id);
typedef void custom_draw_text_layout_type(App* app, Text_Layout_ID layout_id, ARGB_Color special_color, ARGB_Color ghost_color);
typedef void custom_open_color_picker_type(App* app, Color_Picker* picker);
typedef void custom_animate_in_n_milliseconds_type(App* app, u32 n);
typedef String_Match_List custom_buffer_find_all_matches_type(App* app, Arena* arena, Buffer_ID buffer, i1 string_id, Range_i64 range, String needle, Character_Predicate* predicate, Scan_Direction direction);
typedef Profile_Global_List* custom_get_core_profile_list_type(App* app);
typedef Doc_Cluster* custom_get_custom_layer_boundary_docs_type(App* app, Arena* arena);
struct API_VTable_custom{
custom_global_set_setting_type *global_set_setting;
custom_global_get_screen_rectangle_type *global_get_screen_rectangle;
custom_get_thread_context_type *get_thread_context;
custom_create_child_process_type *create_child_process;
custom_child_process_set_target_buffer_type *child_process_set_target_buffer;
custom_buffer_get_attached_child_process_type *buffer_get_attached_child_process;
custom_child_process_get_attached_buffer_type *child_process_get_attached_buffer;
custom_child_process_get_state_type *child_process_get_state;
custom_enqueue_virtual_event_type *enqueue_virtual_event;
custom_get_buffer_count_type *get_buffer_count;
custom_get_buffer_next_type *get_buffer_next;
custom_get_buffer_by_name_type *get_buffer_by_name;
custom_get_buffer_by_filename_type *get_buffer_by_filename;
custom_buffer_read_range_type *buffer_read_range;
custom_buffer_replace_range_type *buffer_replace_range;
custom_buffer_batch_edit_type *buffer_batch_edit;
custom_buffer_seek_string_type *buffer_seek_string;
custom_buffer_seek_character_class_type *buffer_seek_character_class;
custom_buffer_line_y_difference_type *buffer_line_y_difference;
custom_buffer_line_shift_y_type *buffer_line_shift_y;
custom_buffer_pos_at_relative_xy_type *buffer_pos_at_relative_xy;
custom_buffer_relative_box_of_pos_type *buffer_relative_box_of_pos;
custom_buffer_padded_box_of_pos_type *buffer_padded_box_of_pos;
custom_buffer_relative_character_from_pos_type *buffer_relative_character_from_pos;
custom_buffer_pos_from_relative_character_type *buffer_pos_from_relative_character;
custom_view_line_y_difference_type *view_line_y_difference;
custom_view_line_shift_y_type *view_line_shift_y;
custom_view_pos_at_relative_xy_type *view_pos_at_relative_xy;
custom_view_relative_box_of_pos_type *view_relative_box_of_pos;
custom_view_padded_box_of_pos_type *view_padded_box_of_pos;
custom_view_relative_character_from_pos_type *view_relative_character_from_pos;
custom_view_pos_from_relative_character_type *view_pos_from_relative_character;
custom_buffer_exists_type *buffer_exists;
custom_buffer_get_access_flags_type *buffer_get_access_flags;
custom_buffer_get_size_type *buffer_get_size;
custom_buffer_get_line_count_type *buffer_get_line_count;
custom_push_buffer_base_name_type *push_buffer_base_name;
custom_push_buffer_unique_name_type *push_buffer_unique_name;
custom_push_buffer_filename_type *push_buffer_filename;
custom_buffer_get_dirty_state_type *buffer_get_dirty_state;
custom_buffer_set_dirty_state_type *buffer_set_dirty_state;
custom_buffer_set_layout_type *buffer_set_layout;
custom_buffer_clear_layout_cache_type *buffer_clear_layout_cache;
custom_buffer_get_layout_type *buffer_get_layout;
custom_buffer_get_setting_type *buffer_get_setting;
custom_buffer_set_setting_type *buffer_set_setting;
custom_buffer_get_managed_scope_type *buffer_get_managed_scope;
custom_buffer_send_end_signal_type *buffer_send_end_signal;
custom_create_buffer_type *create_buffer;
custom_buffer_save_type *buffer_save;
custom_buffer_kill_type *buffer_kill;
custom_buffer_reopen_type *buffer_reopen;
custom_buffer_get_file_attributes_type *buffer_get_file_attributes;
custom_get_view_next_type *get_view_next;
custom_get_view_prev_type *get_view_prev;
custom_get_this_ctx_view_type *get_this_ctx_view;
custom_get_active_view_type *get_active_view;
custom_view_exists_type *view_exists;
custom_view_get_buffer_type *view_get_buffer;
custom_view_get_cursor_pos_type *view_get_cursor_pos;
custom_view_get_mark_pos_type *view_get_mark_pos;
custom_view_get_preferred_x_type *view_get_preferred_x;
custom_view_set_preferred_x_type *view_set_preferred_x;
custom_view_get_screen_rect_type *view_get_screen_rect;
custom_view_get_panel_type *view_get_panel;
custom_panel_get_view_type *panel_get_view;
custom_panel_is_split_type *panel_is_split;
custom_panel_is_leaf_type *panel_is_leaf;
custom_panel_split_type *panel_split;
custom_panel_set_split_type *panel_set_split;
custom_panel_swap_children_type *panel_swap_children;
custom_panel_get_root_type *panel_get_root;
custom_panel_get_parent_type *panel_get_parent;
custom_panel_get_child_type *panel_get_child;
custom_view_close_type *view_close;
custom_view_get_buffer_region_type *view_get_buffer_region;
custom_view_get_buffer_scroll_type *view_get_buffer_scroll;
custom_view_set_active_type *view_set_active;
custom_view_enqueue_command_function_type *view_enqueue_command_function;
custom_view_get_setting_type *view_get_setting;
custom_view_set_setting_type *view_set_setting;
custom_view_get_managed_scope_type *view_get_managed_scope;
custom_buffer_compute_cursor_type *buffer_compute_cursor;
custom_view_compute_cursor_type *view_compute_cursor;
custom_view_set_camera_bounds_type *view_set_camera_bounds;
custom_view_get_camera_bounds_type *view_get_camera_bounds;
custom_view_set_cursor_type *view_set_cursor;
custom_view_set_buffer_scroll_type *view_set_buffer_scroll;
custom_view_set_mark_type *view_set_mark;
custom_view_quit_ui_type *view_quit_ui;
custom_view_set_buffer_type *view_set_buffer;
custom_view_push_context_type *view_push_context;
custom_view_pop_context_type *view_pop_context;
custom_view_alter_context_type *view_alter_context;
custom_view_current_context_type *view_current_context;
custom_view_current_context_hook_memory_type *view_current_context_hook_memory;
custom_create_user_managed_scope_type *create_user_managed_scope;
custom_destroy_user_managed_scope_type *destroy_user_managed_scope;
custom_get_global_managed_scope_type *get_global_managed_scope;
custom_get_managed_scope_with_multiple_dependencies_type *get_managed_scope_with_multiple_dependencies;
custom_managed_scope_clear_contents_type *managed_scope_clear_contents;
custom_managed_scope_clear_self_all_dependent_scopes_type *managed_scope_clear_self_all_dependent_scopes;
custom_managed_scope_allocator_type *managed_scope_allocator;
custom_managed_id_group_highest_id_type *managed_id_group_highest_id;
custom_managed_id_declare_type *managed_id_declare;
custom_managed_id_get_type *managed_id_get;
custom_managed_scope_get_attachment_type *managed_scope_get_attachment;
custom_managed_scope_attachment_erase_type *managed_scope_attachment_erase;
custom_alloc_managed_memory_in_scope_type *alloc_managed_memory_in_scope;
custom_alloc_buffer_markers_on_buffer_type *alloc_buffer_markers_on_buffer;
custom_managed_object_get_item_size_type *managed_object_get_item_size;
custom_managed_object_get_item_count_type *managed_object_get_item_count;
custom_managed_object_get_pointer_type *managed_object_get_pointer;
custom_managed_object_get_type_type *managed_object_get_type;
custom_managed_object_get_containing_scope_type *managed_object_get_containing_scope;
custom_managed_object_free_type *managed_object_free;
custom_managed_object_store_data_type *managed_object_store_data;
custom_managed_object_load_data_type *managed_object_load_data;
custom_get_next_input_raw_type *get_next_input_raw;
custom_get_current_input_sequence_number_type *get_current_input_sequence_number;
custom_get_current_input_type *get_current_input;
custom_set_current_input_type *set_current_input;
custom_leave_current_input_unhandled_type *leave_current_input_unhandled;
custom_set_custom_hook_type *set_custom_hook;
custom_get_custom_hook_type *get_custom_hook;
custom_set_custom_hook_memory_size_type *set_custom_hook_memory_size;
custom_get_mouse_state_type *get_mouse_state;
custom_get_active_query_bars_type *get_active_query_bars;
custom_start_query_bar_type *start_query_bar;
custom_end_query_bar_type *end_query_bar;
custom_clear_all_query_bars_type *clear_all_query_bars;
custom_print_message_type *print_message;
custom_log_string_type *log_string;
custom_get_largest_face_id_type *get_largest_face_id;
custom_set_global_face_type *set_global_face;
custom_buffer_history_get_max_record_index_type *buffer_history_get_max_record_index;
custom_buffer_history_get_record_info_type *buffer_history_get_record_info;
custom_buffer_history_get_group_sub_record_type *buffer_history_get_group_sub_record;
custom_buffer_history_get_current_state_index_type *buffer_history_get_current_state_index;
custom_buffer_history_set_current_state_index_type *buffer_history_set_current_state_index;
custom_buffer_history_merge_record_range_type *buffer_history_merge_record_range;
custom_buffer_history_clear_after_current_state_type *buffer_history_clear_after_current_state;
custom_global_history_edit_group_begin_type *global_history_edit_group_begin;
custom_global_history_edit_group_end_type *global_history_edit_group_end;
custom_buffer_set_face_type *buffer_set_face;
custom_get_face_description_type *get_face_description;
custom_get_face_metrics_type *get_face_metrics;
custom_get_face_advance_map_type *get_face_advance_map;
custom_get_face_id_type *get_face_id;
custom_try_create_new_face_type *try_create_new_face;
custom_try_modify_face_type *try_modify_face;
custom_try_release_face_type *try_release_face;
custom_push_hot_directory_type *push_hot_directory;
custom_set_hot_directory_type *set_hot_directory;
custom_send_exit_signal_type *send_exit_signal;
custom_hard_exit_type *hard_exit;
custom_set_window_title_type *set_window_title;
custom_acquire_global_frame_mutex_type *acquire_global_frame_mutex;
custom_release_global_frame_mutex_type *release_global_frame_mutex;
custom_draw_string_oriented_type *draw_string_oriented;
custom_get_string_advance_type *get_string_advance;
custom_draw_rect_type *draw_rect;
custom_draw_rect_outline_type *draw_rect_outline;
custom_draw_set_clip_type *draw_set_clip;
custom_text_layout_create_type *text_layout_create;
custom_text_layout_region_type *text_layout_region;
custom_text_layout_get_buffer_type *text_layout_get_buffer;
custom_text_layout_get_visible_range_type *text_layout_get_visible_range;
custom_text_layout_line_on_screen_type *text_layout_line_on_screen;
custom_text_layout_character_on_screen_type *text_layout_character_on_screen;
custom_paint_text_color_type *paint_text_color;
custom_paint_text_color_blend_type *paint_text_color_blend;
custom_text_layout_free_type *text_layout_free;
custom_draw_text_layout_type *draw_text_layout;
custom_open_color_picker_type *open_color_picker;
custom_animate_in_n_milliseconds_type *animate_in_n_milliseconds;
custom_buffer_find_all_matches_type *buffer_find_all_matches;
custom_get_core_profile_list_type *get_core_profile_list;
custom_get_custom_layer_boundary_docs_type *get_custom_layer_boundary_docs;
};





#if defined(STATIC_LINK_API) || defined(EXTERNAL_LINK_API)
#    if defined(STATIC_LINK_API)
#       undef   STATIC_LINK_API
#        define LINKAGE internal
#    elif defined(EXTERNAL_LINK_API)
#         undef   EXTERNAL_LINK_API
#        define LINKAGE extern "C"
#    endif


LINKAGE b32 global_set_setting(App* app, Global_Setting_ID setting, i64 value);
LINKAGE Rect_f32 global_get_screen_rectangle(App* app);
LINKAGE Thread_Context* get_thread_context(App* app);
LINKAGE Child_Process_ID create_child_process(App* app, String path, String command);
LINKAGE b32 child_process_set_target_buffer(App* app, Child_Process_ID child_process_id, Buffer_ID buffer_id, Child_Process_Set_Target_Flags flags);
LINKAGE Child_Process_ID buffer_get_attached_child_process(App* app, Buffer_ID buffer_id);
LINKAGE Buffer_ID child_process_get_attached_buffer(App* app, Child_Process_ID child_process_id);
LINKAGE Process_State child_process_get_state(App* app, Child_Process_ID child_process_id);
LINKAGE b32 enqueue_virtual_event(App* app, Input_Event* event);
LINKAGE i1 get_buffer_count(App* app);
LINKAGE Buffer_ID get_buffer_next(App* app, Buffer_ID buffer_id, Access_Flag access);
LINKAGE Buffer_ID get_buffer_by_name(App* app, String name, Access_Flag access);
LINKAGE Buffer_ID get_buffer_by_filename(App* app, String filename, Access_Flag access);
LINKAGE b32 buffer_read_range(App* app, Buffer_ID buffer_id, Range_i64 range, u8* out);
LINKAGE b32 buffer_replace_range(App* app, Buffer_ID buffer_id, Range_i64 range, String string);
LINKAGE b32 buffer_batch_edit(App* app, Buffer_ID buffer_id, Batch_Edit* batch);
LINKAGE String_Match buffer_seek_string(App* app, Buffer_ID buffer, String needle, Scan_Direction direction, i64 start_pos, b32 case_sensitive);
LINKAGE String_Match buffer_seek_character_class(App* app, Buffer_ID buffer, Character_Predicate* predicate, Scan_Direction direction, i64 start_pos);
LINKAGE f32 buffer_line_y_difference(App* app, Buffer_ID buffer_id, f32 width, Face_ID face_id, i64 line_a, i64 line_b);
LINKAGE Line_Shift_Vertical buffer_line_shift_y(App* app, Buffer_ID buffer_id, f32 width, Face_ID face_id, i64 line, f32 y_shift);
LINKAGE i64 buffer_pos_at_relative_xy(App* app, Buffer_ID buffer_id, f32 width, Face_ID face_id, i64 base_line, Vec2_f32 relative_xy);
LINKAGE Rect_f32 buffer_relative_box_of_pos(App* app, Buffer_ID buffer_id, f32 width, Face_ID face_id, i64 base_line, i64 pos);
LINKAGE Rect_f32 buffer_padded_box_of_pos(App* app, Buffer_ID buffer_id, f32 width, Face_ID face_id, i64 base_line, i64 pos);
LINKAGE i64 buffer_relative_character_from_pos(App* app, Buffer_ID buffer_id, f32 width, Face_ID face_id, i64 base_line, i64 pos);
LINKAGE i64 buffer_pos_from_relative_character(App* app, Buffer_ID buffer_id, f32 width, Face_ID face_id, i64 base_line, i64 relative_character);
LINKAGE f32 view_line_y_difference(App* app, View_ID view_id, i64 line_a, i64 line_b);
LINKAGE Line_Shift_Vertical view_line_shift_y(App* app, View_ID view_id, i64 line, f32 y_shift);
LINKAGE i64 view_pos_at_relative_xy(App* app, View_ID view_id, i64 base_line, Vec2_f32 relative_xy);
LINKAGE Rect_f32 view_relative_box_of_pos(App* app, View_ID view_id, i64 base_line, i64 pos);
LINKAGE Rect_f32 view_padded_box_of_pos(App* app, View_ID view_id, i64 base_line, i64 pos);
LINKAGE i64 view_relative_character_from_pos(App* app, View_ID view_id, i64 base_line, i64 pos);
LINKAGE i64 view_pos_from_relative_character(App* app, View_ID view_id, i64 base_line, i64 character);
LINKAGE b32 buffer_exists(App* app, Buffer_ID buffer_id);
LINKAGE Access_Flag buffer_get_access_flags(App* app, Buffer_ID buffer_id);
LINKAGE i64 buffer_get_size(App* app, Buffer_ID buffer_id);
LINKAGE i64 buffer_get_line_count(App* app, Buffer_ID buffer_id);
LINKAGE String push_buffer_base_name(App* app, Arena* arena, Buffer_ID buffer_id);
LINKAGE String push_buffer_unique_name(App* app, Arena* out, Buffer_ID buffer_id);
LINKAGE String push_buffer_filename(App* app, Arena* arena, Buffer_ID buffer_id);
LINKAGE Dirty_State buffer_get_dirty_state(App* app, Buffer_ID buffer_id);
LINKAGE b32 buffer_set_dirty_state(App* app, Buffer_ID buffer_id, Dirty_State dirty_state);
LINKAGE b32 buffer_set_layout(App* app, Buffer_ID buffer_id, Layout_Function* layout_func);
LINKAGE b32 buffer_clear_layout_cache(App* app, Buffer_ID buffer_id);
LINKAGE Layout_Function* buffer_get_layout(App* app, Buffer_ID buffer_id);
LINKAGE b32 buffer_get_setting(App* app, Buffer_ID buffer_id, Buffer_Setting_ID setting, i64* value_out);
LINKAGE b32 buffer_set_setting(App* app, Buffer_ID buffer_id, Buffer_Setting_ID setting, i64 value);
LINKAGE Managed_Scope buffer_get_managed_scope(App* app, Buffer_ID buffer_id);
LINKAGE b32 buffer_send_end_signal(App* app, Buffer_ID buffer_id);
LINKAGE Buffer_ID create_buffer(App* app, String filename, Buffer_Create_Flag flags);
LINKAGE b32 buffer_save(App* app, Buffer_ID buffer_id, String filename, Buffer_Save_Flag flags);
LINKAGE Buffer_Kill_Result buffer_kill(App* app, Buffer_ID buffer_id, Buffer_Kill_Flag flags);
LINKAGE Buffer_Reopen_Result buffer_reopen(App* app, Buffer_ID buffer_id, Buffer_Reopen_Flag flags);
LINKAGE File_Attributes buffer_get_file_attributes(App* app, Buffer_ID buffer_id);
LINKAGE View_ID get_view_next(App* app, View_ID view_id, Access_Flag access);
LINKAGE View_ID get_view_prev(App* app, View_ID view_id, Access_Flag access);
LINKAGE View_ID get_this_ctx_view(App* app, Access_Flag access);
LINKAGE View_ID get_active_view(App* app, Access_Flag access);
LINKAGE b32 view_exists(App* app, View_ID view_id);
LINKAGE Buffer_ID view_get_buffer(App* app, View_ID view_id, Access_Flag access);
LINKAGE i64 view_get_cursor_pos(App* app, View_ID view_id);
LINKAGE i64 view_get_mark_pos(App* app, View_ID view_id);
LINKAGE f32 view_get_preferred_x(App* app, View_ID view_id);
LINKAGE b32 view_set_preferred_x(App* app, View_ID view_id, f32 x);
LINKAGE Rect_f32 view_get_screen_rect(App* app, View_ID view_id);
LINKAGE Panel_ID view_get_panel(App* app, View_ID view_id);
LINKAGE View_ID panel_get_view(App* app, Panel_ID panel_id, Access_Flag access);
LINKAGE b32 panel_is_split(App* app, Panel_ID panel_id);
LINKAGE b32 panel_is_leaf(App* app, Panel_ID panel_id);
LINKAGE b32 panel_split(App* app, Panel_ID panel_id, Dimension split_dim);
LINKAGE b32 panel_set_split(App* app, Panel_ID panel_id, Panel_Split_Kind kind, f32 t);
LINKAGE b32 panel_swap_children(App* app, Panel_ID panel_id);
LINKAGE Panel_ID panel_get_root(App* app);
LINKAGE Panel_ID panel_get_parent(App* app, Panel_ID panel_id);
LINKAGE Panel_ID panel_get_child(App* app, Panel_ID panel_id, Side which_child);
LINKAGE b32 view_close(App* app, View_ID view_id);
LINKAGE Rect_f32 view_get_buffer_region(App* app, View_ID view_id);
LINKAGE Buffer_Scroll view_get_buffer_scroll(App* app, View_ID view_id);
LINKAGE b32 view_set_active(App* app, View_ID view_id);
LINKAGE b32 view_enqueue_command_function(App* app, View_ID view_id, Custom_Command_Function* custom_func);
LINKAGE b32 view_get_setting(App* app, View_ID view_id, View_Setting_ID setting, i64* value_out);
LINKAGE b32 view_set_setting(App* app, View_ID view_id, View_Setting_ID setting, i64 value);
LINKAGE Managed_Scope view_get_managed_scope(App* app, View_ID view_id);
LINKAGE Buffer_Cursor buffer_compute_cursor(App* app, Buffer_ID buffer, Buffer_Seek seek);
LINKAGE Buffer_Cursor view_compute_cursor(App* app, View_ID view_id, Buffer_Seek seek);
LINKAGE b32 view_set_camera_bounds(App* app, View_ID view_id, Vec2_f32 margin, Vec2_f32 push_in_multiplier);
LINKAGE b32 view_get_camera_bounds(App* app, View_ID view_id, Vec2_f32* margin, Vec2_f32* push_in_multiplier);
LINKAGE b32 view_set_cursor(App* app, View_ID view_id, Buffer_Seek seek);
LINKAGE b32 view_set_buffer_scroll(App* app, View_ID view_id, Buffer_Scroll scroll, Set_Buffer_Scroll_Rule rule);
LINKAGE b32 view_set_mark(App* app, View_ID view_id, Buffer_Seek seek);
LINKAGE b32 view_quit_ui(App* app, View_ID view_id);
LINKAGE b32 view_set_buffer(App* app, View_ID view_id, Buffer_ID buffer_id, Set_Buffer_Flag flags);
LINKAGE b32 view_push_context(App* app, View_ID view_id, View_Context* ctx);
LINKAGE b32 view_pop_context(App* app, View_ID view_id);
LINKAGE b32 view_alter_context(App* app, View_ID view_id, View_Context* ctx);
LINKAGE View_Context view_current_context(App* app, View_ID view_id);
LINKAGE String view_current_context_hook_memory(App* app, View_ID view_id, Hook_ID hook_id);
LINKAGE Managed_Scope create_user_managed_scope(App* app);
LINKAGE b32 destroy_user_managed_scope(App* app, Managed_Scope scope);
LINKAGE Managed_Scope get_global_managed_scope(App* app);
LINKAGE Managed_Scope get_managed_scope_with_multiple_dependencies(App* app, Managed_Scope* scopes, i1 count);
LINKAGE b32 managed_scope_clear_contents(App* app, Managed_Scope scope);
LINKAGE b32 managed_scope_clear_self_all_dependent_scopes(App* app, Managed_Scope scope);
LINKAGE Base_Allocator* managed_scope_allocator(App* app, Managed_Scope scope);
LINKAGE u64 managed_id_group_highest_id(App* app, String group);
LINKAGE Managed_ID managed_id_declare(App* app, String group, String name);
LINKAGE Managed_ID managed_id_get(App* app, String group, String name);
LINKAGE void* managed_scope_get_attachment(App* app, Managed_Scope scope, Managed_ID id, u64 size);
LINKAGE b32 managed_scope_attachment_erase(App* app, Managed_Scope scope, Managed_ID id);
LINKAGE Managed_Object alloc_managed_memory_in_scope(App* app, Managed_Scope scope, i1 item_size, i1 count);
LINKAGE Managed_Object alloc_buffer_markers_on_buffer(App* app, Buffer_ID buffer_id, i1 count, Managed_Scope* optional_extra_scope);
LINKAGE u32 managed_object_get_item_size(App* app, Managed_Object object);
LINKAGE u32 managed_object_get_item_count(App* app, Managed_Object object);
LINKAGE void* managed_object_get_pointer(App* app, Managed_Object object);
LINKAGE Managed_Object_Type managed_object_get_type(App* app, Managed_Object object);
LINKAGE Managed_Scope managed_object_get_containing_scope(App* app, Managed_Object object);
LINKAGE b32 managed_object_free(App* app, Managed_Object object);
LINKAGE b32 managed_object_store_data(App* app, Managed_Object object, u32 first_index, u32 count, void* mem);
LINKAGE b32 managed_object_load_data(App* app, Managed_Object object, u32 first_index, u32 count, void* mem_out);
LINKAGE User_Input get_next_input_raw(App* app);
LINKAGE i64 get_current_input_sequence_number(App* app);
LINKAGE User_Input get_current_input(App* app);
LINKAGE void set_current_input(App* app, User_Input* input);
LINKAGE void leave_current_input_unhandled(App* app);
LINKAGE void set_custom_hook(App* app, Hook_ID hook_id, Void_Func* func_ptr);
LINKAGE Void_Func* get_custom_hook(App* app, Hook_ID hook_id);
LINKAGE b32 set_custom_hook_memory_size(App* app, Hook_ID hook_id, u64 size);
LINKAGE Mouse_State get_mouse_state(App* app);
LINKAGE b32 get_active_query_bars(App* app, View_ID view_id, i1 max_result_count, Query_Bar_Ptr_Array* array_out);
LINKAGE b32 start_query_bar(App* app, Query_Bar* bar, u32 flags);
LINKAGE void end_query_bar(App* app, Query_Bar* bar, u32 flags);
LINKAGE void clear_all_query_bars(App* app, View_ID view_id);
LINKAGE void print_message(App* app, String message);
LINKAGE b32 log_string(App* app, String str);
LINKAGE Face_ID get_largest_face_id(App* app);
LINKAGE b32 set_global_face(App* app, Face_ID id);
LINKAGE History_Record_Index buffer_history_get_max_record_index(App* app, Buffer_ID buffer_id);
LINKAGE Record_Info buffer_history_get_record_info(App* app, Buffer_ID buffer_id, History_Record_Index index);
LINKAGE Record_Info buffer_history_get_group_sub_record(App* app, Buffer_ID buffer_id, History_Record_Index index, i1 sub_index);
LINKAGE History_Record_Index buffer_history_get_current_state_index(App* app, Buffer_ID buffer_id);
LINKAGE b32 buffer_history_set_current_state_index(App* app, Buffer_ID buffer_id, History_Record_Index index);
LINKAGE b32 buffer_history_merge_record_range(App* app, Buffer_ID buffer_id, History_Record_Index first_index, History_Record_Index last_index, Record_Merge_Flag flags);
LINKAGE b32 buffer_history_clear_after_current_state(App* app, Buffer_ID buffer_id);
LINKAGE void global_history_edit_group_begin(App* app);
LINKAGE void global_history_edit_group_end(App* app);
LINKAGE b32 buffer_set_face(App* app, Buffer_ID buffer_id, Face_ID id);
LINKAGE Face_Description get_face_description(App* app, Face_ID face_id);
LINKAGE Face_Metrics get_face_metrics(App* app, Face_ID face_id);
LINKAGE Face_Advance_Map get_face_advance_map(App* app, Face_ID face_id);
LINKAGE Face_ID get_face_id(App* app, Buffer_ID buffer_id);
LINKAGE Face_ID try_create_new_face(App* app, Face_Description* description);
LINKAGE b32 try_modify_face(App* app, Face_ID id, Face_Description* description);
LINKAGE b32 try_release_face(App* app, Face_ID id, Face_ID replacement_id);
LINKAGE String push_hot_directory(App* app, Arena* arena);
LINKAGE void set_hot_directory(App* app, String string);
LINKAGE void send_exit_signal(App* app);
LINKAGE void hard_exit(App* app);
LINKAGE void set_window_title(App* app, String title);
LINKAGE void acquire_global_frame_mutex(App* app);
LINKAGE void release_global_frame_mutex(App* app);
LINKAGE Vec2_f32 draw_string_oriented(App* app, Face_ID font_id, ARGB_Color color, String str, Vec2_f32 point, u32 flags, Vec2_f32 delta);
LINKAGE f32 get_string_advance(App* app, Face_ID font_id, String str);
//LINKAGE void draw_rect(App* app, Rect_f32 rect, f32 roundness, ARGB_Color color);
LINKAGE void draw_rect_outline(draw_rect_outline_params);
LINKAGE Rect_f32 draw_set_clip(App* app, Rect_f32 new_clip);
LINKAGE Text_Layout_ID text_layout_create(App* app, Buffer_ID buffer_id, Rect_f32 rect, Buffer_Point buffer_point);
LINKAGE Rect_f32 text_layout_region(App* app, Text_Layout_ID text_layout_id);
LINKAGE Buffer_ID text_layout_get_buffer(App* app, Text_Layout_ID text_layout_id);
LINKAGE Range_i64 text_layout_get_visible_range(App* app, Text_Layout_ID text_layout_id);
LINKAGE Range_f32 text_layout_line_on_screen(App* app, Text_Layout_ID layout_id, i64 line_number);
LINKAGE Rect_f32 text_layout_character_on_screen(App* app, Text_Layout_ID layout_id, i64 pos);
LINKAGE void paint_text_color(App* app, Text_Layout_ID layout_id, Range_i64 range, ARGB_Color color);
LINKAGE void paint_text_color_blend(App* app, Text_Layout_ID layout_id, Range_i64 range, ARGB_Color color, f32 blend);
LINKAGE b32 text_layout_free(App* app, Text_Layout_ID text_layout_id);
LINKAGE void draw_text_layout(App* app, Text_Layout_ID layout_id, ARGB_Color special_color, ARGB_Color ghost_color);
LINKAGE void open_color_picker(App* app, Color_Picker* picker);
LINKAGE void animate_in_n_milliseconds(App* app, u32 n);
LINKAGE String_Match_List buffer_find_all_matches(App* app, Arena* arena, Buffer_ID buffer, i1 string_id, Range_i64 range, String needle, Character_Predicate* predicate, Scan_Direction direction, b32 case_sensitive);
LINKAGE Profile_Global_List* get_core_profile_list(App* app);
LINKAGE Doc_Cluster* get_custom_layer_boundary_docs(App* app, Arena* arena);

#undef LINKAGE





// NOTE(kv): #unused
#elif defined(DYNAMIC_LINK_API)
global custom_global_set_setting_type *global_set_setting = 0;
global custom_global_get_screen_rectangle_type *global_get_screen_rectangle = 0;
global custom_get_thread_context_type *get_thread_context = 0;
global custom_create_child_process_type *create_child_process = 0;
global custom_child_process_set_target_buffer_type *child_process_set_target_buffer = 0;
global custom_buffer_get_attached_child_process_type *buffer_get_attached_child_process = 0;
global custom_child_process_get_attached_buffer_type *child_process_get_attached_buffer = 0;
global custom_child_process_get_state_type *child_process_get_state = 0;
global custom_enqueue_virtual_event_type *enqueue_virtual_event = 0;
global custom_get_buffer_count_type *get_buffer_count = 0;
global custom_get_buffer_next_type *get_buffer_next = 0;
global custom_get_buffer_by_name_type *get_buffer_by_name = 0;
global custom_get_buffer_by_filename_type *get_buffer_by_filename = 0;
global custom_buffer_read_range_type *buffer_read_range = 0;
global custom_buffer_replace_range_type *buffer_replace_range = 0;
global custom_buffer_batch_edit_type *buffer_batch_edit = 0;
global custom_buffer_seek_string_type *buffer_seek_string = 0;
global custom_buffer_seek_character_class_type *buffer_seek_character_class = 0;
global custom_buffer_line_y_difference_type *buffer_line_y_difference = 0;
global custom_buffer_line_shift_y_type *buffer_line_shift_y = 0;
global custom_buffer_pos_at_relative_xy_type *buffer_pos_at_relative_xy = 0;
global custom_buffer_relative_box_of_pos_type *buffer_relative_box_of_pos = 0;
global custom_buffer_padded_box_of_pos_type *buffer_padded_box_of_pos = 0;
global custom_buffer_relative_character_from_pos_type *buffer_relative_character_from_pos = 0;
global custom_buffer_pos_from_relative_character_type *buffer_pos_from_relative_character = 0;
global custom_view_line_y_difference_type *view_line_y_difference = 0;
global custom_view_line_shift_y_type *view_line_shift_y = 0;
global custom_view_pos_at_relative_xy_type *view_pos_at_relative_xy = 0;
global custom_view_relative_box_of_pos_type *view_relative_box_of_pos = 0;
global custom_view_padded_box_of_pos_type *view_padded_box_of_pos = 0;
global custom_view_relative_character_from_pos_type *view_relative_character_from_pos = 0;
global custom_view_pos_from_relative_character_type *view_pos_from_relative_character = 0;
global custom_buffer_exists_type *buffer_exists = 0;
global custom_buffer_get_access_flags_type *buffer_get_access_flags = 0;
global custom_buffer_get_size_type *buffer_get_size = 0;
global custom_buffer_get_line_count_type *buffer_get_line_count = 0;
global custom_push_buffer_base_name_type *push_buffer_base_name = 0;
global custom_push_buffer_unique_name_type *push_buffer_unique_name = 0;
global custom_push_buffer_filename_type *push_buffer_filename = 0;
global custom_buffer_get_dirty_state_type *buffer_get_dirty_state = 0;
global custom_buffer_set_dirty_state_type *buffer_set_dirty_state = 0;
global custom_buffer_set_layout_type *buffer_set_layout = 0;
global custom_buffer_clear_layout_cache_type *buffer_clear_layout_cache = 0;
global custom_buffer_get_layout_type *buffer_get_layout = 0;
global custom_buffer_get_setting_type *buffer_get_setting = 0;
global custom_buffer_set_setting_type *buffer_set_setting = 0;
global custom_buffer_get_managed_scope_type *buffer_get_managed_scope = 0;
global custom_buffer_send_end_signal_type *buffer_send_end_signal = 0;
global custom_create_buffer_type *create_buffer = 0;
global custom_buffer_save_type *buffer_save = 0;
global custom_buffer_kill_type *buffer_kill = 0;
global custom_buffer_reopen_type *buffer_reopen = 0;
global custom_buffer_get_file_attributes_type *buffer_get_file_attributes = 0;
global custom_get_view_next_type *get_view_next = 0;
global custom_get_view_prev_type *get_view_prev = 0;
global custom_get_this_ctx_view_type *get_this_ctx_view = 0;
global custom_get_active_view_type *get_active_view = 0;
global custom_view_exists_type *view_exists = 0;
global custom_view_get_buffer_type *view_get_buffer = 0;
global custom_view_get_cursor_pos_type *view_get_cursor_pos = 0;
global custom_view_get_mark_pos_type *view_get_mark_pos = 0;
global custom_view_get_preferred_x_type *view_get_preferred_x = 0;
global custom_view_set_preferred_x_type *view_set_preferred_x = 0;
global custom_view_get_screen_rect_type *view_get_screen_rect = 0;
global custom_view_get_panel_type *view_get_panel = 0;
global custom_panel_get_view_type *panel_get_view = 0;
global custom_panel_is_split_type *panel_is_split = 0;
global custom_panel_is_leaf_type *panel_is_leaf = 0;
global custom_panel_split_type *panel_split = 0;
global custom_panel_set_split_type *panel_set_split = 0;
global custom_panel_swap_children_type *panel_swap_children = 0;
global custom_panel_get_root_type *panel_get_root = 0;
global custom_panel_get_parent_type *panel_get_parent = 0;
global custom_panel_get_child_type *panel_get_child = 0;
global custom_view_close_type *view_close = 0;
global custom_view_get_buffer_region_type *view_get_buffer_region = 0;
global custom_view_get_buffer_scroll_type *view_get_buffer_scroll = 0;
global custom_view_set_active_type *view_set_active = 0;
global custom_view_enqueue_command_function_type *view_enqueue_command_function = 0;
global custom_view_get_setting_type *view_get_setting = 0;
global custom_view_set_setting_type *view_set_setting = 0;
global custom_view_get_managed_scope_type *view_get_managed_scope = 0;
global custom_buffer_compute_cursor_type *buffer_compute_cursor = 0;
global custom_view_compute_cursor_type *view_compute_cursor = 0;
global custom_view_set_camera_bounds_type *view_set_camera_bounds = 0;
global custom_view_get_camera_bounds_type *view_get_camera_bounds = 0;
global custom_view_set_cursor_type *view_set_cursor = 0;
global custom_view_set_buffer_scroll_type *view_set_buffer_scroll = 0;
global custom_view_set_mark_type *view_set_mark = 0;
global custom_view_quit_ui_type *view_quit_ui = 0;
global custom_view_set_buffer_type *view_set_buffer = 0;
global custom_view_push_context_type *view_push_context = 0;
global custom_view_pop_context_type *view_pop_context = 0;
global custom_view_alter_context_type *view_alter_context = 0;
global custom_view_current_context_type *view_current_context = 0;
global custom_view_current_context_hook_memory_type *view_current_context_hook_memory = 0;
global custom_create_user_managed_scope_type *create_user_managed_scope = 0;
global custom_destroy_user_managed_scope_type *destroy_user_managed_scope = 0;
global custom_get_global_managed_scope_type *get_global_managed_scope = 0;
global custom_get_managed_scope_with_multiple_dependencies_type *get_managed_scope_with_multiple_dependencies = 0;
global custom_managed_scope_clear_contents_type *managed_scope_clear_contents = 0;
global custom_managed_scope_clear_self_all_dependent_scopes_type *managed_scope_clear_self_all_dependent_scopes = 0;
global custom_managed_scope_allocator_type *managed_scope_allocator = 0;
global custom_managed_id_group_highest_id_type *managed_id_group_highest_id = 0;
global custom_managed_id_declare_type *managed_id_declare = 0;
global custom_managed_id_get_type *managed_id_get = 0;
global custom_managed_scope_get_attachment_type *managed_scope_get_attachment = 0;
global custom_managed_scope_attachment_erase_type *managed_scope_attachment_erase = 0;
global custom_alloc_managed_memory_in_scope_type *alloc_managed_memory_in_scope = 0;
global custom_alloc_buffer_markers_on_buffer_type *alloc_buffer_markers_on_buffer = 0;
global custom_managed_object_get_item_size_type *managed_object_get_item_size = 0;
global custom_managed_object_get_item_count_type *managed_object_get_item_count = 0;
global custom_managed_object_get_pointer_type *managed_object_get_pointer = 0;
global custom_managed_object_get_type_type *managed_object_get_type = 0;
global custom_managed_object_get_containing_scope_type *managed_object_get_containing_scope = 0;
global custom_managed_object_free_type *managed_object_free = 0;
global custom_managed_object_store_data_type *managed_object_store_data = 0;
global custom_managed_object_load_data_type *managed_object_load_data = 0;
global custom_get_next_input_raw_type *get_next_input_raw = 0;
global custom_get_current_input_sequence_number_type *get_current_input_sequence_number = 0;
global custom_get_current_input_type *get_current_input = 0;
global custom_set_current_input_type *set_current_input = 0;
global custom_leave_current_input_unhandled_type *leave_current_input_unhandled = 0;
global custom_set_custom_hook_type *set_custom_hook = 0;
global custom_get_custom_hook_type *get_custom_hook = 0;
global custom_set_custom_hook_memory_size_type *set_custom_hook_memory_size = 0;
global custom_get_mouse_state_type *get_mouse_state = 0;
global custom_get_active_query_bars_type *get_active_query_bars = 0;
global custom_start_query_bar_type *start_query_bar = 0;
global custom_end_query_bar_type *end_query_bar = 0;
global custom_clear_all_query_bars_type *clear_all_query_bars = 0;
global custom_print_message_type *print_message = 0;
global custom_log_string_type *log_string = 0;
global custom_get_largest_face_id_type *get_largest_face_id = 0;
global custom_set_global_face_type *set_global_face = 0;
global custom_buffer_history_get_max_record_index_type *buffer_history_get_max_record_index = 0;
global custom_buffer_history_get_record_info_type *buffer_history_get_record_info = 0;
global custom_buffer_history_get_group_sub_record_type *buffer_history_get_group_sub_record = 0;
global custom_buffer_history_get_current_state_index_type *buffer_history_get_current_state_index = 0;
global custom_buffer_history_set_current_state_index_type *buffer_history_set_current_state_index = 0;
global custom_buffer_history_merge_record_range_type *buffer_history_merge_record_range = 0;
global custom_buffer_history_clear_after_current_state_type *buffer_history_clear_after_current_state = 0;
global custom_global_history_edit_group_begin_type *global_history_edit_group_begin = 0;
global custom_global_history_edit_group_end_type *global_history_edit_group_end = 0;
global custom_buffer_set_face_type *buffer_set_face = 0;
global custom_get_face_description_type *get_face_description = 0;
global custom_get_face_metrics_type *get_face_metrics = 0;
global custom_get_face_advance_map_type *get_face_advance_map = 0;
global custom_get_face_id_type *get_face_id = 0;
global custom_try_create_new_face_type *try_create_new_face = 0;
global custom_try_modify_face_type *try_modify_face = 0;
global custom_try_release_face_type *try_release_face = 0;
global custom_push_hot_directory_type *push_hot_directory = 0;
global custom_set_hot_directory_type *set_hot_directory = 0;
global custom_send_exit_signal_type *send_exit_signal = 0;
global custom_hard_exit_type *hard_exit = 0;
global custom_set_window_title_type *set_window_title = 0;
global custom_acquire_global_frame_mutex_type *acquire_global_frame_mutex = 0;
global custom_release_global_frame_mutex_type *release_global_frame_mutex = 0;
global custom_draw_string_oriented_type *draw_string_oriented = 0;
global custom_get_string_advance_type *get_string_advance = 0;
global custom_draw_rect_type *draw_rect = 0;
global custom_draw_rect_outline_type *draw_rect_outline = 0;
global custom_draw_set_clip_type *draw_set_clip = 0;
global custom_text_layout_create_type *text_layout_create = 0;
global custom_text_layout_region_type *text_layout_region = 0;
global custom_text_layout_get_buffer_type *text_layout_get_buffer = 0;
global custom_text_layout_get_visible_range_type *text_layout_get_visible_range = 0;
global custom_text_layout_line_on_screen_type *text_layout_line_on_screen = 0;
global custom_text_layout_character_on_screen_type *text_layout_character_on_screen = 0;
global custom_paint_text_color_type *paint_text_color = 0;
global custom_paint_text_color_blend_type *paint_text_color_blend = 0;
global custom_text_layout_free_type *text_layout_free = 0;
global custom_draw_text_layout_type *draw_text_layout = 0;
global custom_open_color_picker_type *open_color_picker = 0;
global custom_animate_in_n_milliseconds_type *animate_in_n_milliseconds = 0;
global custom_buffer_find_all_matches_type *buffer_find_all_matches = 0;
global custom_get_core_profile_list_type *get_core_profile_list = 0;
global custom_get_custom_layer_boundary_docs_type *get_custom_layer_boundary_docs = 0;
#undef DYNAMIC_LINK_API
#endif
