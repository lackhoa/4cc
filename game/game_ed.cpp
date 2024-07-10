#if 0
internal Vertex_Data *
get_or_add_vertex(String name)
{
 Vertex_Data *result = 0;
 auto state = global_state;
 
 for_i1(vert_index, 1, state->vertex_count)
 {
  Vertex_Data *vertex = &state->vertices[vert_index];
  if ( string_match(vertex->name, name) )
  {
   result = vertex;
  }
 }
 
 if (result == 0)
 {
  kv_assert(state->vertex_count < alen(state->vertices));
  result = &state->vertices[state->vertex_count++];
  result->name = push_string_copy(&state->permanent_arena, name);
 }
 
 return result;
}

internal Bezier_Data *
get_or_add_bezf(String name)
{
 Bezier_Data *result = 0;
 Game_State *state = global_state;
 kv_assert(name.str);
 
 for_i1(curve_index, 1, state->curve_count)
 {
  String curve_name = state->curves[curve_index].name;
  if ( string_match(curve_name, name) )
  {
   result = &state->curves[curve_index];
  }
 }
 
 if (result == 0)
 {
  kv_assert(state->curve_count < alen(state->curves));
  result = &state->curves[state->curve_count++];
  result->name = push_string_copy(&state->permanent_arena, name);
 }
 
 return result;
}
//
force_inline void
send_bezf(String name, Bezier const&b)
{
 Bezier_Data *result = get_or_add_bezf(name);
 result->bezier = b;
}
//
#define send_bez(NAME)  send_bezf(strlit(#NAME), NAME)
#endif

//~ EOF
