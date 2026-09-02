// Sketchpad-style document model (2026-08-31 pivot, plan step 7). A stroke is
// ONE cubic bezier put down deliberately; its endpoints are indices into a
// shared vertex table, so strokes connected at a vertex can never tear apart.
// Free-handle representation (plan-tablet-free-handles-coplanar.md): both
// interior handles are free 3D offsets from the straight line's 1/3 and 2/3
// points. Invariant: d0, d3 and the chord are coplanar — kept by
// swing_offset_into_plane (a handle drag swings the other handle into the new
// plane) and move_vertex (a chord change rotates both offsets with it).

import { V3, v3, v3_add, v3_cross, v3_dot, v3_length, v3_normalize, v3_rotate_between_directions, v3_scale, v3_sub } from "./math";

export type Stroke = {
  p0_vertex: number; // index into TabletDocument.vertices
  p3_vertex: number;
  d0: V3; // p1 = (2*p0 + p3)/3 + d0 (world units)
  d3: V3; // p2 = (p0 + 2*p3)/3 + d3
};

// Ruled surface between two strokes.
export type Loft = { stroke_a: number; stroke_b: number }; // indices into TabletDocument.strokes

// Coons patch: four boundary strokes (tap order, chained into a loop at
// tessellation time) filled with a bilinearly blended surface.
export type Coons = { strokes: [number, number, number, number] }; // indices into TabletDocument.strokes

// A vertex permanently constrained to ride a host stroke's curve
// (plan-tablet-vertex-insert-and-normal.md Q7/Q10): its position is always
// bezier_point(host, t), re-derived after any host reshape via
// update_pinned_vertex_positions. Dragging it slides t only (clamped [0,1]);
// unpinning — or host deletion — freezes it in place as a free vertex. Stored
// as a parallel table so vertices stay plain V3 for every existing reader.
export type VertexPin = { vertex: number; host_stroke: number; t: number };

export type TabletDocument = {
  vertices: V3[]; // shared junctions; stroke endpoints reference these
  vertex_pins: VertexPin[];
  strokes: Stroke[];
  lofts: Loft[];
  coons: Coons[];
};

export function empty_document(): TabletDocument {
  return { vertices: [], vertex_pins: [], strokes: [], lofts: [], coons: [] };
}

// Delete one stroke. Surfaces built on it are deleted with it; stroke
// indices above it shift down; vertices no longer referenced by any stroke are
// garbage-collected (so they stop acting as invisible snap targets), with the
// surviving strokes' endpoint indices remapped.
export function delete_stroke(tablet_document: TabletDocument, stroke_index: number): void {
  const remap_stroke = (index: number) => (index > stroke_index ? index - 1 : index);
  tablet_document.strokes.splice(stroke_index, 1);
  // Pins hosted on the deleted stroke go with it — the rider vertex freezes at
  // its last position as a free vertex (Q9), kept only while something still
  // references it (the vertex GC below treats surviving pins as references).
  tablet_document.vertex_pins = tablet_document.vertex_pins
    .filter((pin) => pin.host_stroke !== stroke_index)
    .map((pin) => ({ ...pin, host_stroke: remap_stroke(pin.host_stroke) }));
  tablet_document.lofts = tablet_document.lofts
    .filter((loft) => loft.stroke_a !== stroke_index && loft.stroke_b !== stroke_index)
    .map((loft) => ({ stroke_a: remap_stroke(loft.stroke_a), stroke_b: remap_stroke(loft.stroke_b) }));
  tablet_document.coons = tablet_document.coons
    .filter((coons) => !coons.strokes.includes(stroke_index))
    .map((coons): Coons => ({
      strokes: [
        remap_stroke(coons.strokes[0]), remap_stroke(coons.strokes[1]),
        remap_stroke(coons.strokes[2]), remap_stroke(coons.strokes[3]),
      ],
    }));

  const used_vertices = new Set<number>();
  for (const stroke of tablet_document.strokes) {
    used_vertices.add(stroke.p0_vertex);
    used_vertices.add(stroke.p3_vertex);
  }
  for (const pin of tablet_document.vertex_pins) {
    used_vertices.add(pin.vertex);
  }
  const vertex_remap = new Map<number, number>();
  const kept_vertices: V3[] = [];
  for (let vertex_index = 0; vertex_index < tablet_document.vertices.length; vertex_index++) {
    if (!used_vertices.has(vertex_index)) continue;
    vertex_remap.set(vertex_index, kept_vertices.length);
    kept_vertices.push(tablet_document.vertices[vertex_index]);
  }
  tablet_document.vertices = kept_vertices;
  for (const stroke of tablet_document.strokes) {
    stroke.p0_vertex = vertex_remap.get(stroke.p0_vertex)!;
    stroke.p3_vertex = vertex_remap.get(stroke.p3_vertex)!;
  }
  for (const pin of tablet_document.vertex_pins) {
    pin.vertex = vertex_remap.get(pin.vertex)!;
  }
}

// Re-derive every pinned vertex's position from its host curve. Called once
// per frame before tessellation, so any host reshape (handle/vertex drags,
// undo/redo, merges) carries its riders along — and, through move_vertex, the
// strokes ending on those riders.
export function update_pinned_vertex_positions(tablet_document: TabletDocument): void {
  for (const pin of tablet_document.vertex_pins) {
    const host = tablet_document.strokes[pin.host_stroke];
    const points = stroke_control_points(host, tablet_document);
    move_vertex(tablet_document, pin.vertex, bezier_point(points, pin.t));
  }
}

// Move one vertex, rotating the offsets of every stroke ending on it by the
// minimal rotation taking the stroke's old chord direction to its new one
// (plan Q4): the in-plane shape rides the chord, and d0/d3 stay coplanar with
// it. The single choke point for vertex moves — drags, merge snaps, pin slides.
export function move_vertex(tablet_document: TabletDocument, vertex_index: number, new_position: V3): void {
  const old_position = tablet_document.vertices[vertex_index];
  for (const stroke of tablet_document.strokes) {
    if (stroke.p0_vertex !== vertex_index && stroke.p3_vertex !== vertex_index) continue;
    const other_vertex = stroke.p0_vertex === vertex_index ? stroke.p3_vertex : stroke.p0_vertex;
    const other = tablet_document.vertices[other_vertex];
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
  tablet_document.vertices[vertex_index] = new_position;
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

// With d0 = d3 = 0 the control points land at the 1/3 and 2/3 points of a
// straight line.
export function stroke_control_points(stroke: Stroke, tablet_document: TabletDocument): StrokeControlPoints {
  const p0 = tablet_document.vertices[stroke.p0_vertex];
  const p3 = tablet_document.vertices[stroke.p3_vertex];
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
