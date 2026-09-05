// One-off check: middle-button drag pans the camera and never reaches the pen handlers.
import { attach_gestures } from "../src/gestures";
import { default_camera, camera_world_units_per_pixel } from "../src/camera";
const listeners = new Map<string, (e: any) => void>();
const canvas = { addEventListener: (t: string, f: any) => listeners.set(t, f), setPointerCapture() {}, clientWidth: 1280, clientHeight: 720 } as any;
const camera = default_camera();
const pen_calls: string[] = [];
attach_gestures(canvas, camera, { on_pen_down: () => pen_calls.push("down"), on_pen_move: () => pen_calls.push("move"), on_pen_up: () => pen_calls.push("up"), on_undo_tap() {}, on_redo_tap() {} }, () => {});
const ev = (x: number, y: number, altKey: boolean, buttons = 1, button = 0) => ({ pointerType: "mouse", pointerId: 1, clientX: x, clientY: y, altKey, buttons, button, preventDefault() {} });
const before = { ...camera.pivot };
listeners.get("pointerdown")!(ev(100, 100, false, 4, 1));
listeners.get("pointermove")!(ev(200, 100, false)); // button field only matters at down: still panning
listeners.get("pointerup")!(ev(200, 100, false));
const moved = Math.hypot(camera.pivot.x - before.x, camera.pivot.y - before.y, camera.pivot.z - before.z);
console.log("pivot moved by", moved.toFixed(4), "(expect", (100 * camera_world_units_per_pixel(camera, 720)).toFixed(4) + ") pen handler calls:", pen_calls, "(expect none)");
listeners.get("pointerdown")!(ev(100, 100, false)); listeners.get("pointermove")!(ev(120, 100, false)); listeners.get("pointerup")!(ev(120, 100, false));
console.log("plain drag pen calls:", pen_calls, "(expect down, move, up)");
