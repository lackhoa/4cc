// Page `compare-two-heads` (plan-native-head-topology.md, step 3): two documents drawn
// at the same place with the same camera, one at a time, to compare their proportions
// by swapping back and forth; or both at once as lines only, each in its own color.
// The documents are only read, once, when the page loads. Which two: the query
// parameters `first` and `second`; without them the corrected head and the native head.
// (The trial `head-corrected-trial` is shown with `?first=head-corrected-trial`, once
// "write head-corrected-trial" on fit-head-to-native-head has written it again.)
import "../../pages.css";
import { OrbitCamera, camera_view_projection, default_camera } from "../../src/camera";
import { TabletDocument, empty_document } from "../../src/document";
import { attach_orbit_controls } from "../../src/explainer/orbit_controls";
import { HeadMeshes, create_head_meshes, draw_head_meshes, update_head_meshes } from "../../src/head_meshes";
import { v3 } from "../../src/math";
import { apply_document_state } from "../../src/persistence";
import { Rgb } from "../../src/vertex_sink";

const PAGE_NAME = "compare-two-heads";

type ComparedHead = {
  document_name: string;
  tablet_document: TabletDocument;
  revision: string;
  meshes: HeadMeshes;
  stroke_color: Rgb;
};
type ShownHeads = "first" | "second" | "both";

const query = new URLSearchParams(window.location.search);
const first_document_name = query.get("first") ?? "head-corrected";
const second_document_name = query.get("second") ?? "native-head";

const camera: OrbitCamera = { pivot: v3(0, -0.25, 0), yaw: 0, pitch: 0, distance: 4.6 };
const canvas = document.getElementById("compare_canvas") as HTMLCanvasElement;
const shown_name_label = document.getElementById("shown_name") as HTMLSpanElement;
const show_first_button = document.getElementById("show_first") as HTMLButtonElement;
const show_second_button = document.getElementById("show_second") as HTMLButtonElement;
const show_both_button = document.getElementById("show_both") as HTMLButtonElement;
const surfaces_checkbox = document.getElementById("surfaces_visible") as HTMLInputElement;
const lines_checkbox = document.getElementById("lines_visible") as HTMLInputElement;
const skull_checkbox = document.getElementById("skull_visible") as HTMLInputElement;

show_first_button.textContent = first_document_name;
show_second_button.textContent = second_document_name;

const gl = canvas.getContext("webgl");
if (gl === null) console.error(`${PAGE_NAME}: no WebGL, the heads cannot be shown`);

let first_head: ComparedHead | null = null;
let second_head: ComparedHead | null = null;
let shown_heads: ShownHeads = "first";

async function fetch_compared_head(webgl: WebGLRenderingContext, document_name: string, stroke_color: Rgb): Promise<ComparedHead | null> {
  const response = await fetch(`/api/documents/${encodeURIComponent(document_name)}`, { cache: "no-store" });
  if (!response.ok) {
    console.error(`${PAGE_NAME}: document "${document_name}" did not load: HTTP ${response.status}`);
    return null;
  }
  const tablet_document = empty_document();
  if (!apply_document_state(await response.text(), tablet_document, default_camera())) {
    console.error(`${PAGE_NAME}: document "${document_name}" is not a document this app can read`);
    return null;
  }
  console.log(`${PAGE_NAME}: "${document_name}" loaded: ${tablet_document.vertices.length} vertices, ${tablet_document.strokes.length} strokes, ${tablet_document.patches.length} patches`);
  return {
    document_name: document_name,
    tablet_document: tablet_document,
    revision: response.headers.get("X-Revision") ?? "",
    meshes: create_head_meshes(webgl),
    stroke_color: stroke_color,
  };
}

function draw_compared_head(head: ComparedHead, surfaces_visible: boolean, aspect: number): void {
  update_head_meshes(head.meshes, head.tablet_document, head.revision, camera, {
    skull_visible: skull_checkbox.checked,
    surfaces_visible: surfaces_visible,
    strokes_visible: lines_checkbox.checked,
    mirrored: head.tablet_document.is_mirrored_across_midline === true,
    surface_opacity: 1,
    skin_stroke_color: head.stroke_color,
    skull_stroke_color: { r: 0.55, g: 0.6, b: 0.7 },
  });
  draw_head_meshes(head.meshes, camera_view_projection(camera, aspect), camera);
}

function draw(): void {
  if (gl === null) return;
  const device_pixel_ratio = window.devicePixelRatio || 1;
  canvas.width = Math.round(canvas.clientWidth * device_pixel_ratio);
  canvas.height = Math.round(canvas.clientHeight * device_pixel_ratio);
  gl.viewport(0, 0, canvas.width, canvas.height);
  gl.clearColor(0.078, 0.086, 0.11, 1);
  gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
  if (canvas.height === 0) return;
  const aspect = canvas.width / canvas.height;
  // Both at once: lines only, a surface of one head would hide the lines of the other.
  const surfaces_visible = surfaces_checkbox.checked && shown_heads !== "both";
  if (first_head !== null && shown_heads !== "second") draw_compared_head(first_head, surfaces_visible, aspect);
  if (second_head !== null && shown_heads !== "first") draw_compared_head(second_head, surfaces_visible, aspect);
}

function show_heads(heads: ShownHeads): void {
  shown_heads = heads;
  show_first_button.classList.toggle("is_shown", heads === "first");
  show_second_button.classList.toggle("is_shown", heads === "second");
  show_both_button.classList.toggle("is_shown", heads === "both");
  if (heads === "first") shown_name_label.textContent = first_document_name;
  else if (heads === "second") shown_name_label.textContent = second_document_name;
  else shown_name_label.textContent = `${first_document_name} (red) + ${second_document_name} (blue)`;
  draw();
}

function swap_shown_head(): void {
  if (shown_heads === "first") show_heads("second");
  else show_heads("first");
}

function look_from(yaw: number): void {
  camera.yaw = yaw;
  camera.pitch = 0;
  draw();
}

show_first_button.addEventListener("click", () => show_heads("first"));
show_second_button.addEventListener("click", () => show_heads("second"));
show_both_button.addEventListener("click", () => show_heads("both"));
document.getElementById("view_front")!.addEventListener("click", () => look_from(0));
document.getElementById("view_three_quarter")!.addEventListener("click", () => look_from(0.6));
document.getElementById("view_profile")!.addEventListener("click", () => look_from(Math.PI / 2));
for (const checkbox of [surfaces_checkbox, lines_checkbox, skull_checkbox]) checkbox.addEventListener("change", draw);

window.addEventListener("keydown", (event) => {
  const key = event.key.toLowerCase();
  if (key === " ") {
    // A focused button would also be clicked by Space.
    event.preventDefault();
    swap_shown_head();
  } else if (key === "1") show_heads("first");
  else if (key === "2") show_heads("second");
  else if (key === "3") show_heads("both");
  else if (key === "f") look_from(0);
  else if (key === "t") look_from(0.6);
  else if (key === "p") look_from(Math.PI / 2);
});
window.addEventListener("keyup", (event) => {
  if (event.key === " ") event.preventDefault();
});
window.addEventListener("resize", draw);
attach_orbit_controls([canvas], camera, draw);

async function start(): Promise<void> {
  if (gl === null) return;
  first_head = await fetch_compared_head(gl, first_document_name, { r: 1, g: 0.42, b: 0.43 });
  second_head = await fetch_compared_head(gl, second_document_name, { r: 0.5, g: 0.7, b: 1 });
  if (first_head === null || second_head === null) shown_name_label.textContent = "a document did not load, see the console";
  else show_heads("first");
  draw();
}
void start();
