function API_Definition*
system_api_construct(Arena *arena){
API_Definition *result = begin_api(arena, "system");
{
API_Call *call = api_call_with_location(arena, result, strlit("error_box"), strlit("void"), strlit(""));
api_param(arena, call, "char*", "msg");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("get_path"), strlit("String"), strlit(""));
api_param(arena, call, "Arena*", "arena");
api_param(arena, call, "System_Path_Code", "path_code");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("get_canonical"), strlit("String"), strlit(""));
api_param(arena, call, "Arena*", "arena");
api_param(arena, call, "String", "name");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("get_file_list"), strlit("File_List"), strlit(""));
api_param(arena, call, "Arena*", "arena");
api_param(arena, call, "String", "directory");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("quick_file_attributes"), strlit("File_Attributes"), strlit(""));
api_param(arena, call, "Arena*", "scratch");
api_param(arena, call, "String", "file_name");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("load_handle"), strlit("b32"), strlit(""));
api_param(arena, call, "Arena*", "scratch");
api_param(arena, call, "char*", "file_name");
api_param(arena, call, "Plat_Handle*", "out");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("load_attributes"), strlit("File_Attributes"), strlit(""));
api_param(arena, call, "Plat_Handle", "handle");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("load_file"), strlit("b32"), strlit(""));
api_param(arena, call, "Plat_Handle", "handle");
api_param(arena, call, "char*", "buffer");
api_param(arena, call, "u32", "size");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("load_close"), strlit("b32"), strlit(""));
api_param(arena, call, "Plat_Handle", "handle");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("save_file"), strlit("File_Attributes"), strlit(""));
api_param(arena, call, "Arena*", "scratch");
api_param(arena, call, "char*", "file_name");
api_param(arena, call, "String", "data");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("load_library"), strlit("b32"), strlit(""));
api_param(arena, call, "Arena*", "scratch");
api_param(arena, call, "String", "file_name");
api_param(arena, call, "System_Library*", "out");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("release_library"), strlit("b32"), strlit(""));
api_param(arena, call, "System_Library", "handle");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("get_proc"), strlit("Void_Func*"), strlit(""));
api_param(arena, call, "System_Library", "handle");
api_param(arena, call, "char*", "proc_name");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("now_time"), strlit("u64"), strlit(""));
(void)call;
}
{
API_Call *call = api_call_with_location(arena, result, strlit("now_date_time_universal"), strlit("Date_Time"), strlit(""));
(void)call;
}
{
API_Call *call = api_call_with_location(arena, result, strlit("local_date_time_from_universal"), strlit("Date_Time"), strlit(""));
api_param(arena, call, "Date_Time*", "date_time");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("universal_date_time_from_local"), strlit("Date_Time"), strlit(""));
api_param(arena, call, "Date_Time*", "date_time");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("wake_up_timer_create"), strlit("Plat_Handle"), strlit(""));
(void)call;
}
{
API_Call *call = api_call_with_location(arena, result, strlit("wake_up_timer_release"), strlit("void"), strlit(""));
api_param(arena, call, "Plat_Handle", "handle");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("wake_up_timer_set"), strlit("void"), strlit(""));
api_param(arena, call, "Plat_Handle", "handle");
api_param(arena, call, "u32", "time_milliseconds");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("signal_step"), strlit("void"), strlit(""));
api_param(arena, call, "u32", "code");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("sleep"), strlit("void"), strlit(""));
api_param(arena, call, "u64", "microseconds");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("get_clipboard"), strlit("String"), strlit(""));
api_param(arena, call, "Arena*", "arena");
api_param(arena, call, "i1", "index");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("post_clipboard"), strlit("void"), strlit(""));
api_param(arena, call, "String", "str");
api_param(arena, call, "i1", "index");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("set_clipboard_catch_all"), strlit("void"), strlit(""));
api_param(arena, call, "b32", "enabled");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("get_clipboard_catch_all"), strlit("b32"), strlit(""));
(void)call;
}
{
API_Call *call = api_call_with_location(arena, result, strlit("cli_call"), strlit("b32"), strlit(""));
api_param(arena, call, "Arena*", "scratch");
api_param(arena, call, "char*", "path");
api_param(arena, call, "char*", "script");
api_param(arena, call, "CLI_Handles*", "cli_out");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("cli_begin_update"), strlit("void"), strlit(""));
api_param(arena, call, "CLI_Handles*", "cli");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("cli_update_step"), strlit("b32"), strlit(""));
api_param(arena, call, "CLI_Handles*", "cli");
api_param(arena, call, "char*", "dest");
api_param(arena, call, "u32", "max");
api_param(arena, call, "u32*", "amount");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("cli_end_update"), strlit("b32"), strlit(""));
api_param(arena, call, "CLI_Handles*", "cli");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("open_color_picker"), strlit("void"), strlit(""));
api_param(arena, call, "Color_Picker*", "picker");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("get_screen_scale_factor"), strlit("f32"), strlit(""));
(void)call;
}
{
API_Call *call = api_call_with_location(arena, result, strlit("thread_launch"), strlit("System_Thread"), strlit(""));
api_param(arena, call, "Thread_Function*", "proc");
api_param(arena, call, "void*", "ptr");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("thread_join"), strlit("void"), strlit(""));
api_param(arena, call, "System_Thread", "thread");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("thread_free"), strlit("void"), strlit(""));
api_param(arena, call, "System_Thread", "thread");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("thread_get_id"), strlit("i1"), strlit(""));
(void)call;
}
{
API_Call *call = api_call_with_location(arena, result, strlit("acquire_global_frame_mutex"), strlit("void"), strlit(""));
api_param(arena, call, "Thread_Context*", "tctx");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("release_global_frame_mutex"), strlit("void"), strlit(""));
api_param(arena, call, "Thread_Context*", "tctx");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("mutex_make"), strlit("System_Mutex"), strlit(""));
(void)call;
}
{
API_Call *call = api_call_with_location(arena, result, strlit("mutex_acquire"), strlit("void"), strlit(""));
api_param(arena, call, "System_Mutex", "mutex");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("mutex_release"), strlit("void"), strlit(""));
api_param(arena, call, "System_Mutex", "mutex");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("mutex_free"), strlit("void"), strlit(""));
api_param(arena, call, "System_Mutex", "mutex");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("condition_variable_make"), strlit("System_Condition_Variable"), strlit(""));
(void)call;
}
{
API_Call *call = api_call_with_location(arena, result, strlit("condition_variable_wait"), strlit("void"), strlit(""));
api_param(arena, call, "System_Condition_Variable", "cv");
api_param(arena, call, "System_Mutex", "mutex");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("condition_variable_signal"), strlit("void"), strlit(""));
api_param(arena, call, "System_Condition_Variable", "cv");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("condition_variable_free"), strlit("void"), strlit(""));
api_param(arena, call, "System_Condition_Variable", "cv");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("memory_allocate"), strlit("void*"), strlit(""));
api_param(arena, call, "u64", "size");
api_param(arena, call, "String", "location");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("memory_set_protection"), strlit("b32"), strlit(""));
api_param(arena, call, "void*", "ptr");
api_param(arena, call, "u64", "size");
api_param(arena, call, "u32", "flags");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("memory_free"), strlit("void"), strlit(""));
api_param(arena, call, "void*", "ptr");
api_param(arena, call, "u64", "size");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("memory_annotation"), strlit("Memory_Annotation"), strlit(""));
api_param(arena, call, "Arena*", "arena");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("show_mouse_cursor"), strlit("void"), strlit(""));
api_param(arena, call, "i1", "show");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("set_fullscreen"), strlit("b32"), strlit(""));
api_param(arena, call, "b32", "full_screen");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("is_fullscreen"), strlit("b32"), strlit(""));
(void)call;
}
{
API_Call *call = api_call_with_location(arena, result, strlit("get_keyboard_modifiers"), strlit("Input_Modifier_Set"), strlit(""));
api_param(arena, call, "Arena*", "arena");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("set_key_mode"), strlit("void"), strlit(""));
api_param(arena, call, "Key_Mode", "mode");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("set_source_mixer"), strlit("void"), strlit(""));
api_param(arena, call, "void*", "ctx");
api_param(arena, call, "Audio_Mix_Sources_Function*", "mix_func");
}
{
API_Call *call = api_call_with_location(arena, result, strlit("set_destination_mixer"), strlit("void"), strlit(""));
api_param(arena, call, "Audio_Mix_Destination_Function*", "mix_func");
}
return(result);
}
