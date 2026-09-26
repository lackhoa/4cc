// Pointer/wheel orbit for the explainer pages (pages/*): every canvas in the list
// drives the same OrbitCamera, so side-by-side views stay comparable. Ported from
// docs/explainer/explainer.js `attach_orbit`.

import { OrbitCamera, camera_orbit, camera_zoom } from "../camera";

export function attach_orbit_controls(canvases: HTMLCanvasElement[], camera: OrbitCamera, on_change: () => void): void {
  for (const canvas of canvases) {
    let last: { x: number; y: number } | null = null;
    canvas.addEventListener("pointerdown", (event) => {
      last = { x: event.clientX, y: event.clientY };
      canvas.setPointerCapture(event.pointerId);
    });
    canvas.addEventListener("pointermove", (event) => {
      if (last === null) return;
      camera_orbit(camera, -(event.clientX - last.x) * 0.008, (event.clientY - last.y) * 0.008);
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
