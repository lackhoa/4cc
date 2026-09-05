// One-off check (plan-tablet-stable-ids.md Q6 step 4): a v3 file loads through
// document_from_indexed (id = index, counters = lengths), every reference
// resolves, and a save -> load round-trip of the v4 output is stable.
// Usage: npx tsx tmp/verify_v3_to_v4_migration.ts documents/skull-test.json [--write]
import { apply_document_state, serialize_document_state } from "../src/persistence";
import { empty_document, stroke_by_id, vertex_by_id } from "../src/document";
import { default_camera } from "../src/camera";
import { readFileSync, writeFileSync } from "node:fs";
import { strict as assert } from "node:assert";

const path = process.argv[2];
const write_back = process.argv.includes("--write");
const original = readFileSync(path, "utf-8");
const original_version = JSON.parse(original).version;

const doc = empty_document();
const camera = default_camera();
assert.ok(apply_document_state(original, doc, camera), "loads");
assert.equal(doc.next_vertex_id, Math.max(-1, ...doc.vertices.map((vertex) => vertex.id)) + 1);
assert.equal(doc.next_stroke_id, Math.max(-1, ...doc.strokes.map((stroke) => stroke.id)) + 1);
for (const stroke of doc.strokes) { vertex_by_id(doc, stroke.p0_vertex); vertex_by_id(doc, stroke.p3_vertex); }
for (const pin of doc.vertex_pins) { vertex_by_id(doc, pin.vertex); stroke_by_id(doc, pin.host_stroke); }
for (const patch of doc.patches) for (const id of patch.strokes) stroke_by_id(doc, id);

const saved = serialize_document_state(doc, camera);
assert.equal(JSON.parse(saved).version, 4);
const doc2 = empty_document();
assert.ok(apply_document_state(saved, doc2, default_camera()), "v4 output loads");
assert.equal(JSON.stringify(doc2), JSON.stringify(doc), "round-trip stable");
console.log(`ok: ${path} v${original_version} -> v4, ${doc.vertices.length} vertices, ${doc.strokes.length} strokes, ${doc.vertex_pins.length} pins, ${doc.patches.length} patches`);
if (write_back) { writeFileSync(path, saved); console.log("written", path); }
