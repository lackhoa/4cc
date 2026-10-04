// Draws a tablet document over the Loomis school girl plate, to check a fit by eye: the
// front view over the left half, the profile over the right half. Skin strokes are red,
// skull strokes blue and thin. Target landmarks are green crosses, the fitted landmark
// vertices are filled dots (they should sit on the crosses).
//   npx tsx tmp/render_loomis_girl_fit_overlay.ts <document-name> [skull]
// Output: tmp/render-loomis-girl-fit-overlay-<document-name>.html (open through the dev
// server). Hash parameters: #scale=3&x=200&y=300, x and y = image pixel at the top-left.
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { default_camera } from "../src/camera";
import { TabletDocument, bezier_point, empty_document, stroke_control_points, vertex_by_id, vertex_world_position } from "../src/document";
import { V3 } from "../src/math";
import { apply_document_state } from "../src/persistence";
import {
  IMAGE_HEIGHT_PIXELS, IMAGE_WIDTH_PIXELS, LOOMIS_GIRL_IMAGE_PATH, TARGET_LANDMARKS, TARGET_MOST_FORWARD_POINT_LANDMARKS,
  front_pixel_from_world, profile_pixel_from_world, target_landmark_world_position, target_most_forward_point_world_position,
} from "./loomis_girl_target_views";

const tablet_directory = path.join(path.dirname(fileURLToPath(import.meta.url)), "..");
const document_name = process.argv[2];
const show_skull = process.argv[3] === "skull";
if (document_name === undefined) throw new Error("usage: render_loomis_girl_fit_overlay.ts <document-name> [skull]");

const tablet_document: TabletDocument = empty_document();
apply_document_state(fs.readFileSync(path.join(tablet_directory, `documents/${document_name}.json`), "utf8"), tablet_document, default_camera());

const mirror = (p: V3): V3 => ({ x: -p.x, y: p.y, z: p.z });
const point_text = (q: { x: number; y: number }): string => `${q.x.toFixed(1)},${q.y.toFixed(1)}`;
let overlay = "";
for (const stroke of tablet_document.strokes) {
  if (stroke.layer === "skull" && !show_skull) continue;
  const points = stroke_control_points(stroke, tablet_document);
  const samples: V3[] = [];
  for (let i = 0; i <= 24; i++) samples.push(bezier_point(points, i / 24));
  const style = stroke.layer === "skin" ? `stroke="#e8262a" stroke-width="0.9" stroke-opacity="0.85"` : `stroke="#2a62e8" stroke-width="0.5" stroke-opacity="0.7"`;
  for (const list of [samples.map(front_pixel_from_world), samples.map(mirror).map(front_pixel_from_world), samples.map(profile_pixel_from_world)]) {
    overlay += `<polyline fill="none" ${style} points="${list.map(point_text).join(" ")}"/>`;
  }
}
for (const landmark of TARGET_LANDMARKS) {
  const target = target_landmark_world_position(landmark);
  const fitted = vertex_world_position(tablet_document, vertex_by_id(tablet_document, landmark.template_vertex));
  for (const project of [front_pixel_from_world, profile_pixel_from_world]) {
    const t = project(target);
    const f = project(fitted);
    overlay += `<path d="M${t.x - 4},${t.y}H${t.x + 4}M${t.x},${t.y - 4}V${t.y + 4}" stroke="#0a9a3a" stroke-width="0.8"/>`;
    overlay += `<circle cx="${f.x.toFixed(1)}" cy="${f.y.toFixed(1)}" r="1.6" fill="#0a9a3a"/>`;
  }
}

for (const landmark of TARGET_MOST_FORWARD_POINT_LANDMARKS) {
  const t = profile_pixel_from_world(target_most_forward_point_world_position(landmark));
  overlay += `<path d="M${t.x - 4},${t.y}H${t.x + 4}M${t.x},${t.y - 4}V${t.y + 4}" stroke="#0a9a3a" stroke-width="0.8"/>`;
}

const image_base64 =fs.readFileSync(LOOMIS_GIRL_IMAGE_PATH).toString("base64");
const size_style = `position:absolute;left:0;top:0;width:${IMAGE_WIDTH_PIXELS}px;height:${IMAGE_HEIGHT_PIXELS}px`;
const html = `<!doctype html><meta charset="utf-8"><body style="background:#fff;margin:0;overflow:hidden">
<div id="plate" style="position:absolute;transform-origin:0 0;width:${IMAGE_WIDTH_PIXELS}px;height:${IMAGE_HEIGHT_PIXELS}px">
<img src="data:image/webp;base64,${image_base64}" style="${size_style};opacity:0.75">
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${IMAGE_WIDTH_PIXELS} ${IMAGE_HEIGHT_PIXELS}" style="${size_style};overflow:visible">${overlay}</svg>
</div>
<script>
function place_plate() {
  const parameters = new URLSearchParams(location.hash.slice(1));
  const scale = Number(parameters.get("scale") ?? 1), x = Number(parameters.get("x") ?? 0), y = Number(parameters.get("y") ?? 0);
  document.getElementById("plate").style.transform = "scale(" + scale + ") translate(" + -x + "px," + -y + "px)";
}
window.addEventListener("hashchange", place_plate);
place_plate();
</script>`;
const output_path = path.join(tablet_directory, `tmp/render-loomis-girl-fit-overlay-${document_name}.html`);
fs.writeFileSync(output_path, html);
console.log(`wrote ${output_path}`);
