// Border contours (plan-head-picture-from-contours-and-chosen-lines Q7): the computed
// contour (contour.ts) is found only inside a patch's grid, so a contour lying exactly on a
// stroke between patches is never found there. This draws those pieces of the boundary
// strokes themselves: the stroke with its own width and lightness, cut where the patches
// next to it change the way they face the eye.
//
// Which side of the stroke a patch lies on, as the eye sees it, is the sign of
// g = sign × f at the patch's border corners, f the silhouette field of contour.ts. Every
// fill runs its loop counterclockwise in (u, v), so along any side (loop tangent × inward
// direction) is the grid's S_u × S_v and f says on which side of the loop direction the
// patch is seen. `sign` turns that into the stroke's own direction: -1 where the piece runs
// against the stroke, and -1 again for a loft's second side, which runs along u at the far
// row (the loop runs it backwards). The same holds on a mirrored grid, whose normals the
// mirror flips. A part of a stroke is drawn where:
// - one patch borders it (an open edge): always, unless the stroke is a midline stroke;
// - two or more border it: some two of them lie on the same side of it, the fold where one
//   faces the eye and the other faces away. With three or more that is every part (two of
//   any three are on the same side).
// A midline stroke borders a patch and that patch's mirror image, so its two sides meet
// there; it is drawn once, and a part of it only one patch borders (a one-sided patch, or
// no mirror image followed) is not drawn. On the mirrored side, the mirrored copy of every other stroke is
// treated the same with the mirrored patches, unless the stroke is one-sided.
//
// What does not depend on the eye (which border corners each part of each stroke reads,
// and how far between them) is built once by border_contours_of; a frame only reads f at
// those corners.

import { OrbitCamera, camera_eye } from "./camera";
import { SurfacePartials, corner_index } from "./contour";
import { Stroke, StrokeRadii, TabletDocument } from "./document";
import { V3 } from "./math";
import { PatchFill, Side, SurfaceGrid } from "./patch";
import { append_stroke_part_ribbon, append_stroke_ribbon, stroke_length_fractions } from "./ribbon";
import { Rgb, VertexSink } from "./vertex_sink";

// One side of one patch whose border is followed: its fill (which strokes its sides run
// along), the grid sampled from it and that grid's surface_partials. On the mirrored side
// the grid and partials are the mirror image's, the fill the stored one.
export type BorderSurface = { fill: PatchFill; grid: SurfaceGrid; partials: SurfacePartials; is_mirrored_side: boolean };

// One piece of one patch side, along one stroke: knot k is at stroke parameter knot_t[k]
// (monotone in k), where f is knot_weight[k] of the way from border corner
// knot_corner_from[k] to knot_corner_to[k]. knot_values holds g at each knot, written by
// each frame.
type BorderUse = {
  is_mirrored_side: boolean;
  partials: SurfacePartials;
  sign: number;
  t_low: number;
  t_high: number;
  knot_t: Float64Array;
  knot_corner_from: Int32Array;
  knot_corner_to: Int32Array;
  knot_weight: Float64Array;
  knot_values: Float64Array;
};

// A stroke that bounds a followed patch, the pieces of patch sides along it (all of them,
// and split by side), and its stroke_length_fractions.
type BorderedStroke = { stroke: Stroke; length_fractions: Float64Array; uses: BorderUse[]; stored_uses: BorderUse[]; mirrored_uses: BorderUse[] };

export type BorderContours = { bordered_strokes: BorderedStroke[] };

// How a border piece of `stroke` is drawn on one side: its color and lightness, or null
// when it is not drawn there (the caller already draws the whole stroke on that side).
export type BorderPieceLook = { color: Rgb; lightness_additions: StrokeRadii | null };
export type BorderPieceLookOf = (stroke: Stroke, is_mirrored_side: boolean) => BorderPieceLook | null;

// Below this a part of a stroke between two cuts is empty, and a drawn part that reaches
// this close to an end of the stroke is drawn whole.
const STROKE_T_EPSILON = 1e-6;

export function border_contours_of(surfaces: BorderSurface[], tablet_document: TabletDocument): BorderContours {
  const bordered_by_stroke = new Map<Stroke, BorderedStroke>();
  const add_use = (stroke: Stroke, use: BorderUse): void => {
    let bordered = bordered_by_stroke.get(stroke);
    if (bordered === undefined) {
      bordered = { stroke, length_fractions: stroke_length_fractions(stroke, tablet_document), uses: [], stored_uses: [], mirrored_uses: [] };
      bordered_by_stroke.set(stroke, bordered);
    }
    bordered.uses.push(use);
    if (use.is_mirrored_side) bordered.mirrored_uses.push(use);
    else bordered.stored_uses.push(use);
  };
  for (const surface of surfaces) {
    const grid = surface.grid;
    const last_column = grid.columns;
    const last_row = grid.rows;
    if (surface.fill.kind === "loft") {
      // Side a along u at row 0, side b along u at the last row, both sampled from i = 0.
      append_side_uses(surface, surface.fill.side_a, last_column, (k) => corner_index(grid, k, 0), 1, add_use);
      append_side_uses(surface, surface.fill.side_b, last_column, (k) => corner_index(grid, k, last_row), -1, add_use);
      continue;
    }
    // Coons, in loop order (coons_surface_grid): bottom, right, top backwards, left
    // backwards. A three-sided patch has no left side: its i = 0 border is one point.
    const sides = surface.fill.sides;
    append_side_uses(surface, sides[0], last_column, (k) => corner_index(grid, k, 0), 1, add_use);
    append_side_uses(surface, sides[1], last_row, (k) => corner_index(grid, last_column, k), 1, add_use);
    append_side_uses(surface, sides[2], last_column, (k) => corner_index(grid, last_column - k, last_row), 1, add_use);
    if (sides.length === 4) append_side_uses(surface, sides[3], last_row, (k) => corner_index(grid, 0, last_row - k), 1, add_use);
  }
  return { bordered_strokes: [...bordered_by_stroke.values()] };
}

// One use per piece of `side`, whose sample k of 0..last_node is at grid corner
// corner_of_node(k). Piece p takes the side's parameter range [p / pieces, (p + 1) / pieces]
// (side_point); its knots are the two ends of that range and every sample inside it.
// A degenerate corner (the collapsed corner of a three-sided patch) reads the nearest
// sample along the side that is not degenerate, as its f is noise.
function append_side_uses(
  surface: BorderSurface, side: Side, last_node: number, corner_of_node: (node: number) => number, side_sign: number,
  add_use: (stroke: Stroke, use: BorderUse) => void,
): void {
  const is_degenerate = surface.partials.corner_is_degenerate;
  const usable_corners = new Int32Array(last_node + 1);
  for (let node = 0; node <= last_node; node++) {
    let usable = -1;
    for (let distance = 0; distance <= last_node && usable === -1; distance++) {
      for (const candidate of [node - distance, node + distance]) {
        if (candidate < 0 || candidate > last_node) continue;
        if (is_degenerate[corner_of_node(candidate)] === 1) continue;
        usable = corner_of_node(candidate);
        break;
      }
    }
    if (usable === -1) return;
    usable_corners[node] = usable;
  }
  const piece_count = side.length;
  for (let piece_index = 0; piece_index < piece_count; piece_index++) {
    const piece = side[piece_index];
    // Side parameters as fractions over last_node * piece_count, so the samples strictly
    // inside the piece are found by integer comparison.
    const numerators = [piece_index * last_node];
    for (let node = 0; node <= last_node; node++) {
      const numerator = node * piece_count;
      if (numerator > piece_index * last_node && numerator < (piece_index + 1) * last_node) numerators.push(numerator);
    }
    numerators.push((piece_index + 1) * last_node);
    const knot_count = numerators.length;
    const knot_t = new Float64Array(knot_count);
    const knot_corner_from = new Int32Array(knot_count);
    const knot_corner_to = new Int32Array(knot_count);
    const knot_weight = new Float64Array(knot_count);
    for (let knot = 0; knot < knot_count; knot++) {
      const numerator = numerators[knot];
      const node_from = Math.min(Math.floor(numerator / piece_count), last_node - 1);
      const local = (numerator - piece_index * last_node) / last_node;
      knot_t[knot] = piece.start.t + (piece.end.t - piece.start.t) * local;
      knot_corner_from[knot] = usable_corners[node_from];
      knot_corner_to[knot] = usable_corners[node_from + 1];
      knot_weight[knot] = numerator / piece_count - node_from;
    }
    add_use(piece.stroke, {
      is_mirrored_side: surface.is_mirrored_side,
      partials: surface.partials,
      sign: piece.reversed ? -side_sign : side_sign,
      t_low: Math.min(piece.start.t, piece.end.t),
      t_high: Math.max(piece.start.t, piece.end.t),
      knot_t,
      knot_corner_from,
      knot_corner_to,
      knot_weight,
      knot_values: new Float64Array(knot_count),
    });
  }
}

export function append_border_contour_ribbons(
  border_contours: BorderContours, tablet_document: TabletDocument, camera: OrbitCamera, look_of: BorderPieceLookOf, out: VertexSink,
): void {
  const eye = camera_eye(camera);
  for (const bordered of border_contours.bordered_strokes) {
    const stroke = bordered.stroke;
    const stored_look = look_of(stroke, false);
    if (stroke.midline === true) {
      // The patches on both sides of the head meet at a midline stroke.
      if (stored_look !== null) append_drawn_parts(bordered, bordered.uses, eye, stored_look, false, tablet_document, camera, out);
      continue;
    }
    if (stored_look !== null) append_drawn_parts(bordered, bordered.stored_uses, eye, stored_look, false, tablet_document, camera, out);
    if (stroke.is_one_sided === true) continue;
    if (bordered.mirrored_uses.length === 0) continue;
    const mirrored_look = look_of(stroke, true);
    if (mirrored_look !== null) append_drawn_parts(bordered, bordered.mirrored_uses, eye, mirrored_look, true, tablet_document, camera, out);
  }
}

// g at every knot of `use` for this eye.
function write_use_values(use: BorderUse, eye: V3): void {
  const positions = use.partials.corner_positions;
  const normals = use.partials.corner_normals;
  const f_at = (corner: number): number => {
    const k = 3 * corner;
    return normals[k] * (eye.x - positions[k]) + normals[k + 1] * (eye.y - positions[k + 1]) + normals[k + 2] * (eye.z - positions[k + 2]);
  };
  for (let knot = 0; knot < use.knot_t.length; knot++) {
    const f_from = f_at(use.knot_corner_from[knot]);
    const f_to = f_at(use.knot_corner_to[knot]);
    use.knot_values[knot] = use.sign * (f_from + (f_to - f_from) * use.knot_weight[knot]);
  }
}

// g of `use` at stroke parameter t inside its range, linear between its knots.
function use_value_at(use: BorderUse, t: number): number {
  const knot_t = use.knot_t;
  for (let knot = 0; knot + 1 < knot_t.length; knot++) {
    const t_from = knot_t[knot];
    const t_to = knot_t[knot + 1];
    if ((t - t_from) * (t - t_to) > 0) continue;
    if (t_to === t_from) return use.knot_values[knot];
    const fraction = (t - t_from) / (t_to - t_from);
    return use.knot_values[knot] + (use.knot_values[knot + 1] - use.knot_values[knot]) * fraction;
  }
  return use.knot_values[knot_t.length - 1];
}

// Cuts the stroke where a use starts or ends or its g changes sign, decides each part
// between two cuts at its middle, and draws each run of drawn parts as one ribbon.
function append_drawn_parts(
  bordered: BorderedStroke, uses: BorderUse[], eye: V3, look: BorderPieceLook, is_mirrored_side: boolean,
  tablet_document: TabletDocument, camera: OrbitCamera, out: VertexSink,
): void {
  if (uses.length === 0) return;
  const cuts: number[] = [];
  for (const use of uses) {
    write_use_values(use, eye);
    cuts.push(use.t_low, use.t_high);
    const values = use.knot_values;
    for (let knot = 0; knot + 1 < values.length; knot++) {
      const value_from = values[knot];
      const value_to = values[knot + 1];
      if (value_from < 0 === value_to < 0) continue;
      const fraction = value_from / (value_from - value_to);
      cuts.push(use.knot_t[knot] + (use.knot_t[knot + 1] - use.knot_t[knot]) * fraction);
    }
  }
  cuts.sort((a, b) => a - b);
  const is_open_edge_drawn = bordered.stroke.midline !== true;
  let run_start = -1;
  let run_end = -1;
  const draw_run = (): void => {
    if (run_start < 0) return;
    const stroke = bordered.stroke;
    if (run_start <= STROKE_T_EPSILON && run_end >= 1 - STROKE_T_EPSILON) {
      append_stroke_ribbon(stroke, tablet_document, camera, look.color, look.lightness_additions, null, is_mirrored_side, out);
    } else {
      append_stroke_part_ribbon(stroke, tablet_document, bordered.length_fractions, run_start, run_end, camera, look.color, look.lightness_additions, is_mirrored_side, out);
    }
    run_start = -1;
  };
  for (let cut = 0; cut + 1 < cuts.length; cut++) {
    const part_start = cuts[cut];
    const part_end = cuts[cut + 1];
    if (part_end - part_start < STROKE_T_EPSILON) continue;
    if (!part_is_drawn(uses, (part_start + part_end) / 2, is_open_edge_drawn)) {
      draw_run();
      continue;
    }
    if (run_start < 0) run_start = part_start;
    run_end = part_end;
  }
  draw_run();
}

// Whether the part of the stroke around t is drawn: one use borders it (when
// `is_open_edge_drawn`), or two of the uses bordering it lie on the same side of it.
function part_is_drawn(uses: BorderUse[], t: number, is_open_edge_drawn: boolean): boolean {
  let bordering_count = 0;
  let positive_count = 0;
  let negative_count = 0;
  for (const use of uses) {
    if (t < use.t_low || t > use.t_high) continue;
    bordering_count++;
    if (use_value_at(use, t) < 0) negative_count++;
    else positive_count++;
  }
  if (bordering_count === 1) return is_open_edge_drawn;
  return positive_count >= 2 || negative_count >= 2;
}
