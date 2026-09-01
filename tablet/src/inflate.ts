// Inflate (Q26 experiment, Teddy-style fake): a closed-ish stroke silhouette
// puffs into a pillow — rings shrink from the boundary toward the centroid
// while rising on a quarter-circle dome profile, mirrored front/back into a
// closed cushion. Star-shaped-to-centroid, so it wants convex-ish outlines;
// strongly concave silhouettes will self-intersect (accepted for the
// experiment — real Teddy uses a chordal axis). Linked like lofts/revolves.
// Since the Sketchpad pivot a stroke is one cubic, so a silhouette can't
// actually close on itself (coincident endpoints kill the plane frame) —
// inflate wants a multi-stroke chain boundary, which comes later.

import { OrbitCamera, camera_basis } from "./camera";
import {
  Inflate, Stroke, StrokePlane2D, TabletDocument, bezier_point, plane_2d_point_to_world,
  stroke_control_points, stroke_plane_2d, world_point_to_plane_2d,
} from "./document";
import { V2, V3, v3_add, v3_cross, v3_dot, v3_length, v3_normalize, v3_scale, v3_sub } from "./math";

const INFLATE_BOUNDARY_SAMPLES = 48;
const INFLATE_RINGS = 8;
const INFLATE_HEIGHT_FACTOR = 0.6; // pole height = factor * mean boundary-to-centroid radius
const INFLATE_PROFILE_SAMPLES = 64;
const INFLATE_AMBIENT = 0.35;

// One point of a user-drawn cross-section profile, in the silhouette's frame:
// radial_fraction 0 = centroid, 1 = on the silhouette boundary; height is the
// unsigned distance from the silhouette plane (the sign picked which side).
type ProfileSample = { radial_fraction: number; height: number };

// World samples of the stroke's cubic; the gap between last and first point
// closes implicitly when used as a boundary.
function sample_stroke_world(stroke: Stroke, tablet_document: TabletDocument, sample_count: number): V3[] {
  const points = stroke_control_points(stroke, tablet_document);
  const samples: V3[] = [];
  for (let i = 0; i < sample_count; i++) {
    samples.push(bezier_point(points, i / (sample_count - 1)));
  }
  return samples;
}

// Project the profile stroke into the silhouette's frame and split it by which
// side of the silhouette plane each point is on. Every side gets a synthetic
// (radial_fraction 1, height 0) sample so the rim stays welded to the outline
// even when the drawn curve doesn't quite touch the plane. Returns lists sorted
// by radial_fraction; a side with no real samples returns null (default dome).
function build_side_profiles(
  profile_stroke: Stroke, tablet_document: TabletDocument,
  silhouette_plane: StrokePlane2D, boundary: V2[], centroid: V2,
): { front: ProfileSample[] | null; back: ProfileSample[] | null } {
  const centroid_world = plane_2d_point_to_world(silhouette_plane, centroid);
  const front: ProfileSample[] = [];
  const back: ProfileSample[] = [];
  for (const world of sample_stroke_world(profile_stroke, tablet_document, INFLATE_PROFILE_SAMPLES)) {
    const height = v3_dot(v3_sub(world, centroid_world), silhouette_plane.normal);
    const in_plane = world_point_to_plane_2d(silhouette_plane, world);
    const radial: V2 = { x: in_plane.x - centroid.x, y: in_plane.y - centroid.y };
    const radial_length = Math.hypot(radial.x, radial.y);
    if (radial_length < 1e-6) {
      (height >= 0 ? front : back).push({ radial_fraction: 0, height: Math.abs(height) });
      continue;
    }
    // Boundary radius in this direction: the boundary sample most aligned with
    // the radial direction (cosine match — silhouettes are star-shaped anyway).
    let best_alignment = -Infinity;
    let boundary_radius = 1;
    for (const boundary_point of boundary) {
      const direction: V2 = { x: boundary_point.x - centroid.x, y: boundary_point.y - centroid.y };
      const direction_length = Math.hypot(direction.x, direction.y);
      if (direction_length < 1e-6) continue;
      const alignment = (direction.x * radial.x + direction.y * radial.y) / direction_length;
      if (alignment > best_alignment) {
        best_alignment = alignment;
        boundary_radius = direction_length;
      }
    }
    const sample = { radial_fraction: radial_length / boundary_radius, height: Math.abs(height) };
    (height >= 0 ? front : back).push(sample);
  }
  const finish = (samples: ProfileSample[]): ProfileSample[] | null => {
    if (samples.length === 0) return null;
    samples.push({ radial_fraction: 1, height: 0 });
    samples.sort((a, b) => a.radial_fraction - b.radial_fraction);
    return samples;
  };
  return { front: finish(front), back: finish(back) };
}

// Piecewise-linear height lookup, clamped at both ends.
function profile_height(samples: ProfileSample[], radial_fraction: number): number {
  if (radial_fraction <= samples[0].radial_fraction) return samples[0].height;
  for (let i = 1; i < samples.length; i++) {
    if (radial_fraction <= samples[i].radial_fraction) {
      const span = samples[i].radial_fraction - samples[i - 1].radial_fraction;
      const t = span < 1e-9 ? 0 : (radial_fraction - samples[i - 1].radial_fraction) / span;
      return samples[i - 1].height + (samples[i].height - samples[i - 1].height) * t;
    }
  }
  return samples[samples.length - 1].height;
}

export function append_inflate_mesh(
  inflate: Inflate, tablet_document: TabletDocument, camera: OrbitCamera,
  color: { r: number; g: number; b: number }, out: number[],
): void {
  const stroke = tablet_document.strokes[inflate.stroke];
  const plane = stroke_plane_2d(stroke, tablet_document);
  if (v3_length(plane.normal) < 0.5) return; // coincident endpoints — no frame
  const boundary = sample_stroke_world(stroke, tablet_document, INFLATE_BOUNDARY_SAMPLES)
    .map((world) => world_point_to_plane_2d(plane, world));
  const centroid: V2 = { x: 0, y: 0 };
  for (const point of boundary) {
    centroid.x += point.x / boundary.length;
    centroid.y += point.y / boundary.length;
  }
  let mean_radius = 0;
  for (const point of boundary) {
    mean_radius += Math.hypot(point.x - centroid.x, point.y - centroid.y) / boundary.length;
  }
  const pole_height = INFLATE_HEIGHT_FACTOR * mean_radius;
  const camera_forward = camera_basis(camera).forward;
  const profiles = inflate.profile === null
    ? { front: null, back: null }
    : build_side_profiles(tablet_document.strokes[inflate.profile], tablet_document, plane, boundary, centroid);

  // side = +1 front dome, -1 back dome; ring 0 = the flat boundary (shared edge).
  const ring_point = (boundary_index: number, ring: number, side: number): V3 => {
    const t = ring / INFLATE_RINGS;
    const base = boundary[boundary_index % boundary.length];
    const in_plane: V2 = {
      x: base.x + (centroid.x - base.x) * t,
      y: base.y + (centroid.y - base.y) * t,
    };
    const side_profile = side > 0 ? profiles.front : profiles.back;
    const height = side_profile === null
      ? pole_height * Math.sqrt(t * (2 - t)) // quarter-circle dome profile
      : profile_height(side_profile, 1 - t);
    return v3_add(plane_2d_point_to_world(plane, in_plane), v3_scale(plane.normal, side * height));
  };

  const push_triangle = (a: V3, b: V3, c: V3) => {
    const cross = v3_cross(v3_sub(b, a), v3_sub(c, a));
    let brightness = 1;
    if (v3_length(cross) > 1e-9) {
      const face_normal = v3_normalize(cross);
      brightness = INFLATE_AMBIENT + (1 - INFLATE_AMBIENT) * Math.abs(v3_dot(face_normal, camera_forward));
    }
    for (const vertex of [a, b, c]) {
      out.push(vertex.x, vertex.y, vertex.z, color.r * brightness, color.g * brightness, color.b * brightness);
    }
  };

  for (const side of [1, -1]) {
    for (let ring = 0; ring < INFLATE_RINGS; ring++) {
      for (let i = 0; i < boundary.length; i++) {
        const corner_00 = ring_point(i, ring, side);
        const corner_10 = ring_point(i + 1, ring, side);
        const corner_11 = ring_point(i + 1, ring + 1, side);
        const corner_01 = ring_point(i, ring + 1, side);
        push_triangle(corner_00, corner_10, corner_11);
        push_triangle(corner_00, corner_11, corner_01);
      }
    }
  }
}
