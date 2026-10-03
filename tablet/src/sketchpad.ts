// autodraw tablet — iPad drawing companion prototype.
// Sketchpad rework (plan step 7): strokes are single cubics put down with the
// armed line tool and shaped afterwards; endpoints live in a shared vertex
// table so joined strokes can never tear. Bare pen drags orbit (Q27/Q35); a
// drag starting on the selected stroke translates it; vertex/handle drags
// reshape. Finger = camera throughout (1-finger orbit, 2-finger pan/zoom).
//
// The whole editor is `start_sketchpad(setup)`: one page calls it with its own
// reference mesh, document name and localStorage keys (plan-sketchpad-landmarks.md
// Q66/Q67) — `draw/main.ts` is the plain sketchpad, `pages/skull-draw/main.ts` the
// same editor over the Z-Anatomy skull. The page's HTML supplies the canvas and
// the toolbar buttons by id.

import { CameraSnapState, camera_basis, camera_eye, camera_orbit, camera_snap_to_axis_view, camera_view_projection, camera_world_to_screen, camera_world_units_per_pixel, default_camera } from "./camera";
import { ALL_LAYERS, DEFAULT_STROKE_RADII, Layer, SKULL_BONE_ID, StrokeId, StrokeRadii, VertexId, VertexPin, add_straight_stroke, add_vertex,bezier_point, delete_stroke, empty_document, enforce_midline, find_snap_target_stroke, garbage_collect_vertices, move_vertex, patch_layer, pin_by_vertex, pins_on_stroke, set_vertex_world_position, smooth_knot_between_strokes, smooth_knots_at_vertex, smooth_strokes, split_stroke, stroke_by_id, stroke_control_points, stroke_radii, unsmooth_strokes, update_pinned_vertex_positions, vertex_by_id, vertex_is_on_layers, vertex_is_on_midline, vertex_position, vertex_world_position } from "./document";
import { CONTROL_POINT_PICK_RADIUS_PIXELS, EditState, HandleMode, STROKE_PICK_RADIUS_PIXELS, StrokePointKey, TAP_MAX_MOVEMENT_PIXELS, begin_edit_state, camera_plane_drag, edit_pen_down, edit_pen_move, edit_pen_up, find_merge_target_vertex, nearest_t_on_stroke_screen, pick_stroke, pick_stroke_point, pick_vertex } from "./edit_mode";
import { begin_history_step, clear_history, create_history_state, end_history_step, jump_history, redo, undo } from "./history";
import { ORBIT_RADIANS_PER_PIXEL, attach_gestures } from "./gestures";
import { LineToolState, line_pen_down, line_pen_move, line_pen_up } from "./line_tool";
import { merge_adjacent_strokes } from "./stroke_merge";
import { append_patch_mesh, patch_surface_grid, pick_patch, pin_is_locked, stroke_bounds_a_patch } from "./patch";
import { extract_contour_chains } from "./contour";
import { V2, V3, v3, v3_add, v3_scale, v3_sub } from "./math";
import { append_chain_ribbon, append_stroke_ribbon } from "./ribbon";
import { FLOATS_PER_VERTEX, VertexSink, create_vertex_sink, reset_vertex_sink, vertex_sink_view } from "./vertex_sink";
import { ReferenceMesh, append_reference_mesh } from "./reference";
import { create_persistence_state, flush_autosave, list_documents_from_server, load_current_document_on_startup, rename_document, schedule_autosave, switch_document } from "./persistence";
import { ClipPlane, FLOATS_PER_TRANSLUCENT_VERTEX, create_line_renderer, create_translucent_mesh, draw_mesh_translucent, render_frame, set_overlay_lines, set_overlay_triangles, set_preview_line, set_reference_mesh, set_stroke_mesh, set_surface_mesh, set_translucent_mesh } from "./render";

// What differs between the pages that run the sketchpad.
export type SketchpadSetup = {
  // The mesh behind the drawing, already in world units; null = none (fetch failed).
  load_reference_mesh: () => Promise<ReferenceMesh | null>;
  // A second mesh shown with the reference while the page's optional `#eyeball_button`
  // is armed (plan-skin-over-skull-study.md Q9); undefined = the page has none.
  load_eyeball_mesh?: () => Promise<ReferenceMesh | null>;
  default_document_name: string; // opened when localStorage remembers no current document
  storage_key_prefix: string; // localStorage keys `<prefix>_current_document`, `<prefix>_crash_buffer`
  // True: the docs panel lists every server document plus "new…"/"rename…". False: the
  // page is tied to its one document, the panel only names it.
  can_switch_documents: boolean;
  // Layers (plan-skin-over-skull-study.md Q4/Q10): new strokes go to
  // `active_layer`; `locked_layers` are locked for good (the page's optional
  // `#layer_bar` shows them locked but cannot unlock them or draw on them).
  active_layer: Layer;
  locked_layers: Layer[];
};

// Ported from the desktop app (driver.kc default_line_color = gray 0.03
// linear -> 0.196 sRGB; we write sRGB straight to the framebuffer).
const STROKE_COLOR = { r: 0.196, g: 0.196, b: 0.196 };
const HIGHLIGHT_COLOR = { r: 1.0, g: 0.65, b: 0.2 };
const PATCH_HIGHLIGHT_COLOR = { r: 0.75, g: 0.5, b: 0.2 }; // the selected patch's fill (plan-patch-subcurve-boundary.md Q11)
const HOT_COLOR = { r: 1.0, g: 1.0, b: 0.4 }; // what the hovering pen would hit
const PREVIEW_COLOR = { r: 0.6, g: 0.75, b: 1.0 };
const ANCHOR_COLOR = { r: 1.0, g: 1.0, b: 1.0 };
const HANDLE_COLOR = { r: 0.45, g: 0.8, b: 1.0 };
const HANDLE_LINE_COLOR = { r: 0.5, g: 0.5, b: 0.55 };
const PIN_COLOR = { r: 1.0, g: 0.5, b: 0.85 }; // pinned vertices (vertex_pins)
const KNOT_COLOR = { r: 0.55, g: 1.0, b: 0.55 }; // smooth knots (smooth_knots)
const NAMED_VERTEX_COLOR = { r: 0.55, g: 1.0, b: 0.6 }; // landmarks (vertices with a name), always drawn
const MIDLINE_VERTEX_COLOR = { r: 0.5, g: 0.6, b: 1.0 }; // vertices held on x = 0 (plan-sketchpad-midline.md Q74), always drawn
const SURFACE_COLOR = { r: 0.45, g: 0.55, b: 0.7 };
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
  const hidden_layers = new Set<Layer>();
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
  // stroke endpoint or a landmark — selects it instead of a stroke. Exclusive with
  // the stroke selection: selecting either clears the other. The selected vertex
  // drags on the camera plane and is the target of the name/delete buttons.
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
  // Hot item: what a pen-down at the hovering pen's position would grab,
  // resolved every frame (the camera can move under a still pen) with the same
  // picks and priority as edit_pen_down / the tap handlers, and drawn in
  // HOT_COLOR so the user knows before committing.
  type HotItem =
    | { kind: "stroke"; stroke_id: StrokeId }
    | { kind: "point"; key: StrokePointKey } // control point of the selected stroke
    | { kind: "pin"; vertex: VertexId } // pin riding the selected stroke
    | { kind: "vertex"; vertex: VertexId }; // any document vertex, when no stroke is selected
  let hover_screen: V2 | null = null; // null while the pen is down or off the canvas
  let hot_item: HotItem | null = null;

  function resolve_hot_item(): HotItem | null {
    if (hover_screen === null || armed_tool === "line") return null;
    if (edit_state !== null && armed_tool === null) {
      const pin = pick_pin_on_stroke(edit_state.stroke_id, hover_screen);
      if (pin !== null) return { kind: "pin", vertex: pin.vertex };
      const stroke = stroke_by_id(tablet_document, edit_state.stroke_id);
      const key = pick_stroke_point(stroke, tablet_document, camera, hover_screen, canvas);
      if (key !== null) return { kind: "point", key };
    }
    if (edit_state === null && armed_tool === null) {
      const vertex = pick_vertex(tablet_document, camera, hover_screen, canvas, pickable_layers());
      if (vertex !== null) return { kind: "vertex", vertex };
    }
    const picked = pick_stroke(tablet_document, camera, hover_screen, canvas, pickable_layers());
    return picked === null ? null : { kind: "stroke", stroke_id: picked };
  }

  function request_render(): void {
    // Anything that changes what's on screen (strokes, surfaces, camera) goes
    // through here — piggyback the debounced autosave on it; identical
    // serializations are skipped inside. Before the early return: a pending
    // frame must not swallow the save (rAF pauses entirely in hidden tabs).
    // Pinned vertices are derived data — re-derive synchronously (NOT in the
    // rAF, which pauses in hidden tabs) so any host reshape carries its riders
    // before history snapshots and autosave see the document. The midline pass
    // goes first: a pin may ride a midline stroke, never the other way round (Q71).
    enforce_midline(tablet_document);
    update_pinned_vertex_positions(tablet_document);
    refresh_pin_button_armed();
    refresh_split_here_button();
    refresh_lock_buttons();
    refresh_midline_button_armed();
    refresh_smooth_button_armed();
    refresh_history_panel();
    refresh_width_panel();
    persistence.history_snapshot = history.position >= 0 ? history.entries[history.position].snapshot : null;
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
    rebuild_stroke_labels();
  }

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

  function resize_canvas_to_display(): void {
    const dpr = window.devicePixelRatio;
    canvas.width = Math.round(canvas.clientWidth * dpr);
    canvas.height = Math.round(canvas.clientHeight * dpr);
    gl!.viewport(0, 0, canvas.width, canvas.height);
    request_render();
  }
  window.addEventListener("resize", resize_canvas_to_display);

  // The stroke a dragged vertex would get pinned to on release (drag-time
  // warning, same function as the release so they can never disagree), or null.
  function drag_snap_target_stroke(): StrokeId | null {
    if (edit_state === null || (edit_state.dragging !== "p0" && edit_state.dragging !== "p3")) return null;
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
      const color = hot ? HOT_COLOR : highlighted ? HIGHLIGHT_COLOR : STROKE_COLOR;
      append_stroke_ribbon(stroke, tablet_document, camera, color, vertices);
    }
    append_contour_ribbons(vertices);
    set_stroke_mesh(renderer, vertex_sink_view(vertices));
  }

  // Computed contours share the stroke mesh so they get the same depth bias
  // and draw order as drawn strokes; they are derived per frame, never stored.
  function append_contour_ribbons(vertices: VertexSink): void {
    const eye = camera_eye(camera);
    for (const patch of tablet_document.patches) {
      if (hidden_layers.has(patch_layer(patch, tablet_document))) continue;
      const grid = patch_surface_grid(patch, tablet_document);
      if (grid === null) continue;
      for (const chain of extract_contour_chains(grid, eye)) append_chain_ribbon(chain, camera, STROKE_COLOR, vertices);
    }
  }

  function rebuild_surface_mesh(): void {
    const vertices = surface_sink;
    reset_vertex_sink(vertices);
    tablet_document.patches.forEach((patch, index) => {
      if (hidden_layers.has(patch_layer(patch, tablet_document))) return;
      const color = index === selected_patch ? PATCH_HIGHLIGHT_COLOR : surface_colored ? SURFACE_COLOR : SURFACE_BACKGROUND_COLOR;
      append_patch_mesh(patch, tablet_document, camera, color, vertices);
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
    append_reference_mesh(reference_mesh, camera, vertices);
    if (eyeball_visible && eyeball_mesh !== null) append_reference_mesh(eyeball_mesh, camera, vertices);
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

  function rebuild_edit_overlay(): void {
    const basis = camera_basis(camera);
    const units_per_pixel = camera_world_units_per_pixel(camera, canvas.clientHeight);
    const anchor_half = (ANCHOR_SIZE_PIXELS / 2) * units_per_pixel;
    const handle_half = (HANDLE_SIZE_PIXELS / 2) * units_per_pixel;
    const line_vertices: number[] = [];
    const triangle_vertices: number[] = [];

    // Landmarks (named vertices) draw whether or not anything is selected; the hot
    // vertex grows, the selected vertex draws anchor-sized in the highlight colour
    // (a selected unnamed vertex is otherwise invisible).
    const drawn_layers = visible_layers();
    for (const vertex of tablet_document.vertices) {
      if (!vertex_is_on_layers(tablet_document, vertex.id, drawn_layers)) continue;
      const hot = hot_item !== null && hot_item.kind === "vertex" && hot_item.vertex === vertex.id;
      const world_position = vertex_world_position(tablet_document, vertex);
      if (vertex.id === selected_vertex || vertex.id === extra_vertex) {
        append_billboard_square(world_position, anchor_half, basis.right, basis.up, HIGHLIGHT_COLOR, triangle_vertices);
      } else if (hot) {
        append_billboard_square(world_position, handle_half * HOT_SIZE_SCALE, basis.right, basis.up, HOT_COLOR, triangle_vertices);
      } else if (vertex.name !== undefined) {
        append_billboard_square(world_position, handle_half, basis.right, basis.up, NAMED_VERTEX_COLOR, triangle_vertices);
      } else if (vertex_is_on_midline(tablet_document, vertex.id)) {
        append_billboard_square(world_position, handle_half, basis.right, basis.up, MIDLINE_VERTEX_COLOR, triangle_vertices);
      }
    }
    if (edit_state === null) {
      set_overlay_lines(renderer, new Float32Array(0));
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
    // Hot control points / pins draw bigger and in HOT_COLOR.
    const is_hot_point = (key: StrokePointKey) => hot_item !== null && hot_item.kind === "point" && hot_item.key === key;
    const is_hot_pin = (vertex: VertexId) => hot_item !== null && hot_item.kind === "pin" && hot_item.vertex === vertex;
    const push_marker = (center: V3, half_size: number, color: { r: number; g: number; b: number }, hot: boolean) => {
      append_billboard_square(
        center, hot ? half_size * HOT_SIZE_SCALE : half_size, basis.right, basis.up,
        hot ? HOT_COLOR : color, triangle_vertices,
      );
    };
    push_marker(points.p1, handle_half, HANDLE_COLOR, is_hot_point("p1"));
    push_marker(points.p2, handle_half, HANDLE_COLOR, is_hot_point("p2"));
    // Endpoints that are pinned vertices (riding some other stroke) show in the
    // pin color so it's clear they'll slide, not translate, when grabbed; smooth
    // knots in the knot color so it's clear the neighbour's handle will follow.
    const anchor_color = (vertex: VertexId) => {
      if (pin_by_vertex(tablet_document, vertex) !== null) return PIN_COLOR;
      if (smooth_knots_at_vertex(tablet_document, vertex).length > 0) return KNOT_COLOR;
      return ANCHOR_COLOR;
    };
    push_marker(points.p0, anchor_half, anchor_color(selected_stroke.p0_vertex), is_hot_point("p0"));
    push_marker(points.p3, anchor_half, anchor_color(selected_stroke.p3_vertex), is_hot_point("p3"));
    // Pinned vertices riding the selected stroke; the selected pin (the unpin
    // button's target) draws larger.
    for (const pin of tablet_document.vertex_pins) {
      if (pin.host_stroke !== edit_state.stroke_id) continue;
      const half_size = pin.vertex === edit_state.selected_pin ? anchor_half : handle_half;
      push_marker(vertex_position(tablet_document, pin.vertex), half_size, PIN_COLOR, is_hot_pin(pin.vertex));
    }
    // Drag-time snap warning (Q3): while a vertex is being dragged, mark the
    // vertex it would weld into on release so the merge is never a surprise.
    if (edit_state.dragging === "p0" || edit_state.dragging === "p3") {
      const dragged_vertex = edit_state.dragging === "p0" ? selected_stroke.p0_vertex : selected_stroke.p3_vertex;
      const target_vertex = find_merge_target_vertex(tablet_document, dragged_vertex, pickable_layers());
      if (target_vertex !== null) {
        append_billboard_square(
          vertex_position(tablet_document, target_vertex), anchor_half * 2, basis.right, basis.up,
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
    selected_vertex = null;
    extra_vertex = null;
    selected_patch = null;
    if (!multi) {
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

  // Selected-stroke pen-up: a tap on another stroke switches (or ctrl: extends)
  // the selection, or with a pick tool armed, feeds it; a tap on empty space
  // deselects. Drags (control point, whole-stroke move, or orbit) just end.
  function edit_mode_pen_up(position: V2, multi: boolean): void {
    if (edit_state === null) return;
    const was_control_drag =
      edit_state.dragging !== null || edit_state.dragging_pin !== null || edit_state.moving_whole_stroke;
    if (edit_state.moving_whole_stroke) pen_history_label = `move stroke ${edit_state.stroke_id}`;
    else if (edit_state.dragging_pin !== null) pen_history_label = `move pin ${edit_state.dragging_pin}`;
    else if (edit_state.dragging !== null) {
      // Same label for every drag of one control point, so a run of them merges
      // into a single history entry (see end_history_step).
      pen_history_label = `move ${edit_state.dragging} of stroke ${edit_state.stroke_id}`;
      pen_history_merge = true;
    }
    edit_pen_up(edit_state, tablet_document, pickable_layers());
    const was_tap = pen_max_displacement_pixels < TAP_MAX_MOVEMENT_PIXELS;
    if (!was_tap || was_control_drag) return;
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
        edit_state.selected_pin = vertex;
        pen_history_label = `pin vertex ${vertex}`;
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
      set_armed_tool(null); // tap off the curve = cancel, selection kept
      return;
    }
    const picked = pick_stroke(tablet_document, camera, position, canvas, pickable_layers());
    if (picked === null) {
      edit_state = null;
      extra_selection = [];
      selected_patch = pick_patch(tablet_document, camera, position, canvas, pickable_layers()); // fill = last resort (Q10)
      return;
    }
    select_stroke_by_tap(picked, multi);
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
  // The pin button also lights up while a pinned vertex is selected — in that
  // state tapping it unpins (Q11). Re-checked every frame since pin selection
  // changes on pen gestures, not just button presses.
  function refresh_pin_button_armed(): void {
    pin_button.classList.toggle(
      "armed", armed_tool === "pin" || (edit_state !== null && edit_state.selected_pin !== null),
    );
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
  // wherever the pen lands; none = arm the free-hand tool. The new stroke becomes
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

  // Split here (plan-sketchpad-split-at-vertex.md): a pinned vertex is selected by
  // tapping it while its host stroke is selected; this button then cuts the stroke
  // exactly there, the vertex becoming the smooth knot. Shown only while such a
  // vertex is selected -- the same state the pin button reads for "unpin".
  const split_here_button = document.getElementById("split_here_button") as HTMLButtonElement;
  function refresh_split_here_button(): void {
    split_here_button.hidden = edit_state === null || edit_state.selected_pin === null;
  }
  split_here_button.addEventListener("click", () => {
    if (edit_state === null || edit_state.selected_pin === null) return;
    const stroke_id = edit_state.stroke_id;
    const vertex = edit_state.selected_pin;
    const pin = pin_by_vertex(tablet_document, vertex);
    if (pin === null || pin.host_stroke !== stroke_id) return;
    begin_history_step(history, tablet_document);
    const new_stroke = split_stroke(tablet_document, stroke_id, pin.t, vertex);
    if (new_stroke === null) return; // pin within the end guard: nothing to record
    edit_state.selected_pin = null; // the pin is gone; [0, t] half stays selected (Q5)
    end_history_step(history, tablet_document, `split stroke ${stroke_id} at vertex ${vertex}`);
    request_render();
  });

  // Pin: with a pinned vertex selected, unpin it (frozen in place as a free
  // vertex); otherwise arm pin creation — the next tap on the selected stroke's
  // curve drops a vertex constrained there.
  pin_button.addEventListener("click", () => {
    if (edit_state === null) return; // needs a selected host stroke
    if (edit_state.selected_pin !== null) {
      if (unpin_is_locked()) return; // a patch's sub-curve ends here (Q3)
      begin_history_step(history, tablet_document);
      const unpinned_vertex = edit_state.selected_pin;
      tablet_document.vertex_pins = tablet_document.vertex_pins.filter((pin) => pin.vertex !== unpinned_vertex);
      end_history_step(history, tablet_document, `unpin vertex ${unpinned_vertex}`);
      edit_state.selected_pin = null;
      request_render();
      return;
    }
    set_armed_tool(armed_tool === "pin" ? null : "pin");
  });

  // Midline (plan-sketchpad-midline.md Q72): toggles the midline flag on the
  // selected vertex or stroke; enforce_midline in request_render snaps it to
  // x = 0 at once. Setting the flag drops any pin competing for the same
  // position (Q71): the vertex's own, or the endpoints' of a stroke.
  const midline_button = document.getElementById("midline_button") as HTMLButtonElement;
  function refresh_midline_button_armed(): void {
    const flagged = selected_vertex !== null
      ? vertex_by_id(tablet_document, selected_vertex).midline === true
      : edit_state !== null && stroke_by_id(tablet_document, edit_state.stroke_id).midline === true;
    midline_button.classList.toggle("armed", flagged);
  }
  midline_button.addEventListener("click", () => {
    let flagged: { midline?: boolean };
    let claimed_vertices: VertexId[];
    if (selected_vertex !== null) {
      flagged = vertex_by_id(tablet_document, selected_vertex);
      claimed_vertices = [selected_vertex];
    } else if (edit_state !== null) {
      const stroke = stroke_by_id(tablet_document, edit_state.stroke_id);
      flagged = stroke;
      claimed_vertices = [stroke.p0_vertex, stroke.p3_vertex];
    } else {
      return;
    }
    begin_history_step(history, tablet_document);
    const midline_target = selected_vertex !== null ? `vertex ${selected_vertex}` : `stroke ${edit_state!.stroke_id}`;
    let label = `set midline ${midline_target}`;
    if (flagged.midline === true) {
      delete flagged.midline;
      label = `clear midline ${midline_target}`;
    } else {
      flagged.midline = true;
      tablet_document.vertex_pins = tablet_document.vertex_pins.filter((pin) => !claimed_vertices.includes(pin.vertex));
      if (edit_state !== null && edit_state.selected_pin !== null && claimed_vertices.includes(edit_state.selected_pin)) {
        edit_state.selected_pin = null;
      }
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
  // (Khoa 2026-09-27, supersedes plan Q78's one-per-key). A midline or pinned
  // vertex is snapped back by the pass in request_render, so those just don't move.
  const NUDGE_STEP_PIXELS = 4;
  const NUDGE_SHIFT_MULTIPLIER = 5;
  function nudge_selected_vertex(right_steps: number, up_steps: number, forward_steps: number, shift: boolean): void {
    if (selected_vertex === null || pen_down_screen !== null) return;
    const basis = camera_basis(camera);
    const step = NUDGE_STEP_PIXELS * (shift ? NUDGE_SHIFT_MULTIPLIER : 1) * camera_world_units_per_pixel(camera, canvas.clientHeight);
    const delta = v3_add(
      v3_add(v3_scale(basis.right, right_steps * step), v3_scale(basis.up, up_steps * step)),
      v3_scale(basis.forward, forward_steps * step),
    );
    begin_history_step(history, tablet_document);
    set_vertex_world_position(tablet_document, selected_vertex, v3_add(vertex_position(tablet_document, selected_vertex), delta));
    end_history_step(history, tablet_document, `nudge vertex ${selected_vertex}`, true); // a run of nudges = one entry
    request_render();
  }
  window.addEventListener("keydown", (event) => {
    if (event.ctrlKey || event.metaKey || event.altKey) return;
    switch (event.key.toLowerCase()) {
      case "h": nudge_selected_vertex(-1, 0, 0, event.shiftKey); break;
      case "l": nudge_selected_vertex(1, 0, 0, event.shiftKey); break;
      case "j": nudge_selected_vertex(0, -1, 0, event.shiftKey); break;
      case "k": nudge_selected_vertex(0, 1, 0, event.shiftKey); break;
      case "i": nudge_selected_vertex(0, 0, 1, event.shiftKey); break;
      case "o": nudge_selected_vertex(0, 0, -1, event.shiftKey); break;
      // Delete / Backspace (a Mac keyboard's "delete") = the del button, lock
      // included (plan-patch-subcurve-boundary.md Q12). Not while typing a name
      // or a width.
      case "delete":
      case "backspace":
        if (document.activeElement instanceof HTMLInputElement) return;
        event.preventDefault();
        delete_button.click();
        break;
      // Escape = unselect all (lines, vertices, pin, patch) and disarm any
      // armed tool. Not while typing a name or a width.
      case "escape":
        if (document.activeElement instanceof HTMLInputElement) return;
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
      if (armed_tool === "line") {
        line_state = line_pen_down(tablet_document, camera, position, canvas, pickable_layers(), line_start_vertex);
        update_preview_line();
      } else if (edit_state !== null && armed_tool === null) {
        // Consumed only when the pen lands on the selection; otherwise orbit.
        if (!edit_pen_down(edit_state, tablet_document, camera, position, canvas, pickable_layers())) {
          pen_orbit_last_screen = position;
        }
      } else if (selected_vertex !== null && armed_tool === null && pick_vertex(tablet_document, camera, position, canvas, pickable_layers()) === selected_vertex) {
        // Landing on the selected vertex drags it on the camera plane.
        selected_vertex_drag_last_screen = position;
      } else {
        // No selection, or a pick tool armed (pure tap tool): bare drags orbit.
        pen_orbit_last_screen = position;
      }
      request_render();
    },
    on_pen_move: (position, event) => {
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
          set_vertex_world_position(tablet_document, selected_vertex, v3_add(vertex_position(tablet_document, selected_vertex), drag_delta));
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
      } else if (selected_vertex_drag_last_screen !== null) {
        // A vertex drag just ends (no weld/pin on release — landmarks are free points).
        pen_history_label = `move vertex ${selected_vertex}`;
      } else if (pen_max_displacement_pixels < TAP_MAX_MOVEMENT_PIXELS) {
        // Tap with nothing (or a vertex) selected: vertex first, then stroke, else clear.
        // Ctrl-tap on a second vertex sets / clears the extra (Q79).
        const picked_vertex = pick_vertex(tablet_document, camera, position, canvas, pickable_layers());
        const picked_stroke = picked_vertex === null ? pick_stroke(tablet_document, camera, position, canvas, pickable_layers()) : null;
        if (picked_vertex !== null && multi && selected_vertex !== null && picked_vertex !== selected_vertex) {
          extra_vertex = extra_vertex === picked_vertex ? null : picked_vertex;
        } else if (picked_vertex !== null) {
          selected_vertex = picked_vertex;
          extra_vertex = null;
          selected_patch = null;
        } else if (picked_stroke !== null) {
          select_stroke_by_tap(picked_stroke, multi);
        } else {
          selected_vertex = null;
          extra_vertex = null;
          selected_patch = pick_patch(tablet_document, camera, position, canvas, pickable_layers()); // fill = last resort (Q10)
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
    const named = selected_vertex !== null
      ? vertex_by_id(tablet_document, selected_vertex)
      : edit_state !== null ? stroke_by_id(tablet_document, edit_state.stroke_id) : null;
    if (named === null) return;
    const what = selected_vertex !== null ? "Vertex" : "Line";
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
    return edit_state !== null && edit_state.selected_pin !== null && pin_is_locked(tablet_document, edit_state.selected_pin);
  }
  // A button whose handler would do nothing is greyed out, with the reason as
  // its tooltip (plan-sketchpad-pin-replaces-split.md Q7: greyed, not hidden, so
  // the toolbar never shifts under the pen). The button's own tooltip from the
  // page comes back once it is usable.
  const own_button_titles = new Map<HTMLButtonElement, string>();
  function set_button_unavailable_reason(button: HTMLButtonElement, reason: string | null): void {
    if (!own_button_titles.has(button)) own_button_titles.set(button, button.title);
    button.disabled = reason !== null;
    button.title = reason ?? own_button_titles.get(button)!;
  }
  function refresh_lock_buttons(): void {
    const no_line = edit_state === null ? "select a line first" : null;
    const no_line_or_vertex = edit_state === null && selected_vertex === null ? "select a line or a vertex first" : null;
    const not_two_lines = single_extra_stroke() === null ? "select two lines first (tap one, ctrl-tap the other)" : null;

    let delete_reason: string | null = null;
    if (selected_patch === null && selected_vertex === null) {
      if (edit_state === null) delete_reason = "select a line, a vertex or a patch first";
      else if (delete_is_locked()) delete_reason = `the selected line bounds a patch, ${DELETE_PATCH_FIRST}`;
    }
    if (selected_patch === null && selected_vertex !== null && pin_is_locked(tablet_document, selected_vertex)) {
      delete_reason = `this vertex bounds a patch, ${DELETE_PATCH_FIRST}`;
    }
    set_button_unavailable_reason(delete_button, delete_reason);
    set_button_unavailable_reason(
      join_button, not_two_lines ?? (join_is_locked() ? `one of these lines bounds a patch, ${DELETE_PATCH_FIRST}` : null),
    );
    set_button_unavailable_reason(
      pin_button, no_line ?? (unpin_is_locked() ? `this vertex bounds a patch, ${DELETE_PATCH_FIRST}` : null),
    );
    set_button_unavailable_reason(split_button, no_line);
    set_button_unavailable_reason(smooth_button, not_two_lines);
    set_button_unavailable_reason(
      patch_button,
      edit_state === null || extra_selection.length === 0 ? "select two or more lines first (ctrl-tap adds a line)" : null,
    );
    set_button_unavailable_reason(midline_button, no_line_or_vertex);
    set_button_unavailable_reason(name_button, no_line_or_vertex);
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
    function refresh_layer_bar(): void {
      for (const layer of ALL_LAYERS) {
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
    for (const layer of ALL_LAYERS) {
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
    void flush_autosave(persistence, tablet_document, camera, true);
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
  // Debug hook: inspect the document from the browser console / automated tests.
  (window as unknown as { tablet_document: unknown }).tablet_document = tablet_document;
  (window as unknown as { debug_camera: unknown }).debug_camera = camera;
  (window as unknown as { debug_persistence: unknown }).debug_persistence = persistence;
  (window as unknown as { debug_render_now: unknown }).debug_render_now = render_now;
  console.log("autodraw tablet: Sketchpad rework — line tool + vertex-connected single-cubic strokes");
}
