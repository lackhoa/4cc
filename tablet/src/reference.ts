// Reference model (plan-skull-reference.md): an OBJ mesh fetched from the dev
// server, shown dimmed as a drawing reference. Purely visual — nothing snaps
// to it. Parsing handles only v/f lines (positions + faces, fan-triangulated);
// normals/uvs/materials are ignored.

import { OrbitCamera, camera_basis } from "./camera";
import { V3, v3, v3_cross, v3_dot, v3_length, v3_normalize, v3_sub } from "./math";

const REFERENCE_AMBIENT = 0.2; // dimmer than surfaces (LOFT_AMBIENT 0.35)
const REFERENCE_COLOR = { r: 0.5, g: 0.48, b: 0.46 };
const REFERENCE_HEIGHT_WORLD_UNITS = 2;

// Flat per-triangle storage: positions has 3 entries per triangle, normals 1.
export type ReferenceMesh = { triangle_positions: V3[]; triangle_normals: V3[] };

// Returns null when the text yields no triangles (e.g. an error page served
// instead of an OBJ).
export function parse_obj_mesh(obj_text: string): ReferenceMesh | null {
  const positions: V3[] = [];
  const triangle_positions: V3[] = [];
  for (const line of obj_text.split("\n")) {
    const parts = line.trim().split(/\s+/);
    if (parts[0] === "v") {
      positions.push(v3(parseFloat(parts[1]), parseFloat(parts[2]), parseFloat(parts[3])));
    } else if (parts[0] === "f") {
      // "f 5/1/2 6/2/2 ..." — vertex index is the part before the first slash, 1-based.
      const corner_indices = parts.slice(1).map((part) => parseInt(part.split("/")[0], 10) - 1);
      for (let i = 2; i < corner_indices.length; i++) { // fan-triangulate quads/ngons
        triangle_positions.push(
          positions[corner_indices[0]],
          positions[corner_indices[i - 1]],
          positions[corner_indices[i]],
        );
      }
    }
  }
  if (triangle_positions.length === 0) {
    console.error("OBJ parse produced no triangles");
    return null;
  }
  const mesh: ReferenceMesh = { triangle_positions, triangle_normals: [] };
  normalize_reference_mesh(mesh);
  compute_triangle_normals(mesh);
  return mesh;
}

// Center on the origin horizontally, base on the grid plane, height scaled to
// REFERENCE_HEIGHT_WORLD_UNITS — so any model lands orbit-ready with no gizmo.
function normalize_reference_mesh(mesh: ReferenceMesh): void {
  const first = mesh.triangle_positions[0];
  const min = v3(first.x, first.y, first.z);
  const max = v3(first.x, first.y, first.z);
  for (const position of mesh.triangle_positions) {
    min.x = Math.min(min.x, position.x); max.x = Math.max(max.x, position.x);
    min.y = Math.min(min.y, position.y); max.y = Math.max(max.y, position.y);
    min.z = Math.min(min.z, position.z); max.z = Math.max(max.z, position.z);
  }
  const scale = REFERENCE_HEIGHT_WORLD_UNITS / (max.y - min.y);
  const center_x = (min.x + max.x) / 2;
  const center_z = (min.z + max.z) / 2;
  // Positions are shared between triangles (same V3 objects) — dedupe so each
  // is transformed exactly once.
  for (const position of new Set(mesh.triangle_positions)) {
    position.x = (position.x - center_x) * scale;
    position.y = (position.y - min.y) * scale;
    position.z = (position.z - center_z) * scale;
  }
}

function compute_triangle_normals(mesh: ReferenceMesh): void {
  for (let i = 0; i < mesh.triangle_positions.length; i += 3) {
    const edge_ab = v3_sub(mesh.triangle_positions[i + 1], mesh.triangle_positions[i]);
    const edge_ac = v3_sub(mesh.triangle_positions[i + 2], mesh.triangle_positions[i]);
    const cross = v3_cross(edge_ab, edge_ac);
    mesh.triangle_normals.push(v3_length(cross) > 1e-12 ? v3_normalize(cross) : v3(0, 1, 0));
  }
}

export async function fetch_reference_mesh(url: string): Promise<ReferenceMesh | null> {
  try {
    const response = await fetch(url);
    if (!response.ok) {
      console.error(`reference model fetch failed: ${url} -> ${response.status}`);
      return null;
    }
    return parse_obj_mesh(await response.text());
  } catch (error) {
    console.error(`reference model fetch failed: ${url}`, error);
    return null;
  }
}

// Flat-shaded headlight, same scheme as surfaces but dimmer (two-sided).
export function append_reference_mesh(mesh: ReferenceMesh, camera: OrbitCamera, out: number[]): void {
  const camera_forward = camera_basis(camera).forward;
  for (let triangle = 0; triangle < mesh.triangle_normals.length; triangle++) {
    const normal = mesh.triangle_normals[triangle];
    const brightness = REFERENCE_AMBIENT
      + (1 - REFERENCE_AMBIENT) * Math.abs(v3_dot(normal, camera_forward));
    const r = REFERENCE_COLOR.r * brightness;
    const g = REFERENCE_COLOR.g * brightness;
    const b = REFERENCE_COLOR.b * brightness;
    for (let corner = 0; corner < 3; corner++) {
      const position = mesh.triangle_positions[triangle * 3 + corner];
      out.push(position.x, position.y, position.z, r, g, b);
    }
  }
}
