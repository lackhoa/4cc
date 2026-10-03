// Node check: which joins are refused on the saved skull-zanatomy document, and why.
//   npx tsx tmp/verify_join_refusals.ts
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import { default_camera } from "../src/camera";
import { empty_document } from "../src/document";
import { resolve_patch_fill } from "../src/patch";
import { apply_document_state } from "../src/persistence";
import { join_refusal_reason } from "../src/stroke_merge";

const json = readFileSync(new URL("../documents/skull-zanatomy.json", import.meta.url), "utf8");
const doc = empty_document();
assert.equal(apply_document_state(json, doc, default_camera()), true);
const reason_counts = new Map<string, number>();
for (const a of doc.strokes) for (const b of doc.strokes) {
  if (a.id === b.id) continue;
  const reason = (join_refusal_reason(doc, a.id, b.id) ?? "allowed").replace(/line \d+/, "line N");
  reason_counts.set(reason, (reason_counts.get(reason) ?? 0) + 1);
}
console.log(reason_counts);
// A patch made of just the two joined lines has no second side left after the join.
doc.patches.push({ strokes: [155, 73] });
console.log("two-line patch fills", resolve_patch_fill(doc.patches[doc.patches.length - 1], doc) !== null);
console.log("refusal", join_refusal_reason(doc, 155, 73));
