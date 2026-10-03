// Putting a point onto a reference mesh (plan-skin-from-skull-wrap.md): the wrap that
// derives the first skin drawing from the skull drawing moves each copied vertex onto the
// skin mesh with these. Brute force over every triangle: a wrap is a few dozen points
// against ~16k triangles, run once.
import { V3, v3_add, v3_dot, v3_length, v3_scale, v3_sub } from "./math";
import { ReferenceMesh } from "./reference";
import { ray_triangle_distance } from "./reference_skull_view";

// Closest point to `p` on triangle abc (Ericson, Real-Time Collision Detection 5.1.5:
// classify `p` against the vertex, edge and face regions).
function nearest_point_on_triangle(p: V3, a: V3, b: V3, c: V3): V3 {
  const ab = v3_sub(b, a), ac = v3_sub(c, a), ap = v3_sub(p, a);
  const d1 = v3_dot(ab, ap), d2 = v3_dot(ac, ap);
  if (d1 <= 0 && d2 <= 0) return a;
  const bp = v3_sub(p, b);
  const d3 = v3_dot(ab, bp), d4 = v3_dot(ac, bp);
  if (d3 >= 0 && d4 <= d3) return b;
  const vc = d1 * d4 - d3 * d2;
  if (vc <= 0 && d1 >= 0 && d3 <= 0) return v3_add(a, v3_scale(ab, d1 / (d1 - d3)));
  const cp = v3_sub(p, c);
  const d5 = v3_dot(ab, cp), d6 = v3_dot(ac, cp);
  if (d6 >= 0 && d5 <= d6) return c;
  const vb = d5 * d2 - d1 * d6;
  if (vb <= 0 && d2 >= 0 && d6 <= 0) return v3_add(a, v3_scale(ac, d2 / (d2 - d6)));
  const va = d3 * d6 - d5 * d4;
  if (va <= 0 && d4 - d3 >= 0 && d5 - d6 >= 0) {
    return v3_add(b, v3_scale(v3_sub(c, b), (d4 - d3) / ((d4 - d3) + (d5 - d6))));
  }
  const denominator = 1 / (va + vb + vc);
  return v3_add(a, v3_add(v3_scale(ab, vb * denominator), v3_scale(ac, vc * denominator)));
}

// The point of the mesh surface closest to `point`. The mesh has at least one triangle.
export function nearest_point_on_mesh(mesh: ReferenceMesh, point: V3): V3 {
  let best = mesh.triangle_positions[0];
  let best_distance = Infinity;
  for (let i = 0; i < mesh.triangle_positions.length; i += 3) {
    const candidate = nearest_point_on_triangle(point, mesh.triangle_positions[i], mesh.triangle_positions[i + 1], mesh.triangle_positions[i + 2]);
    const distance = v3_length(v3_sub(candidate, point));
    if (distance < best_distance) { best_distance = distance; best = candidate; }
  }
  return best;
}

// How project_vertex_onto_mesh found its point: "ray" = first hit along the ray;
// "nearest" = the ray missed or hit beyond max_push, and the closest mesh point was in
// range; "capped" = even the closest mesh point is beyond max_push, so the point stops
// max_push towards it (short of the mesh).
export type MeshProjectionMethod = "ray" | "nearest" | "capped";
export type MeshProjection = { position: V3; method: MeshProjectionMethod };

// `point` moved onto the mesh: along `direction` (unit) when the first hit is within
// `max_push`, else to the closest mesh point, and never farther than `max_push`.
export function project_vertex_onto_mesh(mesh: ReferenceMesh, point: V3, direction: V3, max_push: number): MeshProjection {
  let ray_distance = Infinity;
  for (let i = 0; i < mesh.triangle_positions.length; i += 3) {
    const t = ray_triangle_distance(point, direction, mesh.triangle_positions[i], mesh.triangle_positions[i + 1], mesh.triangle_positions[i + 2]);
    if (t !== null && t < ray_distance) ray_distance = t;
  }
  if (ray_distance <= max_push) return { position: v3_add(point, v3_scale(direction, ray_distance)), method: "ray" };
  const nearest = nearest_point_on_mesh(mesh, point);
  const to_nearest = v3_sub(nearest, point);
  const nearest_distance = v3_length(to_nearest);
  if (nearest_distance <= max_push) return { position: nearest, method: "nearest" };
  return { position: v3_add(point, v3_scale(to_nearest, max_push / nearest_distance)), method: "capped" };
}
