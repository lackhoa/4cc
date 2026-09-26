// The Loomis ball-and-plane head (Drawing the Head and Hands, 1956, Plate 1 / Plate 18)
// as polylines, built step by step. Unit ball radius, ball center at the origin, y up,
// face toward +z, ears at +-x. Every length is in ball radii. Numbers and what they
// come from: notes/tasks/autodraw_draw_as_data/plan-skull-construction-docs.md Q13.

import { V3, v3, v3_add, v3_scale } from "./math";

export type LoomisParams = {
  slice_depth: number; // how deep each side of the ball is sliced flat ("a fairly thin slice")
  thirds_unit: number; // hairline above the brow, nose base below it, chin two units below
  jaw_width: number; // jaw corner x as a fraction of the side plane's x
  ear_back: number; // ear center behind the side plane's vertical center line
  chin_forward: number; // z of the chin
};

// Plate 1: hairline halfway from brow to the top pole, nose and chin "about equal to the
// space of the forehead". Slice depth, jaw and ear are Loomis's "about" turned into a number.
export const LOOMIS_PLATE_1: LoomisParams = {
  slice_depth: 0.2,
  thirds_unit: 0.5,
  jaw_width: 0.8,
  ear_back: 0.5,
  chin_forward: 0.8,
};
// Plate 18: the head is 3.5 units tall with nose-to-brow one unit, so with the ball top
// at the head top the radius is 1.5 units and a unit is 2/3 r.
export const LOOMIS_PLATE_18: LoomisParams = { ...LOOMIS_PLATE_1, thirds_unit: 2 / 3 };

// What a curve lies on decides which part of it faces the eye: the far side of the ball
// and a side plane turned away are hidden, as an opaque head would hide them.
export type LoomisSurface = { kind: "none" } | { kind: "ball" } | { kind: "plane"; normal: V3 };

export type LoomisPolyline = {
  points: V3[];
  closed: boolean;
  surface: LoomisSurface;
};

export type LoomisStep = {
  name: string;
  polylines: LoomisPolyline[];
};

// Points of a circle (or arc) with the given center and two perpendicular axes of
// length `radius`; angles in radians, counted from axis_u toward axis_v.
function arc(center: V3, axis_u: V3, axis_v: V3, radius: number, from: number, to: number, segments: number): V3[] {
  const points: V3[] = [];
  for (let i = 0; i <= segments; i++) {
    const angle = from + (to - from) * (i / segments);
    points.push(v3_add(center, v3_add(v3_scale(axis_u, radius * Math.cos(angle)), v3_scale(axis_v, radius * Math.sin(angle)))));
  }
  return points;
}

const FULL_TURN = 2 * Math.PI;
const X = v3(1, 0, 0);
const Y = v3(0, 1, 0);
const Z = v3(0, 0, 1);

function closed_on_ball(points: V3[]): LoomisPolyline { return { points, closed: true, surface: { kind: "ball" } }; }
function open_on_ball(points: V3[]): LoomisPolyline { return { points, closed: false, surface: { kind: "ball" } }; }
function on_plane(points: V3[], closed: boolean, normal: V3): LoomisPolyline { return { points, closed, surface: { kind: "plane", normal } }; }
function open_line(points: V3[]): LoomisPolyline { return { points, closed: false, surface: { kind: "none" } }; }

// Latitude ring at height y on the unit ball, front half only (z > 0), left to right.
function front_latitude(y: number): V3[] {
  const radius = Math.sqrt(Math.max(0, 1 - y * y));
  return arc(v3(0, y, 0), Z, X, radius, -Math.PI / 2, Math.PI / 2, 32);
}

// One side of the head (side = +1 right ear, -1 left ear).
function side_cut(params: LoomisParams, side: number): LoomisPolyline[] {
  const x = side * (1 - params.slice_depth);
  const cut_radius = Math.sqrt(1 - (1 - params.slice_depth) ** 2);
  const center = v3(x, 0, 0);
  const normal = v3(side, 0, 0);
  return [
    on_plane(arc(center, Z, Y, cut_radius, 0, FULL_TURN, 64), true, normal),
    on_plane([v3(x, cut_radius, 0), v3(x, -cut_radius, 0)], false, normal), // the side plane's vertical center line
    on_plane([v3(x, 0, cut_radius), v3(x, 0, -cut_radius)], false, normal), // the brow line continued across the side plane
  ];
}

function jaw(params: LoomisParams, side: number): LoomisPolyline {
  const unit = params.thirds_unit;
  const plane_x = side * (1 - params.slice_depth);
  const ear_root = v3(plane_x, 0, 0);
  const jaw_corner = v3(plane_x * params.jaw_width, -1.5 * unit, 0.15);
  const chin_corner = v3(side * 0.25, -2 * unit, params.chin_forward);
  return open_line([ear_root, jaw_corner, chin_corner]);
}

function ear(params: LoomisParams, side: number): LoomisPolyline {
  const unit = params.thirds_unit;
  const plane_x = side * (1 - params.slice_depth);
  // Top on the brow line, bottom on the nose line (one unit tall), a little behind the
  // side plane's vertical center line.
  const center = v3(plane_x, -unit / 2, -params.ear_back);
  const ellipse = arc(center, Z, Y, 1, 0, FULL_TURN, 32).map((p) => v3(p.x, center.y + (p.y - center.y) * (unit / 2), center.z + (p.z - center.z) * 0.14));
  return on_plane(ellipse, true, v3(side, 0, 0));
}

export function build_loomis_head(params: LoomisParams): LoomisStep[] {
  const unit = params.thirds_unit;
  const chin_left = v3(-0.25, -2 * unit, params.chin_forward);
  const chin_right = v3(0.25, -2 * unit, params.chin_forward);
  // Center line: over the front of the ball from the top pole down to the nose line,
  // then straight down the face to the chin.
  const center_line_on_ball = arc(v3(0, 0, 0), Y, Z, 1, 0, Math.PI / 2 + Math.asin(unit), 24);
  const center_line_face = [center_line_on_ball[center_line_on_ball.length - 1], v3(0, -2 * unit, params.chin_forward)];
  return [
    { name: "ball", polylines: [] }, // the ball's outline depends on the eye: the page draws it
    { name: "side cuts", polylines: [...side_cut(params, 1), ...side_cut(params, -1)] },
    {
      name: "brow line and center line",
      polylines: [closed_on_ball(arc(v3(0, 0, 0), Z, X, 1, 0, FULL_TURN, 64)), open_on_ball(center_line_on_ball), open_line(center_line_face)],
    },
    {
      name: "hairline, nose, chin",
      polylines: [open_on_ball(front_latitude(unit)), open_on_ball(front_latitude(-unit)), open_line([chin_left, chin_right])],
    },
    { name: "jaw", polylines: [jaw(params, 1), jaw(params, -1)] },
    { name: "ears", polylines: [ear(params, 1), ear(params, -1)] },
  ];
}
