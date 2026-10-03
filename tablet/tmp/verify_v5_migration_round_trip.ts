// One-off: load the v4 skull-zanatomy document through the v5 loader and check
// every world position equals the stored v4 position (identity skull bone).
import { readFileSync } from "node:fs";
import { empty_document, vertex_world_position } from "../src/document";
import { apply_document_state, serialize_document_state } from "../src/persistence";

const raw = readFileSync("documents/skull-zanatomy.json", "utf8");
const v4 = JSON.parse(raw);
console.log("input version", v4.version, "vertices", v4.document.vertices.length, "strokes", v4.document.strokes.length);

const tablet_document = empty_document();
const camera = { theta: 0, phi: 0, distance: 1, pivot: { x: 0, y: 0, z: 0 } } as any;
if (!apply_document_state(raw, tablet_document, camera)) throw new Error("apply_document_state rejected the file");

let max_error = 0;
for (const vertex of tablet_document.vertices) {
  const old = v4.document.vertices.find((v: any) => v.id === vertex.id);
  const world = vertex_world_position(tablet_document, vertex);
  max_error = Math.max(max_error, Math.abs(world.x - old.position.x), Math.abs(world.y - old.position.y), Math.abs(world.z - old.position.z));
  if (vertex.bone_id !== "skull") throw new Error(`vertex ${vertex.id} bone ${vertex.bone_id}`);
}
for (const stroke of tablet_document.strokes) if (stroke.layer !== "skull") throw new Error(`stroke ${stroke.id} layer ${stroke.layer}`);
const reserialized = JSON.parse(serialize_document_state(tablet_document, camera));
console.log("max position error", max_error, "bones", JSON.stringify(tablet_document.bones), "out version", reserialized.version,
  "vertices", reserialized.document.vertices.length, "strokes", reserialized.document.strokes.length,
  "patches", reserialized.document.patches.length, "pins", reserialized.document.vertex_pins.length, "knots", reserialized.document.smooth_knots.length,
  "v4 patches", v4.document.patches.length, "v4 pins", v4.document.vertex_pins.length, "v4 knots", v4.document.smooth_knots.length);
