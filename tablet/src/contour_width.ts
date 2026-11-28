// The width and lightness of a computed contour (plan-head-picture-from-contours-and-chosen-lines
// Q8): at a place (u, v) of a patch they are blended from the patch's boundary strokes the
// way the surface blends its position from them (patch.ts): the Coons blend of the four
// sides (three sides: the left one is the collapsed corner, one value), or the loft's lerp
// between its two sides. Each stroke's value at a place is what its own ribbon draws there
// (ribbon.ts: its radii and lightness_additions read at the length fraction along its drawn
// line); a stroke without lightness_additions counts as 0. Hidden strokes, and strokes not
// always drawn, count the same as any other.
//
// Where two strokes meet (a patch corner, or two pieces of one side) the value is the
// average of the two. The Coons corner terms are those averages, so every border grid
// corner but the four patch corners has its own stroke's value exactly. A contour knot on
// the border is read from the stroke itself, not between grid corners, so it has exactly
// the value the stroke's ribbon (a border piece, boundary_contour.ts) and the knot of the
// next patch have there.
//
// The values at the grid corners depend only on the document: they are built once per
// patch fill and kept until a boundary stroke's radii or lightness_additions change
// (cached_contour_width_blend). A frame only interpolates them at the chain knots.

import { OrbitCamera } from "./camera";
import { GridEdgePlace, PlacedContourChain, corner_index } from "./contour";
import { Stroke, TabletDocument, stroke_radii, stroke_radii_at } from "./document";
import { PatchFill, Side, SurfaceGrid, side_piece_at } from "./patch";
import { RibbonEndCaps, append_chain_ribbon_with_knot_values, length_fraction_at, stroke_length_fractions } from "./ribbon";
import { Rgb, VertexSink } from "./vertex_sink";

// A width (in Stroke.radii units) and a lightness addition.
export type ProfileValues = { radius: number; lightness_addition: number };

// The blended values at every grid corner of one patch (corner_index layout), what is
// needed to read its boundary strokes at a border knot, and the stroke profiles it was
// built from (profile_key, one stroke after another in key_strokes order).
export type ContourWidthBlend = {
  fill: PatchFill;
  columns: number;
  rows: number;
  corner_radii: Float64Array;
  corner_lightness_additions: Float64Array;
  length_fractions_by_stroke: Map<Stroke, Float64Array>;
  key_strokes: Stroke[];
  profile_key: Float64Array;
};

// Blends kept per patch fill: a new patch geometry has a new fill, so a stale one is never
// looked up again (and the WeakMap lets it go).
export type ContourWidthBlendCache = WeakMap<PatchFill, ContourWidthBlend>;

export function create_contour_width_blend_cache(): ContourWidthBlendCache {
  return new WeakMap();
}

// The blend of `fill` sampled on `grid` (its stored grid; a mirrored grid has the same
// layout and reads the same blend), from the cache while its strokes' profiles are unchanged.
export function cached_contour_width_blend(
  cache: ContourWidthBlendCache, fill: PatchFill, grid: SurfaceGrid, tablet_document: TabletDocument,
): ContourWidthBlend {
  const cached = cache.get(fill);
  if (cached !== undefined && profile_key_matches(cached)) return cached;
  const blend = contour_width_blend_of(fill, grid, tablet_document);
  cache.set(fill, blend);
  return blend;
}

const PROFILE_KEY_NUMBERS_PER_STROKE = 8;

function write_profile_key(strokes: Stroke[], key: Float64Array): void {
  strokes.forEach((stroke, index) => {
    const radii = stroke_radii(stroke);
    const lightness_additions = stroke.lightness_additions;
    for (let k = 0; k < 4; k++) {
      key[PROFILE_KEY_NUMBERS_PER_STROKE * index + k] = radii[k];
      key[PROFILE_KEY_NUMBERS_PER_STROKE * index + 4 + k] = lightness_additions === undefined ? 0 : lightness_additions[k];
    }
  });
}

function profile_key_matches(blend: ContourWidthBlend): boolean {
  const strokes = blend.key_strokes;
  const key = blend.profile_key;
  for (let index = 0; index < strokes.length; index++) {
    const radii = stroke_radii(strokes[index]);
    const lightness_additions = strokes[index].lightness_additions;
    for (let k = 0; k < 4; k++) {
      if (key[PROFILE_KEY_NUMBERS_PER_STROKE * index + k] !== radii[k]) return false;
      const lightness_addition = lightness_additions === undefined ? 0 : lightness_additions[k];
      if (key[PROFILE_KEY_NUMBERS_PER_STROKE * index + 4 + k] !== lightness_addition) return false;
    }
  }
  return true;
}

function sides_of(fill: PatchFill): Side[] {
  return fill.kind === "loft" ? [fill.side_a, fill.side_b] : fill.sides;
}

export function contour_width_blend_of(fill: PatchFill, grid: SurfaceGrid, tablet_document: TabletDocument): ContourWidthBlend {
  const length_fractions_by_stroke = new Map<Stroke, Float64Array>();
  for (const side of sides_of(fill)) {
    for (const oriented of side) {
      if (!length_fractions_by_stroke.has(oriented.stroke)) {
        length_fractions_by_stroke.set(oriented.stroke, stroke_length_fractions(oriented.stroke, tablet_document));
      }
    }
  }
  const key_strokes = [...length_fractions_by_stroke.keys()];
  const profile_key = new Float64Array(PROFILE_KEY_NUMBERS_PER_STROKE * key_strokes.length);
  write_profile_key(key_strokes, profile_key);
  const corner_count = (grid.columns + 1) * (grid.rows + 1);
  const blend: ContourWidthBlend = {
    fill,
    columns: grid.columns,
    rows: grid.rows,
    corner_radii: new Float64Array(corner_count),
    corner_lightness_additions: new Float64Array(corner_count),
    length_fractions_by_stroke,
    key_strokes,
    profile_key,
  };
  if (fill.kind === "loft") write_loft_corner_values(blend, fill.side_a, fill.side_b, grid);
  else write_coons_corner_values(blend, fill.sides, grid);
  return blend;
}

// The stroke's values at curve parameter t, as its ribbon reads them.
function stroke_values_at(blend: ContourWidthBlend, stroke: Stroke, t: number): ProfileValues {
  const length_fraction = length_fraction_at(blend.length_fractions_by_stroke.get(stroke)!, t);
  const lightness_additions = stroke.lightness_additions;
  return {
    radius: stroke_radii_at(stroke_radii(stroke), length_fraction),
    lightness_addition: lightness_additions === undefined ? 0 : stroke_radii_at(lightness_additions, length_fraction),
  };
}

function average_values(a: ProfileValues, b: ProfileValues): ProfileValues {
  return { radius: (a.radius + b.radius) / 2, lightness_addition: (a.lightness_addition + b.lightness_addition) / 2 };
}

function side_start_values(blend: ContourWidthBlend, side: Side): ProfileValues {
  return stroke_values_at(blend, side[0].stroke, side[0].start.t);
}

function side_end_values(blend: ContourWidthBlend, side: Side): ProfileValues {
  const last = side[side.length - 1];
  return stroke_values_at(blend, last.stroke, last.end.t);
}

// The side's values at side parameter u (side_piece_at), the average of the two pieces
// where two meet. A side's two ends are read from its first and last piece.
function side_values_at(blend: ContourWidthBlend, side: Side, u: number): ProfileValues {
  const scaled = u * side.length;
  if (scaled <= 0) return side_start_values(blend, side);
  if (scaled >= side.length) return side_end_values(blend, side);
  if (Number.isInteger(scaled)) {
    const before = side[scaled - 1];
    const after = side[scaled];
    return average_values(stroke_values_at(blend, before.stroke, before.end.t), stroke_values_at(blend, after.stroke, after.start.t));
  }
  const { oriented, t } = side_piece_at(side, u);
  return stroke_values_at(blend, oriented.stroke, t);
}

// The side's values at its node_count + 1 evenly spaced side parameters k / node_count.
function sample_side_values(blend: ContourWidthBlend, side: Side, node_count: number): ProfileValues[] {
  const samples: ProfileValues[] = [];
  for (let node = 0; node <= node_count; node++) samples.push(side_values_at(blend, side, node / node_count));
  return samples;
}

// coons_surface_grid's blend of numbers, with the corner averages as both the sides' end
// values and the corner terms. Sides in loop order: bottom, right, top backwards, left
// backwards; a three-sided patch's left side is its collapsed corner, where the top ends
// and the bottom starts.
function write_coons_corner_values(blend: ContourWidthBlend, sides: Side[], grid: SurfaceGrid): void {
  const n = grid.columns;
  const bottom = sample_side_values(blend, sides[0], n);
  const right = sample_side_values(blend, sides[1], n);
  const top_backwards = sample_side_values(blend, sides[2], n);
  const corner_00 = sides.length === 4
    ? average_values(side_end_values(blend, sides[3]), side_start_values(blend, sides[0]))
    : average_values(side_end_values(blend, sides[2]), side_start_values(blend, sides[0]));
  const corner_10 = average_values(side_end_values(blend, sides[0]), side_start_values(blend, sides[1]));
  const corner_11 = average_values(side_end_values(blend, sides[1]), side_start_values(blend, sides[2]));
  const corner_01 = sides.length === 4 ? average_values(side_end_values(blend, sides[2]), side_start_values(blend, sides[3])) : corner_00;
  const left_backwards = sides.length === 4 ? sample_side_values(blend, sides[3], n) : new Array<ProfileValues>(n + 1).fill(corner_00);
  bottom[0] = corner_00;
  bottom[n] = corner_10;
  right[0] = corner_10;
  right[n] = corner_11;
  top_backwards[0] = corner_11;
  top_backwards[n] = corner_01;
  left_backwards[0] = corner_01;
  left_backwards[n] = corner_00;
  const coons_value = (i: number, j: number, value_of: (values: ProfileValues) => number): number => {
    const s = i / n;
    const t = j / n;
    const bottom_value = value_of(bottom[i]);
    const top_value = value_of(top_backwards[n - i]);
    const left_value = value_of(left_backwards[n - j]);
    const right_value = value_of(right[j]);
    const ruled = (bottom_value + (top_value - bottom_value) * t) + (left_value + (right_value - left_value) * s);
    const bilinear =
      (value_of(corner_00) * (1 - s) * (1 - t) + value_of(corner_10) * s * (1 - t)) +
      (value_of(corner_01) * (1 - s) * t + value_of(corner_11) * s * t);
    return ruled - bilinear;
  };
  for (let i = 0; i <= n; i++) {
    for (let j = 0; j <= n; j++) {
      const corner = corner_index(grid, i, j);
      blend.corner_radii[corner] = coons_value(i, j, (values) => values.radius);
      blend.corner_lightness_additions[corner] = coons_value(i, j, (values) => values.lightness_addition);
    }
  }
}

// loft_surface_grid_from_rails's lerp between the two sides, of numbers. The loft's two
// other edges are open: no stroke meets a side there.
function write_loft_corner_values(blend: ContourWidthBlend, side_a: Side, side_b: Side, grid: SurfaceGrid): void {
  const values_a = sample_side_values(blend, side_a, grid.columns);
  const values_b = sample_side_values(blend, side_b, grid.columns);
  for (let i = 0; i <= grid.columns; i++) {
    for (let j = 0; j <= grid.rows; j++) {
      const corner = corner_index(grid, i, j);
      const t = j / grid.rows;
      const a = values_a[i];
      const b = values_b[i];
      blend.corner_radii[corner] = a.radius + (b.radius - a.radius) * t;
      blend.corner_lightness_additions[corner] = a.lightness_addition + (b.lightness_addition - a.lightness_addition) * t;
    }
  }
}

// The boundary side a grid edge lies along and the side parameter of `place` on it, or
// null for an edge inside the patch (or along a loft's open edge).
function border_side_place(blend: ContourWidthBlend, place: GridEdgePlace): { side: Side; u: number } | null {
  const { from, to, s } = place;
  const last_i = blend.columns;
  const last_j = blend.rows;
  const i_at = from.i + (to.i - from.i) * s;
  const j_at = from.j + (to.j - from.j) * s;
  const fill = blend.fill;
  if (fill.kind === "loft") {
    if (from.j === 0 && to.j === 0) return { side: fill.side_a, u: i_at / last_i };
    if (from.j === last_j && to.j === last_j) return { side: fill.side_b, u: i_at / last_i };
    return null;
  }
  const sides = fill.sides;
  if (from.j === 0 && to.j === 0) return { side: sides[0], u: i_at / last_i };
  if (from.i === last_i && to.i === last_i) return { side: sides[1], u: j_at / last_j };
  if (from.j === last_j && to.j === last_j) return { side: sides[2], u: (last_i - i_at) / last_i };
  if (sides.length === 4 && from.i === 0 && to.i === 0) return { side: sides[3], u: (last_j - j_at) / last_j };
  return null;
}

// The blended values at a chain knot: on the border, the boundary stroke's own; inside,
// linear between the two grid corners of its edge. Exported for checks outside the drawing.
export function contour_knot_values_at(blend: ContourWidthBlend, place: GridEdgePlace): ProfileValues {
  const on_border = border_side_place(blend, place);
  if (on_border !== null) return side_values_at(blend, on_border.side, on_border.u);
  // corner_index, from the blend's own grid size (a mirrored grid has the same).
  const rows = blend.rows;
  const corner_from = place.from.i * (rows + 1) + place.from.j;
  const corner_to = place.to.i * (rows + 1) + place.to.j;
  const radii = blend.corner_radii;
  const lightness_additions = blend.corner_lightness_additions;
  return {
    radius: radii[corner_from] + (radii[corner_to] - radii[corner_from]) * place.s,
    lightness_addition: lightness_additions[corner_from] + (lightness_additions[corner_to] - lightness_additions[corner_from]) * place.s,
  };
}

// Knot values of the chain being drawn, reused by every chain.
let knot_radii = new Float64Array(64);
let knot_lightness_additions = new Float64Array(64);

// One chain of one side of one patch, drawn with its knots' blended width, and with its
// lightness unless `has_lightness` is false (the flat color). Capped at the ends named in
// `end_caps`. With `given_knot_widths` (one per knot) those are its widths instead.
export function append_contour_chain_ribbon_with_blend(
  placed: PlacedContourChain, blend: ContourWidthBlend, has_lightness: boolean,
  camera: OrbitCamera, color: Rgb, out: VertexSink, end_caps: RibbonEndCaps, given_knot_widths: Float64Array | null,
): void {
  const knot_count = placed.knot_places.length;
  if (knot_radii.length < knot_count) {
    knot_radii = new Float64Array(2 * knot_count);
    knot_lightness_additions = new Float64Array(2 * knot_count);
  }
  for (let knot = 0; knot < knot_count; knot++) {
    const values = contour_knot_values_at(blend, placed.knot_places[knot]);
    knot_radii[knot] = values.radius;
    knot_lightness_additions[knot] = values.lightness_addition;
  }
  const drawn_knot_radii = given_knot_widths === null ? knot_radii : given_knot_widths;
  append_chain_ribbon_with_knot_values(placed.chain, drawn_knot_radii, has_lightness ? knot_lightness_additions : null, camera, color, out, end_caps);
}

// The width (a stroke width value, as in Stroke.radii) at each knot of the chain, as
// append_contour_chain_ribbon_with_blend draws it.
export function contour_chain_knot_widths(placed: PlacedContourChain, blend: ContourWidthBlend): Float64Array {
  return Float64Array.from(placed.knot_places, (place) => contour_knot_values_at(blend, place).radius);
}
