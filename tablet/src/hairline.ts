// The hairline, computed every frame (plan-skin-hairline-landmark.md Q12-Q14), never
// stored. Loomis's face-thirds rule: the hairline sits one brow-to-nose-base length
// above the brow. The front part is the level set y = hairline_y of the forehead
// patch's surface, from the midline out to where it leaves the patch (the temple); the
// side part follows the patch boundary down from there to the brow height (the
// sideburn). Both are bezier chains like the contours (src/contour.ts), so the drawers
// treat the hairline as one more computed curve.
//
// Inputs are found by name so any fitted head inherits them (Q13): the vertices
// `brow` and `nose_base`, and the forehead patch = the skin patch with a midline
// stroke whose canon-height level set starts on the midline furthest forward. A
// document without the names, or without such a patch, has no hairline (empty chain).

import { ContourChain, extract_level_set_chains } from "./contour";
import {
  Patch, Stroke, StrokeControlPoints, StrokeRadii, TabletDocument, bezier_point, patch_layer, stroke_by_id, stroke_control_points,
  vertex_by_id, vertex_world_position,
} from "./document";
import { V3, v3_lerp } from "./math";
import { patch_surface_grid } from "./patch";
import { Rgb } from "./vertex_sink";

export const HAIRLINE_COLOR: Rgb = { r: 0.8, g: 0.3, b: 0.6 }; // derived line, unlike any stroke or contour color
export const HAIRLINE_CSS_COLOR = "rgb(204, 77, 153)"; // the same, for 2D canvas drawers
export const HAIRLINE_RADII: StrokeRadii = [0.5, 0.5, 0.5, 0.5]; // twice the default stroke width (Q14)

const MIDLINE_X_TOLERANCE = 0.02; // a level set end this close to x = 0 starts on the midline
const BOUNDARY_SAMPLES = 64; // per stroke, when locating a point or a height on the patch boundary

function named_vertex_y(tablet_document: TabletDocument, name: string): number | null {
  const vertex = tablet_document.vertices.find((v) => v.name === name);
  return vertex === undefined ? null : vertex_world_position(tablet_document, vertex).y;
}

function stroke_is_on_midline(stroke: Stroke, tablet_document: TabletDocument): boolean {
  if (stroke.midline === true) return true;
  const p0 = vertex_world_position(tablet_document, vertex_by_id(tablet_document, stroke.p0_vertex));
  const p3 = vertex_world_position(tablet_document, vertex_by_id(tablet_document, stroke.p3_vertex));
  return Math.abs(p0.x) < 1e-6 && Math.abs(p3.x) < 1e-6;
}

function reversed_cubic(points: StrokeControlPoints): StrokeControlPoints {
  return { p0: points.p3, p1: points.p2, p2: points.p1, p3: points.p0 };
}

function reversed_chain(chain: ContourChain): ContourChain {
  return chain.map(reversed_cubic).reverse();
}

// The piece of a cubic between parameters t0 and t1 (de Casteljau twice), running
// from t0 to t1, so t0 > t1 gives the piece backwards.
function bezier_sub_curve(points: StrokeControlPoints, t0: number, t1: number): StrokeControlPoints {
  if (t0 > t1) return reversed_cubic(bezier_sub_curve(points, t1, t0));
  const right_of = (p: StrokeControlPoints, t: number): StrokeControlPoints => {
    const p01 = v3_lerp(p.p0, p.p1, t), p12 = v3_lerp(p.p1, p.p2, t), p23 = v3_lerp(p.p2, p.p3, t);
    const p012 = v3_lerp(p01, p12, t), p123 = v3_lerp(p12, p23, t);
    return { p0: v3_lerp(p012, p123, t), p1: p123, p2: p23, p3: p.p3 };
  };
  const left_of = (p: StrokeControlPoints, t: number): StrokeControlPoints => reversed_cubic(right_of(reversed_cubic(p), 1 - t));
  const from_t0 = right_of(points, t0);
  return t0 >= 1 ? from_t0 : left_of(from_t0, (t1 - t0) / (1 - t0));
}

// The front part: the canon-height level set of the forehead patch, oriented from
// the midline outwards. Returns the patch too, for the boundary walk.
function front_chain(tablet_document: TabletDocument, hairline_y: number): { chain: ContourChain; patch: Patch } | null {
  let best: { chain: ContourChain; patch: Patch; z: number } | null = null;
  for (const patch of tablet_document.patches) {
    if (patch_layer(patch, tablet_document) !== "skin") continue;
    const strokes = patch.strokes.map((id) => stroke_by_id(tablet_document, id));
    if (!strokes.some((stroke) => stroke_is_on_midline(stroke, tablet_document))) continue;
    const grid = patch_surface_grid(patch, tablet_document);
    if (grid === null) continue;
    for (const raw_chain of extract_level_set_chains(grid, (position) => position.y - hairline_y)) {
      const start = raw_chain[0].p0, end = raw_chain[raw_chain.length - 1].p3;
      const chain = Math.abs(start.x) <= Math.abs(end.x) ? raw_chain : reversed_chain(raw_chain);
      const midline_end = chain[0].p0;
      if (Math.abs(midline_end.x) > MIDLINE_X_TOLERANCE) continue;
      if (best === null || midline_end.z > best.z) best = { chain, patch, z: midline_end.z };
    }
  }
  return best === null ? null : { chain: best.chain, patch: best.patch };
}

type BoundaryHit = { stroke: Stroke; t: number };

function nearest_boundary_point(strokes: Stroke[], tablet_document: TabletDocument, target: V3): BoundaryHit | null {
  let best: (BoundaryHit & { distance: number }) | null = null;
  for (const stroke of strokes) {
    const points = stroke_control_points(stroke, tablet_document);
    for (let i = 0; i <= BOUNDARY_SAMPLES; i++) {
      const t = i / BOUNDARY_SAMPLES;
      const p = bezier_point(points, t);
      const distance = Math.hypot(p.x - target.x, p.y - target.y, p.z - target.z);
      if (best === null || distance < best.distance) best = { stroke, t, distance };
    }
  }
  return best;
}

// The parameter where the cubic first reaches height y, walking from t_from towards
// t_to; null when it stays above y on that stretch.
function parameter_at_height(points: StrokeControlPoints, t_from: number, t_to: number, y: number): number | null {
  let previous_t = t_from;
  let previous_y = bezier_point(points, t_from).y;
  for (let i = 1; i <= BOUNDARY_SAMPLES; i++) {
    const t = t_from + (t_to - t_from) * (i / BOUNDARY_SAMPLES);
    const sample_y = bezier_point(points, t).y;
    if (sample_y <= y) return previous_y === sample_y ? t : previous_t + (t - previous_t) * ((previous_y - y) / (previous_y - sample_y));
    previous_t = t;
    previous_y = sample_y;
  }
  return null;
}

// The side part: from where the front part leaves the patch, down the patch boundary
// (the strokes that are not on the midline) to the brow height.
function side_chain(tablet_document: TabletDocument, patch: Patch, exit: V3, brow_y: number): ContourChain {
  const boundary = patch.strokes.map((id) => stroke_by_id(tablet_document, id)).filter((stroke) => !stroke_is_on_midline(stroke, tablet_document));
  const hit = nearest_boundary_point(boundary, tablet_document, exit);
  if (hit === null) return [];
  const chain: ContourChain = [];
  let stroke = hit.stroke;
  let t_from = hit.t;
  let came_from: Stroke | null = null;
  for (let step = 0; step < boundary.length; step++) {
    const points = stroke_control_points(stroke, tablet_document);
    // Head for the lower end of this stroke.
    const t_to = points.p0.y < points.p3.y ? 0 : 1;
    const t_brow = parameter_at_height(points, t_from, t_to, brow_y);
    chain.push(bezier_sub_curve(points, t_from, t_brow ?? t_to));
    if (t_brow !== null) return chain;
    const end_vertex = t_to === 0 ? stroke.p0_vertex : stroke.p3_vertex;
    const next = boundary.find((other) => other !== stroke && other !== came_from && (other.p0_vertex === end_vertex || other.p3_vertex === end_vertex));
    if (next === undefined) return chain; // the boundary ends above the brow
    came_from = stroke;
    stroke = next;
    t_from = next.p0_vertex === end_vertex ? 0 : 1;
  }
  return chain;
}

// The hairline of a document as one bezier chain (front part then side part), empty
// when the document has no `brow` / `nose_base` vertices or no forehead patch.
export function hairline_chain(tablet_document: TabletDocument): ContourChain {
  const brow_y = named_vertex_y(tablet_document, "brow");
  const nose_base_y = named_vertex_y(tablet_document, "nose_base");
  if (brow_y === null || nose_base_y === null) return [];
  const hairline_y = brow_y + (brow_y - nose_base_y);
  const front = front_chain(tablet_document, hairline_y);
  if (front === null) return [];
  const exit = front.chain[front.chain.length - 1].p3;
  return [...front.chain, ...side_chain(tablet_document, front.patch, exit, brow_y)];
}
