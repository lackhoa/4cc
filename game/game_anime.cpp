global_const v1 Animation_FPS = 12.f;

//NOTE: Thank you C++, this could have just been simple.
enum Pose_Field_Enum{
#define X(NAME,...)  PF_Enum_##NAME,
 X_Pose_Fields(X)
#undef X
};

//NOTE: Metadata about pose fields
struct Pose_Field_Info{
 i1 offset;
 v1 denom;
};
//
#if 0
jump pose_field_info;
#endif
//
global_const Pose_Field_Info pose_field_info[] = {
#define X(NAME,DENOM) \
Pose_Field_Info{ \
.offset = i1(offsetof(Pose, NAME)), \
.denom  = DENOM, \
},
 //
 X_Pose_Fields(X)
  //
#undef X
};

struct Movie_Shot
{
 v1 anime_time;
 
 b32 ready;
 i1 current_frame;
 i1 requested_frame;
 
 Pose *updated_pose;
 Pose out_pose;
};

// NOTE: Reroute destination so that updates don't do anything.
global Pose dummy_pose;

internal void
arest_function(Movie_Shot *shot, i1 nframes)
{
 shot->current_frame += nframes;
 if (shot->requested_frame < shot->current_frame) {
  shot->ready = true;
  shot->updated_pose = &dummy_pose;
 }
}

internal void
aset_func_float(Movie_Shot *shot, Pose_Field_Enum field, v1 value)
{
 u8 *pose = cast(u8*)(shot->updated_pose);
 u8 *dst  = pose + pose_field_info[field].offset;
 block_copy(dst, &value, sizeof(value));
}

internal void
aset_func_int(Movie_Shot *shot, Pose_Field_Enum field, i1 value_int)
{
 v1 value = i2f(value_int, pose_field_info[field].denom);
 aset_func_float(shot, field, value);
}

#define arest(NFRAMES)     arest_function(shot, NFRAMES)
//TODO(kv): The indirections still make me grumpy.
//#define aset(FIELD, VALUE) shot->updated_pose->FIELD  = i2f(VALUE, pose_field_denoms[pf]);
#define aset(FIELD, VALUE)   aset_func_int(shot, PF_Enum_##FIELD, VALUE)
#define asetf(FIELD, VALUE)  aset_func_float(shot, PF_Enum_##FIELD, VALUE)

#define shot_function_return void
#define shot_function_params Movie_Shot *shot
//
typedef shot_function_return Shot_Function(shot_function_params);

//NOTE(kv): We don't zero the pose data,
// so we can play animations on top of each other.
internal void
shot__init(Movie_Shot *shot, i1 requested_frame, Pose *updated_pose)
{
 shot->ready           = false;
 shot->current_frame   = 0;
 shot->requested_frame = requested_frame;
 shot->updated_pose    = updated_pose;
}

internal void
shot_go(Movie_Shot *shot, Shot_Function shot_function)
{
 //NOTE: animation_time has to be filled in
 i1 requested_frame = i1(Animation_FPS * shot->anime_time);
 shot__init(shot, requested_frame, &dummy_pose);
 
 // NOTE: Prepass
 shot_function(shot);
 
 i1 frame_count = shot->current_frame;
 shot__init(shot, requested_frame, &shot->out_pose);
 if (frame_count > 0)
 {
  // NOTE: looping by default
  shot->requested_frame %= frame_count;
  
  // NOTE: real pass
  shot_function(shot);
 }
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
  Frame_Group *group = &ani->groups[ani->group_count++];
  *group = Frame_Group{};
  {
   group->values = V4(macro_control_points(v[0],v[1],v[2],v[3]));
   group->weight = weight;
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
  Frame_Group *group = &ani->groups[ani->group_count++];
  *group=Frame_Group{};
  group->begin_time = 1.f;
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

//~ NOTE: how many animation do you have?
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
  Keyframe *frame = &ani->keyframes[ani->keyframe_count++];
  *frame = Keyframe{};
  {
   frame->nframes = i16(nframes);
   frame->value = i16(value);
  };
  ani->total_frames += nframes;
 }
}

force_inline void 
add_keyframe(Animation *ani, i2 args)
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



//~
