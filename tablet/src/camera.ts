// Orbit camera: eye position derived from (pivot, yaw, pitch, distance).
// yaw 0 / pitch 0 looks down -z toward the pivot; pitch > 0 looks from above.

import { Mat4, V3, mat4_look_at, mat4_multiply, mat4_perspective, v3, v3_add, v3_cross, v3_normalize, v3_scale, v3_sub } from "./math";

export type OrbitCamera = {
  pivot: V3;
  yaw: number; // radians, around world y
  pitch: number; // radians, clamped to avoid pole flip
  distance: number;
};

export const PITCH_LIMIT = Math.PI / 2 - 0.01;
const FOV_Y = Math.PI / 4;
const NEAR = 0.05;
const FAR = 100;

export function default_camera(): OrbitCamera {
  return { pivot: v3(0, 0, 0), yaw: 0, pitch: 0.5, distance: 8 };
}

export function camera_eye(camera: OrbitCamera): V3 {
  const cp = Math.cos(camera.pitch);
  const offset = v3(
    Math.sin(camera.yaw) * cp,
    Math.sin(camera.pitch),
    Math.cos(camera.yaw) * cp,
  );
  return v3_add(camera.pivot, v3_scale(offset, camera.distance));
}

export function camera_orbit(camera: OrbitCamera, delta_yaw: number, delta_pitch: number): void {
  camera.yaw += delta_yaw;
  camera.pitch = Math.max(-PITCH_LIMIT, Math.min(PITCH_LIMIT, camera.pitch + delta_pitch));
}

// Move the pivot in the camera's screen plane; deltas are in "world units at pivot depth".
export function camera_pan(camera: OrbitCamera, delta_x: number, delta_y: number): void {
  const forward = v3_normalize(v3_sub(camera.pivot, camera_eye(camera)));
  const right = v3_normalize(v3_cross(forward, v3(0, 1, 0)));
  const up = v3_cross(right, forward);
  camera.pivot = v3_add(camera.pivot, v3_add(v3_scale(right, delta_x), v3_scale(up, delta_y)));
}

export function camera_zoom(camera: OrbitCamera, factor: number): void {
  camera.distance = Math.max(0.2, Math.min(60, camera.distance * factor));
}

// World units per screen pixel at the pivot's depth — converts pixel drags to pans.
export function camera_world_units_per_pixel(camera: OrbitCamera, viewport_height_pixels: number): number {
  return (2 * camera.distance * Math.tan(FOV_Y / 2)) / viewport_height_pixels;
}

// Camera basis in world space: right, true up, and forward (eye -> pivot).
export function camera_basis(camera: OrbitCamera): { right: V3; up: V3; forward: V3 } {
  const forward = v3_normalize(v3_sub(camera.pivot, camera_eye(camera)));
  const right = v3_normalize(v3_cross(forward, v3(0, 1, 0)));
  return { right, up: v3_cross(right, forward), forward };
}

// Snap-to-axis-view state (desktop `snap_camera`, key A): the last two snap
// yaws, so snapping while already snapped toggles back to the previous view.
export type CameraSnapState = { previous_snap_yaw: number; current_snap_yaw: number };

const QUARTER_TURN = Math.PI / 2;

function wrap_yaw_to_full_turn(yaw: number): number {
  const full_turn = 2 * Math.PI;
  return ((yaw % full_turn) + full_turn) % full_turn;
}

// Snap yaw to the nearest quarter turn (frontal/profile/back) and level the
// pitch. Already at a snap point -> jump to the previous snap; nearest snap is
// the one we're on (drifted but still rounds home) -> step one quarter turn in
// the drift direction. Ported from the desktop app's `snap_camera`.
export function camera_snap_to_axis_view(camera: OrbitCamera, snap_state: CameraSnapState): void {
  const yaw = wrap_yaw_to_full_turn(camera.yaw);
  let new_yaw = wrap_yaw_to_full_turn(Math.round(yaw / QUARTER_TURN) * QUARTER_TURN);
  if (yaw === snap_state.current_snap_yaw && camera.pitch === 0) {
    new_yaw = snap_state.previous_snap_yaw;
  } else if (new_yaw === snap_state.current_snap_yaw) {
    new_yaw = wrap_yaw_to_full_turn(new_yaw + QUARTER_TURN * Math.sign(yaw - new_yaw));
  }
  snap_state.previous_snap_yaw = snap_state.current_snap_yaw;
  snap_state.current_snap_yaw = new_yaw;
  camera.yaw = new_yaw;
  camera.pitch = 0;
}

export type Ray = { origin: V3; direction: V3 };

// Ray through a screen point given in CSS pixels.
export function camera_screen_ray(camera: OrbitCamera, screen: { x: number; y: number }, viewport_width: number, viewport_height: number): Ray {
  const ndc_x = (screen.x / viewport_width) * 2 - 1;
  const ndc_y = 1 - (screen.y / viewport_height) * 2;
  const half_height = Math.tan(FOV_Y / 2);
  const half_width = half_height * (viewport_width / viewport_height);
  const basis = camera_basis(camera);
  const direction = v3_normalize(v3_add(
    basis.forward,
    v3_add(v3_scale(basis.right, ndc_x * half_width), v3_scale(basis.up, ndc_y * half_height)),
  ));
  return { origin: camera_eye(camera), direction };
}

// World point -> CSS-pixel screen position; null when behind the camera.
export function camera_world_to_screen(
  camera: OrbitCamera, world: V3, viewport_width: number, viewport_height: number,
): { x: number; y: number } | null {
  const view_projection = camera_view_projection(camera, viewport_width / viewport_height);
  const clip = [0, 0, 0, 0];
  const world4 = [world.x, world.y, world.z, 1];
  for (let row = 0; row < 4; row++) {
    for (let col = 0; col < 4; col++) clip[row] += view_projection[col * 4 + row] * world4[col];
  }
  if (clip[3] <= 0) return null;
  return {
    x: (clip[0] / clip[3] * 0.5 + 0.5) * viewport_width,
    y: (0.5 - clip[1] / clip[3] * 0.5) * viewport_height,
  };
}

export function camera_view_projection(camera: OrbitCamera, aspect: number): Mat4 {
  const view = mat4_look_at(camera_eye(camera), camera.pivot, v3(0, 1, 0));
  return mat4_multiply(mat4_perspective(FOV_Y, aspect, NEAR, FAR), view);
}
