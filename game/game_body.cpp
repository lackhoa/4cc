//-

#define X_struct_member_v3(NAME, ...)   v3 NAME;
union Head
{
#define macro_head(X) \
X(jaw) \
X(behind_ear) \
X(head_neck_junction) \
X(trapezius_head) \
X(chin_middle) \
//
 struct{ macro_head(X_struct_member_v3); };
 v3 verts[];
};
//
global_const i32 head_vert_count = (sizeof(Head) / sizeof(v3));

union Pelvis
{
#define macro_pelvis(X) \
X(navel) \
X(bikini_up_back) \
//
 struct { macro_pelvis(X_struct_member_v3) };
 v3 verts[];
};
//
global_const i32 pelvis_vert_count = sizeof(Pelvis) / sizeof(v3);

union Torso
{
#define macro_torso(X) \
X(shoulder)   \
X(delt_collar) \
X(scap_delt)    \
X(pectoral_torso) \
X(latis_arm)     \
X(scap_sock_bot) \
//
 struct { macro_torso(X_struct_member_v3) };
 v3 verts[];
};
//
global_const i32 torso_vert_count = sizeof(Torso) / sizeof(v3);

union Arm
{
#define macro_arm(X) \
X(bicep_in_bot) \
X(bicep_out_bot) \
X(white_in) \
X(white_out) \
X(delt_bot) \
X(tricep_wedge) \
X(brachio_a) \
X(brachio_humerus) \
X(brachio_humerus2) \
X(internal_condyle) \
X(external_condyle) \
//
 struct { macro_arm(X_struct_member_v3) };
 v3 verts[];
};
//
global_const i32 arm_vert_count = sizeof(Arm) / sizeof(v3);

union Forearm
{
#define macro_forearm(X) \
X(radius_bump) \
X(middle_finger_meeter) \
X(palm_in) \
//
 struct { macro_forearm(X_struct_member_v3) };
 v3 verts[];
};
//
global_const i32 forearm_vert_count = sizeof(Forearm) / sizeof(v3);

#undef X_struct_member_v3

//-

internal void
import_vertices(v3 *dst, v3 const*src,
                mat4i const&dstT, mat4i const&srcT,
                i32 vert_count)
{
 mat4 to_local = dstT.inverse * srcT.forward;
 for_i32(index,0,vert_count){
  dst[index] = mat4vert(to_local, src[index]);
 }
}

//-