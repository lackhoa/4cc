// Lifting the hairline drawn on the two man plates onto the skin surface
// (plan-hairline-reference-plate.md Q8): each 2D cubic is sampled along its length,
// each sample is cast through its view (front: along -z, profile: along -x) onto the
// skin patches, hits that met the surface at a grazing angle are dropped, the rest are
// merged by their angle around the head's vertical axis (0 = forehead midline), and the
// merged chain becomes straight on-surface strokes with surface-pinned endpoints.
// Where both views hit the same angle, the squarer hit is taken (no blending).
import { Layer, Patch, SKULL_BONE_ID, StrokeId, TabletDocument, add_straight_stroke, add_vertex, stroke_by_id } from "../../src/document";
import { V2, V3, v3, v3_cross, v3_dot, v3_normalize, v3_sub } from "../../src/math";
import { patch_point, pick_surface_point_projected, pin_vertex_to_surface } from "../../src/patch";
import { PlateCubic, PlateRegistration, plate_cubic_samples, world_from_pixel } from "../../src/loomis_planes_man_plate";

export const SAMPLES_PER_CUBIC = 32;
// Hits whose ray met the surface at less than this cosine are grazing: dropped.
export const MIN_RAY_NORMAL_COSINE = 0.3;
// Hits are merged in bins of this angle around the head's vertical axis.
const MERGE_BIN_RADIANS = (3 * Math.PI) / 180;
// Merged points per on-surface stroke (consecutive strokes share an endpoint).
const MERGED_POINTS_PER_STROKE = 8;
const SKIN_LAYERS: ReadonlySet<Layer> = new Set<Layer>(["skin"]);

export type LiftedHit = {
  patch: Patch;
  u: number;
  v: number;
  position: V3;
  cosine: number; // how squarely the ray met the surface
  from_front_view: boolean;
};

// Unit normal of the patch at (u, v) by central differences; null on a degenerate spot.
function surface_normal(patch: Patch, tablet_document: TabletDocument, u: number, v: number): V3 | null {
  const h = 1e-3;
  const at = (du: number, dv: number): V3 | null => patch_point(patch, tablet_document, Math.min(1, Math.max(0, u + du)), Math.min(1, Math.max(0, v + dv)));
  const u_plus = at(h, 0), u_minus = at(-h, 0), v_plus = at(0, h), v_minus = at(0, -h);
  if (u_plus === null || u_minus === null || v_plus === null || v_minus === null) return null;
  const normal = v3_cross(v3_sub(u_plus, u_minus), v3_sub(v_plus, v_minus));
  return v3_dot(normal, normal) < 1e-18 ? null : v3_normalize(normal);
}

// Cast the samples of one view's hairline (its cubics, SAMPLES_PER_CUBIC each) onto the
// skin. The document holds the x >= 0 side only, so front samples left of the midline
// are mirrored onto it.
export function cast_hairline_samples(tablet_document: TabletDocument, registration: PlateRegistration, hairline: PlateCubic[]): LiftedHit[] {
  const is_front = registration.is_front_view;
  const ray_direction = is_front ? v3(0, 0, -1) : v3(-1, 0, 0);
  const eye = is_front ? v3(0, 0, 1000) : v3(1000, 0, 0);
  const project = (world: V3): V2 => (is_front ? { x: world.x, y: world.y } : { x: world.z, y: world.y });
  const hits: LiftedHit[] = [];
  for (const sample of hairline.flatMap((cubic) => plate_cubic_samples(cubic, SAMPLES_PER_CUBIC))) {
    const world = world_from_pixel(registration, sample);
    if (is_front) world.x = Math.abs(world.x);
    const hit = pick_surface_point_projected(tablet_document, project, eye, project(world), SKIN_LAYERS);
    if (hit === null) continue;
    const normal = surface_normal(hit.patch, tablet_document, hit.u, hit.v);
    if (normal === null) continue;
    const cosine = Math.abs(v3_dot(normal, ray_direction));
    if (cosine < MIN_RAY_NORMAL_COSINE) continue;
    hits.push({ patch: hit.patch, u: hit.u, v: hit.v, position: hit.position, cosine, from_front_view: is_front });
  }
  return hits;
}

// Angle around the head's vertical axis: 0 at the forehead midline (+z is forward),
// pi/2 at the side, more towards the back.
export function hit_angle(hit: LiftedHit): number {
  return Math.atan2(hit.position.x, hit.position.z);
}

// One hit per angle bin, the squarest, ordered from the forehead outward.
export function merge_hits_by_angle(hits: LiftedHit[]): LiftedHit[] {
  const best_per_bin = new Map<number, LiftedHit>();
  for (const hit of hits) {
    const bin = Math.round(hit_angle(hit) / MERGE_BIN_RADIANS);
    const current = best_per_bin.get(bin);
    if (current === undefined || hit.cosine > current.cosine) best_per_bin.set(bin, hit);
  }
  return [...best_per_bin.entries()].sort((a, b) => a[0] - b[0]).map((entry) => entry[1]);
}

// Write the merged chain into the document as straight on-surface strokes, every
// MERGED_POINTS_PER_STROKE points, endpoints pinned to the surface. Returns the stroke ids.
export function write_hairline_strokes(tablet_document: TabletDocument, merged: LiftedHit[]): StrokeId[] {
  if (merged.length < 2) return [];
  const endpoint_hits: LiftedHit[] = [];
  for (let i = 0; i < merged.length; i += MERGED_POINTS_PER_STROKE) endpoint_hits.push(merged[i]);
  if (endpoint_hits[endpoint_hits.length - 1] !== merged[merged.length - 1]) endpoint_hits.push(merged[merged.length - 1]);
  const vertices = endpoint_hits.map((hit) => {
    const vertex = add_vertex(tablet_document, hit.position, SKULL_BONE_ID);
    pin_vertex_to_surface(tablet_document, vertex, hit.patch, hit.u, hit.v);
    return vertex;
  });
  const stroke_ids: StrokeId[] = [];
  for (let i = 0; i + 1 < vertices.length; i++) {
    const stroke_id = add_straight_stroke(tablet_document, vertices[i], vertices[i + 1], "skin");
    stroke_by_id(tablet_document, stroke_id).on_surface = true;
    stroke_ids.push(stroke_id);
  }
  return stroke_ids;
}
