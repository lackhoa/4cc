//-
#include "driver_precompiled.h"
//-
#include "sliders0.gen.h"
#include "driver_animate.gen.cpp"
#include "driver.gen.cpp"
#include "sliders0.gen.cpp"

global Arena driver_dll_arena;

function void
driver_update(Model *model, v1 anim_time)
{// NOTE @driver_api
 the_model = model;
 update_text_objects();
 crappy_tests(anim_time);
}
function void
driver_shutdown(void)
{// @driver_api
 arena_free(&driver_dll_arena);
}

dll_export void
driver_dll_entry(Driver_API *driver, Framework_API *framework)
{// @driver_api
 //NOTE(kv) We can't persist the framework code because it can be reloaded, too!
#define X(N) N = framework->N;
 framework_api_xlist(X);
#undef X
 
#define X(N) driver->N = N;
 driver_api_xlist(X);
#undef X
 
 tweaks = framework->tweaks;
 
 Driver_Data &data = driver_data;
 data.sliders = { ArrayAndCount(global_sliders) };
 
 i32 file_count = 2;  // TODO(kv) Hacking
 data.marked_positions.items = push_array0(&driver_dll_arena, Positions_In_File, file_count);
 data.marked_positions.count = file_count;
 // NOTE(kv) Reserve file 0 for null
 data.marked_positions.items[1] = {ArrayAndCount(driver_marked_positions)};
 data.text_objects = {ArrayAndCount(text_objects)};
 data.vertices_info = {ArrayAndCount(vertices_info_)};
 
 data.marker_pairs.items = push_array0(&driver_dll_arena, sarray(Marker_Pair), file_count);
 data.marker_pairs.count = file_count;
 // NOTE(kv) Reserve file 0 for null
 data.marker_pairs.items[1] = {ArrayAndCount(driver_text_ranges)};
 data.valid = true;
 
 // NOTE(kv) The data is constant, so we just copy it over.
 driver->data = data;
 
 init_sliders(&driver_dll_arena, framework->types);
}
//-