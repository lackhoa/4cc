// Surface of revolution (Q26 experiment): spin a profile stroke around the
// line through its own endpoints — a half-circle arc revolves into a sphere, a
// vase profile into a vase, with no separate axis-picking step. Linked like
// lofts: retessellated from the current stroke every frame. Shading matches
// loft.ts (CPU-baked two-sided headlight).

import { OrbitCamera, camera_basis } from "./camera";
import { Revolve, TabletDocument, bezier_point, bezier_tangent, stroke_control_points } from "./document";
import { V3, v3_add, v3_cross, v3_dot, v3_length, v3_normalize, v3_scale, v3_sub } from "./math";

const REVOLVE_PROFILE_SAMPLES = 24;
const REVOLVE_ANGLE_STEPS = 32;
const REVOLVE_AMBIENT = 0.35;
const AXIS_MIN_LENGTH = 1e-4; // closed profiles (endpoints coincide) have no axis

type ProfileSample = { position: V3; tangent: V3 };

// Rodrigues rotation of a vector around a unit axis.
function rotate_around_axis(vector: V3, axis: V3, angle: number): V3 {
  const cos = Math.cos(angle);
  const sin = Math.sin(angle);
  return v3_add(
    v3_add(v3_scale(vector, cos), v3_scale(v3_cross(axis, vector), sin)),
    v3_scale(axis, v3_dot(axis, vector) * (1 - cos)),
  );
}

export function append_revolve_mesh(
  revolve: Revolve, tablet_document: TabletDocument, camera: OrbitCamera,
  color: { r: number; g: number; b: number }, out: number[],
): void {
  const stroke = tablet_document.strokes[revolve.stroke];
  const points = stroke_control_points(stroke, tablet_document);
  const profile: ProfileSample[] = [];
  for (let i = 0; i <= REVOLVE_PROFILE_SAMPLES; i++) {
    const t = i / REVOLVE_PROFILE_SAMPLES;
    profile.push({ position: bezier_point(points, t), tangent: bezier_tangent(points, t) });
  }
  const axis_origin = profile[0].position;
  const axis_span = v3_sub(profile[profile.length - 1].position, axis_origin);
  if (v3_length(axis_span) < AXIS_MIN_LENGTH) return;
  const axis = v3_normalize(axis_span);
  const camera_forward = camera_basis(camera).forward;

  const shaded = (u: number, step: number): { position: V3; brightness: number } => {
    const angle = (step / REVOLVE_ANGLE_STEPS) * 2 * Math.PI;
    const offset = v3_sub(profile[u].position, axis_origin);
    const position = v3_add(axis_origin, rotate_around_axis(offset, axis, angle));
    const du = rotate_around_axis(profile[u].tangent, axis, angle);
    const d_angle = v3_cross(axis, v3_sub(position, axis_origin));
    const cross = v3_cross(du, d_angle);
    let brightness = 1;
    if (v3_length(cross) > 1e-9) {
      const normal = v3_normalize(cross);
      brightness = REVOLVE_AMBIENT + (1 - REVOLVE_AMBIENT) * Math.abs(v3_dot(normal, camera_forward));
    }
    return { position, brightness };
  };

  const push = (vertex: { position: V3; brightness: number }) => {
    out.push(
      vertex.position.x, vertex.position.y, vertex.position.z,
      color.r * vertex.brightness, color.g * vertex.brightness, color.b * vertex.brightness,
    );
  };
  for (let step = 0; step < REVOLVE_ANGLE_STEPS; step++) {
    for (let u = 0; u < REVOLVE_PROFILE_SAMPLES; u++) {
      const corner_00 = shaded(u, step);
      const corner_10 = shaded(u + 1, step);
      const corner_11 = shaded(u + 1, step + 1);
      const corner_01 = shaded(u, step + 1);
      push(corner_00); push(corner_10); push(corner_11);
      push(corner_00); push(corner_11); push(corner_01);
    }
  }
}
