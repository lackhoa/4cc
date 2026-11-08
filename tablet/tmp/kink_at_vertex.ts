// One-off: angle (degrees) at each smooth knot of a document; 0 = smooth.
// Run: npx tsx tmp/kink_at_vertex.ts documents/test-doc.json
import { readFileSync } from "node:fs";
import { stroke_by_id, stroke_control_points, vertex_position } from "../src/document";
import { v3_dot, v3_normalize, v3_sub } from "../src/math";
const d = JSON.parse(readFileSync(process.argv[2], "utf-8")).document;
d.vertex_surface_pins ??= []; d.patches ??= [];
const handle_at = (id: number, v: number) => { const s = stroke_by_id(d, id); const c = stroke_control_points(s, d); return s.p0_vertex === v ? c.p1 : c.p2; };
for (const k of d.smooth_knots) {
  const at = vertex_position(d, k.vertex);
  const a = v3_normalize(v3_sub(handle_at(k.stroke_a, k.vertex), at)), b = v3_normalize(v3_sub(handle_at(k.stroke_b, k.vertex), at));
  console.log(`knot ${k.vertex} (${k.stroke_a},${k.stroke_b}): ${(Math.acos(Math.max(-1, Math.min(1, -v3_dot(a, b)))) * 180 / Math.PI).toFixed(4)}°  16.d3=${JSON.stringify(stroke_by_id(d,16).d3)}`);
}
