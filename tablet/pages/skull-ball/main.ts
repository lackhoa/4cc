// Document `skull-ball` (plan-skull-construction-docs.md Q4/Q5): the Z-Anatomy skull drawn
// translucent, three candidate balls fitted to it (Frankfurt sphere, cranium sphere,
// cranium ellipsoid), each with a heat map of the misses and a row in the numbers table.
// World units = Frankfurt-frame mm * WORLD_PER_MM, x = side, y = up, z = front.
import "../../pages.css";
import { OrbitCamera, camera_eye, camera_pen_ray, camera_view_projection } from "../../src/camera";
import { V3, v3, v3_add, v3_cross, v3_dot, v3_length, v3_normalize, v3_scale, v3_sub } from "../../src/math";
import { attach_orbit_controls, bind_controls } from "../../src/explainer/orbit_controls";
import { CanvasView, canvas_view, stroke_polyline } from "../../src/explainer/canvas_view";
import { TranslucentMesh, create_translucent_mesh, draw_mesh_translucent, set_translucent_mesh } from "../../src/render";
import { FrankfurtFrame, Landmark, fetch_reference_text, find_landmark, frankfurt_coordinates, frankfurt_frame_from_landmarks, frankfurt_to_mesh, parse_landmarks_file, parse_obj_mesh_raw } from "../../src/reference";
import { EllipsoidFit, ResidualStats, SphereFit, ellipsoid_residual, fit_ellipsoid_algebraic, fit_sphere_algebraic, residual_stats, sphere_residual } from "../../src/construction_fit";

const WORLD_PER_MM = 0.01;
const SKULL_NAME = "z-anatomy-head-skull";
const LANDMARKS_URL = `/reference/${SKULL_NAME}.landmarks.txt`;

const colors = {
  bone: v3(0.85, 0.8, 0.7),
  shape: v3(1.0, 0.82, 0.4),
  inside: v3(0.3, 0.5, 1.0),
  zero: v3(0.85, 0.85, 0.85),
  outside: v3(1.0, 0.35, 0.3),
  frankfurt_plane: v3(0.3, 0.83, 0.75),
  cranium_plane: v3(1.0, 0.66, 0.3),
  landmark: v3(1.0, 0.82, 0.4),
};

// The skull in Frankfurt-frame mm: what every fit and every drawing on this page uses.
type Skull = {
  frame: FrankfurtFrame;
  landmarks: Landmark[];
  positions: V3[]; // frame mm
  triangle_indices: number[];
  vertex_normals: V3[]; // area-weighted, unit, frame space
};

type CandidateId = "frankfurt-sphere" | "cranium-sphere" | "cranium-ellipsoid";
type CandidateShape = { kind: "sphere"; fit: SphereFit } | { kind: "ellipsoid"; fit: EllipsoidFit };
type Candidate = {
  id: CandidateId;
  cut_normal: V3; // vault = vertices with dot(p, cut_normal) > 0 (plane through the origin)
  vault_count: number;
  shape: CandidateShape;
  residuals: number[]; // per skull vertex, mm, + = outside
  stats: ResidualStats; // over the vault only
};

type Pane = {
  gl: WebGLRenderingContext;
  canvas: HTMLCanvasElement;
  overlay: HTMLCanvasElement;
  mesh: TranslucentMesh; // skull + shape (+ planes, landmarks) in one sorted mesh
  candidate_id: CandidateId | null; // null = the glossary pane
};

const camera: OrbitCamera = { pivot: v3(0, 0.45, 0), yaw: 0.6, pitch: 0.15, distance: 3.2 };
let skull: Skull | null = null;
let glabella: V3 = v3(0, 30, 90); // frame mm; replaced by the file's landmark or the guess
let candidates: Candidate[] = [];
const panes: Pane[] = [];

// ---- loading ------------------------------------------------------------------------

function compute_vertex_normals(positions: V3[], triangle_indices: number[]): V3[] {
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

// The most forward midline vertex in the brow band, used until the glabella is picked.
function guess_glabella(positions: V3[]): V3 {
  let best = positions[0];
  for (const p of positions) {
    if (Math.abs(p.x) < 4 && p.y > 25 && p.y < 55 && p.z > best.z) best = p;
  }
  return best;
}

async function load_skull(): Promise<Skull | null> {
  const [obj_text, landmarks_text] = await Promise.all([
    fetch_reference_text(`/reference/${SKULL_NAME}.obj`),
    fetch_reference_text(LANDMARKS_URL),
  ]);
  if (obj_text === null || landmarks_text === null) return null;
  const raw = parse_obj_mesh_raw(obj_text);
  const landmarks = parse_landmarks_file(landmarks_text);
  const frame = frankfurt_frame_from_landmarks(landmarks);
  if (raw === null || frame === null) {
    console.error("skull-ball: mesh or Frankfurt landmarks missing", { raw, landmarks });
    return null;
  }
  const positions = raw.positions.map((p) => frankfurt_coordinates(frame, p));
  return { frame, landmarks, positions, triangle_indices: raw.triangle_indices, vertex_normals: compute_vertex_normals(positions, raw.triangle_indices) };
}

// ---- fits ---------------------------------------------------------------------------

function fit_candidates(skull: Skull, glabella: V3): Candidate[] {
  const frankfurt_up = v3(0, 1, 0);
  // Plane through the side axis and the glabella; normal chosen to point up.
  const cranium_normal = v3_normalize(v3(0, glabella.z, -glabella.y));
  const vault_of = (normal: V3) => skull.positions.filter((p) => v3_dot(p, normal) > 0);
  const make = (id: CandidateId, cut_normal: V3, shape: CandidateShape): Candidate => {
    const residual = shape.kind === "sphere"
      ? (p: V3) => sphere_residual(shape.fit, p)
      : (p: V3) => ellipsoid_residual(shape.fit, p);
    const residuals = skull.positions.map(residual);
    const vault_residuals = skull.positions.filter((p) => v3_dot(p, cut_normal) > 0).map(residual);
    return { id, cut_normal, vault_count: vault_residuals.length, shape, residuals, stats: residual_stats(vault_residuals) };
  };
  const result: Candidate[] = [];
  const frankfurt_sphere = fit_sphere_algebraic(vault_of(frankfurt_up));
  if (frankfurt_sphere !== null) result.push(make("frankfurt-sphere", frankfurt_up, { kind: "sphere", fit: frankfurt_sphere }));
  const cranium_vault = vault_of(cranium_normal);
  const cranium_sphere = fit_sphere_algebraic(cranium_vault);
  if (cranium_sphere !== null) result.push(make("cranium-sphere", cranium_normal, { kind: "sphere", fit: cranium_sphere }));
  const cranium_ellipsoid = fit_ellipsoid_algebraic(cranium_vault);
  if (cranium_ellipsoid !== null) result.push(make("cranium-ellipsoid", cranium_normal, { kind: "ellipsoid", fit: cranium_ellipsoid }));
  return result;
}

// ---- mesh building ------------------------------------------------------------------

type MeshBuilder = { data: number[] };

function push_triangle_vertex(builder: MeshBuilder, p_mm: V3, color: V3, alpha: number): void {
  builder.data.push(p_mm.x * WORLD_PER_MM, p_mm.y * WORLD_PER_MM, p_mm.z * WORLD_PER_MM, color.x, color.y, color.z, alpha);
}

function heat_color(residual_mm: number, heat_range_mm: number): V3 {
  const t = Math.max(-1, Math.min(1, residual_mm / heat_range_mm));
  const target = t < 0 ? colors.inside : colors.outside;
  const amount = Math.abs(t);
  return v3(colors.zero.x + (target.x - colors.zero.x) * amount, colors.zero.y + (target.y - colors.zero.y) * amount, colors.zero.z + (target.z - colors.zero.z) * amount);
}

function push_skull(builder: MeshBuilder, skull: Skull, candidate: Candidate | null, eye_mm: V3, alpha: number, color_mode: string, heat_range_mm: number): void {
  const ambient = 0.35;
  const vertex_colors = skull.positions.map((p, index) => {
    const base = candidate !== null && color_mode === "heat" ? heat_color(candidate.residuals[index], heat_range_mm) : colors.bone;
    const brightness = ambient + (1 - ambient) * Math.abs(v3_dot(skull.vertex_normals[index], v3_normalize(v3_sub(eye_mm, p))));
    return v3_scale(base, brightness);
  });
  for (const index of skull.triangle_indices) push_triangle_vertex(builder, skull.positions[index], vertex_colors[index], alpha);
}

// Lat-long ellipsoid (a sphere when the semi-axes are equal), flat-lit by the eye.
function push_ellipsoid(builder: MeshBuilder, center: V3, semi_axes: V3, color: V3, alpha: number, eye_mm: V3): void {
  const rings = 18, segments = 36;
  const point = (ring: number, segment: number): V3 => {
    const phi = (ring / rings) * Math.PI, theta = (segment / segments) * 2 * Math.PI;
    return v3(center.x + semi_axes.x * Math.sin(phi) * Math.cos(theta), center.y + semi_axes.y * Math.cos(phi), center.z + semi_axes.z * Math.sin(phi) * Math.sin(theta));
  };
  const push_face = (a: V3, b: V3, c: V3) => {
    const normal = v3_normalize(v3_cross(v3_sub(b, a), v3_sub(c, a)));
    const brightness = 0.4 + 0.6 * Math.abs(v3_dot(normal, v3_normalize(v3_sub(eye_mm, a))));
    const lit = v3_scale(color, brightness);
    push_triangle_vertex(builder, a, lit, alpha); push_triangle_vertex(builder, b, lit, alpha); push_triangle_vertex(builder, c, lit, alpha);
  };
  for (let ring = 0; ring < rings; ring++) {
    for (let segment = 0; segment < segments; segment++) {
      const a = point(ring, segment), b = point(ring + 1, segment), c = point(ring + 1, segment + 1), d = point(ring, segment + 1);
      if (ring > 0) push_face(a, b, d);
      if (ring < rings - 1) push_face(b, c, d);
    }
  }
}

// A plane through the origin with the given unit normal, as a quad spanning the skull.
function push_plane(builder: MeshBuilder, normal: V3, color: V3, alpha: number): void {
  const side = v3(1, 0, 0);
  const along = v3_normalize(v3_cross(normal, side)); // in the plane, pointing front-ish
  const forward = v3_dot(along, v3(0, 0, 1)) < 0 ? v3_scale(along, -1) : along;
  const corner = (s: number, f: number) => v3_add(v3_scale(side, s), v3_scale(forward, f));
  const a = corner(-95, -95), b = corner(95, -95), c = corner(95, 120), d = corner(-95, 120);
  for (const p of [a, b, c, a, c, d]) push_triangle_vertex(builder, p, color, alpha);
}

function push_landmark_marker(builder: MeshBuilder, p: V3, color: V3): void {
  const r = 2.5;
  const tips = [v3(r, 0, 0), v3(-r, 0, 0), v3(0, r, 0), v3(0, -r, 0), v3(0, 0, r), v3(0, 0, -r)].map((t) => v3_add(p, t));
  const faces = [[0, 2, 4], [2, 1, 4], [1, 3, 4], [3, 0, 4], [2, 0, 5], [1, 2, 5], [3, 1, 5], [0, 3, 5]];
  for (const face of faces) for (const index of face) push_triangle_vertex(builder, tips[index], color, 1);
}

function glossary_landmarks(skull: Skull): Landmark[] {
  const in_frame = skull.landmarks
    .filter((landmark) => landmark.name !== "glabella")
    .map((landmark) => ({ name: landmark.name, p: frankfurt_coordinates(skull.frame, landmark.p) }));
  return [...in_frame, { name: "glabella", p: glabella }];
}

function build_pane_mesh(pane: Pane, eye_mm: V3): Float32Array {
  const builder: MeshBuilder = { data: [] };
  if (skull === null) return new Float32Array(0);
  const skull_alpha = Number(controls.skull_alpha.value);
  const shape_alpha = Number(controls.shape_alpha.value);
  const heat_range = Number(controls.heat_range.value);
  const candidate = candidates.find((c) => c.id === pane.candidate_id) ?? null;
  if (candidate !== null) {
    const shape = candidate.shape;
    if (shape.kind === "sphere") push_ellipsoid(builder, shape.fit.center, v3(shape.fit.radius, shape.fit.radius, shape.fit.radius), colors.shape, shape_alpha, eye_mm);
    else push_ellipsoid(builder, shape.fit.center, shape.fit.semi_axes, colors.shape, shape_alpha, eye_mm);
  } else {
    push_plane(builder, v3(0, 1, 0), colors.frankfurt_plane, 0.3);
    const cranium = candidates.find((c) => c.id === "cranium-sphere");
    if (cranium !== undefined) push_plane(builder, cranium.cut_normal, colors.cranium_plane, 0.3);
    for (const landmark of glossary_landmarks(skull)) push_landmark_marker(builder, landmark.p, colors.landmark);
  }
  push_skull(builder, skull, candidate, eye_mm, skull_alpha, color_mode.value, heat_range);
  return new Float32Array(builder.data);
}

// ---- drawing ------------------------------------------------------------------------

function size_gl_canvas(canvas: HTMLCanvasElement): { width: number; height: number } {
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
function sphere_outline(center_mm: V3, radius_mm: number, eye_mm: V3): V3[] | null {
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

function mm_to_world(p: V3): V3 { return v3_scale(p, WORLD_PER_MM); }

function draw_overlay(pane: Pane, view: CanvasView, eye_mm: V3): void {
  if (skull === null) return;
  const candidate = candidates.find((c) => c.id === pane.candidate_id) ?? null;
  if (candidate !== null) {
    if (candidate.shape.kind === "sphere") {
      const outline = sphere_outline(candidate.shape.fit.center, candidate.shape.fit.radius, eye_mm);
      if (outline !== null) stroke_polyline(view, outline.map(mm_to_world), true, "#ffd166", 1.4);
    }
    return;
  }
  const ctx = view.ctx;
  ctx.font = `${12 * view.dpr}px system-ui, sans-serif`;
  ctx.fillStyle = "#ffd166";
  for (const landmark of glossary_landmarks(skull)) {
    const screen = view.project(mm_to_world(landmark.p));
    if (screen !== null) ctx.fillText(landmark.name, screen.x + 6 * view.dpr, screen.y - 6 * view.dpr);
  }
}

function draw_pane(pane: Pane): void {
  const { width, height } = size_gl_canvas(pane.canvas);
  const gl = pane.gl;
  gl.viewport(0, 0, pane.canvas.width, pane.canvas.height);
  gl.clearColor(0.078, 0.086, 0.11, 1);
  gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
  const eye = camera_eye(camera);
  const eye_mm = v3_scale(eye, 1 / WORLD_PER_MM);
  set_translucent_mesh(pane.mesh, build_pane_mesh(pane, eye_mm));
  draw_mesh_translucent(pane.mesh, camera_view_projection(camera, width / height), eye);
  draw_overlay(pane, canvas_view(pane.overlay, camera), eye_mm);
}

function redraw(): void {
  for (const pane of panes) draw_pane(pane);
}

// ---- numbers table ------------------------------------------------------------------

function format_signed(value: number, digits: number): string {
  const text = value.toFixed(digits);
  return value >= 0 ? `+${text}` : text.replace("-", "−");
}

function update_numbers_table(): void {
  for (const candidate of candidates) {
    const row = document.getElementById(`row-${candidate.id}`)!;
    const label = row.firstElementChild!.outerHTML;
    const center = candidate.shape.fit.center;
    const size = candidate.shape.kind === "sphere"
      ? `r ${candidate.shape.fit.radius.toFixed(1)}`
      : `${candidate.shape.fit.semi_axes.x.toFixed(1)} / ${candidate.shape.fit.semi_axes.y.toFixed(1)} / ${candidate.shape.fit.semi_axes.z.toFixed(1)}`;
    const cells = [
      String(candidate.vault_count), center.x.toFixed(1), format_signed(center.y, 1), format_signed(center.z, 1),
      size, candidate.stats.rms.toFixed(1), format_signed(candidate.stats.min, 1), format_signed(candidate.stats.max, 1),
    ];
    row.innerHTML = label + cells.map((cell) => `<td>${cell}</td>`).join("");
  }
  if (skull !== null) {
    const porion_l = find_landmark(skull.landmarks, "porion_l")!, porion_r = find_landmark(skull.landmarks, "porion_r")!;
    document.getElementById("porion_width")!.textContent = v3_length(v3_sub(porion_r, porion_l)).toFixed(1);
  }
}

// ---- glabella: guess, type, pick, save --------------------------------------------------

function write_glabella_to_inputs(): void {
  controls.glabella_side.value = glabella.x.toFixed(1);
  controls.glabella_up.value = glabella.y.toFixed(1);
  controls.glabella_front.value = glabella.z.toFixed(1);
}

function set_glabella(p: V3): void {
  glabella = p;
  write_glabella_to_inputs();
  refit();
}

function refit(): void {
  if (skull === null) return;
  candidates = fit_candidates(skull, glabella);
  update_numbers_table();
  redraw();
}

// Möller–Trumbore; t along the ray, null when the ray misses.
function ray_triangle_distance(origin: V3, direction: V3, a: V3, b: V3, c: V3): number | null {
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
function pick_skull_vertex(skull: Skull, origin_mm: V3, direction: V3): V3 | null {
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

function attach_pick(pane: Pane): void {
  let down: { x: number; y: number } | null = null;
  pane.canvas.addEventListener("pointerdown", (event) => { down = { x: event.clientX, y: event.clientY }; });
  pane.canvas.addEventListener("pointerup", (event) => {
    if (down === null || skull === null || !pick_mode.checked) return;
    const moved = Math.hypot(event.clientX - down.x, event.clientY - down.y);
    down = null;
    if (moved > 4) return;
    const rect = pane.canvas.getBoundingClientRect();
    const ray = camera_pen_ray(camera, { x: event.clientX - rect.left, y: event.clientY - rect.top }, rect.width, rect.height);
    const picked = pick_skull_vertex(skull, v3_scale(ray.origin, 1 / WORLD_PER_MM), ray.direction);
    if (picked !== null) set_glabella(picked);
  });
}

// The landmarks file as the desktop app writes it (game_reference_landmarks.cpp), so it
// can read the glabella back. Existing landmarks keep their mesh-space values.
function landmarks_file_text(landmarks: Landmark[]): string {
  const number = (value: number) => String(Number(value.toPrecision(9)));
  const lines = landmarks.map((landmark) => `  {name = "${landmark.name}", p = {${number(landmark.p.x)}, ${number(landmark.p.y)}, ${number(landmark.p.z)}}},`);
  return `# Reference_Landmark_File -- written by autodraw; zero members are omitted, unknown ones are skipped.\nlandmarks = [\n${lines.join("\n")}\n]\n`;
}

async function save_glabella(): Promise<void> {
  if (skull === null) return;
  const status = document.getElementById("save_status")!;
  const others = skull.landmarks.filter((landmark) => landmark.name !== "glabella");
  const landmarks = [...others, { name: "glabella", p: frankfurt_to_mesh(skull.frame, glabella) }];
  try {
    const response = await fetch(LANDMARKS_URL, { method: "POST", body: landmarks_file_text(landmarks) });
    if (!response.ok) { status.textContent = `save failed: ${response.status}`; return; }
    skull.landmarks = landmarks;
    status.textContent = `saved glabella into ${SKULL_NAME}.landmarks.txt`;
  } catch (error) {
    status.textContent = `save failed: ${error}`;
  }
}

// ---- wiring -------------------------------------------------------------------------

const color_mode = document.getElementById("color_mode") as HTMLSelectElement;
const pick_mode = document.getElementById("pick_mode") as HTMLInputElement;
const controls = bind_controls(["skull_alpha", "shape_alpha", "heat_range", "glabella_side", "glabella_up", "glabella_front"], () => {
  document.getElementById("heat_range_value")!.textContent = controls.heat_range.value;
  redraw();
});
for (const id of ["glabella_side", "glabella_up", "glabella_front"]) {
  controls[id].addEventListener("change", () => {
    glabella = v3(Number(controls.glabella_side.value), Number(controls.glabella_up.value), Number(controls.glabella_front.value));
    refit();
  });
}
color_mode.addEventListener("change", redraw);
document.getElementById("save_glabella")!.addEventListener("click", () => { void save_glabella(); });
document.getElementById("heat_range_value")!.textContent = controls.heat_range.value;

function make_pane(canvas: HTMLCanvasElement, overlay: HTMLCanvasElement, candidate_id: CandidateId | null): Pane {
  const gl = canvas.getContext("webgl", { premultipliedAlpha: false })!;
  return { gl, canvas, overlay, mesh: create_translucent_mesh(gl), candidate_id };
}
panes.push(make_pane(document.getElementById("glossary_canvas") as HTMLCanvasElement, document.getElementById("glossary_overlay") as HTMLCanvasElement, null));
for (const canvas of Array.from(document.querySelectorAll<HTMLCanvasElement>("canvas.pane"))) {
  const overlay = canvas.nextElementSibling as HTMLCanvasElement;
  panes.push(make_pane(canvas, overlay, canvas.dataset.candidate as CandidateId));
}
attach_orbit_controls(panes.map((pane) => pane.canvas), camera, redraw);
for (const pane of panes) attach_pick(pane);

void load_skull().then((loaded) => {
  if (loaded === null) return;
  skull = loaded;
  const glabella_in_file = find_landmark(skull.landmarks, "glabella");
  glabella = glabella_in_file !== null ? frankfurt_coordinates(skull.frame, glabella_in_file) : guess_glabella(skull.positions);
  write_glabella_to_inputs();
  refit();
});
