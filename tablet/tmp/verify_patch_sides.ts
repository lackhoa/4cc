// One-off check (plan-tablet-multi-select-patch.md Q8 step 5): a patch's
// fill is derived from its corner count — 4 strokes with 2 knots read as
// 4 sides (Coons), a knot removed makes 5 sides (refused until Q7), 2
// detached strokes loft, a 2-corner lens lofts, and an old-format document
// with lofts/coons loads as patches.
// Usage: npx tsx tmp/verify_patch_sides.ts
import { add_stroke, add_vertex, empty_document, smooth_strokes, split_stroke } from "../src/document";
import { resolve_patch_fill } from "../src/patch";
import { apply_document_state, serialize_document_state } from "../src/persistence";
import { default_camera } from "../src/camera";
import { v3 } from "../src/math";
import { readFileSync } from "node:fs";
import { strict as assert } from "node:assert";

// A square with the top and left sides each split in two: 6 strokes.
const doc = empty_document();
const corner_a = add_vertex(doc, v3(0, 0, 0));
const corner_b = add_vertex(doc, v3(2, 0, 0));
const corner_c = add_vertex(doc, v3(2, 2, 0));
const corner_d = add_vertex(doc, v3(0, 2, 0));
const zero = v3(0, 0, 0);
const bottom = add_stroke(doc, corner_a, corner_b, zero, zero);
const right = add_stroke(doc, corner_b, corner_c, zero, zero);
const top = add_stroke(doc, corner_d, corner_c, zero, zero); // drawn backwards on purpose
const left = add_stroke(doc, corner_a, corner_d, zero, zero);
split_stroke(doc, top, 0.5);
const top_second = doc.strokes[doc.strokes.length - 1].id;
split_stroke(doc, left, 0.5);
const left_second = doc.strokes[doc.strokes.length - 1].id;
const patch = { strokes: [right, top_second, bottom, left, top, left_second] }; // any order
const fill = resolve_patch_fill(patch, doc);
assert.ok(fill !== null && fill.kind === "coons");
assert.equal(fill.sides.length, 4);
assert.deepEqual(fill.sides.map((side) => side.length).sort(), [1, 1, 2, 2]);
console.log("ok: 6 strokes with 2 knots -> 4 sides (Coons)");

// Remove the knot on the top side: 5 corners, refused for now.
doc.smooth_knots = doc.smooth_knots.filter((knot) => knot.stroke_a !== top);
assert.equal(resolve_patch_fill(patch, doc), null);
console.log("ok: knot removed -> 5 sides, no fill");

// Two detached strokes: loft, with the second reversed when drawn crossed.
const rail_a = add_stroke(doc, add_vertex(doc, v3(5, 0, 0)), add_vertex(doc, v3(7, 0, 0)), zero, zero);
const rail_b = add_stroke(doc, add_vertex(doc, v3(7, 1, 0)), add_vertex(doc, v3(5, 1, 0)), zero, zero);
const loft = resolve_patch_fill({ strokes: [rail_a, rail_b] }, doc);
assert.ok(loft !== null && loft.kind === "loft");
assert.equal(loft.side_b[0].reversed, true);
console.log("ok: 2 detached strokes -> loft, crossed rail reversed");

// A lens: two arcs between the same corners, one of them split and smoothed.
const lens_a = add_vertex(doc, v3(10, 0, 0));
const lens_b = add_vertex(doc, v3(12, 0, 0));
const arc_upper = add_stroke(doc, lens_a, lens_b, v3(0, 1, 0), v3(0, 1, 0));
const arc_lower = add_stroke(doc, lens_a, lens_b, v3(0, -1, 0), v3(0, -1, 0));
split_stroke(doc, arc_lower, 0.5);
const arc_lower_second = doc.strokes[doc.strokes.length - 1].id;
const lens = resolve_patch_fill({ strokes: [arc_upper, arc_lower_second, arc_lower] }, doc);
assert.ok(lens !== null && lens.kind === "loft");
assert.equal(lens.side_a.length + lens.side_b.length, 3);
// Both sides run the same way: same start vertex.
const side_start = (side: typeof lens.side_a) => (side[0].reversed ? side[0].stroke.p3_vertex : side[0].stroke.p0_vertex);
assert.equal(side_start(lens.side_a), side_start(lens.side_b));
console.log("ok: lens with a knotted side -> loft of two chains");

// Three strokes that don't close: nothing.
assert.equal(resolve_patch_fill({ strokes: [bottom, right, rail_a] }, doc), null);
// A closed ring with every junction smooth: nothing.
const ring_a = add_vertex(doc, v3(20, 0, 0));
const ring_b = add_vertex(doc, v3(22, 0, 0));
const ring_upper = add_stroke(doc, ring_a, ring_b, v3(0, 1, 0), v3(0, 1, 0));
const ring_lower = add_stroke(doc, ring_b, ring_a, v3(0, -1, 0), v3(0, -1, 0));
smooth_strokes(doc, ring_upper, ring_lower);
smooth_strokes(doc, ring_lower, ring_upper);
assert.equal(resolve_patch_fill({ strokes: [ring_upper, ring_lower] }, doc), null);
console.log("ok: open chain and all-smooth ring -> no fill");

// Old-format document: `lofts` + `coons` load as patches and the old keys are
// gone on save. skull-test.json carries two coons entries (as of 2026-09-05);
// a synthetic loft is added to cover that key too.
const old_json = JSON.parse(readFileSync(new URL("../documents/skull-test.json", import.meta.url), "utf8"));
const old_coons: { strokes: number[] }[] = old_json.document.coons;
old_json.document.lofts = [{ stroke_a: old_coons[0].strokes[0], stroke_b: old_coons[0].strokes[2] }];
const loaded = empty_document();
assert.ok(apply_document_state(JSON.stringify(old_json), loaded, default_camera()));
assert.equal(loaded.patches.length, old_coons.length + 1);
assert.deepEqual(loaded.patches[0].strokes, [old_coons[0].strokes[0], old_coons[0].strokes[2]]);
assert.deepEqual(loaded.patches.slice(1).map((patch) => patch.strokes), old_coons.map((coons) => coons.strokes));
for (const patch of loaded.patches.slice(1)) {
  const fill = resolve_patch_fill(patch, loaded);
  assert.ok(fill !== null && fill.kind === "coons", "skull coons patch still fills");
}
const resaved = JSON.parse(serialize_document_state(loaded, default_camera()));
assert.equal(resaved.document.lofts, undefined);
assert.equal(resaved.document.coons, undefined);
assert.equal(resaved.document.patches.length, old_coons.length + 1);
console.log("ok: old lofts/coons load as patches, old keys dropped on save");
