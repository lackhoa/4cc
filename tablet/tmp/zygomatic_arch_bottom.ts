// Prints the lowest vertex of the zygomatic arch per 5 mm front band (right side, side > 45 mm,
// in front of the ear hole) in Frankfurt mm, to see how far the arch's underside sits above the nose line.
// Run: npx tsx tmp/zygomatic_arch_bottom.ts
import { readFileSync } from "node:fs";
import { frankfurt_coordinates, frankfurt_frame_from_landmarks, parse_landmarks_file, parse_obj_mesh_raw } from "../src/reference";

const models = "../data/reference-models/";
const landmarks = parse_landmarks_file(readFileSync(models + "z-anatomy-head-skull.landmarks.txt", "utf8"));
const frame = frankfurt_frame_from_landmarks(landmarks)!;
const raw = parse_obj_mesh_raw(readFileSync(models + "z-anatomy-head-skull.obj", "utf8"))!;
const positions = raw.positions.map((p) => frankfurt_coordinates(frame, p));
for (let front_low = 10; front_low < 80; front_low += 5) {
  let lowest = null as null | { x: number; y: number; z: number };
  for (const p of positions) {
    if (p.z < front_low || p.z >= front_low + 5 || p.x < 45 || p.y < -30 || p.y > 20) continue;
    if (lowest === null || p.y < lowest.y) lowest = p;
  }
  if (lowest !== null) console.log(`front ${front_low}..${front_low + 5}: lowest up ${lowest.y.toFixed(1)} at side ${lowest.x.toFixed(1)} front ${lowest.z.toFixed(1)}`);
}
