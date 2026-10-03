// The mirror: a small second view of the same scene, seen from the right side of
// the main view, so depth in the main view reads as left-right in the mirror.
// Look only: the pen does nothing inside it.

import { OrbitCamera, camera_distance_for_view_half_height, camera_view_half_height } from "./camera";
import { V3, v3_add, v3_length, v3_scale, v3_sub } from "./math";

// What the mirror zooms in on: a ball around the thing being selected or adjusted.
export type MirrorFocus = { center: V3; radius: number };

// The ball around `points` (centre of their bounding box); null for no points.
export function mirror_focus_from_points(points: V3[]): MirrorFocus | null {
  if (points.length === 0) return null;
  let low = points[0];
  let high = points[0];
  for (const point of points) {
    low = { x: Math.min(low.x, point.x), y: Math.min(low.y, point.y), z: Math.min(low.z, point.z) };
    high = { x: Math.max(high.x, point.x), y: Math.max(high.y, point.y), z: Math.max(high.z, point.z) };
  }
  const center = v3_scale(v3_add(low, high), 0.5);
  return { center, radius: v3_length(v3_sub(high, center)) };
}

// The main camera turned a quarter turn to its right. Level (pitch 0), so the
// drawing stays upright whatever the pitch of the main view; the pen ray drawn
// in the mirror then shows that pitch as a slope.
// Without a focus it turns around the main pivot at the main zoom. With one it
// looks at the focus centre and zooms until the focus ball, plus some room
// around it, fills the mirror: never wider than the main view, and never
// closer than a sixth of it (a single vertex has radius 0).
export function mirror_camera_from_main_camera(camera: OrbitCamera, focus: MirrorFocus | null): OrbitCamera {
  const turned = { ...camera, yaw: camera.yaw + Math.PI / 2, pitch: 0 };
  if (focus === null) return turned;
  const main_half_height = camera_view_half_height(camera);
  const half_height = Math.max(main_half_height / 6, Math.min(main_half_height, focus.radius * 1.6));
  return { ...turned, pivot: focus.center, distance: camera_distance_for_view_half_height(half_height) };
}

// Where the mirror sits in the canvas: a square, in CSS pixels from the top-left.
export type MirrorRectangle = { left: number; top: number; size: number };

// Bottom-right corner, a third of the canvas height.
export function mirror_rectangle(canvas_width: number, canvas_height: number): MirrorRectangle {
  const margin = 12;
  const size = Math.round(canvas_height / 3);
  return { left: canvas_width - size - margin, top: canvas_height - size - margin, size };
}
