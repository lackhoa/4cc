// Stroke tessellation, matching the desktop renderer (draw_bezier_inner in
// game/framework_draw.cpp): each sample is offset perpendicular to the curve
// tangent IN CAMERA SPACE (billboard), so a stroke never vanishes edge-on.
// Radius is world-space (thins with distance): RIBBON_RADIUS times the stroke's
// own width profile (`Stroke.radii`, edited in the sketchpad's width panel;
// absent = DEFAULT_STROKE_RADII, a plain constant-width line), read by length
// fraction along the drawn line, not by the curve parameter. Mesh depends on
// the camera — rebuild it whenever the camera moves.

import { OrbitCamera, camera_basis, camera_eye } from "./camera";
import {
  DEFAULT_STROKE_RADII, Stroke, StrokeControlPoints, StrokeRadii, TabletDocument,
  bezier_point, stroke_control_points, stroke_radii, stroke_radii_at, stroke_radii_influence_at,
} from "./document";
import { V3, v3_lerp } from "./math";
import { on_surface_stroke_samples } from "./patch";
import { Rgb, VertexSink, push_vertex_components } from "./vertex_sink";

export const RIBBON_RADIUS = 0.01; // world units; grid cell = 1
// Straight pieces a planar stroke's ribbon is drawn with. A line that bounds a curved patch
// cuts under it between its points; at 48 the cut is no deeper than the patch's own grid allows.
const RIBBON_SAMPLES = 48;
// Straight pieces of the polyline drawn_stroke_points returns for a planar stroke.
const DRAWN_STROKE_POINT_SAMPLES = 24;
const ROUND_CAP_SEGMENTS = 12; // triangles in the disc at each capped ribbon end

// Which ends of a ribbon get a round cap. A drawn stroke caps both; a contour chain
// caps only the start of its first cubic and the end of its last.
export type RibbonEndCaps = { at_start: boolean; at_end: boolean };
export const BOTH_ENDS_CAPPED: RibbonEndCaps = { at_start: true, at_end: true };

export type { Rgb };

// Mixes `color` into a ribbon along its length, by how strongly width number `radii_index`
// acts there (stroke_radii_influence_at at the sample's taper t): full `color` where that
// number sets the width alone, none where it has no effect. The sketchpad's width drag
// shows with it which part of the line the grabbed number changes.
export type RibbonTint = { color: Rgb; radii_index: number };

// Appends interleaved [x,y,z, r,g,b] triangle vertices to out.
// An on-surface stroke (plan-hairline-drawn-on-surface Q4) draws its projected
// samples, tangents by finite differences between them.
// `lightness_additions` (the stroke's own, or null for a flat color) changes the color along
// the line, and `tint` (or null) is mixed in on top. With `is_mirrored_side` the ribbon is
// the stroke's mirror image across the midline (x negated), built for the same camera.
export function append_stroke_ribbon(
  stroke: Stroke, tablet_document: TabletDocument, camera: OrbitCamera, color: Rgb,
  lightness_additions: StrokeRadii | null, tint: RibbonTint | null, is_mirrored_side: boolean, out: VertexSink,
): void {
  if (stroke.on_surface === true) {
    let centers = on_surface_stroke_samples(stroke, tablet_document);
    if (is_mirrored_side) centers = centers.map(mirrored_across_midline);
    append_sampled_ribbon(centers, stroke_radii(stroke), camera, color, out, lightness_additions, BOTH_ENDS_CAPPED, tint);
    return;
  }
  let points = stroke_control_points(stroke, tablet_document);
  if (is_mirrored_side) {
    points = {
      p0: mirrored_across_midline(points.p0), p1: mirrored_across_midline(points.p1),
      p2: mirrored_across_midline(points.p2), p3: mirrored_across_midline(points.p3),
    };
  }
  append_bezier_ribbon(points, stroke_radii(stroke), { start: 0, end: 1 }, camera, color, out, lightness_additions, BOTH_ENDS_CAPPED, tint);
}

function mirrored_across_midline(point: V3): V3 {
  return { x: -point.x, y: point.y, z: point.z };
}

// What a ribbon needs of its camera: the screen plane (right, up) its band lies in, and the
// view rays (forward, eye, is_orthographic) its wide parts are pulled along.
type CameraBasis = { right: V3; up: V3; forward: V3; eye: V3; is_orthographic: boolean };

function ribbon_camera_basis(camera: OrbitCamera): CameraBasis {
  const { right, up, forward } = camera_basis(camera);
  return { right, up, forward, eye: camera_eye(camera), is_orthographic: camera.is_orthographic === true };
}

// A patch fill is drawn behind its own surface by enough for a line of this half-width
// lying on the surface to stay in front of it (render.ts, SURFACE_DEPTH_PUSH_LINE_HALF_WIDTH:
// the same number). A wider line's edge reaches further up a tilted surface, so the part of
// a ribbon wider than this is drawn nearer the camera by
// (half-width - this) * WIDE_LINE_DEPTH_PULL_PER_EXTRA_HALF_WIDTH, each vertex moved along
// its own view ray, so nothing moves on screen. A line of this width or thinner is not moved.
const SURFACE_CLEARED_LINE_HALF_WIDTH = 0.0025;
// tan(70 degrees), the steepest tilt the patch fills' own push covers.
const WIDE_LINE_DEPTH_PULL_PER_EXTRA_HALF_WIDTH = 2.75;

function wide_line_depth_pull(half_width: number): number {
  return Math.max(0, half_width - SURFACE_CLEARED_LINE_HALF_WIDTH) * WIDE_LINE_DEPTH_PULL_PER_EXTRA_HALF_WIDTH;
}

// Pushes the vertex at (x, y, z) moved `depth_pull` world units of view depth toward the
// camera along its view ray: against the forward for an orthographic camera, toward the eye
// for a perspective one.
function push_vertex_pulled_toward_camera(out: VertexSink, x: number, y: number, z: number, depth_pull: number, color: Rgb, basis: CameraBasis): void {
  if (depth_pull === 0) {
    push_vertex_components(out, x, y, z, color);
    return;
  }
  const { forward, eye } = basis;
  if (basis.is_orthographic) {
    push_vertex_components(out, x - forward.x * depth_pull, y - forward.y * depth_pull, z - forward.z * depth_pull, color);
    return;
  }
  const from_eye_x = x - eye.x, from_eye_y = y - eye.y, from_eye_z = z - eye.z;
  const view_depth = from_eye_x * forward.x + from_eye_y * forward.y + from_eye_z * forward.z;
  const kept = 1 - depth_pull / view_depth;
  push_vertex_components(out, eye.x + from_eye_x * kept, eye.y + from_eye_y * kept, eye.z + from_eye_z * kept, color);
}

// One ribbon's samples, in flat arrays reused by every ribbon (one is built at a time, and
// nothing outside this file sees them): x, y, z per sample for centers, tangents and
// offsets; radius, color and length fraction per sample.
type RibbonSamples = {
  centers: Float64Array; tangents: Float64Array; offsets: Float64Array; radii: Float64Array; colors: Rgb[]; length_fractions: Float64Array;
};
const ribbon_samples: RibbonSamples = {
  centers: new Float64Array(3 * (RIBBON_SAMPLES + 1)),
  tangents: new Float64Array(3 * (RIBBON_SAMPLES + 1)),
  offsets: new Float64Array(3 * (RIBBON_SAMPLES + 1)),
  radii: new Float64Array(RIBBON_SAMPLES + 1),
  colors: [],
  length_fractions: new Float64Array(RIBBON_SAMPLES + 1),
};

function ensure_ribbon_sample_room(sample_count: number): void {
  if (ribbon_samples.radii.length >= sample_count) return;
  ribbon_samples.centers = new Float64Array(3 * sample_count);
  ribbon_samples.tangents = new Float64Array(3 * sample_count);
  ribbon_samples.offsets = new Float64Array(3 * sample_count);
  ribbon_samples.radii = new Float64Array(sample_count);
  ribbon_samples.length_fractions = new Float64Array(sample_count);
}

// Writes into `fractions` how far along the polyline each of the first `point_count` points
// of `points` (x, y, z per point) is: the length of the polyline up to the point over its
// whole length, 0 at the first point, 1 at the last. The stroke's width and lightness values
// are read at these fractions, so they run evenly along the drawn line whatever the speed
// of the curve parameter. A polyline of zero length falls back to evenly spaced i / (n - 1).
// Returns the polyline's whole length.
export function polyline_length_fractions(points: Float64Array, point_count: number, fractions: Float64Array): number {
  fractions[0] = 0;
  for (let i = 1; i < point_count; i++) {
    const dx = points[3 * i] - points[3 * (i - 1)];
    const dy = points[3 * i + 1] - points[3 * (i - 1) + 1];
    const dz = points[3 * i + 2] - points[3 * (i - 1) + 2];
    fractions[i] = fractions[i - 1] + Math.sqrt(dx * dx + dy * dy + dz * dz);
  }
  const total_length = fractions[point_count - 1];
  for (let i = 0; i < point_count; i++) {
    fractions[i] = total_length === 0 ? i / (point_count - 1) : fractions[i] / total_length;
  }
  return total_length;
}

// A stroke's line as its ribbon draws it, from p0 to p3, x, y, z per point: an on-surface
// stroke's projected samples, otherwise the cubic at DRAWN_STROKE_POINT_SAMPLES + 1 evenly spaced t.
// Either way point i is at t = i / (point count - 1). The mirrored side has the same lengths.
export function drawn_stroke_points(stroke: Stroke, tablet_document: TabletDocument): Float64Array {
  let centers: V3[];
  if (stroke.on_surface === true) {
    centers = on_surface_stroke_samples(stroke, tablet_document);
  } else {
    const points = stroke_control_points(stroke, tablet_document);
    centers = Array.from({ length: DRAWN_STROKE_POINT_SAMPLES + 1 }, (_, i) =>
      bezier_point(points, i / DRAWN_STROKE_POINT_SAMPLES),
    );
  }
  return Float64Array.from(centers.flatMap((center) => [center.x, center.y, center.z]));
}

// How far along the stroke's drawn line (drawn_stroke_points) the point at curve parameter t
// is, as a fraction of the line's length: interpolated in t between the two drawn points
// around it.
export function stroke_length_fraction_at(stroke: Stroke, tablet_document: TabletDocument, t: number): number {
  return length_fraction_at(stroke_length_fractions(stroke, tablet_document), t);
}

// The length fraction (polyline_length_fractions) of each point of the stroke's drawn line
// (drawn_stroke_points), for a caller that reads it at many t (length_fraction_at).
export function stroke_length_fractions(stroke: Stroke, tablet_document: TabletDocument): Float64Array {
  const points = drawn_stroke_points(stroke, tablet_document);
  const point_count = points.length / 3;
  const fractions = new Float64Array(point_count);
  polyline_length_fractions(points, point_count, fractions);
  return fractions;
}

// stroke_length_fraction_at from the stroke's stroke_length_fractions.
export function length_fraction_at(fractions: Float64Array, t: number): number {
  const point_count = fractions.length;
  const position = t * (point_count - 1);
  const before = Math.min(Math.floor(position), point_count - 2);
  return fractions[before] + (fractions[before + 1] - fractions[before]) * (position - before);
}

// The part of `stroke` between curve parameters t_start < t_end, drawn as that part of the
// stroke's whole ribbon (append_stroke_ribbon) is: the width and the lightness are read
// through a taper window running between the part's two length fractions in
// `length_fractions` (stroke_length_fractions). Both ends are capped. With
// `is_mirrored_side` it is the part's mirror image across the midline.
export function append_stroke_part_ribbon(
  stroke: Stroke, tablet_document: TabletDocument, length_fractions: Float64Array, t_start: number, t_end: number,
  camera: OrbitCamera, color: Rgb, lightness_additions: StrokeRadii | null, is_mirrored_side: boolean, out: VertexSink,
): void {
  const taper_window: TaperWindow = { start: length_fraction_at(length_fractions, t_start), end: length_fraction_at(length_fractions, t_end) };
  if (stroke.on_surface === true) {
    let centers = samples_between(on_surface_stroke_samples(stroke, tablet_document), t_start, t_end);
    if (is_mirrored_side) centers = centers.map(mirrored_across_midline);
    append_sampled_ribbon(centers, stroke_radii(stroke), camera, color, out, lightness_additions, BOTH_ENDS_CAPPED, null, taper_window);
    return;
  }
  let points = cubic_between(stroke_control_points(stroke, tablet_document), t_start, t_end);
  if (is_mirrored_side) {
    points = {
      p0: mirrored_across_midline(points.p0), p1: mirrored_across_midline(points.p1),
      p2: mirrored_across_midline(points.p2), p3: mirrored_across_midline(points.p3),
    };
  }
  append_bezier_ribbon(points, stroke_radii(stroke), taper_window, camera, color, out, lightness_additions);
}

// The cubic's two halves at t, by de Casteljau: together they are the same curve.
function cubic_halves(points: StrokeControlPoints, t: number): { left: StrokeControlPoints; right: StrokeControlPoints } {
  const { p0, p1, p2, p3 } = points;
  const p01 = v3_lerp(p0, p1, t), p12 = v3_lerp(p1, p2, t), p23 = v3_lerp(p2, p3, t);
  const p012 = v3_lerp(p01, p12, t), p123 = v3_lerp(p12, p23, t);
  const knot = v3_lerp(p012, p123, t);
  return { left: { p0, p1: p01, p2: p012, p3: knot }, right: { p0: knot, p1: p123, p2: p23, p3 } };
}

// The cubic's part between t_start < t_end, as a cubic of its own.
function cubic_between(points: StrokeControlPoints, t_start: number, t_end: number): StrokeControlPoints {
  const up_to_end = cubic_halves(points, t_end).left;
  return cubic_halves(up_to_end, t_start / t_end).right;
}

// The part between t_start < t_end of a line sampled at evenly spaced t (sample i at
// t = i / (count - 1)): the points at the two ends, interpolated between their samples,
// and every sample between them.
function samples_between(samples: V3[], t_start: number, t_end: number): V3[] {
  const last = samples.length - 1;
  const point_at = (t: number): V3 => {
    const position = t * last;
    const before = Math.min(Math.floor(position), last - 1);
    return v3_lerp(samples[before], samples[before + 1], position - before);
  };
  const part = [point_at(t_start)];
  for (let i = Math.floor(t_start * last) + 1; i < t_end * last; i++) part.push(samples[i]);
  part.push(point_at(t_end));
  return part;
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
  end_caps: RibbonEndCaps = BOTH_ENDS_CAPPED, tint: RibbonTint | null = null, taper_window: TaperWindow = { start: 0, end: 1 },
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
  append_polyline_ribbon(centers.length, radii, taper_window, ribbon_camera_basis(camera), color, lightness_additions, end_caps, tint, out);
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

// `color` moved toward `toward` by `amount` (0 = color, 1 = toward), per channel.
function mixed_color(color: Rgb, toward: Rgb, amount: number): Rgb {
  return {
    r: color.r + (toward.r - color.r) * amount,
    g: color.g + (toward.g - color.g) * amount,
    b: color.b + (toward.b - color.b) * amount,
  };
}

// Where this cubic sits in the taper of the whole curve it belongs to, as a
// fraction range: a drawn stroke is one cubic covering 0..1; a computed
// contour chain spreads one taper over all its cubics by chord length, and
// inside each cubic by its length fraction.
export type TaperWindow = { start: number; end: number };

export function append_bezier_ribbon(
  points: StrokeControlPoints, radii: StrokeRadii, taper_window: TaperWindow, camera: OrbitCamera, color: Rgb, out: VertexSink,
  lightness_additions: StrokeRadii | null = null, end_caps: RibbonEndCaps = BOTH_ENDS_CAPPED, tint: RibbonTint | null = null,
): void {
  append_cubic_ribbon(points, radii, taper_window, ribbon_camera_basis(camera), color, lightness_additions, end_caps, tint, RIBBON_SAMPLES, out);
}

// The cubic sampled at piece_count + 1 evenly spaced t: bezier_point and bezier_tangent
// written out per coordinate, the same arithmetic in the same order. piece_count must
// divide RIBBON_SAMPLES (the weight table is read with that stride).
function append_cubic_ribbon(
  points: StrokeControlPoints, radii: StrokeRadii, taper_window: TaperWindow, basis: CameraBasis, color: Rgb,
  lightness_additions: StrokeRadii | null, end_caps: RibbonEndCaps, tint: RibbonTint | null, piece_count: number, out: VertexSink,
): void {
  const { p0, p1, p2, p3 } = points;
  const point_weights = bezier_sample_weights.point;
  const tangent_weights = bezier_sample_weights.tangent;
  const centers = ribbon_samples.centers;
  const tangents = ribbon_samples.tangents;
  const weight_stride = RIBBON_SAMPLES / piece_count;
  for (let i = 0; i <= piece_count; i++) {
    const weight_index = i * weight_stride;
    const b0 = point_weights[4 * weight_index], b1 = point_weights[4 * weight_index + 1], b2 = point_weights[4 * weight_index + 2], b3 = point_weights[4 * weight_index + 3];
    centers[3 * i] = (p0.x * b0 + p1.x * b1) + (p2.x * b2 + p3.x * b3);
    centers[3 * i + 1] = (p0.y * b0 + p1.y * b1) + (p2.y * b2 + p3.y * b3);
    centers[3 * i + 2] = (p0.z * b0 + p1.z * b1) + (p2.z * b2 + p3.z * b3);
    const w0 = tangent_weights[3 * weight_index], w1 = tangent_weights[3 * weight_index + 1], w2 = tangent_weights[3 * weight_index + 2];
    tangents[3 * i] = ((p1.x - p0.x) * w0 + (p2.x - p1.x) * w1) + (p3.x - p2.x) * w2;
    tangents[3 * i + 1] = ((p1.y - p0.y) * w0 + (p2.y - p1.y) * w1) + (p3.y - p2.y) * w2;
    tangents[3 * i + 2] = ((p1.z - p0.z) * w0 + (p2.z - p1.z) * w1) + (p3.z - p2.z) * w2;
  }
  append_polyline_ribbon(piece_count + 1, radii, taper_window, basis, color, lightness_additions, end_caps, tint, out);
}

// The first `sample_count` samples in ribbon_samples.centers / .tangents. The taper and the
// lightness are read at each sample's length fraction along this polyline
// (polyline_length_fractions), both mapped into `taper_window`. The part of the line
// whose width is zero or below is not drawn. The tint (if any) is read at the taper's t,
// the same t the width is.
// Each end named in `end_caps` gets a round cap: a filled disc in the camera's screen
// plane, centred on the end sample with that sample's radius and color, so two strokes
// meeting at an angle leave no wedge gap at the corner. An end whose radius is zero
// (a tapered point) gets no cap.
function append_polyline_ribbon(
  sample_count: number, radii: StrokeRadii, taper_window: TaperWindow, basis: CameraBasis, color: Rgb,
  lightness_additions: StrokeRadii | null, end_caps: RibbonEndCaps, tint: RibbonTint | null, out: VertexSink,
): void {
  const { centers, tangents, offsets, radii: sample_radii, colors, length_fractions } = ribbon_samples;
  const { right, up } = basis;
  polyline_length_fractions(centers, sample_count, length_fractions);
  for (let i = 0; i < sample_count; i++) {
    const length_fraction = length_fractions[i];
    const tangent_x = tangents[3 * i], tangent_y = tangents[3 * i + 1], tangent_z = tangents[3 * i + 2];
    // Project onto the camera's screen plane, take the 2D perpendicular there.
    const screen_x = tangent_x * right.x + tangent_y * right.y + tangent_z * right.z;
    const screen_y = tangent_x * up.x + tangent_y * up.y + tangent_z * up.z;
    const len = Math.hypot(screen_x, screen_y);
    const taper_t = taper_window.start + (taper_window.end - taper_window.start) * length_fraction;
    const radius = Math.max(0, RIBBON_RADIUS * stroke_radii_at(radii, taper_t));
    sample_radii[i] = radius;
    const own_color = lightness_additions === null ? color : lightened_color(color, stroke_radii_at(lightness_additions, taper_t));
    if (tint === null) {
      colors[i] = own_color;
    } else {
      colors[i] = mixed_color(own_color, tint.color, stroke_radii_influence_at(tint.radii_index, taper_t));
    }
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
    const pull0 = wide_line_depth_pull(sample_radii[i]);
    const pull1 = wide_line_depth_pull(sample_radii[i + 1]);
    push_vertex_pulled_toward_camera(out, a0_x, a0_y, a0_z, pull0, colors[i], basis);
    push_vertex_pulled_toward_camera(out, b0_x, b0_y, b0_z, pull0, colors[i], basis);
    push_vertex_pulled_toward_camera(out, a1_x, a1_y, a1_z, pull1, colors[i + 1], basis);
    push_vertex_pulled_toward_camera(out, a1_x, a1_y, a1_z, pull1, colors[i + 1], basis);
    push_vertex_pulled_toward_camera(out, b0_x, b0_y, b0_z, pull0, colors[i], basis);
    push_vertex_pulled_toward_camera(out, b1_x, b1_y, b1_z, pull1, colors[i + 1], basis);
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
  const depth_pull = wide_line_depth_pull(radius);
  const push_rim_vertex = (rim_segment: number): void => {
    const along_right = cosines[rim_segment] * radius;
    const along_up = sines[rim_segment] * radius;
    push_vertex_pulled_toward_camera(
      out,
      center_x + (right.x * along_right + up.x * along_up),
      center_y + (right.y * along_right + up.y * along_up),
      center_z + (right.z * along_right + up.z * along_up),
      depth_pull,
      color,
      basis,
    );
  };
  for (let segment = 0; segment < ROUND_CAP_SEGMENTS; segment++) {
    push_vertex_pulled_toward_camera(out, center_x, center_y, center_z, depth_pull, color, basis);
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

// Pieces per cubic of a chain. Must divide RIBBON_SAMPLES.
const CHAIN_CUBIC_PIECES = 4;

// A chain of cubics rendered as one stroke: a single flat profile (default
// width unless given) spread over the chain by chord length (cubics here are
// short cell segments, chord ≈ arc).
export function append_chain_ribbon(
  chain: StrokeControlPoints[], camera: OrbitCamera, color: Rgb, out: VertexSink, radii: StrokeRadii = DEFAULT_STROKE_RADII,
): void {
  let total = 0;
  for (const points of chain) total += chord_length(points);
  if (total < 1e-9) return;
  const basis = ribbon_camera_basis(camera);
  let covered = 0;
  const last_index = chain.length - 1;
  for (let index = 0; index <= last_index; index++) {
    const points = chain[index];
    const start = covered / total;
    covered += chord_length(points);
    // Caps only at the two ends of the whole chain, not at the seams between cubics.
    const end_caps: RibbonEndCaps = { at_start: index === 0, at_end: index === last_index };
    // A chain cubic spans one patch grid cell, a short and nearly straight piece of line.
    append_cubic_ribbon(points, radii, { start, end: covered / total }, basis, color, null, end_caps, null, CHAIN_CUBIC_PIECES, out);
  }
}
