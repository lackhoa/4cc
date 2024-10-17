struct Mesh
{
 v3 *vertices;
 i1 vertex_count;
 i3 *triangles;
 i1 triangle_count;
};

struct Clip_Result
{
 v3 a[3];
 v3 b[3];
};

global v1 F32_INVALID  = F32_MAX;
global v3 VEC3_INVALID = V3(F32_INVALID);

function v3
plane_segment_intersect(v3 n, v1 d, v3 p0, v3 p1)
{
 // start from p0, end at p1
 v3 result = VEC3_INVALID;
 v3 pd = p1-p0;
 if ( !almost_equal(pd, V3()) )
 {
  v1 denom = dot(n, pd);
  if (!almost_equal(denom, 0.f))
  {
   v1 t = (-d - dot(n, p0)) / denom;
   //NOTE: if you call this function then you expect these to intersect -> epsilons should comply
   v1 epsilon = 1e-6;
   if (0.f <= (t+epsilon) && (t-epsilon) <= 1.f)  // NOTE: Because we didn't normalize pd
   {
    result = p0 + t*pd;
   }
  }
 }
 return result;
}

force_inline b32
is_outside_plane(v3 n, v1 d, v3 p)
{
 return (dot(p, n) + d) >= 0.f;
}

// NOTE: Return either 1 or 2 triangles
function Clip_Result
clip_triangle(v3 p_in[3], v3 n, v1 d)
{
 Clip_Result result = {};
 b32 p0_outside = is_outside_plane(n,d,p_in[0]);
 b32 p1_outside = is_outside_plane(n,d,p_in[1]);
 b32 p2_outside = is_outside_plane(n,d,p_in[2]);
 if (p0_outside == p1_outside && p1_outside == p2_outside)
 {
  if (p0_outside == true)
  {// NOTE: Everything outside
   block_copy_array_dst(result.a, p_in);
  }
  //else everything is inside: the triangle is discarded
 }
 else
 {
  v3 p[3];
  block_copy_array_dst(p, p_in);
  
  // NOTE: Rank order: Outside to Inside (made-up 100%)
  if (!p0_outside)
  {
   if (p1_outside)
   {
    if (p2_outside)
    {
     macro_swap(p[0], p[2]);
     p2_outside = false;
    }
    else
    {
     macro_swap(p[0], p[1]);
     p1_outside = false;
    }
   }
   else
   {// NOTE: Doesn't matter whether p2 is outside or not
    macro_swap(p[0], p[2]);
    p2_outside = false;
   }
  }
  else if (!p1_outside)
  {
   if (p2_outside)
   {
    macro_swap(p[1], p[2]);
    p1_outside = true;
    p2_outside = false;
   }
  }
  p0_outside = true;  //NOTE: Just for courtesy
  
  // TODO: back-face culling is broken here!
  if (p1_outside)
  {// NOTE: 2 triangles
   v3 i12 = plane_segment_intersect(n, d, p[1], p[2]);
   v3 i02 = plane_segment_intersect(n, d, p[0], p[2]);
   
   v3 a[] = {p[0], p[1], i02};
   v3 b[] = {p[1], i12, i02};
   block_copy_array_dst(result.a, a);
   block_copy_array_dst(result.b, b);
  }
  else
  {// NOTE: 1 triangle
   v3 i01 = plane_segment_intersect(n, d, p[0], p[1]);
   v3 i02 = plane_segment_intersect(n, d, p[0], p[2]);
   v3 a[] = {p[0], i01, i02};
   block_copy_array_dst(result.a, a);
  }
 }
 
 return result;
}

function std_array<v3,2>
intersect_triangle(v3 n, v1 d, v3 p[3])
{
 std_array<v3,2> result = {VEC3_INVALID, VEC3_INVALID};
 
 v3 intersections[3] = {
  plane_segment_intersect(n,d, p[0], p[1]),
  plane_segment_intersect(n,d, p[0], p[2]),
  plane_segment_intersect(n,d, p[1], p[2]),
 };
 
 i1 fill_slot = 0;
 for_i1(index,0,3)
 {
  if (intersections[index] != VEC3_INVALID &&
      fill_slot < 2)
  {
   result[fill_slot++] = intersections[index];
  }
 }
 
 return result;
}

function Mesh
new_mesh(Arena *arena, 
         v3 *in_vertices, i1 vertex_count, 
         i3 *in_triangles, i1 triangle_count)
{
 Mesh result = { 
  .vertices    =push_array(arena, v3, vertex_count), 
  .vertex_count=vertex_count, 
  .triangles     =push_array(arena, i3, triangle_count),
  .triangle_count=triangle_count,
 };
 
 block_copy_count(result.vertices, in_vertices, vertex_count);
 block_copy_count(result.triangles, in_triangles, triangle_count);
 
 return result;
}

function Mesh
transformed_mesh(Arena *arena, mat4 const&mat, Mesh const&mesh)
{
 Mesh result = mesh;
 result.vertices = push_array(arena, v3, mesh.vertex_count);
 block_copy_count(result.vertices, mesh.vertices, mesh.vertex_count);
 for_i1(vertex_index,0,mesh.vertex_count)
 {
  result.vertices[vertex_index] = mat4vert(mat, mesh.vertices[vertex_index]);
 }
 return result;
}

inline i1
get_box_vertice_index(i3 a)
{
 u32 x = (a.x == 1) ? 1 : 0;
 u32 y = (a.y == 1) ? 1 : 0;
 u32 z = (a.z == 1) ? 1 : 0;
 return( (x << 2) | (y << 1) | (z << 0) );
}

typedef i3 *Triangle_Sbuf;

inline void
add_box_face(Triangle_Sbuf *triangles, i3 a, i3 b, i3 c, i3 d)
{
 i1 A = get_box_vertice_index(a);
 i1 B = get_box_vertice_index(b);
 i1 C = get_box_vertice_index(c);
 i1 D = get_box_vertice_index(d);
 arrpush(*triangles, I3(A,B,C));
 arrpush(*triangles, I3(A,C,D));
}

function void
add_box_opposite_faces(Triangle_Sbuf *triangles, i3 x, i3 y, i3 z)
{
 i3 O = z;
 add_box_face(triangles, O-x-y, O+x-y, O+x+y, O-x+y);
 O = -z;
 add_box_face(triangles, O-x+y, O+x+y, O+x-y, O-x-y);
}

global Mesh the_box_mesh;

function void
make_meshes(Arena *arena)
{
 arena_clear(arena);
 {// NOTE: box
  const i1 vertex_count = 8;
  v3 vertices[vertex_count];
  for_u32(index,0,vertex_count)
  {//NOTE: vertices
   v1 x = ((index >> 2) & 1) ? 1.f : -1.f;
   v1 y = ((index >> 1) & 1) ? 1.f : -1.f;
   v1 z = ((index >> 0) & 1) ? 1.f : -1.f;
   vertices[index] = V3(x,y,z);
  }
  
  Triangle_Sbuf triangles = 0;
  i3 x = I3(1,0,0);
  i3 y = I3(0,1,0);
  i3 z = I3(0,0,1);
  add_box_opposite_faces(&triangles, x,y,z);
  add_box_opposite_faces(&triangles, y,z,x);
  add_box_opposite_faces(&triangles, z,x,y);
  
  the_box_mesh = new_mesh(arena, vertices, vertex_count, triangles, arrlen(triangles));
  breakhere;
 }
}

function b32
triangle_valid(v3 triangle[3])
{
 return !(triangle[0] == V3() && 
          triangle[1] == V3());
}

//~ EOF
