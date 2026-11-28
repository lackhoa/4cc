// Computed contours (plan-tablet-computed-contours.md): where a patch's
// surface turns away from the camera, f = (S_u × S_v) · (eye − S) changes
// sign. The zero set of f on the patch's sample grid is found by marching
// squares, the crossings are joined into polylines by shared grid edge, and
// each polyline becomes a bezier chain (one cubic per cell, tangents from the
// gradient of f), so downstream a contour is the same object as a drawn
// stroke. Everything here is per frame: the result is never stored.
// (Since 2026-11-22 the eye-independent surface partials may be: see SurfacePartials.)
// Walkthrough with pictures: docs/interactive-contour-extraction.html.
//
// The extractor takes any scalar field on the surface (`ScalarField`); the
// silhouette field above is one caller. (The computed hairline's height field was
// another until 2026-10-11; the hairline is now drawn by hand, on the surface.)
//
// Grid indices are the parameter space: u = i, v = j (no rescaling; the
// zero set and the tangent directions don't care about the units).

import { StrokeControlPoints } from "./document";
import { SurfaceGrid } from "./patch";
import { V3, v3_add, v3_cross, v3_dot, v3_length, v3_lerp, v3_normalize, v3_scale, v3_sub } from "./math";

// Fraction of the largest raw normal below which a grid corner counts as
// degenerate (the collapsed corner of a 3-sided patch); cells touching one
// are skipped, their f is noise.
const DEGENERATE_NORMAL_FRACTION = 1e-3;

export type ContourChain = StrokeControlPoints[];

// A scalar on the surface, evaluated at a grid corner from its position and
// the surface partials there; the extractor returns its zero set.
export type ScalarField = (position: V3, partial_u: V3, partial_v: V3) => number;

export function silhouette_field(eye: V3): ScalarField {
  return (position, partial_u, partial_v) => v3_dot(v3_cross(partial_u, partial_v), v3_sub(eye, position));
}

export type GridPoint = { i: number; j: number };
// A place on a grid edge: s of the way from grid corner `from` to grid corner `to`.
export type GridEdgePlace = { from: GridPoint; to: GridPoint; s: number };
// A zero of f on a grid edge. Edge id is unique per grid edge so the two
// cells sharing it produce the same id (2 × corner index of (i, j) for the edge
// along u from (i, j), that + 1 for the edge along v); s is the fraction along the edge.
type Crossing = GridEdgePlace & { edge_id: number };

// A contour chain with the grid places of its knots: cubic k runs from knot_places[k] to
// knot_places[k + 1] (plan-head-picture-from-contours-and-chosen-lines Q8 reads the width
// and lightness there).
export type PlacedContourChain = { chain: ContourChain; knot_places: GridEdgePlace[] };
type Segment = { a: Crossing; b: Crossing };

// f per grid corner, flat: corner (i, j) at i × (rows + 1) + j (corner_index).
type CornerField = { f: Float64Array; partials: SurfacePartials };

export function corner_index(grid: SurfaceGrid, i: number, j: number): number {
  return i * (grid.rows + 1) + j;
}

function central_difference(values: V3[], index: number): V3 {
  const last = values.length - 1;
  if (index === 0) return v3_sub(values[1], values[0]);
  if (index === last) return v3_sub(values[last], values[last - 1]);
  return v3_scale(v3_sub(values[index + 1], values[index - 1]), 0.5);
}

// The part of the corner field that doesn't depend on the eye: the surface partials, their
// cross product (the raw normal) and the degenerate corners. A caller that keeps a grid
// across frames can compute this once per grid (the sketchpad does, during a camera
// gesture) and pass it to extract_contour_chains (and the border contours,
// boundary_contour.ts, read it at the grid's border corners). The flat arrays are indexed by
// corner_index (x, y, z at 3 × that) and, per cell (i, j), by i × rows + j.
// `corner_values` is where each extraction writes its f; it is overwritten by the next one.
export type SurfacePartials = {
  s_u: V3[][];
  s_v: V3[][];
  corner_positions: Float64Array;
  corner_normals: Float64Array;
  corner_is_degenerate: Uint8Array;
  cell_touches_degenerate_corner: Uint8Array;
  corner_values: Float64Array;
};

export function surface_partials(grid: SurfaceGrid): SurfacePartials {
  const s_u: V3[][] = [];
  const s_v: V3[][] = [];
  const normals: V3[][] = [];
  const normal_lengths: number[][] = [];
  let max_normal_length = 0;
  // Gathered once per row, not once per corner (it was the bulk of a contour frame).
  const rows_along_u: V3[][] = [];
  for (let j = 0; j <= grid.rows; j++) rows_along_u.push(grid.positions.map((column) => column[j]));
  for (let i = 0; i <= grid.columns; i++) {
    s_u.push([]);
    s_v.push([]);
    normals.push([]);
    normal_lengths.push([]);
    for (let j = 0; j <= grid.rows; j++) {
      const row_along_u = rows_along_u[j];
      const partial_u = central_difference(row_along_u, i);
      const partial_v = central_difference(grid.positions[i], j);
      const normal = v3_cross(partial_u, partial_v);
      const normal_length = v3_length(normal);
      max_normal_length = Math.max(max_normal_length, normal_length);
      s_u[i].push(partial_u);
      s_v[i].push(partial_v);
      normals[i].push(normal);
      normal_lengths[i].push(normal_length);
    }
  }
  const corner_count = (grid.columns + 1) * (grid.rows + 1);
  const corner_positions = new Float64Array(3 * corner_count);
  const corner_normals = new Float64Array(3 * corner_count);
  const corner_is_degenerate = new Uint8Array(corner_count);
  for (let i = 0; i <= grid.columns; i++) {
    for (let j = 0; j <= grid.rows; j++) {
      const corner = corner_index(grid, i, j);
      const position = grid.positions[i][j];
      const normal = normals[i][j];
      corner_positions[3 * corner] = position.x;
      corner_positions[3 * corner + 1] = position.y;
      corner_positions[3 * corner + 2] = position.z;
      corner_normals[3 * corner] = normal.x;
      corner_normals[3 * corner + 1] = normal.y;
      corner_normals[3 * corner + 2] = normal.z;
      corner_is_degenerate[corner] = normal_lengths[i][j] < DEGENERATE_NORMAL_FRACTION * max_normal_length ? 1 : 0;
    }
  }
  const cell_touches_degenerate_corner = new Uint8Array(grid.columns * grid.rows);
  for (let i = 0; i < grid.columns; i++) {
    for (let j = 0; j < grid.rows; j++) {
      const touches =
        corner_is_degenerate[corner_index(grid, i, j)] === 1 || corner_is_degenerate[corner_index(grid, i + 1, j)] === 1 ||
        corner_is_degenerate[corner_index(grid, i, j + 1)] === 1 || corner_is_degenerate[corner_index(grid, i + 1, j + 1)] === 1;
      cell_touches_degenerate_corner[i * grid.rows + j] = touches ? 1 : 0;
    }
  }
  return { s_u, s_v, corner_positions, corner_normals, corner_is_degenerate, cell_touches_degenerate_corner, corner_values: new Float64Array(corner_count) };
}

function evaluate_corner_field(grid: SurfaceGrid, partials: SurfacePartials, field_value: ScalarField): CornerField {
  const f = partials.corner_values;
  for (let i = 0; i <= grid.columns; i++) {
    for (let j = 0; j <= grid.rows; j++) f[corner_index(grid, i, j)] = field_value(grid.positions[i][j], partials.s_u[i][j], partials.s_v[i][j]);
  }
  return { f, partials };
}

// silhouette_field's values from the precomputed normals: the same arithmetic in the same
// order (cross, then dot with eye − S), so the same numbers bit for bit.
function silhouette_corner_field(partials: SurfacePartials, eye: V3): CornerField {
  const f = partials.corner_values;
  const positions = partials.corner_positions;
  const normals = partials.corner_normals;
  for (let corner = 0; corner < f.length; corner++) {
    const k = 3 * corner;
    f[corner] = normals[k] * (eye.x - positions[k]) + normals[k + 1] * (eye.y - positions[k + 1]) + normals[k + 2] * (eye.z - positions[k + 2]);
  }
  return { f, partials };
}

function edge_crossing(grid: SurfaceGrid, field: CornerField, from: GridPoint, to: GridPoint, edge_id: number): Crossing {
  const f0 = field.f[corner_index(grid, from.i, from.j)];
  const f1 = field.f[corner_index(grid, to.i, to.j)];
  return { edge_id, from, to, s: f0 / (f0 - f1) };
}

// Marching squares: one segment per cell between its two sign-flip edges.
// Four flips (the saddle) are paired in edge order; the double-crossing
// edge case is out of scope for now.
function marching_squares(grid: SurfaceGrid, field: CornerField): Segment[] {
  const segments: Segment[] = [];
  const f = field.f;
  const skipped = field.partials.cell_touches_degenerate_corner;
  for (let j = 0; j < grid.rows; j++) {
    for (let i = 0; i < grid.columns; i++) {
      if (skipped[i * grid.rows + j] === 1) continue;
      const corner_00 = corner_index(grid, i, j);
      const corner_10 = corner_index(grid, i + 1, j);
      const corner_01 = corner_00 + 1;
      const corner_11 = corner_10 + 1;
      const negative_00 = f[corner_00] < 0, negative_10 = f[corner_10] < 0, negative_01 = f[corner_01] < 0, negative_11 = f[corner_11] < 0;
      const flips_along_u_at_j = negative_00 !== negative_10;
      const flips_along_u_at_j_plus_1 = negative_01 !== negative_11;
      const flips_along_v_at_i = negative_00 !== negative_01;
      const flips_along_v_at_i_plus_1 = negative_10 !== negative_11;
      // Most cells have no crossing; the corner points and crossings are built only for the rest.
      if (!flips_along_u_at_j && !flips_along_u_at_j_plus_1 && !flips_along_v_at_i && !flips_along_v_at_i_plus_1) continue;
      const point_00 = { i, j }, point_10 = { i: i + 1, j }, point_01 = { i, j: j + 1 }, point_11 = { i: i + 1, j: j + 1 };
      const crossings: Crossing[] = [];
      if (flips_along_u_at_j) crossings.push(edge_crossing(grid, field, point_00, point_10, 2 * corner_00));
      if (flips_along_u_at_j_plus_1) crossings.push(edge_crossing(grid, field, point_01, point_11, 2 * corner_01));
      if (flips_along_v_at_i) crossings.push(edge_crossing(grid, field, point_00, point_01, 2 * corner_00 + 1));
      if (flips_along_v_at_i_plus_1) crossings.push(edge_crossing(grid, field, point_10, point_11, 2 * corner_10 + 1));
      for (let k = 0; k + 1 < crossings.length; k += 2) segments.push({ a: crossings[k], b: crossings[k + 1] });
    }
  }
  return segments;
}

// Walk segments end to end by shared edge id. Each id sits in at most two
// segments, so a polyline is followed by hopping to the other holder of the
// far end; open polylines start at an unshared end, closed loops anywhere.
function join_into_polylines(segments: Segment[]): Crossing[][] {
  const holders = new Map<number, number[]>();
  segments.forEach((segment, index) => {
    for (const end of [segment.a, segment.b]) {
      if (!holders.has(end.edge_id)) holders.set(end.edge_id, []);
      holders.get(end.edge_id)!.push(index);
    }
  });
  const visited = new Array<boolean>(segments.length).fill(false);
  const polylines: Crossing[][] = [];
  const walk_from = (start: number, first_end: Crossing): void => {
    const polyline: Crossing[] = [first_end];
    let index = start;
    let near = first_end;
    while (!visited[index]) {
      visited[index] = true;
      const segment = segments[index];
      const far = segment.a.edge_id === near.edge_id ? segment.b : segment.a;
      polyline.push(far);
      const next = holders.get(far.edge_id)!.find((other) => other !== index);
      if (next === undefined) break;
      index = next;
      near = far;
    }
    polylines.push(polyline);
  };
  segments.forEach((segment, index) => {
    if (visited[index]) return;
    for (const end of [segment.a, segment.b]) {
      if (holders.get(end.edge_id)!.length === 1) {
        walk_from(index, end);
        return;
      }
    }
  });
  segments.forEach((segment, index) => {
    if (!visited[index]) walk_from(index, segment.a);
  });
  return polylines;
}

function lerp_number(a: number, b: number, t: number): number {
  return a + (b - a) * t;
}

// Gradient of f at a grid corner, by finite differences of the corner values.
function f_gradient_at_corner(grid: SurfaceGrid, field: CornerField, point: GridPoint): { u: number; v: number } {
  const value = (i: number, j: number): number => field.f[corner_index(grid, i, j)];
  const last_i = grid.columns;
  const last_j = grid.rows;
  const { i, j } = point;
  const f_u = i === 0 ? value(1, j) - value(0, j) : i === last_i ? value(last_i, j) - value(last_i - 1, j) : (value(i + 1, j) - value(i - 1, j)) / 2;
  const f_v = j === 0 ? value(i, 1) - value(i, 0) : j === last_j ? value(i, last_j) - value(i, last_j - 1) : (value(i, j + 1) - value(i, j - 1)) / 2;
  return { u: f_u, v: f_v };
}

type Knot = { position: V3; tangent: V3 };

// Position and 3D contour tangent at a crossing. The tangent is the gradient
// of f rotated 90° in (u, v) (the contour runs along the level set), pushed
// to 3D through the surface partials; sign fixed later against the walk.
function knot_at_crossing(grid: SurfaceGrid, field: CornerField, crossing: Crossing): Knot {
  const { from, to, s } = crossing;
  const position = v3_lerp(grid.positions[from.i][from.j], grid.positions[to.i][to.j], s);
  const gradient_from = f_gradient_at_corner(grid, field, from);
  const gradient_to = f_gradient_at_corner(grid, field, to);
  const f_u = lerp_number(gradient_from.u, gradient_to.u, s);
  const f_v = lerp_number(gradient_from.v, gradient_to.v, s);
  const s_u = v3_lerp(field.partials.s_u[from.i][from.j], field.partials.s_u[to.i][to.j], s);
  const s_v = v3_lerp(field.partials.s_v[from.i][from.j], field.partials.s_v[to.i][to.j], s);
  const tangent = v3_add(v3_scale(s_u, -f_v), v3_scale(s_v, f_u));
  return { position, tangent };
}

function oriented_unit_tangent(knot: Knot, along: V3): V3 {
  if (v3_length(knot.tangent) < 1e-9) return v3_normalize(along);
  const unit = v3_normalize(knot.tangent);
  return v3_dot(unit, along) < 0 ? v3_scale(unit, -1) : unit;
}

// Cubic Hermite per cell segment: endpoints at the crossings, handles along
// the contour tangents, chord / 3 long. A segment shorter than 1e-9 is left out, so the
// next cubic starts at the place the last one ended, give or take that.
function fit_bezier_chain(knots: Knot[], crossings: Crossing[]): PlacedContourChain {
  const chain: ContourChain = [];
  const knot_places: GridEdgePlace[] = [];
  for (let k = 0; k + 1 < knots.length; k++) {
    const p0 = knots[k].position;
    const p3 = knots[k + 1].position;
    const chord_vector = v3_sub(p3, p0);
    const chord = v3_length(chord_vector);
    if (chord < 1e-9) continue;
    const t0 = oriented_unit_tangent(knots[k], chord_vector);
    const t3 = oriented_unit_tangent(knots[k + 1], chord_vector);
    chain.push({ p0, p1: v3_add(p0, v3_scale(t0, chord / 3)), p2: v3_sub(p3, v3_scale(t3, chord / 3)), p3 });
    if (knot_places.length === 0) knot_places.push(crossings[k]);
    knot_places.push(crossings[k + 1]);
  }
  return { chain, knot_places };
}

// `partials` must be surface_partials(grid) of this same grid; a caller without a cache
// leaves it out.
export function extract_contour_chains(grid: SurfaceGrid, eye: V3, partials: SurfacePartials = surface_partials(grid)): ContourChain[] {
  return extract_placed_contour_chains(grid, eye, partials).map((placed) => placed.chain);
}

// extract_contour_chains with the grid places of each chain's knots.
export function extract_placed_contour_chains(grid: SurfaceGrid, eye: V3, partials: SurfacePartials = surface_partials(grid)): PlacedContourChain[] {
  return chains_from_corner_field(grid, silhouette_corner_field(partials, eye));
}

// Zero set of `field_value` on the grid as bezier chains, one per polyline.
export function extract_level_set_chains(grid: SurfaceGrid, field_value: ScalarField): ContourChain[] {
  return chains_from_corner_field(grid, evaluate_corner_field(grid, surface_partials(grid), field_value)).map((placed) => placed.chain);
}

function chains_from_corner_field(grid: SurfaceGrid, field: CornerField): PlacedContourChain[] {
  const segments = marching_squares(grid, field);
  const polylines = join_into_polylines(segments);
  return polylines
    .map((polyline) => fit_bezier_chain(polyline.map((crossing) => knot_at_crossing(grid, field, crossing)), polyline))
    .filter((placed) => placed.chain.length > 0);
}
