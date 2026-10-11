// The fit-hairline-to-loomis-man page: the template head (document `skull-zanatomy`)
// over Loomis's planes-head plates, front and profile, registered by three marks per
// plate (src/loomis_planes_man_plate.ts); the hairline is drawn on the plates and lifted
// onto the skin (hairline_lift.ts). Plan: notes/tasks/autodraw_draw_as_data/
// plan-hairline-reference-plate.md.
//
// A view is three canvases stacked in one container, as on fit-head-to-loomis-girl: the
// plate (2D), the head's surfaces and strokes (WebGL, head_meshes.ts) through an
// orthographic camera that reproduces the plate's registration, and the marks, hairline
// and lifted hits (2D) on top. The top canvas takes the gestures; what one pointer does
// depends on the mode (move, drag marks, draw hairline); two fingers always pinch-zoom.
//
// The document is read from the server and polled; the lift writes it back. The
// landmark file is read once and written on every change (this page is its only writer).
import "../../pages.css";
import { OrbitCamera, camera_distance_for_world_units_per_pixel, camera_orthographic_view_projection, default_camera } from "../../src/camera";
import { TabletDocument, VertexId, delete_stroke, empty_document, garbage_collect_vertices, unpin_vertices } from "../../src/document";
import { V2, V3 } from "../../src/math";
import { apply_document_state, list_documents_from_server, serialize_document_state } from "../../src/persistence";
import { Rgb } from "../../src/vertex_sink";
import { HeadMeshes, create_head_meshes, draw_head_meshes, update_head_meshes } from "../fit-head-to-loomis-girl/head_meshes";
import { PlateRegion, PlateViewTransform, plate_view_transform_showing_region, plate_view_transform_zoomed_at } from "../../src/loomis_girl_plate";
import {
  FRONT_PLATE, LoomisPlanesManLandmarksFile, PROFILE_PLATE, POINT_MARKS, PlateImage, PlateLandmarks, PlateRegistration,
  fetch_landmarks_file, named_vertex_world_position, pixel_from_world, plate_registration, save_landmarks_file, starting_landmarks_file, world_from_pixel,
} from "../../src/loomis_planes_man_plate";
import { LiftedHit, cast_hairline_samples, merge_hits_by_angle, write_hairline_strokes } from "./hairline_lift";

const PAGE_NAME = "fit-hairline-to-loomis-man";
const DOCUMENT_NAME = "skull-zanatomy";
const POLL_MILLISECONDS = 2000;
const SKIN_STROKE_COLOR: Rgb = { r: 0.91, g: 0.15, b: 0.16 };
const SKULL_STROKE_COLOR: Rgb = { r: 0.16, g: 0.38, b: 0.91 };
const MARK_COLOR = "#0a9a3a";
const HEAD_POINT_COLOR = "#ffffff";
const HAIRLINE_COLOR = "#ffa94d";
const LIFTED_HIT_COLOR = "#4dd4c0";
const SURFACE_OPACITY = 0.35;
const MARK_GRAB_RADIUS_CSS_PIXELS = 18;

type Mode = "move" | "landmarks" | "hairline";
type MarkName = "brow" | "nasion" | "nose_base" | "porion" | "midline";

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
  tablet_document: TabletDocument;
  revision: string | null; // null = not loaded yet
  landmarks_file: LoomisPlanesManLandmarksFile;
  mode: Mode;
  plate_opacity: number;
  surfaces_visible: boolean;
  skull_visible: boolean;
  last_lift_hits: LiftedHit[]; // the merged hits of the last lift, for checking
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

function view_landmarks(state: PageState, view: PlateView): PlateLandmarks {
  return view.is_front_view ? state.landmarks_file.front : state.landmarks_file.profile;
}

function view_registration(state: PageState, view: PlateView): PlateRegistration | null {
  return plate_registration(state.tablet_document, view_landmarks(state, view), view.is_front_view);
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

function draw_head_layer(state: PageState, view: PlateView): void {
  const gl = view.mesh_gl;
  if (gl === null || view.head_meshes === null) return;
  const { width, height } = size_canvas_to_container(view, view.mesh_canvas);
  gl.viewport(0, 0, width, height);
  gl.clearColor(0, 0, 0, 0);
  gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
  const registration = view_registration(state, view);
  if (state.revision === null || registration === null || height === 0) return;
  const camera = plate_view_camera(view, registration);
  update_head_meshes(view.head_meshes, state.tablet_document, state.revision, camera, {
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

// The head's landmarks of a view (the vertices its marks are matched to), as markers on
// the 3D head. World units are 100 mm (WORLD_PER_MM), so this is a 4 mm marker.
const HEAD_MARKER_RADIUS = 0.04;
const HEAD_MARKER_COLOR: Rgb = { r: 1, g: 1, b: 1 };
function head_landmark_markers(state: PageState, view: PlateView): { position: V3; color: Rgb; radius: number }[] {
  const markers: { position: V3; color: Rgb; radius: number }[] = [];
  for (const point of POINT_MARKS) {
    if (view.is_front_view && !point.in_front_view) continue;
    const position = named_vertex_world_position(state.tablet_document, point.vertex_name);
    if (position !== null) markers.push({ position, color: HEAD_MARKER_COLOR, radius: HEAD_MARKER_RADIUS });
  }
  return markers;
}

// The marks of a view in image pixels: the midline is drawn as a line, the others as crosses.
function mark_pixels(landmarks: PlateLandmarks, is_front_view: boolean): { name: MarkName; pixel: V2 }[] {
  const marks: { name: MarkName; pixel: V2 }[] = [
    { name: "brow", pixel: landmarks.brow },
    { name: "nasion", pixel: landmarks.nasion },
    { name: "nose_base", pixel: landmarks.nose_base },
  ];
  if (is_front_view) marks.push({ name: "midline", pixel: { x: landmarks.midline_column, y: landmarks.brow.y } });
  else marks.push({ name: "porion", pixel: landmarks.porion });
  return marks;
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
  const landmarks = view_landmarks(state, view);

  // Marks: crosses, and the midline as a line down the front plate.
  const arm = 6 / canvas_scale;
  context.strokeStyle = MARK_COLOR;
  context.fillStyle = MARK_COLOR;
  context.lineWidth = 1.4 / canvas_scale;
  context.font = `${11 / canvas_scale}px system-ui, sans-serif`;
  for (const mark of mark_pixels(landmarks, view.is_front_view)) {
    context.beginPath();
    if (mark.name === "midline") {
      context.moveTo(mark.pixel.x, 0);
      context.lineTo(mark.pixel.x, view.plate.height_pixels);
    } else {
      context.moveTo(mark.pixel.x - arm, mark.pixel.y);
      context.lineTo(mark.pixel.x + arm, mark.pixel.y);
      context.moveTo(mark.pixel.x, mark.pixel.y - arm);
      context.lineTo(mark.pixel.x, mark.pixel.y + arm);
    }
    context.stroke();
    context.fillText(mark.name, mark.pixel.x + arm, mark.pixel.y - arm);
  }

  // The head's own brow, nasion, nose base and ear hole, where the registration puts them.
  const registration = view_registration(state, view);
  if (registration !== null) {
    context.fillStyle = HEAD_POINT_COLOR;
    context.strokeStyle = "#000";
    context.lineWidth = 0.8 / canvas_scale;
    for (const name of POINT_MARKS.filter((point) => point.in_front_view || !view.is_front_view).map((point) => point.vertex_name)) {
      const world = named_vertex_world_position(state.tablet_document, name);
      if (world === null) continue;
      const pixel = pixel_from_world(registration, world);
      context.beginPath();
      context.arc(pixel.x, pixel.y, 3 / canvas_scale, 0, 2 * Math.PI);
      context.fill();
      context.stroke();
      // Labelled with the head's vertex name, below-right so it doesn't sit on the
      // green mark's label (above-right).
      context.fillText(`head ${name}`, pixel.x + arm, pixel.y + arm + 10 / canvas_scale);
      context.fillStyle = HEAD_POINT_COLOR;
    }
  }

  // The hairline drawn on this plate.
  const hairline = view.is_front_view ? state.landmarks_file.front_hairline : state.landmarks_file.profile_hairline;
  if (hairline.length > 0) {
    context.strokeStyle = HAIRLINE_COLOR;
    context.lineWidth = 2 / canvas_scale;
    context.lineJoin = "round";
    context.beginPath();
    context.moveTo(hairline[0].x, hairline[0].y);
    for (const point of hairline) context.lineTo(point.x, point.y);
    context.stroke();
  }

  // The merged hits of the last lift, seen through this view (both sides in front).
  if (registration !== null && state.last_lift_hits.length > 0) {
    context.fillStyle = LIFTED_HIT_COLOR;
    for (const hit of state.last_lift_hits) {
      const positions: V3[] = view.is_front_view ? [hit.position, { x: -hit.position.x, y: hit.position.y, z: hit.position.z }] : [hit.position];
      for (const position of positions) {
        const pixel = pixel_from_world(registration, position);
        context.beginPath();
        context.arc(pixel.x, pixel.y, 2.5 / canvas_scale, 0, 2 * Math.PI);
        context.fill();
      }
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
    const saved = await save_landmarks_file(state.landmarks_file);
    if (!saved) show_status("saving the landmark file failed", true);
    save_in_flight = null;
    if (save_again) {
      save_again = false;
      save_landmarks(state);
    }
  })();
}

// ---- gestures ---------------------------------------------------------------------------

// The mark within reach of `canvas_point`, nearest first; null when none is.
function mark_near(state: PageState, view: PlateView, canvas_point: V2): MarkName | null {
  const landmarks = view_landmarks(state, view);
  const image_point = image_pixel_at_canvas_point(view, canvas_point);
  const reach = MARK_GRAB_RADIUS_CSS_PIXELS / view.transform.image_pixels_to_canvas_scale;
  let best: MarkName | null = null;
  let best_distance = reach;
  for (const mark of mark_pixels(landmarks, view.is_front_view)) {
    const distance = mark.name === "midline" ? Math.abs(image_point.x - mark.pixel.x) : Math.hypot(image_point.x - mark.pixel.x, image_point.y - mark.pixel.y);
    if (distance < best_distance) {
      best_distance = distance;
      best = mark.name;
    }
  }
  return best;
}

function move_mark(state: PageState, view: PlateView, mark: MarkName, image_delta: V2): void {
  const landmarks = view_landmarks(state, view);
  if (mark === "midline") landmarks.midline_column += image_delta.x;
  else {
    landmarks[mark].x += image_delta.x;
    landmarks[mark].y += image_delta.y;
  }
}

function attach_view_gestures(state: PageState, view: PlateView): void {
  const canvas = view.overlay_canvas;
  const pointer_positions = new Map<number, V2>(); // pointerId -> last position, canvas CSS pixels
  let last_tap_milliseconds = 0;
  let moved_since_pointer_down = false;
  let dragged_mark: MarkName | null = null;
  let drawing_hairline = false;

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
  const hairline_of_view = (): V2[] => (view.is_front_view ? state.landmarks_file.front_hairline : state.landmarks_file.profile_hairline);

  canvas.addEventListener("pointerdown", (e) => {
    e.preventDefault();
    try { canvas.setPointerCapture(e.pointerId); } catch {} // throws for synthetic events
    const point = canvas_point(e);
    pointer_positions.set(e.pointerId, point);
    moved_since_pointer_down = false;
    if (pointer_positions.size === 1) {
      dragged_mark = state.mode === "landmarks" ? mark_near(state, view, point) : null;
      drawing_hairline = state.mode === "hairline";
      if (drawing_hairline) {
        const image_point = image_pixel_at_canvas_point(view, point);
        if (view.is_front_view) state.landmarks_file.front_hairline = [image_point];
        else state.landmarks_file.profile_hairline = [image_point];
        draw_overlay_layer(state, view);
      }
    } else {
      dragged_mark = null; // a second finger turns any drag into a pinch
      drawing_hairline = false;
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
    if (drawing_hairline) {
      hairline_of_view().push(image_pixel_at_canvas_point(view, after.centroid));
      draw_overlay_layer(state, view);
      return;
    }
    if (dragged_mark !== null) {
      move_mark(state, view, dragged_mark, image_delta);
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
    if ((dragged_mark !== null || drawing_hairline) && moved_since_pointer_down) save_landmarks(state);
    if (drawing_hairline && !moved_since_pointer_down) {
      // A tap in hairline mode leaves a one-point hairline behind: drop it.
      if (view.is_front_view) state.landmarks_file.front_hairline = [];
      else state.landmarks_file.profile_hairline = [];
      draw_overlay_layer(state, view);
    }
    const was_plain_tap = e.type === "pointerup" && !moved_since_pointer_down && dragged_mark === null && !drawing_hairline;
    dragged_mark = null;
    drawing_hairline = false;
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

// ---- the document -----------------------------------------------------------------------

// Read the document again when its revision on the server changed.
async function reload_document_if_changed(state: PageState): Promise<void> {
  const listed = await list_documents_from_server();
  const entry = listed?.find((e) => e.name === DOCUMENT_NAME);
  if (entry === undefined || entry.revision === state.revision) return;
  const response = await fetch(`/api/documents/${encodeURIComponent(DOCUMENT_NAME)}`, { cache: "no-store" });
  if (!response.ok) {
    console.error(`${PAGE_NAME}: loading document '${DOCUMENT_NAME}' failed, status ${response.status}`);
    return;
  }
  // The camera in the file belongs to the sketchpad: read into a throwaway one.
  apply_document_state(await response.text(), state.tablet_document, default_camera());
  state.revision = entry.revision;
  draw_all_views(state);
}

// Rewrite the document on the server: `edit` changes a fresh copy of the server's
// version, which is posted back against the revision it was read at.
async function edit_document_on_server(state: PageState, edit: (tablet_document: TabletDocument) => boolean): Promise<boolean> {
  const url = `/api/documents/${encodeURIComponent(DOCUMENT_NAME)}`;
  const camera = default_camera();
  try {
    const existing = await fetch(url, { cache: "no-store" });
    if (!existing.ok) {
      show_status(`reading ${DOCUMENT_NAME} failed: ${existing.status}`, true);
      return false;
    }
    const headers: Record<string, string> = { "Content-Type": "application/json" };
    const revision = existing.headers.get("X-Revision");
    if (revision !== null) headers["X-Base-Revision"] = revision;
    const edited = empty_document();
    apply_document_state(await existing.text(), edited, camera);
    if (!edit(edited)) return false;
    const written = await fetch(url, { method: "POST", headers, body: serialize_document_state(edited, camera) });
    if (!written.ok) {
      show_status(`writing ${DOCUMENT_NAME} failed: ${written.status} ${await written.text()}`, true);
      return false;
    }
    return true;
  } catch (error) {
    console.error(`${PAGE_NAME}: editing ${DOCUMENT_NAME} failed`, error);
    show_status(`editing ${DOCUMENT_NAME} failed (server unreachable?)`, true);
    return false;
  }
}

// The lift's vertices are surface-pinned, and a surface pin keeps a vertex alive
// through delete_stroke (the document's rule: a pinned vertex is referenced). The
// vertices are the lift's own, so their pins are dropped first and the garbage
// collector then removes any the strokes no longer use.
function remove_lifted_strokes(tablet_document: TabletDocument, stroke_ids: number[]): void {
  const lifted_vertices: VertexId[] = [];
  for (const stroke_id of stroke_ids) {
    const stroke = tablet_document.strokes.find((candidate) => candidate.id === stroke_id);
    if (stroke === undefined) continue;
    lifted_vertices.push(stroke.p0_vertex, stroke.p3_vertex);
    delete_stroke(tablet_document, stroke_id);
  }
  unpin_vertices(tablet_document, lifted_vertices);
  garbage_collect_vertices(tablet_document);
}

async function lift_hairline(state: PageState): Promise<void> {
  const front = state.views.find((view) => view.is_front_view)!;
  const profile = state.views.find((view) => !view.is_front_view)!;
  const front_registration = view_registration(state, front);
  const profile_registration = view_registration(state, profile);
  if (front_registration === null || profile_registration === null) {
    show_status("the head is not loaded yet (no brow / nose base / ear hole)", true);
    return;
  }
  const file = state.landmarks_file;
  if (file.front_hairline.length < 2 && file.profile_hairline.length < 2) {
    show_status("draw a hairline on a plate first", true);
    return;
  }
  show_status("lifting…");
  let written_ids: number[] = [];
  let hit_counts = { front: 0, profile: 0, merged: 0 };
  const written = await edit_document_on_server(state, (tablet_document) => {
    remove_lifted_strokes(tablet_document, file.lifted_stroke_ids);
    const front_hits = cast_hairline_samples(tablet_document, front_registration, file.front_hairline);
    const profile_hits = cast_hairline_samples(tablet_document, profile_registration, file.profile_hairline);
    const merged = merge_hits_by_angle([...front_hits, ...profile_hits]);
    hit_counts = { front: front_hits.length, profile: profile_hits.length, merged: merged.length };
    state.last_lift_hits = merged;
    if (merged.length < 2) {
      show_status(`too few hits to lift: ${front_hits.length} front, ${profile_hits.length} profile`, true);
      return false;
    }
    written_ids = write_hairline_strokes(tablet_document, merged);
    return true;
  });
  if (!written) {
    draw_all_views(state);
    return;
  }
  file.lifted_stroke_ids = written_ids;
  save_landmarks(state);
  show_status(`lifted: ${hit_counts.front} front hits, ${hit_counts.profile} profile hits, ${hit_counts.merged} merged, ${written_ids.length} strokes written`);
  await reload_document_if_changed(state);
}

async function remove_lifted(state: PageState): Promise<void> {
  const file = state.landmarks_file;
  if (file.lifted_stroke_ids.length === 0) {
    show_status("nothing lifted to remove");
    return;
  }
  const written = await edit_document_on_server(state, (tablet_document) => {
    remove_lifted_strokes(tablet_document, file.lifted_stroke_ids);
    return true;
  });
  if (!written) return;
  file.lifted_stroke_ids = [];
  state.last_lift_hits = [];
  save_landmarks(state);
  show_status("lifted strokes removed");
  await reload_document_if_changed(state);
}

// ---- wiring -----------------------------------------------------------------------------

async function start(): Promise<void> {
  const opacity_slider = document.getElementById("plate_opacity") as HTMLInputElement;
  const surfaces_checkbox = document.getElementById("plate_surfaces_visible") as HTMLInputElement;
  const skull_checkbox = document.getElementById("plate_skull_visible") as HTMLInputElement;
  const mode_radios = Array.from(document.querySelectorAll<HTMLInputElement>("input[name=mode]"));

  const landmarks_file = await fetch_landmarks_file();
  if (landmarks_file === null) show_status("loading the landmark file failed; changes will not be saved", true);

  const state: PageState = {
    views: [],
    tablet_document: empty_document(),
    revision: null,
    landmarks_file: landmarks_file ?? starting_landmarks_file(),
    mode: (mode_radios.find((radio) => radio.checked)?.value as Mode | undefined) ?? "move",
    plate_opacity: Number(opacity_slider.value),
    surfaces_visible: surfaces_checkbox.checked,
    skull_visible: skull_checkbox.checked,
    last_lift_hits: [],
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

  for (const radio of mode_radios) {
    radio.addEventListener("change", () => {
      if (radio.checked) state.mode = radio.value as Mode;
      for (const view of state.views) view.overlay_canvas.style.cursor = state.mode === "hairline" ? "crosshair" : "grab";
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

  document.getElementById("lift_button")!.addEventListener("click", () => void lift_hairline(state));
  document.getElementById("remove_lifted")!.addEventListener("click", () => void remove_lifted(state));
  document.getElementById("clear_front_hairline")!.addEventListener("click", () => {
    state.landmarks_file.front_hairline = [];
    save_landmarks(state);
    draw_all_views(state);
  });
  document.getElementById("clear_profile_hairline")!.addEventListener("click", () => {
    state.landmarks_file.profile_hairline = [];
    save_landmarks(state);
    draw_all_views(state);
  });

  draw_all_views(state);
  void reload_document_if_changed(state);
  setInterval(() => void reload_document_if_changed(state), POLL_MILLISECONDS);
}

void start();
