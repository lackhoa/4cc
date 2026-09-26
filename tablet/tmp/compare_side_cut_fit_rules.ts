// Compares candidate rules for the side-cut half_width (plan-skull-construction-docs.md
// Q15/Q16) on the Z-Anatomy skull, so the decision has numbers. Prints, per rule, the
// flats count, half_width (mm and radii) and the clipped-sphere rms over the vault.
//   npx tsx tmp/compare_side_cut_fit_rules.ts
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { V3, v3_dot, v3_sub } from "../src/math";
import { find_landmark, frankfurt_coordinates, frankfurt_frame_from_landmarks, parse_landmarks_file, parse_obj_mesh_raw } from "../src/reference";
import { fit_sphere_algebraic, residual_stats, residuals_sphere_with_side_cuts, sphere_residual } from "../src/construction_fit";
import { compute_vertex_normals, cranium_cut_normal } from "../src/reference_skull_view";

const models_directory = path.join(path.dirname(fileURLToPath(import.meta.url)), "../../data/reference-models");
const landmarks = parse_landmarks_file(fs.readFileSync(path.join(models_directory, "z-anatomy-head-skull.landmarks.txt"), "utf8"));
const frame = frankfurt_frame_from_landmarks(landmarks)!;
const raw = parse_obj_mesh_raw(fs.readFileSync(path.join(models_directory, "z-anatomy-head-skull.obj"), "utf8"))!;
const positions = raw.positions.map((p) => frankfurt_coordinates(frame, p));
const normals = compute_vertex_normals(positions, raw.triangle_indices);
const glabella = frankfurt_coordinates(frame, find_landmark(landmarks, "glabella")!);
const cut_normal = cranium_cut_normal(glabella);

const vault_indices: number[] = [];
positions.forEach((p, i) => { if (v3_dot(p, cut_normal) > 0) vault_indices.push(i); });
const vault = vault_indices.map((i) => positions[i]);
const sphere = fit_sphere_algebraic(vault)!;
const sphere_stats = residual_stats(vault.map((p) => sphere_residual(sphere, p)));
console.log(`vault ${vault.length}, sphere center up ${sphere.center.y.toFixed(1)} front ${sphere.center.z.toFixed(1)} r ${sphere.radius.toFixed(1)} rms ${sphere_stats.rms.toFixed(1)}`);

// Outer surface = normal pointing away from the sphere center (the mesh is a hollow shell).
const is_outer = (i: number) => v3_dot(normals[i], v3_sub(positions[i], sphere.center)) > 0;
const outer_indices = vault_indices.filter(is_outer);
const outer = outer_indices.map((i) => positions[i]);
const outer_sphere = fit_sphere_algebraic(outer)!;
const outer_stats = residual_stats(outer.map((p) => sphere_residual(sphere, p)));
const outer_refit_stats = residual_stats(outer.map((p) => sphere_residual(outer_sphere, p)));
console.log(`outer vault ${outer.length}: same sphere rms ${outer_stats.rms.toFixed(1)} min ${outer_stats.min.toFixed(1)} max ${outer_stats.max.toFixed(1)}; refit on outer only: center up ${outer_sphere.center.y.toFixed(1)} front ${outer_sphere.center.z.toFixed(1)} r ${outer_sphere.radius.toFixed(1)} rms ${outer_refit_stats.rms.toFixed(1)}`);

function report(name: string, flats: V3[], vault_points: V3[], s = sphere): void {
  if (flats.length === 0) { console.log(`${name}: no flats`); return; }
  const half_width = flats.reduce((sum, p) => sum + Math.abs(p.x), 0) / flats.length;
  const stats = residual_stats(vault_points.map((p) => residuals_sphere_with_side_cuts(s, half_width, p)));
  console.log(`${name}: flats ${flats.length}, half_width ${half_width.toFixed(1)} = ${(half_width / s.radius).toFixed(2)} r, clipped rms ${stats.rms.toFixed(1)} min ${stats.min.toFixed(1)} max ${stats.max.toFixed(1)}`);
}

report("A current rule (inside > rms, all vault)", vault.filter((p) => sphere_residual(sphere, p) < -sphere_stats.rms), vault);
report("B inside > rms, outer vault only", outer.filter((p) => sphere_residual(sphere, p) < -outer_stats.rms), outer);
for (const threshold of [3, 5]) report(`B' inside > ${threshold} mm, outer vault only`, outer.filter((p) => sphere_residual(sphere, p) < -threshold), outer);
for (const min_nx of [0.8, 0.9, 0.95]) report(`C sideways-facing outer vault (|n.side| > ${min_nx})`, outer_indices.filter((i) => Math.abs(normals[i].x) > min_nx).map((i) => positions[i]), outer);
const widths = outer.map((p) => Math.abs(p.x)).sort((a, b) => a - b);
for (const q of [0.9, 0.95, 0.99]) {
  const half_width = widths[Math.floor(q * (widths.length - 1))];
  const stats = residual_stats(outer.map((p) => residuals_sphere_with_side_cuts(sphere, half_width, p)));
  console.log(`D width percentile ${q}: half_width ${half_width.toFixed(1)} = ${(half_width / sphere.radius).toFixed(2)} r, clipped rms ${stats.rms.toFixed(1)}`);
}
// Where is the skull widest? |side| envelope per 10 mm band of up, outer vault.
console.log("outer vault max |side| per up band:");
for (let up = 0; up < 130; up += 10) {
  const band = outer.filter((p) => p.y >= up && p.y < up + 10);
  if (band.length > 0) console.log(`  up ${up}-${up + 10}: max |side| ${Math.max(...band.map((p) => Math.abs(p.x))).toFixed(1)} (${band.length} vertices)`);
}

// Khoa's order (2026-09-26): size the ball on the profile (near-midline outer vault), so it
// touches the forehead and the back, THEN cut the sides; the flats are what is left inside.
for (const midline_band of [10, 20]) {
  const profile = outer.filter((p) => Math.abs(p.x) < midline_band);
  const s = fit_sphere_algebraic(profile)!;
  const profile_stats = residual_stats(profile.map((p) => sphere_residual(s, p)));
  const all_stats = residual_stats(outer.map((p) => sphere_residual(s, p)));
  console.log(`E profile sphere (|side| < ${midline_band}, ${profile.length} vertices): center up ${s.center.y.toFixed(1)} front ${s.center.z.toFixed(1)} r ${s.radius.toFixed(1)}; rms on profile ${profile_stats.rms.toFixed(1)}, on outer vault ${all_stats.rms.toFixed(1)} min ${all_stats.min.toFixed(1)} max ${all_stats.max.toFixed(1)}`);
  report(`  E + Q15 rule (inside > profile rms ${profile_stats.rms.toFixed(1)})`, outer.filter((p) => sphere_residual(s, p) < -profile_stats.rms), outer, s);
  for (const threshold of [5, 10]) report(`  E + inside > ${threshold} mm`, outer.filter((p) => sphere_residual(s, p) < -threshold), outer, s);
  for (const q of [0.95, 0.99]) {
    const half_width = widths[Math.floor(q * (widths.length - 1))];
    const stats = residual_stats(outer.map((p) => residuals_sphere_with_side_cuts(s, half_width, p)));
    console.log(`  E + widest percentile ${q}: half_width ${half_width.toFixed(1)} = ${(half_width / s.radius).toFixed(2)} r, clipped rms ${stats.rms.toFixed(1)} min ${stats.min.toFixed(1)} max ${stats.max.toFixed(1)}`);
  }
}

// "Touches the forehead and the back": not least squares -- the sphere whose diameter is
// the front-to-back length of the outer vault near the midline, centered on that segment.
{
  const profile = outer.filter((p) => Math.abs(p.x) < 10);
  const front_most = profile.reduce((a, b) => (b.z > a.z ? b : a));
  const back_most = profile.reduce((a, b) => (b.z < a.z ? b : a));
  const top_most = profile.reduce((a, b) => (b.y > a.y ? b : a));
  console.log(`F midline extremes: front ${front_most.z.toFixed(1)} (up ${front_most.y.toFixed(1)}), back ${back_most.z.toFixed(1)} (up ${back_most.y.toFixed(1)}), top up ${top_most.y.toFixed(1)} (front ${top_most.z.toFixed(1)})`);
  const length = front_most.z - back_most.z;
  for (const center_up of [sphere.center.y, (front_most.y + back_most.y) / 2]) {
    const s = { center: { x: 0, y: center_up, z: (front_most.z + back_most.z) / 2 }, radius: length / 2 };
    const all_stats = residual_stats(outer.map((p) => sphere_residual(s, p)));
    console.log(`F length sphere: center up ${s.center.y.toFixed(1)} front ${s.center.z.toFixed(1)} r ${s.radius.toFixed(1)}; rms on outer vault ${all_stats.rms.toFixed(1)} min ${all_stats.min.toFixed(1)} max ${all_stats.max.toFixed(1)}; top of sphere up ${(s.center.y + s.radius).toFixed(1)} vs skull top ${top_most.y.toFixed(1)}`);
    for (const threshold of [5, 10]) report(`  F + inside > ${threshold} mm`, outer.filter((p) => sphere_residual(s, p) < -threshold), outer, s);
    for (const q of [0.95, 0.99]) {
      const half_width = widths[Math.floor(q * (widths.length - 1))];
      const stats = residual_stats(outer.map((p) => residuals_sphere_with_side_cuts(s, half_width, p)));
      console.log(`  F + widest percentile ${q}: half_width ${half_width.toFixed(1)} = ${(half_width / s.radius).toFixed(2)} r, clipped rms ${stats.rms.toFixed(1)} min ${stats.min.toFixed(1)} max ${stats.max.toFixed(1)}`);
    }
  }
}
