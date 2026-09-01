// Finger gestures → camera (Q9: stylus draws, finger navigates).
// 1-finger drag = orbit, 2-finger drag = pan, pinch = zoom. Pen events are
// forwarded to the caller (drawing modes handle them in later steps).

import { OrbitCamera, camera_orbit, camera_pan, camera_world_units_per_pixel, camera_zoom } from "./camera";
import { V2 } from "./math";

export const ORBIT_RADIANS_PER_PIXEL = 0.006; // shared with pen-drag orbiting in main.ts

export type PenHandlers = {
  on_pen_down: (position: V2, event: PointerEvent) => void;
  on_pen_move: (position: V2, event: PointerEvent) => void;
  on_pen_up: (position: V2, event: PointerEvent) => void;
};

export function attach_gestures(
  canvas: HTMLCanvasElement,
  camera: OrbitCamera,
  pen: PenHandlers,
  on_camera_change: () => void,
): void {
  const touch_positions = new Map<number, V2>(); // pointerId -> last position, CSS pixels

  function touch_centroid_and_spread(): { centroid: V2; spread: number } {
    const points = [...touch_positions.values()];
    const centroid = {
      x: points.reduce((sum, p) => sum + p.x, 0) / points.length,
      y: points.reduce((sum, p) => sum + p.y, 0) / points.length,
    };
    const spread = points.reduce((sum, p) => sum + Math.hypot(p.x - centroid.x, p.y - centroid.y), 0) / points.length;
    return { centroid, spread };
  }

  canvas.addEventListener("pointerdown", (e) => {
    e.preventDefault();
    // NOTE: throws NotFoundError for synthetic events (no active pointer) —
    // guard so dispatched test strokes still reach the handlers below.
    try { canvas.setPointerCapture(e.pointerId); } catch {}
    const position = { x: e.clientX, y: e.clientY };
    if (e.pointerType === "pen") {
      pen.on_pen_down(position, e);
    } else {
      touch_positions.set(e.pointerId, position);
    }
  });

  canvas.addEventListener("pointermove", (e) => {
    e.preventDefault();
    const position = { x: e.clientX, y: e.clientY };
    if (e.pointerType === "pen") {
      if (e.buttons !== 0) pen.on_pen_move(position, e);
      return;
    }
    if (!touch_positions.has(e.pointerId)) return;

    if (touch_positions.size === 1) {
      const previous = touch_positions.get(e.pointerId)!;
      camera_orbit(
        camera,
        -(position.x - previous.x) * ORBIT_RADIANS_PER_PIXEL,
        (position.y - previous.y) * ORBIT_RADIANS_PER_PIXEL,
      );
      touch_positions.set(e.pointerId, position);
      on_camera_change();
    } else {
      const before = touch_centroid_and_spread();
      touch_positions.set(e.pointerId, position);
      const after = touch_centroid_and_spread();
      const units_per_pixel = camera_world_units_per_pixel(camera, canvas.clientHeight);
      camera_pan(
        camera,
        -(after.centroid.x - before.centroid.x) * units_per_pixel,
        (after.centroid.y - before.centroid.y) * units_per_pixel,
      );
      if (before.spread > 1 && after.spread > 1) {
        camera_zoom(camera, before.spread / after.spread);
      }
      on_camera_change();
    }
  });

  for (const type of ["pointerup", "pointercancel"] as const) {
    canvas.addEventListener(type, (e) => {
      e.preventDefault();
      const position = { x: e.clientX, y: e.clientY };
      if (e.pointerType === "pen") {
        pen.on_pen_up(position, e);
      } else {
        touch_positions.delete(e.pointerId);
      }
    });
  }
}
