// Document `skull-side-cuts` (plan-skull-construction-docs.md Q15-Q17): Loomis's ball done
// his way on the skull. Three candidates sharing a camera: the least-squares cranium sphere
// of skull-ball (a), the length ball through the midline extremes (b), that ball clipped by
// the two mirrored side planes at the skull's widest point (c); heat maps, two 2D sections
// (midline profile, coronal through the ball center), a numbers table.
import "../../pages.css";
import { OrbitCamera, camera_eye, camera_view_projection } from "../../src/camera";
import { V3, v3, v3_scale } from "../../src/math";
import { attach_orbit_controls, bind_controls } from "../../src/explainer/orbit_controls";
import { CanvasView, canvas_view, stroke_polyline } from "../../src/explainer/canvas_view";
import { TranslucentMesh, create_translucent_mesh, draw_mesh_translucent, set_translucent_mesh } from "../../src/render";
import { find_landmark, frankfurt_coordinates } from "../../src/reference";
import { LengthSphereFit, ResidualStats, SidePlanesFit, SphereFit, fit_side_planes_mirrored, fit_sphere_algebraic, fit_sphere_through_midline_extremes, residual_stats, residuals_sphere_with_side_cuts, sphere_residual } from "../../src/construction_fit";
import { MeshBuilder, Skull, WORLD_PER_MM, cranium_cut_normal, format_signed, load_skull, mm_to_world, push_ellipsoid, push_landmark_marker, push_side_plane, push_skull, push_sphere_with_side_cuts, side_cut_rim, size_gl_canvas, skull_view_colors as colors, sphere_outline, vault_vertices } from "../../src/reference_skull_view";

const MIDLINE_BAND_MM = 10; // "near the midline" for the length ball's two points
const plane_color = v3(0.5, 0.75, 1.0);

type CandidateId = "least-squares" | "length-ball" | "side-cuts";

type Candidate = {
  id: CandidateId;
  sphere: SphereFit;
  half_width: number | null; // side cuts only
  residuals: number[]; // per skull vertex, mm, + = outside
  stats: ResidualStats; // over the vault
};

// Everything fitted once the skull and the glabella are known.
type Fits = {
  cut_normal: V3;
  vault_count: number;
  skull_top_up: number; // highest vault vertex, mm
  length: LengthSphereFit;
  side_planes: SidePlanesFit;
  candidates: Candidate[];
};

type Pane = {
  gl: WebGLRenderingContext;
  canvas: HTMLCanvasElement;
  overlay: HTMLCanvasElement;
  mesh: TranslucentMesh;
  candidate_id: CandidateId;
};

const camera: OrbitCamera = { pivot: v3(0, 0.45, 0), yaw: 0.6, pitch: 0.15, distance: 3.4 };
let skull: Skull | null = null;
let glabella: V3 | null = null; // frame mm, from the landmarks file
let fits: Fits | null = null;
const panes: Pane[] = [];

// ---- fits ---------------------------------------------------------------------------

function compute_fits(skull: Skull, glabella: V3): Fits | null {
  const cut_normal = cranium_cut_normal(glabella);
  const vault = vault_vertices(skull, cut_normal);
  const least_squares = fit_sphere_algebraic(vault);
  const length = fit_sphere_through_midline_extremes(vault, MIDLINE_BAND_MM);
  const side_planes = fit_side_planes_mirrored(vault);
  if (least_squares === null || length === null || side_planes === null) return null;
  const candidate = (id: CandidateId, sphere: SphereFit, half_width: number | null): Candidate => {
    const residual = (p: V3) => (half_width === null ? sphere_residual(sphere, p) : residuals_sphere_with_side_cuts(sphere, half_width, p));
    return { id, sphere, half_width, residuals: skull.positions.map(residual), stats: residual_stats(vault.map(residual)) };
  };
  return {
    cut_normal,
    vault_count: vault.length,
    skull_top_up: Math.max(...vault.map((p) => p.y)),
    length,
    side_planes,
    candidates: [
      candidate("least-squares", least_squares, null),
      candidate("length-ball", length.sphere, null),
      candidate("side-cuts", length.sphere, side_planes.half_width),
    ],
  };
}

function candidate_by_id(id: CandidateId): Candidate | null {
  return fits?.candidates.find((c) => c.id === id) ?? null;
}

// ---- panes --------------------------------------------------------------------------

function build_pane_mesh(pane: Pane, eye_mm: V3): Float32Array {
  const builder: MeshBuilder = { data: [] };
  const candidate = candidate_by_id(pane.candidate_id);
  if (skull === null || fits === null || candidate === null) return new Float32Array(0);
  const skull_alpha = Number(controls.skull_alpha.value);
  const shape_alpha = Number(controls.shape_alpha.value);
  const heat_range = Number(controls.heat_range.value);
  const r = candidate.sphere.radius;
  if (candidate.half_width === null) {
    push_ellipsoid(builder, candidate.sphere.center, v3(r, r, r), colors.shape, shape_alpha, eye_mm);
  } else {
    push_sphere_with_side_cuts(builder, candidate.sphere.center, r, candidate.half_width, colors.shape, shape_alpha, eye_mm);
    push_side_plane(builder, candidate.half_width, plane_color, 0.15);
    push_side_plane(builder, -candidate.half_width, plane_color, 0.15);
  }
  if (candidate.id !== "least-squares") {
    push_landmark_marker(builder, fits.length.front_most, colors.landmark);
    push_landmark_marker(builder, fits.length.back_most, colors.landmark);
  }
  if (candidate.id === "side-cuts") push_landmark_marker(builder, fits.side_planes.widest, plane_color);
  push_skull(builder, skull, color_mode.value === "heat" ? candidate.residuals : null, eye_mm, skull_alpha, heat_range);
  return new Float32Array(builder.data);
}

function draw_overlay(pane: Pane, view: CanvasView, eye_mm: V3): void {
  const candidate = candidate_by_id(pane.candidate_id);
  if (candidate === null) return;
  const outline = sphere_outline(candidate.sphere.center, candidate.sphere.radius, eye_mm);
  if (outline !== null) stroke_polyline(view, outline.map(mm_to_world), true, "#ffd166", 1.4);
  if (candidate.half_width !== null) {
    for (const x of [candidate.half_width, -candidate.half_width]) {
      const rim = side_cut_rim(candidate.sphere.center, candidate.sphere.radius, x);
      if (rim !== null) stroke_polyline(view, rim.map(mm_to_world), true, "#7fb3ff", 1.4);
    }
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
  draw_sections();
}

// ---- 2D sections --------------------------------------------------------------------
// The skull cut by a plane perpendicular to one frame axis, as 2D segments in the two
// other axes. `cut_axis` = the axis the plane is perpendicular to; the drawing's
// horizontal axis is `across_axis`, vertical is always up (y).

type Axis = "x" | "z";
type SectionPoint = { across: number; up: number }; // mm
type SectionSegment = { a: SectionPoint; b: SectionPoint };

function axis_cut_segments(skull: Skull, cut_axis: Axis, cut_at: number, across_axis: Axis): SectionSegment[] {
  const segments: SectionSegment[] = [];
  const crossing = (p: V3, q: V3): SectionPoint | null => {
    if ((p[cut_axis] >= cut_at) === (q[cut_axis] >= cut_at)) return null;
    const t = (p[cut_axis] - cut_at) / (p[cut_axis] - q[cut_axis]);
    return { across: p[across_axis] + (q[across_axis] - p[across_axis]) * t, up: p.y + (q.y - p.y) * t };
  };
  for (let i = 0; i < skull.triangle_indices.length; i += 3) {
    const a = skull.positions[skull.triangle_indices[i]], b = skull.positions[skull.triangle_indices[i + 1]], c = skull.positions[skull.triangle_indices[i + 2]];
    const points = [crossing(a, b), crossing(b, c), crossing(c, a)].filter((p): p is SectionPoint => p !== null);
    if (points.length === 2) segments.push({ a: points[0], b: points[1] });
  }
  return segments;
}

// A canvas with a mm -> pixel mapping for the given ranges, letterboxed and centered.
type SectionView = { ctx: CanvasRenderingContext2D; dpr: number; scale: number; px: (p: SectionPoint) => { x: number; y: number }; width: number; height: number };

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
  return { ctx, dpr, scale, px: (p) => ({ x: origin_x + p.across * scale, y: origin_y - p.up * scale }), width, height };
}

function stroke_segments(view: SectionView, segments: SectionSegment[], style: string, line_width: number): void {
  const { ctx } = view;
  ctx.strokeStyle = style; ctx.lineWidth = line_width * view.dpr;
  ctx.setLineDash([]);
  ctx.beginPath();
  for (const { a, b } of segments) { const pa = view.px(a), pb = view.px(b); ctx.moveTo(pa.x, pa.y); ctx.lineTo(pb.x, pb.y); }
  ctx.stroke();
}

function stroke_circle(view: SectionView, center: SectionPoint, radius_mm: number, style: string, line_width: number, dash: number[]): void {
  const { ctx } = view;
  const c = view.px(center);
  ctx.strokeStyle = style; ctx.lineWidth = line_width * view.dpr;
  ctx.setLineDash(dash.map((d) => d * view.dpr));
  ctx.beginPath(); ctx.arc(c.x, c.y, radius_mm * view.scale, 0, 2 * Math.PI); ctx.stroke();
  ctx.setLineDash([]);
}

function stroke_line(view: SectionView, a: SectionPoint, b: SectionPoint, style: string, line_width: number): void {
  const { ctx } = view;
  const pa = view.px(a), pb = view.px(b);
  ctx.strokeStyle = style; ctx.lineWidth = line_width * view.dpr;
  ctx.setLineDash([]);
  ctx.beginPath(); ctx.moveTo(pa.x, pa.y); ctx.lineTo(pb.x, pb.y); ctx.stroke();
}

function fill_dot(view: SectionView, p: SectionPoint, style: string): void {
  const q = view.px(p);
  view.ctx.fillStyle = style;
  view.ctx.beginPath(); view.ctx.arc(q.x, q.y, 3 * view.dpr, 0, 2 * Math.PI); view.ctx.fill();
}

function label(view: SectionView, p: SectionPoint, text: string, style: string, dx_px = 6, dy_px = 0): void {
  const q = view.px(p);
  view.ctx.fillStyle = style; view.ctx.font = `${12 * view.dpr}px system-ui, sans-serif`;
  view.ctx.fillText(text, q.x + dx_px * view.dpr, q.y + dy_px * view.dpr);
}

const profile_canvas = document.getElementById("profile_canvas") as HTMLCanvasElement;
const section_canvas = document.getElementById("section_canvas") as HTMLCanvasElement;
const least_squares_style = "#8a8f9a", ball_style = "#ffd166", plane_style = "#7fb3ff", cut_style = "rgba(255, 168, 77, 0.7)";

// Midline cut, seen from the right: front -> right, up -> up. Shows the ball touching the
// forehead and the back, and standing above the skull top.
function draw_profile(): void {
  if (skull === null || fits === null) return;
  const least_squares = fits.candidates[0].sphere, ball = fits.length.sphere;
  const view = section_view(profile_canvas, -120, 110, -45, Math.max(145, ball.center.y + ball.radius + 5));
  stroke_segments(view, axis_cut_segments(skull, "x", 0, "z"), "#8a8f9a", 1);
  // The cranium cut is the line through the origin and the glabella in this section.
  const n = fits.cut_normal;
  if (Math.abs(n.y) > 1e-6) stroke_line(view, { across: -120, up: -(n.z * -120) / n.y }, { across: 110, up: -(n.z * 110) / n.y }, cut_style, 1);
  stroke_circle(view, { across: least_squares.center.z, up: least_squares.center.y }, least_squares.radius, least_squares_style, 1, [6, 4]);
  stroke_circle(view, { across: ball.center.z, up: ball.center.y }, ball.radius, ball_style, 1.5, []);
  for (const p of [fits.length.front_most, fits.length.back_most]) fill_dot(view, { across: p.z, up: p.y }, ball_style);
  const top = { across: ball.center.z, up: ball.center.y + ball.radius };
  stroke_line(view, top, { across: ball.center.z, up: fits.skull_top_up }, plane_style, 1.5);
  label(view, top, `${(top.up - fits.skull_top_up).toFixed(1)} above the skull top`, plane_style, 6, 4);
  label(view, { across: -115, up: -40 }, `length ball r ${ball.radius.toFixed(1)}, least squares r ${least_squares.radius.toFixed(1)} (dashed)`, ball_style, 0, 0);
  label(view, { across: 0, up: 0 }, "porion middle", "#8a8f9a", 4, -4);
  fill_dot(view, { across: 0, up: 0 }, "#8a8f9a");
}

// Coronal cut through the ball's center, seen from the front: side -> right, up -> up.
function draw_section(): void {
  if (skull === null || fits === null) return;
  const least_squares = fits.candidates[0].sphere, ball = fits.length.sphere;
  const half_width = fits.side_planes.half_width;
  const view = section_view(section_canvas, -105, 105, -45, Math.max(145, ball.center.y + ball.radius + 5));
  stroke_segments(view, axis_cut_segments(skull, "z", ball.center.z, "x"), "#8a8f9a", 1);
  const n = fits.cut_normal;
  if (Math.abs(n.y) > 1e-6) { const up = -(n.z * ball.center.z) / n.y; stroke_line(view, { across: -105, up }, { across: 105, up }, cut_style, 1); }
  // The least-squares sphere's circle at this depth (it is centered elsewhere).
  const offset = ball.center.z - least_squares.center.z;
  if (Math.abs(offset) < least_squares.radius) stroke_circle(view, { across: 0, up: least_squares.center.y }, Math.sqrt(least_squares.radius ** 2 - offset ** 2), least_squares_style, 1, [6, 4]);
  stroke_circle(view, { across: 0, up: ball.center.y }, ball.radius, ball_style, 1.5, []);
  for (const x of [half_width, -half_width]) stroke_line(view, { across: x, up: -45 }, { across: x, up: 145 }, plane_style, 1.5);
  fill_dot(view, { across: fits.side_planes.widest.x, up: fits.side_planes.widest.y }, plane_style);
  label(view, { across: -half_width, up: 140 }, `half_width ${half_width.toFixed(1)} = ${(half_width / ball.radius).toFixed(2)} r`, plane_style, 6, 4);
  label(view, { across: -100, up: -40 }, `section at front ${ball.center.z.toFixed(1)}`, ball_style, 0, 0);
  label(view, { across: 0, up: 0 }, "porion middle", "#8a8f9a", 4, -4);
  fill_dot(view, { across: 0, up: 0 }, "#8a8f9a");
}

function draw_sections(): void { draw_profile(); draw_section(); }

// ---- numbers table ------------------------------------------------------------------

function fill_row(id: string, cells: string[]): void {
  const row = document.getElementById(id)!;
  const label_cell = row.firstElementChild!.outerHTML;
  row.innerHTML = label_cell + cells.map((cell) => `<td>${cell}</td>`).join("");
}

function update_numbers_table(): void {
  if (fits === null) return;
  const rows: Record<CandidateId, string> = { "least-squares": "row-least-squares", "length-ball": "row-length-ball", "side-cuts": "row-side-cuts" };
  for (const candidate of fits.candidates) {
    const center = candidate.sphere.center, r = candidate.sphere.radius;
    const size = candidate.half_width === null ? `r ${r.toFixed(1)}` : `half_width ${candidate.half_width.toFixed(1)} = ${(candidate.half_width / r).toFixed(2)} r`;
    fill_row(rows[candidate.id], [
      String(fits.vault_count), format_signed(center.y, 1), format_signed(center.z, 1), size,
      format_signed(center.y + r - fits.skull_top_up, 1),
      candidate.stats.rms.toFixed(1), format_signed(candidate.stats.min, 1), format_signed(candidate.stats.max, 1),
    ]);
  }
  if (glabella !== null) document.getElementById("glabella_text")!.textContent = `up ${glabella.y.toFixed(1)} / front ${glabella.z.toFixed(1)}`;
}

// ---- wiring -------------------------------------------------------------------------

const color_mode = document.getElementById("color_mode") as HTMLSelectElement;
const controls = bind_controls(["skull_alpha", "shape_alpha", "heat_range"], () => {
  document.getElementById("heat_range_value")!.textContent = controls.heat_range.value;
  redraw();
});
color_mode.addEventListener("change", redraw);
document.getElementById("heat_range_value")!.textContent = controls.heat_range.value;

function make_pane(canvas: HTMLCanvasElement, overlay: HTMLCanvasElement, candidate_id: CandidateId): Pane {
  const gl = canvas.getContext("webgl", { premultipliedAlpha: false })!;
  return { gl, canvas, overlay, mesh: create_translucent_mesh(gl), candidate_id };
}
for (const canvas of Array.from(document.querySelectorAll<HTMLCanvasElement>("canvas.pane"))) {
  const overlay = canvas.nextElementSibling as HTMLCanvasElement;
  panes.push(make_pane(canvas, overlay, canvas.dataset.candidate as CandidateId));
}
attach_orbit_controls(panes.map((pane) => pane.canvas), camera, redraw);
window.addEventListener("resize", draw_sections);

void load_skull("skull-side-cuts").then((loaded) => {
  if (loaded === null) return;
  skull = loaded;
  const glabella_in_file = find_landmark(skull.landmarks, "glabella");
  if (glabella_in_file === null) {
    document.getElementById("glabella_text")!.textContent = "MISSING: pick it on the skull-ball page first";
    return;
  }
  glabella = frankfurt_coordinates(skull.frame, glabella_in_file);
  fits = compute_fits(skull, glabella);
  if (fits === null) { console.error("skull-side-cuts: a fit failed"); return; }
  update_numbers_table();
  redraw();
});
