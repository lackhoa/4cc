//------------
// NOTE(kv): I'm just putting some unrelated educational code here.
// Because can't be bothered to make a new repo.
// Also I can just share the code and the build script ->
// cutting down on some nonsense!
//------------
global Arena notebook_frame_arena;

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
begin_graph_()
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
#define GraphBlock  begin_graph_()

function ImU32
argb_to_abgr(argb color)
{// NOTE(kv) Incidentally it also does the opposite.
 u8 *array = (u8 *)&color;
 u8 r = array[2];
 u8 b = array[0];
 array[2] = b;
 array[0] = r;
 return color;
}
function void
graph_bezier(v2 *b, i32 degree, argb color0=sargb_bright_blue)
{
 abgr color = argb_to_abgr(color0);
 ImDrawList *draw_list = ImGui::GetWindowDrawList();
 const i32 max_degree = 3;
 if(degree <= max_degree)
 {
  i32 n = degree;
  
  v2 screen_points[max_degree+1];
  for_i32(i, 0, n+1)
  {
   screen_points[i] = graph_transform_point(b[i]);
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

function void
graph_point(v2 point, ImGuiCol color = ImGui::GetColorU32(ImGuiCol_CheckMark))
{
 v2 center = graph_transform_point(point);
 
 ImDrawList *draw_list = ImGui::GetWindowDrawList();
 draw_list->AddCircle(center, 4.f, color, 16, 2.f);
}
function void
graph_line(v2 a, v2 b, ImGuiCol color = ImGui::GetColorU32(ImGuiCol_CheckMark))
{
 a = graph_transform_point(a);
 b = graph_transform_point(b);
 
 ImDrawList *draw_list = ImGui::GetWindowDrawList();
 draw_list->AddLine(a,b,color);
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
template<class T> function T
lerp_vector(T a, v1 v, T b)
{
 return v * (b-a);
}
template<class T> function void
blossom(T *result, i32 n, v1 t, b32 is_vector=0)
{
 for_i32(i,0,n+1)
 {
  result[i] = (is_vector ?
               lerp_vector(result[i], t, result[i+1]) :
               lerp       (result[i], t, result[i+1]));
 }
}
template<class T> myinline void
blossom_vector(T *result, i32 n, v1 t)
{
 blossom(result, n, t, 1);
}
function v1
bernstein(i32 n, i32 i, v1 t)
{
 return binomial(n,i) * integer_power(1-t, n-i) * integer_power(t,i);
}
template<class T> function void
dcb_rounds(T *b, i32 n, i32 rounds, v1 t, b32 is_vector=0)
{
 for_i32(r,0,rounds)
 {
  // NOTE Suppose n=3, first round we do 3 lerps between 4 control points
  i32 r2 = n-r;
  for_i32(j, 0, r2)
  {
   if(is_vector) { b[j] = lerp_vector(b[j], t, b[j+1]); }
   else          { b[j] = lerp       (b[j], t, b[j+1]); }
  }
 }
}
template<class T> function T
dcb(T *b, i32 n, v1 t, b32 is_vector=0)
{// NOTE(kv) Collapse to a single value.
 Scratch_Block tmp;
 T *b_tmp = push_array(tmp, T, n+1);
 block_copy_count(b_tmp, b, n+1);
 i32 rounds = n;
 dcb_rounds(b_tmp, n, rounds, t, is_vector);
 return b_tmp[0];
}
function v1
dcb(v4 b, v1 t, b32 is_vector=0)
{// #stroustrup
 i32 n = 3;
 return dcb(b.e, n, t, is_vector);
}

function v1
forward_difference(v1 *e, i32 i)
{
 v1 result = 0.f;
 if(i == 0)
 {
  result = e[0];
 }
 else if(i > 0)
 {
  result = forward_difference(e+1,i-1) - forward_difference(e,i-1);
 }
 else { InvalidCodePath; }
 
 return result;
}

#include "backprop.cpp"
#include "nb_internet.cpp"
#include "notebook.gen.cpp"  // "notebook.kc"
//-