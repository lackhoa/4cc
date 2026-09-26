// 2D-canvas view of a scene through an OrbitCamera, for the explainer pages: sizes the
// backing store to devicePixelRatio, clears it, and projects world points to device
// pixels. Lines and labels only; meshes go through the WebGL renderer (render.ts).

import { OrbitCamera, camera_eye, camera_world_to_screen } from "../camera";
import { V3 } from "../math";

export type CanvasView = {
  ctx: CanvasRenderingContext2D;
  width: number; // device pixels
  height: number;
  dpr: number;
  eye: V3;
  project: (world: V3) => { x: number; y: number } | null; // device pixels, null behind the eye
};

export function canvas_view(canvas: HTMLCanvasElement, camera: OrbitCamera): CanvasView {
  const dpr = window.devicePixelRatio || 1;
  // Setting canvas.height rewrites the height attribute, so remember the CSS height once.
  if (!canvas.dataset.cssHeight) canvas.dataset.cssHeight = canvas.getAttribute("height") ?? "300";
  const css_height = Number(canvas.dataset.cssHeight);
  const css_width = canvas.clientWidth;
  canvas.width = Math.round(css_width * dpr);
  canvas.height = Math.round(css_height * dpr);
  canvas.style.height = `${css_height}px`;
  const ctx = canvas.getContext("2d")!;
  ctx.setTransform(1, 0, 0, 1, 0, 0);
  ctx.clearRect(0, 0, canvas.width, canvas.height);
  return {
    ctx,
    width: canvas.width,
    height: canvas.height,
    dpr,
    eye: camera_eye(camera),
    project: (world) => {
      const screen = camera_world_to_screen(camera, world, css_width, css_height);
      return screen === null ? null : { x: screen.x * dpr, y: screen.y * dpr };
    },
  };
}

export function stroke_polyline(view: CanvasView, points: V3[], closed: boolean, style: string, line_width: number): void {
  const ctx = view.ctx;
  ctx.strokeStyle = style;
  ctx.lineWidth = line_width * view.dpr;
  ctx.lineJoin = "round";
  ctx.lineCap = "round";
  ctx.beginPath();
  let pen_down = false;
  const all = closed ? [...points, points[0]] : points;
  for (const point of all) {
    const p = view.project(point);
    if (p === null) { pen_down = false; continue; }
    if (pen_down) ctx.lineTo(p.x, p.y); else ctx.moveTo(p.x, p.y);
    pen_down = true;
  }
  ctx.stroke();
}
