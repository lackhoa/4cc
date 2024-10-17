//NOTE Generated at C:\Users\vodan\4ed\code/meta_klang.cpp:117:
// NOTE: source: C:\Users\vodan\4ed\code\game\driver_test.kc
#pragma once
function void
render_hand(Forearm forearm_obj)
{
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
draw(bez_v3v2(thumb_kbot, (V3()),
               V2(0,0), thumb_palm_conn));
}
