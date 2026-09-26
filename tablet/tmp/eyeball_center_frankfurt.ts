// Fits a sphere to each eyeball of the Z-Anatomy eyeball mesh (right = side > 0) and prints
// its center and radius in Frankfurt mm, next to the midpoint of the picked orbit corners
// (the skull-eyes page's "eye center"), to check that midpoint against the real globe.
// Run: npx tsx tmp/eyeball_center_frankfurt.ts
import { readFileSync } from "node:fs";
import { frankfurt_coordinates, frankfurt_frame_from_landmarks, parse_landmarks_file, parse_obj_mesh_raw } from "../src/reference";

const models = "../data/reference-models/";
const landmarks = parse_landmarks_file(readFileSync(models + "z-anatomy-head-skull.landmarks.txt", "utf8"));
const frame = frankfurt_frame_from_landmarks(landmarks)!;
const raw = parse_obj_mesh_raw(readFileSync(models + "z-anatomy-head-eyeball.obj", "utf8"))!;
const positions = raw.positions.map((p) => frankfurt_coordinates(frame, p));

type Point = { x: number; y: number; z: number };

// Algebraic least-squares sphere: x² + y² + z² = 2 a·x + 2 b·y + 2 c·z + d, solved by 4x4 normal equations.
function fit_sphere(points: Point[]): { center: Point; radius: number } {
  const n = 4;
  const A = Array.from({ length: n }, () => new Array<number>(n).fill(0));
  const B = new Array<number>(n).fill(0);
  for (const p of points) {
    const row = [2 * p.x, 2 * p.y, 2 * p.z, 1];
    const rhs = p.x * p.x + p.y * p.y + p.z * p.z;
    for (let i = 0; i < n; i++) {
      B[i] += row[i] * rhs;
      for (let j = 0; j < n; j++) A[i][j] += row[i] * row[j];
    }
  }
  // Gaussian elimination with partial pivoting.
  for (let col = 0; col < n; col++) {
    let pivot = col;
    for (let r = col + 1; r < n; r++) if (Math.abs(A[r][col]) > Math.abs(A[pivot][col])) pivot = r;
    [A[col], A[pivot]] = [A[pivot], A[col]];
    [B[col], B[pivot]] = [B[pivot], B[col]];
    for (let r = 0; r < n; r++) {
      if (r === col) continue;
      const f = A[r][col] / A[col][col];
      for (let c = col; c < n; c++) A[r][c] -= f * A[col][c];
      B[r] -= f * B[col];
    }
  }
  const [a, b, c, d] = B.map((v, i) => v / A[i][i]);
  return { center: { x: a, y: b, z: c }, radius: Math.sqrt(d + a * a + b * b + c * c) };
}

function describe(name: string, p: Point): string {
  return `${name}: side ${p.x.toFixed(1)}, up ${p.y.toFixed(1)}, front ${p.z.toFixed(1)}`;
}

for (const side of ["right", "left"] as const) {
  const points = positions.filter((p) => (side === "right" ? p.x > 0 : p.x < 0));
  const { center, radius } = fit_sphere(points);
  let rms = 0;
  for (const p of points) { const r = Math.hypot(p.x - center.x, p.y - center.y, p.z - center.z) - radius; rms += r * r; }
  rms = Math.sqrt(rms / points.length);
  console.log(`${describe(`${side} eyeball (${points.length} vertices)`, center)}, radius ${radius.toFixed(1)}, fit rms ${rms.toFixed(2)} mm`);
}

const by_name = (name: string) => landmarks.find((l) => l.name === name)!.p;
const inner = frankfurt_coordinates(frame, by_name("orbit_inner_corner"));
const outer = frankfurt_coordinates(frame, by_name("orbit_outer_corner"));
const midpoint = { x: 0.5 * (inner.x + outer.x), y: 0.5 * (inner.y + outer.y), z: 0.5 * (inner.z + outer.z) };
console.log(describe("picked inner corner", inner));
console.log(describe("picked outer corner", outer));
console.log(describe("corner midpoint (the page's eye center, right)", midpoint));
