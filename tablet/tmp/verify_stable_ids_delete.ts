// One-off check (plan-tablet-stable-ids.md Q6 step 4): deleting a stroke in the
// middle keeps a loft / patch / pin on LATER strokes pointing at the same curve,
// and a delete doesn't orphan or over-collect vertices.
import { add_stroke, add_vertex, delete_stroke, empty_document, stroke_by_id, stroke_control_points, vertex_by_id } from "../src/document";
import { v3 } from "../src/math";
import { strict as assert } from "node:assert";

const doc = empty_document();
// four strokes in a row along x: s0: v0->v1, s1: v1->v2, s2: v2->v3, s3: v3->v4
const vertices = [0, 1, 2, 3, 4].map((x) => add_vertex(doc, v3(x, 0, 0)));
const strokes = [0, 1, 2, 3].map((i) =>
  add_stroke(doc, vertices[i], vertices[i + 1], v3(0.3, 0.5, 0), v3(-0.3, 0.5, 0)));
// references on the LAST two strokes
doc.lofts.push({ stroke_a: strokes[2], stroke_b: strokes[3] });
doc.coons.push({ strokes: [strokes[0], strokes[1], strokes[2], strokes[3]] });
const pinned = add_vertex(doc, v3(3.5, 0.4, 0));
doc.vertex_pins.push({ vertex: pinned, host_stroke: strokes[3], t: 0.5 });
const curve_s3_before = JSON.stringify(stroke_control_points(stroke_by_id(doc, strokes[3]), doc));

delete_stroke(doc, strokes[1]);

assert.equal(doc.strokes.length, 3, "one stroke gone");
assert.equal(doc.coons.length, 0, "patch referencing the deleted stroke is dropped");
assert.equal(doc.lofts.length, 1, "loft on later strokes survives");
assert.deepEqual(doc.lofts[0], { stroke_a: strokes[2], stroke_b: strokes[3] });
assert.equal(doc.vertex_pins.length, 1, "pin on a later stroke survives");
assert.equal(doc.vertex_pins[0].host_stroke, strokes[3]);
assert.equal(JSON.stringify(stroke_control_points(stroke_by_id(doc, strokes[3]), doc)), curve_s3_before,
  "loft/pin target is still the same curve");
// vertex GC: v1 and v2 are still used by s0 / s2; pinned vertex kept; nothing else dropped
assert.equal(doc.vertices.length, 6, "no vertex collected (all still referenced)");
delete_stroke(doc, strokes[0]);
assert.throws(() => vertex_by_id(doc, vertices[0]), "v0 collected once s0 is gone");
assert.throws(() => vertex_by_id(doc, vertices[1]), "v1 collected: s0 and s1 were its only strokes");
assert.equal(vertex_by_id(doc, vertices[2]).id, vertices[2], "v2 kept by s2");
assert.equal(doc.next_stroke_id, 4, "counter never reused");
console.log("ok: delete keeps later references on the same curves");
