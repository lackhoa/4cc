// Merge two strokes that share a vertex into one stroke spanning their outer
// endpoints, least-squares-fitted to the combined sampled shape (a single
// cubic — sharp corners at the junction get smoothed, that's the point).
// Surfaces built on stroke A carry over to the merged stroke; surfaces built
// on stroke B are deleted with it (delete_stroke semantics), and the junction
// vertex is garbage-collected when nothing else uses it.

import { StrokeId, TabletDocument, VertexId, bezier_point, delete_stroke, stroke_by_id, stroke_control_points, stroke_handles_from_control_points, vertex_position } from "./document";
import { V3, v3, v3_add, v3_length, v3_scale, v3_sub } from "./math";

const MERGE_SAMPLES_PER_STROKE = 24;

// Sample one stroke's cubic as a polyline running from a given endpoint vertex
// to the other (reversing the parameterization when needed).
function sample_stroke_from_vertex(
  tablet_document: TabletDocument, stroke_id: StrokeId, from_vertex: VertexId,
): V3[] {
  const stroke = stroke_by_id(tablet_document, stroke_id);
  const points = stroke_control_points(stroke, tablet_document);
  const from_p0 = stroke.p0_vertex === from_vertex;
  const samples: V3[] = [];
  for (let i = 0; i <= MERGE_SAMPLES_PER_STROKE; i++) {
    const t = i / MERGE_SAMPLES_PER_STROKE;
    samples.push(bezier_point(points, from_p0 ? t : 1 - t));
  }
  return samples;
}

// Least-squares fit of the two interior control points to a sampled path
// (endpoints fixed, chord-length parameterization), converted to the offset
// handle representation. Falls back to a straight stroke's handles when the
// path is too short or the normal equations are degenerate.
function fit_stroke_handles(path: V3[], p0: V3, p3: V3): { d0: V3; d3: V3 } {
  const one_third = v3_scale(v3_add(v3_scale(p0, 2), p3), 1 / 3);
  const two_thirds = v3_scale(v3_add(p0, v3_scale(p3, 2)), 1 / 3);
  const straight = stroke_handles_from_control_points(p0, one_third, two_thirds, p3);
  if (path.length < 3) return straight;

  // Chord-length parameter per sample, normalized to [0, 1].
  const parameters: number[] = [0];
  let total_length = 0;
  for (let i = 1; i < path.length; i++) {
    total_length += v3_length(v3_sub(path[i], path[i - 1]));
    parameters.push(total_length);
  }
  if (total_length < 1e-9) return straight;
  for (let i = 0; i < parameters.length; i++) parameters[i] /= total_length;

  // Normal equations for min sum |q_i - (B0 p0 + B1 p1 + B2 p2 + B3 p3)|²
  // over p1, p2 — a 2x2 system with vector right-hand sides.
  let a11 = 0, a12 = 0, a22 = 0;
  let c1 = v3(0, 0, 0), c2 = v3(0, 0, 0);
  for (let i = 0; i < path.length; i++) {
    const t = parameters[i];
    const s = 1 - t;
    const b0 = s * s * s, b1 = 3 * s * s * t, b2 = 3 * s * t * t, b3 = t * t * t;
    const target = v3_sub(v3_sub(path[i], v3_scale(p0, b0)), v3_scale(p3, b3));
    a11 += b1 * b1;
    a12 += b1 * b2;
    a22 += b2 * b2;
    c1 = v3_add(c1, v3_scale(target, b1));
    c2 = v3_add(c2, v3_scale(target, b2));
  }
  const determinant = a11 * a22 - a12 * a12;
  if (Math.abs(determinant) < 1e-12) return straight;
  const p1 = v3_scale(v3_sub(v3_scale(c1, a22), v3_scale(c2, a12)), 1 / determinant);
  const p2 = v3_scale(v3_sub(v3_scale(c2, a11), v3_scale(c1, a12)), 1 / determinant);

  // The handle conversion swings the fitted p2 into p1's plane.
  return stroke_handles_from_control_points(p0, p1, p2, p3);
}

// Merge stroke B into stroke A. Returns the merged stroke's id (A's), or
// null when the strokes don't share exactly one vertex (not adjacent, or a
// closed two-stroke loop — merging that would collapse the loop into a
// degenerate stroke).
export function merge_adjacent_strokes(
  tablet_document: TabletDocument, stroke_a_id: StrokeId, stroke_b_id: StrokeId,
): StrokeId | null {
  const stroke_a = stroke_by_id(tablet_document, stroke_a_id);
  const stroke_b = stroke_by_id(tablet_document, stroke_b_id);
  const a_vertices = [stroke_a.p0_vertex, stroke_a.p3_vertex];
  const b_vertices = [stroke_b.p0_vertex, stroke_b.p3_vertex];
  const shared = a_vertices.filter((vertex) => b_vertices.includes(vertex));
  if (shared.length !== 1) return null;
  const shared_vertex = shared[0];
  const a_far_vertex = stroke_a.p0_vertex === shared_vertex ? stroke_a.p3_vertex : stroke_a.p0_vertex;
  const b_far_vertex = stroke_b.p0_vertex === shared_vertex ? stroke_b.p3_vertex : stroke_b.p0_vertex;

  // Combined polyline: A from its far end to the junction, then B onward
  // (skipping B's duplicate junction sample).
  const path = sample_stroke_from_vertex(tablet_document, stroke_a_id, a_far_vertex);
  path.push(...sample_stroke_from_vertex(tablet_document, stroke_b_id, shared_vertex).slice(1));
  const p0 = vertex_position(tablet_document, a_far_vertex);
  const p3 = vertex_position(tablet_document, b_far_vertex);
  const handles = fit_stroke_handles(path, p0, p3);

  // Reshape A into the merged stroke, then remove B — delete_stroke handles
  // B's surfaces and junction-vertex GC.
  stroke_a.p0_vertex = a_far_vertex;
  stroke_a.p3_vertex = b_far_vertex;
  stroke_a.d0 = handles.d0;
  stroke_a.d3 = handles.d3;
  // The merged stroke is on the midline only if both parts were (plan-sketchpad-midline.md Q75).
  if (stroke_a.midline === true && stroke_b.midline !== true) delete stroke_a.midline;
  delete_stroke(tablet_document, stroke_b_id);
  return stroke_a_id;
}
