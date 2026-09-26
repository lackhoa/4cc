// Document `skull-eyes` (plan-skull-construction-docs.md Q26): the eye sockets. Two corners
// of one orbit are hand-picked on the skull and mirrored: the inner corner (dacryon, where
// the rim meets the nose bridge) and the outer corner (the frontozygomatic suture, the outer
// rim). Their midpoint stands in for the eye's center: the eyeball hangs about 1 mm above
// and 1 mm outside the socket's middle, less than a pick's precision. The socket is much bigger than the eye you see, so "five eyes wide" is
// tested on the centers only (two eye widths apart = 2/5 of the face width), and the orbit's
// width is shown without a verdict. Same ball, side planes, brow line, nasal spine and zygion
// as skull-cheekbones; the section is horizontal at the orbit center's height. The nasion
// (the top of the nose bridge, where the nasal bones meet the brow bone on the midline) is
// picked here too: both corners are picked at the nasion's height, the inner corner on the
// inner rim and the outer corner at the outer rim's furthest-out point.
import "../../pages.css";
import { OrbitCamera, camera_eye, camera_pen_ray, camera_view_projection } from "../../src/camera";
import { V3, v3, v3_scale } from "../../src/math";
import { attach_orbit_controls, bind_controls } from "../../src/explainer/orbit_controls";
import { CanvasView, canvas_view, stroke_polyline } from "../../src/explainer/canvas_view";
import { TranslucentMesh, create_translucent_mesh, draw_mesh_translucent, set_translucent_mesh } from "../../src/render";
import { find_landmark, frankfurt_coordinates } from "../../src/reference";
import { LengthSphereFit, SidePlanesFit, fit_side_planes_mirrored, fit_sphere_through_midline_extremes } from "../../src/construction_fit";
import { LANDMARKS_URL, MeshBuilder, Skull, WORLD_PER_MM, cranium_cut_normal, format_signed, load_skull, mm_to_world, pick_skull_vertex, push_landmark_marker, push_side_plane, push_skull, push_sphere_with_side_cuts, save_landmark, side_cut_rim, size_gl_canvas, skull_view_colors as colors, sphere_outline, vault_vertices } from "../../src/reference_skull_view";

const MIDLINE_BAND_MM = 10; // same length ball as skull-side-cuts and skull-brow-line
const plane_color = v3(0.5, 0.75, 1.0);
const landmark_color = v3(0.55, 1.0, 0.6);
const zygion_color = v3(1.0, 0.69, 0.44);
const orbit_corner_color = v3(0.75, 0.6, 1.0);
const orbit_center_color = v3(1.0, 1.0, 1.0);
const nasion_color = v3(0.55, 1.0, 0.6);

type Construction = {
  cut_normal: V3;
  length: LengthSphereFit;
  side_planes: SidePlanesFit;
  brow_up: number; // the brow line's height = the ball center's, mm
  glabella: V3;
};

// Front view by default: the eyes are a width proportion.
const camera: OrbitCamera = { pivot: v3(0, 0.2, 0), yaw: 0, pitch: 0.05, distance: 3.8 };
let skull: Skull | null = null;
let construction: Construction | null = null;
// Landmarks in frame mm. The nasal spine and the zygion come from the file (picked on
// skull-face-thirds and skull-cheekbones); the orbit corners are picked here, and start from
// a guess when the file has none.
let nasal_spine: V3 | null = null;
let zygion: V3 | null = null;
let orbit_inner_corner: V3 | null = null; // `orbit_inner_corner` in the file
let orbit_outer_corner: V3 | null = null; // `orbit_outer_corner` in the file
let nasion: V3 | null = null; // `nasion` in the file

// ---- construction -------------------------------------------------------------------

function compute_construction(skull: Skull, glabella: V3): Construction | null {
  const cut_normal = cranium_cut_normal(glabella);
  const vault = vault_vertices(skull, cut_normal);
  const length = fit_sphere_through_midline_extremes(vault, MIDLINE_BAND_MM);
  const side_planes = fit_side_planes_mirrored(vault);
  if (length === null || side_planes === null) return null;
  return { cut_normal, length, side_planes, brow_up: length.sphere.center.y, glabella };
}

// First guesses when the file has no corners yet: the mesh vertex nearest a nominal spot in
// the frame. The orbitale (the rim's lowest point, on the Frankfurt plane, so up = 0) pins
// the height: the rim is roughly 35 mm tall, so both corners sit about 20 mm above it. The
// inner corner is a thumb's width from the midline at the nose bridge; the outer corner is
// under the brow's outer end, a few mm inside the zygion. A click on the mesh refines them.
const CORNER_UP_MM = 20;
const INNER_CORNER_SIDE_MM = 10;
const OUTER_CORNER_INSIDE_ZYGION_MM = 12;
function nearest_vertex(skull: Skull, target: V3): V3 | null {
  let best: V3 | null = null, best_distance = Infinity;
  for (const p of skull.positions) {
    const d = Math.hypot(p.x - target.x, p.y - target.y, p.z - target.z);
    if (d < best_distance) { best = p; best_distance = d; }
  }
  return best;
}
function guess_orbit_inner_corner(skull: Skull, glabella: V3): V3 | null {
  return nearest_vertex(skull, v3(INNER_CORNER_SIDE_MM, CORNER_UP_MM, glabella.z - 10));
}
function guess_orbit_outer_corner(skull: Skull, zygion: V3, glabella: V3): V3 | null {
  return nearest_vertex(skull, v3(zygion.x - OUTER_CORNER_INSIDE_ZYGION_MM, CORNER_UP_MM, glabella.z - 20));
}

// First guess for the nasion: the deepest midline vertex of the nose bridge just below the
// glabella (the bridge dips between the brow and the nasal bones).
const NASION_MIDLINE_BAND_MM = 2;
const NASION_BELOW_GLABELLA_MAX_MM = 25;
const NASION_BEHIND_GLABELLA_MAX_MM = 15; // the bridge only: further back the midline band runs into the nasal cavity, and then the back of the skull
function guess_nasion(skull: Skull, glabella: V3): V3 | null {
  let best: V3 | null = null;
  for (const p of skull.positions) {
    if (Math.abs(p.x) > NASION_MIDLINE_BAND_MM || p.z < glabella.z - NASION_BEHIND_GLABELLA_MAX_MM || p.y > glabella.y || p.y < glabella.y - NASION_BELOW_GLABELLA_MAX_MM) continue;
    if (best === null || p.z < best.z) best = p;
  }
  return best;
}

// The eye's center: the socket's middle, halfway between the two corners (the globe is ~1 mm
// up and out of it, ignored).
function orbit_center(): V3 | null {
  if (orbit_inner_corner === null || orbit_outer_corner === null) return null;
  return v3(0.5 * (orbit_inner_corner.x + orbit_outer_corner.x), 0.5 * (orbit_inner_corner.y + orbit_outer_corner.y), 0.5 * (orbit_inner_corner.z + orbit_outer_corner.z));
}

// The landmarks are picked on one side; the skull is treated as symmetric, so the mirror
// stands in for the other side.
function mirrored(p: V3): V3 { return v3(-p.x, p.y, p.z); }

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
  push_landmark_marker(builder, construction.glabella, colors.landmark);
  if (nasal_spine !== null) push_landmark_marker(builder, nasal_spine, landmark_color);
  if (zygion !== null) { push_landmark_marker(builder, zygion, zygion_color); push_landmark_marker(builder, mirrored(zygion), zygion_color); }
  for (const corner of [orbit_inner_corner, orbit_outer_corner]) {
    if (corner !== null) { push_landmark_marker(builder, corner, orbit_corner_color); push_landmark_marker(builder, mirrored(corner), orbit_corner_color); }
  }
  const center_point = orbit_center();
  if (center_point !== null) { push_landmark_marker(builder, center_point, orbit_center_color); push_landmark_marker(builder, mirrored(center_point), orbit_center_color); }
  if (nasion !== null) push_landmark_marker(builder, nasion, nasion_color);
  push_skull(builder, skull, null, eye_mm, Number(controls.skull_alpha.value), 20);
  return new Float32Array(builder.data);
}

// The brow line as a ring (skull-brow-line), the nose line and the cheekbone width as bars,
// and the eyes: the corner-to-corner bar of each orbit and the line through both centers,
// which is where the eye line sits on this skull.
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
  if (nasal_spine !== null) stroke_polyline(view, [v3(-30, nasal_spine.y, nasal_spine.z), v3(30, nasal_spine.y, nasal_spine.z)].map(mm_to_world), false, landmark_style, 2);
  if (zygion !== null) stroke_polyline(view, [zygion, mirrored(zygion)].map(mm_to_world), false, zygion_style, 1.2);
  // The nasion's height across both sockets: all four corners should sit on this bar.
  if (nasion !== null) stroke_polyline(view, [v3(-60, nasion.y, nasion.z), v3(60, nasion.y, nasion.z)].map(mm_to_world), false, nasion_style, 1.2);
  if (orbit_inner_corner !== null && orbit_outer_corner !== null) {
    stroke_polyline(view, [orbit_inner_corner, orbit_outer_corner].map(mm_to_world), false, orbit_corner_style, 2);
    stroke_polyline(view, [mirrored(orbit_inner_corner), mirrored(orbit_outer_corner)].map(mm_to_world), false, orbit_corner_style, 2);
  }
  const center_point = orbit_center();
  if (center_point !== null) stroke_polyline(view, [center_point, mirrored(center_point)].map(mm_to_world), false, orbit_center_style, 2);
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

function redraw(): void { draw_pane(); draw_section(); }

// ---- horizontal section -------------------------------------------------------------
// The skull cut by the horizontal plane at the orbit center's height, seen from above:
// across = side (right of the skull to the right), up on the canvas = front. Both sockets
// are cut open at their middle, with the ball's circle and the side planes on top.

type SectionPoint = { across: number; up: number }; // mm
type SectionSegment = { a: SectionPoint; b: SectionPoint };

function horizontal_cut_segments(mesh: Skull, height: number): SectionSegment[] {
  const segments: SectionSegment[] = [];
  const crossing = (p: V3, q: V3): SectionPoint | null => {
    if ((p.y >= height) === (q.y >= height)) return null;
    const t = (p.y - height) / (p.y - q.y);
    return { across: p.x + (q.x - p.x) * t, up: p.z + (q.z - p.z) * t };
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

const section_canvas = document.getElementById("section_canvas") as HTMLCanvasElement;
const ball_style = "#ffd166", plane_style = "#7fb3ff", landmark_style = "#8cffa0", zygion_style = "#ffb070", nasion_style = "#8cffa0", orbit_corner_style = "#bf99ff", orbit_center_style = "#ffffff";
const ACROSS_MIN = -110, ACROSS_MAX = 110, FRONT_MIN = -110, FRONT_MAX = 120;

function draw_section(): void {
  const center_point = orbit_center();
  if (skull === null || construction === null || zygion === null || orbit_inner_corner === null || orbit_outer_corner === null || center_point === null) return;
  const ball = construction.length.sphere;
  const half_width = construction.side_planes.half_width;
  const view = section_view(section_canvas, ACROSS_MIN, ACROSS_MAX, FRONT_MIN, FRONT_MAX);
  stroke_segments(view, horizontal_cut_segments(skull, center_point.y), "#8a8f9a", 1);
  // The ball's circle at this height (empty when the cut is below the ball).
  const drop = center_point.y - ball.center.y;
  if (Math.abs(drop) < ball.radius) stroke_circle(view, { across: ball.center.x, up: ball.center.z }, Math.sqrt(ball.radius * ball.radius - drop * drop), ball_style, 1.5);
  for (const x of [half_width, -half_width]) stroke_line(view, { across: x, up: FRONT_MIN }, { across: x, up: FRONT_MAX }, plane_style, 1.2, []);
  label(view, { across: half_width, up: FRONT_MAX - 5 }, `side plane ${half_width.toFixed(1)}`, plane_style, 4, 10);
  // The ear hole is the frame's origin.
  fill_dot(view, { across: 0, up: 0 }, "#8a8f9a");
  label(view, { across: 0, up: 0 }, "ear hole", "#8a8f9a", 6, 14);
  // The zygion sits below this cut; its dots show the cheekbone width for scale.
  for (const z of [zygion, mirrored(zygion)]) fill_dot(view, { across: z.x, up: z.z }, zygion_style);
  label(view, { across: zygion.x, up: zygion.z }, `zygion, ${(center_point.y - zygion.y).toFixed(1)} below the cut`, zygion_style, -6, 14);
  // The corners, projected onto the cut (they sit a few mm above or below it), and the centers.
  for (const corner of [orbit_inner_corner, orbit_outer_corner, mirrored(orbit_inner_corner), mirrored(orbit_outer_corner)]) fill_dot(view, { across: corner.x, up: corner.z }, orbit_corner_style);
  for (const c of [center_point, mirrored(center_point)]) fill_dot(view, { across: c.x, up: c.z }, orbit_center_style);
  stroke_line(view, { across: center_point.x, up: center_point.z }, { across: -center_point.x, up: center_point.z }, orbit_center_style, 1.2, [6, 4]);
  label(view, { across: orbit_outer_corner.x, up: orbit_outer_corner.z }, `outer corner side ${orbit_outer_corner.x.toFixed(1)}`, orbit_corner_style, 6, -6);
  label(view, { across: -orbit_inner_corner.x, up: orbit_inner_corner.z }, `inner corner side ${orbit_inner_corner.x.toFixed(1)}`, orbit_corner_style, -6, -6);
  label(view, { across: -center_point.x, up: center_point.z }, `eye center side ${center_point.x.toFixed(1)}, up ${format_signed(center_point.y - construction.brow_up, 1)}`, orbit_center_style, -6, 28);
}

// ---- numbers table ------------------------------------------------------------------

function fill_row(id: string, cells: string[]): void {
  const row = document.getElementById(id)!;
  const label_cell = row.firstElementChild!.outerHTML;
  row.innerHTML = label_cell + cells.map((cell) => `<td>${cell}</td>`).join("");
}

const ROW_IDS = ["row-brow-nose", "row-nasion-nose", "row-inner-corner-nasion", "row-outer-corner-nasion", "row-center-nose", "row-centers-apart", "row-orbit-width", "row-inner-gap", "row-outer-corner-width"];

function update_numbers_table(): void {
  if (construction === null) return;
  const half_width = construction.side_planes.half_width;
  const radius = construction.length.sphere.radius;
  const center_point = orbit_center();
  fill_row("row-half-width", [`${half_width.toFixed(1)} mm`, `${(half_width / radius).toFixed(2)} r`, "Loomis: the ball's flat"]);
  if (nasal_spine === null || zygion === null || orbit_inner_corner === null || orbit_outer_corner === null || center_point === null || nasion === null) {
    for (const id of ROW_IDS) fill_row(id, ["missing", "", ""]);
  } else {
    const brow_to_nose = construction.brow_up - nasal_spine.y;
    const face_width = 2 * zygion.x;
    const center_above_nose = center_point.y - nasal_spine.y;
    const centers_apart = 2 * center_point.x;
    const orbit_width = orbit_outer_corner.x - orbit_inner_corner.x;
    const inner_gap = 2 * orbit_inner_corner.x;
    fill_row("row-brow-nose", [`${brow_to_nose.toFixed(1)} mm`, "1.00 of brow → nose", ""]);
    const nasion_above_nose = nasion.y - nasal_spine.y;
    fill_row("row-nasion-nose", [`${format_signed(nasion_above_nose, 1)} mm`, `${format_signed(nasion_above_nose / brow_to_nose, 2)} of brow → nose`, "no number (the nose bridge's top)"]);
    fill_row("row-inner-corner-nasion", [`${format_signed(orbit_inner_corner.y - nasion.y, 1)} mm`, "", "the rule used here: level (0)"]);
    fill_row("row-outer-corner-nasion", [`${format_signed(orbit_outer_corner.y - nasion.y, 1)} mm`, "", "the rule used here: level (0)"]);
    fill_row("row-center-nose", [`${format_signed(center_above_nose, 1)} mm`, `${format_signed(center_above_nose / brow_to_nose, 2)} of brow → nose`, "Finch: halfway (0.50); Loomis: just under the brow line"]);
    // Five eyes across the face put the centers at 1.5 and 3.5 eye widths: 2/5 of the width apart.
    fill_row("row-centers-apart", [`${centers_apart.toFixed(1)} mm`, `${(centers_apart / face_width).toFixed(2)} of the cheekbone width`, "five eyes wide: 0.40"]);
    fill_row("row-orbit-width", [`${orbit_width.toFixed(1)} mm`, `${(orbit_width / face_width).toFixed(2)} of the cheekbone width`, "no verdict: the socket, not the eye (five eyes: 0.20)"]);
    fill_row("row-inner-gap", [`${inner_gap.toFixed(1)} mm`, `${(inner_gap / orbit_width).toFixed(2)} of the orbit width`, "no verdict: \"one eye between the eyes\" is the eye, not the socket"]);
    fill_row("row-outer-corner-width", [`${orbit_outer_corner.x.toFixed(1)} mm`, `${(orbit_outer_corner.x / half_width).toFixed(2)} of the side plane, ${(orbit_outer_corner.x / zygion.x).toFixed(2)} of the zygion`, "no number"]);
  }
  const center = construction.length.sphere.center;
  document.getElementById("ball_text")!.textContent = `center up ${center.y.toFixed(1)} / front ${center.z.toFixed(1)}, r ${radius.toFixed(1)}`;
  const point_text = (p: V3 | null): string => (p === null ? "missing" : `side ${format_signed(p.x, 1)}, up ${format_signed(p.y, 1)}, front ${format_signed(p.z, 1)}`);
  document.getElementById("orbit_inner_corner_text")!.textContent = point_text(orbit_inner_corner);
  document.getElementById("orbit_outer_corner_text")!.textContent = point_text(orbit_outer_corner);
  document.getElementById("orbit_center_text")!.textContent = point_text(center_point);
  document.getElementById("nasion_text")!.textContent = point_text(nasion);
}

// ---- picking + saving ---------------------------------------------------------------

const pick_mode = document.getElementById("pick_mode") as HTMLInputElement;
// Which point a click sets: the radio buttons `pick_target` (value "inner", "outer" or "nasion").
type PickTarget = "inner" | "outer" | "nasion";
function pick_target(): PickTarget {
  const checked = document.querySelector<HTMLInputElement>("input[name=pick_target]:checked");
  if (checked === null) return "inner";
  return checked.value === "outer" || checked.value === "nasion" ? checked.value : "inner";
}

function attach_pick(): void {
  let down: { x: number; y: number } | null = null;
  pane_canvas.addEventListener("pointerdown", (event) => { down = { x: event.clientX, y: event.clientY }; });
  pane_canvas.addEventListener("pointerup", (event) => {
    if (down === null || skull === null || !pick_mode.checked) return;
    const moved = Math.hypot(event.clientX - down.x, event.clientY - down.y);
    down = null;
    if (moved > 4) return;
    const rect = pane_canvas.getBoundingClientRect();
    const ray = camera_pen_ray(camera, { x: event.clientX - rect.left, y: event.clientY - rect.top }, rect.width, rect.height);
    const picked = pick_skull_vertex(skull, v3_scale(ray.origin, 1 / WORLD_PER_MM), ray.direction);
    if (picked === null) return;
    const target = pick_target();
    // The corners are stored on the right side (side >= 0) whichever side was clicked; the
    // nasion is a midline point and is stored as picked.
    const on_right = picked.x >= 0 ? picked : mirrored(picked);
    if (target === "nasion") nasion = picked;
    else if (target === "inner") orbit_inner_corner = on_right;
    else orbit_outer_corner = on_right;
    update_numbers_table();
    redraw();
  });
}

// All three points go into the file, one after the other (each save rewrites the whole file
// from the mesh's landmark list, which the previous save already updated).
async function save_landmarks(): Promise<void> {
  if (skull === null || orbit_inner_corner === null || orbit_outer_corner === null || nasion === null) return;
  const status = document.getElementById("save_status")!;
  status.textContent = await save_landmark(skull, LANDMARKS_URL, "nasion", nasion);
  status.textContent += "; " + await save_landmark(skull, LANDMARKS_URL, "orbit_inner_corner", orbit_inner_corner);
  status.textContent += "; " + await save_landmark(skull, LANDMARKS_URL, "orbit_outer_corner", orbit_outer_corner);
}

// ---- wiring -------------------------------------------------------------------------

const controls = bind_controls(["skull_alpha", "shape_alpha"], redraw);
attach_orbit_controls([pane_canvas], camera, redraw);
attach_pick();
document.getElementById("save_landmarks")!.addEventListener("click", () => { void save_landmarks(); });
window.addEventListener("resize", draw_section);

void load_skull("skull-eyes").then((loaded) => {
  if (loaded === null) return;
  skull = loaded;
  const from_file = (name: string): V3 | null => {
    const landmark = find_landmark(skull!.landmarks, name);
    return landmark === null ? null : frankfurt_coordinates(skull!.frame, landmark);
  };
  const glabella = from_file("glabella");
  if (glabella === null) {
    document.getElementById("ball_text")!.textContent = "MISSING glabella: pick it on the skull-ball page first";
    return;
  }
  construction = compute_construction(skull, glabella);
  if (construction === null) { console.error("skull-eyes: the ball or the cuts failed"); return; }
  nasal_spine = from_file("nasal_spine");
  if (nasal_spine === null) { document.getElementById("ball_text")!.textContent = "MISSING nasal_spine: pick it on the skull-face-thirds page first"; return; }
  zygion = from_file("zygion");
  if (zygion === null) { document.getElementById("ball_text")!.textContent = "MISSING zygion: pick it on the skull-cheekbones page first"; return; }
  const nasion_in_file = from_file("nasion");
  nasion = nasion_in_file !== null ? nasion_in_file : guess_nasion(skull, glabella);
  const inner_in_file = from_file("orbit_inner_corner");
  const outer_in_file = from_file("orbit_outer_corner");
  orbit_inner_corner = inner_in_file !== null ? inner_in_file : guess_orbit_inner_corner(skull, glabella);
  orbit_outer_corner = outer_in_file !== null ? outer_in_file : guess_orbit_outer_corner(skull, zygion, glabella);
  const source = (in_file: V3 | null): string => (in_file === null ? "guessed from the mesh, not saved yet" : "from the landmarks file");
  document.getElementById("save_status")!.textContent = `nasion ${source(nasion_in_file)}; inner corner ${source(inner_in_file)}; outer corner ${source(outer_in_file)}`;
  update_numbers_table();
  redraw();
});
