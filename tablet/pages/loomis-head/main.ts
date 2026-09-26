import "../../pages.css";
import { OrbitCamera, camera_eye } from "../../src/camera";
import { v3, v3_add, v3_cross, v3_dot, v3_normalize, v3_scale, v3_sub, V3 } from "../../src/math";
import { attach_orbit_controls, bind_controls } from "../../src/explainer/orbit_controls";
import { CanvasView, canvas_view, stroke_polyline } from "../../src/explainer/canvas_view";
import { LOOMIS_PLATE_1, LOOMIS_PLATE_18, LoomisParams, LoomisPolyline, LoomisSurface, build_loomis_head } from "../../src/loomis_head";

const params: LoomisParams = { ...LOOMIS_PLATE_1 };
const camera: OrbitCamera = { pivot: v3(0, -0.3, 0), yaw: 0.6, pitch: 0.15, distance: 3.5 };

const main_canvas = document.getElementById("main_canvas") as HTMLCanvasElement;
const thumbnails = Array.from(document.querySelectorAll<HTMLCanvasElement>("canvas.thumbnail"));
const step_items = Array.from(document.querySelectorAll<HTMLLIElement>("#steps li"));
const step_name = document.getElementById("step_name")!;

const styles = {
  current: "#ffd166",
  done: "#9aa0b4",
  faded: "#3a3f52",
  ball: "#7ab8ff",
};

// A point on a surface faces the eye when the surface normal there points toward the eye.
function faces_eye(point: V3, surface: LoomisSurface, eye: V3): boolean {
  const normal = surface.kind === "ball" ? point : (surface as { normal: V3 }).normal;
  return v3_dot(normal, v3_sub(eye, point)) > 0;
}

// Splits a polyline on the ball into runs that face the eye and runs that don't, so the
// far side can be drawn faded. Off-ball lines (jaw, chin) are drawn whole.
function draw_polyline(view: CanvasView, polyline: LoomisPolyline, style: string, width: number): void {
  if (polyline.surface.kind === "none") {
    stroke_polyline(view, polyline.points, polyline.closed, style, width);
    return;
  }
  const points = polyline.closed ? [...polyline.points, polyline.points[0]] : polyline.points;
  let run: V3[] = [];
  let run_visible = faces_eye(points[0], polyline.surface, view.eye);
  const flush = () => {
    if (run.length > 1) stroke_polyline(view, run, false, run_visible ? style : styles.faded, run_visible ? width : 1);
  };
  for (const point of points) {
    const visible = faces_eye(point, polyline.surface, view.eye);
    if (visible !== run_visible) {
      run.push(point);
      flush();
      run = [point];
      run_visible = visible;
    } else {
      run.push(point);
    }
  }
  flush();
}

// The ball's silhouette: for a perspective camera it is the circle where the eye's
// tangent cone touches the sphere.
function ball_outline(eye: V3): V3[] {
  const distance = Math.hypot(eye.x, eye.y, eye.z);
  const toward_eye = v3_normalize(eye);
  const ring_center_distance = 1 / distance;
  const ring_radius = Math.sqrt(1 - ring_center_distance * ring_center_distance);
  const helper = Math.abs(toward_eye.y) < 0.9 ? v3(0, 1, 0) : v3(1, 0, 0);
  const u = v3_normalize(v3_sub(helper, v3_scale(toward_eye, v3_dot(helper, toward_eye))));
  const w = v3_cross(toward_eye, u);
  const ring_center = v3_scale(toward_eye, ring_center_distance);
  const points: V3[] = [];
  for (let i = 0; i < 72; i++) {
    const angle = (i / 72) * 2 * Math.PI;
    points.push(v3_add(ring_center, v3_add(v3_scale(u, ring_radius * Math.cos(angle)), v3_scale(w, ring_radius * Math.sin(angle)))));
  }
  return points;
}

function draw_head(canvas: HTMLCanvasElement, view_camera: OrbitCamera, step: number, line_width: number): void {
  const view = canvas_view(canvas, view_camera);
  const steps = build_loomis_head(params);
  stroke_polyline(view, ball_outline(camera_eye(view_camera)), true, step === 1 ? styles.current : styles.ball, line_width);
  for (let i = 1; i < step; i++) {
    const is_current = i === step - 1;
    for (const polyline of steps[i].polylines) {
      draw_polyline(view, polyline, is_current ? styles.current : styles.done, is_current ? line_width * 1.4 : line_width);
    }
  }
}

function redraw(): void {
  const step = Number(controls.step.value);
  step_name.textContent = build_loomis_head(params)[step - 1].name;
  for (let i = 0; i < step_items.length; i++) {
    step_items[i].classList.toggle("current", i === step - 1);
    step_items[i].classList.toggle("pending", i >= step);
  }
  draw_head(main_canvas, camera, step, 1.6);
  for (const thumbnail of thumbnails) {
    draw_head(thumbnail, { pivot: camera.pivot, yaw: Number(thumbnail.dataset.yaw), pitch: 0, distance: 3.5 }, step, 1.2);
  }
}

const parameter_ids: (keyof LoomisParams)[] = ["slice_depth", "thirds_unit", "jaw_width", "ear_back", "chin_forward"];

function read_params_from_sliders(): void {
  for (const id of parameter_ids) {
    params[id] = Number(controls[id].value);
    document.getElementById(`${id}_value`)!.textContent = params[id].toFixed(2);
  }
}

function write_params_to_sliders(): void {
  for (const id of parameter_ids) controls[id].value = String(params[id]);
  read_params_from_sliders();
}

const controls = bind_controls(["step", ...parameter_ids], () => {
  read_params_from_sliders();
  redraw();
});
document.getElementById("preset_plate_1")!.addEventListener("click", () => { Object.assign(params, LOOMIS_PLATE_1); write_params_to_sliders(); redraw(); });
document.getElementById("preset_plate_18")!.addEventListener("click", () => { Object.assign(params, LOOMIS_PLATE_18); write_params_to_sliders(); redraw(); });

attach_orbit_controls([main_canvas], camera, redraw);
write_params_to_sliders();
redraw();
