struct Modeler  // see game_init
{
 //-NOTE: Data
 Arena      *permanent_arena;
 kv_array<Vertex_Data> vertices;
 kv_array<Bezier_Data> beziers;
 
 //-NOTE: Editor
 u32 selected_obj_id;
 Vertex_Data previous_vertex;
 Bezier_Data previous_curve;
};

//-NOTE: Vertex
framework_storage Modeler *global_modeler;

internal i32
vertex_index_from_pointer(Vertex_Data *pointer)
{
 i32 result = i32(pointer - global_modeler->vertices.items);
 return result;
}

#if AD_IS_FRAMEWORK
internal Vertex_Data *
get_vertex_by_name(String name)
{
 Vertex_Data *result = 0;
 auto &modeler = *global_modeler;
 for_i32(vert_index, 1, modeler.vertices.count)
 {
  Vertex_Data *vertex = &modeler.vertices[vert_index];
  if( vertex->name == name )
  {
   result = vertex;
  }
 }
 return result;
}
#endif

void
send_vert_func(String name, v3 pos)
#if AD_IS_DRIVER
;
#else
{
 b32 is_new = false;
 auto &modeler = *global_modeler;
 Vertex_Data *result = get_vertex_by_name(name);
 
 if (result == 0)
 {
  is_new = true;
  result = &modeler.vertices.push2();
  *result = {};
  result->name = push_string_copy(modeler.permanent_arena, name);
 }
 
 result->pos = pos;
 if ( is_left() ) {
  // NOTE: weird and arbitrary logic
  result->object_index = current_object_index();
 } else {
  result->symx = true;
 }
}
#endif
//
#define send_vert(NAME)  send_vert_func(strlit(#NAME), NAME)

Bezier bez(v3 p0, v3 d0, v2 d3, v3 p3);

b32
send_bez_func(String name, String p0_name, v3 d0, v2 d3, String p3_name)
#if AD_IS_DRIVER
;
#else
{
 b32 ok = false;
 Bezier_Data *curve = 0;
 auto &modeler = *global_modeler;
 
 for_i32(curve_index, 1, modeler.beziers.count)
 {
  Bezier_Data *it = &modeler.beziers[curve_index];
  if ( string_match(it->name, name) )
  {
   curve = it;
  }
 }
 
 if (curve == 0)
 {
  curve = &modeler.beziers.push2();
  *curve = {};
  curve->name = push_string_copy(modeler.permanent_arena, name);
 }
 
 Vertex_Data *vert0 = get_vertex_by_name(p0_name);
 Vertex_Data *vert3 = get_vertex_by_name(p3_name);
 if (vert0 && vert3)
 {
  ok = true;
  Bez data = bez(vert0->pos, d0, d3, vert3->pos);
  curve->p0_index = vertex_index_from_pointer(vert0);
  curve->p1       = data[1];
  curve->p2       = data[2];
  curve->p3_index = vertex_index_from_pointer(vert3);
 }
 else
 {
  DEBUG_TEXT("bezier error: cannot find vertex");
 }
 
 if ( is_left() ) {
  curve->object_index = current_object_index();
 } else {
  curve->symx = true;
 }
 
 return ok;
}
#endif
//
#define send_bez(name, p0_name, d0, d3, p3_name) \
send_bez_func(strlit(#name), strlit(#p0_name), d0, d3, strlit(#p3_name))


//~ EOF
