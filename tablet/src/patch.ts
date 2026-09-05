// Patch fill (plan-tablet-multi-select-patch.md). A patch stores only a set
// of strokes; everything else is derived here every frame, so the fill
// follows edits live. The strokes are chained head-to-tail by their shared
// vertices into a closed loop, the loop is cut into logical *sides* at its
// corners — a corner is a junction with no smooth knot between the two
// strokes meeting there, so a knotted chain reads as one side — and the side
// count picks the fill: 2 sides = ruled loft, 4 sides = Coons, 3 sides =
// Coons with the fourth side collapsed to a corner. Two strokes that don't
// close a loop are the one exception: a plain loft between them.
// 5+ sides: TODO midpoint subdivision (plan Q7), drawn as nothing now.
// Shading is baked CPU-side per vertex (headlight: brightness from the
// surface normal vs the camera forward, two-sided), which fits the existing
// position+color pipeline.

import { OrbitCamera, camera_basis } from "./camera";
import { Patch, Stroke, TabletDocument, VertexId, bezier_point, bezier_tangent, smooth_knots_at_vertex, stroke_by_id, stroke_control_points, vertex_position } from "./document";
import { V3, v3_add, v3_cross, v3_dot, v3_length, v3_lerp, v3_normalize, v3_scale, v3_sub } from "./math";

const LOFT_SAMPLES_ALONG_RAILS = 24;
const LOFT_ROWS_ACROSS = 4;
const COONS_GRID = 16; // grid cells per side
const SURFACE_AMBIENT = 0.35;

type OrientedStroke = { stroke: Stroke; reversed: boolean };
// One logical side of the loop: a run of strokes between two corners, in
// loop order, each oriented to run forward along the loop.
type Side = OrientedStroke[];
type Color = { r: number; g: number; b: number };

function start_vertex(oriented: OrientedStroke): VertexId {
  return oriented.reversed ? oriented.stroke.p3_vertex : oriented.stroke.p0_vertex;
}

function end_vertex(oriented: OrientedStroke): VertexId {
  return oriented.reversed ? oriented.stroke.p0_vertex : oriented.stroke.p3_vertex;
}

// Chain the strokes head-to-tail by shared vertex ids into a closed loop,
// starting from the first stroke as drawn. Null if some stroke doesn't
// connect or the chain doesn't close.
function chain_into_loop(strokes: Stroke[]): OrientedStroke[] | null {
  const loop: OrientedStroke[] = [{ stroke: strokes[0], reversed: false }];
  const remaining = strokes.slice(1);
  while (remaining.length > 0) {
    const loop_end = end_vertex(loop[loop.length - 1]);
    const index = remaining.findIndex((stroke) => stroke.p0_vertex === loop_end || stroke.p3_vertex === loop_end);
    if (index === -1) return null;
    const stroke = remaining.splice(index, 1)[0];
    loop.push({ stroke, reversed: stroke.p3_vertex === loop_end });
  }
  return end_vertex(loop[loop.length - 1]) === start_vertex(loop[0]) ? loop : null;
}

function strokes_are_smooth_at(tablet_document: TabletDocument, vertex: VertexId, a: Stroke, b: Stroke): boolean {
  return smooth_knots_at_vertex(tablet_document, vertex).some(
    (knot) => (knot.stroke_a === a.id && knot.stroke_b === b.id) || (knot.stroke_a === b.id && knot.stroke_b === a.id),
  );
}

// Cut the loop into sides at its corners. The first side starts at the first
// corner found so no side straddles the array seam. Null if the loop has no
// corner at all (a closed smooth ring has no sides to blend between).
function cut_loop_into_sides(loop: OrientedStroke[], tablet_document: TabletDocument): Side[] | null {
  const count = loop.length;
  const is_corner_after = (i: number): boolean => {
    const next = loop[(i + 1) % count];
    return !strokes_are_smooth_at(tablet_document, end_vertex(loop[i]), loop[i].stroke, next.stroke);
  };
  let first_corner = -1;
  for (let i = 0; i < count; i++) {
    if (is_corner_after(i)) {
      first_corner = i;
      break;
    }
  }
  if (first_corner === -1) return null;
  const sides: Side[] = [];
  let side: Side = [];
  for (let step = 1; step <= count; step++) {
    const i = (first_corner + step) % count;
    side.push(loop[i]);
    if (is_corner_after(i)) {
      sides.push(side);
      side = [];
    }
  }
  return sides;
}

// Position and forward tangent at parameter u in [0, 1] along a whole side,
// each stroke taking an equal share of the parameter range.
function side_point(side: Side, tablet_document: TabletDocument, u: number): { position: V3; tangent: V3 } {
  const scaled = Math.min(u * side.length, side.length - 1e-9);
  const oriented = side[Math.floor(scaled)];
  const local = scaled - Math.floor(scaled);
  const t = oriented.reversed ? 1 - local : local;
  const points = stroke_control_points(oriented.stroke, tablet_document);
  const tangent = bezier_tangent(points, t);
  return { position: bezier_point(points, t), tangent: oriented.reversed ? v3_scale(tangent, -1) : tangent };
}

function sample_side(side: Side, tablet_document: TabletDocument, sample_count: number): { position: V3; tangent: V3 }[] {
  const samples = [];
  for (let i = 0; i <= sample_count; i++) samples.push(side_point(side, tablet_document, i / sample_count));
  return samples;
}

function reversed_side(side: Side): Side {
  return side.map((oriented) => ({ stroke: oriented.stroke, reversed: !oriented.reversed })).reverse();
}

// Two detached rails drawn in opposite directions would twist the loft;
// detect by comparing endpoint pairings and reverse rail B when crossed.
function rails_are_crossed(rail_a: Stroke, rail_b: Stroke, tablet_document: TabletDocument): boolean {
  const a_start = vertex_position(tablet_document, rail_a.p0_vertex);
  const a_end = vertex_position(tablet_document, rail_a.p3_vertex);
  const b_start = vertex_position(tablet_document, rail_b.p0_vertex);
  const b_end = vertex_position(tablet_document, rail_b.p3_vertex);
  const straight = v3_length(v3_sub(a_start, b_start)) + v3_length(v3_sub(a_end, b_end));
  const crossed = v3_length(v3_sub(a_start, b_end)) + v3_length(v3_sub(a_end, b_start));
  return crossed < straight;
}

function brightness_of_normal(cross: V3, camera_forward: V3): number {
  if (v3_length(cross) < 1e-9) return 1;
  return SURFACE_AMBIENT + (1 - SURFACE_AMBIENT) * Math.abs(v3_dot(v3_normalize(cross), camera_forward));
}

function push_shaded_vertex(position: V3, brightness: number, color: Color, out: number[]): void {
  out.push(position.x, position.y, position.z, color.r * brightness, color.g * brightness, color.b * brightness);
}

// Ruled surface P(u, v) = lerp(A(u), B(u), v), both sides running the same
// way — normals from the analytic partials.
function append_loft_mesh(side_a: Side, side_b: Side, tablet_document: TabletDocument, camera: OrbitCamera, color: Color, out: number[]): void {
  const samples_a = sample_side(side_a, tablet_document, LOFT_SAMPLES_ALONG_RAILS);
  const samples_b = sample_side(side_b, tablet_document, LOFT_SAMPLES_ALONG_RAILS);
  const camera_forward = camera_basis(camera).forward;
  const shaded = (u: number, v: number): { position: V3; brightness: number } => {
    const a = samples_a[u];
    const b = samples_b[u];
    const t = v / LOFT_ROWS_ACROSS;
    const du = v3_lerp(a.tangent, b.tangent, t);
    const dv = v3_sub(b.position, a.position);
    return { position: v3_lerp(a.position, b.position, t), brightness: brightness_of_normal(v3_cross(du, dv), camera_forward) };
  };
  for (let v = 0; v < LOFT_ROWS_ACROSS; v++) {
    for (let u = 0; u < LOFT_SAMPLES_ALONG_RAILS; u++) {
      const corner_00 = shaded(u, v);
      const corner_10 = shaded(u + 1, v);
      const corner_11 = shaded(u + 1, v + 1);
      const corner_01 = shaded(u, v + 1);
      for (const corner of [corner_00, corner_10, corner_11, corner_00, corner_11, corner_01]) {
        push_shaded_vertex(corner.position, corner.brightness, color, out);
      }
    }
  }
}

// Bilinearly blended Coons surface over four sides in loop order. Sample-based
// (no bicubic fit): each side is presampled on the grid resolution and blended
// directly, so the patch hugs the drawn boundaries exactly at the rims. Sides
// share their corner vertices (chained by id), so the corners are exact.
// Three sides: the left side is a constant point at the bottom-left corner,
// which the blend degenerates into a triangle fan there (zero-area cells,
// normal falls back to flat shading).
function append_coons_mesh(sides: Side[], tablet_document: TabletDocument, camera: OrbitCamera, color: Color, out: number[]): void {
  // Loop traversal order: bottom (s 0→1), right (t 0→1), top and left run
  // backwards along the loop, so index from the far end when reading them.
  const positions = (side: Side): V3[] => sample_side(side, tablet_document, COONS_GRID).map((sample) => sample.position);
  const bottom = positions(sides[0]);
  const right = positions(sides[1]);
  const top_backwards = positions(sides[2]);
  const left_backwards = sides.length === 4 ? positions(sides[3]) : new Array<V3>(COONS_GRID + 1).fill(bottom[0]);
  const top = (i: number): V3 => top_backwards[COONS_GRID - i];
  const left = (j: number): V3 => left_backwards[COONS_GRID - j];
  const corner_00 = bottom[0];
  const corner_10 = bottom[COONS_GRID];
  const corner_11 = right[COONS_GRID];
  const corner_01 = top(0);

  const surface_point = (i: number, j: number): V3 => {
    const s = i / COONS_GRID;
    const t = j / COONS_GRID;
    const ruled = v3_add(v3_lerp(bottom[i], top(i), t), v3_lerp(left(j), right[j], s));
    const bilinear = v3_add(
      v3_add(v3_scale(corner_00, (1 - s) * (1 - t)), v3_scale(corner_10, s * (1 - t))),
      v3_add(v3_scale(corner_01, (1 - s) * t), v3_scale(corner_11, s * t)),
    );
    return v3_sub(ruled, bilinear);
  };

  const camera_forward = camera_basis(camera).forward;
  const push_triangle = (a: V3, b: V3, c: V3) => {
    const brightness = brightness_of_normal(v3_cross(v3_sub(b, a), v3_sub(c, a)), camera_forward);
    for (const vertex of [a, b, c]) push_shaded_vertex(vertex, brightness, color, out);
  };
  for (let j = 0; j < COONS_GRID; j++) {
    for (let i = 0; i < COONS_GRID; i++) {
      const point_00 = surface_point(i, j);
      const point_10 = surface_point(i + 1, j);
      const point_11 = surface_point(i + 1, j + 1);
      const point_01 = surface_point(i, j + 1);
      push_triangle(point_00, point_10, point_11);
      push_triangle(point_00, point_11, point_01);
    }
  }
}

// The fill a patch resolves to right now; null = nothing drawable (loop
// doesn't close, side count unsupported). Exported for the node check.
export type PatchFill = { kind: "loft"; side_a: Side; side_b: Side } | { kind: "coons"; sides: Side[] };

export function resolve_patch_fill(patch: Patch, tablet_document: TabletDocument): PatchFill | null {
  const strokes = patch.strokes.map((id) => stroke_by_id(tablet_document, id));
  const loop = chain_into_loop(strokes);
  if (loop === null) {
    if (strokes.length !== 2) return null;
    const crossed = rails_are_crossed(strokes[0], strokes[1], tablet_document);
    return { kind: "loft", side_a: [{ stroke: strokes[0], reversed: false }], side_b: [{ stroke: strokes[1], reversed: crossed }] };
  }
  const sides = cut_loop_into_sides(loop, tablet_document);
  if (sides === null) return null;
  // A lens: both sides run corner A → corner B once the second is reversed.
  if (sides.length === 2) return { kind: "loft", side_a: sides[0], side_b: reversed_side(sides[1]) };
  if (sides.length === 3 || sides.length === 4) return { kind: "coons", sides };
  return null;
}

export function append_patch_mesh(patch: Patch, tablet_document: TabletDocument, camera: OrbitCamera, color: Color, out: number[]): void {
  const fill = resolve_patch_fill(patch, tablet_document);
  if (fill === null) return;
  if (fill.kind === "loft") append_loft_mesh(fill.side_a, fill.side_b, tablet_document, camera, color, out);
  else append_coons_mesh(fill.sides, tablet_document, camera, color, out);
}
