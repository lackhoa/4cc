// Coons patch (Q23/Q26): four boundary strokes chained into a loop, filled
// with the bilinearly blended Coons surface. Sample-based (no bicubic fit):
// each side is presampled on the grid resolution and blended directly, so the
// patch hugs the drawn boundaries exactly at the rims. Linked like the other
// surfaces — retessellated every frame, so editing any side reshapes the fill.
// With shared vertices (Sketchpad model) the corners are exact whenever the
// sides actually join; the corner averaging stays for sloppy unjoined picks.

import { OrbitCamera, camera_basis } from "./camera";
import { Coons, Stroke, TabletDocument, bezier_point, stroke_control_points } from "./document";
import { V3, v3_add, v3_cross, v3_dot, v3_length, v3_normalize, v3_scale, v3_sub } from "./math";

const COONS_GRID = 16; // grid cells per side
const COONS_AMBIENT = 0.35;

type OrientedSide = { stroke: Stroke; reversed: boolean };

function stroke_endpoint(stroke: Stroke, tablet_document: TabletDocument, at_end: boolean): V3 {
  return tablet_document.vertices[at_end ? stroke.p3_vertex : stroke.p0_vertex];
}

// Arrange the four tapped strokes into a head-to-tail loop, greedily picking
// the nearest remaining endpoint. Tap order doesn't need to walk the loop and
// draw direction doesn't matter; sloppy corner gaps are fine (averaged later).
function chain_sides(strokes: Stroke[], tablet_document: TabletDocument): OrientedSide[] {
  const sides: OrientedSide[] = [{ stroke: strokes[0], reversed: false }];
  const remaining = strokes.slice(1);
  let loop_end = stroke_endpoint(strokes[0], tablet_document, true);
  while (remaining.length > 0) {
    let best_index = 0;
    let best_reversed = false;
    let best_distance = Infinity;
    for (let i = 0; i < remaining.length; i++) {
      for (const reversed of [false, true]) {
        const distance = v3_length(v3_sub(stroke_endpoint(remaining[i], tablet_document, reversed), loop_end));
        if (distance < best_distance) {
          best_distance = distance;
          best_index = i;
          best_reversed = reversed;
        }
      }
    }
    const stroke = remaining.splice(best_index, 1)[0];
    sides.push({ stroke, reversed: best_reversed });
    loop_end = stroke_endpoint(stroke, tablet_document, !best_reversed);
  }
  return sides;
}

// Uniform world-space samples over the side, inclusive of both endpoints.
function sample_side(side: OrientedSide, tablet_document: TabletDocument, sample_count: number): V3[] {
  const points = stroke_control_points(side.stroke, tablet_document);
  const samples: V3[] = [];
  for (let i = 0; i <= sample_count; i++) {
    const t = side.reversed ? 1 - i / sample_count : i / sample_count;
    samples.push(bezier_point(points, t));
  }
  return samples;
}

function v3_average(a: V3, b: V3): V3 {
  return v3_scale(v3_add(a, b), 0.5);
}

export function append_coons_mesh(
  coons: Coons, tablet_document: TabletDocument, camera: OrbitCamera,
  color: { r: number; g: number; b: number }, out: number[],
): void {
  const sides = chain_sides(coons.strokes.map((index) => tablet_document.strokes[index]), tablet_document);
  // Loop traversal order: bottom (s 0→1), right (t 0→1), top and left run
  // backwards along the loop, so index from the far end when reading them.
  const bottom = sample_side(sides[0], tablet_document, COONS_GRID);
  const right = sample_side(sides[1], tablet_document, COONS_GRID);
  const top_backwards = sample_side(sides[2], tablet_document, COONS_GRID);
  const left_backwards = sample_side(sides[3], tablet_document, COONS_GRID);
  const top = (i: number): V3 => top_backwards[COONS_GRID - i];
  const left = (j: number): V3 => left_backwards[COONS_GRID - j];
  // Adjacent sides won't share exact corner points (sketched) — average them
  // so the discrepancy is smeared across the patch instead of tearing a rim.
  const corner_00 = v3_average(bottom[0], left(0));
  const corner_10 = v3_average(bottom[COONS_GRID], right[0]);
  const corner_11 = v3_average(right[COONS_GRID], top(COONS_GRID));
  const corner_01 = v3_average(top(0), left(COONS_GRID));

  const surface_point = (i: number, j: number): V3 => {
    const s = i / COONS_GRID;
    const t = j / COONS_GRID;
    const ruled = v3_add(
      v3_add(v3_scale(bottom[i], 1 - t), v3_scale(top(i), t)),
      v3_add(v3_scale(left(j), 1 - s), v3_scale(right[j], s)),
    );
    const bilinear = v3_add(
      v3_add(v3_scale(corner_00, (1 - s) * (1 - t)), v3_scale(corner_10, s * (1 - t))),
      v3_add(v3_scale(corner_01, (1 - s) * t), v3_scale(corner_11, s * t)),
    );
    return v3_sub(ruled, bilinear);
  };

  const camera_forward = camera_basis(camera).forward;
  const push_triangle = (a: V3, b: V3, c: V3) => {
    const cross = v3_cross(v3_sub(b, a), v3_sub(c, a));
    let brightness = 1;
    if (v3_length(cross) > 1e-9) {
      const face_normal = v3_normalize(cross);
      brightness = COONS_AMBIENT + (1 - COONS_AMBIENT) * Math.abs(v3_dot(face_normal, camera_forward));
    }
    for (const vertex of [a, b, c]) {
      out.push(vertex.x, vertex.y, vertex.z, color.r * brightness, color.g * brightness, color.b * brightness);
    }
  };
  for (let j = 0; j < COONS_GRID; j++) {
    for (let i = 0; i < COONS_GRID; i++) {
      const point_00 = surface_point(i, j);
      const point_10 = surface_point(i + 1, j);
      const point_11 = surface_point(i + 1, j + 1);
      const point_01 = surface_point(i, j + 1);
      push_triangle(point_00, point_10, point_11);
      push_triangle(point_00, point_11, point_01);
    }
  }
}
