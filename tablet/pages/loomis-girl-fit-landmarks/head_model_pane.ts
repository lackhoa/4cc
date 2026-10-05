// The head model pane of the loomis-girl-fit-landmarks page: the template drawing (document
// `skull-zanatomy`) as lines over the Z-Anatomy skin mesh, with a dot at every landmark's
// template point. Drag orbits (middle button pans, wheel zooms); a tap picks: a landmark,
// or, while the page is adding a landmark, the template point the new landmark gets.
//
// Two canvases stacked: WebGL for the mesh, 2D on top for lines, dots and names. The lines
// are not hidden by the mesh, so the far side of the head shows through.
import { OrbitCamera, camera_eye, camera_screen_projector, camera_view_projection } from "../../src/camera";
import { TabletDocument, bezier_point, stroke_control_points, vertex_world_position } from "../../src/document";
import { attach_orbit_controls } from "../../src/explainer/orbit_controls";
import { FitLandmark, TemplatePoint, template_point_world_position, template_position_is_on_midline } from "../../src/loomis_girl_plate";
import { V2, V3, v3, v3_length, v3_scale, v3_sub } from "../../src/math";
import { ReferenceMesh, append_reference_mesh, fetch_reference_text, frankfurt_coordinates, parse_obj_mesh_raw, reference_mesh_from_positions } from "../../src/reference";
import { SKIN_NAME, WORLD_PER_MM, load_skull } from "../../src/reference_skull_view";
import { FLOATS_PER_TRANSLUCENT_VERTEX, create_translucent_mesh, draw_mesh_translucent, set_translucent_mesh } from "../../src/render";
import { FLOATS_PER_VERTEX, create_vertex_sink, reset_vertex_sink } from "../../src/vertex_sink";

const PAGE_NAME = "loomis-girl-fit-landmarks";
const TAP_RADIUS_PIXELS = 14;
const SKIN_STROKE_STYLE = "#ff6b6e";
const SKULL_STROKE_STYLE = "#7fb3ff";
const LANDMARK_STYLE = "#3fd97a";
const SELECTED_LANDMARK_STYLE = "#ff2fb4";

// What the pane draws, asked from the page on every draw.
export type HeadModelScene = {
  template_document: TabletDocument;
  landmarks: FitLandmark[];
  selected_landmark_index: number | null;
  landmark_names_visible: boolean;
  skull_visible: boolean;
  is_picking_template_point: boolean; // the page is adding a landmark: a tap picks its template point
};

export type HeadModelPaneListeners = {
  on_template_point_picked: (template_point: TemplatePoint) => void;
  on_landmark_tapped: (landmark_index: number | null) => void; // null = the tap hit no landmark
};

// The head's skin in the skull's Frankfurt frame, world units: the space the template is drawn in.
async function load_skin_mesh(): Promise<ReferenceMesh | null> {
  const [skull, obj_text] = await Promise.all([load_skull(PAGE_NAME), fetch_reference_text(`/reference/${SKIN_NAME}.obj`)]);
  if (skull === null || obj_text === null) return null;
  const raw = parse_obj_mesh_raw(obj_text);
  if (raw === null) {
    console.error(`${PAGE_NAME}: ${SKIN_NAME} mesh unreadable`);
    return null;
  }
  return reference_mesh_from_positions(raw.positions.map((p) => v3_scale(frankfurt_coordinates(skull.frame, p), WORLD_PER_MM)), raw.triangle_indices);
}

// A point of the template a tap can pick.
type PickCandidate = { template_point: TemplatePoint; world_position: V3 };

function mirrored(p: V3): V3 {
  return v3(-p.x, p.y, p.z);
}

// The candidate under the tap. The lines are see-through, so a tap can cover points of
// both the near and the far side of the head: the near ones win, then the one closest to
// the tap. A candidate is also hit through its mirror image on the other side of the face.
function candidate_under_tap<Candidate extends { world_position: V3 }>(candidates: Candidate[], tap: V2, project: (world: V3) => V2 | null, eye: V3): Candidate | null {
  const NEAR_SIDE_DEPTH = 0.15; // world units behind the nearest hit that still count as the near side
  const hits: { candidate: Candidate; tap_distance: number; eye_distance: number }[] = [];
  for (const candidate of candidates) {
    for (const position of [candidate.world_position, mirrored(candidate.world_position)]) {
      const screen = project(position);
      if (screen === null) continue;
      const tap_distance = Math.hypot(screen.x - tap.x, screen.y - tap.y);
      if (tap_distance <= TAP_RADIUS_PIXELS) hits.push({ candidate, tap_distance, eye_distance: v3_length(v3_sub(position, eye)) });
    }
  }
  if (hits.length === 0) return null;
  const nearest_eye_distance = Math.min(...hits.map((hit) => hit.eye_distance));
  const near_hits = hits.filter((hit) => hit.eye_distance <= nearest_eye_distance + NEAR_SIDE_DEPTH);
  near_hits.sort((a, b) => a.tap_distance - b.tap_distance);
  return near_hits[0].candidate;
}

// Returns the function that draws the pane again (call it when the scene changed).
export function start_head_model_pane(scene_now: () => HeadModelScene, listeners: HeadModelPaneListeners): () => void {
  const mesh_canvas = document.getElementById("head_model_mesh_canvas") as HTMLCanvasElement;
  const overlay_canvas = document.getElementById("head_model_overlay_canvas") as HTMLCanvasElement;
  const mesh_opacity_slider = document.getElementById("head_model_mesh_opacity") as HTMLInputElement;
  const camera: OrbitCamera = { pivot: v3(0, 0.2, 0), yaw: 0.6, pitch: 0.05, distance: 3.8 };

  const gl = mesh_canvas.getContext("webgl");
  if (gl === null) console.error(`${PAGE_NAME}: no WebGL, the head model pane shows lines only`);
  const translucent_mesh = gl === null ? null : create_translucent_mesh(gl);
  let skin_mesh: ReferenceMesh | null = null;
  const shaded_vertices = create_vertex_sink(1024);
  let translucent_vertices = new Float32Array(0);
  let uploaded_shading = ""; // camera direction and opacity the uploaded vertices were shaded for

  const size_canvas_to_pane = (canvas: HTMLCanvasElement): void => {
    const device_pixel_ratio = window.devicePixelRatio || 1;
    const width = Math.round(canvas.clientWidth * device_pixel_ratio);
    const height = Math.round(canvas.clientHeight * device_pixel_ratio);
    if (canvas.width !== width || canvas.height !== height) {
      canvas.width = width;
      canvas.height = height;
    }
  };

  const draw_mesh = (): void => {
    if (gl === null || translucent_mesh === null) return;
    size_canvas_to_pane(mesh_canvas);
    gl.viewport(0, 0, mesh_canvas.width, mesh_canvas.height);
    gl.clearColor(0.078, 0.086, 0.11, 1);
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
    const opacity = Number(mesh_opacity_slider.value);
    if (skin_mesh === null || opacity <= 0 || mesh_canvas.height === 0) return;
    // The shading is a headlight: it changes when the camera turns.
    const shading = `${camera.yaw} ${camera.pitch} ${opacity}`;
    if (shading !== uploaded_shading) {
      reset_vertex_sink(shaded_vertices);
      append_reference_mesh(skin_mesh, camera, shaded_vertices);
      const vertex_count = shaded_vertices.length / FLOATS_PER_VERTEX;
      const float_count = vertex_count * FLOATS_PER_TRANSLUCENT_VERTEX;
      if (translucent_vertices.length !== float_count) translucent_vertices = new Float32Array(float_count);
      for (let vertex = 0; vertex < vertex_count; vertex++) {
        const source = vertex * FLOATS_PER_VERTEX;
        const target = vertex * FLOATS_PER_TRANSLUCENT_VERTEX;
        for (let i = 0; i < FLOATS_PER_VERTEX; i++) translucent_vertices[target + i] = shaded_vertices.data[source + i];
        translucent_vertices[target + FLOATS_PER_VERTEX] = opacity;
      }
      set_translucent_mesh(translucent_mesh, translucent_vertices);
      uploaded_shading = shading;
    }
    draw_mesh_translucent(translucent_mesh, camera_view_projection(camera, mesh_canvas.width / mesh_canvas.height), camera_eye(camera));
  };

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

  const draw_overlay = (): void => {
    const scene = scene_now();
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

    context.lineJoin = "round";
    for (const stroke of visible_strokes(scene)) {
      const control_points = stroke_control_points(stroke, scene.template_document);
      context.strokeStyle = stroke.layer === "skin" ? SKIN_STROKE_STYLE : SKULL_STROKE_STYLE;
      context.lineWidth = stroke.layer === "skin" ? 1.6 : 1;
      // The document holds one side of the head; the other side is its mirror.
      for (const side of [(p: V3) => p, mirrored]) {
        context.beginPath();
        let pen_is_down = false;
        for (let i = 0; i <= 24; i++) {
          const screen = project(side(bezier_point(control_points, i / 24)));
          if (screen === null) { pen_is_down = false; continue; }
          if (pen_is_down) context.lineTo(screen.x, screen.y); else context.moveTo(screen.x, screen.y);
          pen_is_down = true;
        }
        context.stroke();
      }
    }

    if (scene.is_picking_template_point) {
      context.fillStyle = "#ffffff";
      for (const candidate of vertex_candidates(scene)) {
        const screen = project(candidate.world_position);
        if (screen === null) continue;
        context.beginPath();
        context.arc(screen.x, screen.y, 2.5, 0, 2 * Math.PI);
        context.fill();
      }
    }

    scene.landmarks.forEach((landmark, landmark_index) => {
      const template_position = template_point_world_position(scene.template_document, landmark.template_point);
      if (template_position === null) return; // not in the template: only the plate panes show this landmark
      const is_selected = landmark_index === scene.selected_landmark_index;
      const style = is_selected ? SELECTED_LANDMARK_STYLE : LANDMARK_STYLE;
      const positions = template_position_is_on_midline(template_position) ? [template_position] : [template_position, mirrored(template_position)];
      positions.forEach((position, position_index) => {
        const screen = project(position);
        if (screen === null) return;
        context.fillStyle = style;
        context.beginPath();
        context.arc(screen.x, screen.y, is_selected ? 5.5 : 3.5, 0, 2 * Math.PI);
        context.fill();
        if (position_index === 0 && (is_selected || scene.landmark_names_visible)) {
          context.font = is_selected ? "bold 15px system-ui, sans-serif" : "12px system-ui, sans-serif";
          context.lineWidth = 3;
          context.strokeStyle = "#14161c";
          context.strokeText(landmark.name, screen.x + 8, screen.y - 6);
          context.fillText(landmark.name, screen.x + 8, screen.y - 6);
        }
      });
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
    draw_mesh();
    draw_overlay();
  };

  const tap = (tap_point: V2): void => {
    const scene = scene_now();
    const project = camera_screen_projector(camera, overlay_canvas.clientWidth, overlay_canvas.clientHeight);
    const eye = camera_eye(camera);
    if (scene.is_picking_template_point) {
      const picked = candidate_under_tap(vertex_candidates(scene), tap_point, project, eye) ?? candidate_under_tap(stroke_candidates(scene), tap_point, project, eye);
      if (picked !== null) listeners.on_template_point_picked(picked.template_point);
      return;
    }
    const landmark_candidates: { landmark_index: number; world_position: V3 }[] = [];
    scene.landmarks.forEach((landmark, landmark_index) => {
      const world_position = template_point_world_position(scene.template_document, landmark.template_point);
      if (world_position !== null) landmark_candidates.push({ landmark_index, world_position });
    });
    const tapped = candidate_under_tap(landmark_candidates, tap_point, project, eye);
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

  void load_skin_mesh().then((mesh) => {
    skin_mesh = mesh;
    draw();
  });
  draw();
  return draw;
}
