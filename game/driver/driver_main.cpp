//-
#include "driver_precompiled.h"
//-
#include "driver_fui_data.gen.h"
#include "driver_animate.gen.cpp"
#include "driver.gen.cpp"
#include "driver_fui_data.gen.cpp"

global Arena driver_dll_arena;

function void
driver_update(Model *model, v1 anim_time)
{// NOTE @driver_api
 the_model = model;
 update_driver_data();
 crappy_tests(anim_time);
}
function void
driver_shutdown(void)
{// @driver_api
 arena_free(&driver_dll_arena);
}

dll_export void
driver_dll_entry(Driver_API *driver, Framework_API *framework)
{// See @driver_api @do_work_after_loading_driver
 //NOTE(kv) We can't persist the framework code because it can be reloaded, too!
#define X(N) N = framework->N;
 framework_api_xlist(X);
#undef X
 
#define X(N) driver->N = N;
 driver_api_xlist(X);
#undef X
 
 tweaks = framework->tweaks;
 Arena *dll_arena = &driver_dll_arena;
 
 Driver_Data *data = &driver_data;
 data->files = {ArrayAndCount(fui_files_)};
 data->vertices_info = {ArrayAndCount(vertices_info_)};
 
 data->valid = 1;
 driver->data = data;
 
 init_sliders(framework->types);
}
//-