// A document's surfaces and strokes as WebGL meshes with depth: the surfaces of its
// patches hide the strokes behind them, both for the plate views and for the turned head.
// Shading follows the camera, so the meshes are shaded again when it turns. The document
// holds one side of the head; `mirrored` adds the other side.
import { OrbitCamera, camera_eye } from "./camera";
import { TabletDocument, patch_layer, stroke_control_points, stroke_radii } from "./document";
import { Mat4, V3, v3, v3_length, v3_normalize, v3_sub } from "./math";
import { on_surface_stroke_samples, patch_surface_grid } from "./patch";
import { ReferenceMesh, append_reference_mesh, reference_mesh_from_positions } from "./reference";
import { ray_triangle_distance } from "./reference_skull_view";
import { FLOATS_PER_TRANSLUCENT_VERTEX, TranslucentMesh, create_translucent_mesh, draw_mesh_translucent, draw_mesh_translucent_on_top, draw_mesh_translucent_writing_depth, set_translucent_mesh } from "./render";
import { TaperWindow, append_bezier_ribbon, append_sampled_ribbon } from "./ribbon";
import { FLOATS_PER_VERTEX, Rgb, VertexSink, create_vertex_sink, reset_vertex_sink } from "./vertex_sink";

export const SURFACE_COLOR: Rgb = { r: 0.45, g: 0.55, b: 0.7 };

// How much of the surfaces is pushed back so the strokes on them win the depth test.
const SURFACE_DEPTH_OFFSET = 4;

export type HeadMeshes = {
  surfaces: TranslucentMesh;
  strokes: TranslucentMesh;
  markers: TranslucentMesh; // drawn last, on top of everything unless `markers_depth_tested`
  markers_depth_tested: boolean; // from the options of the last update
  shaded_vertices: VertexSink; // scratch for the shading pass
  surface_triangles: ReferenceMesh | null; // the patches as triangles, shaded per camera
  shaded_for: string; // document, camera and options the uploaded meshes were built for
};

export type HeadMeshOptions = {
  skull_visible: boolean;
  surfaces_visible: boolean;
  strokes_visible: boolean;
  mirrored: boolean;
  surface_opacity: number;
  skin_stroke_color: Rgb;
  skull_stroke_color: Rgb;
  // Points marked on the head itself (landmarks), drawn as small octahedra on top of the
  // surfaces and strokes, mirrored with them. World units.
  markers?: { position: V3; color: Rgb; radius: number }[];
  // Markers hidden by the surfaces in front of them (the head model pane, plan
  // plan-hairline-reference-plate Q14); off, they show through everything (the plate
  // views: a marker under a translucent surface must stay visible).
  markers_depth_tested?: boolean;
};

export function create_head_meshes(gl: WebGLRenderingContext): HeadMeshes {
  return {
    surfaces: create_translucent_mesh(gl),
    strokes: create_translucent_mesh(gl),
    markers: create_translucent_mesh(gl),
    markers_depth_tested: false,
    shaded_vertices: create_vertex_sink(1024),
    surface_triangles: null,
    shaded_for: "",
  };
}

const mirror = (p: V3): V3 => v3(-p.x, p.y, p.z);
const WHOLE_STROKE: TaperWindow = { start: 0, end: 1 };

// Every visible patch as triangles, on one or both sides of the head.
function surface_triangles_of(tablet_document: TabletDocument, options: HeadMeshOptions): ReferenceMesh {
  const positions: V3[] = [];
  for (const patch of tablet_document.patches) {
    if (patch_layer(patch, tablet_document) === "skull" && !options.skull_visible) continue;
    const grid = patch_surface_grid(patch, tablet_document);
    if (grid === null) continue;
    for (let j = 0; j < grid.rows; j++) {
      for (let i = 0; i < grid.columns; i++) {
        const point_00 = grid.positions[i][j];
        const point_10 = grid.positions[i + 1][j];
        const point_11 = grid.positions[i + 1][j + 1];
        const point_01 = grid.positions[i][j + 1];
        const cell_triangles = [point_00, point_10, point_11, point_00, point_11, point_01];
        positions.push(...cell_triangles);
        if (options.mirrored) positions.push(...cell_triangles.map(mirror));
      }
    }
  }
  const triangle_colors = Array.from({ length: positions.length / 3 }, () => SURFACE_COLOR);
  return reference_mesh_from_positions(positions, positions.map((_, index) => index), triangle_colors);
}

// The markers as octahedra (six tips along the axes), one or both sides of the head.
function marker_triangles_of(options: HeadMeshOptions): ReferenceMesh | null {
  const markers = options.markers ?? [];
  if (markers.length === 0) return null;
  const positions: V3[] = [];
  const triangle_colors: Rgb[] = [];
  const faces = [[0, 2, 4], [2, 1, 4], [1, 3, 4], [3, 0, 4], [2, 0, 5], [1, 2, 5], [3, 1, 5], [0, 3, 5]];
  for (const marker of markers) {
    const p = marker.position, r = marker.radius;
    const tips = [v3(p.x + r, p.y, p.z), v3(p.x - r, p.y, p.z), v3(p.x, p.y + r, p.z), v3(p.x, p.y - r, p.z), v3(p.x, p.y, p.z + r), v3(p.x, p.y, p.z - r)];
    for (const face of faces) {
      const triangle = face.map((index) => tips[index]);
      positions.push(...triangle);
      triangle_colors.push(marker.color);
      if (options.mirrored) {
        // Mirroring flips the winding; reversed so the face still points outward.
        positions.push(...triangle.map(mirror).reverse());
        triangle_colors.push(marker.color);
      }
    }
  }
  return reference_mesh_from_positions(positions, positions.map((_, index) => index), triangle_colors);
}

// The shaded [x,y,z, r,g,b] vertices of `sink` with `opacity` appended to each.
function translucent_vertices_of(sink: VertexSink, opacity: number): Float32Array {
  const vertex_count = sink.length / FLOATS_PER_VERTEX;
  const vertices = new Float32Array(vertex_count * FLOATS_PER_TRANSLUCENT_VERTEX);
  for (let vertex = 0; vertex < vertex_count; vertex++) {
    const source = vertex * FLOATS_PER_VERTEX;
    const target = vertex * FLOATS_PER_TRANSLUCENT_VERTEX;
    for (let i = 0; i < FLOATS_PER_VERTEX; i++) vertices[target + i] = sink.data[source + i];
    vertices[target + FLOATS_PER_VERTEX] = opacity;
  }
  return vertices;
}

// Rebuilds the patch triangles when the document or the options changed (`document_revision`
// names the document's state: any string that changes when it does).
export function update_head_meshes(meshes: HeadMeshes, tablet_document: TabletDocument, document_revision: string, camera: OrbitCamera, options: HeadMeshOptions): void {
  const built_for = `${document_revision} ${JSON.stringify(options)}`;
  const shaded_for = `${built_for} ${camera.yaw} ${camera.pitch}`;
  if (shaded_for === meshes.shaded_for) return;
  if (meshes.surface_triangles === null || !meshes.shaded_for.startsWith(built_for + " ")) {
    meshes.surface_triangles = surface_triangles_of(tablet_document, options);
  }
  reset_vertex_sink(meshes.shaded_vertices);
  if (options.surfaces_visible) append_reference_mesh(meshes.surface_triangles, camera, meshes.shaded_vertices);
  set_translucent_mesh(meshes.surfaces, translucent_vertices_of(meshes.shaded_vertices, options.surface_opacity));

  reset_vertex_sink(meshes.shaded_vertices);
  if (options.strokes_visible) {
    for (const stroke of tablet_document.strokes) {
      if (stroke.layer === "skull" && !options.skull_visible) continue;
      const color = stroke.layer === "skin" ? options.skin_stroke_color : options.skull_stroke_color;
      const radii = stroke_radii(stroke);
      if (stroke.on_surface === true) {
        // Drawn through its samples projected onto the surface: the chord between its
        // ends would run inside the head and be hidden by the skin.
        const centers = on_surface_stroke_samples(stroke, tablet_document);
        append_sampled_ribbon(centers, radii, camera, color, meshes.shaded_vertices);
        if (options.mirrored) append_sampled_ribbon(centers.map(mirror), radii, camera, color, meshes.shaded_vertices);
        continue;
      }
      const points = stroke_control_points(stroke, tablet_document);
      append_bezier_ribbon(points, radii, WHOLE_STROKE, camera, color, meshes.shaded_vertices);
      if (options.mirrored) {
        const mirrored_points = { p0: mirror(points.p0), p1: mirror(points.p1), p2: mirror(points.p2), p3: mirror(points.p3) };
        append_bezier_ribbon(mirrored_points, radii, WHOLE_STROKE, camera, color, meshes.shaded_vertices);
      }
    }
  }
  set_translucent_mesh(meshes.strokes, translucent_vertices_of(meshes.shaded_vertices, 1));

  reset_vertex_sink(meshes.shaded_vertices);
  const marker_triangles = marker_triangles_of(options);
  if (marker_triangles !== null) append_reference_mesh(marker_triangles, camera, meshes.shaded_vertices);
  set_translucent_mesh(meshes.markers, translucent_vertices_of(meshes.shaded_vertices, 1));
  meshes.markers_depth_tested = options.markers_depth_tested ?? false;
  meshes.shaded_for = shaded_for;
}

// Surfaces first, then the strokes depth-tested against them, then the markers: on top of
// both by default (a landmark vertex sits on or under a surface, so depth-tested it would
// be hidden), or depth-tested (`markers_depth_tested`): a marker on the surface still
// shows, its near half sticks out of it by its radius, one behind the head is hidden.
export function draw_head_meshes(meshes: HeadMeshes, view_projection: Mat4, camera: OrbitCamera): void {
  const eye = camera_eye(camera);
  draw_mesh_translucent_writing_depth(meshes.surfaces, view_projection, eye, SURFACE_DEPTH_OFFSET);
  draw_mesh_translucent_writing_depth(meshes.strokes, view_projection, eye, 0);
  if (meshes.markers_depth_tested) draw_mesh_translucent(meshes.markers, view_projection, eye);
  else draw_mesh_translucent_on_top(meshes.markers, view_projection, eye);
}

// Whether `position` is hidden from `eye` by a surface of the last `update_head_meshes`
// (both sides of the head when mirrored): some surface triangle is on the ray from the
// eye to the point, nearer than the point by more than `tolerance` world units (a point
// ON a surface is not hidden by it).
export function head_surface_hides_point(meshes: HeadMeshes, eye: V3, position: V3, tolerance: number): boolean {
  const triangles = meshes.surface_triangles;
  if (triangles === null) return false;
  const to_point = v3_sub(position, eye);
  const point_distance = v3_length(to_point);
  const direction = v3_normalize(to_point);
  const positions = triangles.triangle_positions;
  for (let i = 0; i < positions.length; i += 3) {
    const t = ray_triangle_distance(eye, direction, positions[i], positions[i + 1], positions[i + 2]);
    if (t !== null && t < point_distance - tolerance) return true;
  }
  return false;
}
