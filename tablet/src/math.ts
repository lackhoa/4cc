// Minimal vector/matrix math. Right-handed, y-up. Mat4 is column-major Float32Array
// (WebGL convention: m[col*4 + row]).

export type V2 = { x: number; y: number };
export type V3 = { x: number; y: number; z: number };
export type Mat4 = Float32Array;

export function v3(x: number, y: number, z: number): V3 { return { x, y, z }; }
export function v3_add(a: V3, b: V3): V3 { return { x: a.x + b.x, y: a.y + b.y, z: a.z + b.z }; }
export function v3_sub(a: V3, b: V3): V3 { return { x: a.x - b.x, y: a.y - b.y, z: a.z - b.z }; }
export function v3_scale(a: V3, s: number): V3 { return { x: a.x * s, y: a.y * s, z: a.z * s }; }
export function v3_lerp(a: V3, b: V3, t: number): V3 { return v3_add(v3_scale(a, 1 - t), v3_scale(b, t)); }
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

// Column-major, like every Mat4 here: element (row, col) sits at col * 4 + row.
export function mat4_identity(): Mat4 {
  // prettier-ignore
  return new Float32Array([
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1,
  ]);
}

// Apply an affine transform to a point (w = 1, no perspective divide).
export function mat4_transform_point(m: Mat4, p: V3): V3 {
  return {
    x: m[0] * p.x + m[4] * p.y + m[8] * p.z + m[12],
    y: m[1] * p.x + m[5] * p.y + m[9] * p.z + m[13],
    z: m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14],
  };
}

// General 4x4 inverse by cofactors. Throws on a singular matrix: a bone
// transform that cannot be undone is a bug in whoever built it.
export function mat4_invert(m: Mat4): Mat4 {
  const [a00, a01, a02, a03, a10, a11, a12, a13, a20, a21, a22, a23, a30, a31, a32, a33] = m;
  const b00 = a00 * a11 - a01 * a10, b01 = a00 * a12 - a02 * a10, b02 = a00 * a13 - a03 * a10;
  const b03 = a01 * a12 - a02 * a11, b04 = a01 * a13 - a03 * a11, b05 = a02 * a13 - a03 * a12;
  const b06 = a20 * a31 - a21 * a30, b07 = a20 * a32 - a22 * a30, b08 = a20 * a33 - a23 * a30;
  const b09 = a21 * a32 - a22 * a31, b10 = a21 * a33 - a23 * a31, b11 = a22 * a33 - a23 * a32;
  const determinant = b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06;
  if (Math.abs(determinant) < 1e-12) throw new Error("mat4_invert: singular matrix");
  const s = 1 / determinant;
  // prettier-ignore
  return new Float32Array([
    (a11 * b11 - a12 * b10 + a13 * b09) * s, (a02 * b10 - a01 * b11 - a03 * b09) * s, (a31 * b05 - a32 * b04 + a33 * b03) * s, (a22 * b04 - a21 * b05 - a23 * b03) * s,
    (a12 * b08 - a10 * b11 - a13 * b07) * s, (a00 * b11 - a02 * b08 + a03 * b07) * s, (a32 * b02 - a30 * b05 - a33 * b01) * s, (a20 * b05 - a22 * b02 + a23 * b01) * s,
    (a10 * b10 - a11 * b08 + a13 * b06) * s, (a01 * b08 - a00 * b10 - a03 * b06) * s, (a30 * b04 - a31 * b02 + a33 * b00) * s, (a21 * b02 - a20 * b04 - a23 * b00) * s,
    (a11 * b07 - a10 * b09 - a12 * b06) * s, (a00 * b09 - a01 * b07 + a02 * b06) * s, (a31 * b01 - a30 * b03 - a32 * b00) * s, (a20 * b03 - a21 * b01 + a22 * b00) * s,
  ]);
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

// Orthographic projection of the box |x| <= half_width, |y| <= half_height,
// near..far along -z (view space).
export function mat4_orthographic(half_width: number, half_height: number, near: number, far: number): Mat4 {
  const range_inverse = 1 / (near - far);
  // prettier-ignore
  return new Float32Array([
    1 / half_width, 0, 0, 0,
    0, 1 / half_height, 0, 0,
    0, 0, 2 * range_inverse, 0,
    0, 0, (near + far) * range_inverse, 1,
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
