// Line tool (Q27): an armed tool — while armed, a pen drag places one cubic
// stroke (down = p0, up = p3), always the straight segment between the two; it
// is shown live while the pen is down. Each end the pen places is a new vertex
// that snaps to no other vertex (join it to another afterwards by dragging it
// onto that one). A tap (no drag) exits the tool.
//
// Where an end lands: with "surf" on and a fill of a pickable layer under the
// pen, on the point of that fill nearest the eye (position only: the vertex is
// free, not pinned to the surface). Otherwise a start lies on the camera-facing
// plane through the pivot, and the end on the camera-facing plane through the
// start point. A start on an existing vertex (the line button pressed with one
// vertex selected) stays that vertex. A new line whose two ends both landed on
// a fill is drawn on the surface (its "on surface" flag); the "on surface"
// button on the selection (sketchpad.ts) turns that on or off afterwards.

import { OrbitCamera, camera_basis, camera_pen_ray } from "./camera";
import { BoneId, Layer, StrokeId, TabletDocument, VertexId, add_straight_stroke, add_vertex, stroke_by_id, vertex_position } from "./document";
import { V2, V3, v3_add, v3_dot, v3_scale, v3_sub } from "./math";
import { pick_surface_point } from "./patch";

// Both endpoints while the pen is down. `start_vertex`: the existing vertex the
// start is (null = a new vertex will be created on pen-up). `surface_layers`:
// the layers whose fills catch the pen (null = "surf" off, no fill catches it).
// `start_is_on_surface` / `end_is_on_surface`: that end landed on a fill.
export type LineToolState = {
  start_world: V3;
  start_vertex: VertexId | null;
  end_world: V3;
  surface_layers: ReadonlySet<Layer> | null;
  start_is_on_surface: boolean;
  end_is_on_surface: boolean;
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

// Where a pen end lands: the fill point under the pen when a fill of
// `surface_layers` is there, else the camera-facing plane through `plane_point`.
// Null when the pen ray misses that plane.
function pen_point_on_surface_or_camera_plane(
  tablet_document: TabletDocument, camera: OrbitCamera, screen: V2, canvas: HTMLCanvasElement,
  surface_layers: ReadonlySet<Layer> | null, plane_point: V3,
): { position: V3; is_on_surface: boolean } | null {
  if (surface_layers !== null) {
    const surface_hit = pick_surface_point(tablet_document, camera, screen, canvas, surface_layers);
    if (surface_hit !== null) return { position: surface_hit.position, is_on_surface: true };
  }
  const plane_position = pen_point_on_camera_plane(camera, screen, canvas, plane_point);
  if (plane_position === null) return null;
  return { position: plane_position, is_on_surface: false };
}

// With `start_vertex`, the stroke starts at that vertex wherever the pen lands
// (the line button pressed with one vertex selected).
export function line_pen_down(
  tablet_document: TabletDocument, camera: OrbitCamera, screen: V2, canvas: HTMLCanvasElement,
  start_vertex: VertexId | null, surface_layers: ReadonlySet<Layer> | null,
): LineToolState | null {
  let start_world: V3;
  let start_is_on_surface = false;
  if (start_vertex !== null) {
    start_world = vertex_position(tablet_document, start_vertex);
  } else {
    const start = pen_point_on_surface_or_camera_plane(tablet_document, camera, screen, canvas, surface_layers, camera.pivot);
    if (start === null) return null;
    start_world = start.position;
    start_is_on_surface = start.is_on_surface;
  }
  const end = pen_point_on_surface_or_camera_plane(tablet_document, camera, screen, canvas, surface_layers, start_world);
  if (end === null) return null;
  return {
    start_world,
    start_vertex,
    end_world: end.position,
    surface_layers,
    start_is_on_surface,
    end_is_on_surface: end.is_on_surface,
  };
}

export function line_pen_move(
  state: LineToolState, tablet_document: TabletDocument, camera: OrbitCamera, screen: V2, canvas: HTMLCanvasElement,
): void {
  const end = pen_point_on_surface_or_camera_plane(tablet_document, camera, screen, canvas, state.surface_layers, state.start_world);
  if (end === null) return;
  state.end_world = end.position;
  state.end_is_on_surface = end.is_on_surface;
}

// Commit the drag as a straight stroke, creating a vertex for each end the pen placed.
export function line_pen_up(state: LineToolState, tablet_document: TabletDocument, layer: Layer, bone_id: BoneId): StrokeId {
  const p0_vertex = state.start_vertex !== null ? state.start_vertex : add_vertex(tablet_document, state.start_world, bone_id);
  const p3_vertex = add_vertex(tablet_document, state.end_world, bone_id);
  const stroke_id = add_straight_stroke(tablet_document, p0_vertex, p3_vertex, layer);
  if (state.start_is_on_surface && state.end_is_on_surface) stroke_by_id(tablet_document, stroke_id).on_surface = true;
  return stroke_id;
}
