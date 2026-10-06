// The plate views of the fit-head-to-loomis-girl page: the Loomis school girl plate with a
// document drawn over it, a front view and a profile view. The page shows two such pairs:
// the template before the fit and the fitted document after it.
// Both views are orthographic, so a world point maps to an image pixel by a scale and an
// offset (src/loomis_girl_plate.ts).
//
// A view is three canvases stacked in one container: the plate (2D), the document's
// surfaces and strokes (WebGL, depth-tested so the surfaces hide the strokes behind them,
// head_meshes.ts) seen through an orthographic OrbitCamera that reproduces the plate's
// scale and offset, and the landmarks (2D) on top. The top canvas takes the gestures.
//
// View only: the documents and the landmark file are read from the server and never
// written. Both are read again within POLL_MILLISECONDS of changing (the page
// loomis-girl-fit-landmarks writes them).
//
// Gestures, per view: one pointer drags the picture, two fingers pinch-zoom, the wheel
// zooms at the cursor, a double tap fits the head again.
import { OrbitCamera, camera_distance_for_world_units_per_pixel, camera_orthographic_view_projection, default_camera } from "../../src/camera";
import { TabletDocument, empty_document } from "../../src/document";
import { V2, V3, v3 } from "../../src/math";
import { Rgb } from "../../src/vertex_sink";
import { HeadMeshes, create_head_meshes, draw_head_meshes, update_head_meshes } from "./head_meshes";
import { apply_document_state, list_documents_from_server } from "../../src/persistence";
import {
  FRONT_REGION, FitLandmarksFile, IMAGE_HEIGHT_PIXELS, IMAGE_WIDTH_PIXELS, LOOMIS_GIRL_FIT_LANDMARKS_NAME, PIXELS_PER_WORLD_UNIT, PROFILE_PORION_X_PIXELS,
  PROFILE_REGION, PlateRegion, WORLD_Y_ZERO_ROW_PIXELS, fetch_fit_landmarks_file, fit_landmark_mark_world_position, front_pixel_from_world,
  plate_view_transform_showing_region, plate_view_transform_zoomed_at, profile_pixel_from_world, starting_fit_landmarks_file, template_point_world_position,
  template_position_is_on_midline,
} from "../../src/loomis_girl_plate";

const PLATE_IMAGE_URL = "/loomis-girl-front-and-profile.webp";
const POLL_MILLISECONDS = 2000;
const SKIN_STROKE_COLOR: Rgb = { r: 0.91, g: 0.15, b: 0.16 };
const SKULL_STROKE_COLOR: Rgb = { r: 0.16, g: 0.38, b: 0.91 };
const LANDMARK_COLOR = "#0a9a3a";
const SURFACE_OPACITY = 0.35; // the plate must stay readable under the surfaces

// One document drawn over the plate, as last read from the server.
type ShownDocument = {
  name: string;
  tablet_document: TabletDocument;
  revision: string | null; // null = nothing loaded yet
};

// One plate view. `image_pixels_to_canvas_scale` and `image_pixel_at_canvas_origin` say
// where the plate sits in the canvases (canvas CSS pixel = (image pixel - origin) * scale).
type PlateView = {
  container: HTMLElement;
  plate_canvas: HTMLCanvasElement;
  mesh_canvas: HTMLCanvasElement;
  mesh_gl: WebGLRenderingContext | null; // null = no WebGL, the view shows plate and landmarks only
  head_meshes: HeadMeshes | null;
  landmark_canvas: HTMLCanvasElement; // on top, takes the gestures
  shown: ShownDocument;
  region: PlateRegion;
  is_front_view: boolean;
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
  surfaces_visible: boolean;
  skull_visible: boolean;
  landmarks_visible: boolean;
};

// A front and a profile view showing one document; the ids name the views' containers.
export type PlateViewPair = { document_name: string; front_view_id: string; profile_view_id: string };

function fit_view_to_region(view: PlateView): void {
  const transform = plate_view_transform_showing_region(view.container.clientWidth, view.container.clientHeight, view.region);
  view.image_pixels_to_canvas_scale = transform.image_pixels_to_canvas_scale;
  view.image_pixel_at_canvas_origin = transform.image_pixel_at_canvas_origin;
}

// Zoom by `factor`, keeping the image pixel under `canvas_point` where it is.
function zoom_view_at(view: PlateView, canvas_point: V2, factor: number): void {
  const transform = plate_view_transform_zoomed_at(view, canvas_point, factor);
  view.image_pixels_to_canvas_scale = transform.image_pixels_to_canvas_scale;
  view.image_pixel_at_canvas_origin = transform.image_pixel_at_canvas_origin;
}

// Backing store at device resolution; the canvas fills its container by CSS.
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

// The image pixel the view shows at a canvas CSS pixel, inverted from the view transform.
function image_pixel_at_canvas_point(view: PlateView, canvas_point: V2): V2 {
  return {
    x: view.image_pixel_at_canvas_origin.x + canvas_point.x / view.image_pixels_to_canvas_scale,
    y: view.image_pixel_at_canvas_origin.y + canvas_point.y / view.image_pixels_to_canvas_scale,
  };
}

// The orthographic camera that shows the world as the plate does (front_pixel_from_world,
// profile_pixel_from_world): looking down -z for the front view, down -x for the profile
// (+z to the viewer's left), pivot at the world point under the canvas centre.
function plate_view_camera(state: PlateViewsState, view: PlateView): OrbitCamera {
  const centre = image_pixel_at_canvas_point(view, { x: view.container.clientWidth / 2, y: view.container.clientHeight / 2 });
  const y = (WORLD_Y_ZERO_ROW_PIXELS - centre.y) / PIXELS_PER_WORLD_UNIT;
  const pivot = view.is_front_view
    ? v3((centre.x - state.landmarks_file.front_midline_column_pixels) / PIXELS_PER_WORLD_UNIT, y, 0)
    : v3(0, y, (PROFILE_PORION_X_PIXELS - centre.x) / PIXELS_PER_WORLD_UNIT);
  return {
    pivot,
    yaw: view.is_front_view ? 0 : Math.PI / 2,
    pitch: 0,
    distance: camera_distance_for_world_units_per_pixel(1 / (view.image_pixels_to_canvas_scale * PIXELS_PER_WORLD_UNIT), view.container.clientHeight),
  };
}

function draw_plate_layer(state: PlateViewsState, view: PlateView): void {
  const { width, height, device_pixel_ratio } = size_canvas_to_container(view, view.plate_canvas);
  const context = view.plate_canvas.getContext("2d")!;
  context.setTransform(1, 0, 0, 1, 0, 0);
  context.fillStyle = "#fff";
  context.fillRect(0, 0, width, height);
  if (state.plate_image === null) return;
  const scale = view.image_pixels_to_canvas_scale * device_pixel_ratio;
  context.setTransform(scale, 0, 0, scale, -view.image_pixel_at_canvas_origin.x * scale, -view.image_pixel_at_canvas_origin.y * scale);
  context.globalAlpha = state.plate_opacity;
  // The landmark pixels were read on the plate at this size, whatever the file's own.
  context.drawImage(state.plate_image, 0, 0, IMAGE_WIDTH_PIXELS, IMAGE_HEIGHT_PIXELS);
}

function draw_head_layer(state: PlateViewsState, view: PlateView): void {
  const gl = view.mesh_gl;
  if (gl === null || view.head_meshes === null) return;
  const { width, height } = size_canvas_to_container(view, view.mesh_canvas);
  gl.viewport(0, 0, width, height);
  gl.clearColor(0, 0, 0, 0);
  gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
  if (view.shown.revision === null || height === 0) return;
  const camera = plate_view_camera(state, view);
  update_head_meshes(view.head_meshes, view.shown.tablet_document, view.shown.revision, camera, {
    skull_visible: state.skull_visible,
    surfaces_visible: state.surfaces_visible,
    strokes_visible: true,
    mirrored: true, // the document holds one side of the head
    surface_opacity: SURFACE_OPACITY,
    skin_stroke_color: SKIN_STROKE_COLOR,
    skull_stroke_color: SKULL_STROKE_COLOR,
  });
  draw_head_meshes(view.head_meshes, camera_orthographic_view_projection(camera, width / height), camera);
}

function draw_landmark_layer(state: PlateViewsState, view: PlateView): void {
  const tablet_document = view.shown.tablet_document;
  const { width, height, device_pixel_ratio } = size_canvas_to_container(view, view.landmark_canvas);
  const context = view.landmark_canvas.getContext("2d")!;
  context.setTransform(1, 0, 0, 1, 0, 0);
  context.clearRect(0, 0, width, height);
  if (!state.landmarks_visible) return;
  // From here on, coordinates are image pixels.
  const scale = view.image_pixels_to_canvas_scale * device_pixel_ratio;
  context.setTransform(scale, 0, 0, scale, -view.image_pixel_at_canvas_origin.x * scale, -view.image_pixel_at_canvas_origin.y * scale);
  const image_pixel_from_world = (p: V3): V2 => view.is_front_view ? front_pixel_from_world(p, state.landmarks_file.front_midline_column_pixels) : profile_pixel_from_world(p);

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

function draw_view(state: PlateViewsState, view: PlateView): void {
  draw_plate_layer(state, view);
  draw_head_layer(state, view);
  draw_landmark_layer(state, view);
}

function draw_all_views(state: PlateViewsState): void {
  for (const view of state.views) draw_view(state, view);
}

function attach_view_gestures(state: PlateViewsState, view: PlateView): void {
  const canvas = view.landmark_canvas;
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
  const surfaces_checkbox = document.getElementById("plate_surfaces_visible") as HTMLInputElement;
  const skull_checkbox = document.getElementById("plate_skull_visible") as HTMLInputElement;
  const landmarks_checkbox = document.getElementById("plate_landmarks_visible") as HTMLInputElement;
  const state: PlateViewsState = {
    views: [],
    shown_documents: [],
    template_document: empty_document(),
    landmarks_file: starting_fit_landmarks_file(),
    plate_image: null,
    plate_opacity: Number(opacity_slider.value),
    surfaces_visible: surfaces_checkbox.checked,
    skull_visible: skull_checkbox.checked,
    landmarks_visible: landmarks_checkbox.checked,
  };
  const make_view = (container_id: string, shown: ShownDocument, region: PlateRegion, is_front_view: boolean): PlateView => {
    const container = document.getElementById(container_id)!;
    const add_canvas = (): HTMLCanvasElement => container.appendChild(document.createElement("canvas"));
    const plate_canvas = add_canvas();
    const mesh_canvas = add_canvas();
    const landmark_canvas = add_canvas();
    const mesh_gl = mesh_canvas.getContext("webgl");
    if (mesh_gl === null) console.error(`plate views: no WebGL, view '${container_id}' shows no surfaces or strokes`);
    return {
      container, plate_canvas, mesh_canvas, mesh_gl, landmark_canvas,
      head_meshes: mesh_gl === null ? null : create_head_meshes(mesh_gl),
      shown, region, is_front_view,
      image_pixels_to_canvas_scale: 1,
      image_pixel_at_canvas_origin: { x: 0, y: 0 },
    };
  };
  for (const pair of pairs) {
    const shown: ShownDocument = { name: pair.document_name, tablet_document: empty_document(), revision: null };
    if (pair.document_name === template_document_name) state.template_document = shown.tablet_document;
    state.shown_documents.push(shown);
    state.views.push(make_view(pair.front_view_id, shown, FRONT_REGION, true));
    state.views.push(make_view(pair.profile_view_id, shown, PROFILE_REGION, false));
  }
  for (const view of state.views) {
    fit_view_to_region(view);
    attach_view_gestures(state, view);
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
