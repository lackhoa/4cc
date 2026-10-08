// Fits a template drawing to landmarks: every landmark names a point of the template and
// where that point must land (src/loomis_girl_plate.ts).
//
// Method: one smooth warp of space, a 3D thin-plate spline (docs/interactive-landmark-warp.html
// explains the 2D version). warp(p) = affine(p) + sum over landmarks of weight_i * U(|p - landmark_i|),
// with U(r) = r. Per axis there are n weights and 4 affine numbers, found from n + 4 linear
// equations: n say "landmark i lands on its target", 4 say the weights carry no push and no
// tilt. Every paired landmark is entered on both sides of the face, so the warp is
// mirror-symmetric and the midline stays on x = 0.
//
// The warp is applied to every vertex; every stroke becomes the cubic closest to its warped
// self (warped_stroke_control_points).
import {
  StrokeControlPoints, TabletDocument, bezier_point, enforce_midline, stroke_control_points, stroke_handles_from_control_points,
} from "./document";
import { update_pinned_vertex_positions } from "./patch";
import { FitLandmarksFile, fit_landmark_mark_world_position, template_point_world_position, template_position_is_on_midline } from "./loomis_girl_plate";
import { V3, v3, v3_add, v3_cross, v3_dot, v3_length, v3_normalize, v3_scale, v3_sub } from "./math";

type LandmarkPair = { template_position: V3; target_position: V3 };

// Solves a * x = b in place by Gaussian elimination with partial pivoting; b holds one
// column per right-hand side. Null when the system has no single solution.
function solve_linear_system(a: number[][], b: number[][]): number[][] | null {
  const size = a.length;
  for (let column = 0; column < size; column++) {
    let pivot_row = column;
    for (let row = column + 1; row < size; row++) if (Math.abs(a[row][column]) > Math.abs(a[pivot_row][column])) pivot_row = row;
    if (Math.abs(a[pivot_row][column]) < 1e-12) return null;
    [a[column], a[pivot_row]] = [a[pivot_row], a[column]];
    [b[column], b[pivot_row]] = [b[pivot_row], b[column]];
    for (let row = column + 1; row < size; row++) {
      const factor = a[row][column] / a[column][column];
      for (let k = column; k < size; k++) a[row][k] -= factor * a[column][k];
      for (let k = 0; k < b[row].length; k++) b[row][k] -= factor * b[column][k];
    }
  }
  const x: number[][] = b.map((row) => row.map(() => 0));
  for (let row = size - 1; row >= 0; row--) {
    for (let k = 0; k < b[row].length; k++) {
      let sum = b[row][k];
      for (let column = row + 1; column < size; column++) sum -= a[row][column] * x[column][k];
      x[row][k] = sum / a[row][row];
    }
  }
  return x;
}

// weights[i] = the three weights of landmark i (x, y, z axis). affine[0] = shift,
// affine[1..3] = what one unit of x, y, z adds to the result.
type ThinPlateWarp = { landmark_positions: V3[]; weights: V3[]; affine: V3[] };

function thin_plate_kernel(distance: number): number {
  return distance;
}

// stiffness 0: every landmark lands exactly on its target. Larger: the warp bends less
// and lets landmarks miss; very large leaves only the affine part (the best overall
// scale, shear and shift).
// NOTE: the stiffness is SUBTRACTED from the diagonal. With U(r) = r the kernel block is
// negative on the weights the 4 side equations allow, so subtracting moves it away from
// singular; adding would move it toward singular.
function thin_plate_warp_from_landmarks(pairs: LandmarkPair[], stiffness: number): ThinPlateWarp | null {
  const n = pairs.length;
  const size = n + 4;
  const a: number[][] = Array.from({ length: size }, () => new Array<number>(size).fill(0));
  const b: number[][] = Array.from({ length: size }, () => [0, 0, 0]);
  for (let i = 0; i < n; i++) {
    const p = pairs[i].template_position;
    for (let j = 0; j < n; j++) a[i][j] = thin_plate_kernel(v3_length(v3_sub(p, pairs[j].template_position)));
    a[i][i] -= stiffness;
    const affine_terms = [1, p.x, p.y, p.z];
    for (let k = 0; k < 4; k++) {
      a[i][n + k] = affine_terms[k];
      a[n + k][i] = affine_terms[k];
    }
    b[i] = [pairs[i].target_position.x, pairs[i].target_position.y, pairs[i].target_position.z];
  }
  const solution = solve_linear_system(a, b);
  if (solution === null) return null;
  const as_v3 = (row: number[]): V3 => v3(row[0], row[1], row[2]);
  return {
    landmark_positions: pairs.map((pair) => pair.template_position),
    weights: solution.slice(0, n).map(as_v3),
    affine: solution.slice(n).map(as_v3),
  };
}

function thin_plate_warp_point(warp: ThinPlateWarp, p: V3): V3 {
  const affine_terms = [1, p.x, p.y, p.z];
  const result = v3(0, 0, 0);
  for (let k = 0; k < 4; k++) {
    result.x += warp.affine[k].x * affine_terms[k];
    result.y += warp.affine[k].y * affine_terms[k];
    result.z += warp.affine[k].z * affine_terms[k];
  }
  for (let i = 0; i < warp.landmark_positions.length; i++) {
    const u = thin_plate_kernel(v3_length(v3_sub(p, warp.landmark_positions[i])));
    result.x += warp.weights[i].x * u;
    result.y += warp.weights[i].y * u;
    result.z += warp.weights[i].z * u;
  }
  return result;
}

// The warped image of a planar cubic is neither planar nor a cubic, and a document stroke
// must be both. This returns the planar cubic closest to it:
//  1. the endpoints are warped;
//  2. the plane is the one through the new chord that the warped curve bulges along most;
//  3. the two inner control points are chosen by least squares, so the curve at parameter
//     t is as near as possible to the warp of the template curve at the same t, flattened
//     onto that plane. Keeping t matters: a pinned vertex rides its host stroke at a fixed t.
function warped_stroke_control_points(warp: ThinPlateWarp, template_points: StrokeControlPoints): StrokeControlPoints {
  const p0 = thin_plate_warp_point(warp, template_points.p0);
  const p3 = thin_plate_warp_point(warp, template_points.p3);
  const sample_count = 32;
  const sample_parameters: number[] = [];
  const warped_samples: V3[] = [];
  for (let i = 1; i < sample_count; i++) {
    sample_parameters.push(i / sample_count);
    warped_samples.push(thin_plate_warp_point(warp, bezier_point(template_points, i / sample_count)));
  }

  // Plane: every sample's offset from the chord is split along two axes square to the
  // chord; the in-plane direction is the one that collects the most squared offset.
  const chord = v3_normalize(v3_sub(p3, p0));
  const axis_a = v3_normalize(v3_cross(chord, Math.abs(chord.x) < 0.9 ? v3(1, 0, 0) : v3(0, 1, 0)));
  const axis_b = v3_cross(chord, axis_a);
  let sum_a_a = 0, sum_a_b = 0, sum_b_b = 0;
  for (const sample of warped_samples) {
    const a = v3_dot(v3_sub(sample, p0), axis_a), b = v3_dot(v3_sub(sample, p0), axis_b);
    sum_a_a += a * a;
    sum_a_b += a * b;
    sum_b_b += b * b;
  }
  const in_plane_angle = 0.5 * Math.atan2(2 * sum_a_b, sum_a_a - sum_b_b);
  const in_plane_direction = v3_add(v3_scale(axis_a, Math.cos(in_plane_angle)), v3_scale(axis_b, Math.sin(in_plane_angle)));
  const flatten_onto_plane = (p: V3): V3 => {
    const offset = v3_sub(p, p0);
    return v3_add(p0, v3_add(v3_scale(chord, v3_dot(offset, chord)), v3_scale(in_plane_direction, v3_dot(offset, in_plane_direction))));
  };

  // Normal equations of: sum over samples of |b1 * p1 + b2 * p2 - remainder|^2.
  let sum_b1_b1 = 0, sum_b1_b2 = 0, sum_b2_b2 = 0;
  let sum_b1_remainder = v3(0, 0, 0), sum_b2_remainder = v3(0, 0, 0);
  for (let i = 0; i < warped_samples.length; i++) {
    const t = sample_parameters[i], s = 1 - t;
    const b0 = s * s * s, b1 = 3 * s * s * t, b2 = 3 * s * t * t, b3 = t * t * t;
    const remainder = v3_sub(flatten_onto_plane(warped_samples[i]), v3_add(v3_scale(p0, b0), v3_scale(p3, b3)));
    sum_b1_b1 += b1 * b1;
    sum_b1_b2 += b1 * b2;
    sum_b2_b2 += b2 * b2;
    sum_b1_remainder = v3_add(sum_b1_remainder, v3_scale(remainder, b1));
    sum_b2_remainder = v3_add(sum_b2_remainder, v3_scale(remainder, b2));
  }
  const determinant = sum_b1_b1 * sum_b2_b2 - sum_b1_b2 * sum_b1_b2;
  const p1 = v3_scale(v3_sub(v3_scale(sum_b1_remainder, sum_b2_b2), v3_scale(sum_b2_remainder, sum_b1_b2)), 1 / determinant);
  const p2 = v3_scale(v3_sub(v3_scale(sum_b2_remainder, sum_b1_b1), v3_scale(sum_b1_remainder, sum_b1_b2)), 1 / determinant);
  return { p0, p1, p2, p3 };
}

// The template warped so its landmarks land on their marks: a new document with the
// template's vertex and stroke ids. The template is only read. A landmark whose template
// point is not in the template is left out. Null when no warp exists: two landmarks on
// the same template point, or fewer than four landmarks that are not in one plane.
export function fitted_document_from_landmarks(template_document: TabletDocument, landmarks_file: FitLandmarksFile): TabletDocument | null {
  const pairs: LandmarkPair[] = [];
  for (const landmark of landmarks_file.landmarks) {
    const template_position = template_point_world_position(template_document, landmark.template_point);
    if (template_position === null) continue;
    const is_on_midline = template_position_is_on_midline(template_position);
    const target_position = fit_landmark_mark_world_position(landmark, is_on_midline);
    if (is_on_midline) {
      // A midline point drawn a hair off x = 0 is put on it, so the warp stays mirror-symmetric.
      pairs.push({ template_position: v3(0, template_position.y, template_position.z), target_position });
      continue;
    }
    pairs.push({ template_position, target_position });
    pairs.push({
      template_position: v3(-template_position.x, template_position.y, template_position.z),
      target_position: v3(-target_position.x, target_position.y, target_position.z),
    });
  }
  const warp = thin_plate_warp_from_landmarks(pairs, landmarks_file.stiffness);
  if (warp === null) {
    console.error(`landmark fit: no warp exists for these ${pairs.length} landmark positions (two on the same template point, or all in one plane)`);
    return null;
  }

  const fitted_document: TabletDocument = structuredClone(template_document);
  // Control points are read for every stroke before anything moves.
  const template_control_points = new Map<number, StrokeControlPoints>();
  for (const stroke of fitted_document.strokes) template_control_points.set(stroke.id, stroke_control_points(stroke, fitted_document));
  // The only bone is the identity, so world space = bone space.
  for (const vertex of fitted_document.vertices) vertex.position = thin_plate_warp_point(warp, vertex.position);
  for (const stroke of fitted_document.strokes) {
    const fitted = warped_stroke_control_points(warp, template_control_points.get(stroke.id)!);
    const handles = stroke_handles_from_control_points(fitted.p0, fitted.p1, fitted.p2, fitted.p3);
    stroke.d0 = handles.d0;
    stroke.d3 = handles.d3;
  }
  // Twice: a pinned vertex can be the endpoint of a stroke that hosts another pin.
  update_pinned_vertex_positions(fitted_document);
  update_pinned_vertex_positions(fitted_document);
  enforce_midline(fitted_document);
  return fitted_document;
}

// The largest distance between a vertex of `a` and the vertex of `b` with the same id, in
// world units. Infinity when the two do not hold the same vertices.
export function largest_vertex_distance_between_documents(a: TabletDocument, b: TabletDocument): number {
  if (a.vertices.length !== b.vertices.length) return Infinity;
  let largest = 0;
  for (const vertex_a of a.vertices) {
    const vertex_b = b.vertices.find((v) => v.id === vertex_a.id);
    if (vertex_b === undefined) return Infinity;
    largest = Math.max(largest, v3_length(v3_sub(vertex_a.position, vertex_b.position)));
  }
  return largest;
}
