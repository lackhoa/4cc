// Prints the mandible's extent in the Frankfurt frame and the back-bottom corner (gonion
// candidate) per side, to sanity-check the gonion guess on the skull-mouth-jaw page.
// Run: npx tsx tmp/mandible_extent.ts
import { readFileSync } from "node:fs";
import { frankfurt_coordinates, frankfurt_frame_from_landmarks, parse_landmarks_file, parse_obj_mesh_raw } from "../src/reference";

const models = "../data/reference-models/";
const skull_landmarks = parse_landmarks_file(readFileSync(models + "z-anatomy-head-skull.landmarks.txt", "utf8"));
const frame = frankfurt_frame_from_landmarks(skull_landmarks)!;
const raw = parse_obj_mesh_raw(readFileSync(models + "z-anatomy-head-mandible.obj", "utf8"))!;
const positions = raw.positions.map((p) => frankfurt_coordinates(frame, p));
const min = { x: Infinity, y: Infinity, z: Infinity }, max = { x: -Infinity, y: -Infinity, z: -Infinity };
for (const p of positions) {
  min.x = Math.min(min.x, p.x); min.y = Math.min(min.y, p.y); min.z = Math.min(min.z, p.z);
  max.x = Math.max(max.x, p.x); max.y = Math.max(max.y, p.y); max.z = Math.max(max.z, p.z);
}
console.log("vertices", positions.length, "min", min, "max", max);
for (const side of [1, -1]) {
  let best = null as null | { x: number; y: number; z: number };
  for (const p of positions) if (p.x * side > 20 && (best === null || -p.z - p.y > -best.z - best.y)) best = p;
  console.log("side", side, "back-bottom corner", best);
  // The lowest vertex in the back third, per side.
  let lowest = null as null | { x: number; y: number; z: number };
  for (const p of positions) if (p.x * side > 20 && p.z < -40 && (lowest === null || p.y < lowest.y)) lowest = p;
  console.log("side", side, "lowest behind front -40", lowest);
}
