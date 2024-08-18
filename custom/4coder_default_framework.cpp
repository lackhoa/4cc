/*
4coder_default_framework.cpp - Sets up the basics of the framework that is used for default 4coder behaviour.
*/

// TOP

function void
point_stack_push(App *app, Buffer_ID buffer, i64 pos)
{
    Managed_Object object = alloc_buffer_markers_on_buffer(app, buffer, 1, 0);
    Marker *marker = (Marker*)managed_object_get_pointer(app, object);
    marker->pos = pos;
    marker->lean_right = false;
    
    i1 next_top = (point_stack.top + 1)%ArrayCount(point_stack.markers);
    if (next_top == point_stack.bot){
        Point_Stack_Slot *slot = &point_stack.markers[point_stack.bot];
        managed_object_free(app, slot->object);
        block_zero_struct(slot);
        point_stack.bot = (point_stack.bot + 1)%ArrayCount(point_stack.markers);
    }
    
    Point_Stack_Slot *slot = &point_stack.markers[point_stack.top];
    slot->buffer = buffer;
    slot->object = object;
    point_stack.top = next_top;
}

function void
point_stack_push_view_cursor(App *app, View_ID view)
{
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    i64 pos = view_get_cursor_pos(app, view);
    point_stack_push(app, buffer, pos);
}

function b32
point_stack_pop(App *app){
    b32 result = false;
    if (point_stack.top != point_stack.bot){
        result = true;
        if (point_stack.top > 0){
            point_stack.top -= 1;
        }
        else{
            point_stack.top = ArrayCount(point_stack.markers) - 1;
        }
        Point_Stack_Slot *slot = &point_stack.markers[point_stack.top];
        managed_object_free(app, slot->object);
        block_zero_struct(slot);
    }
    return(result);
}

function b32
point_stack_read_top(App *app, Buffer_ID *buffer_out, i64 *pos_out){
    b32 result = false;
    if (point_stack.top != point_stack.bot){
        result = true;
        i1 prev_top = point_stack.top;
        if (prev_top > 0){
            prev_top -= 1;
        }
        else{
            prev_top = ArrayCount(point_stack.markers) - 1;
        }
        Point_Stack_Slot *slot = &point_stack.markers[prev_top];
        Managed_Object object = slot->object;
        Marker *marker = (Marker*)managed_object_get_pointer(app, object);
        if (marker != 0){
            *buffer_out = slot->buffer;
            *pos_out = marker->pos;
        }
        else{
            *buffer_out = 0;
            *pos_out = 0;
        }
    }
    return(result);
}

////////////////////////////////

function void
unlock_jump_buffer(void)
{
    locked_buffer.size = 0;
}

function void
lock_jump_buffer(App *app, String8 name)
{
    if (name.size < sizeof(locked_buffer_space))
    {
        block_copy(locked_buffer_space, name.str, name.size);
        locked_buffer = SCu8(locked_buffer_space, name.size);
        Scratch_Block scratch(app);
        String8 escaped = string_escape(scratch, name);
        LogEventF(log_string(app, M), scratch, 0, 0, system_thread_get_id(),
                  "lock jump buffer [name=\"%.*s\"]", string_expand(escaped));
    }
}

function void
lock_jump_buffer(App *app, char *name, i1 size)
{
    lock_jump_buffer(app, SCu8(name, size));
}

function void
lock_jump_buffer(App *app, Buffer_ID buffer_id)
{
    Scratch_Block scratch(app);
    String8 buffer_name = push_buffer_unique_name(app, scratch, buffer_id);
    lock_jump_buffer(app, buffer_name);
}

function Buffer_ID
get_locked_jump_buffer(App *app)
{
    Buffer_ID result = 0;
    if (locked_buffer.size > 0){
        result = get_buffer_by_name(app, locked_buffer, Access_Always);
    }
    if (result == 0)
    {
        unlock_jump_buffer();
    }
    return(result);
}

function View_ID
get_view_for_locked_jump_buffer(App *app)
{
    View_ID result = 0;
    Buffer_ID buffer = get_locked_jump_buffer(app);
    if (buffer != 0)
    {
        result = get_first_view_with_buffer(app, buffer);
    }
    return(result);
}

////////////////////////////////

// TODO(allen): re-evaluate the setup of this.
function void
new_view_settings(App *app, View_ID view){
    b32 use_file_bars = def_get_config_b32(vars_intern_lit("use_file_bars"));
    view_set_setting(app, view, ViewSetting_ShowFileBar, use_file_bars);
}

////////////////////////////////

function void
view_set_passive(App *app, View_ID view_id, b32 value)
{
    Managed_Scope scope = view_get_managed_scope(app, view_id);
    b32 *is_passive = scope_attachment(app, scope, view_is_passive_loc, b32);
    if (is_passive != 0){
        *is_passive = value;
    }
}

function View_ID
get_prev_view_looped_primary_panels(App *app, View_ID start_view_id, Access_Flag access)
{
    View_ID view_id = start_view_id;
    do{
        view_id = get_prev_view_looped_all_panels(app, view_id, access);
        if (!view_is_passive(app, view_id)){
            break;
        }
    }while(view_id != start_view_id);
    return(view_id);
}

////////////////////////////////

function void
call_after_ctx_shutdown(App *app, View_ID view, Custom_Command_Function *func){
    view_enqueue_command_function(app, view, func);
}

function Fallback_Dispatch_Result
fallback_command_dispatch(App *app, Mapping *mapping, Command_Map *map,
                          User_Input *in){
    Fallback_Dispatch_Result result = {};
    if (mapping != 0 && map != 0){
        Command_Binding binding = map_get_binding_recursive(mapping, map, &in->event);
        if (binding.custom != 0){
            Command_Metadata *metadata = get_command_metadata(binding.custom);
            if (metadata != 0){
                if (metadata->is_ui){
                    result.code = FallbackDispatch_DelayedUICall;
                    result.func = binding.custom;
                }
                else{
                    binding.custom(app);
                    result.code = FallbackDispatch_DidCall;
                }
            }
            else{
                binding.custom(app);
                result.code = FallbackDispatch_DidCall;
            }
        }
    }
    return(result);
}

function b32
ui_fallback_command_dispatch(App *app, View_ID view,
                             Mapping *mapping, Command_Map *map, User_Input *in){
    b32 result = false;
    Fallback_Dispatch_Result disp_result =
        fallback_command_dispatch(app, mapping, map, in);
    if (disp_result.code == FallbackDispatch_DelayedUICall){
        call_after_ctx_shutdown(app, view, disp_result.func);
        result = true;
    }
    if (disp_result.code == FallbackDispatch_Unhandled){
        leave_current_input_unhandled(app);
    }
    return(result);
}

function b32
ui_fallback_command_dispatch(App *app, View_ID view, User_Input *in){
    b32 result = false;
    View_Context ctx = view_current_context(app, view);
    if (ctx.mapping != 0){
        Command_Map *map = mapping_get_map(ctx.mapping, ctx.map_id);
        result = ui_fallback_command_dispatch(app, view, ctx.mapping, map, in);
    }
    else{
        leave_current_input_unhandled(app);
    }
    return(result);
}

////////////////////////////////

function void
view_buffer_set(App *app, Buffer_ID *buffers, i64 *positions, i1 count)
{
    if (count > 0)
    {
        Scratch_Block scratch(app);
        
        struct View_Node{
            View_Node *next;
            View_ID view_id;
        };
        
        View_ID active_view_id = get_active_view(app, Access_Always);
        View_ID first_view_id = active_view_id;
        if (view_is_passive(app, active_view_id))
        {
            first_view_id = get_other_primary_view(app, active_view_id, Access_Always, false);
        }
        
        View_ID view_id = first_view_id;
        
        View_Node *primary_view_first = 0;
        View_Node *primary_view_last = 0;
        i1 available_view_count = 0;
        
        primary_view_first = primary_view_last = push_array(scratch, View_Node, 1);
        primary_view_last->next = 0;
        primary_view_last->view_id = view_id;
        available_view_count += 1;
        for (;;)
        {
            view_id = get_other_primary_view(app, view_id, Access_Always, false);
            if (view_id == first_view_id){
                break;
            }
            View_Node *node = push_array(scratch, View_Node, 1);
            primary_view_last->next = node;
            node->next = 0;
            node->view_id = view_id;
            primary_view_last = node;
            available_view_count += 1;
        }
        
        i1 buffer_set_count = clamp_max(count, available_view_count);
        View_Node *node = primary_view_first;
        for (i1 i = 0; i < buffer_set_count; i += 1, node = node->next){
            if (view_set_buffer(app, node->view_id, buffers[i], 0)){
                view_set_cursor_and_preferred_x(app, node->view_id, seek_pos(positions[i]));
            }
        }
    }
}

////////////////////////////////

function void
change_active_primary_view_send_command(App *app, Custom_Command_Function *custom_func)
{
 View_ID view = get_active_view(app, Access_Always);
 view = get_other_primary_view(app, view, Access_Always, true);
 if (view != 0)
 {
  view_set_active(app, view);
 }
 if (custom_func != 0)
 {
  view_enqueue_command_function(app, view, custom_func);
 }
}

// @deprecated
function i1
hax_guess_which_monitor_the_view_is_in(App *app, View_ID view)
{
 rect2 clip = view_get_screen_rect(app, view);
 if ( in_range_inclusive(0, clip.x0, 1500) )
  return 1;
 else 
  return 2;
}

function View_ID
get_next_view_looped_all_panels(App *app, View_ID view_id, Access_Flag access)
{
 view_id = get_view_next(app, view_id, access);
 if (view_id == 0)
 {
  view_id = get_view_next(app, 0, access);
 }
 return(view_id);
}

function void
change_active_monitor(App *app)
{
    View_ID start_view = get_active_view(app, Access_Always);
    i1 start_monitor = hax_guess_which_monitor_the_view_is_in(app, start_view);
    View_ID view = start_view;
    do 
    {
        view = get_next_view_looped_all_panels(app, view, Access_Always);
        if (!view_is_passive(app, view) &&
            hax_guess_which_monitor_the_view_is_in(app, view) != start_monitor)
        {
            break;
        }
    } while (view != start_view);
        
    if (view != start_view)
    {
        view_set_active(app, view);
    }
}

internal void
toggle_split_panel(App *app)
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    View_ID next_view = get_other_primary_view(app, view, Access_Always, false);
    if ( next_view != view )
    {
        global_other_view_buffer = view_get_buffer(app, next_view, Access_Always);
        view_close(app, next_view);
    }
    else
    {
        View_ID new_view = open_view(app, view, ViewSplit_Right);
        new_view_settings(app, new_view);
        view_set_buffer(app, new_view, global_other_view_buffer, 0);
        view_set_active(app, view);
    }
}

internal void
expand_bottom_view(App *app)
{
    Buffer_ID buffer = view_get_buffer(app, global_bottom_view, Access_Always);
    Face_ID face_id = get_face_id(app, buffer);
    Face_Metrics metrics = get_face_metrics(app, face_id);
    view_set_split_pixel_size(app, global_bottom_view, (i1)(metrics.line_height*24.f));
    global_bottom_view_expanded = true;
}

internal void
collapse_bottom_view(App *app)
{
    Buffer_ID buffer = view_get_buffer(app, global_bottom_view, Access_Always);
    Face_ID face_id = get_face_id(app, buffer);
    Face_Metrics metrics = get_face_metrics(app, face_id);
    view_set_split_pixel_size(app, global_bottom_view, (i1)(metrics.line_height*4.f));
    global_bottom_view_expanded = false;
}

internal void
toggle_bottom_view_command(App *app)
{
    if (global_bottom_view_expanded)
    {// NOTE: collapse
        collapse_bottom_view(app);
        if ( view_is_active(app, global_bottom_view) )
        {
            change_active_primary_view(app);
        }
    } 
    else
    {// NOTE: expand
        expand_bottom_view(app);
        view_set_active(app, global_bottom_view);
    }
}

internal void open_panel_vsplit(App *app);
CUSTOM_DOC("Create a new panel by vertically splitting the active panel.")
CUSTOM_COMMAND_SIG(open_panel_vsplit)
{
    View_ID view = get_active_view(app, Access_Always);
    View_ID new_view = open_view(app, view, ViewSplit_Right);
    new_view_settings(app, new_view);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    view_set_buffer(app, new_view, buffer, 0);
}

internal void open_panel_hsplit(App *app);
CUSTOM_DOC("Create a new panel by horizontally splitting the active panel.")
CUSTOM_COMMAND_SIG(open_panel_hsplit)
{
    View_ID view = get_active_view(app, Access_Always);
    View_ID new_view = open_view(app, view, ViewSplit_Bottom);
    new_view_settings(app, new_view);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    view_set_buffer(app, new_view, buffer, 0);
}

////////////////////////////////

// NOTE(allen): Credits to nj/FlyingSolomon for authoring the original version of this helper.

function Buffer_ID
maybe_create_buffer_and_clear_by_name(App *app, String8 name_string, View_ID default_target_view)
{
    Buffer_ID search_buffer = get_buffer_by_name(app, name_string, Access_Always);
    if (search_buffer != 0)
    {
        buffer_set_setting(app, search_buffer, BufferSetting_ReadOnly, true);
        
        View_ID target_view = default_target_view;
        
        View_ID view_with_buffer_already_open = get_first_view_with_buffer(app, search_buffer);
        if (view_with_buffer_already_open != 0)
        {
            target_view = view_with_buffer_already_open;
            // TODO(allen): there needs to be something like
            // view_exit_to_base_context(app, target_view);
            //view_end_ui_mode(app, target_view);
        }
        else
        {
            view_set_buffer(app, target_view, search_buffer, 0);
        }
        
        clear_buffer(app, search_buffer);
        buffer_send_end_signal(app, search_buffer);
    }
    else
    {
        search_buffer = create_buffer(app, name_string, BufferCreate_AlwaysNew);
        buffer_set_setting(app, search_buffer, BufferSetting_Unimportant, true);
        buffer_set_setting(app, search_buffer, BufferSetting_ReadOnly, true);
        view_set_buffer(app, default_target_view, search_buffer, 0);
    }
    
    return(search_buffer);
}

////////////////////////////////

function void
save_all_dirty_buffers_with_postfix(App *app, String postfix){
    ProfileScope(app, "save all dirty buffers");
    Scratch_Block scratch(app);
    for (Buffer_ID buffer = get_buffer_next(app, 0, Access_ReadWriteVisible);
         buffer != 0;
         buffer = get_buffer_next(app, buffer, Access_ReadWriteVisible)){
        Dirty_State dirty = buffer_get_dirty_state(app, buffer);
        if (dirty == DirtyState_UnsavedChanges){
            Temp_Memory temp = begin_temp(scratch);
            String8 filename = push_buffer_filename(app, scratch, buffer);
            if (string_match(string_postfix(filename, postfix.size), postfix)){
                buffer_save(app, buffer, filename, 0);
            }
            end_temp(temp);
        }
    }
}

CUSTOM_COMMAND_SIG(save_all_dirty_buffers)
CUSTOM_DOC("Saves all buffers marked dirty (showing the '*' indicator).")
{
    String empty = {};
    save_all_dirty_buffers_with_postfix(app, empty);
}

////////////////////////////////

function void
set_mouse_suppression(b32 suppress){
    if (suppress){
        suppressing_mouse = true;
        system_show_mouse_cursor(MouseCursorShow_Never);
    }
    else{
        suppressing_mouse = false;
        system_show_mouse_cursor(MouseCursorShow_Always);
    }
}

CUSTOM_COMMAND_SIG(suppress_mouse)
CUSTOM_DOC("Hides the mouse and causes all mosue input (clicks, position, wheel) to be ignored.")
{
    set_mouse_suppression(true);
}

CUSTOM_COMMAND_SIG(allow_mouse)
CUSTOM_DOC("Shows the mouse and causes all mouse input to be processed normally.")
{
    set_mouse_suppression(false);
}

CUSTOM_COMMAND_SIG(toggle_mouse)
CUSTOM_DOC("Toggles the mouse suppression mode, see suppress_mouse and allow_mouse.")
{
    set_mouse_suppression(!suppressing_mouse);
}

CUSTOM_COMMAND_SIG(set_mode_to_original)
CUSTOM_DOC("Sets the edit mode to 4coder original.")
{
    fcoder_mode = FCoderMode_Original;
}

CUSTOM_COMMAND_SIG(set_mode_to_notepad_like)
CUSTOM_DOC("Sets the edit mode to Notepad like.")
{
    begin_notepad_mode(app);
}

CUSTOM_COMMAND_SIG(toggle_highlight_line_at_cursor)
CUSTOM_DOC("Toggles the line highlight at the cursor.")
{
    String_ID key = vars_intern_lit("highlight_line_at_cursor");
    b32 val = def_get_config_b32(key);
    def_set_config_b32(key, !val);
}

CUSTOM_COMMAND_SIG(toggle_highlight_enclosing_scopes)
CUSTOM_DOC("In code files scopes surrounding the cursor are highlighted with distinguishing colors.")
{
    String_ID key = vars_intern_lit("use_scope_highlight");
    b32 val = def_get_config_b32(key);
    def_set_config_b32(key, !val);
}

CUSTOM_COMMAND_SIG(toggle_paren_matching_helper)
CUSTOM_DOC("In code files matching parentheses pairs are colored with distinguishing colors.")
{
    String_ID key = vars_intern_lit("use_paren_helper");
    b32 val = def_get_config_b32(key);
    def_set_config_b32(key, !val);
}

CUSTOM_COMMAND_SIG(toggle_fullscreen)
CUSTOM_DOC("Toggle fullscreen mode on or off.  The change(s) do not take effect until the next frame.")
{
    system_set_fullscreen(!system_is_fullscreen());
}

CUSTOM_COMMAND_SIG(load_themes_default_folder)
CUSTOM_DOC("Loads all the theme files in the default theme folder.")
{
    String fcoder_extension = string_u8_litexpr(".4coder");
    save_all_dirty_buffers_with_postfix(app, fcoder_extension);
    
    Scratch_Block scratch(app);
    String8List list = {};
    def_search_normal_load_list(scratch, &list);
    
    for (String8Node *node = list.first;
         node != 0;
         node = node->next){
        String8 folder_path = node->string;
        String8 themes_path = push_stringfz(scratch, "%.*sthemes", string_expand(folder_path));
        load_folder_of_themes_into_live_set(app, themes_path);
    }
}

CUSTOM_COMMAND_SIG(load_themes_hot_directory)
CUSTOM_DOC("Loads all the theme files in the current hot directory.")
{
    String fcoder_extension = string_u8_litexpr(".4coder");
    save_all_dirty_buffers_with_postfix(app, fcoder_extension);
    
    Scratch_Block scratch(app);
    String path = push_hot_directory(app, scratch);
    load_folder_of_themes_into_live_set(app, path);
}

CUSTOM_COMMAND_SIG(clear_all_themes)
CUSTOM_DOC("Clear the theme list")
{
    if (global_theme_arena.base_allocator == 0){
        global_theme_arena = make_arena_system();
    }
    else{
        arena_clear(&global_theme_arena);
    }
    
    block_zero_struct(&global_theme_list);
    set_default_color_scheme(app);
}

////////////////////////////////

internal void write_text_input(App *app);

function void
setup_essential_mapping(Mapping *mapping, i64 global_id, i64 file_id, i64 code_id){
    MappingScope();
    SelectMapping(mapping);
    
    SelectMap(global_id);
    //BindCore(default_startup, CoreCode_Startup);
    BindCore(default_try_exit, CoreCode_TryExit);
    BindCore(clipboard_record_clip, CoreCode_NewClipboardContents);
    BindMouseWheel(mouse_wheel_scroll);
    BindMouseWheel(mouse_wheel_change_face_size, Key_Code_Control);
    
    SelectMap(file_id);
    ParentMap(global_id);
    BindTextInput(write_text_input);
    BindMouse(click_set_cursor_and_mark, MouseCode_Left);
    BindMouseRelease(click_set_cursor, MouseCode_Left);
    BindCore(click_set_cursor_and_mark, CoreCode_ClickActivateView);
    BindMouseMove(click_set_cursor_if_lbutton);
    
    SelectMap(code_id);
    ParentMap(file_id);
    BindTextInput(write_text_and_auto_indent);
}

function void
default_4coder_initialize(App *app, String_Array filenames, i1 override_font_size, b32 override_hinting)
{
#define M \
"Welcome to " VERSION "\n" \
"If you're new to 4coder there is a built in tutorial\n" \
"Use the key combination [ X Alt ] (on mac [ X Control ])\n" \
"Type in 'hms_demo_tutorial' and press enter\n" \
"\n" \
"Direct bug reports and feature requests to https://github.com/4coder-editor/4coder/issues\n" \
"\n" \
"Other questions and discussion can be directed to editor@4coder.net or 4coder.handmade.network\n" \
"\n" \
"The change log can be found in CHANGES.txt\n" \
"\n"
    print_message(app, string_u8_litexpr(M));
#undef M
    
    Scratch_Block scratch(app);
    
    load_config_and_apply(app, &global_config_arena, override_font_size, override_hinting);
    
    String bindings_filename = string_u8_litexpr("bindings.4coder");
    String mapping = def_get_config_string(scratch, vars_intern_lit("mapping"));
    
    if (string_match(mapping, string_u8_litexpr("mac-default"))){
        bindings_filename = string_u8_litexpr("mac-bindings.4coder");
    }
    else if (OS_MAC && string_match(mapping, string_u8_litexpr("choose"))){
        bindings_filename = string_u8_litexpr("mac-bindings.4coder");
    }
    
    // TODO(allen): cleanup
    String_ID global_map_id = vars_intern_lit("keys_global");
    String_ID file_map_id = vars_intern_lit("keys_file");
    String_ID code_map_id = vars_intern_lit("keys_code");
    
    if (dynamic_binding_load_from_file(app, &framework_mapping, bindings_filename))
    {
        setup_essential_mapping(&framework_mapping, global_map_id, file_map_id, code_map_id);
    }
    
    // open command line files
    String hot_directory = push_hot_directory(app, scratch);
    for (i1 i = 0; i < filenames.count; i += 1){
        Temp_Memory_Block temp(scratch);
        String input_name = filenames.vals[i];
        String full_name = push_stringfz(scratch, "%.*s/%.*s",
                                                    string_expand(hot_directory),
                                                    string_expand(input_name));
        Buffer_ID new_buffer = create_buffer(app, full_name, BufferCreate_NeverNew|BufferCreate_MustAttachToFile);
        if (new_buffer == 0){
            create_buffer(app, input_name, 0);
        }
    }
}

function void
default_4coder_initialize(App *app, i1 override_font_size, b32 override_hinting){
    String_Array filenames = {};
    default_4coder_initialize(app, filenames, override_font_size, override_hinting);
}

function void
default_4coder_initialize(App *app, String_Array filenames){
    Face_Description description = get_face_description(app, 0);
    default_4coder_initialize(app, filenames,
                              description.parameters.pt_size,
                              description.parameters.hinting);
}

function void
default_4coder_initialize(App *app){
    Face_Description command_line_description = get_face_description(app, 0);
    String_Array filenames = {};
    default_4coder_initialize(app, filenames, command_line_description.parameters.pt_size, command_line_description.parameters.hinting);
}

function void
default_4coder_side_by_side_panels(App *app,
                                   Buffer_Identifier left, Buffer_Identifier right){
    Buffer_ID left_id = buffer_identifier_to_id(app, left);
    Buffer_ID right_id = buffer_identifier_to_id(app, right);
    
    // Left Panel
    View_ID view = get_active_view(app, Access_Always);
    new_view_settings(app, view);
    view_set_buffer(app, view, left_id, 0);
    
    // Right Panel
    open_panel_vsplit(app);
    View_ID right_view = get_active_view(app, Access_Always);
    view_set_buffer(app, right_view, right_id, 0);
    
    // Restore Active to Left
    view_set_active(app, view);
}

function void
default_4coder_side_by_side_panels(App *app,
                                   Buffer_Identifier left, Buffer_Identifier right,
                                   String_Array filenames){
    if (filenames.count > 0){
        left = buffer_identifier(filenames.vals[0]);
        if (filenames.count > 1){
            right = buffer_identifier(filenames.vals[1]);
        }
    }
    default_4coder_side_by_side_panels(app, left, right);
}

function void
default_4coder_side_by_side_panels(App *app, String_Array filenames){
    Buffer_Identifier left = buffer_identifier(string_u8_litexpr("*scratch*"));
    Buffer_Identifier right = buffer_identifier(string_u8_litexpr("*messages*"));
    default_4coder_side_by_side_panels(app, left, right, filenames);
}

function void
default_4coder_side_by_side_panels(App *app){
    String_Array filenames = {};
    default_4coder_side_by_side_panels(app, filenames);
}

function void
default_4coder_one_panel(App *app, Buffer_Identifier buffer){
    Buffer_ID id = buffer_identifier_to_id(app, buffer);
    View_ID view = get_active_view(app, Access_Always);
    new_view_settings(app, view);
    view_set_buffer(app, view, id, 0);
}

function void
default_4coder_one_panel(App *app, String_Array filenames){
    Buffer_Identifier buffer = buffer_identifier(string_u8_litexpr("*messages*"));
    if (filenames.count > 0){
        buffer = buffer_identifier(filenames.vals[0]);
    }
    default_4coder_one_panel(app, buffer);
}

function void
default_4coder_one_panel(App *app){
    String_Array filenames = {};
    default_4coder_one_panel(app, filenames);
}

////////////////////////////////

function void
buffer_modified_set_init(void){
    Buffer_Modified_Set *set = &global_buffer_modified_set;
    
    block_zero_struct(set);
    Base_Allocator *allocator = get_base_allocator_system();
    set->arena = make_arena(allocator);
    set->id_to_node = make_table_u64_u64(allocator, 100);
}

function Buffer_Modified_Node*
buffer_modified_set__alloc_node(Buffer_Modified_Set *set){
    Buffer_Modified_Node *result = set->free;
    if (result == 0){
        result = push_array(&set->arena, Buffer_Modified_Node, 1);
    }
    else{
        sll_stack_pop(set->free);
    }
    return(result);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-null-pointer-arithmetic"
#pragma clang diagnostic ignored "-Wnull-pointer-subtraction"

function void
buffer_mark_as_modified(Buffer_ID buffer){
    Buffer_Modified_Set *set = &global_buffer_modified_set;
    
    Table_Lookup lookup = table_lookup(&set->id_to_node, (u64)buffer);
    if (!lookup.found_match){
        Buffer_Modified_Node *node = buffer_modified_set__alloc_node(set);
        zdll_push_back(set->first, set->last, node);
        node->buffer = buffer;
        table_insert(&set->id_to_node, (u64)buffer, (u64)PtrAsInt(node));
    }
}

function void
buffer_unmark_as_modified(Buffer_ID buffer){
    Buffer_Modified_Set *set = &global_buffer_modified_set;
    
    Table_Lookup lookup = table_lookup(&set->id_to_node, (u64)buffer);
    if (lookup.found_match){
        u64 val = 0;
        table_read(&set->id_to_node, (u64)buffer, &val);
        Buffer_Modified_Node *node = (Buffer_Modified_Node*)IntAsPtr(val);
        zdll_remove(set->first, set->last, node);
        table_erase(&set->id_to_node, lookup);
        sll_stack_push(set->free, node);
    }
}

#pragma clang diagnostic pop

function void
buffer_modified_set_clear(void){
    Buffer_Modified_Set *set = &global_buffer_modified_set;
    
    table_clear(&set->id_to_node);
    if (set->last != 0){
        set->last->next = set->free;
        set->free = set->first;
        set->first = 0;
        set->last = 0;
    }
}

////////////////////////////////

function Fade_Range*
alloc_fade_range(void){
    Fade_Range *result = free_fade_ranges;
    if (result == 0){
        result = push_array(&fade_range_arena, Fade_Range, 1);
    }
    else{
        sll_stack_pop(free_fade_ranges);
    }
    block_zero_struct(result);
    return(result);
}

function void
free_fade_range(Fade_Range *range){
    sll_stack_push(free_fade_ranges, range);
}

function Fade_Range*
buffer_post_fade(App *app, Buffer_ID buffer_id, f32 seconds, Range_i64 range, ARGB_Color color){
    Fade_Range *fade_range = alloc_fade_range();
    sll_queue_push(buffer_fade_ranges.first, buffer_fade_ranges.last, fade_range);
    buffer_fade_ranges.count += 1;
    fade_range->buffer_id = buffer_id;
    fade_range->t = seconds;
    fade_range->full_t = seconds;
    fade_range->range = range;
    fade_range->color = color;
    return(fade_range);
}

function void
buffer_shift_fade_ranges(Buffer_ID buffer_id, i64 shift_after_p, i64 shift_amount){
    for (Fade_Range *node = buffer_fade_ranges.first;
         node != 0;
         node = node->next){
        if (node->buffer_id == buffer_id){
            if (node->range.min >= shift_after_p){
                node->range.min += shift_amount;
                node->range.max += shift_amount;
            }
            else if (node->range.max >= shift_after_p){
                node->range.max += shift_amount;
            }
        }
    }
}

function b32
tick_all_fade_ranges(App *app, f32 t){
    Fade_Range **prev_next = &buffer_fade_ranges.first;
    for (Fade_Range *node = buffer_fade_ranges.first, *next = 0;
         node != 0;
         node = next){
        next = node->next;
        node->t -= t;
        if (node->t <= 0.f){
            if (node->finish_call != 0){
                node->finish_call(app, node);
            }
            *prev_next = next;
            free_fade_range(node);
            buffer_fade_ranges.count -= 1;
        }
        else{
            prev_next = &node->next;
            buffer_fade_ranges.last = node;
        }
    }
    return(buffer_fade_ranges.count > 0);
}

function void
paint_fade_ranges(App *app, Text_Layout_ID layout, Buffer_ID buffer){
    for (Fade_Range *node = buffer_fade_ranges.first;
         node != 0;
         node = node->next){
        if (node->buffer_id == buffer){
            f32 blend = node->t/node->full_t;
            if (node->negate_fade_direction){
                blend = 1.f - blend; 
            }
            paint_text_color_blend(app, layout, node->range, node->color, blend);
        }
    }
}

////////////////////////////////

function void
clipboard_init_empty(Clipboard *clipboard, u32 history_depth)
{
    history_depth = clamp_min(1, history_depth);
    heap_init(&clipboard->heap, &clipboard->arena);
    clipboard->clip_index = 0;
    clipboard->clip_capacity = history_depth;
    clipboard->clips = push_array_zero(&clipboard->arena, String, history_depth);
}

function void
clipboard_init(Base_Allocator *allocator, u32 history_depth, Clipboard *clipboard_out){
    u64 memsize = sizeof(String)*history_depth;
    memsize = round_up_u64(memsize, KB(4));
    clipboard_out->arena = make_arena(allocator, memsize, 8);
    clipboard_init_empty(clipboard_out, history_depth);
}

function void
clipboard_clear(Clipboard *clipboard)
{
    arena_clear(&clipboard->arena);
    clipboard_init_empty(clipboard, clipboard->clip_capacity);
}

function String8
clipboard_post_internal_only(Clipboard *clipboard, String8 string)
{
    u32 rolled_index = clipboard->clip_index % clipboard->clip_capacity;
    clipboard->clip_index += 1;
    String8 *slot = &clipboard->clips[rolled_index];
    if (slot->str != 0)
    {
        if (slot->size < string.size ||
            (slot->size - string.size) > KB(1)){
            heap_free(&clipboard->heap, slot->str);
            goto alloc_new;
        }
    }
    else
    {
        alloc_new:;
        u8 *new_buf = (u8*)heap_allocate(&clipboard->heap, string.size);
        slot->str = new_buf;
    }
    block_copy(slot->str, string.str, string.size);
    slot->size = string.size;
    return(*slot);
}

function u32
clipboard_count(Clipboard *clipboard)
{
    u32 result = clipboard->clip_index;
    result = clamp_max(result, clipboard->clip_capacity);
    return(result);
}

function String8
get_clipboard_index(Clipboard *clipboard, u32 item_index)
{
    String8 result = {};
    u32 top = Min(clipboard->clip_index, clipboard->clip_capacity);
    
    item_index = item_index % top;
    // NOTE(kv): item_index is BACKWARD from the array index
    i1 array_index = ((clipboard->clip_index - 1) - item_index) % top;
    result = clipboard->clips[array_index];
    
    return(result);
}

function String8
push_clipboard_index_inner(Arena *arena, Clipboard *clipboard, i1 item_index)
{
    String8 result = get_clipboard_index(clipboard, item_index);
    result = push_string_copyz(arena, result);
    return(result);
}

////////////////////////////////

function void
clipboard_clear(i1 clipboard_id){
    clipboard_clear(&global_clipboard0);
}

function String
clipboard_post_internal_only(i1 clipboard_id, String string)
{
    return(clipboard_post_internal_only(&global_clipboard0, string));
}

function void
clipboard_post(i1 clipboard_id, String8 string)
{
    clipboard_post_internal_only(clipboard_id, string);
    system_post_clipboard(string, clipboard_id);
}

function i1
clipboard_count(i1 clipboard_id)
{
    return(clipboard_count(&global_clipboard0));
}

function String8
push_clipboard_index_inner(Arena *arena, i1 clipboard_id, i1 item_index)
{
    return(push_clipboard_index_inner(arena, &global_clipboard0, item_index));
}


////////////////////////////////

function void
initialize_managed_id_metadata(App *app);

function void
default_framework_init(App *app)
{
    Thread_Context *tctx = get_thread_context(app);
    async_task_handler_init(app, &global_async_system);
    clipboard_init(get_base_allocator_system(), /*history_depth*/ 64, &global_clipboard0);
    code_index_init();
    buffer_modified_set_init();
    Profile_Global_List *list = get_core_profile_list(app);
    ProfileThreadName(tctx, list, string_u8_litexpr("main"));
    initialize_managed_id_metadata(app);
    set_default_color_scheme(app);
    heap_init(&global_heap, tctx->allocator);
	global_permanent_arena = make_arena_system();
    global_frame_arena     = make_arena_system();
    global_config_arena    = make_arena_system();
    fade_range_arena       = make_arena_system(KB(8));
}

////////////////////////////////

function void
default_input_handler_init(App *app, Arena *arena)
{
    Thread_Context *tctx = get_thread_context(app);
    
    View_ID view = get_this_ctx_view(app, Access_Always);
    String8 name = push_stringfz(arena, "view %d", view);
    
    Profile_Global_List *list = get_core_profile_list(app);
    ProfileThreadName(tctx, list, name);
    
    View_Context ctx = view_current_context(app, view);
    ctx.mapping = &framework_mapping;
    ctx.map_id = vars_intern_lit("keys_global");
    view_alter_context(app, view, &ctx);
}

function Command_Map_ID
default_get_map_id(App *app, View_ID view){
    Command_Map_ID result = 0;
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    Managed_Scope buffer_scope = buffer_get_managed_scope(app, buffer);
    Command_Map_ID *result_ptr = scope_attachment(app, buffer_scope, buffer_map_id, Command_Map_ID);
    if (result_ptr != 0){
        if (*result_ptr == 0){
            *result_ptr = vars_intern_lit("keys_file");
        }
        result = *result_ptr;
    }
    else{
        result = vars_intern_lit("keys_global");
    }
    return(result);
}

function void
set_next_rewrite(App *app, View_ID view, Rewrite_Type rewrite){
    Managed_Scope scope = view_get_managed_scope(app, view);
    Rewrite_Type *next_rewrite = scope_attachment(app, scope, view_next_rewrite_loc, Rewrite_Type);
    if (next_rewrite != 0){
        *next_rewrite = rewrite;
    }
}

function void
default_pre_command(App *app, Managed_Scope scope)
{
    Rewrite_Type *next_rewrite =
        scope_attachment(app, scope, view_next_rewrite_loc, Rewrite_Type);
    *next_rewrite = Rewrite_None;
    if (fcoder_mode == FCoderMode_NotepadLike)
    {
        for (View_ID view_it = get_view_next(app, 0, Access_Always);
             view_it != 0;
             view_it = get_view_next(app, view_it, Access_Always))
        {
            Managed_Scope scope_it = view_get_managed_scope(app, view_it);
            b32 *snap_mark_to_cursor =
                scope_attachment(app, scope_it, view_snap_mark_to_cursor,
                                 b32);
            *snap_mark_to_cursor = true;
        }
    }
}

function void
default_post_command(App *app, Managed_Scope scope)
{
    Rewrite_Type *next_rewrite = scope_attachment(app, scope, view_next_rewrite_loc, Rewrite_Type);
    if (next_rewrite != 0){
        if (*next_rewrite != Rewrite_NoChange){
            Rewrite_Type *rewrite =
                scope_attachment(app, scope, view_rewrite_loc, Rewrite_Type);
            *rewrite = *next_rewrite;
        }
    }
    if (fcoder_mode == FCoderMode_NotepadLike){
        for (View_ID view_it = get_view_next(app, 0, Access_Always);
             view_it != 0;
             view_it = get_view_next(app, view_it, Access_Always)){
            Managed_Scope scope_it = view_get_managed_scope(app, view_it);
            b32 *snap_mark_to_cursor =
                scope_attachment(app, scope_it, view_snap_mark_to_cursor, b32);
            if (*snap_mark_to_cursor){
                i64 pos = view_get_cursor_pos(app, view_it);
                view_set_mark(app, view_it, seek_pos(pos));
            }
        }
    }
}

