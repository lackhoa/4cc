// Node check: joining two lines that bound patches, on an in-memory copy of the
// saved skull-zanatomy document (the file is only read).
//   npx tsx tmp/verify_join_through_patches.ts
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import { default_camera } from "../src/camera";
import { TabletDocument, bezier_point, empty_document, nearest_point_on_stroke_world, pin_by_vertex, pin_vertex_to_stroke, smooth_strokes, split_stroke, stroke_by_id, stroke_control_points, vertex_position } from "../src/document";
import { v3_length, v3_sub } from "../src/math";
import { resolve_patch_fill } from "../src/patch";
import { apply_document_state } from "../src/persistence";
import { join_refusal_reason, merge_adjacent_strokes } from "../src/stroke_merge";

const json = readFileSync(new URL("../documents/skull-zanatomy.json", import.meta.url), "utf8");
function load_copy(): TabletDocument {
  const doc = empty_document();
  assert.equal(apply_document_state(json, doc, default_camera()), true);
  return doc;
}
function patches_without_fill(doc: TabletDocument): string[] {
  return doc.patches.filter((patch) => resolve_patch_fill(patch, doc) === null).map((patch) => patch.strokes.join(","));
}
function stroke_shape(doc: TabletDocument, stroke_id: number): string {
  // Rounded: re-deriving a pinned vertex leaves rounding noise in the last digits.
  return JSON.stringify(stroke_control_points(stroke_by_id(doc, stroke_id), doc), (_key, value) => (typeof value === "number" ? Number(value.toFixed(9)) : value));
}

for (const [kept, deleted] of [[155, 73], [73, 155]]) {
  const doc = load_copy();
  const before = load_copy();
  const patch_count = doc.patches.length;
  const unfilled_before = patches_without_fill(doc);
  const v77 = vertex_position(doc, 77);
  const start = performance.now();
  join_refusal_reason(doc, kept, deleted);
  const start_warm = performance.now();
  const reason = join_refusal_reason(doc, kept, deleted);
  console.log("  warm refusal check", (performance.now() - start_warm).toFixed(1), "ms");
  console.log(`join ${kept}+${deleted}: refusal`, reason, `(${(performance.now() - start).toFixed(1)} ms)`);
  assert.equal(reason, null);
  assert.equal(merge_adjacent_strokes(doc, kept, deleted), kept);
  assert.equal(doc.strokes.some((stroke) => stroke.id === deleted), false);
  const joined = stroke_by_id(doc, kept);
  const pin = pin_by_vertex(doc, 77)!;
  const off_curve = nearest_point_on_stroke_world(joined, doc, v77).distance;
  const moved = v3_length(v3_sub(vertex_position(doc, 77), v77));
  console.log("  joined ends", joined.p0_vertex, joined.p3_vertex, "v77 pin", pin, "off curve", off_curve, "moved", moved);
  assert.equal(pin.host_stroke, kept);
  assert.ok(off_curve < 1e-6 && moved < 1e-6);
  for (const other of [90, 141]) assert.equal(stroke_shape(doc, other), stroke_shape(before, other), `line ${other} moved`);
  assert.equal(doc.patches.length, patch_count);
  assert.deepEqual(patches_without_fill(doc), unfilled_before.map((strokes) => strokes));
  console.log("  patches with the joined line", doc.patches.filter((patch) => patch.strokes.includes(kept)).map((patch) => patch.strokes));
  assert.equal(doc.patches.some((patch) => patch.strokes.includes(deleted)), false);
  // How far the joined curve strays from the two old curves.
  let worst = 0;
  for (const old_id of [155, 73]) {
    const old_points = stroke_control_points(stroke_by_id(before, old_id), before);
    for (let i = 0; i <= 20; i++) worst = Math.max(worst, nearest_point_on_stroke_world(joined, doc, bezier_point(old_points, i / 20)).distance);
  }
  console.log("  worst distance from the old curves", worst.toFixed(4));
}

// Pins riding each line stay where they were (nearest point), knots follow.
{
  const doc = load_copy();
  // A rider on each line: split two unrelated lines' ends onto them by pinning existing free vertices.
  const rider_on_155 = split_stroke(doc, 86, 0.5)!;
  const rider_on_73 = split_stroke(doc, 60, 0.5)!;
  doc.smooth_knots = doc.smooth_knots.filter((knot) => knot.vertex !== rider_on_155 && knot.vertex !== rider_on_73);
  pin_vertex_to_stroke(doc, rider_on_155, 155);
  pin_vertex_to_stroke(doc, rider_on_73, 73);
  const rider_positions = [rider_on_155, rider_on_73].map((vertex) => vertex_position(doc, vertex));
  // A third-line knot at the shared vertex and a knot at the deleted line's far end.
  assert.equal(smooth_strokes(doc, 155, 90), 77);
  const far_knot_line = doc.strokes.find((stroke) => stroke.id !== 73 && (stroke.p0_vertex === 71 || stroke.p3_vertex === 71))!;
  assert.equal(smooth_strokes(doc, far_knot_line.id, 73), 71);
  const reason = join_refusal_reason(doc, 155, 73);
  console.log("riders + knots: refusal", reason);
  if (reason === null) {
    merge_adjacent_strokes(doc, 155, 73);
    [rider_on_155, rider_on_73].forEach((vertex, index) => {
      const pin = pin_by_vertex(doc, vertex)!;
      const drift = v3_length(v3_sub(vertex_position(doc, vertex), rider_positions[index]));
      console.log("  rider", vertex, "host", pin.host_stroke, "t", pin.t.toFixed(3), "drift", drift.toFixed(4));
      assert.equal(pin.host_stroke, 155);
    });
    console.log("  knots at 77", doc.smooth_knots.filter((knot) => knot.vertex === 77), "knots at 71", doc.smooth_knots.filter((knot) => knot.vertex === 71));
    assert.equal(doc.smooth_knots.some((knot) => knot.vertex === 77), false);
    assert.deepEqual(doc.smooth_knots.filter((knot) => knot.vertex === 71), [{ vertex: 71, stroke_a: far_knot_line.id, stroke_b: 155 }]);
  }
}

// A join that would break a patch is refused: two sides of one patch that meet at a corner
// whose patch has only those sides plus one more (the joined line cannot close a 2-side loop with itself).
{
  const doc = load_copy();
  let refused = 0, allowed = 0;
  for (const a of doc.strokes) for (const b of doc.strokes) {
    if (a.id >= b.id) continue;
    const reason = join_refusal_reason(doc, a.id, b.id);
    if (reason === null) allowed++;
    else if (reason.includes("lose its fill")) { refused++; if (refused <= 3) console.log(`  ${a.id}+${b.id}:`, reason); }
  }
  console.log("all pairs: allowed", allowed, "refused for a patch", refused);
}
console.log("ok");
