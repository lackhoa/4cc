// Prints the most forward midline point (|side| < 4 mm) per 2 mm height band of the skull and
// the mandible in the Frankfurt frame, to sanity-check the nasal spine / menton guesses on
// the skull-face-thirds page. Run: npx tsx tmp/midline_front_profile.ts
import { readFileSync } from "node:fs";
import { frankfurt_coordinates, frankfurt_frame_from_landmarks, parse_landmarks_file, parse_obj_mesh_raw } from "../src/reference";

const models = "../data/reference-models/";
const skull_landmarks = parse_landmarks_file(readFileSync(models + "z-anatomy-head-skull.landmarks.txt", "utf8"));
const frame = frankfurt_frame_from_landmarks(skull_landmarks)!;

function bands(name: string, up_min: number, up_max: number): void {
  const raw = parse_obj_mesh_raw(readFileSync(models + name + ".obj", "utf8"))!;
  const positions = raw.positions.map((p) => frankfurt_coordinates(frame, p));
  console.log(name);
  for (let up = up_max; up >= up_min; up -= 2) {
    let front: number | null = null;
    for (const p of positions) if (Math.abs(p.x) < 4 && p.y >= up && p.y < up + 2 && (front === null || p.z > front)) front = p.z;
    console.log(`  up ${up.toString().padStart(4)}: front ${front === null ? "-" : front.toFixed(1)}`);
  }
}

bands("z-anatomy-head-skull", -50, 10);
bands("z-anatomy-head-mandible", -80, -40);
