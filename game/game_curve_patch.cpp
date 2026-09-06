// NOTE(kv) Curve patch (plan-document-mouse-editing Q7/Q8, 2026-09-06): a document
// primitive that references 2-4 CURVE primitives and is evaluated at replay -- port of
// the tablet's patch.ts, minus smooth knots (one curve = one side). The referenced
// curves are chained head-to-tail by their shared table vertices into a loop:
//   2 curves closing a loop (lens)  -> ruled loft, second side reversed;
//   2 curves not closing             -> plain loft (rail B reversed if crossed);
//   3-4 curves closing a loop        -> Coons in loop order (3 = fourth side
//                                       collapsed to the first corner);
//   anything else                    -> nothing drawn.
// Reshaping a side reshapes the fill, since nothing is cached. Filled flat in the
// painter's background color with the group's fill_depth_offset (poly3_inner reads it).

global i32 const curve_patch_loft_columns = 24;
global i32 const curve_patch_loft_rows    = 4;
global i32 const curve_patch_coons_grid   = 16;

struct Curve_Patch_Grid
{// positions[i*(rows+1) + j] = point at u = i/columns, v = j/rows
 i32 columns, rows;
 v3 *positions;
};
struct Curve_Patch_Side
{
 Bezier bezier;  // world space
 b32 reversed;
 i32 start_vertex, end_vertex;  // table indices, already oriented
};

function b32
curve_patch_is_valid_ref(Recording &doc, i32 index)
{
 return (index >= 0 and index < doc.primitives.count and
         doc.primitives[index].type == Primitive_Type_Curve);
}

function v3
curve_patch_side_point(Curve_Patch_Side &side, v1 t)
{
 v3 result = bezier_sample(side.bezier.e, side.reversed ? 1.f - t : t);
 return result;
}

function Curve_Patch_Side
curve_patch_reversed(Curve_Patch_Side side)
{
 side.reversed = not side.reversed;
 i32 start = side.start_vertex;
 side.start_vertex = side.end_vertex;
 side.end_vertex = start;
 return side;
}

function v3 *
curve_patch_sample_side(Arena *arena, Curve_Patch_Side &side, i32 n)
{
 v3 *result = push_array(arena, v3, n+1);
 for_i32(i, 0, n+1){ result[i] = curve_patch_side_point(side, v1(i) / v1(n)); }
 return result;
}

// NOTE(kv) Evaluate the patch's surface grid in WORLD space (replay converts to the
// current bone, the hit-test to camera space). False = nothing to draw.
function b32
curve_patch_world_grid(Arena *arena, Recording &doc, Recorded_Primitive &patch_prim,
                       b32 is_right, Curve_Patch_Grid *out)
{
 Recorded_Curve_Patch &patch = patch_prim.curve_patch;
 i32 count = patch.curve_count;
 if(count < 2 or count > 4){ return false; }

 Curve_Patch_Side sides[4] = {};
 for_i32(ic, 0, count)
 {
  i32 index = patch.curve_index[ic];
  if(not curve_patch_is_valid_ref(doc, index)){ return false; }
  Recorded_Primitive curve = doc.primitives[index];
  Recorded_Group &group = doc.groups[curve.group_index];
  resolve_vertices(doc, curve);
  apply_shape_key(curve);
  Curve_Patch_Side &side = sides[ic];
  for_i32(i, 0, 4)
  {// NOTE(kv) Owning-bone -> world (Bone_None on a point = the curve's group bone).
   tvert point = curve.curve.bezier.e[i];
   Bone_ID bone_id = (point.bone_id.type == Bone_None ? group.bone_id : point.bone_id);
   point.v = mat4vert(get_bone(bone_id, is_right)->world_from_bone.forward, point.v);
   side.bezier.e[i] = point;
  }
  side.start_vertex = curve.vertex_index[0];
  side.end_vertex   = curve.vertex_index[1];
 }

 //-Chain into a loop by shared vertices (tablet chain_into_loop)
 Curve_Patch_Side loop[4] = {sides[0]};
 i32 loop_count = 1;
 b32 used[4] = {true};
 b32 closed = false;
 for(;;)
 {
  i32 loop_end = loop[loop_count-1].end_vertex;
  b32 found = false;
  for_i32(ic, 1, count)
  {
   if(used[ic]){ continue; }
   if(sides[ic].start_vertex == loop_end)
   { loop[loop_count++] = sides[ic]; used[ic] = true; found = true; break; }
   if(sides[ic].end_vertex == loop_end)
   { loop[loop_count++] = curve_patch_reversed(sides[ic]); used[ic] = true; found = true; break; }
  }
  if(not found){ break; }
  if(loop_count == count)
  {
   closed = (loop[loop_count-1].end_vertex == loop[0].start_vertex);
   break;
  }
 }

 b32 loft = false;
 Curve_Patch_Side rail_a = {}, rail_b = {};
 if(closed and count == 2)
 {// NOTE(kv) A lens: both sides run corner A -> corner B once the second is reversed.
  loft = true; rail_a = loop[0]; rail_b = curve_patch_reversed(loop[1]);
 }
 else if(not closed and count == 2)
 {// NOTE(kv) Detached rails: reverse B when the endpoint pairing is crossed.
  loft = true; rail_a = sides[0]; rail_b = sides[1];
  v3 a0 = curve_patch_side_point(rail_a, 0), a1 = curve_patch_side_point(rail_a, 1);
  v3 b0 = curve_patch_side_point(rail_b, 0), b1 = curve_patch_side_point(rail_b, 1);
  v1 straight = lengthof(a0-b0) + lengthof(a1-b1);
  v1 crossed  = lengthof(a0-b1) + lengthof(a1-b0);
  if(crossed < straight){ rail_b = curve_patch_reversed(rail_b); }
 }
 else if(not closed){ return false; }

 if(loft)
 {
  i32 columns = curve_patch_loft_columns, rows = curve_patch_loft_rows;
  v3 *A = curve_patch_sample_side(arena, rail_a, columns);
  v3 *B = curve_patch_sample_side(arena, rail_b, columns);
  out->columns = columns; out->rows = rows;
  out->positions = push_array(arena, v3, (columns+1)*(rows+1));
  for_i32(i, 0, columns+1)
  {
   for_i32(j, 0, rows+1)
   {
    out->positions[i*(rows+1)+j] = lerp(A[i], v1(j)/v1(rows), B[i]);
   }
  }
  return true;
 }

 {// NOTE(kv) Coons (tablet coons_surface_grid): bottom (s 0->1), right (t 0->1), top
  // and left run backwards along the loop, so index them from the far end.
  i32 n = curve_patch_coons_grid;
  v3 *bottom = curve_patch_sample_side(arena, loop[0], n);
  v3 *right  = curve_patch_sample_side(arena, loop[1], n);
  v3 *top_backwards = curve_patch_sample_side(arena, loop[2], n);
  v3 *left_backwards;
  if(count == 4){ left_backwards = curve_patch_sample_side(arena, loop[3], n); }
  else
  {
   left_backwards = push_array(arena, v3, n+1);
   for_i32(j, 0, n+1){ left_backwards[j] = bottom[0]; }
  }
  auto top  = [&](i32 i) -> v3 { return top_backwards[n-i]; };
  auto left = [&](i32 j) -> v3 { return left_backwards[n-j]; };
  v3 c00 = bottom[0], c10 = bottom[n], c11 = right[n], c01 = top(0);
  out->columns = n; out->rows = n;
  out->positions = push_array(arena, v3, (n+1)*(n+1));
  for_i32(i, 0, n+1)
  {
   for_i32(j, 0, n+1)
   {
    v1 s = v1(i)/v1(n), t = v1(j)/v1(n);
    v3 ruled = lerp(bottom[i], t, top(i)) + lerp(left(j), s, right[j]);
    v3 bilinear = (c00*((1-s)*(1-t)) + c10*(s*(1-t)) + c01*((1-s)*t) + c11*(s*t));
    out->positions[i*(n+1)+j] = ruled - bilinear;
   }
  }
  return true;
 }
}

// NOTE(kv) Replay draw: current bone is the patch group's bone (set by the replay loop).
function void
draw_curve_patch(Recording &doc, Recorded_Primitive &prim, b32 is_right)
{
 if(not is_fill_enabled()){ return; }
 Scratch_Block tmp;
 Curve_Patch_Grid grid = {};
 if(not curve_patch_world_grid(tmp, doc, prim, is_right, &grid)){ return; }
 mat4 bone_from_world = current_world_from_bone().inverse;
 argb color = painter->background_color;
 Poly_Flags flags = to_poly_flags(Fill_Flags{});
 i32 stride = grid.rows+1;
 auto at = [&](i32 i, i32 j) -> v3 { return mat4vert(bone_from_world, grid.positions[i*stride+j]); };
 for_i32(j, 0, grid.rows)
 {
  for_i32(i, 0, grid.columns)
  {
   v3 p00 = at(i,j), p10 = at(i+1,j), p11 = at(i+1,j+1), p01 = at(i,j+1);
   poly3_inner({p00,p10,p11}, repeat3(color), flags);
   poly3_inner({p00,p11,p01}, repeat3(color), flags);
  }
 }
}

// NOTE(kv) Hit-test triangles in camera space (see get_primitive_hit_by_mouse).
function void
push_curve_patch_hit_triangles(Arena *arena, darray(Poly3) *triangles,
                               Recording &doc, Recorded_Primitive &prim, b32 is_right,
                               mat4 const &cam_from_world)
{
 Curve_Patch_Grid grid = {};
 if(not curve_patch_world_grid(arena, doc, prim, is_right, &grid)){ return; }
 i32 stride = grid.rows+1;
 auto at = [&](i32 i, i32 j) -> v3 { return mat4vert(cam_from_world, grid.positions[i*stride+j]); };
 set_cap_min(triangles, 2*grid.rows*grid.columns);
 for_i32(j, 0, grid.rows)
 {
  for_i32(i, 0, grid.columns)
  {
   v3 p00 = at(i,j), p10 = at(i+1,j), p11 = at(i+1,j+1), p01 = at(i,j+1);
   push(triangles, Poly3{p00,p10,p11});
   push(triangles, Poly3{p00,p11,p01});
  }
 }
}

//~ Editing (Q8): selection + make/delete, shared by the right-click menu and the channel.

function void
document_selection_toggle(Game_State *state, i32 prim_index)
{
 Document_Selection &sel = state->document_selection;
 Recording &doc = state->model.recordings.document;
 if(not curve_patch_is_valid_ref(doc, prim_index)){ return; }  // only curves select
 for_i32(i, 0, sel.count)
 {
  if(sel.prim_index[i] == prim_index)
  {
   for_i32(j, i, sel.count-1){ sel.prim_index[j] = sel.prim_index[j+1]; }
   sel.count--;
   return;
  }
 }
 if(sel.count < alen(sel.prim_index)){ sel.prim_index[sel.count++] = prim_index; }
}

function b32
document_make_patch(Game_State *state, i32 *curve_index, i32 count)
{
 Recording &doc = state->model.recordings.document;
 if(count < 2 or count > 4){ log_error("make_patch: need 2-4 curves, got %d", count); return false; }
 for_i32(i, 0, count)
 {
  if(not curve_patch_is_valid_ref(doc, curve_index[i]))
  { log_error("make_patch: %d is not a curve primitive", curve_index[i]); return false; }
  for_i32(j, 0, i)
  {
   if(curve_index[j] == curve_index[i])
   { log_error("make_patch: duplicate curve %d", curve_index[i]); return false; }
  }
 }
 Recorded_Primitive prim = {};
 prim.type = Primitive_Type_Curve_Patch;
 prim.group_index = doc.primitives[curve_index[0]].group_index;
 prim.curve_patch.curve_count = count;
 for_i32(i, 0, count){ prim.curve_patch.curve_index[i] = curve_index[i]; }
 push(&doc.primitives, prim);
 doc.captured = true;
 state->document_selection.count = 0;
 return save_document_file(state);
}

function b32
document_delete_patch(Game_State *state, i32 prim_index)
{
 Recording &doc = state->model.recordings.document;
 if(prim_index < 0 or prim_index >= doc.primitives.count or
    doc.primitives[prim_index].type != Primitive_Type_Curve_Patch)
 { log_error("delete_patch: %d is not a curve patch", prim_index); return false; }
 for_i32(i, prim_index, doc.primitives.count-1){ doc.primitives[i] = doc.primitives[i+1]; }
 doc.primitives.count--;
 for_i32(i, 0, doc.primitives.count)
 {// NOTE(kv) References past the hole shift down by one.
  Recorded_Primitive &other = doc.primitives[i];
  if(other.type != Primitive_Type_Curve_Patch){ continue; }
  for_i32(ic, 0, other.curve_patch.curve_count)
  {
   if(other.curve_patch.curve_index[ic] > prim_index){ other.curve_patch.curve_index[ic]--; }
  }
 }
 state->document_selection.count = 0;
 state->document_edit = {};
 return save_document_file(state);
}
