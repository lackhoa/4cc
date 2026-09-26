// Document `skull-cheekbones` (plan-skull-construction-docs.md Q24, Q25): the first width rule.
// The zygion (the widest point of the zygomatic arch, hand-picked on the skull, one side
// mirrored) measured against Loomis's side plane from skull-side-cuts (is the cheekbone
// inside, on, or outside the ball's flat), and the arch's lowest underside point (Q25, also
// picked) against the nose line (Finch: the cheekbones sit "just above" it). Same ball, brow
// line and nasal spine as skull-face-thirds; the section here is horizontal, at the zygion's
// height, since the zygion is off the midline.
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
const zygomatic_bottom_color = v3(1.0, 0.45, 0.6);

type Construction = {
  cut_normal: V3;
  length: LengthSphereFit;
  side_planes: SidePlanesFit;
  brow_up: number; // the brow line's height = the ball center's, mm
  glabella: V3;
};

// Front view by default: the cheekbone is a width proportion.
const camera: OrbitCamera = { pivot: v3(0, 0.2, 0), yaw: 0, pitch: 0.05, distance: 3.8 };
let skull: Skull | null = null;
let construction: Construction | null = null;
// Landmarks in frame mm. The nasal spine comes from the file (picked on skull-face-thirds);
// the zygion is picked here, and starts from a guess on the mesh when the file has none.
let nasal_spine: V3 | null = null;
let zygion: V3 | null = null;
let zygomatic_bottom: V3 | null = null; // `zygomatic_bottom` in the file

// ---- construction -------------------------------------------------------------------

function compute_construction(skull: Skull, glabella: V3): Construction | null {
  const cut_normal = cranium_cut_normal(glabella);
  const vault = vault_vertices(skull, cut_normal);
  const length = fit_sphere_through_midline_extremes(vault, MIDLINE_BAND_MM);
  const side_planes = fit_side_planes_mirrored(vault);
  if (length === null || side_planes === null) return null;
  return { cut_normal, length, side_planes, brow_up: length.sphere.center.y, glabella };
}

// First guess when the zygion is not in the file yet: the widest vertex in the lower half of
// the band between the nose line and the brow line, in front of the ear hole (the frame's
// origin). Higher up the temple narrows and then the braincase gets wider than the arch;
// behind the ear hole the braincase is wider too. A click on the mesh refines it.
const ARCH_MIN_FRONT_MM = 10;
function guess_zygion(skull: Skull, nose_up: number, brow_up: number): V3 | null {
  const band_top = nose_up + 0.5 * (brow_up - nose_up);
  let best: V3 | null = null;
  for (const p of skull.positions) {
    if (p.y < nose_up || p.y > band_top || p.z < ARCH_MIN_FRONT_MM) continue;
    if (best === null || p.x > best.x) best = p;
  }
  return best;
}

// First guess for the arch's underside when the file has none: the lowest vertex of the arch
// within a few mm of the zygion, front to back. The arch is the band of vertices well out to
// the side; without the side floor the guess slides down the cheek body to the teeth. Khoa's
// pick is further forward, the arch's front end at the cheek body, which is ~7 mm lower.
const ARCH_MIN_SIDE_MM = 45;
const ARCH_BOTTOM_FRONT_BAND_MM = 5;
function guess_zygomatic_bottom(skull: Skull, zygion: V3, nose_up: number): V3 | null {
  let best: V3 | null = null;
  for (const p of skull.positions) {
    if (Math.abs(p.z - zygion.z) > ARCH_BOTTOM_FRONT_BAND_MM || p.x < ARCH_MIN_SIDE_MM || p.y < nose_up || p.y > zygion.y) continue;
    if (best === null || p.y < best.y) best = p;
  }
  return best;
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
  if (zygomatic_bottom !== null) { push_landmark_marker(builder, zygomatic_bottom, zygomatic_bottom_color); push_landmark_marker(builder, mirrored(zygomatic_bottom), zygomatic_bottom_color); }
  push_skull(builder, skull, null, eye_mm, Number(controls.skull_alpha.value), 20);
  return new Float32Array(builder.data);
}

// The brow line as a ring (skull-brow-line), the nose line as a short bar across the front
// of the face, and the cheekbone line: a bar from zygion to mirrored zygion, the width the
// face actually has at that height.
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
  if (zygion !== null) stroke_polyline(view, [zygion, mirrored(zygion)].map(mm_to_world), false, zygion_style, 2);
  // Finch's cheekbone line: the arch's underside, mirrored.
  if (zygomatic_bottom !== null) stroke_polyline(view, [zygomatic_bottom, mirrored(zygomatic_bottom)].map(mm_to_world), false, zygomatic_bottom_style, 2);
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
// The skull cut by the horizontal plane at the zygion's height, seen from above: across =
// side (right of the skull to the right), up on the canvas = front. The ball's circle at
// that height and the two side planes are drawn on top, so the arch's width can be read
// against both.

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
const ball_style = "#ffd166", plane_style = "#7fb3ff", landmark_style = "#8cffa0", zygion_style = "#ffb070", zygomatic_bottom_style = "#ff7399";
const ACROSS_MIN = -110, ACROSS_MAX = 110, FRONT_MIN = -110, FRONT_MAX = 120;

function draw_section(): void {
  if (skull === null || construction === null || zygion === null) return;
  const ball = construction.length.sphere;
  const half_width = construction.side_planes.half_width;
  const view = section_view(section_canvas, ACROSS_MIN, ACROSS_MAX, FRONT_MIN, FRONT_MAX);
  stroke_segments(view, horizontal_cut_segments(skull, zygion.y), "#8a8f9a", 1);
  // The ball's circle at this height (empty when the cut is below the ball).
  const drop = zygion.y - ball.center.y;
  if (Math.abs(drop) < ball.radius) stroke_circle(view, { across: ball.center.x, up: ball.center.z }, Math.sqrt(ball.radius * ball.radius - drop * drop), ball_style, 1.5);
  for (const x of [half_width, -half_width]) stroke_line(view, { across: x, up: FRONT_MIN }, { across: x, up: FRONT_MAX }, plane_style, 1.2, []);
  label(view, { across: half_width, up: FRONT_MAX - 5 }, `side plane ${half_width.toFixed(1)}`, plane_style, 4, 10);
  // The ear hole is the frame's origin.
  fill_dot(view, { across: 0, up: 0 }, "#8a8f9a");
  label(view, { across: 0, up: 0 }, "ear hole", "#8a8f9a", 6, 14);
  for (const z of [zygion, mirrored(zygion)]) fill_dot(view, { across: z.x, up: z.z }, zygion_style);
  stroke_line(view, { across: zygion.x, up: zygion.z }, { across: -zygion.x, up: zygion.z }, zygion_style, 1.2, [6, 4]);
  label(view, { across: zygion.x, up: zygion.z }, `zygion side ${zygion.x.toFixed(1)}, up ${format_signed(zygion.y - construction.brow_up, 1)}`, zygion_style, 6, -6);
  // The zygomatic bottom sits below this cut; its dots show where along the arch it was picked.
  if (zygomatic_bottom !== null) {
    for (const a of [zygomatic_bottom, mirrored(zygomatic_bottom)]) fill_dot(view, { across: a.x, up: a.z }, zygomatic_bottom_style);
    label(view, { across: -zygomatic_bottom.x, up: zygomatic_bottom.z }, `zygomatic bottom, ${(zygion.y - zygomatic_bottom.y).toFixed(1)} below the cut`, zygomatic_bottom_style, -6, 14);
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
  const half_width = construction.side_planes.half_width;
  const radius = construction.length.sphere.radius;
  fill_row("row-half-width", [`${half_width.toFixed(1)} mm`, `${(half_width / radius).toFixed(2)} r`, "Loomis: the ball's flat"]);
  if (nasal_spine === null || zygion === null) {
    for (const id of ["row-brow-nose", "row-zygion-nose", "row-zygomatic-bottom-nose", "row-zygion-width", "row-zygion-front"]) fill_row(id, ["missing", "", ""]);
  } else {
    const brow_to_nose = construction.brow_up - nasal_spine.y;
    const above_nose = zygion.y - nasal_spine.y;
    fill_row("row-brow-nose", [`${brow_to_nose.toFixed(1)} mm`, "1.00 of brow → nose", ""]);
    fill_row("row-zygion-nose", [`${format_signed(above_nose, 1)} mm`, `${format_signed(above_nose / brow_to_nose, 2)} of brow → nose`, "no number (the widest point, not Finch's line)"]);
    if (zygomatic_bottom === null) fill_row("row-zygomatic-bottom-nose", ["missing", "", ""]);
    else {
      const arch_above_nose = zygomatic_bottom.y - nasal_spine.y;
      fill_row("row-zygomatic-bottom-nose", [`${format_signed(arch_above_nose, 1)} mm`, `${format_signed(arch_above_nose / brow_to_nose, 2)} of brow → nose`, `Finch: "just above" the nose line`]);
    }
    fill_row("row-zygion-width", [`${zygion.x.toFixed(1)} mm`, `${(zygion.x / half_width).toFixed(2)} of the side plane`, "Loomis: on the side plane (1.00)"]);
    // The frame's origin is the ear hole (the porion middle), so front = mm in front of it.
    fill_row("row-zygion-front", [`${format_signed(zygion.z, 1)} mm`, `${format_signed(zygion.z / radius, 2)} r`, "no number"]);
  }
  const center = construction.length.sphere.center;
  document.getElementById("ball_text")!.textContent = `center up ${center.y.toFixed(1)} / front ${center.z.toFixed(1)}, r ${radius.toFixed(1)}`;
  document.getElementById("zygion_text")!.textContent = zygion === null ? "missing" : `side ${format_signed(zygion.x, 1)}, up ${format_signed(zygion.y, 1)}, front ${format_signed(zygion.z, 1)}`;
  document.getElementById("zygomatic_bottom_text")!.textContent = zygomatic_bottom === null ? "missing" : `side ${format_signed(zygomatic_bottom.x, 1)}, up ${format_signed(zygomatic_bottom.y, 1)}, front ${format_signed(zygomatic_bottom.z, 1)}`;
}

// ---- picking + saving ---------------------------------------------------------------

const pick_mode = document.getElementById("pick_mode") as HTMLInputElement;
// Which landmark a click sets: the radio buttons `pick_target` (value "zygion" or "zygomatic_bottom").
function pick_target(): "zygion" | "zygomatic_bottom" {
  const checked = document.querySelector<HTMLInputElement>("input[name=pick_target]:checked");
  return checked !== null && checked.value === "zygomatic_bottom" ? "zygomatic_bottom" : "zygion";
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
    // Stored on the right side (side >= 0) whichever side was clicked.
    const on_right = picked.x >= 0 ? picked : mirrored(picked);
    if (pick_target() === "zygion") zygion = on_right; else zygomatic_bottom = on_right;
    update_numbers_table();
    redraw();
  });
}

// Both landmarks go into the file, one after the other (each save rewrites the whole file
// from the mesh's landmark list, which the previous save already updated).
async function save_landmarks(): Promise<void> {
  if (skull === null || zygion === null || zygomatic_bottom === null) return;
  const status = document.getElementById("save_status")!;
  status.textContent = await save_landmark(skull, LANDMARKS_URL, "zygion", zygion);
  status.textContent += "; " + await save_landmark(skull, LANDMARKS_URL, "zygomatic_bottom", zygomatic_bottom);
}

// ---- wiring -------------------------------------------------------------------------

const controls = bind_controls(["skull_alpha", "shape_alpha"], redraw);
attach_orbit_controls([pane_canvas], camera, redraw);
attach_pick();
document.getElementById("save_landmarks")!.addEventListener("click", () => { void save_landmarks(); });
window.addEventListener("resize", draw_section);

void load_skull("skull-cheekbones").then((loaded) => {
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
  if (construction === null) { console.error("skull-cheekbones: the ball or the cuts failed"); return; }
  nasal_spine = from_file("nasal_spine");
  if (nasal_spine === null) { document.getElementById("ball_text")!.textContent = "MISSING nasal_spine: pick it on the skull-face-thirds page first"; return; }
  const zygion_in_file = from_file("zygion");
  zygion = zygion_in_file === null ? guess_zygion(skull, nasal_spine.y, construction.brow_up) : zygion_in_file;
  const zygomatic_bottom_in_file = from_file("zygomatic_bottom");
  zygomatic_bottom = zygomatic_bottom_in_file !== null ? zygomatic_bottom_in_file : zygion === null ? null : guess_zygomatic_bottom(skull, zygion, nasal_spine.y);
  const source = (in_file: V3 | null): string => (in_file === null ? "guessed from the mesh, not saved yet" : "from the landmarks file");
  document.getElementById("save_status")!.textContent = `zygion ${source(zygion_in_file)}; zygomatic bottom ${source(zygomatic_bottom_in_file)}`;
  update_numbers_table();
  redraw();
});
