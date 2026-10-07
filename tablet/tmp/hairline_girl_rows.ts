// Probe: where does the computed hairline of the fitted girl land on the plate? (plan step 7)
import fs from "node:fs";
import { default_camera } from "../src/camera";
import { empty_document } from "../src/document";
import { hairline_chain } from "../src/hairline";
import { apply_document_state } from "../src/persistence";
import { front_pixel_from_world, profile_pixel_from_world } from "../src/loomis_girl_plate";
const doc = empty_document(); apply_document_state(fs.readFileSync("documents/loomis-school-girl.json","utf8"), doc, default_camera());
const chain = hairline_chain(doc);
const pts = [chain[0].p0, chain[15]?.p3 ?? chain[chain.length-1].p0, chain[chain.length-1].p3];
for (const [label,p] of [["midline",pts[0]],["temple exit",pts[1]],["sideburn end",pts[2]]] as const) {
  const f = front_pixel_from_world(p, 302), s = profile_pixel_from_world(p);
  console.log(label, `front (${f.x.toFixed(0)}, ${f.y.toFixed(0)})  profile col ${s.x.toFixed(0)}`);
}
