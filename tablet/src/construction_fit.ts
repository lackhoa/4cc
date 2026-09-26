// Least-squares fits of drawable shapes to reference-mesh vertices, in mesh units (mm).
// Mirrors game/game_construction_fit.cpp (same names, same algebra) so the pages under
// tablet/pages/ reproduce the C++ numbers; plan-skull-construction-docs.md step 4.

import { V3, v3, v3_dot, v3_length, v3_sub } from "./math";

// A is n x n row-major, solved in place with partial pivoting (n <= 6 here). Returns null
// when a pivot is ~0 (degenerate points, e.g. all coplanar for a sphere).
export function solve_linear_system_gaussian(n: number, A: Float64Array, b: Float64Array): Float64Array | null {
  for (let col = 0; col < n; col++) {
    let pivot = col;
    for (let row = col + 1; row < n; row++) {
      if (Math.abs(A[row * n + col]) > Math.abs(A[pivot * n + col])) pivot = row;
    }
    if (Math.abs(A[pivot * n + col]) < 1e-9) return null;
    if (pivot !== col) {
      for (let k = 0; k < n; k++) {
        const swap = A[col * n + k]; A[col * n + k] = A[pivot * n + k]; A[pivot * n + k] = swap;
      }
      const swap = b[col]; b[col] = b[pivot]; b[pivot] = swap;
    }
    for (let row = col + 1; row < n; row++) {
      const factor = A[row * n + col] / A[col * n + col];
      for (let k = col; k < n; k++) A[row * n + k] -= factor * A[col * n + k];
      b[row] -= factor * b[col];
    }
  }
  const x = new Float64Array(n);
  for (let row = n - 1; row >= 0; row--) {
    let sum = b[row];
    for (let k = row + 1; k < n; k++) sum -= A[row * n + k] * x[k];
    x[row] = sum / A[row * n + row];
  }
  return x;
}

function mean_of_points(points: V3[]): V3 {
  const mean = v3(0, 0, 0);
  for (const p of points) { mean.x += p.x; mean.y += p.y; mean.z += p.z; }
  return v3(mean.x / points.length, mean.y / points.length, mean.z / points.length);
}

// Accumulates the normal equations A^T A x = A^T rhs for one design row, scaled by 1/count
// at the end by the caller so the entries stay O(mm^2).
function accumulate_normal_equations(A: Float64Array, b: Float64Array, row: number[], rhs: number): void {
  const n = row.length;
  for (let r = 0; r < n; r++) {
    for (let c = 0; c < n; c++) A[r * n + c] += row[r] * row[c];
    b[r] += row[r] * rhs;
  }
}

export type SphereFit = { center: V3; radius: number };

// Linear least squares of |p|^2 = 2 c.p + k with k = r^2 - |c|^2 (the algebraic sphere
// fit): 4x4 normal equations, points recentered on their mean for conditioning. Not the
// geometric (distance) fit, but within a fraction of a mm of it for a near-sphere.
export function fit_sphere_algebraic(points: V3[]): SphereFit | null {
  if (points.length < 4) return null;
  const mean = mean_of_points(points);
  const A = new Float64Array(16);
  const b = new Float64Array(4);
  for (const point of points) {
    const p = v3_sub(point, mean);
    accumulate_normal_equations(A, b, [2 * p.x, 2 * p.y, 2 * p.z, 1], p.x * p.x + p.y * p.y + p.z * p.z);
  }
  for (let k = 0; k < 16; k++) A[k] /= points.length;
  for (let k = 0; k < 4; k++) b[k] /= points.length;
  const x = solve_linear_system_gaussian(4, A, b);
  if (x === null) return null;
  const c = v3(x[0], x[1], x[2]);
  const r2 = x[3] + v3_dot(c, c);
  if (r2 <= 0) return null;
  return { center: v3(c.x + mean.x, c.y + mean.y, c.z + mean.z), radius: Math.sqrt(r2) };
}

export type CircleFit = { center_x: number; center_y: number; radius: number };

// The 2D version of fit_sphere_algebraic (side-view circle, plan-simplified-skull Q4).
export function fit_circle_algebraic(points: { x: number; y: number }[]): CircleFit | null {
  if (points.length < 3) return null;
  let mean_x = 0, mean_y = 0;
  for (const p of points) { mean_x += p.x; mean_y += p.y; }
  mean_x /= points.length; mean_y /= points.length;
  const A = new Float64Array(9);
  const b = new Float64Array(3);
  for (const point of points) {
    const x = point.x - mean_x, y = point.y - mean_y;
    accumulate_normal_equations(A, b, [2 * x, 2 * y, 1], x * x + y * y);
  }
  for (let k = 0; k < 9; k++) A[k] /= points.length;
  for (let k = 0; k < 3; k++) b[k] /= points.length;
  const x = solve_linear_system_gaussian(3, A, b);
  if (x === null) return null;
  const r2 = x[2] + x[0] * x[0] + x[1] * x[1];
  if (r2 <= 0) return null;
  return { center_x: x[0] + mean_x, center_y: x[1] + mean_y, radius: Math.sqrt(r2) };
}

// Ellipsoid with axes along the coordinate axes of the points given (the caller passes
// points already expressed in the frame it wants, e.g. Frankfurt side/up/front).
export type EllipsoidFit = { center: V3; semi_axes: V3 };

// Linear least squares of a x^2 + b y^2 + c z^2 + d x + e y + f z = 1 (the algebraic
// axis-aligned ellipsoid), 6x6 normal equations, points recentered on their mean.
// Center = -d/2a etc., semi-axes from completing the squares.
export function fit_ellipsoid_algebraic(points: V3[]): EllipsoidFit | null {
  if (points.length < 6) return null;
  const mean = mean_of_points(points);
  const A = new Float64Array(36);
  const b = new Float64Array(6);
  for (const point of points) {
    const p = v3_sub(point, mean);
    accumulate_normal_equations(A, b, [p.x * p.x, p.y * p.y, p.z * p.z, p.x, p.y, p.z], 1);
  }
  for (let k = 0; k < 36; k++) A[k] /= points.length;
  for (let k = 0; k < 6; k++) b[k] /= points.length;
  const x = solve_linear_system_gaussian(6, A, b);
  if (x === null) return null;
  const [qa, qb, qc, qd, qe, qf] = x;
  if (qa <= 0 || qb <= 0 || qc <= 0) return null;
  const center = v3(-qd / (2 * qa), -qe / (2 * qb), -qf / (2 * qc));
  // a x^2 + d x = a (x - cx)^2 - a cx^2, so the constant on the right becomes
  // 1 + a cx^2 + b cy^2 + c cz^2.
  const rhs = 1 + qa * center.x * center.x + qb * center.y * center.y + qc * center.z * center.z;
  if (rhs <= 0) return null;
  return {
    center: v3(center.x + mean.x, center.y + mean.y, center.z + mean.z),
    semi_axes: v3(Math.sqrt(rhs / qa), Math.sqrt(rhs / qb), Math.sqrt(rhs / qc)),
  };
}

// Signed distance vertex -> shape surface, mm; positive = outside the shape.
export function sphere_residual(fit: SphereFit, point: V3): number {
  return v3_length(v3_sub(point, fit.center)) - fit.radius;
}

// Approximate signed distance to the ellipsoid: the point's radial scaling s (s = 1 on the
// surface) turned into a length along the ray from the center. Exact for a sphere, within a
// few percent for the mild axis ratios of a cranium.
export function ellipsoid_residual(fit: EllipsoidFit, point: V3): number {
  const q = v3_sub(point, fit.center);
  const s = Math.sqrt(
    (q.x / fit.semi_axes.x) ** 2 + (q.y / fit.semi_axes.y) ** 2 + (q.z / fit.semi_axes.z) ** 2);
  const distance = v3_length(q);
  return s > 0 ? distance - distance / s : -Math.min(fit.semi_axes.x, fit.semi_axes.y, fit.semi_axes.z);
}

// Loomis's side slices (plan-skull-construction-docs.md Q15): two planes parallel to the
// midline, mirrored, side = ±half_width. `flat_count` = how many vertices defined it.
export type SidePlanesFit = { half_width: number; flat_count: number };

// The flats are the vertices sitting inside the sphere by more than `flat_threshold_mm`
// (residual below -threshold); half_width = least squares of |side| over them, i.e. their
// mean distance from the midline, one number for both sides.
export function fit_side_planes_mirrored(points: V3[], sphere: SphereFit, flat_threshold_mm: number): SidePlanesFit | null {
  let sum = 0, count = 0;
  for (const p of points) {
    if (sphere_residual(sphere, p) < -flat_threshold_mm) { sum += Math.abs(p.x); count++; }
  }
  if (count === 0) return null;
  return { half_width: sum / count, flat_count: count };
}

// Signed distance to the sphere clipped by the two side planes (the sphere ∩ the slab
// |side| <= half_width), mm, positive outside. Inside: minus the distance to the nearest of
// the three surfaces. Outside: the distance to the nearest of the spherical part (radial
// foot point, valid when it lies within the slab), the flat disc (foot point on the plane,
// valid when it lies within the sphere) and the rim circle where they meet.
export function residuals_sphere_with_side_cuts(sphere: SphereFit, half_width: number, p: V3): number {
  const q = v3_sub(p, sphere.center);
  const radial = v3_length(q);
  const plane_x = p.x >= 0 ? half_width : -half_width; // the nearer of the two planes
  const to_plane = Math.abs(p.x) - half_width; // + = beyond the plane
  if (radial <= sphere.radius && to_plane <= 0) return -Math.min(sphere.radius - radial, -to_plane);
  let best = Infinity;
  if (radial > 0) {
    const foot_x = sphere.center.x + q.x * (sphere.radius / radial);
    if (Math.abs(foot_x) <= half_width) best = Math.min(best, radial - sphere.radius);
  }
  const plane_offset = plane_x - sphere.center.x; // sphere center -> plane, signed
  const in_plane = Math.hypot(q.y, q.z); // distance from the sphere's axis through the plane's center
  if (Math.abs(plane_offset) < sphere.radius) {
    const rim_radius = Math.sqrt(sphere.radius * sphere.radius - plane_offset * plane_offset);
    if (in_plane <= rim_radius && to_plane > 0) best = Math.min(best, to_plane);
    best = Math.min(best, Math.hypot(Math.abs(p.x) - half_width, in_plane - rim_radius));
  }
  return best;
}

export type ResidualStats = { min: number; max: number; rms: number; count: number };

export function residual_stats(residuals: number[]): ResidualStats {
  const result: ResidualStats = { min: Infinity, max: -Infinity, rms: 0, count: residuals.length };
  let sum_squares = 0;
  for (const residual of residuals) {
    result.min = Math.min(result.min, residual);
    result.max = Math.max(result.max, residual);
    sum_squares += residual * residual;
  }
  if (residuals.length > 0) result.rms = Math.sqrt(sum_squares / residuals.length);
  return result;
}
