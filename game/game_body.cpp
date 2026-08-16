//-
#define XStructMemberTvert(NAME, ...)   tvert NAME;

union Head
{
#define macro_head(X) \
X(jaw) \
X(behind_ear) \
X(head_neck_junction) \
X(trapezius_head) \
X(chin_middle) \
//
 struct{ macro_head(XStructMemberTvert); };
 tvert verts[];
};
//
global_const i32 head_vert_count = sizeof(Head) / sizeof(tvert);

union Pelvis
{
#define macro_pelvis(X) \
X(navel) \
X(bikini_up_back) \
//
 struct { macro_pelvis(XStructMemberTvert) };
 tvert verts[];
};
//
global_const i32 pelvis_vert_count = sizeof(Pelvis) / sizeof(tvert);

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
 struct { macro_torso(XStructMemberTvert) };
 tvert verts[];
};
//
global_const i32 torso_vert_count = sizeof(Torso) / sizeof(tvert);

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
X(brachialis_begin) \
X(internal_condyle) \
X(external_condyle) \
//
 struct { macro_arm(XStructMemberTvert) };
 tvert verts[];
};
//
global_const i32 arm_vert_count = sizeof(Arm) / sizeof(tvert);

union Forearm
{
#define macro_forearm(X) \
X(radius_bump) \
X(middle_finger_meeter) \
X(palm_in) \
//
 struct { macro_forearm(XStructMemberTvert) };
 tvert verts[];
};
//
global_const i32 forearm_vert_count = sizeof(Forearm) / sizeof(tvert);

#undef XStructMemberTvert

//-
// NOTE import_vertices (the bone funnel over these structs) lives in
// driver_utils.cpp: it needs get_world_from_bone, which is defined after this file.
//-