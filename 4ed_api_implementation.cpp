/*
The implementation for the custom API
*/

// TOP

inline b32
access_test(u32 lock_flags, u32 access_flags){
    b32 result = 0;
    
    if ((lock_flags & ~access_flags) == 0){
        result = 1;
    }
    
    return(result);
}

internal void
fill_buffer_summary(Buffer_Summary *buffer, Editing_File *file, Working_Set *working_set){
    *buffer = buffer_summary_zero();
    if (!file->is_dummy){
        buffer->exists = 1;
        buffer->ready = file_is_ready(file);
        
        buffer->is_lexed = file->settings.tokens_exist;
        buffer->buffer_id = file->id.id;
        buffer->size = file->state.buffer.size;
        buffer->buffer_cursor_pos = file->state.cursor_pos;
        
        buffer->file_name_len = file->name.source_path.size;
        buffer->buffer_name_len = file->name.live_name.size;
        buffer->file_name = file->name.source_path.str;
        buffer->buffer_name = file->name.live_name.str;
        
        buffer->map_id = file->settings.base_map_id;
        
        buffer->lock_flags = 0;
        if (file->settings.read_only){
            buffer->lock_flags |= AccessProtected;
        }
    }
}

internal void
fill_buffer_summary(Buffer_Summary *buffer, Editing_File *file, Command_Data *cmd){
    Working_Set *working_set = &cmd->models->working_set;
    fill_buffer_summary(buffer, file, working_set);
}

internal void
fill_view_summary(View_Summary *view, View *vptr, Live_Views *live_set, Working_Set *working_set){
    int buffer_id;
    File_Viewing_Data *data = &vptr->file_data;
    
    *view = view_summary_zero();
    
    if (vptr->in_use){
        view->exists = 1;
        view->view_id = (int)(vptr - live_set->views) + 1;
        view->line_height = (float)(vptr->line_height);
        view->unwrapped_lines = vptr->file_data.unwrapped_lines;
        view->show_whitespace = vptr->file_data.show_whitespace;
        view->lock_flags = view_lock_flags(vptr);
        
        if (data->file){
            buffer_id = vptr->file_data.file->id.id;
            
            view->buffer_id = buffer_id;
            
            view->mark = view_compute_cursor_from_pos(vptr, vptr->recent->mark);
            view->cursor = vptr->recent->cursor;
            view->preferred_x = vptr->recent->preferred_x;
            
            view->file_region = vptr->file_region;
            view->scroll_vars = *vptr->current_scroll;
        }
    }
}

inline void
fill_view_summary(View_Summary *view, View *vptr, Command_Data *cmd){
    fill_view_summary(view, vptr, &cmd->vars->live_set, &cmd->models->working_set);
}

internal Editing_File*
get_file_from_identifier(System_Functions *system, Working_Set *working_set, Buffer_Identifier buffer){
    i32 buffer_id = buffer.id;
    i32 buffer_name_len = buffer.name_len;
    char *buffer_name = buffer.name;
    
    Editing_File *file = 0;
    
    if (buffer_id){
        file = working_set_get_active_file(working_set, buffer_id);
    }
    else if (buffer_name){
        file = working_set_contains(system, working_set, make_string(buffer_name, buffer_name_len));
    }
    
    return(file);
}

internal Editing_File*
imp_get_file(Command_Data *cmd, Buffer_Summary *buffer){
    Editing_File *file = 0;
    Working_Set *working_set = &cmd->models->working_set;;
    
    if (buffer->exists){
        file = working_set_get_active_file(working_set, buffer->buffer_id);
        if (file != 0 && !file_is_ready(file)){
            file = 0;
        }
    }
    
    return(file);
}

internal View*
imp_get_view(Command_Data *cmd, int view_id){
    Live_Views *live_set = cmd->live_set;
    View *vptr = 0;
    
    view_id = view_id - 1;
    if (view_id >= 0 && view_id < live_set->max){
        vptr = live_set->views + view_id;
    }
    
    return(vptr);
}

internal View*
imp_get_view(Command_Data *cmd, View_Summary *view){
    View *vptr = 0;
    
    if (view->exists){
        vptr = imp_get_view(cmd, view->view_id);
    }
    
    return(vptr);
}

EXEC_COMMAND_SIG(external_exec_command)/*
DOC_PARAM(command_id, an integer id enumerated in 4coder_custom.h starting with cmdid)
DOC(Executes the command associated with the command_id passed in)
*/{
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    Command_Function function = command_table[command_id];
    Command_Binding binding = {};
    binding.function = function;
    if (function) function(cmd->system, cmd, binding);
    
    update_command_data(cmd->vars, cmd);
}

// TODO(allen): This is a bit of a mess and needs to be fixed soon
EXEC_SYSTEM_COMMAND_SIG(external_exec_system_command)/*
DOC_PARAM(view, the target view that will display the output buffer, may be NULL, see description for details)
DOC_PARAM(buffer, a buffer identifier for the buffer that will be filled with the output from the command)
DOC_PARAM(path, the path from which the command is executed)
DOC_PARAM(path_len, the length of the path string)
DOC_PARAM(command, the command to be executed)
DOC_PARAM(command_len, the length of the command string)
DOC_PARAM(flags, may be zero or one or more CLI flags ORed together)
DOC_RETURN(returns non-zero if the command is successfully started, returns zero otherwise)
DOC
(
Executes a system command as if called from the command line, and sends the output to a buffer. The buffer
identifier can either name a new buffer that does not exist, name a buffer that does exist, or provide the
id of a buffer that does exist.  If the buffer already exists the command will fail, unless
CLI_OverlapWithConflict is set in the flags.

If the buffer is not already in an open view, and the view parameter is no NULL, then the provided view
will display the output buffer.  If the view parameter is NULL, no view will display the output.

If CLI_OverlapWithConflict is set in the flags, the command will always be executed even if another command
was outputting to the same buffer still.

If CLI_AlwaysBindToView is set and the view parameter is not NULL, then the specified view will always
begin displaying the output buffer, even if another open view already displays that buffer.

If CLI_CursorAtEnd is set the cursor in the output buffer will be placed at the end of the buffer instead
of at the beginning.
)
*/{
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    System_Functions *system = cmd->system;
    App_Vars *vars = cmd->vars;
    Models *models = cmd->models;
    
    char feedback_space[256];
    String feedback_str = make_fixed_width_string(feedback_space);
    
    Working_Set *working_set = &models->working_set;
    CLI_Process *procs = vars->cli_processes.procs, *proc = 0;
    Editing_File *file = 0;
    b32 bind_to_new_view = true;
    General_Memory *general = &models->mem.general;
    
    Partition *part = &models->mem.part;
    Temp_Memory temp = begin_temp_memory(part);
    
    int result = true;
    
    View *vptr = imp_get_view(cmd, view);
    
    if (vars->cli_processes.count < vars->cli_processes.max){
        file = get_file_from_identifier(system, working_set, buffer);
        if (file){
            if (file->settings.read_only == 0){
                append(&feedback_str, "ERROR: ");
                append(&feedback_str, file->name.live_name);
                append(&feedback_str, " is not a read-only buffer\n");
                do_feedback_message(system, models, feedback_str);
                result = false;
                goto done;
            }
            if (file->settings.never_kill){
                append(&feedback_str, "The buffer ");
                append(&feedback_str, file->name.live_name);
                append(&feedback_str, " is not killable");
                do_feedback_message(system, models, feedback_str);
                result = false;
                goto done;
            }
        }
        else if (buffer.name){
            file = working_set_alloc_always(working_set, general);
            if (file == 0){
                append(&feedback_str, "ERROR: unable to  allocate a new buffer\n");
                do_feedback_message(system, models, feedback_str);
                result = false;
                goto done;
            }
            file_create_read_only(system, models, file, buffer.name);
            working_set_add(system, working_set, file, general);
        }
        
        if (file){
            i32 proc_count = vars->cli_processes.count;
            for (i32 i = 0; i < proc_count; ++i){
                if (procs[i].out_file == file){
                    if (flags & CLI_OverlapWithConflict){
                        procs[i].out_file = 0;
                    }
                    else{
                        file = 0;
                    }
                    break;
                }
            }
            
            if (file){
                file_clear(system, models, file, 1);
                file->settings.unimportant = 1;
                
                if (!(flags & CLI_AlwaysBindToView)){
                    View_Iter iter = file_view_iter_init(&models->layout, file, 0);
                    if (file_view_iter_good(iter)){
                        bind_to_new_view = false;
                    }
                }
            }
            else{
                append(&feedback_str, "did not begin command-line command because the target buffer is already in use\n");
                do_feedback_message(system, models, feedback_str);
                result = false;
                goto done;
            }
        }
        
        String path_string = {0};
        if (!path){
            terminate_with_null(&models->hot_directory.string);
            path_string = models->hot_directory.string;
        }
        else{
            path_string = make_string_terminated(part, path, path_len);
        }
        
        {
            String command_string = {0};
            
            if (!command){
#define NO_SCRIPT " echo no script specified"
                command_string.str = NO_SCRIPT;
                command_string.size = sizeof(NO_SCRIPT)-1;
#undef NO_SCRIPT
            }
            else{
                command_string = make_string_terminated(part, command, command_len);
            }
            
            if (vptr && bind_to_new_view){
                view_set_file(vptr, file, models);
                view_show_file(vptr);
            }
            
            proc = procs + vars->cli_processes.count++;
            proc->out_file = file;
            if (flags & CLI_CursorAtEnd){
                proc->cursor_at_end = 1;
            }
            else{
                proc->cursor_at_end = 0;
            }
            
            if (!system->cli_call(path_string.str, command_string.str, &proc->cli)){
                --vars->cli_processes.count;
            }
        }
    }
    else{
        append(&feedback_str, "ERROR: no available process slot\n");
        do_feedback_message(system, models, feedback_str);
        result = false;
        goto done;
    }
    
    done:
    end_temp_memory(temp);
    return(result);
}

DIRECTORY_GET_HOT_SIG(external_directory_get_hot)/*
DOC_PARAM(out, a buffer that receives the 4coder 'hot directory')
DOC_PARAM(capacity, the maximum size to be output to the output buffer)
DOC_RETURN(returns the size of the string written into the buffer)
DOC
(
4coder has a concept of a 'hot directory' which is the directory most recently
accessed in the GUI.  Whenever the GUI is opened it shows the hot directory.

In the future this will be deprecated and eliminated in favor of more flexible
directories controlled by the custom side.
)
*/{
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    Hot_Directory *hot = &cmd->models->hot_directory;
    i32 copy_max = capacity - 1;
    hot_directory_clean_end(hot);
    if (copy_max > hot->string.size)
        copy_max = hot->string.size;
    memcpy(out, hot->string.str, copy_max);
    out[copy_max] = 0;
    return(hot->string.size);
}

#define external_get_4ed_path system->get_4ed_path

#define external_file_exists  system->file_exists

#define external_directory_cd system->directory_cd

GET_FILE_LIST_SIG(external_get_file_list)/*
DOC_PARAM(dir, the directory whose files will be enumerated in the returned list)
DOC_PARAM(len, the length of the dir string)
DOC_RETURN
(
returns a File_List struct containing pointers to the names of the files in
the specified directory.  The File_List returned should be passed to free_file_list
when it is no longer in use.
)
*/{
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    System_Functions *system = cmd->system;
    File_List result = {};
    system->set_file_list(&result, make_string(dir, len));
    return(result);
}

FREE_FILE_LIST_SIG(external_free_file_list)/*
DOC_PARAM(list, the file list to be freed)
DOC(after this call the file list passed in should not be read or written to)
*/{
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    System_Functions *system = cmd->system;
    system->set_file_list(&list, make_string(0, 0));
}

CLIPBOARD_POST_SIG(external_clipboard_post)/*
DOC_PARAM(str, the string to post to the clipboard)
DOC_PARAM(len, the length of the string str)
DOC
(
Stores the string str in the clipboard initially with index 0.
Also reports the copy to the operating system, so that it may
be pasted into other applications.
)
*/{
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    System_Functions *system = cmd->system;
    Models *models = cmd->models;
    General_Memory *general = &models->mem.general;
    Working_Set *working = &models->working_set;
    
    String *dest = working_set_next_clipboard_string(general, working, len);
    copy(dest, make_string(str, len));
    system->post_clipboard(*dest);
}

CLIPBOARD_COUNT_SIG(external_clipboard_count)/*
DOC(returns the number of items in the clipboard)
*/{
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    Working_Set *working = &cmd->models->working_set;
    int count = working->clipboard_size;
    return(count);
}

CLIPBOARD_INDEX_SIG(external_clipboard_index)/*
DOC_PARAM(index, the index of the item to be read)
DOC_PARAM(out, a buffer where the clipboard contents are written or NULL)
DOC_PARAM(len, the length of the out buffer)
DOC_RETURN(returns the size of the item on the clipboard associated with the given index)
DOC
(
There are multiple items on the 4coder clipboard.  The most recent copy is always at
index 0.  The second most recent is at index 1, and so on for all the stored clipboard items.
This function reads one of the clipboard items and stores it into the out buffer, if the out
buffer is not NULL.  This function always returns the size of the clipboard item specified
even if the output buffer is NULL.  If the output buffer is too small to contain the whole
string, it is filled with the first len character of the clipboard contents.  The output
string is not null terminated.
)
*/{
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    Working_Set *working = &cmd->models->working_set;
    
    int size = 0;
    String *str = working_set_clipboard_index(working, index);
    if (str){
        size = str->size;
        if (out){
            String out_str = make_string(out, 0, len);
            copy(&out_str, *str);
        }
    }
    
    return(size);
}

internal void
internal_get_buffer_first(Working_Set *working_set, Buffer_Summary *buffer){
    if (working_set->file_count > 0){
        fill_buffer_summary(buffer, (Editing_File*)working_set->used_sentinel.next, working_set);
    }
}

internal void
internal_get_buffer_next(Working_Set *working_set, Buffer_Summary *buffer){
    Editing_File *file;
    
    file = working_set_get_active_file(working_set, buffer->buffer_id);
    if (file){
        file = (Editing_File*)file->node.next;
        fill_buffer_summary(buffer, file, working_set);
    }
    else{
        *buffer = buffer_summary_zero();
    }
}

GET_BUFFER_FIRST_SIG(external_get_buffer_first)/*
DOC_PARAM(access, the access flags for the access)
DOC_RETURN(returns the summary of the first buffer in a buffer loop)
DOC
(
Begins a loop across all the buffers.

If the buffer returned does not exist, the loop is finished.  Buffers
should not be killed durring a buffer loop.
)
DOC_SEE(Access_Flag)
DOC_SEE(get_buffer_next)
*/{
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    Working_Set *working_set = &cmd->models->working_set;
    Buffer_Summary result = {};
    
    internal_get_buffer_first(working_set, &result);
    while (result.exists && !access_test(result.lock_flags, access)){
        internal_get_buffer_next(working_set, &result);
    }
    
    return(result);
}

GET_BUFFER_NEXT_SIG(external_get_buffer_next)/*
DOC_PARAM(buffer, pointer to the loop buffer originally returned by get_buffer_first)
DOC_PARAM(access, the access flags for the access)
DOC
(
Writes the next buffer into the buffer struct.  To get predictable results every
call to get_buffer_first and get_buffer_next in the loop should have the same
access flags.

If the buffer returned does not exist, the loop is finished.  Buffers
should not be killed durring a buffer loop.
)
DOC_SEE(Access_Flag)
DOC_SEE(get_buffer_first)
*/{
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    Working_Set *working_set = &cmd->models->working_set;
    
    internal_get_buffer_next(working_set, buffer);
    while (buffer->exists && !access_test(buffer->lock_flags, access)){
        internal_get_buffer_next(working_set, buffer);
    }
}

GET_BUFFER_SIG(external_get_buffer)/*
DOC_PARAM(buffer_id, the id of the buffer to get)
DOC_PARAM(access, the access flags for the access)
DOC_RETURN(returns a summary that describes the indicated buffer if it exists and is accessible)
*/{
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    Working_Set *working_set = &cmd->models->working_set;
    Buffer_Summary buffer = {};
    Editing_File *file;
    
    file = working_set_get_active_file(working_set, buffer_id);
    if (file){
        fill_buffer_summary(&buffer, file, working_set);
        if (!access_test(buffer.lock_flags, access)){
            buffer = buffer_summary_zero();
        }
    }
    
    return(buffer);
}

GET_BUFFER_BY_NAME_SIG(external_get_buffer_by_name)/*
DOC_PARAM(name, the name of the buffer)
DOC_PARAM(len, the length of the name string)
DOC_PARAM(access, the access flags for the access)
DOC_RETURN(returns a summary that describes the indicated buffer if it exists and is accessible)
*/{
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    Buffer_Summary buffer = {};
    Editing_File *file;
    Working_Set *working_set = &cmd->models->working_set;
    
    file = working_set_contains(cmd->system, working_set, make_string(name, len));
    if (file && !file->is_dummy){
        fill_buffer_summary(&buffer, file, working_set);
        if (!access_test(buffer.lock_flags, access)){
            buffer = buffer_summary_zero();
        }
    }
    
    return(buffer);
}

BUFFER_SEEK_SIG(external_buffer_seek)/*
DOC_PARAM(buffer, the buffer to seek through)
DOC_PARAM(start_pos, the absolute position in the buffer to begin the seek)
DOC_PARAM(seek_forward, non-zero indicates to seek forward otherwise the seek goes backward)
DOC_PARAM(flags, one or more types of boundaries to use for stopping the seek)
DOC_RETURN(returns the position where the seek stops)
DOC_SEE(Seek_Boundary_Flag)
*/{
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    Editing_File *file;
    int result = false;
    
    file = imp_get_file(cmd, buffer);
    
    if (file){
        // TODO(allen): reduce duplication?
        i32 size = buffer_size(&file->state.buffer);
        i32 pos[4] = {0};
        i32 new_pos = 0;
        
        if (start_pos < 0){
            start_pos = 0;
        }
        else if (start_pos > size){
            start_pos = size;
        }
        
        if (seek_forward){
            for (i32 i = 0; i < ArrayCount(pos); ++i) pos[i] = size;
            
            if (flags & (1)){
                pos[0] = buffer_seek_whitespace_right(&file->state.buffer, start_pos);
            }
            
            if (flags & (1 << 1)){
                if (file->state.tokens_complete){
                    pos[1] = seek_token_right(&file->state.token_stack, start_pos);
                }
                else{
                    pos[1] = buffer_seek_whitespace_right(&file->state.buffer, start_pos);
                }
            }
            
            if (flags & (1 << 2)){
                pos[2] = buffer_seek_alphanumeric_right(&file->state.buffer, start_pos);
                if (flags & (1 << 3)){
                    pos[3] = buffer_seek_range_camel_right(&file->state.buffer, start_pos, pos[2]);
                }
            }
            else{
                if (flags & (1 << 3)){
                    pos[3] = buffer_seek_alphanumeric_or_camel_right(&file->state.buffer, start_pos);
                }
            }
            
            new_pos = size;
            for (i32 i = 0; i < ArrayCount(pos); ++i){
                if (pos[i] < new_pos) new_pos = pos[i];
            }
        }
        else{
            if (flags & (1)){
                pos[0] = buffer_seek_whitespace_left(&file->state.buffer, start_pos);
            }
            
            if (flags & (1 << 1)){
                if (file->state.tokens_complete){
                    pos[1] = seek_token_left(&file->state.token_stack, start_pos);
                }
                else{
                    pos[1] = buffer_seek_whitespace_left(&file->state.buffer, start_pos);
                }
            }
            
            if (flags & (1 << 2)){
                pos[2] = buffer_seek_alphanumeric_left(&file->state.buffer, start_pos);
                if (flags & (1 << 3)){
                    pos[3] = buffer_seek_range_camel_left(&file->state.buffer, start_pos, pos[2]);
                }
            }
            else{
                if (flags & (1 << 3)){
                    pos[3] = buffer_seek_alphanumeric_or_camel_left(&file->state.buffer, start_pos);
                }
            }
            
            new_pos = 0;
            for (i32 i = 0; i < ArrayCount(pos); ++i){
                if (pos[i] > new_pos) new_pos = pos[i];
            }
        }
        result = new_pos;
        
        fill_buffer_summary(buffer, file, cmd);
    }
    
    return(result);
}

BUFFER_READ_RANGE_SIG(external_buffer_read_range)/*
DOC_PARAM(buffer, the buffer to read out of)
DOC_PARAM(start, the beginning of the read range)
DOC_PARAM(end, one past the end of the read range)
DOC_PARAM(out, the output buffer to fill with the result of the read)
DOC_RETURN(returns non-zero on success)
DOC
(
The output buffer might have a capacity of at least (end - start)
The output is not null terminated.
)
*/{
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    Editing_File *file = imp_get_file(cmd, buffer);
    int result = false;
    int size;
    
    if (file){
        size = buffer_size(&file->state.buffer);
        if (0 <= start && start <= end && end <= size){
            result = true;
            buffer_stringify(&file->state.buffer, start, end, out);
        }
        fill_buffer_summary(buffer, file, cmd);
    }
    
    return(result);
}

BUFFER_REPLACE_RANGE_SIG(external_buffer_replace_range)/*
DOC_PARAM(buffer, the buffer to edit)
DOC_PARAM(start, the start of the range to edit)
DOC_PARAM(end, the end of the range to edit)
DOC_PARAM(str, the string to write into the range)
DOC_PARAM(len, the length of the str string)
DOC_RETURN(returns non-zero if the replacement succeeds)
DOC
(
Replace simultaneously deletes the range from start to end and writes str
in the same position.  If end == start then this call is equivalent to
inserting the string at start.  If len == 0 this call is equivalent to
deleteing the range from start to end.
)
*/{
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    Editing_File *file = imp_get_file(cmd, buffer);
    
    int result = false;
    int size = 0;
    int next_cursor = 0, pos = 0;
    
    if (file){
        size = buffer_size(&file->state.buffer);
        if (0 <= start && start <= end && end <= size){
            result = true;
            
            pos = file->state.cursor_pos;
            if (pos < start) next_cursor = pos;
            else if (pos < end) next_cursor = start;
            else next_cursor = pos + end - start - len;
            
            file_replace_range(cmd->system, cmd->models,
                               file, start, end, str, len, next_cursor);
        }
        fill_buffer_summary(buffer, file, cmd);
    }
    
    return(result);
}

BUFFER_SET_SETTING_SIG(external_buffer_set_setting)/*
DOC_PARAM(buffer, the buffer to set a setting on)
DOC_PARAM(setting, one of the Buffer_Setting_ID enum values that identifies the setting to set)
DOC_PARAM(value, the value to set the specified setting to)
DOC_SEE(Buffer_Setting_ID)
*/{
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    System_Functions *system = cmd->system;
    Models *models = cmd->models;
    
    Editing_File *file = imp_get_file(cmd, buffer);
    
    int result = false;
    
    i32 new_mapid = 0;
    
    if (file){
        result = true;
        switch (setting){
            case BufferSetting_Lex:
            {
#if BUFFER_EXPERIMENT_SCALPEL <= 0
                if (file->settings.tokens_exist){
                    if (!value){
                        file_kill_tokens(system, &models->mem.general, file);
                    }
                }
                else{
                    if (value){
                        file_first_lex_parallel(system, &models->mem.general, file);
                    }
                }
#endif
            }break;
            
            case BufferSetting_WrapLine:
            {
                file->settings.unwrapped_lines = !value;
            }break;
            
            case BufferSetting_MapID:
            {
                if (value == mapid_global){
                    file->settings.base_map_id = mapid_global;
                }
                else if (value == mapid_file){
                    file->settings.base_map_id = mapid_file;
                }
                else if (value < mapid_global){
                    new_mapid = get_map_index(models, value);
                    if (new_mapid  < models->user_map_count){
                        file->settings.base_map_id = value;
                    }
                    else{
                        file->settings.base_map_id = mapid_file;
                    }
                }
                
                for (View_Iter iter = file_view_iter_init(&models->layout, file, 0);
                     file_view_iter_good(iter);
                     iter = file_view_iter_next(iter)){
                    iter.view->map = get_map(models, file->settings.base_map_id);
                }
            }break;
        }
        fill_buffer_summary(buffer, file, cmd);
    }
    
    return(result);
}

BUFFER_AUTO_INDENT_SIG(external_buffer_auto_indent)/*
DOC_PARAM(buffer, the buffer in which to apply the auto indenting)
DOC_PARAM(start, the position to start the auto indenting)
DOC_PARAM(end, the position to end the auto indenting)
DOC_PARAM(tab_width, the number of spaces to place as a tab)
DOC_PARAM(flags, the auto tab behavior flags)
DOC_RETURN(returns non-zero when the call succeeds)
DOC
(
Applies the built in auto-indentation rule to the code in the range from
start to end by inserting spaces or tabs at the beginning of the lines.
If the buffer does not have lexing enabled or the lexing job has not
completed this function will fail.
)
DOC_SEE(Auto_Tab_Flag)
*/{
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    System_Functions *system = cmd->system;
    Models *models = cmd->models;
    
    Indent_Options opts = {0};
    int result = false;
    
    Editing_File *file = imp_get_file(cmd, buffer);
    if (file && file->state.token_stack.tokens &&
        file->state.tokens_complete && !file->state.still_lexing){
        result = true;
        
        opts.empty_blank_lines = (flags & AutoIndent_ClearLine);
        opts.use_tabs = (flags & AutoIndent_UseTab);
        opts.tab_width = tab_width;
        
        file_auto_tab_tokens(system, models, file, start, start, end, opts);
        
        fill_buffer_summary(buffer, file, cmd);
    }
    
    return(result);
}

CREATE_BUFFER_SIG(external_create_buffer)/*
DOC_PARAM(filename, the name of the file to be opened or created)
DOC_PARAM(filename_len, the length of the filename string)
DOC_PARAM(flags, flags for buffer creation behavior)
DOC_RETURN(returns the summary of the created buffer on success or a NULL buffer otherwise)
DOC
(
Tries to create a new buffer and associate it to the given filename.  If such a buffer already
exists the existing buffer is returned in the buffer summary and no new buffer is created.
If the buffer does not exist a new buffer is created and named after the given filename.  If
the filename corresponds to a file on the disk that file is loaded and put into buffer, if
the filename does not correspond to a file on disk the buffer is created empty.
)
DOC_SEE(Buffer_Create_Flag)
*/{
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    System_Functions *system = cmd->system;
    Models *models = cmd->models;
    Working_Set *working_set = &models->working_set;
    General_Memory *general = &models->mem.general;
    Partition *part = &models->mem.part;
    
    Buffer_Summary result = {0};
    
    Temp_Memory temp = begin_temp_memory(part);
    if (filename != 0){
        String filename_string = make_string_terminated(part, filename, filename_len);
        Editing_File *file = working_set_contains(system, working_set, filename_string);
        if (file == 0){
            File_Loading loading = {0};
            
            b32 do_new_file = false;
            
            if (flags & BufferCreate_AlwaysNew){
                do_new_file = true;
            }
            else{
                loading = system->file_load_begin(filename_string.str);
                if (!loading.exists){
                    do_new_file = true;
                }
            }
            
            if (do_new_file){
                b32 in_general_mem = false;
                char *buffer = push_array(part, char, loading.size);
                
                if (buffer == 0){
                    buffer = (char*)general_memory_allocate(general, loading.size);
                    if (buffer != 0){
                        in_general_mem = true;
                    }
                }
                
                if (system->file_load_end(loading, buffer)){
                    file = working_set_alloc_always(working_set, general);
                    if (file){
                        file_init_strings(file);
                        file_set_name(working_set, file, filename_string);
                        working_set_add(system, working_set, file, general);
                        init_normal_file(system, models, file,
                                         buffer, loading.size);
                        fill_buffer_summary(&result, file, cmd);
                    }
                }
                
                if (in_general_mem){
                    general_memory_free(general, buffer);
                }
                
            }
            else{
                file = working_set_alloc_always(working_set, general);
                if (file){
                    file_init_strings(file);
                    file_set_name(working_set, file, filename_string);
                    working_set_add(system, working_set, file, general);
                    init_normal_file(system, models, file, 0, 0);
                    fill_buffer_summary(&result, file, cmd);
                }
            }
        }
        else{
            fill_buffer_summary(&result, file, cmd);
        }
    }
    end_temp_memory(temp);
    
    return(result);
}

SAVE_BUFFER_SIG(external_save_buffer)/*
DOC_PARAM(buffer, the buffer to save to a file)
DOC_PARAM(filename, the name of the file to save the buffer into)
DOC_PARAM(filename_len, length of the filename string)
DOC_PARAM(flags, not currently used)
DOC_RETURN(returns non-zero if the save succeeds)
*/{
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    System_Functions *system = cmd->system;
    Models *models = cmd->models;
    int result = false;
    
    Editing_File *file = imp_get_file(cmd, buffer);
    if (file){
        result = true;
        String name = make_string(filename, filename_len);
        view_save_file(system, models, file, 0, name, false);
    }
    
    return(result);
}

KILL_BUFFER_SIG(external_kill_buffer)/*
DOC_PARAM(buffer, a buffer identifier specifying the buffer to try to kill)
DOC_PARAM(view_id, the id of view that will contain the "are you sure" dialogue)
DOC_PARAM(flags, flags for buffer kill behavior)
DOC_RETURN(returns non-zero if the kill succeeds)
DOC
(
Tries to kill the idenfied buffer.  If the buffer is dirty and the "are you sure"
dialogue needs to be displayed the provided view is used to show the dialogue.
If the view is not open the kill fails.
)
DOC_SEE(Buffer_Kill_Flags)
*/{
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    System_Functions *system = cmd->system;
    Models *models = cmd->models;
    Working_Set *working_set = &models->working_set;
    View *vptr = imp_get_view(cmd, view_id);
    Editing_File *file = get_file_from_identifier(system, working_set, buffer);
    int result = false;
    
    if (file){
        if (flags & BufferKill_AlwaysKill){
            result = true;
            kill_file(system, models, file, string_zero());
        }
        else{
            if (vptr == 0){
                result = true;
                try_kill_file(system, models, file, vptr, string_zero());
            }
            else{
                // TODO(allen): message
            }
        }
    }
    
    return(result);
}

internal void
internal_get_view_first(Command_Data *cmd, View_Summary *view){
    Editing_Layout *layout = &cmd->models->layout;
    Panel *panel = layout->used_sentinel.next;
    
    Assert(panel != &layout->used_sentinel);
    fill_view_summary(view, panel->view, cmd);
}

internal void
internal_get_view_next(Command_Data *cmd, View_Summary *view){
    Editing_Layout *layout = &cmd->models->layout;
    Live_Views *live_set = &cmd->vars->live_set;
    int index = view->view_id - 1;
    View *vptr = 0;
    Panel *panel = 0;
    
    if (index >= 0 && index < live_set->max){
        vptr = live_set->views + index;
        panel = vptr->panel;
        if (panel){
            panel = panel->next;
        }
        if (panel && panel != &layout->used_sentinel){
            fill_view_summary(view, panel->view, &cmd->vars->live_set, &cmd->models->working_set);
        }
        else{
            *view = view_summary_zero();
        }
    }
    else{
        *view = view_summary_zero();
    }
}

GET_VIEW_FIRST_SIG(external_get_view_first)/*
DOC_PARAM(access, the access flags for the access)
DOC_RETURN(returns the summary of the first view in a view loop)
DOC
(
Begins a loop across all the open views.

If the view summary returned is NULL, the loop is finished.
Views should not be closed or opened durring a view loop.
)
DOC_SEE(Access_Flag)
DOC_SEE(get_view_next)
*/{
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    View_Summary view = {};
    
    internal_get_view_first(cmd, &view);
    while (view.exists && !access_test(view.lock_flags, access)){
        internal_get_view_next(cmd, &view);
    }
    
    return(view);
}

GET_VIEW_NEXT_SIG(external_get_view_next)/*
DOC_PARAM(view, pointer to the loop view originally returned by get_view_first)
DOC_PARAM(access, the access flags for the access)
DOC
(
Writes the next view into the view struct.  To get predictable results every
call to get_view_first and get_view_next in the loop should have the same
access flags.

If the view summary returned is NULL, the loop is finished.
Views should not be closed or opened durring a view loop.
)
DOC_SEE(Access_Flag)
DOC_SEE(get_view_first)
*/{
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    
    internal_get_view_next(cmd, view);
    while (view->exists && !access_test(view->lock_flags, access)){
        internal_get_view_next(cmd, view);
    }
}

GET_VIEW_SIG(external_get_view)/*
DOC_PARAM(view_id, the id of the view to get)
DOC_PARAM(access, the access flags for the access)
DOC_RETURN(returns a summary that describes the indicated view if it is open and is accessible)
*/{
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    View_Summary view = {0};
    Live_Views *live_set = cmd->live_set;
    int max = live_set->max;
    View *vptr = 0;
    
    view_id -= 1;
    if (view_id >= 0 && view_id < max){
        vptr = live_set->views + view_id;
        fill_view_summary(&view, vptr, live_set, &cmd->models->working_set);
        if (!access_test(view.lock_flags, access)){
            view = view_summary_zero();
        }
    }
    
    return(view);
}

GET_ACTIVE_VIEW_SIG(external_get_active_view)/*
DOC_PARAM(access, the access flags for the access)
DOC_RETURN(returns a summary that describes the active view)
*/{
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    View_Summary view = {};
    fill_view_summary(&view, cmd->view, &cmd->vars->live_set, &cmd->models->working_set);
    if (!access_test(view.lock_flags, access)){
        view = view_summary_zero();
    }
    return(view);
}

VIEW_COMPUTE_CURSOR_SIG(external_view_compute_cursor)/*
DOC_PARAM(view, the view on which to run the cursor computation)
DOC_PARAM(seek, the seek position)
DOC_PARAM(cursor_out, on success this is filled with result of the seek)
DOC_RETURN(returns non-zero on success)
DOC
(
Computes a full cursor for the given seek position.
)
DOC_SEE(Buffer_Seek)
*/{
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    View *vptr = imp_get_view(cmd, view);
    Editing_File *file = 0;
    int result = false;
    
    if (vptr){
        file = vptr->file_data.file;
        if (file && !file->is_loading){
            if (seek.type == buffer_seek_line_char && seek.character <= 0){
                seek.character = 1;
            }
            result = true;
            *cursor_out = view_compute_cursor(vptr, seek);
            fill_view_summary(view, vptr, cmd);
        }
    }
    
    return(result);
}

VIEW_SET_CURSOR_SIG(external_view_set_cursor)/*
DOC_PARAM(view, the view in which to set the cursor)
DOC_PARAM(seek, the seek position)
DOC_PARAM(set_preferred_x, if true the preferred x is updated to match the new cursor position)
DOC_RETURN(returns non-zero on success)
DOC
(
Sets the the view's cursor position.  set_preferred_x should usually be true unless the change in
cursor position is is a vertical motion that tries to keep the cursor in the same column or x position.
)
DOC_SEE(Buffer_Seek)
*/{
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    View *vptr = imp_get_view(cmd, view);
    Editing_File *file = 0;
    int result = false;
    
    if (vptr){
        file = vptr->file_data.file;
        if (file && !file->is_loading){
            result = true;
            if (seek.type == buffer_seek_line_char && seek.character <= 0){
                seek.character = 1;
            }
            vptr->recent->cursor = view_compute_cursor(vptr, seek);
            if (set_preferred_x){
                vptr->recent->preferred_x = view_get_cursor_x(vptr);
            }
            fill_view_summary(view, vptr, cmd);
            file->state.cursor_pos = vptr->recent->cursor.pos;
        }
    }
    
    return(result);
}

VIEW_SET_MARK_SIG(external_view_set_mark)/*
DOC_PARAM(view, the view in which to set the mark)
DOC_PARAM(seek, the seek position)
DOC_RETURN(returns non-zero on success)
DOC
(
Sets the the view's mark position.
)
DOC_SEE(Buffer_Seek)
*/{
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    View *vptr = imp_get_view(cmd, view);
    Full_Cursor cursor = {0};
    int result = false;
    
    if (vptr){
        result = true;
        if (seek.type != buffer_seek_pos){
            cursor = view_compute_cursor(vptr, seek);
            vptr->recent->mark = cursor.pos;
        }
        else{
            vptr->recent->mark = seek.pos;
        }
        fill_view_summary(view, vptr, cmd);
    }
    
    return(result);
}

VIEW_SET_HIGHLIGHT_SIG(external_view_set_highlight)/*
DOC_PARAM(view, the view to set the highlight in)
DOC_PARAM(start, the start of the highlight range)
DOC_PARAM(end, the end of the highlight range)
DOC_PARAM(turn_on, indicates whether the highlight is being turned on or off)
DOC_RETURN(returns non-zero on success)
DOC
(
The highlight is mutually exclusive to the cursor.  When the turn_on parameter
is set to true the highlight will be shown and the cursor will be hidden.  After
that either setting the with view_set_cursor or calling view_set_highlight and
the turn_on set to false, will switch back to showing the cursor.
)
*/{
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    View *vptr = imp_get_view(cmd, view);
    int result = false;
    
    if (vptr){
        result = true;
        if (turn_on){
            view_set_temp_highlight(vptr, start, end);
        }
        else{
            vptr->file_data.show_temp_highlight = 0;
        }
        fill_view_summary(view, vptr, cmd);
    }
    
    return(result);
}

VIEW_SET_BUFFER_SIG(external_view_set_buffer)/*
DOC_PARAM(view, the view to display the buffer in)
DOC_PARAM(buffer_id, the buffer to show in the view)
DOC_PARAM(flags, set buffer behavior flags)
DOC_RETURN(returns non-zero on success)
DOC
(
On success view_set_buffer sets the specified view's current buffer and
cancels and dialogue shown in the view and displays the file.
)
DOC_SEE(Set_Buffer_Flag)
*/{
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    View *vptr = imp_get_view(cmd, view);
    Models *models = cmd->models;
    Editing_File *file = 0;
    int result = false;
    
    if (vptr){
        file = working_set_get_active_file(&models->working_set, buffer_id);
        
        if (file){
            result = true;
            if (file != vptr->file_data.file){
                view_set_file(vptr, file, models);
                if (!(flags & SetBuffer_KeepOriginalGUI)){
                    view_show_file(vptr);
                }
            }
        }
        
        fill_view_summary(view, vptr, cmd);
    }
    
    return(result);
}

VIEW_POST_FADE_SIG(external_view_post_fade)/*
DOC_PARAM(view, the veiw to post a fade effect to)
DOC_PARAM(seconds, the number of seconds the fade effect should last)
DOC_PARAM(start, the first character in the fade range)
DOC_PARAM(end, one after the last character in the fade range)
DOC_PARAM(color, the color to fade from)
*/{
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    View *vptr = imp_get_view(cmd, view);
    
    int result = false;
    
    int size = end - start;
    if (vptr){
        if (size > 0){
            result = true;
            view_post_paste_effect(vptr, seconds, start, size, color);
        }
    }
    
    return(result);
}

VIEW_SET_PASTE_REWRITE__SIG(external_view_set_paste_rewrite_){
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    View *vptr = imp_get_view(cmd, view);
    if (vptr){
        vptr->next_mode.rewrite = true;
    }
}

VIEW_GET_PASTE_REWRITE__SIG(external_view_get_paste_rewrite_){
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    View *vptr = imp_get_view(cmd, view);
    int result = false;
    if (vptr){
        result = vptr->mode.rewrite;
    }
    return(result);
}

GET_USER_INPUT_SIG(external_get_user_input){
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    System_Functions *system = cmd->system;
    Coroutine *coroutine = (Coroutine*)app->current_coroutine;
    User_Input result = {0};
    
    if (app->type_coroutine == Co_Command){
        Assert(coroutine);
        *((u32*)coroutine->out+0) = get_type;
        *((u32*)coroutine->out+1) = abort_type;
        system->yield_coroutine(coroutine);
        result = *(User_Input*)coroutine->in;
    }
    
    return(result);
}

GET_COMMAND_INPUT_SIG(external_get_command_input){
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    User_Input result;
    
    result.type = UserInputKey;
    result.abort = 0;
    result.key = cmd->key;
    result.command = 0;
    
    return(result);
}

GET_MOUSE_STATE_SIG(external_get_mouse_state){
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    App_Vars *vars = cmd->vars;
    Mouse_State mouse = direct_get_mouse_state(&vars->available_input);
    return(mouse);
}

GET_EVENT_MESSAGE_SIG(external_get_event_message){
    Event_Message message = {0};
    System_Functions *system = (System_Functions*)app->system_links;
    Coroutine *coroutine = (Coroutine*)app->current_coroutine;
    
    if (app->type_coroutine == Co_View){
        Assert(coroutine);
        system->yield_coroutine(coroutine);
        message = *(Event_Message*)coroutine->in;
    }
    
    return(message);
}

START_QUERY_BAR_SIG(external_start_query_bar){
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    Query_Slot *slot = 0;
    View *vptr;
    
    vptr = cmd->view;
    
    slot = alloc_query_slot(&vptr->query_set);
    slot->query_bar = bar;
    
    return(slot != 0);
}

END_QUERY_BAR_SIG(external_end_query_bar){
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    View *vptr;
    vptr = cmd->view;
    free_query_slot(&vptr->query_set, bar);
}

PRINT_MESSAGE_SIG(external_print_message){
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    Models *models = cmd->models;
    do_feedback_message(cmd->system, models, make_string(string, len));
}

CHANGE_THEME_SIG(external_change_theme){
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    Style_Library *styles = &cmd->models->styles;
    String theme_name = make_string(name, len);
    
    i32 i = 0;
    i32 count = styles->count;
    Style *s = styles->styles;
    
    for (i = 0; i < count; ++i, ++s){
        if (match(s->name, theme_name)){
            style_copy(main_style(cmd->models), s);
            break;
        }
    }
}

CHANGE_FONT_SIG(external_change_font){
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    Font_Set *set = cmd->models->font_set;
    Style_Font *global_font = &cmd->models->global_font;
    String font_name = make_string(name, len);
    i16 font_id = 0;
    
    if (font_set_extract(set, font_name, &font_id)){
        global_font->font_id = font_id;
        global_font->font_changed = 1;
    }
}

SET_THEME_COLORS_SIG(external_set_theme_colors){
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    Style *style = main_style(cmd->models);
    
    u32 *color = 0;
    i32 i = 0;
    Theme_Color *theme_color = colors;
    
    for (i = 0; i < count; ++i, ++theme_color){
        color = style_index_by_tag(&style->main, theme_color->tag);
        if (color) *color = theme_color->color | 0xFF000000;
    }
}

GET_THEME_COLORS_SIG(external_get_theme_colors){
    Command_Data *cmd = (Command_Data*)app->cmd_context;
    Style *style = main_style(cmd->models);
    
    u32 *color = 0;
    i32 i = 0;
    Theme_Color *theme_color = colors;
    
    for (i = 0; i < count; ++i, ++theme_color){
        color = style_index_by_tag(&style->main, theme_color->tag);
        if (color){
            theme_color->color = *color | 0xFF000000;
        }
        else{
            theme_color->color = 0xFF000000;
        }
    }
}

// BOTTOM
