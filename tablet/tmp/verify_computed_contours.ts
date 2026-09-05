// One-off check (plan-tablet-computed-contours.md v1 step 5): a flat patch
// facing the eye has no contour; the same patch viewed edge-on-ish still has
// none (f keeps one sign); a bulged patch seen from the side yields exactly
// one chain whose knots lie on the surface and whose cubics connect end to
// end. A 3-sided patch (collapsed corner) must not emit garbage at the corner.
// Usage: npx tsx tmp/verify_computed_contours.ts
import { add_stroke, add_vertex, empty_document, bezier_point } from "../src/document";
import { patch_surface_grid } from "../src/patch";
import { extract_contour_chains } from "../src/contour";
import { v3, v3_length, v3_sub } from "../src/math";
import { strict as assert } from "node:assert";

function square_patch(bulge: number) {
  const doc = empty_document();
  const a = add_vertex(doc, v3(0, 0, 0));
  const b = add_vertex(doc, v3(2, 0, 0));
  const c = add_vertex(doc, v3(2, 2, 0));
  const d = add_vertex(doc, v3(0, 2, 0));
  const zero = v3(0, 0, 0);
  // Bottom and top sides arch out in +z: a half-pipe along y.
  const arch = v3(0, 0, bulge);
  const bottom = add_stroke(doc, a, b, arch, arch);
  const right = add_stroke(doc, b, c, zero, zero);
  const top = add_stroke(doc, c, d, arch, arch);
  const left = add_stroke(doc, d, a, zero, zero);
  return { doc, patch: { strokes: [bottom, right, top, left] } };
}

{
  const { doc, patch } = square_patch(0);
  const grid = patch_surface_grid(patch, doc)!;
  assert.equal(extract_contour_chains(grid, v3(1, 1, 5)).length, 0);
  console.log("ok: flat patch facing the eye -> no contour");
  assert.equal(extract_contour_chains(grid, v3(1, 1, 0.3)).length, 0);
  console.log("ok: flat patch from a grazing angle -> no contour");
}

{
  const { doc, patch } = square_patch(1.5);
  const grid = patch_surface_grid(patch, doc)!;
  // Eye far to the +x side, level with the arch: the pipe turns away near its crest.
  const chains = extract_contour_chains(grid, v3(12, 1, 0.8));
  assert.equal(chains.length, 1, `expected one chain, got ${chains.length}`);
  const chain = chains[0];
  assert.ok(chain.length >= 8, `chain too short: ${chain.length} cubics`);
  for (let k = 0; k + 1 < chain.length; k++) assert.ok(v3_length(v3_sub(chain[k].p3, chain[k + 1].p0)) < 1e-9, "cubics don't connect");
  // The contour runs along y, spanning the patch; its knots stay near the crest.
  const y_span = chain[chain.length - 1].p3.y - chain[0].p0.y;
  assert.ok(Math.abs(y_span) > 1.9, `chain should span the patch in y, got ${y_span}`);
  for (const cubic of chain) {
    const mid = bezier_point(cubic, 0.5);
    assert.ok(mid.z > 0.5, `contour left the arch: z=${mid.z}`);
  }
  console.log(`ok: bulged patch from the side -> one chain of ${chain.length} cubics spanning y=${y_span.toFixed(2)}`);
}

{
  // Triangle: 3-sided Coons with the collapsed corner at vertex a.
  const doc = empty_document();
  const a = add_vertex(doc, v3(0, 0, 0));
  const b = add_vertex(doc, v3(2, 0, 0));
  const c = add_vertex(doc, v3(1, 2, 0));
  const arch = v3(0, 0, 1);
  const zero = v3(0, 0, 0);
  const patch = { strokes: [add_stroke(doc, a, b, arch, arch), add_stroke(doc, b, c, zero, zero), add_stroke(doc, c, a, arch, arch)] };
  const grid = patch_surface_grid(patch, doc)!;
  const chains = extract_contour_chains(grid, v3(10, 1, 0.5));
  for (const chain of chains) for (const cubic of chain) {
    assert.ok(v3_length(v3_sub(cubic.p0, v3(0, 0, 0))) > 0.05, "chain touches the collapsed corner");
  }
  console.log(`ok: 3-sided patch -> ${chains.length} chain(s), none at the collapsed corner`);
}
