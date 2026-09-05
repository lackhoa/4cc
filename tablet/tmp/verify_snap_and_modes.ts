// One-off checks: world-space snapping + dial/swing handle modes (node, no DOM).
import { pick_vertex_near_world_point, stroke_by_id, stroke_control_points, vertex_by_id, vertex_position } from "../src/document";
import { begin_edit_state, edit_pen_down, edit_pen_move, edit_pen_up, find_merge_target_vertex } from "../src/edit_mode";
import { v3, v3_cross, v3_dot, v3_length, v3_sub } from "../src/math";
import { readFileSync } from "node:fs";

const file = JSON.parse(readFileSync("documents/test-doc.json", "utf-8"));
const doc = file.document;
const camera = file.camera;
const canvas = { clientWidth: 1280, clientHeight: 720 } as HTMLCanvasElement;

// 1. snapping: vertex 0 vs a point 0.03 away -> hit; 0.08 away -> miss
// ids: vertex/stroke ids equal the old indices in this file (v3 -> v4 migration)
const v0 = vertex_position(doc, 0);
const near = v3(v0.x + 0.03, v0.y, v0.z);
const far = v3(v0.x + 0.08, v0.y, v0.z);
console.log("snap near ->", pick_vertex_near_world_point(doc, near, null), "(expect 0)");
console.log("snap far  ->", pick_vertex_near_world_point(doc, far, null), "(expect null)");
console.log("snap exclude self ->", pick_vertex_near_world_point(doc, v0, 0), "(expect null)");
// merge target while vertices 1 and 3 are 2+ units apart -> none
console.log("merge target v3 ->", find_merge_target_vertex(doc, 3), "(expect null)");
vertex_by_id(doc, 3).position = { ...vertex_position(doc, 1), x: vertex_position(doc, 1).x + 0.02 };
console.log("merge target v3 moved next to v1 ->", find_merge_target_vertex(doc, 3), "(expect 1)");

// 2. dial: pen-down anywhere consumed, drag rolls handles rigidly about chord
const st = begin_edit_state(0);
console.log("dial pen_down consumed ->", edit_pen_down(st, doc, camera, { x: 100, y: 100 }, canvas, "dial"));
const before = stroke_control_points(stroke_by_id(doc, 0), doc);
edit_pen_move(st, doc, camera, { x: 200, y: 100 }, canvas, "dial");
const after = stroke_control_points(stroke_by_id(doc, 0), doc);
const chord = v3_sub(before.p3, before.p0);
const angle = (a: any, b: any) => Math.acos(v3_dot(a, b) / (v3_length(a) * v3_length(b))) * 180 / Math.PI;
const perp = (p: any) => v3_cross(chord, v3_sub(p, before.p0));
console.log("dial p1 roll deg ->", angle(perp(before.p1), perp(after.p1)).toFixed(2), "p2 ->", angle(perp(before.p2), perp(after.p2)).toFixed(2), "(expect 57.30 both)");
edit_pen_up(st, doc);

// 3. swing: dragging p1 must not be consumed off-stroke, and after a real drag d0/d3/chord stay coplanar
console.log("swing pen_down off-stroke consumed ->", edit_pen_down(st, doc, camera, { x: 5, y: 5 }, canvas, "swing"), "(expect false)");
st.dragging = "p1"; st.last_screen = { x: 600, y: 300 };
edit_pen_move(st, doc, camera, { x: 640, y: 330 }, canvas, "swing");
const s = stroke_by_id(doc, 0);
const c = v3_sub(vertex_position(doc, s.p3_vertex), vertex_position(doc, s.p0_vertex));
console.log("swing coplanarity ->", v3_dot(v3_cross(c, s.d0), s.d3).toExponential(2), "(expect ~0)");
