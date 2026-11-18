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
const ROUND_CAP_SEGMENTS = 12; // triangles in the disc at each capped ribbon end

// Which ends of a ribbon get a round cap. A drawn stroke caps both; a contour chain
// caps only the start of its first cubic and the end of its last.
export type RibbonEndCaps = { at_start: boolean; at_end: boolean };
export const BOTH_ENDS_CAPPED: RibbonEndCaps = { at_start: true, at_end: true };

export type { Rgb };

// Appends interleaved [x,y,z, r,g,b] triangle vertices to out.
// An on-surface stroke (plan-hairline-drawn-on-surface Q4) draws its projected
// samples, tangents by finite differences between them.
// `lightness_additions` (the stroke's own, or null for a flat color) changes the color along
// the line. With `is_mirrored_side` the ribbon is the stroke's mirror image across the
// midline (x negated), built for the same camera.
export function append_stroke_ribbon(
  stroke: Stroke, tablet_document: TabletDocument, camera: OrbitCamera, color: Rgb,
  lightness_additions: StrokeRadii | null, is_mirrored_side: boolean, out: VertexSink,
): void {
  if (stroke.on_surface === true) {
    let centers = on_surface_stroke_samples(stroke, tablet_document);
    if (is_mirrored_side) centers = centers.map(mirrored_across_midline);
    append_sampled_ribbon(centers, stroke_radii(stroke), camera, color, out, lightness_additions);
    return;
  }
  let points = stroke_control_points(stroke, tablet_document);
  if (is_mirrored_side) {
    points = {
      p0: mirrored_across_midline(points.p0), p1: mirrored_across_midline(points.p1),
      p2: mirrored_across_midline(points.p2), p3: mirrored_across_midline(points.p3),
    };
  }
  append_bezier_ribbon(points, stroke_radii(stroke), { start: 0, end: 1 }, camera, color, out, lightness_additions);
}

function mirrored_across_midline(point: V3): V3 {
  return { x: -point.x, y: point.y, z: point.z };
}

// A ribbon through sample points already on the curve (an on-surface stroke's projected
// samples), tangents by finite differences between them.
export function append_sampled_ribbon(
  centers: V3[], radii: StrokeRadii, camera: OrbitCamera, color: Rgb, out: VertexSink, lightness_additions: StrokeRadii | null = null,
  end_caps: RibbonEndCaps = BOTH_ENDS_CAPPED,
): void {
  if (centers.length < 2) return;
  const last = centers.length - 1;
  const tangents = centers.map((_, i) => v3_sub(centers[Math.min(i + 1, last)], centers[Math.max(i - 1, 0)]));
  append_polyline_ribbon(centers, tangents, radii, { start: 0, end: 1 }, camera, color, lightness_additions, end_caps, out);
}

function linear_from_srgb(value: number): number {
  if (value <= 0.04045) return value / 12.92;
  return Math.pow((value + 0.055) / 1.055, 2.4);
}

function srgb_from_linear(value: number): number {
  if (value <= 0.0031308) return value * 12.92;
  return Math.pow(value, 1 / 2.4) * 1.055 - 0.055;
}

// The sRGB color with its linear light multiplied by 1 + addition, as the desktop
// renderer does with `Curve.lightness_additions`.
function lightened_color(color: Rgb, addition: number): Rgb {
  const lightened = (channel: number): number => {
    const linear = linear_from_srgb(channel) * (1 + addition);
    return srgb_from_linear(Math.max(0, Math.min(1, linear)));
  };
  return { r: lightened(color.r), g: lightened(color.g), b: lightened(color.b) };
}

// Where this cubic sits in the taper of the whole curve it belongs to, as a
// fraction range: a drawn stroke is one cubic covering 0..1; a computed
// contour chain spreads one taper over all its cubics by arc length.
export type TaperWindow = { start: number; end: number };

export function append_bezier_ribbon(
  points: StrokeControlPoints, radii: StrokeRadii, taper_window: TaperWindow, camera: OrbitCamera, color: Rgb, out: VertexSink,
  lightness_additions: StrokeRadii | null = null, end_caps: RibbonEndCaps = BOTH_ENDS_CAPPED,
): void {
  const centers: V3[] = [];
  const tangents: V3[] = [];
  for (let i = 0; i <= RIBBON_SAMPLES; i++) {
    const t = i / RIBBON_SAMPLES;
    centers.push(bezier_point(points, t));
    tangents.push(bezier_tangent(points, t));
  }
  append_polyline_ribbon(centers, tangents, radii, taper_window, camera, color, lightness_additions, end_caps, out);
}

// Samples evenly spaced in the curve parameter; the taper and the lightness run over
// them by index. The part of the line whose width is zero or below is not drawn.
// Each end named in `end_caps` gets a round cap: a filled disc in the camera's screen
// plane, centred on the end sample with that sample's radius and color, so two strokes
// meeting at an angle leave no wedge gap at the corner. An end whose radius is zero
// (a tapered point) gets no cap.
function append_polyline_ribbon(
  centers: V3[], tangents: V3[], radii: StrokeRadii, taper_window: TaperWindow, camera: OrbitCamera, color: Rgb,
  lightness_additions: StrokeRadii | null, end_caps: RibbonEndCaps, out: VertexSink,
): void {
  const basis = camera_basis(camera);
  const offsets: V3[] = [];
  const sample_radii: number[] = [];
  const colors: Rgb[] = [];
  for (let i = 0; i < centers.length; i++) {
    const t = i / (centers.length - 1);
    const tangent = tangents[i];
    // Project onto the camera's screen plane, take the 2D perpendicular there.
    const screen_x = v3_dot(tangent, basis.right);
    const screen_y = v3_dot(tangent, basis.up);
    const len = Math.hypot(screen_x, screen_y);
    const radius = Math.max(0, RIBBON_RADIUS * stroke_radii_at(radii, taper_window.start + (taper_window.end - taper_window.start) * t));
    sample_radii.push(radius);
    if (lightness_additions === null) colors.push(color);
    else colors.push(lightened_color(color, stroke_radii_at(lightness_additions, t)));
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
    if (sample_radii[i] === 0 && sample_radii[i + 1] === 0) continue;
    const a0 = v3_add(centers[i], offsets[i]);
    const b0 = v3_add(centers[i], v3_scale(offsets[i], -1));
    const a1 = v3_add(centers[i + 1], offsets[i + 1]);
    const b1 = v3_add(centers[i + 1], v3_scale(offsets[i + 1], -1));
    push_vertex(out, a0, colors[i]);
    push_vertex(out, b0, colors[i]);
    push_vertex(out, a1, colors[i + 1]);
    push_vertex(out, a1, colors[i + 1]);
    push_vertex(out, b0, colors[i]);
    push_vertex(out, b1, colors[i + 1]);
  }
  const last = centers.length - 1;
  if (end_caps.at_start) append_round_cap(centers[0], sample_radii[0], colors[0], basis.right, basis.up, out);
  if (end_caps.at_end) append_round_cap(centers[last], sample_radii[last], colors[last], basis.right, basis.up, out);
}

// A filled disc around `center` in the plane spanned by `screen_right` and `screen_up`,
// as a triangle fan pushed as a triangle list. Nothing for a zero radius.
function append_round_cap(center: V3, radius: number, color: Rgb, screen_right: V3, screen_up: V3, out: VertexSink): void {
  if (radius <= 0) return;
  const rim_point = (segment: number): V3 => {
    const angle = (segment / ROUND_CAP_SEGMENTS) * 2 * Math.PI;
    const along_right = v3_scale(screen_right, Math.cos(angle) * radius);
    const along_up = v3_scale(screen_up, Math.sin(angle) * radius);
    return v3_add(center, v3_add(along_right, along_up));
  };
  for (let segment = 0; segment < ROUND_CAP_SEGMENTS; segment++) {
    push_vertex(out, center, color);
    push_vertex(out, rim_point(segment), color);
    push_vertex(out, rim_point(segment + 1), color);
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
  const last_index = chain.length - 1;
  chain.forEach((points, index) => {
    const start = covered / total;
    covered += chords[index];
    // Caps only at the two ends of the whole chain, not at the seams between cubics.
    const end_caps: RibbonEndCaps = { at_start: index === 0, at_end: index === last_index };
    append_bezier_ribbon(points, radii, { start, end: covered / total }, camera, color, out, null, end_caps);
  });
}
