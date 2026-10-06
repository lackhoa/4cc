// Adds the hairline to the template `skull-zanatomy` as free vertices and two thin named
// strokes on the skin layer (plan-skin-hairline-landmark.md). The midline point sits at
// the face-thirds canon height (one brow-to-nose-base length above the brow); the temple
// corner and the sideburn point come from the Loomis girl plate, scaled to the template.
// Every vertex is snapped onto the Z-Anatomy skin mesh (ear triangles excluded, so the
// sideburn point lands on the cheek in front of the ear, not on the ear).
// Re-running moves the three named vertices and leaves everything else alone.
//   npx tsx tmp/add_hairline_strokes.ts
// Node-only (no dev server).
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { default_camera } from "../src/camera";
import {
  SKULL_BONE_ID, StrokeRadii, TabletDocument, VertexId, add_stroke, add_vertex, empty_document, set_vertex_world_position, vertex_by_id,
} from "../src/document";
import { V3, v3, v3_scale } from "../src/math";
import { nearest_point_on_mesh, project_vertex_onto_mesh } from "../src/mesh_projection";
import { apply_document_state, serialize_document_state } from "../src/persistence";
import { ReferenceMesh, frankfurt_coordinates, frankfurt_frame_from_landmarks, parse_landmarks_file, parse_obj_mesh_raw, reference_mesh_from_positions } from "../src/reference";
import { SKIN_NAME, SKULL_NAME, WORLD_PER_MM } from "../src/reference_skull_view";

const tablet_directory = path.join(path.dirname(fileURLToPath(import.meta.url)), "..");
const models_directory = path.join(tablet_directory, "../data/reference-models");
const document_path = path.join(tablet_directory, "documents/skull-zanatomy.json");

const HAIRLINE_RADII: StrokeRadii = [0.5, 0.5, 0.5, 0.5]; // twice the 0.25 default: the hairline must stand out from the skin strokes

// Z-Anatomy region names (without the .l/.r) that belong to the external ear.
const EAR_REGION_WORDS = ["auricle", "auricular", "helix", "tragus", "concha", "scapha", "fossa", "incisure", "crura", "eminentia", "lobule"];

function load_skin_mesh_without_ears(): ReferenceMesh {
  const landmarks = parse_landmarks_file(fs.readFileSync(path.join(models_directory, `${SKULL_NAME}.landmarks.txt`), "utf8"));
  const frame = frankfurt_frame_from_landmarks(landmarks);
  if (frame === null) throw new Error("skull landmarks file lacks porion_l / porion_r / orbitale");
  const raw = parse_obj_mesh_raw(fs.readFileSync(path.join(models_directory, `${SKIN_NAME}.obj`), "utf8"));
  if (raw === null) throw new Error("skin mesh unreadable");
  const positions_world = raw.positions.map((p) => v3_scale(frankfurt_coordinates(frame, p), WORLD_PER_MM));
  const kept_indices: number[] = [];
  let dropped_triangles = 0;
  for (let triangle = 0; triangle < raw.triangle_object_names.length; triangle++) {
    const region = raw.triangle_object_names[triangle].toLowerCase();
    if (EAR_REGION_WORDS.some((word) => region.includes(word))) { dropped_triangles++; continue; }
    kept_indices.push(raw.triangle_indices[3 * triangle], raw.triangle_indices[3 * triangle + 1], raw.triangle_indices[3 * triangle + 2]);
  }
  console.log(`skin mesh: ${raw.triangle_object_names.length} triangles, ${dropped_triangles} ear triangles dropped`);
  return reference_mesh_from_positions(positions_world, kept_indices);
}

const tablet_document: TabletDocument = empty_document();
const camera = default_camera();
apply_document_state(fs.readFileSync(document_path, "utf8"), tablet_document, camera);
console.log(`loaded ${document_path}: ${tablet_document.vertices.length} vertices, ${tablet_document.strokes.length} strokes`);

function named_vertex_position(name: string): V3 {
  const vertex = tablet_document.vertices.find((v) => v.name === name);
  if (vertex === undefined) throw new Error(`template has no vertex named ${name}`);
  return vertex.position;
}

// Canon: hairline = brow + (brow - nose base). Brow = glabella vertex 113, nose base =
// under-nose-tip vertex 101 (both midline skin vertices, unnamed, so looked up by id).
const glabella_y = vertex_by_id(tablet_document, 113).position.y;
const nose_base_y = vertex_by_id(tablet_document, 101).position.y;
const hairline_y = glabella_y + (glabella_y - nose_base_y);
console.log(`brow y ${glabella_y.toFixed(3)}, nose base y ${nose_base_y.toFixed(3)} -> hairline y ${hairline_y.toFixed(3)}`);

const mesh = load_skin_mesh_without_ears();
const FORWARD_TO_BACK = v3(0, 0, -1);
const FROM_FRONT_Z = 2; // a ray start well in front of the face

// Plate reads (plan Q4) scaled to the template, 293.5 px per world unit. In the front
// view the hairline meets the midline at row ~160 and turns down at the temple at
// column ~440, row ~185: 25 px lower, 138 px off the midline (302).
const PIXELS_PER_WORLD_UNIT = 293.5;
const temple_y = hairline_y - 25 / PIXELS_PER_WORLD_UNIT;
const temple_x = 138 / PIXELS_PER_WORLD_UNIT;

type HairlinePoint = { name: string; position: V3; midline: boolean };
const hairline_point = project_vertex_onto_mesh(mesh, v3(0, hairline_y, FROM_FRONT_Z), FORWARD_TO_BACK, 10);
const temple_point = project_vertex_onto_mesh(mesh, v3(temple_x, temple_y, FROM_FRONT_Z), FORWARD_TO_BACK, 10);
const sideburn_point = nearest_point_on_mesh(mesh, v3(0.62, 0.35, 0.3));
const points: HairlinePoint[] = [
  { name: "hairline", position: hairline_point.position, midline: true },
  { name: "hairline_temple", position: temple_point.position, midline: false },
  { name: "hairline_sideburn", position: sideburn_point, midline: false },
];
console.log(`projection methods: hairline ${hairline_point.method}, temple ${temple_point.method}`);

const vertex_ids = new Map<string, VertexId>();
let added_vertices = 0;
for (const point of points) {
  const existing = tablet_document.vertices.find((v) => v.name === point.name);
  let id: VertexId;
  if (existing !== undefined) {
    id = existing.id;
    set_vertex_world_position(tablet_document, id, point.position);
  } else {
    id = add_vertex(tablet_document, point.position, SKULL_BONE_ID);
    added_vertices++;
  }
  const vertex = vertex_by_id(tablet_document, id);
  vertex.name = point.name;
  if (point.midline) vertex.midline = true;
  vertex_ids.set(point.name, id);
  console.log(`${point.name} (vertex ${id}): ${point.position.x.toFixed(3)} ${point.position.y.toFixed(3)} ${point.position.z.toFixed(3)}`);
}

let added_strokes = 0;
function ensure_stroke(name: string, from: string, to: string): void {
  const existing = tablet_document.strokes.find((stroke) => stroke.name === name);
  if (existing !== undefined) { existing.radii = [...HAIRLINE_RADII]; return; } // re-run: only the width is refreshed
  const id = add_stroke(tablet_document, vertex_ids.get(from)!, vertex_ids.get(to)!, v3(0, 0, 0), v3(0, 0, 0), "skin");
  const stroke = tablet_document.strokes.find((s) => s.id === id)!;
  stroke.name = name;
  stroke.radii = [...HAIRLINE_RADII];
  added_strokes++;
  console.log(`stroke ${id} "${name}": ${from} -> ${to}`);
}
ensure_stroke("hairline", "hairline", "hairline_temple");
ensure_stroke("hairline side", "hairline_temple", "hairline_sideburn");

fs.writeFileSync(document_path, serialize_document_state(tablet_document, camera));
console.log(`${added_vertices} vertices and ${added_strokes} strokes added -> ${document_path}`);
