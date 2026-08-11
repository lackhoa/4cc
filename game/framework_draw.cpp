//-;framework_draw
global u32 draw_cycle_counter;

//-NOTE(kv) Replay vertex tee (draw-as-data step 3): when `global_vertex_tee` is set,
// poly3_inner also appends every recorded-scope push into it, so the code path and
// the replay can be diffed as raw vertex streams (see game_replay.cpp).
struct Vertex_Tee_Entry
{
 Location location;  // painter->current_draw_location at push time
 Vertex_Type type;
 i32 vertex_count;   // its vertices live in Vertex_Tee.vertices, in push order
};
struct Vertex_Tee
{
 darray(Render_Vertex) vertices;
 darray(Vertex_Tee_Entry) entries;
};
global Vertex_Tee *global_vertex_tee;
// NOTE(kv) Q24: mutes the platform push in poly3_inner (the tee still records) --
// lets the diff re-run a path without double-drawing.
global b32 global_rendering_suppressed;
// NOTE(kv) Rendering mode B: recorded-scope pushes from the CODE path are muted and
// the replay draws that scope instead. Only raised around driver_render (game_main.cpp)
// so post-render drawing (cursor, vertex indicators) is unaffected.
global b32 global_replay_display;
//-

function void
poly3_inner(Poly3 points,
            argb c0, argb c1, argb c2,
            Poly_Flags flags)
{// NOTE framework_driver_api.kt
 // NOTE(kv) This temp array is ridiculous, but whatevs.
 argb colors[3] = {c0,c1,c2};
 
 TIMED_BLOCK(draw_cycle_counter);
 b32 is_line    = (flags.v & Poly_Line);
 v1 depth_offset = (is_line ?
                    painter->params.line_depth_offset :
                    painter->params.fill_depth_offset);
 b32 shaded     = painter->shading_on and not(is_line);
 b32 is_overlay = (flags.v & Poly_Overlay);
 
 Render_Vertex vertices[3] = {};
 for_i32(i,0,3){ vertices[i].pos = points[i]; }
 
 if(shaded)
 {// ;poly_shading
  v3 normal = noz(cross(points[1]-points[2], points[0]-points[2]));
  for_i32(i,0,3)
  {
   v4 colorv4 = argb_unpack(colors[i]);
   v1 darkness_fuzz = 0.3f;
   colorv4.rgb *= lerp(darkness_fuzz, absolute(normal.z), 1.f);
   colors[i] = argb_pack(colorv4);
  }
 }
 
 if(current_location_is_hot())
 {
  for_i32(i,0,3){ colors[i] = hot_color; }
  is_overlay = true;
 }
 
 for_i32(i,0,3){ vertices[i].color = colors[i]; }
 
 for_u32(index, 0, alen(vertices))
 {
  Render_Vertex *vertex = vertices+index;
  vertex->uvw          = V3();
  vertex->depth_offset = depth_offset;
 }
 
 Vertex_Type type = Vertex_Poly;
 if(is_overlay){ type = Vertex_Overlay; }

 // NOTE(kv) Q25: single game-side funnel for ALL geometry -> tee + mute live here.
 b32 in_recorded_scope = should_send_model_data();
 if(global_vertex_tee and in_recorded_scope)
 {
  Vertex_Tee_Entry entry = {painter->current_draw_location, type, (i32)alen(vertices)};
  push(&global_vertex_tee->entries, entry);
  for_u32(i, 0, alen(vertices)){ push(&global_vertex_tee->vertices, vertices[i]); }
 }
 b32 muted = (global_rendering_suppressed or
              (global_replay_display and in_recorded_scope));
 if(not muted)
 {
  draw__push_vertices(painter->target, ArrayAndCount(vertices), type);
 }
}
function void
draw_bezier_inner(tvert P[4], Line_Params &params, argb base_color)
{// NOTE @framework_api_1
 // NOTE(kv) I've thought about rendering in camera space,
 // but then we're gonna have to convert to camera space in our fills, too.
 // If you optimize for one case, you're gonna screw something else.
 // Of course we should optimize curves, but we're far from optimized anyway so who cares.
 Paint_Params &cparams = painter->params;
 v4 radii = params.radii;
 if(radii == v4{}){ radii = V4(.25, 1, 1, .25); }
 radii *= cparams.radius_mult * default_line_radius_unit;
 
 b32 is_straight = params.flags & Line_Straight;
 if(is_straight)
 {// TODO(kv) Straight line... has different radii logic man...
  for_i32(i,0,4) { radii[i] = radii[1]; }
 }
 
 b32 do_stamp = tweaks->force_stamp_rendering;
 Line_Flags flags = params.flags;
 i32 nslices;
 b32 clipped = false;
 if(is_straight)
 {
  nslices = 1;
 }
 else
 {//-Pre-pass
  i32 const nsegments = 8;
  v1 interval = 1.f / (v1)nsegments;
  
  {//-Clipping
   v4 P_clip[4];
   for_i32(i,0,4)
   {
    P_clip[i] = matvmul(painter->clip_from_bone, V4(P[i], 1.f));
   }
   
   i32 const ndim = 4;
   Range_f32 ranges[ndim];
   {
    for_i32(idim, 0, ndim)
    {// NOTE initialize
     ranges[idim].min = f32_max;
     ranges[idim].max = -f32_max;
    }
    
    for_i32(icontrol_point,0,4)
    {// NOTE compute ranges
     v4 control_point = P_clip[icontrol_point];
     for_i32(idim,0,ndim)
     {
      ClampTop(ranges[idim].min, control_point[idim]);
      ClampBot(ranges[idim].max, control_point[idim]);
     }
    }
   }
   
   clipped = false;
   Range_f32 w_range = ranges[3];
   for_i32(idim, 0, 3)
   {
    if(ranges[idim].min > +w_range.max or
       ranges[idim].max < -w_range.min)
    {
     clipped = true;
     break;
    }
   }
   
#if 0
   for_i32(sample_index, 0, nsegments+1)
   {// NOTE(kv) Pretty hacked way to cull the curve, but whatevs
    b32 sample_clipped = false;
    v1 t = sample_index * interval;
    v4 sample_clip = bezier_sample(P_clip, t);
    v1 w = sample_clip.w;
    for_i32(component,0,3)
    {
     if(sample_clip[component] < -w or
        sample_clip[component] > +w)
     {// NOTE Point is clipped iff one component is out of bound.
      sample_clipped = true;
      clipped_samples[sample_index] = true;
      break;
     }
    }
    
    if(not sample_clipped)
    {// NOTE The whole curve is clipped iff all samples are clipped.
     clipped = false;
     break;
    }
   }
#endif
  }
  
  if(not clipped)
  {//-Computing level of detail
   // NOTE(kv) Working in 2D is no good, because samples move in 3D,
   // so they might be traveling longer distances due to the depth, 
   // and spaced out more when we look at them in 3D -> you'd underestimate the density in 2D.
   Bezier P_camera;
   for_i32(index,0,4)
   {
    P_camera[index] = painter->cam_from_bone * P[index];
   }
   
   v1 length_projected = 0.f;
   {
    v3 cam_sample0;
    v1 near_clip_z = -painter->camera.near_clip;
    v1 focal_length = painter->camera.focal_length;
    for_i32(sample_index, 0, nsegments+1)
    {
     v1 t = interval * (v1)sample_index;
     v3 cam_sample = bezier_sample(P_camera,t);
     if(sample_index != 0)
     {
      v1 slice_average_cam_z = 0.5f*(cam_sample.z + cam_sample0.z);
      b32 not_near_clipped = slice_average_cam_z < near_clip_z;
      if(not_near_clipped)
      {
       kv_assert(slice_average_cam_z < 0.f);
       length_projected += (lengthof(cam_sample - cam_sample0) *
                            (focal_length / -slice_average_cam_z));
      }
     }
     cam_sample0 = cam_sample;
    }
   }
   
   v1 nslices_v1 = cparams.nslice_per_meter * length_projected;
   if(do_stamp)
   {
    nslices_v1 *= tweaks->stamp_density_mult;
   }
   nslices = i32(nslices_v1)+1;
  }
 }
 
 painter->total_curve_count++;
 if(clipped)
 {
  painter->clipped_curve_count++;
 }
 
 if(not clipped)
 {
  Poly_Flags poly_flags = {};
  poly_flags.v |= Poly_Line;
  if(flags & Line_Overlay){ poly_flags.v |= Poly_Overlay; }
  
  // NOTE: Clean up ugly tiny lines (and zeros), possibly unnecessary?
  v1 radius_threshold = 0.1065f*millimeter;
  ClampBot(radius_threshold, 0.f);
  
  v1 interval = 1.0f / (v1)nslices;
  
  if(do_stamp)
  {//-stamp rendering
   i32 const npoly = 6;
   v3 offsets_in_bone[npoly];
   {
    v1 phase = tweaks->stamp_rotation_speed * painter->looping_time;
    v2 offsets_in_camera[npoly];
    for_i32(index, 0, npoly)
    {
     offsets_in_camera[index] = arm2(phase + v1(index) / v1(npoly));
    }
    for_i32(index, 0, npoly)
    {
     mat4 &bone_from_cam = painter->cam_from_bone.inverse;
     v3 offset = V3(offsets_in_camera[index], 0.f);
     offsets_in_bone[index] = mat4vec(bone_from_cam, offset);
    }
   }
   
   for_i32(sample_index, 0, nslices)
   {
    v1 t = interval * (v1)sample_index;
    v3 sample = bezier_sample(P,t);
    v1 radius = bezier_sample(radii,t);
    v3 points[npoly];
    for_i32(index, 0, npoly){
     points[index] = sample + radius*offsets_in_bone[index];
    }
    for_i32(index, 0, npoly)
    {
     i32 next_index = index+1;
     if(index == npoly-1){ next_index = 0; };
     poly3_inner({sample, points[index], points[next_index]},
                 repeat3(base_color), poly_flags);
    }
   }
  }
  else
  {//-non stamp rendering
   v3 last_sample = bezier_sample(P,-interval);  // NOTE: So we can calculate C and D at t=0
   v1 last_radius;
   v3 A0,B0;
   v4 lightness_additions = params.lightness_additions;
   v4 base_color_v4 = argb_unpack(base_color);
   argb color0 = base_color;
   for_i32(sample_index, 0, nslices+1)
   {
    v1 t = interval * (v1)sample_index;
    v3 sample = bezier_sample(P,t);
    v1 radius = bezier_sample(radii,t);
    v1 lightness_addition = bezier_sample(lightness_additions, t);
    if(tweaks->ignore_lightness_additions)
    {
     lightness_addition = 0.f;
    }
    v4 color_v4 = argb_lightness(base_color_v4, 1.f+lightness_addition);
    argb color = argb_pack(color_v4);
    
    v3 A,B;
    {
     v3 perp1;
     {
      v3 d_camera_space = mat4vec(painter->cam_from_bone, (sample - last_sample));
      v2 perp_unit = noz( perp(d_camera_space.xy) );
      perp1 = mat4vec(painter->cam_from_bone.inverse, V3(radius*perp_unit, 0.f));
     }
     A = sample+perp1;
     B = sample-perp1;
    }
    
    b32 do_draw_slice = (sample_index > 0
                         and 0.5f*(radius+last_radius) > radius_threshold);
    if(do_draw_slice)
    {
     poly3_inner({A0,A,B},  color0, color, color, poly_flags);
     poly3_inner({A0,B0,B}, color0, color0, color, poly_flags);
    }
    
    if(sample_index == 0 or
       sample_index == nslices)
    {//-Draw the circular endpoints
     if(radius > radius_threshold)
     {
      fill_disk_camera_space(sample, radius, color, poly_flags);
     }
    }
    
    A0 = A;
    B0 = B;
    last_sample = sample;
    last_radius = radius;
    color0 = color;
   }
  }
 }
}
//-