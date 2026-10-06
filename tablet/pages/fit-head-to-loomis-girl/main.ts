// Page `fit-head-to-loomis-girl` (plan-loomis-girl-elaboration.md): the fit of the template
// head (skull and skin drawing, document `skull-zanatomy`) to Loomis's school girl plate,
// read top to bottom: the template on the plate, the fitted document `loomis-school-girl`
// on the plate, the fitted head turned, and how far each landmark is from its mark.
// View only: nothing on this page writes a document. The page loomis-girl-fit-landmarks
// edits the landmarks and writes `loomis-school-girl` from `skull-zanatomy`.
import "../../pages.css";
import { OrbitCamera, camera_view_projection } from "../../src/camera";
import { TabletDocument } from "../../src/document";
import { V3, v3 } from "../../src/math";
import { attach_orbit_controls } from "../../src/explainer/orbit_controls";
import {
  fit_landmark_mark_world_position, front_pixel_from_world, profile_pixel_from_world, template_point_world_position, template_position_is_on_midline,
} from "../../src/loomis_girl_plate";
import { Rgb } from "../../src/vertex_sink";
import { create_head_meshes, draw_head_meshes, update_head_meshes } from "./head_meshes";
import { PlateViewsLandmarks, start_loomis_girl_plate_views } from "./loomis_girl_plate_views";

const TEMPLATE_DOCUMENT_NAME = "skull-zanatomy";
const FITTED_DOCUMENT_NAME = "loomis-school-girl";
const TURNED_SKIN_STROKE_COLOR: Rgb = { r: 1, g: 0.42, b: 0.43 };
const TURNED_SKULL_STROKE_COLOR: Rgb = { r: 0.5, g: 0.7, b: 1 };

let fitted_document: TabletDocument | null = null;

// ---- the fitted head, turned --------------------------------------------------------
// Surfaces and strokes on one WebGL canvas with depth: the surfaces hide the strokes
// behind them (head_meshes.ts).

const camera: OrbitCamera = { pivot: v3(0, 0.2, 0), yaw: 0.6, pitch: 0.05, distance: 3.8 };
const turned_canvas = document.getElementById("turned_canvas") as HTMLCanvasElement;
const turned_surfaces_checkbox = document.getElementById("turned_surfaces_visible") as HTMLInputElement;
const turned_lines_checkbox = document.getElementById("turned_lines_visible") as HTMLInputElement;
const turned_skull_checkbox = document.getElementById("turned_skull_visible") as HTMLInputElement;

const turned_gl = turned_canvas.getContext("webgl");
if (turned_gl === null) console.error("fit-head-to-loomis-girl: no WebGL, the turned head cannot be shown");
const turned_meshes = turned_gl === null ? null : create_head_meshes(turned_gl);
let fitted_document_revision = 0; // counts the loads of the fitted document

function draw_turned_view(): void {
  if (turned_gl === null || turned_meshes === null) return;
  const device_pixel_ratio = window.devicePixelRatio || 1;
  turned_canvas.width = Math.round(turned_canvas.clientWidth * device_pixel_ratio);
  turned_canvas.height = Math.round(turned_canvas.clientHeight * device_pixel_ratio);
  turned_gl.viewport(0, 0, turned_canvas.width, turned_canvas.height);
  turned_gl.clearColor(0.078, 0.086, 0.11, 1);
  turned_gl.clear(turned_gl.COLOR_BUFFER_BIT | turned_gl.DEPTH_BUFFER_BIT);
  if (fitted_document === null || turned_canvas.height === 0) return;
  update_head_meshes(turned_meshes, fitted_document, String(fitted_document_revision), camera, {
    skull_visible: turned_skull_checkbox.checked,
    surfaces_visible: turned_surfaces_checkbox.checked,
    strokes_visible: turned_lines_checkbox.checked,
    mirrored: true,
    surface_opacity: 1,
    skin_stroke_color: TURNED_SKIN_STROKE_COLOR,
    skull_stroke_color: TURNED_SKULL_STROKE_COLOR,
  });
  draw_head_meshes(turned_meshes, camera_view_projection(camera, turned_canvas.width / turned_canvas.height), camera);
}

// ---- landmark table -----------------------------------------------------------------
// One row per landmark: how far the fitted point is from its mark, in plate pixels, in
// each view.

function update_landmark_table(landmarks: PlateViewsLandmarks): void {
  if (fitted_document === null) return;
  const tablet_document = fitted_document;
  const tbody = document.querySelector("#landmark_table tbody")!;
  tbody.textContent = "";
  for (const landmark of landmarks.landmarks_file.landmarks) {
    const read_from = landmark.read_from_front && landmark.read_from_profile ? "both views" : landmark.read_from_front ? "front only" : landmark.read_from_profile ? "profile only" : "guess";
    const cells = [landmark.name, read_from];
    const template_position = template_point_world_position(landmarks.template_document, landmark.template_point);
    const fitted = template_point_world_position(tablet_document, landmark.template_point);
    if (template_position === null || fitted === null) cells.push("template point missing", "");
    else {
      const marked = fit_landmark_mark_world_position(landmark, template_position_is_on_midline(template_position));
      const miss_pixels = (pixel_from_world: (p: V3) => { x: number; y: number }): string => {
        const a = pixel_from_world(fitted), b = pixel_from_world(marked);
        return Math.hypot(a.x - b.x, a.y - b.y).toFixed(1);
      };
      cells.push(miss_pixels((p) => front_pixel_from_world(p, landmarks.landmarks_file.front_midline_column_pixels)), miss_pixels(profile_pixel_from_world));
    }
    const row = document.createElement("tr");
    for (const cell of cells) row.appendChild(document.createElement("td")).textContent = cell;
    tbody.appendChild(row);
  }
}

// ---- wiring -------------------------------------------------------------------------

attach_orbit_controls([turned_canvas], camera, draw_turned_view);
turned_skull_checkbox.addEventListener("change", draw_turned_view);
turned_surfaces_checkbox.addEventListener("change", draw_turned_view);
turned_lines_checkbox.addEventListener("change", draw_turned_view);
draw_turned_view();

start_loomis_girl_plate_views(
  [
    { document_name: TEMPLATE_DOCUMENT_NAME, front_view_id: "template_front_view", profile_view_id: "template_profile_view" },
    { document_name: FITTED_DOCUMENT_NAME, front_view_id: "fitted_front_view", profile_view_id: "fitted_profile_view" },
  ],
  TEMPLATE_DOCUMENT_NAME,
  (document_name, tablet_document, landmarks) => {
    // The template decides which landmarks are midline ones, so its arrival changes the table too.
    if (document_name === FITTED_DOCUMENT_NAME) {
      fitted_document = tablet_document;
      fitted_document_revision++;
    }
    draw_turned_view();
    update_landmark_table(landmarks);
  },
);
