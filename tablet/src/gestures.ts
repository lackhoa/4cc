// Finger gestures → camera (Q9: stylus draws, finger navigates).
// 1-finger drag = orbit, 2-finger drag = pan, pinch = zoom. A multi-finger
// *tap* (all fingers barely move and lift quickly) is undo (2 fingers) / redo
// (3 fingers), Procreate-style. Pen (and mouse) events are forwarded to the caller
// (drawing modes handle them in later steps).

import { OrbitCamera, camera_orbit, camera_pan, camera_world_units_per_pixel, camera_zoom } from "./camera";
import { V2 } from "./math";

export const ORBIT_RADIANS_PER_PIXEL = 0.006; // shared with pen-drag orbiting in main.ts

// Tap thresholds (Q7, tune by feel): every finger stays within this
// displacement of where it landed, and the last finger lifts within this time
// of the first touch. The finger count is the gesture's PEAK concurrent
// touches, read at release, so a late third finger still counts as 3.
const FINGER_TAP_MAX_MOVEMENT_PIXELS = 12;
const FINGER_TAP_MAX_DURATION_MS = 250;

export type PenHandlers = {
  on_pen_down: (position: V2, event: PointerEvent) => void;
  on_pen_move: (position: V2, event: PointerEvent) => void;
  on_pen_up: (position: V2, event: PointerEvent) => void;
  on_undo_tap: () => void; // two-finger tap
  on_redo_tap: () => void; // three-finger tap
};

// A mouse acts as the pen so the prototype is testable on a desktop (drags on
// empty space still orbit, via the caller's pen handlers). Fingers navigate.
function is_pen_pointer(e: PointerEvent): boolean {
  return e.pointerType === "pen" || e.pointerType === "mouse";
}

export function attach_gestures(
  canvas: HTMLCanvasElement,
  camera: OrbitCamera,
  pen: PenHandlers,
  on_camera_change: () => void,
): void {
  const touch_positions = new Map<number, V2>(); // pointerId -> last position, CSS pixels
  // Tap tracking for the whole touch gesture (first finger down -> last up).
  const touch_down_positions = new Map<number, V2>(); // pointerId -> where the finger landed
  let touch_gesture_start_ms = 0;
  let touch_gesture_peak_fingers = 0;
  let touch_gesture_is_tap = true; // falsified as soon as any finger moves too far

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
    if (is_pen_pointer(e)) {
      pen.on_pen_down(position, e);
    } else {
      if (touch_positions.size === 0) {
        touch_gesture_start_ms = performance.now();
        touch_gesture_peak_fingers = 0;
        touch_gesture_is_tap = true;
        touch_down_positions.clear();
      }
      touch_positions.set(e.pointerId, position);
      touch_down_positions.set(e.pointerId, position);
      touch_gesture_peak_fingers = Math.max(touch_gesture_peak_fingers, touch_positions.size);
    }
  });

  canvas.addEventListener("pointermove", (e) => {
    e.preventDefault();
    const position = { x: e.clientX, y: e.clientY };
    if (is_pen_pointer(e)) {
      if (e.buttons !== 0) pen.on_pen_move(position, e);
      return;
    }
    if (!touch_positions.has(e.pointerId)) return;
    const down = touch_down_positions.get(e.pointerId)!;
    if (Math.hypot(position.x - down.x, position.y - down.y) > FINGER_TAP_MAX_MOVEMENT_PIXELS) {
      touch_gesture_is_tap = false;
    }

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
      if (is_pen_pointer(e)) {
        pen.on_pen_up(position, e);
      } else {
        touch_positions.delete(e.pointerId);
        if (touch_positions.size === 0 && touch_gesture_is_tap && type === "pointerup") {
          const duration = performance.now() - touch_gesture_start_ms;
          if (duration <= FINGER_TAP_MAX_DURATION_MS) {
            if (touch_gesture_peak_fingers === 2) pen.on_undo_tap();
            if (touch_gesture_peak_fingers === 3) pen.on_redo_tap();
          }
        }
      }
    });
  }
}
