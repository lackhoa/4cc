// Line tool (Q27): an armed tool — while armed, a pen drag places a straight
// stroke on the camera-facing plane through the pivot (down = p0, up = p3,
// live straight preview). Endpoints snap to existing vertices within a screen
// radius, reusing them so strokes join at shared vertices. A tap (no drag)
// exits the tool.

import { OrbitCamera, camera_basis, camera_screen_ray, camera_world_to_screen } from "./camera";
import { TabletDocument } from "./document";
import { V2, V3, v3, v3_add, v3_dot, v3_scale, v3_sub } from "./math";

const VERTEX_SNAP_RADIUS_PIXELS = 20;

// Both endpoints while the pen is down: the world position, plus the existing
// vertex index it snapped to (null = a new vertex will be created on pen-up).
export type LineToolState = {
  start_world: V3;
  start_snap_vertex: number | null;
  end_world: V3;
  end_snap_vertex: number | null;
};

// Ray-cast a screen point onto the camera-facing plane through the pivot.
export function pen_point_on_camera_plane(
  camera: OrbitCamera, screen: V2, canvas: HTMLCanvasElement,
): V3 | null {
  const ray = camera_screen_ray(camera, screen, canvas.clientWidth, canvas.clientHeight);
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
  return {
    start_world: endpoint.world,
    start_snap_vertex: endpoint.snap_vertex,
    end_world: endpoint.world,
    end_snap_vertex: endpoint.snap_vertex,
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
}

// Commit the line as a straight stroke (d0 = 0, d3 = (0, 0)), creating
// vertices for unsnapped endpoints. Returns the new stroke's index, or null
// when the two endpoints collapsed to the same vertex.
export function line_pen_up(state: LineToolState, tablet_document: TabletDocument): number | null {
  if (state.start_snap_vertex !== null && state.start_snap_vertex === state.end_snap_vertex) return null;
  const claim_vertex = (world: V3, snap_vertex: number | null): number => {
    if (snap_vertex !== null) return snap_vertex;
    tablet_document.vertices.push(world);
    return tablet_document.vertices.length - 1;
  };
  const p0_vertex = claim_vertex(state.start_world, state.start_snap_vertex);
  const p3_vertex = claim_vertex(state.end_world, state.end_snap_vertex);
  tablet_document.strokes.push({ p0_vertex, d0: v3(0, 0, 0), d3: { x: 0, y: 0 }, p3_vertex });
  return tablet_document.strokes.length - 1;
}
