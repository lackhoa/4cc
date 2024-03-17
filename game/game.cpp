// Stuff: @set_movie_shot

global char* GAME_CPP_FILENAME = __FILE_NAME__;  // update_fui_stores

inline Line_Params
viz_line_params(Painter *p, argb color=0)
{
 if (color == 0) { color = argb_red; }
 Line_Params params = p->line_params;
 params.flags |= Line_Overlay|Line_No_SymX;
 params.color = color;
 params.radii = fci(vec4i(3,3,3,3));
 return params;
}

internal shot_function_return
movie_shot_blinking(shot_function_params)
{
 shot_set(eye_theta, 3);
 shot_rest(8);
 shot_set(eye_theta, -2);
 shot_rest(1);
 shot_set(eye_theta, -3);
 shot_rest(4);
 
 shot_set(tblink, 1);
 shot_rest(1);
 shot_set(tblink, 6);
 shot_rest(1);
 shot_set(tblink, 6);
 shot_rest(1);
 shot_set(tblink, 1);
 shot_set(eye_theta, 0);
 shot_rest(1);
 shot_set(tblink, 0);
 shot_rest(12);
}

internal void
blink_2_frames(Movie_Shot *shot)
{
 shot_set(tblink, 1);
 shot_rest(1);
 shot_set(tblink, 6);
 shot_rest(1);
}

internal void open_eyes(Movie_Shot *shot) { shot_set(tblink,0); }

internal shot_function_return
movie_shot_0(shot_function_params)
{
#define set shot_set
#define rest shot_rest
 
 {// NOTE: right tilt
  set(head_theta, 0);
  rest(2);
  blink_2_frames(shot);
  
  set(head_theta, 1);
  rest(1);
  
  set(head_theta, -6);
  open_eyes(shot);
  rest(6);
 }
 {// NOTE: up tilt
  set(head_phi, fvali(-1));
  rest(1);
  set(head_phi, fvali(-2));
  rest(6);
  blink_2_frames(shot);
 }
 {// NOTE: left tilt
  set(head_theta_phi, fval2i(-8,-3));
  rest(1);
  set(head_theta_phi, fval2i(-7, 0));
  rest(1);
  set(head_theta, 4);  // NOTE: need this in-between, otw it feels really jerky!
  open_eyes(shot);
  rest(1);
 }
 set(head_theta, 6); // rest
 rest(8);
 
#undef set
#undef rest
}

internal game_render_return
render_movie(game_render_params)
{
 kv_assert(viewport_id <= GAME_VIEWPORT_COUNT);
 i32 viewport_index = viewport_id - 1;
 b32 is_main_viewport = (viewport_id == 1);
 
 v1 default_fvert_delta_scale = 0.1f;
 i32 viz_level = 0;
 b32 show_eyeball = false;
 b32 level0, level1, level2;
 Viewport *viewport = &state->viewports[viewport_index];
 viewport_frame_arena = &viewport->frame_arena;  // ;set_viewport_frame_arena
 Arena *arena = viewport_frame_arena;
 global_csg_group = push_struct_zero(viewport_frame_arena, CSG_Group);  //;set_global_csg_group
 b32 show_grid = fbool(0);
 b32 show_loomis_ball = fbool(0);
 {
  switch( viewport->preset )
  {
   case 1:
   {
    show_grid = fbool(1);
    viz_level = 1;
   }break;
   case 2:
   {
    viz_level = 2;
    show_eyeball = true;
   }break;
   case 3:
   {
    show_grid = fbool(1);
    show_loomis_ball = fbool(1);
   }
   case 4:
   {
    show_grid = fbool(1);
    show_loomis_ball = fbool(0);
   }
  }
  
  level0 = true;
  level1 = viz_level >= 1;
  level2 = viz_level >= 2;
  if (fbool(0)) { if (level1) { show_grid = true; }; }
  if (level2) { show_loomis_ball = true; }
 }
 
 Camera camera_value;
 Camera *camera = &camera_value;
 Game_Save *save = &state->save;
 {// NOTE: camera
  setup_camera(camera, &viewport->camera);
 }
 b32 camera_frontal=almost_equal(absolute(camera->z.z), 1.f, 1e-2);
 b32 camera_profile=almost_equal(absolute(camera->z.x), 1.f, 1e-2);
 
 Render_Target *render_target = draw_get_target(app);
 Render_Config old_render_config;
 if (Render_Config *old_render_config_pointer = target_last_config(render_target))
 {
  old_render_config = *old_render_config_pointer;
 }
 else { old_render_config = {}; }
 
 mat4 camera_transform;
 v3 background_hsv = fval3( 0.069f, 0.0648f, 0.5466f);
 v4 background_v4 = srgb_to_linear(hsv_to_srgb(background_hsv));
 argb background_color = argb_pack(background_v4);
 
 mat4 view_transform;
 {
  b32 orthographic = fbool(1) && (camera_frontal || camera_profile);
  view_transform = camera_view_matrix(camera, orthographic);
 }
 
 v2 mouse_viewp;
 i32 scale_down_pow2 = fval(0); // ;scale_down_slider
 macro_clamp_min(scale_down_pow2, 0);
 v1 render_scale = 1.f;
 for_i32 (it,0,scale_down_pow2) { render_scale *= 0.5f; }
 v1 default_meter_to_pixel = 4050.6329;
 v1 meter_to_pixel = default_meter_to_pixel * render_scale;
 {// NOTE: Setup sane math coordinate system. @Ugh
  Render_Config config =
  {
   .viewport_id = viewport_id,
   // NOTE: Don't set the clip box here since we don't change it.
   .y_is_up          = true,
   .background       = background_color,
   .view_transform   = view_transform,
   .camera_transform = camera->transform,
   .meter_to_pixel   = meter_to_pixel,
   .focal_length     = camera->focal_length,
   .near_clip        = camera->near_clip,
   .far_clip         = camera->far_clip,
   .scale_down_pow2  = scale_down_pow2,
  };
  draw_configure(app, &config);
  
  {// NOTE: @Ugh Compute the mouse position in view space
   rect2 clip_box = draw_get_clip(app);  // NOTE: This is the y-up clipbox
   v2 clip_dim = rect2_dim(clip_box);
   v2 view_offset = 0.5f * clip_dim;
   mouse_viewp   = vec2(mouse.p);
   mouse_viewp.y = (v1)render_target->height - mouse_viewp.y;
   mouse_viewp  -= clip_box.min + view_offset;
   //DEBUG_VALUE(mouse_viewp);//c 1920 / 0.474
   mouse_viewp  /= meter_to_pixel;
  }
 }
 
 //~ IMPORTANT: Please don't draw anything before this point! Because the color space are different.
 
 v1 head_scale = 9.2f * centimeter;  // @Tweak I have no idea where I pulled this number from
 v1 default_line_radius_unit = 7.f / default_meter_to_pixel;  //NOTE: Changes size when we resize the image
#define VERT_COUNT(structure)  cast(i32)(sizeof(structure) / sizeof(v3))
 i32 head_vert_count = VERT_COUNT(Head);
#undef VERT_COUNT
 {// NOTE: Drawing the character
  
  //~ IMPORTANT: Conventions
  //
  // 1: Things are on the left (side of the face) by default. Meaning x>0.
  // 2: Things like eyeY are just landmarks, nobody cares if there is actually no vertex lying on that y.
  // 3: Lines are drawn from inner to outer (NEW!)
  //~
  
  b32 lod2 = fbool(1);
  //-
#define TWENTIETH Fui_Options{0, 0.05f}
#define TENTH     Fui_Options{0, 0.1f}
#define TENS      Fui_Options{0, 10.f}
#define indicate_LEVEL(name, worldp, LEVEL) indicate_vertex(p, mouse_viewp, name, __LINE__, worldp, viz_level, LEVEL)
#define indicate(vertex,level)   indicate_LEVEL(#vertex, vertex, level)
#define indicate0(vertex)        indicate(vertex,0)
#define indicate1(vertex)        indicate(vertex,1)
#define indicate2(vertex)        indicate(vertex,2)
#define vv(NAME, V3)      v3 NAME = V3; indicate2(NAME);
#define va(NAME, V3)      NAME = V3; indicate2(NAME);
#define macro_import_vertex(vertex)  v3 vertex = ot.inv * vertex##_world;
#define macro_export_vertex(vertex)  vertex##_world = ot * vertex;
#define macro_world_declare(vertex)   v3 vertex##_world;
  //
  //-
  
  argb default_line_color = argb_gray(srgb_to_linear1(fval(0.3389f)));
  if (level1) {default_line_color = argb_silver;} // because otw blends in too much with the shading
  argb default_fill = background_color;
  
  v1 default_line_radius_min = fval(0.5089f);
  v1 default_line_end_radius = default_line_radius_min;
  if (fbool(1)) {
   default_line_end_radius = i2f6(fval(2));
  }
  // ;init_painter
  global_painter = Painter {
   .target            = render_target,
   .camera            = camera,
   .view_transform    = view_transform,
   .meter_to_pixel    = meter_to_pixel,
   .symx              = false,
   .fill_color        = default_fill,
   .fill_depth_offset = millimeter * 1.f,
   .line_radius_unit  = default_line_radius_unit,
   .line_params       = {
    .radii      = {
     default_line_radius_min, 
     1.f,
     i2f6(fval(5)),
     default_line_end_radius,
    },
    .color      = default_line_color,
    .visibility = 1.0f,
   },
   .poly_viz = level1,
  };
  Painter *p = &global_painter;
  Line_Params *default_line_params = &p->line_params;
  v4 &default_line_radii = p->line_params.radii;
  
  argb shade_color = compute_fill_color(p,0.094014f);
  if ( fbool(0) ) { shade_color = default_fill; }
  
  v1 animation_time = state->time;
  {// NOTE: animation_speed_multiplier
   animation_time *= fval(1.f);  // important
  }
  
  //-NOTE: Animation
  v1 thead_theta;
  v1 thead_phi;
  v1 thead_roll;
  v1 tblink;
  v1 teye_theta;
  v1 teye_phi;
  
  {
   Temp_Memory_Block temp(arena);
   Movie_Shot *shot = push_struct(arena, Movie_Shot);
   {//NOTE: ;set_movie_shot
    Shot_Function *played_animation = movie_shot_blinking;
    switch ( fvali(0) )
    {
     case 1: { played_animation = movie_shot_0; } break;
    }
    shot_go(shot, played_animation, animation_time);
   }
   
   thead_theta = i2f6(shot->head_theta);
   thead_phi   = i2f6(shot->head_phi);
   thead_roll  = i2f6(shot->head_roll);
   tblink      = i2f6(shot->tblink);
   teye_theta  = i2f6(shot->eye_theta);
   teye_phi    = i2f6(shot->eye_phi);
  }
  
  Head head_world;
  v1 head_unit_world = head_scale*(1.f+square_root(2));
  v1 head_top_static = head_scale;
  mat4i headT;
  v1 version2 = fval(1.f);
  v1 tmp_ratio = 19.f / 9.2f;
  {// NOTE: The ;head + face
   mat4i &ot = headT;
   symx_on;
   set_in_block(default_line_params->nslice_per_meter, fval(4.1128f) * 100.f);//NOTE: crank up the lod
   v1 head_unit = 1.f+square_root(2);
   v3 rotation_pivot;
   {
    v1 head_theta_max = 0.15f;   // @Tweak
    v1 head_phi_max   = 0.125f;  // @Tweak
    v1 head_roll_max  = 0.125f;  // @Tweak
    v1 head_theta = lerp(0.f, thead_theta, head_theta_max);
    v1 head_phi   = lerp(0.f, thead_phi, head_phi_max);
    v1 head_roll  = lerp(0.f, thead_roll, head_roll_max);
    
    rotation_pivot = fvert3(0.f, -1.1466f, 0.3598f, clampx);
    ot = (mat4i_scale(head_scale) * 
          mat4i_rotate_tpr(head_theta, head_phi, head_roll, rotation_pivot));
    
    push_object_transform(p, &ot);
   }
   indicate(rotation_pivot,1);
   
   v3 loomis_cross_center = vec3(0.f,0.f,1.f);
   v1 faceZ = 1.f;
   
   v1 root2 = square_root(2.f);
   v1 inv_root2 = 1.f / root2;
   v1 loomis_unit = inv_root2;// NOTE: From brow to nose tip and all that jazz
   
   //NOTE: My nose_tip is actually higher than the nose base (the loomis nose y is for the nose base)
   v1 noseY = -loomis_unit+fval(0.f);
   v3 nose_tip  = v3{0.f, noseY, faceZ} + fvert3(0.f, 0.0237f, 0.0916f,clampx);
   indicate(nose_tip,2);
   
   mat4 &to_local = ot.inverse;
   {// NOTE: profile score
    v3 camera_local = to_local * camera_world_position(camera);
    v3 view_vector = noz(camera_local - nose_tip);
    
    v1 x = view_vector.z;
    v1 y = view_vector.x;
    
    v1 theta = arctan2(y,x);
    
    p->profile_score = absolute(theta*4.f);
    
    if ( fbool(0) && is_main_viewport ) { DEBUG_VALUE(p->profile_score); }
   }
   
   v1 chinY = -2.f*loomis_unit;
   v3 chin_middle = vec3(0,chinY,faceZ) + vec3z(fval(-0.0856f));
   
   v1 face_sideX = inv_root2;
   v3 loomis_side_center = vec3x(face_sideX);
   v1 loomis_side_radius = inv_root2;
   v3 ear_center = vec3x(face_sideX) + fvert3(0.f, -0.3601f, -0.2332f, clampx);
   indicate(ear_center,2);
   
   v3 side_circle_center = vec3(face_sideX,0,0);
   v1 side_circle_radius = loomis_unit;
   
   v3 chinL = vec3(0,chinY,chin_middle.z) + fvert(vec3(0.1572f, 0.0504f, -0.077f));
   indicate(chinL,1);
   
   v3 jaw = vec3(face_sideX, chinY, 0) + fvert(vec3(-0.0844f, 0.4575f, -0.0247f));
   
   v1 mouthY = lerp(chinY, fval(0.5871f), noseY);
   v3 mouth_corner = vec3(0,mouthY,faceZ) +
    fvert(vec3(0.2499f, 0.f, -0.2801f), clampy);
   v3 &mouth_cornerL = mouth_corner;
   v3 mouth_cornerR = negateX(mouth_corner);
   
   v1 nose_baseY = noseY;
   indicate(mouth_corner,2);
   
   //-
   v1 nose_wingX = fval(0.1054f);
   
   v1 bridgeX = lerp(0, 0.284149, nose_wingX);
   
   v1 browY = 0.f;
   
   v1 nose_sideX = fval(0.0586f);
   v3 nose_rootL = vec3(nose_sideX, 
                        lerp(browY, fval(0.1534f), noseY), 
                        faceZ+fval(-0.104f));//NOTE: Just below the brow
   indicate(nose_rootL, 0);
   
   v3 nose_wing = { nose_wingX, nose_baseY, faceZ };
   indicate(nose_wing,2);
   
   // NOTE: Is this point real? Not really, but it's here to stay.
   v3 nose_wing_up = nose_wing+tmp_ratio*fvert3( -0.005304, 0.053581, 0.003336 );
   indicate(nose_wing_up,2);
   //-
   
   v3 lip_low_center;
   {
    Bezier lip_up;
    v3 philtrum_up ;
    v1 philtrumX = fval(0.0435f,TWENTIETH);
    b32 nerf_mouth = lod2 == false;
    Bezier philtrum_line;
    Bezier philtrum_line_mid;
    Bezier lip_low_line;
    v1 radius_unit_multiplier = fval(0.610978f);
    p->line_radius_unit *= radius_unit_multiplier;
    defer(p->line_radius_unit /= radius_unit_multiplier);
    
    philtrum_up  = vec3(0, nose_baseY, faceZ) + fvert3(0,0,0);
    
    v3 philtrum_low = vec3(0,mouthY,faceZ) + fvert(vec3(0.f, 0.078f, 0.035f));
    philtrum_line_mid = bez2(philtrum_up ,
                             tmp_ratio*fvert3(0.f, -0.0601f, 0.0037f),
                             tmp_ratio*fvert3(0.f, 0.0233f, -0.0167f),
                             philtrum_low);
    indicate(philtrum_low,2);
    
    philtrum_line = philtrum_line_mid;
    for_i32(index,0,4) { philtrum_line[index].x += philtrumX; }
    
    for_i32(index,0,4) { philtrum_line_mid[index].z -= tmp_ratio*fval(0.0031516f); }
    
    {// NOTE: philtrum
     Line_Params params = profile_visible(p, 0.68f);
     params.radii = fval4(0.f, 0.f, 0.6235f, 0.0781f);
     draw(p, philtrum_line, params);
    }
    
    v3 philtrum_lowL = philtrum_low+vec3x(philtrumX);
    lip_up  = bez(philtrum_lowL, 
                  fvert(vec3(0.088f, 0.1234f, 0.2037f)),
                  fvert(vec3(-0.0951f, 0.0104f, 0.3071f)),
                  mouth_corner);
    {// NOTE: lip_up 
     Line_Params params = profile_visible(p, fval(0.6014f));
     params.radii = fci(vec4i(1,1,3,0));
     if (level1) { params = *default_line_params;  }
     draw(p, lip_up, params);
    }
    {
     v3 lip_in = sety(philtrum_lowL+fvert(vec3(-0.f, 0.f, -0.0527f), clampy),
                      mouth_corner.y+fval(0.0133f));
     //NOTE: Main mouth line, looks wicked awesome!
     Bezier lip_up_bot = bez(lip_in, fval(vec4(-0.0828f, 0.5237f, 0.f, 0.f)), mouth_corner,
                             funit(vec3(0.f, 0.2748f, 0.9615f)));
     draw(p, lip_up_bot,
          fval4( 0.4876f, 0.3631f, 0.7975f, -0.2905f));
    }
    {
     //NOTE: Upper lip is smaller than lower lip
     lip_low_center = vec3(0,mouthY, faceZ) + fvert(vec3(0.f, -0.088f, -0.0133f));
     indicate(lip_low_center, 1);
     lip_low_line = bez(lip_low_center, fval(vec4(0.f, 1.0473f, 0.f, -0.7242f)), mouth_corner,
                        funit(vec3(0.f, -0.1848f, 0.9828f)));
     if (!nerf_mouth)
     {
      v4 radii = fval4(0.7f, -0.2807f, 0.f, 0.f);
      if (level1) {radii = default_line_radii;}
      draw(p,lip_low_line,radii);
     }
    }
    
    v3 philtrum_offset_point = nose_wing+fvert(vec3(-0.0499f, -0.0725f, -0.0489f));
    indicate(philtrum_offset_point,1);
    fill(p, philtrum_offset_point, lip_up);
    fill(p, philtrum_offset_point, philtrum_line_mid);
    fill3(p, philtrum_offset_point, nose_wing, philtrum_up+vec3x(philtrumX));
    fill3(p, philtrum_offset_point, mouth_corner, nose_wing);
    
    fill3(p,philtrum_lowL,philtrum_low,philtrum_offset_point);;
   }
   
   //-NOTE: Nose
   Bezier nose_line_side = bez(nose_rootL, fval(vec4()), addx(nose_tip, nose_sideX),
                               funit(vec3(0.f, 0.f, 1.f)));
   
   {
    auto params = profile_visible(p,0.3f);
    if (level1) { params = p->line_params; }
    // TODO: our nose needs some renovation
    //draw_bezier(p,nose_line_side, fval4( 0.f, 0.f, 0.f, 1.f), &params);
   }
   
   v3 nose_tipL = nose_tip+vec3x(nose_sideX);
   Bezier nose_line_under = bez2(
                                 nose_tipL,
                                 tmp_ratio*fvert3( 0.005863, -0.019578, -0.053636 ),
                                 vec3(),
                                 nose_wing);
   draw(p, nose_line_under, fval4( 0.151758, 0.699609, 1.149659, -0.148325 ));
   
   {
    symx_off;
    Bezier nose_line_sideR = negateX(nose_line_side);
    fill_dbez(p, nose_line_side, nose_line_sideR);
   }
   
   {//NOTE: Draw that tasty nose wing :>
    v3 d1089 = nose_wing+tmp_ratio*fvert3( 0.026092, 0.033882, -0.020212 );
    Bezier P = bez_raw( nose_wing, d1089, d1089, nose_wing_up);
    draw(p,P,profile_visible(p,0.70f));
   }
   {// NOTE: Some nose tip drawing (very fudgy)
    // TODO: We can merge this with the "nose under" lines, maybe? 
    // Problem is we kinda need to separate out what the nose side is, in order to do triangles fills
    symx_off;
    v3 control = fvert(vec3(-0.0533f, 0.0283f, 0.0438f));
    draw(p,
         bez2(nose_tipL,
              control,
              negateX(control),
              negateX(nose_tipL)),
         fval(0.7097f)*p->line_params.radii);
   }
   
   //~NOTE: Cheek situation pretty complex!
   vv(cheek_low,(vec3(0,noseY,faceZ) + 
                 fvert(vec3(0.6062f, 0.f, -0.4289f))));
   indicate(cheek_low,0);
   
   {
    fill4(p, 
          cheek_low, jaw, chinL, mouth_corner);
   }
   
   v3 nose_root_backL = fvert(vec3(0.f, -0.0651f, -0.1755f), clampx) + vec3x(nose_sideX);
   
   
   v3 brow_out = fvert(vec3(0.585f, -0.1342f, 0.5485f));
   indicate(brow_out,1);
   vv(cheek_up, brow_out+fvert(vec3(0.f, -0.2935f, 0.f)));
   if (level1)
   {
    draw_line(p, brow_out, cheek_up, viz_line_params(p));  //NOTE: Literally a straight line
   }
   
   //NOTE: brow_ridge is the neutral bone structure of the brow, as well as the eye socket
   //NOTE: I have few ideas what its shape should be, also very hard to draw right
   Bez brow_ridge = bez(nose_rootL, fval(vec4(0.f, 0.3208f, 0.f, 0.6236f)), brow_out,
                        funit(vec3(0.f, 0.6019f, 0.7986f)));
   if (level1) { viz_block; draw(p, brow_ridge); }
   
   v3 es_up_in;
   {//~NOTE: The Eyes
    set_in_block(global_debug_scope, "eye");
    // NOTE: We use this "eyeT" as a compromise: as a small eye sub-system is small.
    // TODO: The 5-eyes rule is still elusive to me, I only have like 4 eyes.
    v3 eyeO = fvert(vec3(0.1953f, -0.2348f, 0.8954f));
    v3 eye_scale = fval(vec3(0.8324f, 0.9882f, 0.92f));
    mat4i eyeT_ = mat4i_translate(eyeO)*mat4i_scales(eye_scale);;
    // TODO: Having a mini-coordinate system seems over-engineered to me... but idk man!
    // I think we've managed to dig ourselves out of that situation almost?
    // because we can use eye_in - eye_out to establish a coord frame
    mat4 eyeT     = eyeT_.forward;
    mat4 eyeT_inv = eyeT_.inverse;
    v3 eye_in = eyeT*fvert3(-0.0282f, 0.f, -0.1236f);
    v1 loomis_eye_inZ = faceZ - loomis_unit/3.f;
    WARN_DELTA(eye_in.z, loomis_eye_inZ, 0.05f);
    
    b32 show_eye_guideline = show_grid;
    if (level1 || show_eye_guideline)
    {
     viz_block;
     if (level1)
     {
      draw(p, bez(eye_in, nose_rootL));
     }
    }
    
    vv(eye_out, eye_in + vec3(2.f * eye_in.x + fval(-0.03f),
                              0,
                              fval(-0.154f)));
    
    
    if (level1)
    {
     viz_block;
     draw_line(p, eye_out, brow_out);
    }
    
    va(es_up_in, bezier_sample(brow_ridge, fval(0.3668f)));
    
    Bezier eye_up_line = bez_old(eye_in, 
                                 fvert3(0.0814f, 0.3741f, 0.2401f),
                                 fval2(0.0159f, 0.3341f),
                                 eye_out);
    
    fill(p, es_up_in, eye_up_line);
    
    argb eye_in_shade = shade_color;
    fill3(p,
          nose_rootL, eye_in, es_up_in, 
          repeat3(eye_in_shade));
    Bezier eye_low_line = bez_old(eye_in, 
                                  fvert3(0.1735f, -0.2094f, 0.1663f), 
                                  fval2(0.117f, 0.2053f), 
                                  eye_out);
    
    {
     v3 eye_low_patch = lerp(eye_in, fval(0.4768f), cheek_low);
     indicate1(eye_low_patch);
     fill(p, eye_low_patch, eye_low_line);
     fill3(p, eye_low_patch, eye_out, cheek_low);
     fill3(p, eye_out, brow_out, es_up_in);
    }
    
    argb eye_socket_shade = eye_in_shade;
    Bezier eye_up_line_now;
    {// NOTE: Blinking animation control
     eye_up_line_now = bez_raw(eye_in, 
                               lerp(eye_up_line[1], tblink, eye_low_line[1]),
                               lerp(eye_up_line[2], tblink, eye_low_line[2]),
                               eye_out);
     v1 eye_in_radius  = lerp(fval2(0.0137f, -0.0006f), tblink);
     v1 eye_out_radius = lerp(fval2(0.4791f, 0.17f), tblink);
     v1 eye0_radius = lerp(fval2(0.6426f, 0.499f), tblink);
     v1 eye1_radius = lerp(fval2(0.f, 0.6154f), tblink);
     v4 eye_up_line_radius  = vec4( eye_in_radius, eye0_radius, eye1_radius, eye_out_radius);
     v4 eye_low_line_radius = eye_up_line_radius;
     {
      //eye_low_line_radius[0] = lerp(fval2(0.0052f, 0.f), tblink);
      eye_low_line_radius[1] = fval(0.0555f);
     }
     if (lod2)
     {
      draw(p,eye_up_line_now,eye_up_line_radius);
     }
     if (lod2)
     {
      draw(p,eye_low_line,eye_low_line_radius);
     }
     // NOTE: The eyelid
     if (lod2){
      fill_dbez(p, eye_up_line, eye_up_line_now, eye_in_shade);
     }
    }
    
    {//~NOTE: Eyeball
     symx_off;
     v3 eyeball_center = eyeT*fvert3(0.1531f, 0.0101f, -0.3179f);
     v1 eyeball_radius = (eye_out.x - eye_in.x)*fval(0.4526f);
     mat4 eyeballL_T;
     mat4 eyeballR_T;
     {// NOTE: Eyeball
      mat4 to_eyeballL   = mat4_translate(eyeball_center);
      mat4 to_eyeballR   = mat4_translate(negateX(eyeball_center));
      mat4 eyeball_scale = mat4_scale(eyeball_radius);
      mat4 eyeball_rotate;
      {// NOTE: We don't have phi yet
       v1 eyeball_theta = 0.13f*teye_theta;
       eyeball_rotate = rotateY(eyeball_theta);
      }
      eyeballL_T = to_eyeballL*eyeball_scale*eyeball_rotate;
      eyeballR_T = to_eyeballR*eyeball_scale*eyeball_rotate;
      
      if (show_eyeball)
      {
       viz_block;
       v4 eyeball_radii;
       {
        v1 big   = 0.5f;
        v1 small = 0.25f;
        eyeball_radii = vec4(big,small,small,big);
       }
       draw_bezier_circle(p, eyeballL_T, eyeball_radii);  // NOTE: A prime meridian
       mat4 eyeball_equator = eyeballL_T*rotateX(0.25f);
       draw_bezier_circle(p, eyeball_equator, eyeball_radii);
      }
     }
     
     if (lod2)
     {// NOTE: Iris (featuring Mr. Depth Offset Hack)
      set_in_block(default_line_params->nslice_per_meter, 128.f*fval(7.6378f));
      v1 iris_depth_offset = p->fill_depth_offset + 1*centimeter;
      p->line_params.depth_offset += iris_depth_offset;
      p->fill_depth_offset        += iris_depth_offset;
      
      {
       mat4 irisRelT;
       {
        //NOTE: https://blenderartists.org/t/just-how-big-should-the-iris-be-relative-to-the-actual-eyeball/1214393/3
        // so iris radius is [0.5, 0.6] that of eyeball
        v1 iris_radius     = fval(0.4934f);
        v3 iris_rel_center = vec3z( square_root(1-squared(iris_radius)) );
        irisRelT = mat4_translate(iris_rel_center) * mat4i_scale(iris_radius);
       }
       v4 iris_radii;
       {
        v1 big   = 0.5f;
        v1 small = 0.25f;
        iris_radii = vec4(big,small,small,big);
       }
       {
        mat4 irisLT = eyeballL_T*irisRelT;
        draw_bezier_circle(p, irisLT, iris_radii);
        mat4 irisRT = eyeballR_T*irisRelT;
        draw_bezier_circle(p, irisRT, iris_radii);
       }
      }
      
      p->line_params.depth_offset -= iris_depth_offset;
      p->fill_depth_offset        -= iris_depth_offset;
     }
     
     {//-NOTE: Checking
      for_i32(index,0,2)
      {
       Bez line;
       if (index == 0) { line = eye_up_line; }
       else { line = eye_low_line; }
       auto result = get_eye_min_distance(eyeball_center, eyeball_radius, line);
       if (result.min_distance < 0)
       {
        DEBUG_VALUE(result.min_distance/millimeter);
        indicate(result.closest_point,0);
       }
      }
     }
    }
    
    
    
    // NOTE: These fills are most dubious...
    fill3(p, nose_wing, cheek_low, eye_in);
    fill3(p, nose_root_backL, eye_in, nose_rootL);
    fill3(p, nose_root_backL, nose_wing, eye_in);  // NOTE: smooth shading this thing is a disaster!
    fill3(p, eye_out, cheek_up, brow_out);
   }
   
   //-NOTE: Eyebrows
   {
    radius_scale_block(fval(0.8053f));
    v3 brow_in = (bezier_sample(brow_ridge, fval(0.1313f)) + 
                  fvert(vec3(0.0044f, -0.0132f, 0.0056f)));
    brow_in += fvert(vec3(0.f, -0.0434f, 0.f));  // NOTE: determined expression
    //NOTE: brow_mid is a really fuzzy point!
    v3 brow_mid = (bezier_sample(brow_ridge, fval(0.593f)) + 
                   fvert(vec3(-0.0006f, 0.0008f, 0.0086f)));
    
    indicate(brow_in,2);
    indicate(brow_mid,2);
    
    Bezier brow_in_line = bez( brow_in, brow_mid);
    v1 brow_joint_radius;
    if (lod2)
    {
     v4 radii = fval4(0.3344f, 0.6654f, 0.61f, 1.f);
     brow_joint_radius = radii.v[3];
     draw(p,brow_in_line, radii);
    }
    Bezier brow_out_line = bez(brow_mid, fval(vec4(0.f, 0.5549f, 0.f, 0.4409f)), brow_out,
                               funit(vec3(0.f, 1.f, 0.f)));
    if (lod2)
    {
     v4 radii = fval4(0.25f, 0.8f, 0.8f, 0.25f);
     radii.v[0] = brow_joint_radius;
     draw(p,brow_out_line, radii);
    }
   }
   
   Bezier jaw_line;
   {
    v3 cj = lerp(chinL, 0.5f, jaw);
    indicate(chin_middle,2);
    {
     // TODO: This line (and "cj") is here because it happens to look good
     jaw_line = bez_raw( ear_center, jaw, jaw, cj);
     v4 radii = p->line_params.radii;
     radii[0] = 0.f;
     radii[1] = 0.f;
     draw(p,jaw_line, radii);
    }
   }
   
   fill3(p, nose_wing, mouth_corner, cheek_low);
   
   // TODO(kv): Extreme hackiness, maybe we could replace bezier_poly
   {
    Bezier line = bez( nose_root_backL, nose_wing);
    fill_dbez(p, nose_line_side, line);
   }
   
   v3 foreheadL = brow_out + fvert(vec3(0.0477f, 0.7772f, -0.0232f));
   v1 foreheadY = foreheadL.y;
   indicate(foreheadL,2);
   
   {
    v3 verts[] = {jaw, cheek_up, brow_out, foreheadL};
    fill_fan(p, ear_center, verts, alen(verts));
   }
   
   fill3(p,cheek_up,jaw,cheek_low);
   
   if (level1)
   {
    viz_block;
    draw(p, bez(brow_out, cheek_up));
   }
   
   {//-Ear fetish
    radius_scale_block(0.5f);
    v3 ear_back = ear_center+fvert(vec3(0.1431f, 0.0034f, -0.3328f));
    v3 ear_low = (ear_center) + fvert(vec3(-0.0639f, -0.5264f, 0.1141f));
    v4 radii2 = vec4(0.2694f, 0.2694f, 1.0000f, 0.3598f);
    draw_bezier_6(p,
                  ear_center, 
                  fvert(vec3(0.0872f, 0.3887f, -0.1291f)),
                  fvert(vec3(0.0182f, 0.115f, 0.0165f)),
                  ear_back,
                  fval(vec3(0.0522f, 0.0132f, -0.246f)),
                  ear_low,
                  fval(vec4(0.25f, 0.6196f, 0.7259f, 1.3455f)), 
                  fval(vec2(0.25f, 0.1f)));
   }
   
   //-NOTE: The incredibly non-thrilling story of the head
   
   v3 head_back_out = fvert(tmp_ratio*vec3(0.3006f, 0.0688f, -0.222f));
   indicate(head_back_out,1);
   Bezier head_top_out_line = bez2(foreheadL,
                                   fvert(vec3(0.1239f, 0.1894f, -0.2995f)),
                                   fvert(vec3(-0.0634f, 0.3622f, 0.1935f)),
                                   head_back_out);
   if (level1)
   {
    draw(p,head_top_out_line);
   }
   v3 forehead_in = vec3y(foreheadY) + fvert3(0.1445f, 0.072f, 0.8969f);
   indicate(forehead_in,1);
   
   Bezier hair_hline = bez_raw(foreheadL,
                               forehead_in,
                               negateX(forehead_in),
                               negateX(foreheadL));
   
   {
    symx_off;
    if (level1) { draw(p,hair_hline); }
   }
   
   vv(head_back_in,fvert3(0.2242f, 0.6295f, -1.1258f));
   Bezier head_top_in_line = bez_old(forehead_in,
                                     fvert3(0.f, 0.4276f, -0.1837f),
                                     fval2(0.3589f, 0.1402f),
                                     head_back_in);
   if (level2)
   {
    draw(p,head_top_in_line);
   }
   fill_patch(p, patch_symx(head_top_out_line, head_top_in_line));
   
   v3 behind_ear = ear_center + fvert(tmp_ratio*vec3(-0.0886f, -0.1618f, -0.06f));
   Bezier head_back_out_line;
   {
    //TODO: omg why did you make it so confusing bro?
    head_back_out_line = bez2(head_back_out,
                              fvert(vec3(0.f, 0.001f, -0.0779f)),
                              fvert(vec3(-0.0535f, 0.197f, -0.6398f)),
                              behind_ear);
    if (level2)
    {
     draw(p,head_back_out_line);
    }
    v3 head_back_in2 = fvert(vec3(0.4676f, -0.7579f, -0.1264f));//note: Notice the loomis side circle
    indicate(head_back_in2,2);
    Bezier head_back_in_line = bez2(head_back_in,
                                    fvert(vec3(-0.f, -0.3664f, -0.3108f)),
                                    fvert(vec3(0.f, -0.4228f, -1.1179f)),
                                    head_back_in2);
    Patch head_back = patch_symx(head_back_out_line, head_back_in_line);
    if (level2) { draw(p,head_back_in_line); }
    fill_patch(p, head_back);
    
    //NOTE: draw vertical line
    draw(p, get_uline(head_back, 0.5f));
   }
   
   fill(p, ear_center, head_top_out_line);
   fill_dbez(p, ear_center, head_back_out, head_back_out_line);
   
   if (fbool(0))
   {//NOTE The forehead
    v3 hair_hline_center = bezier_sample(hair_hline, 0.5f);
    v3 hair_hline_half_controls[4];
    v3 esuo = brow_out;
    v3 control_point = vec3(fval(0.2448f), -esuo.y/3.f, (4.f-esuo.z)/3.f);
    // NOTE This line goes through the cross center!
    Bez brow_hline = bez_raw(esuo,
                             control_point,
                             negateX(control_point),
                             negateX(esuo));
    {
     symx_off;
     fill_patch(p, brow_hline,  brow_hline,  hair_hline, hair_hline);
    }
   }
   
   {
    //~NOTE: Chin
    symx_off;
    v3 chin_up_center = chinL + fvert(vec3(-0.1993f, 0.0993f, 0.0805f));
    v3 chin_upL = chin_up_center + fvertx(0.1274f);
    indicate(chin_upL,1);
    v3 chin_upR = negateX(chin_upL);
    v3 chinR = negateX(chinL);
    if (fbool(0))
    {
     Bezier chin_line = bez_raw( (chinL), chin_middle, chin_middle, negateX(chinL));
     //draw(p, chin_line, profile_visible(p, 0.57f));
     draw(p, chin_line);
    }
    
    {//NOTE: chin fixes
     line_radius_fine_tip;
     v3 mouth_low_valley = lip_low_center+fvert3(0.f, -0.0704f, -0.0417f, clampx);
     {
      Line_Params params = profile_visible(p, 0.73);
      draw(p, bez(lip_low_center, mouth_low_valley), params);
      draw(p, bez(chin_middle, setx(chin_upL, 0.f)), params);
     }
     if (level1)
     {
      draw(p,bez(mouth_low_valley,chin_middle));
     }
     // so we want to interpolate valley_under_lip
     v3 chin_point0 = mouth_corner;
     v3 control_point = fvertx(0.f) + vec3(0.f, 
                                           (-chin_point0.y + 4*mouth_low_valley.y)/3.f,
                                           (-chin_point0.z + 4*mouth_low_valley.z)/3.f);
     Bez mouth_low_valley_line = bez_raw(
                                         chin_point0,
                                         control_point,
                                         negateX(control_point),
                                         negateX(chin_point0));
     if (level1) {draw(p,mouth_low_valley_line);}
     {
      symx_on;
      fill3(p,lip_low_center,mouth_corner,mouth_low_valley);
      fill4(p, chin_middle, chinL, mouth_corner, mouth_low_valley);
     }
    }
   }
   
   vv(neck_junction, fvert(vec3(0.f, -1.1949f, 0.3195f), clampx));
   {
    symx_off;
    draw(p, bez(chin_middle, neck_junction));
    fill3(p, neck_junction, chinL, jaw);
    fill3(p, neck_junction, chinL, negateX(chinL));
   }
   
   {// NOTE: Some cheek line
    radius_scale_block(fval(0.3826f));
    line_color_lightness(fval(1.5096f));
    vv(a,fvert(vec3(0.2651f, -0.5154f, 0.8715f)));
    vv(b,fvert(vec3(0.5636f, -0.3601f, 0.5385f)));
    draw(p,bez(a, fval(vec4(0.f, 0.1801f, 0.0364f, 0.3141f)), b,
               funit(vec3(0.866f, 0.f, 0.5f))),
         small_to_big(p));
    vv(c,fvert(vec3(0.5266f, -0.6385f, 0.68f)));
    draw(p,bez(b, fval(vec4()), c,
                    funit(vec3())),
         big_to_small(p));
   }
   
   b32 hair_on = fbool(1);
   if (hair_on)
   {//~NOTE: Hair
    set_in_block(default_line_params->nslice_per_meter, fval(1.5162f)*128.f);
    symx_off;
    radius_scale_block(fval(0.5489f));
    //NOTE: fine tip hair -> thicker, more even radius
    v4 hair_radii = fval4(0.5f, 1.f, 1.f, 0.25f);
    set_in_block(default_line_radii, hair_radii);
    
    v1 hairY = loomis_unit;
    vv(hair_root, fvert3(0.f, 1.087f, 0.f, clampx|clampz)) ;  //NOTE: the point on top of the head, literally the highest point
    vv(bang_root, bezier_sample(hair_hline, 0.5f));
    
    v1 flutter_period = fval(2.75f);
    b32 should_flutter;
    v1 time;
    {
     v1 time0 = animation_time / flutter_period;
     should_flutter = ((i32)time0 % 2 == 0);
     time = cycle01_positive(time0);
    }
    Animation_Old ani_value; auto ani = &ani_value;
    {
     begin_animation(ani);
#define Group(...)  add_frame_group(ani, __VA_ARGS__)
     {
      Group(1.f, fval4(0.f, -0.304f, 0.f, 1.2f));
      Group(1.f, fval4(1.2f, 0.f, 0.f, 1.2f));
      Group(1.f, fval4(0.5752, -0.2f, 0.2f, 1.f));
      Group(0.5f, fval4(1.2f, 0.f, 0.f, 1.2f));
      Group(1.f, fval4(0.2176f, -0.4755f, 0.2169f, 0.f));
     }
#undef Group
     end_animation(ani);
    }
    v1 thairL = 0.f;
    v1 thairR = 0.f;
    if ( should_flutter == true )
    {
     thairL = get_animation_value(ani, time);
     thairR = get_animation_value(ani, time+0.03f);
    }
    if ( fbool(0) )
    {
     thairL = fval01(0.9950f);
     thairR = fval01(0.f);
    }
    
    vv(bang_tip, fvert3(0.7418f, -0.0689f, 0.6115f));
    vv(bang_midpoint, fvert3(0.f, -0.0929f, 1.042f, clampx));
    Bezier bang_vline = bez(bang_root, fval(vec4(0.f, 0.2629f, 0.1602f, 0.3068f)), bang_midpoint,
                            funit(vec3(0.f, 0.f, 1.f)));
    draw(p,bang_vline,profile_visible(p, fval(0.4036f)));
    
    Bez bang_vline_left = bez_old(bang_root, 
                                  fvert3(0.3419f, 0.0757f, 0.129f),
                                  fval2(0.1345f, 0.1639f),
                                  bang_tip);
    Bez bang_vline_right = negateX(bang_vline_left);
    
    Bez bang_hline_left = bez2(bang_midpoint,
                               fvert3(0.1302f, -0.0069f, -0.0297f),
                               fvert3(0.0001f, 0.2158f, 0.056f),
                               bang_tip);
    Bez bang_hline_right = negateX(bang_hline_left);
    
    // NOTE: Animate the hair
    {
     v3 a = fvert3(0.0172, -0.0113, -0.0825);
     v3 b = fvert3(-0.0664, 0.0812, 0.0059);
     v3 c = fvert3(0.0537, 0.1334, -0.0695);
     bang_vline_left[1]  += thairL*a;
     bang_vline_left[2]  += thairL*b;
     bang_vline_left[3]  += thairL*c;
     bang_vline_right[1] += thairR*negateX(a);
     bang_vline_right[2] += thairR*negateX(b);
     bang_vline_right[3] += thairR*negateX(c);
    }
    {
     v3 a = fvert3(0.0000, -0.1217, 0.0000);
     v3 b = fvert3(0.0467, 0.0385, 0.0000);
     bang_hline_left[1] += thairL*b;
     bang_hline_left[2] += thairL*a;
     bang_hline_left[3]  = bang_vline_left[3];
     bang_hline_right[1] += thairR*b;
     bang_hline_right[2] += thairR*negateX(a);
     bang_hline_right[3]  = bang_vline_right[3];
    }
    
    {
     draw(p,bang_vline_left);
     draw(p,bang_vline_right);
     v4 bang_hline_radii = fval(0.5176f) * p->line_params.radii;
     draw(p,bang_hline_left, bang_hline_radii);
     draw(p,bang_hline_right, bang_hline_radii);
    }
    
    //-NOTE: The bang saga is over, let's move onto something else
    v3 hair_main_tip = fvert3(0.7814f, -1.6599f, -0.6047f);
    Bez vline = bez_old(hair_root,
                        fvert3(0.1414f, 0.2226f, -0.3953f),
                        fval2(0.3956f, 0.3102f),
                        hair_main_tip);
    {
     symx_on;
     draw(p,vline);
    }
    v3 hcontrol = fvert3(0.1291f, -0.0058f, -0.2506f);
    Bez connecting = bez2( 
                          hair_main_tip,
                          hcontrol,
                          negateX(hcontrol),
                          negateX(hair_main_tip));
    draw(p, connecting, fval(0.5f)*p->line_params.radii);
    
    Bezier hairline_side = bez2(
                                bang_root,
                                fvert3(0.5271f, -0.0051f, 0.0478f),
                                fvert3(0.0671f, 0.1143f, 0.3924f),
                                ear_center);
    {
     symx_on;
     draw(p, hairline_side);
    }
    
    {
     symx_on;
     // NOTE: Some hair outline so it looks good at the front
     Bez over1 = bez_old(hair_root,
                         fvert3(0.5313f, 0.0177f, 0.1293f),
                         fval2(0.1839f, 0.232f),
                         bezier_sample(hairline_side, fval(0.3991f)));
     Bez over2 = bez_old(hair_root,
                         fvert3(0.3694f, 0.2549f, -0.0512f),
                         fval2(0.0175f, 0.2904f),
                         ear_center);
     draw(p,over1); 
     draw(p,over2); 
    }
    draw(p, bez(hair_root, fval(vec4(0.f, 0.2255f, 0.3279f, 0.2047f)), bang_root,
                funit(vec3(0.f, 1.f, 0.f))));
   }
   
   if (show_grid || show_loomis_ball)
   {//~NOTE: Proportions diagram
    viz_block_color(argb_dark_blue);
    symx_off;
    
    if (show_grid)
    {
     {//NOTE: Grid lines for the face
      viz_block_color(argb_blue);
      radius_scale_block(fval(0.33f));
      
      v1 face_minY = -2.f*loomis_unit;
      v1 unit = 0.25f * loomis_unit;
      i32 yi_min = fval(-7);
      i32 yi_max = fval(8);
      if (camera_frontal)
      {//NOTE: frontal view
       v1 z = faceZ;
       {
        for_i32(xi, 0, 5)
        {//note: Vertical
         v1 x = unit*v1(xi);
         draw_line(p, vec3(x, 1.f, z), vec3(x, face_minY, z));
        }
       }
       {
        v1 xmax = fval(0.9369f);
        for_i32(yi, yi_min, yi_max)
        {//note: Horizontal
         v1 y = unit*v1(yi);
         draw_line(p, vec3(0.f, y, z), vec3(xmax, y, z));
        }
       }
      }
      
      if (camera_profile)
      {// NOTE: Profile view
       v1 x = 0.f;
       for_i32(zi, fval(-11), (2))
       {//NOTE: Vertical
        v1 z = faceZ+unit*v1(zi);
        draw_line(p, vec3(x,1.f,z), vec3(x,face_minY,z));
       }
       for_i32(yi, yi_min, yi_max)
       {//NOTE: Horizontal
        v1 y = unit*v1(yi);
        draw_line(p, vec3(x,y,fval(-1.8208f)),
                  vec3(x,y,fval(1.7172f)));
       }
      }
     }
     
     {//NOTE: Grid lines for the body
      radius_scale_block(fval(0.5f));
      v1 body_minY = 1.f-6*head_unit;
      v1 unit = face_sideX;
      if (camera_frontal)
      {//NOTE: frontal view
       {
        for_i32(xi, 0, 5)
        {//note: Vertical
         v1 x = unit*v1(xi);
         draw_line(p, vec3(x, 1.f, 0.f), vec3(x, body_minY, 0.f));
        }
       }
       {
        v1 xmax = fval(2.8346f);
        for_i32(yi, (-20), (4))
        {//note: Horizontal
         v1 y = unit*v1(yi);
         draw_line(p, vec3(0.f, y, 0.f), vec3(xmax, y, 0.f));
        }
       }
       
       {//NOTE: Where the shoulder should be
        Line_Params params = *default_line_params;
        params.color = argb_blue;
        v1 y = fval(-2.2334f);
        draw_line(p, vec3(0.f, y, 0.f), vec3(0.7f*head_unit, y, 0.f), params);
       }
      }
      
      if (camera_profile)
      {// NOTE: Profile view
       v1 x = 0.f;
       for_i32(xi, (-4), (2))
       {//NOTE: Vertical
        v1 z = faceZ+unit*v1(xi);
        draw_line(p, vec3(x,1.f,z), vec3(x,body_minY,z));
       }
       for_i32(yi, -20, 4)
       {//NOTE: Horizontal
        v1 y = unit*v1(yi);
        draw_line(p, vec3(x,y,fval(-1.8208f)),
                  vec3(x,y,fval(1.7172f)));
       }
      }
      
     }
     
     
     {//NOTE: Head unit body proportions markers
      set_in_block(default_line_params->color, argb_red);
      if (camera_frontal)
      {//NOTE
       v1 r = 0.5f*head_unit;
       for_i32(yi,-6,1)
       {
        v1 y = 1.f + v1(yi)*head_unit;
        v3 left  = vec3(r,y,0);
        draw_line(p, vec3y(y), left);
       }
      }
      if (camera_profile)
      {//NOTE
       for_i32(yi,-6,1)
       {
        v1 y = 1.f + v1(yi)*head_unit;
        draw_line(p, vec3y(y), vec3(0,y,-1.f));
       }
      }
     }
     
    }
    
    
    if (show_loomis_ball)
    {//NOTE: The face
     argb line_color = default_line_params->color;
     if (show_grid) { line_color = argb_silver; }
     set_in_block(default_line_params->color, line_color);
     
     {// NOTE: Loomis The side circle
      mat4 tform = (mat4_translate(loomis_side_center) *
                    mat4_scale(loomis_side_radius) *
                    rotateY(0.25f));
      draw_bezier_circle(p,tform);
     }
     {
      // NOTE: Face z
      draw(p,bez(vec3z(faceZ), setz(chin_middle, faceZ)));
      draw_bezier_circle(p, mat4_identity);  // NOTE: The Loomis "unit circle" (i.e the first circle you draw)
      {
       mat4 equator = rotateX(0.25f);
       draw_bezier_circle(p, equator);
      }
      {
       mat4 longitude = rotateY(0.25f);
       draw_bezier_circle(p, longitude);
      }
     } 
    }
   }
   
   {//;csg_head
    {
     symx_off;
     csg_sphere(vec3(), fval(1.f));
     //csg_box(vec3(), vec3(1.f));
    }
    {
     set_in_block(p->csg_negated, true);
     csg_plane(vec3x(-1.f), face_sideX);
    }
   }
   
   {//~NOTE: Export head (todo just use macro, man!)
    //TODO: Maybe analytically define this point on the skull?
    vv(trapezius_head, (vec3(0, noseY, head_back_out.z) + 
                        fvert(vec3(0.2824f, 0.0086f, -0.323f))));
    
    Head h;
    h.neck_junction   = neck_junction;
    h.behind_ear      = behind_ear;
    h.behind_earR     = negateX(behind_ear);
    h.back_out        = head_back_out;
    h.jaw             = jaw;
    h.noseY           = vec3y(noseY);
    h.trapezius_head  = trapezius_head;
    h.trapezius_headR = negateX(h.trapezius_head);
    h.chin_middle     = chin_middle;
    h.top             = vec3y(1.f);
    
    for_i32(index,0,head_vert_count)
    {
     head_world.verts[index] = mat4vert(ot.m, h.verts[index]);
    }
   }
  }
  
#define macro_pelvis(x) \
x(navel) \
x(bikini_up_back) \
//
  macro_pelvis(macro_world_declare);
  {//~ bookmark: It's pelvis time!
   mat4i ot;
   {
    v3 translate = vec3(0.f, 
                        head_top_static - 3.2f * head_unit_world, 
                        fval(0.067f));
    ot = (mat4i_translate(translate) * mat4i_scale(head_scale));
    push_object_transform(p, &ot);
   }
   //~
   
   v1 navelY = (ot.inv * vec3y(head_top_static - fval(2.5f) * head_unit_world)).y;
   v3 navel = vec3y(navelY) + fvert(vec3(0.f, 0, 0.271f), clampy);
   indicate0(navel);
   
   symx_on;
   vv(crotch, vec3());  // NOTE: Yes, the crotch front is the origin, what about it?
   vv(crotchL, fvertx(0.1f));
   draw_line(p,crotch,crotchL);
   {//- lower
    vv(bikiniL, fvert(vec3(1.0685f, 0.8105f, -0.6117f)));
    v3 bikini_dir = funit(vec3(-0.1823f, -0.0005f, 0.9832f));
    Bez bikini_front = bez(crotchL, fval2(-0.2081f, 0.2062f), 
                           fval2(0.316f, 0.3309f), bikiniL,
                           bikini_dir);
    draw(p,bikini_front);
    Bez bikini_back = bez(crotchL, fval2(-0.2767f, 0.2024f),
                          fval2(0.213f, 0.2503f), bikiniL,
                          -bikini_dir);
    draw(p, bikini_back);
   }
   v3 bikini_up_back;
   {//-Upper
    //NOTE: this is from David Finch's "how to draw female torso", maybe it's an old technique?
    vv(bikini_front_mid, fvert(vec3(0.f, 0.951f, 0.1677f), clampx));
    vv(girdle_front, bikini_front_mid + fvert(vec3(0.8075f, 0.2603f, -0.0717f)));
    va(bikini_up_back, fvert(vec3(0.5102f, 1.3265f, -0.7766f)));
    Bez girdle_side_line = bez(girdle_front, 
                               fval4(-0.3554f, 0.3781f, 0.2608f, 0.2404f),
                               bikini_up_back,
                               funit(vec3(0.7543f, 0.6022f, 0.2614f)));
    draw(p, girdle_side_line, fci(vec4i(0,6,6,1)));
    draw(p, bez_old(bikini_front_mid, fvert(vec3(-0.0581f, -0.1036f, 0.f)),
                    fval(vec2(-0.0136f, 0.1226f)), girdle_front));
    {
     fill3(p, bikini_front_mid, crotch, girdle_front);
     fill3(p, girdle_front, crotch, bikini_up_back);
     fill_bez(p, girdle_side_line);
    }
   }
   
   {//-@Exports
    macro_pelvis(macro_export_vertex);
   }
  }
  
  v1 arm_up_ry = (fval(0.5302f)*(head_unit_world));
  mat4i arm_upT;
  
  b32 show_torso = fbool(1);
  mat4i torso_transform;
#define macro_torso(x) \
x(shoulder) \
x(delt_collar) \
x(scapula_delt) \
x(pectoral_torso) \
x(latis_arm) \
//
  macro_torso(macro_world_declare);
  {//~NOTE: torso
   set_in_block(p->painting_disabled, !show_torso);
   
   mat4i &ot = torso_transform;
   {// NOTE Transform
    ot = mat4i_scale(head_scale*fval(1.2484f));
    push_object_transform(p, &ot);
   }
   
   v3 view_center = fvert(vec3(0.f, -2.3176f, 0.3862f));
   update_view_vector(p, view_center);
   indicate2(view_center);
   if (fbool(0)) {DEBUG_VALUE(p->view_vector);}
   
   v1 head_unit = get_column(ot.inv,1).y * head_unit_world;
   
   //~NOTE: Head situation
   Head head;
   for_i32(index, 0, head_vert_count)
   {
    head.verts[index] = mat4vert(ot.inv, head_world.verts[index]);
   }
   //~
   macro_pelvis(macro_import_vertex);
   
   v1 shoulderY = head.chin_middle.y-0.2f*head_unit; // NOTE: from hpc
   v1 shoulderX = fval(0.53f, TWENTIETH)*head_unit;
   vv(shoulder, vec3(shoulderX, shoulderY, 0) + fvertz(0.0573f));
   if(0)
   {// NOTE: Check shoulder y
    v1 chin_to_shoulder = (head.chin_middle - shoulder).y;
    WARN_DELTA(chin_to_shoulder, 0.2f*head_unit, 2.f*centimeter);
   }
   vv(shoulder_in, fvert3(0.4819f, -1.3302f, -0.5612f));
   
   v3 sternum = fvert3(0.f, -1.5356f, 0.3862f, clampx);
   vv(delt_collar, shoulder + fvert3(-0.3387f, -0.02f, 0.1371f));
   vv(sternumL, sternum + vec3x(fval(0.0919f)));
   {// NOTE: Neck
    Bezier neck_front_vline = bez_old(head.neck_junction, fvert(vec3(-0.f, 0.f, -0.0968f)), 
                                      fval2(0.f, 0.163f), sternum);
    {
     draw(p, neck_front_vline, profile_visible(p, 0.5f));
    }
    
    Bezier neck_back_lineL;
    Bezier neck_back_lineR;
    {
     neck_back_lineL = bez_raw(head.trapezius_head,
                               fvert3(-0.1529f, -0.1932f, 0.2857f),
                               fvert3(-0.1937f, 0.0701f, 0.0969f),
                               shoulder_in);
     neck_back_lineR    = negateX(neck_back_lineL);
     neck_back_lineR[0] = head.trapezius_headR;
     bez_bake(neck_back_lineL);
     bez_bake(neck_back_lineR);
    }
    
    draw(p, neck_back_lineL);
    draw(p, neck_back_lineR);
    
    fill_dbez(p, neck_back_lineL, neck_back_lineR);
    
    Bezier collar_in = bez2(sternumL,
                            fvert3(0.2915f, -0.0448f, -0.0087f),
                            fvert3(-0.1047f, -0.0248f, 0.1171f),
                            delt_collar);
    Bezier collar_out = bez2(delt_collar+fvert(vec3(-0.0957f, 0.0021f, 0.016f)),
                             fvert3(0.f, 0.f, 0.f),
                             fvert3(-0.0607f, 0.0941f, 0.0168f),
                             shoulder);
    {
     symx_on;
     draw(p, bez2(shoulder_in,
                  fvert3(0.0993f, -0.0342f, -0.f),
                  fvert3(-0.0356f, 0.0675f, -0.0634f),
                  shoulder),
          fval(vec4i(5,3,3,1)));
     draw(p,collar_out);
     draw(p,collar_in);
    }
    
    Bezier sternoL;
    Bezier sternoR;
    {
     sternoL = bez_raw(head.behind_ear,
                       fvert3(-0.107f, -0.2444f, 0.2159f),
                       fvert3(0.0786f, 0.1842f, -0.1674f),
                       sternumL);
     sternoR = negateX(sternoL);
     sternoR[0] = head.behind_earR;
     bez_bake(sternoL);
     bez_bake(sternoR);
     
     {// NOTE: sterno contracting animation
      v3 patch = fvec(vec3(-0.1658f, 0.f, -0.0888f));
      v1 tcontractL = clamp01(-thead_theta);
      v1 tcontractR = clamp01(thead_theta);
      sternoL[1] += tcontractL * patch;
      sternoR[1] += tcontractR * negateX(patch);
     }
    }
    {
#if 0  //TODO: the sterno is bugged when looked from behind
     Line_Params prev_line_params = p->line_params;
     if (fbool(1)) { viz_line_params(&p->line_params); }
     defer(p->line_params = prev_line_params;);
#endif
     
     draw(p, sternoL);
     draw(p, sternoR);
    }
    
    fill_dbez(p, neck_back_lineL, sternoL);
    fill_dbez(p, neck_back_lineR, sternoR);
    
    fill_dbez(p, neck_front_vline, sternoL);
    fill_dbez(p, neck_front_vline, sternoR);
   }
   
   vv(scapula_delt,
      shoulder_in + fvert(vec3(-0.0079f, -0.4053f, -0.0611f)));
   vv(pectoral_torso, fvert(vec3(0.7703f, -2.3319f, 0.3404f)));
   
   v3 rib_mid;
   symx_on;
   v3 rib_back;
   {// NOTE: Rib cage
    // NOTE rib_midY is lower than half shoulder to navel
    v1 rib_midY = (shoulder.y - 0.65f*head_unit +
                   fval(-0.0775f));
    va(rib_mid, vec3(0.f, rib_midY, fval(0.9326f)));
    {
     symx_off;
     draw(p, bez_old(sternum, 
                     fvert(vec3(0.0089f, 0.1849f, 0.0188f), clampx),
                     fval(vec2(0.f, 0.0935f)),
                     rib_mid));
    }
    vv(ribL, rib_mid+fvert(vec3(0.559f, -0.7158f, -0.2772f)));
    // NOTE: I guess the rib cage ends where the elbow is
    v3 rib_in = rib_mid+vec3x(fval(0.0905f));
    draw(p, bez(rib_in, fval4(0.f, 0.0689f, 0.0558f, 0.2431f), ribL,
                funit(vec3(0.629f, -0.2482f, 0.7367f))),
         fval4i(2,6,3,1));
    
    fill4(p, rib_mid, ribL, delt_collar, sternum);
    va(rib_back, ribL+fvert(vec3(0.0855f, 0.171f, -0.5742f)));
    draw(p,bez(ribL, fval2(-0.0705f, 0.2667f),
               fval2(0.f, 0.384f), rib_back,
               funit(vec3(0.5421f, -0.8353f, 0.0916f))));
    {//-NOTE: David Finch's vest shape
     vv(vestL,ribL+fvert(vec3(0.0371f, 0.3022f, 0.0011f)));
     draw(p, bez(rib_in, fval(vec4(0.f, 0.4043f, 0.f, 0.2469f)), vestL,
                 funit(vec3(0.f, 0.852f, 0.5235f))));
     vv(vest_back, rib_back+fvert(vec3(-0.f, 0.5114f, -0.0067f)));//todo: where?
     draw(p, bez(vestL, fval(vec4(-0.0067f, 0.1427f, 0.f, 0.3102f)), vest_back,
                 funit(vec3(0.9212f, -0.3891f, -0.f))));
    }
   }
   
   {//NOTE: ;arm_upT Calculate upper arm transform, so we can draw attachments to it
    //NOTE: Surprisingly, length of arm = head unit
    shoulder_world = ot*shoulder;
    v3 translate = addy(shoulder_world, -arm_up_ry);
    translate += head_scale*fvert3(0.1018f, 0.f, 0.1425f);
    mat4i rotateT = mat4i_rotate_tpr(0, 0, fval(-0.0236f), shoulder_world);
    arm_upT = rotateT * mat4i_translate(translate) * mat4i_scale(arm_up_ry);
   }
   mat4 arm_up_local = ot.inv * arm_upT;
   
   vv(latis_arm, arm_up_local*fvert3(-0.2595f, 0.263f, -0.0002f));
   {// NOTE: connection to the arm
    Bez latis_arm_line = bez(rib_back, fval2(0.1935f, 0.0429f), 
                             fval2(0.151f, 0.0235f), latis_arm,
                             funit(vec3(1.f, 0.f, -0.f)));
    draw(p, latis_arm_line, lp_alignment_threshold(0.7f));
   }
   
   {//NOTE: The back
    vv(trap_bot, fvert(vec3(0.f, -3.1104f, -0.1081f), clampx));
    if (fbool(0))
    {
     draw(p, bez_old(scapula_delt, fvert(vec3(0.0015f, 0.0505f, -0.0748f)),
                     fval2(-0.4018f, 0.0773f), trap_bot));
    }
    if (level1)
    {
     viz_block;
     draw(p, bez(scapula_delt, fval(vec4(0.f, 0.1307f, 0.f, 0.0566f)), shoulder,
                 funit(vec3(0.5328f, 0.0057f, -0.8462f))));
    }
    fill3_symx(p, scapula_delt, shoulder_in);
    fill3(p, shoulder_in, shoulder, scapula_delt);
    
    fill3_symx(p,trap_bot,scapula_delt);
    //draw_line(p,scapula_delt,shoulder_in);
    Bez hip_back_line = bez(rib_back, fval2(0.f, 0.1135f),
                            fval2(0.01f, 0.1345f), bikini_up_back,
                            funit(vec3(-0.5382f, 0.f, 0.8428f)));
    draw(p,hip_back_line);
    // NOTE: Back arch
    vv(back_archL, bezier_sample(hip_back_line, fval(0.5606f)));
    draw(p, bez(trap_bot, fvert(vec3(0.235f, 0.3261f, -0.0079f)),
                fvert(vec3(0.0805f, 0.f, 0.0552f)), back_archL));
    
    vv(scapula_bot, fvert(vec3(0.474f, -2.3384f, -0.4857f)));
    draw(p,bez(scapula_delt,fval(vec4(-0.14f, 0.f, 0.1746f, 0.4409f)),scapula_bot,
               funit(vec3(0.9769f, 0.f, -0.2137f))));
    vv(scapula_0, fvert(vec3(0.7392f, -2.0326f, -0.498f)));
    draw(p,bez(scapula_bot + fvert(vec3(0.0426f, 0.1145f, -0.0307f)),
               fval(vec4(0.f, 0.5592f, 0.f, 0.f)), scapula_0,
               funit(vec3(-0.9994f, 0.0335f, -0.009f))));
    
    vv(lower_back, fvert(vec3(0.f, -4.2845f, -0.0495f), clampx));
    fill3(p, lower_back, trap_bot, back_archL);
    
    vv(diamond_up,  vec3z(shoulder_in.z)+fvert(vec3(0.f, -1.2636f, 0.0193f), clampx));
    vv(diamond_low, diamond_up+fvert(vec3(0.f, -0.4125f, -0.1454f), clampx));
    vv(diamondL,    diamond_up+fvert(vec3(0.0856f, -0.2026f, -0.0719f)));
    draw(p, bez(diamond_up, diamondL));
    draw(p, bez(diamond_low, diamondL));
    
    Bez back_midline =  bez(diamond_low, fval(vec4(-0.0746f, 0.0847f, 0.f, 0.f)), trap_bot,
                            vec3z(-1));
    draw(p, back_midline, fval(vec4(0.3513f, 0.9763f, 0.6129f, 1.1402f)));
    
    Bez trap_vline = bez(scapula_delt, 
                         fval(vec4(0.f, 0.0667f, 0.f, 0.f)), 
                         bezier_sample(back_midline, fval(0.9154f)),
                         funit(vec3(-0.379f, 0.f, -0.9254f)));
    draw(p, trap_vline, fval(vec4(0.7542f, 1.f, 1.f, 0.2694f)));
    
    vv(trap_weird_point, bezier_sample(trap_vline, fval(0.5728f)));
    Bez trap_weird_line = bez(trap_weird_point, fval(vec4(0.0369f, 0.1268f, 0.f, 0.f)), rib_back,
                              funit(vec3(-0.8654f, 0.f, -0.5011f)));
    draw(p, trap_weird_line, fval(vec4(0.75f, 1.f, 1.0735f, 0.5f)));
   }
   
   v3 chest_back;//c
   {//NOTE: Nipple and chest!
    v1 nippleY = shoulder.y-0.4f*head_unit;  //NOTE: from hpc
    v3 nipple = (vec3y(nippleY) + fvert(vec3(0.5799f, 0.f, 0.6752f), clampy));
    indicate0(nipple);
    
    vv(chest_in, fvert(vec3(0.1045f, -2.5438f, 0.8167f)));
    vv(chest_out, fvert(vec3(0.6599f, -2.4822f, 0.6002f)));
    fill3(p,delt_collar,pectoral_torso,chest_out);
    draw(p,bez(chest_in, fval4(-0.0299f, 0.2108f, 0.1371f, 0.253f), chest_out,
               funit(vec3(0.0915f, -0.523f, 0.8474f))),
         fval(vec4i(6,4,3,1)));
    va(chest_back, fvert(vec3(0.7613f, -2.0817f, 0.039f)));
   }
   
   {//NOTE: Some latissimus lines that are made-up and probably will have to be redone
    Bez latis_side = bez(chest_back, fval2(-0.1341f, 0.0768f),
                         fval2(0,0), rib_back,
                         funit(vec3(0,0,-1)));
    draw(p,latis_side,lp_alignment_threshold(0.7f));
   }
   
   {//NOTE: Abdoment
    symx_off;
    draw(p,bez(rib_mid, fvert(vec3(-0.f, 0.f, -0.0952f)),
               fval2(0,0), navel));
   }
   
   {//~NOTE: Exports
    macro_torso(macro_export_vertex);
   }
  }
  
  b32 arm_on = fbool(1) || level2;
  if (arm_on)
  {
   mat4 mat4_bilateral = mat4_translate(vec3(-1.f)) * mat4_scale(2.f);
   //
#define macro_arm_up(x) \
x(elbow_center) \
x(bicep_out_bot)\
x(hinge_out)\
//
   macro_arm_up(macro_world_declare);
   
   argb side_plane = shade_color;
   {//;upper_arm @arm_upT
    scale_in_block(default_fvert_delta_scale, 2.f);
    mat4i &ot = arm_upT;
    push_object_transform(p, &ot);
    //
    macro_torso(macro_import_vertex);
    
    if (level1)
    {//NOTE: The skeleton (humerus and friends)
     viz_block;
     v3 o = fvert(vec3(0.0501f, 0.f, -0.1001f));
     {//NOTE: internal
      vv(a, o+fvert(vec3(-0.1693f, -0.7344f, -0.0402f)));
      draw(p, bez(o+fvert(vec3(-0.1711f, 0.8464f, -0.052f)), a),
           vec4(0.5f));
      vv(internal_condyle, o+fvert(vec3(-0.2527f, -0.9575f, -0.0402f)));
      draw(p, bez(a, fval(vec4(0.f, 0.4207f, 0.f, 0.f)), internal_condyle,
                  funit(vec3(0.5647f, -0.8253f, 0.f))),
           0.5f*big_to_small(p));
     }
     {//NOTE: external
      vv(a, o+fvert(vec3(-0.032f, -0.7874f, -0.0402f)));
      draw(p, bez(o+fvert(vec3(-0.0435f, 0.8466f, -0.0052f)),
                  a),
           vec4(0.5f));
      vv(external_condyle, o+fvert(vec3(0.0625f, -0.9168f, 0.f)));
      draw(p, bez(a,fval(vec4(0.f, 1.0357f, 0.f, 0.f)),external_condyle,
                       funit(vec3(-0.1623f, -0.9867f, 0.f))),
           0.5f*big_to_small(p));
     }
    }
    
    v3 elbow_center = vec3y(-1.f);
    indicate2(elbow_center);
    
    vv(delt_head, fvert(vec3(0.0527f, -0.0075f, -0.2028f)));
    vv(delt_bot, fvert(vec3(0.1197f, 0.2332f, -0.019f)));
    v3 delt_bot_back;
    Bez delt_cross;
    Bez delt_back_line;
    vv(pectoral_arm, fvert(vec3(-0.1244f, 0.3872f, 0.1282f)));
    //vv(delt_back_point, fvert(vec3(-0.4738f, 0.6024f, -0.5882f)));
    v3 delt_back_point = scapula_delt;
    {//-Deltoid
     vv(delt_top, shoulder+fvert(vec3(0.0328f, 0.f, 0.f)));
     vv(delt_top_back, delt_top+fvert(vec3(-0.0109f, -0.0068f, -0.2147f)));
     Bez delt_hline = bez(delt_top,
                          fval(vec4(-0.059f, 0.584f, 0.f, 0.f)),
                          delt_top_back +fvert(vec3()),
                          funit(vec3(0.f, 1.f, 0.f)));
     draw(p, delt_hline);
     fill_bez(p, delt_hline);
     
     Bez delt_vfront = bez(delt_top, fval(vec4(-0.3452f, 0.3717f, 0.0022f, 0.1226f)), delt_head,
                           funit(vec3(0.0087f, 0.f, 1.f)));
     Bez delt_vmid = bez(delt_top, fval(vec4(-0.2234f, 0.2079f, 0.015f, 0.1626f)), delt_head,
                         funit(vec3(0.9999f, 0.f, 0.0112f)));
     Bez delt_vback = bez(delt_top_back, fval(vec4(-0.2648f, 0.1421f, -0.1368f, 0.2863f)), delt_head,
                          funit(vec3(0.6431f, 0.f, -0.7657f)));
     // TODO: Please document what this point is?
     va(delt_bot_back, bezier_sample(delt_vback, fval(0.8534f)));
     
     draw(p, delt_vmid, lp_alignment_threshold(fval(0.8202f)));
     draw(p, delt_vfront);
     draw(p, delt_vback);
     //NOTE: fill the side
     fill_dbez(p, delt_vfront, delt_vmid);
     fill_dbez(p, delt_vmid, delt_vback);
     draw(p,bez(delt_top_back, fval(vec4()), scapula_delt,
                funit(vec3())));
     
     delt_back_line = bez(delt_bot_back,
                          fval(vec4(-0.1729f, -0.1804f, 0.1064f, 0.2869f)), 
                          delt_back_point,
                          funit(vec3(0.f, -1.f, 0.004f)));
     draw(p, delt_back_line, fval4i(1,4,4,1));//TODO: Wow this line is terrible
     //fvert(vec3(0.0834f, 0.2737f, -0.3366f))
     //note: fill the back side of the deltoid
     fill4(p, delt_top_back, delt_bot_back, delt_back_point, scapula_delt);
     
     delt_cross = bez_old(delt_collar, 
                          fvert3(0.0677f, 0.1554f, 0.2652f),
                          fval2(0.f, 0.1569f),
                          delt_head);
     //if (level1)
     {
      //viz_block;
      draw(p, delt_cross, fval4i(1,2,5,4));
     }
     
     // NOTE: @Poly Fill in the effing arm-torso connections!!!
     fill(p,delt_top,delt_cross);
     fill_bez(p, delt_cross);
     fill3(p,pectoral_arm,delt_collar,delt_bot);
     fill3(p,pectoral_arm,delt_bot,latis_arm);
     
     {//-Fill in the side 
      //fill4(p, delt_top, delt_top_back, delt_bot_back, delt_bot, 
      //      repeat4(side_plane));
     }
     {
      // Fill in the front
      fill(p, bezier_sample(delt_cross, fval(0.5f)), delt_vfront);
     }
    }
    {
     Bez line = bez(pectoral_arm, fval(vec4(0.f, 0.3866f, 0.f, 0.f)), pectoral_torso,
                    funit(vec3(-0.997f, 0.0775f, 0.f)));
     draw(p, line);
     fill(p, delt_collar, line);
    }
    
    v3 bicep_out_bot, hinge_out;
    {//NOTE: The 'ceps
     {//note: tricep
     }
     
     {//NOTE: bicep
      vv(bicep_out_top, fvert(vec3(-0.0116f, 0.2332f, 0.0614f)));
      va(bicep_out_bot, fvert(vec3(-0.067f, -0.9224f, 0.0133f)));
      //NOTE: Outer line
      Bez line_out = bez(bicep_out_top,fval(vec4(0.f, 0.08f, 0.f, 0.f)),bicep_out_bot,
                         funit(vec3(0.7886f, 0.f, 0.6149f)));
      draw(p, line_out, lp_alignment_threshold(fval(0.4688f)));
      va(hinge_out, bezier_sample(line_out, fval(0.6758f)));
      
      vv(bicep_in_top, bicep_out_top+fvert(vec3(-0.1993f, 0.f, 0.f)));
      vv(bicep_in_bot, bicep_out_bot+fvert(vec3(-0.1107f, 0.f, 0.f)));
      //NOTE: Inner line
      Bez line_in = bez(bicep_in_top,fval(vec4(0.f, 0.08f, 0.0617f, 0.0185f)),bicep_in_bot,
                        funit(vec3(-0.8434f, 0.f, 0.5373f)));
      draw(p, line_in, lp_alignment_threshold(fval(0.6693f)));
      fill_dbez(p, line_in, line_out);
     }
    }
    
    if (fbool(0))
    {// ;upper_arm_csg
     Temp_Memory_Block temp(arena);
     v3 radii = fval3(0.146f, 1.f, 0.1544f);
     
     CSG_Tree *tree = csg_box(vec3(), radii);
     
     Mesh mesh = transformed_mesh(arena, mat4_scales(radii), the_box_mesh);
     {
      v3 n = funit(vec3(-0.9647f, 0.f, 0.2632f));
      v1 d = fval(0.142f);
      
      for_i32(triangle_index,0,mesh.triangle_count)
      {
       v3i tri = mesh.triangles[triangle_index];
       v3 triangle[3] = {
        mesh.vertices[tri[0]],
        mesh.vertices[tri[1]],
        mesh.vertices[tri[2]] 
       };
       {// NOTE: clipping triangle
        Clip_Result clipped = clip_triangle(triangle,n,d);
        if ( triangle_valid(clipped.a) )
        {
         fill3(&global_painter, clipped.a);
         if ( triangle_valid(clipped.b) )
         {
          fill3(&global_painter, clipped.b);
         }
        }
       }
       {// NOTE: Draw out intersections
        std_array<v3,2> intersections = intersect_triangle(n,d,triangle);
        for_i32(index, 0, 2)
        {
         if (intersections[index] != VEC3_INVALID)
         {
          indicate0(intersections[index]);
         }
        }
       }
      }
     }
    }
    
    {//~NOTE: Exports
     macro_arm_up(macro_export_vertex);
    }
   }
   
   {//~NOTE: The lower arm / forearm
    mat4i ot;
    v1 ry_ratio = fval(.8f); //NOTE: 0.8 is measured on the iPad, also the Loomis female proportions chart
    {// NOTE Transform
     mat4i scaleT = mat4i_scale(ry_ratio);
     mat4i to_elbow_center = mat4i_translate(vec3y(-1.f));
     ot = (arm_upT * to_elbow_center *
           scaleT * mat4i_translate(vec3y(-1.f)));
     push_object_transform(p, &ot);
    }
    macro_arm_up(macro_import_vertex);
    
    if (level1)
    {//NOTE: The forearm bones from the front view (as rough guide)
     viz_block;
     {//NOTE: ulna (inner)
      v3 up = fvert(vec3(-0.1422f, 0.9332f, -0.1785f));
      v3 down = fvert(vec3(0.0618f, -1.0018f, 0.f));
      draw(p, bez(up,
                  fvert(vec3(0.0351f, 0.f, -0.0416f)), 
                  fvert(vec3(-0.0342f, -0.0572f, 0.f)), 
                  down));
     }
     {//NOTE: radius (outer)
      {//NOTE: lower part
       {// NOTE: Outer
        v3 up = fvert(vec3(0.0181f, 0.8124f, 0.f));
        v3 down = fvert(vec3(0.2771f, -1.0151f, 0.f));
        draw(p, bez(up, 
                    fvert(vec3()),
                         fvert(vec3(-0.f, 0.f, -0.055f)),
                         down));
       }
      }
     }
     {//NOTE: The hinge
      v3 in  = fvert(vec3(-0.1827f, 0.9833f, 0.f));
      v3 out = fvert(vec3(0.0825f, 1.0294f, 0.f));
      draw(p, bez(in, out));
     }
    }
    
    {// NOTE: Inner side
     vv(a, fvert(vec3(-0.2579f, 1.192f, -0.0282f)));
     vv(b, fvert(vec3(-0.2528f, 0.9637f, -0.0547f)));
     vv(c, fvert(vec3(-0.1288f, 0.4571f, -0.0369f)));
     duo_line(p,a,b,c);
    }
    
    {//NOTE: Outer side (the thumb side): Brachio-radialis
     vv(a, fvert(vec3(0.0723f, 1.2075f, -0.0405f)));
     vv(b, fvert(vec3(0.1744f, 0.997f, -0.0547f)));
     vv(c, fvert(vec3(0.158f, 0.2747f, -0.0369f)));
     if (fbool(1)) {duo_line(p,a,b,c);}
     vv(d, fvert(vec3(0.0998f, 1.1924f, -0.2106f)));
     vv(e, fvert(vec3(0.1851f, 0.9887f, -0.2406f)));
     vv(f, fvert(vec3(0.158f, 0.5238f, -0.1954f)));
     duo_line(p, d,e,f);
     fill4(p,a,b,e,d, side_plane);
     fill4(p,b,c,f,e, side_plane);
     fill3(p, a, hinge_out, bicep_out_bot, side_plane);
    }
    
    {//NOTE: The elbow
     Line_Params prev_line_params = p->line_params;
     if (level1) { p->line_params = viz_line_params(p); }
     defer(p->line_params = prev_line_params;);
     
     v3 offset = fvert(vec3(0.0134f, -0.08f, -0.0281f));
     vv(in0, offset+fvert(vec3(-0.189f, 1.1745f, -0.2553f)));
     vv(in1, offset+fvert(vec3(-0.1289f, 0.9523f, -0.2507f)));
     v4i elbow_radii = fval(vec4i(4,3,3,2));
     draw(p,bez(in0,in1), elbow_radii);
     vv(out0, offset+fvert(vec3(-0.0641f, 1.1807f, -0.2873f)));
     vv(out1, offset+fvert(vec3(-0.0788f, 0.9489f, -0.2507f)));
     draw(p,bez(out0,out1), elbow_radii);
    }
   }
  }
  
  //~
#undef indicate
#undef indicate0
#undef indicate1
#undef indicate2
#undef TENTH
#undef TWENTIETH
#undef TENS
#undef vv
#undef va
 }
 
#if 0
 if ( fbool(1) )
 {//;call_csg_render
  v2 dst_dim = render_scale*rect2_dim(draw_get_clip(app));
  csg_render(render_target->group_last/*this group*/, dst_dim, csg_buffer);
 }
#endif
 
 //~ IMPORTANT: Drawing end: no trespass
 draw_configure(app, &old_render_config);
}

//~EOF