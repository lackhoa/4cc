// One-off check of eased wheel zoom with a fake canvas + manual rAF pump (node).
import { attach_gestures } from "../src/gestures";
import { default_camera } from "../src/camera";

const listeners = new Map<string, (e: any) => void>();
const canvas = { addEventListener: (type: string, fn: any) => listeners.set(type, fn), clientWidth: 1280, clientHeight: 720 } as any;
let frames: (() => void)[] = [];
(globalThis as any).requestAnimationFrame = (fn: () => void) => { frames.push(fn); return 0; };
const camera = default_camera();
let change_count = 0;
attach_gestures(canvas, camera, { on_pen_down() {}, on_pen_move() {}, on_pen_up() {}, on_undo_tap() {}, on_redo_tap() {} }, () => change_count++);
const wheel = (deltaY: number) => listeners.get("wheel")!({ deltaY, preventDefault() {} });
const pump = (max: number) => { let n = 0; while (frames.length && n < max) { const f = frames.shift()!; f(); n++; } return n; };

wheel(100); wheel(100); wheel(100); // 3 notches in one burst
const trace = [camera.distance];
for (let i = 0; i < 6; i++) { pump(1); trace.push(+camera.distance.toFixed(3)); }
const frames_to_settle = 6 + pump(200);
console.log("distance per frame:", trace.join(" -> "), "... settles at", camera.distance.toFixed(4), "after", frames_to_settle, "frames (expect 8*e^0.45 =", (8 * Math.exp(0.45)).toFixed(4) + ")");
wheel(-300); pump(200);
console.log("zoom back in ->", camera.distance.toFixed(4), "(expect 8)");
for (let i = 0; i < 100; i++) wheel(100);
pump(500);
console.log("clamped far ->", camera.distance, "(expect 60), on_camera_change calls:", change_count);
