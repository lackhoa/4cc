// Page `fit-head-to-native-head` (plan-native-head-topology.md, step 3): the fit of the scan
// head (document `skull-zanatomy`) onto the native head (document `native-head`) by the
// landmarks of fit-landmarks/native-head.json, and the place where those landmarks are tuned.
// Works like fit-head-to-loomis-girl, with the native head in place of the plate: a
// landmark is one point of the scan head plus its target on the native head, a vertex of
// it or a fixed position. Top to bottom: the landmark bar (undo, reset, add, the selected
// landmark's name and remove, names, the fit status), the fitted head over the native head
// with the targets to drag (a new landmark's template point is picked there, on the red
// lines; a target let go near a blue vertex is pinned to it) and the legend under them;
// then, in the closed "More views" fold, the stiffness and skin sliders, the head model,
// the fitted head turned, and how far each landmark is from its target. The views in the fold are drawn when it opens.
// The fit is the one of tmp/make_corrected_head_document.ts (src/native_head_fit.ts),
// computed here every time the landmarks or either document change. The button "write
// head-corrected-trial" in the landmark bar writes the document head-corrected-trial from
// the landmarks as they are on the page, built by src/corrected_head_document.ts as the
// script builds it.
// Every landmark edit is saved to the fit-landmarks file at once. Both documents are drawn
// on other pages; this page polls them and refits when one changed.
import "../../pages.css";
import { OrbitCamera, camera_view_projection, default_camera } from "../../src/camera";
import { corrected_head_document_from_landmarks, corrected_head_document_text } from "../../src/corrected_head_document";
import { TabletDocument, empty_document } from "../../src/document";
import { attach_orbit_controls } from "../../src/explainer/orbit_controls";
import { create_fit_landmark_editor } from "../../src/fit_landmark_editing";
import { start_head_model_pane } from "../../src/head_model_pane";
import { create_head_meshes, draw_head_meshes, update_head_meshes } from "../../src/head_meshes";
import { TemplatePoint, template_point_world_position, template_position_is_on_midline } from "../../src/loomis_girl_plate";
import { PlateMark, PlateViewsAdapter, PlateViewsLayout, ShownDocument, start_plate_views } from "../../src/loomis_girl_plate_views";
import { V2, V3, v3, v3_length, v3_sub } from "../../src/math";
import {
  NativeHeadLandmark, NativeHeadLandmarksFile, native_head_fit, native_head_landmark_target_position, scan_head_prepared_for_native_fit,
} from "../../src/native_head_fit";
import { apply_document_state, list_documents_from_server } from "../../src/persistence";
import { Rgb } from "../../src/vertex_sink";

const PAGE_NAME = "fit-head-to-native-head";
const FIT_LANDMARKS_NAME = "native-head";
const CORRECTED_HEAD_DOCUMENT_NAME = "head-corrected-trial";
const DOCUMENT_POLL_INTERVAL_MS = 2000;
const TURNED_SKIN_STROKE_COLOR: Rgb = { r: 1, g: 0.42, b: 0.43 };
const TURNED_SKULL_STROKE_COLOR: Rgb = { r: 0.5, g: 0.7, b: 1 };
const NATIVE_HEAD_STROKE_COLOR: Rgb = { r: 0.5, g: 0.7, b: 1 };
const MILLIMETRES_PER_WORLD_UNIT = 100; // 1 unit = 10 cm

// The views' pixels: the world at a fixed scale, the native head's origin (the centre of
// its Loomis ball) at this pixel in front and in profile. +z is to the viewer's left in
// profile. The regions hold the whole head (y -1.3..0.92, |x| < 0.94, z -0.92..1.23).
const PIXELS_PER_WORLD_UNIT = 250;
const FRONT_ORIGIN_COLUMN_PIXELS = 300;
const PROFILE_ORIGIN_COLUMN_PIXELS = 320;
const ORIGIN_ROW_PIXELS = 300;
const NATIVE_HEAD_VIEWS_LAYOUT: PlateViewsLayout = {
  pixels_per_world_unit: PIXELS_PER_WORLD_UNIT,
  front_region: { left: 50, top: 50, width: 500, height: 600 },
  profile_region: { left: 0, top: 50, width: 560, height: 600 },
  plate_image: null,
  background_stroke_color: NATIVE_HEAD_STROKE_COLOR,
  // The app's dark background (the clear color of the other views).
  paper_style: "#14161c",
  label_outline_style: "#14161c",
  skin_stroke_color: TURNED_SKIN_STROKE_COLOR,
  skull_stroke_color: { r: 0.75, g: 0.75, b: 0.8 }, // grey: the native head's strokes are blue
  landmark_style: "#4cd97b",
  selected_landmark_style: "#ff5fb8",
  landmark_pin_distance_pixels: 8,
};

// ---- state --------------------------------------------------------------------------

// The scan head as the fit warps it (scan_head_prepared_for_native_fit); its revision is
// the scan head's on the server; "" = not loaded.
let template_document: TabletDocument = empty_document();
let template_revision = "";
// The scan head as loaded, before scan_head_prepared_for_native_fit: what the corrected
// head is built from (corrected_head_document_from_landmarks prepares it itself). Null = not loaded.
let loaded_scan_head: TabletDocument | null = null;
let native_head: ShownDocument | null = null;
// The landmarks as the server had them when the page loaded: what "reset" goes back to.
// Until they are loaded, the page shows no landmarks and saves nothing.
let loaded_landmarks_file: NativeHeadLandmarksFile | null = null;
let fitted: ShownDocument | null = null; // the last fit that worked
let fit_count = 0; // counts the fits: the revision of `fitted`
let landmarks_can_be_fitted = true;
let unplaced_landmark_messages: string[] = [];
let failure_message = ""; // the last load or save that failed; cleared by the next save that works
let corrected_head_write_message = ""; // how the last "write head-corrected-trial" went; "" = not pressed yet

const names_checkbox = document.getElementById("landmark_names_visible") as HTMLInputElement;
const skull_checkbox = document.getElementById("plate_skull_visible") as HTMLInputElement;
const stiffness_slider = document.getElementById("fit_stiffness") as HTMLInputElement;
const stiffness_value = document.getElementById("fit_stiffness_value")!;
const fit_status = document.getElementById("fit_status")!;
const write_corrected_head_button = document.getElementById("write_corrected_head_button") as HTMLButtonElement;

// ---- the landmark file --------------------------------------------------------------

function empty_landmarks_file(): NativeHeadLandmarksFile {
  return { about: "", template_document: "skull-zanatomy", target_document: "native-head", stiffness: 0, landmarks: [] };
}

// One landmark per line, as the file is written by hand.
function landmarks_file_text(file: NativeHeadLandmarksFile): string {
  const landmark_lines = file.landmarks.map((landmark) => `  ${JSON.stringify(landmark)}`);
  const lines = [
    "{",
    ` "about": ${JSON.stringify(file.about)},`,
    ` "template_document": ${JSON.stringify(file.template_document)},`,
    ` "target_document": ${JSON.stringify(file.target_document)},`,
    ` "stiffness": ${JSON.stringify(file.stiffness)},`,
    ` "landmarks": [`,
    landmark_lines.join(",\n"),
    " ]",
    "}",
  ];
  return `${lines.join("\n")}\n`;
}

// Null when the server has no such file or cannot be reached: that is an error, there is
// no starting file to fall back on.
async function fetch_landmarks_file(): Promise<NativeHeadLandmarksFile | null> {
  try {
    const response = await fetch(`/api/fit-landmarks/${FIT_LANDMARKS_NAME}`, { cache: "no-store" });
    if (!response.ok) {
      console.error(`${PAGE_NAME}: fit-landmarks/${FIT_LANDMARKS_NAME}.json: HTTP ${response.status}`);
      return null;
    }
    return (await response.json()) as NativeHeadLandmarksFile;
  } catch (error) {
    console.error(`${PAGE_NAME}: reading fit-landmarks/${FIT_LANDMARKS_NAME}.json failed`, error);
    return null;
  }
}

async function post_landmarks_file(file: NativeHeadLandmarksFile): Promise<boolean> {
  try {
    const response = await fetch(`/api/fit-landmarks/${FIT_LANDMARKS_NAME}`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: landmarks_file_text(file),
    });
    return response.ok;
  } catch (error) {
    console.error(`${PAGE_NAME}: saving fit-landmarks/${FIT_LANDMARKS_NAME}.json failed`, error);
    return false;
  }
}

// ---- landmarks ----------------------------------------------------------------------

// The stiffness slider runs 0..1; the useful stiffness values crowd near 0, so the slider is cubed.
function stiffness_from_slider(slider_value: number): number {
  return slider_value ** 3;
}

let save_queue: Promise<void> = Promise.resolve(); // one save at a time, in edit order

function save_landmarks_file(): void {
  if (loaded_landmarks_file === null) return;
  const file_to_save = structuredClone(editor.file);
  save_queue = save_queue.then(async () => {
    const is_saved = await post_landmarks_file(file_to_save);
    if (!is_saved) console.error(`${PAGE_NAME}: saving the landmarks failed`);
    failure_message = is_saved ? "" : "could not save the landmarks (is the server running?)";
    update_bottom_bar();
  });
}

// A new landmark's target is where the fitted head has its template point now: no pull on
// the fit until the target is dragged.
function new_landmark_at(template_point: TemplatePoint, name: string): NativeHeadLandmark | null {
  if (fitted === null) return null;
  const fitted_position = template_point_world_position(fitted.tablet_document, template_point);
  if (fitted_position === null) return null;
  return { name: name, template_point: template_point, target: { native_position: fitted_position }, note: "" };
}

// Undo, selection, add / remove / rename / reset and the landmark bar's buttons for them
// (the editor calls it its bottom bar; on this page it sits above the views).
const editor = create_fit_landmark_editor<NativeHeadLandmarksFile>(empty_landmarks_file(), {
  starting_file: () => structuredClone(loaded_landmarks_file ?? empty_landmarks_file()),
  new_landmark: new_landmark_at,
  on_edited: () => {
    save_landmarks_file();
    refit();
    update_bottom_bar();
    draw_everything();
  },
  on_view_changed: () => draw_everything(),
  armed_add_button_text: "tap a red line in front or profile…",
});

// ---- the fit ------------------------------------------------------------------------

// Fits the scan head to the targets as they are now. Landmarks that cannot be fitted leave
// the last fit that worked on screen.
function refit(): void {
  if (template_revision === "" || native_head === null || loaded_landmarks_file === null) return;
  const fit = native_head_fit(template_document, native_head.tablet_document, editor.file, editor.file.stiffness);
  unplaced_landmark_messages = fit.unplaced_landmark_messages;
  landmarks_can_be_fitted = fit.fitted_document !== null;
  if (fit.fitted_document === null) return;
  fit_count++;
  fitted = { tablet_document: fit.fitted_document, revision: String(fit_count) };
}

// While a mark or the stiffness is dragged: one refit per frame, however many pointer
// events the frame had.
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

// ---- landmark bar -------------------------------------------------------------------

function update_bottom_bar(): void {
  editor.update_bottom_bar();
  stiffness_slider.value = String(Math.cbrt(editor.file.stiffness));
  stiffness_value.textContent = editor.file.stiffness === 0 ? "0" : editor.file.stiffness.toPrecision(2);
  const messages: string[] = [];
  if (corrected_head_write_message !== "") messages.push(corrected_head_write_message);
  if (failure_message !== "") messages.push(failure_message);
  if (!landmarks_can_be_fitted) messages.push("these landmarks cannot be fitted (two landmarks on the same template point?): showing the last fit that worked");
  if (unplaced_landmark_messages.length > 0) messages.push(`left out of the fit: ${unplaced_landmark_messages.join("; ")}`);
  fit_status.textContent = messages.join(" | ");
}

// ---- writing head-corrected-trial ---------------------------------------------------
// The corrected head built from the landmarks as they are on the page (also an edit whose
// save has not finished), written over documents/head-corrected-trial.json. The trial is
// replaced, not edited: the write is based on whatever revision the server lists right
// before it.

async function write_corrected_head_document(): Promise<void> {
  if (loaded_scan_head === null || native_head === null || loaded_landmarks_file === null) {
    corrected_head_write_message = `${CORRECTED_HEAD_DOCUMENT_NAME} not written: the documents or the landmarks are not loaded yet`;
    update_bottom_bar();
    return;
  }
  write_corrected_head_button.disabled = true;
  try {
    const corrected_head_document = corrected_head_document_from_landmarks(loaded_scan_head, native_head.tablet_document, editor.file);
    const document_text = corrected_head_document_text(corrected_head_document.corrected_head);
    const entries = await list_documents_from_server();
    if (entries === null) throw new Error("could not list the documents (is the server running?)");
    const listed_entry = entries.find((entry) => entry.name === CORRECTED_HEAD_DOCUMENT_NAME);
    const headers: Record<string, string> = { "Content-Type": "application/json" };
    // No base revision = a new file: the server refuses to replace a file without one (vite.config.ts).
    if (listed_entry !== undefined) headers["X-Base-Revision"] = listed_entry.revision;
    const response = await fetch(`/api/documents/${CORRECTED_HEAD_DOCUMENT_NAME}`, { method: "POST", headers, body: document_text });
    if (!response.ok) {
      const response_body = (await response.json()) as { error?: string };
      throw new Error(`HTTP ${response.status}: ${response_body.error ?? "the server gave no reason"}`);
    }
    corrected_head_write_message = `${CORRECTED_HEAD_DOCUMENT_NAME} written ${new Date().toLocaleTimeString()}`;
  } catch (error) {
    console.error(`${PAGE_NAME}: writing ${CORRECTED_HEAD_DOCUMENT_NAME} failed`, error);
    const error_text = error instanceof Error ? error.message : String(error);
    corrected_head_write_message = `${CORRECTED_HEAD_DOCUMENT_NAME} not written: ${error_text}`;
  } finally {
    write_corrected_head_button.disabled = false;
    update_bottom_bar();
  }
}

// ---- views --------------------------------------------------------------------------

function image_pixel_from_world(is_front_view: boolean, position: V3): V2 {
  const row = ORIGIN_ROW_PIXELS - position.y * PIXELS_PER_WORLD_UNIT;
  if (is_front_view) return { x: FRONT_ORIGIN_COLUMN_PIXELS + position.x * PIXELS_PER_WORLD_UNIT, y: row };
  return { x: PROFILE_ORIGIN_COLUMN_PIXELS - position.z * PIXELS_PER_WORLD_UNIT, y: row };
}

// A landmark on the midline has one mark in the front view; any other has one on each side
// of the face. The template decides; a landmark whose template point is gone is on the
// midline when its target is.
function landmark_is_on_midline(landmark: NativeHeadLandmark, target_position: V3): boolean {
  const template_position = template_point_world_position(template_document, landmark.template_point);
  if (template_position === null) return target_position.x === 0;
  return template_position_is_on_midline(template_position);
}

function landmark_target_position(landmark: NativeHeadLandmark): V3 | null {
  if (native_head === null) return null;
  return native_head_landmark_target_position(native_head.tablet_document, landmark.target);
}

// No marks for a landmark whose target vertex is not in the native head.
function marks_of_landmark(is_front_view: boolean, landmark_index: number): PlateMark[] {
  const landmark = editor.file.landmarks[landmark_index];
  const target_position = landmark_target_position(landmark);
  if (target_position === null) return [];
  if (!is_front_view) return [{ landmark_index, side_sign: 1, image_pixel: image_pixel_from_world(false, target_position) }];
  if (landmark_is_on_midline(landmark, target_position)) {
    return [{ landmark_index, side_sign: 1, image_pixel: image_pixel_from_world(true, v3(0, target_position.y, target_position.z)) }];
  }
  return [1, -1].map((side_sign) => ({
    landmark_index,
    side_sign,
    image_pixel: image_pixel_from_world(true, v3(side_sign * target_position.x, target_position.y, target_position.z)),
  }));
}

// The target becomes a fixed position, starting from where it is now (a native vertex's
// position): a front drag sets x (not on the midline) and y, a profile drag z and y. A drag
// let go near a native vertex then pins it to that vertex (pin_landmark_to_vertex).
function move_mark_to(is_front_view: boolean, mark: PlateMark): void {
  const landmark = editor.file.landmarks[mark.landmark_index];
  const target_position = landmark_target_position(landmark);
  if (target_position === null) throw new Error(`landmark "${landmark.name}" has no target to move`);
  const position = { x: target_position.x, y: target_position.y, z: target_position.z };
  position.y = (ORIGIN_ROW_PIXELS - mark.image_pixel.y) / PIXELS_PER_WORLD_UNIT;
  if (is_front_view) {
    const world_x = (mark.image_pixel.x - FRONT_ORIGIN_COLUMN_PIXELS) / PIXELS_PER_WORLD_UNIT;
    if (!landmark_is_on_midline(landmark, target_position)) position.x = Math.max(0, world_x * mark.side_sign);
  } else {
    position.z = (PROFILE_ORIGIN_COLUMN_PIXELS - mark.image_pixel.x) / PIXELS_PER_WORLD_UNIT;
  }
  landmark.target = { native_position: position };
}

const native_head_views_adapter: PlateViewsAdapter = {
  scene: () => ({
    template_document,
    landmarks: editor.file.landmarks,
    selected_landmark_index: editor.selected_landmark_index,
    landmark_names_visible: names_checkbox.checked,
    background_document: native_head,
  }),
  image_pixel_from_world,
  camera_pivot_at_image_pixel: (is_front_view, image_pixel) => {
    const y = (ORIGIN_ROW_PIXELS - image_pixel.y) / PIXELS_PER_WORLD_UNIT;
    if (is_front_view) return v3((image_pixel.x - FRONT_ORIGIN_COLUMN_PIXELS) / PIXELS_PER_WORLD_UNIT, y, 0);
    return v3(0, y, (PROFILE_ORIGIN_COLUMN_PIXELS - image_pixel.x) / PIXELS_PER_WORLD_UNIT);
  },
  marks_of_landmark,
  // A cross (or a square when pinned): a target is not read off a picture.
  mark_is_read_from_view: () => true,
  move_mark_to,
  front_midline: null,
  background_vertex_pinning: {
    landmark_is_pinned: (landmark_index) => "native_vertex" in editor.file.landmarks[landmark_index].target,
    landmark_target_position: (landmark_index) => landmark_target_position(editor.file.landmarks[landmark_index]),
    pin_landmark_to_vertex: (landmark_index, native_vertex_id) => {
      editor.file.landmarks[landmark_index].target = { native_vertex: native_vertex_id };
    },
  },
};

const plate_views = start_plate_views(
  NATIVE_HEAD_VIEWS_LAYOUT,
  [
    { front_view_id: "fitted_front_view", profile_view_id: "fitted_profile_view", shown: () => fitted, marks: "editable", on_surface_skin_strokes_highlighted: false },
  ],
  native_head_views_adapter,
  {
    is_editable: () => loaded_landmarks_file !== null,
    on_drag_started: editor.remember_for_undo,
    on_dragged: schedule_refit,
    on_drag_ended: editor.landmarks_edited,
    on_landmark_selected: editor.select_landmark,
    landmark_adding_on_views: { is_armed: () => editor.is_adding_landmark, on_template_point_picked: editor.add_landmark_at },
  },
);

// ---- head model pane ----------------------------------------------------------------
// The scan head with the landmarks on it, for reference (in the fold): "add landmark"
// picks its point on the views above, not here.

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
    is_picking_template_point: false,
  }),
  { on_template_point_picked: editor.add_landmark_at, on_landmark_tapped: editor.select_landmark }, // never picks: is_picking_template_point is false
);

// ---- the fitted head, turned --------------------------------------------------------
// Surfaces and strokes on one WebGL canvas with depth: the surfaces hide the strokes
// behind them (head_meshes.ts).

const camera: OrbitCamera = { pivot: v3(0, -0.2, 0), yaw: 0.6, pitch: 0.05, distance: 4.6 };
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
    is_picture_view: false,
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
// One row per landmark: how far the fitted point is from its target, in mm. A midline
// landmark's target counts at x = 0, as the fit puts it there.

function update_landmark_table(): void {
  if (fitted === null) return;
  const fitted_document = fitted.tablet_document;
  const tbody = document.querySelector("#landmark_table tbody")!;
  tbody.textContent = "";
  for (const landmark of editor.file.landmarks) {
    const target_kind = "native_vertex" in landmark.target ? `vertex ${landmark.target.native_vertex}` : "position";
    const cells = [landmark.name, target_kind];
    const template_position = template_point_world_position(template_document, landmark.template_point);
    const fitted_position = template_point_world_position(fitted_document, landmark.template_point);
    const target_position = landmark_target_position(landmark);
    if (template_position === null || fitted_position === null) cells.push("template point missing");
    else if (target_position === null) cells.push("target vertex missing");
    else {
      const is_on_midline = template_position_is_on_midline(template_position);
      const fitted_target = is_on_midline ? v3(0, target_position.y, target_position.z) : target_position;
      cells.push((v3_length(v3_sub(fitted_position, fitted_target)) * MILLIMETRES_PER_WORLD_UNIT).toFixed(1));
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

type LoadedDocument = { tablet_document: TabletDocument; revision: string };

// The document when the server has a revision of it other than `loaded_revision`; null when
// it has not changed, or cannot be read (logged).
async function document_if_changed(document_name: string, server_revision: string, loaded_revision: string): Promise<LoadedDocument | null> {
  if (server_revision === loaded_revision) return null;
  const response = await fetch(`/api/documents/${encodeURIComponent(document_name)}`, { cache: "no-store" });
  if (!response.ok) {
    console.error(`${PAGE_NAME}: document ${document_name}: ${response.status}`);
    return null;
  }
  const loaded_document = empty_document();
  if (!apply_document_state(await response.text(), loaded_document, default_camera())) {
    console.error(`${PAGE_NAME}: document ${document_name} unreadable`);
    return null;
  }
  // The revision the response itself carries, when it has one: the list may be a save behind.
  const revision = response.headers.get("X-Revision") ?? server_revision;
  console.log(`${PAGE_NAME}: ${document_name} revision ${revision}, ${loaded_document.vertices.length} vertices`);
  return { tablet_document: loaded_document, revision: revision };
}

// Reads the scan head and the native head when the server has a revision of either that
// this page has not loaded. The documents are the ones the landmark file names.
let document_poll_is_running = false;
async function load_documents_if_changed(): Promise<void> {
  if (loaded_landmarks_file === null) return;
  if (document_poll_is_running) return;
  document_poll_is_running = true;
  const template_name = loaded_landmarks_file.template_document;
  const native_name = loaded_landmarks_file.target_document;
  try {
    const entries = await list_documents_from_server();
    const template_entry = entries === null ? undefined : entries.find((entry) => entry.name === template_name);
    const native_entry = entries === null ? undefined : entries.find((entry) => entry.name === native_name);
    if (template_entry === undefined || native_entry === undefined) {
      if (template_revision === "" || native_head === null) console.error(`${PAGE_NAME}: document ${template_name} or ${native_name} is not on the server (or the server is unreachable)`);
      return;
    }
    const scan_head = await document_if_changed(template_name, template_entry.revision, template_revision);
    if (scan_head !== null) {
      loaded_scan_head = scan_head.tablet_document;
      template_document = scan_head_prepared_for_native_fit(scan_head.tablet_document);
      template_revision = scan_head.revision;
    }
    const native_revision = native_head === null ? "" : native_head.revision;
    const loaded_native_head = await document_if_changed(native_name, native_entry.revision, native_revision);
    if (loaded_native_head !== null) native_head = loaded_native_head;
    if (scan_head === null && loaded_native_head === null) return;
    refit();
    update_bottom_bar();
    draw_everything();
  } catch (error) {
    console.error(`${PAGE_NAME}: reading ${template_name} or ${native_name} failed`, error);
  } finally {
    document_poll_is_running = false;
  }
}

async function load_landmarks_file(): Promise<void> {
  const fetched_file = await fetch_landmarks_file();
  if (fetched_file === null) {
    failure_message = `could not read fit-landmarks/${FIT_LANDMARKS_NAME}.json: nothing is shown or saved (reload the page)`;
  } else {
    loaded_landmarks_file = fetched_file;
    editor.file = structuredClone(fetched_file);
    console.log(`${PAGE_NAME}: ${editor.file.landmarks.length} landmarks`);
  }
  update_bottom_bar();
  draw_everything();
  await load_documents_if_changed();
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
write_corrected_head_button.addEventListener("click", () => void write_corrected_head_document());
skull_checkbox.addEventListener("change", draw_head_model_pane); // the plate views listen to it themselves

// The views in the fold have no size while it is closed: they are sized and drawn when it opens.
document.getElementById("more_views")!.addEventListener("toggle", draw_everything);

// The legend's swatches take the colors the views draw with.
function css_color_of_rgb(color: Rgb): string {
  return `rgb(${Math.round(color.r * 255)}, ${Math.round(color.g * 255)}, ${Math.round(color.b * 255)})`;
}
const legend_swatch_colors: { element_id: string; css_color: string }[] = [
  { element_id: "legend_native_head_swatch", css_color: css_color_of_rgb(NATIVE_HEAD_STROKE_COLOR) },
  { element_id: "legend_scan_head_swatch", css_color: css_color_of_rgb(NATIVE_HEAD_VIEWS_LAYOUT.skin_stroke_color) },
  { element_id: "legend_landmark_swatch", css_color: NATIVE_HEAD_VIEWS_LAYOUT.landmark_style },
  { element_id: "legend_selected_landmark_swatch", css_color: NATIVE_HEAD_VIEWS_LAYOUT.selected_landmark_style },
  { element_id: "legend_pinned_landmark_swatch", css_color: NATIVE_HEAD_VIEWS_LAYOUT.landmark_style },
];
for (const swatch of legend_swatch_colors) document.getElementById(swatch.element_id)!.style.color = swatch.css_color;

update_bottom_bar();
draw_everything();
void load_landmarks_file();
setInterval(() => void load_documents_if_changed(), DOCUMENT_POLL_INTERVAL_MS);
