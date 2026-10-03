// Edit mode: pen-tap a stroke to select it. Dragging one of its four control
// points reshapes it — vertices (p0/p3) move in the camera plane and carry
// every attached stroke with them; the p1/p2 handles slide inside the
// stroke's plane (pen ray ∩ plane, plan-tablet-planar-handle-drags.html Q4),
// so the other handle never moves. One experimental mode (HandleMode):
// "swing" keeps picking, but a dragged handle follows the pen in the camera
// plane and the other handle swings into the new plane
// (plan-tablet-free-handles-coplanar.md Q2/Q3). Releasing a vertex drag near another vertex
// merges the two into one shared junction; releasing it near another stroke's
// curve instead pins it there (plan-tablet-vertex-to-line-snap.md). Pinned vertices (vertex_pins) only
// ever slide along their host curve, whichever way they're grabbed.
// A drag starting ON the stroke body translates
// the whole stroke (both vertices) in the camera plane; a drag starting on
// empty space is NOT consumed — the caller orbits the camera instead (Q35).

import { OrbitCamera, camera_basis, camera_pen_ray, camera_screen_projector, camera_world_to_screen, camera_world_units_per_pixel } from "./camera";
import { Layer, Stroke, StrokeId, TabletDocument, VertexId, bezier_point, enforce_smooth_knot, find_snap_target_stroke, move_vertex, pick_vertex_near_world_point, pin_by_vertex, smooth_knots_at_vertex, stroke_by_id, stroke_control_points, stroke_plane_normal, swing_offset_into_plane, vertex_by_id, vertex_is_on_layers, vertex_is_on_midline, vertex_position, vertex_world_position } from "./document";
import { V2, V3, v3_add, v3_dot, v3_length, v3_normalize, v3_scale, v3_sub } from "./math";

// NOTE: tap = max displacement from the pen-down point, NOT accumulated path
// length — a real Apple Pencil tap jitters through many sub-pixel moves whose
// path sum easily exceeds any threshold. A tap only selects: the sketchpad
// calls edit_pen_move only once the pen has travelled past this.
export const TAP_MAX_MOVEMENT_PIXELS = 12;

// Pen/mouse-sized, not finger-sized (fingers never pick). CSS pixels.
export const STROKE_PICK_RADIUS_PIXELS = 10;
export const CONTROL_POINT_PICK_RADIUS_PIXELS = 10;
const PICK_SAMPLES_PER_STROKE = 16;
const PIN_SLIDE_SAMPLES = 128; // t resolution when sliding a pinned vertex
// Below this |cos| between the pen ray and the stroke plane's normal (plane
// within ~9° of edge-on) the ray∩plane hit runs off to infinity: the handle
// stays put instead (Q5).
const EDGE_ON_PLANE_COSINE = 0.15;

// How p1/p2 handle drags behave — see the header comment.
export type HandleMode = "plane" | "swing";

export type StrokePointKey = "p0" | "p1" | "p2" | "p3";

// Pins are addressed by their vertex id (a vertex is pinned at most once).
export type EditState = {
  stroke_id: StrokeId;
  dragging: StrokePointKey | null; // pen is down on a control point
  dragging_pin: VertexId | null; // pen is down on a pinned vertex
  selected_handle: "p1" | "p2" | null; // the handle the nudge keys move, set by a tap on it; survives pen-up
  moving_whole_stroke: boolean; // pen is down on the stroke body
  last_screen: V2 | null; // previous pen position while a drag is active
};

export function begin_edit_state(stroke_id: StrokeId): EditState {
  return {
    stroke_id, dragging: null, dragging_pin: null, selected_handle: null,
    moving_whole_stroke: false, last_screen: null,
  };
}

function distance_point_to_segment(point: V2, a: V2, b: V2): number {
  const abx = b.x - a.x;
  const aby = b.y - a.y;
  const length_squared = abx * abx + aby * aby;
  const t = length_squared < 1e-12 ? 0 : Math.max(0, Math.min(1, ((point.x - a.x) * abx + (point.y - a.y) * aby) / length_squared));
  return Math.hypot(point.x - (a.x + abx * t), point.y - (a.y + aby * t));
}

// Nearest stroke within pick range of a screen tap, or null. Distance is to
// the projected polyline's segments, not its sample points, so a long or
// zoomed-in stroke has no dead zones between samples. Only strokes on `layers`
// count (the sketchpad passes the layers neither locked nor hidden).
export function pick_stroke(
  tablet_document: TabletDocument, camera: OrbitCamera, screen: V2, canvas: HTMLCanvasElement, layers: ReadonlySet<Layer>,
): StrokeId | null {
  let best_id: StrokeId | null = null;
  let best_distance = STROKE_PICK_RADIUS_PIXELS;
  const project = camera_screen_projector(camera, canvas.clientWidth, canvas.clientHeight);
  for (const stroke of tablet_document.strokes) {
    if (!layers.has(stroke.layer)) continue;
    const points = stroke_control_points(stroke, tablet_document);
    let previous: V2 | null = null;
    for (let i = 0; i <= PICK_SAMPLES_PER_STROKE; i++) {
      const world = bezier_point(points, i / PICK_SAMPLES_PER_STROKE);
      const projected = project(world);
      if (projected === null) {
        previous = null;
        continue;
      }
      const distance = previous === null
        ? Math.hypot(projected.x - screen.x, projected.y - screen.y)
        : distance_point_to_segment(screen, previous, projected);
      if (distance < best_distance) {
        best_distance = distance;
        best_id = stroke.id;
      }
      previous = projected;
    }
  }
  return best_id;
}

// Nearest vertex/handle of the edited stroke within pick range, or null.
export function pick_stroke_point(
  stroke: Stroke, tablet_document: TabletDocument, camera: OrbitCamera, screen: V2, canvas: HTMLCanvasElement,
): StrokePointKey | null {
  const points = stroke_control_points(stroke, tablet_document);
  const candidates: { key: StrokePointKey; world: V3 }[] = [
    { key: "p0", world: points.p0 },
    { key: "p1", world: points.p1 },
    { key: "p2", world: points.p2 },
    { key: "p3", world: points.p3 },
  ];
  let best: StrokePointKey | null = null;
  let best_distance = CONTROL_POINT_PICK_RADIUS_PIXELS;
  for (const { key, world } of candidates) {
    const projected = camera_world_to_screen(camera, world, canvas.clientWidth, canvas.clientHeight);
    if (projected === null) continue;
    const distance = Math.hypot(projected.x - screen.x, projected.y - screen.y);
    if (distance < best_distance) {
      best_distance = distance;
      best = key;
    }
  }
  return best;
}

// The t on a stroke's curve whose point projects nearest to a screen position,
// with that nearest screen distance in pixels. Used to place a new pin (caller
// checks the distance against the stroke pick radius) and to slide an existing
// one (distance ignored — the nearest point wins wherever the pen is).
export function nearest_t_on_stroke_screen(
  tablet_document: TabletDocument, stroke_id: StrokeId, camera: OrbitCamera, screen: V2, canvas: HTMLCanvasElement,
): { t: number; distance: number } {
  const points = stroke_control_points(stroke_by_id(tablet_document, stroke_id), tablet_document);
  let best_t = 0;
  let best_distance = Infinity;
  for (let i = 0; i <= PIN_SLIDE_SAMPLES; i++) {
    const t = i / PIN_SLIDE_SAMPLES;
    const projected = camera_world_to_screen(camera, bezier_point(points, t), canvas.clientWidth, canvas.clientHeight);
    if (projected === null) continue;
    const distance = Math.hypot(projected.x - screen.x, projected.y - screen.y);
    if (distance < best_distance) {
      best_distance = distance;
      best_t = t;
    }
  }
  return { t: best_t, distance: best_distance };
}

// Nearest pinned vertex riding the edited stroke within pick range (its
// vertex id), or null.
function pick_pin_on_stroke(
  tablet_document: TabletDocument, stroke_id: StrokeId, camera: OrbitCamera, screen: V2, canvas: HTMLCanvasElement,
): VertexId | null {
  let best: VertexId | null = null;
  let best_distance = CONTROL_POINT_PICK_RADIUS_PIXELS;
  for (const pin of tablet_document.vertex_pins) {
    if (pin.host_stroke !== stroke_id) continue;
    const projected = camera_world_to_screen(
      camera, vertex_position(tablet_document, pin.vertex), canvas.clientWidth, canvas.clientHeight,
    );
    if (projected === null) continue;
    const distance = Math.hypot(projected.x - screen.x, projected.y - screen.y);
    if (distance < best_distance) {
      best_distance = distance;
      best = pin.vertex;
    }
  }
  return best;
}

// Returns false when the pen landed on neither a control point nor the stroke
// body — the caller should treat the drag as a camera orbit.
export function edit_pen_down(
  state: EditState, tablet_document: TabletDocument, camera: OrbitCamera, screen: V2, canvas: HTMLCanvasElement,
  layers: ReadonlySet<Layer>,
): boolean {
  const stroke = stroke_by_id(tablet_document, state.stroke_id);
  // A handle (p1/p2) is checked first, the same order a tap picks in
  // (sketchpad.ts pick_tap_target). Then the pins, before the endpoints: a pin
  // can sit right next to an endpoint (it rides the curve), and the endpoint is
  // still grabbable a bit further out.
  state.dragging_pin = null;
  state.dragging = pick_stroke_point(stroke, tablet_document, camera, screen, canvas);
  if (state.dragging !== "p1" && state.dragging !== "p2") {
    const pinned_vertex = pick_pin_on_stroke(tablet_document, state.stroke_id, camera, screen, canvas);
    if (pinned_vertex !== null) {
      state.dragging = null;
      state.dragging_pin = pinned_vertex;
    }
  }
  if (state.dragging === "p0" || state.dragging === "p3") {
    // A pinned vertex grabbed as another stroke's endpoint still slides on
    // its host curve — the pin owns the vertex's motion.
    const vertex_id = state.dragging === "p0" ? stroke.p0_vertex : stroke.p3_vertex;
    if (pin_by_vertex(tablet_document, vertex_id) !== null) {
      state.dragging = null;
      state.dragging_pin = vertex_id;
    }
  }
  state.moving_whole_stroke =
    state.dragging === null && state.dragging_pin === null &&
    pick_stroke(tablet_document, camera, screen, canvas, layers) === state.stroke_id;
  state.last_screen =
    state.dragging !== null || state.dragging_pin !== null || state.moving_whole_stroke ? screen : null;
  return state.last_screen !== null;
}

// Nearest document vertex within control-point pick range, or null. Any vertex
// (a stroke endpoint or a landmark) — sketchpad.ts checks it before the strokes so
// a tap on a vertex selects the vertex. Only vertices on `layers` count.
export function pick_vertex(
  tablet_document: TabletDocument, camera: OrbitCamera, screen: V2, canvas: HTMLCanvasElement, layers: ReadonlySet<Layer>,
): VertexId | null {
  let best: VertexId | null = null;
  let best_distance = CONTROL_POINT_PICK_RADIUS_PIXELS;
  const project = camera_screen_projector(camera, canvas.clientWidth, canvas.clientHeight);
  for (const vertex of tablet_document.vertices) {
    if (!vertex_is_on_layers(tablet_document, vertex.id, layers)) continue;
    const projected = project(vertex_world_position(tablet_document, vertex));
    if (projected === null) continue;
    const distance = Math.hypot(projected.x - screen.x, projected.y - screen.y);
    if (distance < best_distance) {
      best_distance = distance;
      best = vertex.id;
    }
  }
  return best;
}

// Screen-space pen delta mapped into the camera plane at pivot depth. All
// drags share this 1:1-with-the-pen feel (parallax off pivot depth accepted).
export function camera_plane_drag(
  camera: OrbitCamera, from_screen: V2, to_screen: V2, canvas: HTMLCanvasElement,
): V3 {
  const units_per_pixel = camera_world_units_per_pixel(camera, canvas.clientHeight);
  const basis = camera_basis(camera);
  return v3_add(
    v3_scale(basis.right, (to_screen.x - from_screen.x) * units_per_pixel),
    v3_scale(basis.up, -(to_screen.y - from_screen.y) * units_per_pixel),
  );
}

export function edit_pen_move(
  state: EditState, tablet_document: TabletDocument, camera: OrbitCamera, screen: V2, canvas: HTMLCanvasElement,
  handle_mode: HandleMode,
): void {
  if (state.last_screen === null) return;
  const world_delta = camera_plane_drag(camera, state.last_screen, screen, canvas);
  state.last_screen = screen;
  const stroke = stroke_by_id(tablet_document, state.stroke_id);
  if (state.moving_whole_stroke) {
    // d0/d3 are translation-invariant; moving both vertices moves the stroke
    // (and drags any strokes sharing those vertices — vertices connect).
    // Through move_vertex so those other strokes' offsets rotate with their
    // chords; this stroke's own chord is unchanged once both ends have moved.
    for (const vertex_id of [stroke.p0_vertex, stroke.p3_vertex]) {
      move_vertex(tablet_document, vertex_id, v3_add(vertex_position(tablet_document, vertex_id), world_delta));
    }
    return;
  }
  if (state.dragging_pin !== null) {
    // A pinned vertex only slides along its host curve (Q8): move t to the
    // curve point nearest the pen on screen, and place the vertex there.
    const pin = pin_by_vertex(tablet_document, state.dragging_pin)!;
    pin.t = nearest_t_on_stroke_screen(tablet_document, pin.host_stroke, camera, screen, canvas).t;
    const points = stroke_control_points(stroke_by_id(tablet_document, pin.host_stroke), tablet_document);
    move_vertex(tablet_document, pin.vertex, bezier_point(points, pin.t));
    return;
  }
  if (state.dragging === "p0" || state.dragging === "p3") {
    const vertex_id = state.dragging === "p0" ? stroke.p0_vertex : stroke.p3_vertex;
    move_vertex(tablet_document, vertex_id, v3_add(vertex_position(tablet_document, vertex_id), world_delta));
    return;
  }
  if (state.dragging === "p1" || state.dragging === "p2") {
    move_dragged_handle_leading_smooth_knots(state, stroke, tablet_document, () => {
      drag_handle(state, stroke, tablet_document, camera, screen, canvas, handle_mode, world_delta);
    });
  }
}

// Run `move_handle` (which moves the dragged handle, p1 or p2), then make the
// smooth knots at that end follow.
function move_dragged_handle_leading_smooth_knots(
  state: EditState, stroke: Stroke, tablet_document: TabletDocument, move_handle: () => void,
): void {
  // A handle drag at a smooth knot leads the neighbour: same direction, both
  // lengths scaled by the same factor (Q2) — so remember the length before.
  const knot_vertex = state.dragging === "p1" ? stroke.p0_vertex : stroke.p3_vertex;
  const knots = smooth_knots_at_vertex(tablet_document, knot_vertex)
    .filter((knot) => knot.stroke_a === stroke.id || knot.stroke_b === stroke.id);
  const old_length = dragged_handle_length(state, stroke, tablet_document);
  move_handle();
  const new_length = dragged_handle_length(state, stroke, tablet_document);
  const length_factor = old_length > 1e-9 ? new_length / old_length : 1;
  for (const knot of knots) enforce_smooth_knot(tablet_document, knot, stroke.id, length_factor);
}

// Keyboard move of a handle (p1/p2) of the edited stroke, through the same
// path as a pen drag of it, so every rule of a drag holds (stroke plane, swing,
// smooth neighbours). `screen_step` is in pixels from where the handle shows on
// screen. `forward_distance` is world units along the camera's forward: in
// plane mode it is projected into the stroke's plane (a plane facing the camera
// gives no move), in swing mode it applies freely.
export function edit_nudge_handle(
  state: EditState, tablet_document: TabletDocument, camera: OrbitCamera, canvas: HTMLCanvasElement,
  handle: "p1" | "p2", screen_step: V2, forward_distance: number, handle_mode: HandleMode,
): void {
  const stroke = stroke_by_id(tablet_document, state.stroke_id);
  const handle_world = stroke_control_points(stroke, tablet_document)[handle];
  const handle_screen = camera_world_to_screen(camera, handle_world, canvas.clientWidth, canvas.clientHeight);
  if (handle_screen === null) return; // behind the eye: no screen position to step from
  state.dragging = handle;
  state.dragging_pin = null;
  state.moving_whole_stroke = false;
  if (screen_step.x !== 0 || screen_step.y !== 0) {
    state.last_screen = handle_screen;
    edit_pen_move(
      state, tablet_document, camera, { x: handle_screen.x + screen_step.x, y: handle_screen.y + screen_step.y }, canvas, handle_mode,
    );
  }
  if (forward_distance !== 0) {
    const forward = camera_basis(camera).forward;
    move_dragged_handle_leading_smooth_knots(state, stroke, tablet_document, () => {
      const world_delta = v3_scale(forward, forward_distance);
      if (handle_mode === "swing") {
        swing_dragged_handle(state, stroke, tablet_document, world_delta);
        return;
      }
      const normal = stroke_plane_normal(stroke, tablet_document, forward);
      const dragged_key = handle === "p1" ? "d0" : "d3";
      stroke[dragged_key] = v3_add(stroke[dragged_key], v3_sub(world_delta, v3_scale(normal, v3_dot(world_delta, normal))));
    });
  }
  state.dragging = null;
  state.last_screen = null;
}

// Distance from the dragged handle (p1 or p2) to its own vertex.
function dragged_handle_length(state: EditState, stroke: Stroke, tablet_document: TabletDocument): number {
  const points = stroke_control_points(stroke, tablet_document);
  return state.dragging === "p1" ? v3_length(v3_sub(points.p1, points.p0)) : v3_length(v3_sub(points.p2, points.p3));
}

// Move the dragged interior handle with the pen — see the header comment for
// the HandleModes.
function drag_handle(
  state: EditState, stroke: Stroke, tablet_document: TabletDocument, camera: OrbitCamera, screen: V2,
  canvas: HTMLCanvasElement, handle_mode: HandleMode, world_delta: V3,
): void {
  {
    const dragged_key = state.dragging === "p1" ? "d0" : "d3";
    if (handle_mode === "swing") {
      swing_dragged_handle(state, stroke, tablet_document, world_delta);
      return;
    }
    // The handle goes where the pen ray pierces the stroke's plane: glued to
    // the pen on screen, the plane and the other handle untouched (Q4).
    const p0 = vertex_position(tablet_document, stroke.p0_vertex);
    const p3 = vertex_position(tablet_document, stroke.p3_vertex);
    const normal = stroke_plane_normal(stroke, tablet_document, camera_basis(camera).forward);
    const ray = camera_pen_ray(camera, screen, canvas.clientWidth, canvas.clientHeight);
    const cosine = v3_dot(ray.direction, normal);
    if (Math.abs(cosine) < EDGE_ON_PLANE_COSINE) return;
    const distance_along_ray = v3_dot(v3_sub(p0, ray.origin), normal) / cosine;
    if (distance_along_ray <= 0) return; // plane behind the eye
    const hit = v3_add(ray.origin, v3_scale(ray.direction, distance_along_ray));
    const third_point = state.dragging === "p1"
      ? v3_scale(v3_add(v3_scale(p0, 2), p3), 1 / 3)
      : v3_scale(v3_add(p0, v3_scale(p3, 2)), 1 / 3);
    stroke[dragged_key] = v3_sub(hit, third_point);
  }
}

// Swing mode: the dragged handle moves freely by `world_delta`; the other one
// swings into the plane the dragged handle now spans with the chord (Q2).
function swing_dragged_handle(state: EditState, stroke: Stroke, tablet_document: TabletDocument, world_delta: V3): void {
  const dragged_key = state.dragging === "p1" ? "d0" : "d3";
  const other_key = state.dragging === "p1" ? "d3" : "d0";
  stroke[dragged_key] = v3_add(stroke[dragged_key], world_delta);
  const chord = v3_sub(vertex_position(tablet_document, stroke.p3_vertex), vertex_position(tablet_document, stroke.p0_vertex));
  if (v3_length(chord) < 1e-9) return; // no chord, no plane to keep
  stroke[other_key] = swing_offset_into_plane(v3_normalize(chord), stroke[dragged_key], stroke[other_key]);
}

// The vertex the dragged vertex would weld into on release: nearest other
// vertex within world-space snap range — null when none is in range, or when
// the merge would leave any stroke with both endpoints on the same vertex.
// Also drives the drag-time highlight, so it must match the merge exactly.
export function find_merge_target_vertex(tablet_document: TabletDocument, dragged_vertex: VertexId, layers: ReadonlySet<Layer>): VertexId | null {
  const target_vertex = pick_vertex_near_world_point(
    tablet_document, vertex_position(tablet_document, dragged_vertex), dragged_vertex, layers,
  );
  if (target_vertex === null) return null;
  const remap = (vertex: VertexId) => (vertex === dragged_vertex ? target_vertex! : vertex);
  for (const stroke of tablet_document.strokes) {
    if (remap(stroke.p0_vertex) === remap(stroke.p3_vertex)) return null;
  }
  // Q10 guard: a merge must not leave a pinned vertex as an endpoint of its
  // own host stroke (the constraint would chase its own curve).
  for (const pin of tablet_document.vertex_pins) {
    const host = stroke_by_id(tablet_document, pin.host_stroke);
    const pinned_vertex = remap(pin.vertex);
    if (remap(host.p0_vertex) === pinned_vertex || remap(host.p3_vertex) === pinned_vertex) return null;
  }
  return target_vertex;
}

// Merge the dragged vertex into another vertex within world-space snap range
// (same feel as draw-time endpoint snapping): every stroke referencing it is
// rewired to the target, welding the junction, and the vertex is removed.
export function merge_vertex_if_near_another(tablet_document: TabletDocument, dragged_vertex: VertexId, layers: ReadonlySet<Layer>): boolean {
  const target_vertex = find_merge_target_vertex(tablet_document, dragged_vertex, layers);
  if (target_vertex === null) return false;
  // Snap first so the strokes ending on the dragged vertex rotate their
  // offsets with the chord change (Q4), then rewire them to the target.
  move_vertex(tablet_document, dragged_vertex, vertex_position(tablet_document, target_vertex));
  const remap = (vertex: VertexId) => (vertex === dragged_vertex ? target_vertex : vertex);
  for (const stroke of tablet_document.strokes) {
    stroke.p0_vertex = remap(stroke.p0_vertex);
    stroke.p3_vertex = remap(stroke.p3_vertex);
  }
  // A knot at the dragged vertex rides along (a crossing = two knots, Q4).
  for (const knot of tablet_document.smooth_knots) knot.vertex = remap(knot.vertex);
  // The survivor is on the midline if either was (plan-sketchpad-midline.md Q75).
  if (vertex_by_id(tablet_document, dragged_vertex).midline === true) {
    vertex_by_id(tablet_document, target_vertex).midline = true;
  }
  // A landmark's name survives the weld: an unnamed survivor takes the dragged
  // vertex's name (a named survivor keeps its own).
  const dragged_name = vertex_by_id(tablet_document, dragged_vertex).name;
  if (dragged_name !== undefined && vertex_by_id(tablet_document, target_vertex).name === undefined) {
    vertex_by_id(tablet_document, target_vertex).name = dragged_name;
  }
  // The dragged vertex is never pinned (pin drags slide t and skip merging),
  // so no pin references it.
  tablet_document.vertices = tablet_document.vertices.filter((vertex) => vertex.id !== dragged_vertex);
  return true;
}

// Pin the dragged vertex to the curve it was released next to (vertex weld
// has priority — call after merge_vertex_if_near_another misses). The vertex
// snaps onto the curve through move_vertex so its strokes' offsets follow.
// A pin and the midline both claim the vertex's position (plan-sketchpad-midline.md
// Q71): the pin, being newer, drops the vertex's own midline flag. A vertex held
// on the midline by a midline stroke is not pinned — that flag belongs to the
// stroke and is not dropped behind the user's back.
function pin_vertex_if_near_curve(tablet_document: TabletDocument, dragged_vertex: VertexId, layers: ReadonlySet<Layer>): void {
  const target = find_snap_target_stroke(tablet_document, dragged_vertex, layers);
  if (target === null) return;
  const vertex = vertex_by_id(tablet_document, dragged_vertex);
  delete vertex.midline;
  if (vertex_is_on_midline(tablet_document, dragged_vertex)) return;
  const points = stroke_control_points(stroke_by_id(tablet_document, target.stroke_id), tablet_document);
  move_vertex(tablet_document, dragged_vertex, bezier_point(points, target.t));
  tablet_document.vertex_pins.push({ vertex: dragged_vertex, host_stroke: target.stroke_id, t: target.t });
}

// `layers`: a released vertex welds to / pins on strokes of these layers only.
// `pen_really_dragged`: false for a tap, which never welds or pins anything.
export function edit_pen_up(
  state: EditState, tablet_document: TabletDocument, layers: ReadonlySet<Layer>, pen_really_dragged: boolean,
): void {
  if (pen_really_dragged && (state.dragging === "p0" || state.dragging === "p3")) {
    const stroke = stroke_by_id(tablet_document, state.stroke_id);
    const dragged_vertex = state.dragging === "p0" ? stroke.p0_vertex : stroke.p3_vertex;
    if (!merge_vertex_if_near_another(tablet_document, dragged_vertex, layers)) {
      pin_vertex_if_near_curve(tablet_document, dragged_vertex, layers);
    }
  }
  state.dragging = null;
  state.dragging_pin = null;
  state.moving_whole_stroke = false;
  state.last_screen = null;
}
