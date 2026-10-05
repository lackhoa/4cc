// Page `fit-head-to-loomis-girl` (plan-loomis-girl-elaboration.md): the fit of the template
// head (skull and skin drawing, document `skull-zanatomy`) to Loomis's school girl plate,
// read top to bottom: the template on the plate, the fitted document `loomis-school-girl`
// on the plate, the fitted head turned, and how far each landmark is from its mark.
// View only: nothing on this page writes a document. The page loomis-girl-fit-landmarks
// edits the landmarks and writes `loomis-school-girl` from `skull-zanatomy`.
import "../../pages.css";
import { OrbitCamera } from "../../src/camera";
import { TabletDocument, bezier_point, stroke_control_points } from "../../src/document";
import { V3, v3 } from "../../src/math";
import { attach_orbit_controls } from "../../src/explainer/orbit_controls";
import { canvas_view, stroke_polyline } from "../../src/explainer/canvas_view";
import {
  fit_landmark_mark_world_position, front_pixel_from_world, profile_pixel_from_world, template_point_world_position, template_position_is_on_midline,
} from "../../src/loomis_girl_plate";
import { PlateViewsLandmarks, start_loomis_girl_plate_views } from "./loomis_girl_plate_views";

const TEMPLATE_DOCUMENT_NAME = "skull-zanatomy";
const FITTED_DOCUMENT_NAME = "loomis-school-girl";
const SKIN_STROKE_STYLE = "#ff6b6e";
const SKULL_STROKE_STYLE = "#7fb3ff";

let fitted_document: TabletDocument | null = null;

// ---- the fitted head, turned --------------------------------------------------------
// Lines only, nothing hides anything: the far side of the head shows through the near side.

const camera: OrbitCamera = { pivot: v3(0, 0.2, 0), yaw: 0.6, pitch: 0.05, distance: 3.8 };
const turned_canvas = document.getElementById("turned_canvas") as HTMLCanvasElement;
const turned_skull_checkbox = document.getElementById("turned_skull_visible") as HTMLInputElement;

function draw_turned_view(): void {
  const view = canvas_view(turned_canvas, camera);
  if (fitted_document === null) return;
  // The document holds one side of the head; the other side is its mirror.
  const mirror = (p: V3): V3 => v3(-p.x, p.y, p.z);
  for (const stroke of fitted_document.strokes) {
    if (stroke.layer === "skull" && !turned_skull_checkbox.checked) continue;
    const control_points = stroke_control_points(stroke, fitted_document);
    const points: V3[] = [];
    for (let i = 0; i <= 24; i++) points.push(bezier_point(control_points, i / 24));
    const style = stroke.layer === "skin" ? SKIN_STROKE_STYLE : SKULL_STROKE_STYLE;
    const line_width = stroke.layer === "skin" ? 1.6 : 1;
    stroke_polyline(view, points, false, style, line_width);
    stroke_polyline(view, points.map(mirror), false, style, line_width);
  }
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
draw_turned_view();

start_loomis_girl_plate_views(
  [
    { document_name: TEMPLATE_DOCUMENT_NAME, front_canvas_id: "template_front_canvas", profile_canvas_id: "template_profile_canvas" },
    { document_name: FITTED_DOCUMENT_NAME, front_canvas_id: "fitted_front_canvas", profile_canvas_id: "fitted_profile_canvas" },
  ],
  TEMPLATE_DOCUMENT_NAME,
  (document_name, tablet_document, landmarks) => {
    // The template decides which landmarks are midline ones, so its arrival changes the table too.
    if (document_name === FITTED_DOCUMENT_NAME) fitted_document = tablet_document;
    draw_turned_view();
    update_landmark_table(landmarks);
  },
);
