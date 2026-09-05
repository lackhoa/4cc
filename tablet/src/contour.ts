// Computed contours (plan-tablet-computed-contours.md): where a patch's
// surface turns away from the camera, f = (S_u × S_v) · (eye − S) changes
// sign. The zero set of f on the patch's sample grid is found by marching
// squares, the crossings are joined into polylines by shared grid edge, and
// each polyline becomes a bezier chain (one cubic per cell, tangents from the
// gradient of f), so downstream a contour is the same object as a drawn
// stroke. Everything here is per frame: the result is never stored.
// Walkthrough with pictures: docs/interactive-contour-extraction.html.
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

type GridPoint = { i: number; j: number };
// A zero of f on a grid edge. Edge id is unique per grid edge so the two
// cells sharing it produce the same id ("h" = along u from (i, j), "v" =
// along v from (i, j)); s is the fraction along the edge.
type Crossing = { edge_id: string; from: GridPoint; to: GridPoint; s: number };
type Segment = { a: Crossing; b: Crossing };

type CornerField = { f: number[][]; s_u: V3[][]; s_v: V3[][]; degenerate: boolean[][] };

function central_difference(values: V3[], index: number): V3 {
  const last = values.length - 1;
  if (index === 0) return v3_sub(values[1], values[0]);
  if (index === last) return v3_sub(values[last], values[last - 1]);
  return v3_scale(v3_sub(values[index + 1], values[index - 1]), 0.5);
}

function evaluate_corner_field(grid: SurfaceGrid, eye: V3): CornerField {
  const f: number[][] = [];
  const s_u: V3[][] = [];
  const s_v: V3[][] = [];
  const normal_lengths: number[][] = [];
  let max_normal_length = 0;
  for (let i = 0; i <= grid.columns; i++) {
    f.push([]);
    s_u.push([]);
    s_v.push([]);
    normal_lengths.push([]);
    for (let j = 0; j <= grid.rows; j++) {
      const row_along_u = grid.positions.map((column) => column[j]);
      const partial_u = central_difference(row_along_u, i);
      const partial_v = central_difference(grid.positions[i], j);
      const normal = v3_cross(partial_u, partial_v);
      const normal_length = v3_length(normal);
      max_normal_length = Math.max(max_normal_length, normal_length);
      f[i].push(v3_dot(normal, v3_sub(eye, grid.positions[i][j])));
      s_u[i].push(partial_u);
      s_v[i].push(partial_v);
      normal_lengths[i].push(normal_length);
    }
  }
  const degenerate = normal_lengths.map((column) => column.map((length) => length < DEGENERATE_NORMAL_FRACTION * max_normal_length));
  return { f, s_u, s_v, degenerate };
}

function edge_crossing(field: CornerField, from: GridPoint, to: GridPoint, edge_id: string): Crossing | null {
  const f0 = field.f[from.i][from.j];
  const f1 = field.f[to.i][to.j];
  if ((f0 < 0) === (f1 < 0)) return null;
  return { edge_id, from, to, s: f0 / (f0 - f1) };
}

// Marching squares: one segment per cell between its two sign-flip edges.
// Four flips (the saddle) are paired in edge order; the double-crossing
// edge case is out of scope for now.
function marching_squares(grid: SurfaceGrid, field: CornerField): Segment[] {
  const segments: Segment[] = [];
  for (let j = 0; j < grid.rows; j++) {
    for (let i = 0; i < grid.columns; i++) {
      const corners: GridPoint[] = [{ i, j }, { i: i + 1, j }, { i, j: j + 1 }, { i: i + 1, j: j + 1 }];
      if (corners.some((corner) => field.degenerate[corner.i][corner.j])) continue;
      const candidates = [
        edge_crossing(field, corners[0], corners[1], `h${i},${j}`),
        edge_crossing(field, corners[2], corners[3], `h${i},${j + 1}`),
        edge_crossing(field, corners[0], corners[2], `v${i},${j}`),
        edge_crossing(field, corners[1], corners[3], `v${i + 1},${j}`),
      ];
      const crossings = candidates.filter((crossing): crossing is Crossing => crossing !== null);
      for (let k = 0; k + 1 < crossings.length; k += 2) segments.push({ a: crossings[k], b: crossings[k + 1] });
    }
  }
  return segments;
}

// Walk segments end to end by shared edge id. Each id sits in at most two
// segments, so a polyline is followed by hopping to the other holder of the
// far end; open polylines start at an unshared end, closed loops anywhere.
function join_into_polylines(segments: Segment[]): Crossing[][] {
  const holders = new Map<string, number[]>();
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
function f_gradient_at_corner(field: CornerField, point: GridPoint): { u: number; v: number } {
  const f = field.f;
  const last_i = f.length - 1;
  const last_j = f[0].length - 1;
  const { i, j } = point;
  const f_u = i === 0 ? f[1][j] - f[0][j] : i === last_i ? f[last_i][j] - f[last_i - 1][j] : (f[i + 1][j] - f[i - 1][j]) / 2;
  const f_v = j === 0 ? f[i][1] - f[i][0] : j === last_j ? f[i][last_j] - f[i][last_j - 1] : (f[i][j + 1] - f[i][j - 1]) / 2;
  return { u: f_u, v: f_v };
}

type Knot = { position: V3; tangent: V3 };

// Position and 3D contour tangent at a crossing. The tangent is the gradient
// of f rotated 90° in (u, v) (the contour runs along the level set), pushed
// to 3D through the surface partials; sign fixed later against the walk.
function knot_at_crossing(grid: SurfaceGrid, field: CornerField, crossing: Crossing): Knot {
  const { from, to, s } = crossing;
  const position = v3_lerp(grid.positions[from.i][from.j], grid.positions[to.i][to.j], s);
  const gradient_from = f_gradient_at_corner(field, from);
  const gradient_to = f_gradient_at_corner(field, to);
  const f_u = lerp_number(gradient_from.u, gradient_to.u, s);
  const f_v = lerp_number(gradient_from.v, gradient_to.v, s);
  const s_u = v3_lerp(field.s_u[from.i][from.j], field.s_u[to.i][to.j], s);
  const s_v = v3_lerp(field.s_v[from.i][from.j], field.s_v[to.i][to.j], s);
  const tangent = v3_add(v3_scale(s_u, -f_v), v3_scale(s_v, f_u));
  return { position, tangent };
}

function oriented_unit_tangent(knot: Knot, along: V3): V3 {
  if (v3_length(knot.tangent) < 1e-9) return v3_normalize(along);
  const unit = v3_normalize(knot.tangent);
  return v3_dot(unit, along) < 0 ? v3_scale(unit, -1) : unit;
}

// Cubic Hermite per cell segment: endpoints at the crossings, handles along
// the contour tangents, chord / 3 long.
function fit_bezier_chain(knots: Knot[]): ContourChain {
  const chain: ContourChain = [];
  for (let k = 0; k + 1 < knots.length; k++) {
    const p0 = knots[k].position;
    const p3 = knots[k + 1].position;
    const chord_vector = v3_sub(p3, p0);
    const chord = v3_length(chord_vector);
    if (chord < 1e-9) continue;
    const t0 = oriented_unit_tangent(knots[k], chord_vector);
    const t3 = oriented_unit_tangent(knots[k + 1], chord_vector);
    chain.push({ p0, p1: v3_add(p0, v3_scale(t0, chord / 3)), p2: v3_sub(p3, v3_scale(t3, chord / 3)), p3 });
  }
  return chain;
}

export function extract_contour_chains(grid: SurfaceGrid, eye: V3): ContourChain[] {
  const field = evaluate_corner_field(grid, eye);
  const segments = marching_squares(grid, field);
  const polylines = join_into_polylines(segments);
  return polylines
    .map((polyline) => fit_bezier_chain(polyline.map((crossing) => knot_at_crossing(grid, field, crossing))))
    .filter((chain) => chain.length > 0);
}
