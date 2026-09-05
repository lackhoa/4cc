// One-off check: releasing a free vertex near another stroke's curve pins it there.
import { bezier_point, find_snap_target_stroke, nearest_point_on_stroke_world, stroke_control_points } from "../src/document";
import { begin_edit_state, edit_pen_up } from "../src/edit_mode";
import { v3_length, v3_sub } from "../src/math";
import { readFileSync } from "node:fs";

const file = JSON.parse(readFileSync("documents/test-doc.json", "utf-8"));
const doc = file.document;
// stroke 1 = vertices 0 -> 3; vertex 3 is free (vertex 2 is the existing pin on stroke 0)
const host = stroke_control_points(doc.strokes[0], doc);
const on_curve = bezier_point(host, 0.4);
console.log("far vertex 3 ->", find_snap_target_stroke(doc, 3), "(expect null)");
console.log("pinned vertex 2 ->", find_snap_target_stroke(doc, 2), "(expect null: already pinned)");
doc.vertices[3] = { x: on_curve.x + 0.02, y: on_curve.y + 0.02, z: on_curve.z };
const nearest = nearest_point_on_stroke_world(doc.strokes[0], doc, doc.vertices[3]);
console.log("nearest t ->", nearest.t.toFixed(4), "distance ->", nearest.distance.toFixed(4), "(expect t≈0.4, dist≈0.028)");
console.log("target ->", find_snap_target_stroke(doc, 3), "(expect stroke 0)");
const state = begin_edit_state(1); state.dragging = "p3";
const pins_before = doc.vertex_pins.length;
edit_pen_up(state, doc);
const pin = doc.vertex_pins[doc.vertex_pins.length - 1];
console.log("pins", pins_before, "->", doc.vertex_pins.length, "last pin:", pin, "(expect vertex 3 on stroke 0)");
console.log("vertex on curve ->", v3_length(v3_sub(doc.vertices[3], bezier_point(host, pin.t))).toExponential(2), "(expect ~0)");
console.log("vertices still", doc.vertices.length, "(expect 4, no merge)");
