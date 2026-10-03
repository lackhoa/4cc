// Line tool (Q27): an armed tool — while armed, a pen drag places one cubic
// stroke (down = p0, up = p3), always the straight segment between the two; it
// is shown live while the pen is down. An endpoint with the pen on an existing
// vertex (on screen, at any depth) reuses that vertex, so strokes join at
// shared vertices. A start on empty space lies on the camera-facing plane
// through the pivot; an end on empty space lies on the camera-facing plane
// through the start point. A tap (no drag) exits the tool.

import { OrbitCamera, camera_basis, camera_pen_ray } from "./camera";
import { BoneId, Layer, StrokeId, TabletDocument, VertexId, add_straight_stroke, add_vertex, vertex_position } from "./document";
import { pick_vertex } from "./edit_mode";
import { V2, V3, v3_add, v3_dot, v3_scale, v3_sub } from "./math";

// Both endpoints while the pen is down: the world position, plus the existing
// vertex id it snapped to (null = a new vertex will be created on pen-up).
export type LineToolState = {
  start_world: V3;
  start_snap_vertex: VertexId | null;
  end_world: V3;
  end_snap_vertex: VertexId | null;
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

// An endpoint under the pen: the vertex the pen is on (on screen, at any
// depth), else the pen's point on the camera-facing plane through `plane_point`.
// `layers`: endpoints snap to vertices of these layers only (the sketchpad
// passes the layers neither locked nor hidden).
function resolve_endpoint(
  tablet_document: TabletDocument, camera: OrbitCamera, screen: V2, canvas: HTMLCanvasElement, layers: ReadonlySet<Layer>,
  plane_point: V3,
): { world: V3; snap_vertex: VertexId | null } | null {
  const snap_vertex = pick_vertex(tablet_document, camera, screen, canvas, layers);
  if (snap_vertex !== null) {
    return { world: vertex_position(tablet_document, snap_vertex), snap_vertex };
  }
  const world = pen_point_on_camera_plane(camera, screen, canvas, plane_point);
  if (world === null) return null;
  return { world, snap_vertex: null };
}

// With `start_vertex`, the stroke starts at that vertex wherever the pen lands
// (the line button pressed with one vertex selected).
export function line_pen_down(
  tablet_document: TabletDocument, camera: OrbitCamera, screen: V2, canvas: HTMLCanvasElement,
  layers: ReadonlySet<Layer>, start_vertex: VertexId | null = null,
): LineToolState | null {
  if (start_vertex !== null) {
    const start_world = vertex_position(tablet_document, start_vertex);
    const endpoint = resolve_endpoint(tablet_document, camera, screen, canvas, layers, start_world);
    if (endpoint === null) return null;
    return {
      start_world,
      start_snap_vertex: start_vertex,
      end_world: endpoint.world,
      end_snap_vertex: endpoint.snap_vertex,
    };
  }
  const endpoint = resolve_endpoint(tablet_document, camera, screen, canvas, layers, camera.pivot);
  if (endpoint === null) return null;
  return {
    start_world: endpoint.world,
    start_snap_vertex: endpoint.snap_vertex,
    end_world: endpoint.world,
    end_snap_vertex: endpoint.snap_vertex,
  };
}

export function line_pen_move(
  state: LineToolState, tablet_document: TabletDocument, camera: OrbitCamera,
  screen: V2, canvas: HTMLCanvasElement, layers: ReadonlySet<Layer>,
): void {
  const endpoint = resolve_endpoint(tablet_document, camera, screen, canvas, layers, state.start_world);
  if (endpoint === null) return;
  state.end_world = endpoint.world;
  state.end_snap_vertex = endpoint.snap_vertex;
}

// Commit the drag as a straight stroke, creating vertices for
// unsnapped endpoints. Returns the new stroke's id, or null when the two
// endpoints collapsed to the same vertex.
export function line_pen_up(state: LineToolState, tablet_document: TabletDocument, layer: Layer, bone_id: BoneId): StrokeId | null {
  if (state.start_snap_vertex !== null && state.start_snap_vertex === state.end_snap_vertex) return null;
  const claim_vertex = (world: V3, snap_vertex: VertexId | null): VertexId =>
    snap_vertex !== null ? snap_vertex : add_vertex(tablet_document, world, bone_id);
  const p0_vertex = claim_vertex(state.start_world, state.start_snap_vertex);
  const p3_vertex = claim_vertex(state.end_world, state.end_snap_vertex);
  return add_straight_stroke(tablet_document, p0_vertex, p3_vertex, layer);
}
