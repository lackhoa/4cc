// Regression check for plan-smooth-knot-wrong-handle.md (Q5): swinging the far handle
// of the knot's follower (193, stroke_b) must keep the knot at 174 smooth, and the
// per-frame pin pass must not undo the swing. Built on Fable's 192/193 replay
// (tmp/replay_nudge_p2_then_pins.ts). Khoa removed that knot from the document, so
// the check adds it back itself. Read-only: never writes the document.
// Run: npx tsx tmp/smooth_knot_swing_check.ts
import { readFileSync } from "node:fs";
import { SmoothKnot, TabletDocument, enforce_smooth_knot, stroke_by_id, stroke_control_points, vertex_position } from "../src/document";
import { update_pinned_vertex_positions } from "../src/patch";
import { begin_edit_state, edit_nudge_handle } from "../src/edit_mode";
import { V3, v3_dot, v3_length, v3_normalize, v3_sub } from "../src/math";

const file = JSON.parse(readFileSync("documents/skull-zanatomy.json", "utf-8"));
const camera = { pivot: { x: 0.546871071829559, y: 0.056329128325497566, z: 0.03904602003772208 }, yaw: 2.007181732177727, pitch: 0.15645483398437582, distance: 1.4535239885261702 };
const canvas = { clientWidth: 1384, clientHeight: 811 } as HTMLCanvasElement;
const KNOT: SmoothKnot = { vertex: 174, stroke_a: 192, stroke_b: 193 };

let failures = 0;
function expect(label: string, ok: boolean, detail: string): void {
  if (!ok) failures++;
  console.log(`${ok ? "ok  " : "FAIL"} ${label}: ${detail}`);
}

// Angle in degrees between 192's handle at 174 and the reverse of 193's: 0 = smooth.
function kink_degrees(d: TabletDocument): number {
  const at = vertex_position(d, 174);
  const out_192 = v3_normalize(v3_sub(stroke_control_points(stroke_by_id(d, 192), d).p2, at));
  const out_193 = v3_normalize(v3_sub(stroke_control_points(stroke_by_id(d, 193), d).p1, at));
  return Math.acos(Math.max(-1, Math.min(1, -v3_dot(out_192, out_193)))) * 180 / Math.PI;
}
const distance = (a: V3, b: V3): number => v3_length(v3_sub(a, b));

// The document with the knot back and smooth, and the pins settled once.
function smooth_document(): TabletDocument {
  const d: TabletDocument = structuredClone(file.document);
  d.smooth_knots.push({ ...KNOT });
  enforce_smooth_knot(d, d.smooth_knots[d.smooth_knots.length - 1], 192);
  update_pinned_vertex_positions(d);
  return d;
}
function swing_nudge_193_p2(d: TabletDocument): void {
  const state = begin_edit_state(193);
  state.selected_handle = "p2";
  edit_nudge_handle(state, d, camera, canvas, "p2", { x: -20, y: 0 }, 0, "swing");
}

{
  const d = smooth_document();
  expect("setup: knot smooth", kink_degrees(d) < 1e-4, `${kink_degrees(d).toFixed(6)}°`);
}

// A: 172 is pinned on stroke 79 (as in the file). The nudge keeps the knot smooth,
// and the pin pass after it changes neither the knot nor 193's far handle.
{
  const d = smooth_document();
  const before_p2 = stroke_control_points(stroke_by_id(d, 193), d).p2;
  swing_nudge_193_p2(d);
  const nudged_p2 = stroke_control_points(stroke_by_id(d, 193), d).p2;
  expect("A: nudge moved 193's p2", distance(nudged_p2, before_p2) > 1e-4, `${distance(nudged_p2, before_p2).toFixed(6)}`);
  expect("A: knot smooth after nudge", kink_degrees(d) < 1e-4, `${kink_degrees(d).toFixed(6)}°`);
  update_pinned_vertex_positions(d);
  const after_p2 = stroke_control_points(stroke_by_id(d, 193), d).p2;
  expect("A: pin pass keeps 193's p2", distance(after_p2, nudged_p2) < 1e-9, `moved ${distance(after_p2, nudged_p2).toExponential(2)}`);
  expect("A: knot smooth after pin pass", kink_degrees(d) < 1e-4, `${kink_degrees(d).toFixed(6)}°`);
}

// B: no pin anywhere near (172's pin dropped). The knot is smooth right after the nudge.
{
  const d = smooth_document();
  d.vertex_pins = d.vertex_pins.filter((pin) => pin.vertex !== 172);
  swing_nudge_193_p2(d);
  expect("B: knot smooth after nudge, no pin", kink_degrees(d) < 1e-4, `${kink_degrees(d).toFixed(6)}°`);
}

// C (Q3): the pin pass leaves an unmoved pin alone, so a kink it did not cause stays.
{
  const d = smooth_document();
  const s193 = stroke_by_id(d, 193);
  s193.d0 = { x: s193.d0.x + 0.02, y: s193.d0.y, z: s193.d0.z };
  const kinked = kink_degrees(d);
  const kinked_d0 = { ...s193.d0 };
  update_pinned_vertex_positions(d);
  expect("C: unmoved pin does not re-aim the knot", distance(s193.d0, kinked_d0) < 1e-12 && kinked > 0.1,
    `kink ${kinked.toFixed(3)}° -> ${kink_degrees(d).toFixed(3)}°`);
}

console.log(failures === 0 ? "all passed" : `${failures} failed`);
process.exit(failures === 0 ? 0 : 1);
