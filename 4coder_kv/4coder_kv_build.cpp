function String8 
kv_search_build_file_from_dir(FApp *app, Arena *arena, String8 start_dir)
{
    String8 full_file_path = {};
    for (u32 i = 0; 
         i < ArrayCount(standard_build_filename_array); 
         i += 1)
    {
        full_file_path = search_up_path(app, arena, start_dir, standard_build_filename_array[i]);
        if (full_file_path.size > 0)
        {
            break;
        }
    }
    return full_file_path;
}

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
