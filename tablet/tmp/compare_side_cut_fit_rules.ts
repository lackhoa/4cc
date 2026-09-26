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
