// Imports the reference landmarks (plan-sketchpad-landmarks.md Q65) into the tablet
// document `skull-zanatomy` as named vertices: every entry of the skull's and the
// mandible's `.landmarks.txt` becomes a vertex with that name, in the same world frame
// the skull-draw page shows the meshes in (Frankfurt mm * WORLD_PER_MM). Re-running
// updates each vertex by name (position only) and appends the new names; vertices with
// other names, strokes, pins, knots, patches and the camera are left alone. Creates the
// document (default camera) when it doesn't exist yet.
//   npx tsx tmp/import_landmarks_into_document.ts
// Node-only (no dev server) — imports src/ modules that have no DOM dependency.
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { default_camera } from "../src/camera";
import { TabletDocument, add_vertex, empty_document } from "../src/document";
import { v3_scale } from "../src/math";
import { Landmark, frankfurt_coordinates, frankfurt_frame_from_landmarks, parse_landmarks_file } from "../src/reference";
import { MANDIBLE_NAME, SKULL_NAME, WORLD_PER_MM } from "../src/reference_skull_view";
import { apply_document_state, serialize_document_state } from "../src/persistence";

const tablet_directory = path.join(path.dirname(fileURLToPath(import.meta.url)), "..");
const models_directory = path.join(tablet_directory, "../data/reference-models");
const document_path = path.join(tablet_directory, "documents/skull-zanatomy.json");

function read_landmarks(mesh_name: string): Landmark[] {
  return parse_landmarks_file(fs.readFileSync(path.join(models_directory, `${mesh_name}.landmarks.txt`), "utf8"));
}

const skull_landmarks = read_landmarks(SKULL_NAME);
const frame = frankfurt_frame_from_landmarks(skull_landmarks);
if (frame === null) throw new Error("skull landmarks file lacks porion_l / porion_r / orbitale");
const landmarks = [...skull_landmarks, ...read_landmarks(MANDIBLE_NAME)];

const tablet_document: TabletDocument = empty_document();
const camera = default_camera();
if (fs.existsSync(document_path)) {
  apply_document_state(fs.readFileSync(document_path, "utf8"), tablet_document, camera);
  console.log(`loaded ${document_path}: ${tablet_document.vertices.length} vertices, ${tablet_document.strokes.length} strokes`);
} else {
  console.log(`creating ${document_path}`);
}

let updated = 0;
let added = 0;
for (const landmark of landmarks) {
  const position = v3_scale(frankfurt_coordinates(frame, landmark.p), WORLD_PER_MM);
  const existing = tablet_document.vertices.find((vertex) => vertex.name === landmark.name);
  if (existing !== undefined) {
    existing.position = position;
    updated++;
  } else {
    const id = add_vertex(tablet_document, position);
    tablet_document.vertices.find((vertex) => vertex.id === id)!.name = landmark.name;
    added++;
  }
  console.log(`${landmark.name}: ${position.x.toFixed(3)} ${position.y.toFixed(3)} ${position.z.toFixed(3)}`);
}
fs.writeFileSync(document_path, serialize_document_state(tablet_document, camera));
console.log(`${added} added, ${updated} updated -> ${document_path}`);
