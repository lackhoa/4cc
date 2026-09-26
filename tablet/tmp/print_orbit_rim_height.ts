// Where is the supraorbital margin (the eyebrow line) on the Z-Anatomy skull? Through the
// orbit (side band 25..37), prints the most forward vertex per 2 mm of up: the rim is where
// front drops sharply coming down from the forehead into the orbit cavity.
//   npx tsx tmp/print_orbit_rim_height.ts
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { frankfurt_coordinates, frankfurt_frame_from_landmarks, parse_landmarks_file, parse_obj_mesh_raw } from "../src/reference";
const models_directory = path.join(path.dirname(fileURLToPath(import.meta.url)), "../../data/reference-models");
const mesh = parse_obj_mesh_raw(fs.readFileSync(path.join(models_directory, "z-anatomy-head-skull.obj"), "utf8"))!;
const landmarks = parse_landmarks_file(fs.readFileSync(path.join(models_directory, "z-anatomy-head-skull.landmarks.txt"), "utf8"));
const frame = frankfurt_frame_from_landmarks(landmarks)!;
const positions = mesh.positions.map((p) => frankfurt_coordinates(frame, p));
for (const [side_min, side_max] of [[25, 37], [10, 20], [-3, 3]]) {
  console.log(`side ${side_min}..${side_max}: up -> max front`);
  for (let up = 60; up >= -10; up -= 2) {
    const band = positions.filter((p) => p.x >= side_min && p.x <= side_max && p.y >= up && p.y < up + 2);
    const front = band.length ? Math.max(...band.map((p) => p.z)) : null;
    console.log(`  up ${up}: ${front === null ? "-" : front.toFixed(1)} (${band.length})`);
  }
}
