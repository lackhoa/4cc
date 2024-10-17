//NOTE Generated at C:\Users\vodan\4ed\code/meta_klang.cpp:176:
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
forearm;
import_vertices(forearm.verts, forearm_obj.verts, ot, forearmT, forearm_vert_count);
vv(palm_base_in, (V3(-0.0476f, -2.6071f, -0.2208f)));
indicate(thumb_kbot);
palm_base_line = bez_unit(thumb_kbot, V2(0.f, 0.3416f),
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
tb_vec = fvecy(0.06f);
for_i32(index,0,4){
knuckle_bots[index] = knuckle_tops[index]+tb_vec;
}
}
for_i32(index,0,finger_count){
draw_line(knuckle_tops[index], knuckle_bots[index]);
}
{
middle_knuckle_bot = knuckle_bots[1];
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
offset = -fvecy(0.0818f);
for_i32(index,0,finger_count+1) {
finger_bases[index] = offset+bezier_sample(knuckle_line,
                                               finger_base_ts[index]);
indicate(finger_bases[index]);
}
}
top_tbend = fval(0.f);
for_i32(ifinger,0,finger_count){
bone_block(make_bone_id(Bone_Bottom_Phalanx, ifinger));
k = knuckle_tops[ifinger];
vec = finger_vecs[ifinger];
xvec = fvecx(0.0367f);
vv(kmid, k+0.5f*vec);
ikmid = kmid-xvec;
okmid = kmid+xvec;
{
knuckle_control = V2(0.4816f, 0.329f);
draw(bez_unit(finger_bases[ifinger], V2(0.f, 0.5811f),
                  knuckle_control, V3x(-1), okmid),
         lp(I4(3,1,1,1)));
draw(bez_unit(finger_bases[ifinger+1], V2(-0.3782f, 0.3545f),
                   knuckle_control, V3x(1), ikmid),
         lp(I4(3,3,3,1)));
if (ifinger == 0)
    {
draw(bez_v3v2(kline_out, (V3(0.0001f, 0.f, 0.f)),
                   V2(0,0), finger_bases[0]),
          lp(I4(0,1,1,3)));
}
if (ifinger == finger_count-1)
    {
draw(bez_v3v2(finger_bases[ifinger+1], (V3(-0.0049f, 0.f, 0.f)),
                   V2(0,0), kline_in),
          lp(I4(2,3,0,1)));
}
}
{
bone_block(make_bone_id(Bone_Mid_Phalanx, ifinger));
vv(ktop, k+0.75f*vec);
iktop = ktop-xvec;
oktop = ktop+xvec;
{
control = V2(-0.0473f, 0.7281f);
draw(bez_unit(ikmid, control, V2(), V3x(1), iktop),
          lp(I4(3,3,3,1)));
draw(bez_unit(okmid, control, V2(), V3x(-1), oktop),
          lp(I4(3,0,2,3)));
}
tip = k+vec;
{
bone_block(make_bone_id(Bone_Top_Phalanx, ifinger));
tip_control = V2(-0.3298f, 1.1379f);
knuckle_control = V2(0.2954f, -0.1896f);
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
torso;
import_vertices(torso.verts, torso_obj.verts, ot, torsoT, torso_vert_count);
tarm_bend = (pose->tarm_bend);
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
delt_back;
Bez delt_cross, delt_back_line, delt_bot_line, delt_vfront;
delt_back_point = torso.scap_delt;
{
vv(delt_top, torso.shoulder+(V3(0.0328f, 0.f, 0.f)));
vv(delt_top_back, delt_top+(V3(-0.0109f, -0.0068f, -0.2147f)));
delt_hline = bez_unit2(delt_top,
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
delt_vmid = bez_unit2(delt_top,
                            (V4(-0.2234f, 0.2079f, 0.015f, 0.1626f)),
                            (V3(0.9999f, 0.f, 0.0112f)),
                            delt_bot);
delt_vback = bez_unit2(delt_top_back,
                             V4(-0.13f, 0.1784f, -0.1368f, 0.2863f),
                             V3(0.6431f, 0.f, -0.7657f),
                             delt_bot);
va(delt_back, V3(-0.0076f, 0.2268f, -0.4244f));
fill3(delt_back, delt_bot, delt_top_back);
draw(delt_vmid, lp_alignment_min((0.8202f)));
{
params = lp_alignment_min((0.6061f));
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
pectoral_arm = V3(-0.2013f, 0.475f, 0.1047f);
{
line = bez_unit2(pectoral_arm,
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
if (level1) {
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
front_out = bez_unit2(bicep_out_top,
                             (V4(-0.0571f, 0.0667f, 0.f, 0.f)),
                             (V3(0.7886f, 0.f, 0.6149f)),
                             bicep_out_bot);
vv(hinge_out, bezier_sample(front_out, fval(0.6758f)));
va(bicep_in_top, bicep_out_top+(V3(-0.211f, 0.0922f, 0.0631f)));
va(bicep_in_bot, bicep_out_bot+(V3(-0.148f, 0.0022f, 0.0005f)));
fill3(brachio_a, hinge_out, bicep_out_bot);
front_in = bez_unit2(bicep_in_top,
                            (V4(0.f, 0.0397f, 0.0201f, 0.1254f)),
                            (V3(-0.8434f, 0.f, 0.5373f)),
                            bicep_in_bot);
draw(front_in, lp(I4(0,0,0,4)));
fill_dbez(front_in, front_out);
vv(bicep_side_top, bezier_sample(delt_vfront, fval(0.9539f)));
vv(bicep_side_bot, bicep_out_bot + (V3(0.1103f, 0.1089f, -0.084f)));
side_out = bezd_len(bicep_side_top, (V3(-0.f, 0.f, -0.0502f)),
                           V2(0,0), bicep_side_bot);
draw(side_out, i2f6(I4(4,0,0,0)));
fill3(pectoral_arm, bicep_in_top, bicep_out_top);
fill3(pectoral_arm, torso.delt_collar, bicep_out_top);
vv(bicep_side_top2, bezier_sample(delt_cross, fval(0.7406f)));
vv(bicep_side_bot2, bicep_out_bot+(V3(-0.0601f, 0.f, 0.f)));
{
alignment = fval(0.9169f);
view_vector = get_view_vector();
bicep_side_line2 = bez_unit(bicep_side_top2,
                                     V2(0.f, 0.0537f),
                                     V2(0,0),
                                     V3z(1.f),
                                     bicep_side_bot2);
drawn = draw(bicep_side_line2, lp(alignment, I4(4,3,1,0)));
if (!drawn) {
draw(front_out, lp(I4(5,0,0,0)));
}
fill_bez(bicep_side_line2);
}
fill4(bicep_side_top, bicep_side_top2, bicep_side_bot2, bicep_out_bot);
{
ok = get_view_vector().x < fval(0.07f);
radii = I4(6,6,0,0);
if (get_view_vector().x > fval(-0.20f) ) {
radii.y = fvali(0);
}
if (ok)
    {
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
tricep_cross_line = bez_v3v2(tricep_wedge, (V3(0.0474f, -0.0292f, 0.0884f)),
                                  V2(0,0), elbow_up_out);
draw(tricep_cross_line, i2f6(I4(0,0,4,6)));
fill_bez(tricep_cross_line);
vv(brachio_humerus2, bezier_sample(tricep_cross_line, fval(0.4296f)));
arm_obj;
{
#define export_(vertex)  arm_obj.vertex = vertex;
  
macro_arm(export_);
#undef export_
 
}
return arm_obj
}
function Forearm
render_forearm(Arm &arm_obj, v3 elbow_offset, v3 elbow_up_out){
mat4i &ot   = p_current_world_from_bone();
mat4i &armT = p_mom_bone_xform();
arm;
import_vertices(arm.verts, arm_obj.verts, ot, armT, arm_vert_count);
send_vert(palm_in);
{
hl_block;
l492;
params = lp((0.5f));
down_common;
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
l500;
in_c;
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
elbow_radii = fval(I4(4,3,3,2));
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
l576 = bez_v3v2(brachio_c, (V3(-0.0479f, 0.0162f, 0.0042f)),
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
v2189 = (V3(-0.2725f, -0.8924f, -0.251f));
draw(bez_v3v2(v2189,
               (V3(0.f, 0.1288f, 0.1481f)),
               V2(0,0),
               (V3(0.1923f, -1.8531f, -0.198f))));
l618 = bez_v3v2(arm.external_condyle, (V3(0.0833f, 0.3209f, -0.0449f)),
                     V2(0,0), middle_finger_meeter);
draw(l618);
vv_sample(v612, l618, 0.7612f);
fill3(v612, arm.external_condyle, brachio_c);
fill3(v612, v593, radius_back);
fill3(v612, brachio_c, v593);
forearm_obj;
#define export_(vertex)  forearm_obj.vertex = vertex;
 
macro_forearm(export_);
#undef export_
 
return forearm_obj
}
function Torso
render_torso(Pose *pose, Pelvis pelvis_obj, Head head_obj){
view_vector_block((V3(0.f, -2.3176f, 0.3862f)));
mat4i &ot = p_current_world_from_bone();
mat4i &pelvisT = get_bone_xform(Bone_Pelvis);
mat4i &headT   = get_bone_xform(Bone_Head);
head_unit = get_column(ot.inv,1).y * head_unit_world;
head;
import_vertices(head.verts, head_obj.verts, ot, headT, head_vert_count);
pelvis;
import_vertices(pelvis.verts, pelvis_obj.verts, ot, pelvisT, pelvis_vert_count);
vv(shoulder, V3(1.1765f, -1.6108f, 0.0573f));
send_vert(scap_sock_top);
mat4i &armT = get_bone_xform(Bone_Arm);
arm_local = ot.inv * armT;
vv(shoulder_in, V3(0.4819f, -1.3302f, -0.4461f));
sternum = V3(0.f, -1.6696f, 0.2691f);
vv(delt_collar, shoulder + V3(-0.3387f, -0.02f, 0.1371f));
vv(sternumL, sternum + V3x((0.0919f)));
{
neck_front_vline = bez_bezd_old(head.head_neck_junction,
                                     V3(-0.f, 0.f, -0.0968f), 
                                     V2(0.f, 0.2897f), sternum);
draw(neck_front_vline, profile_visible_transition(0.5f));
neck_back_line = bez_offset(head.trapezius_head,
                                     V3(-0.1529f, -0.1932f, 0.2857f),
                                     V3(-0.1937f, 0.0701f, 0.0969f),
                                     shoulder_in);
draw(neck_back_line, I4(0,1,3,0));
{
symx_off;
fill_dbez(neck_back_line, negateX(neck_back_line));
}
collar_in = bez_offset(sternumL,
                                V3(0.2915f, -0.0448f, -0.0087f),
                                V3(-0.1047f, -0.0248f, 0.1171f),
                                delt_collar);
collar_out = bez_offset(delt_collar+(V3(-0.0957f, 0.0021f, 0.016f)),
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
sterno;
{
sterno = bez_offset(head.behind_ear,
                        V3(-0.107f, -0.2444f, 0.2159f),
                        V3(0.0786f, 0.1842f, -0.1674f),
                        sternumL);
{
patch = (V3(-0.1658f, 0.f, -0.0888f));
thead_theta = pose->thead_theta;
tcontract = (is_left() ?
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
rib_midY = (shoulder.y - 0.65f*head_unit + (-0.0775f));
va(rib_mid, V3(0.f, rib_midY, (0.8061f)));
if(is_left()){
draw(bez_bezd_old(sternum,
                     V3(0.f, 0.0027f, 0.2265f),
                     V2(0.f, 0.0935f),
                     rib_mid));
}
vv(ribL, rib_mid+(V3(0.559f, -0.7158f, -0.2772f)));
rib_in = rib_mid+V3x((0.0905f));
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
latis_arm_line = bez_unit(rib_back, V2(0.1935f, 0.0429f), 
                                V2(0.151f, 0.0235f),
                                V3(1.f, 0.f, -0.f),
                                latis_arm);
draw(latis_arm_line, lp_alignment_min(0.7f));
}
scap_sock_bot;
{
vv(trap_bot, V3(0.f, -3.1104f, -0.1081f));
if(level2)
  {
hl_block;
draw(bez_unit2(scap_delt,
                  V4(0.f, 0.1307f, 0.f, 0.0566f),
                  V3(0.5328f, 0.0057f, -0.8462f),
                  shoulder));
}
fill3_symx(scap_delt, shoulder_in);
fill3(shoulder_in, shoulder, scap_delt);
fill3_symx(trap_bot,scap_delt);
hip_back_line = bez_unit(rib_back,
                               V2(0.f, 0.1135f),
                               V2(0.01f, 0.1345f),
                               V3(-0.5382f, 0.f, 0.8428f),
                               pelvis.bikini_up_back);
draw(hip_back_line);
vv(back_archL, bezier_sample(hip_back_line, (0.5606f)));
draw(bez_v3v3(trap_bot, V3(0.235f, 0.3261f, -0.0079f),
                V3(0.0805f, 0.f, 0.0552f), back_archL));
scap_bot;
{
hl_block;
disabled = is_right();
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
back_midline = bez_unit2(diamond_up,
                               (V4(-0.0605f, 0.1215f, 0.f, 0.f)),
                               V3z(-1),
                               trap_bot);
draw(back_midline, V4(0.3513f, 0.9763f, 0.6129f, 1.1402f));
vv(diamond_low, bezier_sample(back_midline, (0.2302f)));
vv(diamondL,    diamond_up+(V3(0.0856f, -0.2026f, -0.0719f)));
draw( bez_line(diamond_up, diamondL));
draw( bez_line(diamond_low, diamondL));
trap_vline = bez_unit2(scap_delt, 
                             (V4(0.f, 0.0667f, 0.f, 0.f)), 
                             (V3(-0.379f, 0.f, -0.9254f)),
                             bezier_sample(back_midline, (0.9154f)));
draw(trap_vline, (V4(0.7542f, 1.f, 1.f, 0.2694f)));
vv(trap_weird_point, bezier_sample(trap_vline, (0.5728f)));
trap_weird_line = bez_unit2(trap_weird_point,
                                  (V4(0.0369f, 0.1268f, 0.f, 0.f)),
                                  (V3(-0.8654f, 0.f, -0.5011f)),
                                  rib_back);
draw(trap_weird_line, V4(0.75f, 1.f, 1.0735f, 0.5f));
}
v3 chest_back, pectoral_torso;
{
nippleY = shoulder.y-0.4f*head_unit;
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
latis_side = bez_unit(chest_back, V2(-0.1341f, 0.0768f),
                            V2(0,0), 
                            (V3(0,0,-1)),
                            rib_back);
draw(latis_side,lp_alignment_min(0.7f));
}
if(is_left()){
draw(bez_parabola(rib_mid, V3z(-0.0952f), pelvis.navel));
}
torso_obj = {macro_torso(X_comma_list)};
return torso_obj
}
function Pelvis
render_pelvis(Pose &pose){
head_topY = head_radius_world;
mat4i &ot = p_current_world_from_bone();
navelY = (ot.inv * V3y(head_topY - 2.5f * head_unit_world)).y;
vv(navel, V3y(navelY) + V3z(0.271f), 0);
vv(crotch, V3());
vv(crotchL, V3x(0.1f));
bs_line(crotch,crotchL);
{
vv(bikiniL, (V3(1.0685f, 0.8105f, -0.6117f)));
bikini_dir = (V3(-0.1823f, -0.0005f, 0.9832f));
bs_unit(crotchL, V2(-0.2081f, 0.2062f), 
          V2(0.316f, 0.3309f),
          bikini_dir,
          bikiniL);
bs_unit(crotchL, V2(-0.2767f, 0.2024f),
          V2(0.213f, 0.2503f),
          -bikini_dir, bikiniL);
}
bikini_up_back;
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
dfill3(bikini_front_mid, crotch, girdle_front);
dfill3(girdle_front, crotch, bikini_up_back);
dfill_bez(girdle_side_line);
}
}
pelvis_obj;
#define export_(vertex)  pelvis_obj.vertex = vertex;
 
macro_pelvis(export_);
#undef export_
 
return pelvis_obj
}
function void
render_eyes(Pose *pose){
cache v1 pose->tblink;
eyeO = V3(0.1953f, -0.2348f, 0.8954f);
eye_scale = V3(0.8324f, 0.9882f, 0.92f);
eyeT_ = mat4i_translate(eyeO)*mat4i_scales(eye_scale);
eyeT = eyeT_.forward;
vv(eye_in, eyeT*V3(-0.0282f, 0.f, -0.1236f));
loomis_eye_inZ = faceZ - loomis_unit/3.f;
WARN_DELTA(eye_in.z, loomis_eye_inZ, 0.05f);
show_eye_guideline = painter.show_grid;
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
eye_up_line = bez_bezd_old(eye_in, 
                                   V3(0.0814f, 0.3741f, 0.2401f),
                                   V2(0.0159f, 0.3341f),
                                   eye_out);
fill(es_up_in, eye_up_line);
eye_in_shade = painter.shade_color;
fill3(nose_rootL, eye_in, es_up_in, 
       fp(eye_in_shade));
eye_up_line_radii;
eye_low_line_radii;
tblink = pose->tblink;
{
eye_in_radius = lerp(V2(0.0137f, -0.0006f), tblink);
eye_out_radius = lerp(V2(0.4791f, 0.17f), tblink);
eye0_radius = lerp(V2(0.6426f, 0.499f), tblink);
eye1_radius = lerp(V2(0.f, 0.6154f), tblink);
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
eye_socket_shade = eye_in_shade;
eye_up_line_now;
{
eye_up_line_now = bez_raw(eye_in, 
                            lerp(eye_up_line[1], tblink, eye_low_line[1]),
                            lerp(eye_up_line[2], tblink, eye_low_line[2]),
                            eye_out);
{
draw(eye_up_line_now,eye_up_line_radii);
fill_dbez(eye_up_line, eye_up_line_now, eye_in_shade);
}
}
{
eyeball_center = eyeT*V3(0.1531f, 0.0101f, -0.3179f);
eyeball_radius = (eye_out.x - eye_in.x)*0.4526f;
eyeballT;
{
to_eyeballL = mat4_translate(eyeball_center);
eyeball_scale = mat4_scale(eyeball_radius);
rot;
{
eyeball_theta = 0.13f*(pose->teye_theta);
if(painter.lr_index){
eyeball_theta *= -1;
}
rot = rotateY(eyeball_theta);
}
eyeballT = to_eyeballL*eyeball_scale*rot;
show_eyeball = get_preset() == 2;
if(show_eyeball){
hl_block;
eyeball_radii;
{
big = 0.5f;
small = 0.25f;
eyeball_radii = V4(big,small,small,big);
}
draw_bezier_circle(eyeballT, eyeball_radii);
eyeball_equator = eyeballT*rotateX(0.25f);
draw_bezier_circle(eyeball_equator, eyeball_radii);
}
}
{
lp_block(nslice_per_meter, 128.f*(7.6378f));
iris_depth_offset = painter.fill_depth_offset + 1*centimeter;
lp_block(depth_offset, current_line_cparams().depth_offset+iris_depth_offset);
add_in_block(painter.fill_depth_offset, iris_depth_offset);
{
irisRelT;
{
iris_radius = (0.4934f);
iris_rel_center = V3z(square_root(1-squared(iris_radius)));
irisRelT = mat4_translate(iris_rel_center) * mat4i_scale(iris_radius);
}
iris_radii;
{
big = 0.5f;
small = 0.25f;
iris_radii = V4(big,small,small,big);
}
{
irisLT = eyeballT*irisRelT;
draw_bezier_circle(irisLT, iris_radii);
}
}
}
{
for_i32(index,0,2){
line;
if(index == 0){
line = eye_up_line;
}
else{ line = eye_low_line; }
    auto result = get_eye_min_distance(eyeball_center, eyeball_radius, line);
if (result.min_distance < 0){
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
function Head
render_head(Pose *pose, v1 animation_time)