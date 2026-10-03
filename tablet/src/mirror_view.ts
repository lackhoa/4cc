// The mirror: a small second view of the same scene, seen from the right side of
// the main view, so depth in the main view reads as left-right in the mirror.
// Look only: the pen does nothing inside it.

import { OrbitCamera } from "./camera";

// The main camera turned a quarter turn to its right around the pivot, same
// zoom. Level (pitch 0), so the drawing stays upright whatever the pitch of the
// main view; the pen ray drawn in the mirror then shows that pitch as a slope.
export function mirror_camera_from_main_camera(camera: OrbitCamera): OrbitCamera {
  return { ...camera, yaw: camera.yaw + Math.PI / 2, pitch: 0 };
}

// Where the mirror sits in the canvas: a square, in CSS pixels from the top-left.
export type MirrorRectangle = { left: number; top: number; size: number };

// Bottom-right corner, a third of the canvas height.
export function mirror_rectangle(canvas_width: number, canvas_height: number): MirrorRectangle {
  const margin = 12;
  const size = Math.round(canvas_height / 3);
  return { left: canvas_width - size - margin, top: canvas_height - size - margin, size };
}
