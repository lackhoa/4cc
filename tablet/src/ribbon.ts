// Stroke tessellation, matching the desktop renderer (draw_bezier_inner in
// game/framework_draw.cpp): each sample is offset perpendicular to the curve
// tangent IN CAMERA SPACE (billboard), so a stroke never vanishes edge-on.
// Radius is world-space (thins with distance) and tapered toward the ends like
// the desktop's default radii (.25, 1, 1, .25). Mesh depends on the camera —
// rebuild it whenever the camera moves.

import { OrbitCamera, camera_basis } from "./camera";
import { Stroke, TabletDocument, bezier_point, bezier_tangent, stroke_control_points } from "./document";
import { v3_add, v3_dot, v3_scale, V3 } from "./math";

const RIBBON_RADIUS = 0.02; // world units; grid cell = 1
const RIBBON_SAMPLES = 24;
// Desktop taper: cubic-bezier-interpolated radii multipliers along the stroke.
const TAPER = [0.25, 1, 1, 0.25];

export type Rgb = { r: number; g: number; b: number };

function taper_at(t: number): number {
  const s = 1 - t;
  return s * s * s * TAPER[0] + 3 * s * s * t * TAPER[1] + 3 * s * t * t * TAPER[2] + t * t * t * TAPER[3];
}

// Appends interleaved [x,y,z, r,g,b] triangle vertices to out.
export function append_stroke_ribbon(
  stroke: Stroke, tablet_document: TabletDocument, camera: OrbitCamera, color: Rgb, out: number[],
): void {
  const basis = camera_basis(camera);
  const points = stroke_control_points(stroke, tablet_document);
  const centers: V3[] = [];
  const offsets: V3[] = [];
  for (let i = 0; i <= RIBBON_SAMPLES; i++) {
    const t = i / RIBBON_SAMPLES;
    centers.push(bezier_point(points, t));
    const tangent = bezier_tangent(points, t);
    // Project onto the camera's screen plane, take the 2D perpendicular there.
    const screen_x = v3_dot(tangent, basis.right);
    const screen_y = v3_dot(tangent, basis.up);
    const len = Math.hypot(screen_x, screen_y);
    const radius = RIBBON_RADIUS * taper_at(t);
    if (len < 1e-9) {
      // Tangent points straight at the camera; fall back to screen-right.
      offsets.push(v3_scale(basis.right, radius));
    } else {
      const perp_x = -screen_y / len;
      const perp_y = screen_x / len;
      offsets.push(v3_add(v3_scale(basis.right, perp_x * radius), v3_scale(basis.up, perp_y * radius)));
    }
  }
  for (let i = 0; i + 1 < centers.length; i++) {
    const a0 = v3_add(centers[i], offsets[i]);
    const b0 = v3_add(centers[i], v3_scale(offsets[i], -1));
    const a1 = v3_add(centers[i + 1], offsets[i + 1]);
    const b1 = v3_add(centers[i + 1], v3_scale(offsets[i + 1], -1));
    push_vertex(out, a0, color);
    push_vertex(out, b0, color);
    push_vertex(out, a1, color);
    push_vertex(out, a1, color);
    push_vertex(out, b0, color);
    push_vertex(out, b1, color);
  }
}

function push_vertex(out: number[], position: V3, color: Rgb): void {
  out.push(position.x, position.y, position.z, color.r, color.g, color.b);
}
