// Plate views of the Loomis school girl plate: the plate with a document drawn over it, a
// front view and a profile view per pair. Used by the pages fit-head-to-loomis-girl (the
// fitted head with the marks to drag, and the template before the fit) and
// fit-hairline-to-loomis-girl (the fitted head with the hairline highlighted, no marks).
// Both views are orthographic, so a world point maps to an image pixel by a scale and an
// offset (loomis_girl_plate.ts).
//
// The same views without the plate: start_plate_views with a layout and an adapter of the
// page's own, used by fit-head-to-native-head (no plate image; a background document, the
// native head, drawn in blue strokes with the fitted head; marks at world positions; no
// midline). start_loomis_girl_plate_views is that call with the plate's layout and the
// adapter of the plate's landmark file.
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
//
// Two gestures only a page's adapter and editing turn on (fit-head-to-native-head):
//  - adding: while the page has "add landmark" armed, a ring sits on the point of the shown
//    document's skin lines that a tap would take, and the tap makes it a new landmark.
//  - pinning: a mark dragged and let go within the layout's pin distance of a vertex of the
//    background document is pinned to that vertex (Alt held: never); a ring shows that
//    vertex during the drag, and a pinned landmark is drawn as a square.
import { OrbitCamera, camera_distance_for_world_units_per_pixel, camera_orthographic_view_projection } from "./camera";
import { TabletDocument, bezier_point, stroke_control_points, vertex_position } from "./document";
import { V2, V3, v3 } from "./math";
import { stroke_drawn_polyline } from "./patch";
import { Rgb } from "./vertex_sink";
import { HeadMeshes, create_head_meshes, draw_head_meshes, update_head_meshes } from "./head_meshes";
import {
  FRONT_REGION, FitLandmark, FitLandmarksFile, IMAGE_HEIGHT_PIXELS, IMAGE_WIDTH_PIXELS, PIXELS_PER_WORLD_UNIT, PROFILE_PORION_X_PIXELS,
  PROFILE_REGION, PlateRegion, TemplatePoint, WORLD_Y_ZERO_ROW_PIXELS, front_pixel_from_world,
  plate_view_transform_showing_region, plate_view_transform_zoomed_at, profile_pixel_from_world, template_point_world_position,
  template_position_is_on_midline,
} from "./loomis_girl_plate";

const PLATE_IMAGE_URL = "/loomis-girl-front-and-profile.webp";
const MISSING_TEMPLATE_POINT_STYLE = "#c0392b";
const MIDLINE_STYLE = "#7a4bd6";
const ON_SURFACE_STROKE_HIGHLIGHT_STYLE = "#ffa94d";
const HAIRLINE_PLANAR_STROKE_SAMPLES = 16; // segments drawn per hairline stroke that is a plain cubic
const SURFACE_OPACITY = 0.35; // the plate must stay readable under the surfaces
const GRAB_RADIUS_PIXELS = 14;
// The ring on the point "add" would take, and on the vertex a drag would pin to.
const PICK_RING_STYLE = "#ffd166";
const PICK_RING_RADIUS_PIXELS = 7;
// Adding: how near the pointer a skin line must pass, in canvas CSS pixels, and how many
// points along each stroke can be taken.
const ADD_PICK_RADIUS_PIXELS = 10;
const ADD_PICK_SAMPLES_PER_STROKE = 40;
// Where lines cross on the picture, the points this many pixels further from the pointer
// than the nearest one still count as under it; of those, the one nearest the viewer is
// taken (world units of depth: closer than this to the front-most counts as just as near).
const ADD_PICK_NEAR_TIE_PIXELS = 3;
const ADD_PICK_SAME_DEPTH_WORLD_UNITS = 0.03;
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
  // "Add landmark" on the views; null = the page adds landmarks elsewhere (the head model pane).
  landmark_adding_on_views: PlateLandmarkAdding | null;
};

// While `is_armed`, a tap on a skin line of an editable pair's shown document picks that
// template point (the shown document keeps the template's vertex and stroke ids).
export type PlateLandmarkAdding = {
  is_armed: () => boolean;
  on_template_point_picked: (template_point: TemplatePoint) => void;
};

export type PlateViews = { draw: () => void };

// What stays the same for all views of a page. The "image pixels" of a view are the
// plate's pixels, or with no plate any pixels the page's adapter maps the world to.
export type PlateViewsLayout = {
  pixels_per_world_unit: number;
  front_region: PlateRegion; // what a view shows when it is fitted (start, double tap)
  profile_region: PlateRegion;
  plate_image: { url: string; width_pixels: number; height_pixels: number } | null; // null = only the paper
  background_stroke_color: Rgb | null; // the strokes of the adapter's background document; null = the page has none
  // The colors that must read on the view's background.
  paper_style: string; // the fill under the plate image
  label_outline_style: string; // the outline behind the landmark labels
  skin_stroke_color: Rgb; // the shown document's strokes
  skull_stroke_color: Rgb;
  landmark_style: string;
  selected_landmark_style: string;
  // A mark let go this near (canvas CSS pixels) a vertex of the background document is
  // pinned to it, when the adapter can pin; 0 = no pinning.
  landmark_pin_distance_pixels: number;
};

// A mark of a landmark in a view, at an image pixel. side_sign is -1 for the mirrored mark
// of a landmark off the midline in the front view, else 1.
export type PlateMark = { landmark_index: number; side_sign: number; image_pixel: V2 };

// The landmarks as the views see them, asked from the adapter on every draw.
export type PlateMarksScene = {
  template_document: TabletDocument; // says which landmarks are midline landmarks
  landmarks: { name: string; template_point: TemplatePoint }[];
  selected_landmark_index: number | null;
  landmark_names_visible: boolean;
  background_document: ShownDocument | null; // drawn in every view in the layout's background stroke color
};

// The page's side of the views: where its marks are, how a drag moves them, and how the
// world maps to image pixels. A drag changes the page's landmarks in place.
export type PlateViewsAdapter = {
  scene: () => PlateMarksScene;
  image_pixel_from_world: (is_front_view: boolean, position: V3) => V2;
  // The world point the orthographic camera looks at to show `image_pixel` at the canvas centre.
  camera_pivot_at_image_pixel: (is_front_view: boolean, image_pixel: V2) => V3;
  marks_of_landmark: (is_front_view: boolean, landmark_index: number) => PlateMark[];
  mark_is_read_from_view: (is_front_view: boolean, landmark_index: number) => boolean; // a cross; else a circle
  move_mark_to: (is_front_view: boolean, mark: PlateMark) => void; // mark.image_pixel is where the pointer put it
  // The front view's midline, drawn and dragged by its handle where marks are editable; null = none.
  front_midline: { column_pixels: () => number; move_to_column: (column_pixels: number) => void } | null;
  // Landmarks pinned to vertices of the scene's background document; null = none can be.
  background_vertex_pinning: BackgroundVertexPinning | null;
};

export type BackgroundVertexPinning = {
  landmark_is_pinned: (landmark_index: number) => boolean; // drawn as a square
  // Where the landmark's target is in the world now (after move_mark_to during a drag):
  // of the vertices near the mark on the picture, the one nearest this is taken.
  landmark_target_position: (landmark_index: number) => V3 | null;
  pin_landmark_to_vertex: (landmark_index: number, background_vertex_id: number) => void;
};

// The vertex of the background document a dragged mark would be pinned to if let go now.
type PinCandidate = { background_vertex_id: number; image_pixel: V2 };

// The template point a tap would make a new landmark of, while adding is armed.
type AddCandidate = { template_point: TemplatePoint; image_pixel: V2 };

type PlateDrag =
  | { kind: "mark"; landmark_index: number; side_sign: number; pointer_to_mark_offset: V2; has_moved: boolean; pin_candidate: PinCandidate | null }
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
  background_meshes: HeadMeshes | null; // null = no WebGL, or the layout has no background document
  overlay_canvas: HTMLCanvasElement; // on top, takes the gestures
  pair: PlateViewPair;
  region: PlateRegion;
  is_front_view: boolean;
  image_pixels_to_canvas_scale: number;
  image_pixel_at_canvas_origin: V2;
  // False while the view has not been fitted to a container of non-zero size (one inside a
  // closed <details> has none): it is fitted on its first draw that has a size.
  is_fitted_to_container: boolean;
  hovered_landmark_index: number | null;
  add_candidate: AddCandidate | null; // under the pointer while adding is armed
  pointers: Map<number, V2>; // pointerId -> last position, canvas CSS pixels
  drag: PlateDrag | null;
  last_empty_tap_time_ms: number;
};

type PlateViewsState = {
  views: PlateView[];
  layout: PlateViewsLayout;
  adapter: PlateViewsAdapter;
  mark_editing: PlateMarkEditing | null;
  plate_image: HTMLImageElement | null; // null = not loaded (the scan is not in the repo), or the layout has none
  plate_opacity: number;
  surfaces_visible: boolean;
  skull_visible: boolean;
  landmarks_visible: boolean;
};

const LOOMIS_GIRL_PLATE_LAYOUT: PlateViewsLayout = {
  pixels_per_world_unit: PIXELS_PER_WORLD_UNIT,
  front_region: FRONT_REGION,
  profile_region: PROFILE_REGION,
  plate_image: { url: PLATE_IMAGE_URL, width_pixels: IMAGE_WIDTH_PIXELS, height_pixels: IMAGE_HEIGHT_PIXELS },
  background_stroke_color: null,
  paper_style: "#fff",
  label_outline_style: "#ffffff",
  skin_stroke_color: { r: 0.91, g: 0.15, b: 0.16 },
  skull_stroke_color: { r: 0.16, g: 0.38, b: 0.91 },
  landmark_style: "#0a9a3a",
  selected_landmark_style: "#d6007f",
  landmark_pin_distance_pixels: 0,
};

// A container of zero size leaves the view unfitted (a zero scale would divide by zero).
function fit_view_to_region(view: PlateView): void {
  const width = view.container.clientWidth;
  const height = view.container.clientHeight;
  view.is_fitted_to_container = width > 0 && height > 0;
  if (!view.is_fitted_to_container) return;
  const transform = plate_view_transform_showing_region(width, height, view.region);
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

// The orthographic camera that shows the world as the plate does (front_pixel_from_world,
// profile_pixel_from_world): looking down -z for the front view, down -x for the profile
// (+z to the viewer's left), pivot at the world point under the canvas centre.
function plate_view_camera(state: PlateViewsState, view: PlateView): OrbitCamera {
  const centre = image_pixel_at_canvas_point(view, { x: view.container.clientWidth / 2, y: view.container.clientHeight / 2 });
  return {
    pivot: state.adapter.camera_pivot_at_image_pixel(view.is_front_view, centre),
    yaw: view.is_front_view ? 0 : Math.PI / 2,
    pitch: 0,
    distance: camera_distance_for_world_units_per_pixel(1 / (view.image_pixels_to_canvas_scale * state.layout.pixels_per_world_unit), view.container.clientHeight),
    is_orthographic: true,
  };
}

// ---- the adapter of the Loomis plate's landmark file ---------------------------------

// A landmark on the midline has one mark in the front view; any other has one on each side
// of the face. The template decides; a landmark whose template point is gone keeps what its
// mark says.
function landmark_is_on_midline(scene: PlateViewsScene, landmark: FitLandmark): boolean {
  const template_position = template_point_world_position(scene.template_document, landmark.template_point);
  if (template_position === null) return landmark.front_half_width_pixels === 0;
  return template_position_is_on_midline(template_position);
}

// The marks of one landmark in a view: two in the front view when the landmark is paired.
function marks_of_loomis_landmark(is_front_view: boolean, scene: PlateViewsScene, landmark: FitLandmark, landmark_index: number): PlateMark[] {
  if (!is_front_view) return [{ landmark_index, side_sign: 1, image_pixel: { x: landmark.profile_column_pixels, y: landmark.row_pixels } }];
  const midline = scene.landmarks_file.front_midline_column_pixels;
  if (landmark_is_on_midline(scene, landmark)) return [{ landmark_index, side_sign: 1, image_pixel: { x: midline, y: landmark.row_pixels } }];
  return [1, -1].map((side_sign) => ({ landmark_index, side_sign, image_pixel: { x: midline + side_sign * landmark.front_half_width_pixels, y: landmark.row_pixels } }));
}

// Moves the dragged mark to the pointer. The view a mark is dragged in becomes a view the
// landmark is read from.
function move_loomis_mark_to(is_front_view: boolean, scene: PlateViewsScene, mark: PlateMark): void {
  const landmark = scene.landmarks_file.landmarks[mark.landmark_index];
  const image_pixel = mark.image_pixel;
  landmark.row_pixels = image_pixel.y;
  if (is_front_view) {
    if (!landmark_is_on_midline(scene, landmark)) landmark.front_half_width_pixels = Math.max(0, (image_pixel.x - scene.landmarks_file.front_midline_column_pixels) * mark.side_sign);
    landmark.read_from_front = true;
  } else {
    landmark.profile_column_pixels = image_pixel.x;
    landmark.read_from_profile = true;
  }
}

function loomis_girl_plate_adapter(scene: () => PlateViewsScene): PlateViewsAdapter {
  return {
    scene: () => {
      const plate_scene = scene();
      return {
        template_document: plate_scene.template_document,
        landmarks: plate_scene.landmarks_file.landmarks,
        selected_landmark_index: plate_scene.selected_landmark_index,
        landmark_names_visible: plate_scene.landmark_names_visible,
        background_document: null,
      };
    },
    image_pixel_from_world: (is_front_view, position) => (is_front_view ? front_pixel_from_world(position, scene().landmarks_file.front_midline_column_pixels) : profile_pixel_from_world(position)),
    camera_pivot_at_image_pixel: (is_front_view, centre) => {
      const y = (WORLD_Y_ZERO_ROW_PIXELS - centre.y) / PIXELS_PER_WORLD_UNIT;
      if (is_front_view) return v3((centre.x - scene().landmarks_file.front_midline_column_pixels) / PIXELS_PER_WORLD_UNIT, y, 0);
      return v3(0, y, (PROFILE_PORION_X_PIXELS - centre.x) / PIXELS_PER_WORLD_UNIT);
    },
    marks_of_landmark: (is_front_view, landmark_index) => {
      const plate_scene = scene();
      return marks_of_loomis_landmark(is_front_view, plate_scene, plate_scene.landmarks_file.landmarks[landmark_index], landmark_index);
    },
    mark_is_read_from_view: (is_front_view, landmark_index) => {
      const landmark = scene().landmarks_file.landmarks[landmark_index];
      return is_front_view ? landmark.read_from_front : landmark.read_from_profile;
    },
    move_mark_to: (is_front_view, mark) => move_loomis_mark_to(is_front_view, scene(), mark),
    front_midline: {
      column_pixels: () => scene().landmarks_file.front_midline_column_pixels,
      move_to_column: (column_pixels) => {
        scene().landmarks_file.front_midline_column_pixels = column_pixels;
      },
    },
    background_vertex_pinning: null, // a plate mark is read off the picture: there is no background document
  };
}

// ---- the views -----------------------------------------------------------------------

function marks_are_drawn(state: PlateViewsState, view: PlateView): boolean {
  return view.pair.marks !== "hidden" && state.landmarks_visible;
}

function marks_can_be_dragged(state: PlateViewsState, view: PlateView): boolean {
  return view.pair.marks === "editable" && marks_are_drawn(state, view) && state.mark_editing !== null && state.mark_editing.is_editable();
}

function mark_under_pointer(state: PlateViewsState, view: PlateView, canvas_point: V2): PlateMark | null {
  if (!marks_are_drawn(state, view)) return null;
  const scene = state.adapter.scene();
  let nearest: PlateMark | null = null;
  let nearest_distance = GRAB_RADIUS_PIXELS;
  scene.landmarks.forEach((_landmark, landmark_index) => {
    for (const mark of state.adapter.marks_of_landmark(view.is_front_view, landmark_index)) {
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
  const front_midline = state.adapter.front_midline;
  if (front_midline === null) return false;
  if (!view.is_front_view || !marks_can_be_dragged(state, view)) return false;
  const handle_x = canvas_point_at_image_pixel(view, { x: front_midline.column_pixels(), y: 0 }).x;
  return Math.hypot(handle_x - canvas_point.x, MIDLINE_HANDLE_CENTER_Y_PIXELS - canvas_point.y) <= GRAB_RADIUS_PIXELS;
}

// Moves the dragged mark to the pointer.
function drag_mark_to(state: PlateViewsState, view: PlateView, drag: PlateDrag & { kind: "mark" }, canvas_point: V2): void {
  const image_pixel = image_pixel_at_canvas_point(view, { x: canvas_point.x + drag.pointer_to_mark_offset.x, y: canvas_point.y + drag.pointer_to_mark_offset.y });
  state.adapter.move_mark_to(view.is_front_view, { landmark_index: drag.landmark_index, side_sign: drag.side_sign, image_pixel: image_pixel });
}

// How far a world point is toward the viewer of the view: the front view looks down -z, the
// profile down -x.
function depth_toward_viewer(view: PlateView, position: V3): number {
  return view.is_front_view ? position.z : position.x;
}

// The point of the shown document's skin lines a tap at `canvas_point` would make a new
// landmark of: a stroke's end vertex or a point along it, within ADD_PICK_RADIUS_PIXELS.
// Where lines cross on the picture (the orthographic front view puts the back of the head
// over the face), the near ties are decided by depth: the point nearest the viewer, then a
// vertex over a point along a stroke, then the nearest on the picture. Null = none near.
function template_point_to_add_under_pointer(state: PlateViewsState, view: PlateView, canvas_point: V2): AddCandidate | null {
  const shown = view.pair.shown();
  if (shown === null) return null;
  const tablet_document = shown.tablet_document;
  type Candidate = { template_point: TemplatePoint; image_pixel: V2; distance_pixels: number; depth: number; is_vertex: boolean };
  const candidates: Candidate[] = [];
  // The document holds one side of the head and is drawn mirrored (draw_head_layer); in
  // profile the mirrored side lands on the same pixels with the same template point.
  const side_signs = view.is_front_view ? [1, -1] : [1];
  const consider = (template_point: TemplatePoint, position: V3, is_vertex: boolean): void => {
    for (const side_sign of side_signs) {
      const image_pixel = state.adapter.image_pixel_from_world(view.is_front_view, v3(side_sign * position.x, position.y, position.z));
      const point = canvas_point_at_image_pixel(view, image_pixel);
      const distance_pixels = Math.hypot(point.x - canvas_point.x, point.y - canvas_point.y);
      if (distance_pixels > ADD_PICK_RADIUS_PIXELS) continue;
      candidates.push({ template_point, image_pixel, distance_pixels, depth: depth_toward_viewer(view, v3(side_sign * position.x, position.y, position.z)), is_vertex });
    }
  };
  for (const stroke of tablet_document.strokes) {
    if (stroke.layer !== "skin") continue;
    if (stroke.is_hidden === true) continue;
    const control_points = stroke_control_points(stroke, tablet_document);
    consider({ vertex: stroke.p0_vertex }, control_points.p0, true);
    consider({ vertex: stroke.p3_vertex }, control_points.p3, true);
    for (let k = 1; k < ADD_PICK_SAMPLES_PER_STROKE; k++) {
      const position_along_stroke = k / ADD_PICK_SAMPLES_PER_STROKE;
      consider({ stroke: stroke.id, position_along_stroke }, bezier_point(control_points, position_along_stroke), false);
    }
  }
  if (candidates.length === 0) return null;
  const nearest_distance_pixels = Math.min(...candidates.map((candidate) => candidate.distance_pixels));
  const under_pointer = candidates.filter((candidate) => candidate.distance_pixels <= nearest_distance_pixels + ADD_PICK_NEAR_TIE_PIXELS);
  const front_most_depth = Math.max(...under_pointer.map((candidate) => candidate.depth));
  const nearest_viewer = under_pointer.filter((candidate) => candidate.depth >= front_most_depth - ADD_PICK_SAME_DEPTH_WORLD_UNITS);
  const vertices = nearest_viewer.filter((candidate) => candidate.is_vertex);
  const choices = vertices.length > 0 ? vertices : nearest_viewer;
  let chosen = choices[0];
  for (const candidate of choices) if (candidate.distance_pixels < chosen.distance_pixels) chosen = candidate;
  return { template_point: chosen.template_point, image_pixel: chosen.image_pixel };
}

// The vertex of the background document the dragged mark would be pinned to if let go now:
// an end vertex of a drawn background stroke within the layout's pin distance of the mark
// on the picture. Of those, the one nearest in the world to the landmark's target (so a
// vertex at the back of the head, under the face in front, is not taken). A midline
// landmark is pinned only to a vertex on the midline. Null = no pin (none near, Alt held,
// or the page cannot pin).
function background_vertex_to_pin_near_mark(state: PlateViewsState, view: PlateView, drag: PlateDrag & { kind: "mark" }, alt_key_is_held: boolean): PinCandidate | null {
  const pinning = state.adapter.background_vertex_pinning;
  const pin_distance_pixels = state.layout.landmark_pin_distance_pixels;
  if (pinning === null || pin_distance_pixels <= 0 || alt_key_is_held) return null;
  const scene = state.adapter.scene();
  const background = scene.background_document;
  if (background === null) return null;
  const mark = state.adapter.marks_of_landmark(view.is_front_view, drag.landmark_index).find((m) => m.side_sign === drag.side_sign);
  const target_position = pinning.landmark_target_position(drag.landmark_index);
  if (mark === undefined || target_position === null) return null;
  const mark_point = canvas_point_at_image_pixel(view, mark.image_pixel);
  const template_position = template_point_world_position(scene.template_document, scene.landmarks[drag.landmark_index].template_point);
  const landmark_is_on_midline = template_position !== null && template_position_is_on_midline(template_position);
  const background_document = background.tablet_document;
  const vertex_ids = new Set<number>();
  for (const stroke of background_document.strokes) {
    if (stroke.is_hidden === true) continue;
    if (stroke.layer === "skull" && !state.skull_visible) continue;
    vertex_ids.add(stroke.p0_vertex);
    vertex_ids.add(stroke.p3_vertex);
  }
  let chosen: PinCandidate | null = null;
  let chosen_world_distance = Infinity;
  for (const vertex_id of vertex_ids) {
    const position = vertex_position(background_document, vertex_id);
    // A target sits on the drawn side (x >= 0), whether or not the background document
    // holds both sides: a vertex at -x would make a pair that crosses the midline.
    if (position.x < -1e-3) continue;
    if (landmark_is_on_midline && !template_position_is_on_midline(position)) continue;
    // The mirrored mark (side_sign -1) of the front view shows the target at -x.
    const shown_position = view.is_front_view ? v3(drag.side_sign * position.x, position.y, position.z) : position;
    const image_pixel = state.adapter.image_pixel_from_world(view.is_front_view, shown_position);
    const point = canvas_point_at_image_pixel(view, image_pixel);
    if (Math.hypot(point.x - mark_point.x, point.y - mark_point.y) > pin_distance_pixels) continue;
    const world_distance = Math.hypot(position.x - target_position.x, position.y - target_position.y, position.z - target_position.z);
    if (world_distance < chosen_world_distance) {
      chosen = { background_vertex_id: vertex_id, image_pixel };
      chosen_world_distance = world_distance;
    }
  }
  return chosen;
}

// Draws a ring of PICK_RING_RADIUS_PIXELS at an image pixel, in CSS pixel coordinates.
function draw_pick_ring(context: CanvasRenderingContext2D, view: PlateView, image_pixel: V2): void {
  const point = canvas_point_at_image_pixel(view, image_pixel);
  context.strokeStyle = PICK_RING_STYLE;
  context.lineWidth = 2;
  context.beginPath();
  context.arc(point.x, point.y, PICK_RING_RADIUS_PIXELS, 0, 2 * Math.PI);
  context.stroke();
}

function adding_is_armed_in_view(state: PlateViewsState, view: PlateView): boolean {
  if (view.pair.marks !== "editable" || state.mark_editing === null || !state.mark_editing.is_editable()) return false;
  const adding = state.mark_editing.landmark_adding_on_views;
  return adding !== null && adding.is_armed();
}

function draw_plate_layer(state: PlateViewsState, view: PlateView): void {
  const { width, height, device_pixel_ratio } = size_canvas_to_container(view, view.plate_canvas);
  const context = view.plate_canvas.getContext("2d")!;
  context.setTransform(1, 0, 0, 1, 0, 0);
  context.fillStyle = state.layout.paper_style;
  context.fillRect(0, 0, width, height);
  const layout_image = state.layout.plate_image;
  if (state.plate_image === null || layout_image === null) return;
  const scale = view.image_pixels_to_canvas_scale * device_pixel_ratio;
  context.setTransform(scale, 0, 0, scale, -view.image_pixel_at_canvas_origin.x * scale, -view.image_pixel_at_canvas_origin.y * scale);
  context.globalAlpha = state.plate_opacity;
  // The landmark pixels were read on the plate at this size, whatever the file's own.
  context.drawImage(state.plate_image, 0, 0, layout_image.width_pixels, layout_image.height_pixels);
}

function draw_head_layer(state: PlateViewsState, view: PlateView): void {
  const gl = view.mesh_gl;
  if (gl === null || view.head_meshes === null) return;
  const { width, height } = size_canvas_to_container(view, view.mesh_canvas);
  gl.viewport(0, 0, width, height);
  gl.clearColor(0, 0, 0, 0);
  gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
  if (height === 0) return;
  const camera = plate_view_camera(state, view);
  const view_projection = camera_orthographic_view_projection(camera, width / height);
  const shown = view.pair.shown();
  if (shown !== null) {
    update_head_meshes(view.head_meshes, shown.tablet_document, shown.revision, camera, {
      skull_visible: state.skull_visible,
      surfaces_visible: state.surfaces_visible,
      strokes_visible: true,
      mirrored: true, // the document holds one side of the head
      surface_opacity: SURFACE_OPACITY,
      skin_stroke_color: state.layout.skin_stroke_color,
      skull_stroke_color: state.layout.skull_stroke_color,
    });
    draw_head_meshes(view.head_meshes, view_projection, camera);
  }
  // The background document: its strokes only, over the shown document (the depth is
  // cleared), so the shown document's surfaces hide none of them.
  const background = state.adapter.scene().background_document;
  const background_stroke_color = state.layout.background_stroke_color;
  if (background === null || background_stroke_color === null || view.background_meshes === null) return;
  gl.clear(gl.DEPTH_BUFFER_BIT);
  update_head_meshes(view.background_meshes, background.tablet_document, background.revision, camera, {
    skull_visible: state.skull_visible,
    surfaces_visible: false,
    strokes_visible: true,
    mirrored: background.tablet_document.is_mirrored_across_midline === true,
    surface_opacity: SURFACE_OPACITY,
    skin_stroke_color: background_stroke_color,
    skull_stroke_color: background_stroke_color,
  });
  draw_head_meshes(view.background_meshes, view_projection, camera);
}

function draw_overlay_layer(state: PlateViewsState, view: PlateView): void {
  const adapter = state.adapter;
  const scene = adapter.scene();
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
      if (stroke.is_hidden === true) continue;
      const is_hairline_stroke = stroke.on_surface === true || surface_pinned_vertices.has(stroke.p0_vertex) || surface_pinned_vertices.has(stroke.p3_vertex);
      if (!is_hairline_stroke) continue;
      const samples = stroke_drawn_polyline(stroke, shown.tablet_document, HAIRLINE_PLANAR_STROKE_SAMPLES);
      for (const side_sign of view.is_front_view ? [1, -1] : [1]) {
        context.beginPath();
        samples.forEach((sample, k) => {
          const pixel = adapter.image_pixel_from_world(view.is_front_view, v3(side_sign * sample.x, sample.y, sample.z));
          if (k === 0) context.moveTo(pixel.x, pixel.y);
          else context.lineTo(pixel.x, pixel.y);
        });
        context.stroke();
      }
    }
  }
  // The point a tap would add, while adding is armed (drawn in CSS pixels, then back).
  if (view.add_candidate !== null && adding_is_armed_in_view(state, view)) {
    context.setTransform(device_pixel_ratio, 0, 0, device_pixel_ratio, 0, 0);
    draw_pick_ring(context, view, view.add_candidate.image_pixel);
    context.setTransform(scale, 0, 0, scale, -view.image_pixel_at_canvas_origin.x * scale, -view.image_pixel_at_canvas_origin.y * scale);
  }
  if (!marks_are_drawn(state, view)) return;

  const is_editable = view.pair.marks === "editable";
  const pinning = adapter.background_vertex_pinning;
  if (view.is_front_view && is_editable && adapter.front_midline !== null) {
    const midline = adapter.front_midline.column_pixels();
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
  scene.landmarks.forEach((landmark, landmark_index) => {
    const is_selected = is_editable && landmark_index === scene.selected_landmark_index;
    const template_position = template_point_world_position(scene.template_document, landmark.template_point);
    // An empty template is one that is not loaded yet, not one with every point missing.
    const is_missing_template_point = template_position === null && scene.template_document.vertices.length > 0;
    const is_read_from_this_view = adapter.mark_is_read_from_view(view.is_front_view, landmark_index);
    const style = is_missing_template_point ? MISSING_TEMPLATE_POINT_STYLE : is_selected ? state.layout.selected_landmark_style : state.layout.landmark_style;
    const drawn_position = shown === null ? null : template_point_world_position(shown.tablet_document, landmark.template_point);
    const marks = adapter.marks_of_landmark(view.is_front_view, landmark_index);
    for (const mark of marks) {
      const { x, y } = mark.image_pixel;
      context.strokeStyle = style;
      context.fillStyle = style;
      context.globalAlpha = is_read_from_this_view || is_selected ? 1 : 0.6;
      // From the mark to where the shown document has the landmark's point.
      if (drawn_position !== null) {
        const drawn_pixel = adapter.image_pixel_from_world(view.is_front_view, v3(mark.side_sign * Math.abs(drawn_position.x), drawn_position.y, drawn_position.z));
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
      if (pinning !== null && pinning.landmark_is_pinned(landmark_index)) {
        // Pinned to a vertex of the background document: a square.
        const half_side = arm * 0.7;
        context.rect(x - half_side, y - half_side, 2 * half_side, 2 * half_side);
      } else if (is_read_from_this_view) {
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
    // A landmark the adapter cannot place (its target is gone) has no mark to label.
    if (marks.length === 0) return;
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
    context.strokeStyle = state.layout.label_outline_style;
    context.strokeText(label.text, point.x + 9, point.y - 7);
    context.fillStyle = label.style;
    context.fillText(label.text, point.x + 9, point.y - 7);
  }
  // The vertex the dragged mark would be pinned to if let go now.
  if (view.drag !== null && view.drag.kind === "mark" && view.drag.pin_candidate !== null) {
    draw_pick_ring(context, view, view.drag.pin_candidate.image_pixel);
  }
  if (view.is_front_view && is_editable && adapter.front_midline !== null) {
    const handle_x = canvas_point_at_image_pixel(view, { x: adapter.front_midline.column_pixels(), y: 0 }).x;
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
  if (!view.is_fitted_to_container) {
    fit_view_to_region(view);
    if (!view.is_fitted_to_container) return; // no size yet (a closed <details>): drawn when it has one
  }
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
      // A tap while adding is armed: the point under the ring becomes a new landmark (a tap
      // on no skin line adds nothing, and adding stays armed).
      const adding = state.mark_editing?.landmark_adding_on_views ?? null;
      if (adding !== null && adding_is_armed_in_view(state, view)) {
        const add_candidate = view.add_candidate;
        view.add_candidate = null;
        if (add_candidate !== null) adding.on_template_point_picked(add_candidate.template_point);
        draw_overlay_layer(state, view);
        return;
      }
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
    // A mark let go near a vertex of the background document is pinned to it (before the
    // save below, so the pin is saved).
    const pinning = state.adapter.background_vertex_pinning;
    if (drag.kind === "mark" && drag.has_moved && drag.pin_candidate !== null && pinning !== null) {
      pinning.pin_landmark_to_vertex(drag.landmark_index, drag.pin_candidate.background_vertex_id);
      state.mark_editing?.on_dragged();
    }
    // An edit drag (mark or midline) ends: what it changed is saved.
    if (drag.has_moved) state.mark_editing?.on_drag_ended();
    if (drag.kind === "mark") draw_overlay_layer(state, view); // the pin ring goes
  };

  canvas.addEventListener("pointerdown", (e) => {
    if (e.button !== 0 && e.pointerType === "mouse") return;
    e.preventDefault();
    try { canvas.setPointerCapture(e.pointerId); } catch { /* throws for synthetic events */ }
    const canvas_point = canvas_point_of(e);
    view.pointers.set(e.pointerId, canvas_point);
    if (view.pointers.size > 1) {
      // A second finger: the gesture is a pinch, whatever the first finger had grabbed (and
      // the first finger's touch adds nothing).
      view.add_candidate = null;
      end_drag();
      view.drag = { kind: "pan", travel_pixels: Infinity };
      return;
    }
    // While adding is armed nothing is grabbed: a tap adds, a drag still moves the picture.
    if (adding_is_armed_in_view(state, view)) {
      view.add_candidate = template_point_to_add_under_pointer(state, view, canvas_point); // a touch has no hover before it
      view.drag = { kind: "pan", travel_pixels: 0 };
      return;
    }
    const mark = marks_can_be_dragged(state, view) ? mark_under_pointer(state, view, canvas_point) : null;
    if (mark !== null) {
      const mark_point = canvas_point_at_image_pixel(view, mark.image_pixel);
      view.drag = { kind: "mark", landmark_index: mark.landmark_index, side_sign: mark.side_sign, pointer_to_mark_offset: { x: mark_point.x - canvas_point.x, y: mark_point.y - canvas_point.y }, has_moved: false, pin_candidate: null };
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
      // No button down, adding armed: the ring shows what a tap here would add.
      if (adding_is_armed_in_view(state, view)) {
        view.add_candidate = template_point_to_add_under_pointer(state, view, canvas_point);
        canvas.style.cursor = "crosshair";
        draw_overlay_layer(state, view);
        return;
      }
      if (view.add_candidate !== null) {
        view.add_candidate = null; // adding was disarmed (Escape, the button)
        draw_overlay_layer(state, view);
      }
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
    if (drag.kind === "mark") {
      drag_mark_to(state, view, drag, canvas_point);
      drag.pin_candidate = background_vertex_to_pin_near_mark(state, view, drag, e.altKey);
    } else {
      state.adapter.front_midline!.move_to_column(image_pixel_at_canvas_point(view, canvas_point).x); // a midline drag only starts with a midline
    }
    mark_editing.on_dragged();
  });

  const pointer_gone = (e: PointerEvent): void => {
    if (!view.pointers.delete(e.pointerId)) return;
    // Alt pressed or let go after the last move still decides the pin.
    if (view.drag !== null && view.drag.kind === "mark" && view.drag.has_moved) {
      view.drag.pin_candidate = background_vertex_to_pin_near_mark(state, view, view.drag, e.altKey);
    }
    if (view.pointers.size === 0) end_drag();
  };
  canvas.addEventListener("pointerup", pointer_gone);
  canvas.addEventListener("pointercancel", pointer_gone);
  canvas.addEventListener("pointerleave", () => {
    if (view.hovered_landmark_index === null && view.add_candidate === null) return;
    view.hovered_landmark_index = null;
    view.add_candidate = null;
    draw_overlay_layer(state, view);
  });
  canvas.addEventListener("wheel", (e) => {
    // The wheel over a view zooms it (a trackpad pinch arrives as Ctrl + wheel); the page
    // is scrolled beside the views.
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
  return start_plate_views(LOOMIS_GIRL_PLATE_LAYOUT, pairs, loomis_girl_plate_adapter(scene), mark_editing);
}

// The same controls as start_loomis_girl_plate_views; a layout with no plate image needs
// neither `plate_opacity` nor `plate_missing_note`.
export function start_plate_views(layout: PlateViewsLayout, pairs: PlateViewPair[], adapter: PlateViewsAdapter, mark_editing: PlateMarkEditing | null): PlateViews {
  const opacity_slider = document.getElementById("plate_opacity") as HTMLInputElement | null;
  const surfaces_checkbox = document.getElementById("plate_surfaces_visible") as HTMLInputElement;
  const skull_checkbox = document.getElementById("plate_skull_visible") as HTMLInputElement;
  const landmarks_checkbox = document.getElementById("plate_landmarks_visible") as HTMLInputElement | null;
  const state: PlateViewsState = {
    views: [],
    layout,
    adapter,
    mark_editing,
    plate_image: null,
    plate_opacity: opacity_slider === null ? 1 : Number(opacity_slider.value),
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
    const has_background_meshes = mesh_gl !== null && layout.background_stroke_color !== null;
    return {
      container, plate_canvas, mesh_canvas, mesh_gl, overlay_canvas,
      head_meshes: mesh_gl === null ? null : create_head_meshes(mesh_gl),
      background_meshes: has_background_meshes ? create_head_meshes(mesh_gl) : null,
      pair, region, is_front_view,
      image_pixels_to_canvas_scale: 1,
      image_pixel_at_canvas_origin: { x: 0, y: 0 },
      is_fitted_to_container: false,
      hovered_landmark_index: null,
      add_candidate: null,
      pointers: new Map(),
      drag: null,
      last_empty_tap_time_ms: 0,
    };
  };
  for (const pair of pairs) {
    state.views.push(make_view(pair.front_view_id, pair, layout.front_region, true));
    state.views.push(make_view(pair.profile_view_id, pair, layout.profile_region, false));
  }
  for (const view of state.views) {
    fit_view_to_region(view);
    attach_view_gestures(state, view);
  }

  opacity_slider?.addEventListener("input", () => {
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

  const layout_image = layout.plate_image;
  if (layout_image !== null) {
    const plate_image = new Image();
    plate_image.onload = () => {
      state.plate_image = plate_image;
      draw_all_views(state);
    };
    plate_image.onerror = () => {
      console.error(`plate views: ${layout_image.url} not found (the scan is not in the repo, copy it into tablet/public/)`);
      document.getElementById("plate_missing_note")!.hidden = false;
    };
    plate_image.src = layout_image.url;
  }

  draw_all_views(state);
  return { draw: () => draw_all_views(state) };
}
