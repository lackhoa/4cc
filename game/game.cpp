#define AD_COMPILING_DRIVER 1
#include "kv_core.h"
#define AD_IS_COMPILING_GAME 1
#define ED_API_USER 1
#include "4coder_game_shared.h"
#include "game.h"
#include "game_fui.h"
#include "game_config.cpp"
#include "game_colors.cpp"
#define AD_COMPILING_DRIVER 1
#include "framework_driver_shared.h"
#include "ad_debug.h"
#include "game_draw.cpp"
#include "game_anime.cpp"
#include "game_utils.cpp"

// Select animation: @set_movie_shot

void
movie_update(Game_State *state)
{
 debug_frame_time_on      = fbool(0);
 BEZIER_POLY_NSLICE       = fval(16);
 DEFAULT_NSLICE_PER_METER = fval(2.2988f) * 100.f;
 
 v3 hand_pivot = fvert(V3(0.2299f, -0.604f, -0.0145f));
 for_i32(viewport_index,0,GAME_VIEWPORT_COUNT)
 {// TODO: Piggy code to set the camera pivot, but what isn't piggy?
  Viewport *viewport = &state->viewports[viewport_index];
  viewport->camera.pivot = hand_pivot;
 }
}

inline Line_Params
viz_line_params(argb color=0)
{
 if (color == 0) { color = argb_red; }
 Line_Params params = painter.line_params;
 params.flags |= Line_Overlay|Line_No_SymX;
 params.color = color;
 params.radii = fci(I4(3,3,3,3));
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

//~
#define vv(NAME, VAL)      v3 NAME = VAL; indicate(NAME);
#define va(NAME, VAL)      NAME    = VAL; indicate(NAME);
#define vv0(NAME, VAL)     v3 NAME = VAL; indicate0(NAME);
#define vv1(NAME, VAL)     v3 NAME = VAL; indicate_level(NAME, 1);
#define macro_import_vertex(vertex)  v3 vertex = ot.inv * vertex##_world;
#define macro_export_vertex(vertex)  vertex##_world = ot * vertex;
#define macro_world_declare(vertex)   v3 vertex##_world;
#define level1 (painter.viz_level >= 1)
#define level2 (painter.viz_level >= 2)

union Head
{
 struct {
#define macro_head(x) \
x(jaw) \
x(behind_ear) \
x(behind_earR) \
x(head_neck_junction) \
x(trapezius_head) \
x(trapezius_headR) \
x(chin_middle) \
  
#define x(name) v3 name; 
  macro_head(x)
#undef x
 };
 v3 verts[];
};
//
global_const i32 head_vert_count = (sizeof(Head) / sizeof(v3));

union Pelvis
{
#define macro_pelvis(x) \
x(navel) \
x(bikini_up_back) \
//
 struct {
#define x(name) v3 name;
  macro_pelvis(x)
#undef x
 };
 v3 verts[];
};
//
global_const i32 pelvis_vert_count = sizeof(Pelvis) / sizeof(v3);

union Arm
{
#define macro_arm(x) \
x(bicep_in_bot) \
x(bicep_out_bot) \
x(white_in) \
x(white_out) \
x(delt_bot) \
x(tricep_wedge) \
x(brachio_a) \
x(brachio_humerus) \
x(brachio_humerus2) \
x(internal_condyle) \
//
 struct {
#define x(name) v3 name;
  macro_arm(x)
#undef x
 };
 v3 verts[];
};
//
global_const i32 arm_vert_count = sizeof(Arm) / sizeof(v3);

union Forearm
{
#define macro_forearm(x) \
x(radius_bump) \
x(middle_finger_meeter) \
//
 struct {
#define x(name) v3 name;
  macro_forearm(x)
#undef x
 };
 v3 verts[];
};
//
global_const i32 forearm_vert_count = sizeof(Forearm) / sizeof(v3);


//~
global_const v1 head_scale = 9.2f * centimeter;  // @Tweak I have no idea where I pulled this number from

internal void
render_hand(mat4i ot, mat4i forearmLT, Forearm forearm_obj)
{//~NOTE: The hand
 radius_scale_block(fval(0.5038f));
 transform_block(ot);
 vertex_block("hand");
 set_in_block(painter.line_params.nslice_per_meter, fval(5.9797f) * 100.f);
 
 Forearm forearm;
 {
  mat4 transform = ot.inv * forearmLT;
  for_i32(index,0,forearm_vert_count) {
   v3 vertex = forearm_obj.verts[index];
   forearm.verts[index] = mat4vert(transform, vertex);
  }
 }
 
 // NOTE: The "palm_base" moves along with the hand
 vv(palm_base_in, fvert(V3(-0.0476f, -2.6071f, -0.2208f)));
 vv(thumb_kbot, fvert(V3(0.2669f, -2.6071f, -0.2208f)));
 Bez palm_base_line = bez(thumb_kbot, fval2(0.f, 0.3416f),
                          fval2(0,0), palm_base_in,
                          V3y(-1));
 draw(palm_base_line, lp(fval4i(2,2,4,8)));
 bezier_sample(palm_base_line, fval(0.5f));
 
 vv(kline_in, fvert(V3(-0.1346f, -2.987f, -0.3612f)));
 draw(bez(palm_base_in, fval2(-0.1358f, 0.5181f),
          fval2(0.3399f, 0.2818f), kline_in,
          funit(V3(-0.8552f, 0.f, -0.5183f))),
      lp(fval4i(8,0,3,1)));
 vv(kline_out, fvert(V3(0.3755f, -3.0799f, -0.3612f)));
 vv(thumb_palm_conn, fvert(V3(0.3704f, -3.0096f, -0.3481f)));
 draw(bez(thumb_kbot, fvec(V3()),
          fval2(0,0), thumb_palm_conn));
 {// NOTE: Thumb
  //v1 toppose = fval(0.f);      // NOTE: like the meditation hand gesture
  // NOTE: addution;, "adding" the thumb to your hand,
  // NOTE: (max 0.0331f, but meh our anatomy might be totally wrong)
  v1 tadduct = fval(0.f);
  
  mat4i thumbT;
  // TODO: How do we know whether to abduct/adduct or to oppose? Or is this even the right framework?
  //if (toppose) { thumbT = mat4i_rotateY(-toppose, thumb_kbot); }
  {
   // TODO: Hm, this doesn't line the thumb up all the way, there's some other motion in play
   thumbT = mat4i_rotateZ(-tadduct, thumb_kbot);
  }
  transform_block( thumbT );
  
  //NOTE: "thumb_triangle_tip" is a prominent bump on your palm
  vv(thumb_triangle_tip, fvert(V3(0.3693f, -2.6673f, -0.2208f)));
  draw(bez(thumb_kbot, thumb_triangle_tip),
       lp(fval4i(2,4,0,3)));
  
  vv(thumb_kmid_in, fvert(V3(0.4589f, -2.9094f, -0.2208f)));
  if (fbool(1))
  {
   draw(bez(thumb_kmid_in, fvec(V3(-0.0085f, 0.0217f, 0.f)),
            fval2(0,0), from_parent()*thumb_palm_conn));
  }
  else
  {
   draw(bez(thumb_kmid_in, fval2(0.f, 2.9015f),
            fval2(0,0), from_parent()*thumb_palm_conn,
            V3y(1)));
  }
  vv1(thumb_ktop_in, fvert(V3(0.565f, -3.0125f, -0.2208f)));
  draw(thumb_kmid_in, thumb_ktop_in,
       lp(fval4i(8,4,3,1)));
  // NOTE: I really don't know where these should be?
  vv(thumb_tip, fvert(V3(0.671f, -3.0515f, -0.2208f)));
  vv(thumb_kmid_out, fvert(V3(0.5068f, -2.8209f, -0.2208f)));
  draw(bez(thumb_triangle_tip, fval2(-0.2102f, 0.1944f),
           fval2(0.4217f, 0.2078f), thumb_kmid_out,
           V3x(-1)),
       lp(fval4i(2,4,4,6)));
  vv1(thumb_ktop_out, fvert(V3(0.6128f, -2.9593f, -0.2208f)));
  draw(bez(thumb_kmid_out, fval2(0.f, 0.3706f),
           fval2(0,0), thumb_ktop_out,
           V3x(-1)));
  draw(bez(thumb_ktop_out, fval2(0.f, 0.2369f),
           fval2(0.4569f, -1.7741f), thumb_tip,
           V3x(-1)),
       lp(fval4i(4,1,2,1)));
  draw(bez(thumb_ktop_in, fval2(0,0),
           fval2(0.2207f, 1.4562f), thumb_tip, V3x(-1)),
       lp(fval4i(1,1,1,2)));
 }
 draw(thumb_palm_conn, kline_out, lp(fval4i(5,3,1,1)));
 Bez knuckle_line = bez(kline_out, fval2(-0.0263f, 0.259f),
                        fval2(0,0), kline_in,
                        V3y(-1));
 if (level1){ draw(knuckle_line); }
 
 draw(bez(forearm.radius_bump, fvec(V3(-0.f, 0.0101f, 0.0167f)),
          fval2(0,0), thumb_kbot));
 
 {// NOTE: The knuckles and fingers
  vertex_block("knuckles/fingers");
  
  i32 const finger_count = 4;  // NOTE: do I gotta tell you?
  
  v1 knuckle_ts[finger_count] = {
   fval(0.1236f),
   fval(0.3668f),
   fval(0.6424f),
   fval(0.8955f)};
  v3 knuckle_tops[finger_count];  // TODO: Rename me PLEASE!
  for_i32(index,0,4) {
   knuckle_tops[index] = bezier_sample(knuckle_line, knuckle_ts[index]);
  }
  
  v3 knuckle_bots[finger_count];
  {
   v3 tb_vec = fvecy(0.06f);
   for_i32(index,0,4){
    knuckle_bots[index] = knuckle_tops[index]+tb_vec;
   }
  }
  
  for_i32(index,0,4) {
   draw_line(knuckle_tops[index], knuckle_bots[index]);
  }
  
  {// NOTE: trying to define the profile view here
   v3 middle_knuckle_bot = knuckle_bots[1];
   vv(middle_palm_bot, knuckle_bots[1]+fvec(V3(-0.f, 0.2527f, -0.022f)));
   draw_line(middle_knuckle_bot, middle_palm_bot, lp(I4(fval(3))));
   draw(bez(forearm.middle_finger_meeter, fval2(-0.1285f, 0.1793f),
            fval2(0.f, -0.5674f), middle_palm_bot,
            V3z(-1)));
  }
  
  v3 finger_vecs[finger_count] = {
   -fvecy(0.5077f), //index
   -fvecy(0.5619f), //middle
   -fvecy(0.5247f), //ring
   -fvecy(0.4449f), //pinky (NOTE: lines up with the ring's top knuckle)
  };
  v3 finger_bases[finger_count+1];
  {
   v1 finger_base_ts[finger_count+1] = {
    0.f,
    fval(0.2267f),
    fval(0.5f),
    fval(0.75f),
    1.f,
   };
   v3 offset = -fvecy(0.0818f);
   for_i32(index,0,finger_count+1)
   {
    finger_bases[index] = offset+bezier_sample(knuckle_line,
                                               finger_base_ts[index]);
    indicate(finger_bases[index]);
   }
  }
  
  v1 bot_tbend = fval(0.1f);
  v1 mid_tbend = fval(0.057f);
  v1 top_tbend = fval(0.1f);
  if (fbool(0)) { bot_tbend = 0.f; mid_tbend = 0.f; top_tbend = 0.f; }
  
  for_i32(ifinger,0,finger_count)
  {
   v3 k = knuckle_tops[ifinger];
   v3 vec = finger_vecs[ifinger];
   v3 xvec = fvecx(0.0367f);
   
   vv(kmid, k+0.5f*vec);
   vv(ikmid, kmid-xvec);
   vv(okmid, kmid+xvec);
   
   transform_block( mat4i_rotateX(-bot_tbend, k) );
   
   {// NOTE: bottom phalanges
    v2 knuckle_control = fval2(0.4816f, 0.329f);
    // NOTE: the webbing doesn't connect together very well
    draw(bez(finger_bases[ifinger], fval2(0.f, 0.5811f),
             knuckle_control, okmid,
             V3x(-1)),
         lp(fval4i(3,1,1,1)));
    draw(bez(finger_bases[ifinger+1], fval2(-0.3782f, 0.8758f),
                  knuckle_control, ikmid,
                  V3x(1)),
         lp(fval4i(3,3,3,1)));
    
    if (ifinger == 0)
    {// NOTE: concerning the index finger
     draw(bez(kline_out, fvec(V3(0.0001f, 0.f, 0.f)),
              fval2(0,0), finger_bases[0]),
          lp(fval4i(0,1,1,3)));
    }
    if (ifinger == finger_count-1)
    {// NOTE: pinky
     draw(bez(finger_bases[ifinger+1], fvec(V3(-0.0049f, 0.f, 0.f)),
              fval2(0,0), kline_in),
          lp(fval4i(2,3,0,1)));
    }
   }
   
   {// NOTE: middle+top phalanges
    transform_block( mat4i_rotateX(-mid_tbend, kmid) );
    
    vv(ktop, k+0.75f*vec);
    vv(iktop, ktop-xvec);
    vv(oktop, ktop+xvec);
    {// NOTE: middle phalanges
     v2 control = fval2(-0.0473f, 0.7281f);
     draw(bez(ikmid, control, V2(), iktop, V3x(1)),
          lp(fval4i(3,3,3,1)));
     draw(bez(okmid, control, V2(), oktop, V3x(-1)),
          lp(fval4i(3,0,2,3)));
    }
    
    v3 tip = k+vec;
    {// NOTE: top phalanges
     transform_block( mat4i_rotateX(-top_tbend, ktop) );
     
     v2 tip_control = fval2(-0.3298f, 1.1379f);
     v2 knuckle_control = fval2(0.2954f, -0.1896f);
     draw(bez(tip, tip_control, knuckle_control, iktop, V3x(-1)),
          lp(fval4i(3,0,0,2)));
     draw(bez(tip, tip_control, knuckle_control, oktop, V3x(1)),
          lp(fval4i(4,0,2,3)));
    }
   }
  }
 }
}

internal Forearm
render_lower_arm(mat4i const&forearmLT,
                 mat4 const&armLT, Arm const&arm_obj,
                 v3 elbow_offset, v3 elbow_up_out)
{//~NOTE: The lower arm / forearm
 mat4i const&ot = forearmLT;
 transform_block(forearmLT);
 Arm arm;
 for_i32(index,0,arm_vert_count)
 {
  v3 vertex = arm_obj.verts[index];
  arm.verts[index] = mat4vert(ot.inv*armLT, vertex);
 }
 
 if (level1 || get_preset() == 3)
 {//NOTE: The forearm bones from the front view (as rough guide)
  viz_block;
  v3 o = fvert(V3(0.f, -1.8f, -0.3049f));
  {//NOTE: ulna (inner)
   v3 up = o+fvert(V3(-0.1138f, 0.7466f, -0.1428f));
   v3 down = o+fvert(V3(-0.0604f, -0.792f, 0.f));
   draw(bez(up,
            fvert(V3(0.0351f, 0.f, -0.0416f)), 
            fvert(V3(-0.0342f, -0.0572f, 0.f)), 
            down));
  }
  {//NOTE: radius (outer)
   {//NOTE: lower part
    {// NOTE: Outer
     v3 up = o+fvert(V3(0.0145f, 0.6499f, 0.f));
     v3 down = o+fvert(V3(0.073f, -0.8001f, 0.f));
     draw( bez(up, 
               fvert(V3()),
               fvert(V3(-0.f, 0.f, -0.055f)),
               down));
    }
   }
  }
  {//NOTE: The hinge
   v3 in  = o+fvert(V3(-0.1462f, 0.7866f, 0.f));
   v3 out = o+fvert(V3(0.066f, 0.8235f, 0.f));
   draw( bez(in, out));
  }
 }
 
 v3 in_a, in_b, in_c;
 vv(near_palm_in, fvert(V3(-0.0137f, -2.5095f, -0.2402f)));
 {// NOTE: Inner side
  vertex_block("forearm_in");
  va(in_a, fvert(V3(-0.2345f, -0.8464f, -0.0226f)));
  va(in_b, fvert(V3(-0.269f, -1.0908f, -0.0438f)));
  va(in_c, fvert(V3(-0.0881f, -2.0998f, -0.1166f)));
  //fill4(in_a, in_b, in_c, arm.bicep_in_bot);
  draw(bez(in_a, fvert(V3(0.0003f, 0.f, 0.f)),
           fval2(0,0), in_b),
       lp(fval4i(0,2,2,4)));
  draw(bez(in_b, fvert(V3(-0.0318f, 0.f, 0.f)),
           fval2(0,0), near_palm_in),
       lp(fval4i(4,3,1,1)));
 }
 
 v3 brachio_b, brachio_c;
 {//NOTE: Outer side (the thumb side): Brachio-radialis
  vertex_block("brachio");
  v3 &a = arm.brachio_a;
  v3 &b = brachio_b;
  v3 &c = brachio_c;
  va(b, a+fvert(V3(0.1483f, -0.2286f, -0.0134f)));
  va(c, fvert(V3(0.18f, -1.7693f, -0.0295f)));
  draw(bez(b, fvert(V3(0.0352f, 0.f, -0.f)),
           fval2(0,0), c),
       lp(fval4i(6,5,3,0)));
  vv(d, fvert(V3(0.0371f, -0.7757f, -0.3281f)));
  vv(e, d+fvert(V3(0.0682f, -0.2334f, -0.024f)));
  vv(f, d+fvert(V3(0.0466f, -0.6053f, 0.0122f)));
  //fill4(a,b,e,d);
  //fill4(b,c,f,e);
  fill4(a,b,c, arm.bicep_out_bot);
 }
 
 fill4(brachio_c, arm.bicep_out_bot, arm.bicep_in_bot, in_c);
 
 {//NOTE: The elbow (NOTE: It moves when you bend your arm, so it has to be in the forearm)
  vertex_block("elbow");
  
  v3 &o = elbow_offset;
  vv(up_in, o+fvec(V3(-0.1512f, 0.9396f, -0.2042f)));
  vv(low_in, o+fvec(V3(-0.1031f, 0.7618f, -0.2006f)));
  i4 elbow_radii = fval(I4(4,3,3,2));
  draw(bez(up_in,low_in), elbow_radii);
  v3 &up_out = elbow_up_out;
  indicate(up_out);
  vv(low_out, o+fvec(V3(-0.063f, 0.7591f, -0.2006f)));
  draw(bez(up_out,low_out), elbow_radii);
  
  {// NOTE ;tricep_connections
   draw(bez(arm.white_in, up_in), fval4i(0,3,3,6));
   draw(bez(arm.white_out, up_out), fval4i(0,0,0,6));
  }
  
  fill3(arm.tricep_wedge, arm.white_out, up_out);
 }
 
 // brachio_top
 draw(bez(arm.brachio_humerus,
          fval2(0.f, 0.0875f),
          fval2(0,0),
          arm.brachio_a,
          V3z(1)));
 // brachio_mid
 draw(bez(arm.brachio_humerus2,
          fval2(0.f, 0.0875f),
               fval2(0.f, 0.1935f),
               brachio_b,
               funit(V3(0.7928f, 0.f, 0.6095f))),
      fci(fval4i(0,0,3,6)));
 fill3(arm.brachio_humerus, arm.brachio_a, elbow_up_out);
 fill4(arm.brachio_humerus, arm.brachio_humerus2, arm.brachio_a, brachio_b);
 
 vv(radius_bump, fvert(V3(0.2613f, -2.512f, -0.3186f)));
 draw(bez(brachio_c, fvec(V3(0.f, 0.0162f, 0.0319f)),
          fval2(0,0), radius_bump),
      lp(fval4i(0,6,4,0)));
 {//NOTE: wrist
  vv0(ulnar_ball, fvert(V3(-0.0453f, -2.5077f, -0.3152f)));
  
  vv(v566, fvert(V3(-0.0164f, -1.8951f, -0.3487f)));
  draw(bez(ulnar_ball, fvec(V3(0.0482f, 0.f, -0.f)),
           fval2(0,0), v566),
       lp(fval4i(0,5,2,0)));
  draw(bez(arm.internal_condyle, fvert(V3(-0.0671f, 0.f, 0.f)),
           fval2(0,0), v566),
       lp(fval4i(1,5,5,0)));
  // NOTE: This part of the wrist is receded from the palm
  // NOTE: You can feel the radius bump on your wrist
  //vv(radius_front_in, fvert(V3(0.0433f, -2.5696f, -0.3227f)));
  //draw_line(radius_bump, radius_front_in);
  vv(radius_back, fvert(V3(0.2604f, -2.5696f, -0.4646f)));
  draw_line(radius_back, radius_bump);
  draw_line(radius_back, radius_back+fvert(V3(-0.151f, 0.986f, 0.f)));
 }
 vv(middle_finger_meeter, fvert(V3(0.0936f, -2.5386f, -0.4688f)));
 
 v3 v2189 = fvert(V3(-0.2725f, -0.8924f, -0.251f));
 draw(bez(v2189,
          fvec(V3(0.f, 0.1288f, 0.1481f)),
          fval2(0,0),
          fvert(V3(0.1923f, -1.8531f, -0.198f))));
 
 //-@Exports
 Forearm forearm_obj;
#define export_(vertex)  forearm_obj.vertex = vertex;
 macro_forearm(export_);
#undef export_
 return forearm_obj;
}

internal void
render_character(Arena *scratch, v1 state_time, i32 viewport_index,
                 b32 show_grid)
{// NOTE: Drawing the character
 b32 show_eyeball = false;
 b32 show_loomis_ball = fbool(0);
 if( level2 ){ show_loomis_ball = true; }
 
 i32 preset = get_preset();
 {
  if (preset == 2) {show_eyeball = true;}
  if (preset == 3) {show_loomis_ball = fbool(1);}
  if (preset == 4) {show_loomis_ball = fbool(0);}
 }
 
 b32 is_main_viewport = (viewport_index == 0);
 
 //~ IMPORTANT: Conventions
 //
 // 1: Things are on the left (side of the face) by default. Meaning x>0.
 // 2: Things like eyeY are just landmarks, nobody cares if there is actually no vertex lying on that y.
 //~
 
 b32 lod2 = fbool(1);
 //-
 //
 //-
 
 argb shade_color = compute_fill_color(0.094014f);
 if ( fbool(0) ) { shade_color = painter.fill_color; }
 
 v1 animation_time = state_time;
 {// NOTE: animation_speed_multiplier
  animation_time *= fval(1.f);  // important
 }
 
 //-NOTE: Animation
 v1 thead_theta, thead_phi, thead_roll, tblink, teye_theta, teye_phi;
 {// TODO: this crashes when arena is empty, because static areana is crap!
  Temp_Memory_Block temp(scratch);
  Movie_Shot *shot = push_struct(scratch, Movie_Shot);
  {//NOTE: ;set_movie_shot
   i32 sel = fvali(0);
   Shot_Function *played_animation = 
    sel == 1 ? movie_shot_0 :
   movie_shot_blinking;
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
 b32 version2 = fbool(1);
 
 mat4i headT;
 {// NOTE: The ;head + face
  mat4i &ot = headT;
  {// NOTE: The head
   v1 head_theta_max = 0.15f;   // @Tweak
   v1 head_phi_max   = 0.125f;  // @Tweak
   v1 head_roll_max  = 0.125f;  // @Tweak
   v1 head_theta = lerp(0.f, thead_theta, head_theta_max);
   v1 head_phi   = lerp(0.f, thead_phi, head_phi_max);
   v1 head_roll  = lerp(0.f, thead_roll, head_roll_max);
   
   v3 rotation_pivot = fvert3(0.f, -1.1466f, 0.3598f, clampx);  // TODO: this is totally wrong place now!
   headT = (mat4i_scale(head_scale) * 
            mat4i_rotate_tpr(head_theta, head_phi, head_roll, rotation_pivot));
   indicate(headT*rotation_pivot);
  }
  transform_block(ot);
  symx_on;
  set_in_block(painter.line_params.nslice_per_meter, fval(4.1128f) * 100.f);//NOTE: crank up the lod
  v1 head_unit = 1.f+square_root(2);
  
  v3 loomis_cross_center = V3(0.f,0.f,1.f);
  v1 faceZ = 1.f;
  
  v1 root2 = square_root(2.f);
  v1 inv_root2 = 1.f / root2;
  v1 loomis_unit = inv_root2;// NOTE: From brow to nose tip and all that jazz
  
  //NOTE: My nose_tip is actually higher than the nose base (the loomis nose y is for the nose base)
  v1 noseY = -loomis_unit;
  vv(nose_tip, V3(0.f, noseY, faceZ) + fvert3(0.f, 0.0237f, 0.0916f,clampx));
  
  mat4 &to_local = ot.inverse;
  {// NOTE: profile score
   v3 camera_local = to_local * camera_world_position(painter.camera);
   v3 view_vector = noz(camera_local - nose_tip);
   
   v1 x = view_vector.z;
   v1 y = view_vector.x;
   
   v1 theta = arctan2(y,x);
   
   painter.profile_score = absolute(theta*4.f);
   
   if ( fbool(0) && is_main_viewport ) { DEBUG_VALUE(painter.profile_score); }
  }
  
  v1 chinY = -2.f*loomis_unit;
  vv(chin_middle, V3(0,chinY,faceZ) + V3z(fval(-0.0856f)));
  
  v1 face_sideX = inv_root2;
  v3 loomis_side_center = V3x(face_sideX);
  v1 loomis_side_radius = inv_root2;
  v3 ear_center = V3x(face_sideX) + fvert3(0.0416f, -0.3601f, -0.2332f);
  indicate(ear_center);
  
  v3 side_circle_center = V3(face_sideX,0,0);
  v1 side_circle_radius = loomis_unit;
  
  vv(chinL, V3(0,chinY,chin_middle.z) + fvert(V3(0.1572f, 0.0504f, -0.077f)));
  
  v3 jaw = V3(face_sideX, chinY, 0) + fvert(V3(-0.0844f, 0.4575f, -0.0247f));
  
  v3 mouth_base = V3(0, lerp(chinY, fval(0.5871f), noseY), faceZ);
  v3 mouth_corner = mouth_base + fvert(V3(0.2499f, 0.f, -0.2801f), clampy);
  v3 &mouth_cornerL = mouth_corner;
  v3 mouth_cornerR = negateX(mouth_corner);
  
  v1 nose_baseY = noseY;
  indicate(mouth_corner);
  
  //-
  v1 nose_wingX = fval(0.1054f);
  
  v1 bridgeX = lerp(0.f, 0.284149f, nose_wingX);
  
  v1 browY = 0.f;
  
  v1 nose_sideX = fval(0.0586f);
  //NOTE: nose_rootL is just below the brow
  v3 nose_rootL = V3(nose_sideX, 
                     lerp(browY, fval(0.1534f), noseY), 
                     faceZ+fval(-0.104f));
  indicate(nose_rootL);
  
  v3 nose_wing = { nose_wingX, nose_baseY, faceZ };
  indicate(nose_wing);
  
  // NOTE: Is this point real? Not really, but it's here to stay.
  v3 nose_wing_up = nose_wing+fvert(V3(-0.011f, 0.1107f, 0.0069f));
  indicate(nose_wing_up);
  //-
  
  v3 lip_low_center;
  {
   Bezier lip_up, philtrum_line, philtrum_line_mid, lip_low_line;
   v1 philtrumX = fval(0.0435f, f20th);
   b32 nerf_mouth = lod2 == false;
   scale_in_block(painter.line_radius_unit, fval(0.610978f));
   
   v3 philtrum_up = V3(0, nose_baseY, faceZ) + fvert3(0,0,0);
   
   v3 philtrum_low = mouth_base + fvert(V3(0.f, 0.078f, 0.035f));
   philtrum_line_mid = bez_old(philtrum_up ,
                               fvert(V3(0.f, -0.1241f, 0.0076f)),
                               fvert(V3(0.f, 0.0481f, -0.0345f)),
                               philtrum_low);
   indicate(philtrum_low);
   
   philtrum_line = philtrum_line_mid;
   for_i32(index,0,4) { philtrum_line[index].x += philtrumX; }
   
   for_i32(index,0,4) { philtrum_line_mid[index].z -= fval(0.0065f); }
   
   {// NOTE: philtrum
    Line_Params params = profile_visible(fval(0.7535f));
    params.radii = fval4(0.f, 0.f, 0.6235f, 0.0781f);
    draw(philtrum_line, params);
   }
   
   v3 philtrum_lowL = philtrum_low+V3x(philtrumX);
   lip_up  = bez(philtrum_lowL, 
                 fvert(V3(0.088f, 0.1234f, 0.2037f)),
                 fvert(V3(-0.0951f, 0.0104f, 0.3071f)),
                 mouth_corner);
   {// NOTE: lip_up 
    Line_Params params = profile_visible(fval(0.6014f));
    params.radii = fci(I4(1,1,3,0));
    if (level1) { params = painter.line_params;  }
    draw(lip_up, params);
   }
   {
    v3 lip_in = sety(philtrum_lowL+fvert(V3(-0.f, 0.f, -0.0527f), clampy),
                     mouth_corner.y+fval(0.0133f));
    //NOTE: Main mouth line, looks wicked awesome!
    Bezier lip_up_bot = bez(lip_in, fval(V4(-0.0828f, 0.5237f, 0.f, 0.f)), mouth_corner,
                            funit(V3(0.f, 0.2748f, 0.9615f)));
    draw(lip_up_bot,
         fval4( 0.4876f, 0.3631f, 0.5066f, -0.2905f));
   }
   {
    //NOTE: Upper lip is smaller than lower lip
    lip_low_center = mouth_base + fvert(V3(0.f, -0.088f, -0.0133f));
    indicate(lip_low_center);
    lip_low_line = bez(lip_low_center, fval(V4(0.f, 1.0473f, 0.f, -0.7242f)), mouth_corner,
                       funit(V3(0.f, -0.1848f, 0.9828f)));
    if (!nerf_mouth)
    {
     v4 radii = fval4(0.7f, -0.2807f, 0.f, 0.f);
     if (level1) {radii = painter.line_params.radii;}
     draw(lip_low_line,radii);
    }
   }
   
   v3 philtrum_offset_point = nose_wing+fvert(V3(-0.0499f, -0.0725f, -0.0489f));
   indicate(philtrum_offset_point);
   fill( philtrum_offset_point, lip_up);
   fill( philtrum_offset_point, philtrum_line_mid);
   fill3( philtrum_offset_point, nose_wing, philtrum_up+V3x(philtrumX));
   fill3( philtrum_offset_point, mouth_corner, nose_wing);
   
   fill3(philtrum_lowL,philtrum_low,philtrum_offset_point);;
  }
  
  //-NOTE: Nose
  Bezier nose_line_side = bez(nose_rootL, fval(V4()), addx(nose_tip, nose_sideX),
                              funit(V3(0.f, 0.f, 1.f)));
  
  {
   auto params = profile_visible(0.3f);
   if (level1) { params = painter.line_params; }
   // TODO: our nose needs some renovation
  }
  
  v3 nose_tipL = nose_tip+V3x(nose_sideX);
  Bezier nose_line_under = bez_old(nose_tipL,
                                   fvert(V3(0.0121f, -0.0404f, -0.1108f)),
                                   V3(),
                                   nose_wing);
  draw(nose_line_under, fval4(0.151758f, 0.699609f, 1.149659f, -0.148325f ));
  
  {
   symx_off;
   Bezier nose_line_sideR = negateX(nose_line_side);
   fill_dbez( nose_line_side, nose_line_sideR);
  }
  
  {//NOTE: Draw that tasty nose wing :>
   v3 d1089 = nose_wing+fvert(V3(0.0539f, 0.07f, -0.0417f));
   Bezier P = bez_raw( nose_wing, d1089, d1089, nose_wing_up);
   draw(P,profile_visible(0.70f));
  }
  {// NOTE: Some nose tip drawing (very fudgy)
   // TODO: We can merge this with the "nose under" lines, maybe? 
   // Problem is we kinda need to separate out what the nose side is, in order to do triangles fills
   symx_off;
   v3 control = fvert(V3(-0.0533f, 0.0283f, 0.0438f));
   draw(bez_old(nose_tipL,
                control,
                negateX(control),
                negateX(nose_tipL)),
        fval(0.7097f)*painter.line_params.radii);
  }
  
  //~NOTE: Cheek situation pretty complex!
  vv(cheek_low,(V3(0,noseY,faceZ) + 
                fvert(V3(0.6062f, 0.f, -0.4289f))));
  
  fill4(cheek_low, jaw, chinL, mouth_corner);
  
  v3 nose_root_backL = fvert(V3(0.f, -0.0651f, -0.1755f), clampx) + V3x(nose_sideX);
  
  vv(brow_out, fvert(V3(0.585f, -0.1342f, 0.5485f)));
  vv(cheek_up, brow_out+fvert(V3(0.f, -0.2935f, 0.f)));
  if (level1)
  {
   draw_line( brow_out, cheek_up, viz_line_params());  //NOTE: Literally a straight line
  }
  
  //NOTE: brow_ridge is the neutral bone structure of the brow, as well as the eye socket
  //NOTE: I have few ideas what its shape should be, also very hard to draw right
  Bez brow_ridge = bez(nose_rootL, fval(V4(0.f, 0.3208f, 0.f, 0.6236f)), brow_out,
                       funit(V3(0.f, 0.6019f, 0.7986f)));
  if (level1) { viz_block; draw(brow_ridge); }
  
  v3 es_up_in;
  {//~NOTE: The Eyes
   vertex_block("eye");
   // NOTE: We use this "eyeT" as a compromise: as a small eye sub-system is small.
   // TODO: The 5-eyes rule is still elusive to me, I only have like 4 eyes.
   v3 eyeO = fvert(V3(0.1953f, -0.2348f, 0.8954f));
   v3 eye_scale = fval(V3(0.8324f, 0.9882f, 0.92f));
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
    if (level1) { draw(bez(eye_in, nose_rootL)); }
   }
   
   v3 eye_out = eye_in + V3(2.f * eye_in.x + fval(-0.03f),
                            0,
                            fval(-0.154f));
   indicate(eye_out);
   
   if (level1) { viz_block; draw_line( eye_out, brow_out); }
   
   va(es_up_in, bezier_sample(brow_ridge, fval(0.3668f)));
   
   Bezier eye_up_line = bezd_old(eye_in, 
                                 fvert3(0.0814f, 0.3741f, 0.2401f),
                                 fval2(0.0159f, 0.3341f),
                                 eye_out);
   
   fill( es_up_in, eye_up_line);
   
   argb eye_in_shade = shade_color;
   fill3(
         nose_rootL, eye_in, es_up_in, 
         repeat3(eye_in_shade));
   Bezier eye_low_line = bezd_old(eye_in, 
                                  fvert3(0.1735f, -0.2094f, 0.1663f), 
                                  fval2(0.117f, 0.2053f), 
                                  eye_out);
   
   {
    v3 eye_low_patch = lerp(eye_in, fval(0.4768f), cheek_low);
    indicate(eye_low_patch);
    fill( eye_low_patch, eye_low_line);
    fill3(eye_low_patch, eye_out, cheek_low);
    fill3(eye_out, brow_out, es_up_in);
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
    v4 eye_up_line_radius  = V4( eye_in_radius, eye0_radius, eye1_radius, eye_out_radius);
    v4 eye_low_line_radius = eye_up_line_radius;
    eye_low_line_radius[1] = fval(0.0555f);
    if (lod2)
    {
     draw(eye_up_line_now,eye_up_line_radius);
     draw(eye_low_line,eye_low_line_radius);
     // NOTE: The eyelid
     fill_dbez( eye_up_line, eye_up_line_now, eye_in_shade);
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
       eyeball_radii = V4(big,small,small,big);
      }
      draw_bezier_circle(eyeballL_T, eyeball_radii);  // NOTE: A prime meridian
      mat4 eyeball_equator = eyeballL_T*rotateX(0.25f);
      draw_bezier_circle(eyeball_equator, eyeball_radii);
     }
    }
    
    if (lod2)
    {// NOTE: Iris (featuring Mr. Depth Offset Hack)
     set_in_block(painter.line_params.nslice_per_meter, 128.f*fval(7.6378f));
     v1 iris_depth_offset = painter.fill_depth_offset + 1*centimeter;
     painter.line_params.depth_offset += iris_depth_offset;
     painter.fill_depth_offset        += iris_depth_offset;
     
     {
      mat4 irisRelT;
      {
       //NOTE: https://blenderartists.org/t/just-how-big-should-the-iris-be-relative-to-the-actual-eyeball/1214393/3
       // so iris radius is [0.5, 0.6] that of eyeball
       v1 iris_radius     = fval(0.4934f);
       v3 iris_rel_center = V3z( square_root(1-squared(iris_radius)) );
       irisRelT = mat4_translate(iris_rel_center) * mat4i_scale(iris_radius);
      }
      v4 iris_radii;
      {
       v1 big   = 0.5f;
       v1 small = 0.25f;
       iris_radii = V4(big,small,small,big);
      }
      {
       mat4 irisLT = eyeballL_T*irisRelT;
       draw_bezier_circle(irisLT, iris_radii);
       mat4 irisRT = eyeballR_T*irisRelT;
       draw_bezier_circle(irisRT, iris_radii);
      }
     }
     
     painter.line_params.depth_offset -= iris_depth_offset;
     painter.fill_depth_offset        -= iris_depth_offset;
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
       indicate0(result.closest_point);
      }
     }
    }
   }
   
   // NOTE: These fills are most dubious...
   fill3( nose_wing, cheek_low, eye_in);
   fill3( nose_root_backL, eye_in, nose_rootL);
   fill3( nose_root_backL, nose_wing, eye_in);  // NOTE: smooth shading this thing is a disaster!
   fill3( eye_out, cheek_up, brow_out);
  }
  
  //-NOTE: Eyebrows
  {
   radius_scale_block(fval(0.8053f));
   // NOTE: determined expression
   v3 brow_in = (bezier_sample(brow_ridge, fval(0.1313f)) +
                 fvert(V3(0.0044f, -0.0566f, 0.0056f)));
   indicate(brow_in);
   //NOTE: brow_mid is a really fuzzy point!
   v3 brow_mid = (bezier_sample(brow_ridge, fval(0.593f)) + 
                  fvert(V3(-0.0006f, 0.0008f, 0.0086f)));
   indicate(brow_mid);
   
   Bezier brow_in_line = bez(brow_in, brow_mid);
   v1 brow_joint_radius;
   if (lod2)
   {
    {
     v4 radii = fval4(0.3344f, 0.6654f, 0.61f, 1.f);
     brow_joint_radius = radii.v[3];
     draw(brow_in_line, radii);
    }
    {
     Bezier brow_out_line = bez(brow_mid, fval((Planar_Bezier{v2{0.f, 0.5549f}, v2{0.f, 0.4409f}, v3{0.f, 1.f, 0.f}})), brow_out);
     v4 radii = fval4(0.25f, 0.8f, 0.8f, 0.25f);
     radii.v[0] = brow_joint_radius;
     draw(brow_out_line, radii);
    }
   }
  }
  
  Bezier jaw_line;
  {
   v3 cj = lerp(chinL, 0.5f, jaw);
   {
    // TODO: This line (and "cj") is here because it happens to look good
    jaw_line = bez_raw( ear_center, jaw, jaw, cj);
    v4 radii = painter.line_params.radii;
    radii[0] = 0.f;
    radii[1] = 0.f;
    draw(jaw_line, radii);
   }
  }
  
  fill3( nose_wing, mouth_corner, cheek_low);
  
  // TODO(kv): Extreme hackiness, maybe we could replace bezier_poly
  {
   Bezier line = bez( nose_root_backL, nose_wing);
   fill_dbez( nose_line_side, line);
  }
  
  v3 foreheadL = brow_out + fvert(V3(0.0477f, 0.7772f, -0.0232f));
  v1 foreheadY = foreheadL.y;
  indicate(foreheadL);
  
  {
   v3 verts[] = {jaw, cheek_up, brow_out, foreheadL};
   fill_fan( ear_center, verts, alen(verts));
  }
  
  fill3(cheek_up,jaw,cheek_low);
  
  if (level1)
  {
   viz_block;
   draw(bez(brow_out, cheek_up));
  }
  
  {//-;Ear
   radius_scale_block(0.5f);
   v3 ear_back = ear_center + fvert(V3(0.1431f, 0.0034f, -0.3328f));
   v3 ear_low  = ear_center + fvert(V3(-0.0639f, -0.5264f, 0.1141f));
   Bez ear1 = bez(ear_center, fval(V4(0.2792f, 3.5588f, 0.1526f, 0.3068f)), ear_back,
                  funit(V3(0.3587f, 0.692f, -0.6264f)));
   Bez ear2 = bez_c2(ear1, fvert(V3()), ear_low);
   v4 radii1 = fval(V4(0.25f, 1.9122f, 0.7259f, 1.3455f));
   v4 radii2 = radii_c2(radii1, fval2(-0.9383f, 0.f));
   draw(ear1, radii1);
   draw(ear2, radii2);
  }
  
  //-NOTE: The incredibly non-thrilling story of the head
  
  v3 head_back_out = fvert(V3(0.6208f, 0.1421f, -0.4585f));
  indicate(head_back_out);
  Bezier head_top_out_line = bez_old(foreheadL,
                                     fvert(V3(0.1239f, 0.1894f, -0.2995f)),
                                     fvert(V3(-0.0634f, 0.3622f, 0.1935f)),
                                     head_back_out);
  if (level1)
  {
   draw(head_top_out_line);
  }
  v3 forehead_in = V3y(foreheadY) + fvert3(0.1445f, 0.072f, 0.8969f);
  indicate(forehead_in);
  
  Bezier hair_hline = bez_raw(foreheadL,
                              forehead_in,
                              negateX(forehead_in),
                              negateX(foreheadL));
  
  {
   symx_off;
   if (level1) { draw(hair_hline); }
  }
  
  vv(head_back_in,fvert3(0.2242f, 0.6295f, -1.1258f));
  Bezier head_top_in_line = bezd_old(forehead_in,
                                     fvert3(0.f, 0.4276f, -0.1837f),
                                     fval2(0.3589f, 0.1402f),
                                     head_back_in);
  if (level2)
  {
   draw(head_top_in_line);
  }
  fill_patch( patch_symx(head_top_out_line, head_top_in_line));
  
  vv(behind_ear, ear_center + fvert(V3(-0.183f, -0.3342f, -0.1239f)));
  //sendv(behind_ear);
  Bezier head_back_out_line;
  {
   //TODO: omg why did you make it so confusing bro?
   head_back_out_line = bez_old(head_back_out,
                                fvert(V3(0.f, 0.001f, -0.0779f)),
                                fvert(V3(-0.0535f, 0.197f, -0.6398f)),
                                behind_ear);
   if (level2) { draw(head_back_out_line); }
   vv(head_back_in2, fvert(V3(0.4676f, -0.7579f, -0.1264f)));//note: Notice the loomis side circle
   
   Bezier head_back_in_line = bez_old(head_back_in,
                                      fvert(V3(-0.f, -0.3664f, -0.3108f)),
                                      fvert(V3(0.f, -0.4228f, -1.1179f)),
                                      head_back_in2);
   
   
   
   
   Patch head_back = patch_symx(head_back_out_line, head_back_in_line);
   if (level2) { draw(head_back_in_line); }
   fill_patch( head_back);
   
   //NOTE: draw vertical line
   {
    Line_Params lp = lp_alignment_threshold(fval(0.9797f));
    lp.radii = fci(I4(1,3,3,0));
    draw(get_uline(head_back, 0.5f), lp);
   }
  }
  
  fill(ear_center, head_top_out_line);
  fill_dbez(ear_center, head_back_out, head_back_out_line);
  
  if (fbool(0))
  {//NOTE The forehead
   v3 hair_hline_center = bezier_sample(hair_hline, 0.5f);
   v3 hair_hline_half_controls[4];
   v3 esuo = brow_out;
   v3 control_point = V3(fval(0.2448f), -esuo.y/3.f, (4.f-esuo.z)/3.f);
   // NOTE This line goes through the cross center!
   Bez brow_hline = bez_raw(esuo,
                            control_point,
                            negateX(control_point),
                            negateX(esuo));
   {
    symx_off;
    fill_patch( brow_hline,  brow_hline,  hair_hline, hair_hline);
   }
  }
  
  {
   //~NOTE: Chin
   symx_off;
   v3 chin_up_center = chinL + fvert(V3(-0.1993f, 0.0993f, 0.0805f));
   v3 chin_upL = chin_up_center + fvert(V3x(0.1274f));
   indicate(chin_upL);
   v3 chin_upR = negateX(chin_upL);
   v3 chinR = negateX(chinL);
   if (fbool(0))
   {
    Bezier chin_line = bez_raw( (chinL), chin_middle, chin_middle, negateX(chinL));
    //draw(p, chin_line, profile_visible(p, 0.57f));
    draw( chin_line);
   }
   
   {//NOTE: chin fixes
    line_radius_fine_tip;
    v3 mouth_low_valley = lip_low_center+fvert3(0.f, -0.0704f, -0.0417f, clampx);
    {
     Line_Params params = profile_visible(0.73f);
     draw( bez(lip_low_center, mouth_low_valley), params);
     draw( bez(chin_middle, setx(chin_upL, 0.f)), params);
    }
    if (level1)
    {
     draw(bez(mouth_low_valley,chin_middle));
    }
    // so we want to interpolate valley_under_lip
    v3 chin_point0 = mouth_corner;
    v3 control_point = V3x(fval(0.f)) + V3(0.f, 
                                           (-chin_point0.y + 4*mouth_low_valley.y)/3.f,
                                           (-chin_point0.z + 4*mouth_low_valley.z)/3.f);
    Bez mouth_low_valley_line = bez_raw(
                                        chin_point0,
                                        control_point,
                                        negateX(control_point),
                                        negateX(chin_point0));
    if (level1) {draw(mouth_low_valley_line);}
    {
     symx_on;
     fill3(lip_low_center,mouth_corner,mouth_low_valley);
     fill4( chin_middle, chinL, mouth_corner, mouth_low_valley);
    }
   }
  }
  
  vv(head_neck_junction, fvert(V3(0.f, -1.1949f, 0.3195f), clampx));
  {
   symx_off;
   draw( bez(chin_middle, head_neck_junction));
   fill3( head_neck_junction, chinL, jaw);
   fill3( head_neck_junction, chinL, negateX(chinL));
  }
  
  {// NOTE: Some cheek line
   radius_scale_block(fval(0.3826f));
   line_color_lightness(fval(1.5096f));
   vv(a,fvert(V3(0.2651f, -0.5154f, 0.8715f)));
   vv(b,fvert(V3(0.5636f, -0.3601f, 0.5385f)));
   draw(bez(a, fval(V4(0.f, 0.1801f, 0.0364f, 0.3141f)), b,
            funit(V3(0.866f, 0.f, 0.5f))),
        small_to_big());
   vv(c,fvert(V3(0.5266f, -0.6385f, 0.68f)));
   draw(bez(b, fval(V4()), c,
            funit(V3())),
        big_to_small());
  }
  
  b32 hair_on = fbool(1);
  if (hair_on)
  {//~NOTE: Hair
   set_in_block(painter.line_params.nslice_per_meter, fval(1.5162f)*128.f);
   symx_off;
   radius_scale_block(fval(0.5489f));
   //NOTE: fine tip hair -> thicker, more even radius
   v4 hair_radii = fval4(0.5f, 1.f, 1.f, 0.25f);
   set_in_block(painter.line_params.radii, hair_radii);
   
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
     Group(1.f, fval4(0.5752f, -0.2f, 0.2f, 1.f));
     Group(0.5f, fval4(1.2f, 0.f, 0.f, 1.2f));
     Group(1.f, fval4(0.2176f, -0.4755f, 0.2169f, 0.f));
    }
#undef Group
    end_animation(ani);
   }
   v1 thairL = 0.f;
   v1 thairR = 0.f;
   if ( should_flutter )
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
   Bezier bang_vline = bez(bang_root, fval(V4(0.f, 0.2629f, 0.1602f, 0.3068f)), bang_midpoint,
                           funit(V3(0.f, 0.f, 1.f)));
   draw(bang_vline,profile_visible(fval(0.4036f)));
   
   Bez bang_vlineL = bezd_old(bang_root, 
                              fvert3(0.3419f, 0.0757f, 0.129f),
                              fval2(0.1345f, 0.1639f),
                              bang_tip);
   Bez bang_vlineR = negateX(bang_vlineL);
   
   Bez bang_hlineL = bez_old(bang_midpoint,
                             fvert3(0.1302f, -0.0069f, -0.0297f),
                             fvert3(0.0001f, 0.2158f, 0.056f),
                             bang_tip);
   Bez bang_hlineR = negateX(bang_hlineL);
   
   // NOTE: Animate the hair
   {
    v3 a = fvert3(0.0172f, -0.0113f, -0.0825f);
    v3 b = fvert3(-0.0664f, 0.0812f, 0.0059f);
    v3 c = fvert3(0.0537f, 0.1334f, -0.0695f);
    bang_vlineL[1] += thairL*a;
    bang_vlineL[2] += thairL*b;
    bang_vlineL[3] += thairL*c;
    bang_vlineR[1] += thairR*negateX(a);
    bang_vlineR[2] += thairR*negateX(b);
    bang_vlineR[3] += thairR*negateX(c);
   }
   {
    v3 a = fvert3(0.0000f, -0.1217f, 0.0000f);
    v3 b = fvert3(0.0467f, 0.0385f, 0.0000f);
    bang_hlineL[1] += thairL*b;
    bang_hlineL[2] += thairL*a;
    bang_hlineL[3]  = bang_vlineL[3];
    bang_hlineR[1] += thairR*b;
    bang_hlineR[2] += thairR*negateX(a);
    bang_hlineR[3]  = bang_vlineR[3];
   }
   
   {
    draw(bang_vlineL);
    draw(bang_vlineR);
    v4 bang_hline_radii = fval(0.5176f) * painter.line_params.radii;
    draw(bang_hlineL, bang_hline_radii);
    draw(bang_hlineR, bang_hline_radii);
   }
   
   //-NOTE: The bang saga is over, let's move onto something else
   v3 hair_main_tip = fvert3(0.7814f, -1.6599f, -0.6047f);
   Bez vline = bezd_old(hair_root,
                        fvert3(0.1414f, 0.2764f, -0.471f),
                        fval2(0.3956f, 0.3102f),
                        hair_main_tip);
   { symx_on; draw(vline); }
   
#if 0
   {// NOTE: ;inflection-point-experiment
    Bez line = bezd(fvert3(1.2193f, 1.2078f, -0.0168f),
                    fvert3(0.3117f, 0.2096f, -0.0057f),
                    fval2(0,0),
                    fvert3(1.6441f, -1.6599f, -0.6047f));
    draw(line);
    v3 u = noz(line[3]-line[0]);
    v3 v;
    {
     v3 w = cross(u, line[1]-line[0]);
     v = noz(cross(w,u));
    }
    Bez rebased = line;
    for_i32(index,0,4)
    {
     rebased[index] -= line[0];
    }
    v2 b2[4];
    for_i32(index,0,4)
    {
     b2[index] = V2(dot(rebased[index], u),
                    dot(rebased[index], v));
    }
    if( fbool(0) )
    {
     viz_block;
     Bez line_check = {};
     for_i32(index,0,4)
     {
      line_check[index] = line[0] + b2[index][0]*u + b2[index][1]*v;
     }
     draw(line_check);
    }
    i32 nslices = 32;
    v1 slice = 1.f / v1(nslices);
    v1 circle_radius = 3.f*millimeter;
    for_i32(index,0,nslices+1)
    {
     v1 t = v1(index)*slice;
     v1 sign = signof(bezier_curvature(b2, t));
     argb color = argb_green;
     if (sign >= 1e-6)
     {
      color = argb_red;
     }
     draw_disk(bezier_sample(line,t), circle_radius, color, 0.f, 0);
    }
   }
#endif
   v3 hcontrol = fvert3(0.1291f, -0.0058f, -0.2506f);
   Bez connecting = bez_old(hair_main_tip,
                            hcontrol,
                            negateX(hcontrol),
                            negateX(hair_main_tip));
   draw(connecting, fval(0.5f)*painter.line_params.radii);
   
   Bezier hairline_side = bez_old(bang_root,
                                  fvert3(0.5271f, -0.0051f, 0.0478f),
                                  fvert3(0.0671f, 0.1143f, 0.3924f),
                                  ear_center);
   { symx_on; draw(hairline_side); }
   
   {
    symx_on;
    // NOTE: Some hair outline so it looks good at the front
    Bez over1 = bezd_old(hair_root,
                         fvert3(0.5313f, 0.0177f, 0.1293f),
                         fval2(0.1839f, 0.232f),
                         bezier_sample(hairline_side, fval(0.3991f)));
    Bez over2 = bezd_old(hair_root,
                         fvert3(0.3694f, 0.2549f, -0.0512f),
                         fval2(0.0175f, 0.2904f),
                         ear_center);
    draw(over1); 
    draw(over2, lp_alignment_threshold(fval(0.4764f)));
   }
   {
    Bez line = bez(hair_root, fval(V4(0.f, 0.2255f, 0.3279f, 0.2047f)), bang_root,
                   funit(V3(0.f, 1.f, 0.f)));
    draw(line);
   }
  }
  
  if (show_grid || show_loomis_ball)
  {//~NOTE: Proportions diagram
   viz_block_color(argb_dark_blue);
   symx_off;
   
   if (show_grid)
   {
    b32 camera_frontal=almost_equal(absolute(painter.camera->z.z), 1.f, 1e-2f);
    b32 camera_profile=almost_equal(absolute(painter.camera->z.x), 1.f, 1e-2f);
    
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
        draw_line( V3(x, 1.f, z), V3(x, face_minY, z));
       }
      }
      {
       v1 xmax = fval(0.9369f);
       for_i32(yi, yi_min, yi_max)
       {//note: Horizontal
        v1 y = unit*v1(yi);
        draw_line( V3(0.f, y, z), V3(xmax, y, z));
       }
      }
     }
     
     if (camera_profile)
     {// NOTE: Profile view
      v1 x = 0.f;
      for_i32(zi, fval(-11), (2))
      {//NOTE: Vertical
       v1 z = faceZ+unit*v1(zi);
       draw_line( V3(x,1.f,z), V3(x,face_minY,z));
      }
      for_i32(yi, yi_min, yi_max)
      {//NOTE: Horizontal
       v1 y = unit*v1(yi);
       draw_line( V3(x,y,fval(-1.8208f)),
                 V3(x,y,fval(1.7172f)));
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
       {//NOTE: Vertical
        v1 x = unit*v1(xi);
        draw_line(V3(x, 1.f, 0.f), V3(x, body_minY, 0.f));
       }
      }
      {
       v1 xmax = fval(2.8346f);
       for_i32(yi, (-20), (4))
       {//NOTE: Horizontal
        v1 y = unit*v1(yi);
        draw_line(V3(0.f, y, 0.f), V3(xmax, y, 0.f));
       }
      }
      
      {//NOTE: Where the shoulder should be
       Line_Params params = painter.line_params;
       params.color = argb_blue;
       v1 y = fval(-2.2334f);
       draw_line( V3(0.f, y, 0.f), V3(0.7f*head_unit, y, 0.f), params);
      }
     }
     
     if (camera_profile)
     {// NOTE: Profile view
      v1 x = 0.f;
      for_i32(xi, (-4), (2))
      {//NOTE: Vertical
       v1 z = faceZ+unit*v1(xi);
       draw_line( V3(x,1.f,z), V3(x,body_minY,z));
      }
      for_i32(yi, -20, 4)
      {//NOTE: Horizontal
       v1 y = unit*v1(yi);
       draw_line( V3(x,y,fval(-1.8208f)),
                 V3(x,y,fval(1.7172f)));
      }
     }
    }
    
    if (fbool(0))
    {//NOTE: Head unit body proportions markers
     set_in_block(painter.line_params.color, argb_red);
     if (camera_frontal)
     {//NOTE
      v1 r = 0.5f*head_unit;
      for_i32(yi,-6,1)
      {
       v1 y = 1.f + v1(yi)*head_unit;
       v3 left  = V3(r,y,0);
       draw_line( V3y(y), left);
      }
     }
     if (camera_profile)
     {//NOTE
      for_i32(yi,-6,1)
      {
       v1 y = 1.f + v1(yi)*head_unit;
       draw_line( V3y(y), V3(0,y,-1.f));
      }
     }
    }
   }
   
   if (show_loomis_ball)
   {//NOTE: The face
    argb line_color = painter.line_params.color;
    if (show_grid) { line_color = argb_silver; }
    set_in_block(painter.line_params.color, line_color);
    
    {// NOTE: Loomis The side circle
     mat4 tform = (mat4_translate(loomis_side_center) *
                   mat4_scale(loomis_side_radius) *
                   rotateY(0.25f));
     draw_bezier_circle(tform);
    }
    {
     // NOTE: Face z
     draw(bez(V3z(faceZ), setz(chin_middle, faceZ)));
     draw_bezier_circle( mat4_identity);  // NOTE: The Loomis "unit circle" (i.e the first circle you draw)
     {
      mat4 equator = rotateX(0.25f);
      draw_bezier_circle( equator);
     }
     {
      mat4 longitude = rotateY(0.25f);
      draw_bezier_circle( longitude);
     }
    } 
   }
  }
  
  {//~NOTE: Export head
   //TODO: Maybe analytically define this point on the skull?
   vv(trapezius_head, (V3(0, noseY, head_back_out.z) + 
                       fvert(V3(0.2824f, 0.0086f, -0.323f))));
   
   v3 behind_earR     = negateX(behind_ear);
   v3 trapezius_headR = negateX(trapezius_head);
   
#define head_export(vertex)  head_world.vertex = mat4vert(ot, vertex);
   macro_head(head_export)
#undef head_export
  }
 }
 
 b32 show_body = fbool(1);  //;show_body  NOTE: Her face somehow looks a lot worse when the body is on?
 
 Pelvis pelvis_obj;
 mat4i pelvisT;
 {//~ BOOKMARK: It's pelvis time!
  set_in_block(painter.painting_disabled,
               painter.painting_disabled || !show_body);
  symx_on;
  mat4i &ot = pelvisT;
  {
   v3 translate = V3(0.f, 
                     head_top_static - 3.2f * head_unit_world, 
                     fval(0.067f));
   ot = (mat4i_translate(translate) * mat4i_scale(head_scale));
  }
  transform_block(ot);
  
  v1 navelY = (ot.inv * V3y(head_top_static - fval(2.5f) * head_unit_world)).y;
  vv0(navel, V3y(navelY) + fvert(V3(0.f, 0, 0.271f), clampy));
  
  vv(crotch, V3());  // NOTE: Yes, the crotch front is the origin, what about it?
  vv(crotchL, V3x(fval(0.1f)));
  draw_line(crotch,crotchL);
  {//- lower
   vv(bikiniL, fvert(V3(1.0685f, 0.8105f, -0.6117f)));
   v3 bikini_dir = funit(V3(-0.1823f, -0.0005f, 0.9832f));
   Bez bikini_front = bez(crotchL, fval2(-0.2081f, 0.2062f), 
                          fval2(0.316f, 0.3309f), bikiniL,
                          bikini_dir);
   draw(bikini_front);
   Bez bikini_back = bez(crotchL, fval2(-0.2767f, 0.2024f),
                         fval2(0.213f, 0.2503f), bikiniL,
                         -bikini_dir);
   draw(bikini_back);
  }
  v3 bikini_up_back;
  {//-Upper
   //NOTE: This is from David Finch's "how to draw female torso", which he took from Loomis
   vv(bikini_front_mid, fvert(V3(0.f, 0.951f, 0.1677f), clampx));
   vv(girdle_front, bikini_front_mid + fvert(V3(0.8075f, 0.2603f, -0.0717f)));
   va(bikini_up_back, fvert(V3(0.5102f, 1.3265f, -0.7766f)));
   Bez girdle_side_line = bez(girdle_front, 
                              fval4(-0.3554f, 0.3781f, 0.2608f, 0.2404f),
                              bikini_up_back,
                              funit(V3(0.7543f, 0.6022f, 0.2614f)));
   draw(girdle_side_line, fci(I4(0,6,6,1)));
   draw(bezd_old(bikini_front_mid, fvert(V3(-0.0581f, -0.1036f, 0.f)),
                 fval(V2(-0.0136f, 0.1226f)), girdle_front));
   {
    fill3(bikini_front_mid, crotch, girdle_front);
    fill3(girdle_front, crotch, bikini_up_back);
    fill_bez(girdle_side_line);
   }
  }
  
#define export_(vertex)  pelvis_obj.vertex = vertex;
  macro_pelvis(export_);
#undef export_
 }
 
 v1 arm_ry = fval(0.5302f)*(head_unit_world);
 mat4i armLT, forearmLT;
 
#define macro_torso(x) \
x(shoulder)   \
x(delt_collar) \
x(scap_delt)    \
x(pectoral_torso) \
x(latis_arm)     \
x(pectoral_arm) \
x(scap_sock_bot) \
//
 macro_torso(macro_world_declare);
 mat4i torsoT = mat4i_scale(head_scale*fval(1.2484f) );
 for_i32(left_right_index, 0, 2)
 {//~NOTE: torso
  set_in_block(painter.painting_disabled, 
               painter.painting_disabled || !show_body);
  b32 is_left  = (left_right_index == 0);
  b32 is_right = !is_left;
  
  mat4i ot = torsoT;
  if (is_right)
  {
   mat4i neg = mat4i_scales(-1,1,1);
   ot = torsoT * neg;
  }
  object_block(ot, fvert(V3(0.f, -2.3176f, 0.3862f)));
  
  v1 head_unit = get_column(ot.inv,1).y * head_unit_world;
  
  //~ NOTE: Imports
  Head head;
  for_i32(index, 0, head_vert_count) {
   head.verts[index] = mat4vert(ot.inv, head_world.verts[index]);
  }
  Pelvis pelvis;
  {
   mat4 transform = ot.inv * pelvisT;
   for_i32(index, 0, pelvis_vert_count) {
    v3 vertex = pelvis_obj.verts[index];
    if (is_right) { vertex = negateX(vertex); }
    pelvis.verts[index] = mat4vert(transform, vertex);
   }
  }
  
  v1 shoulderY = head.chin_middle.y-0.2f*head_unit; // NOTE: from hpc
  v1 shoulderX = fval(0.53f, f20th)*head_unit;
  // NOTE: Let's decide that this is part of the scapula,
  // and NOT the clavicle (which is a bit to the inner side)
  vv(shoulder, V3(shoulderX, shoulderY, 0) + fval(V3z(0.0573f)));
  if(1)
  {// NOTE: Check shoulder y
   v1 chin_to_shoulder = (head.chin_middle - shoulder).y;
   WARN_DELTA(chin_to_shoulder, 0.2f*head_unit, 2.f*centimeter);
  }
  
  v3 scap_sock_top = fvert(V3(0.916f, -1.611f, -0.0102f));  //NOTE: for @armT
  
  mat4i armT;
  {//NOTE: ;armT Calculate upper arm transform, so we can draw attachments to it
   // NOTE: transform is done in world space, because it's too mind-bending
   v3 scap_sock_top_world = ot * scap_sock_top;
   v3 translate = (addy(scap_sock_top_world, -arm_ry) +
                   head_unit_world * fvert3(0.0345f, 0.0065f, 0.1425f));
   mat4i rotateT = mat4i_rotate_tpr(0, 0, fval(-0.1008f, f20th), scap_sock_top_world);
   armT =  rotateT * mat4i_translate(translate) * mat4i_scale(arm_ry);
   if (is_left) { armLT = armT; }
  }
  
  // NOTE: "arm_local" is the arm on the side of the torso being drawn (left/right)
  // because: you wouldn't ever wanna connect the torso to the arm on a different side.
  mat4 arm_local = ot.inv * armT;
  
  vv(shoulder_in, fvert3(0.4819f, -1.3302f, -0.4461f));
  
  v3 sternum = fvert3(0.f, -1.5356f, 0.2691f, clampx);
  vv(delt_collar, shoulder + fvert3(-0.3387f, -0.02f, 0.1371f));
  vv(sternumL, sternum + V3x(fval(0.0919f)));
  {// NOTE: Neck
   Bezier neck_front_vline = bezd_old(head.head_neck_junction,
                                      fvert(V3(-0.f, 0.f, -0.0968f)), 
                                      fval2(0.f, 0.2897f), sternum);
   draw( neck_front_vline, profile_visible(0.5f));
   
   Bezier neck_back_line = bez_old(is_left ? head.trapezius_head : head.trapezius_headR,
                                   fvert3(-0.1529f, -0.1932f, 0.2857f),
                                   fvert3(-0.1937f, 0.0701f, 0.0969f),
                                   shoulder_in);
   draw(neck_back_line, fci(I4(0,1,3,0)));
   
   if (is_left)
   {
    fill_dbez(neck_back_line, negateX(neck_back_line));
   }
   
   Bezier collar_in = bez_old(sternumL,
                              fvert3(0.2915f, -0.0448f, -0.0087f),
                              fvert3(-0.1047f, -0.0248f, 0.1171f),
                              delt_collar);
   Bezier collar_out = bez_old(delt_collar+fvert(V3(-0.0957f, 0.0021f, 0.016f)),
                               fvert3(0.f, 0.f, 0.f),
                               fvert3(-0.0607f, 0.0941f, 0.0168f),
                               shoulder);
   {
    auto lp = painter.line_params;
    if (level1) { lp = viz_line_params(); }
    draw( bez_old(shoulder_in,
                  fvert3(0.0993f, -0.0342f, -0.f),
                  fvert3(-0.0356f, 0.0675f, -0.0634f),
                  shoulder),
         fval(I4(5,3,3,1)));
    draw(collar_out, lp);
    draw(collar_in, lp);
   }
   
   //~TODO: Weird code alert!!!
   {
    Bezier sterno;
    {
     sterno = bez_old((is_left ? head.behind_ear : head.behind_earR),
                      fvert3(-0.107f, -0.2444f, 0.2159f),
                      fvert3(0.0786f, 0.1842f, -0.1674f),
                      sternumL);
     {// NOTE: sterno contracting animation
      // It's just a lerp between rest pose and turned pose, that's all folks
      v3 patch = fvec(V3(-0.1658f, 0.f, -0.0888f));
      v1 tcontract = is_left ? clamp01(-thead_theta) : clamp01(thead_theta);
      sterno[1] += tcontract * patch;
     }
    }
    
    //TODO: the sterno is bugged when looked from behind ;sterno_drawing
    draw(sterno, fci(fval4i(2,0,0,3)));
    
    fill_dbez(neck_back_line, sterno);
    fill_dbez(neck_front_vline, sterno);
   }
  }
  
  vv(scap_delt, shoulder_in + fvert(V3(-0.1213f, -0.3863f, -0.0611f)));
  
  v3 rib_mid, rib_back;
  {// NOTE: Rib cage
   // NOTE rib_midY is lower than half shoulder to navel
   v1 rib_midY = (shoulder.y - 0.65f*head_unit + fval(-0.0775f));
   va(rib_mid, V3(0.f, rib_midY, fval(0.8061f)));
   if (is_left)
   {
    draw(bezd_old(sternum,
                  fvert(V3(0.f, 0.0027f, 0.2265f), clampx),
                  fval(V2(0.f, 0.0935f)),
                  rib_mid));
   }
   vv(ribL, rib_mid+fvert(V3(0.559f, -0.7158f, -0.2772f)));
   // NOTE: I guess the rib cage ends where the elbow is
   v3 rib_in = rib_mid+V3x(fval(0.0905f));
   draw( bez(rib_in, fval4(0.f, 0.0689f, 0.0558f, 0.2431f), ribL,
             funit(V3(0.629f, -0.2482f, 0.7367f))),
        fval4i(2,6,3,1));
   
   fill4( rib_mid, ribL, delt_collar, sternum);
   va(rib_back, ribL+fvert(V3(0.0855f, 0.171f, -0.5742f)));
   draw(bez(ribL, fval2(-0.0705f, 0.2667f),
            fval2(0.f, 0.384f), rib_back,
            funit(V3(0.5421f, -0.8353f, 0.0916f))));
   {//-NOTE: David Finch's vest shape
    vv(vestL,ribL+fvert(V3(0.0371f, 0.3022f, 0.0011f)));
    draw( bez(rib_in, fval(V4(0.f, 0.4043f, 0.f, 0.2469f)), vestL,
              funit(V3(0.f, 0.852f, 0.5235f))));
    vv(vest_back, rib_back+fvert(V3(-0.f, 0.5114f, -0.0067f)));//todo: where?
    draw( bez(vestL, fval(V4(-0.0067f, 0.1427f, 0.f, 0.3102f)), vest_back,
              funit(V3(0.9212f, -0.3891f, -0.f))));
   }
  }
  
  vv(latis_arm, arm_local*fvert3(-0.2595f, 0.263f, -0.0002f));
  vv(pectoral_arm, arm_local*fvert(V3(-0.2013f, 0.475f, 0.1047f)));
  {// NOTE: connection to the arm
   Bez latis_arm_line = bez(rib_back, fval2(0.1935f, 0.0429f), 
                            fval2(0.151f, 0.0235f), latis_arm,
                            funit(V3(1.f, 0.f, -0.f)));
   draw(latis_arm_line, lp_alignment_threshold(0.7f));
  }
  
  v3 scap_sock_bot;
  {//NOTE: The back
   vv(trap_bot, fvert(V3(0.f, -3.1104f, -0.1081f), clampx));
   if (fbool(0))
   {
    draw(bezd_old(scap_delt, fvert(V3(0.0015f, 0.0505f, -0.0748f)),
                  fval2(-0.4018f, 0.0773f), trap_bot));
   }
   if (level2)
   {
    viz_block;
    draw(bez(scap_delt, fval(V4(0.f, 0.1307f, 0.f, 0.0566f)), shoulder,
             funit(V3(0.5328f, 0.0057f, -0.8462f))));
   }
   fill3_symx(scap_delt, shoulder_in);
   fill3(shoulder_in, shoulder, scap_delt);
   
   fill3_symx(trap_bot,scap_delt);
   Bez hip_back_line = bez(rib_back, fval2(0.f, 0.1135f),
                           fval2(0.01f, 0.1345f), pelvis.bikini_up_back,
                           funit(V3(-0.5382f, 0.f, 0.8428f)));
   draw(hip_back_line);
   // NOTE: Back arch
   vv(back_archL, bezier_sample(hip_back_line, fval(0.5606f)));
   draw(bez(trap_bot, fvert(V3(0.235f, 0.3261f, -0.0079f)),
            fvert(V3(0.0805f, 0.f, 0.0552f)), back_archL));
   
   vv(scap_bot, scap_delt+fvert(V3(0.1134f, -0.7434f, 0.1812f)));
   va(scap_sock_bot, scap_sock_top+fvert(V3(0.0217f, -0.1349f, -0.f)));
   if (level2 || preset == 3 || preset == 4)
   {// ;show_scapula
    set_in_block(painter.painting_disabled, false);
    viz_block;
    draw(bez(scap_delt,fval(V4(-0.14f, 0.1607f, 0.4409f, 0.224f)),scap_bot,
             funit(V3(-0.6328f, 0.f, -0.7743f))));
    draw(bez(scap_delt, fval4(0,0,0,0), scap_sock_top,
             funit(V3())));
    draw(bez(scap_bot, fval4(0.f, 0.118f, 0.f, 0.f), scap_sock_bot,
             funit(V3(-0.f, 0.f, -1.f))));
    draw(bez(scap_sock_top, fval4(0,0,0,0), scap_sock_bot,
             funit(V3())));
   }
   
   vv(lower_back, fvert(V3(0.f, -4.2845f, -0.0495f), clampx));
   fill3( lower_back, trap_bot, back_archL);
   
   vv(diamond_up,  V3z(shoulder_in.z)+fvert(V3(0.f, -1.2636f, 0.0193f), clampx));
   Bez back_midline =  bez(diamond_up, fval(V4(-0.0605f, 0.1215f, 0.f, 0.f)), trap_bot,
                           V3z(-1));
   draw( back_midline, fval(V4(0.3513f, 0.9763f, 0.6129f, 1.1402f)));
   
   vv(diamond_low, bezier_sample(back_midline, fval(0.2302f)));
   vv(diamondL,    diamond_up+fvert(V3(0.0856f, -0.2026f, -0.0719f)));
   draw( bez(diamond_up, diamondL));
   draw( bez(diamond_low, diamondL));
   
   
   Bez trap_vline = bez(scap_delt, 
                        fval(V4(0.f, 0.0667f, 0.f, 0.f)), 
                        bezier_sample(back_midline, fval(0.9154f)),
                        funit(V3(-0.379f, 0.f, -0.9254f)));
   draw( trap_vline, fval(V4(0.7542f, 1.f, 1.f, 0.2694f)));
   
   vv(trap_weird_point, bezier_sample(trap_vline, fval(0.5728f)));
   Bez trap_weird_line = bez(trap_weird_point, fval(V4(0.0369f, 0.1268f, 0.f, 0.f)), rib_back,
                             funit(V3(-0.8654f, 0.f, -0.5011f)));
   draw( trap_weird_line, fval(V4(0.75f, 1.f, 1.0735f, 0.5f)));
  }
  
  v3 chest_back, pectoral_torso;
  {//NOTE: Nipple and chest!
   v1 nippleY = shoulder.y-0.4f*head_unit;  //NOTE: from hpc
   vv0(nipple, V3y(nippleY) + fvert(V3(0.5799f, 0.f, 0.6752f), clampy));
   
   vv(chest_in, fvert(V3(0.1045f, -2.5438f, 0.8167f)));
   vv(chest_out, fvert(V3(0.6599f, -2.4822f, 0.6002f)));
   va(pectoral_torso, lerp(chest_out, fval(0.5f), pectoral_arm));
   fill3(delt_collar,pectoral_torso,chest_out);
   draw(bez(chest_in, fval4(-0.0299f, 0.2108f, 0.1371f, 0.253f), chest_out,
            funit(V3(0.0915f, -0.523f, 0.8474f))),
        fval(I4(6,4,3,1)));
   va(chest_back, fvert(V3(0.7613f, -2.0817f, 0.039f)));
  }
  
  {//NOTE: Some latissimus lines that are made-up and probably will have to be redone
   Bez latis_side = bez(chest_back, fval2(-0.1341f, 0.0768f),
                        fval2(0,0), rib_back,
                        funit(V3(0,0,-1)));
   draw(latis_side,lp_alignment_threshold(0.7f));
  }
  
  if (is_left)
  {//NOTE: Abdoment
   draw(bez(rib_mid, fvert(V3(-0.f, 0.f, -0.0952f)), pelvis.navel));
  }
  
  if (is_left)
  {//~NOTE: Exports
   macro_torso(macro_export_vertex);
  }
 }
 
 b32 arm_on = fbool(1) || level2;
 mat4 mat4_bilateral = mat4_translate(V3(-1.f)) * mat4_scale(2.f);
 //
 
 argb side_plane = shade_color;
 Arm arm_obj;
 v3 elbow_offset = fvert(V3(0.0107f, -1.864f, -0.1208f));
 v3 elbow_up_out = elbow_offset+fvec(V3(0.0527f, 0.9446f, -0.2298f));
 {//;upper_arm @armLT
  scale_in_block(default_fvert_delta_scale, 2.f);
  mat4i &ot = armLT;
  object_block(ot, fvert(V3(-0.0802f, 0.f, -0.1673f)));
  //
  macro_torso(macro_import_vertex);
  
  {// NOTE ;forearmLT
   vv0(elbow_center, fvert(V3(-0.0878f, -1.f, -0.458f)));  //NOTE: The rotation pivot
   // IMPORTANT: The ratio forearm/upper_arm=0.8 is measured on the iPad, Loomis female proportions, also gvaat
   // I'm almost certain it's "correct"
   v1 forearm_bend_t = fval(0.1747f);   //;forearm_bend_t
   v1 forearm_turn = lerp(0.f, forearm_bend_t, -0.5f);
   forearmLT = armLT * mat4i_rotateX(forearm_turn, elbow_center);
  }
  
  v3 internal_condyle;
  {//NOTE: The skeleton (humerus and ulna)
   v3 o = fvert(V3(0.0501f, 0.f, -0.1803f));
   v3 o2 = o+fvert3(-0.f, 0.f, -0.1157f);
   va(internal_condyle, o2+fvert(V3(-0.2963f, -0.9241f, -0.0402f)));
   if ( level2 || preset == 3 || preset == 4 )
   {
    viz_block;
    {//NOTE: internal
     vv(a, o2+fvert(V3(-0.1693f, -0.7344f, -0.0402f)));
     draw( bez(o+fvert(V3(-0.1711f, 0.7636f, -0.052f)), a),
          V4(0.5f));
     draw( bez(a, fval(V4(0.f, 0.4207f, 0.f, 0.f)), internal_condyle,
               funit(V3(0.5647f, -0.8253f, 0.f))),
          0.5f*big_to_small());
    }
    {//NOTE: external
     vv(a, o2+fvert(V3(-0.032f, -0.7874f, -0.0402f)));
     vv(external_condyle, o2+fvert(V3(0.0625f, -0.9168f, 0.f)));
     draw( bez(o+fvert(V3(-0.0435f, 0.8466f, -0.0052f)),
               a),
          V4(0.5f));
     draw(bez(a,fval(V4(0.f, 1.0357f, 0.f, 0.f)),external_condyle,
              funit(V3(-0.1623f, -0.9867f, 0.f))),
          0.5f*big_to_small());
    }
   }
  }
  
  vv(delt_bot, fvert(V3(0.0527f, 0.1499f, -0.2028f)));
  v3 delt_back;
  Bez delt_cross, delt_back_line, delt_bot_line, delt_vfront;
  v3 delt_back_point = scap_delt;
  {//-Deltoid
   vv(delt_top, shoulder+fvert(V3(0.0328f, 0.f, 0.f)));
   vv(delt_top_back, delt_top+fvert(V3(-0.0109f, -0.0068f, -0.2147f)));
   Bez delt_hline = bez(delt_top,
                        fval(V4(-0.059f, 0.584f, 0.f, 0.f)),
                        delt_top_back +fvert(V3()),
                        funit(V3(0.f, 1.f, 0.f)));
   draw( delt_hline, fci(fval4i(0,3,2,0)));
   fill_bez( delt_hline);
   
   delt_vfront = bez(delt_top, fval(V4(-0.3452f, 0.3717f, 0.0022f, 0.1226f)), delt_bot,
                     funit(V3(0.0087f, 0.f, 1.f)));
   draw(delt_vfront);
   Bez delt_vmid = bez(delt_top, fval(V4(-0.2234f, 0.2079f, 0.015f, 0.1626f)), delt_bot,
                       funit(V3(0.9999f, 0.f, 0.0112f)));
   Bez delt_vback = bez(delt_top_back, fval(V4(-0.13f, 0.1784f, -0.1368f, 0.2863f)), delt_bot,
                        funit(V3(0.6431f, 0.f, -0.7657f)));
   va(delt_back, fvert(V3(-0.0076f, 0.2268f, -0.4244f)));
   fill3(delt_back, delt_bot, delt_top_back);
   
   draw(delt_vmid, lp_alignment_threshold(fval(0.8202f)));
   //if (level1)
   {
    Line_Params params = lp_alignment_threshold(fval(0.6061f));
    params.radii = fval(V4(0.712f, 0.f, 0.f, 0.5464f));
    draw(delt_vback, params);
    fill_bez(delt_vback);
   }
   delt_bot_line = bezd_old(delt_bot, fvert(V3()),
                            fval2(0,0), delt_back);
   draw(delt_bot_line, fci(fval4i(4,0,0,4)));
   //NOTE: fill the side
   fill_dbez( delt_vfront, delt_vmid);
   fill_dbez( delt_vmid, delt_vback);
   draw(bez(delt_top_back, fval(V4()), scap_delt,
            funit(V3())));
   
   delt_back_line = bez(delt_back,
                        fval(V4(-0.1729f, -0.1804f, 0.1064f, 0.2869f)), 
                        delt_back_point,
                        funit(V3(0.f, -1.f, 0.004f)));
   draw( delt_back_line, fval4i(1,4,4,1));//TODO: Wow this line is terrible
   //note: fill the back side of the deltoid
   fill4( delt_top_back, delt_back, delt_back_point, scap_delt);
   
   delt_cross = bezd_len(delt_collar, 
                         fvert3(0.0677f, 0.1554f, 0.2652f),
                         fval2(0.f, 0.2143f),
                         delt_bot);
   draw(delt_cross, fval4i(1,2,5,4));
   
   // NOTE: @Poly Fill in the effing arm-torso connections!!!
   fill(delt_top,delt_cross);
   fill_bez( delt_cross);
   
   {
    // Fill in the front
    fill( bezier_sample(delt_cross, fval(0.5f)), delt_vfront);
   }
  }
  {
   Bez line = bez(pectoral_arm, fval(V4(0.f, 0.3866f, 0.f, 0.f)), pectoral_torso,
                  funit(V3(-0.997f, 0.0775f, 0.f)));
   draw(line);
   fill(delt_collar, line);
  }
  
  v3 bicep_in_bot, bicep_in_top, bicep_out_top, bicep_out_bot, white_in, white_out, tricep_mid;
  vv(tricep_wedge, delt_bot+fvert(V3(-0.f, -0.1879f, -0.0035f)));
  
  vv(brachio_a, fvert(V3(0.1031f, -0.7618f, -0.0498f)));
  {//NOTE: The 'ceps
   {//NOTE: tricep
    vv(tricep_up, bezier_sample(delt_back_line, fval(0.2993f)));
    va(tricep_mid, fvert(V3(-0.1105f, -0.1135f, -0.475f)));
    //NOTE: The white part
    va(white_out, tricep_mid+fvert(V3(0.0446f, -0.1341f, 0.0966f)));
    va(white_in, tricep_mid+fvert(V3(-0.1177f, -0.3581f, 0.007f)));
    {
     draw(bezd_len(delt_back, fvert(V3(-0.f, 0.f, -0.0662f)),
                   fval2(0,0), tricep_mid),
          fci(fval4i(5,0,0,0)));
     draw(bez(tricep_mid, white_in), fci(fval4i(3,0,0,0)));
     draw(bez(tricep_mid, white_out), fci(fval4i(1,1,1,1)));
    }
    vv(tricep_out_top, bezier_sample(delt_bot_line, fval(.3f)));
    if (level1) { draw(bez(tricep_out_top, tricep_wedge)); }
    fill4(tricep_wedge, tricep_mid, delt_back, delt_bot);
    fill3(white_out, tricep_mid, tricep_wedge);
    draw(bez(scap_sock_bot, fvert(V3(-0.2354f, 0.048f, 0.f)),
             fval2(0.2603f, -0.0407f), white_in),
         lp(fval4i(0,0,8,0)));
   }
   
   {//NOTE: bicep
    va(bicep_out_top, bezier_sample(delt_cross, fval(0.8389f)));
    va(bicep_out_bot, fvert(V3(0.0079f, -0.9224f, -0.027f)));
    //NOTE: Outer line
    Bez front_out = bez(bicep_out_top,fval(V4(-0.0571f, 0.0667f, 0.f, 0.f)),bicep_out_bot,
                        funit(V3(0.7886f, 0.f, 0.6149f)));
    {
     Line_Params params = painter.line_params;
     if (!level1) {
      params = lp_alignment_threshold(fval(0.955f));
     }
     params.radii = fci(fval4i(5,2,3,2));
     if( level1 ) { draw(front_out, params); }
    }
    vv(hinge_out, bezier_sample(front_out, fval(0.6758f)));
    
    va(bicep_in_top, bicep_out_top+fvert(V3(-0.211f, 0.0922f, 0.0631f)));
    va(bicep_in_bot, bicep_out_bot+fvert(V3(-0.148f, 0.0022f, 0.0005f)));
    fill3(brachio_a, hinge_out, bicep_out_bot);
    //NOTE: Inner line (TODO: this line sucks but there's nothing I can do to tweak it into shape)
    Bez front_in = bez(bicep_in_top,fval(V4(0.f, 0.0397f, 0.0201f, 0.1254f)),bicep_in_bot,
                       funit(V3(-0.8434f, 0.f, 0.5373f)));
    {
     auto lp = lp_alignment_threshold(fval(0.6693f));
     lp.radii = fci(fval4i(0,0,0,4));
     draw(front_in, lp);
    }
    fill_dbez(front_in, front_out);
    vv(bicep_side_top, bezier_sample(delt_vfront, fval(0.9539f)));
    vv(bicep_side_bot, bicep_out_bot + fvert(V3(0.1103f, 0.1089f, -0.084f)));
    Bez side_out = bezd_len(bicep_side_top, fvert(V3(-0.f, 0.f, -0.0502f)),
                            fval2(0,0), bicep_side_bot);
    draw(side_out, fci(fval4i(4,0,0,0)));
    fill3(pectoral_arm, bicep_in_top, bicep_out_top);
    fill3(pectoral_arm, delt_collar, bicep_out_top);
    
    vv(bicep_side_top2, bezier_sample(delt_cross, fval(0.7406f)));
    vv(bicep_side_bot2, bicep_out_bot+fvert(V3(-0.0601f, 0.f, 0.f)));
    {
     v1 alignment = fval(0.9169f);
     if (get_view_vector().z < 0.f) { alignment = 0; }
     Bez bicep_side_line2 = bez(bicep_side_top2,
                                fval2(0.f, 0.0537f),
                                fval2(0,0),
                                bicep_side_bot2,
                                V3z(1.f));
     draw(bicep_side_line2, lp(alignment, fval4i(4,3,1,0)));
     fill_bez(bicep_side_line2);
    }
    
    fill4(bicep_side_top, bicep_side_top2, bicep_side_bot2, bicep_out_bot);
    
    vv(bicep_side_top3, bezier_sample(delt_cross, fval(0.7406f)));
    {
     b32 ok = get_view_vector().x < fval(0.07f);
     i4 radii = fval4i(6,6,0,0);
     if (get_view_vector().x > fval(-0.20f) ) {
      radii.y = fvali(0);
     }
     if (ok)
     {
      draw(bez(bicep_side_top3,
               fvec(V3(0.0184f, 0.f, 0.0925f)),
               fval2(0,0),
               brachio_a),
           lp(radii));
      
     }
    }
   }
  }
  
  draw(bez(brachio_a, bicep_out_bot), fci(fval4i(3,3,3,1)));
  
  vv(brachio_humerus, fvert(V3(0.0464f, -0.2894f, -0.2469f)));
  
  {// NOTE: Patching
   // TODO: wow this patch sucks, big time
   fill4(brachio_humerus, delt_bot, bicep_out_top, bicep_out_bot);
  }
  
  Bez tricep_cross_line = bez(tricep_wedge, fvert(V3(0.0474f, -0.0292f, 0.0884f)),
                              fval2(0,0), elbow_up_out);
  draw(tricep_cross_line, fci(fval4i(0,0,4,6)));
  fill_bez(tricep_cross_line);
  vv(brachio_humerus2, bezier_sample(tricep_cross_line, fval(0.4296f)));
  
#if 0
  draw(bez(delt_bot, fvert(V3(0.1505f, 0.1454f, -0.f)),
           fval2(0,0), elbow_up_out));
#endif
  
  {//~NOTE: Exports
#define export_(vertex)  arm_obj.vertex = vertex;
   macro_arm(export_);
#undef export_
  }
 }
 
 Forearm forearm_obj = render_lower_arm(forearmLT, armLT, arm_obj,
                                        elbow_offset, elbow_up_out); 
 mat4i handLT = forearmLT;  //@Temp
 render_hand(handLT, forearmLT, forearm_obj);
}

render_movie_return
render_movie(render_movie_params)
{
#ifndef KV_NO_RENDER
 kv_assert(viewport_id <= GAME_VIEWPORT_COUNT);
 i32 viewport_index = viewport_id - 1;
 b32 is_main_viewport = (viewport_id == 1);
 Viewport *viewport = &state->viewports[viewport_index];
 Arena *arena = &viewport->frame_arena;
 push_struct(arena, i32);  // NOTE: arena can't handle emptyness lol
 
 i32 scale_down_pow2 = fval(0); // ;scale_down_slider
 macro_clamp_min(scale_down_pow2, 0);
 v1 render_scale = 1.f;
 for_i32 (it,0,scale_down_pow2) { render_scale *= 0.5f; }
 v1 default_meter_to_pixel = 4050.6329f;
 v1 meter_to_pixel = default_meter_to_pixel * render_scale;
 
 b32 mouse_inside_view = false;
 v2 mouse_viewp;
 {// NOTE: @Ugh Compute the mouse position in view space
  rect2 clip_box = draw_get_clip(render_target);
  v2 clip_dim = rect2_dim(clip_box);
  v2 mousep = V2(mouse.p);
  v2 mouse_viewp_px   = mousep;
  mouse_viewp_px -= clip_box.min + 0.5f * clip_dim;
  mouse_viewp_px.y *= -1.0f;
  mouse_viewp  = mouse_viewp_px / meter_to_pixel;
  
  rect2 rect = rect2_center_dim(V2(), clip_dim);
  if ( contains(rect, mouse_viewp_px) )
  {
   mouse_inside_view = true;
  }
 }
 
 i32 viz_level = 0;
 switch(viewport->preset){
  case 1: viz_level = 1; break;
  case 2: viz_level = 2; break;
 }
 // if( mouse_inside_view ){ macro_clamp_min(viz_level,1) ;}
 
 
 Camera camera_value;
 Camera *camera = &camera_value;
 Game_Save *save = &state->save;
 {// NOTE: camera
  setup_camera(camera, &viewport->camera);
 }
 b32 camera_frontal=almost_equal(absolute(camera->z.z), 1.f, 1e-2f);
 b32 camera_profile=almost_equal(absolute(camera->z.x), 1.f, 1e-2f);
 
 Render_Config old_render_config;
 if (Render_Config *old_render_config_pointer = target_last_config(render_target))
 {
  old_render_config = *old_render_config_pointer;
 }
 else { old_render_config = {}; }
 
 mat4 camera_transform;
 v3 background_hsv = fval3(0.069f, 0.0648f, 0.5466f);
 v4 background_v4 = srgb_to_linear(hsv_to_srgb(background_hsv));
 argb background_color = argb_pack(background_v4);
 
 b32 show_grid        = fbool(0);
 switch( viewport->preset )
 {
  case 1: { show_grid = fbool(0); }break;
  case 2: { show_grid = fbool(0); }break;
  case 3: { show_grid = fbool(1); }break;
  case 4: { show_grid = fbool(1); }break;
 }
 if (fbool(0)) { if (level1) { show_grid = true; }; }
 
 mat4 view_transform;
 {
  b32 orthographic = show_grid && (camera_frontal || camera_profile);
  if (fbool(0)){orthographic = show_grid;}
  view_transform = camera_view_matrix(camera, orthographic);
 }
 
 {
  Render_Config config = {};
  {
   config.viewport_id = viewport_id;
   // NOTE: Don't set the clip box here since we don't change it.
   config.y_is_up          = true;
   config.background       = background_color;
   config.view_transform   = view_transform;
   config.camera_transform = camera->transform;
   config.meter_to_pixel   = meter_to_pixel;
   config.focal_length     = camera->focal_length;
   config.near_clip        = camera->near_clip;
   config.far_clip         = camera->far_clip;
   config.scale_down_pow2  = scale_down_pow2;
  };
  void *pointer = (void*)DEBUG_send_entry;
  draw_configure(render_target, &config);
 }
 
 //~ IMPORTANT: Please don't draw anything before this point! Because the color space are different.
 
 v1 default_line_radius_unit = 7.f / default_meter_to_pixel;  //NOTE: Changes size when we resize the image
 u64 start_cycle = ad_rdtsc();
 draw_cycle_counter   = 0;
 bs_cycle_counter     = 0;
 //f64 start_time  = gb_time_now();
 
 argb default_line_color = argb_gray(srgb_to_linear1(fval(0.3389f)));
 if (level1) {default_line_color = argb_dark_blue;} // because otw blends in too much with the shading
 argb default_fill = background_color;
 if (level1) {default_fill = argb_lightness(default_fill, fval(0.7797f));}
 
 v1 default_line_radius_min = fval(0.5089f);
 v1 default_line_end_radius = default_line_radius_min;
 if (fbool(1)) {
  default_line_end_radius = i2f6(fval(2));
 }
 // ;init_painter
 painter = Painter {
  .target            = render_target,
  .viewport          = viewport,
  .mouse_viewp       = mouse_viewp,
  .camera            = camera,
  .view_transform    = view_transform,
  .meter_to_pixel    = meter_to_pixel,
  .symx              = false,
  .fill_color        = default_fill,
  .fill_depth_offset = millimeter * 1.f,
  .line_radius_unit  = default_line_radius_unit,
  .line_params       =
  {
   .radii      = V4(default_line_radius_min, 
                    1.f,
                    i2f6(fval(5)),
                    default_line_end_radius),
   .color      = default_line_color,
   .visibility = 1.0f,
  },
  .is_viz            = level1,
  .viz_level         = viz_level,
  .disable_radii     = level1,
 };
 painter.transform_stack[painter.transform_count++] = mat4i_identity;
 painter.view_vector_stack[painter.view_vector_count++] = v3{};
 painter.painting_disabled = fbool(0);
 
 render_character(arena, state->time, viewport_index, show_grid);
 
 
 i32 preset = get_preset();
 if ( preset == 3 || preset == 4 || preset == 5 )
 {// NOTE: The @ReferenceImage experience
  push_object_transform( mat4i_scale(head_scale) );
  if ( camera_is_right() )
  {
   v3 center = fvert(V3(-0.2112f, -4.5306f, -0.2689f));
   v1 width = fval(0.7966f);
   char *filename = "G:/My Drive/Art/arm medial.jpg";
   draw_image(filename, center, V3z(width), V3y(1.f),
              fval(0.5f));
  }
  if ( camera_is_front() )
  {
   if (preset == 3)
   {
    v3 center = fvert(V3(2.4019f, -2.691f, -0.2689f));
    v1 width = fval(2.4645f);
    char *filename = "G:/My Drive/Art/arm front.PNG";
    draw_image(filename, center, V3x(width), V3y(1.f),
               fval(0.3033f));
   }
  }
  if ( camera_is_back() )
  {// NOTE ;back_reference @armT
   if (preset == 3)
   {
    v3 center = fvert(V3(2.8062f, -4.0188f, -0.2689f));
    v1 width = fval(2.4645f);
    char *filename = "G:/My Drive/Art/left arm back.PNG";
    draw_image(filename, center, V3x(-width), V3y(1.f),
               fval(0.3033f));
   }
   else if (preset == 4)
   {
    v3 center = fvert(V3(2.1904f, -3.8928f, -0.2689f));
    v1 width = fval(2.4645f);
    char *filename = "G:/My Drive/Art/arm back bone.PNG";
    draw_image(filename, center, V3x(-width), V3y(1.f),
               fval(0.3033f));
   }
  }
  if ( camera_is_left() )
  {
   v3 center = fvert(V3(-0.2112f, -3.9204f, 0.5898f));
   v1 width = fval(1.1454f);
   char *filename = "G:/My Drive/Art/arm left.PNG";
   draw_image(filename, center, V3z(-width), V3y(1.f),
              fval(0.5f));
  }
  
  if (preset == 5)
  {
   v3 center = fvert(V3(-0.097f, -2.3941f, 0.704f));
   v1 width = fval(3.4275f);
   char *filename = "G:/My Drive/Art/arm weird angle check.PNG";
   draw_image(filename, center,
              width * funit(V3(0.7072f, 0.f, 0.707f)),
              V3y(1.f), fval(0.3916f));
  }
  
   
  pop_object_transform();
 }
 
 if (debug_frame_time_on)
 {//@movie_update
  //f64 end_time  = gb_time_now();
  //f32 time_taken = f32(end_time - start_time);
  //DEBUG_VALUE( time_taken );
  u64 end_cycle = ad_rdtsc();
  u32 total_cycles = u32(end_cycle - start_cycle);
  DEBUG_VALUE(total_cycles);
  DEBUG_VALUE(draw_cycle_counter);
  DEBUG_VALUE(u32(bs_cycle_counter));
 }
 
 //~ IMPORTANT: Drawing end: no trespass
 draw_configure(render_target, &old_render_config);
#endif
}

#undef indicate_level
#undef indicate
#undef indicate0
#undef vv
#undef va

//~EOF
