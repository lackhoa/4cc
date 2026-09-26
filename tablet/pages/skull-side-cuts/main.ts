// Document `skull-side-cuts` (plan-skull-construction-docs.md Q15): the cranium sphere of
// skull-ball with Loomis's two side slices fitted to the skull: mirrored planes side =
// ±half_width through the flats (vertices well inside the sphere). Two panes (sphere alone,
// sphere with cuts) with heat maps, a front-view section through the sphere center, numbers.
import "../../pages.css";
import { OrbitCamera, camera_eye, camera_view_projection } from "../../src/camera";
import { V3, v3, v3_dot, v3_scale } from "../../src/math";
import { attach_orbit_controls, bind_controls } from "../../src/explainer/orbit_controls";
import { CanvasView, canvas_view, stroke_polyline } from "../../src/explainer/canvas_view";
import { TranslucentMesh, create_translucent_mesh, draw_mesh_translucent, set_translucent_mesh } from "../../src/render";
import { find_landmark, frankfurt_coordinates } from "../../src/reference";
import { ResidualStats, SidePlanesFit, SphereFit, fit_side_planes_mirrored, fit_sphere_algebraic, residual_stats, residuals_sphere_with_side_cuts, sphere_residual } from "../../src/construction_fit";
import { MeshBuilder, Skull, WORLD_PER_MM, cranium_cut_normal, format_signed, load_skull, mm_to_world, push_ellipsoid, push_side_plane, push_skull, push_sphere_with_side_cuts, side_cut_rim, size_gl_canvas, skull_view_colors as colors, sphere_outline, vault_vertices } from "../../src/reference_skull_view";

const flats_color = v3(0.5, 0.75, 1.0);

type CandidateId = "sphere" | "side-cuts";

// Everything fitted, recomputed whenever the threshold changes.
type Fits = {
  cut_normal: V3;
  vault_count: number;
  sphere: SphereFit;
  sphere_residuals: number[]; // per skull vertex, mm, + = outside
  sphere_stats: ResidualStats; // over the vault
  side_planes: SidePlanesFit | null;
  is_flat: boolean[]; // per skull vertex: in the vault and inside the sphere by more than the threshold
  side_cuts_residuals: number[]; // per skull vertex, to the clipped sphere
  side_cuts_stats: ResidualStats; // over the vault
};

type Pane = {
  gl: WebGLRenderingContext;
  canvas: HTMLCanvasElement;
  overlay: HTMLCanvasElement;
  mesh: TranslucentMesh;
  candidate_id: CandidateId;
};

const camera: OrbitCamera = { pivot: v3(0, 0.45, 0), yaw: 0.6, pitch: 0.15, distance: 3.2 };
let skull: Skull | null = null;
let glabella: V3 | null = null; // frame mm, from the landmarks file
let fits: Fits | null = null;
const panes: Pane[] = [];

// ---- fits ---------------------------------------------------------------------------

function compute_fits(skull: Skull, glabella: V3, flat_threshold_mm: number | null): Fits | null {
  const cut_normal = cranium_cut_normal(glabella);
  const vault = vault_vertices(skull, cut_normal);
  const sphere = fit_sphere_algebraic(vault);
  if (sphere === null) return null;
  const sphere_residuals = skull.positions.map((p) => sphere_residual(sphere, p));
  const sphere_stats = residual_stats(vault.map((p) => sphere_residual(sphere, p)));
  const threshold = flat_threshold_mm ?? sphere_stats.rms;
  const side_planes = fit_side_planes_mirrored(vault, sphere, threshold);
  const is_flat = skull.positions.map((p, index) => v3_dot(p, cut_normal) > 0 && sphere_residuals[index] < -threshold);
  const half_width = side_planes?.half_width ?? Infinity;
  const side_cuts_residuals = skull.positions.map((p) => residuals_sphere_with_side_cuts(sphere, half_width, p));
  const side_cuts_stats = residual_stats(skull.positions.filter((p) => v3_dot(p, cut_normal) > 0).map((p) => residuals_sphere_with_side_cuts(sphere, half_width, p)));
  return { cut_normal, vault_count: vault.length, sphere, sphere_residuals, sphere_stats, side_planes, is_flat, side_cuts_residuals, side_cuts_stats };
}

// ---- panes --------------------------------------------------------------------------

function build_pane_mesh(pane: Pane, eye_mm: V3): Float32Array {
  const builder: MeshBuilder = { data: [] };
  if (skull === null || fits === null) return new Float32Array(0);
  const skull_alpha = Number(controls.skull_alpha.value);
  const shape_alpha = Number(controls.shape_alpha.value);
  const heat_range = Number(controls.heat_range.value);
  const with_cuts = pane.candidate_id === "side-cuts" && fits.side_planes !== null;
  const r = fits.sphere.radius;
  if (with_cuts) {
    const half_width = fits.side_planes!.half_width;
    push_sphere_with_side_cuts(builder, fits.sphere.center, r, half_width, colors.shape, shape_alpha, eye_mm);
    push_side_plane(builder, half_width, flats_color, 0.15);
    push_side_plane(builder, -half_width, flats_color, 0.15);
  } else {
    push_ellipsoid(builder, fits.sphere.center, v3(r, r, r), colors.shape, shape_alpha, eye_mm);
  }
  const residuals = with_cuts ? fits.side_cuts_residuals : fits.sphere_residuals;
  if (color_mode.value === "flats") {
    // The flats in their own color on a bone skull: the fit's input, so it can be judged.
    const flats = fits.is_flat;
    const skull_start = builder.data.length; // push_skull appends one 7-float vertex per triangle index
    push_skull(builder, skull, null, eye_mm, skull_alpha, heat_range);
    for (let i = 0; i < skull.triangle_indices.length; i++) {
      if (flats[skull.triangle_indices[i]]) {
        const base = skull_start + 7 * i;
        builder.data[base + 3] = flats_color.x; builder.data[base + 4] = flats_color.y; builder.data[base + 5] = flats_color.z;
      }
    }
  } else {
    push_skull(builder, skull, color_mode.value === "heat" ? residuals : null, eye_mm, skull_alpha, heat_range);
  }
  return new Float32Array(builder.data);
}

function draw_overlay(pane: Pane, view: CanvasView, eye_mm: V3): void {
  if (fits === null) return;
  const outline = sphere_outline(fits.sphere.center, fits.sphere.radius, eye_mm);
  if (outline !== null) stroke_polyline(view, outline.map(mm_to_world), true, "#ffd166", 1.4);
  if (pane.candidate_id === "side-cuts" && fits.side_planes !== null) {
    for (const x of [fits.side_planes.half_width, -fits.side_planes.half_width]) {
      const rim = side_cut_rim(fits.sphere.center, fits.sphere.radius, x);
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
  draw_section();
}

// ---- front-view section through the sphere center -----------------------------------
// The skull cut by the coronal plane front = sphere center front, as 2D segments (side,
// up), where a side slice shows as a straight vertical line.

type SectionPoint = { side: number; up: number }; // mm
type SectionSegment = { a: SectionPoint; b: SectionPoint };

function coronal_cut_segments(skull: Skull, front: number): SectionSegment[] {
  const segments: SectionSegment[] = [];
  const crossing = (p: V3, q: V3): SectionPoint | null => {
    if ((p.z >= front) === (q.z >= front)) return null;
    const t = (p.z - front) / (p.z - q.z);
    return { side: p.x + (q.x - p.x) * t, up: p.y + (q.y - p.y) * t };
  };
  for (let i = 0; i < skull.triangle_indices.length; i += 3) {
    const a = skull.positions[skull.triangle_indices[i]], b = skull.positions[skull.triangle_indices[i + 1]], c = skull.positions[skull.triangle_indices[i + 2]];
    const points = [crossing(a, b), crossing(b, c), crossing(c, a)].filter((p): p is SectionPoint => p !== null);
    if (points.length === 2) segments.push({ a: points[0], b: points[1] });
  }
  return segments;
}

const section_canvas = document.getElementById("section_canvas") as HTMLCanvasElement;
const SECTION_SIDE_MIN = -100, SECTION_SIDE_MAX = 100, SECTION_UP_MIN = -40, SECTION_UP_MAX = 130; // mm shown

function draw_section(): void {
  const { width: css_width, height: css_height } = size_gl_canvas(section_canvas);
  const dpr = window.devicePixelRatio || 1;
  const ctx = section_canvas.getContext("2d")!;
  const width = css_width * dpr, height = css_height * dpr;
  ctx.fillStyle = "#14161c";
  ctx.fillRect(0, 0, width, height);
  if (skull === null || fits === null) return;
  const pad = 10 * dpr;
  const scale = Math.min((width - 2 * pad) / (SECTION_SIDE_MAX - SECTION_SIDE_MIN), (height - 2 * pad) / (SECTION_UP_MAX - SECTION_UP_MIN));
  const origin_x = width / 2, origin_y = height - pad + SECTION_UP_MIN * scale;
  const px = (p: SectionPoint) => ({ x: origin_x + p.side * scale, y: origin_y - p.up * scale });
  const center = fits.sphere.center;
  // The cut in grey.
  ctx.strokeStyle = "#8a8f9a"; ctx.lineWidth = 1 * dpr;
  ctx.beginPath();
  for (const { a, b } of coronal_cut_segments(skull, center.z)) { const pa = px(a), pb = px(b); ctx.moveTo(pa.x, pa.y); ctx.lineTo(pb.x, pb.y); }
  ctx.stroke();
  // The cranium cut's trace at this depth: a horizontal line at the height where the plane
  // through the porions and the glabella passes front = center.z.
  const n = fits.cut_normal;
  if (Math.abs(n.y) > 1e-6) {
    const up = -(n.z * center.z) / n.y;
    const a = px({ side: SECTION_SIDE_MIN, up }), b = px({ side: SECTION_SIDE_MAX, up });
    ctx.strokeStyle = "rgba(255, 168, 77, 0.7)";
    ctx.beginPath(); ctx.moveTo(a.x, a.y); ctx.lineTo(b.x, b.y); ctx.stroke();
  }
  // The flats, as dots, near this section only (within 6 mm front-to-back).
  ctx.fillStyle = "#7fb3ff";
  skull.positions.forEach((p, index) => {
    if (!fits!.is_flat[index] || Math.abs(p.z - center.z) > 6) return;
    const q = px({ side: p.x, up: p.y });
    ctx.fillRect(q.x - 1.5 * dpr, q.y - 1.5 * dpr, 3 * dpr, 3 * dpr);
  });
  // The sphere's great circle and the two planes.
  const c = px({ side: center.x, up: center.y });
  ctx.strokeStyle = "#ffd166"; ctx.lineWidth = 1.5 * dpr;
  ctx.beginPath(); ctx.arc(c.x, c.y, fits.sphere.radius * scale, 0, 2 * Math.PI); ctx.stroke();
  if (fits.side_planes !== null) {
    ctx.strokeStyle = "#7fb3ff"; ctx.lineWidth = 1.5 * dpr;
    for (const x of [fits.side_planes.half_width, -fits.side_planes.half_width]) {
      const a = px({ side: x, up: SECTION_UP_MIN }), b = px({ side: x, up: SECTION_UP_MAX });
      ctx.beginPath(); ctx.moveTo(a.x, a.y); ctx.lineTo(b.x, b.y); ctx.stroke();
    }
    ctx.fillStyle = "#7fb3ff"; ctx.font = `${12 * dpr}px system-ui, sans-serif`;
    const label = px({ side: fits.side_planes.half_width, up: SECTION_UP_MAX - 6 });
    ctx.fillText(`half_width ${fits.side_planes.half_width.toFixed(1)}`, label.x + 6 * dpr, label.y);
  }
  ctx.fillStyle = "#ffd166"; ctx.font = `${12 * dpr}px system-ui, sans-serif`;
  ctx.fillText(`sphere r ${fits.sphere.radius.toFixed(1)}, section at front ${center.z.toFixed(1)}`, pad + 4 * dpr, pad + 14 * dpr);
  ctx.fillStyle = "#8a8f9a";
  const origin = px({ side: 0, up: 0 });
  ctx.fillText("porion middle", origin.x + 4 * dpr, origin.y - 4 * dpr);
}

// ---- numbers table ------------------------------------------------------------------

function fill_row(id: string, cells: string[]): void {
  const row = document.getElementById(id)!;
  const label = row.firstElementChild!.outerHTML;
  row.innerHTML = label + cells.map((cell) => `<td>${cell}</td>`).join("");
}

function update_numbers_table(): void {
  if (fits === null) return;
  const center = fits.sphere.center;
  fill_row("row-sphere", [
    String(fits.vault_count), format_signed(center.y, 1), format_signed(center.z, 1), `r ${fits.sphere.radius.toFixed(1)}`,
    fits.sphere_stats.rms.toFixed(1), format_signed(fits.sphere_stats.min, 1), format_signed(fits.sphere_stats.max, 1),
  ]);
  if (fits.side_planes === null) {
    fill_row("row-side-cuts", ["0", "", "", "no flats at this threshold", "", "", ""]);
  } else {
    const half_width = fits.side_planes.half_width;
    fill_row("row-side-cuts", [
      String(fits.side_planes.flat_count), format_signed(center.y, 1), format_signed(center.z, 1),
      `half_width ${half_width.toFixed(1)} = ${(half_width / fits.sphere.radius).toFixed(2)} r`,
      fits.side_cuts_stats.rms.toFixed(1), format_signed(fits.side_cuts_stats.min, 1), format_signed(fits.side_cuts_stats.max, 1),
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
const flat_threshold_input = document.getElementById("flat_threshold") as HTMLInputElement;
color_mode.addEventListener("change", redraw);
document.getElementById("heat_range_value")!.textContent = controls.heat_range.value;

function refit(flat_threshold_mm: number | null): void {
  if (skull === null || glabella === null) return;
  fits = compute_fits(skull, glabella, flat_threshold_mm);
  if (fits !== null && flat_threshold_mm === null) flat_threshold_input.value = fits.sphere_stats.rms.toFixed(1);
  update_numbers_table();
  redraw();
}
flat_threshold_input.addEventListener("change", () => refit(Number(flat_threshold_input.value)));
document.getElementById("reset_threshold")!.addEventListener("click", () => refit(null));

function make_pane(canvas: HTMLCanvasElement, overlay: HTMLCanvasElement, candidate_id: CandidateId): Pane {
  const gl = canvas.getContext("webgl", { premultipliedAlpha: false })!;
  return { gl, canvas, overlay, mesh: create_translucent_mesh(gl), candidate_id };
}
for (const canvas of Array.from(document.querySelectorAll<HTMLCanvasElement>("canvas.pane"))) {
  const overlay = canvas.nextElementSibling as HTMLCanvasElement;
  panes.push(make_pane(canvas, overlay, canvas.dataset.candidate as CandidateId));
}
attach_orbit_controls(panes.map((pane) => pane.canvas), camera, redraw);
window.addEventListener("resize", draw_section);

void load_skull("skull-side-cuts").then((loaded) => {
  if (loaded === null) return;
  skull = loaded;
  const glabella_in_file = find_landmark(skull.landmarks, "glabella");
  if (glabella_in_file === null) {
    document.getElementById("glabella_text")!.textContent = "MISSING: pick it on the skull-ball page first";
    return;
  }
  glabella = frankfurt_coordinates(skull.frame, glabella_in_file);
  refit(null);
});
