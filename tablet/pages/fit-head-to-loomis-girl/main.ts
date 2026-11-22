// Page `fit-head-to-loomis-girl` (plan-loomis-girl-elaboration.md, plan-hairline-on-loomis-girl.md):
// the fit of the template head (skull and skin drawing, document `skull-zanatomy`) to
// Loomis's school girl plate, and the place where that fit is tuned.
// A landmark is one point of the template plus where that point sits on the plate (its
// "mark" in each view). Top to bottom: the fitted head on the plate with the marks to
// drag, the landmark bar and the head model (a new landmark's template point is picked
// there), the fitted head turned, the template on the plate before the fit, and how far
// each landmark is from its mark.
// The fitted head is computed here, from the template and the landmark file, every time
// either changes; it is saved nowhere. Every landmark edit is saved to the fit-landmarks
// file at once. The template is drawn on other pages (skin-draw, fit-hairline-to-loomis-girl);
// this page polls it and refits when it changed.
import "../../pages.css";
import { OrbitCamera, camera_view_projection, default_camera } from "../../src/camera";
import { TabletDocument, empty_document } from "../../src/document";
import { attach_orbit_controls } from "../../src/explainer/orbit_controls";
import { create_fit_landmark_editor } from "../../src/fit_landmark_editing";
import { start_head_model_pane } from "../../src/head_model_pane";
import { create_head_meshes, draw_head_meshes, update_head_meshes } from "../../src/head_meshes";
import { fitted_document_from_landmarks } from "../../src/landmark_fit";
import {
  FitLandmark, FitLandmarksFile, LOOMIS_GIRL_FIT_LANDMARKS_NAME, PIXELS_PER_WORLD_UNIT, PROFILE_PORION_X_PIXELS, TemplatePoint, WORLD_Y_ZERO_ROW_PIXELS,
  fetch_fit_landmarks_file, fit_landmark_mark_world_position, front_pixel_from_world, profile_pixel_from_world, save_fit_landmarks_file,
  starting_fit_landmarks_file, template_point_world_position, template_position_is_on_midline,
} from "../../src/loomis_girl_plate";
import { ShownDocument, start_loomis_girl_plate_views } from "../../src/loomis_girl_plate_views";
import { V3, v3 } from "../../src/math";
import { apply_document_state, list_documents_from_server } from "../../src/persistence";
import { Rgb } from "../../src/vertex_sink";

const PAGE_NAME = "fit-head-to-loomis-girl";
const TEMPLATE_DOCUMENT_NAME = "skull-zanatomy";
const TEMPLATE_POLL_INTERVAL_MS = 2000;
const TURNED_SKIN_STROKE_COLOR: Rgb = { r: 1, g: 0.42, b: 0.43 };
const TURNED_SKULL_STROKE_COLOR: Rgb = { r: 0.5, g: 0.7, b: 1 };

// ---- state --------------------------------------------------------------------------

let template_document: TabletDocument = empty_document();
let template_revision = ""; // the server's revision of the template as loaded; "" = not loaded
// The saved marks are in `editor.file`. Until they are, the page shows the starting marks
// and saves nothing: a save would write the starting marks over the saved ones.
let landmarks_file_is_loaded = false;
let fitted: ShownDocument | null = null; // the last fit that worked
let fit_count = 0; // counts the fits: the revision of `fitted`
let marks_can_be_fitted = true;
let failure_message = ""; // the last load or save that failed; cleared by the next save that works

const names_checkbox = document.getElementById("landmark_names_visible") as HTMLInputElement;
const skull_checkbox = document.getElementById("plate_skull_visible") as HTMLInputElement;
const stiffness_slider = document.getElementById("fit_stiffness") as HTMLInputElement;
const stiffness_value = document.getElementById("fit_stiffness_value")!;
const fit_status = document.getElementById("fit_status")!;

// ---- landmarks ----------------------------------------------------------------------

// The stiffness slider runs 0..1; the useful stiffness values crowd near 0, so the slider is cubed.
function stiffness_from_slider(slider_value: number): number {
  return slider_value ** 3;
}

let save_queue: Promise<void> = Promise.resolve(); // one save at a time, in edit order

function save_landmarks_file(): void {
  if (!landmarks_file_is_loaded) return;
  const file_to_save = structuredClone(editor.file);
  save_queue = save_queue.then(async () => {
    const is_saved = await save_fit_landmarks_file(LOOMIS_GIRL_FIT_LANDMARKS_NAME, file_to_save);
    if (!is_saved) console.error(`${PAGE_NAME}: saving the landmarks failed`);
    failure_message = is_saved ? "" : "could not save the marks (is the server running?)";
    update_bottom_bar();
  });
}

// A new landmark starts with its marks where the template point itself is: no pull on the
// fit until the marks are dragged.
function new_landmark_at(template_point: TemplatePoint, name: string): FitLandmark | null {
  const template_position = template_point_world_position(template_document, template_point);
  if (template_position === null) return null;
  return {
    name,
    template_point,
    front_half_width_pixels: template_position_is_on_midline(template_position) ? 0 : Math.abs(template_position.x) * PIXELS_PER_WORLD_UNIT,
    row_pixels: WORLD_Y_ZERO_ROW_PIXELS - template_position.y * PIXELS_PER_WORLD_UNIT,
    profile_column_pixels: PROFILE_PORION_X_PIXELS - template_position.z * PIXELS_PER_WORLD_UNIT,
    read_from_front: false,
    read_from_profile: false,
  };
}

// Undo, selection, add / remove / rename / reset and the bottom bar's buttons for them.
const editor = create_fit_landmark_editor<FitLandmarksFile>(starting_fit_landmarks_file(), {
  starting_file: starting_fit_landmarks_file,
  new_landmark: new_landmark_at,
  on_edited: () => {
    save_landmarks_file();
    refit();
    update_bottom_bar();
    draw_everything();
  },
  on_view_changed: () => draw_everything(),
  armed_add_button_text: "tap a point on the head model…",
});

// ---- the fit ------------------------------------------------------------------------

// Fits the template to the marks as they are now. Marks that cannot be fitted leave the
// last fit that worked on screen.
function refit(): void {
  if (template_revision === "") return;
  const fitted_now = fitted_document_from_landmarks(template_document, editor.file);
  marks_can_be_fitted = fitted_now !== null;
  if (fitted_now === null) return;
  fit_count++;
  fitted = { tablet_document: fitted_now, revision: String(fit_count) };
}

// While a mark, the midline or the stiffness is dragged: one refit per frame, however
// many pointer events the frame had.
let refit_is_scheduled = false;
function schedule_refit(): void {
  if (refit_is_scheduled) return;
  refit_is_scheduled = true;
  requestAnimationFrame(() => {
    refit_is_scheduled = false;
    refit();
    update_bottom_bar();
    draw_everything();
  });
}

// ---- bottom bar ---------------------------------------------------------------------

function update_bottom_bar(): void {
  editor.update_bottom_bar();
  stiffness_slider.value = String(Math.cbrt(editor.file.stiffness));
  stiffness_value.textContent = editor.file.stiffness === 0 ? "0" : editor.file.stiffness.toPrecision(2);
  fit_status.textContent =
    failure_message !== "" ? failure_message
    : !marks_can_be_fitted ? "these marks cannot be fitted (two landmarks on the same template point?): showing the last fit that worked"
    : "";
}

// ---- plate views --------------------------------------------------------------------

const plate_views = start_loomis_girl_plate_views(
  [
    { front_view_id: "fitted_front_view", profile_view_id: "fitted_profile_view", shown: () => fitted, marks: "editable", on_surface_skin_strokes_highlighted: true },
    {
      front_view_id: "template_front_view", profile_view_id: "template_profile_view",
      shown: () => (template_revision === "" ? null : { tablet_document: template_document, revision: template_revision }),
      marks: "shown", on_surface_skin_strokes_highlighted: false,
    },
  ],
  () => ({ template_document, landmarks_file: editor.file, selected_landmark_index: editor.selected_landmark_index, landmark_names_visible: names_checkbox.checked }),
  {
    is_editable: () => landmarks_file_is_loaded,
    on_drag_started: editor.remember_for_undo,
    on_dragged: schedule_refit,
    on_drag_ended: editor.landmarks_edited,
    on_landmark_selected: editor.select_landmark,
    landmark_adding_on_views: null, // a landmark is added on the head model pane
  },
);

// ---- head model pane ----------------------------------------------------------------
// The template with the landmarks on it; "add landmark" picks a point here.

const draw_head_model_pane = start_head_model_pane(
  PAGE_NAME,
  { mesh_canvas: "head_model_mesh_canvas", overlay_canvas: "head_model_overlay_canvas", mesh_opacity_slider: "head_model_mesh_opacity" },
  () => ({
    template_document,
    document_revision: template_revision,
    landmarks: editor.file.landmarks,
    selected_landmark_index: editor.selected_landmark_index,
    landmark_names_visible: names_checkbox.checked,
    skull_visible: skull_checkbox.checked,
    is_picking_template_point: editor.is_adding_landmark,
  }),
  { on_template_point_picked: editor.add_landmark_at, on_landmark_tapped: editor.select_landmark },
);

// ---- the fitted head, turned --------------------------------------------------------
// Surfaces and strokes on one WebGL canvas with depth: the surfaces hide the strokes
// behind them (head_meshes.ts).

const camera: OrbitCamera = { pivot: v3(0, 0.2, 0), yaw: 0.6, pitch: 0.05, distance: 3.8 };
const turned_canvas = document.getElementById("turned_canvas") as HTMLCanvasElement;
const turned_surfaces_checkbox = document.getElementById("turned_surfaces_visible") as HTMLInputElement;
const turned_lines_checkbox = document.getElementById("turned_lines_visible") as HTMLInputElement;
const turned_skull_checkbox = document.getElementById("turned_skull_visible") as HTMLInputElement;

const turned_gl = turned_canvas.getContext("webgl");
if (turned_gl === null) console.error(`${PAGE_NAME}: no WebGL, the turned head cannot be shown`);
const turned_meshes = turned_gl === null ? null : create_head_meshes(turned_gl);

function draw_turned_view(): void {
  if (turned_gl === null || turned_meshes === null) return;
  const device_pixel_ratio = window.devicePixelRatio || 1;
  turned_canvas.width = Math.round(turned_canvas.clientWidth * device_pixel_ratio);
  turned_canvas.height = Math.round(turned_canvas.clientHeight * device_pixel_ratio);
  turned_gl.viewport(0, 0, turned_canvas.width, turned_canvas.height);
  turned_gl.clearColor(0.078, 0.086, 0.11, 1);
  turned_gl.clear(turned_gl.COLOR_BUFFER_BIT | turned_gl.DEPTH_BUFFER_BIT);
  if (fitted === null || turned_canvas.height === 0) return;
  update_head_meshes(turned_meshes, fitted.tablet_document, fitted.revision, camera, {
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

function update_landmark_table(): void {
  if (fitted === null) return;
  const fitted_document = fitted.tablet_document;
  const tbody = document.querySelector("#landmark_table tbody")!;
  tbody.textContent = "";
  for (const landmark of editor.file.landmarks) {
    const read_from = landmark.read_from_front && landmark.read_from_profile ? "both views" : landmark.read_from_front ? "front only" : landmark.read_from_profile ? "profile only" : "guess";
    const cells = [landmark.name, read_from];
    const template_position = template_point_world_position(template_document, landmark.template_point);
    const fitted_position = template_point_world_position(fitted_document, landmark.template_point);
    if (template_position === null || fitted_position === null) cells.push("template point missing", "");
    else {
      const marked = fit_landmark_mark_world_position(landmark, template_position_is_on_midline(template_position));
      const miss_pixels = (pixel_from_world: (p: V3) => { x: number; y: number }): string => {
        const a = pixel_from_world(fitted_position), b = pixel_from_world(marked);
        return Math.hypot(a.x - b.x, a.y - b.y).toFixed(1);
      };
      cells.push(miss_pixels((p) => front_pixel_from_world(p, editor.file.front_midline_column_pixels)), miss_pixels(profile_pixel_from_world));
    }
    const row = document.createElement("tr");
    for (const cell of cells) row.appendChild(document.createElement("td")).textContent = cell;
    tbody.appendChild(row);
  }
}

function draw_everything(): void {
  plate_views.draw();
  draw_head_model_pane();
  draw_turned_view();
  update_landmark_table();
}

// ---- loading ------------------------------------------------------------------------

// Reads the template when the server has a revision of it this page has not loaded.
let template_poll_is_running = false;
async function load_template_if_changed(): Promise<void> {
  if (template_poll_is_running) return;
  template_poll_is_running = true;
  try {
    const entries = await list_documents_from_server();
    const entry = entries === null ? undefined : entries.find((e) => e.name === TEMPLATE_DOCUMENT_NAME);
    if (entry === undefined) {
      if (template_revision === "") console.error(`${PAGE_NAME}: document ${TEMPLATE_DOCUMENT_NAME} is not on the server (or the server is unreachable)`);
      return;
    }
    if (entry.revision === template_revision) return;
    const response = await fetch(`/api/documents/${TEMPLATE_DOCUMENT_NAME}`, { cache: "no-store" });
    if (!response.ok) {
      console.error(`${PAGE_NAME}: document ${TEMPLATE_DOCUMENT_NAME}: ${response.status}`);
      return;
    }
    const loaded_document = empty_document();
    if (!apply_document_state(await response.text(), loaded_document, default_camera())) {
      console.error(`${PAGE_NAME}: document ${TEMPLATE_DOCUMENT_NAME} unreadable`);
      return;
    }
    template_document = loaded_document;
    // The revision the response itself carries, when it has one: the list may be a save behind.
    template_revision = response.headers.get("X-Revision") ?? entry.revision;
    console.log(`${PAGE_NAME}: template ${TEMPLATE_DOCUMENT_NAME} revision ${template_revision}, ${template_document.vertices.length} vertices`);
    refit();
    update_bottom_bar();
    draw_everything();
  } catch (error) {
    console.error(`${PAGE_NAME}: reading ${TEMPLATE_DOCUMENT_NAME} failed`, error);
  } finally {
    template_poll_is_running = false;
  }
}

async function load_landmarks_file(): Promise<void> {
  const fetched_file = await fetch_fit_landmarks_file(LOOMIS_GIRL_FIT_LANDMARKS_NAME);
  if (fetched_file === null) {
    failure_message = "could not read the saved marks: showing the starting ones, nothing is saved (reload the page)";
  } else {
    editor.file = fetched_file;
    landmarks_file_is_loaded = true;
    console.log(`${PAGE_NAME}: ${editor.file.landmarks.length} landmarks`);
  }
  refit();
  update_bottom_bar();
  draw_everything();
}

// ---- wiring -------------------------------------------------------------------------

attach_orbit_controls([turned_canvas], camera, draw_turned_view);
turned_skull_checkbox.addEventListener("change", draw_turned_view);
turned_surfaces_checkbox.addEventListener("change", draw_turned_view);
turned_lines_checkbox.addEventListener("change", draw_turned_view);

// Dragging the stiffness slider is one edit: remembered for undo when the drag starts, saved when it ends.
let stiffness_drag_is_remembered = false;
stiffness_slider.addEventListener("input", () => {
  if (!stiffness_drag_is_remembered) editor.remember_for_undo();
  stiffness_drag_is_remembered = true;
  editor.file.stiffness = stiffness_from_slider(Number(stiffness_slider.value));
  stiffness_value.textContent = editor.file.stiffness === 0 ? "0" : editor.file.stiffness.toPrecision(2);
  schedule_refit();
});
stiffness_slider.addEventListener("change", () => {
  stiffness_drag_is_remembered = false;
  editor.landmarks_edited();
});

names_checkbox.addEventListener("change", draw_everything);
skull_checkbox.addEventListener("change", draw_head_model_pane); // the plate views listen to it themselves

update_bottom_bar();
draw_everything();
void load_landmarks_file();
void load_template_if_changed();
setInterval(() => void load_template_if_changed(), TEMPLATE_POLL_INTERVAL_MS);
