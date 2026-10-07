// Probe: where does the canon hairline level set leave the forehead patch? (plan step 6)
import fs from "node:fs";
import { default_camera } from "../src/camera";
import { empty_document, stroke_by_id, stroke_control_points, bezier_point, vertex_by_id } from "../src/document";
import { patch_surface_grid } from "../src/patch";
import { extract_level_set_chains } from "../src/contour";
import { apply_document_state } from "../src/persistence";
const doc = empty_document(); apply_document_state(fs.readFileSync("documents/skull-zanatomy.json","utf8"), doc, default_camera());
const brow = doc.vertices.find(v=>v.name==="brow")!.position.y, nose = doc.vertices.find(v=>v.name==="nose_base")!.position.y;
const hy = brow + (brow - nose);
console.log("hairline y", hy.toFixed(3));
const patch = doc.patches.find(p=>p.strokes.includes(58))!;
const grid = patch_surface_grid(patch, doc)!;
console.log("grid", grid.columns, grid.rows);
for (const chain of extract_level_set_chains(grid, (p)=>p.y-hy)) {
  const a=chain[0].p0, b=chain[chain.length-1].p3;
  console.log("chain", chain.length, "cubics", a.x.toFixed(3),a.y.toFixed(3),a.z.toFixed(3), "->", b.x.toFixed(3),b.y.toFixed(3),b.z.toFixed(3));
}
for (const id of [58,63,62,65]) { const s=stroke_by_id(doc,id); const cp=stroke_control_points(s,doc);
  console.log("stroke",id, s.p0_vertex,"->",s.p3_vertex, [0,0.25,0.5,0.75,1].map(t=>{const p=bezier_point(cp,t);return `(${p.x.toFixed(2)},${p.y.toFixed(2)},${p.z.toFixed(2)})`;}).join(" ")); }
import { hairline_chain } from "../src/hairline";
const hc = hairline_chain(doc);
console.log("hairline chain:", hc.length, "cubics");
for (const c of hc.slice(-4)) console.log(" ", [c.p0,c.p3].map(p=>`(${p.x.toFixed(3)},${p.y.toFixed(3)},${p.z.toFixed(3)})`).join(" -> "));
