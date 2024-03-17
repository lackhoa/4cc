global const v1 Animation_FPS = 12.f;

struct Movie_Shot
{
 Arena *arena;
 
 b32 ready;
 i32 current_frame;
 i32 requested_frame;
 
 //-NOTE: The data
 union {
  v3i head_rotation;
  v2i head_theta_phi;
  struct { v1i head_theta; v1i head_phi; v1i head_roll; };
 };
 v1i tblink;
 v1i eye_theta;
 v1i eye_phi;
};

internal void
shot_rest_function(Movie_Shot *shot, i32 nframes)
{
 shot->current_frame += nframes;
 if (shot->requested_frame < shot->current_frame)
 {
  shot->ready = true;
 }
}

#define shot_rest(NFRAMES) shot_rest_function(shot, NFRAMES)

#define shot_set(FIELD, VALUE) if (!shot->ready) { shot->FIELD = VALUE; }

#define shot_add(FIELD, VALUE) if (!shot->ready) { shot->FIELD = shot->FIELD + VALUE; }

#define shot_function_return void
#define shot_function_params Movie_Shot *shot
//
typedef shot_function_return Shot_Function(shot_function_params);

internal void
shot__init(Movie_Shot *shot, i32 requested_frame)
{
 *shot = {};
 shot->requested_frame = requested_frame;
}

internal void
shot_go(Movie_Shot *shot, Shot_Function shot_function, v1 animation_time)
{
 i32 requested_frame = i32(Animation_FPS * animation_time);
 shot__init(shot, requested_frame);
 
 // NOTE: prepass
 shot_function(shot);
 
 i32 frame_count = shot->current_frame;
 shot__init(shot, requested_frame);
 if (frame_count > 0)
 {
  // NOTE: looping by default
  shot->requested_frame %= frame_count;
  
  // NOTE: real pass
  shot_function(shot);
 }
}
