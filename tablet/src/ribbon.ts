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
  stroke_control_points, stroke_radii, stroke_radii_at,
} from "./document";
import { V3 } from "./math";
import { on_surface_stroke_samples } from "./patch";
import { Rgb, VertexSink, push_vertex_components } from "./vertex_sink";

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

type CameraBasis = { right: V3; up: V3 };

// One ribbon's samples, in flat arrays reused by every ribbon (one is built at a time, and
// nothing outside this file sees them): x, y, z per sample for centers, tangents and
// offsets; radius and color per sample.
type RibbonSamples = { centers: Float64Array; tangents: Float64Array; offsets: Float64Array; radii: Float64Array; colors: Rgb[] };
const ribbon_samples: RibbonSamples = {
  centers: new Float64Array(3 * (RIBBON_SAMPLES + 1)),
  tangents: new Float64Array(3 * (RIBBON_SAMPLES + 1)),
  offsets: new Float64Array(3 * (RIBBON_SAMPLES + 1)),
  radii: new Float64Array(RIBBON_SAMPLES + 1),
  colors: [],
};

function ensure_ribbon_sample_room(sample_count: number): void {
  if (ribbon_samples.radii.length >= sample_count) return;
  ribbon_samples.centers = new Float64Array(3 * sample_count);
  ribbon_samples.tangents = new Float64Array(3 * sample_count);
  ribbon_samples.offsets = new Float64Array(3 * sample_count);
  ribbon_samples.radii = new Float64Array(sample_count);
}

// The cubic's Bernstein weights for bezier_point and its derivative weights for
// bezier_tangent at each of the RIBBON_SAMPLES + 1 evenly spaced t, computed as those
// functions do.
type BezierSampleWeights = { point: Float64Array; tangent: Float64Array };
const bezier_sample_weights: BezierSampleWeights = ((): BezierSampleWeights => {
  const point = new Float64Array(4 * (RIBBON_SAMPLES + 1));
  const tangent = new Float64Array(3 * (RIBBON_SAMPLES + 1));
  for (let i = 0; i <= RIBBON_SAMPLES; i++) {
    const t = i / RIBBON_SAMPLES;
    const s = 1 - t;
    point[4 * i] = s * s * s;
    point[4 * i + 1] = 3 * s * s * t;
    point[4 * i + 2] = 3 * s * t * t;
    point[4 * i + 3] = t * t * t;
    tangent[3 * i] = 3 * s * s;
    tangent[3 * i + 1] = 6 * s * t;
    tangent[3 * i + 2] = 3 * t * t;
  }
  return { point, tangent };
})();

// cos and sin of the round cap's rim angles, segment 0 through ROUND_CAP_SEGMENTS (the last
// is the full turn, not angle 0 again).
type RoundCapRim = { cosines: Float64Array; sines: Float64Array };
const round_cap_rim: RoundCapRim = ((): RoundCapRim => {
  const cosines = new Float64Array(ROUND_CAP_SEGMENTS + 1);
  const sines = new Float64Array(ROUND_CAP_SEGMENTS + 1);
  for (let segment = 0; segment <= ROUND_CAP_SEGMENTS; segment++) {
    const angle = (segment / ROUND_CAP_SEGMENTS) * 2 * Math.PI;
    cosines[segment] = Math.cos(angle);
    sines[segment] = Math.sin(angle);
  }
  return { cosines, sines };
})();

// A ribbon through sample points already on the curve (an on-surface stroke's projected
// samples), tangents by finite differences between them.
export function append_sampled_ribbon(
  centers: V3[], radii: StrokeRadii, camera: OrbitCamera, color: Rgb, out: VertexSink, lightness_additions: StrokeRadii | null = null,
  end_caps: RibbonEndCaps = BOTH_ENDS_CAPPED,
): void {
  if (centers.length < 2) return;
  ensure_ribbon_sample_room(centers.length);
  const sample_centers = ribbon_samples.centers;
  const sample_tangents = ribbon_samples.tangents;
  const last = centers.length - 1;
  for (let i = 0; i <= last; i++) {
    const center = centers[i];
    const ahead = centers[Math.min(i + 1, last)];
    const behind = centers[Math.max(i - 1, 0)];
    sample_centers[3 * i] = center.x;
    sample_centers[3 * i + 1] = center.y;
    sample_centers[3 * i + 2] = center.z;
    sample_tangents[3 * i] = ahead.x - behind.x;
    sample_tangents[3 * i + 1] = ahead.y - behind.y;
    sample_tangents[3 * i + 2] = ahead.z - behind.z;
  }
  append_polyline_ribbon(centers.length, radii, { start: 0, end: 1 }, camera_basis(camera), color, lightness_additions, end_caps, out);
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
  append_cubic_ribbon(points, radii, taper_window, camera_basis(camera), color, lightness_additions, end_caps, out);
}

// The cubic sampled at RIBBON_SAMPLES + 1 evenly spaced t: bezier_point and bezier_tangent
// written out per coordinate, the same arithmetic in the same order.
function append_cubic_ribbon(
  points: StrokeControlPoints, radii: StrokeRadii, taper_window: TaperWindow, basis: CameraBasis, color: Rgb,
  lightness_additions: StrokeRadii | null, end_caps: RibbonEndCaps, out: VertexSink,
): void {
  const { p0, p1, p2, p3 } = points;
  const point_weights = bezier_sample_weights.point;
  const tangent_weights = bezier_sample_weights.tangent;
  const centers = ribbon_samples.centers;
  const tangents = ribbon_samples.tangents;
  for (let i = 0; i <= RIBBON_SAMPLES; i++) {
    const b0 = point_weights[4 * i], b1 = point_weights[4 * i + 1], b2 = point_weights[4 * i + 2], b3 = point_weights[4 * i + 3];
    centers[3 * i] = (p0.x * b0 + p1.x * b1) + (p2.x * b2 + p3.x * b3);
    centers[3 * i + 1] = (p0.y * b0 + p1.y * b1) + (p2.y * b2 + p3.y * b3);
    centers[3 * i + 2] = (p0.z * b0 + p1.z * b1) + (p2.z * b2 + p3.z * b3);
    const w0 = tangent_weights[3 * i], w1 = tangent_weights[3 * i + 1], w2 = tangent_weights[3 * i + 2];
    tangents[3 * i] = ((p1.x - p0.x) * w0 + (p2.x - p1.x) * w1) + (p3.x - p2.x) * w2;
    tangents[3 * i + 1] = ((p1.y - p0.y) * w0 + (p2.y - p1.y) * w1) + (p3.y - p2.y) * w2;
    tangents[3 * i + 2] = ((p1.z - p0.z) * w0 + (p2.z - p1.z) * w1) + (p3.z - p2.z) * w2;
  }
  append_polyline_ribbon(RIBBON_SAMPLES + 1, radii, taper_window, basis, color, lightness_additions, end_caps, out);
}

// The first `sample_count` samples in ribbon_samples.centers / .tangents, evenly spaced in
// the curve parameter; the taper and the lightness run over them by index. The part of the
// line whose width is zero or below is not drawn.
// Each end named in `end_caps` gets a round cap: a filled disc in the camera's screen
// plane, centred on the end sample with that sample's radius and color, so two strokes
// meeting at an angle leave no wedge gap at the corner. An end whose radius is zero
// (a tapered point) gets no cap.
function append_polyline_ribbon(
  sample_count: number, radii: StrokeRadii, taper_window: TaperWindow, basis: CameraBasis, color: Rgb,
  lightness_additions: StrokeRadii | null, end_caps: RibbonEndCaps, out: VertexSink,
): void {
  const { centers, tangents, offsets, radii: sample_radii, colors } = ribbon_samples;
  const { right, up } = basis;
  for (let i = 0; i < sample_count; i++) {
    const t = i / (sample_count - 1);
    const tangent_x = tangents[3 * i], tangent_y = tangents[3 * i + 1], tangent_z = tangents[3 * i + 2];
    // Project onto the camera's screen plane, take the 2D perpendicular there.
    const screen_x = tangent_x * right.x + tangent_y * right.y + tangent_z * right.z;
    const screen_y = tangent_x * up.x + tangent_y * up.y + tangent_z * up.z;
    const len = Math.hypot(screen_x, screen_y);
    const radius = Math.max(0, RIBBON_RADIUS * stroke_radii_at(radii, taper_window.start + (taper_window.end - taper_window.start) * t));
    sample_radii[i] = radius;
    colors[i] = lightness_additions === null ? color : lightened_color(color, stroke_radii_at(lightness_additions, t));
    if (len < 1e-9) {
      // Tangent points straight at the camera; fall back to screen-right.
      offsets[3 * i] = right.x * radius;
      offsets[3 * i + 1] = right.y * radius;
      offsets[3 * i + 2] = right.z * radius;
    } else {
      const along_right = (-screen_y / len) * radius;
      const along_up = (screen_x / len) * radius;
      offsets[3 * i] = right.x * along_right + up.x * along_up;
      offsets[3 * i + 1] = right.y * along_right + up.y * along_up;
      offsets[3 * i + 2] = right.z * along_right + up.z * along_up;
    }
  }
  for (let i = 0; i + 1 < sample_count; i++) {
    if (sample_radii[i] === 0 && sample_radii[i + 1] === 0) continue;
    const c0 = 3 * i;
    const c1 = 3 * (i + 1);
    // a = center + offset, b = center − offset, at sample i (0) and i + 1 (1).
    const a0_x = centers[c0] + offsets[c0], a0_y = centers[c0 + 1] + offsets[c0 + 1], a0_z = centers[c0 + 2] + offsets[c0 + 2];
    const b0_x = centers[c0] - offsets[c0], b0_y = centers[c0 + 1] - offsets[c0 + 1], b0_z = centers[c0 + 2] - offsets[c0 + 2];
    const a1_x = centers[c1] + offsets[c1], a1_y = centers[c1 + 1] + offsets[c1 + 1], a1_z = centers[c1 + 2] + offsets[c1 + 2];
    const b1_x = centers[c1] - offsets[c1], b1_y = centers[c1 + 1] - offsets[c1 + 1], b1_z = centers[c1 + 2] - offsets[c1 + 2];
    push_vertex_components(out, a0_x, a0_y, a0_z, colors[i]);
    push_vertex_components(out, b0_x, b0_y, b0_z, colors[i]);
    push_vertex_components(out, a1_x, a1_y, a1_z, colors[i + 1]);
    push_vertex_components(out, a1_x, a1_y, a1_z, colors[i + 1]);
    push_vertex_components(out, b0_x, b0_y, b0_z, colors[i]);
    push_vertex_components(out, b1_x, b1_y, b1_z, colors[i + 1]);
  }
  const last = sample_count - 1;
  if (end_caps.at_start) append_round_cap(0, sample_radii[0], colors[0], basis, out);
  if (end_caps.at_end) append_round_cap(last, sample_radii[last], colors[last], basis, out);
}

// A filled disc around center sample `sample` in the camera's screen plane, as a triangle
// fan pushed as a triangle list. Nothing for a zero radius.
function append_round_cap(sample: number, radius: number, color: Rgb, basis: CameraBasis, out: VertexSink): void {
  if (radius <= 0) return;
  const { right, up } = basis;
  const { cosines, sines } = round_cap_rim;
  const center_x = ribbon_samples.centers[3 * sample];
  const center_y = ribbon_samples.centers[3 * sample + 1];
  const center_z = ribbon_samples.centers[3 * sample + 2];
  const push_rim_vertex = (rim_segment: number): void => {
    const along_right = cosines[rim_segment] * radius;
    const along_up = sines[rim_segment] * radius;
    push_vertex_components(
      out,
      center_x + (right.x * along_right + up.x * along_up),
      center_y + (right.y * along_right + up.y * along_up),
      center_z + (right.z * along_right + up.z * along_up),
      color,
    );
  };
  for (let segment = 0; segment < ROUND_CAP_SEGMENTS; segment++) {
    push_vertex_components(out, center_x, center_y, center_z, color);
    push_rim_vertex(segment);
    push_rim_vertex(segment + 1);
  }
}

function chord_length(points: StrokeControlPoints): number {
  const x = points.p3.x - points.p0.x;
  const y = points.p3.y - points.p0.y;
  const z = points.p3.z - points.p0.z;
  return Math.sqrt(x * x + y * y + z * z);
}

// A chain of cubics rendered as one stroke: a single flat profile (default
// width unless given) spread over the chain by chord length (cubics here are
// short cell segments, chord ≈ arc).
export function append_chain_ribbon(
  chain: StrokeControlPoints[], camera: OrbitCamera, color: Rgb, out: VertexSink, radii: StrokeRadii = DEFAULT_STROKE_RADII,
): void {
  let total = 0;
  for (const points of chain) total += chord_length(points);
  if (total < 1e-9) return;
  const basis = camera_basis(camera);
  let covered = 0;
  const last_index = chain.length - 1;
  for (let index = 0; index <= last_index; index++) {
    const points = chain[index];
    const start = covered / total;
    covered += chord_length(points);
    // Caps only at the two ends of the whole chain, not at the seams between cubics.
    const end_caps: RibbonEndCaps = { at_start: index === 0, at_end: index === last_index };
    append_cubic_ribbon(points, radii, { start, end: covered / total }, basis, color, null, end_caps, out);
  }
}
