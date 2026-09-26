// Prints the saved glabella landmark in Frankfurt-frame mm (side / up / front).
//   npx tsx tmp/print_glabella_frankfurt.ts
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { frankfurt_coordinates, frankfurt_frame_from_landmarks, parse_landmarks_file } from "../src/reference";
const models_directory = path.join(path.dirname(fileURLToPath(import.meta.url)), "../../data/reference-models");
const landmarks = parse_landmarks_file(fs.readFileSync(path.join(models_directory, "z-anatomy-head-skull.landmarks.txt"), "utf8"));
const frame = frankfurt_frame_from_landmarks(landmarks)!;
for (const l of landmarks) { const q = frankfurt_coordinates(frame, l.p); console.log(`${l.name}: side ${q.x.toFixed(1)} up ${q.y.toFixed(1)} front ${q.z.toFixed(1)}`); }
