// Sketchpad-style document model (2026-08-31 pivot, plan step 7). A stroke is
// ONE cubic bezier put down deliberately; its endpoints are ids of entries in
// a shared vertex table, so strokes connected at a vertex can never tear apart.
// Free-handle representation (plan-tablet-free-handles-coplanar.md): both
// interior handles are free 3D offsets from the straight line's 1/3 and 2/3
// points. Invariant: d0, d3 and the chord are coplanar — kept by
// swing_offset_into_plane (imported control points get d3 swung into d0's
// plane), in-plane and swing handle drags (edit_mode.ts), and
// move_vertex (a chord change rotates both offsets with it).
//
// Stable ids (plan-tablet-stable-ids.md): every stroke and vertex carries an
// id handed out by the document's counters and never reused, and every
// cross-reference (endpoints, pins, knots, patches, the selection) holds ids,
// never array positions. Deleting is therefore a plain filter — nothing has
// to be renumbered. Arrays (not Maps) so the document stays plain JSON for
// snapshots and autosave; lookups are linear scans, fine at dozens of
// entries (put a derived Map behind the two lookup helpers if it ever shows
// in a profile).
//
// Bones (plan-skin-over-skull-study.md Q14, ported from the C++ app's `Bone` +
// `make_bone`): a vertex belongs to a bone and stores its position in that
// bone's space; `world_from_bone` composes the chain up to the root. Moving a
// bone moves every vertex on it — that is how a skin vertex stays anchored to
// the skull it was drawn over. The skull was drawn in the Frankfurt frame,
// which is the `skull` bone's identity transform, so bone space = world space
// for everything drawn so far.

import { Mat4, V3, mat4_identity, mat4_invert, mat4_multiply, mat4_transform_point, v3, v3_add, v3_cross, v3_dot, v3_length, v3_lerp, v3_normalize, v3_rotate_between_directions, v3_scale, v3_sub } from "./math";

export type StrokeId = number;
export type VertexId = number;
export type BoneId = string;

export type Bone = {
  id: BoneId;
  parent_id: BoneId; // "" = root: parent space is world space
  parent_from_bone: number[]; // 4x4 column-major (Mat4 layout), bone space -> parent space; plain numbers so the document stays JSON
};

// The one bone every document starts with; the migration from format 4 puts every vertex on it.
export const SKULL_BONE_ID: BoneId = "skull";

// Which drawing a stroke belongs to. Layers are shown, hidden and locked
// separately (plan-skin-over-skull-study.md Q10); the active layer decides the
// bone of new vertices (Q15).
export type Layer = "skull" | "skin";
export const ALL_LAYERS: readonly Layer[] = ["skull", "skin"];

export type Vertex = {
  id: VertexId;
  bone_id: BoneId;
  position: V3; // in the bone's space — read through vertex_world_position / vertex_position
  name?: string; // a named vertex is a landmark: it survives garbage collection with no stroke using it
  midline?: boolean; // held on the sagittal plane x = 0 (bone space) by enforce_midline (plan-sketchpad-midline.md Q69)
};

export type Stroke = {
  id: StrokeId;
  layer: Layer;
  p0_vertex: VertexId;
  p3_vertex: VertexId;
  // TODO(kv): handle offsets are world-space; with a bone that is not the identity a
  // stroke between two bones has no single bone to store them in. Revisit with the
  // mandible bone.
  d0: V3; // p1 = (2*p0 + p3)/3 + d0 (world units)
  d3: V3; // p2 = (p0 + 2*p3)/3 + d3
  name?: string; // optional label drawn at the curve's midpoint (absent = unnamed)
  midline?: boolean; // the whole curve lies in x = 0: both endpoints and both handles (Q69)
  radii?: StrokeRadii; // width profile; absent = DEFAULT_STROKE_RADII (flat, width 0.25)
};

// Width profile of a stroke, same as the C++ `Curve.radii`: four scalar bezier
// control values, multipliers of the base ribbon radius, sampled along t.
// Flat = constant width; (.25,1,1,.25)-shaped = the classic taper. A stroke
// without a stored profile is flat at width 0.25 (Khoa's pick, 2026-09-27).
export type StrokeRadii = [number, number, number, number];
export const DEFAULT_STROKE_RADII: StrokeRadii = [0.25, 0.25, 0.25, 0.25];

export function stroke_radii(stroke: Stroke): StrokeRadii {
  return stroke.radii ?? DEFAULT_STROKE_RADII;
}

export function stroke_radii_at(radii: StrokeRadii, t: number): number {
  const s = 1 - t;
  return s * s * s * radii[0] + 3 * s * s * t * radii[1] + 3 * s * t * t * radii[2] + t * t * t * radii[3];
}

// De Casteljau of the scalar profile at t: the two halves keep the width the
// whole stroke had, so a split is invisible.
function split_stroke_radii(radii: StrokeRadii, t: number): { left: StrokeRadii; right: StrokeRadii } {
  const lerp = (a: number, b: number) => a + (b - a) * t;
  const r01 = lerp(radii[0], radii[1]), r12 = lerp(radii[1], radii[2]), r23 = lerp(radii[2], radii[3]);
  const r012 = lerp(r01, r12), r123 = lerp(r12, r23);
  const knot = lerp(r012, r123);
  return { left: [radii[0], r01, r012, knot], right: [knot, r123, r23, radii[3]] };
}

// A patch is the set of strokes selected when it was made — unordered,
// undirected, two or more. How it's filled (loft, Coons, N-sided) is derived
// every frame from the current strokes and smooth knots (patch.ts), never
// stored, so reshaping a side or adding/removing a knot reshapes the fill.
export type Patch = { strokes: StrokeId[] };

// A vertex permanently constrained to ride a host stroke's curve
// (plan-tablet-vertex-insert-and-normal.md Q7/Q10): its position is always
// bezier_point(host, t), re-derived after any host reshape via
// update_pinned_vertex_positions. Dragging it slides t only (clamped [0,1]);
// unpinning — or host deletion — freezes it in place as a free vertex. A
// vertex is pinned at most once, so a pin is identified by its vertex id.
export type VertexPin = { vertex: VertexId; host_stroke: StrokeId; t: number };

// Smooth knot (plan-tablet-partial-boundary-patches.md): a vertex where two
// strokes meet end-to-end with their tangents locked, so a chain of strokes
// reads as one curve. Made by split_stroke (exact de Casteljau) and kept by
// enforce_smooth_knot at the edit choke points (Q3): the follower's handle at
// the knot is re-aimed opposite the leader's, its length kept (or scaled with
// the leader's on a handle drag — G1 with a locked ratio, Q2). A welded
// crossing holds two independent knots at one vertex (Q4). Deleting either
// stroke, or joining across the knot, drops the record (Q7).
export type SmoothKnot = { vertex: VertexId; stroke_a: StrokeId; stroke_b: StrokeId };

export type TabletDocument = {
  next_vertex_id: VertexId; // counters only ever grow — ids are never reused
  next_stroke_id: StrokeId;
  bones: Bone[]; // never empty: the skull bone is always there
  vertices: Vertex[]; // shared junctions; stroke endpoints reference these by id
  vertex_pins: VertexPin[];
  smooth_knots: SmoothKnot[];
  strokes: Stroke[]; // array order = draw order
  patches: Patch[];
};

export function skull_bone(): Bone {
  return { id: SKULL_BONE_ID, parent_id: "", parent_from_bone: Array.from(mat4_identity()) };
}

export function empty_document(): TabletDocument {
  return {
    next_vertex_id: 0, next_stroke_id: 0, bones: [skull_bone()], vertices: [], vertex_pins: [], smooth_knots: [], strokes: [], patches: [],
  };
}

export function bone_by_id(tablet_document: TabletDocument, bone_id: BoneId): Bone {
  const bone = tablet_document.bones.find((candidate) => candidate.id === bone_id);
  if (bone === undefined) throw new Error(`no bone with id ${bone_id}`);
  return bone;
}

// Compose parent_from_bone up the chain to the root (same as the C++
// make_bone, which composes the parent off the bone stack).
export function world_from_bone(tablet_document: TabletDocument, bone_id: BoneId): Mat4 {
  const bone = bone_by_id(tablet_document, bone_id);
  const parent_from_bone = new Float32Array(bone.parent_from_bone);
  if (bone.parent_id === "") return parent_from_bone;
  return mat4_multiply(world_from_bone(tablet_document, bone.parent_id), parent_from_bone);
}

// World position of a vertex record (the loops over tablet_document.vertices
// use this; by-id lookups go through vertex_position).
export function vertex_world_position(tablet_document: TabletDocument, vertex: Vertex): V3 {
  return mat4_transform_point(world_from_bone(tablet_document, vertex.bone_id), vertex.position);
}

// Store a world point as the vertex's bone-space position. Writes the position
// only — move_vertex is the choke point that also carries the attached strokes.
export function set_vertex_world_position(tablet_document: TabletDocument, vertex_id: VertexId, world_position: V3): void {
  const vertex = vertex_by_id(tablet_document, vertex_id);
  vertex.position = mat4_transform_point(mat4_invert(world_from_bone(tablet_document, vertex.bone_id)), world_position);
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

// World position of a vertex by id.
export function vertex_position(tablet_document: TabletDocument, vertex_id: VertexId): V3 {
  return vertex_world_position(tablet_document, vertex_by_id(tablet_document, vertex_id));
}

export function pin_by_vertex(tablet_document: TabletDocument, vertex_id: VertexId): VertexPin | null {
  return tablet_document.vertex_pins.find((pin) => pin.vertex === vertex_id) ?? null;
}

// A vertex is on the layers of the strokes ending on it. A vertex no stroke
// uses (a bare landmark) is on every layer: nothing could hide or lock it.
export function vertex_is_on_layers(tablet_document: TabletDocument, vertex_id: VertexId, layers: ReadonlySet<Layer>): boolean {
  let has_stroke = false;
  for (const stroke of tablet_document.strokes) {
    if (stroke.p0_vertex !== vertex_id && stroke.p3_vertex !== vertex_id) continue;
    has_stroke = true;
    if (layers.has(stroke.layer)) return true;
  }
  return !has_stroke;
}

// A patch is on the layer of its first boundary stroke (a patch across layers
// is not expected; the layer bar's lock/hide act on whole patches).
export function patch_layer(patch: Patch, tablet_document: TabletDocument): Layer {
  return stroke_by_id(tablet_document, patch.strokes[0]).layer;
}

// Every vertex riding the given stroke (plan-patch-subcurve-boundary.md: a
// patch loop may enter or leave a stroke at one of these).
export function pins_on_stroke(tablet_document: TabletDocument, stroke_id: StrokeId): VertexPin[] {
  return tablet_document.vertex_pins.filter((pin) => pin.host_stroke === stroke_id);
}

// 0, 1 or (at a welded crossing) 2 knots.
export function smooth_knots_at_vertex(tablet_document: TabletDocument, vertex_id: VertexId): SmoothKnot[] {
  return tablet_document.smooth_knots.filter((knot) => knot.vertex === vertex_id);
}

// `world_position` is converted into the bone's space for storage.
export function add_vertex(tablet_document: TabletDocument, world_position: V3, bone_id: BoneId): VertexId {
  const id = tablet_document.next_vertex_id++;
  tablet_document.vertices.push({ id, bone_id, position: v3(0, 0, 0) });
  set_vertex_world_position(tablet_document, id, world_position);
  return id;
}

export function add_stroke(
  tablet_document: TabletDocument, p0_vertex: VertexId, p3_vertex: VertexId, d0: V3, d3: V3, layer: Layer,
): StrokeId {
  const id = tablet_document.next_stroke_id++;
  tablet_document.strokes.push({ id, layer, p0_vertex, p3_vertex, d0, d3 });
  return id;
}

// A straight cubic between two existing vertices: handles a third of the chord
// from each end, so the curve is the segment until the user bends it. d0/d3
// are OFFSETS from those 1/3 and 2/3 points, so straight = zero offsets (a
// chord-sized offset would put p1 at 2/3 and p2 at 1/3: crossed handles).
export function add_straight_stroke(tablet_document: TabletDocument, p0_vertex: VertexId, p3_vertex: VertexId, layer: Layer): StrokeId {
  return add_stroke(tablet_document, p0_vertex, p3_vertex, v3(0, 0, 0), v3(0, 0, 0), layer);
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
  tablet_document.patches = tablet_document.patches.filter((patch) => !patch.strokes.includes(stroke_id));
  // A knot with one stroke isn't a knot (Q7).
  tablet_document.smooth_knots = tablet_document.smooth_knots
    .filter((knot) => knot.stroke_a !== stroke_id && knot.stroke_b !== stroke_id);
  garbage_collect_vertices(tablet_document);
}

// A cut this close to an end would leave a near-zero-length half.
const SPLIT_MIN_T_FROM_ENDS = 0.02;

// Split a stroke at t into two strokes meeting at a smooth knot. Exact:
// de Casteljau subdivision reproduces the original curve. The original
// stroke keeps the [0, t] half; patches bounded by it gain the other half;
// the new stroke takes [t, 1]. Pins on the original re-parameterize to
// whichever half they land on. The knot is a new vertex, unless the cut is
// at a vertex pinned to this stroke: then that vertex becomes the knot (its
// pin dropped — it's now a real junction), so a chain welded there stays
// welded. Returns the knot vertex, or null for a cut too close to an end.
export function split_stroke(
  tablet_document: TabletDocument, stroke_id: StrokeId, t: number, pinned_vertex: VertexId | null = null,
): VertexId | null {
  if (t < SPLIT_MIN_T_FROM_ENDS || t > 1 - SPLIT_MIN_T_FROM_ENDS) return null;
  const stroke = stroke_by_id(tablet_document, stroke_id);
  const { p0, p1, p2, p3 } = stroke_control_points(stroke, tablet_document);
  const p01 = v3_lerp(p0, p1, t), p12 = v3_lerp(p1, p2, t), p23 = v3_lerp(p2, p3, t);
  const p012 = v3_lerp(p01, p12, t), p123 = v3_lerp(p12, p23, t);
  const knot_position = v3_lerp(p012, p123, t);
  let knot_vertex: VertexId;
  if (pinned_vertex === null) {
    // The knot joins the stroke's starting vertex's bone.
    knot_vertex = add_vertex(tablet_document, knot_position, vertex_by_id(tablet_document, stroke.p0_vertex).bone_id);
  } else {
    knot_vertex = pinned_vertex;
    // Already on the curve (a pin rides bezier_point(host, t)); the caller
    // passed that pin's t, so this is a no-op up to rounding.
    set_vertex_world_position(tablet_document, knot_vertex, knot_position);
    tablet_document.vertex_pins = tablet_document.vertex_pins.filter((pin) => pin.vertex !== pinned_vertex);
  }
  const far_vertex = stroke.p3_vertex;
  const first_half = stroke_handles_from_control_points(p0, p01, p012, knot_position);
  stroke.p3_vertex = knot_vertex;
  stroke.d0 = first_half.d0;
  stroke.d3 = first_half.d3;
  const second_half = stroke_handles_from_control_points(knot_position, p123, p23, p3);
  const second_stroke = add_stroke(tablet_document, knot_vertex, far_vertex, second_half.d0, second_half.d3, stroke.layer);
  for (const pin of tablet_document.vertex_pins) {
    if (pin.host_stroke !== stroke_id) continue;
    if (pin.t <= t) {
      pin.t = pin.t / t;
    } else {
      pin.host_stroke = second_stroke;
      pin.t = (pin.t - t) / (1 - t);
    }
  }
  // Both halves of a midline stroke stay in the plane (Q75); the knot is on it
  // through them, so the vertex needs no flag of its own.
  if (stroke.midline === true) stroke_by_id(tablet_document, second_stroke).midline = true;
  if (stroke.radii !== undefined) {
    const halves = split_stroke_radii(stroke.radii, t);
    stroke.radii = halves.left;
    stroke_by_id(tablet_document, second_stroke).radii = halves.right;
  }
  tablet_document.smooth_knots.push({ vertex: knot_vertex, stroke_a: stroke_id, stroke_b: second_stroke });
  // A patch bounded by the split stroke is now bounded by both halves.
  for (const patch of tablet_document.patches) {
    if (patch.strokes.includes(stroke_id)) patch.strokes.push(second_stroke);
  }
  return knot_vertex;
}

// The offset key of the handle next to `vertex_id` on a stroke ending there.
export function handle_key_at_vertex(stroke: Stroke, vertex_id: VertexId): "d0" | "d3" {
  return stroke.p0_vertex === vertex_id ? "d0" : "d3";
}

// World position of the handle next to `vertex_id`.
function handle_point_at_vertex(stroke: Stroke, tablet_document: TabletDocument, vertex_id: VertexId): V3 {
  const points = stroke_control_points(stroke, tablet_document);
  return handle_key_at_vertex(stroke, vertex_id) === "d0" ? points.p1 : points.p2;
}

// Place the handle next to `vertex_id` at a world point, keeping the stroke's
// other handle coplanar with the chord (swung into the new plane).
function set_handle_point_at_vertex(stroke: Stroke, tablet_document: TabletDocument, vertex_id: VertexId, handle_point: V3): void {
  const p0 = vertex_position(tablet_document, stroke.p0_vertex);
  const p3 = vertex_position(tablet_document, stroke.p3_vertex);
  const key = handle_key_at_vertex(stroke, vertex_id);
  const third_point = key === "d0"
    ? v3_scale(v3_add(v3_scale(p0, 2), p3), 1 / 3)
    : v3_scale(v3_add(p0, v3_scale(p3, 2)), 1 / 3);
  stroke[key] = v3_sub(handle_point, third_point);
  const other_key = key === "d0" ? "d3" : "d0";
  stroke[other_key] = swing_offset_into_plane(chord_direction(p0, p3), stroke[key], stroke[other_key]);
}

// Re-aim the knot's follower handle opposite the leader's tangent. The
// follower's handle length is kept, times `length_factor` (a leader handle
// drag passes its own length ratio so the split ratio survives, Q2).
// Make two strokes that share an endpoint vertex smooth there: a knot record
// with `stroke_a` leading, and stroke_b's tangent re-aimed once to match.
// Returns the knot vertex, or null if they share no endpoint (a stroke's own
// two ends never count) or are already smooth at it.
export function smooth_strokes(tablet_document: TabletDocument, stroke_a: StrokeId, stroke_b: StrokeId): VertexId | null {
  if (stroke_a === stroke_b) return null;
  const shared = shared_vertex_of_strokes(tablet_document, stroke_a, stroke_b);
  if (shared === null) return null;
  if (smooth_knot_between_strokes(tablet_document, stroke_a, stroke_b) !== null) return null;
  const knot: SmoothKnot = { vertex: shared, stroke_a, stroke_b };
  tablet_document.smooth_knots.push(knot);
  enforce_smooth_knot(tablet_document, knot, stroke_a);
  return shared;
}

// Drop the smooth knot joining two strokes; their handles stay where they are
// (the tangents just stop being locked). Returns whether there was one.
export function unsmooth_strokes(tablet_document: TabletDocument, stroke_a: StrokeId, stroke_b: StrokeId): boolean {
  const knot = smooth_knot_between_strokes(tablet_document, stroke_a, stroke_b);
  if (knot === null) return false;
  tablet_document.smooth_knots = tablet_document.smooth_knots.filter((other) => other !== knot);
  return true;
}

// The knot locking these two strokes' tangents, or null.
export function smooth_knot_between_strokes(tablet_document: TabletDocument, stroke_a: StrokeId, stroke_b: StrokeId): SmoothKnot | null {
  return tablet_document.smooth_knots.find(
    (knot) => (knot.stroke_a === stroke_a && knot.stroke_b === stroke_b) || (knot.stroke_a === stroke_b && knot.stroke_b === stroke_a),
  ) ?? null;
}

// The endpoint two strokes have in common, or null (a stroke's own two ends never count).
function shared_vertex_of_strokes(tablet_document: TabletDocument, stroke_a: StrokeId, stroke_b: StrokeId): VertexId | null {
  const a = stroke_by_id(tablet_document, stroke_a);
  const b = stroke_by_id(tablet_document, stroke_b);
  return [a.p0_vertex, a.p3_vertex].find((vertex) => vertex === b.p0_vertex || vertex === b.p3_vertex) ?? null;
}

export function enforce_smooth_knot(
  tablet_document: TabletDocument, knot: SmoothKnot, leader_stroke: StrokeId, length_factor: number = 1,
): void {
  const leader = stroke_by_id(tablet_document, leader_stroke);
  const follower = stroke_by_id(tablet_document, leader_stroke === knot.stroke_a ? knot.stroke_b : knot.stroke_a);
  const knot_position = vertex_position(tablet_document, knot.vertex);
  const leader_outward = v3_sub(handle_point_at_vertex(leader, tablet_document, knot.vertex), knot_position);
  if (v3_length(leader_outward) < COLLINEAR_EPSILON) return; // no tangent to follow
  const follower_length = v3_length(v3_sub(handle_point_at_vertex(follower, tablet_document, knot.vertex), knot_position));
  const follower_outward = v3_scale(v3_normalize(leader_outward), -follower_length * length_factor);
  set_handle_point_at_vertex(follower, tablet_document, knot.vertex, v3_add(knot_position, follower_outward));
}

// Re-aim every knot the stroke takes part in, with the stroke as leader —
// after a reshape of that stroke alone (tilt, swing, handle drag).
export function enforce_smooth_knots_of_stroke(tablet_document: TabletDocument, stroke_id: StrokeId): void {
  for (const knot of tablet_document.smooth_knots) {
    if (knot.stroke_a === stroke_id || knot.stroke_b === stroke_id) enforce_smooth_knot(tablet_document, knot, stroke_id);
  }
}

// Make each stroke the straight segment between its endpoints (both handles back on
// the chord's thirds). A smooth knot between two of the straightened strokes is
// dropped, since two straight lines cannot both keep it; a knot with any other
// stroke stays and that stroke is re-aimed to follow.
export function straighten_strokes(tablet_document: TabletDocument, stroke_ids: StrokeId[]): void {
  for (const stroke_id of stroke_ids) {
    const stroke = stroke_by_id(tablet_document, stroke_id);
    stroke.d0 = v3(0, 0, 0);
    stroke.d3 = v3(0, 0, 0);
  }
  tablet_document.smooth_knots = tablet_document.smooth_knots.filter(
    (knot) => !(stroke_ids.includes(knot.stroke_a) && stroke_ids.includes(knot.stroke_b)),
  );
  for (const stroke_id of stroke_ids) enforce_smooth_knots_of_stroke(tablet_document, stroke_id);
}

// Drop vertices that no stroke endpoint and no pin references. Named vertices (landmarks) stay.
export function garbage_collect_vertices(tablet_document: TabletDocument): void {
  const used_vertices = new Set<VertexId>();
  for (const stroke of tablet_document.strokes) {
    used_vertices.add(stroke.p0_vertex);
    used_vertices.add(stroke.p3_vertex);
  }
  for (const pin of tablet_document.vertex_pins) {
    used_vertices.add(pin.vertex);
  }
  tablet_document.vertices = tablet_document.vertices.filter((vertex) => used_vertices.has(vertex.id) || vertex.name !== undefined);
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

// A vertex is on the midline if flagged itself or if a midline stroke ends on
// it (Q75: the stroke flag never writes the vertex flag, so deleting a stroke
// needs no cleanup).
export function vertex_is_on_midline(tablet_document: TabletDocument, vertex_id: VertexId): boolean {
  if (vertex_by_id(tablet_document, vertex_id).midline === true) return true;
  return tablet_document.strokes.some(
    (stroke) => stroke.midline === true && (stroke.p0_vertex === vertex_id || stroke.p3_vertex === vertex_id),
  );
}

// Midline constraint (plan-sketchpad-midline.md Q70): one pass, run at the edit
// choke point after every pen event and button edit, rather than at each site
// that moves geometry. Every midline vertex is moved to x = 0 (through
// move_vertex, so the strokes ending on it rotate along), then the handles of
// every midline stroke get x = 0 — with the chord already in the plane that
// keeps d0, d3, chord coplanar. x = 0 is the bone's sagittal plane (same as the
// C++ app: midline = x = 0 in bone space).
export function enforce_midline(tablet_document: TabletDocument): void {
  for (const vertex of tablet_document.vertices) {
    if (vertex.position.x === 0 || !vertex_is_on_midline(tablet_document, vertex.id)) continue;
    const bone_to_world = world_from_bone(tablet_document, vertex.bone_id);
    move_vertex(tablet_document, vertex.id, mat4_transform_point(bone_to_world, v3(0, vertex.position.y, vertex.position.z)));
  }
  for (const stroke of tablet_document.strokes) {
    if (stroke.midline !== true) continue;
    stroke.d0 = v3(0, stroke.d0.y, stroke.d0.z);
    stroke.d3 = v3(0, stroke.d3.y, stroke.d3.z);
  }
}

// Vertex snapping is done in world space (not on screen): two vertices weld
// only when they are actually close in 3D, however the camera lines them up.
const VERTEX_SNAP_RADIUS_WORLD = 0.05;

// Nearest vertex within world snap range of a point, or null. `exclude_vertex`
// keeps a dragged vertex from snapping to itself. Only vertices on `layers`
// count (the sketchpad passes the layers neither locked nor hidden).
export function pick_vertex_near_world_point(
  tablet_document: TabletDocument, point: V3, exclude_vertex: VertexId | null, layers: ReadonlySet<Layer>,
): VertexId | null {
  let best_id: VertexId | null = null;
  let best_distance = VERTEX_SNAP_RADIUS_WORLD;
  for (const vertex of tablet_document.vertices) {
    if (vertex.id === exclude_vertex) continue;
    if (!vertex_is_on_layers(tablet_document, vertex.id, layers)) continue;
    const distance = v3_length(v3_sub(vertex_world_position(tablet_document, vertex), point));
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
  tablet_document: TabletDocument, vertex_id: VertexId, layers: ReadonlySet<Layer>,
): { stroke_id: StrokeId; t: number } | null {
  if (pin_by_vertex(tablet_document, vertex_id) !== null) return null;
  const point = vertex_position(tablet_document, vertex_id);
  let best: { stroke_id: StrokeId; t: number } | null = null;
  let best_distance = VERTEX_SNAP_RADIUS_WORLD;
  for (const stroke of tablet_document.strokes) {
    if (stroke.p0_vertex === vertex_id || stroke.p3_vertex === vertex_id) continue;
    if (!layers.has(stroke.layer)) continue;
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
  const old_position = vertex_position(tablet_document, vertex_id);
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
  set_vertex_world_position(tablet_document, vertex_id, new_position);
  // The chord rotations above changed the tangents at both ends of every
  // attached stroke: re-aim the knots there, stroke_a leading (Q3).
  const touched_vertices = new Set<VertexId>([vertex_id]);
  for (const stroke of tablet_document.strokes) {
    if (stroke.p0_vertex === vertex_id) touched_vertices.add(stroke.p3_vertex);
    if (stroke.p3_vertex === vertex_id) touched_vertices.add(stroke.p0_vertex);
  }
  for (const knot of tablet_document.smooth_knots) {
    if (touched_vertices.has(knot.vertex)) enforce_smooth_knot(tablet_document, knot, knot.stroke_a);
  }
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

// Copy every stroke of `from_layer` onto `to_layer`, with the patches, pins and smooth
// knots built on those strokes (plan-skin-from-skull-wrap.md Q4). The copies get fresh
// vertex and stroke ids and share nothing with the originals; vertex and stroke names are
// not copied (they name things on the source layer), the midline flags and radii are.
// Returns old vertex id -> copied vertex id.
export function copy_layer_strokes(tablet_document: TabletDocument, from_layer: Layer, to_layer: Layer): Map<VertexId, VertexId> {
  const copied_vertex = new Map<VertexId, VertexId>();
  const copied_stroke = new Map<StrokeId, StrokeId>();
  function copy_vertex(vertex_id: VertexId): VertexId {
    const existing = copied_vertex.get(vertex_id);
    if (existing !== undefined) return existing;
    const source = vertex_by_id(tablet_document, vertex_id);
    const copy_id = add_vertex(tablet_document, vertex_world_position(tablet_document, source), source.bone_id);
    if (source.midline === true) vertex_by_id(tablet_document, copy_id).midline = true;
    copied_vertex.set(vertex_id, copy_id);
    return copy_id;
  }
  // Snapshots of the source arrays: the copies are appended to the arrays being walked.
  for (const source of tablet_document.strokes.filter((stroke) => stroke.layer === from_layer)) {
    const copy_id = add_stroke(
      tablet_document, copy_vertex(source.p0_vertex), copy_vertex(source.p3_vertex), { ...source.d0 }, { ...source.d3 }, to_layer,
    );
    const copy = stroke_by_id(tablet_document, copy_id);
    if (source.midline === true) copy.midline = true;
    if (source.radii !== undefined) copy.radii = [...source.radii];
    copied_stroke.set(source.id, copy_id);
  }
  for (const pin of [...tablet_document.vertex_pins]) {
    const host_stroke = copied_stroke.get(pin.host_stroke);
    if (host_stroke === undefined) continue;
    tablet_document.vertex_pins.push({ vertex: copy_vertex(pin.vertex), host_stroke, t: pin.t });
  }
  for (const knot of [...tablet_document.smooth_knots]) {
    const stroke_a = copied_stroke.get(knot.stroke_a), stroke_b = copied_stroke.get(knot.stroke_b);
    if (stroke_a === undefined || stroke_b === undefined) continue;
    tablet_document.smooth_knots.push({ vertex: copy_vertex(knot.vertex), stroke_a, stroke_b });
  }
  for (const patch of [...tablet_document.patches]) {
    if (patch_layer(patch, tablet_document) !== from_layer) continue;
    tablet_document.patches.push({ strokes: patch.strokes.map((stroke_id) => copied_stroke.get(stroke_id)!) });
  }
  return copied_vertex;
}
