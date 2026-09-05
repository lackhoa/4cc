// Line tool (Q27): an armed tool — while armed, a pen drag places one cubic
// stroke on the camera-facing plane through the pivot (down = p0, up = p3);
// the pen's path is shown live and least-squares-fitted to the stroke's
// interior control points on pen-up, so the stroke resembles the drawn curve.
// Endpoints snap to existing vertices within a screen radius, reusing them so
// strokes join at shared vertices. A tap (no drag) exits the tool.

import { OrbitCamera, camera_basis, camera_pen_ray, camera_world_to_screen } from "./camera";
import { Stroke, TabletDocument, stroke_handles_from_control_points } from "./document";
import { V2, V3, v3, v3_add, v3_dot, v3_length, v3_scale, v3_sub } from "./math";

const VERTEX_SNAP_RADIUS_PIXELS = 20;

// Both endpoints while the pen is down: the world position, plus the existing
// vertex index it snapped to (null = a new vertex will be created on pen-up).
// path_world is every raw (unsnapped) plane sample from down to up, the input
// to the curve fit and the live preview polyline.
export type LineToolState = {
  start_world: V3;
  start_snap_vertex: number | null;
  end_world: V3;
  end_snap_vertex: number | null;
  path_world: V3[];
};

// Ray-cast a screen point onto the camera-facing plane through the pivot.
export function pen_point_on_camera_plane(
  camera: OrbitCamera, screen: V2, canvas: HTMLCanvasElement,
): V3 | null {
  const ray = camera_pen_ray(camera, screen, canvas.clientWidth, canvas.clientHeight);
  const normal = camera_basis(camera).forward;
  const denominator = v3_dot(ray.direction, normal);
  if (Math.abs(denominator) < 1e-9) return null;
  const t = v3_dot(v3_sub(camera.pivot, ray.origin), normal) / denominator;
  if (t <= 0) return null;
  return v3_add(ray.origin, v3_scale(ray.direction, t));
}

// Nearest existing vertex within snap range of a screen point, or null.
export function pick_vertex(
  tablet_document: TabletDocument, camera: OrbitCamera, screen: V2, canvas: HTMLCanvasElement,
): number | null {
  let best_index: number | null = null;
  let best_distance = VERTEX_SNAP_RADIUS_PIXELS;
  for (let vertex_index = 0; vertex_index < tablet_document.vertices.length; vertex_index++) {
    const projected = camera_world_to_screen(
      camera, tablet_document.vertices[vertex_index], canvas.clientWidth, canvas.clientHeight,
    );
    if (projected === null) continue;
    const distance = Math.hypot(projected.x - screen.x, projected.y - screen.y);
    if (distance < best_distance) {
      best_distance = distance;
      best_index = vertex_index;
    }
  }
  return best_index;
}

function resolve_endpoint(
  tablet_document: TabletDocument, camera: OrbitCamera, screen: V2, canvas: HTMLCanvasElement,
): { world: V3; snap_vertex: number | null } | null {
  const snap_vertex = pick_vertex(tablet_document, camera, screen, canvas);
  if (snap_vertex !== null) {
    return { world: tablet_document.vertices[snap_vertex], snap_vertex };
  }
  const world = pen_point_on_camera_plane(camera, screen, canvas);
  return world === null ? null : { world, snap_vertex: null };
}

export function line_pen_down(
  tablet_document: TabletDocument, camera: OrbitCamera, screen: V2, canvas: HTMLCanvasElement,
): LineToolState | null {
  const endpoint = resolve_endpoint(tablet_document, camera, screen, canvas);
  if (endpoint === null) return null;
  const plane_point = pen_point_on_camera_plane(camera, screen, canvas);
  return {
    start_world: endpoint.world,
    start_snap_vertex: endpoint.snap_vertex,
    end_world: endpoint.world,
    end_snap_vertex: endpoint.snap_vertex,
    path_world: [plane_point === null ? endpoint.world : plane_point],
  };
}

export function line_pen_move(
  state: LineToolState, tablet_document: TabletDocument, camera: OrbitCamera,
  screen: V2, canvas: HTMLCanvasElement,
): void {
  const endpoint = resolve_endpoint(tablet_document, camera, screen, canvas);
  if (endpoint === null) return;
  state.end_world = endpoint.world;
  state.end_snap_vertex = endpoint.snap_vertex;
  const plane_point = pen_point_on_camera_plane(camera, screen, canvas);
  if (plane_point !== null) state.path_world.push(plane_point);
}

// Least-squares fit of the two interior control points to the pen path
// (endpoints fixed, chord-length parameterization), converted to the offset
// handle representation. Falls back to a straight stroke's handles when the
// path is too short or the normal equations are degenerate.
export function fit_stroke_handles(path: V3[], p0: V3, p3: V3): { d0: V3; d3: V3 } {
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

  // The fitted p1/p2 are near-coplanar with the chord (the pen path lives on
  // one camera plane); the handle conversion swings p2 into p1's plane.
  return stroke_handles_from_control_points(p0, p1, p2, p3);
}

// Commit the pen path as a fitted cubic stroke, creating vertices for
// unsnapped endpoints. Returns the new stroke's index, or null when the two
// endpoints collapsed to the same vertex.
export function line_pen_up(state: LineToolState, tablet_document: TabletDocument): number | null {
  if (state.start_snap_vertex !== null && state.start_snap_vertex === state.end_snap_vertex) return null;
  const claim_vertex = (world: V3, snap_vertex: number | null): number => {
    if (snap_vertex !== null) return snap_vertex;
    tablet_document.vertices.push(world);
    return tablet_document.vertices.length - 1;
  };
  const p0_vertex = claim_vertex(state.start_world, state.start_snap_vertex);
  const p3_vertex = claim_vertex(state.end_world, state.end_snap_vertex);
  const handles = fit_stroke_handles(state.path_world, state.start_world, state.end_world);
  const stroke: Stroke = { p0_vertex, p3_vertex, d0: handles.d0, d3: handles.d3 };
  tablet_document.strokes.push(stroke);
  return tablet_document.strokes.length - 1;
}
