// Dumps what the midsagittal cut of the Z-Anatomy skull looks like (for the skull-ball
// page's profile panel): the obj object/group names, the up/front extent of the midline
// vertices per object, and the silhouette samples in the face band.
//   npx tsx tmp/inspect_midline_profile.ts
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { frankfurt_coordinates, frankfurt_frame_from_landmarks, parse_landmarks_file, parse_obj_mesh_raw } from "../src/reference";

const models_directory = path.join(path.dirname(fileURLToPath(import.meta.url)), "../../data/reference-models");
const obj_text = fs.readFileSync(path.join(models_directory, "z-anatomy-head-skull.obj"), "utf8");
const mesh = parse_obj_mesh_raw(obj_text)!;
const landmarks = parse_landmarks_file(fs.readFileSync(path.join(models_directory, "z-anatomy-head-skull.landmarks.txt"), "utf8"));
const frame = frankfurt_frame_from_landmarks(landmarks)!;
const positions = mesh.positions.map((p) => frankfurt_coordinates(frame, p));

// Which obj objects own which vertex ranges (o / g lines vs v lines, in file order).
const objects: { name: string; first_vertex: number }[] = [];
let vertex_count = 0;
for (const line of obj_text.split("\n")) {
  if (line.startsWith("o ") || line.startsWith("g ")) objects.push({ name: line.trim(), first_vertex: vertex_count });
  else if (line.startsWith("v ")) vertex_count++;
}
console.log(`objects: ${objects.length}, vertices: ${vertex_count}`);
for (let i = 0; i < objects.length; i++) {
  const end = i + 1 < objects.length ? objects[i + 1].first_vertex : vertex_count;
  const slice = positions.slice(objects[i].first_vertex, end);
  const midline = slice.filter((p) => Math.abs(p.x) < 3);
  const extent = (values: number[]) => (values.length ? `${Math.min(...values).toFixed(0)}..${Math.max(...values).toFixed(0)}` : "-");
  console.log(`${objects[i].name}: ${slice.length} vertices, ${midline.length} near midline; up ${extent(slice.map((p) => p.y))}, front ${extent(slice.map((p) => p.z))}`);
}

// Nasal region: is there bone near the midline between the brow and the nasal spine, and
// how far from x = 0 does it sit (a gap between paired nasal bones would hide the nasion
// from an exact x = 0 cut)?
const nasal = positions.filter((p) => Math.abs(p.x) < 8 && p.y > -2 && p.y < 32 && p.z > 80);
nasal.sort((a, b) => b.y - a.y);
for (const p of nasal) console.log(`nasal: side ${p.x.toFixed(1)} up ${p.y.toFixed(1)} front ${p.z.toFixed(1)}`);

// Overall extent and the slab suspects: midline vertices with up in -32..-14.
console.log("up extent", Math.min(...positions.map((p) => p.y)).toFixed(1), Math.max(...positions.map((p) => p.y)).toFixed(1));
const slab = positions.filter((p) => Math.abs(p.x) < 3 && p.y > -32 && p.y < -14);
console.log(`slab band: ${slab.length} midline vertices, front ${Math.min(...slab.map((p) => p.z)).toFixed(0)}..${Math.max(...slab.map((p) => p.z)).toFixed(0)}`);
