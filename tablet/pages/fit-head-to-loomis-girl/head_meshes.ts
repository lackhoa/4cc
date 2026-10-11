// A document's surfaces and strokes as WebGL meshes with depth: the surfaces of its
// patches hide the strokes behind them, both for the plate views and for the turned head.
// Shading follows the camera, so the meshes are shaded again when it turns. The document
// holds one side of the head; `mirrored` adds the other side.
import { OrbitCamera, camera_eye } from "../../src/camera";
import { TabletDocument, patch_layer, stroke_control_points, stroke_radii } from "../../src/document";
import { Mat4, V3, v3 } from "../../src/math";
import { patch_surface_grid } from "../../src/patch";
import { ReferenceMesh, append_reference_mesh, reference_mesh_from_positions } from "../../src/reference";
import { FLOATS_PER_TRANSLUCENT_VERTEX, TranslucentMesh, create_translucent_mesh, draw_mesh_translucent_writing_depth, set_translucent_mesh } from "../../src/render";
import { TaperWindow, append_bezier_ribbon } from "../../src/ribbon";
import { FLOATS_PER_VERTEX, Rgb, VertexSink, create_vertex_sink, reset_vertex_sink } from "../../src/vertex_sink";

export const SURFACE_COLOR: Rgb = { r: 0.45, g: 0.55, b: 0.7 };

// How much of the surfaces is pushed back so the strokes on them win the depth test.
const SURFACE_DEPTH_OFFSET = 4;

export type HeadMeshes = {
  surfaces: TranslucentMesh;
  strokes: TranslucentMesh;
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
};

export function create_head_meshes(gl: WebGLRenderingContext): HeadMeshes {
  return {
    surfaces: create_translucent_mesh(gl),
    strokes: create_translucent_mesh(gl),
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
      const points = stroke_control_points(stroke, tablet_document);
      const radii = stroke_radii(stroke);
      append_bezier_ribbon(points, radii, WHOLE_STROKE, camera, color, meshes.shaded_vertices);
      if (options.mirrored) {
        const mirrored_points = { p0: mirror(points.p0), p1: mirror(points.p1), p2: mirror(points.p2), p3: mirror(points.p3) };
        append_bezier_ribbon(mirrored_points, radii, WHOLE_STROKE, camera, color, meshes.shaded_vertices);
      }
    }
  }
  set_translucent_mesh(meshes.strokes, translucent_vertices_of(meshes.shaded_vertices, 1));
  meshes.shaded_for = shaded_for;
}

// Surfaces first, then the strokes depth-tested against them.
export function draw_head_meshes(meshes: HeadMeshes, view_projection: Mat4, camera: OrbitCamera): void {
  const eye = camera_eye(camera);
  draw_mesh_translucent_writing_depth(meshes.surfaces, view_projection, eye, SURFACE_DEPTH_OFFSET);
  draw_mesh_translucent_writing_depth(meshes.strokes, view_projection, eye, 0);
}
