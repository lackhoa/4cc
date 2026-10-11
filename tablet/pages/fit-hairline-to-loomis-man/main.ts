// The fit-hairline-to-loomis-man page: the sketchpad (src/sketchpad.ts, as on skin-draw,
// document `skull-zanatomy`) with Loomis's planes-head plates beside it, front and
// profile, showing the head FITTED to the plates (src/loomis_planes_man_plate.ts,
// src/landmark_fit.ts): the hairline is drawn on the template in the sketchpad, as
// on-surface skin strokes, and the plate views show where it lands on the plates. Plan:
// notes/tasks/autodraw_draw_as_data/plan-hairline-reference-plate.md (Q19-Q28).
//
// A plate view is three canvases stacked in one container, as on fit-head-to-loomis-girl:
// the plate (2D), the fitted head's surfaces and strokes (WebGL, head_meshes.ts) through
// an orthographic camera that reproduces the plate's registration, and the marks and the
// highlighted skin strokes (2D) on top. The top canvas takes the gestures; one pointer
// moves the plate or drags a mark by the mode; two fingers always pinch-zoom.
//
// The marks are landmarks of the fit: each is a point of the template and where it must
// land on each plate. The registration (scale + offset per view, from the same marks)
// puts the template's points on the plates; the fit (thin-plate warp, stiffness 0) then
// moves the head so each landmark lands on its marks exactly. The fitted head is
// recomputed on every mark edit and every document edit; it is never saved, the
// document is the template.
//
// The document is the sketchpad's: it loads, edits, polls and saves it, and tells this
// page after each change (`on_document_edited`). The landmark file is read once and
// written on every change (this page is its only writer). The landmarks are edited
// through the shared editor (src/fit_landmark_editing.ts: remove, rename, undo, reset;
// there is no "add" here, the landmarks come from the starting marks and the girl
// plate's landmarks, src/loomis_planes_man_plate.ts). The keyboard belongs to whichever
// of the two (plates, sketchpad) was touched last: Ctrl+Z undoes there.
import "../../pages.css";
import { OrbitCamera, camera_distance_for_world_units_per_pixel, camera_orthographic_view_projection } from "../../src/camera";
import { TabletDocument } from "../../src/document";
import { FitLandmarkEditor, create_fit_landmark_editor } from "../../src/fit_landmark_editing";
import { fitted_document_from_landmark_pairs } from "../../src/landmark_fit";
import { V2, V3 } from "../../src/math";
import { on_surface_stroke_samples } from "../../src/patch";
import { Rgb } from "../../src/vertex_sink";
import { HeadMeshes, create_head_meshes, draw_head_meshes, update_head_meshes } from "../../src/head_meshes";
import { LOOMIS_GIRL_FIT_LANDMARKS_NAME, PlateRegion, PlateViewTransform, TemplatePoint, fetch_fit_landmarks_file, plate_view_transform_showing_region, plate_view_transform_zoomed_at, template_point_world_position } from "../../src/loomis_girl_plate";
import {
  FRONT_PLATE, LoomisPlanesManLandmarksFile, NamedMark, PROFILE_PLATE, PlateImage, PlateLandmark, PlateRegistration, STARTING_MARKS, TemplatePointMark,
  fetch_landmarks_file, landmark_mark, landmarks_from_named_marks, man_fit_pairs, pixel_from_world, plate_registration, save_landmarks_file, starting_landmarks_file,
  template_point_marks_missing_from, world_from_pixel,
} from "../../src/loomis_planes_man_plate";
import { ReferenceMesh, reference_mesh_from_positions } from "../../src/reference";
import { Skull, WORLD_PER_MM, load_eyeball, load_skull } from "../../src/reference_skull_view";
import { load_skin_reference_mesh } from "../../src/skin_reference_mesh";
import { start_sketchpad } from "../../src/sketchpad";
import { v3_scale } from "../../src/math";

const PAGE_NAME = "fit-hairline-to-loomis-man";
const SKIN_STROKE_COLOR: Rgb = { r: 0.91, g: 0.15, b: 0.16 };
const SKULL_STROKE_COLOR: Rgb = { r: 0.16, g: 0.38, b: 0.91 };
const MARK_COLOR = "#0a9a3a";
const SELECTED_MARK_COLOR = "#ff2fb4";
const HEAD_POINT_COLOR = "#ffffff";
const ON_SURFACE_STROKE_HIGHLIGHT_COLOR = "#ffa94d"; // the skin's on-surface strokes (the hairline) over the plate
const SURFACE_OPACITY = 0.35;
const MARK_GRAB_RADIUS_CSS_PIXELS = 18;

type Mode = "move" | "landmarks";
type KeyboardOwner = "plates" | "sketchpad";

type PlateView = {
  container: HTMLElement;
  plate_canvas: HTMLCanvasElement;
  mesh_canvas: HTMLCanvasElement;
  mesh_gl: WebGLRenderingContext | null;
  head_meshes: HeadMeshes | null;
  overlay_canvas: HTMLCanvasElement; // on top, takes the gestures
  plate: PlateImage;
  plate_image: HTMLImageElement | null; // null = not loaded
  is_front_view: boolean;
  transform: PlateViewTransform;
};

type PageState = {
  views: PlateView[];
  // The sketchpad's document (the template); null until it loaded. Read only here.
  template_document: TabletDocument | null;
  // The template fitted to the plates (refit): the template itself while there are no
  // landmark pairs or the fit failed; null while the template is null.
  fitted_document: TabletDocument | null;
  fit_revision: number; // counts refits, the head meshes rebuild when it changes
  editor: FitLandmarkEditor<LoomisPlanesManLandmarksFile>; // holds the landmark file (`editor.file`)
  // Marks by vertex name waiting for the document (the starting marks, or a version 1
  // file's): added to the landmarks when it loads, then null.
  named_marks_to_add: readonly NamedMark[] | null;
  // The girl plate's landmarks: added by name, placed by the registration, once the
  // document is loaded and both views are registered; then null.
  template_point_marks_to_add: readonly TemplatePointMark[] | null;
  mode: Mode;
  keyboard_owner: KeyboardOwner;
  plate_opacity: number;
  surfaces_visible: boolean;
  skull_visible: boolean;
};

const status_element = document.getElementById("status")!;
function show_status(text: string, failed: boolean = false): void {
  status_element.textContent = text;
  status_element.classList.toggle("failed", failed);
}

// ---- view geometry --------------------------------------------------------------------

function whole_plate_region(plate: PlateImage): PlateRegion {
  return { left: 0, top: 0, width: plate.width_pixels, height: plate.height_pixels };
}

function fit_view_to_plate(view: PlateView): void {
  view.transform = plate_view_transform_showing_region(view.container.clientWidth, view.container.clientHeight, whole_plate_region(view.plate));
}

function image_pixel_at_canvas_point(view: PlateView, canvas_point: V2): V2 {
  return {
    x: view.transform.image_pixel_at_canvas_origin.x + canvas_point.x / view.transform.image_pixels_to_canvas_scale,
    y: view.transform.image_pixel_at_canvas_origin.y + canvas_point.y / view.transform.image_pixels_to_canvas_scale,
  };
}

// The registration is of the TEMPLATE (the marks are template points).
function view_registration(state: PageState, view: PlateView): PlateRegistration | null {
  return state.template_document === null ? null : plate_registration(state.template_document, state.editor.file.landmarks, view.is_front_view);
}

// The landmarks marked in a view, with their marks (image pixels) and their template points.
type ViewMark = { index: number; landmark: PlateLandmark; pixel: V2; head_position: V3 | null };
function view_marks(state: PageState, view: PlateView): ViewMark[] {
  const marks: ViewMark[] = [];
  state.editor.file.landmarks.forEach((landmark, index) => {
    const pixel = landmark_mark(landmark, view.is_front_view);
    if (pixel === null) return;
    const head_position = state.template_document === null ? null : template_point_world_position(state.template_document, landmark.template_point);
    marks.push({ index, landmark, pixel, head_position });
  });
  return marks;
}

// The orthographic camera showing the world as the registered plate does: looking down
// -z in front, down -x in profile (+z to the viewer's left), pivot under the canvas centre.
function plate_view_camera(view: PlateView, registration: PlateRegistration): OrbitCamera {
  const centre = image_pixel_at_canvas_point(view, { x: view.container.clientWidth / 2, y: view.container.clientHeight / 2 });
  return {
    pivot: world_from_pixel(registration, centre),
    yaw: view.is_front_view ? 0 : Math.PI / 2,
    pitch: 0,
    distance: camera_distance_for_world_units_per_pixel(1 / (view.transform.image_pixels_to_canvas_scale * registration.pixels_per_world_unit), view.container.clientHeight),
  };
}

function size_canvas_to_container(view: PlateView, canvas: HTMLCanvasElement): { width: number; height: number; device_pixel_ratio: number } {
  const device_pixel_ratio = window.devicePixelRatio || 1;
  const width = Math.round(view.container.clientWidth * device_pixel_ratio);
  const height = Math.round(view.container.clientHeight * device_pixel_ratio);
  if (canvas.width !== width || canvas.height !== height) {
    canvas.width = width;
    canvas.height = height;
  }
  return { width, height, device_pixel_ratio };
}

// ---- the fit ----------------------------------------------------------------------------

// The template fitted to the current marks (plan Q19/Q22). No pairs (nothing marked
// yet): the template as it is. A failed fit (two landmarks on one point) is reported and
// shows the template too.
function refit(state: PageState): void {
  state.fit_revision += 1;
  if (state.template_document === null) {
    state.fitted_document = null;
    return;
  }
  const front = state.views.find((view) => view.is_front_view)!;
  const profile = state.views.find((view) => !view.is_front_view)!;
  const pairs = man_fit_pairs(state.template_document, state.editor.file.landmarks, view_registration(state, front), view_registration(state, profile));
  if (pairs.length === 0) {
    state.fitted_document = state.template_document;
    return;
  }
  const fitted = fitted_document_from_landmark_pairs(state.template_document, pairs, state.editor.file.stiffness);
  if (fitted === null) {
    show_status(`the fit failed (${pairs.length} pairs; two landmarks on one point?), showing the template`, true);
    state.fitted_document = state.template_document;
    return;
  }
  state.fitted_document = fitted;
  show_status(`fitted: ${state.editor.file.landmarks.length} landmarks, ${pairs.length} pairs, stiffness ${state.editor.file.stiffness}`);
}

// ---- drawing --------------------------------------------------------------------------

function draw_plate_layer(state: PageState, view: PlateView): void {
  const { width, height, device_pixel_ratio } = size_canvas_to_container(view, view.plate_canvas);
  const context = view.plate_canvas.getContext("2d")!;
  context.setTransform(1, 0, 0, 1, 0, 0);
  context.fillStyle = "#fff";
  context.fillRect(0, 0, width, height);
  if (view.plate_image === null) return;
  const scale = view.transform.image_pixels_to_canvas_scale * device_pixel_ratio;
  context.setTransform(scale, 0, 0, scale, -view.transform.image_pixel_at_canvas_origin.x * scale, -view.transform.image_pixel_at_canvas_origin.y * scale);
  context.globalAlpha = state.plate_opacity;
  context.drawImage(view.plate_image, 0, 0, view.plate.width_pixels, view.plate.height_pixels);
}

// The FITTED head through the plate's registration.
function draw_head_layer(state: PageState, view: PlateView): void {
  const gl = view.mesh_gl;
  if (gl === null || view.head_meshes === null) return;
  const { width, height } = size_canvas_to_container(view, view.mesh_canvas);
  gl.viewport(0, 0, width, height);
  gl.clearColor(0, 0, 0, 0);
  gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
  const registration = view_registration(state, view);
  if (state.fitted_document === null || registration === null || height === 0) return;
  const camera = plate_view_camera(view, registration);
  update_head_meshes(view.head_meshes, state.fitted_document, `fit ${state.fit_revision}`, camera, {
    skull_visible: state.skull_visible,
    surfaces_visible: state.surfaces_visible,
    strokes_visible: true,
    mirrored: true, // the document holds one side of the head
    surface_opacity: SURFACE_OPACITY,
    skin_stroke_color: SKIN_STROKE_COLOR,
    skull_stroke_color: SKULL_STROKE_COLOR,
    markers: head_landmark_markers(state, view),
  });
  draw_head_meshes(view.head_meshes, camera_orthographic_view_projection(camera, width / height), camera);
}

// The fitted head's landmarks of a view, as markers on the 3D head: with stiffness 0
// each sits on its mark. World units are 100 mm (WORLD_PER_MM), so this is a 4 mm marker.
const HEAD_MARKER_RADIUS = 0.04;
const HEAD_MARKER_COLOR: Rgb = { r: 1, g: 1, b: 1 };
function head_landmark_markers(state: PageState, view: PlateView): { position: V3; color: Rgb; radius: number }[] {
  const markers: { position: V3; color: Rgb; radius: number }[] = [];
  if (state.fitted_document === null) return markers;
  for (const mark of view_marks(state, view)) {
    const position = template_point_world_position(state.fitted_document, mark.landmark.template_point);
    if (position !== null) markers.push({ position, color: HEAD_MARKER_COLOR, radius: HEAD_MARKER_RADIUS });
  }
  return markers;
}

function draw_overlay_layer(state: PageState, view: PlateView): void {
  const { width, height, device_pixel_ratio } = size_canvas_to_container(view, view.overlay_canvas);
  const context = view.overlay_canvas.getContext("2d")!;
  context.setTransform(1, 0, 0, 1, 0, 0);
  context.clearRect(0, 0, width, height);
  // From here on, coordinates are image pixels.
  const canvas_scale = view.transform.image_pixels_to_canvas_scale;
  const scale = canvas_scale * device_pixel_ratio;
  context.setTransform(scale, 0, 0, scale, -view.transform.image_pixel_at_canvas_origin.x * scale, -view.transform.image_pixel_at_canvas_origin.y * scale);
  const marks = view_marks(state, view);
  const registration = view_registration(state, view);

  // The skin's on-surface strokes of the fitted head (the hairline), highlighted over the
  // plate: through the registration, both sides in front (the document holds one side).
  if (registration !== null && state.fitted_document !== null) {
    context.strokeStyle = ON_SURFACE_STROKE_HIGHLIGHT_COLOR;
    context.lineWidth = 2.5 / canvas_scale;
    context.lineJoin = "round";
    for (const stroke of state.fitted_document.strokes) {
      if (stroke.layer !== "skin" || stroke.on_surface !== true) continue;
      const samples = on_surface_stroke_samples(stroke, state.fitted_document);
      const sides = view.is_front_view ? [1, -1] : [1];
      for (const side of sides) {
        context.beginPath();
        samples.forEach((sample, k) => {
          const pixel = pixel_from_world(registration, { x: side * sample.x, y: sample.y, z: sample.z });
          if (k === 0) context.moveTo(pixel.x, pixel.y);
          else context.lineTo(pixel.x, pixel.y);
        });
        context.stroke();
      }
    }
  }

  // Marks: crosses with their names, the selected one in its own colour.
  const arm = 6 / canvas_scale;
  context.lineWidth = 1.4 / canvas_scale;
  context.font = `${11 / canvas_scale}px system-ui, sans-serif`;
  for (const mark of marks) {
    const color = mark.index === state.editor.selected_landmark_index ? SELECTED_MARK_COLOR : MARK_COLOR;
    context.strokeStyle = color;
    context.fillStyle = color;
    context.beginPath();
    context.moveTo(mark.pixel.x - arm, mark.pixel.y);
    context.lineTo(mark.pixel.x + arm, mark.pixel.y);
    context.moveTo(mark.pixel.x, mark.pixel.y - arm);
    context.lineTo(mark.pixel.x, mark.pixel.y + arm);
    context.stroke();
    context.fillText(mark.landmark.name, mark.pixel.x + arm, mark.pixel.y - arm);
  }

  // The template's own points of the marks, where the registration puts them (the gap to
  // the green mark is what the fit closes), and in front the head's midline.
  if (registration !== null) {
    context.strokeStyle = HEAD_POINT_COLOR;
    context.lineWidth = 0.8 / canvas_scale;
    if (view.is_front_view) {
      context.beginPath();
      context.moveTo(registration.pixel_of_world_origin.x, 0);
      context.lineTo(registration.pixel_of_world_origin.x, view.plate.height_pixels);
      context.stroke();
    }
    context.fillStyle = HEAD_POINT_COLOR;
    context.strokeStyle = "#000";
    for (const mark of marks) {
      if (mark.head_position === null) continue;
      const pixel = pixel_from_world(registration, mark.head_position);
      context.beginPath();
      context.arc(pixel.x, pixel.y, 3 / canvas_scale, 0, 2 * Math.PI);
      context.fill();
      context.stroke();
    }
  }
}

function draw_view(state: PageState, view: PlateView): void {
  draw_plate_layer(state, view);
  draw_head_layer(state, view);
  draw_overlay_layer(state, view);
}

function draw_all_views(state: PageState): void {
  for (const view of state.views) draw_view(state, view);
}

// ---- landmark file ----------------------------------------------------------------------

let save_in_flight: Promise<void> | null = null;
let save_again = false;

// Save the landmark file, one request at a time; a change during a save triggers another.
function save_landmarks(state: PageState): void {
  if (save_in_flight !== null) {
    save_again = true;
    return;
  }
  save_in_flight = (async () => {
    const saved = await save_landmarks_file(state.editor.file);
    if (!saved) show_status("saving the landmark file failed", true);
    save_in_flight = null;
    if (save_again) {
      save_again = false;
      save_landmarks(state);
    }
  })();
}

// A landmark for a template point: marked in each view where the registration puts that
// point (no registration in a view: not marked there).
function new_plate_landmark(state: PageState, template_point: TemplatePoint, name: string): PlateLandmark | null {
  const position = state.template_document === null ? null : template_point_world_position(state.template_document, template_point);
  if (position === null) return null;
  const mark_in = (view: PlateView): V2 | null => {
    const registration = view_registration(state, view);
    return registration === null ? null : pixel_from_world(registration, position);
  };
  const front = state.views.find((view) => view.is_front_view)!;
  const profile = state.views.find((view) => !view.is_front_view)!;
  return { name, template_point, front: mark_in(front), profile: mark_in(profile) };
}

// "reset marks": the starting marks and the girl's landmarks again, placed afresh.
function file_with_starting_landmarks(state: PageState): LoomisPlanesManLandmarksFile {
  const file: LoomisPlanesManLandmarksFile = { ...starting_landmarks_file(), stiffness: state.editor.file.stiffness };
  if (state.template_document === null) return file;
  file.landmarks = landmarks_from_named_marks(state.template_document, STARTING_MARKS) ?? [];
  if (state.template_point_marks_to_add === null) {
    // The girl's landmarks were added already: take them from the current file, placed
    // by the starting marks' registration.
    const before = state.editor.file.landmarks;
    state.editor.file = file;
    add_template_point_marks(state, template_point_marks_missing_from(file.landmarks, before));
  }
  return state.editor.file;
}

// Append landmarks for the template points not yet among the landmarks, placed by the
// current registration. True when any was added.
function add_template_point_marks(state: PageState, marks: readonly TemplatePointMark[]): boolean {
  let added = false;
  for (const mark of template_point_marks_missing_from(state.editor.file.landmarks, marks)) {
    const landmark = new_plate_landmark(state, mark.template_point, mark.name);
    if (landmark === null || (landmark.front === null && landmark.profile === null)) continue;
    state.editor.file.landmarks.push(landmark);
    added = true;
  }
  return added;
}

// The marks waiting for the document (named marks, then the girl's landmarks, which need
// the registration the named marks give). Saves when any were added.
function add_waiting_marks(state: PageState): void {
  if (state.template_document === null) return;
  let added = false;
  if (state.named_marks_to_add !== null) {
    const landmarks = landmarks_from_named_marks(state.template_document, state.named_marks_to_add);
    if (landmarks !== null) {
      state.editor.file.landmarks.push(...landmarks);
      state.named_marks_to_add = null;
      added = true;
    }
  }
  if (state.template_point_marks_to_add !== null && state.views.every((view) => view_registration(state, view) !== null)) {
    added = add_template_point_marks(state, state.template_point_marks_to_add) || added;
    state.template_point_marks_to_add = null;
  }
  if (added) save_landmarks(state);
}

// ---- gestures ---------------------------------------------------------------------------

// The landmark (index into the file's list) whose mark in this view is within reach of
// `canvas_point`, nearest first; null when none is.
function mark_near(state: PageState, view: PlateView, canvas_point: V2): number | null {
  const image_point = image_pixel_at_canvas_point(view, canvas_point);
  const reach = MARK_GRAB_RADIUS_CSS_PIXELS / view.transform.image_pixels_to_canvas_scale;
  let best: number | null = null;
  let best_distance = reach;
  for (const mark of view_marks(state, view)) {
    const distance = Math.hypot(image_point.x - mark.pixel.x, image_point.y - mark.pixel.y);
    if (distance < best_distance) {
      best_distance = distance;
      best = mark.index;
    }
  }
  return best;
}

function move_mark(state: PageState, view: PlateView, landmark_index: number, image_delta: V2): void {
  const mark = landmark_mark(state.editor.file.landmarks[landmark_index], view.is_front_view);
  if (mark === null) return;
  mark.x += image_delta.x;
  mark.y += image_delta.y;
}

function attach_view_gestures(state: PageState, view: PlateView): void {
  const canvas = view.overlay_canvas;
  const pointer_positions = new Map<number, V2>(); // pointerId -> last position, canvas CSS pixels
  let last_tap_milliseconds = 0;
  let moved_since_pointer_down = false;
  let dragged_mark: number | null = null; // index into the landmarks list
  let mark_drag_is_remembered = false; // the undo entry of this mark drag is taken (at its first move)

  const canvas_point = (e: PointerEvent | WheelEvent): V2 => {
    const rectangle = canvas.getBoundingClientRect();
    return { x: e.clientX - rectangle.left, y: e.clientY - rectangle.top };
  };
  const centroid_and_spread = (): { centroid: V2; spread: number } => {
    const points = [...pointer_positions.values()];
    const centroid = { x: points.reduce((sum, p) => sum + p.x, 0) / points.length, y: points.reduce((sum, p) => sum + p.y, 0) / points.length };
    const spread = points.reduce((sum, p) => sum + Math.hypot(p.x - centroid.x, p.y - centroid.y), 0) / points.length;
    return { centroid, spread };
  };

  // Chrome's middle-click autoscroll starts on mousedown, not pointerdown.
  canvas.addEventListener("mousedown", (e) => { if (e.button === 1) e.preventDefault(); });
  canvas.addEventListener("pointerdown", (e) => {
    e.preventDefault();
    try { canvas.setPointerCapture(e.pointerId); } catch {} // throws for synthetic events
    const point = canvas_point(e);
    pointer_positions.set(e.pointerId, point);
    moved_since_pointer_down = false;
    // The middle mouse button pans in every mode (the fall-through below).
    const middle_button = e.pointerType === "mouse" && e.button === 1;
    if (pointer_positions.size === 1) {
      if (middle_button) return;
      dragged_mark = state.mode === "landmarks" ? mark_near(state, view, point) : null;
      if (dragged_mark !== null) state.editor.select_landmark(dragged_mark);
    } else {
      dragged_mark = null; // a second finger turns any drag into a pinch
    }
  });
  canvas.addEventListener("pointermove", (e) => {
    if (!pointer_positions.has(e.pointerId)) return;
    const before = centroid_and_spread();
    pointer_positions.set(e.pointerId, canvas_point(e));
    const after = centroid_and_spread();
    const delta = { x: after.centroid.x - before.centroid.x, y: after.centroid.y - before.centroid.y };
    if (Math.hypot(delta.x, delta.y) > 0) moved_since_pointer_down = true;
    const image_delta = { x: delta.x / view.transform.image_pixels_to_canvas_scale, y: delta.y / view.transform.image_pixels_to_canvas_scale };
    if (dragged_mark !== null) {
      if (!mark_drag_is_remembered) {
        state.editor.remember_for_undo();
        mark_drag_is_remembered = true;
      }
      move_mark(state, view, dragged_mark, image_delta);
      // The registration moves with the mark; the fit follows when the drag ends.
      draw_view(state, view);
      return;
    }
    view.transform.image_pixel_at_canvas_origin = {
      x: view.transform.image_pixel_at_canvas_origin.x - image_delta.x,
      y: view.transform.image_pixel_at_canvas_origin.y - image_delta.y,
    };
    if (pointer_positions.size >= 2 && before.spread > 0) view.transform = plate_view_transform_zoomed_at(view.transform, after.centroid, after.spread / before.spread);
    draw_view(state, view);
  });
  const end_pointer = (e: PointerEvent): void => {
    if (!pointer_positions.delete(e.pointerId)) return;
    if (pointer_positions.size > 0) return;
    // The edit is done: save, refit, draw.
    if (dragged_mark !== null && moved_since_pointer_down) state.editor.landmarks_edited();
    const was_plain_tap = e.type === "pointerup" && !moved_since_pointer_down && dragged_mark === null;
    dragged_mark = null;
    mark_drag_is_remembered = false;
    if (was_plain_tap) {
      if (e.timeStamp - last_tap_milliseconds < 350) {
        fit_view_to_plate(view);
        draw_view(state, view);
      }
      last_tap_milliseconds = e.timeStamp;
    }
  };
  canvas.addEventListener("pointerup", end_pointer);
  canvas.addEventListener("pointercancel", end_pointer);
  canvas.addEventListener("wheel", (e) => {
    e.preventDefault();
    view.transform = plate_view_transform_zoomed_at(view.transform, canvas_point(e), Math.exp(-e.deltaY * 0.0015));
    draw_view(state, view);
  }, { passive: false });
}

// ---- the sketchpad --------------------------------------------------------------------

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

// ---- wiring -----------------------------------------------------------------------------

async function start(): Promise<void> {
  const opacity_slider = document.getElementById("plate_opacity") as HTMLInputElement;
  const surfaces_checkbox = document.getElementById("plate_surfaces_visible") as HTMLInputElement;
  const skull_checkbox = document.getElementById("plate_skull_visible") as HTMLInputElement;
  const mode_radios = Array.from(document.querySelectorAll<HTMLInputElement>("input[name=mode]"));
  const editor_container = document.getElementById("editor")!;

  const [fetched, girl_file] = await Promise.all([fetch_landmarks_file(), fetch_fit_landmarks_file(LOOMIS_GIRL_FIT_LANDMARKS_NAME)]);
  if (fetched === null) show_status("loading the landmark file failed; changes will not be saved", true);
  if (girl_file === null) show_status("loading the girl's landmark file failed; her landmarks are not added", true);
  const landmarks_file = fetched?.file ?? starting_landmarks_file();

  // The editor's listeners run on user edits, once `state` below exists.
  const editor = create_fit_landmark_editor<LoomisPlanesManLandmarksFile>(landmarks_file, {
    starting_file: () => file_with_starting_landmarks(state),
    new_landmark: (template_point, name) => new_plate_landmark(state, template_point, name),
    on_edited: () => {
      save_landmarks(state);
      refit(state);
      draw_all_views(state);
    },
    on_view_changed: () => draw_all_views(state),
    owns_keyboard: () => state.keyboard_owner === "plates",
  });

  const state: PageState = {
    views: [],
    template_document: null,
    fitted_document: null,
    fit_revision: 0,
    editor,
    named_marks_to_add: fetched === null ? STARTING_MARKS : fetched.named_marks_to_add,
    template_point_marks_to_add: girl_file === null ? null : girl_file.landmarks.map((landmark) => ({ name: landmark.name, template_point: landmark.template_point })),
    mode: (mode_radios.find((radio) => radio.checked)?.value as Mode | undefined) ?? "move",
    keyboard_owner: "sketchpad",
    plate_opacity: Number(opacity_slider.value),
    surfaces_visible: surfaces_checkbox.checked,
    skull_visible: skull_checkbox.checked,
  };

  const make_view = (container_id: string, plate: PlateImage, is_front_view: boolean): PlateView => {
    const container = document.getElementById(container_id)!;
    const add_canvas = (): HTMLCanvasElement => container.appendChild(document.createElement("canvas"));
    const plate_canvas = add_canvas();
    const mesh_canvas = add_canvas();
    const overlay_canvas = add_canvas();
    const mesh_gl = mesh_canvas.getContext("webgl");
    if (mesh_gl === null) console.error(`${PAGE_NAME}: no WebGL, view '${container_id}' shows no surfaces or strokes`);
    const view: PlateView = {
      container, plate_canvas, mesh_canvas, mesh_gl, overlay_canvas, plate, is_front_view,
      head_meshes: mesh_gl === null ? null : create_head_meshes(mesh_gl),
      plate_image: null,
      transform: { image_pixels_to_canvas_scale: 1, image_pixel_at_canvas_origin: { x: 0, y: 0 } },
    };
    const image = new Image();
    image.onload = () => {
      view.plate_image = image;
      draw_view(state, view);
    };
    image.onerror = () => {
      console.error(`${PAGE_NAME}: ${plate.url} not found (copy it from Drive into tablet/public/)`);
      document.getElementById("plate_missing_note")!.hidden = false;
    };
    image.src = plate.url;
    return view;
  };
  state.views.push(make_view("front_view", FRONT_PLATE, true));
  state.views.push(make_view("profile_view", PROFILE_PLATE, false));
  for (const view of state.views) {
    fit_view_to_plate(view);
    attach_view_gestures(state, view);
  }

  // The keyboard goes to whichever was touched last: the sketchpad, or anything else
  // (the plate views, the bottom bar).
  window.addEventListener("pointerdown", (e) => {
    state.keyboard_owner = e.target instanceof Node && editor_container.contains(e.target) ? "sketchpad" : "plates";
  }, true);

  // "bigger" on a view gives it most of the row, until "smaller" or another view's "bigger".
  const enlarge_buttons = Array.from(document.querySelectorAll<HTMLButtonElement>(".enlarge_pane_button"));
  for (const button of enlarge_buttons) {
    button.addEventListener("click", () => {
      const figure = button.closest("figure")!;
      const is_enlarged = !figure.classList.contains("enlarged");
      for (const other of enlarge_buttons) {
        other.closest("figure")!.classList.remove("enlarged");
        other.textContent = "bigger";
      }
      figure.classList.toggle("enlarged", is_enlarged);
      if (is_enlarged) button.textContent = "smaller";
      for (const view of state.views) fit_view_to_plate(view);
      draw_all_views(state);
    });
  }

  for (const radio of mode_radios) {
    radio.addEventListener("change", () => {
      if (radio.checked) state.mode = radio.value as Mode;
    });
  }
  opacity_slider.addEventListener("input", () => {
    state.plate_opacity = Number(opacity_slider.value);
    draw_all_views(state);
  });
  surfaces_checkbox.addEventListener("change", () => {
    state.surfaces_visible = surfaces_checkbox.checked;
    draw_all_views(state);
  });
  skull_checkbox.addEventListener("change", () => {
    state.skull_visible = skull_checkbox.checked;
    draw_all_views(state);
  });
  window.addEventListener("resize", () => {
    for (const view of state.views) fit_view_to_plate(view);
    draw_all_views(state);
  });

  draw_all_views(state);

  // The sketchpad, as on skin-draw but with its own localStorage keys: it owns the
  // document; this page reads it after each change.
  start_sketchpad({
    load_reference_mesh: () => load_skin_reference_mesh(PAGE_NAME),
    load_eyeball_mesh: () => load_skull_frame_mesh(load_eyeball),
    default_document_name: "skull-zanatomy",
    storage_key_prefix: "autodraw_fit_hairline_to_loomis_man",
    can_switch_documents: false,
    active_layer: "skin",
    locked_layers: ["skull"],
    hidden_layers: [],
    on_document_edited: (tablet_document) => {
      state.template_document = tablet_document;
      add_waiting_marks(state);
      refit(state);
      draw_all_views(state);
    },
    owns_keyboard: () => state.keyboard_owner === "sketchpad",
  });
}

void start();
