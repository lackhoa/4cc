// Prints the saved zygion landmark in Frankfurt mm (side, up, front) against the nose line and side plane.
// Run: npx tsx tmp/zygion_frankfurt.ts
import { readFileSync } from "node:fs";
import { frankfurt_coordinates, frankfurt_frame_from_landmarks, parse_landmarks_file } from "../src/reference";

const models = "../data/reference-models/";
const landmarks = parse_landmarks_file(readFileSync(models + "z-anatomy-head-skull.landmarks.txt", "utf8"));
const frame = frankfurt_frame_from_landmarks(landmarks)!;
for (const name of ["glabella", "nasal_spine", "zygion", "zygomatic_arch_bottom"]) {
  const l = landmarks.find((x) => x.name === name)!;
  const p = frankfurt_coordinates(frame, l.p);
  console.log(`${name}: side ${p.x.toFixed(1)} up ${p.y.toFixed(1)} front ${p.z.toFixed(1)}`);
}
