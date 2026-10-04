// Writes an HTML page showing the Loomis girl plate with a pixel grid on top, so landmark
// pixel positions can be read off a screenshot. The image is embedded as a data URI.
//   npx tsx tmp/render_loomis_girl_pixel_grid.ts
// Open tmp/render-loomis-girl-pixel-grid.html#scale=3&x=200&y=300 through the dev server:
// scale = zoom factor, x and y = image pixel shown at the top-left corner.
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";

const tablet_directory = path.join(path.dirname(fileURLToPath(import.meta.url)), "..");
const image_path = path.join(os.homedir(), "Downloads/autodraw-loomis-girl/loomis-girl-front-and-profile.webp");
const image_base64 = fs.readFileSync(image_path).toString("base64");

const IMAGE_WIDTH = 1230;
const IMAGE_HEIGHT = 740;
let grid = "";
for (let x = 0; x <= IMAGE_WIDTH; x += 10) {
  const major = x % 50 === 0;
  grid += `<line x1="${x}" y1="0" x2="${x}" y2="${IMAGE_HEIGHT}" stroke="${major ? "#ff3b3b" : "#3b8bff"}" stroke-opacity="${major ? 0.7 : 0.25}" stroke-width="0.3"/>`;
  if (major) for (let y = 50; y <= IMAGE_HEIGHT; y += 100) grid += `<text x="${x + 1}" y="${y - 1}" fill="#ff3b3b" font-size="6" font-family="monospace">${x}</text>`;
}
for (let y = 0; y <= IMAGE_HEIGHT; y += 10) {
  const major = y % 50 === 0;
  grid += `<line x1="0" y1="${y}" x2="${IMAGE_WIDTH}" y2="${y}" stroke="${major ? "#ff3b3b" : "#3b8bff"}" stroke-opacity="${major ? 0.7 : 0.25}" stroke-width="0.3"/>`;
  if (major) for (let x = 25; x <= IMAGE_WIDTH; x += 100) grid += `<text x="${x + 1}" y="${y - 1}" fill="#c400c4" font-size="6" font-family="monospace">${y}</text>`;
}

const html = `<!doctype html><meta charset="utf-8"><body style="background:#fff;margin:0;overflow:hidden">
<div id="plate" style="position:absolute;transform-origin:0 0;width:${IMAGE_WIDTH}px;height:${IMAGE_HEIGHT}px">
<img src="data:image/webp;base64,${image_base64}" style="position:absolute;left:0;top:0;width:${IMAGE_WIDTH}px;height:${IMAGE_HEIGHT}px">
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${IMAGE_WIDTH} ${IMAGE_HEIGHT}" style="position:absolute;left:0;top:0;width:${IMAGE_WIDTH}px;height:${IMAGE_HEIGHT}px">${grid}</svg>
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
const output_path = path.join(tablet_directory, "tmp/render-loomis-girl-pixel-grid.html");
fs.writeFileSync(output_path, html);
console.log(`wrote ${output_path}`);
