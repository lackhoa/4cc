// The head model pane of the landmark pages (loomis-girl-fit-landmarks,
// fit-hairline-to-loomis-man): the template drawing (document `skull-zanatomy`) as its
// surfaces and strokes with depth (head_meshes.ts, plan-hairline-reference-plate Q13),
// with a marker at every landmark's template point, hidden when the head is in front of
// it (Q14). Drag orbits (middle button pans, wheel zooms); a tap picks: a landmark, or,
// while the page is adding a landmark, the template point the new landmark gets. Only
// what is visible can be picked: a tap on the cheek adds nothing on the far ear.
//
// Two canvases stacked: WebGL for the head and the markers, 2D on top for the names.
import { OrbitCamera, camera_eye, camera_screen_projector, camera_view_projection } from "./camera";
import { TabletDocument, bezier_point, stroke_control_points, vertex_world_position } from "./document";
import { attach_orbit_controls } from "./explainer/orbit_controls";
import { EditableLandmark } from "./fit_landmark_editing";
import { HeadMeshes, create_head_meshes, draw_head_meshes, head_surface_hides_point, update_head_meshes } from "./head_meshes";
import { TemplatePoint, template_point_world_position, template_position_is_on_midline } from "./loomis_girl_plate";
import { V2, V3, v3, v3_length, v3_sub } from "./math";
import { Rgb } from "./vertex_sink";

const TAP_RADIUS_PIXELS = 14;
const SKIN_STROKE_COLOR: Rgb = { r: 1, g: 0.42, b: 0.43 };
const SKULL_STROKE_COLOR: Rgb = { r: 0.5, g: 0.7, b: 1 };
const LANDMARK_COLOR: Rgb = { r: 0.25, g: 0.85, b: 0.48 };
const SELECTED_LANDMARK_COLOR: Rgb = { r: 1, g: 0.18, b: 0.7 };
const CANDIDATE_COLOR: Rgb = { r: 1, g: 1, b: 1 };
const LANDMARK_RADIUS = 0.02; // world units
const SELECTED_LANDMARK_RADIUS = 0.03;
const CANDIDATE_RADIUS = 0.008;
// A point this far behind the surface in front of it still counts as on that surface
// (the landmarks sit on the strokes, which lie on the patches' surfaces, give or take).
const ON_SURFACE_TOLERANCE = 0.02;

// What the pane draws, asked from the page on every draw.
export type HeadModelScene = {
  template_document: TabletDocument;
  document_revision: string; // any string that changes when the document does (the meshes are rebuilt then)
  landmarks: EditableLandmark[]; // a page's own landmark type: only `name` and `template_point` are read
  selected_landmark_index: number | null;
  landmark_names_visible: boolean;
  skull_visible: boolean;
  is_picking_template_point: boolean; // the page is adding a landmark: a tap picks its template point
};

export type HeadModelPaneListeners = {
  on_template_point_picked: (template_point: TemplatePoint) => void;
  on_landmark_tapped: (landmark_index: number | null) => void; // null = the tap hit no landmark
};

// A point of the template a tap can pick.
type PickCandidate = { template_point: TemplatePoint; world_position: V3 };

function mirrored(p: V3): V3 {
  return v3(-p.x, p.y, p.z);
}

// The candidate under the tap: among the candidates within reach that the head does
// not hide, the one closest to the tap. A candidate is also hit through its mirror image
// on the other side of the face.
function candidate_under_tap<Candidate extends { world_position: V3 }>(
  candidates: Candidate[], tap: V2, project: (world: V3) => V2 | null, is_hidden: (world: V3) => boolean,
): Candidate | null {
  let best: Candidate | null = null;
  let best_distance = TAP_RADIUS_PIXELS;
  for (const candidate of candidates) {
    for (const position of [candidate.world_position, mirrored(candidate.world_position)]) {
      const screen = project(position);
      if (screen === null) continue;
      const tap_distance = Math.hypot(screen.x - tap.x, screen.y - tap.y);
      if (tap_distance >= best_distance || is_hidden(position)) continue;
      best = candidate;
      best_distance = tap_distance;
    }
  }
  return best;
}

// Returns the function that draws the pane again (call it when the scene changed).
// `page_name` names the page in the pane's errors; `element_ids` are the page's two
// stacked canvases (the WebGL one below) and its surface opacity slider.
export function start_head_model_pane(
  page_name: string,
  element_ids: { mesh_canvas: string; overlay_canvas: string; mesh_opacity_slider: string },
  scene_now: () => HeadModelScene,
  listeners: HeadModelPaneListeners,
): () => void {
  const mesh_canvas = document.getElementById(element_ids.mesh_canvas) as HTMLCanvasElement;
  const overlay_canvas = document.getElementById(element_ids.overlay_canvas) as HTMLCanvasElement;
  const mesh_opacity_slider = document.getElementById(element_ids.mesh_opacity_slider) as HTMLInputElement;
  const camera: OrbitCamera = { pivot: v3(0, 0.2, 0), yaw: 0.6, pitch: 0.05, distance: 3.8 };

  const gl = mesh_canvas.getContext("webgl");
  if (gl === null) console.error(`${page_name}: no WebGL, the head model pane shows names only`);
  const head_meshes: HeadMeshes | null = gl === null ? null : create_head_meshes(gl);

  const size_canvas_to_pane = (canvas: HTMLCanvasElement): void => {
    const device_pixel_ratio = window.devicePixelRatio || 1;
    const width = Math.round(canvas.clientWidth * device_pixel_ratio);
    const height = Math.round(canvas.clientHeight * device_pixel_ratio);
    if (canvas.width !== width || canvas.height !== height) {
      canvas.width = width;
      canvas.height = height;
    }
  };

  // Whether the head hides `position` from the camera (false before the meshes are built).
  const is_hidden = (position: V3): boolean => head_meshes !== null && head_surface_hides_point(head_meshes, camera_eye(camera), position, ON_SURFACE_TOLERANCE);

  const visible_strokes = (scene: HeadModelScene) => scene.template_document.strokes.filter((stroke) => stroke.layer === "skin" || scene.skull_visible);

  // The points a new landmark can sit on: the endpoints of the visible strokes.
  const vertex_candidates = (scene: HeadModelScene): PickCandidate[] => {
    const vertex_ids = new Set<number>();
    for (const stroke of visible_strokes(scene)) vertex_ids.add(stroke.p0_vertex).add(stroke.p3_vertex);
    return scene.template_document.vertices
      .filter((vertex) => vertex_ids.has(vertex.id))
      .map((vertex) => ({ template_point: { vertex: vertex.id }, world_position: vertex_world_position(scene.template_document, vertex) }));
  };

  // ... and points along them, when the tap is on no endpoint.
  const stroke_candidates = (scene: HeadModelScene): PickCandidate[] => {
    const SAMPLES_PER_STROKE = 40;
    const candidates: PickCandidate[] = [];
    for (const stroke of visible_strokes(scene)) {
      const control_points = stroke_control_points(stroke, scene.template_document);
      for (let i = 1; i < SAMPLES_PER_STROKE; i++) {
        const position_along_stroke = i / SAMPLES_PER_STROKE;
        candidates.push({ template_point: { stroke: stroke.id, position_along_stroke }, world_position: bezier_point(control_points, position_along_stroke) });
      }
    }
    return candidates;
  };

  // The landmarks' template positions, null for a landmark not in the template (only the
  // plate panes show that one).
  const landmark_positions = (scene: HeadModelScene): (V3 | null)[] =>
    scene.landmarks.map((landmark) => template_point_world_position(scene.template_document, landmark.template_point));

  const draw_head = (scene: HeadModelScene): void => {
    if (gl === null || head_meshes === null) return;
    size_canvas_to_pane(mesh_canvas);
    gl.viewport(0, 0, mesh_canvas.width, mesh_canvas.height);
    gl.clearColor(0.078, 0.086, 0.11, 1);
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
    if (mesh_canvas.height === 0) return;
    // The document holds one side of the head, the meshes mirror it; a midline landmark
    // is one marker, any other is mirrored with the head.
    const markers: { position: V3; color: Rgb; radius: number }[] = [];
    landmark_positions(scene).forEach((position, landmark_index) => {
      if (position === null) return;
      const is_selected = landmark_index === scene.selected_landmark_index;
      markers.push({ position, color: is_selected ? SELECTED_LANDMARK_COLOR : LANDMARK_COLOR, radius: is_selected ? SELECTED_LANDMARK_RADIUS : LANDMARK_RADIUS });
    });
    if (scene.is_picking_template_point) {
      for (const candidate of vertex_candidates(scene)) markers.push({ position: candidate.world_position, color: CANDIDATE_COLOR, radius: CANDIDATE_RADIUS });
    }
    update_head_meshes(head_meshes, scene.template_document, scene.document_revision, camera, {
      skull_visible: scene.skull_visible,
      surfaces_visible: true,
      strokes_visible: true,
      mirrored: true,
      surface_opacity: Number(mesh_opacity_slider.value),
      skin_stroke_color: SKIN_STROKE_COLOR,
      skull_stroke_color: SKULL_STROKE_COLOR,
      markers,
      markers_depth_tested: true,
    });
    draw_head_meshes(head_meshes, camera_view_projection(camera, mesh_canvas.width / mesh_canvas.height), camera);
  };

  // The names, beside the markers the head does not hide.
  const draw_overlay = (scene: HeadModelScene): void => {
    size_canvas_to_pane(overlay_canvas);
    const device_pixel_ratio = window.devicePixelRatio || 1;
    const width = overlay_canvas.clientWidth;
    const height = overlay_canvas.clientHeight;
    const context = overlay_canvas.getContext("2d")!;
    context.setTransform(1, 0, 0, 1, 0, 0);
    context.clearRect(0, 0, overlay_canvas.width, overlay_canvas.height);
    if (height === 0) return;
    // From here on, coordinates are CSS pixels.
    context.setTransform(device_pixel_ratio, 0, 0, device_pixel_ratio, 0, 0);
    const project = camera_screen_projector(camera, width, height);
    landmark_positions(scene).forEach((template_position, landmark_index) => {
      if (template_position === null) return;
      const is_selected = landmark_index === scene.selected_landmark_index;
      if (!(is_selected || scene.landmark_names_visible)) return;
      // The name goes by the side nearer the camera, if the head does not hide it.
      const positions = template_position_is_on_midline(template_position) ? [template_position] : [template_position, mirrored(template_position)];
      const eye = camera_eye(camera);
      positions.sort((a, b) => v3_length(v3_sub(a, eye)) - v3_length(v3_sub(b, eye)));
      const shown = positions.find((position) => !is_hidden(position));
      if (shown === undefined) return;
      const screen = project(shown);
      if (screen === null) return;
      context.font = is_selected ? "bold 15px system-ui, sans-serif" : "12px system-ui, sans-serif";
      context.lineWidth = 3;
      context.strokeStyle = "#14161c";
      context.fillStyle = is_selected ? "#ff2fb4" : "#3fd97a";
      context.strokeText(scene.landmarks[landmark_index].name, screen.x + 8, screen.y - 6);
      context.fillText(scene.landmarks[landmark_index].name, screen.x + 8, screen.y - 6);
    });
  };

  // A narrow pane starts further back, so the whole head is in it.
  let camera_is_fitted_to_pane = false;
  const draw = (): void => {
    if (!camera_is_fitted_to_pane && overlay_canvas.clientHeight > 0) {
      const WIDTH_PER_HEIGHT_THE_HEAD_FITS_IN = 0.8;
      camera.distance *= Math.max(1, WIDTH_PER_HEIGHT_THE_HEAD_FITS_IN / (overlay_canvas.clientWidth / overlay_canvas.clientHeight));
      camera_is_fitted_to_pane = true;
    }
    const scene = scene_now();
    draw_head(scene);
    draw_overlay(scene);
  };

  const tap = (tap_point: V2): void => {
    const scene = scene_now();
    const project = camera_screen_projector(camera, overlay_canvas.clientWidth, overlay_canvas.clientHeight);
    if (scene.is_picking_template_point) {
      const picked = candidate_under_tap(vertex_candidates(scene), tap_point, project, is_hidden) ?? candidate_under_tap(stroke_candidates(scene), tap_point, project, is_hidden);
      if (picked !== null) listeners.on_template_point_picked(picked.template_point);
      return;
    }
    const landmark_candidates: { landmark_index: number; world_position: V3 }[] = [];
    landmark_positions(scene).forEach((world_position, landmark_index) => {
      if (world_position !== null) landmark_candidates.push({ landmark_index, world_position });
    });
    const tapped = candidate_under_tap(landmark_candidates, tap_point, project, is_hidden);
    listeners.on_landmark_tapped(tapped === null ? null : tapped.landmark_index);
  };

  // A press that ends within a few pixels of where it began is a tap; anything longer was an orbit.
  let press_start: V2 | null = null;
  let press_travel_pixels = 0;
  overlay_canvas.addEventListener("pointerdown", (e) => {
    press_start = { x: e.clientX, y: e.clientY };
    press_travel_pixels = 0;
  });
  overlay_canvas.addEventListener("pointermove", (e) => {
    if (press_start !== null) press_travel_pixels = Math.max(press_travel_pixels, Math.hypot(e.clientX - press_start.x, e.clientY - press_start.y));
  });
  overlay_canvas.addEventListener("pointerup", (e) => {
    const was_tap = press_start !== null && press_travel_pixels < 4 && e.button === 0;
    press_start = null;
    if (!was_tap) return;
    const rectangle = overlay_canvas.getBoundingClientRect();
    tap({ x: e.clientX - rectangle.left, y: e.clientY - rectangle.top });
  });
  overlay_canvas.addEventListener("pointercancel", () => { press_start = null; });

  attach_orbit_controls([overlay_canvas], camera, draw);
  mesh_opacity_slider.addEventListener("input", draw);
  new ResizeObserver(draw).observe(overlay_canvas);
  draw();
  return draw;
}
