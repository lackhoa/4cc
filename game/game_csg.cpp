//~NOTE: Utilities for building csg trees 
// @game_optimization ~/4ed/code/kv-build.py

force_inline CSG_Tree *
csg_push_tree(CSG_Type type)
{
 CSG_Tree *tree = push_struct_zero(viewport_frame_arena, CSG_Tree);
 tree->type = type;
 //tree->symx = global_painter.symx;
 {
  CSG_Group *c = global_csg_group;
  sll_queue_push(c->first, c->last, tree);
  c->count++;
 }
 
 tree->negated = global_painter.csg_negated;
 
 return tree;
}

force_inline v1
get_object_scale()
{
 // @object_uniform_scale
 return get_xscale(object_transform);
}

internal CSG_Tree *
csg_sphere(v3 center, v1 radius)
{
 v1 object_scale = get_object_scale();
 CSG_Tree *tree = csg_push_tree(CSG_Sphere);
 tree->center =  mat4vert_no_div(raycast_transform, center);
 tree->radius = object_scale * radius;
 
 return tree;
}

internal void
csg_plane_inner(v3 n, v1 d)
{
 v4 plane = plane_transform(raycast_transform, n, d);
 CSG_Tree *tree = csg_push_tree(CSG_Plane);
 tree->n = plane.xyz;
 tree->d = plane.w;
}

internal void
csg_plane(v3 n, v1 d)
{
 csg_plane_inner(n, d);
 if (global_painter.symx)
 {
  csg_plane_inner(negateX(n), d);
 }
}

internal CSG_Tree *
csg_box(v3 center, v3 radius)
{
 v1 object_scale = get_object_scale();
 
 // NOTE: knock the scale out
 mat4 to_aabb = raycast_transform.inv;
 for_i32(r,0,3)
 {
  for_i32(c,0,4)
  {
   to_aabb.e[r][c] *= object_scale;
  }
 }
 
 CSG_Tree *tree = csg_push_tree(CSG_Box);
 tree->to_aabb    = to_aabb;
 tree->box_radius = object_scale * radius;
 
 return tree;
}
#if 0
// NOTE: I anticipate we wouldn't actually use rotation (relative to objects) that much,
// but still, objects do rotate...
force_inline CSG_Tree *
csg_box(v3 center, v3 radius)
{
 return csg_box(center, mat3_identity, radius);
}
#endif

internal CSG_Tree *
csg_union(CSG_Tree *l, CSG_Tree *r)
{
 CSG_Tree *tree = csg_push_tree(CSG_Union);
 tree->l = l;
 tree->r = r;
 
 return tree;
}

//~NOTE: Operations
force_inline v1
sd_intersect(v1 d1, v1 d2)
{
 return macro_max(d1, d2);
}

force_inline v1
sd_union(v1 d1, v1 d2)
{
 return macro_min(d1, d2);
}

#if 0
//~NOTE: Shapes
force_inline v1
sd_sphere(v3 center, v1 radius, v3 pos)
{
 v1 result = lengthof(pos-center) - radius;
 return result;
}

force_inline v1
sd_plane(v3 unit_normal, v1 d, v3 pos)
{//NOTE: The plane normal points "outside" of the plane, which translates directly to this sdf.
 return dot(unit_normal, pos) + d;
}

internal v1
sd_box(v3 center, v3 dim, v3 pos)
{
 v3 q = absolute(pos-center) - dim;
 return (lengthof(max(q,vec3())) +
         macro_min(sd_intersect(q.x,sd_intersect(q.y,q.z)), 0.f));
}
#endif


//~EOF
