//-

struct mat2
{
 union
 {
  v2 rows[2];
  v1 e[2][2];
 };
 myinline v1* operator[](i32 i){ return e[i]; }
};
function v2
get_column(mat2 A, i32 i)
{
 return V2(A[0][i], A[1][i]);
}
function v2
operator *(mat2 A, v2 x)
{
 v2 result = {};
 for_i32(r,0,2)
 {
  for_i32(i,0,2)
  {
   result[r] += A[r][i] * x[i];
  }
 }
 return result;
}
function v1
bezier_beta(v4 b, v1 t, i32 i, i32 stage)
{
 v1 result;
 if(stage == 0)
 {
  result = b[i];
 }
 else
 {
  v1 A = bezier_beta(b, t, i,   stage-1);
  v1 B = bezier_beta(b, t, i+1, stage-1);
  result = lerp(A, t, B);
 }
 return result;
}
function v1
bezier_recursive(v4 b, v1 t)
{// NOTE(kv) This is the "mathematical" way of doing it.
 i32 degree = 3;
 // NOTE We "fold" the control points to the left.
 // (way easier to understand when you look at the iterative version).
 return bezier_beta(b, t, 0, degree);
}
function v1
bezier_iterative(v4 b, v1 t)
{// NOTE(kv) Just keep lerping until there's one left
 // This IS also de casteljau, as a matter of fact.
 i32 degree = 3;
 for_i32(stage, 0, degree)
 {
  i32 control_point_count = degree - stage;
  for_i32(i, 0, control_point_count)
  {
   b[i] = lerp(b[i], t, b[i+1]);
  }
  // NOTE So "b[0](stage=2) == lerp(b[0](stage=1), t, b[1](stage=1))"
  // EXACTLY like the de casteljau process!
 }
 return b[0];
}
function v4
bezier_first_split(v4 b, v1 t)
{
 v4 result;
 i32 degree = 3;
 for_i32(stage, 0, degree+1)
 {
  result[stage] = bezier_beta(b,t, 0, stage);
 }
 return result;
}

struct Graph
{
 v1 dim;
 v2 scale;
 v2 origin;
};
global Graph the_graph;

function v2
graph_transform_point(v2 point)
{
 v2 result;
 for_i32(i,0,2)
 {
  result[i] = the_graph.origin[i] + the_graph.scale[i]*point[i];
 }
 return result;
}

function void
add_graph()
{
 v2 pos = ImGui::GetCursorScreenPos();
 v1 unit_px = 50.f;  // #Tweak
 v1 graph_dim = 10.f;  // #Tweak
 v1 dim_px = graph_dim*unit_px;
 v1 y_padding = 32.f;
 the_graph.origin = {pos.x, pos.y+dim_px+y_padding};
 the_graph.scale = {unit_px, -unit_px};
 
 {// NOTE Draw coordinate system, so we know wth is going on
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  v2 origin = the_graph.origin;
  ImGuiCol color = ImGui::GetColorU32(ImGuiCol_CheckMark);
  draw_list->AddLine(origin, v2{origin.x, origin.y-dim_px}, color, 2.f);
  draw_list->AddLine(origin, v2{origin.x+dim_px, origin.y}, color, 2.f);
 }
}

function void
graph_bezier(v1 *b, i32 degree)
{
 ImDrawList *draw_list = ImGui::GetWindowDrawList();
 const i32 max_degree = 3;
 if(degree <= max_degree)
 {
  i32 n = degree;
  ImGuiCol color = ImGui::GetColorU32(ImGuiCol_CheckMark);
  v2 points[max_degree+1];
  
  v1 interval = the_graph.dim / v1(n);
  for_i32(i, 0, n+1)
  {
   points[i] = {interval * i, b[i]};
  }
  
  v2 screen_points[max_degree+1];
  for_i32(i, 0, n+1)
  {
   screen_points[i] = graph_transform_point(points[i]);
  }
  
  v1 thickness = 2.f;
  i32 nsegment = 32;
  switch(degree)
  {
   case 2:
   {
    draw_list->AddBezierQuadratic(expand3(screen_points), color, thickness, nsegment);
   }break;
   
   case 3:
   {
    draw_list->AddBezierCubic(expand4(screen_points), color, thickness, nsegment);
   }break;
  }
 }
}
myinline void graph_bezier(v4 b) { graph_bezier(b.e, 3); }
myinline void graph_bezier(v3 b) { graph_bezier(b.e, 2); }

function void
graph_point(v2 point)
{
 ImDrawList *draw_list = ImGui::GetWindowDrawList();
 ImGuiCol color = ImGui::GetColorU32(ImGuiCol_CheckMark);
 v2 center = graph_transform_point(point);
 draw_list->AddCircle(center, 4.f, color, 16, 2.f);
}
function i32
factorial(i32 n)
{
 kv_assert(n >= 0);
 i32 result = 1;
 for_i32(i, 2, n+1)
 {
  result *= i;
 }
 return result;
}
function i32
falling_factorial(i32 n, i32 k)
{
 kv_assert(0 <= k and k <= n);
 i32 result = 1;
 for_i32(i, 0, k)
 {
  result *= (n-i);
 }
 return result;
}
function i32
binomial(i32 n, i32 k)
{// @binomial_coefficients
 kv_assert(n >= 0);
 i32 result = 0;
 if(0 <= k and k <= n)
 {
  result = falling_factorial(n,k) / factorial(k);
 }
 return result;
}
function void
blossom(v1 *result, i32 n, v1 t)
{
 for_i32(i,0,n+1)
 {
  result[i] = lerp(result[i], t, result[i+1]);
 }
}
function v1
lerp_vector(v1 a, v1 v, v1 b)
{// todo what?
 return v * (b-a);
}
function void
blossom_vector(v1 *result, i32 n, v1 t)
{// todo what?
 for_i32(i,0,n+1)
 {
  result[i] = lerp_vector(result[i], t, result[i+1]);
 }
}
function v1
bernstein(i32 n, i32 i, v1 t)
{
 return binomial(n,i) * integer_power(1-t, n-i) * integer_power(t,i);
}
function void
deCasteljau_rounds(v1 *b, i32 n, i32 rounds, v1 t)
{
 for_i32(r,0,rounds)
 {
  // NOTE Suppose n=3, first round we do 3 lerps between 4 control points
  i32 r2 = n-r;
  for_i32(j, 0, r2)
  {
   b[j] = lerp(b[j], t, b[j+1]);
  }
 }
}
function void
deCasteljau(v1 *b, i32 n, v1 t)
{
 deCasteljau_rounds(b,n,n,t);
}
function void
notebook_update(Notebook_State *state)
{// NOTE(kv) These should only run once... but whatever
 if(0)
 {// ;cramers_rule_test
  mat2 A = {
   1,3,
   3,5,
  };
  v2 b = {5,8};
  v2 x;
  v2 A0 = get_column(A,0);
  v2 A1 = get_column(A,1);
  v1 det = cross2d(A0, A1);
  x[0] = cross2d(b, A1) / det;
  x[1] = cross2d(A0, b) / det;
  
  v2 test = A * x;
  DEBUG_VALUE(x);
  DEBUG_VALUE(test);
  DEBUG_VALUE(b);
 }
 
 if(0)
 {// ;Menelaos_theorem_test @Menelaos_theorem
  add_graph();
  v1 t = .3f;
  v1 s = .6f;
  
  v2 b0 = {0,0};
  v2 b1 = {5,9};
  v2 b2 = {8,0};
  
  v2 b0t = lerp(b0,t,b1);
  v2 b0s = lerp(b0,s,b1);
  v2 b1t = lerp(b1,t,b2);
  v2 b1s = lerp(b1,s,b2);
  
  v2 bst = lerp(b0s, t, b1s);
  v2 bts = lerp(b0t, s, b1t);
  
  graph_point(b0);
  graph_point(b1);
  graph_point(b2);
  graph_point(b0t);
  graph_point(b0s);
  graph_point(b1t);
  graph_point(b1s);
  graph_point(bst);
  graph_point(bts);
 }
 
 if(0)
 {// ;test_Leibniz_formula
  v1 alpha = -0.9f;
  v1 beta  = 1.9f;
  v1 r = 4.f;
  v1 s = 3.f;
  v4 b = {2.f, 1.f, 5.f, 3.f};
  v1 value0 = bezier_iterative(b, alpha*r + beta*s);
  DEBUG_VALUE(value0);
  
  v1 value = 0.f;
  const i32 n = 3;
  for_i32(i,0,n+1)
  {
   v4 b_tmp = b;
   for_i32(j,0,i)  { blossom(b_tmp,n,r); }
   for_i32(j,0,n-i){ blossom(b_tmp,n,s); }
   
   value += (binomial(n,i) *
             integer_power(alpha, i) *
             integer_power(beta, n-i) *
             b_tmp[0]);
  }
  DEBUG_VALUE(value);
 }
 
 if(0)
 {
  const i32 n = 3;
  v4 b = {2.f, 1.f, 5.f, 3.f};
  v1 t = .6f;
  v1 lhs = bezier_iterative(b,t);
  DEBUG_VALUE(lhs);
  
  const i32 r = 0;
  v1 rhs = 0.f;
  
  {
   v4 b_tmp = b;
   deCasteljau_rounds(b_tmp,n,r,t);
   for_i32(i, 0, n-r+1)
   {
    rhs += bernstein(n-r,i,t) * b_tmp[i];
   }
   DEBUG_VALUE(rhs);
  }
 }
 
 if(0)
 {// ;linear_precision_identity_test
  {
   v4 b = {2.f, 1.f, 5.f, 3.f};
   const i32 n = 3;
   v1 t = .3f;
   v1 sum = 0.f;
   for_i32(i, 0, n+1)
   {
    sum += v1(i)/n * bernstein(n,i,t);
   }
   DEBUG_VALUE(sum);
   DEBUG_VALUE(t);
  }
  {
   i32 i = 3;
   i32 n = 5;
   DEBUG_VALUE(i*binomial(n,i));
   DEBUG_VALUE(n*binomial(n-1, i-1));
  }
 }
 
 if(1)
 {// I don't believe this!
  v1 a = 2.5f;
  v1 b = 12.f;
  v1 v = .3f;
  v1 t = .1f;
  v1 lhs = lerp(a,t+v,b) - lerp(a,t,b);
  v1 rhs = lerp(a,v,b)   - lerp(a,0,b);
  DEBUG_VALUE(lhs);
  DEBUG_VALUE(rhs);
 }
 
 if(1)
 {
  const i32 n = 3;
  v4 b = {2.f, 1.f, 5.f, 3.f};
  v1 t = .4f;
  v1 v = .1f;
  v1 lhs;
  {
   v4 b_lhs = b;
   deCasteljau(b_lhs, n, t+v);
   lhs = b_lhs[0];
   DEBUG_VALUE(lhs);
  }
  {
   v1 rhs = 0.f;
   for_i32(i, 0, n+1)
   {
    v4 b_tmp = b;
    for_repeat(n-i){ blossom(b_tmp, n, t); }
    for_repeat(i)  { blossom_vector(b_tmp, n, v); }
    rhs += binomial(n,i) * b_tmp[0];
   }
   DEBUG_VALUE(rhs);
  }
 }
}
//-