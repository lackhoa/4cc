// Document `skull-ball` (plan-skull-construction-docs.md Q4/Q5): the Z-Anatomy skull drawn
// translucent, three candidate balls fitted to it (Frankfurt sphere, cranium sphere,
// cranium ellipsoid), each with a heat map of the misses and a row in the numbers table.
// World units = Frankfurt-frame mm * WORLD_PER_MM, x = side, y = up, z = front.
import "../../pages.css";
import { OrbitCamera, camera_eye, camera_pen_ray, camera_view_projection } from "../../src/camera";
import { V3, v3, v3_add, v3_cross, v3_dot, v3_length, v3_scale, v3_sub } from "../../src/math";
import { attach_orbit_controls, bind_controls } from "../../src/explainer/orbit_controls";
import { CanvasView, canvas_view, stroke_polyline } from "../../src/explainer/canvas_view";
import { TranslucentMesh, create_translucent_mesh, draw_mesh_translucent, set_translucent_mesh } from "../../src/render";
import { Landmark, find_landmark, frankfurt_coordinates, frankfurt_to_mesh } from "../../src/reference";
import { EllipsoidFit, ResidualStats, SphereFit, ellipsoid_residual, fit_ellipsoid_algebraic, fit_sphere_algebraic, residual_stats, sphere_residual } from "../../src/construction_fit";
import { LANDMARKS_URL, MeshBuilder, SKULL_NAME, Skull, WORLD_PER_MM, cranium_cut_normal, format_signed, load_skull, mm_to_world, push_ellipsoid, push_landmark_marker, push_plane, push_skull, size_gl_canvas, skull_view_colors as colors, sphere_outline, supraorbital_rim_point, vault_vertices } from "../../src/reference_skull_view";

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
let profile: Profile | null = null;
let glabella: V3 = v3(0, 30, 90); // frame mm; replaced by the file's landmark or the initial marker
let candidates: Candidate[] = [];
const panes: Pane[] = [];

// ---- midline profile ----------------------------------------------------------------
// The skull cut by the midsagittal plane (side = 0), as 2D segments in the profile
// plane, plus the front silhouette: the most forward cut point at each height. The
// glabella is picked on that silhouette, so it is a 1D choice (its height).

type ProfilePoint = { front: number; up: number }; // mm
type ProfileSegment = { a: ProfilePoint; b: ProfilePoint };
type SilhouetteSample = { up: number; front: number | null }; // null = no bone at that height
type Profile = {
  segments: ProfileSegment[];
  silhouette: SilhouetteSample[]; // every SILHOUETTE_STEP_MM from SILHOUETTE_UP_MIN
  bumps: ProfilePoint[]; // local maxima of front along the silhouette
  dips: ProfilePoint[]; // local minima
  orbit_rim_up: number | null; // height of the supraorbital margin (the eyebrow line), null if not found
};

const SILHOUETTE_UP_MIN = -40, SILHOUETTE_UP_MAX = 130, SILHOUETTE_STEP_MM = 0.5; // the mesh spans up -34..122

function midline_cut_segments(skull: Skull): ProfileSegment[] {
  const segments: ProfileSegment[] = [];
  const crossing = (p: V3, q: V3): ProfilePoint | null => {
    if ((p.x >= 0) === (q.x >= 0)) return null;
    const t = p.x / (p.x - q.x);
    return { front: p.z + (q.z - p.z) * t, up: p.y + (q.y - p.y) * t };
  };
  for (let i = 0; i < skull.triangle_indices.length; i += 3) {
    const a = skull.positions[skull.triangle_indices[i]], b = skull.positions[skull.triangle_indices[i + 1]], c = skull.positions[skull.triangle_indices[i + 2]];
    const points = [crossing(a, b), crossing(b, c), crossing(c, a)].filter((p): p is ProfilePoint => p !== null);
    if (points.length === 2) segments.push({ a: points[0], b: points[1] });
  }
  return segments;
}

function silhouette_front_at(segments: ProfileSegment[], up: number): number | null {
  let best: number | null = null;
  for (const { a, b } of segments) {
    if (up < Math.min(a.up, b.up) || up > Math.max(a.up, b.up) || a.up === b.up) continue;
    const front = a.front + (b.front - a.front) * ((up - a.up) / (b.up - a.up));
    if (best === null || front > best) best = front;
  }
  return best;
}

// Local extrema of the silhouette that stand out by at least 1 mm over a ±4 mm window.
function silhouette_extrema(silhouette: SilhouetteSample[], sign: 1 | -1): ProfilePoint[] {
  const window = Math.round(4 / SILHOUETTE_STEP_MM);
  const result: ProfilePoint[] = [];
  for (let i = window; i < silhouette.length - window; i++) {
    const center = silhouette[i].front;
    if (center === null) continue;
    let is_extreme = true, edge_gap = Infinity;
    for (let j = i - window; j <= i + window; j++) {
      const other = silhouette[j].front;
      if (other === null) { is_extreme = false; break; }
      if (j !== i && sign * (other - center) > 0) { is_extreme = false; break; }
      if (j === i - window || j === i + window) edge_gap = Math.min(edge_gap, sign * (center - other));
    }
    if (is_extreme && edge_gap >= 1) result.push({ up: silhouette[i].up, front: center });
  }
  return result;
}

function compute_profile(skull: Skull): Profile {
  const segments = midline_cut_segments(skull);
  const silhouette: SilhouetteSample[] = [];
  for (let up = SILHOUETTE_UP_MIN; up <= SILHOUETTE_UP_MAX; up += SILHOUETTE_STEP_MM) silhouette.push({ up, front: silhouette_front_at(segments, up) });
  const orbit_rim = supraorbital_rim_point(skull); // shared with skull-brow-line
  return { segments, silhouette, bumps: silhouette_extrema(silhouette, 1), dips: silhouette_extrema(silhouette, -1), orbit_rim_up: orbit_rim === null ? null : orbit_rim.y };
}

// Where the marker starts when the landmarks file has no glabella: on the silhouette at
// eyebrow height (the glabella sits level with the brows). Not a guess of the landmark,
// just a starting point for the drag; the person picks.
function initial_glabella_marker(profile: Profile): V3 {
  const up = profile.orbit_rim_up ?? 45;
  const front = silhouette_front_at(profile.segments, up);
  return v3(0, up, front ?? 85);
}

// ---- fits ---------------------------------------------------------------------------

function fit_candidates(skull: Skull, glabella: V3): Candidate[] {
  const frankfurt_up = v3(0, 1, 0);
  const cranium_normal = cranium_cut_normal(glabella);
  const vault_of = (normal: V3) => vault_vertices(skull, normal);
  const make = (id: CandidateId, cut_normal: V3, shape: CandidateShape): Candidate => {
    const residual = shape.kind === "sphere"
      ? (p: V3) => sphere_residual(shape.fit, p)
      : (p: V3) => ellipsoid_residual(shape.fit, p);
    const residuals = skull.positions.map(residual);
    const vault_residuals = vault_of(cut_normal).map(residual);
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
  push_skull(builder, skull, candidate !== null && color_mode.value === "heat" ? candidate.residuals : null, eye_mm, skull_alpha, heat_range);
  return new Float32Array(builder.data);
}

// ---- drawing ------------------------------------------------------------------------

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
  draw_profile();
}

// ---- profile panel ------------------------------------------------------------------

const profile_canvas = document.getElementById("profile_canvas") as HTMLCanvasElement;
const PROFILE_FRONT_MIN = -100, PROFILE_FRONT_MAX = 120; // mm shown, up range = the silhouette's

type ProfileMapping = { scale: number; origin_x: number; origin_y: number }; // device px per mm, px of (front 0, up 0)

function profile_mapping(width: number, height: number, dpr: number): ProfileMapping {
  const pad = 10 * dpr;
  const scale = Math.min((width - 2 * pad) / (PROFILE_FRONT_MAX - PROFILE_FRONT_MIN), (height - 2 * pad) / (SILHOUETTE_UP_MAX - SILHOUETTE_UP_MIN));
  return { scale, origin_x: pad - PROFILE_FRONT_MIN * scale, origin_y: height - pad + SILHOUETTE_UP_MIN * scale };
}

function profile_to_px(mapping: ProfileMapping, p: ProfilePoint): { x: number; y: number } {
  return { x: mapping.origin_x + p.front * mapping.scale, y: mapping.origin_y - p.up * mapping.scale };
}

function draw_profile(): void {
  const { width: css_width, height: css_height } = size_gl_canvas(profile_canvas);
  const dpr = window.devicePixelRatio || 1;
  const ctx = profile_canvas.getContext("2d")!;
  const width = css_width * dpr, height = css_height * dpr;
  ctx.fillStyle = "#14161c";
  ctx.fillRect(0, 0, width, height);
  if (profile === null) return;
  const mapping = profile_mapping(width, height, dpr);
  const px = (p: ProfilePoint) => profile_to_px(mapping, p);
  // Frankfurt plane and the cranium cut, as lines through the porion middle.
  const line = (direction: ProfilePoint, color: string) => {
    const a = px({ front: -direction.front * 300, up: -direction.up * 300 }), b = px({ front: direction.front * 300, up: direction.up * 300 });
    ctx.strokeStyle = color; ctx.lineWidth = 1 * dpr;
    ctx.beginPath(); ctx.moveTo(a.x, a.y); ctx.lineTo(b.x, b.y); ctx.stroke();
  };
  line({ front: 1, up: 0 }, "rgba(77, 212, 191, 0.7)");
  line({ front: glabella.z, up: glabella.y }, "rgba(255, 168, 77, 0.7)");
  // The whole cut in grey, the front silhouette on top in bone color.
  ctx.strokeStyle = "#5a5f6b"; ctx.lineWidth = 1 * dpr;
  ctx.beginPath();
  for (const { a, b } of profile.segments) { const pa = px(a), pb = px(b); ctx.moveTo(pa.x, pa.y); ctx.lineTo(pb.x, pb.y); }
  ctx.stroke();
  ctx.strokeStyle = "#d9cfb3"; ctx.lineWidth = 2 * dpr;
  ctx.beginPath();
  let pen_down = false;
  for (const sample of profile.silhouette) {
    if (sample.front === null) { pen_down = false; continue; }
    const p = px({ front: sample.front, up: sample.up });
    if (pen_down) ctx.lineTo(p.x, p.y); else ctx.moveTo(p.x, p.y);
    pen_down = true;
  }
  ctx.stroke();
  // Ticks at the bumps (pointing forward) and dips (pointing back).
  ctx.strokeStyle = "#ffffff"; ctx.lineWidth = 1 * dpr;
  ctx.beginPath();
  for (const bump of profile.bumps) { const p = px(bump); ctx.moveTo(p.x + 3 * dpr, p.y); ctx.lineTo(p.x + 10 * dpr, p.y); }
  for (const dip of profile.dips) { const p = px(dip); ctx.moveTo(p.x - 3 * dpr, p.y); ctx.lineTo(p.x - 10 * dpr, p.y); }
  ctx.stroke();
  // The eyebrow line, dotted: the glabella sits level with it.
  if (profile.orbit_rim_up !== null) {
    const a = px({ front: PROFILE_FRONT_MIN, up: profile.orbit_rim_up }), b = px({ front: PROFILE_FRONT_MAX, up: profile.orbit_rim_up });
    ctx.strokeStyle = "rgba(127, 179, 255, 0.5)"; ctx.lineWidth = 1 * dpr; ctx.setLineDash([4 * dpr, 4 * dpr]);
    ctx.beginPath(); ctx.moveTo(a.x, a.y); ctx.lineTo(b.x, b.y); ctx.stroke();
    ctx.setLineDash([]);
    ctx.fillStyle = "#7fb3ff"; ctx.font = `${11 * dpr}px system-ui, sans-serif`;
    ctx.fillText(`orbit rim (eyebrows) up ${profile.orbit_rim_up.toFixed(0)}`, a.x + 8 * dpr, a.y - 4 * dpr);
  }
  // The marker.
  const marker = px({ front: glabella.z, up: glabella.y });
  ctx.strokeStyle = "#ffb14d"; ctx.lineWidth = 1.5 * dpr;
  ctx.beginPath(); ctx.arc(marker.x, marker.y, 6 * dpr, 0, 2 * Math.PI); ctx.stroke();
  ctx.font = `${12 * dpr}px system-ui, sans-serif`;
  ctx.fillStyle = "#ffb14d";
  ctx.fillText(`glabella up ${glabella.y.toFixed(1)} front ${glabella.z.toFixed(1)}`, marker.x + 12 * dpr, marker.y + 4 * dpr);
  ctx.fillStyle = "#8a8f9a";
  ctx.fillText("porion middle", mapping.origin_x + 4 * dpr, mapping.origin_y - 4 * dpr);
}

// Dragging anywhere in the panel moves the marker to the pointer's height, on the silhouette.
function attach_profile_drag(): void {
  let dragging = false;
  const move_marker_to = (event: PointerEvent) => {
    if (profile === null) return;
    const rect = profile_canvas.getBoundingClientRect();
    const dpr = window.devicePixelRatio || 1;
    const mapping = profile_mapping(rect.width * dpr, rect.height * dpr, dpr);
    const up = (mapping.origin_y - (event.clientY - rect.top) * dpr) / mapping.scale;
    const front = silhouette_front_at(profile.segments, up);
    if (front !== null) set_glabella(v3(0, up, front));
  };
  profile_canvas.addEventListener("pointerdown", (event) => { dragging = true; profile_canvas.setPointerCapture(event.pointerId); move_marker_to(event); });
  profile_canvas.addEventListener("pointermove", (event) => { if (dragging) move_marker_to(event); });
  profile_canvas.addEventListener("pointerup", () => { dragging = false; });
  profile_canvas.addEventListener("pointercancel", () => { dragging = false; });
}

// ---- numbers table ------------------------------------------------------------------

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

// ---- glabella: type, pick, save --------------------------------------------------------

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
attach_profile_drag();
window.addEventListener("resize", draw_profile);

void load_skull("skull-ball").then((loaded) => {
  if (loaded === null) return;
  skull = loaded;
  profile = compute_profile(skull);
  const glabella_in_file = find_landmark(skull.landmarks, "glabella");
  glabella = glabella_in_file !== null ? frankfurt_coordinates(skull.frame, glabella_in_file) : initial_glabella_marker(profile);
  write_glabella_to_inputs();
  refit();
});
