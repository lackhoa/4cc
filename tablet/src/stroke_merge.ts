// Join two strokes that share a vertex into one stroke spanning their outer
// endpoints: a single cubic, least-squares-fitted to the combined sampled
// shape and forced through the shared vertex (a sharp corner there gets
// smoothed, that's the point). The shared vertex is never deleted: it stays
// as a pin on the joined stroke, so the other strokes ending on it and the
// patches cornered on it keep their junction. Patches, pins and far-end
// smooth knots of stroke B carry over to the joined stroke
// (plan-join-lines-through-patches.html).

import { StrokeId, StrokeRadii, TabletDocument, VertexId, bezier_point, delete_stroke, drop_surface_pins_of_missing_patches, enforce_smooth_knots_of_stroke, nearest_point_on_stroke_world, pin_by_vertex, stroke_by_id, stroke_control_points, stroke_handles_from_control_points, stroke_radii, stroke_radii_at, surface_pin_by_vertex, vertex_by_id, vertex_position } from "./document";
import { V3, v3, v3_add, v3_cross, v3_dot, v3_length, v3_normalize, v3_scale, v3_sub } from "./math";
import { resolve_patch_fill, update_pinned_vertex_positions } from "./patch";

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

// The stroke's width at the same samples as sample_stroke_from_vertex.
function sample_stroke_widths_from_vertex(
  tablet_document: TabletDocument, stroke_id: StrokeId, from_vertex: VertexId,
): number[] {
  const stroke = stroke_by_id(tablet_document, stroke_id);
  const radii = stroke_radii(stroke);
  const from_p0 = stroke.p0_vertex === from_vertex;
  const widths: number[] = [];
  for (let i = 0; i <= MERGE_SAMPLES_PER_STROKE; i++) {
    const t = i / MERGE_SAMPLES_PER_STROKE;
    widths.push(stroke_radii_at(radii, from_p0 ? t : 1 - t));
  }
  return widths;
}

// Chord-length parameter per sample, normalized to [0, 1]; null for a path with no length.
function chord_length_parameters(path: V3[]): number[] | null {
  const parameters: number[] = [0];
  let total_length = 0;
  for (let i = 1; i < path.length; i++) {
    total_length += v3_length(v3_sub(path[i], path[i - 1]));
    parameters.push(total_length);
  }
  if (total_length < 1e-9) return null;
  return parameters.map((length) => length / total_length);
}

export type ThroughPointFit = { d0: V3; d3: V3; through_t: number };

// Least-squares fit of the two interior control points to a sampled path
// (endpoints fixed, `parameters` = one curve parameter per sample), with the
// curve forced through path[through_index] at that sample's parameter,
// converted to the offset handle representation. A stroke's curve is planar
// (d0, d3 and the chord are coplanar by contract), so the fit runs in the
// plane through both endpoints and the through point: the samples are
// projected onto it first. Falls back to a straight stroke's handles when the
// through point sits on an endpoint or the fit is degenerate.
export function fit_stroke_handles_through_point(
  path: V3[], parameters: number[], through_index: number, p0: V3, p3: V3,
): ThroughPointFit {
  const through_t = parameters[through_index];
  const one_third = v3_scale(v3_add(v3_scale(p0, 2), p3), 1 / 3);
  const two_thirds = v3_scale(v3_add(p0, v3_scale(p3, 2)), 1 / 3);
  const straight: ThroughPointFit = { ...stroke_handles_from_control_points(p0, one_third, two_thirds, p3), through_t };
  if (through_t < 1e-6 || through_t > 1 - 1e-6) return straight;

  const through_point = path[through_index];
  // With the through point on the chord there is no such plane; the constraint
  // then makes the two handles' off-chord parts parallel, which is planar already.
  const plane_cross = v3_cross(v3_sub(p3, p0), v3_sub(through_point, p0));
  const plane_normal = v3_length(plane_cross) > 1e-9 ? v3_normalize(plane_cross) : null;
  const in_plane = (point: V3): V3 => (
    plane_normal === null ? point : v3_sub(point, v3_scale(plane_normal, v3_dot(v3_sub(point, p0), plane_normal)))
  );

  // The constraint B(through_t) = through_point reads b1 p1 + b2 p2 = constraint_target,
  // which gives p2 in terms of p1 and leaves min sum |weight_i p1 - residual_i|² over p1 alone.
  const through_s = 1 - through_t;
  const through_b1 = 3 * through_s * through_s * through_t, through_b2 = 3 * through_s * through_t * through_t;
  const constraint_target = v3_sub(
    v3_sub(through_point, v3_scale(p0, through_s * through_s * through_s)), v3_scale(p3, through_t * through_t * through_t),
  );
  let weight_squared_sum = 0;
  let weighted_residual_sum = v3(0, 0, 0);
  for (let i = 0; i < path.length; i++) {
    const t = parameters[i];
    const s = 1 - t;
    const b0 = s * s * s, b1 = 3 * s * s * t, b2 = 3 * s * t * t, b3 = t * t * t;
    const weight = b1 - b2 * through_b1 / through_b2;
    const residual = v3_sub(
      v3_sub(v3_sub(in_plane(path[i]), v3_scale(p0, b0)), v3_scale(p3, b3)), v3_scale(constraint_target, b2 / through_b2),
    );
    weight_squared_sum += weight * weight;
    weighted_residual_sum = v3_add(weighted_residual_sum, v3_scale(residual, weight));
  }
  if (weight_squared_sum < 1e-12) return straight;
  const p1 = v3_scale(weighted_residual_sum, 1 / weight_squared_sum);
  const p2 = v3_scale(v3_sub(constraint_target, v3_scale(p1, through_b1)), 1 / through_b2);
  // Exact: p1 and p2 are in one plane with the chord.
  return { ...stroke_handles_from_control_points(p0, p1, p2, p3), through_t };
}

// Least-squares fit of one width profile to widths sampled along the joined
// path: the two end values are kept, the two middle control values are fitted.
function fit_stroke_radii(widths: number[], parameters: number[]): StrokeRadii {
  const first = widths[0], last = widths[widths.length - 1];
  let a11 = 0, a12 = 0, a22 = 0, c1 = 0, c2 = 0;
  for (let i = 0; i < widths.length; i++) {
    const t = parameters[i];
    const s = 1 - t;
    const b0 = s * s * s, b1 = 3 * s * s * t, b2 = 3 * s * t * t, b3 = t * t * t;
    const target = widths[i] - first * b0 - last * b3;
    a11 += b1 * b1;
    a12 += b1 * b2;
    a22 += b2 * b2;
    c1 += target * b1;
    c2 += target * b2;
  }
  const determinant = a11 * a22 - a12 * a12;
  if (Math.abs(determinant) < 1e-12) return [first, first, last, last];
  return [first, (c1 * a22 - c2 * a12) / determinant, (c2 * a11 - c1 * a12) / determinant, last];
}

// The one endpoint vertex two strokes have in common, or null when they share
// none or both (a closed two-stroke loop — joining that would collapse the loop
// into a degenerate stroke).
function single_shared_vertex(tablet_document: TabletDocument, stroke_a_id: StrokeId, stroke_b_id: StrokeId): VertexId | null {
  const stroke_a = stroke_by_id(tablet_document, stroke_a_id);
  const stroke_b = stroke_by_id(tablet_document, stroke_b_id);
  const b_vertices = [stroke_b.p0_vertex, stroke_b.p3_vertex];
  const shared = [stroke_a.p0_vertex, stroke_a.p3_vertex].filter((vertex) => b_vertices.includes(vertex));
  return shared.length === 1 ? shared[0] : null;
}

// Why the shared vertex cannot become a pin on the joined stroke, or null when it can.
function shared_vertex_pin_refusal_reason(
  tablet_document: TabletDocument, stroke_a_id: StrokeId, stroke_b_id: StrokeId, shared_vertex: VertexId,
): string | null {
  // A vertex is pinned at most once.
  const existing_pin = pin_by_vertex(tablet_document, shared_vertex);
  if (existing_pin !== null) return `the point where these lines meet is pinned to line ${existing_pin.host_stroke}, unpin it first`;
  if (surface_pin_by_vertex(tablet_document, shared_vertex) !== null) return "the point where these lines meet is pinned on the surface, unpin it first";
  // A midline stroke ending on the vertex holds it on the midline; the pin would pull it off.
  const joined_is_midline = stroke_by_id(tablet_document, stroke_a_id).midline === true && stroke_by_id(tablet_document, stroke_b_id).midline === true;
  const held_on_midline = tablet_document.strokes.some(
    (stroke) => stroke.midline === true && stroke.id !== stroke_a_id && stroke.id !== stroke_b_id
      && (stroke.p0_vertex === shared_vertex || stroke.p3_vertex === shared_vertex),
  );
  if (held_on_midline && !joined_is_midline) return "a midline line ends where these lines meet";
  return null;
}

// Every patch listing `replaced_stroke` lists `kept_stroke` instead (once). A
// patch left with a single stroke is removed.
export function replace_stroke_in_patches(tablet_document: TabletDocument, replaced_stroke: StrokeId, kept_stroke: StrokeId): void {
  for (const patch of tablet_document.patches) {
    if (!patch.strokes.includes(replaced_stroke)) continue;
    const without_replaced = patch.strokes.filter((id) => id !== replaced_stroke);
    patch.strokes = without_replaced.includes(kept_stroke) ? without_replaced : [...without_replaced, kept_stroke];
  }
  tablet_document.patches = tablet_document.patches.filter((patch) => patch.strokes.length >= 2);
  drop_surface_pins_of_missing_patches(tablet_document);
}

// Merge stroke B into stroke A. Returns the merged stroke's id (A's), or
// null when the strokes don't share exactly one vertex (not adjacent, or a
// closed two-stroke loop — merging that would collapse the loop into a
// degenerate stroke), or when that vertex cannot become a pin (see
// join_refusal_reason). A keeps its id, name and layer.
export function merge_adjacent_strokes(
  tablet_document: TabletDocument, stroke_a_id: StrokeId, stroke_b_id: StrokeId,
): StrokeId | null {
  const shared_vertex = single_shared_vertex(tablet_document, stroke_a_id, stroke_b_id);
  if (shared_vertex === null) return null;
  if (shared_vertex_pin_refusal_reason(tablet_document, stroke_a_id, stroke_b_id, shared_vertex) !== null) return null;
  const stroke_a = stroke_by_id(tablet_document, stroke_a_id);
  const stroke_b = stroke_by_id(tablet_document, stroke_b_id);
  const a_far_vertex = stroke_a.p0_vertex === shared_vertex ? stroke_a.p3_vertex : stroke_a.p0_vertex;
  const b_far_vertex = stroke_b.p0_vertex === shared_vertex ? stroke_b.p3_vertex : stroke_b.p0_vertex;

  // Combined polyline: A from its far end to the junction, then B onward
  // (skipping B's duplicate junction sample).
  const path = sample_stroke_from_vertex(tablet_document, stroke_a_id, a_far_vertex);
  path.push(...sample_stroke_from_vertex(tablet_document, stroke_b_id, shared_vertex).slice(1));
  const parameters = chord_length_parameters(path);
  if (parameters === null) return null;
  const p0 = vertex_position(tablet_document, a_far_vertex);
  const p3 = vertex_position(tablet_document, b_far_vertex);
  const fit = fit_stroke_handles_through_point(path, parameters, MERGE_SAMPLES_PER_STROKE, p0, p3);

  // Where each vertex riding either stroke is now, to re-pin it at the nearest point of the joined curve.
  const riding_pins = tablet_document.vertex_pins
    .filter((pin) => pin.host_stroke === stroke_a_id || pin.host_stroke === stroke_b_id)
    .map((pin) => ({ pin, world_position: vertex_position(tablet_document, pin.vertex) }));

  // One width profile across both strokes; two default-width strokes stay without a stored profile.
  if (stroke_a.radii !== undefined || stroke_b.radii !== undefined) {
    const widths = sample_stroke_widths_from_vertex(tablet_document, stroke_a_id, a_far_vertex);
    widths.push(...sample_stroke_widths_from_vertex(tablet_document, stroke_b_id, shared_vertex).slice(1));
    stroke_a.radii = fit_stroke_radii(widths, parameters);
  }

  // Reshape A into the merged stroke.
  stroke_a.p0_vertex = a_far_vertex;
  stroke_a.p3_vertex = b_far_vertex;
  stroke_a.d0 = fit.d0;
  stroke_a.d3 = fit.d3;
  // The merged stroke is on the midline only if both parts were (plan-sketchpad-midline.md Q75).
  if (stroke_a.midline === true && stroke_b.midline !== true) delete stroke_a.midline;
  // Likewise on the surface (plan-hairline-drawn-on-surface Q4).
  if (stroke_a.on_surface === true && stroke_b.on_surface !== true) delete stroke_a.on_surface;

  for (const { pin, world_position } of riding_pins) {
    pin.host_stroke = stroke_a_id;
    pin.t = nearest_point_on_stroke_world(stroke_a, tablet_document, world_position).t;
  }
  // The shared vertex rides the joined stroke where the fit passes through it
  // (its own midline flag goes: the pin wins the position, as in pin_vertex_to_stroke).
  delete vertex_by_id(tablet_document, shared_vertex).midline;
  tablet_document.vertex_pins.push({ vertex: shared_vertex, host_stroke: stroke_a_id, t: fit.through_t });

  // Smooth knots: the joined stroke passes straight through the shared vertex,
  // so a knot of A or B there (with each other or with a third stroke) has no
  // end to lock and goes. B's knot at its far end becomes the joined stroke's.
  tablet_document.smooth_knots = tablet_document.smooth_knots.filter((knot) => {
    const involves_joined = [stroke_a_id, stroke_b_id].includes(knot.stroke_a) || [stroke_a_id, stroke_b_id].includes(knot.stroke_b);
    return !(knot.vertex === shared_vertex && involves_joined);
  });
  for (const knot of tablet_document.smooth_knots) {
    if (knot.stroke_a === stroke_b_id) knot.stroke_a = stroke_a_id;
    if (knot.stroke_b === stroke_b_id) knot.stroke_b = stroke_a_id;
  }

  // Nothing references B any more, so delete_stroke removes the stroke alone.
  replace_stroke_in_patches(tablet_document, stroke_b_id, stroke_a_id);
  delete_stroke(tablet_document, stroke_b_id);
  // The fit changed the tangents at both far ends: the strokes knotted there follow.
  enforce_smooth_knots_of_stroke(tablet_document, stroke_a_id);
  update_pinned_vertex_positions(tablet_document);
  return stroke_a_id;
}

// Why the join button refuses these two strokes, or null when the join may
// run. Besides the selection not being joinable, a join is refused when a
// patch that has a fill would lose it: the join runs on a copy of the document
// first and every filled patch is checked there.
export function join_refusal_reason(tablet_document: TabletDocument, stroke_a_id: StrokeId, stroke_b_id: StrokeId): string | null {
  const shared_vertex = single_shared_vertex(tablet_document, stroke_a_id, stroke_b_id);
  if (shared_vertex === null) return "these two lines do not meet at exactly one end";
  const pin_refusal = shared_vertex_pin_refusal_reason(tablet_document, stroke_a_id, stroke_b_id, shared_vertex);
  if (pin_refusal !== null) return pin_refusal;

  const trial_document = structuredClone(tablet_document);
  const trial_patches = [...trial_document.patches];
  const has_fill = trial_patches.map((patch) => resolve_patch_fill(patch, trial_document) !== null);
  const lines_of_patch = trial_patches.map((patch) => patch.strokes.join(", "));
  if (merge_adjacent_strokes(trial_document, stroke_a_id, stroke_b_id) === null) return "these two lines have no length to join";
  for (let i = 0; i < trial_patches.length; i++) {
    if (!has_fill[i]) continue;
    const keeps_fill = trial_document.patches.includes(trial_patches[i]) && resolve_patch_fill(trial_patches[i], trial_document) !== null;
    if (!keeps_fill) return `the patch between lines ${lines_of_patch[i]} would lose its fill`;
  }
  return null;
}
