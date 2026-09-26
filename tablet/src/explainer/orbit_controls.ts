// Pointer/wheel orbit for the explainer pages (pages/*): every canvas in the list
// drives the same OrbitCamera, so side-by-side views stay comparable. Ported from
// docs/explainer/explainer.js `attach_orbit`. Left drag = orbit, middle drag = pan
// (the pivot moves 1:1 with the pointer at its own depth), wheel = zoom.

import { OrbitCamera, camera_orbit, camera_pan, camera_world_units_per_pixel, camera_zoom } from "../camera";

export function attach_orbit_controls(canvases: HTMLCanvasElement[], camera: OrbitCamera, on_change: () => void): void {
  for (const canvas of canvases) {
    let last: { x: number; y: number } | null = null;
    let is_panning = false;
    canvas.addEventListener("pointerdown", (event) => {
      last = { x: event.clientX, y: event.clientY };
      is_panning = event.button === 1;
      if (is_panning) event.preventDefault(); // no browser autoscroll on middle button
      canvas.setPointerCapture(event.pointerId);
    });
    canvas.addEventListener("pointermove", (event) => {
      if (last === null) return;
      const delta_x = event.clientX - last.x;
      const delta_y = event.clientY - last.y;
      if (is_panning) {
        const units_per_pixel = camera_world_units_per_pixel(camera, canvas.clientHeight);
        camera_pan(camera, -delta_x * units_per_pixel, delta_y * units_per_pixel);
      } else {
        camera_orbit(camera, -delta_x * 0.008, delta_y * 0.008);
      }
      last = { x: event.clientX, y: event.clientY };
      on_change();
    });
    canvas.addEventListener("pointerup", () => { last = null; });
    canvas.addEventListener("wheel", (event) => {
      event.preventDefault();
      camera_zoom(camera, Math.exp(event.deltaY * 0.001));
      on_change();
    }, { passive: false });
  }
  window.addEventListener("resize", on_change);
}

// Every "input" event on the listed element ids calls `on_change`. Returns the elements by id.
export function bind_controls(ids: string[], on_change: () => void): Record<string, HTMLInputElement> {
  const controls: Record<string, HTMLInputElement> = {};
  for (const id of ids) {
    const element = document.getElementById(id) as HTMLInputElement;
    element.addEventListener("input", on_change);
    controls[id] = element;
  }
  return controls;
}
