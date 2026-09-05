// Edit mode: pen-tap a stroke to select it. Dragging one of its four control
// points reshapes it — vertices (p0/p3) move in the camera plane and carry
// every attached stroke with them; the p1/p2 handles slide inside the
// stroke's plane (pen ray ∩ plane, plan-tablet-planar-handle-drags.html Q4),
// so the other handle never moves. Two experimental tilt modes (HandleMode):
// "dial" picks nothing — any drag spins the whole stroke about its chord,
// Procreate-dial style, horizontal pen travel mapping to the roll angle (Q8);
// "swing" keeps picking, but a dragged handle follows the pen in the camera
// plane and the other handle swings into the new plane
// (plan-tablet-free-handles-coplanar.md Q2/Q3). Releasing a vertex drag near another vertex
// merges the two into one shared junction. Pinned vertices (vertex_pins) only
// ever slide along their host curve, whichever way they're grabbed.
// A drag starting ON the stroke body translates
// the whole stroke (both vertices) in the camera plane; a drag starting on
// empty space is NOT consumed — the caller orbits the camera instead (Q35).

import { OrbitCamera, camera_basis, camera_pen_ray, camera_world_to_screen, camera_world_units_per_pixel } from "./camera";
import { Stroke, TabletDocument, bezier_point, move_vertex, pick_vertex_near_world_point, stroke_control_points, stroke_plane_normal, swing_offset_into_plane } from "./document";
import { V2, V3, v3_add, v3_dot, v3_length, v3_normalize, v3_rotate_about_axis, v3_scale, v3_sub } from "./math";

// NOTE: tap = max displacement from the pen-down point, NOT accumulated path
// length — a real Apple Pencil tap jitters through many sub-pixel moves whose
// path sum easily exceeds any threshold.
export const TAP_MAX_MOVEMENT_PIXELS = 12;

export const STROKE_PICK_RADIUS_PIXELS = 24;
const CONTROL_POINT_PICK_RADIUS_PIXELS = 20;
const PICK_SAMPLES_PER_STROKE = 16;
const PIN_SLIDE_SAMPLES = 128; // t resolution when sliding a pinned vertex
// Below this |cos| between the pen ray and the stroke plane's normal (plane
// within ~9° of edge-on) the ray∩plane hit runs off to infinity: the handle
// stays put instead (Q5).
const EDGE_ON_PLANE_COSINE = 0.15;
// Tilt dial: one full turn of the stroke about its chord per ~630 px of
// horizontal pen travel.
const TILT_RADIANS_PER_PIXEL = 0.01;

// How p1/p2 handle drags behave — see the header comment.
export type HandleMode = "plane" | "dial" | "swing";

export type StrokePointKey = "p0" | "p1" | "p2" | "p3";

export type EditState = {
  stroke_index: number;
  dragging: StrokePointKey | null; // pen is down on a control point
  dragging_pin: number | null; // index into vertex_pins; pen is down on a pinned vertex
  selected_pin: number | null; // last pin the pen landed on — the unpin button's target
  moving_whole_stroke: boolean; // pen is down on the stroke body
  tilting: boolean; // pen is down in tilt mode — the drag rolls the stroke about its chord
  last_screen: V2 | null; // previous pen position while a drag is active
};

export function begin_edit_state(stroke_index: number): EditState {
  return {
    stroke_index, dragging: null, dragging_pin: null, selected_pin: null,
    moving_whole_stroke: false, tilting: false, last_screen: null,
  };
}

// Nearest stroke within pick range of a screen tap, or null.
export function pick_stroke(
  tablet_document: TabletDocument, camera: OrbitCamera, screen: V2, canvas: HTMLCanvasElement,
): number | null {
  let best_index: number | null = null;
  let best_distance = STROKE_PICK_RADIUS_PIXELS;
  for (let stroke_index = 0; stroke_index < tablet_document.strokes.length; stroke_index++) {
    const points = stroke_control_points(tablet_document.strokes[stroke_index], tablet_document);
    for (let i = 0; i <= PICK_SAMPLES_PER_STROKE; i++) {
      const world = bezier_point(points, i / PICK_SAMPLES_PER_STROKE);
      const projected = camera_world_to_screen(camera, world, canvas.clientWidth, canvas.clientHeight);
      if (projected === null) continue;
      const distance = Math.hypot(projected.x - screen.x, projected.y - screen.y);
      if (distance < best_distance) {
        best_distance = distance;
        best_index = stroke_index;
      }
    }
  }
  return best_index;
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
  tablet_document: TabletDocument, stroke_index: number, camera: OrbitCamera, screen: V2, canvas: HTMLCanvasElement,
): { t: number; distance: number } {
  const points = stroke_control_points(tablet_document.strokes[stroke_index], tablet_document);
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

// Nearest pinned vertex riding the edited stroke within pick range, or null.
function pick_pin_on_stroke(
  tablet_document: TabletDocument, stroke_index: number, camera: OrbitCamera, screen: V2, canvas: HTMLCanvasElement,
): number | null {
  let best: number | null = null;
  let best_distance = CONTROL_POINT_PICK_RADIUS_PIXELS;
  for (let pin_index = 0; pin_index < tablet_document.vertex_pins.length; pin_index++) {
    const pin = tablet_document.vertex_pins[pin_index];
    if (pin.host_stroke !== stroke_index) continue;
    const projected = camera_world_to_screen(
      camera, tablet_document.vertices[pin.vertex], canvas.clientWidth, canvas.clientHeight,
    );
    if (projected === null) continue;
    const distance = Math.hypot(projected.x - screen.x, projected.y - screen.y);
    if (distance < best_distance) {
      best_distance = distance;
      best = pin_index;
    }
  }
  return best;
}

// Returns false when the pen landed on neither a control point nor the stroke
// body — the caller should treat the drag as a camera orbit. In dial mode
// every pen-down is consumed: the drag is the dial, wherever it starts.
export function edit_pen_down(
  state: EditState, tablet_document: TabletDocument, camera: OrbitCamera, screen: V2, canvas: HTMLCanvasElement,
  handle_mode: HandleMode,
): boolean {
  const stroke = tablet_document.strokes[state.stroke_index];
  if (handle_mode === "dial") {
    state.tilting = true;
    state.last_screen = screen;
    return true;
  }
  // Pins are checked before control points: a pin can sit right next to a
  // handle (it rides the curve), and the handle is still grabbable a bit
  // further out.
  state.dragging = null;
  state.dragging_pin = pick_pin_on_stroke(tablet_document, state.stroke_index, camera, screen, canvas);
  if (state.dragging_pin === null) {
    state.dragging = pick_stroke_point(stroke, tablet_document, camera, screen, canvas);
    if (state.dragging === "p0" || state.dragging === "p3") {
      // A pinned vertex grabbed as another stroke's endpoint still slides on
      // its host curve — the pin owns the vertex's motion.
      const vertex_index = state.dragging === "p0" ? stroke.p0_vertex : stroke.p3_vertex;
      const pin_index = tablet_document.vertex_pins.findIndex((pin) => pin.vertex === vertex_index);
      if (pin_index !== -1) {
        state.dragging = null;
        state.dragging_pin = pin_index;
      }
    }
  }
  // Pin selection follows the pen: landing on a pin selects it for the unpin
  // button; landing anywhere else clears it.
  state.selected_pin = state.dragging_pin;
  state.moving_whole_stroke =
    state.dragging === null && state.dragging_pin === null &&
    pick_stroke(tablet_document, camera, screen, canvas) === state.stroke_index;
  state.last_screen =
    state.dragging !== null || state.dragging_pin !== null || state.moving_whole_stroke ? screen : null;
  return state.last_screen !== null;
}

// Screen-space pen delta mapped into the camera plane at pivot depth. All
// drags share this 1:1-with-the-pen feel (parallax off pivot depth accepted).
function camera_plane_drag(
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
  const screen_dx = screen.x - state.last_screen.x;
  const world_delta = camera_plane_drag(camera, state.last_screen, screen, canvas);
  state.last_screen = screen;
  const stroke = tablet_document.strokes[state.stroke_index];
  if (state.tilting) {
    // Both handles rotate rigidly about the chord: the plane rolls, the
    // curve's shape within it is untouched. Rightward drag = positive angle
    // about the p0→p3 axis.
    const chord = v3_sub(tablet_document.vertices[stroke.p3_vertex], tablet_document.vertices[stroke.p0_vertex]);
    if (v3_length(chord) < 1e-9) return; // no chord, no axis
    const axis = v3_normalize(chord);
    const angle = screen_dx * TILT_RADIANS_PER_PIXEL;
    stroke.d0 = v3_rotate_about_axis(stroke.d0, axis, angle);
    stroke.d3 = v3_rotate_about_axis(stroke.d3, axis, angle);
    return;
  }
  if (state.moving_whole_stroke) {
    // d0/d3 are translation-invariant; moving both vertices moves the stroke
    // (and drags any strokes sharing those vertices — vertices connect).
    // Through move_vertex so those other strokes' offsets rotate with their
    // chords; this stroke's own chord is unchanged once both ends have moved.
    for (const vertex_index of [stroke.p0_vertex, stroke.p3_vertex]) {
      move_vertex(tablet_document, vertex_index, v3_add(tablet_document.vertices[vertex_index], world_delta));
    }
    return;
  }
  if (state.dragging_pin !== null) {
    // A pinned vertex only slides along its host curve (Q8): move t to the
    // curve point nearest the pen on screen, and place the vertex there.
    const pin = tablet_document.vertex_pins[state.dragging_pin];
    pin.t = nearest_t_on_stroke_screen(tablet_document, pin.host_stroke, camera, screen, canvas).t;
    const points = stroke_control_points(tablet_document.strokes[pin.host_stroke], tablet_document);
    move_vertex(tablet_document, pin.vertex, bezier_point(points, pin.t));
    return;
  }
  if (state.dragging === "p0" || state.dragging === "p3") {
    const vertex_index = state.dragging === "p0" ? stroke.p0_vertex : stroke.p3_vertex;
    move_vertex(tablet_document, vertex_index, v3_add(tablet_document.vertices[vertex_index], world_delta));
    return;
  }
  if (state.dragging === "p1" || state.dragging === "p2") {
    const dragged_key = state.dragging === "p1" ? "d0" : "d3";
    if (handle_mode === "swing") {
      // The dragged handle moves freely with the pen; the other one swings into
      // the plane the dragged handle now spans with the chord (Q2).
      const other_key = state.dragging === "p1" ? "d3" : "d0";
      stroke[dragged_key] = v3_add(stroke[dragged_key], world_delta);
      const chord = v3_sub(tablet_document.vertices[stroke.p3_vertex], tablet_document.vertices[stroke.p0_vertex]);
      if (v3_length(chord) < 1e-9) return; // no chord, no plane to keep
      stroke[other_key] = swing_offset_into_plane(v3_normalize(chord), stroke[dragged_key], stroke[other_key]);
      return;
    }
    // The handle goes where the pen ray pierces the stroke's plane: glued to
    // the pen on screen, the plane and the other handle untouched (Q4).
    const p0 = tablet_document.vertices[stroke.p0_vertex];
    const p3 = tablet_document.vertices[stroke.p3_vertex];
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

// The vertex the dragged vertex would weld into on release: nearest other
// vertex within world-space snap range — null when none is in range, or when
// the merge would leave any stroke with both endpoints on the same vertex.
// Also drives the drag-time highlight, so it must match the merge exactly.
export function find_merge_target_vertex(tablet_document: TabletDocument, dragged_vertex: number): number | null {
  const target_vertex = pick_vertex_near_world_point(
    tablet_document, tablet_document.vertices[dragged_vertex], dragged_vertex,
  );
  if (target_vertex === null) return null;
  const remap = (vertex: number) => (vertex === dragged_vertex ? target_vertex! : vertex);
  for (const stroke of tablet_document.strokes) {
    if (remap(stroke.p0_vertex) === remap(stroke.p3_vertex)) return null;
  }
  // Q10 guard: a merge must not leave a pinned vertex as an endpoint of its
  // own host stroke (the constraint would chase its own curve).
  for (const pin of tablet_document.vertex_pins) {
    const host = tablet_document.strokes[pin.host_stroke];
    const pinned_vertex = remap(pin.vertex);
    if (remap(host.p0_vertex) === pinned_vertex || remap(host.p3_vertex) === pinned_vertex) return null;
  }
  return target_vertex;
}

// Merge the dragged vertex into another vertex within world-space snap range
// (same feel as draw-time endpoint snapping): every stroke referencing it is
// rewired to the target, welding the junction, and the vertex is removed.
function merge_vertex_if_near_another(tablet_document: TabletDocument, dragged_vertex: number): void {
  const target_vertex = find_merge_target_vertex(tablet_document, dragged_vertex);
  if (target_vertex === null) return;
  // Snap first so the strokes ending on the dragged vertex rotate their
  // offsets with the chord change (Q4), then rewire them to the target.
  move_vertex(tablet_document, dragged_vertex, tablet_document.vertices[target_vertex]);
  const remap = (vertex: number) => (vertex === dragged_vertex ? target_vertex : vertex);
  for (const stroke of tablet_document.strokes) {
    stroke.p0_vertex = remap(stroke.p0_vertex);
    stroke.p3_vertex = remap(stroke.p3_vertex);
  }
  tablet_document.vertices.splice(dragged_vertex, 1);
  for (const stroke of tablet_document.strokes) {
    if (stroke.p0_vertex > dragged_vertex) stroke.p0_vertex--;
    if (stroke.p3_vertex > dragged_vertex) stroke.p3_vertex--;
  }
  // The dragged vertex is never pinned (pin drags slide t and skip merging),
  // so pins only need the index shift.
  for (const pin of tablet_document.vertex_pins) {
    if (pin.vertex > dragged_vertex) pin.vertex--;
  }
}

export function edit_pen_up(state: EditState, tablet_document: TabletDocument): void {
  if (state.dragging === "p0" || state.dragging === "p3") {
    const stroke = tablet_document.strokes[state.stroke_index];
    const dragged_vertex = state.dragging === "p0" ? stroke.p0_vertex : stroke.p3_vertex;
    merge_vertex_if_near_another(tablet_document, dragged_vertex);
  }
  state.dragging = null;
  state.dragging_pin = null;
  state.moving_whole_stroke = false;
  state.tilting = false;
  state.last_screen = null;
}
