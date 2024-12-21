
//~Framework api
api framework_api_1
{
 void poly3_inner_2(v3 points[3], argb color, v1 depth_offset, Poly_Flags flags);
 u64 ad_rdtsc(void);
 Bone *get_bones(Modeler *m);
}
meta_table(name) framework_api_2
{
 push_object_transform_to_target,
 DEBUG_send_entry,
 push_image,
}

gen_file "framework_api.gen.h"
{
 gen_for(framework_api_1)
 {
#define `(name)__return `return
#define `(name)__params `params
  
 }
 
#define framework_api_xlist_1(X) \
gen_for(framework_api_1)
 {
  X(`name) \
 }
 
#define framework_api_xlist_2(X) \
gen_for(framework_api_2)
 {
  X(`name) \
 }
 
#define framework_api_xlist(X) \
framework_api_xlist_1(X) \
framework_api_xlist_2(X) \
memory_functions_xlist(X)
}

//~Driver api
api driver_api
{
 void render_movie(Arena *arena, Painter *painter, Pose *pose, v1 anime_time);
 Pose driver_update(Modeler *m, v1 anim_time);
 void driver_reload(Framework_API *framework_api);
 void driver_shutdown(void);
}

gen_file "driver_api.gen.h"
{
 gen_for(driver_api)
 {
#define `(name)__return `return
#define `(name)__params `params
  
 }
 
#define driver_api_xlist(X) \
gen_for(driver_api)
 {
  X(`name) \
 }
}
//~