// The plate views of the fit-head-to-loomis-girl page: the Loomis school girl plate with a
// document drawn over it, front view in one canvas, profile in the other. The page shows
// two such pairs: the template before the fit and the fitted document after it.
// Both views are orthographic, so a world point maps to an image pixel by a scale and an
// offset (src/loomis_girl_plate.ts).
//
// View only: the documents and the landmark file are read from the server and never
// written. Both are read again within POLL_MILLISECONDS of changing (the page
// loomis-girl-fit-landmarks writes them).
//
// Gestures, per view: one pointer drags the picture, two fingers pinch-zoom, the wheel
// zooms at the cursor, a double tap fits the head again.
import { default_camera } from "../../src/camera";
import { TabletDocument, bezier_point, empty_document, stroke_control_points } from "../../src/document";
import { V2, V3 } from "../../src/math";
import { apply_document_state, list_documents_from_server } from "../../src/persistence";
import {
  FRONT_REGION, FitLandmarksFile, IMAGE_HEIGHT_PIXELS, IMAGE_WIDTH_PIXELS, LOOMIS_GIRL_FIT_LANDMARKS_NAME, PROFILE_REGION, PlateRegion,
  fetch_fit_landmarks_file, fit_landmark_mark_world_position, front_pixel_from_world, plate_view_transform_showing_region, plate_view_transform_zoomed_at,
  profile_pixel_from_world, starting_fit_landmarks_file, template_point_world_position, template_position_is_on_midline,
} from "../../src/loomis_girl_plate";

const PLATE_IMAGE_URL = "/loomis-girl-front-and-profile.webp";
const POLL_MILLISECONDS = 2000;
const SKIN_STROKE_COLOR = "#e8262a";
const SKULL_STROKE_COLOR = "#2a62e8";
const LANDMARK_COLOR = "#0a9a3a";

// One document drawn over the plate, as last read from the server.
type ShownDocument = {
  name: string;
  tablet_document: TabletDocument;
  revision: string | null; // null = nothing loaded yet
};

// One plate view. `image_pixels_to_canvas_scale` and `image_pixel_at_canvas_origin` say
// where the plate sits in the canvas (canvas CSS pixel = (image pixel - origin) * scale).
type PlateView = {
  canvas: HTMLCanvasElement;
  shown: ShownDocument;
  region: PlateRegion;
  is_front_view: boolean; // the front view also draws the mirrored side: the document holds one side of the face
  image_pixels_to_canvas_scale: number;
  image_pixel_at_canvas_origin: V2;
};

type PlateViewsState = {
  views: PlateView[];
  shown_documents: ShownDocument[];
  template_document: TabletDocument; // says which landmarks are midline landmarks
  landmarks_file: FitLandmarksFile;
  plate_image: HTMLImageElement | null; // null = not loaded (the scan is not in the repo)
  plate_opacity: number;
  skull_visible: boolean;
  landmarks_visible: boolean;
};

// A front and a profile canvas showing one document.
export type PlateViewPair = { document_name: string; front_canvas_id: string; profile_canvas_id: string };

function fit_view_to_region(view: PlateView): void {
  const transform = plate_view_transform_showing_region(view.canvas.clientWidth, view.canvas.clientHeight, view.region);
  view.image_pixels_to_canvas_scale = transform.image_pixels_to_canvas_scale;
  view.image_pixel_at_canvas_origin = transform.image_pixel_at_canvas_origin;
}

// Zoom by `factor`, keeping the image pixel under `canvas_point` where it is.
function zoom_view_at(view: PlateView, canvas_point: V2, factor: number): void {
  const transform = plate_view_transform_zoomed_at(view, canvas_point, factor);
  view.image_pixels_to_canvas_scale = transform.image_pixels_to_canvas_scale;
  view.image_pixel_at_canvas_origin = transform.image_pixel_at_canvas_origin;
}

function draw_view(state: PlateViewsState, view: PlateView): void {
  const canvas = view.canvas;
  const tablet_document = view.shown.tablet_document;
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

  const image_pixel_from_world = (p: V3): V2 => view.is_front_view ? front_pixel_from_world(p, state.landmarks_file.front_midline_column_pixels) : profile_pixel_from_world(p);
  const mirror = (p: V3): V3 => ({ x: -p.x, y: p.y, z: p.z });
  const sides = view.is_front_view ? [(p: V3) => p, mirror] : [(p: V3) => p];
  for (const stroke of tablet_document.strokes) {
    if (stroke.layer === "skull" && !state.skull_visible) continue;
    const control_points = stroke_control_points(stroke, tablet_document);
    context.strokeStyle = stroke.layer === "skin" ? SKIN_STROKE_COLOR : SKULL_STROKE_COLOR;
    context.lineWidth = (stroke.layer === "skin" ? 1.6 : 1) / view.image_pixels_to_canvas_scale;
    for (const side of sides) {
      context.beginPath();
      for (let i = 0; i <= 24; i++) {
        const pixel = image_pixel_from_world(side(bezier_point(control_points, i / 24)));
        if (i === 0) context.moveTo(pixel.x, pixel.y);
        else context.lineTo(pixel.x, pixel.y);
      }
      context.stroke();
    }
  }

  if (state.landmarks_visible) {
    // A cross where the landmark was marked on the plate, a dot where the document's point
    // is, and a line between the two: how far the point is from its mark.
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
    for (const landmark of state.landmarks_file.landmarks) {
      const template_position = template_point_world_position(state.template_document, landmark.template_point);
      if (template_position === null) continue; // template not loaded yet, or the point was deleted from it
      const marked = image_pixel_from_world(fit_landmark_mark_world_position(landmark, template_position_is_on_midline(template_position)));
      draw_cross(marked);
      const drawn_position = template_point_world_position(tablet_document, landmark.template_point);
      if (drawn_position === null) continue; // not loaded yet, or deleted from the document
      const drawn = image_pixel_from_world(drawn_position);
      context.beginPath();
      context.moveTo(drawn.x, drawn.y);
      context.lineTo(marked.x, marked.y);
      context.stroke();
      context.beginPath();
      context.arc(drawn.x, drawn.y, 2.5 / view.image_pixels_to_canvas_scale, 0, 2 * Math.PI);
      context.fill();
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

// Read again every document whose file on the server is not the revision last drawn, and
// the landmark file.
async function reload_documents_if_changed(state: PlateViewsState, on_document_loaded: PlateViewsListener): Promise<void> {
  const landmarks_file = await fetch_fit_landmarks_file(LOOMIS_GIRL_FIT_LANDMARKS_NAME);
  if (landmarks_file !== null && JSON.stringify(landmarks_file) !== JSON.stringify(state.landmarks_file)) {
    state.landmarks_file = landmarks_file;
    draw_all_views(state);
    for (const shown of state.shown_documents) if (shown.revision !== null) on_document_loaded(shown.name, shown.tablet_document, state);
  }
  const listed = await list_documents_from_server();
  for (const shown of state.shown_documents) {
    const entry = listed?.find((e) => e.name === shown.name);
    if (entry === undefined || entry.revision === shown.revision) continue;
    const response = await fetch(`/api/documents/${encodeURIComponent(shown.name)}`, { cache: "no-store" });
    if (!response.ok) {
      console.error(`plate views: loading document '${shown.name}' failed, status ${response.status}`);
      continue;
    }
    // The camera in the file belongs to the sketchpad: read into a throwaway one.
    apply_document_state(await response.text(), shown.tablet_document, default_camera());
    shown.revision = entry.revision;
    draw_all_views(state);
    on_document_loaded(shown.name, shown.tablet_document, state);
  }
}

// What the listener gets besides the document: the landmarks and the template they point into.
export type PlateViewsLandmarks = { template_document: TabletDocument; landmarks_file: FitLandmarksFile };
export type PlateViewsListener = (document_name: string, tablet_document: TabletDocument, landmarks: PlateViewsLandmarks) => void;

// `on_document_loaded` is called each time a document was read (first load and every
// change), and for every loaded document when the landmark file changed.
// `template_document_name` must be one of the pairs' documents.
export function start_loomis_girl_plate_views(pairs: PlateViewPair[], template_document_name: string, on_document_loaded: PlateViewsListener): void {
  const opacity_slider = document.getElementById("plate_opacity") as HTMLInputElement;
  const skull_checkbox = document.getElementById("plate_skull_visible") as HTMLInputElement;
  const landmarks_checkbox = document.getElementById("plate_landmarks_visible") as HTMLInputElement;
  const state: PlateViewsState = {
    views: [],
    shown_documents: [],
    template_document: empty_document(),
    landmarks_file: starting_fit_landmarks_file(),
    plate_image: null,
    plate_opacity: Number(opacity_slider.value),
    skull_visible: skull_checkbox.checked,
    landmarks_visible: landmarks_checkbox.checked,
  };
  const make_view = (canvas_id: string, shown: ShownDocument, region: PlateRegion, is_front_view: boolean): PlateView => ({
    canvas: document.getElementById(canvas_id) as HTMLCanvasElement,
    shown, region, is_front_view,
    image_pixels_to_canvas_scale: 1,
    image_pixel_at_canvas_origin: { x: 0, y: 0 },
  });
  for (const pair of pairs) {
    const shown: ShownDocument = { name: pair.document_name, tablet_document: empty_document(), revision: null };
    if (pair.document_name === template_document_name) state.template_document = shown.tablet_document;
    state.shown_documents.push(shown);
    state.views.push(make_view(pair.front_canvas_id, shown, FRONT_REGION, true));
    state.views.push(make_view(pair.profile_canvas_id, shown, PROFILE_REGION, false));
  }
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
  // The canvases follow the page's width, so the head is fitted to the new size.
  window.addEventListener("resize", () => {
    for (const view of state.views) fit_view_to_region(view);
    draw_all_views(state);
  });

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
  void reload_documents_if_changed(state, on_document_loaded);
  setInterval(() => void reload_documents_if_changed(state, on_document_loaded), POLL_MILLISECONDS);
}
