// Line tool (Q27): an armed tool — while armed, a pen drag places one cubic
// stroke (down = p0, up = p3), always the straight segment between the two; it
// is shown live while the pen is down. Nothing snaps: each end the pen places
// is a new vertex (join it to another afterwards by dragging it onto that one).
// A start lies on the camera-facing plane through the pivot; the end lies on
// the camera-facing plane through the start point. A tap (no drag) exits the tool.

import { OrbitCamera, camera_basis, camera_pen_ray } from "./camera";
import { BoneId, Layer, StrokeId, TabletDocument, VertexId, add_straight_stroke, add_vertex, vertex_position } from "./document";
import { V2, V3, v3_add, v3_dot, v3_scale, v3_sub } from "./math";

// Both endpoints while the pen is down. `start_vertex`: the existing vertex the
// stroke starts from (null = a new vertex will be created on pen-up).
export type LineToolState = {
  start_world: V3;
  start_vertex: VertexId | null;
  end_world: V3;
};

// Ray-cast a screen point onto the camera-facing plane through `plane_point`.
export function pen_point_on_camera_plane(
  camera: OrbitCamera, screen: V2, canvas: HTMLCanvasElement, plane_point: V3,
): V3 | null {
  const ray = camera_pen_ray(camera, screen, canvas.clientWidth, canvas.clientHeight);
  const normal = camera_basis(camera).forward;
  const denominator = v3_dot(ray.direction, normal);
  if (Math.abs(denominator) < 1e-9) return null;
  const t = v3_dot(v3_sub(plane_point, ray.origin), normal) / denominator;
  if (t <= 0) return null;
  return v3_add(ray.origin, v3_scale(ray.direction, t));
}

// With `start_vertex`, the stroke starts at that vertex wherever the pen lands
// (the line button pressed with one vertex selected).
export function line_pen_down(
  tablet_document: TabletDocument, camera: OrbitCamera, screen: V2, canvas: HTMLCanvasElement,
  start_vertex: VertexId | null = null,
): LineToolState | null {
  const start_world = start_vertex !== null
    ? vertex_position(tablet_document, start_vertex)
    : pen_point_on_camera_plane(camera, screen, canvas, camera.pivot);
  if (start_world === null) return null;
  const end_world = pen_point_on_camera_plane(camera, screen, canvas, start_world);
  if (end_world === null) return null;
  return { start_world, start_vertex, end_world };
}

export function line_pen_move(state: LineToolState, camera: OrbitCamera, screen: V2, canvas: HTMLCanvasElement): void {
  const end_world = pen_point_on_camera_plane(camera, screen, canvas, state.start_world);
  if (end_world !== null) state.end_world = end_world;
}

// Commit the drag as a straight stroke, creating a vertex for each end the pen placed.
export function line_pen_up(state: LineToolState, tablet_document: TabletDocument, layer: Layer, bone_id: BoneId): StrokeId {
  const p0_vertex = state.start_vertex !== null ? state.start_vertex : add_vertex(tablet_document, state.start_world, bone_id);
  const p3_vertex = add_vertex(tablet_document, state.end_world, bone_id);
  return add_straight_stroke(tablet_document, p0_vertex, p3_vertex, layer);
}
