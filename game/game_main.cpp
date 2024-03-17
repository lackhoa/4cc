#define KV_IMPLEMENTATION
#include "kv.h"
#include <array> // @std
#define AD_IS_COMPILING_GAME 1
#define ED_API_USER_SIDE 1
#include "4coder_game_shared.h"
#include "game_fui.cpp"
#include "game.h"
#include "4coder_fancy.cpp"
#include "4coder_app_links_allocator.cpp"
#include "game_config.cpp"
#include "game_mesh.cpp"
#include "game_draw.cpp"
#include "game_colors.cpp"
#include "game_anime.cpp"
#include "game_api.cpp"
#include "4ed_render_target.cpp"

/*
  IMPORTANT Rule for the renderer
  1. Colors are in linear space (todo precision loss if passed as u32)

  (NOTE: I've debated about pushing a scale down to the renderer,
  but that wouldn't help since there are MANY units we can use)

  OLD Rules for the renderer (putting here for reference):
  - pma = Pre-multiplied alpha
  - v4 colors are in linear space, alpha=1 (so pma doesn't matter)
 */

global const u32 data_current_version = 7;
global const u32 data_magic_number = *(u32 *)"kvda";

global const v1 widget_margin = 5.0f;

#define X(N) internal N##_return N(N##_params);
// Note: forward declare
X_GAME_API_FUNCTIONS(X)
//
#undef X


internal b32
is_key_newly_pressed(Game_Input *input, Key_Mods modifiers, Key_Code keycode)
{
 return ((input->key_states       [keycode])     &&
         (input->key_state_changes[keycode] > 0) &&
         (input->active_mods == modifiers));
}

internal b32
is_key_newly_pressed_no_mod(Game_Input *input, Key_Code keycode)
{
 return ((input->key_states       [keycode])     &&
         (input->key_state_changes[keycode] > 0) &&
         (input->active_mods == Key_Mod_NULL));
}

force_inline b32
is_key_down(Game_Input *input, Key_Mods modifiers, Key_Code keycode)
{
 return ((input->key_states[keycode]) &&
         (input->active_mods == modifiers));
}

internal v4
key_direction_v4(Game_Input *input, Key_Mods wanted_mods, b32 new_keypress, b32 *optional_shift=0)
{
 v4 direction = {};
 b32 match_exactly = (input->active_mods == wanted_mods);
 b32 match_with_shift = optional_shift && (input->active_mods == (wanted_mods|Key_Mod_Sft));
  
 if (match_exactly || match_with_shift)
 {
#define Hit(N) \
((input->key_states[Key_Code_##N] != 0) && \
((new_keypress == false) || (input->key_state_changes[Key_Code_##N] > 0)))
  //
  if (Hit(L))      {direction.x = +1;}
  if (Hit(H))      {direction.x = -1;}
  if (Hit(K))      {direction.y = +1;}
  if (Hit(J))      {direction.y = -1;}
  if (Hit(O))      {direction.z = +1;}
  if (Hit(I))      {direction.z = -1;}
  if (Hit(Period)) {direction.w = +1;}
  if (Hit(Comma))  {direction.w = -1;}
  //
#undef Hit
 }
 if (optional_shift) { *optional_shift = match_with_shift; }
 return direction;
}

force_inline v3
key_direction(Game_Input *input, Key_Mods wanted_mods, b32 new_keypress)
{
 return key_direction_v4(input, wanted_mods, new_keypress).xyz;
}

internal mat4i
mat4i_rotate_tpr(v1 theta, v1 phi, v1 roll, v3 pivot)
{// NOTE: Roll around z, then rotate around x, then rotate around y
 // NOTE Weird, in the inverse, we want to the roll_inv *last*
 // and so we endup doing the roll *first* in the forward direction.
 
 phi  *= -1.f;  // NOTE: But the rotation axes are "mirrored" since we want camera control to be intuitive
 roll *= -1.f;
 
 mat4i rotate;
 {
  v1 ct = cosine(theta);
  v1 st = sine  (theta);
  v1 cp = cosine(phi);
  v1 sp = sine  (phi);
  rotate.inverse = {{
    ct,     0,  -st,    0,
    sp*st, cp,   ct*sp, 0,
    cp*st, -sp,  cp*ct,  0,
    0,0,0,1,}};
 }
 
 rotate.inverse = rotateZ(-roll)*rotate.inverse;
 rotate.forward = transpose(rotate.inverse);
 
 mat4i translate = mat4i_translate(-pivot);
 
 mat4i result;
 result.forward = translate.inv * rotate     * translate;
 result.inverse = translate.inv * rotate.inv * translate;
 
 return result;
}

internal v3
tpr_point(v1 theta, v1 phi)
{
 return mat4i_rotate_tpr(theta,phi,0,vec3()) * vec3(0,0,1);
}

internal void
setup_camera(Camera *camera, Camera_Data *data)
{
 *camera = {};
 
 camera->near_clip    = 1*centimeter;
 camera->far_clip     = 10.f;
 camera->focal_length = 0.6169f;
 
 // TODO We can just block copy here
#define X(type,name)   camera->name = data->name;
 CAMERA_DATA_FIELDS(X)
#undef X
 
 camera->transform = (mat4i_translate(data->pan) *
                      mat4i_rotate_tpr(data->theta, data->phi, data->roll, vec3()) *
                      mat4i_translate(vec3z(data->distance)));
}

DLL_EXPORT game_api_export_return 
game_api_export(game_api_export_params)
{
#define X(N) api->N = N;
 //
 X_GAME_API_FUNCTIONS(X)
  //
#undef X
 api->is_valid = true;
}

global i32 MAIN_VIEWPORT_INDEX = MAIN_VIEWPORT_ID - 1;

inline i32
get_viewport_index(i32 viewport_id)
{
 kv_assert(viewport_id <= GAME_VIEWPORT_COUNT);
 return (viewport_id - 1);
}

internal b32
camera_data_equal(Camera_Data *a, Camera_Data *b)
{
 return block_match(a, b, sizeof(Camera_Data));
}

inline v1
animate_value(v1 start, v1 end, v1 dt, v1 difference_multiplier, v1 min_speed)
{
 macro_clamp_min(min_speed, 0.f);
 v1 abs_difference = absolute(end-start);
 v1 abs_delta = abs_difference * difference_multiplier;
 macro_clamp_min(abs_delta, min_speed*dt);
 macro_clamp_max(abs_delta, abs_difference);
 
 v1 result = (end > start) ? (start+abs_delta) : (start-abs_delta);
 return result;
}

global v1 CAMERA_DISTANCE_STEP         = 5.f  *centimeter;
global v1 CAMERA_PAN_STEP_PER_DISTANCE = 3.75f*centimeter;

internal b32
animate_camera(Camera_Data *current, Camera_Data *saved, v1 dt)
{
 current->distance_level = saved->distance_level;//@distance_level_nonsense
 b32 animation_ended = camera_data_equal(current, saved);
 if ( animation_ended )
 {
  saved->theta   = cycle01(saved->theta);
  current->theta = cycle01(current->theta);
 }
 else
 {
#define ANIMATE(FIELD, MIN_SPEED) \
current->FIELD = animate_value(current->FIELD, saved->FIELD, dt, 0.1f, MIN_SPEED)
  ANIMATE(theta,    0.004f);
  ANIMATE(phi,      0.004f);
  ANIMATE(distance, CAMERA_DISTANCE_STEP/3.0f);
  ANIMATE(pan.x,  CAMERA_PAN_STEP_PER_DISTANCE/3.0f);
  ANIMATE(pan.y,  CAMERA_PAN_STEP_PER_DISTANCE/3.0f);
  ANIMATE(pan.z,  CAMERA_PAN_STEP_PER_DISTANCE/3.0f);
#undef ANIMATE
  
  current->roll  = saved->roll;
 }
 
 saved  ->UNUSED_FIELD = {};
 current->UNUSED_FIELD = {};
 
 return animation_ended;
}

// TODO: We really should merge this with game_update, come on why are there two calls instead of one?
internal game_viewport_update_return
game_viewport_update(game_viewport_update_params)
{
 b32 should_animate_next_frame = false;
 i32 viewport_index = get_viewport_index(viewport_id);
 Viewport *viewport = &state->viewports[viewport_index];
 Game_Save *save = &state->save;
 {// NOTE: camera animtion
  Camera_Data *saved   = &save->camera[viewport_index];
  Camera_Data *current = &viewport->camera;
  b32 animation_ended = animate_camera(current, saved, dt);
  if ( !animation_ended ) { should_animate_next_frame = true; }
 }
 
 return should_animate_next_frame;
}

inline v1 
round_to_multiple_of(v1 value, v1 n)
{
 v1 result = roundv1(value / n) * n;
 return result;
}


force_inline Line_Params
line_color(Painter *p, argb color)
{
 Line_Params result = p->line_params;
 result.color = color;
 return result;
}

force_inline Bezier
reverse(Bezier B)
{
 return {B[3], B[2], B[1], B[0]};
}

force_inline v3
reflect_origin(v3 origin, v3 point)
{
 return origin-(point-origin);
}

internal argb
hsv_to_argb(v1 h, v1 s, v1 v)
{
 return argb_pack(srgb_to_linear(hsv_to_srgb(h,s,v)));
}

force_inline void
profile_visible(Painter *p, Line_Params *params, v1 edge1)
{
 if (p->profile_score > edge1) { params->visibility = 1.f; }
 else { params->visibility = 0.f; }
}
force_inline Line_Params
profile_visible(Painter *p, v1 edge1)
{
 Line_Params params = p->line_params;
 profile_visible(p, &params, edge1);
 return params;
}

internal i32
get_frame_index(i32 nframes, v1 t)
{
 v1 interval = 1.f / v1(nframes);
 i32 frame = i32(t / interval);
 // NOTE: nframes = nintervals, when t=1.0, frame=ninterval-1
 macro_clamp_max(frame, nframes-1);
 return(frame);
}

//~
struct Keyframe
{
 i16 nframes;
 i16 value;
};

struct Animation
{
 Keyframe keyframes[64];  // TODO: Need a dynamic allocation scheme here, if we'll be using one of these for each attribute
 i32 keyframe_count;
 i32 total_frames;
 b32 looping;
};

internal void
add_keyframe(Animation *ani, i32 nframes, i32 value)
{
 if ((nframes > 0) &&
     (ani->keyframe_count < alen(ani->keyframes)))
 {
  ani->keyframes[ani->keyframe_count++] = Keyframe
  {
   .nframes = i16(nframes),
   .value = i16(value),
  };
  ani->total_frames += nframes;
 }
}

force_inline void 
add_keyframe(Animation *ani, v2i args)
{
 add_keyframe(ani, args[0], args[1]);
}

inline void
begin_animation(Animation *ani, b32 looping)
{
 ani->keyframe_count = 0;
 ani->total_frames   = 0;
 ani->looping        = looping;
}

inline void
end_animation(Animation *ani) { }

internal v1
get_animation_value(Animation *ani, v1 global_t)
{
 if (ani->keyframe_count > 0 && 
     ani->total_frames > 0)
 {
  // NOTE: Let's animate on two's to start
  i32 frame_requested = (i32)(global_t * Animation_FPS);
  if (ani->looping)
  {
   frame_requested %= ani->total_frames;
  }
  
  i32 frame_index_sofar = ani->keyframes[0].nframes;
  i32 keyframe_index = 1;
  for (;
       keyframe_index < ani->keyframe_count &&
       (frame_requested > frame_index_sofar);
       keyframe_index++)
  {
   frame_index_sofar += ani->keyframes[keyframe_index].nframes;
  }
  // NOTE: linger on last frame if out of frames
  v1 result = cast(v1)(ani->keyframes[keyframe_index-1].value);
  return result;
 }
 else { return 0.f; }
}

//~NOTE: Old animation
struct Frame_Group
{
 v4 values;
 v1 weight;
 v1 begin_time;
};

struct Animation_Old
{
 Frame_Group groups[16];
 i32 group_count;
};

internal void
add_frame_group(Animation_Old *ani, v1 weight, v4 values)
{
 if (ani->group_count < alen(ani->groups))
 {
  auto &v = values;
  ani->groups[ani->group_count++] = Frame_Group
  {
   .values = vec4(macro_control_points(v[0],v[1],v[2],v[3])),
   .weight = weight,
  };
 }
}
internal void 
begin_animation(Animation_Old *ani)
{
 ani->group_count = 0;
}

internal void
end_animation(Animation_Old *ani)
{
 v1 total_weight = 0.f;
 for_i32(group_index, 0, ani->group_count)
 {
  total_weight += ani->groups[group_index].weight;
 }
 
 v1 begin_time = 0.f;
 for_i32(group_index, 0, ani->group_count)
 {
  Frame_Group *group = &ani->groups[group_index];
  group->begin_time = begin_time;
  
  begin_time += group->weight / total_weight;
 }
 
 if (ani->group_count < alen(ani->groups))
 {
  ani->groups[ani->group_count++] = Frame_Group { .begin_time = 1.f, };
 }
 else  { todo_error_report; }
}

internal v1
get_animation_value(Animation_Old *ani, v1 global_t)
{
 macro_clamp01(global_t);
 Frame_Group *group = 0;
 v1 end_time;
 i32 group_index = 1;
 for(;
     group_index < ani->group_count && group == 0;
     group_index++)
 {
  Frame_Group *test_group = &ani->groups[group_index];
  if (test_group->begin_time >= global_t)
  {
   group    = &ani->groups[group_index-1];
   end_time = ani->groups[group_index].begin_time;
  }
 }
 
 if (group)
 {
  v1 result;
  v1 local_t = unlerp(group->begin_time, global_t, end_time);
  auto &values = group->values;
  if      (local_t < 0.25f) { result = values[0]; }
  else if (local_t < 0.50f) { result = values[1]; }
  else if (local_t < 0.75f) { result = values[2]; }
  else                      { result = values[3]; }
  return result;
 }
 else { return 69.f; }
}
//~

// NOTE
#include "game_utils.cpp"
#include "game_csg.cpp"
// NOTE: You can't nest "optimize on" inside of "optimize off", 
// but we don't wanna optimize this file anyway, so it's whatever
#pragma clang optimize off
#include "game.cpp"
#include "game_physics.cpp"
#pragma clang optimize on

internal game_render_return
game_render(game_render_params)
{//;switch_game_or_physics
 if( fbool(1) )
 {
  render_movie(state, app, viewport_id, mouse, csg_buffer);
 }
 else
 {
  render_physics(state, app, viewport_id, mouse, csg_buffer);
 }
}

internal void
update_fui_stores(Game_State *state)
{
 fui_stores       = state->fui_store;
 fui_stores_count = alen(state->fui_store);
 //NOTE: Filename pointer changes every time you reload the DLL!
 Fui_Store *stores = state->fui_store;
 stores[0].filename = GAME_CPP_FILENAME;
}

internal game_init_return
game_init(game_init_params)
{
 // API import
#define X(N) N = ed_api->N;
 X_ED_API_FUNCTIONS(X)
#undef X
 Game_State *state = push_struct(bootstrap_arena, Game_State);
 state->permanent_arena = *bootstrap_arena;
 Arena *arena = &state->permanent_arena;
 
 {// NOTE: Save/Load business
  Scratch_Block scratch(app);
  String binary_dir = system_get_path(scratch, SystemPath_BinaryDirectory);
  state->save_dir  = pjoin(arena, binary_dir, "data");
  state->save_path = pjoin(arena, state->save_dir, "data.kv");
  
  String read_string = read_entire_file(scratch, state->save_path);
  if (read_string.len)
  {
   Game_Save *read = (Game_Save *)read_string.str;
   if (read->magic_number == data_magic_number)
   {
    if (read->version == data_current_version)
    {
     state->save = *read;
    }
    else if ( read->version == (data_current_version-1) )
    {
     print_message(app, strlit("Game data load: Converting old version to new version!\n"));
     Game_Save_Old *old = (Game_Save_Old *)read;
     {// NOTE(kv): Conversion code!
      //state->save = *(Game_Save *)old;
     }
    }
    else {print_message(app, strlit("Game data load: Unknown version\n"));}
   }
   else  {print_message(app, strlit("Game data load: Wrong magic number!\n"));}
  }
  else {print_message(app, strlit("Game data load: can't load the file!\n"));}
  
  // NOTE: No matter what, init the save data
  state->save.magic_number = data_magic_number;
  state->save.version      = data_current_version;
 }
 
 {// NOTE: Fui sliders init (Not reloaded so can't change slider count on the fly)
  // NOTE: Zero element is invalid
  i32 store_index = 0;
  {
   i32 count = 4096;
   state->fui_store[store_index] = {
    .sliders  = push_array(arena, Fui_Slider, count),
    .count    = count,
   };
  }
  update_fui_stores(state);
 }
 
 for_i32(viewport_index,0,GAME_VIEWPORT_COUNT)
 {//;frame_arena_init
  usize frame_arena_size = KB(32);
  state->viewports[viewport_index].frame_arena = make_sub_arena(arena, frame_arena_size);
 }
 
 {//NOTE: Making primitive meshes!
  state->mesh_arena = make_sub_arena(arena, KB(8));
  make_meshes(&state->mesh_arena);
 }
 
 return state;
}

internal game_reload_return
game_reload(game_reload_params)
{
 // API import
#define X(N) N = ed_api->N;
 X_ED_API_FUNCTIONS(X)
#undef X
 
 {
  //TODO: @Speed Maybe iterating and setting "inited" flag is faster?
  for_i32(store_index,0,alen(state->fui_store))
  {
   Fui_Store *store = &state->fui_store[store_index];
   block_zero(store->sliders, sizeof(Fui_Slider)*store->count);
  }
  update_fui_stores(state);
 }
 
 {// NOTE: re-make meshes
  make_meshes(&state->mesh_arena);
 }
}

#include "time.h"
internal b32
save_game(App *app, Game_State *state)
{
 Scratch_Block scratch(app);
 String backup_dir = pjoin(scratch, state->save_dir, "backups");
 char *save_path_cstr = to_c_string(scratch, state->save_path);
 
 b32 ok = true;
 if (!state->has_done_backup &&
     gb_file_exists(save_path_cstr))
 {
  // NOTE: Backup game if is_first_write_since_launched
  time_t rawtime;
  time(&rawtime);
  struct tm *timeinfo = localtime(&rawtime);
  char time_string[256];
  size_t strftime_result = strftime(time_string, sizeof(time_string), "%d_%m_%Y_%H_%M_%S", timeinfo);
  if (strftime_result == 0)
  {
   print_message(app, str8lit("strftime failed... go figure that out!\n"));
   ok = false;
  }
  else
  {
   String backup_path = push_stringf(scratch, "%.*s/data_%s.kv", string_expand(backup_dir), time_string);
   ok = move_file(state->save_path, backup_path);
   if (ok) state->has_done_backup = true;
  }
  
  if (ok)
  {// NOTE: cycle out old backup files
   File_List backup_files = system_get_file_list(scratch, backup_dir);
   if (backup_files.count > 100)
   {
    u64 oldest_mtime = U64_MAX;
    String file_to_delete = {};
    File_Info **opl = backup_files.infos + backup_files.count;
    for (File_Info **backup = backup_files.infos;
         backup < opl;
         backup++)
    {
     File_Attributes attr = (*backup)->attributes;
     if (attr.last_write_time < oldest_mtime)
     {
      oldest_mtime   = attr.last_write_time;
      file_to_delete = pjoin(scratch, backup_dir, (*backup)->filename);
     }
    }
    b32 delete_ok = remove_file(file_to_delete);
    if (delete_ok) printf_message(app, "INFO: deleted backup file %.*s because it's too old", string_expand(file_to_delete));
    else           printf_message(app, "ERROR: failed to delete backup file %.*s", string_expand(file_to_delete));
   }
  }
 }
 
 if (ok)
 {// NOTE: save the file
  ok = write_entire_file(scratch, state->save_path, &state->save, sizeof(state->save));
  if (ok)
   vim_set_bottom_text(str8lit("Saved game state!"));
  else
   printf_message(app, "Failed to write to file %.*s", string_expand(state->save_path));
 }
 
 return ok;
}

// TODO: Input handling: how about we add a callback to look at all the events and report to the game if we would process them or not?
internal game_update_return
game_update(game_update_params)
{
 b32 game_active = active_viewport_id != 0;
 b32 game_hot = (game_active != 0 || fui_is_active());
 
 i32 update_viewport_id = (active_viewport_id ? active_viewport_id : 1);
 
 Game_Input *input = &input_value;
 
 b32 should_animate_next_frame = false;
 kv_assert(active_viewport_id <= GAME_VIEWPORT_COUNT);
 i32 update_viewport_index = update_viewport_id - 1;
 Viewport *update_viewport = &state->viewports[update_viewport_index];
 
 v1 dt = input->dt;
 {
  state->time += dt;
  if (state->time >= 1000.0f) { state->time -= 1000.0f; }
 }
 
 if (game_hot)
 {
  should_animate_next_frame = true;
 }
 
 Arena *permanent_arena = &state->permanent_arena;
 
 Scratch_Block scratch(app);
 
 Game_Save *save = &state->save;
 if ( is_key_newly_pressed_no_mod(input, Key_Code_Return) )
 {
  save_game(app, state);
 }
 
 Camera camera_value = {};
 Camera *camera = &camera_value;
 {// NOTE: camera init
  setup_camera(camera, &update_viewport->camera);
 }
 
 if (game_hot)
 {// NOTE: Camera update
  Camera_Data *save_camera = &save->camera[update_viewport_index];
  
  if (is_key_newly_pressed_no_mod(input, Key_Code_X))
  {
   save_camera->theta *= -1.f;
  }
  
  v3 ctrl_direction = key_direction(input, Key_Mod_Ctl, true);
  if (ctrl_direction == vec3() && game_active)
  {
   ctrl_direction = key_direction(input, 0, true);
  }
  
  {// NOTE: Zoom levels
   i32 *level = &save_camera->distance_level;
   {
    v1 dz = ctrl_direction.z;
    if (dz > 0.f)      { (*level)++; }
    else if (dz < 0.f) { (*level)--; }
    macro_clamp_min((*level), 0);
   }
   {//note: Syncing the distance with the distance level
    v1 distance = 5.f*centimeter;
    v1 mult = 1.2f;
    for_i32(whatever,0,*level) { distance *= mult; }
    save_camera->distance = distance;
   }
  }
  
  {// NOTE: pan
   v1 distance = camera->distance;
   if (distance == 0) { distance = 0.5f; }
   v1 step = CAMERA_PAN_STEP_PER_DISTANCE * distance;  // distance = .92m -> 3*pivot_step
   v3 pan = save_camera->pan / step;
   
   v2 delta_pan = key_direction(input, Key_Mod_Alt, true).xy;
   pan += (delta_pan.x * camera->x.xyz + 
           delta_pan.y * camera->y.xyz);
   
   save_camera->pan = step*pan;
  }
  
  {
   v1 interval = 1.0f / 24.f;
   v1 theta = roundv1(save_camera->theta / interval);
   v1 phi   = roundv1(save_camera->phi   / interval);
   {
    v2 delta = ctrl_direction.xy;
    theta += delta.x;
    phi   += delta.y;  // NOTE: pitch up when we go up
   }
   
   save_camera->theta = theta * interval;
   save_camera->phi   = phi * interval;
   macro_clamp(-0.25f, save_camera->phi, 0.25f);
  }
  
  {// NOTE: roll
   i32 roll_direction = 0.f;
   if ( is_key_down(input, Key_Mod_Sft, Key_Code_Period) ) {roll_direction = +1;}
   if ( is_key_down(input, Key_Mod_Sft, Key_Code_Comma) )  {roll_direction = -1;}
   save_camera->roll += 0.1f * (v1)roll_direction * dt;
  }
  
  if ( is_key_down(input, Key_Mod_Sft, Key_Code_0) ) { save_camera->roll  = {}; }
 }
 
 if (game_hot)
 {
  if ( is_key_newly_pressed_no_mod(input, Key_Code_Space) )
  {
   game_last_preset(state, update_viewport_id);
  }
 }
 
 if ( fui_is_active() )
 {// NOTE: Update Fui slider
  if (is_key_newly_pressed_no_mod(input, Key_Code_Tab))
  {
   b32 &zw = fui_v4_zw_active;
   zw = !zw;
  }
  
  Fui_Slider *slider = fui_active_slider;
  if (slider->type == Fui_Type_i32 || 
      slider->type == Fui_Type_v2i ||
      slider->type == Fui_Type_v3i ||
      slider->type == Fui_Type_v4i)
  {
   v4i *value = &slider->value.v4i;
   v4 direction = key_direction_v4(input, 0, true);
   for_i32(index,0,4)
   {
    value->v[index] += i32(direction[index]);
   }
   
   if (slider->flags & Fui_Flag_Clamp_01)
   {
    for_i32(index,0,4)
    {
     macro_clamp01i(value->v[index])
    }
   }
  }
  else
  {
   v4 *value = &slider->value.v4;
   
   b32 j_pressed = is_key_newly_pressed_no_mod(input, Key_Code_J);
   b32 k_pressed = is_key_newly_pressed_no_mod(input, Key_Code_K);
   if (slider->type == Fui_Type_v1 && (j_pressed || k_pressed))
   {
    if (j_pressed) { value->x = 0.f; }
    if (k_pressed) { value->x = 1.f; }
   }
   else
   {
    b32 shifted;
    v4 direction = key_direction_v4(input, 0, false, &shifted);
    // NOTE: Switch active pair
    if (fui_v4_zw_active &&
        slider->type == Fui_Type_v4)
    {
     direction.zw = direction.xy;
     direction.xy = {};
    }
    if (slider->flags & Fui_Flag_Camera_Aligned)
    {// NOTE(kv): I don't know what would happens with w, but we don't care rn
     direction = noz( camera->forward * direction );
    }
    v1 delta_scale = slider->delta_scale;
    if (delta_scale == 0) { delta_scale = 0.2f; }
    v4 delta = delta_scale * dt * direction;
    if (shifted) { delta *= 10.f; }
    *value += delta;
    
    if (slider->flags & Fui_Flag_Clamp_X)  {value->x = 0;}
    if (slider->flags & Fui_Flag_Clamp_Y)  {value->y = 0;}
    if (slider->flags & Fui_Flag_Clamp_Z)  {value->z = 0;}
    
    if (slider->flags & Fui_Flag_NOZ)
    {
     value->xyz = noz(value->xyz);
    }
    
    if (slider->flags & Fui_Flag_Clamp_01)
    {
     for_i32(index,0,4)
     {
      macro_clamp01(value->v[index])
     }
    }
   }
  }
 }
 
 {
  Camera_Data *cam = &update_viewport->camera;
  if (input->debug_camera_on)
  {
   DEBUG_NAME("camera(theta,phi,distance)", vec3(cam->theta, cam->phi, camera->distance));
   DEBUG_NAME("camera(pan)", cam->pan);
  }
 }
 
 for_i32 (index, 0, GAME_VIEWPORT_COUNT)
 {// NOTE: Set viewport presets to useful values
  Viewport *viewport = &state->viewports[index];
  
  if (viewport->preset == viewport->last_preset)
  {
   if (viewport->preset == 0) { viewport->last_preset = 2; }
   else { viewport->last_preset = 0; }
  }
  
  //;frame_arena_clear (@speed We're re-allocating memory every frame, no good)
  arena_clear(&viewport->frame_arena);
 }
 
 update_game_config();
 
 return should_animate_next_frame;
}


#undef WARN_DELTA
#undef viz_block
#undef funit

//~EOF
