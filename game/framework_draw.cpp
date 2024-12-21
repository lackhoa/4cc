global u32 draw_cycle_counter;

function void
poly3_inner_2(v3 points[3], argb color,
              v1 depth_offset, Poly_Flags flags)
{// NOTE: Triangle
 TIMED_BLOCK(draw_cycle_counter);
 Painter *p = painter;
 b32 shaded     = (flags & Poly_Shaded);
 b32 is_line    = (flags & Poly_Line);
 b32 is_overlay = (flags & Poly_Overlay);
 Render_Vertex vertices[3] = {};
 for_i32(i,0,3){ vertices[i].pos = points[i]; }
 // TODO(kv): @Speed The caller should be in charge of passing the color in!
 u32 prim_id = p->draw_prim_id;
 b32 is_hot      = prim_id == get_hot_prim_id();
 b32 is_selected = prim_id == selected_prim_id(p->modeler);
 b32 is_active   = is_prim_id_active(p->modeler, prim_id);
 // TODO(kv): @cleanup wtf is this code?
 if(is_hot || is_selected || is_active){
  argb hl_color = hot_color;
  if(is_hot){
   hl_color = (color == hot_color) ? hot_color2 : hot_color;
  }else if(is_selected){
   hl_color = argb_red;
  }else if(is_active){
   //TODO: pick color
   hl_color = hot_color;
  }
  color = hl_color;
 }else if(shaded && !is_line){
  //NOTE(kv) Shading logic doesn't apply to line, because we can see them just fine.
  // ;poly_shading
  v3 normal = noz(cross(points[1]-points[2], points[0]-points[2]));
  {
   v4 colorv = argb_unpack(color);
   v1 darkness_fuzz = 0.3f;
   colorv.rgb *= lerp(darkness_fuzz,absolute(normal.z),1.f);
   color = argb_pack(colorv);
  }
 }
 
 for_i32(index,0,3){
  vertices[index].color = color;
 }
 
 for_u32(index,0,alen(vertices)){
  Render_Vertex *vertex = vertices+index;
  vertex->uvw          = V3();
  vertex->depth_offset = depth_offset;
  vertex->prim_id      = p->draw_prim_id;
 }
 
 Vertex_Type type = Vertex_Poly;
 if(is_overlay){type = Vertex_Overlay;}
 draw__push_vertices(p->target, vertices, alen(vertices), type);
}
myinline void
poly3_inner(v3 a, v3 b, v3 c, argb color,
            v1 depth_offset, Poly_Flags flags);

//-