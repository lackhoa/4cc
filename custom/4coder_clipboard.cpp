/*
4coder_clipboard.cpp - Copy paste commands and clipboard related setup.
*/

// TOP

CUSTOM_COMMAND_SIG(clipboard_record_clip)
CUSTOM_DOC("In response to a new clipboard contents events, saves the new clip onto the clipboard history")
{
    User_Input in = get_current_input(app);
    if (in.event.kind == InputEventKind_Core &&
        in.event.core.code == CoreCode_NewClipboardContents){
        clipboard_post_function_only(0, in.event.core.string);
    }
}

////////////////////////////////

function b32
clipboard_post_buffer_range(App *app, i1 clipboard_index, Buffer_ID buffer, Range_i64 range)
{
    b32 success = false;
    Scratch_Block scratch(app);
    String string = push_buffer_range(app, scratch, buffer, range);
    if (string.size > 0){
        clipboard_post(clipboard_index, string);
        success = true;
    }
    return(success);
}

function b32
clipboard_update_history_from_system(App *app, i1 clipboard_id)
{
    Scratch_Block scratch(app);
    b32 result = false;
    String8 string = system_get_clipboard(scratch, clipboard_id);
    if (string.str != 0)
    {
        clipboard_post_function_only(clipboard_id, string);
        result = true;
    }
    return(result);
}

global List_String clipboard_collection_list = {};


CUSTOM_COMMAND_SIG(copy)
CUSTOM_DOC("Copy the text in the range from the cursor to the mark onto the clipboard.")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    Range_i64 range = get_view_range(app, view);
    clipboard_post_buffer_range(app, 0, buffer, range);
}

CUSTOM_COMMAND_SIG(cut)
CUSTOM_DOC("Cut the text in the range from the cursor to the mark onto the clipboard.")
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    Range_i64 range = get_view_range(app, view);
    if (clipboard_post_buffer_range(app, 0, buffer, range)){
        buffer_replace_range(app, buffer, range, empty_string);
    }
}

CUSTOM_COMMAND_SIG(paste)
CUSTOM_DOC("At the cursor, insert the text at the top of the clipboard.")
{
    clipboard_update_history_from_system(app, 0);
    i1 count = clipboard_count(0);
    if (count > 0)
    {
        View_ID view = get_active_view(app, Access_ReadWriteVisible);
        if_view_has_highlighted_range_delete_range(app, view);
        
        set_next_rewrite(app, view, Rewrite_Paste);
        
        Managed_Scope scope = view_get_managed_scope(app, view);
        i1 *paste_index = scope_attachment(app, scope, view_paste_index_loc, i1);
        if (paste_index != 0)
        {
            *paste_index = 0;
            
            Scratch_Block scratch(app);
            
            String8 string = push_clipboard_index_inner(scratch, 0, *paste_index);
            if (string.size > 0)
            {
                Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
                
                i64 pos = view_get_cursor_pos(app, view);
                buffer_replace_range(app, buffer, Ii64(pos), string);
                view_set_mark(app, view, seek_pos(pos));
                view_set_cursor_and_preferred_x(app, view, seek_pos(pos + (i1)string.size));
                
                ARGB_Color argb = fcolor_resolve(fcolor_id(defcolor_paste));
                buffer_post_fade(app, buffer, 0.667f, Ii64_size(pos, string.size), argb);
            }
        }
    }
}

CUSTOM_COMMAND_SIG(paste_next)
CUSTOM_DOC("If the previous command was paste or paste_next, replaces the paste range with the next text down on the clipboard, otherwise operates as the paste command.")
{
    Scratch_Block scratch(app);
    
    b32 new_clip = clipboard_update_history_from_system(app, 0);
    
    i1 count = clipboard_count(0);
    if (count > 0)
    {
        View_ID view = get_active_view(app, Access_ReadWriteVisible);
        Managed_Scope scope = view_get_managed_scope(app, view);
        
        Rewrite_Type *rewrite = scope_attachment(app, scope, view_rewrite_loc, Rewrite_Type);
        if (rewrite != 0)
        {
            if (*rewrite == Rewrite_Paste && !new_clip){
                no_mark_snap_to_cursor(app, scope);
                
                set_next_rewrite(app, view, Rewrite_Paste);
                
                i1 *paste_index_ptr = scope_attachment(app, scope, view_paste_index_loc, i1);
                i1 paste_index = (*paste_index_ptr) + 1;
                *paste_index_ptr = paste_index;
                
                String string = push_clipboard_index_inner(scratch, 0, paste_index);
                
                Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
                
                Range_i64 range = get_view_range(app, view);
                i64 pos = range.min;
                
                buffer_replace_range(app, buffer, range, string);
                view_set_cursor_and_preferred_x(app, view, seek_pos(pos + string.size));
                
                ARGB_Color argb = fcolor_resolve(fcolor_id(defcolor_paste));
                buffer_post_fade(app, buffer, 0.667f, Ii64_size(pos, string.size), argb);
            }
            else{
                paste(app);
            }
        }
    }
}

////////////////////////////////

CUSTOM_COMMAND_SIG(multi_paste)
CUSTOM_DOC("Paste multiple entries from the clipboard at once")
{
    Scratch_Block scratch(app);
    
    i1 count = clipboard_count(0);
    if (count > 0)
    {
        View_ID view = get_active_view(app, Access_ReadWriteVisible);
        Managed_Scope scope = view_get_managed_scope(app, view);
        
        Rewrite_Type *rewrite = scope_attachment(app, scope, view_rewrite_loc, Rewrite_Type);
        if (rewrite != 0)
        {
            if (*rewrite == Rewrite_Paste){
                Rewrite_Type *next_rewrite = scope_attachment(app, scope, view_next_rewrite_loc, Rewrite_Type);
                *next_rewrite = Rewrite_Paste;
                i1 *paste_index_ptr = scope_attachment(app, scope, view_paste_index_loc, i1);
                i1 paste_index = (*paste_index_ptr) + 1;
                *paste_index_ptr = paste_index;
                
                String string = push_clipboard_index_inner(scratch, 0, paste_index);
                
                String insert_string = push_stringfz(scratch, "\n%.*s", string_expand(string));
                
                Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
                Range_i64 range = get_view_range(app, view);
                buffer_replace_range(app, buffer, Ii64(range.max), insert_string);
                view_set_mark(app, view, seek_pos(range.max + 1));
                view_set_cursor_and_preferred_x(app, view, seek_pos(range.max + insert_string.size));
                
                ARGB_Color argb = fcolor_resolve(fcolor_id(defcolor_paste));
                buffer_post_fade(app, buffer, 0.667f, Ii64(range.max + 1, range.max + insert_string.size), argb);
            }
            else{
                paste(app);
            }
        }
    }
}

function Range_i64
multi_paste_range(App *app, View_ID view, Range_i64 range, i1 paste_count, b32 old_to_new){
    Scratch_Block scratch(app);
    
    Range_i64 finish_range = range;
    if (paste_count >= 1){
        Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
        if (buffer != 0){
            i64 total_size = 0;
            for (i1 paste_index = 0; paste_index < paste_count; ++paste_index){
                Temp_Memory temp = begin_temp(scratch);
                String8 string = push_clipboard_index_inner(scratch, 0, paste_index);
                total_size += string.size + 1;
                end_temp(temp);
            }
            total_size -= 1;
            
            i1 first = paste_count - 1;
            i1 one_past_last = -1;
            i1 step = -1;
            if (!old_to_new){
                first = 0;
                one_past_last = paste_count;
                step = 1;
            }
            
            List_String list = {};
            
            for (i1 paste_index = first; paste_index != one_past_last; paste_index += step){
                if (paste_index != first){
                    string_list_push(scratch, &list, SCu8("\n", 1));
                }
                String string = push_clipboard_index_inner(scratch, 0, paste_index);
                if (string.size > 0){
                    string_list_push(scratch, &list, string);
                }
            }
            
            String flattened = string_list_flatten(scratch, list);
            
            buffer_replace_range(app, buffer, range, flattened);
            i64 pos = range.min;
            finish_range.min = pos;
            finish_range.max = pos + total_size;
            view_set_mark(app, view, seek_pos(finish_range.min));
            view_set_cursor_and_preferred_x(app, view, seek_pos(finish_range.max));
            
            ARGB_Color argb = fcolor_resolve(fcolor_id(defcolor_paste));
            buffer_post_fade(app, buffer, 0.667f, finish_range, argb);
        }
    }
    return(finish_range);
}

function void
multi_paste_interactive_up_down(App *app, i1 paste_count, i1 clip_count){
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    i64 pos = view_get_cursor_pos(app, view);
    b32 old_to_new = true;
    Range_i64 range = multi_paste_range(app, view, Ii64(pos), paste_count, old_to_new);
    
    Query_Bar_Group group(app);
    Query_Bar bar = {};
    bar.prompt = strlit("Up and Down to condense and expand paste stages; R to reverse order; Return to finish; Escape to abort.");
    if (start_query_bar(app, &bar, 0) == 0) return;
    
    User_Input in = {};
    for (;;){
        in = get_next_input(app, EventProperty_AnyKey, EventProperty_Escape);
        if (in.abort) break;
        
        b32 did_modify = false;
        if (match_key_code(&in, Key_Code_Up)){
            if (paste_count > 1){
                --paste_count;
                did_modify = true;
            }
        }
        else if (match_key_code(&in, Key_Code_Down)){
            if (paste_count < clip_count){
                ++paste_count;
                did_modify = true;
            }
        }
        else if (match_key_code(&in, Key_Code_R)){
            old_to_new = !old_to_new;
            did_modify = true;
        }
        else if (match_key_code(&in, Key_Code_Return)){
            break;
        }
        
        if (did_modify){
            range = multi_paste_range(app, view, range, paste_count, old_to_new);
        }
    }
    
    if (in.abort){
        Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
        buffer_replace_range(app, buffer, range, SCu8(""));
    }
}

CUSTOM_COMMAND_SIG(multi_paste_interactive)
CUSTOM_DOC("Paste multiple lines from the clipboard history, controlled with arrow keys")
{
    i1 clip_count = clipboard_count(0);
    if (clip_count > 0){
        multi_paste_interactive_up_down(app, 1, clip_count);
    }
}

CUSTOM_COMMAND_SIG(multi_paste_interactive_quick)
CUSTOM_DOC("Paste multiple lines from the clipboard history, controlled by inputing the number of lines to paste")
{
    i1 clip_count = clipboard_count(0);
    if (clip_count > 0){
        u8 string_space[256];
        Query_Bar_Group group(app);
        Query_Bar bar = {};
        bar.prompt = strlit("How Many Slots To Paste: ");
        bar.string = SCu8(string_space, (u64)0);
        bar.string_capacity = sizeof(string_space);
        query_user_number(app, &bar);
        
        i1 initial_paste_count = (i1)string_to_u64(bar.string, 10);
        initial_paste_count = clamp_between(1, initial_paste_count, clip_count);
        end_query_bar(app, &bar, 0);
        
        multi_paste_interactive_up_down(app, initial_paste_count, clip_count);
    }
}

////////////////////////////////

#if FCODER_TRANSITION_TO < 4001004
function void
clipboard_clear(App *app, i1 clipboard_id){
    clipboard_clear(clipboard_id);
}
function void
clipboard_post(App *app, i1 clipboard_id, String8 string)
{
    clipboard_post(clipboard_id, string);
}
function i1
clipboard_count(App *app, i1 clipboard_id)
{
    return(clipboard_count(clipboard_id));
}
function String8
push_clipboard_index(App *app, Arena *arena, i1 clipboard_id, i1 item_index)
{
    return push_clipboard_index_inner(arena, clipboard_id, item_index);
}
#endif

function void
clipboard_pop(App *app, i1 clipboard_id)
{// TODO(kv): My code, too lazy (and ignorant) to preserve the API
    // @Experiment Watch out for the system clipboard
    
    Clipboard *clipboard = &global_clipboard0;
    
    clipboard_update_history_from_system(app, clipboard_id);
    
    if (clipboard->clip_index > 1)
    {
        clipboard->clip_index -= 1;
    }
}


// BOTTOM
