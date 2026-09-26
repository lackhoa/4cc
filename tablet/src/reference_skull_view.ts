// The Z-Anatomy skull as the construction pages see it (plan-skull-construction-docs.md
// Q4/Q5, shared by pages/skull-ball and pages/skull-side-cuts): loaded into Frankfurt-frame
// mm, plus the translucent-mesh builders (heat-mapped skull, fitted shapes, planes, landmark
// markers) and the small drawing helpers the pages share.
// World units = Frankfurt-frame mm * WORLD_PER_MM, x = side, y = up, z = front.
import { V3, v3, v3_add, v3_cross, v3_dot, v3_length, v3_normalize, v3_scale, v3_sub } from "./math";
import { FrankfurtFrame, Landmark, fetch_reference_text, frankfurt_coordinates, frankfurt_frame_from_landmarks, frankfurt_to_mesh, parse_landmarks_file, parse_obj_mesh_raw } from "./reference";

export const WORLD_PER_MM = 0.01;
export const SKULL_NAME = "z-anatomy-head-skull";
export const LANDMARKS_URL = `/reference/${SKULL_NAME}.landmarks.txt`;
export const MANDIBLE_NAME = "z-anatomy-head-mandible";
export const MANDIBLE_LANDMARKS_URL = `/reference/${MANDIBLE_NAME}.landmarks.txt`;

export const skull_view_colors = {
  bone: v3(0.85, 0.8, 0.7),
  shape: v3(1.0, 0.82, 0.4),
  inside: v3(0.3, 0.5, 1.0),
  zero: v3(0.85, 0.85, 0.85),
  outside: v3(1.0, 0.35, 0.3),
  frankfurt_plane: v3(0.3, 0.83, 0.75),
  cranium_plane: v3(1.0, 0.66, 0.3),
  landmark: v3(1.0, 0.82, 0.4),
};

// The skull in Frankfurt-frame mm: what every fit and every drawing on a page uses.
export type Skull = {
  frame: FrankfurtFrame;
  landmarks: Landmark[];
  positions: V3[]; // frame mm
  triangle_indices: number[];
  vertex_normals: V3[]; // area-weighted, unit, frame space
};

export function compute_vertex_normals(positions: V3[], triangle_indices: number[]): V3[] {
  const sums = positions.map(() => v3(0, 0, 0));
  for (let i = 0; i < triangle_indices.length; i += 3) {
    const a = positions[triangle_indices[i]], b = positions[triangle_indices[i + 1]], c = positions[triangle_indices[i + 2]];
    const cross = v3_cross(v3_sub(b, a), v3_sub(c, a)); // length = 2 * area, so this is area-weighted
    for (let corner = 0; corner < 3; corner++) {
      const sum = sums[triangle_indices[i + corner]];
      sum.x += cross.x; sum.y += cross.y; sum.z += cross.z;
    }
  }
  return sums.map((sum) => (v3_length(sum) > 1e-12 ? v3_normalize(sum) : v3(0, 1, 0)));
}

export async function load_skull(page_name: string): Promise<Skull | null> {
  const [obj_text, landmarks_text] = await Promise.all([
    fetch_reference_text(`/reference/${SKULL_NAME}.obj`),
    fetch_reference_text(LANDMARKS_URL),
  ]);
  if (obj_text === null || landmarks_text === null) return null;
  const raw = parse_obj_mesh_raw(obj_text);
  const landmarks = parse_landmarks_file(landmarks_text);
  const frame = frankfurt_frame_from_landmarks(landmarks);
  if (raw === null || frame === null) {
    console.error(`${page_name}: mesh or Frankfurt landmarks missing`, { raw, landmarks });
    return null;
  }
  const positions = raw.positions.map((p) => frankfurt_coordinates(frame, p));
  return { frame, landmarks, positions, triangle_indices: raw.triangle_indices, vertex_normals: compute_vertex_normals(positions, raw.triangle_indices) };
}

// The mandible (a second mesh in the same scan, articulated in the jaw joint, mouth
// closed) in the skull's Frankfurt frame; it has no Frankfurt landmarks of its own, so the
// skull's frame is passed in. Same shape as a Skull so the builders and the picker apply.
export async function load_mandible(page_name: string, frame: FrankfurtFrame): Promise<Skull | null> {
  const [obj_text, landmarks_text] = await Promise.all([
    fetch_reference_text(`/reference/${MANDIBLE_NAME}.obj`),
    fetch_reference_text(MANDIBLE_LANDMARKS_URL),
  ]);
  if (obj_text === null || landmarks_text === null) return null;
  const raw = parse_obj_mesh_raw(obj_text);
  if (raw === null) {
    console.error(`${page_name}: mandible mesh unreadable`);
    return null;
  }
  const positions = raw.positions.map((p) => frankfurt_coordinates(frame, p));
  return { frame, landmarks: parse_landmarks_file(landmarks_text), positions, triangle_indices: raw.triangle_indices, vertex_normals: compute_vertex_normals(positions, raw.triangle_indices) };
}

// ---- picking and saving landmarks ---------------------------------------------------

// Möller–Trumbore; t along the ray, null when the ray misses.
export function ray_triangle_distance(origin: V3, direction: V3, a: V3, b: V3, c: V3): number | null {
  const edge_ab = v3_sub(b, a), edge_ac = v3_sub(c, a);
  const p = v3_cross(direction, edge_ac);
  const determinant = v3_dot(edge_ab, p);
  if (Math.abs(determinant) < 1e-12) return null;
  const inverse = 1 / determinant;
  const to_origin = v3_sub(origin, a);
  const u = v3_dot(to_origin, p) * inverse;
  if (u < 0 || u > 1) return null;
  const q = v3_cross(to_origin, edge_ab);
  const v = v3_dot(direction, q) * inverse;
  if (v < 0 || u + v > 1) return null;
  const t = v3_dot(edge_ac, q) * inverse;
  return t > 0 ? t : null;
}

// The skull vertex nearest to where the ray first enters the mesh; null on a miss.
export function pick_skull_vertex(skull: Skull, origin_mm: V3, direction: V3): V3 | null {
  let best_t = Infinity;
  let best_triangle = -1;
  for (let i = 0; i < skull.triangle_indices.length; i += 3) {
    const t = ray_triangle_distance(origin_mm, direction, skull.positions[skull.triangle_indices[i]], skull.positions[skull.triangle_indices[i + 1]], skull.positions[skull.triangle_indices[i + 2]]);
    if (t !== null && t < best_t) { best_t = t; best_triangle = i; }
  }
  if (best_triangle < 0) return null;
  const hit = v3_add(origin_mm, v3_scale(direction, best_t));
  let best = skull.positions[skull.triangle_indices[best_triangle]];
  for (let corner = 1; corner < 3; corner++) {
    const candidate = skull.positions[skull.triangle_indices[best_triangle + corner]];
    if (v3_length(v3_sub(candidate, hit)) < v3_length(v3_sub(best, hit))) best = candidate;
  }
  return best;
}

// The landmarks file as the desktop app writes it (game_reference_landmarks.cpp), so it
// can read the glabella back. Existing landmarks keep their mesh-space values.
export function landmarks_file_text(landmarks: Landmark[]): string {
  const number = (value: number) => String(Number(value.toPrecision(9)));
  const lines = landmarks.map((landmark) => `  {name = "${landmark.name}", p = {${number(landmark.p.x)}, ${number(landmark.p.y)}, ${number(landmark.p.z)}}},`);
  return `# Reference_Landmark_File -- written by autodraw; zero members are omitted, unknown ones are skipped.\nlandmarks = [\n${lines.join("\n")}\n]\n`;
}

// Replaces (or adds) one landmark in a mesh's sidecar file; `p` in frame mm. Returns the
// status text for the page. On success the in-memory list is updated too.
export async function save_landmark(mesh: Skull, landmarks_url: string, name: string, p: V3): Promise<string> {
  const others = mesh.landmarks.filter((landmark) => landmark.name !== name);
  const landmarks = [...others, { name, p: frankfurt_to_mesh(mesh.frame, p) }];
  try {
    const response = await fetch(landmarks_url, { method: "POST", body: landmarks_file_text(landmarks) });
    if (!response.ok) return `save failed: ${response.status}`;
    mesh.landmarks = landmarks;
    return `saved ${name} into ${landmarks_url.slice("/reference/".length)}`;
  } catch (error) {
    return `save failed: ${error}`;
  }
}

// Plane through the side axis and the glabella (the cranium cut of plan-simplified-skull
// Q8); normal chosen to point up. Vault = vertices with dot(p, normal) > 0.
export function cranium_cut_normal(glabella: V3): V3 {
  return v3_normalize(v3(0, glabella.z, -glabella.y));
}

// The vertices the fits see: above the cranium cut AND on the outer surface. The mesh is
// a closed shell (outer and inner table), and the inner table is a second surface 5-10 mm
// inside the one we draw; outer = normal pointing away from the centroid of the vault.
export function vault_vertices(skull: Skull, cut_normal: V3): V3[] {
  const above_cut = skull.positions.map((p) => v3_dot(p, cut_normal) > 0);
  const centroid = v3(0, 0, 0);
  let count = 0;
  skull.positions.forEach((p, index) => { if (above_cut[index]) { centroid.x += p.x; centroid.y += p.y; centroid.z += p.z; count++; } });
  centroid.x /= count; centroid.y /= count; centroid.z /= count;
  return skull.positions.filter((p, index) => above_cut[index] && v3_dot(skull.vertex_normals[index], v3_sub(p, centroid)) > 0);
}

// The supraorbital margin (the top edge of the eye socket, where the eyebrow sits): through
// the right orbit (side 25..37), coming down from the forehead in 2 mm bands, the most
// forward vertex sits on the brow bone until the band falls into the orbit cavity, where
// it jumps back by tens of mm. The rim is the last brow band before that jump, returned as
// a point in the middle of the side band.
export function supraorbital_rim_point(skull: Skull): V3 | null {
  let previous: { up: number; front: number } | null = null;
  for (let up = 60; up >= 20; up -= 2) {
    let front: number | null = null;
    for (const p of skull.positions) {
      if (p.x >= 25 && p.x <= 37 && p.y >= up && p.y < up + 2 && (front === null || p.z > front)) front = p.z;
    }
    if (front === null) continue;
    if (previous !== null && previous.front - front > 15) return v3(31, previous.up, previous.front);
    previous = { up, front };
  }
  return null;
}

// ---- mesh building ------------------------------------------------------------------

export type MeshBuilder = { data: number[] };

export function push_triangle_vertex(builder: MeshBuilder, p_mm: V3, color: V3, alpha: number): void {
  builder.data.push(p_mm.x * WORLD_PER_MM, p_mm.y * WORLD_PER_MM, p_mm.z * WORLD_PER_MM, color.x, color.y, color.z, alpha);
}

export function heat_color(residual_mm: number, heat_range_mm: number): V3 {
  const colors = skull_view_colors;
  const t = Math.max(-1, Math.min(1, residual_mm / heat_range_mm));
  const target = t < 0 ? colors.inside : colors.outside;
  const amount = Math.abs(t);
  return v3(colors.zero.x + (target.x - colors.zero.x) * amount, colors.zero.y + (target.y - colors.zero.y) * amount, colors.zero.z + (target.z - colors.zero.z) * amount);
}

// The skull, heat-mapped by `residuals` (one per vertex, mm) or bone-colored when null.
export function push_skull(builder: MeshBuilder, skull: Skull, residuals: number[] | null, eye_mm: V3, alpha: number, heat_range_mm: number): void {
  const ambient = 0.35;
  const vertex_colors = skull.positions.map((p, index) => {
    const base = residuals !== null ? heat_color(residuals[index], heat_range_mm) : skull_view_colors.bone;
    const brightness = ambient + (1 - ambient) * Math.abs(v3_dot(skull.vertex_normals[index], v3_normalize(v3_sub(eye_mm, p))));
    return v3_scale(base, brightness);
  });
  for (const index of skull.triangle_indices) push_triangle_vertex(builder, skull.positions[index], vertex_colors[index], alpha);
}

function push_lit_face(builder: MeshBuilder, a: V3, b: V3, c: V3, color: V3, alpha: number, eye_mm: V3): void {
  const normal = v3_normalize(v3_cross(v3_sub(b, a), v3_sub(c, a)));
  const brightness = 0.4 + 0.6 * Math.abs(v3_dot(normal, v3_normalize(v3_sub(eye_mm, a))));
  const lit = v3_scale(color, brightness);
  push_triangle_vertex(builder, a, lit, alpha); push_triangle_vertex(builder, b, lit, alpha); push_triangle_vertex(builder, c, lit, alpha);
}

// Lat-long grid with the poles along `pole_axis` (a unit vector), `point` maps (ring,
// segment) to a position; the ring/segment counts are fixed so callers only supply the shape.
function push_lat_long_grid(builder: MeshBuilder, point: (ring: number, segment: number) => V3, color: V3, alpha: number, eye_mm: V3): void {
  const rings = 18, segments = 36;
  for (let ring = 0; ring < rings; ring++) {
    for (let segment = 0; segment < segments; segment++) {
      const a = point(ring, segment), b = point(ring + 1, segment), c = point(ring + 1, segment + 1), d = point(ring, segment + 1);
      if (ring > 0) push_lit_face(builder, a, b, d, color, alpha, eye_mm);
      if (ring < rings - 1) push_lit_face(builder, b, c, d, color, alpha, eye_mm);
    }
  }
}

// Lat-long ellipsoid (a sphere when the semi-axes are equal), flat-lit by the eye.
export function push_ellipsoid(builder: MeshBuilder, center: V3, semi_axes: V3, color: V3, alpha: number, eye_mm: V3): void {
  const rings = 18, segments = 36;
  push_lat_long_grid(builder, (ring, segment) => {
    const phi = (ring / rings) * Math.PI, theta = (segment / segments) * 2 * Math.PI;
    return v3(center.x + semi_axes.x * Math.sin(phi) * Math.cos(theta), center.y + semi_axes.y * Math.cos(phi), center.z + semi_axes.z * Math.sin(phi) * Math.sin(theta));
  }, color, alpha, eye_mm);
}

// The sphere with the two side slices taken off (planes side = ±half_width, mirrored about
// the midline): a lat-long sphere with its poles along the side axis, every vertex beyond a
// cut plane pushed onto it, which turns the caps into the flat discs.
export function push_sphere_with_side_cuts(builder: MeshBuilder, center: V3, radius: number, half_width: number, color: V3, alpha: number, eye_mm: V3): void {
  const rings = 18, segments = 36;
  push_lat_long_grid(builder, (ring, segment) => {
    const phi = (ring / rings) * Math.PI, theta = (segment / segments) * 2 * Math.PI;
    const x = center.x + radius * Math.cos(phi);
    return v3(Math.max(-half_width, Math.min(half_width, x)), center.y + radius * Math.sin(phi) * Math.cos(theta), center.z + radius * Math.sin(phi) * Math.sin(theta));
  }, color, alpha, eye_mm);
}

// A plane through the origin with the given unit normal, as a quad spanning the skull.
export function push_plane(builder: MeshBuilder, normal: V3, color: V3, alpha: number): void {
  const side = v3(1, 0, 0);
  const along = v3_normalize(v3_cross(normal, side)); // in the plane, pointing front-ish
  const forward = v3_dot(along, v3(0, 0, 1)) < 0 ? v3_scale(along, -1) : along;
  const corner = (s: number, f: number) => v3_add(v3_scale(side, s), v3_scale(forward, f));
  const a = corner(-95, -95), b = corner(95, -95), c = corner(95, 120), d = corner(-95, 120);
  for (const p of [a, b, c, a, c, d]) push_triangle_vertex(builder, p, color, alpha);
}

// A plane side = x (parallel to the midline), as a quad spanning the skull.
export function push_side_plane(builder: MeshBuilder, x: number, color: V3, alpha: number): void {
  const a = v3(x, -40, -100), b = v3(x, -40, 120), c = v3(x, 130, 120), d = v3(x, 130, -100);
  for (const p of [a, b, c, a, c, d]) push_triangle_vertex(builder, p, color, alpha);
}

export function push_landmark_marker(builder: MeshBuilder, p: V3, color: V3): void {
  const r = 2.5;
  const tips = [v3(r, 0, 0), v3(-r, 0, 0), v3(0, r, 0), v3(0, -r, 0), v3(0, 0, r), v3(0, 0, -r)].map((t) => v3_add(p, t));
  const faces = [[0, 2, 4], [2, 1, 4], [1, 3, 4], [3, 0, 4], [2, 0, 5], [1, 2, 5], [3, 1, 5], [0, 3, 5]];
  for (const face of faces) for (const index of face) push_triangle_vertex(builder, tips[index], color, 1);
}

// ---- drawing helpers ----------------------------------------------------------------

export function size_gl_canvas(canvas: HTMLCanvasElement): { width: number; height: number } {
  const dpr = window.devicePixelRatio || 1;
  if (!canvas.dataset.cssHeight) canvas.dataset.cssHeight = canvas.getAttribute("height") ?? "300";
  const css_height = Number(canvas.dataset.cssHeight);
  const css_width = canvas.clientWidth;
  canvas.width = Math.round(css_width * dpr);
  canvas.height = Math.round(css_height * dpr);
  canvas.style.height = `${css_height}px`;
  return { width: css_width, height: css_height };
}

// Silhouette of a sphere under perspective: the ring where the eye's tangent cone touches it.
export function sphere_outline(center_mm: V3, radius_mm: number, eye_mm: V3): V3[] | null {
  const to_eye = v3_sub(eye_mm, center_mm);
  const distance = v3_length(to_eye);
  if (distance <= radius_mm) return null;
  const toward_eye = v3_scale(to_eye, 1 / distance);
  const ring_center = v3_add(center_mm, v3_scale(toward_eye, radius_mm * radius_mm / distance));
  const ring_radius = radius_mm * Math.sqrt(1 - (radius_mm / distance) ** 2);
  const helper = Math.abs(toward_eye.y) < 0.9 ? v3(0, 1, 0) : v3(1, 0, 0);
  const u = v3_normalize(v3_sub(helper, v3_scale(toward_eye, v3_dot(helper, toward_eye))));
  const w = v3_cross(toward_eye, u);
  const points: V3[] = [];
  for (let i = 0; i < 72; i++) {
    const angle = (i / 72) * 2 * Math.PI;
    points.push(v3_add(ring_center, v3_add(v3_scale(u, ring_radius * Math.cos(angle)), v3_scale(w, ring_radius * Math.sin(angle)))));
  }
  return points;
}

// The circle where the plane side = x cuts the sphere (Loomis's side-plane rim); null when
// the plane misses the sphere.
export function side_cut_rim(center_mm: V3, radius_mm: number, x: number): V3[] | null {
  const offset = x - center_mm.x;
  if (Math.abs(offset) >= radius_mm) return null;
  const rim_radius = Math.sqrt(radius_mm * radius_mm - offset * offset);
  const points: V3[] = [];
  for (let i = 0; i < 72; i++) {
    const angle = (i / 72) * 2 * Math.PI;
    points.push(v3(x, center_mm.y + rim_radius * Math.cos(angle), center_mm.z + rim_radius * Math.sin(angle)));
  }
  return points;
}

export function mm_to_world(p: V3): V3 { return v3_scale(p, WORLD_PER_MM); }

export function format_signed(value: number, digits: number): string {
  const text = value.toFixed(digits);
  return value >= 0 ? `+${text}` : text.replace("-", "−");
}
