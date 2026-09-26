// Document `skull-brow-line` (plan-skull-construction-docs.md Q19/Q20): Loomis's brow line,
// the side plane's horizontal center line carried around the ball, drawn on the length ball
// with side cuts of skull-side-cuts; the skull's own landmarks (glabella, orbit rim,
// orbitale, porion) measured against it in mm and in radii. No fit of its own.
import "../../pages.css";
import { OrbitCamera, camera_eye, camera_view_projection } from "../../src/camera";
import { V3, v3, v3_scale } from "../../src/math";
import { attach_orbit_controls, bind_controls } from "../../src/explainer/orbit_controls";
import { CanvasView, canvas_view, stroke_polyline } from "../../src/explainer/canvas_view";
import { TranslucentMesh, create_translucent_mesh, draw_mesh_translucent, set_translucent_mesh } from "../../src/render";
import { find_landmark, frankfurt_coordinates } from "../../src/reference";
import { LengthSphereFit, SidePlanesFit, fit_side_planes_mirrored, fit_sphere_through_midline_extremes } from "../../src/construction_fit";
import { MeshBuilder, Skull, WORLD_PER_MM, cranium_cut_normal, format_signed, load_skull, mm_to_world, push_landmark_marker, push_side_plane, push_skull, push_sphere_with_side_cuts, side_cut_rim, size_gl_canvas, skull_view_colors as colors, sphere_outline, supraorbital_rim_point, vault_vertices } from "../../src/reference_skull_view";

const MIDLINE_BAND_MM = 10; // same length ball as skull-side-cuts
const plane_color = v3(0.5, 0.75, 1.0);
const landmark_color = v3(0.55, 1.0, 0.6);

// A landmark measured against the brow line. `p` is in frame mm; porion is the right one.
type BrowLandmark = { row_id: string; p: V3; loomis: string };

type Construction = {
  cut_normal: V3;
  length: LengthSphereFit;
  side_planes: SidePlanesFit;
  brow_up: number; // the brow line's height = the ball center's, mm
  landmarks: BrowLandmark[];
};

const camera: OrbitCamera = { pivot: v3(0, 0.45, 0), yaw: 0.6, pitch: 0.15, distance: 3.4 };
let skull: Skull | null = null;
let construction: Construction | null = null;

// ---- construction -------------------------------------------------------------------

function compute_construction(skull: Skull, glabella: V3): Construction | null {
  const cut_normal = cranium_cut_normal(glabella);
  const vault = vault_vertices(skull, cut_normal);
  const length = fit_sphere_through_midline_extremes(vault, MIDLINE_BAND_MM);
  const side_planes = fit_side_planes_mirrored(vault);
  const orbit_rim = supraorbital_rim_point(skull);
  const orbitale_in_file = find_landmark(skull.landmarks, "orbitale");
  const porion_in_file = find_landmark(skull.landmarks, "porion_r");
  if (length === null || side_planes === null || orbit_rim === null || orbitale_in_file === null || porion_in_file === null) return null;
  return {
    cut_normal,
    length,
    side_planes,
    brow_up: length.sphere.center.y,
    landmarks: [
      { row_id: "row-glabella", p: glabella, loomis: "the eyebrows sit on the line" },
      { row_id: "row-orbit-rim", p: orbit_rim, loomis: "the eyebrows sit on the line" },
      { row_id: "row-orbitale", p: frankfurt_coordinates(skull.frame, orbitale_in_file), loomis: "no eye line in the book" },
      { row_id: "row-porion", p: frankfurt_coordinates(skull.frame, porion_in_file), loomis: "the ear hangs from the line, one unit (0.5 r) tall" },
    ],
  };
}

// The brow line as a ring: the ball's horizontal section at the center's height, with the
// part outside the cut planes clamped onto them (which is exactly the planes' center line).
function brow_ring(c: Construction): V3[] {
  const { center, radius } = c.length.sphere;
  const half_width = c.side_planes.half_width;
  const points: V3[] = [];
  for (let i = 0; i < 96; i++) {
    const angle = (i / 96) * 2 * Math.PI;
    const x = Math.max(-half_width, Math.min(half_width, center.x + radius * Math.cos(angle)));
    points.push(v3(x, c.brow_up, center.z + radius * Math.sin(angle)));
  }
  return points;
}

// ---- 3D pane ------------------------------------------------------------------------

const pane_canvas = document.querySelector<HTMLCanvasElement>("canvas.pane")!;
const overlay_canvas = pane_canvas.nextElementSibling as HTMLCanvasElement;
const gl = pane_canvas.getContext("webgl", { premultipliedAlpha: false })!;
const pane_mesh: TranslucentMesh = create_translucent_mesh(gl);

function build_pane_mesh(eye_mm: V3): Float32Array {
  const builder: MeshBuilder = { data: [] };
  if (skull === null || construction === null) return new Float32Array(0);
  const { center, radius } = construction.length.sphere;
  const half_width = construction.side_planes.half_width;
  push_sphere_with_side_cuts(builder, center, radius, half_width, colors.shape, Number(controls.shape_alpha.value), eye_mm);
  push_side_plane(builder, half_width, plane_color, 0.12);
  push_side_plane(builder, -half_width, plane_color, 0.12);
  for (const landmark of construction.landmarks) push_landmark_marker(builder, landmark.p, landmark_color);
  push_landmark_marker(builder, v3(-construction.landmarks[3].p.x, 0, 0), landmark_color); // the left porion
  push_skull(builder, skull, null, eye_mm, Number(controls.skull_alpha.value), 20);
  return new Float32Array(builder.data);
}

function draw_overlay(view: CanvasView, eye_mm: V3): void {
  if (construction === null) return;
  const { center, radius } = construction.length.sphere;
  const outline = sphere_outline(center, radius, eye_mm);
  if (outline !== null) stroke_polyline(view, outline.map(mm_to_world), true, "rgba(255, 209, 102, 0.5)", 1);
  for (const x of [construction.side_planes.half_width, -construction.side_planes.half_width]) {
    const rim = side_cut_rim(center, radius, x);
    if (rim !== null) stroke_polyline(view, rim.map(mm_to_world), true, "#7fb3ff", 1.2);
  }
  stroke_polyline(view, brow_ring(construction).map(mm_to_world), true, "#ffd166", 2.2);
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
// The skull cut along the midline (x = 0), seen from the right: front -> right, up -> up.

type SectionPoint = { across: number; up: number }; // mm
type SectionSegment = { a: SectionPoint; b: SectionPoint };

function midline_cut_segments(skull: Skull): SectionSegment[] {
  const segments: SectionSegment[] = [];
  const crossing = (p: V3, q: V3): SectionPoint | null => {
    if ((p.x >= 0) === (q.x >= 0)) return null;
    const t = p.x / (p.x - q.x);
    return { across: p.z + (q.z - p.z) * t, up: p.y + (q.y - p.y) * t };
  };
  for (let i = 0; i < skull.triangle_indices.length; i += 3) {
    const a = skull.positions[skull.triangle_indices[i]], b = skull.positions[skull.triangle_indices[i + 1]], c = skull.positions[skull.triangle_indices[i + 2]];
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

// A negative dx puts the text to the left of the point (right-aligned) so labels near the
// face stay inside the canvas.
function label(view: SectionView, p: SectionPoint, text: string, style: string, dx_px: number, dy_px: number): void {
  const q = view.px(p);
  view.ctx.fillStyle = style; view.ctx.font = `${12 * view.dpr}px system-ui, sans-serif`;
  view.ctx.textAlign = dx_px < 0 ? "right" : "left";
  view.ctx.fillText(text, q.x + dx_px * view.dpr, q.y + dy_px * view.dpr);
  view.ctx.textAlign = "left";
}

const profile_canvas = document.getElementById("profile_canvas") as HTMLCanvasElement;
const ball_style = "#ffd166", plane_style = "#7fb3ff", cut_style = "rgba(255, 168, 77, 0.7)", landmark_style = "#8cffa0";
const ACROSS_MIN = -120, ACROSS_MAX = 110;

function draw_profile(): void {
  if (skull === null || construction === null) return;
  const ball = construction.length.sphere;
  const view = section_view(profile_canvas, ACROSS_MIN, ACROSS_MAX, -45, Math.max(145, ball.center.y + ball.radius + 5));
  stroke_segments(view, midline_cut_segments(skull), "#8a8f9a", 1);
  const n = construction.cut_normal;
  if (Math.abs(n.y) > 1e-6) stroke_line(view, { across: ACROSS_MIN, up: -(n.z * ACROSS_MIN) / n.y }, { across: ACROSS_MAX, up: -(n.z * ACROSS_MAX) / n.y }, cut_style, 1, []);
  stroke_circle(view, { across: ball.center.z, up: ball.center.y }, ball.radius, ball_style, 1.5);
  stroke_line(view, { across: ACROSS_MIN, up: construction.brow_up }, { across: ACROSS_MAX, up: construction.brow_up }, ball_style, 2, []);
  // The porions are behind the midline plane; the profile shows them projected onto it.
  for (const landmark of construction.landmarks) {
    const p = { across: landmark.p.z, up: landmark.p.y };
    fill_dot(view, p, landmark_style);
    stroke_line(view, p, { across: p.across, up: construction.brow_up }, landmark_style, 1, [3, 3]);
    const name = landmark.row_id.replace("row-", "").replace("-", " ");
    const dx = -6; // the face landmarks sit at the right edge, so every label goes leftwards
    label(view, p, `${name} ${format_signed(p.up - construction.brow_up, 1)}`, landmark_style, dx, landmark.p.y > construction.brow_up ? -6 : 14);
  }
  label(view, { across: ACROSS_MIN + 5, up: construction.brow_up }, `brow line up ${construction.brow_up.toFixed(1)}`, ball_style, 0, -6);
  label(view, { across: ACROSS_MIN + 5, up: -40 }, `length ball r ${ball.radius.toFixed(1)}, cut at ±${construction.side_planes.half_width.toFixed(1)}`, plane_style, 0, 0);
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
  fill_row("row-brow-line", [format_signed(construction.brow_up, 1), "0", "0", "the ball's equator"]);
  for (const landmark of construction.landmarks) {
    const offset = landmark.p.y - construction.brow_up;
    fill_row(landmark.row_id, [format_signed(landmark.p.y, 1), format_signed(offset, 1), `${format_signed(offset / radius, 2)} r`, landmark.loomis]);
  }
  const center = construction.length.sphere.center;
  document.getElementById("ball_text")!.textContent = `center up ${center.y.toFixed(1)} / front ${center.z.toFixed(1)}, r ${radius.toFixed(1)}, half_width ${construction.side_planes.half_width.toFixed(1)}`;
}

// ---- wiring -------------------------------------------------------------------------

const controls = bind_controls(["skull_alpha", "shape_alpha"], redraw);
attach_orbit_controls([pane_canvas], camera, redraw);
window.addEventListener("resize", draw_profile);

void load_skull("skull-brow-line").then((loaded) => {
  if (loaded === null) return;
  skull = loaded;
  const glabella_in_file = find_landmark(skull.landmarks, "glabella");
  if (glabella_in_file === null) {
    document.getElementById("ball_text")!.textContent = "MISSING glabella: pick it on the skull-ball page first";
    return;
  }
  construction = compute_construction(skull, frankfurt_coordinates(skull.frame, glabella_in_file));
  if (construction === null) { console.error("skull-brow-line: the ball, the cuts or a landmark is missing"); return; }
  update_numbers_table();
  redraw();
});
