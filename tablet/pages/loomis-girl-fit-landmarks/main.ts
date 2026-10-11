// Page `loomis-girl-fit-landmarks` (plan-loomis-girl-landmark-picking.md): pick the landmarks
// the template head (document `skull-zanatomy`) is fitted to Loomis's school girl plate with.
// Three panes side by side: the plate's front view, its profile view, and the head model.
// A landmark is one point of the template plus where that point sits on the plate (its
// "mark" in each view). Every edit is saved to the fit-landmarks file at once; "recompute fit"
// fits the template to the marks and writes document `loomis-school-girl`.
import "../../pages.css";
import { default_camera } from "../../src/camera";
import { TabletDocument, bezier_point, empty_document, stroke_control_points } from "../../src/document";
import { fitted_document_from_landmarks, largest_vertex_distance_between_documents } from "../../src/landmark_fit";
import {
  FRONT_REGION, FitLandmark, FitLandmarksFile, IMAGE_HEIGHT_PIXELS, IMAGE_WIDTH_PIXELS, LOOMIS_GIRL_FIT_LANDMARKS_NAME, PIXELS_PER_WORLD_UNIT,
  PROFILE_PORION_X_PIXELS, PROFILE_REGION, PlateRegion, PlateViewTransform, TemplatePoint, WORLD_Y_ZERO_ROW_PIXELS, fetch_fit_landmarks_file,
  front_pixel_from_world, plate_view_transform_showing_region, plate_view_transform_zoomed_at, profile_pixel_from_world, save_fit_landmarks_file,
  starting_fit_landmarks_file, template_point_world_position, template_position_is_on_midline,
} from "../../src/loomis_girl_plate";
import { V2, V3, v3 } from "../../src/math";
import { apply_document_state, serialize_document_state } from "../../src/persistence";
import { start_head_model_pane } from "./head_model_pane";

const PAGE_NAME = "loomis-girl-fit-landmarks";
const TEMPLATE_DOCUMENT_NAME = "skull-zanatomy";
const FITTED_DOCUMENT_NAME = "loomis-school-girl";
const GRAB_RADIUS_PIXELS = 14;
const SKIN_STROKE_STYLE = "#e8262a";
const SKULL_STROKE_STYLE = "#2a62e8";
const LANDMARK_STYLE = "#0a9a3a";
const SELECTED_LANDMARK_STYLE = "#d6007f";
const MISSING_TEMPLATE_POINT_STYLE = "#c0392b";
const MIDLINE_STYLE = "#7a4bd6";

// ---- state --------------------------------------------------------------------------

let landmarks_file: FitLandmarksFile = starting_fit_landmarks_file();
const undo_stack: FitLandmarksFile[] = []; // the file as it was before each edit, oldest first
let selected_landmark_index: number | null = null;
let template_document: TabletDocument = empty_document();
let template_document_is_loaded = false;
let fitted_document: TabletDocument | null = null; // document `loomis-school-girl` as on the server
let is_adding_landmark = false;
let marks_differ_from_last_fit = false;
let marks_can_be_fitted = true;
let failure_message = ""; // the last save or recompute that failed; cleared by the next one that works
let plate_image: HTMLImageElement | null = null;

const add_landmark_button = document.getElementById("add_landmark_button") as HTMLButtonElement;
const landmark_name_field = document.getElementById("landmark_name_field") as HTMLInputElement;
const remove_landmark_button = document.getElementById("remove_landmark_button") as HTMLButtonElement;
const undo_button = document.getElementById("undo_landmark_edit_button") as HTMLButtonElement;
const reset_button = document.getElementById("reset_landmarks_button") as HTMLButtonElement;
const names_checkbox = document.getElementById("landmark_names_visible") as HTMLInputElement;
const skull_checkbox = document.getElementById("skull_visible") as HTMLInputElement;
const plate_opacity_slider = document.getElementById("plate_opacity") as HTMLInputElement;
const stiffness_slider = document.getElementById("fit_stiffness") as HTMLInputElement;
const stiffness_value = document.getElementById("fit_stiffness_value")!;
const recompute_button = document.getElementById("recompute_fit_button") as HTMLButtonElement;
const fit_status = document.getElementById("fit_status")!;

// ---- landmarks ----------------------------------------------------------------------

// A landmark on the midline has one mark in the front view; any other has one on each side of
// the face. The template decides; a landmark whose template point is gone keeps what its mark says.
function landmark_is_on_midline(landmark: FitLandmark): boolean {
  const template_position = template_point_world_position(template_document, landmark.template_point);
  if (template_position === null) return landmark.front_half_width_pixels === 0;
  return template_position_is_on_midline(template_position);
}

// The stiffness slider runs 0..1; the useful stiffness values crowd near 0, so the slider is cubed.
function stiffness_from_slider(slider_value: number): number {
  return slider_value ** 3;
}

let save_queue: Promise<void> = Promise.resolve(); // one save at a time, in edit order

function save_landmarks_file(): void {
  const file_to_save = structuredClone(landmarks_file);
  save_queue = save_queue.then(async () => {
    const is_saved = await save_fit_landmarks_file(LOOMIS_GIRL_FIT_LANDMARKS_NAME, file_to_save);
    if (!is_saved) console.error(`${PAGE_NAME}: saving the landmarks failed`);
    failure_message = is_saved ? "" : "could not save the marks (is the server running?)";
    update_bottom_bar();
  });
}

// Whether a fit of the marks as they are now gives the document that is on the server.
function compare_marks_with_last_fit(): void {
  if (!template_document_is_loaded) return;
  const fitted_now = fitted_document_from_landmarks(template_document, landmarks_file);
  marks_can_be_fitted = fitted_now !== null;
  marks_differ_from_last_fit = fitted_now !== null && (fitted_document === null || largest_vertex_distance_between_documents(fitted_now, fitted_document) > 1e-6);
}

// Call before changing `landmarks_file`, then `landmarks_edited` after.
function remember_for_undo(): void {
  undo_stack.push(structuredClone(landmarks_file));
}

function landmarks_edited(): void {
  save_landmarks_file();
  compare_marks_with_last_fit();
  update_bottom_bar();
  draw_everything();
}

function select_landmark(landmark_index: number | null): void {
  selected_landmark_index = landmark_index;
  update_bottom_bar();
  draw_everything();
}

function undo_landmark_edit(): void {
  const before = undo_stack.pop();
  if (before === undefined) return;
  landmarks_file = before;
  if (selected_landmark_index !== null && selected_landmark_index >= landmarks_file.landmarks.length) selected_landmark_index = null;
  landmarks_edited();
}

function remove_selected_landmark(): void {
  if (selected_landmark_index === null) return;
  remember_for_undo();
  landmarks_file.landmarks.splice(selected_landmark_index, 1);
  selected_landmark_index = null;
  landmarks_edited();
}

// A new landmark starts with its marks where the template point itself is: no pull on the
// fit until the marks are dragged.
function add_landmark_at(template_point: TemplatePoint): void {
  const template_position = template_point_world_position(template_document, template_point);
  if (template_position === null) return;
  const names = new Set(landmarks_file.landmarks.map((landmark) => landmark.name));
  let number = landmarks_file.landmarks.length + 1;
  while (names.has(`landmark ${number}`)) number++;
  remember_for_undo();
  landmarks_file.landmarks.push({
    name: `landmark ${number}`,
    template_point,
    front_half_width_pixels: template_position_is_on_midline(template_position) ? 0 : Math.abs(template_position.x) * PIXELS_PER_WORLD_UNIT,
    row_pixels: WORLD_Y_ZERO_ROW_PIXELS - template_position.y * PIXELS_PER_WORLD_UNIT,
    profile_column_pixels: PROFILE_PORION_X_PIXELS - template_position.z * PIXELS_PER_WORLD_UNIT,
    read_from_front: false,
    read_from_profile: false,
  });
  selected_landmark_index = landmarks_file.landmarks.length - 1;
  is_adding_landmark = false;
  landmarks_edited();
}

// ---- bottom bar ---------------------------------------------------------------------

function update_bottom_bar(): void {
  const selected = selected_landmark_index === null ? null : landmarks_file.landmarks[selected_landmark_index];
  if (document.activeElement !== landmark_name_field) landmark_name_field.value = selected === null ? "" : selected.name;
  landmark_name_field.disabled = selected === null;
  remove_landmark_button.disabled = selected === null;
  undo_button.disabled = undo_stack.length === 0;
  add_landmark_button.classList.toggle("armed", is_adding_landmark);
  add_landmark_button.textContent = is_adding_landmark ? "tap a point on the head model…" : "add landmark";
  stiffness_slider.value = String(Math.cbrt(landmarks_file.stiffness));
  stiffness_value.textContent = landmarks_file.stiffness === 0 ? "0" : landmarks_file.stiffness.toPrecision(2);
  recompute_button.disabled = !template_document_is_loaded || !marks_can_be_fitted;
  fit_status.textContent =
    failure_message !== "" ? failure_message
    : !marks_can_be_fitted ? "these marks cannot be fitted (two landmarks on the same template point?)"
    : marks_differ_from_last_fit ? "marks changed since the last fit"
    : "";
}

// ---- plate panes --------------------------------------------------------------------

type PlateDrag =
  | { kind: "mark"; landmark_index: number; side_sign: number; pointer_to_mark_offset: V2; has_moved: boolean }
  | { kind: "midline"; has_moved: boolean }
  | { kind: "pan"; travel_pixels: number };

type PlatePane = {
  canvas: HTMLCanvasElement;
  is_front: boolean;
  region: PlateRegion;
  transform: PlateViewTransform; // in CSS pixels of the canvas
  is_showing_whole_region: boolean; // until the first pan or zoom: a resize fits the region again
  hovered_landmark_index: number | null;
  pointers: Map<number, V2>;
  drag: PlateDrag | null;
  last_empty_tap_time_ms: number;
};

type PlateMark = { landmark_index: number; side_sign: number; image_pixel: V2 };

function create_plate_pane(canvas_id: string, is_front: boolean): PlatePane {
  const region = is_front ? FRONT_REGION : PROFILE_REGION;
  return {
    canvas: document.getElementById(canvas_id) as HTMLCanvasElement,
    is_front,
    region,
    transform: plate_view_transform_showing_region(1, 1, region),
    is_showing_whole_region: true,
    hovered_landmark_index: null,
    pointers: new Map(),
    drag: null,
    last_empty_tap_time_ms: 0,
  };
}

const plate_panes = [create_plate_pane("plate_front_canvas", true), create_plate_pane("plate_profile_canvas", false)];

function plate_pixel_from_world(pane: PlatePane, p: V3): V2 {
  return pane.is_front ? front_pixel_from_world(p, landmarks_file.front_midline_column_pixels) : profile_pixel_from_world(p);
}

function canvas_point_from_image_pixel(pane: PlatePane, image_pixel: V2): V2 {
  const scale = pane.transform.image_pixels_to_canvas_scale;
  const origin = pane.transform.image_pixel_at_canvas_origin;
  return { x: (image_pixel.x - origin.x) * scale, y: (image_pixel.y - origin.y) * scale };
}

function image_pixel_from_canvas_point(pane: PlatePane, canvas_point: V2): V2 {
  const scale = pane.transform.image_pixels_to_canvas_scale;
  const origin = pane.transform.image_pixel_at_canvas_origin;
  return { x: canvas_point.x / scale + origin.x, y: canvas_point.y / scale + origin.y };
}

// The marks of one landmark in a pane: two in the front view when the landmark is paired.
function plate_marks_of_landmark(pane: PlatePane, landmark: FitLandmark, landmark_index: number): PlateMark[] {
  if (!pane.is_front) return [{ landmark_index, side_sign: 1, image_pixel: { x: landmark.profile_column_pixels, y: landmark.row_pixels } }];
  const midline = landmarks_file.front_midline_column_pixels;
  if (landmark_is_on_midline(landmark)) return [{ landmark_index, side_sign: 1, image_pixel: { x: midline, y: landmark.row_pixels } }];
  return [1, -1].map((side_sign) => ({ landmark_index, side_sign, image_pixel: { x: midline + side_sign * landmark.front_half_width_pixels, y: landmark.row_pixels } }));
}

function plate_mark_under_pointer(pane: PlatePane, canvas_point: V2): PlateMark | null {
  let nearest: PlateMark | null = null;
  let nearest_distance = GRAB_RADIUS_PIXELS;
  landmarks_file.landmarks.forEach((landmark, landmark_index) => {
    for (const mark of plate_marks_of_landmark(pane, landmark, landmark_index)) {
      const mark_point = canvas_point_from_image_pixel(pane, mark.image_pixel);
      let distance = Math.hypot(mark_point.x - canvas_point.x, mark_point.y - canvas_point.y);
      if (landmark_index === selected_landmark_index) distance *= 0.5; // where marks crowd, the selected one is the one grabbed
      if (distance <= nearest_distance) {
        nearest = mark;
        nearest_distance = distance;
      }
    }
  });
  return nearest;
}

// The midline is dragged by its handle at the top edge of the front pane.
const MIDLINE_HANDLE_CENTER_Y_PIXELS = 12;

function midline_handle_is_under_pointer(pane: PlatePane, canvas_point: V2): boolean {
  if (!pane.is_front) return false;
  const handle_x = canvas_point_from_image_pixel(pane, { x: landmarks_file.front_midline_column_pixels, y: 0 }).x;
  return Math.hypot(handle_x - canvas_point.x, MIDLINE_HANDLE_CENTER_Y_PIXELS - canvas_point.y) <= GRAB_RADIUS_PIXELS;
}

// Moves the dragged mark to the pointer. The view a mark is dragged in becomes a view the
// landmark is read from.
function plate_mark_drag(pane: PlatePane, drag: PlateDrag & { kind: "mark" }, canvas_point: V2): void {
  const landmark = landmarks_file.landmarks[drag.landmark_index];
  const image_pixel = image_pixel_from_canvas_point(pane, { x: canvas_point.x + drag.pointer_to_mark_offset.x, y: canvas_point.y + drag.pointer_to_mark_offset.y });
  landmark.row_pixels = image_pixel.y;
  if (pane.is_front) {
    if (!landmark_is_on_midline(landmark)) landmark.front_half_width_pixels = Math.max(0, (image_pixel.x - landmarks_file.front_midline_column_pixels) * drag.side_sign);
    landmark.read_from_front = true;
  } else {
    landmark.profile_column_pixels = image_pixel.x;
    landmark.read_from_profile = true;
  }
}

function fit_plate_pane_to_region(pane: PlatePane): void {
  pane.transform = plate_view_transform_showing_region(pane.canvas.clientWidth, pane.canvas.clientHeight, pane.region);
  pane.is_showing_whole_region = true;
}

function draw_plate_pane(pane: PlatePane): void {
  const canvas = pane.canvas;
  const device_pixel_ratio = window.devicePixelRatio || 1;
  const width = Math.round(canvas.clientWidth * device_pixel_ratio);
  const height = Math.round(canvas.clientHeight * device_pixel_ratio);
  if (width === 0 || height === 0) return;
  if (canvas.width !== width || canvas.height !== height) {
    canvas.width = width;
    canvas.height = height;
  }
  const context = canvas.getContext("2d")!;
  context.setTransform(1, 0, 0, 1, 0, 0);
  context.fillStyle = "#ffffff";
  context.fillRect(0, 0, width, height);

  // Image pixel coordinates from here.
  const scale = pane.transform.image_pixels_to_canvas_scale;
  const origin = pane.transform.image_pixel_at_canvas_origin;
  const set_image_pixel_coordinates = (): void => context.setTransform(scale * device_pixel_ratio, 0, 0, scale * device_pixel_ratio, -origin.x * scale * device_pixel_ratio, -origin.y * scale * device_pixel_ratio);
  set_image_pixel_coordinates();
  if (plate_image !== null) {
    context.globalAlpha = Number(plate_opacity_slider.value);
    context.drawImage(plate_image, 0, 0, IMAGE_WIDTH_PIXELS, IMAGE_HEIGHT_PIXELS);
    context.globalAlpha = 1;
  }

  if (pane.is_front) {
    const midline = landmarks_file.front_midline_column_pixels;
    context.strokeStyle = MIDLINE_STYLE;
    context.lineWidth = 1 / scale;
    context.setLineDash([6 / scale, 4 / scale]);
    context.beginPath();
    context.moveTo(midline, -IMAGE_HEIGHT_PIXELS);
    context.lineTo(midline, 2 * IMAGE_HEIGHT_PIXELS);
    context.stroke();
    context.setLineDash([]);
  }

  // The fitted head as on the server. The document holds one side; the front view shows both.
  if (fitted_document !== null) {
    const sides = pane.is_front ? [(p: V3) => p, (p: V3) => v3(-p.x, p.y, p.z)] : [(p: V3) => p];
    context.lineJoin = "round";
    for (const stroke of fitted_document.strokes) {
      if (stroke.layer === "skull" && !skull_checkbox.checked) continue;
      const control_points = stroke_control_points(stroke, fitted_document);
      context.strokeStyle = stroke.layer === "skin" ? SKIN_STROKE_STYLE : SKULL_STROKE_STYLE;
      context.lineWidth = (stroke.layer === "skin" ? 1.5 : 1) / scale;
      for (const side of sides) {
        context.beginPath();
        for (let i = 0; i <= 24; i++) {
          const pixel = plate_pixel_from_world(pane, side(bezier_point(control_points, i / 24)));
          if (i === 0) context.moveTo(pixel.x, pixel.y); else context.lineTo(pixel.x, pixel.y);
        }
        context.stroke();
      }
    }
  }

  const labels: { text: string; image_pixel: V2; is_selected: boolean; style: string }[] = [];
  landmarks_file.landmarks.forEach((landmark, landmark_index) => {
    const is_selected = landmark_index === selected_landmark_index;
    const template_position = template_point_world_position(template_document, landmark.template_point);
    const is_missing_template_point = template_document_is_loaded && template_position === null;
    const is_read_from_this_view = pane.is_front ? landmark.read_from_front : landmark.read_from_profile;
    const style = is_missing_template_point ? MISSING_TEMPLATE_POINT_STYLE : is_selected ? SELECTED_LANDMARK_STYLE : LANDMARK_STYLE;
    const fitted_position = fitted_document === null ? null : template_point_world_position(fitted_document, landmark.template_point);
    const marks = plate_marks_of_landmark(pane, landmark, landmark_index);
    for (const mark of marks) {
      const { x, y } = mark.image_pixel;
      context.strokeStyle = style;
      context.fillStyle = style;
      context.globalAlpha = is_read_from_this_view || is_selected ? 1 : 0.6;
      // From the mark to where the last fit put the landmark.
      if (fitted_position !== null) {
        const fitted_pixel = plate_pixel_from_world(pane, v3(mark.side_sign * Math.abs(fitted_position.x), fitted_position.y, fitted_position.z));
        context.lineWidth = 1 / scale;
        context.beginPath();
        context.moveTo(x, y);
        context.lineTo(fitted_pixel.x, fitted_pixel.y);
        context.stroke();
        context.beginPath();
        context.arc(fitted_pixel.x, fitted_pixel.y, 1.8 / scale, 0, 2 * Math.PI);
        context.fill();
      }
      const arm = (is_selected ? 9 : 6) / scale;
      context.lineWidth = (is_selected ? 2.5 : 1.6) / scale;
      context.beginPath();
      if (is_read_from_this_view) {
        context.moveTo(x - arm, y);
        context.lineTo(x + arm, y);
        context.moveTo(x, y - arm);
        context.lineTo(x, y + arm);
      } else {
        context.arc(x, y, arm * 0.7, 0, 2 * Math.PI);
      }
      context.stroke();
      context.globalAlpha = 1;
    }
    if (is_selected || names_checkbox.checked || landmark_index === pane.hovered_landmark_index) {
      labels.push({ text: is_missing_template_point ? `${landmark.name} (template point missing)` : landmark.name, image_pixel: marks[0].image_pixel, is_selected, style });
    }
  });

  // CSS pixel coordinates from here: text and the midline handle keep their size when zoomed.
  context.setTransform(device_pixel_ratio, 0, 0, device_pixel_ratio, 0, 0);
  for (const label of labels) {
    const point = canvas_point_from_image_pixel(pane, label.image_pixel);
    context.font = label.is_selected ? "bold 15px system-ui, sans-serif" : "12px system-ui, sans-serif";
    context.lineWidth = 3;
    context.strokeStyle = "#ffffff";
    context.strokeText(label.text, point.x + 9, point.y - 7);
    context.fillStyle = label.style;
    context.fillText(label.text, point.x + 9, point.y - 7);
  }
  if (pane.is_front) {
    const handle_x = canvas_point_from_image_pixel(pane, { x: landmarks_file.front_midline_column_pixels, y: 0 }).x;
    context.fillStyle = MIDLINE_STYLE;
    context.beginPath();
    context.moveTo(handle_x - 9, MIDLINE_HANDLE_CENTER_Y_PIXELS - 9);
    context.lineTo(handle_x + 9, MIDLINE_HANDLE_CENTER_Y_PIXELS - 9);
    context.lineTo(handle_x, MIDLINE_HANDLE_CENTER_Y_PIXELS + 9);
    context.closePath();
    context.fill();
    context.font = "12px system-ui, sans-serif";
    context.fillText("midline", handle_x + 13, MIDLINE_HANDLE_CENTER_Y_PIXELS + 4);
  }
}

function attach_plate_pane_gestures(pane: PlatePane): void {
  const canvas = pane.canvas;
  const canvas_point_of = (e: PointerEvent | WheelEvent): V2 => {
    const rectangle = canvas.getBoundingClientRect();
    return { x: e.clientX - rectangle.left, y: e.clientY - rectangle.top };
  };
  const centroid_and_spread = (): { centroid: V2; spread: number } => {
    const points = [...pane.pointers.values()];
    const centroid = { x: 0, y: 0 };
    for (const point of points) {
      centroid.x += point.x / points.length;
      centroid.y += point.y / points.length;
    }
    let spread = 0;
    for (const point of points) spread += Math.hypot(point.x - centroid.x, point.y - centroid.y) / points.length;
    return { centroid, spread };
  };

  // An edit drag (mark or midline) ends: what it changed is saved.
  const end_drag = (): void => {
    const drag = pane.drag;
    pane.drag = null;
    if (drag === null) return;
    if (drag.kind === "pan") {
      if (drag.travel_pixels >= 4) return;
      // A tap on nothing: deselect; two in a row show the whole view again.
      const now_ms = performance.now();
      if (now_ms - pane.last_empty_tap_time_ms < 350) fit_plate_pane_to_region(pane);
      pane.last_empty_tap_time_ms = now_ms;
      select_landmark(null);
      return;
    }
    if (drag.has_moved) landmarks_edited();
  };

  canvas.addEventListener("pointerdown", (e) => {
    if (e.button !== 0 && e.pointerType === "mouse") return;
    try { canvas.setPointerCapture(e.pointerId); } catch { /* the pointer is already gone */ }
    const canvas_point = canvas_point_of(e);
    pane.pointers.set(e.pointerId, canvas_point);
    if (pane.pointers.size > 1) {
      // A second finger: the gesture is a pinch, whatever the first finger had grabbed.
      end_drag();
      pane.drag = { kind: "pan", travel_pixels: Infinity };
      return;
    }
    const mark = plate_mark_under_pointer(pane, canvas_point);
    if (mark !== null) {
      const mark_point = canvas_point_from_image_pixel(pane, mark.image_pixel);
      pane.drag = { kind: "mark", landmark_index: mark.landmark_index, side_sign: mark.side_sign, pointer_to_mark_offset: { x: mark_point.x - canvas_point.x, y: mark_point.y - canvas_point.y }, has_moved: false };
      select_landmark(mark.landmark_index);
    } else if (midline_handle_is_under_pointer(pane, canvas_point)) {
      pane.drag = { kind: "midline", has_moved: false };
    } else {
      pane.drag = { kind: "pan", travel_pixels: 0 };
    }
  });

  canvas.addEventListener("pointermove", (e) => {
    const canvas_point = canvas_point_of(e);
    const previous_point = pane.pointers.get(e.pointerId);
    if (previous_point === undefined || pane.drag === null) {
      const mark = plate_mark_under_pointer(pane, canvas_point);
      const hovered = mark === null ? null : mark.landmark_index;
      canvas.style.cursor = mark !== null ? "move" : midline_handle_is_under_pointer(pane, canvas_point) ? "ew-resize" : "grab";
      if (hovered !== pane.hovered_landmark_index) {
        pane.hovered_landmark_index = hovered;
        draw_plate_pane(pane);
      }
      return;
    }
    const drag = pane.drag;
    if (drag.kind === "pan") {
      const before = centroid_and_spread();
      pane.pointers.set(e.pointerId, canvas_point);
      const after = centroid_and_spread();
      drag.travel_pixels += Math.hypot(after.centroid.x - before.centroid.x, after.centroid.y - before.centroid.y);
      const scale = pane.transform.image_pixels_to_canvas_scale;
      const origin = pane.transform.image_pixel_at_canvas_origin;
      pane.transform = { image_pixels_to_canvas_scale: scale, image_pixel_at_canvas_origin: { x: origin.x - (after.centroid.x - before.centroid.x) / scale, y: origin.y - (after.centroid.y - before.centroid.y) / scale } };
      if (pane.pointers.size > 1 && before.spread > 0) pane.transform = plate_view_transform_zoomed_at(pane.transform, after.centroid, after.spread / before.spread);
      if (drag.travel_pixels >= 4) pane.is_showing_whole_region = false;
      draw_plate_pane(pane);
      return;
    }
    pane.pointers.set(e.pointerId, canvas_point);
    if (!drag.has_moved) {
      remember_for_undo();
      drag.has_moved = true;
    }
    if (drag.kind === "mark") plate_mark_drag(pane, drag, canvas_point);
    else landmarks_file.front_midline_column_pixels = image_pixel_from_canvas_point(pane, canvas_point).x;
    draw_plate_panes();
  });

  const pointer_gone = (e: PointerEvent): void => {
    if (!pane.pointers.delete(e.pointerId)) return;
    if (pane.pointers.size === 0) end_drag();
  };
  canvas.addEventListener("pointerup", pointer_gone);
  canvas.addEventListener("pointercancel", pointer_gone);
  canvas.addEventListener("pointerleave", () => {
    if (pane.hovered_landmark_index === null) return;
    pane.hovered_landmark_index = null;
    draw_plate_pane(pane);
  });

  canvas.addEventListener("wheel", (e) => {
    e.preventDefault();
    pane.transform = plate_view_transform_zoomed_at(pane.transform, canvas_point_of(e), Math.exp(-e.deltaY * 0.0015));
    pane.is_showing_whole_region = false;
    draw_plate_pane(pane);
  }, { passive: false });

  new ResizeObserver(() => {
    if (pane.is_showing_whole_region) fit_plate_pane_to_region(pane);
    draw_plate_pane(pane);
  }).observe(canvas);
}

function draw_plate_panes(): void {
  for (const pane of plate_panes) draw_plate_pane(pane);
}

// ---- head model pane ----------------------------------------------------------------

const draw_head_model_pane = start_head_model_pane(
  () => ({
    template_document,
    landmarks: landmarks_file.landmarks,
    selected_landmark_index,
    landmark_names_visible: names_checkbox.checked,
    skull_visible: skull_checkbox.checked,
    is_picking_template_point: is_adding_landmark,
  }),
  { on_template_point_picked: add_landmark_at, on_landmark_tapped: select_landmark },
);

function draw_everything(): void {
  draw_plate_panes();
  draw_head_model_pane();
}

// ---- recompute ----------------------------------------------------------------------

// Fits the template to the marks and writes the result as document `loomis-school-girl`,
// over whatever that document held. The document keeps its camera.
async function recompute_fit(): Promise<void> {
  const fitted_now = fitted_document_from_landmarks(template_document, landmarks_file);
  if (fitted_now === null) return;
  recompute_button.disabled = true;
  const url = `/api/documents/${FITTED_DOCUMENT_NAME}`;
  const camera = default_camera();
  const headers: Record<string, string> = { "Content-Type": "application/json" };
  let is_written = false;
  try {
    const existing = await fetch(url);
    if (existing.ok) {
      const revision = existing.headers.get("X-Revision");
      if (revision !== null) headers["X-Base-Revision"] = revision;
      apply_document_state(await existing.text(), empty_document(), camera);
    }
    if (existing.ok || existing.status === 404) {
      const written = await fetch(url, { method: "POST", headers, body: serialize_document_state(fitted_now, camera) });
      is_written = written.ok;
      if (!written.ok) console.error(`${PAGE_NAME}: writing ${FITTED_DOCUMENT_NAME} failed: ${written.status} ${await written.text()}`);
    } else {
      console.error(`${PAGE_NAME}: reading ${FITTED_DOCUMENT_NAME} failed: ${existing.status}`);
    }
  } catch (error) {
    console.error(`${PAGE_NAME}: recompute failed`, error);
  }
  failure_message = is_written ? "" : `could not write document ${FITTED_DOCUMENT_NAME}`;
  if (is_written) fitted_document = fitted_now;
  compare_marks_with_last_fit();
  update_bottom_bar();
  draw_everything();
}

// ---- wiring -------------------------------------------------------------------------

for (const pane of plate_panes) attach_plate_pane_gestures(pane);

const enlarge_pane_buttons = document.querySelectorAll<HTMLButtonElement>(".enlarge_pane_button");
enlarge_pane_buttons.forEach((button) => {
  button.addEventListener("click", () => {
    const pane_of_button = button.closest(".pane")!;
    const is_enlarged = !pane_of_button.classList.contains("enlarged");
    document.querySelectorAll(".pane").forEach((pane_element) => pane_element.classList.remove("enlarged"));
    enlarge_pane_buttons.forEach((other) => { other.textContent = "bigger"; });
    pane_of_button.classList.toggle("enlarged", is_enlarged);
    if (is_enlarged) button.textContent = "smaller";
  });
});

add_landmark_button.addEventListener("click", () => {
  is_adding_landmark = !is_adding_landmark;
  update_bottom_bar();
  draw_head_model_pane();
});
remove_landmark_button.addEventListener("click", remove_selected_landmark);
undo_button.addEventListener("click", undo_landmark_edit);
reset_button.addEventListener("click", () => {
  remember_for_undo();
  landmarks_file = starting_fit_landmarks_file();
  selected_landmark_index = null;
  landmarks_edited();
});
recompute_button.addEventListener("click", () => void recompute_fit());

landmark_name_field.addEventListener("change", () => {
  const name = landmark_name_field.value.trim();
  if (selected_landmark_index === null || name === "" || name === landmarks_file.landmarks[selected_landmark_index].name) {
    update_bottom_bar();
    return;
  }
  remember_for_undo();
  landmarks_file.landmarks[selected_landmark_index].name = name;
  landmarks_edited();
});
landmark_name_field.addEventListener("keydown", (e) => {
  if (e.key === "Enter" || e.key === "Escape") landmark_name_field.blur();
});

// Dragging the stiffness slider is one edit: remembered for undo when the drag starts, saved when it ends.
let stiffness_drag_is_remembered = false;
stiffness_slider.addEventListener("input", () => {
  if (!stiffness_drag_is_remembered) remember_for_undo();
  stiffness_drag_is_remembered = true;
  landmarks_file.stiffness = stiffness_from_slider(Number(stiffness_slider.value));
  stiffness_value.textContent = landmarks_file.stiffness === 0 ? "0" : landmarks_file.stiffness.toPrecision(2);
});
stiffness_slider.addEventListener("change", () => {
  stiffness_drag_is_remembered = false;
  landmarks_edited();
});

names_checkbox.addEventListener("change", draw_everything);
skull_checkbox.addEventListener("change", draw_everything);
plate_opacity_slider.addEventListener("input", draw_plate_panes);

window.addEventListener("keydown", (e) => {
  if (document.activeElement === landmark_name_field) return; // the keys belong to the name being typed
  if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === "z") {
    e.preventDefault();
    undo_landmark_edit();
  } else if (e.key === "Delete") {
    e.preventDefault();
    remove_selected_landmark();
  } else if (e.key === "Escape" && is_adding_landmark) {
    is_adding_landmark = false;
    update_bottom_bar();
    draw_head_model_pane();
  }
});

async function fetch_document(document_name: string): Promise<TabletDocument | null> {
  try {
    const response = await fetch(`/api/documents/${document_name}`);
    if (!response.ok) {
      console.error(`${PAGE_NAME}: document ${document_name}: ${response.status}`);
      return null;
    }
    const tablet_document = empty_document();
    if (!apply_document_state(await response.text(), tablet_document, default_camera())) {
      console.error(`${PAGE_NAME}: document ${document_name} unreadable`);
      return null;
    }
    return tablet_document;
  } catch (error) {
    console.error(`${PAGE_NAME}: document ${document_name} fetch failed`, error);
    return null;
  }
}

async function load_everything(): Promise<void> {
  const image = new Image();
  image.onload = () => {
    plate_image = image;
    draw_plate_panes();
  };
  image.onerror = () => {
    console.error(`${PAGE_NAME}: plate image missing`);
    document.getElementById("plate_missing_note")!.hidden = false;
  };
  image.src = "/loomis-girl-front-and-profile.webp";

  const [fetched_landmarks_file, fetched_template, fetched_fitted] = await Promise.all([
    fetch_fit_landmarks_file(LOOMIS_GIRL_FIT_LANDMARKS_NAME), fetch_document(TEMPLATE_DOCUMENT_NAME), fetch_document(FITTED_DOCUMENT_NAME),
  ]);
  if (fetched_landmarks_file === null) failure_message = "could not read the saved marks: showing the starting ones";
  else landmarks_file = fetched_landmarks_file;
  if (fetched_template === null) failure_message = `could not read document ${TEMPLATE_DOCUMENT_NAME}`;
  else {
    template_document = fetched_template;
    template_document_is_loaded = true;
  }
  fitted_document = fetched_fitted;
  compare_marks_with_last_fit();
  update_bottom_bar();
  draw_everything();
  console.log(`${PAGE_NAME}: ${landmarks_file.landmarks.length} landmarks, template ${template_document.vertices.length} vertices, fitted document ${fitted_document === null ? "missing" : "loaded"}`);
}

update_bottom_bar();
void load_everything();
