// Merge two strokes that share a vertex into one stroke spanning their outer
// endpoints, least-squares-fitted to the combined sampled shape (a single
// cubic — sharp corners at the junction get smoothed, that's the point).
// Surfaces built on stroke A carry over to the merged stroke; surfaces built
// on stroke B are deleted with it (delete_stroke semantics), and the junction
// vertex is garbage-collected when nothing else uses it.

import { TabletDocument, bezier_point, delete_stroke, stroke_control_points } from "./document";
import { V3 } from "./math";
import { fit_stroke_handles } from "./line_tool";

const MERGE_SAMPLES_PER_STROKE = 24;

// Sample one stroke's cubic as a polyline running from a given endpoint vertex
// to the other (reversing the parameterization when needed).
function sample_stroke_from_vertex(
  tablet_document: TabletDocument, stroke_index: number, from_vertex: number,
): V3[] {
  const stroke = tablet_document.strokes[stroke_index];
  const points = stroke_control_points(stroke, tablet_document);
  const from_p0 = stroke.p0_vertex === from_vertex;
  const samples: V3[] = [];
  for (let i = 0; i <= MERGE_SAMPLES_PER_STROKE; i++) {
    const t = i / MERGE_SAMPLES_PER_STROKE;
    samples.push(bezier_point(points, from_p0 ? t : 1 - t));
  }
  return samples;
}

// Merge stroke B into stroke A. Returns the merged stroke's index (A's,
// shifted if B sat below it), or null when the strokes don't share exactly
// one vertex (not adjacent, or a closed two-stroke loop — merging that would
// collapse the loop into a degenerate stroke).
export function merge_adjacent_strokes(
  tablet_document: TabletDocument, stroke_a_index: number, stroke_b_index: number,
): number | null {
  const stroke_a = tablet_document.strokes[stroke_a_index];
  const stroke_b = tablet_document.strokes[stroke_b_index];
  const a_vertices = [stroke_a.p0_vertex, stroke_a.p3_vertex];
  const b_vertices = [stroke_b.p0_vertex, stroke_b.p3_vertex];
  const shared = a_vertices.filter((vertex) => b_vertices.includes(vertex));
  if (shared.length !== 1) return null;
  const shared_vertex = shared[0];
  const a_far_vertex = stroke_a.p0_vertex === shared_vertex ? stroke_a.p3_vertex : stroke_a.p0_vertex;
  const b_far_vertex = stroke_b.p0_vertex === shared_vertex ? stroke_b.p3_vertex : stroke_b.p0_vertex;

  // Combined polyline: A from its far end to the junction, then B onward
  // (skipping B's duplicate junction sample).
  const path = sample_stroke_from_vertex(tablet_document, stroke_a_index, a_far_vertex);
  path.push(...sample_stroke_from_vertex(tablet_document, stroke_b_index, shared_vertex).slice(1));
  const p0 = tablet_document.vertices[a_far_vertex];
  const p3 = tablet_document.vertices[b_far_vertex];
  const handles = fit_stroke_handles(path, p0, p3);

  // Reshape A into the merged stroke, then remove B — delete_stroke handles
  // B's surfaces, the stroke-index shift, and junction-vertex GC (it also
  // remaps A's fresh endpoint indices).
  stroke_a.p0_vertex = a_far_vertex;
  stroke_a.p3_vertex = b_far_vertex;
  stroke_a.normal = handles.normal;
  stroke_a.d0 = handles.d0;
  stroke_a.d3 = handles.d3;
  delete_stroke(tablet_document, stroke_b_index);
  return stroke_a_index > stroke_b_index ? stroke_a_index - 1 : stroke_a_index;
}
