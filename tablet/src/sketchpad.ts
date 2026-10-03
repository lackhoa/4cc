// autodraw tablet — iPad drawing companion prototype.
// Sketchpad rework (plan step 7): strokes are single cubics put down with the
// armed line tool and shaped afterwards; endpoints live in a shared vertex
// table so joined strokes can never tear. Bare pen drags orbit (Q27/Q35), also
// when they start on the selected stroke; vertex/handle drags reshape.
// Finger = camera throughout (1-finger orbit, 2-finger pan/zoom).
//
// The whole editor is `start_sketchpad(setup)`: one page calls it with its own
// reference mesh, document name and localStorage keys (plan-sketchpad-landmarks.md
// Q66/Q67) — `draw/main.ts` is the plain sketchpad, `pages/skull-draw/main.ts` the
// same editor over the Z-Anatomy skull. The page's HTML supplies the canvas and
// the toolbar buttons by id.

import { CameraSnapState, OrbitCamera, camera_basis, camera_eye, camera_orbit, camera_orthographic_view_projection, camera_pen_ray, camera_snap_to_axis_view, camera_view_projection, camera_world_to_screen, camera_world_units_per_pixel, default_camera } from "./camera";
import { ALL_LAYERS, DEFAULT_STROKE_RADII, Layer, SKULL_BONE_ID, StrokeId, StrokeRadii, VertexId, VertexPin, add_straight_stroke, add_vertex,bezier_point, copy_layer_strokes, delete_stroke, pin_vertex_to_stroke, vertex_can_pin_to_stroke, detach_stroke_end_from_weld, stroke_end_can_detach_from_weld, empty_document, enforce_midline, find_snap_target_stroke, garbage_collect_vertices, move_vertex, patch_layer, pin_by_vertex, pins_on_stroke, smooth_knot_between_strokes, smooth_knots_at_vertex, smooth_strokes, split_stroke, straighten_strokes, stroke_by_id, stroke_control_points, stroke_radii, unsmooth_strokes, update_pinned_vertex_positions, vertex_by_id, vertex_is_on_layers, vertex_is_on_midline, vertex_position, vertex_world_position } from "./document";
import { CONTROL_POINT_PICK_RADIUS_PIXELS, EditState, HandleMode, STROKE_PICK_RADIUS_PIXELS, TAP_MAX_MOVEMENT_PIXELS, begin_edit_state, camera_plane_drag, edit_nudge_handle, edit_pen_down, edit_pen_move, edit_pen_up, find_merge_target_vertex, merge_vertex_if_near_another, nearest_t_on_stroke_screen, pick_stroke, pick_stroke_point, pick_vertex } from "./edit_mode";
import { begin_history_step, clear_history, create_history_state, end_history_step, jump_history, redo, undo } from "./history";
import { ORBIT_RADIANS_PER_PIXEL, attach_gestures } from "./gestures";
import { LineToolState, line_pen_down, line_pen_move, line_pen_up } from "./line_tool";
import { merge_adjacent_strokes } from "./stroke_merge";
import { append_patch_mesh, drop_unused_patch_strokes, patch_surface_grid, pick_patch, pin_is_locked, stroke_bounds_a_patch } from "./patch";
import { extract_contour_chains } from "./contour";
import { describe_selection } from "./selection_readout";
import { mirror_camera_from_main_camera, mirror_rectangle } from "./mirror_view";
import { V2, V3, v3, v3_add, v3_length, v3_normalize, v3_scale, v3_sub } from "./math";
import { MeshProjectionMethod, project_vertex_onto_mesh } from "./mesh_projection";
import { WORLD_PER_MM } from "./reference_skull_view";
import { append_chain_ribbon, append_stroke_ribbon } from "./ribbon";
import { FLOATS_PER_VERTEX, VertexSink, create_vertex_sink, reset_vertex_sink, vertex_sink_view } from "./vertex_sink";
import { ReferenceMesh, append_reference_mesh } from "./reference";
import { FetchedDocument, apply_fetched_document, create_persistence_state, fetch_current_document, fetch_document_if_changed_elsewhere, flush_autosave, fork_document_to_conflict_copy, has_unsaved_edits, list_documents_from_server, load_current_document_on_startup, remember_camera, rename_document, schedule_autosave, switch_document } from "./persistence";
import { ClipPlane, FLOATS_PER_TRANSLUCENT_VERTEX, create_line_renderer, create_translucent_mesh, draw_mesh_translucent, render_frame, set_overlay_lines, set_overlay_triangles, set_preview_line, set_reference_mesh, set_stroke_mesh, set_surface_mesh, set_translucent_mesh } from "./render";

// One vertex moved by wrap_skull_onto_skin: how it reached the mesh and how far it went.
type WrapRow = { vertex: VertexId; method: MeshProjectionMethod; push_mm: number };

// What differs between the pages that run the sketchpad.
export type SketchpadSetup = {
  // The mesh behind the drawing, already in world units; null = none (fetch failed).
  load_reference_mesh: () => Promise<ReferenceMesh | null>;
  // A second mesh shown with the reference while the page's optional `#eyeball_button`
  // is armed (plan-skin-over-skull-study.md Q9); undefined = the page has none.
  load_eyeball_mesh?: () => Promise<ReferenceMesh | null>;
  default_document_name: string; // opened when localStorage remembers no current document
  storage_key_prefix: string; // localStorage keys `<prefix>_current_document`, `<prefix>_crash_buffer`, `<prefix>_camera`
  // True: the docs panel lists every server document plus "new…"/"rename…". False: the
  // page is tied to its one document, the panel only names it.
  can_switch_documents: boolean;
  // Layers (plan-skin-over-skull-study.md Q4/Q10): new strokes go to
  // `active_layer`; `locked_layers` are locked for good (the page's optional
  // `#layer_bar` shows them locked but cannot unlock them or draw on them).
  active_layer: Layer;
  locked_layers: Layer[];
  // Layers this page never draws or picks; the layer bar has no row for them.
  hidden_layers: Layer[];
};

// Ported from the desktop app (driver.kc default_line_color = gray 0.03
// linear -> 0.196 sRGB; we write sRGB straight to the framebuffer).
const STROKE_COLOR = { r: 0.196, g: 0.196, b: 0.196 };
const HIGHLIGHT_COLOR = { r: 1.0, g: 0.65, b: 0.2 };
const PATCH_HIGHLIGHT_COLOR = { r: 0.75, g: 0.5, b: 0.2 }; // the selected patch's fill (plan-patch-subcurve-boundary.md Q11)
const HOT_COLOR = { r: 1.0, g: 1.0, b: 0.4 }; // what the hovering pen would hit
const PEN_RAY_COLOR = { r: 1.0, g: 0.55, b: 0.2 }; // the pen's line of sight, in the mirror
const PREVIEW_COLOR = { r: 0.6, g: 0.75, b: 1.0 };
const ANCHOR_COLOR = { r: 1.0, g: 1.0, b: 1.0 };
const HANDLE_COLOR = { r: 0.45, g: 0.8, b: 1.0 };
const HANDLE_LINE_COLOR = { r: 0.5, g: 0.5, b: 0.55 };
const PIN_COLOR = { r: 1.0, g: 0.5, b: 0.85 }; // pinned vertices (vertex_pins)
const KNOT_COLOR = { r: 0.55, g: 1.0, b: 0.55 }; // smooth knots (smooth_knots)
const SURFACE_COLOR = { r: 0.45, g: 0.55, b: 0.7 };
// A locked layer (the skull under the skin on the skin page) draws in its own
// colours, so it reads as the thing drawn over, not the thing being drawn.
const LOCKED_LAYER_STROKE_COLOR = { r: 0.4, g: 0.27, b: 0.13 };
const LOCKED_LAYER_SURFACE_COLOR = { r: 0.66, g: 0.6, b: 0.48 };
// "surf" off: same opaque fill, painted in the clear color (render.ts) so the
// patch still occludes what's behind it but reads as background.
const SURFACE_BACKGROUND_COLOR = { r: 0.384, g: 0.384, b: 0.384 };
const ANCHOR_SIZE_PIXELS = 12;
const HANDLE_SIZE_PIXELS = 9;
const HOT_SIZE_SCALE = 1.5; // hot markers grow by this much

export function start_sketchpad(setup: SketchpadSetup): void {
  const canvas = document.getElementById("canvas") as HTMLCanvasElement;
  const gl = canvas.getContext("webgl");
  if (!gl) {
    document.body.textContent = "WebGL not available";
    throw new Error("WebGL not available");
  }

  const camera = default_camera();
  // The camera the per-frame meshes (ribbons, shading, overlay markers) are built
  // for, and the height in CSS pixels of the view they draw into: the main view,
  // except during the mirror's pass (render_mirror).
  let mesh_camera: OrbitCamera = camera;
  let mesh_viewport_height_pixels = canvas.clientHeight;
  let mirror_visible = false;
  let pen_ray_screen: V2 | null = null; // where the pen last was, down or hovering
  let camera_rock_start_ms: number | null = null; // non-null while the rock key is held
  const tablet_document = empty_document();
  const renderer = create_line_renderer(gl);
  const persistence = create_persistence_state(setup.default_document_name, setup.storage_key_prefix);
  const history = create_history_state();
  let reference_mesh: ReferenceMesh | null = null;
  let reference_visible = true;
  let eyeball_mesh: ReferenceMesh | null = null; // loaded on the first arm of `#eyeball_button`
  let eyeball_visible = false;
  // Reference opacity (plan-skin-over-skull-study.md Q6): the page's optional
  // `#reference_alpha` slider; below 1 the reference (eyeballs included) goes
  // through the translucent renderer instead of the opaque reference buffer.
  let reference_alpha = 1;
  const reference_translucent = create_translucent_mesh(gl);
  let reference_translucent_vertices = new Float32Array(0); // reused across frames, grown on demand
  // Clipping plane (Q5): one plane cutting both the reference and the drawing,
  // sagittal (x > offset cut away). Driven by the page's optional `#clip_offset`
  // slider; parked at its max it cuts nothing.
  const clip_plane: ClipPlane = { normal: v3(1, 0, 0), offset: 0, enabled: false };
  let surface_colored = true; // "surf" button: blue fill vs. background-colored fill
  // Layers (plan-skin-over-skull-study.md): new strokes and their vertices go to
  // active_layer; locked layers are ignored by every pick and snap; hidden
  // layers are not drawn (and not picked either). A vertex is on the layers of
  // its strokes (vertex_is_on_layers), a patch on its first stroke's.
  let active_layer: Layer = setup.active_layer;
  const locked_layers = new Set<Layer>(setup.locked_layers);
  const hidden_layers = new Set<Layer>(setup.hidden_layers);
  function visible_layers(): Set<Layer> {
    return new Set(ALL_LAYERS.filter((layer) => !hidden_layers.has(layer)));
  }
  function pickable_layers(): Set<Layer> {
    return new Set(ALL_LAYERS.filter((layer) => !hidden_layers.has(layer) && !locked_layers.has(layer)));
  }
  let edit_state: EditState | null = null; // non-null = a stroke is selected (the primary)
  // Ctrl-tapped additions to the selection (plan-tablet-multi-select-patch.md
  // Q4): highlighted only, no handles; the patch/join/smooth buttons and delete
  // act on primary + extras. Kept apart from EditState so the selection can
  // later widen to vertices/patches without touching stroke editing.
  let extra_selection: StrokeId[] = [];
  // Vertex selection (plan-sketchpad-landmarks.md Q63): a tap on any vertex — a
  // stroke endpoint or a landmark — selects it instead of a stroke. A plain tap
  // selects one kind and clears the other; a ctrl-tap keeps both
  // (plan-sketchpad-pin-vertex-to-selected-line.md Q1). While a stroke is selected
  // too, the stroke owns the pen, the nudge keys and the name / midline / pin
  // buttons, and the vertex is only marked (Q2): see sole_selected_vertex. Alone,
  // the selected vertex drags on the camera plane and is the target of the
  // name/delete buttons.
  let selected_vertex: VertexId | null = null;
  // Second vertex (plan-sketchpad-vertex-nudge-link.md Q79): ctrl-tap on another
  // vertex while one is selected. Only the add-line button reads it; nudge and
  // drag move the primary alone. Cleared wherever selected_vertex is.
  let extra_vertex: VertexId | null = null;
  // Patch selection (plan-patch-subcurve-boundary.md Q9-Q11): a tap on a fill
  // that hit no vertex and no stroke. Index into tablet_document.patches
  // (patches have no ids); exclusive with the stroke and vertex selections.
  // Only `del` acts on it. Cleared wherever the others are.
  let selected_patch: number | null = null;
  let selected_vertex_drag_last_screen: V2 | null = null; // non-null while the pen drags the selected vertex
  // Armed tools: "line" creates strokes. "pin" waits for a tap on the selected
  // stroke's curve and creates a vertex pinned there. "split" waits for a tap on
  // the selected curve and splits it there into two strokes joined by a smooth
  // knot.
  type ArmedTool = "line" | "pin" | "split";
  let armed_tool: ArmedTool | null = null;
  // Swing is a sticky mode, not a tap tool: a handle drag tilts the plane by
  // swinging the other handle (edit_pen_move).
  let handle_mode: HandleMode = "plane";
  let line_state: LineToolState | null = null; // non-null while the line tool's pen is down
  let line_start_vertex: VertexId | null = null; // line tool armed from a selected vertex: the stroke starts there
  let pen_orbit_last_screen: V2 | null = null; // non-null while a bare pen drag orbits
  let frame_requested = false;
  // Pen tap detection (tap = select/deselect instead of moving anything).
  // Displacement from the down point, not path length — pencil taps jitter.
  let pen_down_screen: V2 | null = null;
  let pen_max_displacement_pixels = 0;
  // Below this displacement the pen gesture is a tap. Set at pen-down: smaller
  // when the pen lands on a control point or vertex, so its drag starts at once.
  let pen_tap_max_movement_pixels = TAP_MAX_MOVEMENT_PIXELS;
  // Hot item: what a tap at the hovering pen's position would select,
  // resolved every frame (the camera can move under a still pen) with the same
  // picks and priority as the tap (pick_tap_target), and drawn in
  // HOT_COLOR so the user knows before committing.
  type HotItem =
    | { kind: "stroke"; stroke_id: StrokeId }
    | { kind: "handle"; key: "p1" | "p2" } // handle of the selected stroke
    | { kind: "vertex"; vertex: VertexId }; // any document vertex (endpoints, landmarks, pins)
  let hover_screen: V2 | null = null; // null while the pen is down or off the canvas
  let hot_item: HotItem | null = null;

  // What a tap at a screen position selects. One order whatever is selected
  // (plan-sketchpad-selection-revamp.md Q1): a handle of the selected stroke,
  // any vertex, any stroke, a patch fill, nothing.
  type TapTarget =
    | { kind: "handle"; key: "p1" | "p2" }
    | { kind: "vertex"; vertex: VertexId }
    | { kind: "stroke"; stroke_id: StrokeId }
    | { kind: "patch"; patch: number } // index into tablet_document.patches
    | { kind: "nothing" };
  function pick_tap_target(screen: V2): TapTarget {
    if (edit_state !== null) {
      const stroke = stroke_by_id(tablet_document, edit_state.stroke_id);
      const key = pick_stroke_point(stroke, tablet_document, camera, screen, canvas);
      if (key === "p1" || key === "p2") return { kind: "handle", key };
    }
    const vertex = pick_vertex(tablet_document, camera, screen, canvas, pickable_layers());
    if (vertex !== null) return { kind: "vertex", vertex };
    const stroke_id = pick_stroke(tablet_document, camera, screen, canvas, pickable_layers());
    if (stroke_id !== null) return { kind: "stroke", stroke_id };
    const patch = pick_patch(tablet_document, camera, screen, canvas, pickable_layers()); // fill = last resort (Q10)
    return patch === null ? { kind: "nothing" } : { kind: "patch", patch };
  }

  function resolve_hot_item(): HotItem | null {
    if (hover_screen === null || armed_tool === "line") return null;
    if (armed_tool !== null) {
      // Pin / split armed: the tap feeds the tool, which only reads curves.
      const picked = pick_stroke(tablet_document, camera, hover_screen, canvas, pickable_layers());
      return picked === null ? null : { kind: "stroke", stroke_id: picked };
    }
    const target = pick_tap_target(hover_screen);
    if (target.kind === "patch" || target.kind === "nothing") return null;
    return target;
  }

  // The history entry the document equals; null before the first edit.
  function current_history_snapshot(): string | null {
    return history.position >= 0 ? history.entries[history.position].snapshot : null;
  }

  function request_render(): void {
    // Anything that changes what's on screen (strokes, surfaces, camera) goes
    // through here — piggyback the debounced autosave on it; it saves only when
    // the history snapshot changed. Before the early return: a pending
    // frame must not swallow the save (rAF pauses entirely in hidden tabs).
    // Pinned vertices are derived data — re-derive synchronously (NOT in the
    // rAF, which pauses in hidden tabs) so any host reshape carries its riders
    // before history snapshots and autosave see the document. The midline pass
    // goes first: a pin may ride a midline stroke, never the other way round (Q71).
    enforce_midline(tablet_document);
    update_pinned_vertex_positions(tablet_document);
    // A stroke operation (join, split here) can garbage-collect a vertex that is
    // selected along with the strokes.
    if (extra_vertex !== null && !tablet_document.vertices.some((vertex) => vertex.id === extra_vertex)) extra_vertex = null;
    if (selected_vertex !== null && !tablet_document.vertices.some((vertex) => vertex.id === selected_vertex)) {
      selected_vertex = extra_vertex;
      extra_vertex = null;
    }
    refresh_pin_button_armed();
    refresh_split_here_button();
    refresh_lock_buttons();
    refresh_midline_button_armed();
    refresh_smooth_button_armed();
    refresh_history_panel();
    refresh_width_panel();
    persistence.history_snapshot = current_history_snapshot();
    schedule_autosave(persistence, tablet_document, camera);
    if (frame_requested) return;
    frame_requested = true;
    requestAnimationFrame(() => {
      frame_requested = false;
      render_now();
    });
  }
  // One frame, synchronously. Also the debug hook `window.debug_render_now` for
  // automated tests in a hidden tab, where requestAnimationFrame never fires.
  function render_now(): void {
    const unrocked_yaw = camera.yaw;
    camera.yaw += camera_rock_yaw_offset();
    mesh_viewport_height_pixels = canvas.clientHeight;
    hot_item = resolve_hot_item();
    // Ribbons are camera-facing (desktop parity) — retessellate every frame.
    rebuild_stroke_mesh(edit_state === null ? null : edit_state.stroke_id, drag_snap_target_stroke());
    rebuild_surface_mesh();
    rebuild_reference_mesh();
    rebuild_edit_overlay();
    const view_projection = camera_view_projection(camera, canvas.width / canvas.height);
    const clip = clip_plane.enabled ? clip_plane : null;
    render_frame(renderer, view_projection, clip, () => {
      draw_mesh_translucent(reference_translucent, view_projection, camera_eye(camera), clip);
    });
    if (mirror_visible) render_mirror(clip);
    rebuild_stroke_labels();
    refresh_selection_readout();
    camera.yaw = unrocked_yaw;
  }

  // The mirror (see mirror_view.ts): the same scene again, into a corner of the
  // canvas, from the main view's right side and without perspective. The
  // camera-facing meshes are rebuilt for the mirror's camera; the next frame
  // rebuilds them for the main view.
  function render_mirror(clip: ClipPlane | null): void {
    const rectangle = mirror_rectangle(canvas.clientWidth, canvas.clientHeight);
    mesh_camera = mirror_camera_from_main_camera(camera);
    mesh_viewport_height_pixels = rectangle.size;
    rebuild_stroke_mesh(edit_state === null ? null : edit_state.stroke_id, drag_snap_target_stroke());
    rebuild_surface_mesh();
    rebuild_reference_mesh();
    rebuild_edit_overlay();
    const pixel_ratio = canvas.width / canvas.clientWidth;
    const x = Math.round(rectangle.left * pixel_ratio);
    const y = Math.round((canvas.clientHeight - rectangle.top - rectangle.size) * pixel_ratio); // GL counts from the bottom
    const size = Math.round(rectangle.size * pixel_ratio);
    gl!.enable(gl!.SCISSOR_TEST); // render_frame clears: keep that inside the mirror
    gl!.scissor(x, y, size, size);
    gl!.viewport(x, y, size, size);
    const view_projection = camera_orthographic_view_projection(mesh_camera, 1);
    const mirror_eye = camera_eye(mesh_camera);
    render_frame(renderer, view_projection, clip, () => {
      draw_mesh_translucent(reference_translucent, view_projection, mirror_eye, clip);
    });
    gl!.disable(gl!.SCISSOR_TEST);
    gl!.viewport(0, 0, canvas.width, canvas.height);
    mesh_camera = camera;
    mesh_viewport_height_pixels = canvas.clientHeight;
  }

  // Camera rock: while `r` is held the main view swings left and right around
  // its yaw, so depth shows as motion. The swing exists inside render_now only:
  // the yaw that autosave and the pen read never changes.
  function camera_rock_yaw_offset(): number {
    if (camera_rock_start_ms === null) return 0;
    const swing_radians = (8 * Math.PI) / 180;
    const period_seconds = 1;
    const seconds = (performance.now() - camera_rock_start_ms) / 1000;
    return swing_radians * Math.sin((2 * Math.PI * seconds) / period_seconds);
  }
  function camera_rock_frame(): void {
    if (camera_rock_start_ms === null) return;
    render_now();
    requestAnimationFrame(camera_rock_frame);
  }
  function stop_camera_rock(): void {
    if (camera_rock_start_ms === null) return;
    camera_rock_start_ms = null;
    render_now();
  }
  window.addEventListener("keydown", (event) => {
    if (event.key.toLowerCase() !== "r" || event.repeat || event.ctrlKey || event.metaKey || event.altKey) return;
    if (document.activeElement instanceof HTMLInputElement) return;
    if (camera_rock_start_ms !== null) return;
    camera_rock_start_ms = performance.now();
    requestAnimationFrame(camera_rock_frame);
  });
  window.addEventListener("keyup", (event) => {
    if (event.key.toLowerCase() === "r") stop_camera_rock();
  });
  window.addEventListener("blur", stop_camera_rock);

  // Names live in an HTML overlay (no text rendering in WebGL): only the selected
  // stroke's name shows, placed at the curve's midpoint each frame; a named vertex
  // (landmark) shows its name beside its dot only while hot or selected.
  const stroke_labels = document.getElementById("stroke_labels") as HTMLDivElement;
  function rebuild_stroke_labels(): void {
    const labels: HTMLDivElement[] = [];
    const push_label = (text: string, world: V3, offset_pixels: number) => {
      const screen = camera_world_to_screen(camera, world, canvas.clientWidth, canvas.clientHeight);
      if (screen === null) return;
      const label = document.createElement("div");
      label.textContent = text;
      label.style.left = `${screen.x + offset_pixels}px`;
      label.style.top = `${screen.y}px`;
      labels.push(label);
    };
    const stroke = edit_state === null ? null : stroke_by_id(tablet_document, edit_state.stroke_id);
    if (stroke !== null && stroke.name !== undefined) {
      push_label(stroke.name, bezier_point(stroke_control_points(stroke, tablet_document), 0.5), 0);
    }
    // A vertex's name shows only while it is hot (hovered) or selected — the
    // landmarks would otherwise paper the skull with text.
    const hot_vertex = hot_item !== null && hot_item.kind === "vertex" ? hot_item.vertex : null;
    for (const vertex_id of new Set([selected_vertex, hot_vertex])) {
      if (vertex_id === null) continue;
      const vertex = vertex_by_id(tablet_document, vertex_id);
      if (vertex.name !== undefined) push_label(vertex.name, vertex_world_position(tablet_document, vertex), ANCHOR_SIZE_PIXELS * 2);
    }
    stroke_labels.replaceChildren(...labels);
  }

  // Selection readout (bottom left): the document ids of what is selected and
  // hovered, as text, so a bug report can name the items. A click copies it.
  const selection_readout_style = document.createElement("style");
  selection_readout_style.textContent = `
    #selection_readout {
      position: fixed; left: max(12px, env(safe-area-inset-left)); bottom: max(12px, env(safe-area-inset-bottom));
      z-index: 20; padding: 6px 10px; border-radius: 6px; border: 1px solid #444; background: #222228cc;
      font: 13px monospace; color: #ccc; white-space: pre; cursor: copy;
    }
    #selection_readout.copied { border-color: #6c9f5a; }
    #selection_readout.copy_failed { border-color: #e05a5a; }`;
  document.head.appendChild(selection_readout_style);
  const selection_readout = document.createElement("div");
  selection_readout.id = "selection_readout";
  selection_readout.title = "click to copy";
  document.body.append(selection_readout);
  function selection_readout_text(): string {
    return describe_selection(tablet_document, {
      stroke: edit_state === null ? null : edit_state.stroke_id,
      selected_handle: edit_state === null ? null : edit_state.selected_handle,
      extra_strokes: extra_selection,
      vertex: selected_vertex,
      extra_vertex: extra_vertex,
      patch: selected_patch,
      hot: hot_item,
    });
  }
  function refresh_selection_readout(): void {
    const text = selection_readout_text();
    if (selection_readout.textContent !== text) selection_readout.textContent = text;
  }
  selection_readout.addEventListener("click", async () => {
    // navigator.clipboard only exists on https and localhost: absent on the tablet over plain http.
    let copied = false;
    if (navigator.clipboard !== undefined) {
      copied = await navigator.clipboard.writeText(selection_readout_text()).then(() => true, () => false);
    }
    const flash = copied ? "copied" : "copy_failed";
    selection_readout.classList.add(flash);
    window.setTimeout(() => selection_readout.classList.remove(flash), 600);
  });

  function resize_canvas_to_display(): void {
    const dpr = window.devicePixelRatio;
    canvas.width = Math.round(canvas.clientWidth * dpr);
    canvas.height = Math.round(canvas.clientHeight * dpr);
    gl!.viewport(0, 0, canvas.width, canvas.height);
    request_render();
  }
  window.addEventListener("resize", resize_canvas_to_display);

  // The vertex the selected vertex would weld into if its drag ended now, or null:
  // only while the pen has really dragged it (a tap never welds), and never for a
  // pinned vertex.
  function selected_vertex_weld_target(): VertexId | null {
    if (selected_vertex === null || selected_vertex_drag_last_screen === null) return null;
    if (pen_max_displacement_pixels < pen_tap_max_movement_pixels) return null;
    if (pin_by_vertex(tablet_document, selected_vertex) !== null) return null;
    return find_merge_target_vertex(tablet_document, selected_vertex, pickable_layers());
  }

  // The stroke a dragged vertex would get pinned to on release (drag-time
  // warning, same function as the release so they can never disagree), or null.
  function drag_snap_target_stroke(): StrokeId | null {
    if (edit_state === null || (edit_state.dragging !== "p0" && edit_state.dragging !== "p3")) return null;
    if (pen_max_displacement_pixels < pen_tap_max_movement_pixels) return null; // a tap never pins
    const stroke = stroke_by_id(tablet_document, edit_state.stroke_id);
    const dragged_vertex = edit_state.dragging === "p0" ? stroke.p0_vertex : stroke.p3_vertex;
    if (find_merge_target_vertex(tablet_document, dragged_vertex, pickable_layers()) !== null) return null; // weld wins
    const target = find_snap_target_stroke(tablet_document, dragged_vertex, pickable_layers());
    return target === null ? null : target.stroke_id;
  }

  // Per-frame meshes reuse one sink each so a frame allocates nothing for them.
  const stroke_sink = create_vertex_sink(1 << 15);
  const surface_sink = create_vertex_sink(1 << 15);
  const reference_sink = create_vertex_sink(1 << 15);

  function rebuild_stroke_mesh(highlighted_stroke: StrokeId | null, snap_target_stroke: StrokeId | null): void {
    const vertices = stroke_sink;
    reset_vertex_sink(vertices);
    for (const stroke of tablet_document.strokes) {
      if (hidden_layers.has(stroke.layer)) continue;
      const highlighted = stroke.id === highlighted_stroke || stroke.id === snap_target_stroke || extra_selection.includes(stroke.id);
      const hot = hot_item !== null && hot_item.kind === "stroke" && hot_item.stroke_id === stroke.id;
      const color = hot ? HOT_COLOR : highlighted ? HIGHLIGHT_COLOR : locked_layers.has(stroke.layer) ? LOCKED_LAYER_STROKE_COLOR : STROKE_COLOR;
      append_stroke_ribbon(stroke, tablet_document, mesh_camera, color, vertices);
    }
    append_contour_ribbons(vertices);
    set_stroke_mesh(renderer, vertex_sink_view(vertices));
  }

  // Computed contours share the stroke mesh so they get the same depth bias
  // and draw order as drawn strokes; they are derived per frame, never stored.
  function append_contour_ribbons(vertices: VertexSink): void {
    const eye = camera_eye(mesh_camera);
    for (const patch of tablet_document.patches) {
      const layer = patch_layer(patch, tablet_document);
      if (hidden_layers.has(layer)) continue;
      const grid = patch_surface_grid(patch, tablet_document);
      if (grid === null) continue;
      const color = locked_layers.has(layer) ? LOCKED_LAYER_STROKE_COLOR : STROKE_COLOR;
      for (const chain of extract_contour_chains(grid, eye)) append_chain_ribbon(chain, mesh_camera, color, vertices);
    }
  }

  function rebuild_surface_mesh(): void {
    const vertices = surface_sink;
    reset_vertex_sink(vertices);
    tablet_document.patches.forEach((patch, index) => {
      const layer = patch_layer(patch, tablet_document);
      if (hidden_layers.has(layer)) return;
      const color = index === selected_patch ? PATCH_HIGHLIGHT_COLOR
        : !surface_colored ? SURFACE_BACKGROUND_COLOR
        : locked_layers.has(layer) ? LOCKED_LAYER_SURFACE_COLOR : SURFACE_COLOR;
      append_patch_mesh(patch, tablet_document, mesh_camera, color, vertices);
    });
    set_surface_mesh(renderer, vertex_sink_view(vertices));
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
      set_translucent_mesh(reference_translucent, new Float32Array(0));
      return;
    }
    const vertices = reference_sink;
    reset_vertex_sink(vertices);
    append_reference_mesh(reference_mesh, mesh_camera, vertices);
    if (eyeball_visible && eyeball_mesh !== null) append_reference_mesh(eyeball_mesh, mesh_camera, vertices);
    if (reference_alpha >= 1) {
      set_reference_mesh(renderer, vertex_sink_view(vertices));
      set_translucent_mesh(reference_translucent, new Float32Array(0));
      return;
    }
    set_reference_mesh(renderer, new Float32Array(0));
    set_translucent_mesh(reference_translucent, translucent_vertices_from_sink(vertices, reference_alpha));
  }

  // The 6-float headlight-shaded sink, widened to the translucent renderer's
  // 7-float layout with one alpha for every vertex.
  function translucent_vertices_from_sink(sink: VertexSink, alpha: number): Float32Array {
    const vertex_count = sink.length / FLOATS_PER_VERTEX;
    const float_count = vertex_count * FLOATS_PER_TRANSLUCENT_VERTEX;
    if (reference_translucent_vertices.length < float_count) reference_translucent_vertices = new Float32Array(float_count);
    const out = reference_translucent_vertices;
    for (let vertex = 0; vertex < vertex_count; vertex++) {
      const source = vertex * FLOATS_PER_VERTEX;
      const target = vertex * FLOATS_PER_TRANSLUCENT_VERTEX;
      for (let i = 0; i < FLOATS_PER_VERTEX; i++) out[target + i] = sink.data[source + i];
      out[target + FLOATS_PER_VERTEX] = alpha;
    }
    return out.subarray(0, float_count);
  }

  // Mirror pass only: the line of sight under the pen, as the main view casts
  // it. In the mirror it shows every depth the pen's position could mean.
  function append_pen_ray_overlay(line_vertices: number[]): void {
    if (mesh_camera === camera || pen_ray_screen === null) return;
    const ray = camera_pen_ray(camera, pen_ray_screen, canvas.clientWidth, canvas.clientHeight);
    const far_end = v3_add(ray.origin, v3_scale(ray.direction, camera.distance * 2));
    line_vertices.push(ray.origin.x, ray.origin.y, ray.origin.z, PEN_RAY_COLOR.r, PEN_RAY_COLOR.g, PEN_RAY_COLOR.b);
    line_vertices.push(far_end.x, far_end.y, far_end.z, PEN_RAY_COLOR.r, PEN_RAY_COLOR.g, PEN_RAY_COLOR.b);
  }

  function rebuild_edit_overlay(): void {
    const basis = camera_basis(mesh_camera);
    const units_per_pixel = camera_world_units_per_pixel(mesh_camera, mesh_viewport_height_pixels);
    const anchor_half = (ANCHOR_SIZE_PIXELS / 2) * units_per_pixel;
    const handle_half = (HANDLE_SIZE_PIXELS / 2) * units_per_pixel;
    const line_vertices: number[] = [];
    const triangle_vertices: number[] = [];

    // Vertices have no marker of their own: only the hot vertex (grown) and the
    // selected vertex (anchor-sized, in the highlight colour) draw.
    const drawn_layers = visible_layers();
    for (const vertex of tablet_document.vertices) {
      if (!vertex_is_on_layers(tablet_document, vertex.id, drawn_layers)) continue;
      const hot = hot_item !== null && hot_item.kind === "vertex" && hot_item.vertex === vertex.id;
      const world_position = vertex_world_position(tablet_document, vertex);
      if (vertex.id === selected_vertex || vertex.id === extra_vertex) {
        append_billboard_square(world_position, anchor_half, basis.right, basis.up, HIGHLIGHT_COLOR, triangle_vertices);
      } else if (hot) {
        append_billboard_square(world_position, handle_half * HOT_SIZE_SCALE, basis.right, basis.up, HOT_COLOR, triangle_vertices);
      }
    }
    if (edit_state === null) {
      // Drag-time weld warning for a dragged vertex, same marker as an endpoint drag below.
      const weld_target = selected_vertex_weld_target();
      if (weld_target !== null) {
        append_billboard_square(
          vertex_position(tablet_document, weld_target), anchor_half * 2, basis.right, basis.up,
          HIGHLIGHT_COLOR, triangle_vertices,
        );
      }
      const pen_ray_vertices: number[] = [];
      append_pen_ray_overlay(pen_ray_vertices);
      set_overlay_lines(renderer, new Float32Array(pen_ray_vertices));
      set_overlay_triangles(renderer, new Float32Array(triangle_vertices));
      return;
    }
    const selected_stroke = stroke_by_id(tablet_document, edit_state.stroke_id);
    const points = stroke_control_points(selected_stroke, tablet_document);
    const push_line = (a: V3, b: V3, color: { r: number; g: number; b: number } = HANDLE_LINE_COLOR) => {
      line_vertices.push(a.x, a.y, a.z, color.r, color.g, color.b);
      line_vertices.push(b.x, b.y, b.z, color.r, color.g, color.b);
    };
    push_line(points.p0, points.p1);
    push_line(points.p3, points.p2);
    // The hot handle / endpoint / pin draws bigger and in HOT_COLOR.
    const is_hot_handle = (key: "p1" | "p2") => hot_item !== null && hot_item.kind === "handle" && hot_item.key === key;
    const is_hot_vertex = (vertex: VertexId) => hot_item !== null && hot_item.kind === "vertex" && hot_item.vertex === vertex;
    const push_marker = (center: V3, half_size: number, color: { r: number; g: number; b: number }, hot: boolean) => {
      append_billboard_square(
        center, hot ? half_size * HOT_SIZE_SCALE : half_size, basis.right, basis.up,
        hot ? HOT_COLOR : color, triangle_vertices,
      );
    };
    // The selected handle (the nudge keys' target) draws anchor-sized.
    const selected_handle = edit_state.selected_handle;
    push_marker(points.p1, selected_handle === "p1" ? anchor_half : handle_half, HANDLE_COLOR, is_hot_handle("p1"));
    push_marker(points.p2, selected_handle === "p2" ? anchor_half : handle_half, HANDLE_COLOR, is_hot_handle("p2"));
    // Endpoints that are pinned vertices (riding some other stroke) show in the
    // pin color so it's clear they'll slide, not translate, when grabbed; smooth
    // knots in the knot color so it's clear the neighbour's handle will follow.
    const anchor_color = (vertex: VertexId) => {
      if (pin_by_vertex(tablet_document, vertex) !== null) return PIN_COLOR;
      if (smooth_knots_at_vertex(tablet_document, vertex).length > 0) return KNOT_COLOR;
      return ANCHOR_COLOR;
    };
    push_marker(points.p0, anchor_half, anchor_color(selected_stroke.p0_vertex), is_hot_vertex(selected_stroke.p0_vertex));
    push_marker(points.p3, anchor_half, anchor_color(selected_stroke.p3_vertex), is_hot_vertex(selected_stroke.p3_vertex));
    // Pinned vertices riding the selected stroke.
    for (const pin of tablet_document.vertex_pins) {
      if (pin.host_stroke !== edit_state.stroke_id) continue;
      push_marker(vertex_position(tablet_document, pin.vertex), handle_half, PIN_COLOR, is_hot_vertex(pin.vertex));
    }
    // Drag-time snap warning (Q3): while a vertex is being dragged, mark the
    // vertex it would weld into on release so the merge is never a surprise.
    // Not during a tap, which never welds.
    const pen_really_dragged = pen_max_displacement_pixels >= pen_tap_max_movement_pixels;
    if (pen_really_dragged && (edit_state.dragging === "p0" || edit_state.dragging === "p3")) {
      const dragged_vertex = edit_state.dragging === "p0" ? selected_stroke.p0_vertex : selected_stroke.p3_vertex;
      const target_vertex = find_merge_target_vertex(tablet_document, dragged_vertex, pickable_layers());
      if (target_vertex !== null) {
        append_billboard_square(
          vertex_position(tablet_document, target_vertex), anchor_half * 2, basis.right, basis.up,
          HIGHLIGHT_COLOR, triangle_vertices,
        );
      }
    }
    append_pen_ray_overlay(line_vertices);
    set_overlay_lines(renderer, new Float32Array(line_vertices));
    set_overlay_triangles(renderer, new Float32Array(triangle_vertices));
  }

  function update_preview_line(): void {
    if (line_state === null) {
      set_preview_line(renderer, new Float32Array(0));
      return;
    }
    // The straight segment the stroke will be, from the start to the snapped end point.
    const vertices: number[] = [];
    for (const point of [line_state.start_world, line_state.end_world]) {
      vertices.push(point.x, point.y, point.z, PREVIEW_COLOR.r, PREVIEW_COLOR.g, PREVIEW_COLOR.b);
    }
    set_preview_line(renderer, new Float32Array(vertices));
  }

  // Line-tool pen-up: a drag commits a straight stroke and
  // auto-selects it; a tap exits the tool (Q27). Either way the tool disarms, so
  // the very next drag adjusts the fresh stroke instead of creating another.
  function line_mode_pen_up(): void {
    const was_tap = pen_max_displacement_pixels < pen_tap_max_movement_pixels;
    if (!was_tap && line_state !== null) {
      // Every layer's vertices live on the skull bone until a mandible exists (Q15).
      const stroke_id = line_pen_up(line_state, tablet_document, active_layer, SKULL_BONE_ID);
      if (stroke_id !== null) {
        select_stroke_by_tap(stroke_id, false);
        pen_history_label = `add line ${stroke_id}`;
      }
    }
    set_armed_tool(null); // also clears line_state
    update_preview_line();
  }

  // The pin riding `host_stroke` under the pen (within control-point pick
  // range), or null.
  function pick_pin_on_stroke(host_stroke: StrokeId, screen: V2): VertexPin | null {
    let best: VertexPin | null = null;
    let best_distance = CONTROL_POINT_PICK_RADIUS_PIXELS;
    for (const pin of tablet_document.vertex_pins) {
      if (pin.host_stroke !== host_stroke) continue;
      const projected = camera_world_to_screen(camera, vertex_position(tablet_document, pin.vertex), canvas.clientWidth, canvas.clientHeight);
      if (projected === null) continue;
      const distance = Math.hypot(projected.x - screen.x, projected.y - screen.y);
      if (distance < best_distance) {
        best_distance = distance;
        best = pin;
      }
    }
    return best;
  }

  // Plain tap on a stroke: it becomes the sole selection. Ctrl-tap: toggles it
  // in the selection — on the primary, the first extra is promoted (Q4).
  function select_stroke_by_tap(picked: StrokeId, multi: boolean): void {
    selected_patch = null;
    if (!multi) {
      selected_vertex = null;
      extra_vertex = null;
      extra_selection = [];
      edit_state = begin_edit_state(picked);
      return;
    }
    if (edit_state === null) {
      edit_state = begin_edit_state(picked);
    } else if (picked === edit_state.stroke_id) {
      const promoted = extra_selection.shift();
      edit_state = promoted === undefined ? null : begin_edit_state(promoted);
    } else if (extra_selection.includes(picked)) {
      extra_selection = extra_selection.filter((id) => id !== picked);
    } else {
      extra_selection.push(picked);
    }
  }

  // The one place a tap changes the selection (plan-sketchpad-selection-revamp.md
  // Q8): a plain tap replaces the selection; a ctrl-tap toggles the tapped line
  // or vertex and keeps the rest, the other kind included
  // (plan-sketchpad-pin-vertex-to-selected-line.md Q1). Vertices: two at most.
  function select_tap_target(target: TapTarget, multi: boolean): void {
    switch (target.kind) {
      case "handle":
        // Sub-selection of the selected line: the line and its extras stay (Q4).
        if (edit_state !== null) edit_state.selected_handle = target.key;
        break;
      case "vertex":
        if (multi) {
          selected_patch = null;
          if (selected_vertex === null) {
            selected_vertex = target.vertex;
          } else if (target.vertex === selected_vertex) {
            // Ctrl-tap on the primary drops it; the extra is promoted.
            selected_vertex = extra_vertex;
            extra_vertex = null;
          } else {
            // Ctrl-tap on a second vertex sets / clears the extra (Q79).
            extra_vertex = extra_vertex === target.vertex ? null : target.vertex;
          }
          break;
        }
        edit_state = null;
        extra_selection = [];
        selected_patch = null;
        selected_vertex = target.vertex;
        extra_vertex = null;
        break;
      case "stroke":
        if (!multi && edit_state !== null && target.stroke_id === edit_state.stroke_id) {
          // Plain tap on the body of the selected line: only the handle
          // sub-selection goes.
          edit_state.selected_handle = null;
          break;
        }
        select_stroke_by_tap(target.stroke_id, multi);
        break;
      case "patch":
      case "nothing":
        edit_state = null;
        extra_selection = [];
        selected_vertex = null;
        extra_vertex = null;
        selected_patch = target.kind === "patch" ? target.patch : null;
        break;
    }
  }

  // Selected-stroke pen-up: a drag (control point or orbit)
  // just ends. A tap moved nothing: with a pick tool armed it feeds the tool,
  // otherwise it selects what it hit (select_tap_target).
  function edit_mode_pen_up(position: V2, multi: boolean): void {
    if (edit_state === null) return;
    const was_tap = pen_max_displacement_pixels < pen_tap_max_movement_pixels;
    if (!was_tap) {
      if (edit_state.dragging_pin !== null) pen_history_label = `move pin ${edit_state.dragging_pin}`;
      else if (edit_state.dragging !== null) {
        // Same label for every drag of one control point, so a run of them merges
        // into a single history entry (see end_history_step).
        pen_history_label = `move ${edit_state.dragging} of stroke ${edit_state.stroke_id}`;
        pen_history_merge = true;
      }
    }
    edit_pen_up(edit_state, tablet_document, pickable_layers(), !was_tap);
    if (!was_tap) return;
    if (armed_tool === "pin") {
      // Pin creation (Q2): a tap on the selected stroke's curve drops a new
      // vertex at the nearest curve point, constrained there permanently.
      const nearest = nearest_t_on_stroke_screen(tablet_document, edit_state.stroke_id, camera, position, canvas);
      if (nearest.distance < STROKE_PICK_RADIUS_PIXELS) {
        const host_stroke = stroke_by_id(tablet_document, edit_state.stroke_id);
        const points = stroke_control_points(host_stroke, tablet_document);
        // The pin rides its host stroke, so it lives on the host's bone.
        const vertex = add_vertex(tablet_document, bezier_point(points, nearest.t), vertex_by_id(tablet_document, host_stroke.p0_vertex).bone_id);
        tablet_document.vertex_pins.push({ vertex, host_stroke: edit_state.stroke_id, t: nearest.t });
        pen_history_label = `pin vertex ${vertex}`;
        // The new pinned vertex becomes the selection, ready for unpin, split
        // here, or a line starting from it.
        select_tap_target({ kind: "vertex", vertex }, false);
      }
      set_armed_tool(null); // tap off the curve = cancel, selection kept
      return;
    }
    if (armed_tool === "split") {
      // Split (Q4): the selected stroke is cut at the tapped point into two
      // strokes joined by a smooth knot; the [0, t] half stays selected. Only
      // the selection is a candidate so a tap at a junction is unambiguous. A
      // tap on a vertex pinned to the selection cuts exactly there, reusing it.
      const selected_id = edit_state.stroke_id;
      pen_history_label = `split stroke ${selected_id}`;
      const pinned = pick_pin_on_stroke(selected_id, position);
      if (pinned !== null) {
        split_stroke(tablet_document, selected_id, pinned.t, pinned.vertex);
      } else {
        const nearest = nearest_t_on_stroke_screen(tablet_document, selected_id, camera, position, canvas);
        if (nearest.distance < STROKE_PICK_RADIUS_PIXELS) split_stroke(tablet_document, selected_id, nearest.t);
      }
      drop_unused_patch_strokes(tablet_document); // a patch that used one side of the cut keeps only that half
      set_armed_tool(null); // tap off the curve = cancel, selection kept
      return;
    }
    select_tap_target(pick_tap_target(position), multi);
  }

  const line_button = document.getElementById("line_button") as HTMLButtonElement;
  const patch_button = document.getElementById("patch_button") as HTMLButtonElement;
  const join_button = document.getElementById("join_button") as HTMLButtonElement;
  const split_button = document.getElementById("split_button") as HTMLButtonElement;
  const smooth_button = document.getElementById("smooth_button") as HTMLButtonElement;
  const pin_button = document.getElementById("pin_button") as HTMLButtonElement;
  const swing_button = document.getElementById("swing_button") as HTMLButtonElement;
  function set_handle_mode(mode: HandleMode): void {
    handle_mode = handle_mode === mode ? "plane" : mode;
    swing_button.classList.toggle("armed", handle_mode === "swing");
  }
  swing_button.addEventListener("click", () => set_handle_mode("swing"));
  // The pin of the selected vertex, or null: the target of unpin and split here.
  // The selected vertex when no stroke is selected with it. With a stroke
  // selected too, the stroke owns the buttons that take either kind
  // (plan-sketchpad-pin-vertex-to-selected-line.md Q2).
  function sole_selected_vertex(): VertexId | null {
    return edit_state === null ? selected_vertex : null;
  }
  function selected_vertex_pin(): VertexPin | null {
    const vertex = sole_selected_vertex();
    return vertex === null ? null : pin_by_vertex(tablet_document, vertex);
  }
  // The pin button also lights up while a pinned vertex is selected — in that
  // state tapping it unpins (Q11). Re-checked every frame since the selection
  // changes on pen gestures, not just button presses.
  function refresh_pin_button_armed(): void {
    pin_button.classList.toggle("armed", armed_tool === "pin" || selected_vertex_pin() !== null);
  }
  function set_armed_tool(tool: ArmedTool | null): void {
    armed_tool = tool;
    if (tool !== "line") {
      line_state = null;
      line_start_vertex = null;
    }
    line_button.classList.toggle("armed", tool === "line");
    split_button.classList.toggle("armed", tool === "split");
    refresh_pin_button_armed();
  }
  // The line button depends on the vertex selection (Khoa 2026-09-27, replaces
  // the separate add-line button of plan-sketchpad-vertex-nudge-link.md Q80):
  // two vertices (tap one, ctrl-tap the other) = a straight stroke between them
  // at once; one vertex = arm the tool with the stroke starting at that vertex
  // wherever the pen lands; none = arm the tool to drag a line from anywhere. The new stroke becomes
  // the selection with its handles up, ready to bend.
  line_button.addEventListener("click", () => {
    if (armed_tool === "line") {
      set_armed_tool(null);
      return;
    }
    if (selected_vertex !== null && extra_vertex !== null) {
      begin_history_step(history, tablet_document);
      const stroke_id = add_straight_stroke(tablet_document, selected_vertex, extra_vertex, active_layer);
      end_history_step(history, tablet_document, `add line ${stroke_id}`);
      selected_vertex = null;
      extra_vertex = null;
      edit_state = begin_edit_state(stroke_id);
      request_render();
      return;
    }
    set_armed_tool("line");
    line_start_vertex = selected_vertex;
  });
  // Patch: fill the selected strokes (primary + extras, 2 or more). Which fill
  // they get is derived per frame from their corners (patch.ts).
  patch_button.addEventListener("click", () => {
    if (edit_state === null || extra_selection.length === 0) return;
    begin_history_step(history, tablet_document);
    tablet_document.patches.push({ strokes: [edit_state.stroke_id, ...extra_selection] });
    end_history_step(history, tablet_document, `make patch [${[edit_state.stroke_id, ...extra_selection].join(" ")}]`);
    edit_state = null;
    extra_selection = [];
    request_render();
  });

  // The one extra stroke of a two-stroke selection, or null (join/smooth need
  // exactly a primary and one extra; the primary leads).
  function single_extra_stroke(): StrokeId | null {
    return edit_state !== null && extra_selection.length === 1 ? extra_selection[0] : null;
  }

  // Join: the primary and the one extra merge into one cubic (they must share
  // a vertex); the merged stroke keeps the primary's id and stays selected.
  join_button.addEventListener("click", () => {
    const other = single_extra_stroke();
    if (edit_state === null || other === null || join_is_locked()) return;
    begin_history_step(history, tablet_document);
    const merged_id = merge_adjacent_strokes(tablet_document, edit_state.stroke_id, other);
    end_history_step(history, tablet_document, `join strokes ${edit_state.stroke_id} ${other}`);
    if (merged_id === null) return; // not adjacent (or a closed loop): selection kept
    edit_state = begin_edit_state(merged_id);
    extra_selection = [];
    request_render();
  });

  // Split: the next tap on the selected curve cuts it there into two strokes
  // joined by a smooth knot.
  split_button.addEventListener("click", () => {
    if (edit_state === null) return;
    set_armed_tool(armed_tool === "split" ? null : "split");
  });

  // Smooth: the one extra becomes smooth with the primary at their shared
  // endpoint (the primary keeps its tangent, the extra is re-aimed). A toggle:
  // if the two are already smooth, the knot is dropped instead (handles stay).
  // Lit while the selected pair is smooth, re-checked every frame.
  function refresh_smooth_button_armed(): void {
    const other = single_extra_stroke();
    smooth_button.classList.toggle(
      "armed", edit_state !== null && other !== null && smooth_knot_between_strokes(tablet_document, edit_state.stroke_id, other) !== null,
    );
  }
  smooth_button.addEventListener("click", () => {
    const other = single_extra_stroke();
    if (edit_state === null || other === null) return;
    begin_history_step(history, tablet_document);
    let label = `unsmooth strokes ${edit_state.stroke_id} ${other}`;
    if (!unsmooth_strokes(tablet_document, edit_state.stroke_id, other)) {
      smooth_strokes(tablet_document, edit_state.stroke_id, other); // no shared vertex: nothing
      label = `smooth strokes ${edit_state.stroke_id} ${other}`;
    }
    end_history_step(history, tablet_document, label);
    request_render();
  });

  // Straighten: every selected line becomes the straight segment between its
  // endpoints (the vertices stay where they are).
  const straighten_button = document.getElementById("straighten_button") as HTMLButtonElement;
  straighten_button.addEventListener("click", () => {
    if (edit_state === null) return;
    begin_history_step(history, tablet_document);
    straighten_strokes(tablet_document, selected_strokes());
    end_history_step(history, tablet_document, `straighten strokes ${selected_strokes().join(" ")}`);
    request_render();
  });

  // Split here (plan-sketchpad-split-at-vertex.md): with a pinned vertex selected,
  // this button cuts its host stroke exactly there, the vertex becoming the smooth
  // knot (and staying selected). Shown only while such a vertex is selected -- the
  // same state the pin button reads for "unpin".
  const split_here_button = document.getElementById("split_here_button") as HTMLButtonElement;
  function refresh_split_here_button(): void {
    split_here_button.hidden = selected_vertex_pin() === null;
  }
  split_here_button.addEventListener("click", () => {
    const pin = selected_vertex_pin();
    if (pin === null) return;
    const stroke_id = pin.host_stroke;
    const vertex = pin.vertex;
    begin_history_step(history, tablet_document);
    const new_stroke = split_stroke(tablet_document, stroke_id, pin.t, vertex);
    if (new_stroke === null) return; // pin within the end guard: nothing to record
    drop_unused_patch_strokes(tablet_document); // a patch that used one side of the cut keeps only that half
    end_history_step(history, tablet_document, `split stroke ${stroke_id} at vertex ${vertex}`);
    request_render();
  });

  // Pin: with a pinned vertex selected, unpin it (frozen in place as a free
  // vertex, still selected); otherwise, with a stroke selected, arm pin creation
  // — the next tap on the selected stroke's curve drops a vertex constrained there.
  pin_button.addEventListener("click", () => {
    const selected_pin = selected_vertex_pin();
    if (selected_pin !== null) {
      if (unpin_is_locked()) return; // a patch's sub-curve ends here (Q3)
      begin_history_step(history, tablet_document);
      const unpinned_vertex = selected_pin.vertex;
      tablet_document.vertex_pins = tablet_document.vertex_pins.filter((pin) => pin.vertex !== unpinned_vertex);
      end_history_step(history, tablet_document, `unpin vertex ${unpinned_vertex}`);
      request_render();
      return;
    }
    if (edit_state === null) return; // arming needs a selected host stroke
    set_armed_tool(armed_tool === "pin" ? null : "pin");
  });

  // Detach end: with exactly one stroke and one vertex selected, the vertex being
  // an end of the stroke that other strokes are welded to, gives the stroke its
  // own new vertex at the same place. The stroke and the new vertex stay
  // selected; drag the stroke's end away to separate them (releasing it next to
  // the old vertex welds it back). Disabled while the stroke bounds a patch.
  const detach_end_button = document.getElementById("detach_end_button") as HTMLButtonElement;
  function selected_detach_end_pair(): { stroke_id: StrokeId; vertex: VertexId } | null {
    if (edit_state === null || extra_selection.length > 0 || selected_vertex === null || extra_vertex !== null) return null;
    if (!stroke_end_can_detach_from_weld(tablet_document, edit_state.stroke_id, selected_vertex)) return null;
    return { stroke_id: edit_state.stroke_id, vertex: selected_vertex };
  }
  detach_end_button.addEventListener("click", () => {
    const pair = selected_detach_end_pair();
    if (pair === null || stroke_bounds_a_patch(tablet_document, pair.stroke_id)) return;
    begin_history_step(history, tablet_document);
    selected_vertex = detach_stroke_end_from_weld(tablet_document, pair.stroke_id, pair.vertex);
    end_history_step(history, tablet_document, `detach line ${pair.stroke_id} from vertex ${pair.vertex}`);
    request_render();
  });

  // Pin to line (plan-sketchpad-pin-vertex-to-selected-line.md): with exactly one
  // vertex and one stroke selected, pins the vertex to the nearest point of the
  // stroke; a vertex pinned elsewhere moves its pin (Q7). Hidden when the pair is
  // refused. Both stay selected (Q6).
  const pin_to_line_button = document.getElementById("pin_to_line_button") as HTMLButtonElement;
  function selected_pin_to_line_pair(): { vertex: VertexId; stroke_id: StrokeId } | null {
    if (edit_state === null || extra_selection.length > 0 || selected_vertex === null || extra_vertex !== null) return null;
    if (!vertex_can_pin_to_stroke(tablet_document, selected_vertex, edit_state.stroke_id)) return null;
    // A patch's sub-curve ends at its current pin (Q7).
    if (pin_by_vertex(tablet_document, selected_vertex) !== null && pin_is_locked(tablet_document, selected_vertex)) return null;
    return { vertex: selected_vertex, stroke_id: edit_state.stroke_id };
  }
  pin_to_line_button.addEventListener("click", () => {
    const pair = selected_pin_to_line_pair();
    if (pair === null) return;
    begin_history_step(history, tablet_document);
    pin_vertex_to_stroke(tablet_document, pair.vertex, pair.stroke_id);
    end_history_step(history, tablet_document, `pin vertex ${pair.vertex} to line ${pair.stroke_id}`);
    request_render();
  });

  // Midline (plan-sketchpad-midline.md Q72): toggles the midline flag on the
  // selected vertex or stroke; enforce_midline in request_render snaps it to
  // x = 0 at once. Setting the flag drops any pin competing for the same
  // position (Q71): the vertex's own, or the endpoints' of a stroke.
  const midline_button = document.getElementById("midline_button") as HTMLButtonElement;
  function refresh_midline_button_armed(): void {
    const vertex = sole_selected_vertex();
    const flagged = vertex !== null
      ? vertex_by_id(tablet_document, vertex).midline === true
      : edit_state !== null && stroke_by_id(tablet_document, edit_state.stroke_id).midline === true;
    midline_button.classList.toggle("armed", flagged);
  }
  midline_button.addEventListener("click", () => {
    let flagged: { midline?: boolean };
    let claimed_vertices: VertexId[];
    const vertex = sole_selected_vertex();
    if (vertex !== null) {
      flagged = vertex_by_id(tablet_document, vertex);
      claimed_vertices = [vertex];
    } else if (edit_state !== null) {
      const stroke = stroke_by_id(tablet_document, edit_state.stroke_id);
      flagged = stroke;
      claimed_vertices = [stroke.p0_vertex, stroke.p3_vertex];
    } else {
      return;
    }
    begin_history_step(history, tablet_document);
    const midline_target = vertex !== null ? `vertex ${vertex}` : `stroke ${edit_state!.stroke_id}`;
    let label = `set midline ${midline_target}`;
    if (flagged.midline === true) {
      delete flagged.midline;
      label = `clear midline ${midline_target}`;
    } else {
      flagged.midline = true;
      tablet_document.vertex_pins = tablet_document.vertex_pins.filter((pin) => !claimed_vertices.includes(pin.vertex));
    }
    end_history_step(history, tablet_document, label);
    request_render();
  });

  // Width panel: the C++ Selection panel's radii controls (game_main.cpp, search
  // "raw radii") for the selected stroke. `width` scales the stroke's own profile
  // (keeps its taper), `taper` picks a preset end shape at that width, `raw`
  // edits the four multipliers directly. One history entry per slider drag or
  // per picked value; consecutive edits of one stroke merge into one entry.
  const width_button = document.getElementById("width_button") as HTMLButtonElement;
  const width_panel = document.getElementById("width_panel") as HTMLDivElement;
  const WIDTH_SLIDER_MIN = 0.1, WIDTH_SLIDER_MAX = 4; // log scale, as the C++ slider
  const TAPER_PRESETS: { name: string; radii: StrokeRadii }[] = [
    { name: "flat", radii: [1, 1, 1, 1] },
    { name: "default (.25 1 1 .25)", radii: [0.25, 1, 1, 0.25] },
    { name: "tip in (.25 1 1 1)", radii: [0.25, 1, 1, 1] },
    { name: "tip out (1 1 1 .25)", radii: [1, 1, 1, 0.25] },
  ];
  const width_panel_empty = document.createElement("div");
  width_panel_empty.className = "empty";
  width_panel_empty.textContent = "(no line selected)";
  const width_panel_controls = document.createElement("div");
  width_panel_controls.className = "controls";
  const width_slider = document.createElement("input");
  width_slider.type = "range";
  width_slider.min = "0";
  width_slider.max = "1";
  width_slider.step = "0.001";
  const width_readout = document.createElement("span");
  const taper_select = document.createElement("select");
  for (const preset of TAPER_PRESETS) taper_select.appendChild(new Option(preset.name));
  taper_select.appendChild(new Option("custom"));
  const raw_inputs: HTMLInputElement[] = [0, 1, 2, 3].map(() => {
    const input = document.createElement("input");
    input.type = "number";
    input.min = "0";
    input.max = "8";
    input.step = "0.05";
    return input;
  });
  function labelled_row(label: string, ...controls: HTMLElement[]): HTMLLabelElement {
    const row = document.createElement("label");
    row.append(label, ...controls);
    return row;
  }
  width_panel_controls.append(
    labelled_row("width", width_slider, width_readout),
    labelled_row("taper", taper_select),
    labelled_row("raw", ...raw_inputs),
  );
  width_panel.append(width_panel_empty, width_panel_controls);

  function width_of_radii(radii: StrokeRadii): number {
    return Math.max(radii[0], radii[1], radii[2], radii[3]);
  }
  function radii_equal(a: StrokeRadii, b: StrokeRadii): boolean {
    return a.every((value, index) => Math.abs(value - b[index]) < 1e-6);
  }
  function refresh_width_panel(): void {
    const has_stroke = edit_state !== null;
    width_panel_empty.style.display = has_stroke ? "none" : "";
    width_panel_controls.style.display = has_stroke ? "" : "none";
    if (!has_stroke) return;
    const radii = stroke_radii(stroke_by_id(tablet_document, edit_state!.stroke_id));
    const width = width_of_radii(radii);
    // Don't fight the control being dragged/typed into.
    const active = document.activeElement;
    if (active !== width_slider) {
      const clamped = Math.min(Math.max(width, WIDTH_SLIDER_MIN), WIDTH_SLIDER_MAX);
      width_slider.value = String(Math.log(clamped / WIDTH_SLIDER_MIN) / Math.log(WIDTH_SLIDER_MAX / WIDTH_SLIDER_MIN));
    }
    width_readout.textContent = width.toFixed(2);
    const preset_index = TAPER_PRESETS.findIndex((preset) => width > 0 && radii_equal(radii, preset.radii.map((r) => r * width) as StrokeRadii));
    taper_select.selectedIndex = preset_index >= 0 ? preset_index : TAPER_PRESETS.length;
    raw_inputs.forEach((input, index) => {
      if (active !== input) input.value = radii[index].toFixed(2);
    });
  }
  // A slider drag is many `input` events and one `change`; the step opens on
  // the first input and closes on the change.
  let width_edit_pending = false;
  function apply_radii_to_selected_stroke(radii: StrokeRadii): void {
    if (edit_state === null) return;
    if (!width_edit_pending) {
      begin_history_step(history, tablet_document);
      width_edit_pending = true;
    }
    const stroke = stroke_by_id(tablet_document, edit_state.stroke_id);
    if (radii_equal(radii, DEFAULT_STROKE_RADII)) delete stroke.radii;
    else stroke.radii = radii;
    request_render();
  }
  function commit_radii_edit(): void {
    if (!width_edit_pending || edit_state === null) return;
    width_edit_pending = false;
    end_history_step(history, tablet_document, `set radii stroke ${edit_state.stroke_id}`, true);
    request_render();
  }
  width_slider.addEventListener("input", () => {
    if (edit_state === null) return;
    const new_width = WIDTH_SLIDER_MIN * Math.pow(WIDTH_SLIDER_MAX / WIDTH_SLIDER_MIN, Number(width_slider.value));
    const radii = stroke_radii(stroke_by_id(tablet_document, edit_state.stroke_id));
    const width = width_of_radii(radii);
    const scaled = width > 0 ? radii.map((r) => r * new_width / width) : [1, 1, 1, 1].map((r) => r * new_width);
    apply_radii_to_selected_stroke(scaled as StrokeRadii);
  });
  width_slider.addEventListener("change", commit_radii_edit);
  taper_select.addEventListener("change", () => {
    if (edit_state === null || taper_select.selectedIndex >= TAPER_PRESETS.length) return;
    const width = width_of_radii(stroke_radii(stroke_by_id(tablet_document, edit_state.stroke_id)));
    apply_radii_to_selected_stroke(TAPER_PRESETS[taper_select.selectedIndex].radii.map((r) => r * width) as StrokeRadii);
    commit_radii_edit();
  });
  raw_inputs.forEach((input) => {
    input.addEventListener("input", () => {
      const raw = raw_inputs.map((each) => Number(each.value));
      if (raw.some((value) => !Number.isFinite(value) || value < 0)) return;
      apply_radii_to_selected_stroke(raw as StrokeRadii);
    });
    input.addEventListener("change", commit_radii_edit);
  });
  width_button.addEventListener("click", () => {
    width_panel.classList.toggle("open");
    width_button.classList.toggle("armed", width_panel.classList.contains("open"));
    refresh_width_panel();
  });

  // Keyboard nudge of the selected vertex (plan-sketchpad-vertex-nudge-link.md
  // Q76-Q78): h/l along the camera's right, j/k along its up, i/o along forward
  // (i = in, away from the viewer). Step is screen pixels at the pivot depth,
  // Shift multiplies by 5; a run of nudges on one vertex is one history entry
  // (Khoa 2026-09-27, supersedes plan Q78's one-per-key). A midline vertex is
  // snapped back by the pass in request_render, so it just doesn't leave x = 0;
  // a pinned vertex is left alone (it only slides on its host, by a drag). The
  // move goes through move_vertex, so the attached curves turn with the vertex.
  const NUDGE_STEP_PIXELS = 4;
  const NUDGE_SHIFT_MULTIPLIER = 5;
  function nudge_selected_vertex(right_steps: number, up_steps: number, forward_steps: number, shift: boolean): void {
    if (selected_vertex === null || pen_down_screen !== null) return;
    if (edit_state !== null) return; // a stroke selected too owns the keys
    if (pin_by_vertex(tablet_document, selected_vertex) !== null) return;
    const basis = camera_basis(camera);
    const step = NUDGE_STEP_PIXELS * (shift ? NUDGE_SHIFT_MULTIPLIER : 1) * camera_world_units_per_pixel(camera, canvas.clientHeight);
    const delta = v3_add(
      v3_add(v3_scale(basis.right, right_steps * step), v3_scale(basis.up, up_steps * step)),
      v3_scale(basis.forward, forward_steps * step),
    );
    begin_history_step(history, tablet_document);
    move_vertex(tablet_document, selected_vertex, v3_add(vertex_position(tablet_document, selected_vertex), delta));
    end_history_step(history, tablet_document, `nudge vertex ${selected_vertex}`, true); // a run of nudges = one entry
    request_render();
  }
  // Keyboard nudge of the selected handle (plan-sketchpad-selection-revamp.md
  // Q5/Q13): same keys, step and Shift as the vertex nudge, through the pen-drag
  // path of that handle (edit_nudge_handle), so the smooth neighbour follows.
  // A nudge swings (tilts the line's plane): a key step is an exact, known move,
  // so it needs no plane to hold it. `in_plane` (Ctrl held) keeps the handle in
  // the line's plane instead. The sticky swing button is for pen drags only. A
  // run of nudges on one handle is one history entry.
  function nudge_selected_handle(right_steps: number, up_steps: number, forward_steps: number, shift: boolean, in_plane: boolean): void {
    if (edit_state === null || edit_state.selected_handle === null || pen_down_screen !== null) return;
    const handle = edit_state.selected_handle;
    const step_pixels = NUDGE_STEP_PIXELS * (shift ? NUDGE_SHIFT_MULTIPLIER : 1);
    begin_history_step(history, tablet_document);
    edit_nudge_handle(
      edit_state, tablet_document, camera, canvas, handle,
      { x: right_steps * step_pixels, y: -up_steps * step_pixels },
      forward_steps * step_pixels * camera_world_units_per_pixel(camera, canvas.clientHeight),
      in_plane ? "plane" : "swing",
    );
    end_history_step(history, tablet_document, `nudge handle ${handle} of stroke ${edit_state.stroke_id}`, true);
    request_render();
  }
  window.addEventListener("keydown", (event) => {
    // The only modifier chord the sketchpad takes: Ctrl/cmd + a nudge key while a
    // handle is selected = nudge inside the line's plane (a bare nudge swings).
    // Every other chord is the browser's.
    const modifier_held = event.ctrlKey || event.metaKey || event.altKey;
    const handle_selected = edit_state !== null && edit_state.selected_handle !== null;
    const in_plane = (event.ctrlKey || event.metaKey) && !event.altKey;
    if (modifier_held && !(handle_selected && in_plane)) return;
    const nudge = (right_steps: number, up_steps: number, forward_steps: number) => {
      if (handle_selected) {
        event.preventDefault(); // Ctrl + h/l/j/k/o are browser shortcuts
        nudge_selected_handle(right_steps, up_steps, forward_steps, event.shiftKey, in_plane);
      } else {
        nudge_selected_vertex(right_steps, up_steps, forward_steps, event.shiftKey);
      }
    };
    switch (event.key.toLowerCase()) {
      case "h": nudge(-1, 0, 0); break;
      case "l": nudge(1, 0, 0); break;
      case "j": nudge(0, -1, 0); break;
      case "k": nudge(0, 1, 0); break;
      case "i": nudge(0, 0, 1); break;
      case "o": nudge(0, 0, -1); break;
      // Delete / Backspace (a Mac keyboard's "delete") = the del button, lock
      // included (plan-patch-subcurve-boundary.md Q12). Not while typing a name
      // or a width.
      case "delete":
      case "backspace":
        if (modifier_held || document.activeElement instanceof HTMLInputElement) return;
        event.preventDefault();
        delete_button.click();
        break;
      // Escape = unselect all (lines, handle, vertices, patch) and disarm any
      // armed tool. Not while typing a name or a width.
      case "escape":
        if (modifier_held || document.activeElement instanceof HTMLInputElement) return;
        clear_selection_after_history_jump();
        break;
    }
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
  // selected stroke may not survive the snapshot.
  function clear_selection_after_history_jump(): void {
    edit_state = null;
    extra_selection = [];
    selected_vertex = null;
    extra_vertex = null;
    selected_patch = null;
    set_armed_tool(null);
    request_render();
  }
  function perform_undo(): void {
    if (undo(history, tablet_document)) clear_selection_after_history_jump();
  }
  function perform_redo(): void {
    if (redo(history, tablet_document)) clear_selection_after_history_jump();
  }
  function perform_history_jump(position: number): void {
    if (jump_history(history, tablet_document, position)) clear_selection_after_history_jump();
  }

  // What the current pen gesture did, for the history panel; branches of pen-up
  // overwrite it, the fallback covers anything unlabelled.
  let pen_history_label = "edit";
  let pen_history_merge = false; // true = a repeat of the previous gesture extends its entry

  attach_gestures(canvas, camera, {
    on_pen_down: (position) => {
      begin_history_step(history, tablet_document);
      pen_history_label = "edit";
      pen_history_merge = false;
      pen_down_screen = position;
      pen_max_displacement_pixels = 0;
      pen_orbit_last_screen = null;
      hover_screen = null; // nothing is hot while the pen is down
      pen_ray_screen = position;
      if (armed_tool === "line") {
        line_state = line_pen_down(tablet_document, camera, position, canvas, pickable_layers(), line_start_vertex);
        update_preview_line();
      } else if (edit_state !== null && armed_tool === null) {
        // Consumed only when the pen lands on a control point or pin of the selection; otherwise orbit.
        if (!edit_pen_down(edit_state, tablet_document, camera, position, canvas)) {
          pen_orbit_last_screen = position;
        }
      } else if (selected_vertex !== null && armed_tool === null && pick_vertex(tablet_document, camera, position, canvas, pickable_layers()) === selected_vertex) {
        // Landing on the selected vertex drags it on the camera plane.
        selected_vertex_drag_last_screen = position;
      } else {
        // No selection, or a pick tool armed (pure tap tool): bare drags orbit.
        pen_orbit_last_screen = position;
      }
      // A point is small, so landing on one already says "this point": a few
      // pixels of jitter is still a tap, anything more is a drag of it.
      const pen_landed_on_point = selected_vertex_drag_last_screen !== null
        || (edit_state !== null && (edit_state.dragging !== null || edit_state.dragging_pin !== null));
      pen_tap_max_movement_pixels = pen_landed_on_point ? 4 : TAP_MAX_MOVEMENT_PIXELS;
      request_render();
    },
    on_pen_move: (position, event) => {
      pen_ray_screen = position;
      if (pen_down_screen !== null) {
        pen_max_displacement_pixels = Math.max(
          pen_max_displacement_pixels,
          Math.hypot(position.x - pen_down_screen.x, position.y - pen_down_screen.y),
        );
      }
      if (armed_tool === "line") {
        if (line_state !== null) {
          line_pen_move(line_state, tablet_document, camera, position, canvas, pickable_layers());
          update_preview_line();
        }
      } else if (pen_orbit_last_screen !== null) {
        pen_orbit(position);
      } else if (pen_max_displacement_pixels < pen_tap_max_movement_pixels) {
        // Still a tap: nothing moves (a tap only selects). The drag's last
        // screen position stays at the pen-down point, so the first real move
        // catches up with the pen.
      } else if (selected_vertex_drag_last_screen !== null && selected_vertex !== null) {
        const pin = pin_by_vertex(tablet_document, selected_vertex);
        if (pin !== null) {
          // A pinned vertex only slides along its host curve, same as a pin drag
          // in edit mode; a free move would be snapped back by the pin anyway.
          pin.t = nearest_t_on_stroke_screen(tablet_document, pin.host_stroke, camera, position, canvas).t;
          const host_points = stroke_control_points(stroke_by_id(tablet_document, pin.host_stroke), tablet_document);
          move_vertex(tablet_document, selected_vertex, bezier_point(host_points, pin.t));
        } else {
          const drag_delta = camera_plane_drag(camera, selected_vertex_drag_last_screen, position, canvas);
          move_vertex(tablet_document, selected_vertex, v3_add(vertex_position(tablet_document, selected_vertex), drag_delta));
        }
        selected_vertex_drag_last_screen = position;
      } else if (edit_state !== null) {
        // Ctrl/cmd held during a handle drag = swing for that drag, without
        // reaching for the swing button; the button stays the sticky mode.
        const drag_handle_mode: HandleMode = event.ctrlKey || event.metaKey ? "swing" : handle_mode;
        edit_pen_move(edit_state, tablet_document, camera, position, canvas, drag_handle_mode);
      }
      request_render();
    },
    on_pen_up: (position, event) => {
      const multi = event.ctrlKey || event.metaKey; // ctrl/cmd-tap extends the selection (Q1)
      if (armed_tool === "line") {
        line_mode_pen_up();
      } else if (edit_state !== null) {
        edit_mode_pen_up(position, multi);
      } else if (pen_max_displacement_pixels < pen_tap_max_movement_pixels) {
        select_tap_target(pick_tap_target(position), multi);
      } else if (selected_vertex_drag_last_screen !== null) {
        // A dragged vertex welds into a vertex it was released on, same as an
        // endpoint drag of a selected stroke; the survivor takes the selection.
        // No pin on release.
        pen_history_label = `move vertex ${selected_vertex}`;
        const weld_target = selected_vertex_weld_target();
        if (weld_target !== null && merge_vertex_if_near_another(tablet_document, selected_vertex!, pickable_layers())) {
          pen_history_label = `weld vertex ${selected_vertex} into ${weld_target}`;
          selected_vertex = weld_target;
          extra_vertex = null;
        }
      }
      pen_down_screen = null;
      pen_orbit_last_screen = null;
      selected_vertex_drag_last_screen = null;
      hover_screen = position;
      end_history_step(history, tablet_document, pen_history_label, pen_history_merge);
      request_render();
    },
    on_pen_hover: (position) => {
      hover_screen = position;
      pen_ray_screen = position;
      request_render();
    },
    on_undo_tap: perform_undo,
    on_redo_tap: perform_redo,
  }, () => {
    request_render();
  });

  // Name (or rename; empty clears) the selected vertex or stroke. A named vertex
  // is a landmark and survives garbage collection.
  const name_button = document.getElementById("name_button") as HTMLButtonElement;
  name_button.addEventListener("click", () => {
    const vertex = sole_selected_vertex();
    const named = vertex !== null
      ? vertex_by_id(tablet_document, vertex)
      : edit_state !== null ? stroke_by_id(tablet_document, edit_state.stroke_id) : null;
    if (named === null) return;
    const what = vertex !== null ? "Vertex" : "Line";
    const name = window.prompt(`${what} name (empty to clear):`, named.name ?? "");
    if (name === null) return;
    begin_history_step(history, tablet_document);
    if (name.trim() === "") delete named.name;
    else named.name = name.trim();
    end_history_step(history, tablet_document, `name ${what.toLowerCase()} ${named.id} "${name.trim()}"`);
    request_render();
  });

  // Delete the whole selection (patches built on any of it go with it). For a
  // vertex: clear its name, then garbage-collect — a vertex some stroke still
  // uses stays (unnamed), a landmark goes.
  const delete_button = document.getElementById("delete_button") as HTMLButtonElement;
  // Locks (plan-patch-subcurve-boundary.md Q3/Q7/Q8): a patch owns its
  // boundary, so while one exists its strokes can't be deleted or joined and
  // its junction pins can't be unpinned -- the buttons go disabled with the
  // reason in their tooltip, and the handlers refuse the same way (the Delete
  // key goes through the handler). Delete the patch first. Re-checked every
  // frame from request_render, like the other button states.
  const DELETE_PATCH_FIRST = "delete the patch first";
  function selected_strokes(): StrokeId[] {
    return edit_state === null ? [] : [edit_state.stroke_id, ...extra_selection];
  }
  function delete_is_locked(): boolean {
    return selected_strokes().some((id) => stroke_bounds_a_patch(tablet_document, id));
  }
  function join_is_locked(): boolean {
    if (edit_state === null) return false;
    // The kept (primary) stroke is reshaped by a join, which would move every
    // pin riding it -- refused if any of those pins bounds a patch (Q7).
    return delete_is_locked() || pins_on_stroke(tablet_document, edit_state.stroke_id).some((pin) => pin_is_locked(tablet_document, pin.vertex));
  }
  function unpin_is_locked(): boolean {
    const pin = selected_vertex_pin();
    return pin !== null && pin_is_locked(tablet_document, pin.vertex);
  }
  // A button with nothing to act on in the current selection is hidden. A button
  // whose action a patch blocks stays, greyed out, with the reason as its tooltip;
  // the button's own tooltip from the page comes back once it is usable.
  const own_button_titles = new Map<HTMLButtonElement, string>();
  function set_button_availability(button: HTMLButtonElement, selection_fits: boolean, lock_reason: string | null): void {
    if (!own_button_titles.has(button)) own_button_titles.set(button, button.title);
    button.hidden = !selection_fits;
    button.disabled = lock_reason !== null;
    button.title = lock_reason ?? own_button_titles.get(button)!;
  }
  function refresh_lock_buttons(): void {
    const line_selected = edit_state !== null;
    const line_or_vertex_selected = edit_state !== null || selected_vertex !== null;
    const two_lines_selected = single_extra_stroke() !== null;

    let delete_lock_reason: string | null = null;
    if (selected_patch === null && selected_vertex === null && delete_is_locked()) {
      delete_lock_reason = `the selected line bounds a patch, ${DELETE_PATCH_FIRST}`;
    }
    if (selected_patch === null && selected_vertex !== null && pin_is_locked(tablet_document, selected_vertex)) {
      delete_lock_reason = `this vertex bounds a patch, ${DELETE_PATCH_FIRST}`;
    }
    // del does nothing while a vertex and a line are selected together (Q2).
    const vertex_and_line_selected = edit_state !== null && selected_vertex !== null;
    set_button_availability(delete_button, selected_patch !== null || (line_or_vertex_selected && !vertex_and_line_selected), delete_lock_reason);
    set_button_availability(pin_to_line_button, selected_pin_to_line_pair() !== null, null);
    const detach_end_pair = selected_detach_end_pair();
    set_button_availability(
      detach_end_button, detach_end_pair !== null,
      detach_end_pair !== null && stroke_bounds_a_patch(tablet_document, detach_end_pair.stroke_id)
        ? `the selected line bounds a patch, ${DELETE_PATCH_FIRST}` : null,
    );
    set_button_availability(
      join_button, two_lines_selected, join_is_locked() ? `one of these lines bounds a patch, ${DELETE_PATCH_FIRST}` : null,
    );
    set_button_availability(
      pin_button, line_selected || selected_vertex_pin() !== null, unpin_is_locked() ? `this vertex bounds a patch, ${DELETE_PATCH_FIRST}` : null,
    );
    set_button_availability(split_button, line_selected, null);
    set_button_availability(smooth_button, two_lines_selected, null);
    set_button_availability(straighten_button, line_selected, null);
    set_button_availability(patch_button, edit_state !== null && extra_selection.length > 0, null);
    set_button_availability(midline_button, line_or_vertex_selected, null);
    set_button_availability(name_button, line_or_vertex_selected, null);
  }
  delete_button.addEventListener("click", () => {
    if (selected_patch !== null) {
      begin_history_step(history, tablet_document);
      const deleted = tablet_document.patches[selected_patch];
      tablet_document.patches.splice(selected_patch, 1);
      end_history_step(history, tablet_document, `delete patch [${deleted.strokes.join(" ")}]`);
      selected_patch = null;
      request_render();
      return;
    }
    if (edit_state !== null && selected_vertex !== null) return; // which of the two is meant is unclear (Q2)
    if (selected_vertex !== null) {
      if (pin_is_locked(tablet_document, selected_vertex)) return; // a patch's sub-curve ends here
      begin_history_step(history, tablet_document);
      // Deleting a pinned vertex takes its pin along; without that the pin
      // keeps the vertex alive and delete does nothing.
      const deleted_vertex = selected_vertex;
      tablet_document.vertex_pins = tablet_document.vertex_pins.filter((pin) => pin.vertex !== deleted_vertex);
      delete vertex_by_id(tablet_document, selected_vertex).name;
      garbage_collect_vertices(tablet_document);
      end_history_step(history, tablet_document, `delete vertex ${selected_vertex}`);
      selected_vertex = null;
      extra_vertex = null;
      request_render();
      return;
    }
    if (edit_state === null || delete_is_locked()) return;
    begin_history_step(history, tablet_document);
    const deleted_strokes = selected_strokes();
    for (const stroke_id of deleted_strokes) delete_stroke(tablet_document, stroke_id);
    end_history_step(history, tablet_document, `delete strokes [${deleted_strokes.join(" ")}]`);
    edit_state = null;
    extra_selection = [];
    selected_vertex = null;
    extra_vertex = null;
    set_armed_tool(null);
    request_render();
  });

  // NOTE(kv): the toolbar "clear" button was removed on 2026-09-28 (Khoa: one tap next to
  // ref/surf must not be able to erase a document); delete strokes one by one, or undo.

  // Snap the camera to the nearest frontal/profile/back view (desktop key A);
  // snapping again toggles back to the previous view. previous starts at
  // profile so the very first snap from frontal has somewhere to toggle to.
  const camera_snap_state: CameraSnapState = { previous_snap_yaw: Math.PI / 2, current_snap_yaw: 0 };
  // Mirror toggle, remembered per page. The frame is the mirror's border; it
  // sits over that corner of the canvas and takes the pointer, so the pen and
  // fingers do nothing there.
  const mirror_button = document.getElementById("mirror_button") as HTMLButtonElement;
  const mirror_storage_key = `${setup.storage_key_prefix}_mirror`;
  const mirror_frame = document.createElement("div");
  mirror_frame.style.cssText = "position: fixed; box-sizing: border-box; border: 1px solid #aaa; touch-action: none;";
  canvas.after(mirror_frame);
  function place_mirror_frame(): void {
    const rectangle = mirror_rectangle(canvas.clientWidth, canvas.clientHeight);
    mirror_frame.style.left = `${rectangle.left}px`;
    mirror_frame.style.top = `${rectangle.top}px`;
    mirror_frame.style.width = `${rectangle.size}px`;
    mirror_frame.style.height = `${rectangle.size}px`;
    mirror_frame.style.display = mirror_visible ? "block" : "none";
    mirror_button.classList.toggle("armed", mirror_visible);
  }
  mirror_visible = localStorage.getItem(mirror_storage_key) === "1";
  place_mirror_frame();
  window.addEventListener("resize", place_mirror_frame);
  mirror_button.addEventListener("click", () => {
    mirror_visible = !mirror_visible;
    localStorage.setItem(mirror_storage_key, mirror_visible ? "1" : "0");
    place_mirror_frame();
    request_render();
  });

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

  // History panel, a port of the C++ app's "History" ImGui panel: one row per
  // entry, `>` marks the entry the document equals, rows past it (undone) are
  // dimmed, tapping a row jumps there. Rebuilt on every request_render while open.
  const history_button = document.getElementById("history_button") as HTMLButtonElement;
  const history_panel = document.getElementById("history_panel") as HTMLDivElement;
  history_button.addEventListener("click", () => {
    history_panel.classList.toggle("open");
    history_button.classList.toggle("armed", history_panel.classList.contains("open"));
    refresh_history_panel();
  });
  function refresh_history_panel(): void {
    undo_button.disabled = history.position <= 0;
    redo_button.disabled = history.position >= history.entries.length - 1;
    if (!history_panel.classList.contains("open")) return;
    history_panel.replaceChildren();
    if (history.entries.length === 0) {
      const empty = document.createElement("div");
      empty.className = "empty";
      empty.textContent = "(no edits yet)";
      history_panel.appendChild(empty);
      return;
    }
    let current_row: HTMLButtonElement | null = null;
    history.entries.forEach((entry, position) => {
      const row = document.createElement("button");
      row.textContent = `${position === history.position ? ">" : " "} ${entry.label}`;
      if (position === history.position) { row.classList.add("current"); current_row = row; }
      if (position > history.position) row.classList.add("undone");
      row.addEventListener("click", () => perform_history_jump(position));
      history_panel.appendChild(row);
    });
    current_row!.scrollIntoView({ block: "nearest" });
  }
  window.addEventListener("keydown", (event) => {
    if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === "z") {
      event.preventDefault();
      if (event.shiftKey) perform_redo();
      else perform_undo();
    }
  });

  // Reference mesh on/off: the "ref" button, or the q key (same key as the desktop app).
  const reference_button = document.getElementById("reference_button") as HTMLButtonElement;
  function toggle_reference_visible(): void {
    reference_visible = !reference_visible;
    reference_button.classList.toggle("armed", reference_visible);
    request_render();
  }
  reference_button.addEventListener("click", toggle_reference_visible);
  reference_button.classList.toggle("armed", reference_visible);
  window.addEventListener("keydown", (event) => {
    if (event.key === "q" && !event.repeat && !event.ctrlKey && !event.metaKey) toggle_reference_visible();
  });
  // Eyeballs with the reference (Q9): the mesh is fetched on the first arm only.
  const eyeball_button = document.getElementById("eyeball_button") as HTMLButtonElement | null;
  if (eyeball_button !== null && setup.load_eyeball_mesh !== undefined) {
    const load_eyeball_mesh = setup.load_eyeball_mesh;
    eyeball_button.addEventListener("click", () => {
      eyeball_visible = !eyeball_visible;
      eyeball_button.classList.toggle("armed", eyeball_visible);
      if (eyeball_visible && eyeball_mesh === null) {
        void load_eyeball_mesh().then((mesh) => {
          eyeball_mesh = mesh;
          request_render();
        });
      }
      request_render();
    });
  }
  // Reference opacity slider (Q6), optional per page.
  const reference_alpha_slider = document.getElementById("reference_alpha") as HTMLInputElement | null;
  if (reference_alpha_slider !== null) {
    reference_alpha_slider.addEventListener("input", () => {
      reference_alpha = Number(reference_alpha_slider.value);
      request_render();
    });
  }
  // Clip plane (Q5), optional per page: the slider value is the plane offset in
  // world units (WORLD_PER_MM * mm, so +-1 spans the whole head); at its max the
  // plane is off. A slider rather than a pen drag, so it never competes with the
  // orbit.
  const clip_offset_slider = document.getElementById("clip_offset") as HTMLInputElement | null;
  if (clip_offset_slider !== null) {
    clip_offset_slider.addEventListener("input", () => {
      clip_plane.offset = Number(clip_offset_slider.value);
      clip_plane.enabled = clip_plane.offset < Number(clip_offset_slider.max);
      request_render();
    });
  }

  const surface_button = document.getElementById("surface_button") as HTMLButtonElement;
  surface_button.addEventListener("click", () => {
    surface_colored = !surface_colored;
    surface_button.classList.toggle("armed", surface_colored);
    request_render();
  });
  surface_button.classList.toggle("armed", surface_colored);

  // Layer bar (plan-skin-over-skull-study.md): per layer a name button (tap =
  // active layer), a lock toggle and a hide toggle. Pages without a `#layer_bar`
  // keep the setup's layers for good. Any toggle drops the selection, since the
  // selected item may just have become unpickable or invisible.
  const layer_bar = document.getElementById("layer_bar");
  if (layer_bar !== null) {
    const name_buttons = new Map<Layer, HTMLButtonElement>();
    const lock_buttons = new Map<Layer, HTMLButtonElement>();
    const hide_buttons = new Map<Layer, HTMLButtonElement>();
    // A layer the page hides for good gets no row.
    const layers_with_a_row = ALL_LAYERS.filter((layer) => !setup.hidden_layers.includes(layer));
    function refresh_layer_bar(): void {
      for (const layer of layers_with_a_row) {
        name_buttons.get(layer)!.classList.toggle("armed", layer === active_layer);
        lock_buttons.get(layer)!.classList.toggle("armed", locked_layers.has(layer));
        hide_buttons.get(layer)!.classList.toggle("armed", hidden_layers.has(layer));
      }
    }
    function drop_selection_after_layer_change(): void {
      edit_state = null;
      extra_selection = [];
      selected_vertex = null;
      extra_vertex = null;
      selected_patch = null;
      set_armed_tool(null);
      refresh_layer_bar();
      request_render();
    }
    function make_layer_button(label: string, title: string, on_click: () => void): HTMLButtonElement {
      const button = document.createElement("button");
      button.textContent = label;
      button.title = title;
      button.addEventListener("click", on_click);
      layer_bar!.appendChild(button);
      return button;
    }
    for (const layer of layers_with_a_row) {
      const row = document.createElement("div");
      row.className = "layer_row";
      layer_bar.appendChild(row);
      name_buttons.set(layer, make_layer_button(layer, `draw on the ${layer} layer`, () => {
        active_layer = layer;
        refresh_layer_bar();
      }));
      lock_buttons.set(layer, make_layer_button("lock", `${layer}: pen and taps ignore it`, () => {
        if (locked_layers.has(layer)) locked_layers.delete(layer);
        else locked_layers.add(layer);
        drop_selection_after_layer_change();
      }));
      // A layer the page locks for good stays locked: no unlocking, no drawing on it.
      if (setup.locked_layers.includes(layer)) {
        name_buttons.get(layer)!.disabled = true;
        lock_buttons.get(layer)!.disabled = true;
        lock_buttons.get(layer)!.title = `${layer}: locked for good on this page`;
      }
      hide_buttons.set(layer, make_layer_button("hide", `${layer}: not drawn`, () => {
        if (hidden_layers.has(layer)) hidden_layers.delete(layer);
        else hidden_layers.add(layer);
        drop_selection_after_layer_change();
      }));
      // The three buttons of a layer sit on one row.
      for (const button of [name_buttons.get(layer)!, lock_buttons.get(layer)!, hide_buttons.get(layer)!]) row.appendChild(button);
    }
    refresh_layer_bar();
  }

  // Docs panel: lists server documents to switch between, plus "new…" (prompt
  // for a name; unknown names start empty) and "rename…" for the current one.
  // Autosave keeps targeting whichever document is current. A page tied to one
  // document (setup.can_switch_documents = false) only sees that document's name.
  const DOCUMENT_NAME_PATTERN = /^[A-Za-z0-9_-]{1,64}$/; // mirrors is_safe_document_name in vite.config.ts

  function prompt_document_name(message: string, initial: string): string | null {
    const name = window.prompt(message, initial);
    if (name === null) return null;
    if (!DOCUMENT_NAME_PATTERN.test(name)) {
      window.alert("Bad name — letters, digits, - and _ only.");
      return null;
    }
    return name;
  }
  const docs_button = document.getElementById("docs_button") as HTMLButtonElement;
  const docs_panel = document.getElementById("docs_panel") as HTMLDivElement;

  async function switch_to_document_and_rerender(name: string): Promise<void> {
    docs_panel.classList.remove("open");
    if (persistence.is_in_conflict) { // leaving would drop this tab's unsaved edits
      window.alert("This document changed elsewhere. Choose 'load newer' or 'fork' first.");
      return;
    }
    edit_state = null;
    extra_selection = [];
    selected_vertex = null;
    extra_vertex = null;
    selected_patch = null;
    set_armed_tool(null);
    clear_history(history); // history is per-document (Q5)
    await switch_document(persistence, tablet_document, camera, name);
    request_render();
  }

  async function open_docs_panel(): Promise<void> {
    docs_panel.replaceChildren();
    if (!setup.can_switch_documents) {
      const current_button = document.createElement("button");
      current_button.textContent = persistence.current_document_name;
      current_button.classList.add("current");
      current_button.disabled = true;
      docs_panel.appendChild(current_button);
      docs_panel.classList.add("open");
      return;
    }
    const entries = await list_documents_from_server();
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
        const name = prompt_document_name("Document name (letters, digits, - and _):", "");
        if (name !== null) void switch_to_document_and_rerender(name);
      });
      docs_panel.appendChild(new_button);
      const rename_button = document.createElement("button");
      rename_button.textContent = "rename…";
      rename_button.addEventListener("click", () => {
        const name = prompt_document_name("New document name:", persistence.current_document_name);
        if (name === null) return;
        void rename_document(persistence, tablet_document, camera, name).then((renamed) => {
          if (renamed) docs_panel.classList.remove("open");
        });
      });
      docs_panel.appendChild(rename_button);
    }
    docs_panel.classList.add("open");
  }

  // Back to the menu: flush the pending autosave first, or a stroke drawn in the last
  // ~2 s is lost with the tab.
  document.getElementById("pages_button")!.addEventListener("click", () => {
    void flush_autosave(persistence, tablet_document, camera).then(() => {
      window.location.href = "/";
    });
  });
  // Tab closed / reloaded / navigated by other means: same flush, keepalive so the
  // request outlives the page.
  window.addEventListener("pagehide", () => {
    remember_camera(persistence, camera);
    void flush_autosave(persistence, tablet_document, camera, true);
  });
  // The exit flush is not guaranteed to arrive (a keepalive body is limited to 64 KB),
  // so leaving with unsaved edits asks first.
  window.addEventListener("beforeunload", (event) => {
    if (has_unsaved_edits(persistence)) event.preventDefault();
  });

  // Changes made elsewhere (another tab or page, a script, the agent): the file on the
  // server is no longer the revision this tab is based on. A tab without unsaved edits
  // takes the file over; a tab with unsaved edits stops saving and shows the conflict
  // bar, whose two buttons are the only ways out.
  const change_style = document.createElement("style");
  change_style.textContent = `
    #save_status.conflict { color: #e05a5a; font-weight: bold; }
    #conflict_bar, #change_notice {
      position: fixed; top: 64px; left: 50%; transform: translateX(-50%); z-index: 30;
      display: none; gap: 10px; align-items: center; padding: 8px 12px; border-radius: 8px;
      font: 14px sans-serif; color: #fff;
    }
    #conflict_bar { background: #7a2a2a; }
    #change_notice { background: #33485e; }
    #conflict_bar button { font: inherit; padding: 6px 12px; }`;
  document.head.appendChild(change_style);
  const conflict_bar = document.createElement("div");
  conflict_bar.id = "conflict_bar";
  const conflict_text = document.createElement("span");
  conflict_text.textContent = "This document changed elsewhere. Your edits are not saved.";
  const load_newer_button = document.createElement("button");
  load_newer_button.textContent = "load newer";
  const fork_button = document.createElement("button");
  fork_button.textContent = "fork";
  conflict_bar.append(conflict_text, load_newer_button, fork_button);
  const change_notice = document.createElement("div");
  change_notice.id = "change_notice";
  document.body.append(conflict_bar, change_notice);
  persistence.conflict_bar_element = conflict_bar;

  let change_notice_timer: number | null = null;
  function show_change_notice(text: string): void {
    change_notice.textContent = text;
    change_notice.style.display = "flex";
    if (change_notice_timer !== null) window.clearTimeout(change_notice_timer);
    change_notice_timer = window.setTimeout(() => {
      change_notice.style.display = "none";
    }, 4000);
  }

  // The document becomes the fetched file, as one history step, so undo returns to
  // what this tab had. Returns false when the file is unreadable.
  function replace_document_with_fetched(fetched: FetchedDocument): boolean {
    begin_history_step(history, tablet_document);
    if (!apply_fetched_document(persistence, tablet_document, camera, fetched)) {
      history.pending = null;
      return false;
    }
    end_history_step(history, tablet_document, "changed elsewhere");
    // Before the render request: the new entry is what the server has, not an edit to save.
    persistence.history_snapshot = current_history_snapshot();
    persistence.last_saved_history_snapshot = persistence.history_snapshot;
    clear_selection_after_history_jump();
    return true;
  }

  // Not while a pen or finger is down: the document must not change under a gesture.
  let pointer_is_down = false;
  window.addEventListener("pointerdown", () => { pointer_is_down = true; }, true);
  for (const type of ["pointerup", "pointercancel"] as const) {
    window.addEventListener(type, () => { pointer_is_down = false; }, true);
  }

  let pull_is_running = false;
  async function pull_document_changes_if_any(): Promise<void> {
    if (pull_is_running) return;
    pull_is_running = true;
    const fetched = await fetch_document_if_changed_elsewhere(persistence);
    pull_is_running = false;
    if (fetched === null || pointer_is_down) return; // pointer down: the next poll takes it
    if (replace_document_with_fetched(fetched)) show_change_notice("changed elsewhere, reloaded");
  }
  // Hidden tabs do not poll; they catch up the moment they are shown again.
  window.setInterval(() => {
    if (document.visibilityState === "visible") void pull_document_changes_if_any();
  }, 2000);
  document.addEventListener("visibilitychange", () => {
    if (document.visibilityState === "visible") void pull_document_changes_if_any();
    else remember_camera(persistence, camera);
  });

  async function load_newer_document(): Promise<boolean> {
    const fetched = await fetch_current_document(persistence);
    if (fetched === null || !replace_document_with_fetched(fetched)) {
      window.alert(`The newer '${persistence.current_document_name}' could not be loaded. Nothing changed in this tab.`);
      return false;
    }
    return true;
  }
  load_newer_button.addEventListener("click", () => void load_newer_document());
  // Fork: this tab's version goes to its own file first, then the tab takes the newer one.
  fork_button.addEventListener("click", () => {
    void fork_document_to_conflict_copy(persistence, tablet_document, camera).then(async (conflict_copy_name) => {
      if (conflict_copy_name === null) {
        window.alert("The fork could not be saved. Nothing changed in this tab.");
        return;
      }
      if (await load_newer_document()) show_change_notice(`your version is saved as ${conflict_copy_name}`);
    });
  });

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
  void setup.load_reference_mesh().then((mesh) => {
    reference_mesh = mesh;
    request_render();
  });
  // First draft of the skin drawing (plan-skin-from-skull-wrap.md): copy the skull layer
  // onto the skin layer, then move each copied vertex outward onto the reference mesh,
  // which on the skin-draw page is the skin. Endpoints only: move_vertex rotates the
  // handles with the chord, so every copy keeps its d0/d3/chord plane. Pinned copies are
  // left to update_pinned_vertex_positions. One history step. Returns one row per moved
  // vertex, for checking the result.
  function wrap_skull_onto_skin(): WrapRow[] {
    if (reference_mesh === null) throw new Error("wrap: the skin mesh is not loaded yet");
    if (tablet_document.strokes.some((stroke) => stroke.layer === "skin")) throw new Error("wrap: the skin layer is not empty");
    const WRAP_MAX_PUSH_MM = 25;
    const mesh = reference_mesh;
    let head_center = v3(0, 0, 0);
    for (const position of mesh.triangle_positions) head_center = v3_add(head_center, position);
    head_center = v3_scale(head_center, 1 / mesh.triangle_positions.length);
    begin_history_step(history, tablet_document);
    const copied_vertex = copy_layer_strokes(tablet_document, "skull", "skin");
    const rows: WrapRow[] = [];
    for (const vertex_id of copied_vertex.values()) {
      if (pin_by_vertex(tablet_document, vertex_id) !== null) continue;
      const position = vertex_position(tablet_document, vertex_id);
      let outward = v3_sub(position, head_center);
      // Midline copies stay in the sagittal plane: the ray has no sideways part.
      if (vertex_is_on_midline(tablet_document, vertex_id)) outward = v3(0, outward.y, outward.z);
      const projection = project_vertex_onto_mesh(mesh, position, v3_normalize(outward), WRAP_MAX_PUSH_MM * WORLD_PER_MM);
      move_vertex(tablet_document, vertex_id, projection.position);
      rows.push({ vertex: vertex_id, method: projection.method, push_mm: v3_length(v3_sub(projection.position, position)) / WORLD_PER_MM });
    }
    end_history_step(history, tablet_document, "wrap");
    request_render();
    return rows;
  }
  // Only where the reference mesh is the skin (the skin-draw page).
  if (setup.active_layer === "skin") {
    (window as unknown as { debug_wrap_skull_onto_skin: unknown }).debug_wrap_skull_onto_skin = wrap_skull_onto_skin;
  }
  // Debug hook: inspect the document from the browser console / automated tests.
  (window as unknown as { tablet_document: unknown }).tablet_document = tablet_document;
  (window as unknown as { debug_camera: unknown }).debug_camera = camera;
  (window as unknown as { debug_persistence: unknown }).debug_persistence = persistence;
  (window as unknown as { debug_render_now: unknown }).debug_render_now = render_now;
  (window as unknown as { debug_selection_text: unknown }).debug_selection_text = selection_readout_text;
  (window as unknown as { debug_pull_document_changes_if_any: unknown }).debug_pull_document_changes_if_any = pull_document_changes_if_any;
  // Read by the page-reload script of the build (vite.config.ts reload_on_rebuild_plugin).
  (window as unknown as { tablet_has_unsaved_edits: unknown }).tablet_has_unsaved_edits = () => has_unsaved_edits(persistence);
  console.log("autodraw tablet: Sketchpad rework — line tool + vertex-connected single-cubic strokes");
}
