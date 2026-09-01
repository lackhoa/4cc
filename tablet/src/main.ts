// autodraw tablet — iPad drawing companion prototype.
// Sketchpad rework (plan step 7): strokes are single cubics put down with the
// armed line tool and shaped afterwards; endpoints live in a shared vertex
// table so joined strokes can never tear. Bare pen drags orbit (Q27/Q35); a
// drag starting on the selected stroke translates it; vertex/handle drags
// reshape. Finger = camera throughout (1-finger orbit, 2-finger pan/zoom).

import { CameraSnapState, camera_basis, camera_orbit, camera_snap_to_axis_view, camera_view_projection, camera_world_units_per_pixel, default_camera } from "./camera";
import { delete_stroke, empty_document, stroke_control_points } from "./document";
import { EditState, TAP_MAX_MOVEMENT_PIXELS, begin_edit_state, edit_pen_down, edit_pen_move, edit_pen_up, find_merge_target_vertex, pick_stroke } from "./edit_mode";
import { begin_history_step, clear_history, create_history_state, end_history_step, redo, undo } from "./history";
import { ORBIT_RADIANS_PER_PIXEL, attach_gestures } from "./gestures";
import { LineToolState, line_pen_down, line_pen_move, line_pen_up } from "./line_tool";
import { merge_adjacent_strokes } from "./stroke_merge";
import { append_coons_mesh } from "./coons";
import { append_loft_mesh } from "./loft";
import { V2, V3, v3_add, v3_scale, v3_sub } from "./math";
import { append_inflate_mesh } from "./inflate";
import { append_revolve_mesh } from "./revolve";
import { append_stroke_ribbon } from "./ribbon";
import { ReferenceMesh, append_reference_mesh, fetch_reference_mesh } from "./reference";
import { clear_document_in_place, create_persistence_state, list_documents_from_server, load_current_document_on_startup, schedule_autosave, switch_document } from "./persistence";
import { create_line_renderer, render_frame, set_overlay_lines, set_overlay_triangles, set_preview_line, set_reference_mesh, set_stroke_mesh, set_surface_mesh } from "./render";

const STROKE_COLOR = { r: 0.85, g: 0.85, b: 0.9 };
const HIGHLIGHT_COLOR = { r: 1.0, g: 0.65, b: 0.2 };
const PREVIEW_COLOR = { r: 0.6, g: 0.75, b: 1.0 };
const ANCHOR_COLOR = { r: 1.0, g: 1.0, b: 1.0 };
const HANDLE_COLOR = { r: 0.45, g: 0.8, b: 1.0 };
const HANDLE_LINE_COLOR = { r: 0.5, g: 0.5, b: 0.55 };
const SURFACE_COLOR = { r: 0.45, g: 0.55, b: 0.7 };
const ANCHOR_SIZE_PIXELS = 12;
const HANDLE_SIZE_PIXELS = 9;

const canvas = document.getElementById("canvas") as HTMLCanvasElement;
const gl = canvas.getContext("webgl");
if (!gl) {
  document.body.textContent = "WebGL not available";
  throw new Error("WebGL not available");
}

const camera = default_camera();
const tablet_document = empty_document();
const renderer = create_line_renderer(gl);
const persistence = create_persistence_state();
const history = create_history_state();
let reference_mesh: ReferenceMesh | null = null;
let reference_visible = true;
let edit_state: EditState | null = null; // non-null = a stroke is selected
// Armed tools: "line" creates strokes; the pick tools make the next tapped
// stroke the loft's second rail ("loft"), the selected inflate's cross-section
// curve ("profile"), one of the Coons patch's remaining boundary sides
// ("patch", collects until 4), or the adjacent stroke to merge the selection
// with ("join").
type ArmedTool = "line" | "loft" | "profile" | "patch" | "join";
let armed_tool: ArmedTool | null = null;
const patch_picks: number[] = []; // stroke indices collected while "patch" is armed
let line_state: LineToolState | null = null; // non-null while the line tool's pen is down
let pen_orbit_last_screen: V2 | null = null; // non-null while a bare pen drag orbits
let frame_requested = false;
// Pen tap detection (tap = select/deselect instead of moving anything).
// Displacement from the down point, not path length — pencil taps jitter.
let pen_down_screen: V2 | null = null;
let pen_max_displacement_pixels = 0;

function request_render(): void {
  // Anything that changes what's on screen (strokes, surfaces, camera) goes
  // through here — piggyback the debounced autosave on it; identical
  // serializations are skipped inside. Before the early return: a pending
  // frame must not swallow the save (rAF pauses entirely in hidden tabs).
  schedule_autosave(persistence, tablet_document, camera);
  if (frame_requested) return;
  frame_requested = true;
  requestAnimationFrame(() => {
    frame_requested = false;
    // Ribbons are camera-facing (desktop parity) — retessellate every frame.
    rebuild_stroke_mesh(edit_state === null ? null : edit_state.stroke_index);
    rebuild_surface_mesh();
    rebuild_reference_mesh();
    rebuild_edit_overlay();
    render_frame(renderer, camera_view_projection(camera, canvas.width / canvas.height));
  });
}

function resize_canvas_to_display(): void {
  const dpr = window.devicePixelRatio;
  canvas.width = Math.round(canvas.clientWidth * dpr);
  canvas.height = Math.round(canvas.clientHeight * dpr);
  gl!.viewport(0, 0, canvas.width, canvas.height);
  request_render();
}
window.addEventListener("resize", resize_canvas_to_display);

function rebuild_stroke_mesh(highlighted_index: number | null): void {
  const vertices: number[] = [];
  for (let i = 0; i < tablet_document.strokes.length; i++) {
    const color = i === highlighted_index ? HIGHLIGHT_COLOR : STROKE_COLOR;
    append_stroke_ribbon(tablet_document.strokes[i], tablet_document, camera, color, vertices);
  }
  set_stroke_mesh(renderer, new Float32Array(vertices));
}

function rebuild_surface_mesh(): void {
  const vertices: number[] = [];
  for (const loft of tablet_document.lofts) {
    append_loft_mesh(loft, tablet_document, camera, SURFACE_COLOR, vertices);
  }
  for (const revolve of tablet_document.revolves) {
    append_revolve_mesh(revolve, tablet_document, camera, SURFACE_COLOR, vertices);
  }
  for (const inflate of tablet_document.inflates) {
    append_inflate_mesh(inflate, tablet_document, camera, SURFACE_COLOR, vertices);
  }
  for (const coons of tablet_document.coons) {
    append_coons_mesh(coons, tablet_document, camera, SURFACE_COLOR, vertices);
  }
  set_surface_mesh(renderer, new Float32Array(vertices));
}

// Camera-facing square marker, two triangles.
function append_billboard_square(
  center: V3, half_size: number, right: V3, up: V3,
  color: { r: number; g: number; b: number }, out: number[],
): void {
  const right_half = v3_scale(right, half_size);
  const up_half = v3_scale(up, half_size);
  const corner_a = v3_sub(v3_sub(center, right_half), up_half);
  const corner_b = v3_sub(v3_add(center, right_half), up_half);
  const corner_c = v3_add(v3_add(center, right_half), up_half);
  const corner_d = v3_add(v3_sub(center, right_half), up_half);
  for (const corner of [corner_a, corner_b, corner_c, corner_a, corner_c, corner_d]) {
    out.push(corner.x, corner.y, corner.z, color.r, color.g, color.b);
  }
}

// Headlight shading is camera-dependent — rebuilt per frame like the surfaces.
function rebuild_reference_mesh(): void {
  if (reference_mesh === null || !reference_visible) {
    set_reference_mesh(renderer, new Float32Array(0));
    return;
  }
  const vertices: number[] = [];
  append_reference_mesh(reference_mesh, camera, vertices);
  set_reference_mesh(renderer, new Float32Array(vertices));
}

function rebuild_edit_overlay(): void {
  if (edit_state === null) {
    set_overlay_lines(renderer, new Float32Array(0));
    set_overlay_triangles(renderer, new Float32Array(0));
    return;
  }
  const points = stroke_control_points(tablet_document.strokes[edit_state.stroke_index], tablet_document);
  const basis = camera_basis(camera);
  const units_per_pixel = camera_world_units_per_pixel(camera, canvas.clientHeight);
  const anchor_half = (ANCHOR_SIZE_PIXELS / 2) * units_per_pixel;
  const handle_half = (HANDLE_SIZE_PIXELS / 2) * units_per_pixel;

  const line_vertices: number[] = [];
  const triangle_vertices: number[] = [];
  const push_line = (a: V3, b: V3) => {
    line_vertices.push(a.x, a.y, a.z, HANDLE_LINE_COLOR.r, HANDLE_LINE_COLOR.g, HANDLE_LINE_COLOR.b);
    line_vertices.push(b.x, b.y, b.z, HANDLE_LINE_COLOR.r, HANDLE_LINE_COLOR.g, HANDLE_LINE_COLOR.b);
  };
  push_line(points.p0, points.p1);
  push_line(points.p3, points.p2);
  append_billboard_square(points.p1, handle_half, basis.right, basis.up, HANDLE_COLOR, triangle_vertices);
  append_billboard_square(points.p2, handle_half, basis.right, basis.up, HANDLE_COLOR, triangle_vertices);
  append_billboard_square(points.p0, anchor_half, basis.right, basis.up, ANCHOR_COLOR, triangle_vertices);
  append_billboard_square(points.p3, anchor_half, basis.right, basis.up, ANCHOR_COLOR, triangle_vertices);
  // Drag-time snap warning (Q3): while a vertex is being dragged, mark the
  // vertex it would weld into on release so the merge is never a surprise.
  if (edit_state.dragging === "p0" || edit_state.dragging === "p3") {
    const stroke = tablet_document.strokes[edit_state.stroke_index];
    const dragged_vertex = edit_state.dragging === "p0" ? stroke.p0_vertex : stroke.p3_vertex;
    const target_vertex = find_merge_target_vertex(tablet_document, camera, canvas, dragged_vertex);
    if (target_vertex !== null) {
      append_billboard_square(
        tablet_document.vertices[target_vertex], anchor_half * 2, basis.right, basis.up,
        HIGHLIGHT_COLOR, triangle_vertices,
      );
    }
  }
  set_overlay_lines(renderer, new Float32Array(line_vertices));
  set_overlay_triangles(renderer, new Float32Array(triangle_vertices));
}

function update_preview_line(): void {
  if (line_state === null) {
    set_preview_line(renderer, new Float32Array(0));
    return;
  }
  // The freehand path, plus the snapped end point so snapping is visible.
  const vertices: number[] = [];
  for (const point of [line_state.start_world, ...line_state.path_world, line_state.end_world]) {
    vertices.push(point.x, point.y, point.z, PREVIEW_COLOR.r, PREVIEW_COLOR.g, PREVIEW_COLOR.b);
  }
  set_preview_line(renderer, new Float32Array(vertices));
}

// Line-tool pen-up: a drag commits a stroke fitted to the pen path and
// auto-selects it; a tap exits the tool (Q27). Either way the tool disarms, so
// the very next drag adjusts the fresh stroke instead of creating another.
function line_mode_pen_up(): void {
  const was_tap = pen_max_displacement_pixels < TAP_MAX_MOVEMENT_PIXELS;
  if (!was_tap && line_state !== null) {
    const stroke_index = line_pen_up(line_state, tablet_document);
    if (stroke_index !== null) edit_state = begin_edit_state(stroke_index);
  }
  set_armed_tool(null); // also clears line_state
  update_preview_line();
}

// Selected-stroke pen-up: a tap on another stroke switches the selection (or,
// with a pick tool armed, feeds it); a tap on empty space deselects. Drags
// (control point, whole-stroke move, or orbit) just end.
function edit_mode_pen_up(position: V2): void {
  if (edit_state === null) return;
  const was_control_drag = edit_state.dragging !== null || edit_state.moving_whole_stroke;
  edit_pen_up(edit_state, tablet_document, camera, canvas);
  const was_tap = pen_max_displacement_pixels < TAP_MAX_MOVEMENT_PIXELS;
  if (!was_tap || was_control_drag) return;
  const picked = pick_stroke(tablet_document, camera, position, canvas);
  if (armed_tool === "patch") {
    if (picked === null) {
      set_armed_tool(null); // tap empty = cancel, selection kept
      return;
    }
    if (!patch_picks.includes(picked)) patch_picks.push(picked);
    if (patch_picks.length === 4) {
      tablet_document.coons.push({ strokes: [patch_picks[0], patch_picks[1], patch_picks[2], patch_picks[3]] });
      set_armed_tool(null);
      edit_state = null;
    }
    return;
  }
  if (armed_tool !== null) {
    if (picked !== null && picked !== edit_state.stroke_index) {
      if (armed_tool === "loft") {
        tablet_document.lofts.push({ stroke_a: edit_state.stroke_index, stroke_b: picked });
        edit_state = null;
      } else if (armed_tool === "join") {
        const merged_index = merge_adjacent_strokes(tablet_document, edit_state.stroke_index, picked);
        // Not adjacent (or a closed loop): keep the selection, just disarm.
        if (merged_index !== null) edit_state = begin_edit_state(merged_index);
      } else {
        assign_inflate_profile(edit_state.stroke_index, picked);
      }
    }
    set_armed_tool(null); // tap empty (or the same stroke) = cancel, selection kept
    return;
  }
  edit_state = picked === null ? null : begin_edit_state(picked);
}

// Attach a cross-section profile curve to the silhouette's inflate, creating
// the inflate on the spot if the silhouette doesn't have one yet (so circle A
// + curve B needs no separate inflate tap). Selection stays on the silhouette.
function assign_inflate_profile(silhouette_index: number, profile_index: number): void {
  for (let i = tablet_document.inflates.length - 1; i >= 0; i--) {
    if (tablet_document.inflates[i].stroke === silhouette_index) {
      tablet_document.inflates[i].profile = profile_index;
      return;
    }
  }
  tablet_document.inflates.push({ stroke: silhouette_index, profile: profile_index });
}

const line_button = document.getElementById("line_button") as HTMLButtonElement;
const surface_button = document.getElementById("surface_button") as HTMLButtonElement;
const profile_button = document.getElementById("profile_button") as HTMLButtonElement;
const patch_button = document.getElementById("patch_button") as HTMLButtonElement;
const join_button = document.getElementById("join_button") as HTMLButtonElement;
function set_armed_tool(tool: ArmedTool | null): void {
  armed_tool = tool;
  if (tool !== "patch") patch_picks.length = 0;
  if (tool !== "line") line_state = null;
  line_button.classList.toggle("armed", tool === "line");
  surface_button.classList.toggle("armed", tool === "loft");
  profile_button.classList.toggle("armed", tool === "loft" ? false : tool === "profile");
  patch_button.classList.toggle("armed", tool === "patch");
  join_button.classList.toggle("armed", tool === "join");
}
line_button.addEventListener("click", () => {
  set_armed_tool(armed_tool === "line" ? null : "line");
});
surface_button.addEventListener("click", () => {
  if (edit_state === null) return; // needs a selected first rail
  set_armed_tool(armed_tool === "loft" ? null : "loft");
});
profile_button.addEventListener("click", () => {
  if (edit_state === null) return; // needs a selected silhouette
  set_armed_tool(armed_tool === "profile" ? null : "profile");
});
// Patch: the selected stroke is the first boundary side; the next three taps
// pick the rest (any order — sides are chained by endpoint proximity).
patch_button.addEventListener("click", () => {
  if (edit_state === null) return;
  if (armed_tool === "patch") {
    set_armed_tool(null);
    return;
  }
  set_armed_tool("patch");
  patch_picks.push(edit_state.stroke_index);
});

// Join: the next tapped stroke merges with the selection into one cubic
// (they must share a vertex).
join_button.addEventListener("click", () => {
  if (edit_state === null) return; // needs a selected first stroke
  set_armed_tool(armed_tool === "join" ? null : "join");
});

// Revolve acts immediately on the selected stroke (the axis is the line through
// its own endpoints) — selection is kept so the profile can be tweaked live.
const revolve_button = document.getElementById("revolve_button") as HTMLButtonElement;
revolve_button.addEventListener("click", () => {
  if (edit_state === null) return;
  begin_history_step(history, tablet_document);
  tablet_document.revolves.push({ stroke: edit_state.stroke_index });
  end_history_step(history, tablet_document);
  request_render();
});

// Inflate acts immediately on the selected stroke (its silhouette becomes a
// pillow); selection kept so the outline can be tweaked live.
const inflate_button = document.getElementById("inflate_button") as HTMLButtonElement;
inflate_button.addEventListener("click", () => {
  if (edit_state === null) return;
  begin_history_step(history, tablet_document);
  tablet_document.inflates.push({ stroke: edit_state.stroke_index, profile: null });
  end_history_step(history, tablet_document);
  request_render();
});

function pen_orbit(position: V2): void {
  if (pen_orbit_last_screen === null) return;
  camera_orbit(
    camera,
    -(position.x - pen_orbit_last_screen.x) * ORBIT_RADIANS_PER_PIXEL,
    (position.y - pen_orbit_last_screen.y) * ORBIT_RADIANS_PER_PIXEL,
  );
  pen_orbit_last_screen = position;
}

// Undo/redo (plan-tablet-undo-redo.md): restore drops the selection — the
// selected stroke index may not survive the snapshot.
function perform_undo(): void {
  if (!undo(history, tablet_document)) return;
  edit_state = null;
  set_armed_tool(null);
  request_render();
}
function perform_redo(): void {
  if (!redo(history, tablet_document)) return;
  edit_state = null;
  set_armed_tool(null);
  request_render();
}

attach_gestures(canvas, camera, {
  on_pen_down: (position) => {
    begin_history_step(history, tablet_document);
    pen_down_screen = position;
    pen_max_displacement_pixels = 0;
    pen_orbit_last_screen = null;
    if (armed_tool === "line") {
      line_state = line_pen_down(tablet_document, camera, position, canvas);
      update_preview_line();
    } else if (edit_state !== null && armed_tool === null) {
      // Consumed only when the pen lands on the selection; otherwise orbit.
      if (!edit_pen_down(edit_state, tablet_document, camera, position, canvas)) {
        pen_orbit_last_screen = position;
      }
    } else {
      // No selection, or a pick tool armed (pure tap tool): bare drags orbit.
      pen_orbit_last_screen = position;
    }
    request_render();
  },
  on_pen_move: (position) => {
    if (pen_down_screen !== null) {
      pen_max_displacement_pixels = Math.max(
        pen_max_displacement_pixels,
        Math.hypot(position.x - pen_down_screen.x, position.y - pen_down_screen.y),
      );
    }
    if (armed_tool === "line") {
      if (line_state !== null) {
        line_pen_move(line_state, tablet_document, camera, position, canvas);
        update_preview_line();
      }
    } else if (pen_orbit_last_screen !== null) {
      pen_orbit(position);
    } else if (edit_state !== null) {
      edit_pen_move(edit_state, tablet_document, camera, position, canvas);
    }
    request_render();
  },
  on_pen_up: (position) => {
    if (armed_tool === "line") {
      line_mode_pen_up();
    } else if (edit_state !== null) {
      edit_mode_pen_up(position);
    } else if (pen_max_displacement_pixels < TAP_MAX_MOVEMENT_PIXELS) {
      const picked = pick_stroke(tablet_document, camera, position, canvas);
      if (picked !== null) edit_state = begin_edit_state(picked);
    }
    pen_down_screen = null;
    pen_orbit_last_screen = null;
    end_history_step(history, tablet_document);
    request_render();
  },
  on_undo_tap: perform_undo,
  on_redo_tap: perform_redo,
}, () => {
  request_render();
});

// Delete the selected stroke (surfaces built on it go with it).
const delete_button = document.getElementById("delete_button") as HTMLButtonElement;
delete_button.addEventListener("click", () => {
  if (edit_state === null) return;
  begin_history_step(history, tablet_document);
  delete_stroke(tablet_document, edit_state.stroke_index);
  end_history_step(history, tablet_document);
  edit_state = null;
  set_armed_tool(null);
  request_render();
});

const clear_button = document.getElementById("clear_button") as HTMLButtonElement;
clear_button.addEventListener("click", () => {
  if (tablet_document.strokes.length === 0) return;
  if (!window.confirm("Erase all strokes?")) return;
  begin_history_step(history, tablet_document);
  clear_document_in_place(tablet_document);
  end_history_step(history, tablet_document);
  edit_state = null;
  set_armed_tool(null);
  request_render();
});

// Snap the camera to the nearest frontal/profile/back view (desktop key A);
// snapping again toggles back to the previous view. previous starts at
// profile so the very first snap from frontal has somewhere to toggle to.
const camera_snap_state: CameraSnapState = { previous_snap_yaw: Math.PI / 2, current_snap_yaw: 0 };
const view_button = document.getElementById("view_button") as HTMLButtonElement;
view_button.addEventListener("click", () => {
  camera_snap_to_axis_view(camera, camera_snap_state);
  request_render();
});
window.addEventListener("keydown", (event) => {
  if (event.key === "a" && !event.repeat && !event.ctrlKey && !event.metaKey) {
    camera_snap_to_axis_view(camera, camera_snap_state);
    request_render();
  }
});

// Undo/redo buttons + desktop shortcuts (the iPad path is the two/three-finger
// tap in gestures.ts).
const undo_button = document.getElementById("undo_button") as HTMLButtonElement;
const redo_button = document.getElementById("redo_button") as HTMLButtonElement;
undo_button.addEventListener("click", perform_undo);
redo_button.addEventListener("click", perform_redo);
window.addEventListener("keydown", (event) => {
  if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === "z") {
    event.preventDefault();
    if (event.shiftKey) perform_redo();
    else perform_undo();
  }
});

const reference_button = document.getElementById("reference_button") as HTMLButtonElement;
reference_button.addEventListener("click", () => {
  reference_visible = !reference_visible;
  reference_button.classList.toggle("armed", reference_visible);
  request_render();
});
reference_button.classList.toggle("armed", reference_visible);

// Docs panel: lists server documents to switch between, plus "new…" (prompt
// for a name; unknown names start empty). Autosave keeps targeting whichever
// document is current.
const docs_button = document.getElementById("docs_button") as HTMLButtonElement;
const docs_panel = document.getElementById("docs_panel") as HTMLDivElement;

async function switch_to_document_and_rerender(name: string): Promise<void> {
  docs_panel.classList.remove("open");
  edit_state = null;
  set_armed_tool(null);
  clear_history(history); // history is per-document (Q5)
  await switch_document(persistence, tablet_document, camera, name);
  request_render();
}

async function open_docs_panel(): Promise<void> {
  const entries = await list_documents_from_server();
  docs_panel.replaceChildren();
  if (entries === null) {
    const note = document.createElement("button");
    note.textContent = "server unreachable";
    note.disabled = true;
    docs_panel.appendChild(note);
  } else {
    const names = entries.map((entry) => entry.name);
    if (!names.includes(persistence.current_document_name)) names.push(persistence.current_document_name);
    for (const name of names.sort()) {
      const entry_button = document.createElement("button");
      entry_button.textContent = name;
      entry_button.classList.toggle("current", name === persistence.current_document_name);
      entry_button.addEventListener("click", () => void switch_to_document_and_rerender(name));
      docs_panel.appendChild(entry_button);
    }
    const new_button = document.createElement("button");
    new_button.textContent = "new…";
    new_button.addEventListener("click", () => {
      const name = window.prompt("Document name (letters, digits, - and _):");
      if (name === null) return;
      if (!/^[A-Za-z0-9_-]{1,64}$/.test(name)) {
        window.alert("Bad name — letters, digits, - and _ only.");
        return;
      }
      void switch_to_document_and_rerender(name);
    });
    docs_panel.appendChild(new_button);
  }
  docs_panel.classList.add("open");
}

docs_button.addEventListener("click", () => {
  if (docs_panel.classList.contains("open")) {
    docs_panel.classList.remove("open");
  } else {
    void open_docs_panel();
  }
});

resize_canvas_to_display();
void load_current_document_on_startup(persistence, tablet_document, camera).then(() => {
  request_render();
});
void fetch_reference_mesh("/reference/skull.obj").then((mesh) => {
  reference_mesh = mesh;
  request_render();
});
// Debug hook: inspect the document from the browser console / automated tests.
(window as unknown as { tablet_document: unknown }).tablet_document = tablet_document;
(window as unknown as { debug_camera: unknown }).debug_camera = camera;
(window as unknown as { debug_persistence: unknown }).debug_persistence = persistence;
console.log("autodraw tablet: Sketchpad rework — line tool + vertex-connected single-cubic strokes");
