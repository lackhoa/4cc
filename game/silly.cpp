#if 0
using namespace std;
int N = 64, S, J;
function arrayof<int>
party_init(){
 arrayof<int> r = {};
 for_repeat(N) { r.push(rand() % 180); }
 return r;
}
arrayof<int> silly_v;
arrayof<i4> st;
#define A st.last()[0]
#define B st.last()[1]
#define I st.last()[2]
function void
FX(ImDrawList* d, ImVec2 a, ImVec2 b, ImVec2 s, ImVec4, float t)
{
 local_persist b32 init = 1;
 if (init){
  silly_v = party_init();
  init = 0;
  st.set_count(0);
  st.push(i4{ 0, N-1, 0, 0});
 }
 float bs = s.x / N;
 float y, c;
 for (int i = 0; i < N; i++) {
  y = a.y+silly_v[i];
  c = float(70+silly_v[i]);
  d->AddRectFilled(ImVec2(a.x+bs*i,y),
                   ImVec2(a.x+bs*(i+1),b.y),
                   IM_COL32(c,255-c,255,255));
 }
 d->AddText(a, max_u32, "Quicksort");
 if (st.count) {
  d->AddRect(ImVec2(a.x+bs*A,a.y),
             ImVec2(a.x+bs*(B+1),b.y),
             0xFF00FF00,
             8, 0, 2);
  switch (S) {
   case 0:case 5: {
    if (A>=B) { st.pop(); S+=3; }
    else { I=J=A; S++; }
   } break;
   
   case 1:case 6: {
    if (silly_v[J]>silly_v[B]) { macro_swap(silly_v[I], silly_v[J]); I++; }
    if (++J>B) { macro_swap(silly_v[I],silly_v[B]); S++; }
   } break;
   
   case 2:case 7:{ st.push({A,I-1,A,3}); S=0; }break;
   case 3:{ st.push({I+1,B,A,8}); S=5; }break;
   case 8:{ S = st.last()[3]; st.pop(); }break;
  }
 }else{
  init = true;
 }
}
#elif 0
#define V2 ImVec2
function void
FX (ImDrawList* d, V2 a, V2 b, V2 s, ImVec4 m, float t)
{
	int N = 25;
	v1 sp = s.y / N;
 v1 st = sinf(t) * 0.5f + 0.5f;
 const i1 nblobs = 3;
 v1 r[nblobs] = { 1500, 1087 + 200 * st, 1650, };
 v1 center[nblobs][2] = {
  { 150, 140 },
  { s.x * m.x, s.y * m.y },
  { 40 + 200 * st, 73 + 20*sinf(st * 5) },
 };
	for (int i = 0; i < N; i++) {
		v1 y = a.y + sp*(i+0.5f);
		for (v1 x = a.x;
       x <= b.x;
       x += 2) {
			float D = 0, o = 255;
			for (int k = 0; k < nblobs; k++) {
				D += r[k]/(squared(x-a.x-center[k][0]) +
               squared(y-a.y-center[k][1]));
   }
			if (D < 1)    continue;
			if (D > sp)  D = sp; 
			if (D < 1.15) o /= 2; 
			d->AddLine(V2(x,  y),
              V2(x+2,y),
              IM_COL32(239, 129, 19, o),
              D + sinf(2.3f*t + 0.3f*i));
		}
	}
}
#undef V2

#elif 1

#define V2 ImVec2
struct v2pair{ V2 first; V2 second; };
inline v1 lensq(V2 v)    { return squared(v.x) + squared(v.y); }
inline v1 lengthof(V2 v) { return square_root(lensq(v)); }
function void
init_v(arrayof<v2pair> &v, i1 sx, i1 sy){
 for_i32(i,0,v.count) {
  v[i].first = v[i].second = V2(rand() % sx, rand() % sy);
 }
}
void FX(ImDrawList* d,V2 a,V2 b,V2 s,ImVec4 mouse,float t)
{
 int N = 300;
 local_persist b32 init = true;
 local_persist arrayof<v2pair> v = {};
 i1 sx = i1(s.x);
 i1 sy = i1(s.y);
 if (init){
  init = false;
  v.set_count(N);
  init_v(v,sx,sy);
 }
 if (mouse.z == 0){
  init_v(v,sx,sy);
 }
 
	for_i32 (i,0,N) {
  auto &p = v[i];
		v1 dist = lengthof(p.first - p.second);
		if (dist > 0){
   V2 delta = (p.second - p.first);  // NOTE(kv) this is a unit vector
   p.first += delta*fval(1.1659f)*.01f;
  }
  if(1){
   if (dist < 4){
    p.second = V2(rand() % sx, rand() % sy);
   }
  }
	}
 
	for_i32(i,0,N) {
		for_i32(j,i+1,N) {
			v1 D = lensq(v[i].first - v[j].first);
   //NOTE(kv) think of it like (v[i].first - s/2) + (v[j].first - s/2)
   // it gets smaller when things are in the middle
			if (D < 400) {
    v1 T = lensq((v[i].first + v[j].first) - s) / 200;
    if (T > 255) T = 255;
    d->AddLine(a+v[i].first,
               a+v[j].first,
               IM_COL32(255-T, T, 255, 255-T),
               2);
   }
		}
	}
}
#undef V2
#endif

function void
FxTestBed()
{
 ImGuiIO& io = ImGui::GetIO();
 ImGui::Begin("FX", NULL, ImGuiWindowFlags_AlwaysAutoResize);
 ImVec2 size(320.0f, 180.0f);
 ImGui::InvisibleButton("canvas", size);
 ImVec2 p0 = ImGui::GetItemRectMin();
 ImVec2 p1 = ImGui::GetItemRectMax();
 ImDrawList* draw_list = ImGui::GetWindowDrawList();
 draw_list->PushClipRect(p0, p1);
 
 ImVec4 mouse_data;
 mouse_data.x = (io.MousePos.x - p0.x) / size.x;
 mouse_data.y = (io.MousePos.y - p0.y) / size.y;
 mouse_data.z = io.MouseDownDuration[0];
 mouse_data.w = io.MouseDownDuration[1];
 
 FX(draw_list, p0, p1, size, mouse_data, (float)ImGui::GetTime());
 draw_list->PopClipRect();
 ImGui::End();
}

//~