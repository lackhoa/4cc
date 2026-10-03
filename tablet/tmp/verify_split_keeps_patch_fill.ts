// Node check: a split of a line that a patch uses only up to a pin must leave
// the patch with a fill, and the saved skull-zanatomy document loads with every patch filled.
//   npx tsx tmp/verify_split_keeps_patch_fill.ts
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import { default_camera } from "../src/camera";
import { empty_document, pins_on_stroke, split_stroke } from "../src/document";
import { drop_unused_patch_strokes, pin_is_locked, resolve_patch_fill, stroke_bounds_a_patch } from "../src/patch";
import { apply_document_state } from "../src/persistence";

const json = readFileSync(new URL("../documents/skull-zanatomy.json", import.meta.url), "utf8");
const doc = empty_document();
assert.equal(apply_document_state(json, doc, default_camera()), true);
const without_fill = doc.patches.flatMap((patch, index) => (resolve_patch_fill(patch, doc) === null ? [index] : []));
console.log("patches", doc.patches.length, "without fill", without_fill, "patch 12", doc.patches[12].strokes);
assert.deepEqual(without_fill, []);
console.log("line 50 locked", stroke_bounds_a_patch(doc, 50), "line 109 locked", stroke_bounds_a_patch(doc, 109),
  "v60 pin locked", pin_is_locked(doc, 60));

// Split every patch-bounding line that carries a pin, on both sides of the pin: every patch must keep its fill.
let split_count = 0;
for (const stroke of [...doc.strokes]) {
  if (!stroke_bounds_a_patch(doc, stroke.id)) continue;
  for (const pin of [...pins_on_stroke(doc, stroke.id)]) {
    for (const t of [pin.t * 0.5, pin.t + (1 - pin.t) * 0.5]) {
      const copy = empty_document();
      apply_document_state(json, copy, default_camera());
      if (split_stroke(copy, stroke.id, t) === null) continue;
      drop_unused_patch_strokes(copy);
      const broken = copy.patches.flatMap((patch, index) => (resolve_patch_fill(patch, copy) === null ? [index] : []));
      assert.deepEqual(broken, [], `split of line ${stroke.id} at ${t} left patches without fill`);
      split_count++;
    }
  }
}
console.log("splits checked", split_count);
