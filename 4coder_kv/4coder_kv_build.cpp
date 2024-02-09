function String8 
kv_search_build_file_from_dir(FApp *app, Arena *arena, String8 start_dir)
{
    String8 full_file_path = {};
    for (u32 i = 0; 
         i < ArrayCount(standard_build_file_name_array); 
         i += 1)
    {
        full_file_path = push_file_search_up_path(app, arena, start_dir, standard_build_file_name_array[i]);
        if (full_file_path.size > 0)
        {
            break;
        }
    }
    return full_file_path;
}

/*
function b32
kv_search_and_build_from_dir(FApp *app, View_ID view, String8 start_dir, char *command_args)
{
    Scratch_Block scratch(app);

    // NOTE(allen): Search
    String_Const_u8 full_file_path = {};
    String_Const_u8 cmd_string  = {};
    for (u32 i = 0; i < ArrayCount(kv_build_file_name_array); i += 1)
    {
        full_file_path = push_file_search_up_path(app, scratch, start_dir, kv_build_file_name_array[i]);
        if (full_file_path.size > 0){
            cmd_string = kv_build_cmd_string_array[i];
            break;
        }
    }

    b32 result = (full_file_path.size > 0);
    if (result)
    {
        // NOTE(allen): Build
        String8 path = string_remove_last_folder(full_file_path);
        String8 command = push_u8_stringf(scratch, "\"%.*s/%.*s\" %s",
                                          string_expand(path),
                                          string_expand(cmd_string),
                                          command_args);
        b32 auto_save = def_get_config_b32(vars_intern_lit("automatically_save_changes_on_build"));
        if (auto_save)
        {
            save_all_dirty_buffers(app);
        }
        standard_build_exec_command(app, view, path, command);
        print_message(app, push_u8_stringf(scratch, "Building with: %.*s\n",
                                           string_expand(full_file_path)));
    }
    
    return(result);
}

// NOTE(allen): This searches first using the active file's directory,
// then if no build script is found, it searches from 4coders hot directory.
static void
kv_search_and_build(Application_Links *app, char *command_args)
{
    GET_VIEW_AND_BUFFER;
    
    Scratch_Block scratch(app);
    b32 did_build = false;
    String8 build_dir = push_build_directory_at_file(app, scratch, buffer);
    if (build_dir.size > 0)
    {
        did_build = kv_search_and_build_from_dir(app, view, build_dir, command_args);
    }
    if (!did_build)
    {
        build_dir = push_hot_directory(app, scratch);
        if (build_dir.size > 0)
        {
            did_build = kv_search_and_build_from_dir(app, view, build_dir, command_args);
        }
    }
}
*/

internal void 
kv_build_normal(FApp *app)
{
  build_in_bottom_view(app, "");
}

internal void 
kv_build_run_only(FApp *app)
{
  build_in_bottom_view(app, "run");
}

internal void 
kv_build_full_rebuild(FApp *app)
{
  build_in_bottom_view(app, "full");
}
