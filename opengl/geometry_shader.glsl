layout (points) in;
layout (triangle_strip, max_vertices=4) out;

in Fragment_Data {
 v4 color;
 v3 uvw;
 v2 xy;
 v1 half_thickness;
} gs_in[];

in v2 radius_clip[];

out Fragment_Data {
 v4 color;
 v3 uvw;
 v2 xy;
 v1 half_thickness;
} gs_out;

void main()
{
 gs_out = gs_in[0];
 
 {
  v2 r   = radius_clip[0];
  v4 center = gl_in[0].gl_Position;
  
  v2 gs_out_values[4] = {V2(-1,-1), V2(+1,-1), V2(-1,+1), V2(+1,+1)};
  for(i1 index = 0; 
      index < 4;
      index++)
  {
   gs_out.xy   = gs_out_values[index];
   gl_Position     = center;
   gl_Position.xy += gs_out.xy * r;
   EmitVertex();
  }
  
  EndPrimitive();
 }
}
