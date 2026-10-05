// The two plate views of the fit-head-to-loomis-girl page: the Loomis school girl plate
// with the fitted document drawn over it, front view in one canvas, profile in the other.
// Both are orthographic, so a world point maps to an image pixel by a scale and an offset
// (src/loomis_girl_target_views.ts).
//
// The views keep a document of their own, read from the server: the sketchpad next to
// them autosaves its edits, and the views pick the new file up within POLL_MILLISECONDS.
//
// Gestures, per view: one pointer drags the picture, two fingers pinch-zoom, the wheel
// zooms at the cursor, a double tap fits the head again.
import { default_camera } from "../../src/camera";
import { TabletDocument, bezier_point, empty_document, stroke_control_points, vertex_by_id, vertex_world_position } from "../../src/document";
import { V2, V3 } from "../../src/math";
import { apply_document_state, list_documents_from_server } from "../../src/persistence";
import {
  IMAGE_HEIGHT_PIXELS, IMAGE_WIDTH_PIXELS, TARGET_LANDMARKS, TARGET_MOST_FORWARD_POINT_LANDMARKS, front_pixel_from_world, profile_pixel_from_world,
  target_landmark_world_position, target_most_forward_point_world_position,
} from "../../src/loomis_girl_target_views";

const PLATE_IMAGE_URL = "/loomis-girl-front-and-profile.webp";
const POLL_MILLISECONDS = 2000;
const SKIN_STROKE_COLOR = "#e8262a";
const SKULL_STROKE_COLOR = "#2a62e8";
const LANDMARK_COLOR = "#0a9a3a";

// The part of the plate a view shows when fitted, in image pixels.
type PlateRegion = { left: number; top: number; width: number; height: number };

// One plate view. `image_pixels_to_canvas_scale` and `image_pixel_at_canvas_origin` say
// where the plate sits in the canvas (canvas CSS pixel = (image pixel - origin) * scale).
type PlateView = {
  canvas: HTMLCanvasElement;
  region: PlateRegion;
  image_pixel_from_world: (p: V3) => V2;
  draws_mirrored_side: boolean; // front view: the document holds one side of the face
  image_pixels_to_canvas_scale: number;
  image_pixel_at_canvas_origin: V2;
};

type PlateViewsState = {
  views: PlateView[];
  tablet_document: TabletDocument;
  document_revision: string | null; // null = nothing loaded yet
  plate_image: HTMLImageElement | null; // null = not loaded (the scan is not in the repo)
  plate_opacity: number;
  skull_visible: boolean;
  landmarks_visible: boolean;
};

function fit_view_to_region(view: PlateView): void {
  const scale = Math.min(view.canvas.clientWidth / view.region.width, view.canvas.clientHeight / view.region.height);
  view.image_pixels_to_canvas_scale = scale;
  view.image_pixel_at_canvas_origin = {
    x: view.region.left - (view.canvas.clientWidth / scale - view.region.width) / 2,
    y: view.region.top - (view.canvas.clientHeight / scale - view.region.height) / 2,
  };
}

// Zoom by `factor`, keeping the image pixel under `canvas_point` where it is.
function zoom_view_at(view: PlateView, canvas_point: V2, factor: number): void {
  const new_scale = Math.max(0.2, Math.min(40, view.image_pixels_to_canvas_scale * factor));
  const image_x = view.image_pixel_at_canvas_origin.x + canvas_point.x / view.image_pixels_to_canvas_scale;
  const image_y = view.image_pixel_at_canvas_origin.y + canvas_point.y / view.image_pixels_to_canvas_scale;
  view.image_pixels_to_canvas_scale = new_scale;
  view.image_pixel_at_canvas_origin = { x: image_x - canvas_point.x / new_scale, y: image_y - canvas_point.y / new_scale };
}

function draw_view(state: PlateViewsState, view: PlateView): void {
  const canvas = view.canvas;
  const device_pixel_ratio = window.devicePixelRatio || 1;
  const width = Math.round(canvas.clientWidth * device_pixel_ratio);
  const height = Math.round(canvas.clientHeight * device_pixel_ratio);
  if (canvas.width !== width || canvas.height !== height) {
    canvas.width = width;
    canvas.height = height;
  }
  const context = canvas.getContext("2d")!;
  context.setTransform(1, 0, 0, 1, 0, 0);
  context.fillStyle = "#fff";
  context.fillRect(0, 0, width, height);
  // From here on, coordinates are image pixels.
  const scale = view.image_pixels_to_canvas_scale * device_pixel_ratio;
  context.setTransform(scale, 0, 0, scale, -view.image_pixel_at_canvas_origin.x * scale, -view.image_pixel_at_canvas_origin.y * scale);
  if (state.plate_image !== null) {
    context.globalAlpha = state.plate_opacity;
    // The landmark pixels were read on the plate at this size, whatever the file's own.
    context.drawImage(state.plate_image, 0, 0, IMAGE_WIDTH_PIXELS, IMAGE_HEIGHT_PIXELS);
    context.globalAlpha = 1;
  }

  const mirror = (p: V3): V3 => ({ x: -p.x, y: p.y, z: p.z });
  const sides = view.draws_mirrored_side ? [(p: V3) => p, mirror] : [(p: V3) => p];
  for (const stroke of state.tablet_document.strokes) {
    if (stroke.layer === "skull" && !state.skull_visible) continue;
    const control_points = stroke_control_points(stroke, state.tablet_document);
    context.strokeStyle = stroke.layer === "skin" ? SKIN_STROKE_COLOR : SKULL_STROKE_COLOR;
    context.lineWidth = (stroke.layer === "skin" ? 1.6 : 1) / view.image_pixels_to_canvas_scale;
    for (const side of sides) {
      context.beginPath();
      for (let i = 0; i <= 24; i++) {
        const pixel = view.image_pixel_from_world(side(bezier_point(control_points, i / 24)));
        if (i === 0) context.moveTo(pixel.x, pixel.y);
        else context.lineTo(pixel.x, pixel.y);
      }
      context.stroke();
    }
  }

  if (state.landmarks_visible) {
    // A cross where the landmark was marked on the plate, a dot where the fitted vertex is.
    const arm = 5 / view.image_pixels_to_canvas_scale;
    context.strokeStyle = LANDMARK_COLOR;
    context.fillStyle = LANDMARK_COLOR;
    context.lineWidth = 1.2 / view.image_pixels_to_canvas_scale;
    const draw_cross = (pixel: V2): void => {
      context.beginPath();
      context.moveTo(pixel.x - arm, pixel.y);
      context.lineTo(pixel.x + arm, pixel.y);
      context.moveTo(pixel.x, pixel.y - arm);
      context.lineTo(pixel.x, pixel.y + arm);
      context.stroke();
    };
    for (const landmark of TARGET_LANDMARKS) {
      draw_cross(view.image_pixel_from_world(target_landmark_world_position(landmark)));
      const vertex = state.tablet_document.vertices.find((v) => v.id === landmark.template_vertex);
      if (vertex === undefined) continue; // deleted by hand in the sketchpad
      const fitted = view.image_pixel_from_world(vertex_world_position(state.tablet_document, vertex_by_id(state.tablet_document, landmark.template_vertex)));
      context.beginPath();
      context.arc(fitted.x, fitted.y, 2.5 / view.image_pixels_to_canvas_scale, 0, 2 * Math.PI);
      context.fill();
    }
    // The profile view is the one with z in it: the most-forward-point landmarks are
    // midline points marked in the profile only.
    if (!view.draws_mirrored_side) {
      for (const landmark of TARGET_MOST_FORWARD_POINT_LANDMARKS) draw_cross(view.image_pixel_from_world(target_most_forward_point_world_position(landmark)));
    }
  }
}

function draw_all_views(state: PlateViewsState): void {
  for (const view of state.views) draw_view(state, view);
}

function attach_view_gestures(state: PlateViewsState, view: PlateView): void {
  const canvas = view.canvas;
  const pointer_positions = new Map<number, V2>(); // pointerId -> last position, canvas CSS pixels
  let last_tap_milliseconds = 0;
  let moved_since_pointer_down = false;

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

  canvas.addEventListener("pointerdown", (e) => {
    e.preventDefault();
    try { canvas.setPointerCapture(e.pointerId); } catch {} // throws for synthetic events
    pointer_positions.set(e.pointerId, canvas_point(e));
    moved_since_pointer_down = false;
  });
  canvas.addEventListener("pointermove", (e) => {
    if (!pointer_positions.has(e.pointerId)) return;
    const before = centroid_and_spread();
    pointer_positions.set(e.pointerId, canvas_point(e));
    const after = centroid_and_spread();
    if (Math.hypot(after.centroid.x - before.centroid.x, after.centroid.y - before.centroid.y) > 0) moved_since_pointer_down = true;
    view.image_pixel_at_canvas_origin = {
      x: view.image_pixel_at_canvas_origin.x - (after.centroid.x - before.centroid.x) / view.image_pixels_to_canvas_scale,
      y: view.image_pixel_at_canvas_origin.y - (after.centroid.y - before.centroid.y) / view.image_pixels_to_canvas_scale,
    };
    if (pointer_positions.size >= 2 && before.spread > 0) zoom_view_at(view, after.centroid, after.spread / before.spread);
    draw_view(state, view);
  });
  const end_pointer = (e: PointerEvent): void => {
    if (!pointer_positions.delete(e.pointerId)) return;
    if (e.type === "pointerup" && !moved_since_pointer_down && pointer_positions.size === 0) {
      if (e.timeStamp - last_tap_milliseconds < 350) {
        fit_view_to_region(view);
        draw_view(state, view);
      }
      last_tap_milliseconds = e.timeStamp;
    }
  };
  canvas.addEventListener("pointerup", end_pointer);
  canvas.addEventListener("pointercancel", end_pointer);
  canvas.addEventListener("wheel", (e) => {
    e.preventDefault();
    zoom_view_at(view, canvas_point(e), Math.exp(-e.deltaY * 0.0015));
    draw_view(state, view);
  }, { passive: false });
}

// Read the document again when the file on the server is not the revision last drawn.
async function reload_document_if_changed(state: PlateViewsState, document_name: string): Promise<void> {
  const listed = await list_documents_from_server();
  const entry = listed?.find((e) => e.name === document_name);
  if (entry === undefined || entry.revision === state.document_revision) return;
  const response = await fetch(`/api/documents/${encodeURIComponent(document_name)}`, { cache: "no-store" });
  if (!response.ok) {
    console.error(`plate views: loading document '${document_name}' failed, status ${response.status}`);
    return;
  }
  // The camera in the file belongs to the sketchpad: read into a throwaway one.
  apply_document_state(await response.text(), state.tablet_document, default_camera());
  state.document_revision = entry.revision;
  draw_all_views(state);
}

export function start_loomis_girl_plate_views(document_name: string): void {
  const make_view = (canvas_id: string, region: PlateRegion, image_pixel_from_world: (p: V3) => V2, draws_mirrored_side: boolean): PlateView => ({
    canvas: document.getElementById(canvas_id) as HTMLCanvasElement,
    region, image_pixel_from_world, draws_mirrored_side,
    image_pixels_to_canvas_scale: 1,
    image_pixel_at_canvas_origin: { x: 0, y: 0 },
  });
  const opacity_slider = document.getElementById("plate_opacity") as HTMLInputElement;
  const skull_checkbox = document.getElementById("plate_skull_visible") as HTMLInputElement;
  const landmarks_checkbox = document.getElementById("plate_landmarks_visible") as HTMLInputElement;
  const state: PlateViewsState = {
    views: [
      make_view("front_plate_canvas", { left: 50, top: 30, width: 505, height: 690 }, front_pixel_from_world, true),
      make_view("profile_plate_canvas", { left: 580, top: 30, width: 630, height: 690 }, profile_pixel_from_world, false),
    ],
    tablet_document: empty_document(),
    document_revision: null,
    plate_image: null,
    plate_opacity: Number(opacity_slider.value),
    skull_visible: skull_checkbox.checked,
    landmarks_visible: landmarks_checkbox.checked,
  };
  for (const view of state.views) {
    fit_view_to_region(view);
    attach_view_gestures(state, view);
  }

  opacity_slider.addEventListener("input", () => {
    state.plate_opacity = Number(opacity_slider.value);
    draw_all_views(state);
  });
  skull_checkbox.addEventListener("change", () => {
    state.skull_visible = skull_checkbox.checked;
    draw_all_views(state);
  });
  landmarks_checkbox.addEventListener("change", () => {
    state.landmarks_visible = landmarks_checkbox.checked;
    draw_all_views(state);
  });
  window.addEventListener("resize", () => draw_all_views(state));

  const plate_image = new Image();
  plate_image.onload = () => {
    state.plate_image = plate_image;
    draw_all_views(state);
  };
  plate_image.onerror = () => {
    console.error(`plate views: ${PLATE_IMAGE_URL} not found (the scan is not in the repo, copy it into tablet/public/)`);
    document.getElementById("plate_missing_note")!.hidden = false;
  };
  plate_image.src = PLATE_IMAGE_URL;

  draw_all_views(state);
  void reload_document_if_changed(state, document_name);
  setInterval(() => void reload_document_if_changed(state, document_name), POLL_MILLISECONDS);
}
