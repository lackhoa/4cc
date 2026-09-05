// One-off check (plan-tablet-partial-boundary-patches.md Q8 step 5):
// split_stroke reproduces the original curve exactly, the knot survives a
// vertex move with G1 continuity, deleting one side drops the knot record,
// and a weld of two knots leaves two records at the vertex.
// Usage: npx tsx tmp/verify_smooth_knot_split.ts
import { add_stroke, add_vertex, bezier_point, bezier_tangent, delete_stroke, empty_document, move_vertex, smooth_knots_at_vertex, smooth_strokes, split_stroke, stroke_by_id, stroke_control_points } from "../src/document";
import { v3, v3_length, v3_normalize, v3_sub, v3_dot } from "../src/math";
import { strict as assert } from "node:assert";

const doc = empty_document();
const a = add_vertex(doc, v3(0, 0, 0));
const b = add_vertex(doc, v3(3, 0, 0));
// Offsets coplanar with the chord (document invariant), so the converter
// doesn't swing anything.
const s0 = add_stroke(doc, a, b, v3(0, 1, 0.3), v3(0, -1, -0.3));
const original = stroke_control_points(stroke_by_id(doc, s0), doc);
doc.vertex_pins.push({ vertex: add_vertex(doc, bezier_point(original, 0.2)), host_stroke: s0, t: 0.2 });
doc.vertex_pins.push({ vertex: add_vertex(doc, bezier_point(original, 0.7)), host_stroke: s0, t: 0.7 });

const split_t = 0.4;
const knot = split_stroke(doc, s0, split_t);
const s1 = doc.strokes[doc.strokes.length - 1].id;
assert.deepEqual(smooth_knots_at_vertex(doc, knot), [{ vertex: knot, stroke_a: s0, stroke_b: s1 }]);
const first = stroke_control_points(stroke_by_id(doc, s0), doc);
const second = stroke_control_points(stroke_by_id(doc, s1), doc);
for (let i = 0; i <= 20; i++) {
  const t = i / 20;
  const expected = bezier_point(original, t);
  const actual = t <= split_t ? bezier_point(first, t / split_t) : bezier_point(second, (t - split_t) / (1 - split_t));
  assert.ok(v3_length(v3_sub(expected, actual)) < 1e-9, `curve differs at t=${t}`);
}
// Pins re-parameterized onto the right halves.
assert.equal(doc.vertex_pins[0].host_stroke, s0);
assert.ok(Math.abs(doc.vertex_pins[0].t - 0.5) < 1e-12);
assert.equal(doc.vertex_pins[1].host_stroke, s1);
assert.ok(Math.abs(doc.vertex_pins[1].t - 0.5) < 1e-12);
console.log("ok: split at t=0.4 reproduces the curve, pins re-parameterized");

// Moving the far vertex of s1 rotates s1's offsets; the knot must stay G1.
move_vertex(doc, b, v3(3, 1.5, -0.5));
const tangent_cosine = () => {
  const t0 = v3_normalize(bezier_tangent(stroke_control_points(stroke_by_id(doc, s0), doc), 1));
  const t1 = v3_normalize(bezier_tangent(stroke_control_points(stroke_by_id(doc, s1), doc), 0));
  return v3_dot(t0, t1);
};
assert.ok(tangent_cosine() > 1 - 1e-9, `tangents diverge after vertex move: ${tangent_cosine()}`);
// And moving the knot itself.
move_vertex(doc, knot, v3(1.5, 0.8, 0.2));
assert.ok(tangent_cosine() > 1 - 1e-9, `tangents diverge after knot move: ${tangent_cosine()}`);
console.log("ok: G1 kept through vertex moves");

// Weld: a second split chain whose knot lands on the first knot -> 2 records.
const c = add_vertex(doc, v3(0, 2, 0));
const d = add_vertex(doc, v3(3, 2, 0));
const s2 = add_stroke(doc, c, d, v3(0, 0.5, 0), v3(0, -0.5, 0));
const knot2 = split_stroke(doc, s2, 0.5);
for (const stroke of doc.strokes) {
  if (stroke.p0_vertex === knot2) stroke.p0_vertex = knot;
  if (stroke.p3_vertex === knot2) stroke.p3_vertex = knot;
}
for (const record of doc.smooth_knots) if (record.vertex === knot2) record.vertex = knot;
assert.equal(smooth_knots_at_vertex(doc, knot).length, 2);
console.log("ok: welded crossing holds two knots");

// Delete one side of the first knot -> its record goes, the other stays.
delete_stroke(doc, s1);
assert.deepEqual(smooth_knots_at_vertex(doc, knot).map((record) => record.stroke_a), [s2]);
console.log("ok: deleting one side drops the knot");

// Split at a pinned vertex: that vertex becomes the knot, its pin is dropped,
// and a stroke ending there stays attached.
const pinned = doc.vertex_pins[0]; // was at 0.2 on the original, now 0.5 on s0
const hanger = add_stroke(doc, pinned.vertex, add_vertex(doc, v3(1, 3, 0)), v3(0, 1, 0), v3(0, -1, 0));
const pin_count_before = doc.vertex_pins.length;
assert.equal(split_stroke(doc, s0, pinned.t, pinned.vertex), pinned.vertex);
assert.equal(doc.vertex_pins.length, pin_count_before - 1);
assert.equal(stroke_by_id(doc, hanger).p0_vertex, pinned.vertex);
assert.equal(smooth_knots_at_vertex(doc, pinned.vertex).length, 1);
assert.equal(split_stroke(doc, s0, 0.001), null);
console.log("ok: split at a pinned vertex reuses it; end cuts refused");

// smooth_strokes: two strokes meeting at a corner become G1, the first keeps
// its tangent; unrelated or repeated pairs are refused.
const corner = add_vertex(doc, v3(5, 0, 0));
const left = add_stroke(doc, add_vertex(doc, v3(4, 0, 0)), corner, v3(0, 0.2, 0), v3(0, 0.2, 0));
const right = add_stroke(doc, corner, add_vertex(doc, v3(5, 1, 0)), v3(0.3, 0, 0), v3(0.3, 0, 0));
const left_end_tangent_before = v3_normalize(bezier_tangent(stroke_control_points(stroke_by_id(doc, left), doc), 1));
assert.equal(smooth_strokes(doc, left, right), corner);
const left_end_tangent = v3_normalize(bezier_tangent(stroke_control_points(stroke_by_id(doc, left), doc), 1));
const right_start_tangent = v3_normalize(bezier_tangent(stroke_control_points(stroke_by_id(doc, right), doc), 0));
assert.ok(v3_dot(left_end_tangent, left_end_tangent_before) > 1 - 1e-9, "leader tangent moved");
assert.ok(v3_dot(left_end_tangent, right_start_tangent) > 1 - 1e-9, `corner not smooth: ${v3_dot(left_end_tangent, right_start_tangent)}`);
assert.equal(smooth_strokes(doc, left, right), null);
assert.equal(smooth_strokes(doc, right, left), null);
assert.equal(smooth_strokes(doc, left, s2), null);
console.log("ok: smooth_strokes makes a corner G1, refuses repeats/unrelated");
