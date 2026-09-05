// Sketchpad-style document model (2026-08-31 pivot, plan step 7). A stroke is
// ONE cubic bezier put down deliberately; its endpoints are ids of entries in
// a shared vertex table, so strokes connected at a vertex can never tear apart.
// Free-handle representation (plan-tablet-free-handles-coplanar.md): both
// interior handles are free 3D offsets from the straight line's 1/3 and 2/3
// points. Invariant: d0, d3 and the chord are coplanar — kept by
// swing_offset_into_plane (imported control points get d3 swung into d0's
// plane), in-plane handle drags and the tilt dial (edit_mode.ts), and
// move_vertex (a chord change rotates both offsets with it).
//
// Stable ids (plan-tablet-stable-ids.md): every stroke and vertex carries an
// id handed out by the document's counters and never reused, and every
// cross-reference (endpoints, pins, lofts, patches, the selection) holds ids,
// never array positions. Deleting is therefore a plain filter — nothing has
// to be renumbered. Arrays (not Maps) so the document stays plain JSON for
// snapshots and autosave; lookups are linear scans, fine at dozens of
// entries (put a derived Map behind the two lookup helpers if it ever shows
// in a profile).

import { V3, v3, v3_add, v3_cross, v3_dot, v3_length, v3_normalize, v3_rotate_between_directions, v3_scale, v3_sub } from "./math";

export type StrokeId = number;
export type VertexId = number;

export type Vertex = { id: VertexId; position: V3 };

export type Stroke = {
  id: StrokeId;
  p0_vertex: VertexId;
  p3_vertex: VertexId;
  d0: V3; // p1 = (2*p0 + p3)/3 + d0 (world units)
  d3: V3; // p2 = (p0 + 2*p3)/3 + d3
  name?: string; // optional label drawn at the curve's midpoint (absent = unnamed)
};

// Ruled surface between two strokes.
export type Loft = { stroke_a: StrokeId; stroke_b: StrokeId };

// Coons patch: four boundary strokes (tap order, chained into a loop at
// tessellation time) filled with a bilinearly blended surface.
export type Coons = { strokes: [StrokeId, StrokeId, StrokeId, StrokeId] };

// A vertex permanently constrained to ride a host stroke's curve
// (plan-tablet-vertex-insert-and-normal.md Q7/Q10): its position is always
// bezier_point(host, t), re-derived after any host reshape via
// update_pinned_vertex_positions. Dragging it slides t only (clamped [0,1]);
// unpinning — or host deletion — freezes it in place as a free vertex. A
// vertex is pinned at most once, so a pin is identified by its vertex id.
export type VertexPin = { vertex: VertexId; host_stroke: StrokeId; t: number };

export type TabletDocument = {
  next_vertex_id: VertexId; // counters only ever grow — ids are never reused
  next_stroke_id: StrokeId;
  vertices: Vertex[]; // shared junctions; stroke endpoints reference these by id
  vertex_pins: VertexPin[];
  strokes: Stroke[]; // array order = draw order
  lofts: Loft[];
  coons: Coons[];
};

export function empty_document(): TabletDocument {
  return { next_vertex_id: 0, next_stroke_id: 0, vertices: [], vertex_pins: [], strokes: [], lofts: [], coons: [] };
}

// A dangling id is a bug in whoever stored it (delete_stroke drops every
// reference to what it removes), so lookups throw rather than return null.
export function stroke_by_id(tablet_document: TabletDocument, stroke_id: StrokeId): Stroke {
  const stroke = tablet_document.strokes.find((candidate) => candidate.id === stroke_id);
  if (stroke === undefined) throw new Error(`no stroke with id ${stroke_id}`);
  return stroke;
}

export function vertex_by_id(tablet_document: TabletDocument, vertex_id: VertexId): Vertex {
  const vertex = tablet_document.vertices.find((candidate) => candidate.id === vertex_id);
  if (vertex === undefined) throw new Error(`no vertex with id ${vertex_id}`);
  return vertex;
}

export function vertex_position(tablet_document: TabletDocument, vertex_id: VertexId): V3 {
  return vertex_by_id(tablet_document, vertex_id).position;
}

export function pin_by_vertex(tablet_document: TabletDocument, vertex_id: VertexId): VertexPin | null {
  return tablet_document.vertex_pins.find((pin) => pin.vertex === vertex_id) ?? null;
}

export function add_vertex(tablet_document: TabletDocument, position: V3): VertexId {
  const id = tablet_document.next_vertex_id++;
  tablet_document.vertices.push({ id, position });
  return id;
}

export function add_stroke(
  tablet_document: TabletDocument, p0_vertex: VertexId, p3_vertex: VertexId, d0: V3, d3: V3,
): StrokeId {
  const id = tablet_document.next_stroke_id++;
  tablet_document.strokes.push({ id, p0_vertex, p3_vertex, d0, d3 });
  return id;
}

// Delete one stroke. Surfaces built on it are deleted with it; vertices no
// longer referenced by any stroke or pin are garbage-collected (so they stop
// acting as invisible snap targets). Ids of everything else are untouched.
export function delete_stroke(tablet_document: TabletDocument, stroke_id: StrokeId): void {
  tablet_document.strokes = tablet_document.strokes.filter((stroke) => stroke.id !== stroke_id);
  // Pins hosted on the deleted stroke go with it — the rider vertex freezes at
  // its last position as a free vertex (Q9), kept only while something still
  // references it (the vertex GC below treats surviving pins as references).
  tablet_document.vertex_pins = tablet_document.vertex_pins.filter((pin) => pin.host_stroke !== stroke_id);
  tablet_document.lofts = tablet_document.lofts
    .filter((loft) => loft.stroke_a !== stroke_id && loft.stroke_b !== stroke_id);
  tablet_document.coons = tablet_document.coons.filter((coons) => !coons.strokes.includes(stroke_id));
  garbage_collect_vertices(tablet_document);
}

// Drop vertices that no stroke endpoint and no pin references.
export function garbage_collect_vertices(tablet_document: TabletDocument): void {
  const used_vertices = new Set<VertexId>();
  for (const stroke of tablet_document.strokes) {
    used_vertices.add(stroke.p0_vertex);
    used_vertices.add(stroke.p3_vertex);
  }
  for (const pin of tablet_document.vertex_pins) {
    used_vertices.add(pin.vertex);
  }
  tablet_document.vertices = tablet_document.vertices.filter((vertex) => used_vertices.has(vertex.id));
}

// Re-derive every pinned vertex's position from its host curve. Called once
// per frame before tessellation, so any host reshape (handle/vertex drags,
// undo/redo, merges) carries its riders along — and, through move_vertex, the
// strokes ending on those riders.
export function update_pinned_vertex_positions(tablet_document: TabletDocument): void {
  for (const pin of tablet_document.vertex_pins) {
    const host = stroke_by_id(tablet_document, pin.host_stroke);
    const points = stroke_control_points(host, tablet_document);
    move_vertex(tablet_document, pin.vertex, bezier_point(points, pin.t));
  }
}

// Vertex snapping is done in world space (not on screen): two vertices weld
// only when they are actually close in 3D, however the camera lines them up.
const VERTEX_SNAP_RADIUS_WORLD = 0.05;

// Nearest vertex within world snap range of a point, or null. `exclude_vertex`
// keeps a dragged vertex from snapping to itself.
export function pick_vertex_near_world_point(
  tablet_document: TabletDocument, point: V3, exclude_vertex: VertexId | null,
): VertexId | null {
  let best_id: VertexId | null = null;
  let best_distance = VERTEX_SNAP_RADIUS_WORLD;
  for (const vertex of tablet_document.vertices) {
    if (vertex.id === exclude_vertex) continue;
    const distance = v3_length(v3_sub(vertex.position, point));
    if (distance < best_distance) {
      best_distance = distance;
      best_id = vertex.id;
    }
  }
  return best_id;
}

// Nearest point of a stroke's curve to a world point: coarse t sweep, then a
// local ternary refinement around the best sample.
const CURVE_DISTANCE_SAMPLES = 128;
export function nearest_point_on_stroke_world(
  stroke: Stroke, tablet_document: TabletDocument, point: V3,
): { t: number; distance: number } {
  const points = stroke_control_points(stroke, tablet_document);
  const distance_at = (t: number) => v3_length(v3_sub(bezier_point(points, t), point));
  let best_t = 0;
  let best_distance = Infinity;
  for (let i = 0; i <= CURVE_DISTANCE_SAMPLES; i++) {
    const t = i / CURVE_DISTANCE_SAMPLES;
    const distance = distance_at(t);
    if (distance < best_distance) {
      best_distance = distance;
      best_t = t;
    }
  }
  let low = Math.max(0, best_t - 1 / CURVE_DISTANCE_SAMPLES);
  let high = Math.min(1, best_t + 1 / CURVE_DISTANCE_SAMPLES);
  for (let i = 0; i < 20; i++) {
    const t1 = low + (high - low) / 3;
    const t2 = high - (high - low) / 3;
    if (distance_at(t1) < distance_at(t2)) high = t2; else low = t1;
  }
  const t = (low + high) / 2;
  return { t, distance: distance_at(t) };
}

// The stroke a free vertex would get pinned to on release: the nearest curve
// within VERTEX_SNAP_RADIUS_WORLD, or null. Skips strokes ending on the vertex
// (always at distance 0) and vertices that are already pinned (unpin first).
// Vertex-to-vertex welding takes priority — the caller checks that first.
export function find_snap_target_stroke(
  tablet_document: TabletDocument, vertex_id: VertexId,
): { stroke_id: StrokeId; t: number } | null {
  if (pin_by_vertex(tablet_document, vertex_id) !== null) return null;
  const point = vertex_position(tablet_document, vertex_id);
  let best: { stroke_id: StrokeId; t: number } | null = null;
  let best_distance = VERTEX_SNAP_RADIUS_WORLD;
  for (const stroke of tablet_document.strokes) {
    if (stroke.p0_vertex === vertex_id || stroke.p3_vertex === vertex_id) continue;
    const nearest = nearest_point_on_stroke_world(stroke, tablet_document, point);
    if (nearest.distance < best_distance) {
      best_distance = nearest.distance;
      best = { stroke_id: stroke.id, t: nearest.t };
    }
  }
  return best;
}

// Move one vertex, rotating the offsets of every stroke ending on it by the
// minimal rotation taking the stroke's old chord direction to its new one
// (plan Q4): the in-plane shape rides the chord, and d0/d3 stay coplanar with
// it. The single choke point for vertex moves — drags, merge snaps, pin slides.
export function move_vertex(tablet_document: TabletDocument, vertex_id: VertexId, new_position: V3): void {
  const vertex = vertex_by_id(tablet_document, vertex_id);
  const old_position = vertex.position;
  for (const stroke of tablet_document.strokes) {
    if (stroke.p0_vertex !== vertex_id && stroke.p3_vertex !== vertex_id) continue;
    const other_vertex = stroke.p0_vertex === vertex_id ? stroke.p3_vertex : stroke.p0_vertex;
    const other = vertex_position(tablet_document, other_vertex);
    // Chord orientation is irrelevant: the minimal rotation a -> b equals -a -> -b.
    const old_chord = v3_sub(old_position, other);
    const new_chord = v3_sub(new_position, other);
    if (v3_length(old_chord) < COLLINEAR_EPSILON || v3_length(new_chord) < COLLINEAR_EPSILON) continue;
    const from = v3_normalize(old_chord);
    const to = v3_normalize(new_chord);
    const flip_axis = fallback_perpendicular(from);
    stroke.d0 = v3_rotate_between_directions(stroke.d0, from, to, flip_axis);
    stroke.d3 = v3_rotate_between_directions(stroke.d3, from, to, flip_axis);
  }
  vertex.position = new_position;
}

// The four world-space bezier control points of a stroke.
export type StrokeControlPoints = { p0: V3; p1: V3; p2: V3; p3: V3 };

const COLLINEAR_EPSILON = 1e-9;

// Deterministic unit vector perpendicular to u (for degenerate strokes that
// have no plane of their own).
export function fallback_perpendicular(u: V3): V3 {
  const with_y = v3_cross(u, v3(0, 1, 0));
  if (v3_length(with_y) > COLLINEAR_EPSILON) return v3_normalize(with_y);
  return v3_normalize(v3_cross(u, v3(1, 0, 0)));
}

// Unit chord direction p0 -> p3 (a fixed axis when the endpoints coincide).
function chord_direction(p0: V3, p3: V3): V3 {
  const chord = v3_sub(p3, p0);
  return v3_length(chord) > COLLINEAR_EPSILON ? v3_normalize(chord) : v3(1, 0, 0);
}

// Component of an offset perpendicular to the unit chord direction.
function perpendicular_to_chord(u: V3, offset: V3): V3 {
  return v3_sub(offset, v3_scale(u, v3_dot(offset, u)));
}

// Unit normal of the stroke's plane (plan-tablet-planar-handle-drags.html
// Q6): cross(chord, d0), else cross(chord, d3), else — a straight stroke has
// no plane of its own — the plane through the chord that faces the camera
// (camera_forward with its along-chord part removed), else any perpendicular.
export function stroke_plane_normal(stroke: Stroke, tablet_document: TabletDocument, camera_forward: V3): V3 {
  const u = chord_direction(
    vertex_position(tablet_document, stroke.p0_vertex), vertex_position(tablet_document, stroke.p3_vertex),
  );
  for (const candidate of [v3_cross(u, stroke.d0), v3_cross(u, stroke.d3), perpendicular_to_chord(u, camera_forward)]) {
    if (v3_length(candidate) > COLLINEAR_EPSILON) return v3_normalize(candidate);
  }
  return fallback_perpendicular(u);
}

// With d0 = d3 = 0 the control points land at the 1/3 and 2/3 points of a
// straight line.
export function stroke_control_points(stroke: Stroke, tablet_document: TabletDocument): StrokeControlPoints {
  const p0 = vertex_position(tablet_document, stroke.p0_vertex);
  const p3 = vertex_position(tablet_document, stroke.p3_vertex);
  const p1 = v3_add(v3_scale(v3_add(v3_scale(p0, 2), p3), 1 / 3), stroke.d0);
  const p2 = v3_add(v3_scale(v3_add(p0, v3_scale(p3, 2)), 1 / 3), stroke.d3);
  return { p0, p1, p2, p3 };
}

// Swing `follower` into the plane spanned by the unit chord direction u and
// `leader` (plan Q2): its along-chord component and its perpendicular length
// are kept, and the perpendicular part lands on the side of the chord it was
// already on (project onto the new plane, restore length). A leader on the
// chord (or ~0) defines no plane — the follower comes back unchanged.
export function swing_offset_into_plane(u: V3, leader: V3, follower: V3): V3 {
  const leader_perpendicular = perpendicular_to_chord(u, leader);
  if (v3_length(leader_perpendicular) < COLLINEAR_EPSILON) return follower;
  const v = v3_normalize(leader_perpendicular);
  const along = v3_dot(follower, u);
  const follower_perpendicular = perpendicular_to_chord(u, follower);
  const side = v3_dot(follower_perpendicular, v) < 0 ? -1 : 1;
  return v3_add(v3_scale(u, along), v3_scale(v, side * v3_length(follower_perpendicular)));
}

// Re-express four explicit world control points in offset form, with d3 swung
// into the plane of d0 (the representation is coplanar by contract). Exact for
// planar inputs.
export function stroke_handles_from_control_points(p0: V3, p1: V3, p2: V3, p3: V3): { d0: V3; d3: V3 } {
  const u = chord_direction(p0, p3);
  const d0 = v3_sub(p1, v3_scale(v3_add(v3_scale(p0, 2), p3), 1 / 3));
  const d3 = v3_sub(p2, v3_scale(v3_add(p0, v3_scale(p3, 2)), 1 / 3));
  return { d0, d3: swing_offset_into_plane(u, d0, d3) };
}

export function bezier_point(points: StrokeControlPoints, t: number): V3 {
  const s = 1 - t;
  const b0 = s * s * s, b1 = 3 * s * s * t, b2 = 3 * s * t * t, b3 = t * t * t;
  return v3_add(
    v3_add(v3_scale(points.p0, b0), v3_scale(points.p1, b1)),
    v3_add(v3_scale(points.p2, b2), v3_scale(points.p3, b3)),
  );
}

export function bezier_tangent(points: StrokeControlPoints, t: number): V3 {
  const s = 1 - t;
  return v3_add(
    v3_add(
      v3_scale(v3_sub(points.p1, points.p0), 3 * s * s),
      v3_scale(v3_sub(points.p2, points.p1), 6 * s * t),
    ),
    v3_scale(v3_sub(points.p3, points.p2), 3 * t * t),
  );
}
