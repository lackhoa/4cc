// Edit mode: pen-tap a stroke to select it. Dragging one of its four control
// points reshapes it — vertices (p0/p3) move in the camera plane and carry
// every attached stroke with them; the p1 handle sets d0 freely in 3D
// (camera-plane drag); the p2 handle solves d3 inside the stroke's plane, so
// the curve stays planar (Q34). A drag starting ON the stroke body translates
// the whole stroke (both vertices) in the camera plane; a drag starting on
// empty space is NOT consumed — the caller orbits the camera instead (Q35).

import { OrbitCamera, camera_basis, camera_world_to_screen, camera_world_units_per_pixel } from "./camera";
import { Stroke, TabletDocument, bezier_point, stroke_control_points, stroke_frame } from "./document";
import { V2, V3, v3_add, v3_dot, v3_scale, v3_sub } from "./math";

// NOTE: tap = max displacement from the pen-down point, NOT accumulated path
// length — a real Apple Pencil tap jitters through many sub-pixel moves whose
// path sum easily exceeds any threshold.
export const TAP_MAX_MOVEMENT_PIXELS = 12;

const STROKE_PICK_RADIUS_PIXELS = 24;
const CONTROL_POINT_PICK_RADIUS_PIXELS = 20;
const PICK_SAMPLES_PER_STROKE = 16;

export type StrokePointKey = "p0" | "p1" | "p2" | "p3";

export type EditState = {
  stroke_index: number;
  dragging: StrokePointKey | null; // pen is down on a control point
  moving_whole_stroke: boolean; // pen is down on the stroke body
  last_screen: V2 | null; // previous pen position while a drag is active
};

export function begin_edit_state(stroke_index: number): EditState {
  return { stroke_index, dragging: null, moving_whole_stroke: false, last_screen: null };
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
  let best: StrokePointKey | null = null;
  let best_distance = CONTROL_POINT_PICK_RADIUS_PIXELS;
  const keys: StrokePointKey[] = ["p0", "p1", "p2", "p3"];
  for (const key of keys) {
    const projected = camera_world_to_screen(camera, points[key], canvas.clientWidth, canvas.clientHeight);
    if (projected === null) continue;
    const distance = Math.hypot(projected.x - screen.x, projected.y - screen.y);
    if (distance < best_distance) {
      best_distance = distance;
      best = key;
    }
  }
  return best;
}

// Returns false when the pen landed on neither a control point nor the stroke
// body — the caller should treat the drag as a camera orbit.
export function edit_pen_down(
  state: EditState, tablet_document: TabletDocument, camera: OrbitCamera, screen: V2, canvas: HTMLCanvasElement,
): boolean {
  const stroke = tablet_document.strokes[state.stroke_index];
  state.dragging = pick_stroke_point(stroke, tablet_document, camera, screen, canvas);
  state.moving_whole_stroke =
    state.dragging === null && pick_stroke(tablet_document, camera, screen, canvas) === state.stroke_index;
  state.last_screen = state.dragging !== null || state.moving_whole_stroke ? screen : null;
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
): void {
  if (state.last_screen === null) return;
  const world_delta = camera_plane_drag(camera, state.last_screen, screen, canvas);
  state.last_screen = screen;
  const stroke = tablet_document.strokes[state.stroke_index];
  if (state.moving_whole_stroke) {
    // d0/d3 are translation-invariant; moving both vertices moves the stroke
    // (and drags any strokes sharing those vertices — vertices connect).
    for (const vertex_index of [stroke.p0_vertex, stroke.p3_vertex]) {
      tablet_document.vertices[vertex_index] = v3_add(tablet_document.vertices[vertex_index], world_delta);
    }
    return;
  }
  if (state.dragging === "p0" || state.dragging === "p3") {
    const vertex_index = state.dragging === "p0" ? stroke.p0_vertex : stroke.p3_vertex;
    tablet_document.vertices[vertex_index] = v3_add(tablet_document.vertices[vertex_index], world_delta);
    return;
  }
  const points = stroke_control_points(stroke, tablet_document);
  if (state.dragging === "p1") {
    // d0 is free: p1 follows the pen, and the stroke's plane rotates with it.
    const p0 = tablet_document.vertices[stroke.p0_vertex];
    const p3 = tablet_document.vertices[stroke.p3_vertex];
    const p1_target = v3_add(points.p1, world_delta);
    stroke.d0 = v3_sub(p1_target, v3_scale(v3_add(v3_scale(p0, 2), p3), 1 / 3));
    return;
  }
  if (state.dragging === "p2") {
    // d3 is plane-confined: project the dragged p2 into the stroke's frame,
    // discarding the out-of-plane component of the pen motion.
    const frame = stroke_frame(stroke, tablet_document);
    const p2_target = v3_add(points.p2, world_delta);
    const from_mid = v3_sub(p2_target, v3_scale(v3_add(points.p1, points.p3), 0.5));
    const u2_squared = v3_dot(frame.u2, frame.u2);
    const v_squared = v3_dot(frame.v, frame.v);
    if (u2_squared < 1e-12) return; // p1 sits on p3 — no frame to solve in
    stroke.d3 = {
      x: v3_dot(from_mid, frame.u2) / u2_squared,
      y: v_squared < 1e-12 ? stroke.d3.y : v3_dot(from_mid, frame.v) / v_squared,
    };
  }
}

export function edit_pen_up(state: EditState): void {
  state.dragging = null;
  state.moving_whole_stroke = false;
  state.last_screen = null;
}
