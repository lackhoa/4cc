// Reference model (plan-skull-reference.md): an OBJ mesh fetched from the dev
// server, shown dimmed as a drawing reference. Purely visual — nothing snaps
// to it. Parsing handles only v/f lines (positions + faces, fan-triangulated);
// normals/uvs/materials are ignored.

import { OrbitCamera, camera_basis } from "./camera";
import { V3, v3, v3_cross, v3_dot, v3_length, v3_lerp, v3_normalize, v3_sub } from "./math";
import { VertexSink, push_vertex } from "./vertex_sink";

const REFERENCE_AMBIENT = 0.2; // dimmer than surfaces (LOFT_AMBIENT 0.35)
const REFERENCE_COLOR = { r: 0.5, g: 0.48, b: 0.46 };
const REFERENCE_HEIGHT_WORLD_UNITS = 2;

// Flat per-triangle storage: positions has 3 entries per triangle, normals 1.
export type ReferenceMesh = { triangle_positions: V3[]; triangle_normals: V3[] };

// The OBJ as stored: unique vertices in the file's own units (mm for the Z-Anatomy
// set), triangle corners as 0-based vertex indices, 3 per triangle. This is what the
// construction pages fit shapes to (plan-skull-construction-docs.md).
export type RawObjMesh = { positions: V3[]; triangle_indices: number[] };

// Returns null when the text yields no triangles (e.g. an error page served
// instead of an OBJ).
export function parse_obj_mesh_raw(obj_text: string): RawObjMesh | null {
  const positions: V3[] = [];
  const triangle_indices: number[] = [];
  for (const line of obj_text.split("\n")) {
    const parts = line.trim().split(/\s+/);
    if (parts[0] === "v") {
      positions.push(v3(parseFloat(parts[1]), parseFloat(parts[2]), parseFloat(parts[3])));
    } else if (parts[0] === "f") {
      // "f 5/1/2 6/2/2 ..." — vertex index is the part before the first slash, 1-based.
      const corner_indices = parts.slice(1).map((part) => parseInt(part.split("/")[0], 10) - 1);
      for (let i = 2; i < corner_indices.length; i++) { // fan-triangulate quads/ngons
        triangle_indices.push(corner_indices[0], corner_indices[i - 1], corner_indices[i]);
      }
    }
  }
  if (triangle_indices.length === 0) {
    console.error("OBJ parse produced no triangles");
    return null;
  }
  return { positions, triangle_indices };
}

export function parse_obj_mesh(obj_text: string): ReferenceMesh | null {
  const raw = parse_obj_mesh_raw(obj_text);
  if (raw === null) return null;
  const triangle_positions = raw.triangle_indices.map((index) => raw.positions[index]);
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

// A reference mesh from positions already in world units (no normalization), e.g. the
// Z-Anatomy skull in Frankfurt-frame mm * WORLD_PER_MM. Several meshes (skull + mandible)
// concatenate by calling this once each and joining the arrays.
export function reference_mesh_from_positions(positions_world: V3[], triangle_indices: number[]): ReferenceMesh {
  const mesh: ReferenceMesh = { triangle_positions: triangle_indices.map((index) => positions_world[index]), triangle_normals: [] };
  compute_triangle_normals(mesh);
  return mesh;
}

function compute_triangle_normals(mesh: ReferenceMesh): void {
  for (let i = 0; i < mesh.triangle_positions.length; i += 3) {
    const edge_ab = v3_sub(mesh.triangle_positions[i + 1], mesh.triangle_positions[i]);
    const edge_ac = v3_sub(mesh.triangle_positions[i + 2], mesh.triangle_positions[i]);
    const cross = v3_cross(edge_ab, edge_ac);
    mesh.triangle_normals.push(v3_length(cross) > 1e-12 ? v3_normalize(cross) : v3(0, 1, 0));
  }
}

// Text of one file under /reference/ (an .obj or a .landmarks.txt); null on any failure.
export async function fetch_reference_text(url: string): Promise<string | null> {
  try {
    const response = await fetch(url);
    if (!response.ok) {
      console.error(`reference fetch failed: ${url} -> ${response.status}`);
      return null;
    }
    return await response.text();
  } catch (error) {
    console.error(`reference fetch failed: ${url}`, error);
    return null;
  }
}

export async function fetch_reference_mesh(url: string): Promise<ReferenceMesh | null> {
  const text = await fetch_reference_text(url);
  return text === null ? null : parse_obj_mesh(text);
}

// Landmarks (game_reference_landmarks.cpp): labeled points in the mesh's own coordinates,
// stored beside the .obj as `<mesh>.landmarks.txt`, written by the desktop app as
//   landmarks = [ {name = "porion_l", p = {-51.3, 4.6, -19.0}}, ... ]
export type Landmark = { name: string; p: V3 };

export function parse_landmarks_file(text: string): Landmark[] {
  const landmarks: Landmark[] = [];
  const entry = /\{\s*name\s*=\s*"([^"]*)"\s*,\s*p\s*=\s*\{\s*([^,}]+),\s*([^,}]+),\s*([^,}]+)\}\s*\}/g;
  for (const match of text.matchAll(entry)) {
    landmarks.push({ name: match[1], p: v3(parseFloat(match[2]), parseFloat(match[3]), parseFloat(match[4])) });
  }
  return landmarks;
}

export function find_landmark(landmarks: Landmark[], name: string): V3 | null {
  const landmark = landmarks.find((candidate) => candidate.name === name);
  return landmark === undefined ? null : landmark.p;
}

// The Frankfurt frame (reference_frankfurt_frame in C++): origin at the porion middle,
// side = porion_l -> porion_r, up = normal of the plane through the porions and the
// orbitale, front = side x up. Unit axes in mesh coordinates.
export type FrankfurtFrame = { porion_middle: V3; side: V3; up: V3; front: V3 };

// null when a landmark is missing or the three are (nearly) collinear.
export function frankfurt_frame_from_landmarks(landmarks: Landmark[]): FrankfurtFrame | null {
  const porion_l = find_landmark(landmarks, "porion_l");
  const porion_r = find_landmark(landmarks, "porion_r");
  const orbitale = find_landmark(landmarks, "orbitale");
  if (porion_l === null || porion_r === null || orbitale === null) return null;
  const porion_middle = v3_lerp(porion_l, porion_r, 0.5);
  const side_unnormalized = v3_sub(porion_r, porion_l);
  const up_unnormalized = v3_cross(v3_sub(orbitale, porion_middle), side_unnormalized);
  if (v3_length(side_unnormalized) < 0.5 || v3_length(up_unnormalized) < 0.5) return null;
  const side = v3_normalize(side_unnormalized);
  const up = v3_normalize(up_unnormalized);
  return { porion_middle, side, up, front: v3_cross(side, up) };
}

// Mesh point -> (side, up, front) coordinates in mm from the porion middle.
export function frankfurt_coordinates(frame: FrankfurtFrame, point: V3): V3 {
  const q = v3_sub(point, frame.porion_middle);
  return v3(v3_dot(q, frame.side), v3_dot(q, frame.up), v3_dot(q, frame.front));
}

// Inverse of frankfurt_coordinates.
export function frankfurt_to_mesh(frame: FrankfurtFrame, coordinates: V3): V3 {
  return v3(
    frame.porion_middle.x + coordinates.x * frame.side.x + coordinates.y * frame.up.x + coordinates.z * frame.front.x,
    frame.porion_middle.y + coordinates.x * frame.side.y + coordinates.y * frame.up.y + coordinates.z * frame.front.y,
    frame.porion_middle.z + coordinates.x * frame.side.z + coordinates.y * frame.up.z + coordinates.z * frame.front.z,
  );
}

// Flat-shaded headlight, same scheme as surfaces but dimmer (two-sided).
export function append_reference_mesh(mesh: ReferenceMesh, camera: OrbitCamera, out: VertexSink): void {
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
      push_vertex(out, position, { r, g, b });
    }
  }
}
