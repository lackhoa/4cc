//NOTE Generated at C:\Users\vodan\4ed\code/meta_klang.cpp:254:
// NOTE: source: C:\Users\vodan\4ed\code\game\driver.kc
#pragma once
//@generates driver.gen.cpp
//-NOTE(kv): The (deprecating) "script"
//IMPORTANT(kv): DO NOT delete until you've resolved all TODOs
//#define AD_IS_DRIVER 1
//#define AD_IS_GAME 1
//#define ED_API_USER 1

// Select animation: @set_movie_shot
//-
#define cache
#define cached
//-
global const v1 head_radius_world = 9.2f * centimeter;  // @Tweak I have no idea where I pulled this number from
global const v1 head_unit_world   = head_radius_world*(1.f+square_root(2));
global const v1 root2 = square_root(2.f);
global const v1 inv_root2 = 1.f / root2;
global const v1 loomis_unit = inv_root2;// NOTE: From brow to nose tip and all that jazz
global const v1 faceZ = 1.f;
//-
global v3 scap_sock_top;
global v3 palm_in;
global v3 thumb_kbot;
global Bez knuckle_line;
global v4  knuckle_ts;
global v3  knuckle_tops[4];
global v3 kline_in;
global v3 kline_out;
global v3 finger_vecs[4];
// NOTE: "arm_rotation_pivot" is in arm space
global v3 arm_rotation_pivot;
// NOTE: "forearm_rotation_pivot" is in forearm space 
global v3 forearm_rotation_pivot;
//-TODO(kv) Gotta put these in a struct
global v3 nose_rootL;
global v3 nose_wing;
global v3 brow_out;
global Bez brow_ridge;
global v3 cheek_low;
global v3 nose_root_backL;
global v3 cheek_up;
//-
global v3 eyeball_center;
global v1 eyeball_radius;
//-
xfunction void
driver_update(Viewport *viewports){
debug_frame_time_on = fbool(0);
bezier_poly_nslice  = fval(16);
}
function shot_function_return
movie_shot_blinking(shot_function_params){
aset(teye_theta, 3);
arest(8);
aset(teye_theta, -2);
arest(1);
aset(teye_theta, -3);
arest(4);
aset(tblink, 1);
arest(1);
aset(tblink, 6);
arest(1);
aset(tblink, 6);
arest(1);
aset(tblink, 1);
aset(teye_theta, 0);
arest(1);
aset(tblink, 0);
arest(12);
}
function shot_function_return
movie_shot_arm(shot_function_params){
asetf(tarm_abduct, fval(0.0211f, f20th));
if(fbool(0)){
aset(tarm_bend, fval(11));
}
arest(6);
}
function void
blink_2_frames(Movie_Shot *shot){
aset(tblink, 1);
arest(1);
aset(tblink, 6);
arest(1);
}
function void
open_eyes(Movie_Shot *shot){
aset(tblink,0);
}
function shot_function_return
movie_shot_head_tilt(shot_function_params){
{
aset(thead_theta, 0);
arest(2);
blink_2_frames(shot);
aset(thead_theta, 1);
arest(1);
aset(thead_theta, -6);
open_eyes(shot);
arest(6);
}
{
aset(thead_phi, fvali(-1));
arest(1);
aset(thead_phi, fvali(-2));
arest(6);
blink_2_frames(shot);
}
{
aset(thead_theta, fval(-8));
aset(thead_phi,   fval(-3));
arest(1);
aset(thead_theta, fval(-7));
aset(thead_phi,   fval(0));
arest(1);
aset(thead_theta, 4);
open_eyes(shot);
arest(1);
}
aset(thead_theta, 6);
arest(8);
}
#define vv(NAME, VAL, ...) \
v3 NAME = VAL; \
send_vert(NAME);

#define va(NAME, VAL, ...) \
NAME = VAL; \
send_vert(NAME);

//TODO(kv) @incomplete We're not storing the fact that it is a sampling vertex
#define vv_sample(name, curve, t, ...) \
v3 name = bezier_sample(curve, t); \
send_vert(name);

#define macro_import_vertex(vertex)  v3 vertex = ot.inv * vertex##_world;
#define macro_export_vertex(vertex)  vertex##_world = ot * vertex;
#define macro_world_declare(vertex)   v3 vertex##_world;
#define level1 (painter.viz_level >= 1)
#define level2 (painter.viz_level >= 2)
#define X_comma_list(NAME)              NAME,
//~
function void
render_hand(Forearm forearm_obj){
vertex_block("hand");
radius_scale_block(fval(0.5038f));
lp_block(nslice_per_meter, fval(5.9797f) * 100.f);
mat4i &ot       = p_current_world_from_bone();
mat4i &forearmT = p_mom_bone_xform();
Forearm forearm;
import_vertices(forearm.verts, forearm_obj.verts, ot, forearmT, forearm_vert_count);
vv(palm_base_in, (V3(-0.0476f, -2.6071f, -0.2208f)));
indicate(thumb_kbot);
Bez palm_base_line = bez_unit(thumb_kbot, V2(0.f, 0.3416f),
                               V2(0,0),
                               V3y(-1), palm_base_in);
draw(palm_base_line, lp(I4(2,2,4,8)));
bezier_sample(palm_base_line, (0.5f));
draw(bez_unit(palm_base_in,
               V2(-0.1358f, 0.5181f),
               V2(0.3399f, 0.2818f), 
               V3(-0.8552f, 0.f, -0.5183f),
               kline_in),
      lp(I4(8,0,3,1)));
vv(thumb_palm_conn, (V3(0.3704f, -3.0096f, -0.3481f)));
draw(bez_v3v2(thumb_kbot, V3(),
               V2(0,0), thumb_palm_conn));
{
bone_block(Bone_Thumb);
vv(thumb_triangle_tip, (V3(0.3693f, -2.6673f, -0.2208f)));
draw(bez_line(thumb_kbot, thumb_triangle_tip),
       lp(I4(2,4,0,3)));
vv(thumb_kmid_in, (V3(0.4589f, -2.9094f, -0.2208f)));
draw(bez_unit(thumb_kmid_in, V2(0.f, 2.9015f),
                 V2(0,0), V3y(1), from_parent()*thumb_palm_conn));
vv(thumb_ktop_in, (V3(0.565f, -3.0125f, -0.2208f)), 1);
draw(thumb_kmid_in, thumb_ktop_in,
       lp(I4(8,4,3,1)));
vv(thumb_tip, (V3(0.671f, -3.0515f, -0.2208f)));
vv(thumb_kmid_out, (V3(0.5068f, -2.8209f, -0.2208f)));
draw(bez_unit(thumb_triangle_tip, V2(-0.2102f, 0.1944f),
                V2(0.4217f, 0.2078f),
                      V3x(-1),
                      thumb_kmid_out),
       lp(I4(2,4,4,6)));
vv(thumb_ktop_out, (V3(0.6128f, -2.9593f, -0.2208f)), 1);
draw(bez_unit(thumb_kmid_out, V2(0.f, 0.3706f),
                 V2(0,0), V3x(-1)
                 , thumb_ktop_out));
draw(bez_unit(thumb_ktop_out, V2(0.f, 0.2369f),
                V2(0.4569f, -1.7741f), V3x(-1), thumb_tip),
       lp(I4(4,1,2,1)));
draw(bez_unit(thumb_ktop_in, V2(0,0),
                V2(0.2207f, 1.4562f), V3x(-1), thumb_tip),
       lp(I4(1,1,1,2)));
}
draw(thumb_palm_conn, kline_out, lp(I4(5,3,1,1)));
if(level1){
draw(knuckle_line);
}
draw(bez_v3v2(forearm.radius_bump, (V3(-0.f, 0.0101f, 0.0167f)),
               V2(0,0), thumb_kbot));
{
vertex_block("knuckles/fingers");
i32 const finger_count = 4;
v3 knuckle_bots[finger_count];
{
v3 tb_vec = fvecy(0.06f);
for_i32(index,0,4){
knuckle_bots[index] = knuckle_tops[index]+tb_vec;
}
}
for_i32(index,0,finger_count){
draw_line(knuckle_tops[index], knuckle_bots[index]);
}
{
v3 middle_knuckle_bot = knuckle_bots[1];
vv(middle_palm_bot, knuckle_bots[1]+(V3(-0.f, 0.2527f, -0.022f)));
draw_line(middle_knuckle_bot, middle_palm_bot, lp(I4((3))));
draw(bez_unit(forearm.middle_finger_meeter, V2(-0.1285f, 0.1793f),
                  V2(0.f, -0.5674f), V3z(-1), middle_palm_bot));
}
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
for_i32(index,0,finger_count+1) {
finger_bases[index] = offset+bezier_sample(knuckle_line,
                                               finger_base_ts[index]);
indicate(finger_bases[index]);
}
}
v1 top_tbend = fval(0.f);
for_i32(ifinger,0,finger_count){
bone_block(make_bone_id(Bone_Bottom_Phalanx, ifinger));
v3 k = knuckle_tops[ifinger];
v3 vec = finger_vecs[ifinger];
v3 xvec = fvecx(0.0367f);
vv(kmid, k+0.5f*vec);
v3 ikmid = kmid-xvec;
v3 okmid = kmid+xvec;
{
v2 knuckle_control = V2(0.4816f, 0.329f);
draw(bez_unit(finger_bases[ifinger], V2(0.f, 0.5811f),
                  knuckle_control, V3x(-1), okmid),
         lp(I4(3,1,1,1)));
draw(bez_unit(finger_bases[ifinger+1], V2(-0.3782f, 0.3545f),
                   knuckle_control, V3x(1), ikmid),
         lp(I4(3,3,3,1)));
if(ifinger == 0){
draw(bez_v3v2(kline_out, (V3(0.0001f, 0.f, 0.f)),
                   V2(0,0), finger_bases[0]),
          lp(I4(0,1,1,3)));
}
if(ifinger == finger_count-1){
draw(bez_v3v2(finger_bases[ifinger+1], (V3(-0.0049f, 0.f, 0.f)),
                   V2(0,0), kline_in),
          lp(I4(2,3,0,1)));
}
}
{
bone_block(make_bone_id(Bone_Mid_Phalanx, ifinger));
vv(ktop, k+0.75f*vec);
v3 iktop = ktop-xvec;
v3 oktop = ktop+xvec;
{
v2 control = V2(-0.0473f, 0.7281f);
draw(bez_unit(ikmid, control, V2(), V3x(1), iktop),
          lp(I4(3,3,3,1)));
draw(bez_unit(okmid, control, V2(), V3x(-1), oktop),
          lp(I4(3,0,2,3)));
}
v3 tip = k+vec;
{
bone_block(make_bone_id(Bone_Top_Phalanx, ifinger));
v2 tip_control = V2(-0.3298f, 1.1379f);
v2 knuckle_control = V2(0.2954f, -0.1896f);
draw(bez_unit(tip, tip_control, knuckle_control, V3x(-1), iktop),
          lp(I4(3,0,0,2)));
draw(bez_unit(tip, tip_control, knuckle_control, V3x(1), oktop),
          lp(I4(4,0,2,3)));
}
}
}
}
}
function Arm
render_arm(Pose *pose, Torso &torso_obj, v3 elbow_up_out){
view_vector_block((V3(-0.0802f, 0.f, -0.1673f)));
vertex_block("upper_arm");
scale_in_block(default_fvert_delta_scale, 2.f);
mat4i &ot     = p_current_world_from_bone();
mat4i &torsoT = p_mom_bone_xform();
Torso torso;
import_vertices(torso.verts, torso_obj.verts, ot, torsoT, torso_vert_count);
v1 tarm_bend = (pose->tarm_bend);
indicate(arm_rotation_pivot,0,true);
v3 function_condyle, external_condyle;
{
va(function_condyle, V3(-0.1053f, -1.0756f, -0.1775f));
va(external_condyle, function_condyle+(V3(0.4031f, 0.1064f, 0.0402f)));
{
hl_block;
vv(sock, (V3(-0.0675f, 0.733f, -0.2688f)));
{
vv(v89, function_condyle+(V3(0.127f, 0.1897f, -0.f)));
bs_parabola(sock,
                V3(0.137f, 0.5108f, 0.0235f),
                v89,
                lp(V4(0.5f)));
bs_unit2(v89, V4(0.f, 0.4207f, 0.f, 0.f),
             V3(0.5647f, -0.8253f, 0.f), function_condyle,
             lp(0.5f*big_to_small()));
}
{
vv(a,external_condyle+V3(-0.0777f, 0.3589f, -0.0402f));
vv(b,sock+V3(0.2662f, -0.0344f, 0.0375f));
bs_unit2(a, V4(0.f, 1.0357f, 0.f, 0.f), V3(-0.1623f, -0.9867f, 0.f), external_condyle);
bs_parabola(b, V3(-0.0431f, 0.296f, 0.f), a, lp(.5f));
vv(c, b+V3(-0.152f, 0.2585f, 0.f));
bs_v3v2(b, V3(0.1369f, 0.0569f, 0.f), V2(0.0914f, 0.1488f), c, lp(.5f));
}
vv(v98, sock+(V3(0.0947f, 0.1683f, 0.2105f)));
vv(v99, v98+(V3(-0.f, -0.6114f, -0.0063f)));
bs_parabola(v98,V3(-0.f, -0.1087f, 0.0612f),v99,
               lp(.9f));
vv(v01, v99+(V3(0.f, -1.2272f, 0.03f)));
bs_v3v2(v99, (V3(-0.f, 0.f, -0.0834f)),
           V2(0.2691f, 0.0874f), v01, lp(.9f));
vv(v20, v01+fvert(V3(-0.0184f, -0.1643f, -0.0453f)));
bs_parabola(v01, fval(V3(0.f, 0.0368f, 0.1927f)), v20, lp(.9f));
}
}
vv(delt_bot, (V3(0.0527f, 0.0729f, -0.2028f)));
v3 delt_back;
Bez delt_cross, delt_back_line, delt_bot_line, delt_vfront;
v3 delt_back_point = torso.scap_delt;
{
vv(delt_top, torso.shoulder+(V3(0.0328f, 0.f, 0.f)));
vv(delt_top_back, delt_top+(V3(-0.0109f, -0.0068f, -0.2147f)));
Bez delt_hline = bez_unit2(delt_top,
                             (V4(-0.059f, 0.584f, 0.f, 0.f)),
                             (V3(0.f, 1.f, 0.f)),
                             delt_top_back+(V3()));
draw(delt_hline, i2f6(I4(0,3,2,0)));
fill_bez( delt_hline);
delt_vfront = bez_unit2(delt_top, 
                          (V4(-0.3452f, 0.3717f, 0.0022f, 0.1226f)),
                          (V3(0.0087f, 0.f, 1.f)),
                          delt_bot);
draw(delt_vfront);
Bez delt_vmid = bez_unit2(delt_top,
                            (V4(-0.2234f, 0.2079f, 0.015f, 0.1626f)),
                            (V3(0.9999f, 0.f, 0.0112f)),
                            delt_bot);
Bez delt_vback = bez_unit2(delt_top_back,
                             V4(-0.13f, 0.1784f, -0.1368f, 0.2863f),
                             V3(0.6431f, 0.f, -0.7657f),
                             delt_bot);
va(delt_back, V3(-0.0076f, 0.2268f, -0.4244f));
fill3(delt_back, delt_bot, delt_top_back);
draw(delt_vmid, lp_alignment_min((0.8202f)));
{
Line_Params params = lp_alignment_min((0.6061f));
params.radii = (V4(0.712f, 0.f, 0.f, 0.5464f));
draw(delt_vback, params);
fill_bez(delt_vback);
}
delt_bot_line = bez_bezd_old(delt_bot, (V3()),
                           V2(0,0), delt_back);
draw(delt_bot_line, i2f6(I4(4,0,0,4)));
fill_dbez(delt_vfront, delt_vmid);
fill_dbez(delt_vmid, delt_vback);
draw(bez_unit2(delt_top_back, V4(), V3(), torso.scap_delt));
delt_back_line = bez_unit2(delt_back,
                             V4(-0.1729f, -0.1804f, 0.1064f, 0.2869f), 
                             (V3(0.f, -1.f, 0.004f)),
                             delt_back_point);
draw( delt_back_line, I4(1,4,4,1));
fill4( delt_top_back, delt_back, delt_back_point, torso.scap_delt);
delt_cross = bezd_len(torso.delt_collar, 
                        V3(0.0677f, 0.1554f, 0.2652f),
                        V2(0.f, 0.2143f),
                        delt_bot);
draw(delt_cross, I4(1,2,5,4));
fill(delt_top,delt_cross);
fill_bez( delt_cross);
{
fill( bezier_sample(delt_cross, (0.5f)), delt_vfront);
}
}
v3 pectoral_arm = V3(-0.2013f, 0.475f, 0.1047f);
{
Bez line = bez_unit2(pectoral_arm,
                       V4(0.f, 0.3866f, 0.f, 0.f),
                       V3(-0.997f, 0.0775f, 0.f),
                       torso.pectoral_torso);
draw(line);
fill(torso.delt_collar, line);
}
v3 bicep_in_bot, bicep_in_top, bicep_out_top, bicep_out_bot, white_in, white_out, tricep_mid;
vv(tricep_wedge, delt_bot+(V3(-0.f, -0.1879f, -0.0035f)));
vv(brachio_a, (V3(0.1031f, -0.7618f, -0.0498f)));
{
{
vv_sample(tricep_up, delt_back_line, 0.2993f);
va(tricep_mid, (V3(-0.1105f, -0.1135f, -0.475f)));
va(white_out, tricep_mid+V3(0.0446f, -0.1341f, 0.0966f));
va(white_in, tricep_mid+V3(-0.1177f, -0.3581f, 0.007f));
{
draw(bezd_len(delt_back, V3(-0.f, 0.f, -0.0662f),
                  V2(0,0), tricep_mid),
         I4(5,0,0,0));
draw(bez_line(tricep_mid, white_in), i2f6(I4(3,0,0,0)));
draw(bez_line(tricep_mid, white_out), i2f6(I4(1,1,1,1)));
}
vv(tricep_out_top, bezier_sample(delt_bot_line, (.3f)));
if(level1){
draw(bez_line(tricep_out_top, tricep_wedge));
}
fill4(tricep_wedge, tricep_mid, delt_back, delt_bot);
fill3(white_out, tricep_mid, tricep_wedge);
draw(bez_v3v2(torso.scap_sock_bot, (V3(-0.2354f, 0.048f, 0.f)),
            V2(0.2603f, -0.0407f), white_in),
        lp(I4(0,0,8,0)));
}
{
va(bicep_out_top, bezier_sample(delt_cross, fval(0.8389f)));
bicep_out_bot = ((V3(0.0079f, -0.9224f, -0.027f)) +
                    lerp(V3(), tarm_bend, (V3())));
Bez front_out = bez_unit2(bicep_out_top,
                             (V4(-0.0571f, 0.0667f, 0.f, 0.f)),
                             (V3(0.7886f, 0.f, 0.6149f)),
                             bicep_out_bot);
vv(hinge_out, bezier_sample(front_out, fval(0.6758f)));
va(bicep_in_top, bicep_out_top+(V3(-0.211f, 0.0922f, 0.0631f)));
va(bicep_in_bot, bicep_out_bot+(V3(-0.148f, 0.0022f, 0.0005f)));
fill3(brachio_a, hinge_out, bicep_out_bot);
Bez front_in = bez_unit2(bicep_in_top,
                            (V4(0.f, 0.0397f, 0.0201f, 0.1254f)),
                            (V3(-0.8434f, 0.f, 0.5373f)),
                            bicep_in_bot);
draw(front_in, lp(I4(0,0,0,4)));
fill_dbez(front_in, front_out);
vv(bicep_side_top, bezier_sample(delt_vfront, fval(0.9539f)));
vv(bicep_side_bot, bicep_out_bot + (V3(0.1103f, 0.1089f, -0.084f)));
Bez side_out = bezd_len(bicep_side_top, (V3(-0.f, 0.f, -0.0502f)),
                           V2(0,0), bicep_side_bot);
draw(side_out, i2f6(I4(4,0,0,0)));
fill3(pectoral_arm, bicep_in_top, bicep_out_top);
fill3(pectoral_arm, torso.delt_collar, bicep_out_top);
vv(bicep_side_top2, bezier_sample(delt_cross, fval(0.7406f)));
vv(bicep_side_bot2, bicep_out_bot+(V3(-0.0601f, 0.f, 0.f)));
{
v1 alignment = fval(0.9169f);
v3 view_vector = get_view_vector();
Bez bicep_side_line2 = bez_unit(bicep_side_top2,
                                     V2(0.f, 0.0537f),
                                     V2(0,0),
                                     V3z(1.f),
                                     bicep_side_bot2);
b32 drawn = draw(bicep_side_line2, lp(alignment, I4(4,3,1,0)));
if(!drawn){
draw(front_out, lp(I4(5,0,0,0)));
}
fill_bez(bicep_side_line2);
}
fill4(bicep_side_top, bicep_side_top2, bicep_side_bot2, bicep_out_bot);
{
b32 ok = get_view_vector().x < fval(0.07f);
i4 radii = I4(6,6,0,0);
if(get_view_vector().x > fval(-0.20f)){
radii.y = fvali(0);
}
if(ok){
draw(bez_v3v2(bicep_side_top2,
              (V3(0.0184f, 0.f, 0.0925f)),
              V2(0,0),
              brachio_a),
          lp(radii));
}
}
}
}
draw(bez_line(brachio_a, bicep_out_bot), i2f6(I4(3,3,3,1)));
vv(brachio_humerus, (V3(0.0464f, -0.2894f, -0.2469f)));
{
fill4(brachio_humerus, delt_bot, bicep_out_top, bicep_out_bot);
}
Bez tricep_cross_line = bez_v3v2(tricep_wedge, (V3(0.0474f, -0.0292f, 0.0884f)),
                                  V2(0,0), elbow_up_out);
draw(tricep_cross_line, i2f6(I4(0,0,4,6)));
fill_bez(tricep_cross_line);
vv(brachio_humerus2, bezier_sample(tricep_cross_line, fval(0.4296f)));
Arm arm_obj;
{
#define export_(vertex)  arm_obj.vertex = vertex;
  
macro_arm(export_);
#undef export_
 
}
return arm_obj;
}
function Forearm
render_forearm(Arm &arm_obj, v3 elbow_offset, v3 elbow_up_out){
mat4i &ot   = p_current_world_from_bone();
mat4i &armT = p_mom_bone_xform();
Arm arm;
import_vertices(arm.verts, arm_obj.verts, ot, armT, arm_vert_count);
send_vert(palm_in);
{
hl_block;
Bez l492;
auto params = lp((0.5f));
v3 down_common;
{
vv(up , palm_in+(V3(-0.2986f, 1.4924f, -0.1428f)));
vv(up1 , up+(V3(0.1879f, 0.f, -0.0369f)));
va(down_common , palm_in+(V3(0.1191f, 0.0489f, 0.f)));
ba_parabola(l492, down_common, (V3(-0.0036f, -0.2325f, 0.f)), up1);
bs_parabola(up, (V3(0.0822f, 0.1457f, 0.f)), palm_in, params);
vv(v676, V3(-0.1753f, -0.854f, -0.0817f));
vv(v677, v676+V3(0.f, -0.3293f, -0.0435f));
bs_v3v2(v676,
           V3(-0.f, 0.1052f, -0.0852f),
           V2(0.f, 0.0936f), v677);
}
{
vv_sample(up, l492, fval(0.9006f));
bs_parabola(down_common, (V3(0.0871f, 0.0251f, 0.f)), up, params);
vv(down2, down_common+(V3(0.175f, -0.0472f, 0.f)));
vv(up2, up+(V3(0.1097f, 0.161f, 0.f)));
bs_parabola(down2, (V3(0.0988f, -0.2945f, 0.f)), up2, params);
}
}
vv(near_palm_in, palm_in+(V3(-0.0491f, 0.0154f, 0.04f)));
Bez l500;
v3 in_c;
{
vv(in_a, (V3(-0.2345f, -0.8464f, -0.0226f)));
vv(in_b, (V3(-0.269f, -1.3024f, 0.254f)));
va(in_c, (V3(-0.1582f, -1.8388f, 0.1688f)));
bb_v3v2(l493, in_a, V3(-0.0385f, 0.f, 0.f),
          V2(0,0), in_b,
          lp(I4(0,2,2,5)));
fill3(in_b+V3(0.0076f, 0.f, -0.0131f), arm.bicep_in_bot, in_c);
{
bb_c2(l499, l493, V3(), in_c, lp(I4(5,3,3,0)));
fill_bez(l499);
ba_c2(l500, l499, V3(0.0348f, 0.0219f, -0.0201f), near_palm_in);
draw(l500, lp(I4(0,1,5,1)));
}
}
v3 brachio_b, brachio_c;
{
vertex_block("brachio");
v3 &a = arm.brachio_a;
v3 &b = brachio_b;
v3 &c = brachio_c;
va(b, a+(V3(0.1483f, -0.2286f, -0.0134f)));
va(c, (V3(0.2034f, -1.85f, 0.1108f)));
draw(bez_v3v2(b, (V3(0.0352f, 0.f, -0.f)),
                V2(0,0), c),
       lp(I4(6,5,3,1)));
vv(d, (V3(0.0371f, -0.7757f, -0.3281f)));
vv(e, d+(V3(0.0682f, -0.2334f, -0.024f)));
vv(f, d+(V3(0.0466f, -0.6053f, 0.0122f)));
fill4(a,b,c, arm.bicep_out_bot);
fill(c, l500);
fill4(in_c, brachio_c, arm.bicep_out_bot, arm.bicep_in_bot);
fill3(arm.external_condyle,b,c);
fill3(arm.external_condyle,arm.brachio_humerus2,b);
}
{
vertex_block("elbow");
v3 &o = elbow_offset;
vv(up_in, o+(V3(-0.1512f, 0.9396f, -0.2042f)));
vv(low_in, o+(V3(-0.1031f, 0.7618f, -0.2006f)));
i4 elbow_radii = fval(I4(4,3,3,2));
draw(bez_line(up_in,low_in), elbow_radii);
v3 &up_out = elbow_up_out;
indicate(up_out,0);
vv(low_out, o+(V3(-0.063f, 0.7591f, -0.2006f)));
draw(bez_line(up_out,low_out), elbow_radii);
{
draw(bez_line(arm.white_in, up_in), I4(0,3,3,6));
draw(bez_line(arm.white_out, up_out), I4(0,0,0,6));
}
fill3(arm.tricep_wedge, arm.white_out, up_out);
}
draw(bez_unit(arm.brachio_humerus,
               V2(0.f, 0.0875f),
               V2(0,0),
               V3z(1),
               arm.brachio_a));
draw(bez_unit(arm.brachio_humerus2,
               V2(0.f, 0.0875f),
               V2(0.f, 0.1935f),
               (V3(0.7928f, 0.f, 0.6095f)),
               brachio_b),
      I4(0,0,3,6));
fill3(arm.brachio_humerus, arm.brachio_a, elbow_up_out);
fill4(arm.brachio_humerus, arm.brachio_humerus2, arm.brachio_a, brachio_b);
vv(radius_bump, palm_in+(V3(0.2725f, 0.1637f, -0.0384f)));
{
Bez l576 = bez_v3v2(brachio_c, (V3(-0.0479f, 0.0162f, 0.0042f)),
                      V2(0,0), radius_bump);
draw(l576, lp(I4(1,3,3,0)));
fill(near_palm_in, l576);
}
vv(v593, lerp(brachio_c, fval(0.1619f), in_c));
vv(radius_back, palm_in+(V3(0.2716f, 0.1061f, -0.1844f)));
{
vv(ulnar_ball, palm_in+(V3(-0.0308f, 0.2166f, -0.0893f)), 0);
vv(v566, (V3(-0.1204f, -1.8951f, -0.0509f)));
draw(bez_v3v2(ulnar_ball, (V3(0.0482f, 0.f, -0.f)),
           V2(0,0), v566),
       lp(I4(0,5,2,0)));
draw(bez_v3v2(arm.function_condyle, (V3(-0.0671f, 0.f, 0.f)),
                V2(0,0), v566),
       lp(I4(1,5,5,0)));
draw_line(radius_back, radius_bump);
fill3(radius_back, v593,
        lerp(radius_bump, ((0.1308f)), near_palm_in));
}
vv(middle_finger_meeter, (V3(0.0936f, -2.5386f, -0.171f)));
v3 v2189 = (V3(-0.2725f, -0.8924f, -0.251f));
draw(bez_v3v2(v2189,
               (V3(0.f, 0.1288f, 0.1481f)),
               V2(0,0),
               (V3(0.1923f, -1.8531f, -0.198f))));
Bez l618 = bez_v3v2(arm.external_condyle, (V3(0.0833f, 0.3209f, -0.0449f)),
                     V2(0,0), middle_finger_meeter);
draw(l618);
vv_sample(v612, l618, 0.7612f);
fill3(v612, arm.external_condyle, brachio_c);
fill3(v612, v593, radius_back);
fill3(v612, brachio_c, v593);
Forearm forearm_obj;
#define export_(vertex)  forearm_obj.vertex = vertex;
 
macro_forearm(export_);
#undef export_
 
return forearm_obj;
}
function Torso
render_torso(Pose *pose, Pelvis pelvis_obj, Head head_obj){
view_vector_block((V3(0.f, -2.3176f, 0.3862f)));
mat4i &ot = p_current_world_from_bone();
mat4i &pelvisT = get_bone_xform(Bone_Pelvis);
mat4i &headT   = get_bone_xform(Bone_Head);
v1 head_unit = get_column(ot.inv,1).y * head_unit_world;
Head head;
import_vertices(head.verts, head_obj.verts, ot, headT, head_vert_count);
Pelvis pelvis;
import_vertices(pelvis.verts, pelvis_obj.verts, ot, pelvisT, pelvis_vert_count);
vv(shoulder, V3(1.1765f, -1.6108f, 0.0573f));
send_vert(scap_sock_top);
mat4i &armT = get_bone_xform(Bone_Arm);
mat4 arm_local = ot.inv * armT;
vv(shoulder_in, V3(0.4819f, -1.3302f, -0.4461f));
v3 sternum = V3(0.f, -1.6696f, 0.2691f);
vv(delt_collar, shoulder + V3(-0.3387f, -0.02f, 0.1371f));
vv(sternumL, sternum + V3x((0.0919f)));
{
Bezier neck_front_vline = bez_bezd_old(head.head_neck_junction,
                                     V3(-0.f, 0.f, -0.0968f), 
                                     V2(0.f, 0.2897f), sternum);
draw(neck_front_vline, profile_visible_transition(0.5f));
Bezier neck_back_line = bez_offset(head.trapezius_head,
                                     V3(-0.1529f, -0.1932f, 0.2857f),
                                     V3(-0.1937f, 0.0701f, 0.0969f),
                                     shoulder_in);
draw(neck_back_line, I4(0,1,3,0));
{
symx_off;
fill_dbez(neck_back_line, negateX(neck_back_line));
}
Bezier collar_in = bez_offset(sternumL,
                                V3(0.2915f, -0.0448f, -0.0087f),
                                V3(-0.1047f, -0.0248f, 0.1171f),
                                delt_collar);
Bezier collar_out = bez_offset(delt_collar+(V3(-0.0957f, 0.0021f, 0.016f)),
                                 V3(0.f, 0.f, 0.f),
                                 V3(-0.0607f, 0.0941f, 0.0168f),
                                 shoulder);
draw(bez_offset(shoulder_in,
                  V3(0.0993f, -0.0342f, -0.f),
                  V3(-0.0356f, 0.0675f, -0.0634f),
                  shoulder),
       I4(5,3,3,1));
{
if(level1){
push_hl();
}
draw(collar_out);
draw(collar_in);
if(level1){
pop_hl();
}
}
{
Bezier sterno;
{
sterno = bez_offset(head.behind_ear,
                        V3(-0.107f, -0.2444f, 0.2159f),
                        V3(0.0786f, 0.1842f, -0.1674f),
                        sternumL);
{
v3 patch = (V3(-0.1658f, 0.f, -0.0888f));
v1 thead_theta = pose->thead_theta;
v1 tcontract = (is_left() ?
                     clamp01(-thead_theta) : 
                     clamp01(+thead_theta));
sterno[1] += tcontract * patch;
}
}
draw(sterno, I4(2,0,0,3));
fill_dbez(neck_back_line, sterno);
fill_dbez(neck_front_vline, sterno);
}
}
vv(scap_delt, shoulder_in + (V3(-0.1213f, -0.3863f, -0.0611f)));
v3 rib_mid, rib_back;
{
v1 rib_midY = (shoulder.y - 0.65f*head_unit + (-0.0775f));
va(rib_mid, V3(0.f, rib_midY, (0.8061f)));
if(is_left()){
draw(bez_bezd_old(sternum,
                     V3(0.f, 0.0027f, 0.2265f),
                     V2(0.f, 0.0935f),
                     rib_mid));
}
vv(ribL, rib_mid+(V3(0.559f, -0.7158f, -0.2772f)));
v3 rib_in = rib_mid+V3x((0.0905f));
draw(bez_unit2(rib_in,
                 V4(0.f, 0.0689f, 0.0558f, 0.2431f),
                 V3(0.629f, -0.2482f, 0.7367f),
                 ribL),
       I4(2,6,3,1));
fill4( rib_mid, ribL, delt_collar, sternum);
va(rib_back, ribL+(V3(0.0855f, 0.171f, -0.5742f)));
draw(bez_unit(ribL, V2(-0.0705f, 0.2667f),
                V2(0.f, 0.384f), 
                V3(0.5421f, -0.8353f, 0.0916f),
                rib_back));
{
vv(vestL,ribL+(V3(0.0371f, 0.3022f, 0.0011f)));
draw(bez_unit2(rib_in,
                  V4(0.f, 0.4043f, 0.f, 0.2469f),
                  (V3(0.f, 0.852f, 0.5235f)),
                  vestL));
vv(vest_back, rib_back+(V3(-0.f, 0.5114f, -0.0067f)));
draw(bez_unit2(vestL,
                  V4(-0.0067f, 0.1427f, 0.f, 0.3102f),
                  V3(0.9212f, -0.3891f, -0.f),
                  vest_back));
}
}
vv(latis_arm, arm_local*V3(-0.2595f, 0.263f, -0.0002f));
{
Bez latis_arm_line = bez_unit(rib_back, V2(0.1935f, 0.0429f), 
                                V2(0.151f, 0.0235f),
                                V3(1.f, 0.f, -0.f),
                                latis_arm);
draw(latis_arm_line, lp_alignment_min(0.7f));
}
v3 scap_sock_bot;
{
vv(trap_bot, V3(0.f, -3.1104f, -0.1081f));
if(level2){
hl_block;
draw(bez_unit2(scap_delt,
                  V4(0.f, 0.1307f, 0.f, 0.0566f),
                  V3(0.5328f, 0.0057f, -0.8462f),
                  shoulder));
}
fill3_symx(scap_delt, shoulder_in);
fill3(shoulder_in, shoulder, scap_delt);
fill3_symx(trap_bot,scap_delt);
Bez hip_back_line = bez_unit(rib_back,
                               V2(0.f, 0.1135f),
                               V2(0.01f, 0.1345f),
                               V3(-0.5382f, 0.f, 0.8428f),
                               pelvis.bikini_up_back);
draw(hip_back_line);
vv(back_archL, bezier_sample(hip_back_line, (0.5606f)));
draw(bez_v3v3(trap_bot, V3(0.235f, 0.3261f, -0.0079f),
                V3(0.0805f, 0.f, 0.0552f), back_archL));
v3 scap_bot;
{
hl_block;
b32 disabled = is_right();
set_in_block(painter.painting_disabled, disabled);
va(scap_bot, scap_delt+V3(0.1134f, -0.7434f, 0.1812f));
va(scap_sock_bot, scap_sock_top+V3(0.0098f, -0.2771f, 0.f));
bs_unit2(scap_delt,
            V4(-0.14f, 0.1607f, 0.4409f, 0.224f),
            V3(-0.6328f, 0.f, -0.7743f),
            scap_bot);
bs_unit2(scap_delt,
            V4(0,0,0,0),
            V3(),
            scap_sock_top);
bs_unit2(scap_bot,
            V4(0.f, 0.118f, 0.f, 0.f),
            V3(-0.f, 0.f, -1.f),
            scap_sock_bot);
bs_parabola(scap_sock_top, fvec(V3(-0.1101f, -0.0058f, -0.0915f)), scap_sock_bot);
}
vv(lower_back, V3(0.f, -4.2845f, -0.0495f));
fill3( lower_back, trap_bot, back_archL);
vv(diamond_up,  V3z(shoulder_in.z)+V3(0.f, -1.2636f, 0.0193f));
Bez back_midline = bez_unit2(diamond_up,
                               (V4(-0.0605f, 0.1215f, 0.f, 0.f)),
                               V3z(-1),
                               trap_bot);
draw(back_midline, V4(0.3513f, 0.9763f, 0.6129f, 1.1402f));
vv(diamond_low, bezier_sample(back_midline, (0.2302f)));
vv(diamondL,    diamond_up+(V3(0.0856f, -0.2026f, -0.0719f)));
draw( bez_line(diamond_up, diamondL));
draw( bez_line(diamond_low, diamondL));
Bez trap_vline = bez_unit2(scap_delt, 
                             (V4(0.f, 0.0667f, 0.f, 0.f)), 
                             (V3(-0.379f, 0.f, -0.9254f)),
                             bezier_sample(back_midline, (0.9154f)));
draw(trap_vline, (V4(0.7542f, 1.f, 1.f, 0.2694f)));
vv(trap_weird_point, bezier_sample(trap_vline, (0.5728f)));
Bez trap_weird_line = bez_unit2(trap_weird_point,
                                  (V4(0.0369f, 0.1268f, 0.f, 0.f)),
                                  (V3(-0.8654f, 0.f, -0.5011f)),
                                  rib_back);
draw(trap_weird_line, V4(0.75f, 1.f, 1.0735f, 0.5f));
}
v3 chest_back, pectoral_torso;
{
v1 nippleY = shoulder.y-0.4f*head_unit;
vv(nipple, V3y(nippleY) + V3(0.5799f, 0.f, 0.6752f), 0);
vv(chest_in, (V3(0.1045f, -2.5438f, 0.8167f)));
vv(chest_out, (V3(0.6599f, -2.4822f, 0.6002f)));
va(pectoral_torso, chest_out+(V3(0.1171f, 0.1455f, -0.1676f)));
fill3(delt_collar,pectoral_torso,chest_out);
draw(bez_unit2(chest_in,
                 V4(-0.0299f, 0.2108f, 0.1371f, 0.253f),
                 (V3(0.0915f, -0.523f, 0.8474f)),
                 chest_out),
       I4(6,4,3,1));
va(chest_back, (V3(0.7613f, -2.0817f, 0.039f)));
}
{
Bez latis_side = bez_unit(chest_back, V2(-0.1341f, 0.0768f),
                            V2(0,0), 
                            (V3(0,0,-1)),
                            rib_back);
draw(latis_side,lp_alignment_min(0.7f));
}
if(is_left()){
draw(bez_parabola(rib_mid, V3z(-0.0952f), pelvis.navel));
}
Torso torso_obj = {macro_torso(X_comma_list)};
return torso_obj;
}
function Pelvis
render_pelvis(Pose &pose){
v1 head_topY = head_radius_world;
mat4i &ot = p_current_world_from_bone();
v1 navelY = (ot.inv * V3y(head_topY - 2.5f * head_unit_world)).y;
vv(navel, V3y(navelY) + V3z(0.271f), 0);
vv(crotch, V3());
vv(crotchL, V3x(0.1f));
bs_line(crotch,crotchL);
{
vv(bikiniL, (V3(1.0685f, 0.8105f, -0.6117f)));
v3 bikini_dir = (V3(-0.1823f, -0.0005f, 0.9832f));
bs_unit(crotchL, V2(-0.2081f, 0.2062f), 
          V2(0.316f, 0.3309f),
          bikini_dir,
          bikiniL);
bs_unit(crotchL, V2(-0.2767f, 0.2024f),
          V2(0.213f, 0.2503f),
          -bikini_dir, bikiniL);
}
v3 bikini_up_back;
{
vv(bikini_front_mid, V3(0.f, 0.951f, 0.1677f));
vv(girdle_front, bikini_front_mid + (V3(0.8075f, 0.2603f, -0.0717f)));
va(bikini_up_back, (V3(0.5102f, 1.3265f, -0.7766f)));
bb_unit2(girdle_side_line,
           girdle_front,
           V4(-0.3554f, 0.3781f, 0.2608f, 0.2404f),
           V3(0.7543f, 0.6022f, 0.2614f),
           bikini_up_back,
           I4(0,6,6,1));
bs_bezd_old(bikini_front_mid, V3(-0.0581f, -0.1036f, 0.f),
              V2(-0.0136f, 0.1226f), girdle_front);
{
bs_fill3(bikini_front_mid, crotch, girdle_front);
bs_fill3(girdle_front, crotch, bikini_up_back);
bs_fill_bez(girdle_side_line);
}
}
Pelvis pelvis_obj;
#define export_(vertex)  pelvis_obj.vertex = vertex;
 
macro_pelvis(export_);
#undef export_
 
return pelvis_obj;
}
//  C:\Users\vodan\4ed\code/meta_klang.cpp:449:
struct Cache_Storage_34971{
b32 cache_initialized;
v1 tblink;
};
global Cache_Storage_34971 cache_storage_34971;
function void
render_eyes(Pose *pose){
v1 tblink = pose->tblink;
if(not cache_storage_34971.cache_initialized or
not(tblink == cache_storage_34971.tblink &&
true)){
{
cache_storage_34971.cache_initialized = true;
cache_storage_34971.tblink = tblink;
}
{
v3 eyeO = V3(0.1953f, -0.2348f, 0.8954f);
v3 eye_scale = V3(0.8324f, 0.9882f, 0.92f);
mat4i eyeT = mat4i_translate(eyeO)*mat4i_scales(eye_scale);
vv(eye_in, eyeT*V3(-0.0282f, 0.f, -0.1236f));
v1 loomis_eye_inZ = faceZ - loomis_unit/3.f;
WARN_DELTA(eye_in.z, loomis_eye_inZ, 0.05f);
b32 show_eye_guideline = painter.show_grid;
if(level1 || show_eye_guideline){
hl_block;
if(level1){
draw(bez_line(eye_in, nose_rootL));
}
}
vv(eye_out, eye_in + V3(2.f * eye_in.x+(-0.03f), 0, -0.154f));
if(level1){
hl_block;
draw_line(eye_out, brow_out);
}
vv_sample(es_up_in, brow_ridge, 0.3668f);
Bezier eye_up_line = bez_bezd_old(eye_in, 
                                    V3(0.0814f, 0.3741f, 0.2401f),
                                    V2(0.0159f, 0.3341f),
                                    eye_out);
fill(es_up_in, eye_up_line);
argb eye_in_shade = painter.shade_color;
fill3(nose_rootL, eye_in, es_up_in, 
        fp(eye_in_shade));
v4 eye_up_line_radii;
v4 eye_low_line_radii;
{
v1 eye_in_radius = lerp(V2(0.0137f, -0.0006f), tblink);
v1 eye_out_radius = lerp(V2(0.4791f, 0.17f), tblink);
v1 eye0_radius = lerp(V2(0.6426f, 0.499f), tblink);
v1 eye1_radius = lerp(V2(0.f, 0.6154f), tblink);
eye_up_line_radii= V4(eye_in_radius, eye0_radius, eye1_radius, eye_out_radius);
eye_low_line_radii= eye_up_line_radii;
eye_low_line_radii[1] = 0.0555f;
}
bb_bezd_old(eye_low_line,
              eye_in, 
              V3(0.1735f, -0.2094f, 0.1663f), 
              V2(0.117f, 0.2053f), 
              eye_out,
              eye_low_line_radii);
{
vv(eye_low_patch, lerp(eye_in, 0.4768f, cheek_low));
fill(eye_low_patch, eye_low_line);
fill3(eye_low_patch, eye_out, cheek_low);
fill3(eye_out, brow_out, es_up_in);
}
{
bb_raw(eye_up_line_now,
          eye_in, 
          lerp(eye_up_line[1], tblink, eye_low_line[1]),
          lerp(eye_up_line[2], tblink, eye_low_line[2]),
          eye_out,
          eye_up_line_radii);
fill_dbez(eye_up_line, eye_up_line_now, eye_in_shade);
}
{
bone_block(Bone_Eyeball);
{
b32 show_eyeball = get_preset() == 2;
if(show_eyeball){
hl_block;
v4 eyeball_radii;
{
v1 big = 0.5f;
v1 small = 0.25f;
eyeball_radii = V4(big,small,small,big);
}
bs_circle(V3(),V3z(1),eyeball_radii);
bs_circle(V3(),V3x(1),eyeball_radii);
}
}
{
lp_block(nslice_per_meter, 128.f*(7.6378f));
v1 iris_depth_offset = painter.fill_depth_offset + 1*centimeter;
lp_block(depth_offset, current_line_cparams().depth_offset+iris_depth_offset);
add_in_block(painter.fill_depth_offset, iris_depth_offset);
{
mat4 irisRelT;
v1 iris_radius = (0.4934f);
v3 iris_rel_center = V3z(square_root(1-squared(iris_radius)));
v4 iris_radii;
{
v1 big = 0.5f;
v1 small = 0.25f;
iris_radii = V4(big,small,small,big);
}
bs_circle(iris_rel_center, V3z(iris_radius), iris_radii);
}
}
{
for_i32(index,0,2){
Bez line = index==0 ? eye_up_line : eye_low_line;
auto result = get_eye_min_distance(eyeball_center, eyeball_radius, line);
if(result.min_distance < 0){
DEBUG_VALUE(result.min_distance/millimeter);
indicate(result.closest_point, 0);
}
}
}
}
fill3(nose_wing, cheek_low, eye_in);
fill3(nose_root_backL, eye_in, nose_rootL);
fill3(nose_root_backL, nose_wing, eye_in);
fill3(eye_out, cheek_up, brow_out);
}
}
}
function Head
render_head(Pose *pose, v1 animation_time){
lp_block(nslice_per_meter, (4.1128f) * 100.f);
const v1 head_unit = 1.f+square_root(2);
b32 show_loomis_ball = fbool(0);
if(level2){
show_loomis_ball = true;
}
i32 preset = get_preset();
{
if(preset == 3){
show_loomis_ball = fbool(1);
}
if(preset == 4){
show_loomis_ball = fbool(0);
}
}
const v3 loomis_cross_center = V3(0.f,0.f,1.f);
mat4i &ot = p_current_world_from_bone();
v1 noseY = -loomis_unit;
vv(nose_tip, V3(0.f, noseY, faceZ) + V3(0.f, 0.0237f, 0.0916f));
view_vector_block(nose_tip);
mat4 &to_local = ot.inverse;
{
v3 camera_local = to_local * camera_world_position(&painter.camera);
v3 view_vector = noz(camera_local - nose_tip);
v1 x = view_vector.z;
v1 y = view_vector.x;
v1 theta = arctan2(y,x);
painter.profile_score = absolute(theta*4.f);
}
v1 chinY = -2.f*loomis_unit;
vv(chin_middle, V3(0,chinY,faceZ) + V3z((-0.0856f)));
v1 face_sideX = inv_root2;
v3 loomis_side_center = V3x(face_sideX);
v1 loomis_side_radius = inv_root2;
vv(ear_center, V3x(face_sideX) + V3(0.0416f, -0.3601f, -0.2332f));
v3 side_circle_center = V3(face_sideX,0,0);
v1 side_circle_radius = loomis_unit;
vv(chinL, V3(0,chinY,chin_middle.z) + V3(0.1572f, 0.0504f, -0.077f));
v3 jaw = V3(face_sideX, chinY, 0) + (V3(-0.0844f, 0.4575f, -0.0247f));
v3 mouth_base = V3(0, lerp(chinY, (0.5871f), noseY), faceZ);
v3 mouth_corner = mouth_base + (V3(0.2499f, 0.f, -0.2801f));
v3 &mouth_cornerL = mouth_corner;
v3 mouth_cornerR = negateX(mouth_corner);
v1 nose_baseY = noseY;
indicate(mouth_corner);
v1 nose_wingX = (0.1054f);
v1 bridgeX = lerp(0.f, 0.284149f, nose_wingX);
v1 browY = 0.f;
v1 nose_sideX = (0.0586f);
va(nose_rootL, V3(nose_sideX, 
                   lerp(browY, (0.1534f), noseY), 
                   faceZ+(-0.104f)));
va(nose_wing, V3(nose_wingX, nose_baseY, faceZ));
vv(nose_wing_up, nose_wing+V3(-0.011f, 0.1107f, 0.0069f));
v3 lip_low_center;
{
Bezier lip_up, philtrum_line, philtrum_line_mid, lip_low_line;
v1 philtrumX = 0.0435f;
b32 nerf_mouth = false;
radius_scale_block(0.610978f);
v3 philtrum_up = V3(0, nose_baseY, faceZ) + V3(0,0,0);
v3 philtrum_low = mouth_base + (V3(0.f, 0.078f, 0.035f));
philtrum_line_mid = bez_offset(philtrum_up ,
                              (V3(0.f, -0.1241f, 0.0076f)),
                              (V3(0.f, 0.0481f, -0.0345f)),
                              philtrum_low);
indicate(philtrum_low);
philtrum_line = philtrum_line_mid;
for_i32(index,0,4) {
philtrum_line[index].x += philtrumX;
}
for_i32(index,0,4) {
philtrum_line_mid[index].z -= (0.0065f);
}
{
symx_off;
v1 threshold = (0.8284f);
draw(philtrum_line, lp(threshold, I4(0,0,3,0)));
}
v3 philtrum_lowL = philtrum_low+V3x(philtrumX);
lip_up  = bez_v3v3(philtrum_lowL, 
                     (V3(0.088f, 0.1234f, 0.2037f)),
                     (V3(-0.0951f, 0.0104f, 0.3071f)),
                     mouth_corner);
{
Line_Params params = profile_visible_transition(0.6014f);
params.radii = i2f6(I4(1,1,3,0));
if(level1){
params = painter.line_params;
}
}
{
v3 lip_in = sety(philtrum_lowL+(V3(-0.f, 0.f, -0.0527f)),
                    mouth_corner.y+(0.0133f));
Bezier lip_up_bot = bez_unit2(lip_in,
                                 (V4(-0.0828f, 0.5237f, 0.f, 0.f)),
                                 (V3(0.f, 0.2748f, 0.9615f)),
                                 mouth_corner);
draw(lip_up_bot,
        (V4( 0.4876f, 0.3631f, 0.5066f, -0.2905f)));
}
{
lip_low_center = mouth_base + (V3(0.f, -0.088f, -0.0133f));
indicate(lip_low_center);
lip_low_line = bez_unit2(lip_low_center,
                            (V4(0.f, 1.0473f, 0.f, -0.7242f)),
                            (V3(0.f, -0.1848f, 0.9828f)),
                            mouth_corner);
if(!nerf_mouth){
v4 radii = fval4(0.7f, -0.2807f, 0.f, 0.f);
if(level1){
radii = painter.line_params.radii;
}
draw(lip_low_line,radii);
}
}
vv(philtrum_offset_point, nose_wing+(V3(-0.0499f, -0.0725f, -0.0489f)));
fill(philtrum_offset_point, lip_up);
fill(philtrum_offset_point, philtrum_line_mid);
fill3(philtrum_offset_point, nose_wing, philtrum_up+V3x(philtrumX));
fill3(philtrum_offset_point, mouth_corner, nose_wing);
fill3(philtrum_lowL,philtrum_low,philtrum_offset_point);
}
vv(v1219, addx(nose_tip, nose_sideX));
bb_unit2(nose_line_side,
          nose_rootL,
          V4(),
          V3(0.f, 0.f, 1.f),
          v1219,
          lp_invisible());
vv(nose_tipL, nose_tip+V3x(nose_sideX));
bs_offset(nose_tipL,
           V3(0.0121f, -0.0404f, -0.1108f),
           V3(),
           nose_wing,
           V4(0.151758f, 0.699609f, 1.149659f, -0.148325f));
{
symx_off;
bb_negateX(nose_line_sideR, nose_line_side, lp_invisible());
bs_fill_dbez(nose_line_side, nose_line_sideR);
}
{
bs_parabola(nose_wing, V3(0.0539f, 0.07f, -0.0417f), nose_wing_up,
              profile_visible_transition(0.70f));
}
{
symx_off;
v3 control = V3(-0.0533f, 0.0283f, 0.0438f);
draw(bez_offset(nose_tipL,
                  control,
                  negateX(control),
                  negateX(nose_tipL)),
       0.7097f*painter.line_params.radii);
}
va(cheek_low,(V3(0,noseY,faceZ) +
               V3(0.6062f, 0.f, -0.4289f)));
fill4(jaw, chinL, cheek_low, mouth_corner);
va(nose_root_backL, V3(0.f, -0.0651f, -0.1755f) + V3x(nose_sideX));
va(brow_out, V3(0.585f, -0.1342f, 0.5485f));
va(cheek_up, brow_out+V3(0.f, -0.2935f, 0.f));
if(level1){
hl_block;
draw_line(brow_out, cheek_up);
}
brow_ridge = bez_unit2(nose_rootL,
                        V4(0.f, 0.3208f, 0.f, 0.6236f),
                        V3(0.f, 0.6019f, 0.7986f),
                        brow_out);
if(level1){
hl_block;
draw(brow_ridge);
}
{
radius_scale_block(0.8053f);
v3 brow_in = (bezier_sample(brow_ridge, 0.1313f) +
                V3(0.0044f, -0.0566f, 0.0056f));
indicate(brow_in);
v3 brow_mid = (bezier_sample(brow_ridge, (0.593f)) + 
                 V3(-0.0006f, 0.0008f, 0.0086f));
indicate(brow_mid);
Bezier brow_in_line = bez_line(brow_in, brow_mid);
v1 brow_joint_radius;
{
v4 radii = fval4(0.3344f, 0.6654f, 0.61f, 1.f);
brow_joint_radius = radii.v[3];
draw(brow_in_line, radii);
}
{
Bezier brow_out_line = bez_unit(brow_mid, (v2{0.f, 0.5549f}), (v2{0.f, 0.4409f}), V3(0.f, 1.f, 0.f), brow_out);
v4 radii = fval4(0.25f, 0.8f, 0.8f, 0.25f);
radii.v[0] = brow_joint_radius;
draw(brow_out_line, radii);
}
}
{
vv(cj, lerp(chinL, 0.5f, jaw));
{
v4 radii = painter.line_params.radii;
radii[0] = 0.f;
radii[1] = 0.f;
bb_raw(jaw_line, ear_center, jaw, jaw, cj, radii);
}
}
fill3(nose_wing, mouth_corner, cheek_low);
{
Bezier line = bez_line( nose_root_backL, nose_wing);
fill_dbez( nose_line_side, line);
}
v3 foreheadL = brow_out + (V3(0.0477f, 0.7772f, -0.0232f));
v1 foreheadY = foreheadL.y;
indicate(foreheadL);
{
v3 verts[] = {jaw, cheek_up, brow_out, foreheadL};
fill_fan( ear_center, verts, alen(verts));
}
fill3(cheek_up,jaw,cheek_low);
if(level1){
hl_block;
draw(bez_line(brow_out, cheek_up));
}
{
radius_scale_block(0.5f);
vv(ear_back, ear_center + V3(0.1697f, 0.0034f, -0.3328f));
vv(ear_low,  ear_center + V3(-0.0639f, -0.5264f, 0.1141f));
v4 radii1 = V4(0.25f, 1.9122f, 0.7259f, 1.3455f);
v4 radii2 = radii_c2(radii1, V2(-0.9383f, 0.f));
bb_unit(ear1,
          ear_center,
          V2(0.2792f, 3.5588f),
          V2(0.1526f, 0.3068f),
          V3(0.3587f, 0.692f, -0.6264f),
          ear_back,
          lp(radii1));
bb_c2(ear2, ear1, V3(), ear_low, lp(radii2));
}
v3 head_back_out = (V3(0.6208f, 0.1421f, -0.4585f));
indicate(head_back_out);
Bezier head_top_out_line = bez_offset(foreheadL,
                                    (V3(0.1239f, 0.1894f, -0.2995f)),
                                    (V3(-0.0634f, 0.3622f, 0.1935f)),
                                    head_back_out);
if(level1){
draw(head_top_out_line);
}
v3 forehead_in = V3y(foreheadY) + V3(0.1445f, 0.072f, 0.8969f);
indicate(forehead_in);
Bezier hair_hline = bez_raw(foreheadL,
                             forehead_in,
                             negateX(forehead_in),
                             negateX(foreheadL));
{
symx_off;
if(level1){
draw(hair_hline);
}
}
vv(head_back_in,V3(0.2242f, 0.6295f, -1.1258f));
Bezier head_top_in_line = bez_bezd_old(forehead_in,
                                    V3(0.f, 0.4276f, -0.1837f),
                                    V2(0.3589f, 0.1402f),
                                    head_back_in);
if(level2){
draw(head_top_in_line);
}
fill_patch( patch_symx(head_top_out_line, head_top_in_line));
vv(behind_ear, ear_center + (V3(-0.183f, -0.3342f, -0.1239f)));
Bezier head_back_out_line;
{
head_back_out_line = bez_offset(head_back_out,
                               (V3(0.f, 0.001f, -0.0779f)),
                               (V3(-0.0535f, 0.197f, -0.6398f)),
                               behind_ear);
if(level2){
draw(head_back_out_line);
}
vv(head_back_in2, (V3(0.4676f, -0.7579f, -0.1264f)));
Bezier head_back_in_line = bez_offset(head_back_in,
                                     (V3(-0.f, -0.3664f, -0.3108f)),
                                     (V3(0.f, -0.4228f, -1.1179f)),
                                     head_back_in2);
Patch head_back = patch_symx(head_back_out_line, head_back_in_line);
if(level2){
draw(head_back_in_line);
}
fill_patch( head_back);
draw(get_uline(head_back, 0.5f), lp((0.9797f),
                                      I4(1,3,3,0)));
}
fill(ear_center, head_top_out_line);
fill_dbez(ear_center, head_back_out, head_back_out_line);
if(fbool(0)){
v3 hair_hline_center = bezier_sample(hair_hline, 0.5f);
v3 hair_hline_half_controls[4];
v3 esuo = brow_out;
v3 control_point = V3((0.2448f), -esuo.y/3.f, (4.f-esuo.z)/3.f);
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
symx_off;
v3 chin_up_center = chinL + (V3(-0.1993f, 0.0993f, 0.0805f));
v3 chin_upL = chin_up_center + (V3x(0.1274f));
indicate(chin_upL);
v3 chin_upR = negateX(chin_upL);
v3 chinR = negateX(chinL);
if(fbool(0)){
Bezier chin_line = bez_raw(chinL, chin_middle, chin_middle, negateX(chinL));
draw(chin_line);
}
{
radius_scale_block(0.25f);
v3 mouth_low_valley = lip_low_center+V3(0.f, -0.0704f, -0.0417f);
{
Line_Params params = profile_visible_transition(0.73f);
draw(bez_line(lip_low_center, mouth_low_valley), params);
draw(bez_line(chin_middle, setx(chin_upL, 0.f)), params);
}
if(level1){
draw(bez_line(mouth_low_valley,chin_middle));
}
v3 chin_point0 = mouth_corner;
v3 control_point = V3(0.f, 
                         (-chin_point0.y + 4*mouth_low_valley.y)/3.f,
                         (-chin_point0.z + 4*mouth_low_valley.z)/3.f);
Bez mouth_low_valley_line = bez_raw(chin_point0,
                                       control_point,
                                       negateX(control_point),
                                       negateX(chin_point0));
if(level1){
draw(mouth_low_valley_line);
}
{
symx_on;
fill3(lip_low_center,mouth_corner,mouth_low_valley);
fill4(chin_middle, chinL, mouth_corner, mouth_low_valley);
}
}
}
vv(head_neck_junction, V3(0.f, -1.1949f, 0.3195f));
{
symx_off;
draw(bez_line(chin_middle, head_neck_junction));
fill3(head_neck_junction, chinL, jaw);
fill3(head_neck_junction, chinL, negateX(chinL));
}
{
radius_scale_block(0.3826f);
line_color_lightness(1.5096f);
vv(a,V3(0.2651f, -0.5154f, 0.8715f));
vv(b,V3(0.5636f, -0.3601f, 0.5385f));
draw(bez_unit2(a,
                 V4(0.f, 0.1801f, 0.0364f, 0.3141f),
                 V3(0.866f, 0.f, 0.5f),
                 b),
       small_to_big());
vv(c,V3(0.5266f, -0.6385f, 0.68f));
draw(bez_unit2(b, V4(), V3(), c),
       big_to_small());
}
b32 hair_on = fbool(1);
if(hair_on){
lp_block(nslice_per_meter, 1.5162f*128.f);
symx_off;
radius_scale_block(0.5489f);
lp_block(radii, fval4(0.5f, 1.f, 1.f, 0.25f));
v1 hairY = loomis_unit;
vv(hair_root, V3(0.f, 1.087f, 0.f));
vv(bang_root, bezier_sample(hair_hline, 0.5f));
v1 flutter_period = 2.75f;
b32 should_flutter;
v1 time;
{
v1 time0 = animation_time / flutter_period;
should_flutter = ((i32)time0 % 2 == 0);
time = cycle01_positive(time0);
}
Animation_Old ani_value;
auto ani = &ani_value;
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
v1 thair = 0.f;
if(should_flutter){
if(is_right()){
thair = get_animation_value(ani, time+0.06f);
}else {
thair = get_animation_value(ani, time);
}
}
vv(bang_midpoint, V3(0.f, -0.0929f, 1.042f));
{
Bezier bang_vline = bez_unit2(bang_root,
                                 V4(0.f, 0.2629f, 0.1602f, 0.3068f),
                                 V3(0.f, 0.f, 1.f),
                                 bang_midpoint);
draw(bang_vline,profile_visible_transition(0.4036f));
}
v3 bang_tip;
{
v3 c = V3(0.0537f, 0.1334f, -0.0695f);
va(bang_tip, V3(0.7418f, -0.0689f, 0.6115f)+thair*c);
}
Bez bang_vline2 = bez_bezd_old(bang_root, 
                             V3(0.3419f, 0.0757f, 0.129f),
                             V2(0.1345f, 0.1639f),
                             bang_tip);
{
v3 a = V3(0.0172f, -0.0113f, -0.0825f);
v3 b = V3(-0.0664f, 0.0812f, 0.0059f);
bang_vline2[1] += thair*a;
bang_vline2[2] += thair*b;
}
Bez bang_hline = bez_offset(bang_midpoint,
                              V3(0.1302f, -0.0069f, -0.0297f),
                              V3(0.0001f, 0.2158f, 0.056f),
                              bang_tip);
{
v3 a = V3(0.0000f, -0.1217f, 0.0000f);
v3 b = V3(0.0467f, 0.0385f, 0.0000f);
bang_hline[1] += thair*b;
bang_hline[2] += thair*a;
}
{
draw(bang_vline2);
draw(bang_hline, (0.5176f) * painter.line_params.radii);
}
v3 hair_main_tip = V3(0.7814f, -1.6599f, -0.6047f);
Bez vline = bez_bezd_old(hair_root,
                       V3(0.1414f, 0.2764f, -0.471f),
                       V2(0.3956f, 0.3102f),
                       hair_main_tip);
{
symx_on;
draw(vline);
}
v3 hcontrol = V3(0.1291f, -0.0058f, -0.2506f);
Bez connecting = bez_offset(hair_main_tip,
                              hcontrol,
                              negateX(hcontrol),
                              negateX(hair_main_tip));
draw(connecting, (0.5f)*painter.line_params.radii);
Bezier hairline_side = bez_offset(bang_root,
                                    V3(0.5271f, -0.0051f, 0.0478f),
                                    V3(0.0671f, 0.1143f, 0.3924f),
                                    ear_center);
{
symx_on;
draw(hairline_side);
}
{
symx_on;
Bez over1 = bez_bezd_old(hair_root,
                        V3(0.5313f, 0.0177f, 0.1293f),
                        V2(0.1839f, 0.232f),
                        bezier_sample(hairline_side, (0.3991f)));
Bez over2 = bez_bezd_old(hair_root,
                        V3(0.3694f, 0.2549f, -0.0512f),
                        V2(0.0175f, 0.2904f),
                        ear_center);
draw(over1);
draw(over2, lp_alignment_min((0.4764f)));
}
{
Bez line = bez_unit2(hair_root,
                        V4(0.f, 0.2255f, 0.3279f, 0.2047f),
                        (V3(0.f, 1.f, 0.f)),
                        bang_root);
draw(line);
}
}
if(painter.show_grid || show_loomis_ball){
hl_block_color(argb_dark_blue);
symx_off;
if(painter.show_grid){
b32 camera_frontal = almost_equal(absolute(painter.camera.z.z), 1.f, 1e-2f);
b32 camera_profile = almost_equal(absolute(painter.camera.z.x), 1.f, 1e-2f);
{
hl_block_color(argb_blue);
radius_scale_block(0.33f);
v1 face_minY = -2.f*loomis_unit;
v1 unit = 0.25f * loomis_unit;
i32 yi_min = (-7);
i32 yi_max = (8);
if(camera_frontal){
v1 z = faceZ;
{
for_i32(xi, 0, 5)
      {
v1 x = unit*v1(xi);
draw_line( V3(x, 1.f, z), V3(x, face_minY, z));
}
}
{
v1 xmax = (0.9369f);
for_i32(yi, yi_min, yi_max)
      {
v1 y = unit*v1(yi);
draw_line( V3(0.f, y, z), V3(xmax, y, z));
}
}
}
if(camera_profile){
v1 x = 0.f;
for_i32(zi, (-11), (2))
     {
v1 z = faceZ+unit*v1(zi);
draw_line( V3(x,1.f,z), V3(x,face_minY,z));
}
for_i32(yi, yi_min, yi_max)
     {
v1 y = unit*v1(yi);
draw_line(V3(x,y,(-1.8208f)),
                V3(x,y,(1.7172f)));
}
}
}
{
radius_scale_block((0.5f));
v1 body_minY = 1.f-6*head_unit;
v1 unit = face_sideX;
if(camera_frontal){
{
for_i32(xi, 0, 5)
      {
v1 x = unit*v1(xi);
draw_line(V3(x, 1.f, 0.f), V3(x, body_minY, 0.f));
}
}
{
v1 xmax = (2.8346f);
for_i32(yi, -20, 4)
      {
v1 y = unit*v1(yi);
draw_line(V3(0.f, y, 0.f), V3(xmax, y, 0.f));
}
}
{
lp_block(color, argb_blue);
v1 y = (-2.2334f);
draw_line(V3(0.f, y, 0.f), V3(0.7f*head_unit, y, 0.f));
}
}
if(camera_profile){
v1 x = 0.f;
for_i32(xi, -4, 2)
     {
v1 z = faceZ+unit*v1(xi);
draw_line( V3(x,1.f,z), V3(x,body_minY,z));
}
for_i32(yi, -20, 4)
     {
v1 y = unit*v1(yi);
draw_line( V3(x,y,(-1.8208f)),
                V3(x,y,(1.7172f)));
}
}
}
if(fbool(1)){
lp_block(color, argb_red);
if(camera_frontal){
v1 r = 0.5f*head_unit;
for_i32(yi,-6,1)
     {
v1 y = 1.f + v1(yi)*head_unit;
v3 left = V3(r,y,0);
draw_line( V3y(y), left);
}
}
if(camera_profile){
for_i32(yi,-6,1)
     {
v1 y = 1.f + v1(yi)*head_unit;
draw_line( V3y(y), V3(0,y,-1.f));
}
}
}
}
if(show_loomis_ball){
argb line_color = current_line_cparams().color;
if(painter.show_grid){
line_color = argb_silver;
}
lp_block(color, line_color);
{
bs_circle(loomis_side_center, V3x(loomis_side_radius));
}
{
draw(bez_line(V3z(faceZ), setz(chin_middle, faceZ)));
bs_circle(V3(), V3z(1));
bs_circle(V3(), V3y(1));
bs_circle(V3(), V3x(1));
}
}
}
Head head_obj;
{
vv(trapezius_head, (V3(0, noseY, head_back_out.z) + 
                      V3(0.2824f, 0.0086f, -0.323f)));
#define head_export(vertex)  head_obj.vertex = vertex;
  
macro_head(head_export);
#undef head_export
 
}
return head_obj;
}
function void
render_character(Pose *pose, v1 animation_time){
painter.shade_color = compute_fill_color(0.094014f);
if(fbool(0)){
painter.shade_color = painter.line_params.fill.color;
}
b32 show_body = fbool(1);
v1 arm_ry = head_unit_world*(0.5302f);
macro_torso(macro_world_declare);
for_i32(lr_index,0,2)
 {
set_in_block(painter.lr_index, lr_index);
Head head;
{
bone_block(Bone_Head);
head = render_head(pose, animation_time);
render_eyes(pose);
}
Pelvis pelvis_obj;
{
bone_block(Bone_Pelvis);
pelvis_obj = render_pelvis(*pose);
}
bone_block(Bone_Torso);
Torso torso_obj;
{
set_in_block(painter.painting_disabled,
                !show_body || painter.painting_disabled);
torso_obj= render_torso(pose, pelvis_obj, head);
}
v3 elbow_offset = (V3(0.0107f, -1.864f, -0.1208f));
v3 elbow_up_out = elbow_offset+(V3(0.0527f, 0.9446f, -0.2298f));
{
bone_block(Bone_Arm);
Arm arm_obj = render_arm(pose, torso_obj, elbow_up_out);
{
bone_block(Bone_Forearm);
Forearm forearm_obj = render_forearm(arm_obj, elbow_offset, elbow_up_out);
{
bone_block(Bone_Hand);
render_hand(forearm_obj);
}
}
}
}
}
function void
render_reference_images(b32 full_alpha){
i32 preset = get_preset();
if(preset >= 3){
bone_block(Bone_References);
if(camera_is_right()){
v3 center = (V3(-0.2112f, -4.5306f, -0.2689f));
v1 width = (0.7966f);
char *filename = "G:/My Drive/Art/arm medial.jpg";
draw_image(filename, center, V3z(width), V3y(1.f),
              (0.5f));
}
if(camera_is_front()){
const i32 start = 4;
struct Reference_Image
   {
    char *filename;
    v3 center;
    v1 width;
    v1 alpha;
    b32 reflected;
   };
Reference_Image references[] =
   {
    {// NOTE: full-body MM skeleton
     .filename  = "G:/My Drive/Art/skeletal meat outline.JPG",
     .center    = (V3(1.7822f, -6.9027f, -0.2689f)),
     .width     = fval(1.8194f),
     .alpha     = fval(0.3033f),
     .reflected = false,
    },
    {// NOTE: full-body MM diagram
     .filename  = "G:/My Drive/Art/AM full body muscle front.JPG",
     .center    = (V3(1.8527f, -6.8952f, -0.2689f)),
     .width     = fval(1.8695f),
     .alpha     = fval(0.3033f),
     .reflected = false,
    },
    {
     .filename="G:/My Drive/Art/AM arm front.JPG",
     .center  =(V3(1.2723f, -4.8425f, -0.2689f)),
     .width   =fval(1.3938f),
     .alpha   =fval(0.1421f),
     .reflected = true,
    },
    {
     .filename="G:/My Drive/Art/loomis 6 heads.JPG",
     .center  = (V3(1.0471f, -6.2154f, -0.2689f)),
     .width   = fval(1.0668f),
     .alpha   = fval(0.1421f),
     .reflected = true,
    },
    {
     .filename="G:/My Drive/Art/hpc.JPG",
     .center  = (V3(-0.5712f, -6.2775f, -0.2689f)),
     .width   = fval(2.6661f),
     .alpha   = fval(0.4221f),
    },
   };
i1 ref_index = preset - 4;
if(ref_index >= 0 &&
       ref_index < alen(references)){
auto &ref = references[ref_index];
v3 x_axis = V3x(ref.width);
if(ref.reflected){
x_axis.x *= -1.f;
}
v1 alpha = full_alpha ? 1.f : ref.alpha;
draw_image(ref.filename, ref.center, x_axis, V3y(1.f), alpha);
}
}
if(camera_is_back()){
if(preset == 3){
v3 center = (V3(2.8062f, -4.0188f, -0.2689f));
v1 width = fval(2.4645f);
char *filename = "G:/My Drive/Art/left arm back.PNG";
draw_image(filename, center, V3x(-width), V3y(1.f),
               fval(0.3033f));
}else if(preset == 4){
v3 center = (V3(2.1904f, -3.8928f, -0.2689f));
v1 width = fval(2.4645f);
char *filename = "G:/My Drive/Art/arm back bone.PNG";
draw_image(filename, center, V3x(-width), V3y(1.f),
               fval(0.3033f));
}
}
if(camera_is_left()){
if(preset >= 4){
v3 center = (V3(-0.2112f, -3.9204f, 0.5898f));
v1 width = fval(1.1454f);
char *filename = "G:/My Drive/Art/arm left.PNG";
i1 sel = fval(1);
v3 x = V3z(1);
if(sel == 1){
center   = (V3(-0.2112f, -4.983f, 0.0321f));
width    = -fval(0.7916f);
filename =  "G:/My Drive/Art/AM arm profile 2.JPG";
}
v1 alpha = fval(0.299f);
if(full_alpha){
alpha = 1.f;
}
draw_image(filename, center, -width*x, V3y(1.f),
               alpha);
}
}
}
}
function void
compute_bones_from_pose(Modeler *m, Arena *scratch, Pose *pose){
scap_sock_top = V3(1.0676f, -1.6393f, -0.0102f);
thumb_kbot = V3(0.2669f, -2.6071f, -0.2208f);
arrayof<Bone *>stack;
init_static(stack, scratch, 16);
Bone null_bone = {.xform=mat4i_identity};
stack.push(&null_bone);
#define bone_blockm(...) \
defer_block(push_bone_inner(*m, stack, lr_index, __VA_ARGS__), stack.pop())
 
 
v1 torso_scale = head_radius_world*fval(1.2484f);
for_i32(lr_index,0,2){
{
const v1 head_theta_max = 0.15f;
const v1 head_phi_max   = 0.125f;
const v1 head_roll_max  = 0.125f;
v1 head_theta = pose->thead_theta * head_theta_max;
v1 head_phi = pose->thead_phi   * head_phi_max;
v1 head_roll = pose->thead_roll  * head_roll_max;
v3 rotation_pivot = V3(0.f, -1.1466f, 0.3598f);
mat4i headT = (mat4i_scale(head_radius_world) *
                  mat4i_rotate_tpr(head_theta, head_phi, head_roll, rotation_pivot));
if(lr_index){
headT = negateX(headT);
}
bone_blockm(Bone_Head, headT){
v3 eyeO = V3(0.1953f, -0.2348f, 0.8954f);
v3 eye_scale = V3(0.8324f, 0.9882f, 0.92f);
mat4i eyeT = mat4i_translate(eyeO)*mat4i_scales(eye_scale);
v3 eye_in = eyeT*V3(-0.0282f, 0.f, -0.1236f);
v3 eye_out = eye_in + V3(2.f * eye_in.x+(-0.03f), 0, -0.154f);
eyeball_center = eyeT*V3(0.1531f, 0.0101f, -0.3179f);
mat4i to_eyeball = mat4i_translate(eyeball_center);
eyeball_radius = (eye_out.x - eye_in.x)*0.4526f;
mat4i eyeball_scale = mat4i_scale(eyeball_radius);
mat4i rot;
{
v1 eyeball_theta = 0.13f*(pose->teye_theta);
if(painter.lr_index){
eyeball_theta *= -1;
}
rot = mat4i_rotateY(eyeball_theta);
}
mat4i eyeballT = to_eyeball*eyeball_scale*rot;
bone_blockm(Bone_Eyeball, eyeballT);
}
}
{
v1 head_topY = head_radius_world;
v3 translate = V3(0.f,
                     head_topY - 3.2f * head_unit_world, 
                     (0.067f));
mat4i pelvisT = mat4i_translate(translate) * mat4i_scale(head_radius_world);
if(lr_index){
pelvisT = negateX(pelvisT);
}
bone_blockm(Bone_Pelvis, pelvisT);
}
mat4i torsoT = mat4i_scale(torso_scale);
if(lr_index){
torsoT = negateX(torsoT);
}
bone_blockm(Bone_Torso, torsoT){
mat4i armT;
{
v1 arm_ry = head_unit_world*(0.5302f);
v1 scale = arm_ry / torso_scale;
v3 translate = scap_sock_top + V3(0.0764f, -0.9755f, 0.2612f);
v1 roll = -pose->tarm_abduct;
mat4i rotateT = mat4i_rotate_tpr(0, 0, roll);
arm_rotation_pivot = (V3(0.0551f, 0.8153f, -0.1578f));
armT = trs_pivot_transform(translate, rotateT, scale, arm_rotation_pivot);
}
bone_blockm(Bone_Arm, armT){
const v1 arm_bend_max = fval(-0.5f);
v1 forearm_turn = pose->tarm_bend * arm_bend_max;
v3 translate = (V3(0.3231f, -0.0051f, -0.117f));
mat4i rotate = mat4i_rotateX(forearm_turn);
forearm_rotation_pivot = (V3(-0.2068f, -1.0341f, -0.0909f));
mat4i forearmT = trs_pivot_transform(translate, rotate, 1.f,
                                         forearm_rotation_pivot);
bone_blockm(Bone_Forearm, forearmT){
palm_in = (V3(-0.0112f, -2.6757f, 0.0176f));
mat4i handT = mat4i_translate(palm_in +
                                   (V3(0.0112f, 2.5863f, 0.2802f)));
bone_blockm(Bone_Hand, handT){
mat4i thumbT;
{
v1 tadduct = fval(0.f);
thumbT = mat4i_rotateZ(-tadduct, thumb_kbot);
}
bone_blockm(Bone_Thumb, thumbT);
v1 bot_tbend = fval(0.f);
i32 const finger_count = 4;
knuckle_ts = v4{
       fval(0.1236f),
       fval(0.3668f),
       fval(0.6424f),
       fval(0.8955f),
      };
kline_in  = (V3(-0.1346f, -2.987f, -0.3612f));
kline_out = (V3(0.3755f, -3.0799f, -0.3612f));
knuckle_line = bez_unit(kline_out, V2(-0.0263f, 0.259f),
                              V2(0,0),
                              V3y(-1), kline_in);
for_i32(index,0,finger_count) {
knuckle_tops[index] = bezier_sample(knuckle_line, knuckle_ts[index]);
}
v3 finger_vecs_[4] = {
       -fvecy(0.5077f), //index
       -fvecy(0.5619f), //middle
       -fvecy(0.5247f), //ring
       -fvecy(0.4449f), //pinky (NOTE: lines up with the ring's top knuckle)
      };
copy_array_src(finger_vecs, finger_vecs_);
for_i32(ifinger,0,finger_count) {
v3 k = knuckle_tops[ifinger];
bone_blockm(make_bone_id(Bone_Bottom_Phalanx, ifinger),
                   mat4i_rotateX(-bot_tbend, k)) {
v1 mid_tbend = fval(0.f);
v3 vec = finger_vecs[ifinger];
v3 kmid = k+0.5f*vec;
bone_blockm(make_bone_id(Bone_Mid_Phalanx, ifinger),
                    mat4i_rotateX(-mid_tbend, kmid)){
v1 top_tbend = fval(0.f);
v3 ktop = k+0.75f*vec;
bone_blockm(make_bone_id(Bone_Top_Phalanx, ifinger),
                     mat4i_rotateX(-top_tbend, ktop));
}
}
}
}
}
}
}
}
{
b32 lr_index = 0;
bone_blockm(Bone_References, mat4i_scale(head_radius_world));
}
#undef bone_blockm

}
xfunction Pose
driver_animate(driver_animate_params){
Pose pose;
{
Temp_Memory_Block temp(scratch);
Movie_Shot *shot = push_struct(scratch, Movie_Shot, true);
shot->anime_time = anime_time;
{
i32 sel = fvali(2);
if(false){
}else if(sel == 0){
shot_go(shot, movie_shot_blinking);
}else if(sel == 1){
shot_go(shot, movie_shot_head_tilt);
}else if(sel == 2){
shot_go(shot, movie_shot_arm);
shot_go(shot, movie_shot_blinking);
}
}
pose = shot->out_pose;
}
compute_bones_from_pose(m, scratch, &pose);
return pose;
}
xfunction void
render_movie(render_movie_params){
i32 viz_level = 0;
auto viewport = painter.viewport;
switch(viewport->preset){
case 1: viz_level = 1; break;
case 2: viz_level = 2; break;
}
argb background_color;
{
v3 background_hsv = fval3(0.069f, 0.0648f, 0.5466f);
v4 background_v4 = srgb_to_linear(hsv_to_srgb(background_hsv));
background_color = argb_pack(background_v4);
}
painter.show_grid = viewport->preset >= 3;
{
auto c = render_config;
auto &camera = painter.camera;
c->viewport_id     = viewport->index+1;
c->view_from_world = painter.view_from_world;
c->world_from_cam  = camera.transform;
c->focal_length    = camera.focal_length;
c->near_clip       = camera.near_clip;
c->far_clip        = camera.far_clip;
c->background      = background_color;
}
u64 start_cycle = ad_rdtsc();
draw_cycle_counter   = 0;
bs_cycle_counter     = 0;
argb default_fill = background_color;
if(level1){
default_fill = argb_lightness(default_fill, fval(0.7797f));
}
v1 default_line_radius_min = fval(0.5089f);
v1 default_line_end_radius = default_line_radius_min;
if(fbool(1)){
default_line_end_radius = i2f6(fval(2));
}
{
Painter &p = painter;
auto &m = *p.modeler;
p.symx              = true;
p.line_params.fill.color = default_fill;
p.fill_depth_offset = millimeter * 1.f;
p.line_params.visibility = 1.0f;
{
Common_Line_Params cp = get_line_cparams_list(m)[0];
cp.radii = V4(default_line_radius_min, 
                 1.f,
                 i2f6(fval(5)),
                 default_line_end_radius);
argb default_line_color = argb_gray(srgb_to_linear1(fval(0.3389f)));
if(level1){
default_line_color = argb_dark_blue;
}
cp.color = default_line_color;
push_line_cparams(cp);
}
p.viz_level            = viz_level;
p.ignore_radii         = viz_level!=0;
p.ignore_alignment_min = viz_level!=0;
init_static(p.bone_stack, arena, 16);
auto &bones = get_bones(m);
p.bone_stack.push(&bones[0]);
push_view_vector(&p, v3{});
}
render_character(pose, anime_time);
if(debug_frame_time_on){
u64 end_cycle = ad_rdtsc();
u32 render_total = u32(end_cycle - start_cycle);
DEBUG_VALUE(render_total);
DEBUG_VALUE(draw_cycle_counter);
DEBUG_VALUE(u32(bs_cycle_counter));
}
render_reference_images(references_full_alpha);
if(0){
test_speed_block(cy_per, 16, bez_v3v2(v3{0,0,0}, v3{0,0,0}, v2{0,0}, v3{0,0,0}););
}
}
#undef indicate
#undef indicate0
#undef vv
#undef va
#undef X_comma_list
//~EOF