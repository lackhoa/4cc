// Document `skull-face-thirds` (plan-skull-construction-docs.md Q21): Loomis's face units,
// brow to nose base and nose base to chin, measured on the skull + mandible against the
// book's two numbers (Plate 1: 0.5 r each, Plate 18: 2/3 r each). The nose base is the
// anterior nasal spine on the skull, the chin is the menton on the mandible; both are
// hand-picked here and stored in the meshes' landmarks files. The hairline is not on bone,
// so the upper unit is not tested. Ball + brow line as on skull-brow-line, no fit of its own.
import "../../pages.css";
import { OrbitCamera, camera_eye, camera_pen_ray, camera_view_projection } from "../../src/camera";
import { V3, v3, v3_scale } from "../../src/math";
import { attach_orbit_controls, bind_controls } from "../../src/explainer/orbit_controls";
import { CanvasView, canvas_view, stroke_polyline } from "../../src/explainer/canvas_view";
import { TranslucentMesh, create_translucent_mesh, draw_mesh_translucent, set_translucent_mesh } from "../../src/render";
import { find_landmark, frankfurt_coordinates } from "../../src/reference";
import { LengthSphereFit, SidePlanesFit, fit_side_planes_mirrored, fit_sphere_through_midline_extremes } from "../../src/construction_fit";
import { LANDMARKS_URL, MANDIBLE_LANDMARKS_URL, MeshBuilder, Skull, WORLD_PER_MM, cranium_cut_normal, format_signed, load_mandible, load_skull, mm_to_world, pick_skull_vertex, push_landmark_marker, push_side_plane, push_skull, push_sphere_with_side_cuts, save_landmark, side_cut_rim, size_gl_canvas, skull_view_colors as colors, sphere_outline, vault_vertices } from "../../src/reference_skull_view";

const MIDLINE_BAND_MM = 10; // same length ball as skull-side-cuts and skull-brow-line
const plane_color = v3(0.5, 0.75, 1.0);
const landmark_color = v3(0.55, 1.0, 0.6);
const BOOK_UNITS = [{ id: "plate-1", label: "Plate 1", unit_r: 0.5 }, { id: "plate-18", label: "Plate 18", unit_r: 2 / 3 }];

type LandmarkName = "nasal_spine" | "menton";

type Construction = {
  cut_normal: V3;
  length: LengthSphereFit;
  side_planes: SidePlanesFit;
  brow_up: number; // the brow line's height = the ball center's, mm
  glabella: V3;
};

const camera: OrbitCamera = { pivot: v3(0, 0.2, 0), yaw: 0.6, pitch: 0.1, distance: 3.8 };
let skull: Skull | null = null;
let mandible: Skull | null = null;
let construction: Construction | null = null;
// The two picked landmarks, frame mm; start from the files, else from a guess on the mesh.
let nasal_spine: V3 | null = null;
let menton: V3 | null = null;

// ---- construction -------------------------------------------------------------------

function compute_construction(skull: Skull, glabella: V3): Construction | null {
  const cut_normal = cranium_cut_normal(glabella);
  const vault = vault_vertices(skull, cut_normal);
  const length = fit_sphere_through_midline_extremes(vault, MIDLINE_BAND_MM);
  const side_planes = fit_side_planes_mirrored(vault);
  if (length === null || side_planes === null) return null;
  return { cut_normal, length, side_planes, brow_up: length.sphere.center.y, glabella };
}

// First guesses when the landmarks are not in the files yet: the nasal spine is the most
// forward midline vertex in the band just under the nose opening; the band stops above the
// alveolar edge (up -30 on this skull, the mesh ends there, the teeth are a separate model),
// which sticks out as far as the spine. The menton is the lowest midline vertex of the
// mandible. A click on the mesh refines them.
function guess_nasal_spine(skull: Skull): V3 | null {
  let best: V3 | null = null;
  for (const p of skull.positions) {
    if (Math.abs(p.x) < 4 && p.y > -24 && p.y < -8 && (best === null || p.z > best.z)) best = p;
  }
  return best;
}

function guess_menton(mandible: Skull): V3 | null {
  let best: V3 | null = null;
  for (const p of mandible.positions) {
    if (Math.abs(p.x) < 4 && (best === null || p.y < best.y)) best = p;
  }
  return best;
}

// ---- 3D pane ------------------------------------------------------------------------

const pane_canvas = document.querySelector<HTMLCanvasElement>("canvas.pane")!;
const overlay_canvas = pane_canvas.nextElementSibling as HTMLCanvasElement;
const gl = pane_canvas.getContext("webgl", { premultipliedAlpha: false })!;
const pane_mesh: TranslucentMesh = create_translucent_mesh(gl);

function build_pane_mesh(eye_mm: V3): Float32Array {
  const builder: MeshBuilder = { data: [] };
  if (skull === null || mandible === null || construction === null) return new Float32Array(0);
  const { center, radius } = construction.length.sphere;
  const half_width = construction.side_planes.half_width;
  push_sphere_with_side_cuts(builder, center, radius, half_width, colors.shape, Number(controls.shape_alpha.value), eye_mm);
  push_side_plane(builder, half_width, plane_color, 0.12);
  push_side_plane(builder, -half_width, plane_color, 0.12);
  push_landmark_marker(builder, construction.glabella, landmark_color);
  if (nasal_spine !== null) push_landmark_marker(builder, nasal_spine, landmark_color);
  if (menton !== null) push_landmark_marker(builder, menton, landmark_color);
  const skull_alpha = Number(controls.skull_alpha.value);
  push_skull(builder, skull, null, eye_mm, skull_alpha, 20);
  push_skull(builder, mandible, null, eye_mm, skull_alpha, 20);
  return new Float32Array(builder.data);
}

// The brow line as a ring (skull-brow-line), and the nose and chin lines as short bars
// across the front of the face at their heights.
function draw_overlay(view: CanvasView, eye_mm: V3): void {
  if (construction === null) return;
  const { center, radius } = construction.length.sphere;
  const half_width = construction.side_planes.half_width;
  const outline = sphere_outline(center, radius, eye_mm);
  if (outline !== null) stroke_polyline(view, outline.map(mm_to_world), true, "rgba(255, 209, 102, 0.5)", 1);
  for (const x of [half_width, -half_width]) {
    const rim = side_cut_rim(center, radius, x);
    if (rim !== null) stroke_polyline(view, rim.map(mm_to_world), true, "#7fb3ff", 1.2);
  }
  const ring: V3[] = [];
  for (let i = 0; i < 96; i++) {
    const angle = (i / 96) * 2 * Math.PI;
    const x = Math.max(-half_width, Math.min(half_width, center.x + radius * Math.cos(angle)));
    ring.push(v3(x, construction.brow_up, center.z + radius * Math.sin(angle)));
  }
  stroke_polyline(view, ring.map(mm_to_world), true, "#ffd166", 2.2);
  for (const p of [nasal_spine, menton]) {
    if (p !== null) stroke_polyline(view, [v3(-30, p.y, p.z), v3(30, p.y, p.z)].map(mm_to_world), false, "#8cffa0", 2);
  }
}

function draw_pane(): void {
  const { width, height } = size_gl_canvas(pane_canvas);
  gl.viewport(0, 0, pane_canvas.width, pane_canvas.height);
  gl.clearColor(0.078, 0.086, 0.11, 1);
  gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
  const eye = camera_eye(camera);
  const eye_mm = v3_scale(eye, 1 / WORLD_PER_MM);
  set_translucent_mesh(pane_mesh, build_pane_mesh(eye_mm));
  draw_mesh_translucent(pane_mesh, camera_view_projection(camera, width / height), eye);
  draw_overlay(canvas_view(overlay_canvas, camera), eye_mm);
}

function redraw(): void { draw_pane(); draw_profile(); }

// ---- profile section ----------------------------------------------------------------
// The skull and mandible cut along the midline (x = 0), seen from the right.

type SectionPoint = { across: number; up: number }; // mm
type SectionSegment = { a: SectionPoint; b: SectionPoint };

function midline_cut_segments(mesh: Skull): SectionSegment[] {
  const segments: SectionSegment[] = [];
  const crossing = (p: V3, q: V3): SectionPoint | null => {
    if ((p.x >= 0) === (q.x >= 0)) return null;
    const t = p.x / (p.x - q.x);
    return { across: p.z + (q.z - p.z) * t, up: p.y + (q.y - p.y) * t };
  };
  for (let i = 0; i < mesh.triangle_indices.length; i += 3) {
    const a = mesh.positions[mesh.triangle_indices[i]], b = mesh.positions[mesh.triangle_indices[i + 1]], c = mesh.positions[mesh.triangle_indices[i + 2]];
    const points = [crossing(a, b), crossing(b, c), crossing(c, a)].filter((p): p is SectionPoint => p !== null);
    if (points.length === 2) segments.push({ a: points[0], b: points[1] });
  }
  return segments;
}

type SectionView = { ctx: CanvasRenderingContext2D; dpr: number; scale: number; px: (p: SectionPoint) => { x: number; y: number } };

function section_view(canvas: HTMLCanvasElement, across_min: number, across_max: number, up_min: number, up_max: number): SectionView {
  const { width: css_width, height: css_height } = size_gl_canvas(canvas);
  const dpr = window.devicePixelRatio || 1;
  const ctx = canvas.getContext("2d")!;
  const width = css_width * dpr, height = css_height * dpr;
  const pad = 10 * dpr;
  const scale = Math.min((width - 2 * pad) / (across_max - across_min), (height - 2 * pad) / (up_max - up_min));
  const origin_x = width / 2 - ((across_min + across_max) / 2) * scale;
  const origin_y = height / 2 + ((up_min + up_max) / 2) * scale;
  ctx.fillStyle = "#14161c";
  ctx.fillRect(0, 0, width, height);
  return { ctx, dpr, scale, px: (p) => ({ x: origin_x + p.across * scale, y: origin_y - p.up * scale }) };
}

function stroke_segments(view: SectionView, segments: SectionSegment[], style: string, line_width: number): void {
  const { ctx } = view;
  ctx.strokeStyle = style; ctx.lineWidth = line_width * view.dpr;
  ctx.setLineDash([]);
  ctx.beginPath();
  for (const { a, b } of segments) { const pa = view.px(a), pb = view.px(b); ctx.moveTo(pa.x, pa.y); ctx.lineTo(pb.x, pb.y); }
  ctx.stroke();
}

function stroke_circle(view: SectionView, center: SectionPoint, radius_mm: number, style: string, line_width: number): void {
  const { ctx } = view;
  const c = view.px(center);
  ctx.strokeStyle = style; ctx.lineWidth = line_width * view.dpr;
  ctx.setLineDash([]);
  ctx.beginPath(); ctx.arc(c.x, c.y, radius_mm * view.scale, 0, 2 * Math.PI); ctx.stroke();
}

function stroke_line(view: SectionView, a: SectionPoint, b: SectionPoint, style: string, line_width: number, dash: number[]): void {
  const { ctx } = view;
  const pa = view.px(a), pb = view.px(b);
  ctx.strokeStyle = style; ctx.lineWidth = line_width * view.dpr;
  ctx.setLineDash(dash.map((d) => d * view.dpr));
  ctx.beginPath(); ctx.moveTo(pa.x, pa.y); ctx.lineTo(pb.x, pb.y); ctx.stroke();
  ctx.setLineDash([]);
}

function fill_dot(view: SectionView, p: SectionPoint, style: string): void {
  const q = view.px(p);
  view.ctx.fillStyle = style;
  view.ctx.beginPath(); view.ctx.arc(q.x, q.y, 3 * view.dpr, 0, 2 * Math.PI); view.ctx.fill();
}

// A negative dx puts the text to the left of the point (right-aligned).
function label(view: SectionView, p: SectionPoint, text: string, style: string, dx_px: number, dy_px: number): void {
  const q = view.px(p);
  view.ctx.fillStyle = style; view.ctx.font = `${12 * view.dpr}px system-ui, sans-serif`;
  view.ctx.textAlign = dx_px < 0 ? "right" : "left";
  view.ctx.fillText(text, q.x + dx_px * view.dpr, q.y + dy_px * view.dpr);
  view.ctx.textAlign = "left";
}

const profile_canvas = document.getElementById("profile_canvas") as HTMLCanvasElement;
const ball_style = "#ffd166", landmark_style = "#8cffa0", book_styles = ["#ff8fb1", "#c9a0ff"];
const ACROSS_MIN = -120, ACROSS_MAX = 110;

function draw_profile(): void {
  if (skull === null || mandible === null || construction === null) return;
  const ball = construction.length.sphere;
  const radius = ball.radius;
  const view = section_view(profile_canvas, ACROSS_MIN, ACROSS_MAX, Math.min(-95, construction.brow_up - 1.4 * radius), ball.center.y + radius + 5);
  stroke_segments(view, midline_cut_segments(skull), "#8a8f9a", 1);
  stroke_segments(view, midline_cut_segments(mandible), "#8a8f9a", 1);
  stroke_circle(view, { across: ball.center.z, up: ball.center.y }, radius, ball_style, 1.5);
  stroke_line(view, { across: ACROSS_MIN, up: construction.brow_up }, { across: ACROSS_MAX, up: construction.brow_up }, ball_style, 2, []);
  label(view, { across: ACROSS_MIN + 5, up: construction.brow_up }, `brow line up ${construction.brow_up.toFixed(1)}`, ball_style, 0, -6);
  // The book's nose and chin lines, dashed, one color per plate, ticks at the left.
  BOOK_UNITS.forEach((book, index) => {
    for (const units of [1, 2]) {
      const up = construction!.brow_up - units * book.unit_r * radius;
      stroke_line(view, { across: ACROSS_MIN, up }, { across: ACROSS_MAX, up }, book_styles[index], 1, [6, 4]);
      label(view, { across: ACROSS_MIN + 5, up }, `${book.label} ${units === 1 ? "nose" : "chin"} ${format_signed(up - construction!.brow_up, 1)}`, book_styles[index], 0, index === 0 ? -6 : 14);
    }
  });
  // The skull's own lines, solid, labeled at the face.
  const rows: { name: string; p: V3 | null }[] = [{ name: "glabella", p: construction.glabella }, { name: "nasal spine", p: nasal_spine }, { name: "menton", p: menton }];
  for (const { name, p } of rows) {
    if (p === null) continue;
    const q = { across: p.z, up: p.y };
    fill_dot(view, q, landmark_style);
    stroke_line(view, { across: q.across - 40, up: q.up }, { across: ACROSS_MAX, up: q.up }, landmark_style, 1.2, []);
    label(view, q, `${name} ${format_signed(q.up - construction.brow_up, 1)}`, landmark_style, -8, -6);
  }
}

// ---- numbers table ------------------------------------------------------------------

function fill_row(id: string, cells: string[]): void {
  const row = document.getElementById(id)!;
  const label_cell = row.firstElementChild!.outerHTML;
  row.innerHTML = label_cell + cells.map((cell) => `<td>${cell}</td>`).join("");
}

function update_numbers_table(): void {
  if (construction === null) return;
  const radius = construction.length.sphere.radius;
  const brow_up = construction.brow_up;
  const missing = ["pick it", "", "", ""];
  const book_cells = (units: number) => BOOK_UNITS.map((book) => `${(units * book.unit_r).toFixed(2)} r = ${(units * book.unit_r * radius).toFixed(1)} mm`);
  const distance_row = (id: string, from_up: number | null, to_up: number | null, units: number) => {
    if (from_up === null || to_up === null) { fill_row(id, missing); return; }
    const mm = from_up - to_up;
    fill_row(id, [`${mm.toFixed(1)} mm`, `${(mm / radius).toFixed(2)} r`, ...book_cells(units)]);
  };
  const nose_up = nasal_spine === null ? null : nasal_spine.y;
  const chin_up = menton === null ? null : menton.y;
  distance_row("row-brow-nose", brow_up, nose_up, 1);
  distance_row("row-nose-chin", nose_up, chin_up, 1);
  distance_row("row-brow-chin", brow_up, chin_up, 2);
  const center = construction.length.sphere.center;
  document.getElementById("ball_text")!.textContent = `center up ${center.y.toFixed(1)} / front ${center.z.toFixed(1)}, r ${radius.toFixed(1)}`;
  const point_text = (p: V3 | null) => (p === null ? "not picked" : `side ${format_signed(p.x, 1)}, up ${format_signed(p.y, 1)}, front ${format_signed(p.z, 1)}`);
  document.getElementById("nasal_spine_text")!.textContent = point_text(nasal_spine);
  document.getElementById("menton_text")!.textContent = point_text(menton);
}

// ---- picking + saving ---------------------------------------------------------------

const pick_mode = document.getElementById("pick_mode") as HTMLInputElement;
const pick_target = document.getElementById("pick_target") as HTMLSelectElement;

function attach_pick(): void {
  let down: { x: number; y: number } | null = null;
  pane_canvas.addEventListener("pointerdown", (event) => { down = { x: event.clientX, y: event.clientY }; });
  pane_canvas.addEventListener("pointerup", (event) => {
    if (down === null || skull === null || mandible === null || !pick_mode.checked) return;
    const moved = Math.hypot(event.clientX - down.x, event.clientY - down.y);
    down = null;
    if (moved > 4) return;
    const rect = pane_canvas.getBoundingClientRect();
    const ray = camera_pen_ray(camera, { x: event.clientX - rect.left, y: event.clientY - rect.top }, rect.width, rect.height);
    const target = pick_target.value as LandmarkName;
    const picked = pick_skull_vertex(target === "nasal_spine" ? skull : mandible, v3_scale(ray.origin, 1 / WORLD_PER_MM), ray.direction);
    if (picked === null) return;
    if (target === "nasal_spine") nasal_spine = picked; else menton = picked;
    update_numbers_table();
    redraw();
  });
}

async function save_landmarks(): Promise<void> {
  if (skull === null || mandible === null) return;
  const status = document.getElementById("save_status")!;
  const lines: string[] = [];
  if (nasal_spine !== null) lines.push(await save_landmark(skull, LANDMARKS_URL, "nasal_spine", nasal_spine));
  if (menton !== null) lines.push(await save_landmark(mandible, MANDIBLE_LANDMARKS_URL, "menton", menton));
  status.textContent = lines.join("; ");
}

// ---- wiring -------------------------------------------------------------------------

const controls = bind_controls(["skull_alpha", "shape_alpha"], redraw);
attach_orbit_controls([pane_canvas], camera, redraw);
attach_pick();
document.getElementById("save_landmarks")!.addEventListener("click", () => { void save_landmarks(); });
window.addEventListener("resize", draw_profile);

void load_skull("skull-face-thirds").then(async (loaded) => {
  if (loaded === null) return;
  skull = loaded;
  mandible = await load_mandible("skull-face-thirds", skull.frame);
  if (mandible === null) { document.getElementById("ball_text")!.textContent = "MISSING mandible mesh"; return; }
  const glabella_in_file = find_landmark(skull.landmarks, "glabella");
  if (glabella_in_file === null) {
    document.getElementById("ball_text")!.textContent = "MISSING glabella: pick it on the skull-ball page first";
    return;
  }
  construction = compute_construction(skull, frankfurt_coordinates(skull.frame, glabella_in_file));
  if (construction === null) { console.error("skull-face-thirds: the ball or the cuts failed"); return; }
  const nasal_spine_in_file = find_landmark(skull.landmarks, "nasal_spine");
  const menton_in_file = find_landmark(mandible.landmarks, "menton");
  nasal_spine = nasal_spine_in_file === null ? guess_nasal_spine(skull) : frankfurt_coordinates(skull.frame, nasal_spine_in_file);
  menton = menton_in_file === null ? guess_menton(mandible) : frankfurt_coordinates(mandible.frame, menton_in_file);
  document.getElementById("save_status")!.textContent = nasal_spine_in_file === null || menton_in_file === null ? "guessed from the mesh, not saved yet" : "from the landmarks files";
  update_numbers_table();
  redraw();
});
