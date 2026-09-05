// Minimal vector/matrix math. Right-handed, y-up. Mat4 is column-major Float32Array
// (WebGL convention: m[col*4 + row]).

export type V2 = { x: number; y: number };
export type V3 = { x: number; y: number; z: number };
export type Mat4 = Float32Array;

export function v3(x: number, y: number, z: number): V3 { return { x, y, z }; }
export function v3_add(a: V3, b: V3): V3 { return { x: a.x + b.x, y: a.y + b.y, z: a.z + b.z }; }
export function v3_sub(a: V3, b: V3): V3 { return { x: a.x - b.x, y: a.y - b.y, z: a.z - b.z }; }
export function v3_scale(a: V3, s: number): V3 { return { x: a.x * s, y: a.y * s, z: a.z * s }; }
export function v3_dot(a: V3, b: V3): number { return a.x * b.x + a.y * b.y + a.z * b.z; }
export function v3_cross(a: V3, b: V3): V3 {
  return { x: a.y * b.z - a.z * b.y, y: a.z * b.x - a.x * b.z, z: a.x * b.y - a.y * b.x };
}
export function v3_length(a: V3): number { return Math.sqrt(v3_dot(a, a)); }
export function v3_normalize(a: V3): V3 {
  const len = v3_length(a);
  return len === 0 ? v3(0, 0, 0) : v3_scale(a, 1 / len);
}

// Rotate x by `angle` radians about the unit `axis` (Rodrigues, right-handed).
export function v3_rotate_about_axis(x: V3, axis: V3, angle: number): V3 {
  const cosine = Math.cos(angle);
  const sine = Math.sin(angle);
  return v3_add(
    v3_add(v3_scale(x, cosine), v3_scale(v3_cross(axis, x), sine)),
    v3_scale(axis, v3_dot(axis, x) * (1 - cosine)),
  );
}

// Rotate x by the minimal rotation that takes unit direction `from` to unit
// direction `to` (Rodrigues). Parallel directions return x unchanged (exactly —
// no float drift for the no-op case); antiparallel ones rotate 180° about
// `flip_axis`, which the caller picks perpendicular to `from`.
export function v3_rotate_between_directions(x: V3, from: V3, to: V3, flip_axis: V3): V3 {
  const axis_raw = v3_cross(from, to);
  const sine = v3_length(axis_raw);
  const cosine = v3_dot(from, to);
  if (sine < 1e-12) {
    if (cosine > 0) return x;
    // 180°: x -> 2 (k·x) k - x
    return v3_sub(v3_scale(flip_axis, 2 * v3_dot(flip_axis, x)), x);
  }
  const axis = v3_scale(axis_raw, 1 / sine);
  return v3_add(
    v3_add(v3_scale(x, cosine), v3_scale(v3_cross(axis, x), sine)),
    v3_scale(axis, v3_dot(axis, x) * (1 - cosine)),
  );
}

export function mat4_multiply(a: Mat4, b: Mat4): Mat4 {
  const out = new Float32Array(16);
  for (let col = 0; col < 4; col++) {
    for (let row = 0; row < 4; row++) {
      let sum = 0;
      for (let k = 0; k < 4; k++) sum += a[k * 4 + row] * b[col * 4 + k];
      out[col * 4 + row] = sum;
    }
  }
  return out;
}

export function mat4_perspective(fov_y_radians: number, aspect: number, near: number, far: number): Mat4 {
  const f = 1 / Math.tan(fov_y_radians / 2);
  const range_inverse = 1 / (near - far);
  // prettier-ignore
  return new Float32Array([
    f / aspect, 0, 0, 0,
    0, f, 0, 0,
    0, 0, (near + far) * range_inverse, -1,
    0, 0, 2 * near * far * range_inverse, 0,
  ]);
}

export function mat4_look_at(eye: V3, target: V3, up: V3): Mat4 {
  const forward = v3_normalize(v3_sub(eye, target)); // camera looks down -forward
  const right = v3_normalize(v3_cross(up, forward));
  const true_up = v3_cross(forward, right);
  // prettier-ignore
  return new Float32Array([
    right.x, true_up.x, forward.x, 0,
    right.y, true_up.y, forward.y, 0,
    right.z, true_up.z, forward.z, 0,
    -v3_dot(right, eye), -v3_dot(true_up, eye), -v3_dot(forward, eye), 1,
  ]);
}
