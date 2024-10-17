/*
4coder_default_hooks.cpp - Sets up the hooks for the default framework.
*/

// TOP

#if 0
CUSTOM_COMMAND_SIG(default_startup)
CUSTOM_DOC("Default command for responding to a startup event")
{
    ProfileScope(app, "default startup");
    User_Input input = get_current_input(app);
    if ( match_core_code(&input, CoreCode_Startup) )
    {
        String8_Array filenames = input.event.core.filenames;
        load_themes_default_folder(app);
        default_4coder_initialize(app, filenames);
        default_4coder_side_by_side_panels(app, filenames);
        b32 auto_load = def_get_config_b32(vars_intern_lit("automatically_load_project"));
        if (auto_load)
        {
            load_project(app);
        }
    }
   
    {
        //def_audio_init();
    }
    
    {
        def_enable_virtual_whitespace = def_get_config_b32(vars_intern_lit("enable_virtual_whitespace"));
        clear_all_layouts(app);
    }
}
#endif

CUSTOM_COMMAND_SIG(default_try_exit)
CUSTOM_DOC("Default command for responding to a try-exit event")
{
    User_Input input = get_current_input(app);
    if ( match_core_code(&input, CoreCode_TryExit) ){
        b32 do_exit = true;
        if (!allow_immediate_close_without_checking_for_changes){
            b32 has_unsaved_changes = false;
            for(Buffer_ID buffer = get_buffer_next(app, 0, Access_Always);
                buffer != 0;
                buffer = get_buffer_next(app, buffer, Access_Always)){
                Dirty_State dirty = buffer_get_dirty_state(app, buffer);
                if (HasFlag(dirty, DirtyState_UnsavedChanges)){
                    has_unsaved_changes = true;
                    break;
                }
            }
            if (has_unsaved_changes){
                View_ID view = get_active_view(app, Access_Always);
                do_exit = do_4coder_close_user_check(app, view);
            }
        }
        if (do_exit){
            hard_exit(app);
        }
    }
}

function Implicit_Map_Result
default_implicit_map(App *app, String_ID lang, String_ID mode, Input_Event *event)
{
    Implicit_Map_Result result = {};
    
    View_ID view = get_this_ctx_view(app, Access_Always);
    
    Command_Map_ID map_id = default_get_map_id(app, view);
    Command_Binding binding = map_get_binding_recursive(&framework_mapping, map_id, event);
    
    // TODO(allen): map_id <-> map name?
 result.map = 0;
 result.command = binding.custom;
 
 return(result);
}

CUSTOM_COMMAND_SIG(default_view_input_handler)
CUSTOM_DOC("Input consumption loop for default view behavior")
{
 Scratch_Block scratch(app);
 default_input_handler_init(app, scratch);
 
 View_ID view = get_this_ctx_view(app, Access_Always);
 Managed_Scope scope = view_get_managed_scope(app, view);
 
 for (;;){
  // NOTE(allen): Get input
  User_Input input = get_next_input(app, EventPropertyGroup_Any, 0);
  if (input.abort){
   break;
  }
  
  ProfileScopeNamed(app, "before view input", view_input_profile);
  
  // NOTE(allen): Mouse Suppression
  Event_Property event_properties = get_event_properties(&input.event);
  if (suppressing_mouse && (event_properties & EventPropertyGroup_AnyMouseEvent) != 0){
   continue;
  }
  
  // NOTE(allen): Get binding
  if (implicit_map_function == 0){
   implicit_map_function = default_implicit_map;
  }
  Implicit_Map_Result map_result = implicit_map_function(app, 0, 0, &input.event);
  if (map_result.command == 0){
   leave_current_input_unhandled(app);
   continue;
  }
  
  // NOTE(allen): Run the command and pre/post command stuff
  default_pre_command(app, scope);
  ProfileCloseNow(view_input_profile);
  map_result.command(app);
  ProfileScope(app, "after view input");
  default_post_command(app, scope);
 }
}

//NOTE(kv) This function actually does... thing.
//  IDK what but the editor crashes if you don't call it
function void
code_index_update_tick(App *app){
 Scratch_Block scratch(app);
 for (Buffer_Modified_Node *node = global_buffer_modified_set.first;
      node != 0;
      node = node->next){
  Temp_Memory_Block temp(scratch);
  Buffer_ID buffer_id = node->buffer;
  
  String contents = push_whole_buffer(app, scratch, buffer_id);
  Token_Array tokens = get_token_array_from_buffer(app, buffer_id);
  if(tokens.count){
   Arena arena = make_arena_system(KB(16));
   auto index = push_array_zero(&arena, Code_Index_File, 1);
   index->buffer = buffer_id;
   
   Generic_Parse_State state = {};
   generic_parse_init(app, &arena, contents, &tokens, &state);
   // TODO(allen): Actually determine this in a fair way.
   // Maybe switch to an enum?
   // Actually probably a pointer to a struct that defines the language.
   state.do_cpp_parse = true;
   generic_parse_full_input_breaks(index, &state, max_i32);
   
   code_index_lock();
   code_index_set_file(buffer_id, arena, index);
   code_index_unlock();
   buffer_clear_layout_cache(app, buffer_id);
  }
 }
 
 buffer_modified_set_clear();
}
function void
default_tick(App *app, Frame_Info frame_info){
 arena_clear(&global_frame_arena);
 code_index_update_tick(app);
 if(tick_all_fade_ranges(app, frame_info.animation_dt)){
  animate_in_n_milliseconds(app, 0);
 }
}

function Rect_f32
default_buffer_region(App *app, View_ID view_id, Rect_f32 region){
    Buffer_ID buffer = view_get_buffer(app, view_id, Access_Always);
    Face_ID face_id = get_face_id(app, buffer);
    Face_Metrics metrics = get_face_metrics(app, face_id);
    f32 line_height = metrics.line_height;
    f32 digit_advance = metrics.decimal_digit_advance;
    
    // NOTE(allen): margins
    region = rect_inner(region, 3.f);
    
    // NOTE(allen): file bar
    b64 showing_file_bar = false;
    if (view_get_setting(app, view_id, ViewSetting_ShowFileBar, &showing_file_bar) &&
        showing_file_bar){
        rect2_Pair pair = layout_file_bar_on_top(region, line_height);
        region = pair.max;
    }
    
    // NOTE(allen): query bars
    {
        Query_Bar *space[32];
        Query_Bar_Ptr_Array query_bars = {};
        query_bars.ptrs = space;
        if (get_active_query_bars(app, view_id, ArrayCount(space), &query_bars)){
            rect2_Pair pair = layout_query_bar_on_top(region, line_height, query_bars.count);
            region = pair.max;
        }
    }
    
    // NOTE(allen): FPS hud
    if (show_fps_hud){
        rect2_Pair pair = layout_fps_hud_on_bottom(region, line_height);
        region = pair.min;
    }
    
    // NOTE(allen): line numbers
    b32 show_line_number_margins = def_get_config_b32(vars_intern_lit("show_line_number_margins"));
    if (show_line_number_margins){
        rect2_Pair pair = layout_line_number_margin(app, buffer, region, digit_advance);
        region = pair.max;
    }
    
    return(region);
}

function void
recursive_nest_highlight(App *app, Text_Layout_ID layout_id, Range_i64 range,
                         Code_Index_Nest_Ptr_Array *array, i32 counter){
    Code_Index_Nest **ptr = array->ptrs;
    Code_Index_Nest **ptr_end = ptr + array->count;
    
    for (;ptr < ptr_end; ptr += 1){
        Code_Index_Nest *nest = *ptr;
        if (!nest->is_closed){
            break;
        }
        if (range.first <= nest->close.max){
            break;
        }
    }
    
    ARGB_Color argb = finalize_color(defcolor_text_cycle, counter);
    
    for (;ptr < ptr_end; ptr += 1){
        Code_Index_Nest *nest = *ptr;
        if (range.max <= nest->open.min){
            break;
        }
        
        paint_text_color(app, layout_id, nest->open, argb);
        if (nest->is_closed){
   paint_text_color(app, layout_id, nest->close, argb);
  }
  recursive_nest_highlight(app, layout_id, range, &nest->nest_array, counter + 1);
 }
}

function void
recursive_nest_highlight(App *app, Text_Layout_ID layout_id, Range_i64 range,
                         Code_Index_File *file){
 recursive_nest_highlight(app, layout_id, range, &file->nest_array, 0);
}

function Rect_f32
default_draw_query_bars(App *app, Rect_f32 region, View_ID view_id, Face_ID face_id){
 Face_Metrics face_metrics = get_face_metrics(app, face_id);
 f32 line_height = face_metrics.line_height;
 
 Query_Bar *space[32];
 Query_Bar_Ptr_Array query_bars = {};
 query_bars.ptrs = space;
 if (get_active_query_bars(app, view_id, ArrayCount(space), &query_bars)){
  for (i32 i = 0; i < query_bars.count; i += 1){
   rect2_Pair pair = layout_query_bar_on_top(region, line_height, 1);
   draw_query_bar(app, query_bars.ptrs[i], face_id, pair.min);
   region = pair.max;
  }
 }
 return(region);
}

function void
default_render_caller(App *app, Frame_Info frame_info, View_ID view_id)
{
}

function void
default_whole_screen_render_caller(App *app, Frame_Info frame_info)
{
}

HOOK_SIG(default_view_adjust){
    // NOTE(allen): Called whenever the view layout/sizes have been modified,
    // including by full window resize.
    return(0);
}

BUFFER_NAME_RESOLVER_SIG(default_buffer_name_resolution){
    ProfileScope(app, "default buffer name resolution");
    if (conflict_count > 1){
        // List of unresolved conflicts
        Scratch_Block scratch(app);
        
        i32 *unresolved = push_array(scratch, i32, conflict_count);
        i32 unresolved_count = conflict_count;
        for (i32 i = 0; i < conflict_count; ++i){
            unresolved[i] = i;
        }
        
        // Resolution Loop
        i32 x = 0;
        for (;;){
            // Resolution Pass
            ++x;
            for (i32 i = 0; i < unresolved_count; ++i){
                i32 conflict_index = unresolved[i];
                Buffer_Name_Conflict_Entry *conflict = &conflicts[conflict_index];
                
                u64 size = conflict->base_name.size;
                size = clamp_max(size, conflict->unique_name_capacity);
                conflict->unique_name_len_in_out = size;
                block_copy(conflict->unique_name_in_out, conflict->base_name.str, size);
                
                if (conflict->filename.str != 0){
                    Temp_Memory_Block temp(scratch);
                    String uniqueifier = {};
                    
                    String8 filename = path_dirname(conflict->filename);
                    if (filename.size > 0){
                        filename = string_chop(filename, 1);
                        u8 *end = filename.str + filename.size;
                        b32 past_the_end = false;
                        for (i32 j = 0; j < x; ++j){
                            filename = path_dirname(filename);
                            if (j + 1 < x){
                                filename = string_chop(filename, 1);
                            }
                            if (filename.size == 0){
                                if (j + 1 < x){
                                    past_the_end = true;
                                }
                                break;
                            }
                        }
                        u8 *start = filename.str + filename.size;
                        
                        uniqueifier = SCu8(start, end);
                        if (past_the_end){
                            uniqueifier = push_stringfz(scratch, "%.*s~%d",
                                                          string_expand(uniqueifier), i);
                        }
                    }
                    else{
                        uniqueifier = push_stringfz(scratch, "%d", i);
                    }
                    
                    String_u8 builder = Su8(conflict->unique_name_in_out,
                                            conflict->unique_name_len_in_out,
                                            conflict->unique_name_capacity);
                    string_concat(&builder, strlit(" <"));
                    string_concat(&builder, uniqueifier);
                    string_concat(&builder, strlit(">"));
                    conflict->unique_name_len_in_out = builder.size;
                }
            }
            
            // Conflict Check Pass
            b32 has_conflicts = false;
            for (i32 i = 0; i < unresolved_count; ++i){
                i32 conflict_index = unresolved[i];
                Buffer_Name_Conflict_Entry *conflict = &conflicts[conflict_index];
                String conflict_name = SCu8(conflict->unique_name_in_out,
                                                     conflict->unique_name_len_in_out);
                
                b32 hit_conflict = false;
                if (conflict->filename.str != 0){
                    for (i32 j = 0; j < unresolved_count; ++j){
                        if (i == j) continue;
                        
                        i32 conflict_j_index = unresolved[j];
                        Buffer_Name_Conflict_Entry *conflict_j = &conflicts[conflict_j_index];
                        
                        String conflict_name_j = SCu8(conflict_j->unique_name_in_out,
                                                               conflict_j->unique_name_len_in_out);
                        
                        if (string_match(conflict_name, conflict_name_j)){
                            hit_conflict = true;
                            break;
                        }
                    }
                }
                
                if (hit_conflict){
                    has_conflicts = true;
                }
                else{
                    --unresolved_count;
                    unresolved[i] = unresolved[unresolved_count];
                    --i;
                }
            }
            
            if (!has_conflicts){
                break;
            }
        }
    }
}

function void
parse_async__inner(Async_Context *actx, Buffer_ID buffer_id,
                   String contents, Token_Array *tokens, i32 limit_factor){
    App *app = actx->app;
    ProfileBlock(app, "async parse");
    
    Arena arena = make_arena_system(KB(16));
    Code_Index_File *index = push_array_zero(&arena, Code_Index_File, 1);
    index->buffer = buffer_id;
    
    Generic_Parse_State state = {};
    generic_parse_init(app, &arena, contents, tokens, &state);
    
    b32 canceled = false;
    
    for (;;){
        if (generic_parse_full_input_breaks(index, &state, limit_factor)){
            break;
        }
        if (async_check_canceled(actx)){
            canceled = true;
            break;
        }
    }
    
    if (!canceled){
        acquire_global_frame_mutex(app);
        code_index_lock();
        code_index_set_file(buffer_id, arena, index);
        code_index_unlock();
        buffer_clear_layout_cache(app, buffer_id);
        release_global_frame_mutex(app);
    }
    else{
        arena_clear(&arena);
    }
}

function void
do_full_lex_async__inner(Async_Context *actx, Buffer_ID buffer_id){
    App *app = actx->app;
    ProfileScope(app, "async lex");
    Scratch_Block scratch(app);
    
    String contents = {};
    {
        ProfileBlock(app, "async lex contents (before mutex)");
        acquire_global_frame_mutex(app);
        ProfileBlock(app, "async lex contents (after mutex)");
        contents = push_whole_buffer(app, scratch, buffer_id);
        release_global_frame_mutex(app);
    }
    
    i32 limit_factor = 10000;
    
    Token_List list = {};
    b32 canceled = false;
    
    Lex_State_Cpp state = {};
    lex_full_input_cpp_init(&state, contents);
    for (;;){
        ProfileBlock(app, "async lex block");
        if (lex_full_input_cpp_breaks(scratch, &list, &state, limit_factor)){
            break;
        }
        if (async_check_canceled(actx)){
            canceled = true;
            break;
        }
    }
    
    if (!canceled){
        ProfileBlock(app, "async lex save results (before mutex)");
        acquire_global_frame_mutex(app);
        ProfileBlock(app, "async lex save results (after mutex)");
        Managed_Scope scope = buffer_get_managed_scope(app, buffer_id);
        if (scope != 0){
            Base_Allocator *allocator = managed_scope_allocator(app, scope);
            Token_Array *tokens_ptr = scope_attachment(app, scope, attachment_tokens, Token_Array);
            base_free(allocator, tokens_ptr->tokens);
            Token_Array tokens = {};
            tokens.tokens = base_array(allocator, Token, list.total_count);
            tokens.count = list.total_count;
            tokens.max = list.total_count;
            token_fill_memory_from_list(tokens.tokens, &list);
            block_copy_struct(tokens_ptr, &tokens);
        }
        buffer_mark_as_modified(buffer_id);
        release_global_frame_mutex(app);
    }
}

function void
do_full_lex_async(Async_Context *actx, String data){
 if (data.size == sizeof(Buffer_ID)){
  Buffer_ID buffer = *(Buffer_ID*)data.str;
  do_full_lex_async__inner(actx, buffer);
 }
}

BUFFER_HOOK_SIG(default_begin_buffer)
{
 ProfileScope(app, "begin buffer");
 
 Scratch_Block scratch(app);
 
 b32 treat_as_code = false;
 String filename = push_buffer_filename(app, scratch, buffer_id);
 if (filename.size > 0){
  String treat_as_code_string = def_get_config_string(scratch, vars_intern_lit("treat_as_code"));
  String_Array extensions = parse_extension_line_to_extension_list(app, scratch, treat_as_code_string);
  String ext = path_extension(filename);
  for (i32 i = 0; i < extensions.count; ++i){
   if (string_match(ext, extensions.strings[i])){
    
    if (string_match(ext, strlit("cpp")) ||
        string_match(ext, strlit("h")) ||
        string_match(ext, strlit("c")) ||
        string_match(ext, strlit("hpp")) ||
        string_match(ext, strlit("cc"))){
     treat_as_code = true;
    }
    
#if 0
    treat_as_code = true;
    
    if (string_match(ext, strlit("cs"))){
     if (parse_context_language_cs == 0){
      init_language_cs(app);
     }
     parse_context_id = parse_context_language_cs;
    }
    
    if (string_match(ext, strlit("java"))){
     if (parse_context_language_java == 0){
      init_language_java(app);
     }
     parse_context_id = parse_context_language_java;
    }
    
    if (string_match(ext, strlit("rs"))){
     if (parse_context_language_rust == 0){
      init_language_rust(app);
     }
     parse_context_id = parse_context_language_rust;
    }
    
    if (string_match(ext, strlit("cpp")) ||
        string_match(ext, strlit("h")) ||
        string_match(ext, strlit("c")) ||
        string_match(ext, strlit("hpp")) ||
        string_match(ext, strlit("cc"))){
     if (parse_context_language_cpp == 0){
      init_language_cpp(app);
     }
     parse_context_id = parse_context_language_cpp;
    }
    
    // TODO(NAME): Real GLSL highlighting
    if (string_match(ext, strlit("glsl"))){
     if (parse_context_language_cpp == 0){
      init_language_cpp(app);
     }
     parse_context_id = parse_context_language_cpp;
    }
    
    // TODO(NAME): Real Objective-C highlighting
    if (string_match(ext, strlit("m"))){
     if (parse_context_language_cpp == 0){
      init_language_cpp(app);
     }
     parse_context_id = parse_context_language_cpp;
    }
#endif
    
    break;
   }
  }
 }
 
 String_ID file_map_id = vars_intern_lit("keys_file");
 String_ID code_map_id = vars_intern_lit("keys_code");
 
 Command_Map_ID map_id = (treat_as_code)?(code_map_id):(file_map_id);
 Managed_Scope scope = buffer_get_managed_scope(app, buffer_id);
 Command_Map_ID *map_id_ptr = scope_attachment(app, scope, buffer_map_id, Command_Map_ID);
 *map_id_ptr = map_id;
 
 Line_Ending_Kind setting = guess_line_ending_kind_from_buffer(app, buffer_id);
 Line_Ending_Kind *eol_setting = scope_attachment(app, scope, buffer_eol_setting, Line_Ending_Kind);
 *eol_setting = setting;
 
 // NOTE(allen): Decide buffer settings
 b32 wrap_lines = true;
 b32 use_lexer = false;
 if (treat_as_code){
  wrap_lines = def_get_config_b32(vars_intern_lit("enable_code_wrapping"));
  use_lexer = true;
 }
 
 String buffer_name = push_buffer_base_name(app, scratch, buffer_id);
 if (buffer_name.size > 0 && buffer_name.str[0] == '*' && buffer_name.str[buffer_name.size - 1] == '*'){
  wrap_lines = def_get_config_b32(vars_intern_lit("enable_output_wrapping"));
 }
 
 if (use_lexer){
  ProfileBlock(app, "begin buffer kick off lexer");
  Async_Task *lex_task_ptr = scope_attachment(app, scope, buffer_lex_task, Async_Task);
  *lex_task_ptr = async_task_no_dep(&global_async_system, do_full_lex_async, make_data_struct(&buffer_id));
 }
 
 buffer_set_layout(app, buffer_id, layout_basic);
 
 // no meaning for return
 return(0);
}

BUFFER_HOOK_SIG(default_new_file){
    Scratch_Block scratch(app);
    String filename = push_buffer_base_name(app, scratch, buffer_id);
    if (!string_match(string_postfix(filename, 2), strlit(".h"))) {
        return(0);
    }
    
    List_String guard_list = {};
    for (u64 i = 0; i < filename.size; ++i){
        u8 c[2] = {};
        u64 c_size = 1;
        u8 ch = filename.str[i];
        if ('A' <= ch && ch <= 'Z'){
            c_size = 2;
            c[0] = '_';
            c[1] = ch;
        }
        else if ('0' <= ch && ch <= '9'){
            c[0] = ch;
        }
        else if ('a' <= ch && ch <= 'z'){
            c[0] = ch - ('a' - 'A');
        }
        else{
            c[0] = '_';
        }
        String part = push_stringz(scratch, SCu8(c, c_size));
        string_list_push(scratch, &guard_list, part);
    }
    String guard = string_list_flatten(scratch, guard_list);
    
    Date_Time date_time = system_now_date_time_universal();
    date_time = system_local_date_time_from_universal(&date_time);
    String date_string = date_time_format(scratch, "month day yyyy h:mimi ampm", &date_time);
    
    Buffer_Insertion insert = begin_buffer_insertion_at_buffered2(app, buffer_id, 0, scratch, KB(16));
    insertf(&insert,
            "/* date = %.*s */\n"
            "\n",
            string_expand(date_string));
    insertf(&insert,
            "#ifndef %.*s\n"
            "#define %.*s\n"
            "\n"
            "#endif //%.*s\n",
            string_expand(guard),
            string_expand(guard),
            string_expand(guard));
    end_buffer_insertion(&insert);
    
    return(0);
}

BUFFER_HOOK_SIG(default_file_save){
    // buffer_id
    ProfileScope(app, "default file save");
    
    b32 auto_indent = def_get_config_b32(vars_intern_lit("automatically_indent_text_on_save"));
    b32 is_virtual = def_get_config_b32(vars_intern_lit("enable_virtual_whitespace"));
    if (auto_indent && is_virtual){
        auto_indent_buffer(app, buffer_id, buffer_range(app, buffer_id));
    }
    
    Managed_Scope scope = buffer_get_managed_scope(app, buffer_id);
    Line_Ending_Kind *eol = scope_attachment(app, scope, buffer_eol_setting,
                                             Line_Ending_Kind);
    switch (*eol){
        case LineEndingKind_LF:
        {
            rewrite_lines_to_lf(app, buffer_id);
        }break;
        case LineEndingKind_CRLF:
        {
            rewrite_lines_to_crlf(app, buffer_id);
        }break;
    }
    
    // no meaning for return
    return(0);
}

BUFFER_EDIT_RANGE_SIG(default_buffer_edit_range){
    // buffer_id, new_range, original_size
    ProfileScope(app, "default edit range");
    
    Range_i64 old_range = Ii64(old_cursor_range.min.pos, old_cursor_range.max.pos);
    
    buffer_shift_fade_ranges(buffer_id, old_range.max, (new_range.max - old_range.max));
    
    {
        code_index_lock();
        Code_Index_File *file = code_index_get_file(buffer_id);
        if (file != 0){
            code_index_shift(file, old_range, range_size(new_range));
        }
        code_index_unlock();
    }
    
    i64 insert_size = range_size(new_range);
    i64 text_shift = replace_range_shift(old_range, insert_size);
    
    Scratch_Block scratch(app);
    
    Managed_Scope scope = buffer_get_managed_scope(app, buffer_id);
    Async_Task *lex_task_ptr = scope_attachment(app, scope, buffer_lex_task, Async_Task);
    
    Base_Allocator *allocator = managed_scope_allocator(app, scope);
    b32 do_full_relex = false;
    
    if (async_task_is_running_or_pending(&global_async_system, *lex_task_ptr)){
        async_task_cancel(app, &global_async_system, *lex_task_ptr);
        buffer_unmark_as_modified(buffer_id);
        do_full_relex = true;
        *lex_task_ptr = 0;
    }
    
    Token_Array *ptr = scope_attachment(app, scope, attachment_tokens, Token_Array);
    if (ptr != 0 && ptr->tokens != 0){
        ProfileBlockNamed(app, "attempt resync", profile_attempt_resync);
        
        i64 token_index_first = token_relex_first(ptr, old_range.first, 1);
        i64 token_index_resync_guess =
            token_relex_resync(ptr, old_range.opl, 16);
        
        if (token_index_resync_guess - token_index_first >= 4000){
            do_full_relex = true;
        }
        else{
            Token *token_first = ptr->tokens + token_index_first;
            Token *token_resync = ptr->tokens + token_index_resync_guess;
            
            Range_i64 relex_range = Ii64(token_first->pos, token_resync->pos + token_resync->size + text_shift);
            String partial_text = push_buffer_range(app, scratch, buffer_id, relex_range);
            
            Token_List relex_list = lex_full_input_cpp(scratch, partial_text);
            if (relex_range.opl < buffer_get_size(app, buffer_id)){
                token_drop_eof(&relex_list);
            }
            
            Token_Relex relex = token_relex(relex_list, relex_range.first - text_shift, ptr->tokens, token_index_first, token_index_resync_guess);
            
            ProfileCloseNow(profile_attempt_resync);
            
            if (!relex.successful_resync){
                do_full_relex = true;
            }
            else{
                ProfileBlock(app, "apply resync");
                
                i64 token_index_resync = relex.first_resync_index;
                
                Range_i64 head = Ii64(0, token_index_first);
                Range_i64 replaced = Ii64(token_index_first, token_index_resync);
                Range_i64 tail = Ii64(token_index_resync, ptr->count);
                i64 resynced_count = (token_index_resync_guess + 1) - token_index_resync;
                i64 relexed_count = relex_list.total_count - resynced_count;
                i64 tail_shift = relexed_count - (token_index_resync - token_index_first);
                
                i64 new_tokens_count = ptr->count + tail_shift;
                Token *new_tokens = base_array(allocator, Token, new_tokens_count);
                
                Token *old_tokens = ptr->tokens;
                block_copy_array_dst_shift(new_tokens, old_tokens, head, 0);
                token_fill_memory_from_list(new_tokens + replaced.first, &relex_list, relexed_count);
                for (i64 i = 0, index = replaced.first; i < relexed_count; i += 1, index += 1){
                    new_tokens[index].pos += relex_range.first;
                }
                for (i64 i = tail.first; i < tail.opl; i += 1){
                    old_tokens[i].pos += text_shift;
                }
                block_copy_array_dst_shift(new_tokens, ptr->tokens, tail, tail_shift);
                
                base_free(allocator, ptr->tokens);
                
                ptr->tokens = new_tokens;
                ptr->count = new_tokens_count;
                ptr->max = new_tokens_count;
                
                buffer_mark_as_modified(buffer_id);
            }
        }
    }
    
    if (do_full_relex){
        *lex_task_ptr = async_task_no_dep(&global_async_system, do_full_lex_async,
                                          make_data_struct(&buffer_id));
    }
    
    // no meaning for return
    return(0);
}

BUFFER_HOOK_SIG(default_end_buffer){
    Managed_Scope scope = buffer_get_managed_scope(app, buffer_id);
    Async_Task *lex_task_ptr = scope_attachment(app, scope, buffer_lex_task, Async_Task);
    if (lex_task_ptr != 0){
        async_task_cancel(app, &global_async_system, *lex_task_ptr);
    }
    buffer_unmark_as_modified(buffer_id);
    code_index_lock();
    code_index_erase_file(buffer_id);
    code_index_unlock();
    // no meaning for return
    return(0);
}

function void
default_view_change_buffer(App *app, View_ID view_id,
                           Buffer_ID old_buffer_id, Buffer_ID new_buffer_id){
 Managed_Scope scope = view_get_managed_scope(app, view_id);
 Buffer_ID *prev_buffer_id = scope_attachment(app, scope, view_previous_buffer, Buffer_ID);
	if (prev_buffer_id != 0){
		*prev_buffer_id = old_buffer_id;
	}
}

function void
set_all_default_hooks(App *app)
{
 set_custom_hook(app, HookID_BufferViewerUpdate, default_view_adjust);
 
 set_custom_hook(app, HookID_ViewEventHandler, default_view_input_handler);
 set_custom_hook(app, HookID_Tick, default_tick);
 set_custom_hook(app, HookID_RenderCaller, default_render_caller);
 set_custom_hook(app, HookID_WholeScreenRenderCaller, default_whole_screen_render_caller);
 
 set_custom_hook(app, HookID_DeltaRule, fixed_time_cubic_delta);
 set_custom_hook_memory_size(app, HookID_DeltaRule,
                             delta_ctx_size(fixed_time_cubic_delta_memory_size));
 
 set_custom_hook(app, HookID_BufferNameResolver, default_buffer_name_resolution);
 
 set_custom_hook(app, HookID_BeginBuffer, default_begin_buffer);
 set_custom_hook(app, HookID_EndBuffer, end_buffer_close_jump_list);
 set_custom_hook(app, HookID_NewFile, default_new_file);
 set_custom_hook(app, HookID_SaveFile, default_file_save);
 set_custom_hook(app, HookID_BufferEditRange, default_buffer_edit_range);
 set_custom_hook(app, HookID_BufferRegion, default_buffer_region);
 set_custom_hook(app, HookID_ViewChangeBuffer, default_view_change_buffer);
 
 set_custom_hook(app, HookID_Layout, layout_unwrapped);
 //set_custom_hook(app, HookID_Layout, layout_wrap_whitespace);
}

// BOTTOM

