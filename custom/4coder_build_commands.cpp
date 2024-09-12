/*
4coder_build_commands.cpp - Commands for building.
*/

// TOP

static String8
push_build_directory_at_file(App *app, Arena *arena, Buffer_ID buffer)
{
    String8 result = {};
    String8 filename = push_buffer_filename(app, arena, buffer);
    Temp_Memory restore_point = begin_temp(arena);
    String8 base_name = push_buffer_base_name(app, arena, buffer);
    b32 is_match = string_match(filename, base_name);
    end_temp(restore_point);
 if ( !is_match )
 {
  result = push_stringz(arena, path_dirname(filename));
 }
 return(result);
}

global String standard_build_filename_array[] = 
{
 str8_lit("kv-build.py"),
 str8_lit("build.py"),
 str8_lit("kv-build.sh"),
 str8_lit("build.sh"),
 str8_lit("Makefile"),
#if OS_WINDOWS
 str8_lit("build.bat"),
#endif
};

global String standard_build_cmd_string_array[] = 
{
  str8_lit("kv-build.py"),
  str8_lit("build.py"),
  str8_lit("kv-build.sh"),
  str8_lit("build.sh"),
  str8_lit("make"),
#if OS_WINDOWS
  str8_lit("build"),
#endif
};

internal String
push_fallback_command(Arena *arena, String filename){
    return(push_stringfz(arena, "echo could not find %.*s", string_expand(filename)));
}

internal String
push_fallback_command(Arena *arena){
    return(push_fallback_command(arena, standard_build_filename_array[0]));
}

global const String compilation_buffer_name = str8_lit("*compilation*");
global const Buffer_Identifier standard_build_compilation_buffer_identifier = buffer_identifier(compilation_buffer_name);

global const u32 standard_build_exec_flags = CLI_OverlapWithConflict|CLI_SendEndSignal;

internal void
standard_build_exec_command(App *app, View_ID view, String8 dir, String8 cmd)
{
 exec_system_command(app, view, standard_build_compilation_buffer_identifier,
                     dir, cmd,
                     standard_build_exec_flags);
}

function void 
vim_set_bottom_text(String msg);

internal b32
standard_search_and_build_from_dir(App *app, View_ID view, String8 start_dir, char *command_args)
{
 Scratch_Block scratch(app);
 
 // NOTE(allen): Search
    String8 full_file_path = {};
    String8 cmd_string  = {};
    for (u32 i = 0; i < ArrayCount(standard_build_filename_array); i += 1)
    {
        full_file_path = search_up_path(scratch, start_dir, standard_build_filename_array[i]);
        if (full_file_path.size > 0){
            cmd_string = standard_build_cmd_string_array[i];
            break;
        }
    }
    
    b32 result = (full_file_path.size > 0);
    if (result)
    {
        // NOTE(allen): Build
        String8 path = path_dirname(full_file_path);
        String8 command = push_stringfz(scratch, "\"%.*s/%.*s\" %s",
                                       string_expand(path),
                                       string_expand(cmd_string),
                                       command_args);
        b32 auto_save = def_get_config_b32(vars_intern_lit("automatically_save_changes_on_build"));
        if (auto_save)
        {
            save_all_dirty_buffers(app);
        }
        standard_build_exec_command(app, view, path, command);
        vim_set_bottom_text(push_stringfz(scratch, "Building with: %.*s\n", string_expand(full_file_path)));
    }
 
 return(result);
}

// NOTE(allen): This searches first using the active file's directory,
// then if no build script is found, it searches from 4coders hot directory.
internal void
standard_search_and_build(App *app, View_ID view, Buffer_ID active_buffer, char *command_args)
{
 Scratch_Block scratch(app);
 b32 did_build = false;
 String8 build_dir = push_build_directory_at_file(app, scratch, active_buffer);
 if (build_dir.size > 0)
 {
  did_build = standard_search_and_build_from_dir(app, view, build_dir, command_args);
 }
 if (!did_build)
 {
  build_dir = push_hot_directory(app, scratch);
  if (build_dir.size > 0)
  {
   did_build = standard_search_and_build_from_dir(app, view, build_dir, command_args);
  }
 }
 if (!did_build)
 {
  standard_build_exec_command(app, view,
                              push_hot_directory(app, scratch),
                              push_fallback_command(scratch));
 }
}

#if 0
CUSTOM_COMMAND_SIG(build_search)
CUSTOM_DOC("Looks for a build.bat, build.sh, or makefile in the current and parent directories.  Runs the first that it finds and prints the output to *compilation*.")
{
    View_ID view = get_active_view(app, Access_Always);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    standard_search_and_build(app, view, buffer, "");
    block_zero_struct(&prev_location);
    lock_jump_buffer(app, compilation_buffer_name);
}
#endif

static Buffer_ID
get_comp_buffer(App *app)
{
    return(get_buffer_by_name(app, compilation_buffer_name, Access_Always));
}

/*
internal View_ID
get_or_open_build_panel(App *app)
{
    View_ID view = 0;
    Buffer_ID buffer = get_comp_buffer(app);
    if (buffer != 0)
    {
        view = get_first_view_with_buffer(app, buffer);
    }
    if (view == 0)
    {
        view = open_build_footer_panel(app);
    }
    return(view);
}
*/

function void
set_fancy_compilation_buffer_font(App *app)
{
 Scratch_Block scratch(app);
 Buffer_ID buffer = get_comp_buffer(app);
 Font_Load_Location font = {};
 font.filename = def_search_normal_full_path(scratch, str8_lit("fonts/Inconsolata-Regular.ttf"));
 set_buffer_face_by_font_load_location(app, buffer, &font);
}

inline String8
push_buffer_dirname(App *app, Arena *arena, Buffer_ID buffer)
{
 String8 filename = push_buffer_filename(app, arena, buffer);
 return path_dirname(filename);
}


internal void 
build_in_bottom_view(App *app, char *command_args)
{
 View_ID   view   = get_active_view(app, Access_Always);
 Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
 
 {
  Scratch_Block scratch(app);
  String8 dirname = push_buffer_dirname(app, scratch, buffer);
  if (dirname.size) { set_hot_directory(app, dirname); }
 }
 
 standard_search_and_build(app, global_bottom_view, buffer, command_args);
 set_fancy_compilation_buffer_font(app);
 
 block_zero_struct(&prev_location);
 lock_jump_buffer(app, compilation_buffer_name);
}

internal String
kv_search_build_file_from_dir(Arena *arena, String start_dir)
{
    String full_file_path = {};
    for_u32( i,0,alen(standard_build_filename_array) )
    {
        full_file_path = search_up_path(arena, start_dir, standard_build_filename_array[i]);
        if (full_file_path.size > 0)
        {
            break;
  }
 }
 return full_file_path;
}

internal void 
kv_build_normal(App *app)
{
 GET_VIEW_AND_BUFFER;
 
 // NOTE: ;build_filename_hack
 Scratch_Block scratch(app);
 String filename = push_buffer_filename(app, scratch, buffer);
 String arg = push_stringfz(scratch, "--file %.*s", string_expand(filename));
 
 build_in_bottom_view(app, (char *)arg.str);
}

internal void
kv_build_run_only(App *app)
{
 build_in_bottom_view(app, "--action run");
}

internal void 
kv_build_full_rebuild(App *app)
{
 build_in_bottom_view(app, "--full");
}
