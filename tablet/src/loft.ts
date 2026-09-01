// Ruled-surface loft between two strokes (step 6 / Q14, Q22). Linked, not
// baked: the loft stores stroke indices and is retessellated from the current
// strokes every frame, so editing a rail reshapes the surface. Shading is baked
// CPU-side per vertex (headlight: brightness from the surface normal vs the
// camera forward, two-sided), which fits the existing position+color pipeline.

import { OrbitCamera, camera_basis } from "./camera";
import { Loft, Stroke, TabletDocument, bezier_point, bezier_tangent, stroke_control_points } from "./document";
import { V3, v3_add, v3_cross, v3_dot, v3_length, v3_normalize, v3_scale, v3_sub } from "./math";

const LOFT_SAMPLES_ALONG_RAILS = 24;
const LOFT_ROWS_ACROSS = 4;
const LOFT_AMBIENT = 0.35;

type RailSample = { position: V3; tangent: V3 };

function sample_rail(stroke: Stroke, tablet_document: TabletDocument, sample_count: number, reversed: boolean): RailSample[] {
  const points = stroke_control_points(stroke, tablet_document);
  const samples: RailSample[] = [];
  for (let i = 0; i <= sample_count; i++) {
    const t = reversed ? 1 - i / sample_count : i / sample_count;
    const tangent = bezier_tangent(points, t);
    samples.push({
      position: bezier_point(points, t),
      tangent: reversed ? v3_scale(tangent, -1) : tangent,
    });
  }
  return samples;
}

// Rails drawn in opposite directions would twist the surface; detect by
// comparing endpoint pairings and reverse rail B when crossed.
function rails_are_crossed(rail_a: Stroke, rail_b: Stroke, tablet_document: TabletDocument): boolean {
  const a_start = tablet_document.vertices[rail_a.p0_vertex];
  const a_end = tablet_document.vertices[rail_a.p3_vertex];
  const b_start = tablet_document.vertices[rail_b.p0_vertex];
  const b_end = tablet_document.vertices[rail_b.p3_vertex];
  const straight = v3_length(v3_sub(a_start, b_start)) + v3_length(v3_sub(a_end, b_end));
  const crossed = v3_length(v3_sub(a_start, b_end)) + v3_length(v3_sub(a_end, b_start));
  return crossed < straight;
}

// P(u, v) = lerp(A(u), B(u), v) — normals from the analytic partials.
export function append_loft_mesh(
  loft: Loft, tablet_document: TabletDocument, camera: OrbitCamera,
  color: { r: number; g: number; b: number }, out: number[],
): void {
  const rail_a = tablet_document.strokes[loft.stroke_a];
  const rail_b = tablet_document.strokes[loft.stroke_b];
  const samples_a = sample_rail(rail_a, tablet_document, LOFT_SAMPLES_ALONG_RAILS, false);
  const samples_b = sample_rail(rail_b, tablet_document, LOFT_SAMPLES_ALONG_RAILS, rails_are_crossed(rail_a, rail_b, tablet_document));
  const camera_forward = camera_basis(camera).forward;

  const shaded = (u: number, v: number): { position: V3; brightness: number } => {
    const a = samples_a[u];
    const b = samples_b[u];
    const t = v / LOFT_ROWS_ACROSS;
    const position = v3_add(v3_scale(a.position, 1 - t), v3_scale(b.position, t));
    const du = v3_add(v3_scale(a.tangent, 1 - t), v3_scale(b.tangent, t));
    const dv = v3_sub(b.position, a.position);
    const cross = v3_cross(du, dv);
    let brightness = 1;
    if (v3_length(cross) > 1e-9) {
      const normal = v3_normalize(cross);
      brightness = LOFT_AMBIENT + (1 - LOFT_AMBIENT) * Math.abs(v3_dot(normal, camera_forward));
    }
    return { position, brightness };
  };

  const push = (vertex: { position: V3; brightness: number }) => {
    out.push(
      vertex.position.x, vertex.position.y, vertex.position.z,
      color.r * vertex.brightness, color.g * vertex.brightness, color.b * vertex.brightness,
    );
  };
  for (let v = 0; v < LOFT_ROWS_ACROSS; v++) {
    for (let u = 0; u < LOFT_SAMPLES_ALONG_RAILS; u++) {
      const corner_00 = shaded(u, v);
      const corner_10 = shaded(u + 1, v);
      const corner_11 = shaded(u + 1, v + 1);
      const corner_01 = shaded(u, v + 1);
      push(corner_00); push(corner_10); push(corner_11);
      push(corner_00); push(corner_11); push(corner_01);
    }
  }
}
