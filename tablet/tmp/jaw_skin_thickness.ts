// Mandible-to-skin distance of the Z-Anatomy head (plan-jaw-skin-thickness-check.md):
// table 1 = soft tissue depth at the forensic mandible landmarks + the midline chin
// profile (walk out along the mandible's vertex normal, first skin triangle hit; plus the
// nearest skin vertex as a sanity check); table 2 = the drawn `skull` layer strokes of
// documents/skull-zanatomy.json near the mandible: distance to the mandible mesh and to
// the skin mesh. All in mm, Frankfurt frame (x right, y up, z front).
// Run: npx tsx tmp/jaw_skin_thickness.ts
import { readFileSync } from "node:fs";
import { frankfurt_coordinates, frankfurt_frame_from_landmarks, parse_landmarks_file, parse_obj_mesh_raw } from "../src/reference";
import { V3, v3_add, v3_cross, v3_dot, v3_normalize, v3_scale, v3_sub } from "../src/math";
import { bezier_point } from "../src/document";
import { WORLD_PER_MM } from "../src/reference_skull_view";

const models = "../data/reference-models/";
const skull_landmarks = parse_landmarks_file(readFileSync(models + "z-anatomy-head-skull.landmarks.txt", "utf8"));
const frame = frankfurt_frame_from_landmarks(skull_landmarks)!;

type Mesh = { positions: V3[]; triangle_indices: number[]; vertex_normals: V3[] };

function load_mesh(file_name: string): Mesh {
  const raw = parse_obj_mesh_raw(readFileSync(models + file_name, "utf8"))!;
  const positions = raw.positions.map((p) => frankfurt_coordinates(frame, p));
  const vertex_normals: V3[] = positions.map(() => ({ x: 0, y: 0, z: 0 }));
  for (let i = 0; i < raw.triangle_indices.length; i += 3) {
    const [a, b, c] = [raw.triangle_indices[i], raw.triangle_indices[i + 1], raw.triangle_indices[i + 2]];
    const n = v3_cross(v3_sub(positions[b], positions[a]), v3_sub(positions[c], positions[a])); // area-weighted
    for (const k of [a, b, c]) vertex_normals[k] = v3_add(vertex_normals[k], n);
  }
  return { positions, triangle_indices: raw.triangle_indices, vertex_normals: vertex_normals.map(v3_normalize) };
}

const mandible = load_mesh("z-anatomy-head-mandible.obj");
const skin = load_mesh("z-anatomy-head-skin_head.obj");
const mandible_landmarks = parse_landmarks_file(readFileSync(models + "z-anatomy-head-mandible.landmarks.txt", "utf8"))
  .map((l) => ({ name: l.name, p: frankfurt_coordinates(frame, l.p) }));
console.log("mandible vertices", mandible.positions.length, "skin vertices", skin.positions.length);

function nearest_vertex(mesh: Mesh, p: V3): { index: number; distance: number } {
  let best = 0, best_d2 = Infinity;
  for (let i = 0; i < mesh.positions.length; i++) {
    const d = v3_sub(mesh.positions[i], p);
    const d2 = v3_dot(d, d);
    if (d2 < best_d2) { best_d2 = d2; best = i; }
  }
  return { index: best, distance: Math.sqrt(best_d2) };
}

// Moller-Trumbore; null when the ray misses. Returns the distance along the unit direction.
function ray_triangle_distance(origin: V3, direction: V3, a: V3, b: V3, c: V3): number | null {
  const e1 = v3_sub(b, a), e2 = v3_sub(c, a);
  const h = v3_cross(direction, e2);
  const det = v3_dot(e1, h);
  if (Math.abs(det) < 1e-9) return null;
  const f = 1 / det;
  const s = v3_sub(origin, a);
  const u = f * v3_dot(s, h);
  if (u < 0 || u > 1) return null;
  const q = v3_cross(s, e1);
  const v = f * v3_dot(direction, q);
  if (v < 0 || u + v > 1) return null;
  const t = f * v3_dot(e2, q);
  return t > 1e-6 ? t : null;
}

function ray_mesh_distance(mesh: Mesh, origin: V3, direction: V3): number | null {
  let best: number | null = null;
  for (let i = 0; i < mesh.triangle_indices.length; i += 3) {
    const t = ray_triangle_distance(origin, direction, mesh.positions[mesh.triangle_indices[i]],
      mesh.positions[mesh.triangle_indices[i + 1]], mesh.positions[mesh.triangle_indices[i + 2]]);
    if (t !== null && (best === null || t < best)) best = t;
  }
  return best;
}

// The mandible vertex nearest `p`, with an outward normal (the mandible is a closed-ish
// shell around its own centroid, so outward = away from the centroid).
let mandible_centroid = { x: 0, y: 0, z: 0 };
for (const p of mandible.positions) mandible_centroid = v3_add(mandible_centroid, p);
mandible_centroid = v3_scale(mandible_centroid, 1 / mandible.positions.length);
function mandible_vertex_with_outward_normal(p: V3): { position: V3; normal: V3 } {
  const { index } = nearest_vertex(mandible, p);
  const position = mandible.positions[index];
  let normal = mandible.vertex_normals[index];
  if (v3_dot(normal, v3_sub(position, mandible_centroid)) < 0) normal = v3_scale(normal, -1);
  return { position, normal };
}

const fmt = (v: V3) => `(${v.x.toFixed(1)}, ${v.y.toFixed(1)}, ${v.z.toFixed(1)})`;
const mm = (d: number | null) => d === null ? "miss" : d.toFixed(1);

function depth_row(name: string, p: V3): void {
  const { position, normal } = mandible_vertex_with_outward_normal(p);
  const along_normal = ray_mesh_distance(skin, position, normal);
  const nearest_skin = nearest_vertex(skin, position).distance;
  console.log(`| ${name} | ${fmt(position)} | ${mm(along_normal)} | ${nearest_skin.toFixed(1)} |`);
}

const menton = mandible_landmarks.find((l) => l.name === "menton")!.p;
const gonion_r = mandible_landmarks.find((l) => l.name === "gonion")!.p;
const incisor_lower = mandible_landmarks.find((l) => l.name === "incisor_lower")!.p;
// Pogonion: the most anterior mandible vertex on the midline, between menton and the incisors.
let pogonion = menton;
for (const p of mandible.positions)
  if (Math.abs(p.x) < 4 && p.y > menton.y && p.y < incisor_lower.y && p.z > pogonion.z) pogonion = p;
// Mid-body: the most lateral mandible vertex halfway between pogonion and gonion (in z),
// within 8 mm above the inferior border there (the lateral surface of the body under M1).
function mid_body(side: number): V3 {
  const z_mid = (pogonion.z + gonion_r.z) / 2;
  const slice = mandible.positions.filter((p) => p.x * side > 0 && Math.abs(p.z - z_mid) < 3);
  const y_bottom = Math.min(...slice.map((p) => p.y));
  let best = slice[0];
  for (const p of slice) if (p.y < y_bottom + 8 && Math.abs(p.x) > Math.abs(best.x)) best = p;
  return best;
}

console.log("\n## Table 1: soft tissue depth at the mandible landmarks (mm)\n");
console.log("| landmark | mandible point (x, y, z) | along normal | nearest skin vertex |");
console.log("|---|---|---|---|");
depth_row("pogonion", pogonion);
depth_row("menton", menton);
depth_row("mid-body R", mid_body(1));
depth_row("mid-body L", mid_body(-1));
depth_row("gonion R", gonion_r);
depth_row("gonion L", { x: -gonion_r.x, y: gonion_r.y, z: gonion_r.z });
// Mid-ramus: the most lateral mandible vertex halfway up between gonion and the condyle
// (the masseter sits here, so this is the thickest spot of the jaw in the tables).
const condyle_r = mandible_landmarks.find((l) => l.name === "condyle_r")!.p;
for (const side of [1, -1]) {
  const y_mid = (gonion_r.y + condyle_r.y) / 2;
  let best: V3 | null = null;
  for (const p of mandible.positions)
    if (p.x * side > 0 && Math.abs(p.y - y_mid) < 3 && p.z < 45 && (best === null || Math.abs(p.x) > Math.abs(best.x))) best = p;
  depth_row(`mid-ramus ${side === 1 ? "R" : "L"}`, best!);
}

console.log("\n## Midline chin profile: menton up to the lower incisors, every 5 mm of height\n");
console.log("| height y | mandible point (x, y, z) | along normal | nearest skin vertex |");
console.log("|---|---|---|---|");
for (let y = Math.ceil(menton.y); y <= incisor_lower.y; y += 5) {
  // The most anterior midline mandible vertex at this height = the front edge of the chin.
  let front: V3 | null = null;
  for (const p of mandible.positions) if (Math.abs(p.x) < 2 && Math.abs(p.y - y) < 1.5 && (front === null || p.z > front.z)) front = p;
  if (front !== null) depth_row(`y=${y}`, front);
}

// Table 2: the drawn skull strokes near the mandible. A sample point counts as on the
// mandible when the mandible mesh is within 6 mm of it.
const stored = JSON.parse(readFileSync("documents/skull-zanatomy.json", "utf8")).document;
const vertex_by_id = new Map<number, V3>(stored.vertices.map((v: any) => [v.id, v3_scale(v.position, 1 / WORLD_PER_MM)]));
console.log("\n## Table 2: drawn `skull` layer strokes on the mandible (mm)\n");
console.log("| stroke | t | point (x, y, z) | to mandible mesh | to skin mesh | skin along mandible normal |");
console.log("|---|---|---|---|---|---|");
for (const stroke of stored.strokes) {
  if (stroke.layer !== "skull") continue;
  const p0 = vertex_by_id.get(stroke.p0_vertex)!, p3 = vertex_by_id.get(stroke.p3_vertex)!;
  const d0 = v3_scale(stroke.d0, 1 / WORLD_PER_MM), d3 = v3_scale(stroke.d3, 1 / WORLD_PER_MM);
  const points = {
    p0, p3,
    p1: v3_add(v3_scale(v3_add(v3_scale(p0, 2), p3), 1 / 3), d0),
    p2: v3_add(v3_scale(v3_add(p0, v3_scale(p3, 2)), 1 / 3), d3),
  };
  for (const t of [0, 0.25, 0.5, 0.75, 1]) {
    const p = bezier_point(points, t);
    const to_mandible = nearest_vertex(mandible, p).distance;
    if (to_mandible > 6) continue;
    const { position, normal } = mandible_vertex_with_outward_normal(p);
    const label = stroke.name !== undefined ? `${stroke.id} (${stroke.name})` : `${stroke.id}`;
    console.log(`| ${label} | ${t} | ${fmt(p)} | ${to_mandible.toFixed(1)} | ${nearest_vertex(skin, p).distance.toFixed(1)} | ${mm(ray_mesh_distance(skin, position, normal))} |`);
  }
}
