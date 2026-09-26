// Document `skull-mouth-jaw` (plan-skull-construction-docs.md Q23): the two lines below the
// nose that the face-thirds page leaves out. The mouth line (on bone: the teeth line, the
// upper incisor tip) measured as a fraction of nose base to chin, against Finch's "halfway,
// a little higher"; and the jaw angle (the gonion, hand-picked on the mandible, one side
// mirrored) measured against the teeth line, Finch's "the back of the jaw is level with the
// mouth". Same ball, brow line, nasal spine and menton as skull-face-thirds; the upper teeth
// are a third mesh (the skull mesh stops at the alveolar edge).
import "../../pages.css";
import { OrbitCamera, camera_eye, camera_pen_ray, camera_view_projection } from "../../src/camera";
import { V3, v3, v3_scale } from "../../src/math";
import { attach_orbit_controls, bind_controls } from "../../src/explainer/orbit_controls";
import { CanvasView, canvas_view, stroke_polyline } from "../../src/explainer/canvas_view";
import { TranslucentMesh, create_translucent_mesh, draw_mesh_translucent, set_translucent_mesh } from "../../src/render";
import { find_landmark, frankfurt_coordinates } from "../../src/reference";
import { LengthSphereFit, SidePlanesFit, fit_side_planes_mirrored, fit_sphere_through_midline_extremes } from "../../src/construction_fit";
import { MANDIBLE_LANDMARKS_URL, MeshBuilder, Skull, WORLD_PER_MM, cranium_cut_normal, format_signed, load_mandible, load_skull, load_teeth_upper, mm_to_world, pick_skull_vertex, push_landmark_marker, push_side_plane, push_skull, push_sphere_with_side_cuts, save_landmark, side_cut_rim, size_gl_canvas, skull_view_colors as colors, sphere_outline, vault_vertices } from "../../src/reference_skull_view";

const MIDLINE_BAND_MM = 10; // same length ball as skull-side-cuts and skull-brow-line
const plane_color = v3(0.5, 0.75, 1.0);
const landmark_color = v3(0.55, 1.0, 0.6);
const teeth_color = v3(1.0, 0.55, 0.85);
const gonion_color = v3(0.6, 0.8, 1.0);
// Finch: the mouth is "just about halfway" from the nose base to the chin, "a little higher"
// on an idealized face; as a fraction of nose-to-chin measured down from the nose.
const FINCH_MOUTH_FRACTION = 0.5;

type Construction = {
  cut_normal: V3;
  length: LengthSphereFit;
  side_planes: SidePlanesFit;
  brow_up: number; // the brow line's height = the ball center's, mm
  glabella: V3;
};

// Side view by default: the jaw angle is a profile proportion.
const camera: OrbitCamera = { pivot: v3(0, 0.2, 0), yaw: Math.PI / 2, pitch: 0.05, distance: 3.8 };
let skull: Skull | null = null;
let mandible: Skull | null = null;
let teeth_upper: Skull | null = null;
let construction: Construction | null = null;
// Landmarks in frame mm. The nasal spine, the menton and the two incisor tips come from the
// files (picked on skull-face-thirds / in the desktop app); the gonion is picked here, and
// starts from a guess on the mesh when the file has none.
let nasal_spine: V3 | null = null;
let menton: V3 | null = null;
let incisor_upper: V3 | null = null;
let incisor_lower: V3 | null = null;
let gonion: V3 | null = null;

// ---- construction -------------------------------------------------------------------

function compute_construction(skull: Skull, glabella: V3): Construction | null {
  const cut_normal = cranium_cut_normal(glabella);
  const vault = vault_vertices(skull, cut_normal);
  const length = fit_sphere_through_midline_extremes(vault, MIDLINE_BAND_MM);
  const side_planes = fit_side_planes_mirrored(vault);
  if (length === null || side_planes === null) return null;
  return { cut_normal, length, side_planes, brow_up: length.sphere.center.y, glabella };
}

// First guess when the gonion is not in the file yet: the mandible angle is the vertex on
// the right half (side > 20 mm, clear of the chin) that sticks out furthest back-and-down,
// i.e. the corner between the body and the ramus. A click on the mesh refines it.
function guess_gonion(mandible: Skull): V3 | null {
  let best: V3 | null = null;
  for (const p of mandible.positions) {
    if (p.x > 20 && (best === null || -p.z - p.y > -best.z - best.y)) best = p;
  }
  return best;
}

// The gonion is picked on one side; the mandible is treated as symmetric, so its mirror
// stands in for the other side.
function mirrored(p: V3): V3 { return v3(-p.x, p.y, p.z); }

// ---- 3D pane ------------------------------------------------------------------------

const pane_canvas = document.querySelector<HTMLCanvasElement>("canvas.pane")!;
const overlay_canvas = pane_canvas.nextElementSibling as HTMLCanvasElement;
const gl = pane_canvas.getContext("webgl", { premultipliedAlpha: false })!;
const pane_mesh: TranslucentMesh = create_translucent_mesh(gl);

function build_pane_mesh(eye_mm: V3): Float32Array {
  const builder: MeshBuilder = { data: [] };
  if (skull === null || mandible === null || teeth_upper === null || construction === null) return new Float32Array(0);
  const { center, radius } = construction.length.sphere;
  const half_width = construction.side_planes.half_width;
  push_sphere_with_side_cuts(builder, center, radius, half_width, colors.shape, Number(controls.shape_alpha.value), eye_mm);
  push_side_plane(builder, half_width, plane_color, 0.12);
  push_side_plane(builder, -half_width, plane_color, 0.12);
  push_landmark_marker(builder, construction.glabella, colors.landmark);
  if (nasal_spine !== null) push_landmark_marker(builder, nasal_spine, landmark_color);
  if (menton !== null) push_landmark_marker(builder, menton, landmark_color);
  if (incisor_upper !== null) push_landmark_marker(builder, incisor_upper, teeth_color);
  if (gonion !== null) { push_landmark_marker(builder, gonion, gonion_color); push_landmark_marker(builder, mirrored(gonion), gonion_color); }
  const skull_alpha = Number(controls.skull_alpha.value);
  push_skull(builder, skull, null, eye_mm, skull_alpha, 20);
  push_skull(builder, mandible, null, eye_mm, skull_alpha, 20);
  push_skull(builder, teeth_upper, null, eye_mm, skull_alpha, 20);
  return new Float32Array(builder.data);
}

// The brow line as a ring (skull-brow-line), the nose and chin lines as short bars across
// the front of the face, the teeth line as a bar in its own color, and at each jaw angle a
// line forward at the gonion's height (what "level with the mouth" is judged against).
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
    if (p !== null) stroke_polyline(view, [v3(-30, p.y, p.z), v3(30, p.y, p.z)].map(mm_to_world), false, landmark_style, 2);
  }
  if (incisor_upper !== null) stroke_polyline(view, [v3(-30, incisor_upper.y, incisor_upper.z), v3(30, incisor_upper.y, incisor_upper.z)].map(mm_to_world), false, teeth_style, 2);
  if (gonion !== null) {
    for (const g of [gonion, mirrored(gonion)]) stroke_polyline(view, [v3(g.x, g.y, g.z - 15), v3(g.x, g.y, g.z + 40)].map(mm_to_world), false, gonion_style, 1.5);
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
// The skull, mandible and upper teeth cut along the midline (x = 0), seen from the right,
// plus the right half of the mandible as a silhouette so the jaw angle sits on something.

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

// The silhouette of the mesh's right half (side >= 0) seen from the right: the edges whose
// two triangles face opposite ways along the side axis. Projected onto the profile plane.
function side_silhouette_segments(mesh: Skull): SectionSegment[] {
  const first_sign = new Map<string, number>(); // edge key -> the first adjacent triangle's facing
  const silhouette_keys: string[] = [];
  const edge_key = (i: number, j: number) => (i < j ? `${i},${j}` : `${j},${i}`);
  for (let i = 0; i < mesh.triangle_indices.length; i += 3) {
    const ia = mesh.triangle_indices[i], ib = mesh.triangle_indices[i + 1], ic = mesh.triangle_indices[i + 2];
    const a = mesh.positions[ia], b = mesh.positions[ib], c = mesh.positions[ic];
    if (a.x < 0 || b.x < 0 || c.x < 0) continue;
    const normal_x = (b.y - a.y) * (c.z - a.z) - (b.z - a.z) * (c.y - a.y);
    const sign = normal_x >= 0 ? 1 : -1;
    for (const [p, q] of [[ia, ib], [ib, ic], [ic, ia]]) {
      const key = edge_key(p, q);
      const seen = first_sign.get(key);
      if (seen === undefined) first_sign.set(key, sign);
      else if (seen !== sign) silhouette_keys.push(key);
    }
  }
  return silhouette_keys.map((key) => {
    const [p, q] = key.split(",").map(Number);
    const a = mesh.positions[p], b = mesh.positions[q];
    return { a: { across: a.z, up: a.y }, b: { across: b.z, up: b.y } };
  });
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
const ball_style = "#ffd166", landmark_style = "#8cffa0", teeth_style = "#ff8fd9", gonion_style = "#99ccff", finch_style = "#ffb070";
const ACROSS_MIN = -120, ACROSS_MAX = 110;

function draw_profile(): void {
  if (skull === null || mandible === null || teeth_upper === null || construction === null) return;
  const ball = construction.length.sphere;
  const radius = ball.radius;
  const view = section_view(profile_canvas, ACROSS_MIN, ACROSS_MAX, Math.min(-95, construction.brow_up - 1.4 * radius), ball.center.y + radius + 5);
  stroke_segments(view, side_silhouette_segments(mandible), "#3e434e", 1);
  stroke_segments(view, midline_cut_segments(skull), "#8a8f9a", 1);
  stroke_segments(view, midline_cut_segments(mandible), "#8a8f9a", 1);
  stroke_segments(view, midline_cut_segments(teeth_upper), "#8a8f9a", 1);
  stroke_circle(view, { across: ball.center.z, up: ball.center.y }, radius, ball_style, 1.5);
  stroke_line(view, { across: ACROSS_MIN, up: construction.brow_up }, { across: ACROSS_MAX, up: construction.brow_up }, ball_style, 2, []);
  label(view, { across: ACROSS_MIN + 5, up: construction.brow_up }, `brow line up ${construction.brow_up.toFixed(1)}`, ball_style, 0, -6);
  // Finch's mouth line, dashed, halfway from the nose base down to the chin.
  if (nasal_spine !== null && menton !== null) {
    const up = nasal_spine.y - FINCH_MOUTH_FRACTION * (nasal_spine.y - menton.y);
    stroke_line(view, { across: ACROSS_MIN, up }, { across: ACROSS_MAX, up }, finch_style, 1, [6, 4]);
    label(view, { across: ACROSS_MIN + 5, up }, `Finch mouth (halfway) ${format_signed(up - construction.brow_up, 1)}`, finch_style, 0, 14);
  }
  // The skull's own lines, solid, labeled at the face; the gonion labeled at the back.
  const rows: { name: string; p: V3 | null; style: string }[] = [
    { name: "nasal spine", p: nasal_spine, style: landmark_style },
    { name: "teeth line (upper incisor tip)", p: incisor_upper, style: teeth_style },
    { name: "menton", p: menton, style: landmark_style },
  ];
  for (const { name, p, style } of rows) {
    if (p === null) continue;
    const q = { across: p.z, up: p.y };
    fill_dot(view, q, style);
    stroke_line(view, { across: q.across - 40, up: q.up }, { across: ACROSS_MAX, up: q.up }, style, 1.2, []);
    label(view, q, `${name} ${format_signed(q.up - construction.brow_up, 1)}`, style, -8, -6);
  }
  if (gonion !== null) {
    const q = { across: gonion.z, up: gonion.y };
    fill_dot(view, q, gonion_style);
    stroke_line(view, q, { across: ACROSS_MAX, up: q.up }, gonion_style, 1.2, []);
    label(view, q, `gonion ${format_signed(q.up - construction.brow_up, 1)}`, gonion_style, -8, 14);
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
  if (nasal_spine === null || menton === null || incisor_upper === null) {
    for (const id of ["row-nose-chin", "row-nose-teeth", "row-teeth-chin"]) fill_row(id, ["missing", "", ""]);
  } else {
    const nose_to_chin = nasal_spine.y - menton.y;
    const nose_to_teeth = nasal_spine.y - incisor_upper.y;
    fill_row("row-nose-chin", [`${nose_to_chin.toFixed(1)} mm`, "1.00", "1.00"]);
    fill_row("row-nose-teeth", [`${nose_to_teeth.toFixed(1)} mm`, (nose_to_teeth / nose_to_chin).toFixed(2), `${FINCH_MOUTH_FRACTION.toFixed(2)}, "a little" less`]);
    fill_row("row-teeth-chin", [`${(nose_to_chin - nose_to_teeth).toFixed(1)} mm`, (1 - nose_to_teeth / nose_to_chin).toFixed(2), `${(1 - FINCH_MOUTH_FRACTION).toFixed(2)}, "a little" more`]);
  }
  if (gonion === null || incisor_upper === null || nasal_spine === null || menton === null) {
    for (const id of ["row-gonion-teeth", "row-gonion-front"]) fill_row(id, ["pick it", "", ""]);
  } else {
    const nose_to_chin = nasal_spine.y - menton.y;
    const above_teeth = gonion.y - incisor_upper.y;
    fill_row("row-gonion-teeth", [`${format_signed(above_teeth, 1)} mm`, format_signed(above_teeth / nose_to_chin, 2), "0 (level)"]);
    // The frame's origin is the ear hole (the porion middle), so front = mm in front of it.
    fill_row("row-gonion-front", [`${format_signed(gonion.z, 1)} mm`, `${format_signed(gonion.z / construction.length.sphere.radius, 2)} r`, "no number"]);
  }
  const center = construction.length.sphere.center;
  document.getElementById("ball_text")!.textContent = `center up ${center.y.toFixed(1)} / front ${center.z.toFixed(1)}, r ${construction.length.sphere.radius.toFixed(1)}`;
  const point_text = (p: V3 | null) => (p === null ? "missing" : `side ${format_signed(p.x, 1)}, up ${format_signed(p.y, 1)}, front ${format_signed(p.z, 1)}`);
  document.getElementById("incisor_upper_text")!.textContent = point_text(incisor_upper);
  const closed_mouth = incisor_upper === null || incisor_lower === null ? "" : ` (${format_signed(incisor_lower.y - incisor_upper.y, 1)} mm vs the upper tip, so the mouth is closed)`;
  document.getElementById("incisor_lower_text")!.textContent = point_text(incisor_lower) + closed_mouth;
  document.getElementById("gonion_text")!.textContent = point_text(gonion);
}

// ---- picking + saving ---------------------------------------------------------------

const pick_mode = document.getElementById("pick_mode") as HTMLInputElement;

function attach_pick(): void {
  let down: { x: number; y: number } | null = null;
  pane_canvas.addEventListener("pointerdown", (event) => { down = { x: event.clientX, y: event.clientY }; });
  pane_canvas.addEventListener("pointerup", (event) => {
    if (down === null || mandible === null || !pick_mode.checked) return;
    const moved = Math.hypot(event.clientX - down.x, event.clientY - down.y);
    down = null;
    if (moved > 4) return;
    const rect = pane_canvas.getBoundingClientRect();
    const ray = camera_pen_ray(camera, { x: event.clientX - rect.left, y: event.clientY - rect.top }, rect.width, rect.height);
    const picked = pick_skull_vertex(mandible, v3_scale(ray.origin, 1 / WORLD_PER_MM), ray.direction);
    if (picked === null) return;
    // Stored on the right side (side >= 0) whichever side was clicked.
    gonion = picked.x >= 0 ? picked : mirrored(picked);
    update_numbers_table();
    redraw();
  });
}

async function save_landmarks(): Promise<void> {
  if (mandible === null || gonion === null) return;
  document.getElementById("save_status")!.textContent = await save_landmark(mandible, MANDIBLE_LANDMARKS_URL, "gonion", gonion);
}

// ---- wiring -------------------------------------------------------------------------

const controls = bind_controls(["skull_alpha", "shape_alpha"], redraw);
attach_orbit_controls([pane_canvas], camera, redraw);
attach_pick();
document.getElementById("save_landmarks")!.addEventListener("click", () => { void save_landmarks(); });
window.addEventListener("resize", draw_profile);

void load_skull("skull-mouth-jaw").then(async (loaded) => {
  if (loaded === null) return;
  skull = loaded;
  [mandible, teeth_upper] = await Promise.all([load_mandible("skull-mouth-jaw", skull.frame), load_teeth_upper("skull-mouth-jaw", skull.frame)]);
  if (mandible === null || teeth_upper === null) { document.getElementById("ball_text")!.textContent = "MISSING mandible or upper teeth mesh"; return; }
  const from_file = (mesh: Skull, name: string): V3 | null => {
    const landmark = find_landmark(mesh.landmarks, name);
    return landmark === null ? null : frankfurt_coordinates(mesh.frame, landmark);
  };
  const glabella = from_file(skull, "glabella");
  if (glabella === null) {
    document.getElementById("ball_text")!.textContent = "MISSING glabella: pick it on the skull-ball page first";
    return;
  }
  construction = compute_construction(skull, glabella);
  if (construction === null) { console.error("skull-mouth-jaw: the ball or the cuts failed"); return; }
  nasal_spine = from_file(skull, "nasal_spine");
  menton = from_file(mandible, "menton");
  incisor_upper = from_file(teeth_upper, "incisor_upper");
  incisor_lower = from_file(mandible, "incisor_lower");
  if (nasal_spine === null || menton === null) document.getElementById("ball_text")!.textContent = "MISSING nasal_spine or menton: pick them on the skull-face-thirds page first";
  const gonion_in_file = from_file(mandible, "gonion");
  gonion = gonion_in_file === null ? guess_gonion(mandible) : gonion_in_file;
  document.getElementById("save_status")!.textContent = gonion_in_file === null ? "gonion guessed from the mesh, not saved yet" : "gonion from the landmarks file";
  update_numbers_table();
  redraw();
});
