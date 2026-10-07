// Undoes the data version of the hairline on the template `skull-zanatomy`
// (plan-skin-hairline-landmark.md step 5): the hairline is computed per frame now
// (src/hairline.ts), so the two hairline strokes and their three free vertices go, and
// the rule's two inputs get their names: `brow` on the glabella vertex and `nose_base`
// on the under-nose-tip vertex. Re-running is a no-op.
//   npx tsx tmp/retire_hairline_data.ts
// Node-only (no dev server).
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { default_camera } from "../src/camera";
import { TabletDocument, delete_stroke, empty_document, vertex_by_id } from "../src/document";
import { apply_document_state, serialize_document_state } from "../src/persistence";

const tablet_directory = path.join(path.dirname(fileURLToPath(import.meta.url)), "..");
const document_path = path.join(tablet_directory, "documents/skull-zanatomy.json");

const HAIRLINE_STROKE_NAMES = ["hairline", "hairline side"];
const HAIRLINE_VERTEX_NAMES = ["hairline", "hairline_temple", "hairline_sideburn"];
const RULE_INPUT_NAMES: { vertex: number; name: string }[] = [
  { vertex: 113, name: "brow" }, // glabella
  { vertex: 101, name: "nose_base" }, // under the nose tip
];

const tablet_document: TabletDocument = empty_document();
const camera = default_camera();
apply_document_state(fs.readFileSync(document_path, "utf8"), tablet_document, camera);
console.log(`loaded ${document_path}: ${tablet_document.vertices.length} vertices, ${tablet_document.strokes.length} strokes`);

// A named vertex survives garbage collection, so the names go first; then deleting
// the strokes collects the vertices (nothing else references them).
for (const vertex of tablet_document.vertices) {
  if (vertex.name !== undefined && HAIRLINE_VERTEX_NAMES.includes(vertex.name)) {
    console.log(`vertex ${vertex.id} "${vertex.name}": name dropped`);
    delete vertex.name;
    delete vertex.midline;
  }
}
for (const stroke of [...tablet_document.strokes]) {
  if (stroke.name !== undefined && HAIRLINE_STROKE_NAMES.includes(stroke.name)) {
    console.log(`stroke ${stroke.id} "${stroke.name}": deleted`);
    delete_stroke(tablet_document, stroke.id);
  }
}
for (const { vertex, name } of RULE_INPUT_NAMES) {
  const v = vertex_by_id(tablet_document, vertex);
  if (v.name !== undefined && v.name !== name) throw new Error(`vertex ${vertex} is already named "${v.name}"`);
  v.name = name;
  console.log(`vertex ${vertex} named "${name}" (y ${v.position.y.toFixed(3)})`);
}

fs.writeFileSync(document_path, serialize_document_state(tablet_document, camera));
console.log(`${tablet_document.vertices.length} vertices, ${tablet_document.strokes.length} strokes -> ${document_path}`);
