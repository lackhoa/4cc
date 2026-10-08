// Stroke tessellation, matching the desktop renderer (draw_bezier_inner in
// game/framework_draw.cpp): each sample is offset perpendicular to the curve
// tangent IN CAMERA SPACE (billboard), so a stroke never vanishes edge-on.
// Radius is world-space (thins with distance): RIBBON_RADIUS times the stroke's
// own width profile (`Stroke.radii`, edited in the sketchpad's width panel;
// absent = DEFAULT_STROKE_RADII, a plain constant-width line). Mesh depends on
// the camera — rebuild it whenever the camera moves.

import { OrbitCamera, camera_basis } from "./camera";
import {
  DEFAULT_STROKE_RADII, Stroke, StrokeControlPoints, StrokeRadii, TabletDocument,
  bezier_point, bezier_tangent, stroke_control_points, stroke_radii, stroke_radii_at,
} from "./document";
import { v3_add, v3_dot, v3_length, v3_scale, v3_sub, V3 } from "./math";
import { on_surface_stroke_samples } from "./patch";
import { Rgb, VertexSink, push_vertex } from "./vertex_sink";

const RIBBON_RADIUS = 0.01; // world units; grid cell = 1
const RIBBON_SAMPLES = 24;

export type { Rgb };

// Appends interleaved [x,y,z, r,g,b] triangle vertices to out.
// An on-surface stroke (plan-hairline-drawn-on-surface Q4) draws its projected
// samples, tangents by finite differences between them.
export function append_stroke_ribbon(
  stroke: Stroke, tablet_document: TabletDocument, camera: OrbitCamera, color: Rgb, out: VertexSink,
): void {
  if (stroke.on_surface === true) {
    const centers = on_surface_stroke_samples(stroke, tablet_document);
    const last = centers.length - 1;
    const tangents = centers.map((_, i) => v3_sub(centers[Math.min(i + 1, last)], centers[Math.max(i - 1, 0)]));
    append_polyline_ribbon(centers, tangents, stroke_radii(stroke), { start: 0, end: 1 }, camera, color, out);
    return;
  }
  append_bezier_ribbon(
    stroke_control_points(stroke, tablet_document), stroke_radii(stroke), { start: 0, end: 1 }, camera, color, out,
  );
}

// Where this cubic sits in the taper of the whole curve it belongs to, as a
// fraction range: a drawn stroke is one cubic covering 0..1; a computed
// contour chain spreads one taper over all its cubics by arc length.
export type TaperWindow = { start: number; end: number };

export function append_bezier_ribbon(
  points: StrokeControlPoints, radii: StrokeRadii, taper_window: TaperWindow, camera: OrbitCamera, color: Rgb, out: VertexSink,
): void {
  const centers: V3[] = [];
  const tangents: V3[] = [];
  for (let i = 0; i <= RIBBON_SAMPLES; i++) {
    const t = i / RIBBON_SAMPLES;
    centers.push(bezier_point(points, t));
    tangents.push(bezier_tangent(points, t));
  }
  append_polyline_ribbon(centers, tangents, radii, taper_window, camera, color, out);
}

// Samples evenly spaced in the curve parameter; the taper runs over them by index.
function append_polyline_ribbon(
  centers: V3[], tangents: V3[], radii: StrokeRadii, taper_window: TaperWindow, camera: OrbitCamera, color: Rgb, out: VertexSink,
): void {
  const basis = camera_basis(camera);
  const offsets: V3[] = [];
  for (let i = 0; i < centers.length; i++) {
    const t = i / (centers.length - 1);
    const tangent = tangents[i];
    // Project onto the camera's screen plane, take the 2D perpendicular there.
    const screen_x = v3_dot(tangent, basis.right);
    const screen_y = v3_dot(tangent, basis.up);
    const len = Math.hypot(screen_x, screen_y);
    const radius = RIBBON_RADIUS * stroke_radii_at(radii, taper_window.start + (taper_window.end - taper_window.start) * t);
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

// A chain of cubics rendered as one stroke: a single flat profile (default
// width unless given) spread over the chain by chord length (cubics here are
// short cell segments, chord ≈ arc).
export function append_chain_ribbon(
  chain: StrokeControlPoints[], camera: OrbitCamera, color: Rgb, out: VertexSink, radii: StrokeRadii = DEFAULT_STROKE_RADII,
): void {
  const chords = chain.map((points) => v3_length(v3_sub(points.p3, points.p0)));
  const total = chords.reduce((sum, chord) => sum + chord, 0);
  if (total < 1e-9) return;
  let covered = 0;
  chain.forEach((points, index) => {
    const start = covered / total;
    covered += chords[index];
    append_bezier_ribbon(points, radii, { start, end: covered / total }, camera, color, out);
  });
}
