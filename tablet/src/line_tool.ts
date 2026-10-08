// Line tool (Q27): an armed tool — while armed, a pen drag places one cubic
// stroke (down = p0, up = p3), always the straight segment between the two; it
// is shown live while the pen is down. Nothing snaps: each end the pen places
// is a new vertex (join it to another afterwards by dragging it onto that one).
// A start lies on the camera-facing plane through the pivot; the end lies on
// the camera-facing plane through the start point. A tap (no drag) exits the tool.
//
// "On surface" (plan-hairline-drawn-on-surface Q8): the ends land where the pen
// hits the surface of the stroke's layer instead, or on an existing vertex
// under the pen (snap). A new end vertex gets a surface pin at the hit (Q5), and
// the stroke is flagged on_surface, so it is drawn projected onto the surface.

import { OrbitCamera, camera_basis, camera_pen_ray } from "./camera";
import { BoneId, Layer, PatchId, StrokeId, TabletDocument, VertexId, add_straight_stroke, add_vertex, stroke_by_id, vertex_position } from "./document";
import { pick_vertex } from "./edit_mode";
import { V2, V3, v3_add, v3_dot, v3_scale, v3_sub } from "./math";
import { patch_by_id, pick_surface_point, pin_vertex_to_surface } from "./patch";

// Where an on-surface end sits on the surface.
type SurfaceHit = { patch: PatchId; u: number; v: number };

// Both endpoints while the pen is down. `start_vertex` / `end_vertex`: the
// existing vertex that end is (null = a new vertex will be created on pen-up).
// `start_hit` / `end_hit`: the surface point of a new on-surface end.
export type LineToolState = {
  start_world: V3;
  start_vertex: VertexId | null;
  end_world: V3;
  surface: LineToolSurface | null;
  start_hit: SurfaceHit | null;
  end_vertex: VertexId | null;
  end_hit: SurfaceHit | null;
};

// On-surface mode: the patches of `layer` are the surface; vertices on
// `snap_layers` are snap targets.
export type LineToolSurface = { layer: Layer; snap_layers: ReadonlySet<Layer> };

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

// An on-surface end under the pen: an existing vertex within pick range, else
// the surface point, else null (the pen is off the surface).
function surface_end_under_pen(
  tablet_document: TabletDocument, camera: OrbitCamera, screen: V2, canvas: HTMLCanvasElement, surface: LineToolSurface,
): { world: V3; vertex: VertexId | null; hit: SurfaceHit | null } | null {
  const vertex = pick_vertex(tablet_document, camera, screen, canvas, surface.snap_layers);
  if (vertex !== null) return { world: vertex_position(tablet_document, vertex), vertex, hit: null };
  const hit = pick_surface_point(tablet_document, camera, screen, canvas, new Set([surface.layer]));
  if (hit === null) return null;
  return { world: hit.position, vertex: null, hit: { patch: hit.patch.id, u: hit.u, v: hit.v } };
}

// With `start_vertex`, the stroke starts at that vertex wherever the pen lands
// (the line button pressed with one vertex selected). With `surface`, a pen
// landing off the surface (and on no vertex) starts nothing.
export function line_pen_down(
  tablet_document: TabletDocument, camera: OrbitCamera, screen: V2, canvas: HTMLCanvasElement,
  start_vertex: VertexId | null = null, surface: LineToolSurface | null = null,
): LineToolState | null {
  if (surface !== null) {
    const start = start_vertex !== null
      ? { world: vertex_position(tablet_document, start_vertex), vertex: start_vertex, hit: null }
      : surface_end_under_pen(tablet_document, camera, screen, canvas, surface);
    if (start === null) return null;
    return {
      start_world: start.world, start_vertex: start.vertex, start_hit: start.hit,
      end_world: start.world, end_vertex: start.vertex, end_hit: start.hit, surface,
    };
  }
  const start_world = start_vertex !== null
    ? vertex_position(tablet_document, start_vertex)
    : pen_point_on_camera_plane(camera, screen, canvas, camera.pivot);
  if (start_world === null) return null;
  const end_world = pen_point_on_camera_plane(camera, screen, canvas, start_world);
  if (end_world === null) return null;
  return { start_world, start_vertex, end_world, surface: null, start_hit: null, end_vertex: null, end_hit: null };
}

// On surface, a pen off the surface leaves the end where it last was.
export function line_pen_move(
  state: LineToolState, tablet_document: TabletDocument, camera: OrbitCamera, screen: V2, canvas: HTMLCanvasElement,
): void {
  if (state.surface !== null) {
    const end = surface_end_under_pen(tablet_document, camera, screen, canvas, state.surface);
    if (end === null) return;
    state.end_world = end.world;
    state.end_vertex = end.vertex;
    state.end_hit = end.hit;
    return;
  }
  const end_world = pen_point_on_camera_plane(camera, screen, canvas, state.start_world);
  if (end_world !== null) state.end_world = end_world;
}

// A new on-surface end vertex, pinned where the pen hit.
function add_surface_vertex(tablet_document: TabletDocument, world: V3, hit: SurfaceHit, bone_id: BoneId): VertexId {
  const vertex = add_vertex(tablet_document, world, bone_id);
  pin_vertex_to_surface(tablet_document, vertex, patch_by_id(tablet_document, hit.patch)!, hit.u, hit.v);
  return vertex;
}

// Commit the drag as a straight stroke, creating a vertex for each end the pen
// placed. Null when both ends are the same vertex (nothing to draw).
export function line_pen_up(state: LineToolState, tablet_document: TabletDocument, layer: Layer, bone_id: BoneId): StrokeId | null {
  if (state.surface === null) {
    const p0_vertex = state.start_vertex !== null ? state.start_vertex : add_vertex(tablet_document, state.start_world, bone_id);
    const p3_vertex = add_vertex(tablet_document, state.end_world, bone_id);
    return add_straight_stroke(tablet_document, p0_vertex, p3_vertex, layer);
  }
  if (state.start_vertex !== null && state.start_vertex === state.end_vertex) return null;
  const p0_vertex = state.start_vertex ?? add_surface_vertex(tablet_document, state.start_world, state.start_hit!, bone_id);
  const p3_vertex = state.end_vertex ?? add_surface_vertex(tablet_document, state.end_world, state.end_hit!, bone_id);
  const stroke_id = add_straight_stroke(tablet_document, p0_vertex, p3_vertex, layer);
  stroke_by_id(tablet_document, stroke_id).on_surface = true;
  return stroke_id;
}
