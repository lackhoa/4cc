// One-off: numerically check mat4_invert against mat4_multiply (M * inv(M) == I).
import { mat4_invert, mat4_multiply, mat4_transform_point, v3 } from "../src/math";

const m = new Float32Array([
  0.8, 0.3, -0.2, 0,
  -0.1, 1.2, 0.4, 0,
  0.5, -0.3, 0.9, 0,
  3, -2, 7, 1,
]);
const product = mat4_multiply(m, mat4_invert(m));
let max_error = 0;
for (let i = 0; i < 16; i++) max_error = Math.max(max_error, Math.abs(product[i] - (i % 5 === 0 ? 1 : 0)));
const p = v3(1.5, -2.5, 4);
const round_trip = mat4_transform_point(mat4_invert(m), mat4_transform_point(m, p));
console.log("max |M*inv(M) - I| =", max_error, "round trip =", round_trip);
if (max_error > 1e-5) throw new Error("mat4_invert is wrong");
