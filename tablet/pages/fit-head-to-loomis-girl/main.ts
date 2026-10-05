// Page `fit-head-to-loomis-girl` (plan-loomis-girl-elaboration.md): the fit of the template
// head (skull and skin drawing, document `skull-zanatomy`) to Loomis's school girl plate,
// read top to bottom: the template on the plate, the fitted document `loomis-school-girl`
// on the plate, the fitted head turned, and how far each landmark is from its mark.
// View only: nothing on this page writes a document. The page loomis-girl-fit-landmarks
// edits the landmarks and writes `loomis-school-girl` from `skull-zanatomy`.
import "../../pages.css";
import { OrbitCamera, camera_eye, camera_view_projection } from "../../src/camera";
import { TabletDocument, bezier_point, patch_layer, stroke_control_points } from "../../src/document";
import { V3, v3 } from "../../src/math";
import { patch_surface_grid } from "../../src/patch";
import { ReferenceMesh, append_reference_mesh, reference_mesh_from_positions } from "../../src/reference";
import { FLOATS_PER_TRANSLUCENT_VERTEX, create_translucent_mesh, draw_mesh_translucent, set_translucent_mesh } from "../../src/render";
import { FLOATS_PER_VERTEX, Rgb, create_vertex_sink, reset_vertex_sink } from "../../src/vertex_sink";
import { attach_orbit_controls } from "../../src/explainer/orbit_controls";
import { canvas_view, stroke_polyline } from "../../src/explainer/canvas_view";
import {
  fit_landmark_mark_world_position, front_pixel_from_world, profile_pixel_from_world, template_point_world_position, template_position_is_on_midline,
} from "../../src/loomis_girl_plate";
import { PlateViewsLandmarks, start_loomis_girl_plate_views } from "./loomis_girl_plate_views";

const TEMPLATE_DOCUMENT_NAME = "skull-zanatomy";
const FITTED_DOCUMENT_NAME = "loomis-school-girl";
const SKIN_STROKE_STYLE = "#ff6b6e";
const SKULL_STROKE_STYLE = "#7fb3ff";

let fitted_document: TabletDocument | null = null;

// ---- the fitted head, turned --------------------------------------------------------
// The surfaces of the document's patches on a WebGL canvas, the lines on a 2D canvas over
// it. The lines are not hidden by the surfaces: the lines of the far side show through.

const TURNED_SURFACE_COLOR: Rgb = { r: 0.45, g: 0.55, b: 0.7 };

const camera: OrbitCamera = { pivot: v3(0, 0.2, 0), yaw: 0.6, pitch: 0.05, distance: 3.8 };
const turned_canvas = document.getElementById("turned_canvas") as HTMLCanvasElement;
const turned_mesh_canvas = document.getElementById("turned_mesh_canvas") as HTMLCanvasElement;
const turned_surfaces_checkbox = document.getElementById("turned_surfaces_visible") as HTMLInputElement;
const turned_lines_checkbox = document.getElementById("turned_lines_visible") as HTMLInputElement;
const turned_skull_checkbox = document.getElementById("turned_skull_visible") as HTMLInputElement;

const turned_gl = turned_mesh_canvas.getContext("webgl");
if (turned_gl === null) console.error("fit-head-to-loomis-girl: no WebGL, the turned head shows lines only");
const turned_translucent_mesh = turned_gl === null ? null : create_translucent_mesh(turned_gl);
let turned_surface_mesh: ReferenceMesh | null = null;
const turned_shaded_vertices = create_vertex_sink(1024);
let turned_uploaded_shading = ""; // camera direction the uploaded vertices were shaded for

// The document holds one side of the head; the other side is its mirror.
const mirror = (p: V3): V3 => v3(-p.x, p.y, p.z);

// Every visible patch of the fitted document as triangles, both sides of the head.
function rebuild_turned_surface_mesh(): void {
  turned_surface_mesh = null;
  turned_uploaded_shading = "";
  if (fitted_document === null) return;
  const positions: V3[] = [];
  for (const patch of fitted_document.patches) {
    if (patch_layer(patch, fitted_document) === "skull" && !turned_skull_checkbox.checked) continue;
    const grid = patch_surface_grid(patch, fitted_document);
    if (grid === null) continue;
    for (let j = 0; j < grid.rows; j++) {
      for (let i = 0; i < grid.columns; i++) {
        const point_00 = grid.positions[i][j];
        const point_10 = grid.positions[i + 1][j];
        const point_11 = grid.positions[i + 1][j + 1];
        const point_01 = grid.positions[i][j + 1];
        const cell_triangles = [point_00, point_10, point_11, point_00, point_11, point_01];
        positions.push(...cell_triangles, ...cell_triangles.map(mirror));
      }
    }
  }
  const triangle_colors = Array.from({ length: positions.length / 3 }, () => TURNED_SURFACE_COLOR);
  turned_surface_mesh = reference_mesh_from_positions(positions, positions.map((_, index) => index), triangle_colors);
}

function draw_turned_surfaces(): void {
  if (turned_gl === null || turned_translucent_mesh === null) return;
  turned_mesh_canvas.width = turned_canvas.width;
  turned_mesh_canvas.height = turned_canvas.height;
  turned_gl.viewport(0, 0, turned_mesh_canvas.width, turned_mesh_canvas.height);
  turned_gl.clearColor(0.078, 0.086, 0.11, 1);
  turned_gl.clear(turned_gl.COLOR_BUFFER_BIT | turned_gl.DEPTH_BUFFER_BIT);
  if (turned_surface_mesh === null || !turned_surfaces_checkbox.checked || turned_mesh_canvas.height === 0) return;
  const shading = `${camera.yaw} ${camera.pitch}`;
  if (shading !== turned_uploaded_shading) {
    reset_vertex_sink(turned_shaded_vertices);
    append_reference_mesh(turned_surface_mesh, camera, turned_shaded_vertices);
    const vertex_count = turned_shaded_vertices.length / FLOATS_PER_VERTEX;
    const translucent_vertices = new Float32Array(vertex_count * FLOATS_PER_TRANSLUCENT_VERTEX);
    for (let vertex = 0; vertex < vertex_count; vertex++) {
      const source = vertex * FLOATS_PER_VERTEX;
      const target = vertex * FLOATS_PER_TRANSLUCENT_VERTEX;
      for (let i = 0; i < FLOATS_PER_VERTEX; i++) translucent_vertices[target + i] = turned_shaded_vertices.data[source + i];
      translucent_vertices[target + FLOATS_PER_VERTEX] = 1; // opaque
    }
    set_translucent_mesh(turned_translucent_mesh, translucent_vertices);
    turned_uploaded_shading = shading;
  }
  draw_mesh_translucent(turned_translucent_mesh, camera_view_projection(camera, turned_mesh_canvas.width / turned_mesh_canvas.height), camera_eye(camera));
}

function draw_turned_view(): void {
  const view = canvas_view(turned_canvas, camera);
  draw_turned_surfaces();
  if (fitted_document === null || !turned_lines_checkbox.checked) return;
  for (const stroke of fitted_document.strokes) {
    if (stroke.layer === "skull" && !turned_skull_checkbox.checked) continue;
    const control_points = stroke_control_points(stroke, fitted_document);
    const points: V3[] = [];
    for (let i = 0; i <= 24; i++) points.push(bezier_point(control_points, i / 24));
    const style = stroke.layer === "skin" ? SKIN_STROKE_STYLE : SKULL_STROKE_STYLE;
    const line_width = stroke.layer === "skin" ? 1.6 : 1;
    stroke_polyline(view, points, false, style, line_width);
    stroke_polyline(view, points.map(mirror), false, style, line_width);
  }
}


// ---- landmark table -----------------------------------------------------------------
// One row per landmark: how far the fitted point is from its mark, in plate pixels, in
// each view.

function update_landmark_table(landmarks: PlateViewsLandmarks): void {
  if (fitted_document === null) return;
  const tablet_document = fitted_document;
  const tbody = document.querySelector("#landmark_table tbody")!;
  tbody.textContent = "";
  for (const landmark of landmarks.landmarks_file.landmarks) {
    const read_from = landmark.read_from_front && landmark.read_from_profile ? "both views" : landmark.read_from_front ? "front only" : landmark.read_from_profile ? "profile only" : "guess";
    const cells = [landmark.name, read_from];
    const template_position = template_point_world_position(landmarks.template_document, landmark.template_point);
    const fitted = template_point_world_position(tablet_document, landmark.template_point);
    if (template_position === null || fitted === null) cells.push("template point missing", "");
    else {
      const marked = fit_landmark_mark_world_position(landmark, template_position_is_on_midline(template_position));
      const miss_pixels = (pixel_from_world: (p: V3) => { x: number; y: number }): string => {
        const a = pixel_from_world(fitted), b = pixel_from_world(marked);
        return Math.hypot(a.x - b.x, a.y - b.y).toFixed(1);
      };
      cells.push(miss_pixels((p) => front_pixel_from_world(p, landmarks.landmarks_file.front_midline_column_pixels)), miss_pixels(profile_pixel_from_world));
    }
    const row = document.createElement("tr");
    for (const cell of cells) row.appendChild(document.createElement("td")).textContent = cell;
    tbody.appendChild(row);
  }
}

// ---- wiring -------------------------------------------------------------------------

attach_orbit_controls([turned_canvas], camera, draw_turned_view);
turned_skull_checkbox.addEventListener("change", () => { rebuild_turned_surface_mesh(); draw_turned_view(); });
turned_surfaces_checkbox.addEventListener("change", draw_turned_view);
turned_lines_checkbox.addEventListener("change", draw_turned_view);
draw_turned_view();

start_loomis_girl_plate_views(
  [
    { document_name: TEMPLATE_DOCUMENT_NAME, front_canvas_id: "template_front_canvas", profile_canvas_id: "template_profile_canvas" },
    { document_name: FITTED_DOCUMENT_NAME, front_canvas_id: "fitted_front_canvas", profile_canvas_id: "fitted_profile_canvas" },
  ],
  TEMPLATE_DOCUMENT_NAME,
  (document_name, tablet_document, landmarks) => {
    // The template decides which landmarks are midline ones, so its arrival changes the table too.
    if (document_name === FITTED_DOCUMENT_NAME) {
      fitted_document = tablet_document;
      rebuild_turned_surface_mesh();
    }
    draw_turned_view();
    update_landmark_table(landmarks);
  },
);
