// Draws a tablet document as two orthographic line drawings, front (x right, y up) and
// profile facing left (z left, y up), into one HTML file with inline SVG. Skin strokes are
// white, skull strokes orange, the mirrored half is drawn dim. Vertex ids are printed next
// to the skin vertices so landmarks can be picked by id.
//   npx tsx tmp/render_document_front_and_profile.ts <document-name> [labels]
// Output: tmp/render-<document-name>.html (open through the dev server).
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { default_camera } from "../src/camera";
import { TabletDocument, bezier_point, empty_document, stroke_control_points, vertex_is_on_layers, vertex_world_position } from "../src/document";
import { V3 } from "../src/math";
import { apply_document_state } from "../src/persistence";

const tablet_directory = path.join(path.dirname(fileURLToPath(import.meta.url)), "..");
const document_name = process.argv[2];
const show_labels = process.argv[3] === "labels";
if (document_name === undefined) throw new Error("usage: render_document_front_and_profile.ts <document-name> [labels]");

const tablet_document: TabletDocument = empty_document();
apply_document_state(fs.readFileSync(path.join(tablet_directory, `documents/${document_name}.json`), "utf8"), tablet_document, default_camera());

const PIXELS_PER_UNIT = 420;
const VIEW_SIZE = 900;
type View = { name: string; project: (p: V3) => { x: number; y: number } };
const views: View[] = [
  { name: "front", project: (p) => ({ x: VIEW_SIZE / 2 + p.x * PIXELS_PER_UNIT, y: VIEW_SIZE / 2 - (p.y - 0.1) * PIXELS_PER_UNIT }) },
  { name: "profile", project: (p) => ({ x: VIEW_SIZE / 2 - (p.z - 0.1) * PIXELS_PER_UNIT, y: VIEW_SIZE / 2 - (p.y - 0.1) * PIXELS_PER_UNIT }) },
];

const mirror = (p: V3): V3 => ({ x: -p.x, y: p.y, z: p.z });
let html = `<!doctype html><meta charset="utf-8"><body style="background:#14161c;margin:0">`;
for (const view of views) {
  let svg = `<svg xmlns="http://www.w3.org/2000/svg" width="${VIEW_SIZE}" height="${VIEW_SIZE}" style="display:block">`;
  for (const stroke of tablet_document.strokes) {
    const points = stroke_control_points(stroke, tablet_document);
    const samples: V3[] = [];
    for (let i = 0; i <= 24; i++) samples.push(bezier_point(points, i / 24));
    const color = stroke.layer === "skin" ? "#e6e6ef" : "#ffa94d";
    const polyline = (list: V3[], opacity: number) =>
      `<polyline fill="none" stroke="${color}" stroke-opacity="${opacity}" stroke-width="1.2" points="${list.map((p) => { const q = view.project(p); return `${q.x.toFixed(1)},${q.y.toFixed(1)}`; }).join(" ")}"/>`;
    svg += polyline(samples, stroke.layer === "skin" ? 1 : 0.5);
    if (view.name === "front") svg += polyline(samples.map(mirror), 0.3);
  }
  if (show_labels) {
    const skin = new Set(["skin"] as const);
    for (const vertex of tablet_document.vertices) {
      if (!vertex_is_on_layers(tablet_document, vertex.id, skin)) continue;
      const q = view.project(vertex_world_position(tablet_document, vertex));
      svg += `<circle cx="${q.x.toFixed(1)}" cy="${q.y.toFixed(1)}" r="2" fill="#4dd4c0"/>`;
      svg += `<text x="${(q.x + 3).toFixed(1)}" y="${(q.y - 3).toFixed(1)}" fill="#4dd4c0" font-size="11" font-family="monospace">${vertex.id}</text>`;
    }
  }
  html += svg + `</svg>`;
}
const output_path = path.join(tablet_directory, `tmp/render-${document_name}.html`);
fs.writeFileSync(output_path, html);
console.log(`wrote ${output_path}`);
