// Sphere primitives (Sphere in document.ts): a centre vertex and a radius. A sphere is
// drawn as one more surface grid, like a patch's fill; it has no strokes and no patch,
// so it is not picked, pinned to or drawn on.
import { Sphere, TabletDocument, vertex_by_id, vertex_is_on_midline, vertex_world_position } from "./document";
import { v3 } from "./math";
import { SurfaceGrid, mirrored_surface_grid } from "./patch";
import { Rgb } from "./vertex_sink";

export const SPHERE_COLOR: Rgb = { r: 0.95, g: 0.95, b: 0.95 };

const SPHERE_COLUMNS = 24; // around the front-back axis
const SPHERE_ROWS = 12; // from the front pole to the back pole

// The poles are on the front-back (z) axis: an eye is looked at from the front, where a
// pole's thin triangles sit in the middle of the ball, not on its edge.
export function sphere_surface_grid(sphere: Sphere, tablet_document: TabletDocument): SurfaceGrid {
  const center = vertex_world_position(tablet_document, vertex_by_id(tablet_document, sphere.center_vertex));
  const positions = [];
  for (let i = 0; i <= SPHERE_COLUMNS; i++) {
    // The last column is the first again, exactly, so the seam is closed.
    const around = ((i % SPHERE_COLUMNS) / SPHERE_COLUMNS) * Math.PI * 2;
    const column = [];
    for (let j = 0; j <= SPHERE_ROWS; j++) {
      const from_front = (j / SPHERE_ROWS) * Math.PI;
      const ring_radius = sphere.radius * Math.sin(from_front);
      column.push(v3(
        center.x + ring_radius * Math.cos(around),
        center.y + ring_radius * Math.sin(around),
        center.z + sphere.radius * Math.cos(from_front),
      ));
    }
    positions.push(column);
  }
  return { columns: SPHERE_COLUMNS, rows: SPHERE_ROWS, positions };
}

// The grids a sphere is drawn as: itself, and its mirror image when the other side of
// the head is drawn (`mirrored_side_drawn`) and its centre is not on the midline.
export function sphere_drawn_grids(sphere: Sphere, tablet_document: TabletDocument, mirrored_side_drawn: boolean): SurfaceGrid[] {
  const grid = sphere_surface_grid(sphere, tablet_document);
  if (!mirrored_side_drawn) return [grid];
  if (vertex_is_on_midline(tablet_document, sphere.center_vertex)) return [grid];
  return [grid, mirrored_surface_grid(grid)];
}
