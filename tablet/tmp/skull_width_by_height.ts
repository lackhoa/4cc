// Prints the skull's widest vertex (right side, in front of the ear hole) per 5 mm height
// band in the Frankfurt frame, to place the zygion guess on the skull-cheekbones page.
// Run: npx tsx tmp/skull_width_by_height.ts
import { readFileSync } from "node:fs";
import { frankfurt_coordinates, frankfurt_frame_from_landmarks, parse_landmarks_file, parse_obj_mesh_raw } from "../src/reference";

const models = "../data/reference-models/";
const skull_landmarks = parse_landmarks_file(readFileSync(models + "z-anatomy-head-skull.landmarks.txt", "utf8"));
const frame = frankfurt_frame_from_landmarks(skull_landmarks)!;
const raw = parse_obj_mesh_raw(readFileSync(models + "z-anatomy-head-skull.obj", "utf8"))!;
const positions = raw.positions.map((p) => frankfurt_coordinates(frame, p));
for (let band_low = -30; band_low < 50; band_low += 5) {
  let best = null as null | { x: number; y: number; z: number };
  for (const p of positions) {
    if (p.y < band_low || p.y >= band_low + 5 || p.z < 10) continue;
    if (best === null || p.x > best.x) best = p;
  }
  if (best !== null) console.log(`up ${band_low}..${band_low + 5}: side ${best.x.toFixed(1)} at up ${best.y.toFixed(1)} front ${best.z.toFixed(1)}`);
}
