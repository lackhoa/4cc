// Page `fit-hairline-to-loomis-girl` (plan-hairline-on-loomis-girl.md): the place where
// the hairline is drawn. On top, Loomis's school girl plate (front and profile) with the
// fitted head on it and the hairline in orange; below, the sketchpad (src/sketchpad.ts, as
// on skin-draw) on the template `skull-zanatomy`.
//
// Where the fitted head comes from: it is the template drawn below, warped so that the
// landmarks of the page fit-head-to-loomis-girl land on their marks on the plate
// (src/landmark_fit.ts). It is computed here after every edit of the template and saved
// nowhere; the document is the template.
//
// The document is the sketchpad's: it loads, edits, polls and saves it, and tells this
// page after each change (`on_document_edited`). The landmark file is read once, when the
// page opens, and never written here: the marks are changed on the fit page, and are not
// drawn here so that they do not cover the hairline.
import "../../pages.css";
import { TabletDocument, empty_document } from "../../src/document";
import { fitted_document_from_landmarks } from "../../src/landmark_fit";
import { FitLandmarksFile, LOOMIS_GIRL_FIT_LANDMARKS_NAME, fetch_fit_landmarks_file, starting_fit_landmarks_file } from "../../src/loomis_girl_plate";
import { ShownDocument, start_loomis_girl_plate_views } from "../../src/loomis_girl_plate_views";
import { v3_scale } from "../../src/math";
import { ReferenceMesh, reference_mesh_from_positions } from "../../src/reference";
import { Skull, WORLD_PER_MM, load_eyeball, load_skull } from "../../src/reference_skull_view";
import { load_skin_reference_mesh } from "../../src/skin_reference_mesh";
import { start_sketchpad } from "../../src/sketchpad";

const PAGE_NAME = "fit-hairline-to-loomis-girl";

// ---- state --------------------------------------------------------------------------

let template_document: TabletDocument | null = null; // the sketchpad's document; null until it loaded. Read only here.
let landmarks_file: FitLandmarksFile | null = null; // null until read; never changed after
let fitted: ShownDocument | null = null; // the last fit that worked
let fit_count = 0; // counts the fits: the revision of `fitted`

const status_element = document.getElementById("status")!;
function show_status(text: string): void {
  status_element.textContent = text;
}

// ---- the fit ------------------------------------------------------------------------

// Needs both the template (from the sketchpad) and the landmark file: whichever arrives
// second starts the first fit. A fit that fails leaves the last one that worked on screen.
function refit(): void {
  if (template_document === null || landmarks_file === null) return;
  const fitted_now = fitted_document_from_landmarks(template_document, landmarks_file);
  if (fitted_now === null) {
    show_status("the template cannot be fitted as it is now: showing the last fit that worked");
    return;
  }
  show_status("");
  fit_count++;
  fitted = { tablet_document: fitted_now, revision: String(fit_count) };
}

// One refit per frame, however many edits the frame had (a vertex drag edits on every pointer event).
let refit_is_scheduled = false;
function schedule_refit(): void {
  if (refit_is_scheduled) return;
  refit_is_scheduled = true;
  requestAnimationFrame(() => {
    refit_is_scheduled = false;
    refit();
    plate_views.draw();
  });
}

// ---- plate views --------------------------------------------------------------------

const scene_template_before_load = empty_document();
const scene_landmarks_before_load = starting_fit_landmarks_file(); // only its midline is used (the marks are hidden)
const plate_views = start_loomis_girl_plate_views(
  [{ front_view_id: "front_view", profile_view_id: "profile_view", shown: () => fitted, marks: "hidden", on_surface_skin_strokes_highlighted: true }],
  () => ({
    template_document: template_document ?? scene_template_before_load,
    landmarks_file: landmarks_file ?? scene_landmarks_before_load,
    selected_landmark_index: null,
    landmark_names_visible: false,
  }),
  null,
);

// ---- the sketchpad ------------------------------------------------------------------

function reference_mesh_from_skull_frame_mesh(mesh: Skull): ReferenceMesh {
  return reference_mesh_from_positions(mesh.positions.map((p) => v3_scale(p, WORLD_PER_MM)), mesh.triangle_indices);
}

// The eyeball mesh has no landmarks of its own: the skull is loaded for its Frankfurt
// frame only (the frame every document vertex was drawn in). As on skin-draw.
async function load_skull_frame_mesh(load: (page_name: string, frame: Skull["frame"]) => Promise<Skull | null>): Promise<ReferenceMesh | null> {
  const skull = await load_skull(PAGE_NAME);
  if (skull === null) return null;
  const mesh = await load(PAGE_NAME, skull.frame);
  return mesh === null ? null : reference_mesh_from_skull_frame_mesh(mesh);
}

// The keyboard goes to the sketchpad when it was touched last (Ctrl+Z undoes there), and
// to nobody after a touch anywhere else on the page.
const editor_container = document.getElementById("editor")!;
let sketchpad_owns_keyboard = true;
window.addEventListener("pointerdown", (e) => {
  sketchpad_owns_keyboard = e.target instanceof Node && editor_container.contains(e.target);
}, true);

async function load_landmarks_file(): Promise<void> {
  const fetched_file = await fetch_fit_landmarks_file(LOOMIS_GIRL_FIT_LANDMARKS_NAME);
  if (fetched_file === null) {
    console.error(`${PAGE_NAME}: reading the landmark file failed`);
    show_status("could not read the landmarks of the fit: no fitted head (reload the page)");
    return;
  }
  landmarks_file = fetched_file;
  console.log(`${PAGE_NAME}: ${landmarks_file.landmarks.length} landmarks`);
  refit();
  plate_views.draw();
}

plate_views.draw();
void load_landmarks_file();

// As on skin-draw but with its own localStorage keys: the sketchpad owns the document;
// this page reads it after each change.
start_sketchpad({
  load_reference_mesh: () => load_skin_reference_mesh(PAGE_NAME),
  load_eyeball_mesh: () => load_skull_frame_mesh(load_eyeball),
  default_document_name: "skull-zanatomy",
  storage_key_prefix: "autodraw_fit_hairline_to_loomis_girl",
  can_switch_documents: false,
  active_layer: "skin",
  locked_layers: [],
  hidden_layers: ["skull"],
  on_document_edited: (tablet_document) => {
    template_document = tablet_document;
    schedule_refit();
  },
  owns_keyboard: () => sketchpad_owns_keyboard,
});
