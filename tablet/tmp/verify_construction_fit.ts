// Checks src/construction_fit.ts against the C++ construction_fit printout (plan
// plan-skull-construction-docs.md step 4): Frankfurt-vault sphere of the Z-Anatomy skull,
// expected center side 0 / up +47.7 / front -11.2 mm, radius 78.0, rms 12.2.
//   npx tsx tmp/verify_construction_fit.ts
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { fit_circle_algebraic, fit_ellipsoid_algebraic, fit_sphere_algebraic, ellipsoid_residual, residual_stats, sphere_residual } from "../src/construction_fit";
import { frankfurt_coordinates, frankfurt_frame_from_landmarks, parse_landmarks_file, parse_obj_mesh_raw } from "../src/reference";
import { v3_dot, v3_sub } from "../src/math";

const models_directory = path.join(path.dirname(fileURLToPath(import.meta.url)), "../../data/reference-models");
const mesh = parse_obj_mesh_raw(fs.readFileSync(path.join(models_directory, "z-anatomy-head-skull.obj"), "utf8"))!;
const landmarks = parse_landmarks_file(fs.readFileSync(path.join(models_directory, "z-anatomy-head-skull.landmarks.txt"), "utf8"));
console.log("landmarks:", landmarks);
const frame = frankfurt_frame_from_landmarks(landmarks)!;
console.log("frame:", frame);

const vault = mesh.positions.filter((p) => v3_dot(v3_sub(p, frame.porion_middle), frame.up) > 0);
console.log(`vault ${vault.length} of ${mesh.positions.length} skull vertices`);

const sphere = fit_sphere_algebraic(vault)!;
const center_frame = frankfurt_coordinates(frame, sphere.center);
console.log(`sphere: center frame side ${center_frame.x.toFixed(1)} up ${center_frame.y.toFixed(1)} front ${center_frame.z.toFixed(1)}, radius ${sphere.radius.toFixed(2)}`);
console.log("sphere residuals:", residual_stats(vault.map((p) => sphere_residual(sphere, p))));

const side_view = vault.map((p) => { const q = frankfurt_coordinates(frame, p); return { x: q.z, y: q.y }; });
const circle = fit_circle_algebraic(side_view)!;
console.log(`side-view circle: center front ${circle.center_x.toFixed(1)} up ${circle.center_y.toFixed(1)}, radius ${circle.radius.toFixed(2)}`);

const vault_frame = vault.map((p) => frankfurt_coordinates(frame, p));
const ellipsoid = fit_ellipsoid_algebraic(vault_frame)!;
console.log("ellipsoid (frame coords):", ellipsoid);
console.log("ellipsoid residuals:", residual_stats(vault_frame.map((p) => ellipsoid_residual(ellipsoid, p))));
