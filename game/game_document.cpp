//-NOTE(kv) Document export (draw-as-data, plan-data-only-region-poc Q95/Q97): move a
// tagged region of the live capture into the persisted document.
// `export_group <tag>` (debug channel) = copy every group tagged `tag` (the tag is
// inherited by the whole scope subtree, so this IS the subtree) plus its primitives
// and vertices into Model.recordings.document, replacing any earlier export of
// the same tag, weld coincident vertices, save driver.document.ad. The code block can
// then be deleted; the document replays in its place.

struct Document_Export_Result
{
 b32 ok;
 i32 group_count;      // groups exported this call
 i32 primitive_count;
 i32 vertex_count;     // whole document table, after welding
 i32 welded_count;     // vertices merged away by the weld
};

function void
weld_document_vertices(Recording &doc, Arena *tmp, i32 *welded_count_out)
{// NOTE(kv) Same bone + distance < eps in bone space -> one vertex (Q95). Rebuilds
 // the table compacted (unreferenced vertices drop out) and remaps every vertex index.
 v1 const weld_eps = 1e-5f;
 i32 old_count = doc.vertices.count;
 i32 *remap = push_array(tmp, i32, old_count);
 for_i32(i,0,old_count){ remap[i] = -1; }

 darray(Recorded_Vertex) welded = {};
 init_dynamic(welded, tmp, maximum(1, old_count));
 i32 welded_count = 0;
 for_i32(iprim, 0, doc.primitives.count)
 {// NOTE(kv) Walk references (not the raw table) so the result only holds live vertices.
  Recorded_Primitive &prim = doc.primitives.items[iprim];
  i32 vertex_count = primitive_vertex_count(prim.type);
  for_i32(ivertex,0,vertex_count)
  {
   i32 old_index = prim.vertex_index[ivertex];
   if(remap[old_index] == -1)
   {
    Recorded_Vertex vertex = doc.vertices.items[old_index];
    i32 found = -1;
    for_i32(j,0,welded.count)
    {
     Recorded_Vertex &candidate = welded.items[j];
     if(candidate.bone == vertex.bone and
        length_squared(candidate.p - vertex.p) < weld_eps*weld_eps)
     { found = j; break; }
    }
    if(found == -1){ found = welded.count; push(&welded, vertex); }
    else           { welded_count++; }
    remap[old_index] = found;
   }
   prim.vertex_index[ivertex] = remap[old_index];
  }
 }

 init_dynamic(doc.vertices, &doc.arena, maximum(1, welded.count));
 set_count(&doc.vertices, welded.count);
 block_copy(doc.vertices.items, welded.items, sizeof(Recorded_Vertex) * welded.count);
 *welded_count_out = welded_count;
}

function Document_Export_Result
export_group_to_document(Game_State *state, Group_Vis tag)
{
 Document_Export_Result result = {};
 Model *m = &state->model;
 Recording &src = m->recordings.recording;
 Recording &doc = m->recordings.document;
 if(not src.captured){ log_error("export_group: no live capture"); return result; }

 Scratch_Scope tmp;
 // NOTE(kv) Stage the merged document on the scratch arena (old document minus this
 // tag, then the fresh subtree), then rebuild the document arena from the staging.
 darray(Recorded_Group)     groups     = {};
 darray(Recorded_Primitive) primitives = {};
 darray(Recorded_Vertex)    vertices   = {};
 init_dynamic(groups,     tmp, maximum(1, doc.groups.count     + src.groups.count));
 init_dynamic(primitives, tmp, maximum(1, doc.primitives.count + src.primitives.count));
 init_dynamic(vertices,   tmp, maximum(1, doc.vertices.count   + src.vertices.count));

 {//-Old document, minus any earlier export of this tag
  i32 *group_remap = push_array(tmp, i32, maximum(1, doc.groups.count));
  for_i32(ig,0,doc.groups.count)
  {
   Recorded_Group group = doc.groups.items[ig];
   group_remap[ig] = -1;
   if(group.vis_tag != tag)
   {
    if(group.parent_index != -1){ group.parent_index = group_remap[group.parent_index]; }
    group_remap[ig] = groups.count;
    push(&groups, group);
   }
  }
  i32 *vertex_remap = push_array(tmp, i32, maximum(1, doc.vertices.count));
  for_i32(iv,0,doc.vertices.count){ vertex_remap[iv] = -1; }
  for_i32(ip,0,doc.primitives.count)
  {
   Recorded_Primitive prim = doc.primitives.items[ip];
   i32 new_group = group_remap[prim.group_index];
   if(new_group != -1)
   {
    prim.group_index = new_group;
    prim.location = {};
    i32 vertex_count = primitive_vertex_count(prim.type);
    for_i32(ic,0,vertex_count)
    {
     i32 old_vertex = prim.vertex_index[ic];
     if(vertex_remap[old_vertex] == -1)
     {
      vertex_remap[old_vertex] = vertices.count;
      push(&vertices, doc.vertices.items[old_vertex]);
     }
     prim.vertex_index[ic] = vertex_remap[old_vertex];
    }
    push(&primitives, prim);
   }
  }
 }

 {//-The tagged subtree from the live capture
  i32 *group_remap = push_array(tmp, i32, maximum(1, src.groups.count));
  for_i32(ig,0,src.groups.count)
  {
   Recorded_Group group = src.groups.items[ig];
   group_remap[ig] = -1;
   if(group.vis_tag == tag)
   {// NOTE(kv) Subtree roots (parent not tagged) re-parent to the document root.
    i32 parent = (group.parent_index == -1 ? -1 : group_remap[group.parent_index]);
    group.parent_index = parent;
    // NOTE(kv) Source locations die with the export: the range indices point into
    // driver.kc's marked positions as they were, and the code is about to be deleted,
    // so keeping them would hot-highlight whatever code lands on those slots later.
    group.location = {};
    group_remap[ig] = groups.count;
    push(&groups, group);
    result.group_count++;
   }
  }
  i32 *vertex_remap = push_array(tmp, i32, maximum(1, src.vertices.count));
  for_i32(iv,0,src.vertices.count){ vertex_remap[iv] = -1; }
  for_i32(ip,0,src.primitives.count)
  {
   Recorded_Primitive prim = src.primitives.items[ip];
   i32 new_group = group_remap[prim.group_index];
   if(new_group != -1)
   {
    prim.group_index = new_group;
    prim.location = {};
    i32 vertex_count = primitive_vertex_count(prim.type);
    for_i32(ic,0,vertex_count)
    {
     i32 old_vertex = prim.vertex_index[ic];
     if(vertex_remap[old_vertex] == -1)
     {
      vertex_remap[old_vertex] = vertices.count;
      push(&vertices, src.vertices.items[old_vertex]);
     }
     prim.vertex_index[ic] = vertex_remap[old_vertex];
    }
    push(&primitives, prim);
    result.primitive_count++;
   }
  }
 }
 {//-Rebuild the document arena from the staging
  arena_clear(&doc.arena);
  init_dynamic(doc.groups,     &doc.arena, maximum(1, groups.count));
  init_dynamic(doc.primitives, &doc.arena, maximum(1, primitives.count));
  init_dynamic(doc.vertices,   &doc.arena, maximum(1, vertices.count));
  set_count(&doc.groups, groups.count);
  block_copy(doc.groups.items, groups.items, sizeof(Recorded_Group) * groups.count);
  set_count(&doc.primitives, primitives.count);
  block_copy(doc.primitives.items, primitives.items, sizeof(Recorded_Primitive) * primitives.count);
  set_count(&doc.vertices, vertices.count);
  block_copy(doc.vertices.items, vertices.items, sizeof(Recorded_Vertex) * vertices.count);
  for_i32(ip,0,doc.primitives.count)
  {// NOTE(kv) Image filenames point into the SOURCE arena; own them.
   Recorded_Primitive &prim = doc.primitives.items[ip];
   if(prim.type == Primitive_Type_Image)
   {
    Stringz filename = prim.image.filename;
    u8 *bytes = cast(u8 *)push_size(&doc.arena, filename.len + 1);
    block_copy(bytes, filename.str, filename.len);
    bytes[filename.len] = 0;
    prim.image.filename.str = bytes;
   }
  }
  doc.captured = (doc.primitives.count > 0);
 }

 weld_document_vertices(doc, tmp, &result.welded_count);
 // NOTE(kv) The weld runs over the whole table, so a per-region vertex count isn't
 // separable; report the whole table (the PoC has one region anyway).
 result.vertex_count = doc.vertices.count;

 log_string("export_group %s: %d groups, %d primitives, %d vertices (%d welded); document now %d/%d/%d",
            group_vis_names[tag], result.group_count, result.primitive_count,
            result.vertex_count, result.welded_count,
            doc.groups.count, doc.primitives.count, doc.vertices.count);
 result.ok = save_document_file(state);
 return result;
}
//-EOF
