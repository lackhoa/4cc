// Plate views of the Loomis school girl plate: the plate with a document drawn over it, a
// front view and a profile view per pair. Used by the pages fit-head-to-loomis-girl (the
// fitted head with the marks to drag, and the template before the fit) and
// fit-hairline-to-loomis-girl (the fitted head with the hairline highlighted, no marks).
// Both views are orthographic, so a world point maps to an image pixel by a scale and an
// offset (loomis_girl_plate.ts).
//
// A view is three canvases stacked in one container: the plate (2D), the document's
// surfaces and strokes (WebGL, depth-tested so the surfaces hide the strokes behind them,
// head_meshes.ts) seen through an orthographic OrbitCamera that reproduces the plate's
// scale and offset, and an overlay (2D) on top: the on-surface skin strokes (the hairline)
// and the landmarks. The top canvas takes the gestures.
//
// The module loads only the plate image. The page owns the documents and the landmark
// file: the views ask for them on every draw (`shown` of a pair, `scene`), and the page
// calls `draw` of the returned PlateViews after it changed any of them.
//
// A landmark's "mark" is where it sits on the plate. A mark read from the view is a cross,
// a guessed one a circle; a line runs from the mark to where the shown document has the
// landmark's point (a dot). A landmark off the midline has two marks in the front view.
//
// Gestures, per view: a drag on a mark moves it (pairs with `marks: "editable"`), a drag
// on the midline handle moves the midline, a drag elsewhere moves the picture, two fingers
// pinch-zoom, the wheel zooms at the cursor, a double tap on nothing fits the head again.
import { OrbitCamera, camera_distance_for_world_units_per_pixel, camera_orthographic_view_projection } from "./camera";
import { TabletDocument } from "./document";
import { V2, V3, v3 } from "./math";
import { stroke_drawn_polyline } from "./patch";
import { Rgb } from "./vertex_sink";
import { HeadMeshes, create_head_meshes, draw_head_meshes, update_head_meshes } from "./head_meshes";
import {
  FRONT_REGION, FitLandmark, FitLandmarksFile, IMAGE_HEIGHT_PIXELS, IMAGE_WIDTH_PIXELS, PIXELS_PER_WORLD_UNIT, PROFILE_PORION_X_PIXELS,
  PROFILE_REGION, PlateRegion, WORLD_Y_ZERO_ROW_PIXELS, front_pixel_from_world,
  plate_view_transform_showing_region, plate_view_transform_zoomed_at, profile_pixel_from_world, template_point_world_position,
  template_position_is_on_midline,
} from "./loomis_girl_plate";

const PLATE_IMAGE_URL = "/loomis-girl-front-and-profile.webp";
const SKIN_STROKE_COLOR: Rgb = { r: 0.91, g: 0.15, b: 0.16 };
const SKULL_STROKE_COLOR: Rgb = { r: 0.16, g: 0.38, b: 0.91 };
const LANDMARK_STYLE = "#0a9a3a";
const SELECTED_LANDMARK_STYLE = "#d6007f";
const MISSING_TEMPLATE_POINT_STYLE = "#c0392b";
const MIDLINE_STYLE = "#7a4bd6";
const ON_SURFACE_STROKE_HIGHLIGHT_STYLE = "#ffa94d";
const HAIRLINE_PLANAR_STROKE_SAMPLES = 16; // segments drawn per hairline stroke that is a plain cubic
const SURFACE_OPACITY = 0.35; // the plate must stay readable under the surfaces
const GRAB_RADIUS_PIXELS = 14;
// The midline is dragged by its handle at the top edge of the front view.
const MIDLINE_HANDLE_CENTER_Y_PIXELS = 12;

// A document to draw over the plate. `revision` is any string that changes when the
// document does (the meshes are rebuilt then).
export type ShownDocument = { tablet_document: TabletDocument; revision: string };

// A front and a profile view showing one document; the ids name the views' containers.
export type PlateViewPair = {
  front_view_id: string;
  profile_view_id: string;
  shown: () => ShownDocument | null; // null = nothing to show yet
  marks: "editable" | "shown" | "hidden";
  on_surface_skin_strokes_highlighted: boolean; // the hairline, in orange over everything
};

// What the views draw besides the documents, asked from the page on every draw.
export type PlateViewsScene = {
  template_document: TabletDocument; // says which landmarks are midline landmarks
  landmarks_file: FitLandmarksFile;
  selected_landmark_index: number | null;
  landmark_names_visible: boolean;
};

// What a page with editable marks is told. A drag changes `landmarks_file` of the scene
// in place.
export type PlateMarkEditing = {
  is_editable: () => boolean; // false = marks cannot be grabbed (the landmark file is not loaded)
  on_drag_started: () => void; // before the first change of a drag: remember the file for undo
  on_dragged: () => void; // the file changed, the drag goes on: refit, draw
  on_drag_ended: () => void; // the drag is over: save
  on_landmark_selected: (landmark_index: number | null) => void;
};

export type PlateViews = { draw: () => void };

type PlateDrag =
  | { kind: "mark"; landmark_index: number; side_sign: number; pointer_to_mark_offset: V2; has_moved: boolean }
  | { kind: "midline"; has_moved: boolean }
  | { kind: "pan"; travel_pixels: number };

// One plate view. `image_pixels_to_canvas_scale` and `image_pixel_at_canvas_origin` say
// where the plate sits in the canvases (canvas CSS pixel = (image pixel - origin) * scale).
type PlateView = {
  container: HTMLElement;
  plate_canvas: HTMLCanvasElement;
  mesh_canvas: HTMLCanvasElement;
  mesh_gl: WebGLRenderingContext | null; // null = no WebGL, the view shows plate and overlay only
  head_meshes: HeadMeshes | null;
  overlay_canvas: HTMLCanvasElement; // on top, takes the gestures
  pair: PlateViewPair;
  region: PlateRegion;
  is_front_view: boolean;
  image_pixels_to_canvas_scale: number;
  image_pixel_at_canvas_origin: V2;
  hovered_landmark_index: number | null;
  pointers: Map<number, V2>; // pointerId -> last position, canvas CSS pixels
  drag: PlateDrag | null;
  last_empty_tap_time_ms: number;
};

type PlateViewsState = {
  views: PlateView[];
  scene: () => PlateViewsScene;
  mark_editing: PlateMarkEditing | null;
  plate_image: HTMLImageElement | null; // null = not loaded (the scan is not in the repo)
  plate_opacity: number;
  surfaces_visible: boolean;
  skull_visible: boolean;
  landmarks_visible: boolean;
};

type PlateMark = { landmark_index: number; side_sign: number; image_pixel: V2 };

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

function canvas_point_at_image_pixel(view: PlateView, image_pixel: V2): V2 {
  return {
    x: (image_pixel.x - view.image_pixel_at_canvas_origin.x) * view.image_pixels_to_canvas_scale,
    y: (image_pixel.y - view.image_pixel_at_canvas_origin.y) * view.image_pixels_to_canvas_scale,
  };
}

function view_pixel_from_world(view: PlateView, landmarks_file: FitLandmarksFile, p: V3): V2 {
  return view.is_front_view ? front_pixel_from_world(p, landmarks_file.front_midline_column_pixels) : profile_pixel_from_world(p);
}

// The orthographic camera that shows the world as the plate does (front_pixel_from_world,
// profile_pixel_from_world): looking down -z for the front view, down -x for the profile
// (+z to the viewer's left), pivot at the world point under the canvas centre.
function plate_view_camera(state: PlateViewsState, view: PlateView): OrbitCamera {
  const centre = image_pixel_at_canvas_point(view, { x: view.container.clientWidth / 2, y: view.container.clientHeight / 2 });
  const y = (WORLD_Y_ZERO_ROW_PIXELS - centre.y) / PIXELS_PER_WORLD_UNIT;
  const pivot = view.is_front_view
    ? v3((centre.x - state.scene().landmarks_file.front_midline_column_pixels) / PIXELS_PER_WORLD_UNIT, y, 0)
    : v3(0, y, (PROFILE_PORION_X_PIXELS - centre.x) / PIXELS_PER_WORLD_UNIT);
  return {
    pivot,
    yaw: view.is_front_view ? 0 : Math.PI / 2,
    pitch: 0,
    distance: camera_distance_for_world_units_per_pixel(1 / (view.image_pixels_to_canvas_scale * PIXELS_PER_WORLD_UNIT), view.container.clientHeight),
  };
}

// A landmark on the midline has one mark in the front view; any other has one on each side
// of the face. The template decides; a landmark whose template point is gone keeps what its
// mark says.
function landmark_is_on_midline(scene: PlateViewsScene, landmark: FitLandmark): boolean {
  const template_position = template_point_world_position(scene.template_document, landmark.template_point);
  if (template_position === null) return landmark.front_half_width_pixels === 0;
  return template_position_is_on_midline(template_position);
}

// The marks of one landmark in a view: two in the front view when the landmark is paired.
function marks_of_landmark(view: PlateView, scene: PlateViewsScene, landmark: FitLandmark, landmark_index: number): PlateMark[] {
  if (!view.is_front_view) return [{ landmark_index, side_sign: 1, image_pixel: { x: landmark.profile_column_pixels, y: landmark.row_pixels } }];
  const midline = scene.landmarks_file.front_midline_column_pixels;
  if (landmark_is_on_midline(scene, landmark)) return [{ landmark_index, side_sign: 1, image_pixel: { x: midline, y: landmark.row_pixels } }];
  return [1, -1].map((side_sign) => ({ landmark_index, side_sign, image_pixel: { x: midline + side_sign * landmark.front_half_width_pixels, y: landmark.row_pixels } }));
}

function marks_are_drawn(state: PlateViewsState, view: PlateView): boolean {
  return view.pair.marks !== "hidden" && state.landmarks_visible;
}

function marks_can_be_dragged(state: PlateViewsState, view: PlateView): boolean {
  return view.pair.marks === "editable" && marks_are_drawn(state, view) && state.mark_editing !== null && state.mark_editing.is_editable();
}

function mark_under_pointer(state: PlateViewsState, view: PlateView, canvas_point: V2): PlateMark | null {
  if (!marks_are_drawn(state, view)) return null;
  const scene = state.scene();
  let nearest: PlateMark | null = null;
  let nearest_distance = GRAB_RADIUS_PIXELS;
  scene.landmarks_file.landmarks.forEach((landmark, landmark_index) => {
    for (const mark of marks_of_landmark(view, scene, landmark, landmark_index)) {
      const mark_point = canvas_point_at_image_pixel(view, mark.image_pixel);
      let distance = Math.hypot(mark_point.x - canvas_point.x, mark_point.y - canvas_point.y);
      if (landmark_index === scene.selected_landmark_index) distance *= 0.5; // where marks crowd, the selected one is the one grabbed
      if (distance <= nearest_distance) {
        nearest = mark;
        nearest_distance = distance;
      }
    }
  });
  return nearest;
}

function midline_handle_is_under_pointer(state: PlateViewsState, view: PlateView, canvas_point: V2): boolean {
  if (!view.is_front_view || !marks_can_be_dragged(state, view)) return false;
  const handle_x = canvas_point_at_image_pixel(view, { x: state.scene().landmarks_file.front_midline_column_pixels, y: 0 }).x;
  return Math.hypot(handle_x - canvas_point.x, MIDLINE_HANDLE_CENTER_Y_PIXELS - canvas_point.y) <= GRAB_RADIUS_PIXELS;
}

// Moves the dragged mark to the pointer. The view a mark is dragged in becomes a view the
// landmark is read from.
function drag_mark_to(state: PlateViewsState, view: PlateView, drag: PlateDrag & { kind: "mark" }, canvas_point: V2): void {
  const scene = state.scene();
  const landmark = scene.landmarks_file.landmarks[drag.landmark_index];
  const image_pixel = image_pixel_at_canvas_point(view, { x: canvas_point.x + drag.pointer_to_mark_offset.x, y: canvas_point.y + drag.pointer_to_mark_offset.y });
  landmark.row_pixels = image_pixel.y;
  if (view.is_front_view) {
    if (!landmark_is_on_midline(scene, landmark)) landmark.front_half_width_pixels = Math.max(0, (image_pixel.x - scene.landmarks_file.front_midline_column_pixels) * drag.side_sign);
    landmark.read_from_front = true;
  } else {
    landmark.profile_column_pixels = image_pixel.x;
    landmark.read_from_profile = true;
  }
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
  const shown = view.pair.shown();
  if (shown === null || height === 0) return;
  const camera = plate_view_camera(state, view);
  update_head_meshes(view.head_meshes, shown.tablet_document, shown.revision, camera, {
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

function draw_overlay_layer(state: PlateViewsState, view: PlateView): void {
  const scene = state.scene();
  const shown = view.pair.shown();
  const { width, height, device_pixel_ratio } = size_canvas_to_container(view, view.overlay_canvas);
  const context = view.overlay_canvas.getContext("2d")!;
  context.setTransform(1, 0, 0, 1, 0, 0);
  context.clearRect(0, 0, width, height);
  // From here on, coordinates are image pixels.
  const canvas_scale = view.image_pixels_to_canvas_scale;
  const scale = canvas_scale * device_pixel_ratio;
  context.setTransform(scale, 0, 0, scale, -view.image_pixel_at_canvas_origin.x * scale, -view.image_pixel_at_canvas_origin.y * scale);

  // The skin's on-surface strokes of the shown document (the hairline), highlighted over
  // the plate: both sides in front (the document holds one side).
  if (view.pair.on_surface_skin_strokes_highlighted && shown !== null) {
    context.strokeStyle = ON_SURFACE_STROKE_HIGHLIGHT_STYLE;
    context.lineWidth = 2.5 / canvas_scale;
    context.lineJoin = "round";
    // A hairline stroke is a skin stroke drawn on the surface, or one that ends at a point
    // pinned to the surface (the hairline drawn so far is of the second kind).
    const surface_pinned_vertices = new Set(shown.tablet_document.vertex_surface_pins.map((pin) => pin.vertex));
    for (const stroke of shown.tablet_document.strokes) {
      if (stroke.layer !== "skin") continue;
      const is_hairline_stroke = stroke.on_surface === true || surface_pinned_vertices.has(stroke.p0_vertex) || surface_pinned_vertices.has(stroke.p3_vertex);
      if (!is_hairline_stroke) continue;
      const samples = stroke_drawn_polyline(stroke, shown.tablet_document, HAIRLINE_PLANAR_STROKE_SAMPLES);
      for (const side_sign of view.is_front_view ? [1, -1] : [1]) {
        context.beginPath();
        samples.forEach((sample, k) => {
          const pixel = view_pixel_from_world(view, scene.landmarks_file, v3(side_sign * sample.x, sample.y, sample.z));
          if (k === 0) context.moveTo(pixel.x, pixel.y);
          else context.lineTo(pixel.x, pixel.y);
        });
        context.stroke();
      }
    }
  }
  if (!marks_are_drawn(state, view)) return;

  const is_editable = view.pair.marks === "editable";
  if (view.is_front_view && is_editable) {
    const midline = scene.landmarks_file.front_midline_column_pixels;
    context.strokeStyle = MIDLINE_STYLE;
    context.lineWidth = 1 / canvas_scale;
    context.setLineDash([6 / canvas_scale, 4 / canvas_scale]);
    context.beginPath();
    context.moveTo(midline, -IMAGE_HEIGHT_PIXELS);
    context.lineTo(midline, 2 * IMAGE_HEIGHT_PIXELS);
    context.stroke();
    context.setLineDash([]);
  }

  const labels: { text: string; image_pixel: V2; is_selected: boolean; style: string }[] = [];
  scene.landmarks_file.landmarks.forEach((landmark, landmark_index) => {
    const is_selected = is_editable && landmark_index === scene.selected_landmark_index;
    const template_position = template_point_world_position(scene.template_document, landmark.template_point);
    // An empty template is one that is not loaded yet, not one with every point missing.
    const is_missing_template_point = template_position === null && scene.template_document.vertices.length > 0;
    const is_read_from_this_view = view.is_front_view ? landmark.read_from_front : landmark.read_from_profile;
    const style = is_missing_template_point ? MISSING_TEMPLATE_POINT_STYLE : is_selected ? SELECTED_LANDMARK_STYLE : LANDMARK_STYLE;
    const drawn_position = shown === null ? null : template_point_world_position(shown.tablet_document, landmark.template_point);
    const marks = marks_of_landmark(view, scene, landmark, landmark_index);
    for (const mark of marks) {
      const { x, y } = mark.image_pixel;
      context.strokeStyle = style;
      context.fillStyle = style;
      context.globalAlpha = is_read_from_this_view || is_selected ? 1 : 0.6;
      // From the mark to where the shown document has the landmark's point.
      if (drawn_position !== null) {
        const drawn_pixel = view_pixel_from_world(view, scene.landmarks_file, v3(mark.side_sign * Math.abs(drawn_position.x), drawn_position.y, drawn_position.z));
        context.lineWidth = 1 / canvas_scale;
        context.beginPath();
        context.moveTo(x, y);
        context.lineTo(drawn_pixel.x, drawn_pixel.y);
        context.stroke();
        context.beginPath();
        context.arc(drawn_pixel.x, drawn_pixel.y, 1.8 / canvas_scale, 0, 2 * Math.PI);
        context.fill();
      }
      const arm = (is_selected ? 9 : 6) / canvas_scale;
      context.lineWidth = (is_selected ? 2.5 : 1.6) / canvas_scale;
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
    if (is_selected || scene.landmark_names_visible || landmark_index === view.hovered_landmark_index) {
      labels.push({ text: is_missing_template_point ? `${landmark.name} (template point missing)` : landmark.name, image_pixel: marks[0].image_pixel, is_selected, style });
    }
  });

  // CSS pixel coordinates from here: text and the midline handle keep their size when zoomed.
  context.setTransform(device_pixel_ratio, 0, 0, device_pixel_ratio, 0, 0);
  for (const label of labels) {
    const point = canvas_point_at_image_pixel(view, label.image_pixel);
    context.font = label.is_selected ? "bold 15px system-ui, sans-serif" : "12px system-ui, sans-serif";
    context.lineWidth = 3;
    context.strokeStyle = "#ffffff";
    context.strokeText(label.text, point.x + 9, point.y - 7);
    context.fillStyle = label.style;
    context.fillText(label.text, point.x + 9, point.y - 7);
  }
  if (view.is_front_view && is_editable) {
    const handle_x = canvas_point_at_image_pixel(view, { x: scene.landmarks_file.front_midline_column_pixels, y: 0 }).x;
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

function draw_view(state: PlateViewsState, view: PlateView): void {
  draw_plate_layer(state, view);
  draw_head_layer(state, view);
  draw_overlay_layer(state, view);
}

function draw_all_views(state: PlateViewsState): void {
  for (const view of state.views) draw_view(state, view);
}

function attach_view_gestures(state: PlateViewsState, view: PlateView): void {
  const canvas = view.overlay_canvas;
  const canvas_point_of = (e: PointerEvent | WheelEvent): V2 => {
    const rectangle = canvas.getBoundingClientRect();
    return { x: e.clientX - rectangle.left, y: e.clientY - rectangle.top };
  };
  const centroid_and_spread = (): { centroid: V2; spread: number } => {
    const points = [...view.pointers.values()];
    const centroid = { x: points.reduce((sum, p) => sum + p.x, 0) / points.length, y: points.reduce((sum, p) => sum + p.y, 0) / points.length };
    const spread = points.reduce((sum, p) => sum + Math.hypot(p.x - centroid.x, p.y - centroid.y), 0) / points.length;
    return { centroid, spread };
  };

  const end_drag = (): void => {
    const drag = view.drag;
    view.drag = null;
    if (drag === null) return;
    if (drag.kind === "pan") {
      if (drag.travel_pixels >= 4) return;
      // A tap on nothing: deselect; two in a row show the whole head again.
      const now_ms = performance.now();
      if (now_ms - view.last_empty_tap_time_ms < 350) {
        fit_view_to_region(view);
        draw_view(state, view);
      }
      view.last_empty_tap_time_ms = now_ms;
      if (view.pair.marks === "editable") state.mark_editing?.on_landmark_selected(null);
      return;
    }
    // An edit drag (mark or midline) ends: what it changed is saved.
    if (drag.has_moved) state.mark_editing?.on_drag_ended();
  };

  canvas.addEventListener("pointerdown", (e) => {
    if (e.button !== 0 && e.pointerType === "mouse") return;
    e.preventDefault();
    try { canvas.setPointerCapture(e.pointerId); } catch { /* throws for synthetic events */ }
    const canvas_point = canvas_point_of(e);
    view.pointers.set(e.pointerId, canvas_point);
    if (view.pointers.size > 1) {
      // A second finger: the gesture is a pinch, whatever the first finger had grabbed.
      end_drag();
      view.drag = { kind: "pan", travel_pixels: Infinity };
      return;
    }
    const mark = marks_can_be_dragged(state, view) ? mark_under_pointer(state, view, canvas_point) : null;
    if (mark !== null) {
      const mark_point = canvas_point_at_image_pixel(view, mark.image_pixel);
      view.drag = { kind: "mark", landmark_index: mark.landmark_index, side_sign: mark.side_sign, pointer_to_mark_offset: { x: mark_point.x - canvas_point.x, y: mark_point.y - canvas_point.y }, has_moved: false };
      state.mark_editing!.on_landmark_selected(mark.landmark_index);
    } else if (midline_handle_is_under_pointer(state, view, canvas_point)) {
      view.drag = { kind: "midline", has_moved: false };
    } else {
      view.drag = { kind: "pan", travel_pixels: 0 };
    }
  });

  canvas.addEventListener("pointermove", (e) => {
    const canvas_point = canvas_point_of(e);
    const previous_point = view.pointers.get(e.pointerId);
    if (previous_point === undefined || view.drag === null) {
      // No button down: say what a press here would grab.
      const mark = mark_under_pointer(state, view, canvas_point);
      const hovered = mark === null ? null : mark.landmark_index;
      const can_drag = marks_can_be_dragged(state, view);
      canvas.style.cursor = mark !== null && can_drag ? "move" : midline_handle_is_under_pointer(state, view, canvas_point) ? "ew-resize" : "grab";
      if (hovered !== view.hovered_landmark_index) {
        view.hovered_landmark_index = hovered;
        draw_overlay_layer(state, view);
      }
      return;
    }
    const drag = view.drag;
    if (drag.kind === "pan") {
      const before = centroid_and_spread();
      view.pointers.set(e.pointerId, canvas_point);
      const after = centroid_and_spread();
      drag.travel_pixels += Math.hypot(after.centroid.x - before.centroid.x, after.centroid.y - before.centroid.y);
      view.image_pixel_at_canvas_origin = {
        x: view.image_pixel_at_canvas_origin.x - (after.centroid.x - before.centroid.x) / view.image_pixels_to_canvas_scale,
        y: view.image_pixel_at_canvas_origin.y - (after.centroid.y - before.centroid.y) / view.image_pixels_to_canvas_scale,
      };
      if (view.pointers.size > 1 && before.spread > 0) zoom_view_at(view, after.centroid, after.spread / before.spread);
      draw_view(state, view);
      return;
    }
    view.pointers.set(e.pointerId, canvas_point);
    const mark_editing = state.mark_editing!; // a mark or midline drag only starts with one
    if (!drag.has_moved) {
      mark_editing.on_drag_started();
      drag.has_moved = true;
    }
    if (drag.kind === "mark") drag_mark_to(state, view, drag, canvas_point);
    else state.scene().landmarks_file.front_midline_column_pixels = image_pixel_at_canvas_point(view, canvas_point).x;
    mark_editing.on_dragged();
  });

  const pointer_gone = (e: PointerEvent): void => {
    if (!view.pointers.delete(e.pointerId)) return;
    if (view.pointers.size === 0) end_drag();
  };
  canvas.addEventListener("pointerup", pointer_gone);
  canvas.addEventListener("pointercancel", pointer_gone);
  canvas.addEventListener("pointerleave", () => {
    if (view.hovered_landmark_index === null) return;
    view.hovered_landmark_index = null;
    draw_overlay_layer(state, view);
  });
  canvas.addEventListener("wheel", (e) => {
    e.preventDefault();
    zoom_view_at(view, canvas_point_of(e), Math.exp(-e.deltaY * 0.0015));
    draw_view(state, view);
  }, { passive: false });
}

// The controls are read from the page by id: `plate_opacity`, `plate_surfaces_visible`,
// `plate_skull_visible`, and `plate_landmarks_visible` when the page has one (a page whose
// pairs all hide the marks needs none). `plate_missing_note` is shown when the plate image
// is not there. `mark_editing` is null on a page with no editable pair.
export function start_loomis_girl_plate_views(pairs: PlateViewPair[], scene: () => PlateViewsScene, mark_editing: PlateMarkEditing | null): PlateViews {
  const opacity_slider = document.getElementById("plate_opacity") as HTMLInputElement;
  const surfaces_checkbox = document.getElementById("plate_surfaces_visible") as HTMLInputElement;
  const skull_checkbox = document.getElementById("plate_skull_visible") as HTMLInputElement;
  const landmarks_checkbox = document.getElementById("plate_landmarks_visible") as HTMLInputElement | null;
  const state: PlateViewsState = {
    views: [],
    scene,
    mark_editing,
    plate_image: null,
    plate_opacity: Number(opacity_slider.value),
    surfaces_visible: surfaces_checkbox.checked,
    skull_visible: skull_checkbox.checked,
    landmarks_visible: landmarks_checkbox === null || landmarks_checkbox.checked,
  };
  const make_view = (container_id: string, pair: PlateViewPair, region: PlateRegion, is_front_view: boolean): PlateView => {
    const container = document.getElementById(container_id)!;
    const add_canvas = (): HTMLCanvasElement => container.appendChild(document.createElement("canvas"));
    const plate_canvas = add_canvas();
    const mesh_canvas = add_canvas();
    const overlay_canvas = add_canvas();
    const mesh_gl = mesh_canvas.getContext("webgl");
    if (mesh_gl === null) console.error(`plate views: no WebGL, view '${container_id}' shows no surfaces or strokes`);
    return {
      container, plate_canvas, mesh_canvas, mesh_gl, overlay_canvas,
      head_meshes: mesh_gl === null ? null : create_head_meshes(mesh_gl),
      pair, region, is_front_view,
      image_pixels_to_canvas_scale: 1,
      image_pixel_at_canvas_origin: { x: 0, y: 0 },
      hovered_landmark_index: null,
      pointers: new Map(),
      drag: null,
      last_empty_tap_time_ms: 0,
    };
  };
  for (const pair of pairs) {
    state.views.push(make_view(pair.front_view_id, pair, FRONT_REGION, true));
    state.views.push(make_view(pair.profile_view_id, pair, PROFILE_REGION, false));
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
  landmarks_checkbox?.addEventListener("change", () => {
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
  return { draw: () => draw_all_views(state) };
}
