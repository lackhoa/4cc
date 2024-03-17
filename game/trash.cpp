//internal v3
//perspective_project(Camera *camera, v4 worldP)
{
 v4 result = matvmul4(camera->project, worldP);
 result.xy -= camera->pan;
 v1 depth = camera->distance-result.z;
 
 v1 focal = camera->focal_length;
 v1 n = camera->near_clip;
 v1 f = camera->far_clip;
 
 result.xy *= focal / depth;
 result.z = ((n+f)*depth - 2*f*n)/(f-n);
 result.w = depth;
 
 if ( absolute(result.w) > 1e-4 )
 {
  result /= result.w;
 }
 return result.xyz;
}

if (0)
{
 v3 A = vec3(nose_root.x,nose_tip.y,nose_root.z);
 v3 B = nose_tip;
 v3 C = nose_root;
 
 v2 a = perspective_project(p, A).xy;
 v2 b = perspective_project(p, B).xy;
 v2 c = perspective_project(p, C).xy;
 
 v2 ab = (b-a);
 v2 ac = (c-a);
 
 //DEBUG_VALUE(ab);
 //DEBUG_VALUE(ac);
 
 profile_score = absolute(cross2d(ab, ac));
 {
  v3 D = vec3(nose_wing.x, nose_tip.y,nose_root.z);
  v2 d = perspective_project(p, D).xy;
  v2 ad = (d-a);
  v1 calibration_term = absolute(cross2d(ad, ac));
  profile_score /= calibration_term;
 }
 
 DEBUG_VALUE(profile_score);
}

{// NOTE: The new procedural hair experiment
 // NOTE: Assume that L is the side that curves more
 symx_off;
 
 v3 root  = fvert(0.2531, -0.0059, 0.8966);
 v3 tip   = fvert(0.0865, -0.2857, 1.0268);
 v3 droot = fvert(0.0766, -0.1442, 0.0722);
 v3 dtip  = fvert(0.1043, 0.0618, 0.1511);
 v3 rootL = root+fvert(-0.0075, 0.0108, -0.0237);
 v3 rootR = root+fvert(-0.0730, 0.0104, 0.0309);
 v3 tip_distance = fvert(0,0,0);
 Bezier edgeL = make_bezier(p, rootL, droot, dtip, tip+tip_distance);
 Bezier edgeR = make_bezier(p, rootR, droot, dtip, tip);
 draw_bezier(p,edgeL);
 draw_bezier(p,edgeR);
 double_bezier_poly(p,edgeL,edgeR);
 
 v3 offset = fvert(-0.0062, -0.0051, -0.0196);
 v3 tip_offset = fvert(0.0173, 0.0803, -0.0328);
 Bezier edgeL1 = make_bezier(p,
                             rootL,
                             offset+droot,
                             offset+dtip,
                             offset+tip+tip_offset+tip_distance);
 v1 dfudge = 0.6f;
 Bezier edgeR1 = make_bezier(p,
                             offset+rootR+0.5f*(rootR-root),//@Hack
                             offset+dfudge*droot,// NOTE: The side that doesn't curve, should curve less
                             offset+dfudge*dtip,
                             offset+tip+tip_offset);
 draw_bezier(p,edgeL1);
 draw_bezier(p,edgeR1);
 double_bezier_poly(p,edgeL1,edgeR1);
}
//-
Hair middle_left = {
 .droot = fvert(0.1271, -0.1522, 0.0778),
 .dtip  = fvert(-0.1361, 0.0063, 0.1545),
 .tip   = fvert(0.0874, -0.2555, 0.9991),
 .full  = fbool(-0.0102),
};
Hair middle_right = {//middle right
 .droot = fvert(-0.1518, -0.0471, 0.1518),
 .dtip  = fvert(-0.1305, -0.0232, 0.1063),
 .full  = fbool(0),
};
middle_right.tip = middle_left.tip;

Hair left_left = {//left left
 .droot = fvert(0.1391, -0.1127, 0.0776),
 .dtip  = fvert(0.2082, 0.1134, -0.0030),
 .tip   = fvert(0.1110, -0.2267, 0.9960),
 .full  = fbool(-0.0117),
};
Hair left_right = {//left right
 .droot = fvert(0.0950, -0.1389, 0.1265),
 .dtip  = fvert(0.0894, 0.1403, -0.0511),
 .full  = fbool(-0.0000),
};
left_right.tip = left_left.tip;

Hair right_left = {// right left
 .droot = fvert(-0.1348, -0.1039, 0.1672),
 .dtip  = fvert(-0.0318, 0.0114, 0.0477),
 .tip   = fvert(-0.0270, -0.2459, 1.0039),
 .full  = false,
};
Hair right_right = {// right right
 .droot = fvert(-0.1305, -0.1190, 0.1578),
 .dtip  = fvert(-0.2647, 0.0863, 0.0775),
 .full  = fbool(-0.0051),
};
right_right.tip = right_left.tip;

struct Hair {
 v3  droot;
 v3  dtip;
 v3  tip;
 b32 full;
 b32 cut;  // NOTE: pun not intended, at least originally
 Bezier line;
};

//-
//NOTE Animation of the head using 8th's instead of 6th's
if(use_8)
{
 if(fbool(1))
 {// NOTE: right tilt
  fkeyframe(4, 0);   // rest
  fkeyframe(1, 1);  // anticipation
  fkeyframe(8, -8); // rest
 }
 
 if(fbool(1))
 {// NOTE: left tilt
  fkeyframe(1, -10); // anticipation
  fkeyframe(1, -9);  // movement
  fkeyframe(1, 6);  // movement
 }
 
 fkeyframe(8, 8);// rest
}
//-
force_inline rect2 
draw_widget_children(App *app, Widget_State *state, Widget *parent, v2 top_left)
{
 return draw_sibling_widgets(app, state, parent->children, parent->children_count, top_left);
}

internal rect2
draw_single_widget(App *app, Widget_State *state, Widget *widget, v2 top_left)
{
 Scratch_Block scratch(app);
 
#if 0
 Face_ID face = get_face_id(app,0);
 Face_Metrics face_metrics = get_face_metrics(app, face);
 
 Fancy_Line line_value = {}; 
 Fancy_Line *line = &line_value;
 push_fancy_string(scratch, line, f_white, widget->name);
 // 
 v2 name_dim = vec2(get_fancy_line_width(app, face, line),
                    face_metrics.line_height);
 // 
 draw_fancy_line(app, face, fcolor_zero(), line, widget_min);
#else
 v2 name_dim = vec2(50.0f, 50.0f);
 v2 widget_min = vec2(top_left.x, top_left.y-name_dim.y);
 draw_rect2(app, rect2_min_dim(widget_min, name_dim), argb_gray(.5));
#endif
 
 auto widget_indentation = 10.0f;
 v2 children_top_left = vec2(widget_min.x + widget_indentation,
                             widget_min.y);
 rect2 children = draw_widget_children(app, state, widget, children_top_left);
 //
 v2 whole_max = vec2(macro_max(widget_min.x + name_dim.x, children.max.x),
                     widget_min.y + name_dim.y + widget_margin);
 //
 widget_min.y = children.min.y - widget_margin;
 
 rect2 whole_box = rect2_min_max(widget_min, whole_max);
 if (widget == state->hot_item)
 {
  u32 color = argb_yellow;
  if (state->is_editing)  color = argb_red;
  v1 thickness = 2.0f;
  draw_rect_outline2(app, whole_box, thickness, color);
 }
 
 return whole_box;
}
internal rect2 draw_single_widget(App *app, Widget_State *state, Widget *tree, v2 top_left);

internal rect2
draw_sibling_widgets(App *app, Widget_State *state, Widget *siblings, u32 sibling_count, v2 top_left)
{
 v2 min = top_left;
 v2 max = top_left;
 for_u32 (index,0,sibling_count)
 {
  Widget *widget = siblings + index;
  rect2 rect = draw_single_widget(app, state, widget, min);
  
  max.x = macro_max(rect.max.x, max.x);
  min.y = rect.min.y - widget_margin;
 }
 return rect2_min_max(min, max);
}

//-

#if 0
Animation *theta_animation = push_struct(arena, Animation);
{// NOTE: head tilting animation
 Animation *ani = theta_animation;
 begin_animation(ani, true);
 if(fbool(1))
 {
  fkeyframe(4, 0);  // rest
  fkeyframe(1, 1);  // anticipation
  fkeyframe(8, -6); // rest
 }
 
 if(fbool(1))
 {// NOTE: left tilt
  fkeyframe(1, -8); // anticipation
  fkeyframe(1, -7);  // movement
  fkeyframe(1, 5);  // movement
 }
 
 fkeyframe(8, 6); // rest
 end_animation(ani);
}
#endif

//-
enum Anime_Command_Type
{
 Anime_Command_Null,
 Anime_Rest,
 Anime_Set,
 Anime_Add,
};

struct Anime_Command
{
 Anime_Command_Type type;
 union
 {
  struct
  {
   i32 offset;
   v4 data;
   i32 data_size;     
  };
  i32 nframes;
 };
};
//-
{
#if 0
 v1 blink_period = fval(0.4f);
 v1 tblink0 = animation_time / blink_period;
 b32 should_blink = ((i32)tblink0 % 4 == 0 ||
                     (i32)tblink0 % 4 == 1);
 v1 tblink = cycle01_positive(tblink0);
 
 v1 tclose = shot->tblink;
 {
  const i32 nframes = 6;
  v1 tclose_middle = fval(0.203905f);
  v1 tclose_keys[nframes] = { 0.f, tclose_middle, 1.f, 1.f, tclose_middle, 0.f };
  tclose = tclose_keys[ get_frame_index(nframes, tblink) ];
 }
#endif
 
}
//-NOTE: Oh widget, don't need it, it's not what the project is about!

struct Widget
{
 // Widget_ID id;
 String name;
 
 Widget *parent;
 
 u32     children_count;
 Widget *children;
};

struct Widget_State
{
 Widget root;
 Widget *hot_item;
 b32 is_editing;
};
typedef u32 Widget_ID;

{
 Widget_State widget_state;
 Widget *curve_widgets;
 Widget *surface_widget;
}

{// NOTE: widget init
 Widget_State *widget_state = &state->widget_state;
 // TODO: We should make this a parsed data structure if we decide to use this system
 u32 WIDGET_COUNT = 4;
 
 Widget *widgets = push_array(permanent_arena, Widget, WIDGET_COUNT);
 
 Widget *root = &widget_state->root;
 *root = {str8lit("root"),0, WIDGET_COUNT,widgets};
 
 widget_state->hot_item = root->children;
 
 // NOTE: surface
 Widget *surface = widgets + 0;
 *surface        = {str8lit("surface"),root, 0,0};
 state->surface_widget = surface;
 
 // NOTE: Bezier curve
 Widget *curves = widgets + 1;
 for_u32 (curve_index,0,3)
 {
  curves[curve_index] = {str8lit("curve"),root, 0,0};
 }
 state->curve_widgets = curves;
}

{// bookmark: The eye experiment
 viz_block;
 v2 A0 = fval2(-0.1f, 0.f);
 v2 B0 = fval2(0.15f, 0.08f);
 v1 At = A0.x; v1 Ap = A0.y;
 v1 Bt = B0.x; v1 Bp = B0.y;
 //note: find the plane between A and B
 v3 A = theta_phi_point(At, Ap);
 v3 B = theta_phi_point(Bt, Bp);
 v3 z_axis = cross(A,B);
 // NOTE: The plane is just the normal, d=0 since it goes through 0,0,0
 // We need z to end up at this normal, but we have no idea what the other axes are
 // Well, we also know that the x axis line up with OA, since we wanna draw from there
 mat4i rotation = mat4_columns(A, cross(z_axis, A), z_axis);
 mat4 forward = to_eyeballL*eyeball_scale*rotation.forward;
 draw_bezier_circle(p, forward);
 
 v3 B_new = mat4vert(rotation.inverse,B);
 v1 turn = arccos(B_new.x);
 DEBUG_VALUE(B_new);
 DEBUG_VALUE(turn);
}

internal v3
perspective_project(Painter *p, v3 worldP)
{
 Camera *camera = p->camera;
 v4 result = &camera->inverse * vec4(worldP, 1.f);
 
 if (1)
 {
  mat4 camera_view = camera_perspective_view_matrix(p->camera);
  result = camera_view*result;
 }
 else
 {// NOTE: Debug path
  v1 depth = camera->distance-result.z;
  
  v1 focal = camera->focal_length;
  v1 n = camera->near_clip;
  v1 f = camera->far_clip;
  
  result.xy *= focal;
  result.z = ((n+f)*depth - 2*f*n)/(f-n);
  result.w = depth;
 }
 
 result /= result.w;
 return result.xyz;
}

{//NOTE: Trying to compute the camera normal
 Bez &line = delt_hline;
 v3 normal = cross(line[1]-line[0], line[3]-line[0]);
 v3 centroid = (line[0]+line[1]+line[2]) / 3.f;
 v3 P = centroid+normal;
 {
  viz_block;
  draw(p, bez1(centroid,P));
 }
 v3 camP = perspective_project_non_hyperbolic(p->camera, to_world*P);
 v3 cam_displacement = camP - perspective_project_non_hyperbolic(p->camera, to_world*centroid);
 v1 visibility = absolute(noz(cam_displacement).z);
 DEBUG_VALUE(visibility);
 if(fbool(1) && (visibility > 0.9f))
 {
  draw(p, delt_hline);
 }
}

{// ;aabb_hit_test
 v3 rd_inv = 1.f / rd;
 
 v3 tminv, tmaxv;
 for_i32(index,0,3)
 {
  tminv[index] = (-ro[index] -R[index]) * rd_inv[index];
  tmaxv[index] = (-ro[index] +R[index]) * rd_inv[index];
  swap_minmax(tminv[index], tmaxv[index]);
 }
 v1 &tmin0 = tminv[0];
 v1 &tmax0 = tmaxv[0];
 
 if ((tmax0 < tminv[1] || tmaxv[1] < tmin0) == false)
 {// xy ranges overlap
  macro_clamp_min(tmin0, tminv[1]);
  macro_clamp_max(tmax0, tmaxv[1]);
  
  if ((tmax0 < tminv[2] || tmaxv[2] < tmin0) == false)
  {// xy common range overlaps with z
   macro_clamp_min(tmin0, tminv[2]);
   macro_clamp_max(tmax0, tmaxv[2]);
   
   if (lower_bound < tmin0) { hit_time = tmin0; }
   else                     { hit_time = tmax0; }
   
   if (hit_time == F32_MAX) { hit_time = miss_time; }
   
   hit_normal = -rd_old;
  }
 }
}

//~ The great ray tracing disaster
#if 0
enum Hit_Type
{
 CSG_Enter = 0,
 CSG_Exit  = 1,
 CSG_Miss  = 2,
};

struct CSG_Stack_Entry
{
 CSG_Stack_Entry *next;
 CSG_Type operation;
 v1 arg1_time;
 b32 arg1_normal;
 Hit_Type hit_status;
 CSG_Tree *next_arg;
};

struct CSG_Stack
{
 CSG_Stack_Entry *first;
};

enum CSG_Action
{
 CA_Null,
 
 Ret_Miss,
 RetL,
 RetR,
 RetL_If_Closer,
 RetR_If_Closer,
 
 LoopL,
 LoopR,
 Loop_Closer,
 
 FlipR,
};

typedef CSG_Action Action_Table[3][3][3];
// NOTE: [L][R][list of actions]
Action_Table csg_union_table = {
 {{RetL_If_Closer, RetR_If_Closer},
  {RetR_If_Closer, LoopL},
  {RetL}},
 {{RetL_If_Closer, LoopR},
  {Loop_Closer},
  {RetL}},
 {{RetR}, {RetR}, {Ret_Miss}},
};
//
Action_Table csg_subtraction_table = {
 {{RetL_If_Closer, LoopR},
  {Loop_Closer},
  {RetL}},
 {{RetL_If_Closer, RetR_If_Closer, FlipR},
  {RetR_If_Closer, LoopL, LoopL},
  {RetL}},
 {{Ret_Miss}, {Ret_Miss}, {Ret_Miss}},
};

struct Hit_Result
{
 v1 t;  // NOTE: zero for miss
 v3 n;
};

force_inline Hit_Type
classify_hit(v3 rd, v1 t, v3 n)
{
 return ((t == 0.f)         ? CSG_Miss :
         (dot(rd, n) > 0.f) ? CSG_Exit : CSG_Enter);
}

force_inline b32
is_exiting(v3 rd, v3 n)
{
 return dot(rd, n) > 0.f;
}

inline b32
action_in(CSG_Action action, CSG_Action actions[3])
{
 return (action == actions[0] ||
         action == actions[1] ||
         action == actions[2]);
}

//todo @incomplete
#define elemof(value, a,b) (value == (a) || value == (b))

internal Hit_Result
hit_test(v3 rd, CSG_Tree *tree, v1 lower_bound /*@tmin_exclusive*/)
{
 v1 miss_time = 0.f;  // ;hit_test_zero NOTE hit_test returns "0.f" for a miss
 b32 is_op = elemof(tree->type, CSG_Union, CSG_Subtract);
 if (is_op)
 {
  Hit_Result hitl = hit_test(rd, tree->l, lower_bound);
  Hit_Result hitr = hit_test(rd, tree->r, lower_bound);
  while(true)
  {
   v1 tl = hitl.t;
   v1 tr = hitr.t;
   CSG_Action *actions;
   {
    Hit_Type typel = classify_hit(rd, tl, hitl.n);
    Hit_Type typer = classify_hit(rd, tr, hitr.n);
    Action_Table &table = (tree->type == CSG_Union) ? csg_union_table : csg_subtraction_table;
    actions =  table[typel][typer];
   }
   if ( action_in(Ret_Miss, actions) )
   {
    return {};
   }
   else if (action_in(RetL,actions) || 
            (action_in(RetL_If_Closer, actions) && (tl <= tr)))
   {
    return hitl;
   }
   else if (action_in(RetR,actions) || 
            (action_in(RetR_If_Closer,actions) && (tr < tl)))
   {
    return hitr;
   }
   else if (action_in(LoopL, actions) ||
            (action_in(Loop_Closer, actions) && (tl <= tr)))
   {
    hitl = hit_test(rd, tree->l, tl);
   }
   else if (action_in(LoopR, actions) ||
            (action_in(Loop_Closer, actions) && (tr < tl)))
   {
    hitr = hit_test(rd, tree->r, tr);
   }
   else { invalid_code_path; }
  }
 }
 else
 {
  v1 hit_time = miss_time;
  v3 hit_normal = {};
  switch(tree->type)
  {// ;csg_primitives
   //IMPORTANT: ray origin at (0,0,0) thanks to @raycast_transform
   
   case CSG_Sphere:
   {
    v3 ro = -tree->center;
    v1 b = 2.f*dot(ro, rd);
    v1 c = lensq(ro)-squared(tree->radius);
    v1 inside_root_term = squared(b) - 4.f*c;
    if (inside_root_term > 0.f)
    {
     v1 root_term = square_root(inside_root_term);
     hit_time = (-b-root_term) * 0.5f;
     if (hit_time <= lower_bound)  //@tmin_exclusive
     {
      hit_time = (-b+root_term) * 0.5f;
     }
     {//NOTE: normal
      v3 hit_pos = rd*hit_time;
      hit_normal = (hit_pos - tree->center) / tree->radius;
     }
    }
   }break;
   
   case CSG_Plane:
   {
    v3 n = tree->n;
    v1 d = tree->d;
    v1 denom = dot(n, rd);
    if (absolute(denom) > 1e-4)
    {
     // NOTE: We can omit one dot product due thanks to @raycast_transform
     hit_time = -d / denom;
     if (hit_time > lower_bound) { hit_normal = n; }
     else if ( !is_exiting(rd, n) )
     {// NOTE: Hack half-space: If you enter, you come out at infinity
      hit_time = F32_MAX;
      hit_normal = rd;  // @Hack
     }
    }
    else if (d < 0.f)
    {// NOTE: origin is inside the plane -> exit at infinity
     hit_time = F32_MAX;
     hit_normal = rd;
    }
   }break;
   
   case CSG_Box:
   {// ;csg_primitive_box ; see also csg_box and @aabb_hit_test
    // Reference reindeer: https://www.shadertoy.com/view/tl23Rm
    v3 ro = mat4vert_no_div(tree->to_aabb, vec3());
    v3 &R = tree->box_radius;
    set_in_block(rd, mat4vec(tree->to_aabb, rd));
    
    // TODO: no idea what this does...
    v3 t1,t2;
    {
     v3 m = signof(rd) / max(absolute(rd), vec3(1e-8));
     v3 n = m*ro;
     v3 k = absolute(m)*R;
     t1 = -n - k;
     t2 = -n + k;
    }
    
    v1 tmin = max(t1.x, t1.y, t1.z);
    v1 tmax = min(t2.x, t2.y, t2.z);
    
    if (tmin > tmax || tmax <= lower_bound) { hit_time = miss_time; }
    else
    {
     hit_normal = -signof(rd)*step(yzx(t1), t1)*step(zxy(t1), t1);
     mat3 aabb_to_camera = transpose( to_mat3(tree->to_aabb) );  // @speed
     hit_normal = aabb_to_camera*hit_normal;
     hit_time   = (tmin > lower_bound) ? tmin : tmax;
    }
   }break;
  }
  
  if (hit_time <= lower_bound) { hit_time = miss_time; }  //@tmin_exclusive
  
  return {hit_time, hit_normal};
 }
}

// @call_csg_render
internal void
csg_render(Render_Group *group, v2 dst_dim, argb *csg_buffer)
{
 v1 default_fvert_delta_scale = 0.1f;
 argb ball_color = argb_blue;
 
 i32 W = clamp_between(0, cast(i32)(dst_dim.x), 1920);  // @dim_round_down
 i32 H = clamp_between(0, cast(i32)(dst_dim.y), 1080);
 
 //NOTE: Clear the framebuffer
 block_zero( csg_buffer, W*H*sizeof(argb) );
 
 v1 far_clip = group->far_clip;
 v1 focal_length = group->focal_length;
 v1 pixel_to_meter = 1.f / group->meter_to_pixel;
 
 CSG_Group *csg_group = global_csg_group;
 CSG_Tree *csg_tree = 0;
 {//NOTE: CSG tree pre-pass
  for(CSG_Tree *tree = csg_group->first;
      tree;
      tree = tree->next)
  {
   if (csg_tree)
   {
    CSG_Tree *new_tree = push_struct_zero(viewport_frame_arena, CSG_Tree);
    new_tree->type = (tree->negated) ? CSG_Subtract : CSG_Union;
    new_tree->l = csg_tree;
    new_tree->r = tree;
    
    csg_tree = new_tree;
   }
   else
   {
    csg_tree = tree;
   }
  }
 }
 
 v4 main_color = argb_unpack(argb_silver);
 
 //~IMPORTANT: Danger zone! Mega loop!
 for_i32(py,0,H)
 {
  for_i32(px,0,W)
  {
   v3 rd;
   {//note compute rd
    v1 filmx = pixel_to_meter*(v1(px) - 0.5f*v1(W) + 0.5f);
    v1 filmy = pixel_to_meter*(v1(py) - 0.5f*v1(H) + 0.5f);
    v3 pixel_camera_space = vec3(filmx, filmy, -focal_length);
    rd = noz(pixel_camera_space);
   }
   
   v1 time = 0.f;
   v3 hit_normal = vec3();
   v1 tmin = group->near_clip;
   {// note: compute hit time
    CSG_Tree *tree = csg_tree;
    CSG_Stack stack = {};
    Temp_Memory_Block temp(viewport_frame_arena); //@speed
    CSG_Type current_op = CSG_NULL;
    Hit_Result hit = hit_test(rd, tree, tmin);
    time = hit.t;
    hit_normal = hit.n;
    
#if 0
    while(tree)
    {
     if (is_op)
     {//NOTE: Operation
      CSG_Stack_Entry *entry = push_struct_zero(viewport_frame_arena, CSG_Stack_Entry);
      entry->operation = tree->type;
      entry->next_arg = tree->arg2;
      sll_stack_push(stack.first, entry);
      
      tree = tree->arg1;
     }
     else
     {//NOTE: Primitive
      b32 hit_exiting = false;
      v1 hit_normal = vec3();
      
      Hit_Type hit_status;
      {//note: compute hit status
       if ((min_time < time) && (time < far_clip))
       {
        hit_status = (dot(hit_normal, rd) > 0.f) ? CSG_Exit : CSG_Enter;
       }
       else { hit_status = CSG_Miss; }
      }
      
      tree = 0;
      while(CSG_Stack_Entry *head = stack.first)
      {
       if (head->next_arg) 
       {//NOTE: Compute other arg
        head->arg1_time = time;
        head->arg1_normal = hit_normal;
        tree = head->next_arg;
        head->next_arg = 0;
        head->arg1_hit_status = hit_status;
        break;
       }
       else
       {//NOTE: resolve operation
        b32 const hl = head->arg1_hit_status;
        b32 const hr = hit_status;
        v1 const tl = head->arg1_time;
        v1 const tr = time;
        if (action in )
         CSG_Action *actions = csg_union_table[hl][hr];
        if ((retl in actions) || (retl_if_closer in actions && (tl <= tr)))
        {
         tr = tl;
         Nr = Nl;
        }
        if ((retr in actions) || (retr_if_closer in actions && (tr < tl)))
        {
         if (flipr in actions) { Nr = -Nr; }
         tl = tr;
         Nl = Nr;
        }
        else if ((loopl in actions) || (loopl_if_closer in actions && tl <= tr))
        {
         tmin = tl;
        }
        
        time = macro_min(head->arg1_time, time);
        
        sll_stack_pop(stack.first);
       }
      }
     }
    }
#endif
   }
   
   argb color = 0;
   if ((time > tmin) && (time < far_clip)) // NOTE: near-far clipping
   {// ;csg_shading  TODO: hacky lighting... probably inaccurate
    v1 darkness_fuzz = 0.0f;
    v4 colorv = main_color;
    colorv.rgb *= lerp(darkness_fuzz, absolute(hit_normal.z), 1.f);
    color = argb_pack(colorv);
   }
   
   {// Write it out
    csg_buffer[py*W + px] = color;
   }
  }
 }
}
#endif

#if 0

{
 //~ Code from the state machine paper
 function paper_machine_main()
 {
  tmin = 0;
  tree = V; // virtual root whose left subtree is the real root
  (tL, nL) = invalid;
  (tR, nR) = invalid;
  states = [Compute];
  state = GotoLft;
  while true
  {
   if state == SaveLft
   {
    tmin = POP(tmins);
    PUSH(primitives, (tL, nL));
    state = GotoRgh;
   }
   if state in {GotoLft, GotoRgh}
   {
    GOTO()
   }
   if state in {LoadLft, LoadRgh, Compute}
   {
    COMPUTE()
   }
  }
 }
 
 function GOTO()
 {
  if state == GotoLft  { tree = L(tree) }
  else { tree = R(tree) }
  if tree is Operation 
  {
   gotoL = INTERSECTBOX(L(tree));
   gotoR = INTERSECTBOX(R(tree));
   
   if gotoL and L(tree) is Primitive 
   {
    (tL, nL) = INTERSECT(L(tree), tmin);
    gotoL = false;
   }
   if gotoR and R(tree) is Primitive 
   {
    (tR, nR) = INTERSECT(R(tree), tmin);
    gotoR = false;
   }
   
   if gotoL or gotoR
   {// NOTE(kv): non-primitive branch
    if gotoL
    {// note(kv): left is non-primitive
     PUSH(primitives, (L(tree), tL));  // todo: tL and tR is worth looking into, are they necessary, and if yes what do they mean?
     PUSH(states, LoadLft);
    }
    else if gotoR
    {// note(kv): right is non-primitive, but left is primitive
     PUSH(primitives, (R(tree), tR));
     PUSH(states, LoadRgh);
    }
    else
    {
     PUSH(tmins,tmin);
     PUSH(states, LoadLft);
     PUSH(states, SaveLft);
    }
    
    if gotoL { state = GotoLft }
    else     { state = GotoRgh }
   }
   else { state = Compute; }
  }
  else
  {
   // tree is a Primitive
   if state == GotoLft
   {
    (tL, nL) = Intersect(tree, tmin)
   }
   else
   {
    (tR, nR) = Intersect(tree, tmin)
   }
   state = Compute;
   GOTOPARENT(tree);
  }
 }
 
 function COMPUTE()
 {
  if      state == LoadLft { (tL, nL) = POP(primitives); }
  else if state == LoadRgh { (tR, nR) = POP(primitives); }
  
  actions = table(tree)[CLASSIFY(tL, nL),
                        CLASSIFY(tR, nR)];
  
  if RetL in actions or (RetL_If_Closer in actions and tL <= tR)
  {
   (tR, nR) = (tL, nL);  // todo(kv): what for?
   state = POP(states);
   GOTOPARENT(tree);
  }
  else if RetR in actions or (RetR_If_Closer in actions and tR < tL)
  {
   if FlipNormR in actions { nR = -nR; }
   (tL, nL) = (tR, nR);  // todo(kv): what for?
   state = POP(states);
   GOTOPARENT(tree);
  }
  else if LoopL in actions or (Loop_Closer in actions and tL <= tR)
  {
   tmin = tL;
   PUSH(primitives, (tR, nR));
   PUSH(states, LoadRgh);
   state = GotoLft;
  }
  else if LoopR in actions or (Loop_Closer in actions and tR < tL)
  {
   tmin = tR;
   PUSH(primitives, (tL, nL));
   PUSH(states, LoadLft);
   state = GotoRgh;
  }
  else
  {
   tR = invalid;
   state = POP(states);
  }
 }
}

{//-NOTE: Code from the earlier paper
 function IntersectWith(tree, tmin)
 {
  if ( IsPrimitive(tree) )
  {
   return (t, N);
  }
  else
  {
   (tL, nL) = IntersectWith( L(tree), tmin );
   (tR, nR) = IntersectWith( R(tree), tmin );
   stateL = ClassifyEnterExitOrMiss( tL, nL );
   stateR = ClassifyEnterExitOrMiss( tR, nR );
   while true
   {
    actions = table[stateL, stateR];
    if ReturnMiss in actions
    {
     return miss;
    }
    else if (RetL in actions or
             (RetL_If_Closer in actions and tL <= tR) or
             (RetLIfFarther in actions and tL > tR))
    {
     return (tL, nL);
    }
    else if (RetR in actions or
             (RetR_If_Closer in actions and tR <= tL) or
             (RetRIfFarther in actions and tR > tL))
    {
     if FlipB in actions { nR = -nR; }
     return (tR, nR);
    }
    else if LoopL in actions
    {
     (tL, nL) = IntersectWith( L(tree), tL );
     stateL = ClassifyEnterExitOrMiss( tL, nL );
    }
    else if LoopR in actions
    {
     (tR, nR) = IntersectWith( R(tree), tR );
     stateR = ClassifyEnterExitOrMiss( tR, nR );
    }
   }
  }
 }
}

#endif
{// NOTE: good-old dottted lines, holy cow!
 v4 main_color_v4 = argb_unpack(params->color);
 
 i32 nslices;
 {//~NOTE: pre-pass
  // NOTE: Working in 2D is no good, because we don't take into account that samples are moving in 3D, 
  // so they might be traveling longer distances due to the depth, 
  // and spaced out more when we look at them in 3D -> you'd underestimate the density in 2D
  const mat4 &transform = objectT.forward;
  v1 the_length = 0.f;
  {
   i32 pre_nslices = 8;
   v1 inv_nslices = 1.0f / (v1)pre_nslices;
   v3 last_sample_transformed;
   for (i32 sample_index=0;
        sample_index < pre_nslices+1;
        sample_index++)
   {
    v1 t = inv_nslices * (v1)sample_index;
    v3 sample_transformed = mat4vert(transform, bezier_sample(P,t));
    if (sample_index != 0)
    {
     the_length += lengthof(sample_transformed - last_sample_transformed);
    }
    last_sample_transformed = sample_transformed;
   }
  }
  
#if 0
  //NOTE: depending on the radius is quite dumb
  v1 max_radius = 0.f;
  for_i32(index,0,4) 
  {//NOTE: Approximating maximum radius (after transformation)
   // TODO: This number is just pure garbage
   v1 radius = params->radii[index];
   v4 control_point_transformed = view_objectT*vec4(P[index],1.f);
   radius *= (camera->focal_length / control_point_transformed.w);
   
   macro_clamp_min(max_radius, radius);
  }
  
  if (max_radius > 1e-5)
  {
   v1 slice_density = 1.f;
   // NOTE: length / radius is actually unit-independent
   nslices = i32_roundv1( slice_density * the_length / max_radius );
  }
  else { nslices = 0; }
#else
  //NOTE: applying perspective to this value is just stupid: just model it Llike usual, dude!
  v1 max_radius = 0.f;
  for_i32(index,0,4) 
  {//NOTE: We rely on maximum radius, because there are cases where the user supplies radius 0, to fade the line out
   v1 radius = params->radii[index];
   macro_clamp_min(max_radius, radius);
  }
  
  if (max_radius > 1e-5)
  {
   v1 slice_density = 1.25f;
   // NOTE: length / radius is actually unit-independent
   nslices = i32_roundv1( slice_density * the_length / max_radius );
  }
  else { nslices = 0; }
#endif
  
  macro_clamp_min(nslices, 16);
  i32 MAX_NSLICES = 256;
  if (nslices > 2*MAX_NSLICES)
  {
   DEBUG_VALUE(nslices);
  }
  macro_clamp_max(nslices, MAX_NSLICES);
 }
 v1 interval = 1.0f / (v1)nslices;
 
 // NOTE: visibility control
 nslices = roundv1( params->visibility * (v1)nslices );
 
 Vertex_Type vertex_type = Vertex_Line;
 if (params->flags & Line_Overlay) { vertex_type = Vertex_Overlay; }
 
 argb color = argb_pack(main_color_v4);
 for (i32 sample_index=0;
      sample_index < nslices+1;
      sample_index++)
 {
  v1 t = interval * (v1)sample_index;
  
  v3 center = bezier_sample(P,t);
  v1 radius = bezier_sample(params->radii, t);
  draw_point(target, center, radius, color, params->depth_offset, vertex_type); // @Slow
 }
}

#if 0
internal void
fill_parallel(Painter *p, v3 a[], i32 acount, v3 b[], i32 bcount, argb color=0)
{
 if (acount > bcount) { macro_swap(a,b); macro_swap(acount,bcount); }
 
 for_i32(index,0,acount-1)
 {
  fill4(p, a[index], b[index], b[index+1], a[index+1]);
 }
 
 if (bcount > acount)
 {
  i32 last = acount-1;
  fill_fan(p, a[last], b+last, bcount-last);
 }
}
#endif


//~eof
