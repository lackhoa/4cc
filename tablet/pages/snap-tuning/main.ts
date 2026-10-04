// Page `snap-tuning`: one slider per snap distance of the drawing view (src/snap_tuning.ts),
// stored in localStorage on every change, plus a to-scale picture of what each distance
// covers. The drawing tabs of the same browser pick the values up through the `storage` event.
import "../../pages.css";
import { SNAP_TUNING_FIELDS, SnapTuning, SnapTuningUnit, default_snap_tuning, snap_tuning, store_snap_tuning } from "../../src/snap_tuning";

const WORLD_PER_MM = 0.01; // the skull pages' scale (src/reference_skull_view.ts)
const DEFAULT_LINE_HALF_WIDTH_WORLD = 0.0025; // ribbon radius 0.01 * default width 0.25
const VERTEX_MARKER_SIZE_PIXELS = 12; // the drawing view's anchor square

const values: SnapTuning = { ...snap_tuning() };
const defaults = default_snap_tuning();

function format_value(value: number, unit: SnapTuningUnit): string {
  if (unit === "pixels") return `${value} px`;
  return `${value.toFixed(3)} world = ${(value / WORLD_PER_MM).toFixed(1)} mm`;
}

const fields_element = document.getElementById("fields")!;
const refresh_field_rows: (() => void)[] = [];
for (const field of SNAP_TUNING_FIELDS) {
  const row = document.createElement("div");
  row.className = "field";
  const name = document.createElement("span");
  name.className = "name";
  name.textContent = field.label;
  const slider = document.createElement("input");
  slider.type = "range";
  slider.min = String(field.min);
  slider.max = String(field.max);
  slider.step = String(field.step);
  const value_text = document.createElement("span");
  value_text.className = "value";
  const reset_button = document.createElement("button");
  reset_button.textContent = "default";
  reset_button.title = `back to ${format_value(field.default_value, field.unit)}`;
  const description = document.createElement("p");
  description.className = "description";
  description.textContent = field.description;
  row.append(name, slider, value_text, reset_button, description);
  fields_element.append(row);

  refresh_field_rows.push(() => {
    slider.value = String(values[field.key]);
    value_text.textContent = format_value(values[field.key], field.unit);
    value_text.classList.toggle("changed", values[field.key] !== field.default_value);
  });
  slider.addEventListener("input", () => set_value(field.key, Number(slider.value)));
  reset_button.addEventListener("click", () => set_value(field.key, field.default_value));
}

function refresh_page(): void {
  for (const refresh of refresh_field_rows) refresh();
  draw_scale_picture();
}

function set_value(key: keyof SnapTuning, value: number): void {
  values[key] = value;
  store_snap_tuning(values);
  refresh_page();
}

// The drawing view under the sliders: a real sketchpad page in a frame (same browser
// storage, so it follows the sliders through the `storage` event like any other tab).
const drawing_frame = document.getElementById("drawing_frame") as HTMLIFrameElement;
document.querySelectorAll<HTMLButtonElement>("#drawing_pages button").forEach((button) => {
  button.addEventListener("click", () => { drawing_frame.src = button.dataset.page!; });
});

document.getElementById("reset_all_button")!.addEventListener("click", () => {
  Object.assign(values, defaults);
  store_snap_tuning(values);
  refresh_page();
});

// Another tab of this page changed the values: follow it.
window.addEventListener("storage", () => {
  Object.assign(values, snap_tuning());
  refresh_page();
});

const canvas = document.getElementById("scale_canvas") as HTMLCanvasElement;
const context = canvas.getContext("2d")!;
const color = (name: string) => getComputedStyle(document.documentElement).getPropertyValue(name).trim();

function draw_scale_picture(): void {
  const pixel_ratio = window.devicePixelRatio || 1;
  const width = canvas.clientWidth;
  const height = canvas.clientHeight;
  canvas.width = Math.round(width * pixel_ratio);
  canvas.height = Math.round(height * pixel_ratio);
  context.setTransform(pixel_ratio, 0, 0, pixel_ratio, 0, 0);
  context.clearRect(0, 0, width, height);
  context.font = "13px system-ui, sans-serif";
  context.textAlign = "center";

  // Left: world distances, magnified so the largest slider value (0.1 world) fits.
  const world_area_width = width * 0.62;
  const pixels_per_world = Math.min((height / 2 - 26) / 0.1, world_area_width / 0.44);
  const center_y = height / 2;
  const line_start_x = 16;
  const line_end_x = world_area_width - 16;
  // The pin band around the line.
  const pin_radius = values.vertex_pin_radius_world * pixels_per_world;
  context.fillStyle = color("--teal");
  context.globalAlpha = 0.22;
  context.fillRect(line_start_x, center_y - pin_radius, line_end_x - line_start_x, pin_radius * 2);
  context.globalAlpha = 1;
  // The line itself, at its default width.
  const line_half_width = DEFAULT_LINE_HALF_WIDTH_WORLD * pixels_per_world;
  context.fillStyle = color("--ink");
  context.fillRect(line_start_x, center_y - line_half_width, line_end_x - line_start_x, line_half_width * 2);
  // The weld circle around a vertex.
  const vertex_x = world_area_width * 0.5;
  const weld_radius = values.vertex_weld_radius_world * pixels_per_world;
  context.beginPath();
  context.arc(vertex_x, center_y, weld_radius, 0, Math.PI * 2);
  context.fillStyle = color("--orange");
  context.globalAlpha = 0.25;
  context.fill();
  context.globalAlpha = 1;
  context.strokeStyle = color("--orange");
  context.lineWidth = 1.5;
  context.stroke();
  context.fillStyle = color("--ink");
  context.beginPath();
  context.arc(vertex_x, center_y, line_half_width * 1.6, 0, Math.PI * 2);
  context.fill();
  // Scale bar: 0.05 world.
  const bar_length = 0.05 * pixels_per_world;
  const bar_y = height - 14;
  context.strokeStyle = color("--muted");
  context.lineWidth = 1;
  context.beginPath();
  context.moveTo(line_start_x, bar_y);
  context.lineTo(line_start_x + bar_length, bar_y);
  context.moveTo(line_start_x, bar_y - 4);
  context.lineTo(line_start_x, bar_y + 4);
  context.moveTo(line_start_x + bar_length, bar_y - 4);
  context.lineTo(line_start_x + bar_length, bar_y + 4);
  context.stroke();
  context.fillStyle = color("--muted");
  context.textAlign = "left";
  context.fillText("0.05 world", line_start_x + bar_length + 8, bar_y + 4);
  context.fillStyle = color("--teal");
  context.fillText("pin", line_start_x, 16);
  context.textAlign = "center";
  context.fillStyle = color("--orange");
  context.fillText("weld", vertex_x, 16);

  // Right: the pixel distance at real size, around a vertex marker.
  const pixel_circles: [string, number][] = [
    ["tap or drag", values.point_drag_start_pixels],
  ];
  const pixel_cell_width = (width - world_area_width) / pixel_circles.length;
  pixel_circles.forEach(([label, radius], index) => {
    const x = world_area_width + pixel_cell_width * (index + 0.5);
    context.fillStyle = color("--ink");
    context.fillRect(x - VERTEX_MARKER_SIZE_PIXELS / 2, center_y - VERTEX_MARKER_SIZE_PIXELS / 2, VERTEX_MARKER_SIZE_PIXELS, VERTEX_MARKER_SIZE_PIXELS);
    context.beginPath();
    context.arc(x, center_y, radius, 0, Math.PI * 2);
    context.fillStyle = color("--accent");
    context.globalAlpha = 0.35;
    context.fill();
    context.globalAlpha = 1;
    context.strokeStyle = color("--accent");
    context.lineWidth = 1.5;
    context.stroke();
    context.fillStyle = color("--accent");
    context.fillText(label, x, 16);
  });
}

window.addEventListener("resize", draw_scale_picture);
refresh_page();
