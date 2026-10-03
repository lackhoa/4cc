// The mirror: a small second view of the same scene with a camera of its own,
// so depth in the main view can be read from another side. The user steers it
// with drags inside it; nothing moves it automatically. The pen edits nothing
// inside it.

import { OrbitCamera, camera_orbit, camera_pan, camera_world_units_per_pixel, camera_zoom } from "./camera";
import { ORBIT_RADIANS_PER_PIXEL } from "./gestures";
import { V2 } from "./math";

// The main camera turned a quarter turn to its right around the pivot, same
// zoom, level (pitch 0): the mirror's first view, and the view a double tap
// inside the mirror returns to.
export function mirror_camera_from_main_camera(camera: OrbitCamera): OrbitCamera {
  return { pivot: { ...camera.pivot }, yaw: camera.yaw + Math.PI / 2, pitch: 0, distance: camera.distance };
}

// Where the mirror sits in the canvas: a square, in CSS pixels from the top-left.
export type MirrorRectangle = { left: number; top: number; size: number };

// Bottom-right corner, a third of the canvas height.
export function mirror_rectangle(canvas_width: number, canvas_height: number): MirrorRectangle {
  const margin = 12;
  const size = Math.round(canvas_height / 3);
  return { left: canvas_width - size - margin, top: canvas_height - size - margin, size };
}

// Drags inside the mirror's frame steer the mirror's camera, with the main
// view's gestures: one pointer (pen, mouse or finger) orbits, two fingers pan
// and pinch-zoom, alt+drag or a middle-button drag pans, the wheel zooms, a
// double tap calls `on_reset`. `mirror_camera` returns null while the mirror
// has no camera yet.
export function attach_mirror_gestures(
  frame: HTMLElement, mirror_camera: () => OrbitCamera | null, on_camera_change: () => void, on_reset: () => void,
): void {
  const pointer_positions = new Map<number, V2>(); // pointerId -> last position, CSS pixels
  let panning_pointer: number | null = null; // the alt / middle-button drag

  function centroid_and_spread(): { centroid: V2; spread: number } {
    const points = [...pointer_positions.values()];
    const centroid = {
      x: points.reduce((sum, p) => sum + p.x, 0) / points.length,
      y: points.reduce((sum, p) => sum + p.y, 0) / points.length,
    };
    const spread = points.reduce((sum, p) => sum + Math.hypot(p.x - centroid.x, p.y - centroid.y), 0) / points.length;
    return { centroid, spread };
  }

  function pan_by_pixels(camera: OrbitCamera, delta_x: number, delta_y: number): void {
    const units_per_pixel = camera_world_units_per_pixel(camera, frame.clientHeight);
    camera_pan(camera, -delta_x * units_per_pixel, delta_y * units_per_pixel);
  }

  frame.addEventListener("pointerdown", (e) => {
    e.preventDefault();
    try { frame.setPointerCapture(e.pointerId); } catch {} // throws for synthetic events
    pointer_positions.set(e.pointerId, { x: e.clientX, y: e.clientY });
    if (e.altKey || e.button === 1) panning_pointer = e.pointerId;
  });

  frame.addEventListener("pointermove", (e) => {
    e.preventDefault();
    const camera = mirror_camera();
    const previous = pointer_positions.get(e.pointerId);
    if (camera === null || previous === undefined) return;
    const position = { x: e.clientX, y: e.clientY };
    if (pointer_positions.size === 1) {
      if (panning_pointer === e.pointerId) {
        pan_by_pixels(camera, position.x - previous.x, position.y - previous.y);
      } else {
        camera_orbit(
          camera,
          -(position.x - previous.x) * ORBIT_RADIANS_PER_PIXEL,
          (position.y - previous.y) * ORBIT_RADIANS_PER_PIXEL,
        );
      }
      pointer_positions.set(e.pointerId, position);
    } else {
      const before = centroid_and_spread();
      pointer_positions.set(e.pointerId, position);
      const after = centroid_and_spread();
      pan_by_pixels(camera, after.centroid.x - before.centroid.x, after.centroid.y - before.centroid.y);
      if (before.spread > 1 && after.spread > 1) camera_zoom(camera, before.spread / after.spread);
    }
    on_camera_change();
  });

  for (const type of ["pointerup", "pointercancel"] as const) {
    frame.addEventListener(type, (e) => {
      e.preventDefault();
      pointer_positions.delete(e.pointerId);
      if (panning_pointer === e.pointerId) panning_pointer = null;
    });
  }

  frame.addEventListener("wheel", (e) => {
    e.preventDefault();
    const camera = mirror_camera();
    if (camera === null) return;
    camera_zoom(camera, Math.exp(e.deltaY * 0.0015));
    on_camera_change();
  }, { passive: false });

  frame.addEventListener("dblclick", (e) => {
    e.preventDefault();
    on_reset();
  });
}
