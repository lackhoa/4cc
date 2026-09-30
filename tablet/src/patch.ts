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

import { OrbitCamera, camera_basis, camera_eye, camera_world_to_screen } from "./camera";
import { Patch, Stroke, StrokeId, TabletDocument, VertexId, bezier_point, bezier_tangent, pins_on_stroke, smooth_knots_at_vertex, stroke_by_id, stroke_control_points, vertex_position } from "./document";
import { V2, V3, v3_add, v3_cross, v3_dot, v3_length, v3_lerp, v3_normalize, v3_scale, v3_sub } from "./math";
import { VertexSink, push_vertex } from "./vertex_sink";

const LOFT_SAMPLES_ALONG_RAILS = 24;
const LOFT_ROWS_ACROSS = 4;
const COONS_GRID = 16; // grid cells per side
const SURFACE_AMBIENT = 0.35;

// A vertex where the loop may switch from one stroke to another: one of the
// stroke's endpoints (t = 0 / 1) or a vertex pinned to it
// (plan-patch-subcurve-boundary.md Q1).
type Junction = { vertex: VertexId; t: number };
// The piece of a stroke the loop runs along, from the `start` junction to the
// `end` junction — the whole cubic when both are endpoints, a sub-curve when
// either is a pin. `reversed` = the piece runs against the stroke's own
// direction (end.t < start.t).
type OrientedStroke = { stroke: Stroke; start: Junction; end: Junction; reversed: boolean };
// One logical side of the loop: a run of strokes between two corners, in
// loop order, each oriented to run forward along the loop.
type Side = OrientedStroke[];
type Color = { r: number; g: number; b: number };

function oriented_between(stroke: Stroke, start: Junction, end: Junction): OrientedStroke {
  return { stroke, start, end, reversed: end.t < start.t };
}

function whole_stroke(stroke: Stroke, reversed: boolean): OrientedStroke {
  const p0: Junction = { vertex: stroke.p0_vertex, t: 0 };
  const p3: Junction = { vertex: stroke.p3_vertex, t: 1 };
  return reversed ? oriented_between(stroke, p3, p0) : oriented_between(stroke, p0, p3);
}

// Endpoints first so whole strokes are preferred by the search below.
function stroke_junctions(stroke: Stroke, tablet_document: TabletDocument): Junction[] {
  const junctions: Junction[] = [{ vertex: stroke.p0_vertex, t: 0 }, { vertex: stroke.p3_vertex, t: 1 }];
  for (const pin of pins_on_stroke(tablet_document, stroke.id)) junctions.push({ vertex: pin.vertex, t: pin.t });
  return junctions;
}

function start_vertex(oriented: OrientedStroke): VertexId {
  return oriented.start.vertex;
}

function end_vertex(oriented: OrientedStroke): VertexId {
  return oriented.end.vertex;
}

// Chain the strokes head-to-tail by shared vertex ids into a closed loop,
// starting from the first stroke as drawn. A stroke may be entered or left at
// any of its junctions, so a loop can use just the piece of a stroke between two
// of them (Q1); each stroke is used once. Depth-first over (stroke, entry,
// exit) — a patch has a handful of strokes — returning the first closed
// chain, whole strokes tried before sub-curves (Q2). Null if some stroke
// doesn't connect or no chain closes.
function chain_into_loop(strokes: Stroke[], tablet_document: TabletDocument): OrientedStroke[] | null {
  const search = (loop: OrientedStroke[], remaining: Stroke[]): OrientedStroke[] | null => {
    const loop_end = end_vertex(loop[loop.length - 1]);
    if (remaining.length === 0) return loop_end === start_vertex(loop[0]) ? loop : null;
    for (let i = 0; i < remaining.length; i++) {
      const stroke = remaining[i];
      const junctions = stroke_junctions(stroke, tablet_document);
      const entry = junctions.find((junction) => junction.vertex === loop_end);
      if (entry === undefined) continue;
      const rest = remaining.filter((_, j) => j !== i);
      for (const exit of junctions) {
        if (exit === entry) continue;
        const found = search([...loop, oriented_between(stroke, entry, exit)], rest);
        if (found !== null) return found;
      }
    }
    return null;
  };
  const first = strokes[0];
  const first_junctions = stroke_junctions(first, tablet_document);
  for (const start of first_junctions) {
    for (const end of first_junctions) {
      if (start === end) continue;
      const found = search([oriented_between(first, start, end)], strokes.slice(1));
      if (found !== null) return found;
    }
  }
  return null;
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
// each stroke piece taking an equal share of the parameter range (Q4).
function side_point(side: Side, tablet_document: TabletDocument, u: number): { position: V3; tangent: V3 } {
  const scaled = Math.min(u * side.length, side.length - 1e-9);
  const oriented = side[Math.floor(scaled)];
  const local = scaled - Math.floor(scaled);
  const t = oriented.start.t + (oriented.end.t - oriented.start.t) * local;
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
  return side.map((oriented) => oriented_between(oriented.stroke, oriented.end, oriented.start)).reverse();
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

function push_shaded_vertex(position: V3, brightness: number, color: Color, out: VertexSink): void {
  push_vertex(out, position, { r: color.r * brightness, g: color.g * brightness, b: color.b * brightness });
}

// Ruled surface P(u, v) = lerp(A(u), B(u), v), both sides running the same
// way — normals from the analytic partials.
function append_loft_mesh(side_a: Side, side_b: Side, tablet_document: TabletDocument, camera: OrbitCamera, color: Color, out: VertexSink): void {
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

// A patch surface sampled on a regular (u, v) grid: positions[i][j] is the
// point at u = i / columns, v = j / rows. Shared by the mesh and the contour
// extraction (contour.ts), so both see the exact same surface.
export type SurfaceGrid = { columns: number; rows: number; positions: V3[][] };

// Bilinearly blended Coons surface over four sides in loop order. Sample-based
// (no bicubic fit): each side is presampled on the grid resolution and blended
// directly, so the patch hugs the drawn boundaries exactly at the rims. Sides
// share their corner vertices (chained by id), so the corners are exact.
// Three sides: the left side is a constant point at the bottom-left corner,
// which the blend degenerates into a triangle fan there (zero-area cells,
// normal falls back to flat shading).
function coons_surface_grid(sides: Side[], tablet_document: TabletDocument): SurfaceGrid {
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

  const grid_positions: V3[][] = [];
  for (let i = 0; i <= COONS_GRID; i++) {
    const column: V3[] = [];
    for (let j = 0; j <= COONS_GRID; j++) column.push(surface_point(i, j));
    grid_positions.push(column);
  }
  return { columns: COONS_GRID, rows: COONS_GRID, positions: grid_positions };
}

// The loft as a grid: rows are the lerp steps between the two rails.
function loft_surface_grid(side_a: Side, side_b: Side, tablet_document: TabletDocument): SurfaceGrid {
  const samples_a = sample_side(side_a, tablet_document, LOFT_SAMPLES_ALONG_RAILS);
  const samples_b = sample_side(side_b, tablet_document, LOFT_SAMPLES_ALONG_RAILS);
  const grid_positions: V3[][] = [];
  for (let u = 0; u <= LOFT_SAMPLES_ALONG_RAILS; u++) {
    const column: V3[] = [];
    for (let v = 0; v <= LOFT_ROWS_ACROSS; v++) column.push(v3_lerp(samples_a[u].position, samples_b[u].position, v / LOFT_ROWS_ACROSS));
    grid_positions.push(column);
  }
  return { columns: LOFT_SAMPLES_ALONG_RAILS, rows: LOFT_ROWS_ACROSS, positions: grid_positions };
}

function append_grid_mesh(grid: SurfaceGrid, camera: OrbitCamera, color: Color, out: VertexSink): void {
  const camera_forward = camera_basis(camera).forward;
  const push_triangle = (a: V3, b: V3, c: V3) => {
    const brightness = brightness_of_normal(v3_cross(v3_sub(b, a), v3_sub(c, a)), camera_forward);
    for (const vertex of [a, b, c]) push_shaded_vertex(vertex, brightness, color, out);
  };
  for (let j = 0; j < grid.rows; j++) {
    for (let i = 0; i < grid.columns; i++) {
      const point_00 = grid.positions[i][j];
      const point_10 = grid.positions[i + 1][j];
      const point_11 = grid.positions[i + 1][j + 1];
      const point_01 = grid.positions[i][j + 1];
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
  const loop = chain_into_loop(strokes, tablet_document);
  if (loop === null) {
    // Detached rails: whole strokes only, there is no shared vertex to say
    // where a sub-curve would start (Q5).
    if (strokes.length !== 2) return null;
    const crossed = rails_are_crossed(strokes[0], strokes[1], tablet_document);
    return { kind: "loft", side_a: [whole_stroke(strokes[0], false)], side_b: [whole_stroke(strokes[1], crossed)] };
  }
  const sides = cut_loop_into_sides(loop, tablet_document);
  if (sides === null) return null;
  // A lens: both sides run corner A → corner B once the second is reversed.
  if (sides.length === 2) return { kind: "loft", side_a: sides[0], side_b: reversed_side(sides[1]) };
  if (sides.length === 3 || sides.length === 4) return { kind: "coons", sides };
  return null;
}

// Null when the patch has no drawable fill.
export function patch_surface_grid(patch: Patch, tablet_document: TabletDocument): SurfaceGrid | null {
  const fill = resolve_patch_fill(patch, tablet_document);
  if (fill === null) return null;
  if (fill.kind === "loft") return loft_surface_grid(fill.side_a, fill.side_b, tablet_document);
  return coons_surface_grid(fill.sides, tablet_document);
}

export function append_patch_mesh(patch: Patch, tablet_document: TabletDocument, camera: OrbitCamera, color: Color, out: VertexSink): void {
  const fill = resolve_patch_fill(patch, tablet_document);
  if (fill === null) return;
  // The loft keeps its analytic-partial shading; Coons shades per triangle.
  if (fill.kind === "loft") append_loft_mesh(fill.side_a, fill.side_b, tablet_document, camera, color, out);
  else append_grid_mesh(coons_surface_grid(fill.sides, tablet_document), camera, color, out);
}

// Locks (plan-patch-subcurve-boundary.md Q3/Q7): a patch owns its boundary.
// While a patch lists a stroke, the stroke can't be deleted or joined; while
// the resolved loop passes through a pinned vertex, that vertex can't be
// unpinned (the sub-curve would lose its end). Both derived per query, never
// stored: delete the patch and the lock is gone.

// The vertices at the ends of every stroke piece the fill uses (the loop's
// junctions; a detached loft's rail endpoints); empty when the patch has no fill.
export function patch_junction_vertices(patch: Patch, tablet_document: TabletDocument): VertexId[] {
  const fill = resolve_patch_fill(patch, tablet_document);
  if (fill === null) return [];
  const sides = fill.kind === "loft" ? [fill.side_a, fill.side_b] : fill.sides;
  const vertices: VertexId[] = [];
  for (const side of sides) for (const oriented of side) vertices.push(oriented.start.vertex, oriented.end.vertex);
  return vertices;
}

export function stroke_bounds_a_patch(tablet_document: TabletDocument, stroke_id: StrokeId): boolean {
  return tablet_document.patches.some((patch) => patch.strokes.includes(stroke_id));
}

export function pin_is_locked(tablet_document: TabletDocument, vertex: VertexId): boolean {
  return tablet_document.patches.some((patch) => patch_junction_vertices(patch, tablet_document).includes(vertex));
}

// Fill hit test (Q10): the index of the patch whose surface is under the tap,
// nearest to the eye when several overlap, or null. Every triangle of the
// per-frame surface grid is projected and tested in 2D, so what you see is
// what you pick. Callers try vertices and strokes first.
export function pick_patch(tablet_document: TabletDocument, camera: OrbitCamera, screen: V2, canvas: HTMLCanvasElement): number | null {
  const eye = camera_eye(camera);
  const project = (world: V3): V2 | null => camera_world_to_screen(camera, world, canvas.clientWidth, canvas.clientHeight);
  const side = (a: V2, b: V2): number => (b.x - a.x) * (screen.y - a.y) - (b.y - a.y) * (screen.x - a.x);
  const contains = (a: V2, b: V2, c: V2): boolean => {
    const ab = side(a, b), bc = side(b, c), ca = side(c, a);
    return (ab >= 0 && bc >= 0 && ca >= 0) || (ab <= 0 && bc <= 0 && ca <= 0);
  };
  let best_index: number | null = null;
  let best_distance = Infinity;
  tablet_document.patches.forEach((patch, index) => {
    const grid = patch_surface_grid(patch, tablet_document);
    if (grid === null) return;
    const test = (p: V3, q: V3, r: V3): void => {
      const a = project(p), b = project(q), c = project(r);
      if (a === null || b === null || c === null || !contains(a, b, c)) return;
      const distance = v3_length(v3_sub(v3_scale(v3_add(v3_add(p, q), r), 1 / 3), eye));
      if (distance < best_distance) {
        best_distance = distance;
        best_index = index;
      }
    };
    for (let j = 0; j < grid.rows; j++) {
      for (let i = 0; i < grid.columns; i++) {
        test(grid.positions[i][j], grid.positions[i + 1][j], grid.positions[i + 1][j + 1]);
        test(grid.positions[i][j], grid.positions[i + 1][j + 1], grid.positions[i][j + 1]);
      }
    }
  });
  return best_index;
}
