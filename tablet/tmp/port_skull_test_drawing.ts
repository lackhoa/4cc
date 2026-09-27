// Ports the drawing in documents/skull-test.json (drawn on the plain draw page, over
// `skull.obj` normalized to height 2 with its base on y=0) into documents/skull-zanatomy.json
// (the skull-draw page: Z-Anatomy skull + mandible in Frankfurt mm * WORLD_PER_MM).
// Rough fit only, per Khoa: a uniform scale + translation matching the two meshes'
// bounding boxes (no rotation; both skulls stand upright facing +z, checked below).
// Strokes, patches, pins and knots are appended with their ids offset past the target's
// counters; the target's own vertices (the landmarks) are left alone.
//   npx tsx tmp/port_skull_test_drawing.ts        # dry run: prints the fit
//   npx tsx tmp/port_skull_test_drawing.ts --run  # writes skull-zanatomy.json
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { default_camera } from "../src/camera";
import { TabletDocument, empty_document } from "../src/document";
import { V3, v3, v3_scale } from "../src/math";
import { frankfurt_coordinates, frankfurt_frame_from_landmarks, parse_landmarks_file, parse_obj_mesh_raw } from "../src/reference";
import { MANDIBLE_NAME, SKULL_NAME, WORLD_PER_MM } from "../src/reference_skull_view";
import { apply_document_state, serialize_document_state } from "../src/persistence";

const dry_run = !process.argv.includes("--run");
const tablet_directory = path.join(path.dirname(fileURLToPath(import.meta.url)), "..");
const models_directory = path.join(tablet_directory, "../data/reference-models");
const source_path = path.join(tablet_directory, "documents/skull-test.json");
const target_path = path.join(tablet_directory, "documents/skull-zanatomy.json");

type BoundingBox = { min: V3; max: V3 };

function bounding_box(positions: V3[]): BoundingBox {
  const min = v3(Infinity, Infinity, Infinity);
  const max = v3(-Infinity, -Infinity, -Infinity);
  for (const p of positions) {
    min.x = Math.min(min.x, p.x); max.x = Math.max(max.x, p.x);
    min.y = Math.min(min.y, p.y); max.y = Math.max(max.y, p.y);
    min.z = Math.min(min.z, p.z); max.z = Math.max(max.z, p.z);
  }
  return { min, max };
}

// Which way the skull faces: the chin hangs lowest, so the mean z of the lowest tenth
// of the vertices tells whether the face is on +z or -z.
function facing_z_sign(positions: V3[], box: BoundingBox): number {
  const cutoff = box.min.y + (box.max.y - box.min.y) * 0.1;
  const low = positions.filter((p) => p.y < cutoff);
  const center_z = (box.min.z + box.max.z) / 2;
  const mean_z = low.reduce((sum, p) => sum + p.z, 0) / low.length;
  return Math.sign(mean_z - center_z);
}

function read_obj_positions(file_name: string): V3[] {
  const mesh = parse_obj_mesh_raw(fs.readFileSync(path.join(models_directory, file_name), "utf8"));
  if (mesh === null) throw new Error(`no triangles in ${file_name}`);
  return mesh.positions;
}

// Source frame: skull.obj as the draw page shows it (normalize_reference_mesh in
// src/reference.ts: height 2, base y=0, centered in x/z).
const skull_obj_raw = read_obj_positions("skull.obj");
const skull_obj_box_raw = bounding_box(skull_obj_raw);
const source_scale = 2 / (skull_obj_box_raw.max.y - skull_obj_box_raw.min.y);
const skull_obj_positions = skull_obj_raw.map((p) => v3(
  (p.x - (skull_obj_box_raw.min.x + skull_obj_box_raw.max.x) / 2) * source_scale,
  (p.y - skull_obj_box_raw.min.y) * source_scale,
  (p.z - (skull_obj_box_raw.min.z + skull_obj_box_raw.max.z) / 2) * source_scale,
));
const source_box = bounding_box(skull_obj_positions);

// Target frame: Z-Anatomy skull + mandible as the skull-draw page shows them.
const skull_landmarks = parse_landmarks_file(fs.readFileSync(path.join(models_directory, `${SKULL_NAME}.landmarks.txt`), "utf8"));
const frame = frankfurt_frame_from_landmarks(skull_landmarks);
if (frame === null) throw new Error("skull landmarks file lacks porion_l / porion_r / orbitale");
const zanatomy_positions = [...read_obj_positions(`${SKULL_NAME}.obj`), ...read_obj_positions(`${MANDIBLE_NAME}.obj`)]
  .map((p) => v3_scale(frankfurt_coordinates(frame, p), WORLD_PER_MM));
const target_box = bounding_box(zanatomy_positions);

const format_box = (box: BoundingBox) =>
  `x ${box.min.x.toFixed(3)}..${box.max.x.toFixed(3)}  y ${box.min.y.toFixed(3)}..${box.max.y.toFixed(3)}  z ${box.min.z.toFixed(3)}..${box.max.z.toFixed(3)}`;
console.log(`source (skull.obj, draw page):      ${format_box(source_box)}  faces z${facing_z_sign(skull_obj_positions, source_box) > 0 ? "+" : "-"}`);
console.log(`target (Z-Anatomy, skull-draw page): ${format_box(target_box)}  faces z${facing_z_sign(zanatomy_positions, target_box) > 0 ? "+" : "-"}`);
if (facing_z_sign(skull_obj_positions, source_box) !== facing_z_sign(zanatomy_positions, target_box)) {
  throw new Error("the two skulls face opposite ways; this script does no rotation");
}

// Uniform scale from the heights, translation from the box centers.
const scale = (target_box.max.y - target_box.min.y) / (source_box.max.y - source_box.min.y);
const center = (box: BoundingBox) => v3((box.min.x + box.max.x) / 2, (box.min.y + box.max.y) / 2, (box.min.z + box.max.z) / 2);
const source_center = center(source_box);
const target_center = center(target_box);
const port_position = (p: V3): V3 => v3(
  target_center.x + (p.x - source_center.x) * scale,
  target_center.y + (p.y - source_center.y) * scale,
  target_center.z + (p.z - source_center.z) * scale,
);
console.log(`scale ${scale.toFixed(4)}, source center ${JSON.stringify(source_center)} -> target center ${JSON.stringify(target_center)}`);

const source: TabletDocument = empty_document();
apply_document_state(fs.readFileSync(source_path, "utf8"), source, default_camera());
const target: TabletDocument = empty_document();
const target_camera = default_camera();
apply_document_state(fs.readFileSync(target_path, "utf8"), target, target_camera);
console.log(`source: ${source.strokes.length} strokes, ${source.vertices.length} vertices, ${source.patches.length} patches, ${source.vertex_pins.length} pins, ${source.smooth_knots.length} knots`);
console.log(`target before: ${target.strokes.length} strokes, ${target.vertices.length} vertices (${target.vertices.filter((v) => v.name !== undefined).length} named)`);

const vertex_offset = target.next_vertex_id;
const stroke_offset = target.next_stroke_id;
for (const vertex of source.vertices) {
  target.vertices.push({ ...vertex, id: vertex.id + vertex_offset, position: port_position(vertex.position) });
}
for (const stroke of source.strokes) {
  target.strokes.push({
    ...stroke,
    id: stroke.id + stroke_offset,
    p0_vertex: stroke.p0_vertex + vertex_offset,
    p3_vertex: stroke.p3_vertex + vertex_offset,
    d0: v3_scale(stroke.d0, scale), // handle offsets: scale only, no translation
    d3: v3_scale(stroke.d3, scale),
  });
}
for (const patch of source.patches) target.patches.push({ strokes: patch.strokes.map((id) => id + stroke_offset) });
for (const pin of source.vertex_pins) target.vertex_pins.push({ ...pin, vertex: pin.vertex + vertex_offset, host_stroke: pin.host_stroke + stroke_offset });
for (const knot of source.smooth_knots) {
  target.smooth_knots.push({ vertex: knot.vertex + vertex_offset, stroke_a: knot.stroke_a + stroke_offset, stroke_b: knot.stroke_b + stroke_offset });
}
target.next_vertex_id += source.next_vertex_id;
target.next_stroke_id += source.next_stroke_id;
console.log(`target after: ${target.strokes.length} strokes, ${target.vertices.length} vertices, ${target.patches.length} patches`);
const ported_box = bounding_box(target.vertices.filter((v) => v.name === undefined).map((v) => v.position));
console.log(`ported vertices: ${format_box(ported_box)}`);

if (dry_run) {
  console.log("dry run — pass --run to write");
} else {
  fs.writeFileSync(target_path, serialize_document_state(target, target_camera));
  console.log(`wrote ${target_path}`);
}
