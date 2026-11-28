// autodraw tablet — iPad drawing companion prototype.
// Sketchpad rework (plan step 7): strokes are single cubics put down with the
// armed line tool and shaped afterwards; endpoints live in a shared vertex
// table so joined strokes can never tear. Bare pen drags orbit (Q27/Q35), also
// when they start on the selected stroke; vertex/handle drags reshape. In width mode
// (the width panel open) a pen drag on any line changes its width instead, and
// nothing is reshaped.
// Finger = camera throughout (1-finger orbit, 2-finger pan/zoom).
//
// The whole editor is `start_sketchpad(setup)`: one page calls it with its own
// reference mesh, document name and localStorage keys (plan-sketchpad-landmarks.md
// Q66/Q67) — `draw/main.ts` is the plain sketchpad and, with `?page=<name>`, every
// drawing page of `src/drawing_pages.ts` (skull-draw: the same editor over the
// Z-Anatomy skull). The page's HTML supplies the canvas and the toolbar buttons by id.

import { CameraSnapState, camera_basis, camera_eye, camera_orbit, camera_screen_projector, camera_snap_to_axis_view, camera_view_projection, camera_world_to_screen, camera_world_units_per_pixel, default_camera } from "./camera";
import { ALL_LAYERS, DEFAULT_STROKE_RADII, Layer, Patch, SKULL_BONE_ID, StrokeId, StrokeRadii, TabletDocument, VertexId, VertexPin, add_patch, add_straight_stroke, add_vertex,bezier_point, copy_layer_strokes, delete_stroke, first_layer_with_strokes, pin_vertex_to_stroke, vertex_can_pin_to_stroke, detach_stroke_end_from_weld, stroke_end_can_detach_from_weld, empty_document, enforce_midline, garbage_collect_vertices, move_vertex, patch_layer, pin_by_vertex, reflect_new_stroke_onto_stored_side, smooth_knot_between_strokes, smooth_knots_at_vertex, smooth_strokes, split_stroke, straighten_strokes, Stroke, transfer_stroke_style, stroke_by_id, stroke_control_points, stroke_radii, stroke_radii_at, stroke_radii_with_width_change_at, strongest_stroke_radii_index_at, set_strokes_have_one_handle, set_strokes_always_drawn, stroke_single_handle_point, unsmooth_strokes, unpin_vertices, vertex_by_id, vertex_is_pinned, surface_pin_by_vertex, drop_surface_pins_of_missing_patches, vertex_is_on_layers, vertex_is_on_midline, vertex_position, vertex_world_position, world_from_bone } from "./document";
import { CONTROL_POINT_PICK_RADIUS_PIXELS, EditState, HandleMode, STROKE_PICK_RADIUS_PIXELS, TAP_MAX_MOVEMENT_PIXELS, begin_edit_state, camera_plane_drag, edit_nudge_handle, edit_pen_down, edit_pen_move, edit_pen_up, vertex_can_weld_into, weld_vertex_into, nearest_point_on_segment, nearest_t_on_stroke_screen, pick_stroke, pick_stroke_point, pick_vertex } from "./edit_mode";
import { begin_history_step, clear_history, create_history_state, end_history_step, jump_history, redo, undo } from "./history";
import { ORBIT_RADIANS_PER_PIXEL, attach_gestures } from "./gestures";
import { LineToolState, line_pen_down, line_pen_move, line_pen_up } from "./line_tool";
import { join_refusal_reason, merge_adjacent_strokes } from "./stroke_merge";
import { SPHERE_COLOR, sphere_drawn_grids } from "./sphere";
import { PatchGeometry, SurfaceGrid, append_grid_mesh_with_normals, append_flat_grid_mesh_with_normals, append_patch_geometry_mesh_with_normals, begin_patch_geometry_check_pass, cached_mirrored_grid, cached_patch_geometry, document_patch_geometry_cache, drop_unused_patch_strokes, pick_patch, patch_junction_vertices, resolve_patch_fill, pin_is_locked, point_is_visible_past_surfaces, stroke_bounds_a_patch, stroke_drawn_point, update_pinned_vertex_positions, drag_surface_pin, nearest_point_on_patch, pin_vertex_to_surface, refit_on_surface_stroke_control_points, vertex_can_pin_to_selected_surface } from "./patch";
import { SurfacePartials, extract_placed_contour_chains, surface_partials } from "./contour";
import { BorderContours, BorderPieceLook, BorderSurface, border_contours_of, border_runs_of } from "./boundary_contour";
import { cached_contour_width_blend, create_contour_width_blend_cache } from "./contour_width";
import { append_outline_chains, append_outline_ribbons, create_outline_pieces, outline_lines_of } from "./outline_lines";
import { describe_selection } from "./selection_readout";
import { V2, V3, mat4_transform_point, v3, v3_add, v3_length, v3_normalize, v3_scale, v3_sub } from "./math";
import { MeshProjectionMethod, project_vertex_onto_mesh } from "./mesh_projection";
import { WORLD_PER_MM } from "./reference_skull_view";
import { RIBBON_RADIUS, RibbonTint, append_stroke_ribbon, drawn_stroke_points, polyline_length_fractions, stroke_length_fraction_at } from "./ribbon";
import { FLOATS_PER_VERTEX, Rgb, VertexSink, create_vertex_sink, reset_vertex_sink, vertex_sink_view } from "./vertex_sink";
import { ReferenceMesh, append_reference_mesh } from "./reference";
import { FetchedDocument, apply_fetched_document, create_persistence_state, fetch_current_document, fetch_document_if_changed_elsewhere, flush_autosave, fork_document_to_conflict_copy, has_unsaved_edits, list_documents_from_server, load_current_document_on_startup, remember_camera, rename_document, schedule_autosave, switch_document } from "./persistence";
import { ClipPlane, FLOATS_PER_TRANSLUCENT_VERTEX, FloatRange, create_line_renderer, create_translucent_mesh, draw_mesh_translucent, render_frame, set_overlay_lines, set_overlay_triangles, set_preview_line, set_reference_mesh, set_stroke_mesh, set_surface_camera_forward, set_surface_mesh, set_surface_mesh_ranges, set_translucent_mesh } from "./render";

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
  // "first_layer_with_strokes": decided once the document has loaded
  // (`first_layer_with_strokes`, src/document.ts); a link that names only a document.
  active_layer: Layer | "first_layer_with_strokes";
  locked_layers: Layer[];
  // Layers this page never draws or picks; the layer bar has no row for them.
  hidden_layers: Layer[];
  // A page that embeds the sketchpad beside other views (fit-hairline-to-loomis-girl,
  // plan-hairline-reference-plate Q26/Q27): called with the document after it loaded and
  // after every history step (an edit, undo, redo, a reload from the server), never for
  // a camera move. The document is the sketchpad's own, read it, don't keep it.
  on_document_edited?: (tablet_document: TabletDocument) => void;
  // False while another view of the page owns the keyboard (the one last touched): the
  // sketchpad's window key listeners (undo, nudge, axis view, reference) do nothing.
  owns_keyboard?: () => boolean;
};

// Ported from the desktop app (driver.kc default_line_color = gray 0.03
// linear -> 0.196 sRGB; we write sRGB straight to the framebuffer).
const STROKE_COLOR = { r: 0.196, g: 0.196, b: 0.196 };
const HIGHLIGHT_COLOR = { r: 1.0, g: 0.65, b: 0.2 };
const PATCH_HIGHLIGHT_COLOR = { r: 0.75, g: 0.5, b: 0.2 }; // the selected patch's fill (plan-patch-subcurve-boundary.md Q11)
// The hot patch's fill: SURFACE_COLOR halfway toward HOT_COLOR, a pale yellow-green
// that stands out from the blue-gray surface but stays quieter than the selected orange.
const PATCH_HOT_COLOR = { r: 0.72, g: 0.78, b: 0.55 };
const HOT_COLOR = { r: 1.0, g: 1.0, b: 0.4 }; // what the hovering pen would hit
// What the selected vertex is pinned to: its host stroke, or the fill of its host patch.
const PIN_HOST_STROKE_COLOR = { r: 0.3, g: 0.85, b: 1.0 };
const PIN_HOST_PATCH_COLOR = { r: 0.45, g: 0.72, b: 0.85 };
const PREVIEW_COLOR = { r: 0.6, g: 0.75, b: 1.0 };
const ANCHOR_COLOR = { r: 1.0, g: 1.0, b: 1.0 };
const HANDLE_COLOR = { r: 0.45, g: 0.8, b: 1.0 };
const HANDLE_LINE_COLOR = { r: 0.5, g: 0.5, b: 0.55 };
const KNOT_COLOR = { r: 0.55, g: 1.0, b: 0.55 }; // smooth knots (smooth_knots)
const SURFACE_COLOR = { r: 0.45, g: 0.55, b: 0.7 };
// A locked layer (the skull under the skin on the skin page) draws in its own
// colours, so it reads as the thing drawn over, not the thing being drawn.
const LOCKED_LAYER_STROKE_COLOR = { r: 0.4, g: 0.27, b: 0.13 };
const LOCKED_LAYER_SURFACE_COLOR = { r: 0.66, g: 0.6, b: 0.48 };
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
  let surface_drawn = true; // "surf" button: off = no fill, the reference shows through everywhere
  // "picture" button / V key (plan-head-picture-from-contours-and-chosen-lines Q1): on = draw
  // and pick only the strokes marked is_always_drawn, plus the computed contours. A view
  // state, never saved in the document.
  let is_picture_view = false;
  // Layers (plan-skin-over-skull-study.md): new strokes and their vertices go to
  // active_layer; locked layers are ignored by every pick and snap; hidden
  // layers are not drawn (and not picked either). A vertex is on the layers of
  // its strokes (vertex_is_on_layers), a patch on its first stroke's.
  // Until the document has loaded, a page that lets the document decide draws on the first layer.
  let active_layer: Layer = ALL_LAYERS[0];
  if (setup.active_layer !== "first_layer_with_strokes") active_layer = setup.active_layer;
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
  // the selected curve and splits it there into two strokes joined at a plain
  // junction.
  type ArmedTool = "line" | "pin" | "split";
  let armed_tool: ArmedTool | null = null;
  // Swing is a sticky mode, not a tap tool: a handle drag tilts the plane by
  // swinging the other handle (edit_pen_move).
  let handle_mode: HandleMode = "plane";
  let line_state: LineToolState | null = null; // non-null while the line tool's pen is down
  let line_start_vertex: VertexId | null = null; // line tool armed from a selected vertex: the stroke starts there
  let pen_orbit_last_screen: V2 | null = null; // non-null while a bare pen drag orbits
  let frame_requested = false;
  // The patches' grids (and loft rails, mirrored grids) for frames and picks; request_render
  // begins a check pass (a patch whose inputs changed is built again), camera-only and hover
  // requests keep them. The document's own cache: the pin pass and the on-surface strokes
  // (project_onto_surface) read the same grids.
  const patch_geometry_cache = document_patch_geometry_cache(tablet_document);
  // The contour extractor's eye-independent part per cached grid (cached_surface_partials).
  const surface_partials_by_grid = new WeakMap<SurfaceGrid, SurfacePartials>();
  // The computed contours' width and lightness at every grid corner, per cached patch fill
  // (contour_width.ts).
  const contour_width_blends = create_contour_width_blend_cache();
  // The border contours' eye-independent part (border_contours_of), and what it was built
  // from: per patch its geometry (null when the patch has no contours) and whether its
  // mirror image is followed too. Built again when any of that changes.
  let border_contours: BorderContours | null = null;
  const border_contours_built_from_geometries: (PatchGeometry | null)[] = [];
  const border_contours_built_from_mirroring: boolean[] = [];
  // This frame's patch geometries and mirroring, compared with the ones above.
  const frame_contour_geometries: (PatchGeometry | null)[] = [];
  const frame_contour_mirroring: boolean[] = [];
  // The strokes rebuild_stroke_mesh drew whole on each side this frame: their border
  // contours are not drawn again there.
  const strokes_drawn_on_stored_side = new Set<StrokeId>();
  const strokes_drawn_on_mirrored_side = new Set<StrokeId>();
  // Where the time of a render request and of a frame goes: milliseconds per part,
  // summed since the last reset, read by profiling scripts as `window.debug_frame_timings`
  // (reset with `window.debug_reset_frame_timings()`). performance.now() around each part
  // is cheap enough to leave in.
  type FrameTimings = { frame_count: number; full_request_count: number; camera_request_count: number; hover_request_count: number; milliseconds_by_part: Record<string, number> };
  let frame_timings: FrameTimings = { frame_count: 0, full_request_count: 0, camera_request_count: 0, hover_request_count: 0, milliseconds_by_part: {} };
  function add_frame_timing(part: string, started_ms: number): void {
    const elapsed_ms = performance.now() - started_ms;
    frame_timings.milliseconds_by_part[part] = (frame_timings.milliseconds_by_part[part] ?? 0) + elapsed_ms;
  }
  function timed<T>(part: string, step: () => T): T {
    const started_ms = performance.now();
    const result = step();
    add_frame_timing(part, started_ms);
    return result;
  }
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
  // HOT_COLOR so the user knows before committing. In width mode only lines are hot
  // (what the pen-down grabs), drawn tinted (resolve_stroke_width_tint).
  type HotItem =
    | { kind: "stroke"; stroke_id: StrokeId }
    | { kind: "handle"; key: "p1" | "p2" } // handle of the selected stroke
    | { kind: "vertex"; vertex: VertexId } // any document vertex (endpoints, landmarks, pins)
    | { kind: "patch"; patch: number }; // index into tablet_document.patches, drawn in PATCH_HOT_COLOR
  let hover_screen: V2 | null = null; // null while the pen is down or off the canvas
  let hot_item: HotItem | null = null;
  // The running width drag (stroke_width_drag_begin), and the line and number the tint
  // shows this frame (resolve_stroke_width_tint).
  let stroke_width_drag: StrokeWidthDrag | null = null;
  type StrokeWidthTint = { stroke_id: StrokeId; radii_index: number };
  let stroke_width_tint: StrokeWidthTint | null = null;

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
    // A vertex behind a drawn fill can't be seen, so it isn't picked
    // (plan-tablet-pick-ignores-hidden-vertices Q1); with "surf" off nothing hides (Q4).
    // The same holds for a line, judged at its point nearest the pen.
    // The fills' grids are built on the first vertex or line in range, once per pick.
    const is_point_visible = point_visibility_test_for_one_pick();
    const visibility_filter = surface_drawn ? is_point_visible : undefined;
    // The p0 and p3 ends of the active line are drawn on top like its handles, so
    // they stay pickable behind a fill; they still compete on distance.
    const active_line_end_vertices: VertexId[] = [];
    if (edit_state !== null) {
      const active_stroke = stroke_by_id(tablet_document, edit_state.stroke_id);
      active_line_end_vertices.push(active_stroke.p0_vertex, active_stroke.p3_vertex);
    }
    const is_vertex_pickable = (world_position: V3, vertex_id: VertexId): boolean => {
      if (active_line_end_vertices.includes(vertex_id)) return true;
      return is_point_visible(world_position);
    };
    const vertex_visibility_filter = surface_drawn ? is_vertex_pickable : undefined;
    const vertex = timed("pick.vertex", () => pick_vertex(tablet_document, camera, screen, canvas, pickable_layers(),vertex_visibility_filter));
    if (vertex !== null) return { kind: "vertex", vertex };
    const stroke_id = timed("pick.stroke", () => pick_stroke(tablet_document, camera, screen, canvas, pickable_layers(),visibility_filter));
    if (stroke_id !== null) return { kind: "stroke", stroke_id };
    const grid_of_patch = (patch: Patch): SurfaceGrid | null => cached_patch_geometry(patch_geometry_cache, patch, tablet_document)?.grid ?? null;
    const patch = timed("pick.patch", () => pick_patch(tablet_document, camera, screen, canvas, pickable_layers(), grid_of_patch)); // fill = last resort (Q10)
    return patch === null ? { kind: "nothing" } : { kind: "patch", patch };
  }

  // Whether a world point is in front of every drawn fill, for the picks of one pen
  // position: the fills' grids are built on the first call and kept for the later ones.
  function point_visibility_test_for_one_pick(): (world_position: V3) => boolean {
    const eye = camera_eye(camera);
    let occluding_grids: SurfaceGrid[] | null = null;
    return (world_position: V3): boolean => {
      if (occluding_grids === null) occluding_grids = timed("pick.occluding_grids", drawn_surface_grids);
      const grids = occluding_grids;
      return timed("pick.occlusion_tests", () => point_is_visible_past_surfaces(grids, eye, world_position));
    };
  }

  // The line a width-mode pen-down at `screen` grabs: the nearest pickable line, judged
  // like a tap's line pick (hidden behind a drawn fill = not picked), but ahead of
  // vertices and handles, which width mode never moves.
  function pick_width_mode_stroke(screen: V2): StrokeId | null {
    const visibility_filter = surface_drawn ? point_visibility_test_for_one_pick() : undefined;
    return timed("pick.stroke", () => pick_stroke(tablet_document, camera, screen, canvas, pickable_layers(),visibility_filter));
  }

  function resolve_hot_item(): HotItem | null {
    if (hover_screen === null || armed_tool === "line") return null;
    if (armed_tool !== null) {
      // Pin / split armed: the tap feeds the tool, which only reads curves.
      const picked = pick_stroke(tablet_document, camera, hover_screen, canvas, pickable_layers());
      return picked === null ? null : { kind: "stroke", stroke_id: picked };
    }
    if (width_mode_is_on()) {
      // Width mode: the pen grabs lines only, so only a line lights up.
      const picked = pick_width_mode_stroke(hover_screen);
      return picked === null ? null : { kind: "stroke", stroke_id: picked };
    }
    const target = pick_tap_target(hover_screen);
    if (target.kind === "nothing") return null;
    return target;
  }

  // The snapshot `setup.on_document_edited` last saw (the loaded document counts as one).
  let last_reported_history_snapshot: string | null = null;
  // True unless the page says another of its views has the keyboard.
  function owns_keyboard(): boolean {
    return setup.owns_keyboard === undefined || setup.owns_keyboard();
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
    const request_started_ms = performance.now();
    frame_timings.full_request_count++;
    timed("request.midline", () => enforce_midline(tablet_document));
    timed("request.pins", () => update_pinned_vertex_positions(tablet_document));
    // Anything may have changed the document since the last full request: each patch's
    // inputs are compared again, once, by the next frame or pick that needs its grid, and
    // only the patches that changed are built again.
    begin_patch_geometry_check_pass(patch_geometry_cache, tablet_document);
    const refresh_started_ms = performance.now();
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
    refresh_on_surface_button_armed();
    refresh_one_handle_button_armed();
    refresh_always_drawn_button_armed();
    refresh_smooth_button_armed();
    refresh_history_panel();
    refresh_width_panel();
    add_frame_timing("request.refresh_panels", refresh_started_ms);
    persistence.history_snapshot = current_history_snapshot();
    timed("request.autosave", () => schedule_autosave(persistence, tablet_document, camera));
    if (setup.on_document_edited !== undefined && persistence.history_snapshot !== last_reported_history_snapshot) {
      last_reported_history_snapshot = persistence.history_snapshot;
      setup.on_document_edited(tablet_document);
    }
    add_frame_timing("request.total", request_started_ms);
    next_frame_is_camera_only = false;
    request_frame();
  }
  // True while every request since the last frame came from request_camera_render: the
  // next frame then only follows the camera (see render_now). A full request wins.
  let next_frame_is_camera_only = false;
  // A render request from a camera move alone (orbit, pan, zoom): the document is
  // unchanged, so the midline and pin passes, the panel refreshes and the history
  // snapshot are left alone. Only the orbit/pan/zoom call sites use it; anything that
  // may have touched the document or the selection goes through request_render.
  function request_camera_render(): void {
    frame_timings.camera_request_count++;
    // The camera is never saved on its own; this only keeps a pending save armed.
    schedule_autosave(persistence, tablet_document, camera);
    if (!frame_requested) next_frame_is_camera_only = true;
    request_frame();
  }
  // A render request from the pen hovering alone: the document and the selection are
  // unchanged, only what is hot may be, so the passes and refreshes of request_render
  // are skipped (and the patch grids kept), but the frame is a full one: it picks the
  // hot item and rebuilds the meshes that show it.
  function request_hover_render(): void {
    frame_timings.hover_request_count++;
    // Nothing to save; this only keeps a pending save armed, as a camera request does.
    schedule_autosave(persistence, tablet_document, camera);
    next_frame_is_camera_only = false;
    request_frame();
  }
  function request_frame(): void {
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
    const frame_started_ms = performance.now();
    frame_timings.frame_count++;
    const frame_is_camera_only = next_frame_is_camera_only;
    next_frame_is_camera_only = false;
    // During a camera gesture nothing is hovered on purpose, and the pick (a ray against
    // every patch) was a third of a hovered frame: no hot item until the pen moves again.
    if (frame_is_camera_only) hot_item = null;
    else hot_item = timed("frame.hot_item", resolve_hot_item);
    stroke_width_tint = resolve_stroke_width_tint();
    refresh_width_panel_picked_number();
    // Ribbons are camera-facing (desktop parity) — retessellate every frame.
    timed("frame.stroke_mesh", () => rebuild_stroke_mesh(edit_state === null ? null : edit_state.stroke_id));
    // See surface_mesh_built_without_hot_patch: a camera-only frame usually keeps the fills.
    if (!frame_is_camera_only || !surface_mesh_built_without_hot_patch) timed("frame.surface_mesh", rebuild_surface_mesh);
    set_surface_camera_forward(renderer, camera_basis(camera).forward);
    timed("frame.reference_mesh", rebuild_reference_mesh);
    timed("frame.edit_overlay", rebuild_edit_overlay);
    const view_projection = camera_view_projection(camera, canvas.width / canvas.height);
    const clip = clip_plane.enabled ? clip_plane : null;
    timed("frame.draw", () => render_frame(renderer, view_projection, clip, () => {
      draw_mesh_translucent(reference_translucent, view_projection, camera_eye(camera), clip);
    }));
    timed("frame.stroke_labels", rebuild_stroke_labels);
    timed("frame.selection_readout", refresh_selection_readout);
    add_frame_timing("frame.total", frame_started_ms);
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
      push_label(stroke.name, stroke_drawn_point(stroke, tablet_document, 0.5), 0);
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
      position: absolute; left: max(12px, env(safe-area-inset-left)); bottom: max(12px, env(safe-area-inset-bottom));
      z-index: 20; padding: 6px 10px; border-radius: 6px; border: 1px solid #444; background: #222228cc;
      font: 13px monospace; color: #ccc; white-space: pre; cursor: copy;
    }
    #selection_readout.copied { border-color: #6c9f5a; }
    #selection_readout.copy_failed { border-color: #e05a5a; }`;
  document.head.appendChild(selection_readout_style);
  const selection_readout = document.createElement("div");
  selection_readout.id = "selection_readout";
  selection_readout.title = "click to copy";
  // In the canvas's offset parent (the body on a full-page sketchpad, the embedding box on
  // fit-hairline-to-loomis-girl), absolute.
  (canvas.offsetParent ?? document.body).append(selection_readout);
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

  // Per-frame meshes reuse one sink each so a frame allocates nothing for them.
  const stroke_sink = create_vertex_sink(1 << 15);
  const surface_sink = create_vertex_sink(1 << 15);
  const reference_sink = create_vertex_sink(1 << 15);

  function rebuild_stroke_mesh(highlighted_stroke: StrokeId | null): void {
    const vertices = stroke_sink;
    reset_vertex_sink(vertices);
    const selected_vertex_pin = selected_vertex === null ? null : pin_by_vertex(tablet_document, selected_vertex);
    strokes_drawn_on_stored_side.clear();
    strokes_drawn_on_mirrored_side.clear();
    for (const stroke of tablet_document.strokes) {
      if (hidden_layers.has(stroke.layer)) continue;
      if (stroke.is_hidden === true) continue;
      const highlighted = stroke.id === highlighted_stroke || extra_selection.includes(stroke.id);
      const is_pin_host = selected_vertex_pin !== null && selected_vertex_pin.host_stroke === stroke.id;
      const hot = hot_item !== null && hot_item.kind === "stroke" && hot_item.stroke_id === stroke.id;
      // The picture view draws the always-drawn strokes, on both sides; every other stroke
      // is still picked, and is drawn on its stored side only while it is hot or selected.
      // The contours below are drawn either way
      // (plan-head-picture-from-contours-and-chosen-lines Q1, Q2).
      const is_in_picture = !is_picture_view || stroke.is_always_drawn === true;
      const is_shown_for_picking = hot || highlighted || (stroke_width_tint !== null && stroke_width_tint.stroke_id === stroke.id);
      if (!is_in_picture && !is_shown_for_picking) continue;
      // The stroke's own color and lightness: what its mirrored side always shows, and
      // its stored side while it is neither hot, selected, nor the host of a pin.
      const own_color = stroke_own_color(stroke);
      const own_lightness_additions = stroke.lightness_additions ?? null;
      let color = own_color;
      let lightness_additions = own_lightness_additions;
      // The line a width drag changes, or a hovering pen in width mode would grab: its own
      // color on both sides, with HOT_COLOR mixed in where the picked number acts.
      let tint: RibbonTint | null = null;
      if (stroke_width_tint !== null && stroke_width_tint.stroke_id === stroke.id) {
        tint = { color: HOT_COLOR, radii_index: stroke_width_tint.radii_index };
      } else if (hot) {
        color = HOT_COLOR;
        lightness_additions = null;
      } else if (highlighted) {
        color = HIGHLIGHT_COLOR;
        lightness_additions = null;
      } else if (is_pin_host) {
        color = PIN_HOST_STROKE_COLOR;
        lightness_additions = null;
      }
      append_stroke_ribbon(stroke, tablet_document, camera, color, lightness_additions, tint, /* is_mirrored_side */ false, vertices);
      strokes_drawn_on_stored_side.add(stroke.id);
      const is_drawn_on_mirrored_side = is_in_picture && tablet_document.is_mirrored_across_midline === true && stroke.midline !== true && stroke.is_one_sided !== true;
      if (is_drawn_on_mirrored_side) {
        append_stroke_ribbon(stroke, tablet_document, camera, own_color, own_lightness_additions, tint, /* is_mirrored_side */ true, vertices);
        strokes_drawn_on_mirrored_side.add(stroke.id);
      }
    }
    timed("frame.contours", () => append_contour_ribbons(vertices));
    timed("frame.upload", () => set_stroke_mesh(renderer, vertex_sink_view(vertices)));
  }

  // Computed contours share the stroke mesh so they get the same depth bias
  // and draw order as drawn strokes; they are derived per frame, never stored.
  function append_contour_ribbons(vertices: VertexSink): void {
    const eye = camera_eye(camera);
    const outline_pieces = create_outline_pieces();
    frame_contour_geometries.length = 0;
    frame_contour_mirroring.length = 0;
    for (const patch of tablet_document.patches) {
      frame_contour_geometries.push(null);
      frame_contour_mirroring.push(false);
      const layer = patch_layer(patch, tablet_document);
      if (hidden_layers.has(layer)) continue;
      // A flat fill has no shape to turn away from the eye: no contour line.
      if (patch.fill_color !== undefined) continue;
      const geometry = timed("contours.grids", () => cached_patch_geometry(patch_geometry_cache, patch, tablet_document));
      if (geometry === null) continue;
      const is_mirrored = tablet_document.is_mirrored_across_midline === true && patch.is_one_sided !== true;
      frame_contour_geometries[frame_contour_geometries.length - 1] = geometry;
      frame_contour_mirroring[frame_contour_mirroring.length - 1] = is_mirrored;
      const grid = geometry.grid;
      const color = locked_layers.has(layer) ? LOCKED_LAYER_STROKE_COLOR : STROKE_COLOR;
      // Width and lightness from the patch's boundary strokes (plan Q8), the same on both sides.
      const blend = timed("contours.widths", () => cached_contour_width_blend(contour_width_blends, geometry.fill, grid, tablet_document));
      const partials = cached_surface_partials(grid);
      const chains = timed("contours.extract", () => extract_placed_contour_chains(grid, eye, partials));
      append_outline_chains(outline_pieces, chains, partials, grid, blend, /* has_lightness */ true, color);
      if (is_mirrored) {
        const mirrored_grid = timed("contours.grids", () => cached_mirrored_grid(geometry));
        const mirrored_partials = cached_surface_partials(mirrored_grid);
        const mirrored_chains = timed("contours.extract", () => extract_placed_contour_chains(mirrored_grid, eye, mirrored_partials));
        append_outline_chains(outline_pieces, mirrored_chains, mirrored_partials, mirrored_grid, blend, /* has_lightness */ true, color);
      }
    }
    // The contours lying on the strokes between patches (boundary_contour.ts).
    if (border_contours === null || !frame_contours_match_built_border_contours()) {
      border_contours = timed("contours.borders.build", () => build_border_contours());
    }
    const built_border_contours = border_contours;
    outline_pieces.runs = timed("contours.borders", () => border_runs_of(built_border_contours, camera, border_piece_look_of));
    // The chains and the runs joined into lines (outline_lines.ts), drawn line by line.
    const outline_lines = timed("contours.lines", () => outline_lines_of(outline_pieces, built_border_contours, tablet_document, camera));
    timed("contours.ribbons", () => append_outline_ribbons(outline_pieces, outline_lines, tablet_document, camera, vertices));
  }
  function frame_contours_match_built_border_contours(): boolean {
    if (frame_contour_geometries.length !== border_contours_built_from_geometries.length) return false;
    for (let index = 0; index < frame_contour_geometries.length; index++) {
      if (frame_contour_geometries[index] !== border_contours_built_from_geometries[index]) return false;
      if (frame_contour_mirroring[index] !== border_contours_built_from_mirroring[index]) return false;
    }
    return true;
  }
  function build_border_contours(): BorderContours {
    border_contours_built_from_geometries.length = 0;
    border_contours_built_from_mirroring.length = 0;
    const surfaces: BorderSurface[] = [];
    for (let index = 0; index < frame_contour_geometries.length; index++) {
      const geometry = frame_contour_geometries[index];
      const is_mirrored = frame_contour_mirroring[index];
      border_contours_built_from_geometries.push(geometry);
      border_contours_built_from_mirroring.push(is_mirrored);
      if (geometry === null) continue;
      surfaces.push({ fill: geometry.fill, grid: geometry.grid, partials: cached_surface_partials(geometry.grid), is_mirrored_side: false });
      if (!is_mirrored) continue;
      const mirrored_grid = cached_mirrored_grid(geometry);
      surfaces.push({ fill: geometry.fill, grid: mirrored_grid, partials: cached_surface_partials(mirrored_grid), is_mirrored_side: true });
    }
    return border_contours_of(surfaces, tablet_document);
  }
  // A border piece is the stroke drawn in its own color and lightness, on a side where
  // rebuild_stroke_mesh did not already draw it whole.
  function border_piece_look_of(stroke: Stroke, is_mirrored_side: boolean): BorderPieceLook | null {
    if (hidden_layers.has(stroke.layer)) return null;
    const drawn_strokes = is_mirrored_side ? strokes_drawn_on_mirrored_side : strokes_drawn_on_stored_side;
    if (drawn_strokes.has(stroke.id)) return null;
    return { color: stroke_own_color(stroke), lightness_additions: stroke.lightness_additions ?? null };
  }
  function stroke_own_color(stroke: Stroke): Rgb {
    if (stroke.line_color !== undefined) return stroke.line_color;
    if (locked_layers.has(stroke.layer)) return LOCKED_LAYER_STROKE_COLOR;
    return STROKE_COLOR;
  }
  // Keyed on the grid object: invalidating the patch geometry cache makes new grids, so a
  // stale entry is never looked up again (and the WeakMap lets it go).
  function cached_surface_partials(grid: SurfaceGrid): SurfacePartials {
    const cached = surface_partials_by_grid.get(grid);
    if (cached !== undefined) return cached;
    const partials = surface_partials(grid);
    surface_partials_by_grid.set(grid, partials);
    return partials;
  }

  // The fills carry base colors and normals and the shader lights them from the camera,
  // so the uploaded mesh depends on the document, the selected and hot patch, the "surf"
  // button and the layer toggles, never on the camera. Each of those changes goes through
  // request_render; a camera-only frame changes only the hot patch, to none. So a
  // camera-only frame keeps the uploaded mesh when it was built with no hot patch.
  let surface_mesh_built_without_hot_patch = false;

  // One drawn patch's run of floats in the surface mesh and what it was written from: the
  // same geometry object (a cached geometry is never changed in place), color, flat or
  // shaded, and mirrored side give the same floats, so a frame keeps the run as it is.
  // A patch gives one slice for its stored side and, when mirrored, one for its mirrored side.
  type SurfaceSlice = { geometry: PatchGeometry; color: Rgb; is_flat: boolean; is_drawn_on_mirrored_side: boolean; start: number; end: number };
  // The slices the surface buffer holds now, in mesh order (the sink holds the same
  // floats: only rebuild_surface_mesh writes it); null while the buffer holds no known
  // slices (nothing uploaded yet), so the next upload is whole.
  let uploaded_surface_slices: SurfaceSlice[] | null = null;

  function rebuild_surface_mesh(): void {
    const vertices = surface_sink;
    reset_vertex_sink(vertices);
    const hot_patch = hot_item !== null && hot_item.kind === "patch" ? hot_item.patch : null;
    surface_mesh_built_without_hot_patch = hot_patch === null;
    const previous_slices = uploaded_surface_slices;
    const slices: SurfaceSlice[] = [];
    const changed_ranges: FloatRange[] = [];
    const mark_changed = (start: number, end: number): void => {
      const last = changed_ranges[changed_ranges.length - 1];
      if (last !== undefined && last.end === start) last.end = end;
      else changed_ranges.push({ start, end });
    };
    // Writes the patch's floats unless the slice at the same place last frame was written
    // from the same inputs: then its floats are still in the sink and in the buffer.
    const write_slice = (geometry: PatchGeometry, color: Rgb, is_flat: boolean, is_drawn_on_mirrored_side: boolean, write: () => void): void => {
      const start = vertices.length;
      const previous = previous_slices === null ? undefined : previous_slices[slices.length];
      const is_unchanged = previous !== undefined
        && previous.start === start
        && previous.geometry === geometry
        && previous.color.r === color.r && previous.color.g === color.g && previous.color.b === color.b
        && previous.is_flat === is_flat
        && previous.is_drawn_on_mirrored_side === is_drawn_on_mirrored_side;
      if (is_unchanged) {
        vertices.length = previous.end;
      } else {
        write();
        mark_changed(start, vertices.length);
      }
      slices.push({ geometry, color: { r: color.r, g: color.g, b: color.b }, is_flat, is_drawn_on_mirrored_side, start, end: vertices.length });
    };
    const selected_vertex_surface_pin = selected_vertex === null ? null : surface_pin_by_vertex(tablet_document, selected_vertex);
    tablet_document.patches.forEach((patch, index) => {
      const layer = patch_layer(patch, tablet_document);
      if (hidden_layers.has(layer)) return;
      const is_pin_host = selected_vertex_surface_pin !== null && selected_vertex_surface_pin.patch === patch.id;
      // The selected and hot patches, and the one the selected vertex is pinned to, are
      // treated alike below; selected wins the color, then hot.
      const is_selected_or_hot = index === selected_patch || index === hot_patch || is_pin_host;
      // The selected patch is drawn even with no fill, so the selection stays visible.
      // The hot one too, so the pen shows what a tap would select.
      if (!surface_drawn && !is_selected_or_hot) return;
      // The mirrored side never shows selected, hot or pin host: it is drawn like any
      // other patch, so not at all while surfaces are off.
      const is_drawn_on_mirrored_side = surface_drawn && tablet_document.is_mirrored_across_midline === true && patch.is_one_sided !== true;
      const geometry = cached_patch_geometry(patch_geometry_cache, patch, tablet_document);
      if (geometry === null) return;
      // The patch's own look: flat in its fill color when it has one, else shaded.
      const fill_color = patch.fill_color;
      const own_shaded_color = locked_layers.has(layer) ? LOCKED_LAYER_SURFACE_COLOR : SURFACE_COLOR;
      // The stored side: a patch with its own fill color is drawn flat in it, unless it is the selected (or hot) one.
      if (fill_color !== undefined && !is_selected_or_hot) {
        write_slice(geometry, fill_color, /* is_flat */ true, /* is_drawn_on_mirrored_side */ false, () => {
          append_flat_grid_mesh_with_normals(geometry.grid, fill_color, vertices);
        });
      } else {
        let color = own_shaded_color;
        if (index === selected_patch) {
          color = PATCH_HIGHLIGHT_COLOR;
        } else if (index === hot_patch) {
          color = PATCH_HOT_COLOR;
        } else if (is_pin_host) {
          color = PIN_HOST_PATCH_COLOR;
        }
        write_slice(geometry, color, /* is_flat */ false, /* is_drawn_on_mirrored_side */ false, () => {
          append_patch_geometry_mesh_with_normals(geometry, color, vertices);
        });
      }
      if (!is_drawn_on_mirrored_side) return;
      if (fill_color !== undefined) {
        write_slice(geometry, fill_color, /* is_flat */ true, /* is_drawn_on_mirrored_side */ true, () => {
          append_flat_grid_mesh_with_normals(cached_mirrored_grid(geometry), fill_color, vertices);
        });
      } else {
        write_slice(geometry, own_shaded_color, /* is_flat */ false, /* is_drawn_on_mirrored_side */ true, () => {
          append_grid_mesh_with_normals(cached_mirrored_grid(geometry), own_shaded_color, vertices);
        });
      }
    });
    // The spheres are rebuilt from the document each frame: always written.
    const spheres_start = vertices.length;
    if (surface_drawn) {
      for (const grid of drawn_sphere_grids()) append_grid_mesh_with_normals(grid, SPHERE_COLOR, vertices);
    }
    mark_changed(spheres_start, vertices.length);
    uploaded_surface_slices = slices;
    if (previous_slices === null) timed("frame.upload", () => set_surface_mesh(renderer, vertex_sink_view(vertices)));
    else timed("frame.upload", () => set_surface_mesh_ranges(renderer, vertex_sink_view(vertices), changed_ranges));
  }

  // The spheres on visible layers as grids, with their mirror images when the document
  // is mirrored across the midline.
  function drawn_sphere_grids(): SurfaceGrid[] {
    const grids: SurfaceGrid[] = [];
    for (const sphere of tablet_document.spheres) {
      if (hidden_layers.has(sphere.layer)) continue;
      grids.push(...sphere_drawn_grids(sphere, tablet_document, tablet_document.is_mirrored_across_midline === true));
    }
    return grids;
  }

  // The grids of every fill rebuild_surface_mesh draws while "surf" is on: patches
  // on visible layers (locked ones too) and their mirrored halves
  // (plan-tablet-pick-ignores-hidden-vertices Q5), and the spheres. The fills hide
  // vertices from picking.
  function drawn_surface_grids(): SurfaceGrid[] {
    const grids: SurfaceGrid[] = drawn_sphere_grids();
    for (const patch of tablet_document.patches) {
      if (hidden_layers.has(patch_layer(patch, tablet_document))) continue;
      const geometry = cached_patch_geometry(patch_geometry_cache, patch, tablet_document);
      if (geometry === null) continue;
      grids.push(geometry.grid);
      const is_drawn_on_mirrored_side = tablet_document.is_mirrored_across_midline === true && patch.is_one_sided !== true;
      if (is_drawn_on_mirrored_side) grids.push(cached_mirrored_grid(geometry));
    }
    return grids;
  }

  // Camera-facing round marker.
  // A disc of radius `half_size`, as a triangle fan.
  function append_billboard_disc(
    center: V3, half_size: number, right: V3, up: V3,
    color: { r: number; g: number; b: number }, out: number[],
  ): void {
    const segment_count = 16;
    const rim_point = (index: number) => {
      const angle = (index / segment_count) * Math.PI * 2;
      return v3_add(center, v3_add(v3_scale(right, Math.cos(angle) * half_size), v3_scale(up, Math.sin(angle) * half_size)));
    };
    for (let index = 0; index < segment_count; index++) {
      for (const corner of [center, rim_point(index), rim_point(index + 1)]) {
        out.push(corner.x, corner.y, corner.z, color.r, color.g, color.b);
      }
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

    // Vertices have no marker of their own: only the hot vertex (grown) and the
    // selected vertex (anchor-sized, in the highlight colour) draw.
    const drawn_layers = visible_layers();
    for (const vertex of tablet_document.vertices) {
      if (!vertex_is_on_layers(tablet_document, vertex.id, drawn_layers)) continue;
      const hot = hot_item !== null && hot_item.kind === "vertex" && hot_item.vertex === vertex.id;
      const world_position = vertex_world_position(tablet_document, vertex);
      if (vertex.id === selected_vertex || vertex.id === extra_vertex) {
        append_billboard_disc(world_position, anchor_half, basis.right, basis.up, HIGHLIGHT_COLOR, triangle_vertices);
      } else if (hot) {
        append_billboard_disc(world_position, handle_half * HOT_SIZE_SCALE, basis.right, basis.up, HOT_COLOR, triangle_vertices);
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
    const has_one_handle = selected_stroke.has_one_handle === true;
    const single_handle_point = stroke_single_handle_point(selected_stroke, tablet_document);
    if (has_one_handle) {
      push_line(points.p0, single_handle_point);
      push_line(points.p3, single_handle_point);
    } else {
      push_line(points.p0, points.p1);
      push_line(points.p3, points.p2);
    }
    // The hot handle / endpoint / pin draws bigger and in HOT_COLOR.
    const is_hot_handle = (key: "p1" | "p2") => hot_item !== null && hot_item.kind === "handle" && hot_item.key === key;
    const is_hot_vertex = (vertex: VertexId) => hot_item !== null && hot_item.kind === "vertex" && hot_item.vertex === vertex;
    const push_marker = (center: V3, half_size: number, color: { r: number; g: number; b: number }, hot: boolean) => {
      append_billboard_disc(
        center, hot ? half_size * HOT_SIZE_SCALE : half_size, basis.right, basis.up,
        hot ? HOT_COLOR : color, triangle_vertices,
      );
    };
    // The selected handle (the nudge keys' target) draws anchor-sized.
    const selected_handle = edit_state.selected_handle;
    if (has_one_handle) {
      // The single handle answers to "p1" when picked; any selected handle is this one.
      push_marker(single_handle_point, selected_handle !== null ? anchor_half : handle_half, HANDLE_COLOR, is_hot_handle("p1"));
    } else {
      push_marker(points.p1, selected_handle === "p1" ? anchor_half : handle_half, HANDLE_COLOR, is_hot_handle("p1"));
      push_marker(points.p2, selected_handle === "p2" ? anchor_half : handle_half, HANDLE_COLOR, is_hot_handle("p2"));
    }
    // Endpoints that are smooth knots show in the knot color so it is clear the
    // neighbour handle will follow. Pinned vertices get no color of their own.
    const anchor_color = (vertex: VertexId) => {
      if (smooth_knots_at_vertex(tablet_document, vertex).length > 0) return KNOT_COLOR;
      return ANCHOR_COLOR;
    };
    push_marker(points.p0, anchor_half, anchor_color(selected_stroke.p0_vertex), is_hot_vertex(selected_stroke.p0_vertex));
    push_marker(points.p3, anchor_half, anchor_color(selected_stroke.p3_vertex), is_hot_vertex(selected_stroke.p3_vertex));
    set_overlay_lines(renderer, new Float32Array(line_vertices));
    set_overlay_triangles(renderer, new Float32Array(triangle_vertices));
  }

  function update_preview_line(): void {
    if (line_state === null) {
      set_preview_line(renderer, new Float32Array(0));
      return;
    }
    // The straight segment the stroke will be, from the start to the end point.
    const vertices: number[] = [];
    for (const point of [line_state.start_world, line_state.end_world]) {
      vertices.push(point.x, point.y, point.z, PREVIEW_COLOR.r, PREVIEW_COLOR.g, PREVIEW_COLOR.b);
    }
    set_preview_line(renderer, new Float32Array(vertices));
  }

  // How close a pen end of the line tool must come to the centre plane, on screen, to
  // land on it (the native line tool's line_tool_midline_px).
  const LINE_TOOL_MIDLINE_SNAP_RADIUS_PIXELS = 8;

  // A new end vertex of the line tool that the pen put within
  // LINE_TOOL_MIDLINE_SNAP_RADIUS_PIXELS of the centre plane lands on it: x = 0 in its
  // bone's space, held there by the vertex's `midline` flag. Measured as the native
  // line tool does: on screen, from the end to its foot on the plane. Returns whether
  // it landed.
  function land_new_line_end_on_midline(vertex_id: VertexId): boolean {
    const vertex = vertex_by_id(tablet_document, vertex_id);
    const bone_to_world = world_from_bone(tablet_document, vertex.bone_id);
    const foot_on_midline = mat4_transform_point(bone_to_world, v3(0, vertex.position.y, vertex.position.z));
    const end_screen = camera_world_to_screen(camera, vertex_world_position(tablet_document, vertex), canvas.clientWidth, canvas.clientHeight);
    const foot_screen = camera_world_to_screen(camera, foot_on_midline, canvas.clientWidth, canvas.clientHeight);
    if (end_screen === null || foot_screen === null) return false;
    const distance_pixels = Math.hypot(end_screen.x - foot_screen.x, end_screen.y - foot_screen.y);
    if (distance_pixels >= LINE_TOOL_MIDLINE_SNAP_RADIUS_PIXELS) return false;
    move_vertex(tablet_document, vertex_id, foot_on_midline);
    vertex.midline = true;
    return true;
  }

  // A new line in a document mirrored across the midline. Its new ends near the centre
  // plane land on it (Q4); the line is a midline stroke when both ends are on the plane
  // (an existing start vertex counts when it is at x = 0, as in the native tool). Then
  // a line with no end on the stored side and one on the mirrored side is stored
  // reflected onto the stored side (Q3, Q5), but only when both ends are new vertices:
  // an existing start vertex is never moved.
  function settle_new_line_in_mirrored_document(stroke_id: StrokeId, start_is_new_vertex: boolean): void {
    const stroke = stroke_by_id(tablet_document, stroke_id);
    let start_is_on_midline: boolean;
    if (start_is_new_vertex) {
      start_is_on_midline = land_new_line_end_on_midline(stroke.p0_vertex);
    } else {
      start_is_on_midline = vertex_by_id(tablet_document, stroke.p0_vertex).position.x === 0;
    }
    const end_is_on_midline = land_new_line_end_on_midline(stroke.p3_vertex);
    if (start_is_on_midline && end_is_on_midline) stroke.midline = true;
    if (!start_is_new_vertex) return;
    const end_x_values = [stroke.p0_vertex, stroke.p3_vertex].map((vertex_id) => vertex_position(tablet_document, vertex_id).x);
    const has_end_on_stored_side = end_x_values.some((x) => x > 0);
    const has_end_on_mirrored_side = end_x_values.some((x) => x < 0);
    if (has_end_on_stored_side || !has_end_on_mirrored_side) return;
    reflect_new_stroke_onto_stored_side(tablet_document, stroke_id);
  }

  // Line-tool pen-up: a drag commits a straight stroke and
  // auto-selects it; a tap exits the tool (Q27). Either way the tool disarms, so
  // the very next drag adjusts the fresh stroke instead of creating another.
  function line_mode_pen_up(): void {
    const was_tap = pen_max_displacement_pixels < pen_tap_max_movement_pixels;
    if (!was_tap && line_state !== null) {
      const start_is_new_vertex = line_state.start_vertex === null;
      // Every layer's vertices live on the skull bone until a mandible exists (Q15).
      const stroke_id = line_pen_up(line_state, tablet_document, active_layer, SKULL_BONE_ID);
      if (tablet_document.is_mirrored_across_midline === true) {
        settle_new_line_in_mirrored_document(stroke_id, start_is_new_vertex);
      }
      select_stroke_by_tap(stroke_id, false);
      pen_history_label = `add line ${stroke_id}`;
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
    if (!multi) {
      selected_patch = null;
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
  // or vertex and keeps the rest, the other kinds included
  // (plan-sketchpad-pin-vertex-to-selected-line.md Q1, and the patch since
  // plan-sketchpad-pin-vertex-to-selected-surface.md Q1). Vertices: two at most;
  // patches: one.
  function select_tap_target(target: TapTarget, multi: boolean): void {
    switch (target.kind) {
      case "handle":
        // Sub-selection of the selected line: the line and its extras stay (Q4).
        if (edit_state !== null) edit_state.selected_handle = target.key;
        break;
      case "vertex":
        if (multi) {
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
        if (multi) {
          // Ctrl-tap on a patch's fill sets / clears the patch, the lines and
          // vertices stay (surface plan Q1).
          selected_patch = selected_patch === target.patch ? null : target.patch;
          break;
        }
        edit_state = null;
        extra_selection = [];
        selected_vertex = null;
        extra_vertex = null;
        selected_patch = target.patch;
        break;
      case "nothing":
        edit_state = null;
        extra_selection = [];
        selected_vertex = null;
        extra_vertex = null;
        selected_patch = null;
        break;
    }
  }

  // The strokes with an end on the vertex.
  function strokes_ending_on_vertex(vertex: VertexId): StrokeId[] {
    return tablet_document.strokes
      .filter((stroke) => stroke.p0_vertex === vertex || stroke.p3_vertex === vertex)
      .map((stroke) => stroke.id);
  }

  // Selected-stroke pen-up: a drag (control point or orbit)
  // just ends. A tap moved nothing: with a pick tool armed it feeds the tool,
  // otherwise it selects what it hit (select_tap_target).
  // A drag of a handle refits the stroke if it is on the surface; a drag of an end
  // or of a pinned vertex refits every on-surface stroke ending on that vertex
  // (refit_on_surface_stroke_control_points), inside the drag's history step.
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
      const stroke = stroke_by_id(tablet_document, edit_state.stroke_id);
      if (edit_state.dragging_pin !== null) {
        refit_on_surface_stroke_control_points(tablet_document, strokes_ending_on_vertex(edit_state.dragging_pin));
      } else if (edit_state.dragging === "p0" || edit_state.dragging === "p3") {
        const dragged_vertex = edit_state.dragging === "p0" ? stroke.p0_vertex : stroke.p3_vertex;
        refit_on_surface_stroke_control_points(tablet_document, strokes_ending_on_vertex(dragged_vertex));
      } else if (edit_state.dragging !== null) {
        refit_on_surface_stroke_control_points(tablet_document, [stroke.id]);
      }
    }
    edit_pen_up(edit_state);
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
      // strokes joined at a plain junction; the [0, t] half stays selected. Only
      // the selection is a candidate so a tap at a junction is unambiguous. A
      // tap on a vertex pinned to the selection cuts exactly there, reusing it.
      const selected_id = edit_state.stroke_id;
      pen_history_label = `split stroke ${selected_id}`;
      const pinned = pick_pin_on_stroke(selected_id, position);
      const selected_stroke = stroke_by_id(tablet_document, selected_id);
      if (pinned !== null) {
        const cut_length_fraction = stroke_length_fraction_at(selected_stroke, tablet_document, pinned.t);
        split_stroke(tablet_document, selected_id, pinned.t, pinned.vertex, cut_length_fraction);
      } else {
        const nearest = nearest_t_on_stroke_screen(tablet_document, selected_id, camera, position, canvas);
        if (nearest.distance < STROKE_PICK_RADIUS_PIXELS) {
          const cut_length_fraction = stroke_length_fraction_at(selected_stroke, tablet_document, nearest.t);
          split_stroke(tablet_document, selected_id, nearest.t, null, cut_length_fraction);
        }
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
  const on_surface_button = document.getElementById("on_surface_button") as HTMLButtonElement;
  // Acts on the selected lines only (Khoa, 2026-11-15; the sticky line-tool mode of
  // plan-hairline-drawn-on-surface Q8 was removed the same day): it flips the
  // on_surface flag of the selected lines (all to the opposite of the primary's) and is
  // lit by the primary's flag. Their ends keep their pins: only the curve between the
  // ends is drawn on the surface.
  // Turning it on refits the lines it switched (refit_on_surface_stroke_control_points):
  // an end with no pin is pinned to the nearest surface point (unless it is on the
  // midline or a stroke bounding a patch ends on it), and the handles are fitted to the drawn line.
  function refresh_on_surface_button_armed(): void {
    const armed = edit_state !== null && stroke_by_id(tablet_document, edit_state.stroke_id).on_surface === true;
    on_surface_button.classList.toggle("armed", armed);
  }
  on_surface_button.addEventListener("click", () => {
    if (edit_state === null) return;
    const strokes = selected_strokes();
    const turn_on = stroke_by_id(tablet_document, edit_state.stroke_id).on_surface !== true;
    begin_history_step(history, tablet_document);
    const switched_on: StrokeId[] = [];
    for (const id of strokes) {
      const stroke = stroke_by_id(tablet_document, id);
      if (turn_on && stroke.on_surface !== true) switched_on.push(id);
      if (turn_on) stroke.on_surface = true;
      else delete stroke.on_surface;
    }
    refit_on_surface_stroke_control_points(tablet_document, switched_on);
    end_history_step(history, tablet_document, `${turn_on ? "set" : "clear"} on surface strokes ${strokes.join(" ")}`);
    request_render();
  });
  const one_handle_button = document.getElementById("one_handle_button") as HTMLButtonElement;
  // Like the on surface button: it switches the selected lines between one handle and
  // two (all to the opposite of the primary's) and is lit by the primary's flag.
  function refresh_one_handle_button_armed(): void {
    const armed = edit_state !== null && stroke_by_id(tablet_document, edit_state.stroke_id).has_one_handle === true;
    one_handle_button.classList.toggle("armed", armed);
  }
  one_handle_button.addEventListener("click", () => {
    if (edit_state === null) return;
    const strokes = selected_strokes();
    const turn_on = stroke_by_id(tablet_document, edit_state.stroke_id).has_one_handle !== true;
    begin_history_step(history, tablet_document);
    set_strokes_have_one_handle(tablet_document, strokes, turn_on);
    end_history_step(history, tablet_document, `${turn_on ? "one handle" : "two handles"} on strokes ${strokes.join(" ")}`);
    request_render();
  });
  const always_drawn_button = document.getElementById("always_drawn_button") as HTMLButtonElement;
  // Like the one handle button: marks the selected lines as always drawn, or unmarks them
  // (all to the opposite of the primary's), lit by the primary's flag. Also the K key.
  // In the picture view an unmarked line stops being drawn, but stays selected with its
  // handles until the selection changes (plan-head-picture-from-contours-and-chosen-lines Q3).
  function refresh_always_drawn_button_armed(): void {
    const armed = edit_state !== null && stroke_by_id(tablet_document, edit_state.stroke_id).is_always_drawn === true;
    always_drawn_button.classList.toggle("armed", armed);
  }
  function document_toggle_always_drawn(): void {
    if (edit_state === null) return;
    const strokes = selected_strokes();
    const turn_on = stroke_by_id(tablet_document, edit_state.stroke_id).is_always_drawn !== true;
    begin_history_step(history, tablet_document);
    set_strokes_always_drawn(tablet_document, strokes, turn_on);
    end_history_step(history, tablet_document, `${turn_on ? "always drawn" : "construction"} on strokes ${strokes.join(" ")}`);
    request_render();
  }
  always_drawn_button.addEventListener("click", document_toggle_always_drawn);
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
  // The selected vertex when it carries a pin of either kind (on a line or on a
  // surface): the target of the unpin button.
  function selected_pinned_vertex(): VertexId | null {
    const vertex = sole_selected_vertex();
    return vertex !== null && vertex_is_pinned(tablet_document, vertex) ? vertex : null;
  }
  // The pin button also lights up while a pinned vertex is selected — in that
  // state tapping it unpins (Q11). Re-checked every frame since the selection
  // changes on pen gestures, not just button presses.
  // With a vertex and a line (or a patch) selected and the pair allowed, the
  // button pins the vertex there instead, and its label says so.
  function refresh_pin_button_armed(): void {
    const pins_selected_pair = selected_pin_to_line_pair() !== null || selected_pin_to_surface_pair() !== null;
    pin_button.classList.toggle("armed", armed_tool === "pin" || (!pins_selected_pair && selected_pinned_vertex() !== null));
    const label = selected_pin_to_line_pair() !== null ? "pin to line" : selected_pin_to_surface_pair() !== null ? "pin to surface" : "pin";
    if (pin_button.textContent !== label) pin_button.textContent = label;
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
  // Weld: with two vertices selected (tap A, ctrl-tap B), A is welded into B:
  // A's stroke ends move onto B, B stays where it is and takes the selection.
  // Hidden when the pair is refused (see vertex_can_weld_into).
  const weld_button = document.getElementById("weld_button") as HTMLButtonElement;
  function selected_weld_pair(): { vertex: VertexId; target_vertex: VertexId } | null {
    if (selected_vertex === null || extra_vertex === null) return null;
    if (!vertex_can_weld_into(tablet_document, selected_vertex, extra_vertex)) return null;
    return { vertex: selected_vertex, target_vertex: extra_vertex };
  }
  weld_button.addEventListener("click", () => {
    const pair = selected_weld_pair();
    if (pair === null) return;
    begin_history_step(history, tablet_document);
    weld_vertex_into(tablet_document, pair.vertex, pair.target_vertex);
    end_history_step(history, tablet_document, `weld vertex ${pair.vertex} into ${pair.target_vertex}`);
    selected_vertex = pair.target_vertex;
    extra_vertex = null;
    request_render();
  });
  // Patch: fill the selected strokes (primary + extras, 2 or more). Which fill
  // they get is derived per frame from their corners (patch.ts).
  patch_button.addEventListener("click", () => {
    if (edit_state === null || extra_selection.length === 0) return;
    // Strokes that give no fill (the loop doesn't close, or it has more than 4 corners)
    // make no patch: the selection stays, so the missing stroke can be added to it.
    const fill = resolve_patch_fill({ id: -1, strokes: [edit_state.stroke_id, ...extra_selection] }, tablet_document);
    if (fill === null) {
      show_change_notice("no patch: these strokes don't close into a loop of 2 to 4 sides");
      return;
    }
    begin_history_step(history, tablet_document);
    add_patch(tablet_document, [edit_state.stroke_id, ...extra_selection]);
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
    if (edit_state === null || other === null || join_refusal_reason(tablet_document, edit_state.stroke_id, other) !== null) return;
    begin_history_step(history, tablet_document);
    const merged_id = merge_adjacent_strokes(tablet_document, edit_state.stroke_id, other);
    end_history_step(history, tablet_document, `join strokes ${edit_state.stroke_id} ${other}`);
    if (merged_id === null) return; // not adjacent (or a closed loop): selection kept
    edit_state = begin_edit_state(merged_id);
    extra_selection = [];
    request_render();
  });

  // Split: the next tap on the selected curve cuts it there into two strokes
  // joined at a plain junction.
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
  // this button cuts its host stroke exactly there, the vertex becoming the plain
  // junction (and staying selected). Shown only while such a vertex is selected -- the
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
    const cut_length_fraction = stroke_length_fraction_at(stroke_by_id(tablet_document, stroke_id), tablet_document, pin.t);
    const new_stroke = split_stroke(tablet_document, stroke_id, pin.t, vertex, cut_length_fraction);
    if (new_stroke === null) return; // pin within the end guard: nothing to record
    drop_unused_patch_strokes(tablet_document); // a patch that used one side of the cut keeps only that half
    end_history_step(history, tablet_document, `split stroke ${stroke_id} at vertex ${vertex}`);
    request_render();
  });

  // Pin: with a pinned vertex selected, unpin it (frozen in place as a free
  // vertex, still selected); otherwise, with a stroke selected, arm pin creation
  // — the next tap on the selected stroke's curve drops a vertex constrained there.
  // First of all, with a vertex and a line (or a patch) selected and the pair
  // allowed, pins the vertex there (Khoa 2026-11-10: the separate "pin to line" /
  // "pin to surface" buttons folded into this one). A line wins over a patch.
  pin_button.addEventListener("click", () => {
    if (selected_pin_to_line_pair() !== null) { pin_selected_vertex_to_line(); return; }
    if (selected_pin_to_surface_pair() !== null) { pin_selected_vertex_to_surface(); return; }
    const unpinned_vertex = selected_pinned_vertex();
    if (unpinned_vertex !== null) {
      if (unpin_is_locked()) return; // a patch's sub-curve ends here (Q3)
      begin_history_step(history, tablet_document);
      unpin_vertices(tablet_document, [unpinned_vertex]);
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
  // stroke; a vertex pinned elsewhere moves its pin (Q7). Done by the pin button,
  // which falls back to its other jobs when the pair is refused. Both stay
  // selected (Q6).
  function selected_pin_to_line_pair(): { vertex: VertexId; stroke_id: StrokeId } | null {
    if (edit_state === null || extra_selection.length > 0 || selected_vertex === null || extra_vertex !== null) return null;
    if (!vertex_can_pin_to_stroke(tablet_document, selected_vertex, edit_state.stroke_id)) return null;
    // A patch's sub-curve ends at its current pin (Q7).
    if (pin_by_vertex(tablet_document, selected_vertex) !== null && pin_is_locked(tablet_document, selected_vertex)) return null;
    return { vertex: selected_vertex, stroke_id: edit_state.stroke_id };
  }
  function pin_selected_vertex_to_line(): void {
    const pair = selected_pin_to_line_pair();
    if (pair === null) return;
    begin_history_step(history, tablet_document);
    pin_vertex_to_stroke(tablet_document, pair.vertex, pair.stroke_id);
    end_history_step(history, tablet_document, `pin vertex ${pair.vertex} to line ${pair.stroke_id}`);
    request_render();
  }

  // Pin to surface (plan-sketchpad-pin-vertex-to-selected-surface.md): with
  // exactly one vertex and one patch selected (lines or not), pins the vertex to
  // the nearest point of the patch's surface (Q4); a vertex pinned elsewhere
  // moves its pin. Done by the pin button, not when the pair is refused (Q5).
  // Both stay selected (Q6).
  function selected_pin_to_surface_pair(): { vertex: VertexId; patch_index: number } | null {
    if (selected_patch === null || selected_vertex === null || extra_vertex !== null) return null;
    const patch = tablet_document.patches[selected_patch];
    if (patch === undefined || !vertex_can_pin_to_selected_surface(tablet_document, selected_vertex, patch)) return null;
    // A patch's sub-curve ends at its current line pin.
    if (pin_by_vertex(tablet_document, selected_vertex) !== null && pin_is_locked(tablet_document, selected_vertex)) return null;
    return { vertex: selected_vertex, patch_index: selected_patch };
  }
  function pin_selected_vertex_to_surface(): void {
    const pair = selected_pin_to_surface_pair();
    if (pair === null) return;
    const patch = tablet_document.patches[pair.patch_index];
    const nearest = nearest_point_on_patch(tablet_document, patch, vertex_position(tablet_document, pair.vertex));
    if (nearest === null) return;
    begin_history_step(history, tablet_document);
    pin_vertex_to_surface(tablet_document, pair.vertex, patch, nearest.u, nearest.v);
    // The label says the patch's list position, as the selection readout does.
    end_history_step(history, tablet_document, `pin vertex ${pair.vertex} to surface ${pair.patch_index}`);
    request_render();
  }

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
      unpin_vertices(tablet_document, claimed_vertices);
    }
    end_history_step(history, tablet_document, label);
    request_render();
  });

  // Width panel: the C++ Selection panel's radii controls (game_main.cpp, search
  // "raw radii") for the selected stroke. `width` scales the stroke's own profile
  // (keeps its taper), `taper` picks a preset end shape at that width, `raw`
  // shows the four multipliers (read only; a width drag on the line changes one of
  // them, and the one it changes or would change is highlighted). One history entry per
  // slider drag or per picked value; consecutive edits of one stroke merge into one entry.
  // While the panel is open the sketchpad is in width mode (see stroke_width_drag_begin).
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
  const raw_numbers: HTMLSpanElement[] = [0, 1, 2, 3].map(() => {
    const number = document.createElement("span");
    number.className = "raw_number";
    return number;
  });
  function labelled_row(label: string, ...controls: HTMLElement[]): HTMLLabelElement {
    const row = document.createElement("label");
    row.append(label, ...controls);
    return row;
  }
  width_panel_controls.append(
    labelled_row("width", width_slider, width_readout),
    labelled_row("taper", taper_select),
    labelled_row("raw", ...raw_numbers),
  );
  // Transfer style: the first selected line (the one this panel shows) gives its width
  // and lightness to the other selected lines.
  const transfer_style_button = document.createElement("button");
  transfer_style_button.textContent = "transfer style";
  width_panel_controls.append(transfer_style_button);
  transfer_style_button.addEventListener("click", () => {
    if (edit_state === null || extra_selection.length === 0) return;
    begin_history_step(history, tablet_document);
    transfer_stroke_style(tablet_document, edit_state.stroke_id, extra_selection);
    end_history_step(history, tablet_document, `transfer style of stroke ${edit_state.stroke_id} to ${extra_selection.join(" ")}`);
    request_render();
  });
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
    transfer_style_button.disabled = extra_selection.length === 0;
    transfer_style_button.title = extra_selection.length === 0
      ? "select the line with the style first, then the lines that get it"
      : `width and lightness of line ${edit_state!.stroke_id} go to line ${extra_selection.join(", ")}`;
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
    raw_numbers.forEach((number, index) => {
      number.textContent = radii[index].toFixed(2);
    });
  }
  // Highlights the number a width drag changes, or a hovering pen would grab, when that
  // line is the one the panel shows (another line's number is not in the row). Every
  // frame, since a hover frame does not refresh the rest of the panel.
  function refresh_width_panel_picked_number(): void {
    let picked_index = -1;
    if (stroke_width_tint !== null && edit_state !== null && stroke_width_tint.stroke_id === edit_state.stroke_id) {
      picked_index = stroke_width_tint.radii_index;
    }
    raw_numbers.forEach((number, index) => {
      number.classList.toggle("picked", index === picked_index);
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
    store_stroke_radii(edit_state.stroke_id, radii);
    request_render();
  }
  // The default profile is stored as no radii at all.
  function store_stroke_radii(stroke_id: StrokeId, radii: StrokeRadii): void {
    const stroke = stroke_by_id(tablet_document, stroke_id);
    if (radii_equal(radii, DEFAULT_STROKE_RADII)) delete stroke.radii;
    else stroke.radii = radii;
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
  width_button.addEventListener("click", () => {
    width_panel.classList.toggle("open");
    width_button.classList.toggle("armed", width_panel.classList.contains("open"));
    // The hot item and the tint follow width mode, so the whole frame is redrawn.
    request_render();
  });

  // Width mode: on while the width panel is open. Pen down on any line (its stored side)
  // selects it and starts a width drag; control points, pins and handles don't move;
  // pen down on empty space orbits. Fingers are the camera as always.
  function width_mode_is_on(): boolean {
    return width_panel.classList.contains("open");
  }

  // The spot of a line nearest a screen position, on the line as its ribbon draws it
  // (drawn_stroke_points), in CSS pixels.
  type StrokeWidthGrabbedSpot = {
    length_fraction: number; // along the drawn line, 0 at p0, 1 at p3
    center_world: V3;
    center_screen: V2;
    line_direction_screen: V2; // unit, along the screen segment the spot is on
  };
  function stroke_width_grabbed_spot(stroke_id: StrokeId, screen: V2): StrokeWidthGrabbedSpot | null {
    const points = drawn_stroke_points(stroke_by_id(tablet_document, stroke_id), tablet_document);
    const point_count = points.length / 3;
    const fractions = new Float64Array(point_count);
    polyline_length_fractions(points, point_count, fractions);
    const project = camera_screen_projector(camera, canvas.clientWidth, canvas.clientHeight);
    const world_point = (index: number): V3 => v3(points[3 * index], points[3 * index + 1], points[3 * index + 2]);
    let nearest: StrokeWidthGrabbedSpot | null = null;
    let nearest_distance = Infinity;
    let previous_screen = project(world_point(0));
    for (let index = 1; index < point_count; index++) {
      const segment_start = previous_screen;
      const segment_end = project(world_point(index));
      previous_screen = segment_end;
      if (segment_start === null || segment_end === null) continue;
      const on_segment = nearest_point_on_segment(screen, segment_start, segment_end);
      if (on_segment.distance >= nearest_distance) continue;
      nearest_distance = on_segment.distance;
      const t = on_segment.t;
      const segment_x = segment_end.x - segment_start.x;
      const segment_y = segment_end.y - segment_start.y;
      const segment_length = Math.hypot(segment_x, segment_y);
      // A segment seen end-on has no direction on screen; screen-right stands in.
      let line_direction_screen: V2 = { x: 1, y: 0 };
      if (segment_length > 1e-9) line_direction_screen = { x: segment_x / segment_length, y: segment_y / segment_length };
      nearest = {
        length_fraction: fractions[index - 1] + (fractions[index] - fractions[index - 1]) * t,
        center_world: v3_add(world_point(index - 1), v3_scale(v3_sub(world_point(index), world_point(index - 1)), t)),
        center_screen: { x: segment_start.x + segment_x * t, y: segment_start.y + segment_y * t },
        line_direction_screen,
      };
    }
    return nearest;
  }

  // One pen drag that changes one width number of one line. Relative: nothing changes on
  // pen down, then the line's edge at the grabbed spot moves on screen as far as the pen
  // moves along `outward_direction_screen`.
  type StrokeWidthDrag = {
    stroke_id: StrokeId;
    radii_index: number; // the strongest number at the grabbed spot, the only one changed
    length_fraction: number; // of the grabbed spot
    radii_at_begin: StrokeRadii;
    pen_down_screen: V2;
    outward_direction_screen: V2; // unit, from the line's centre at the spot toward the pen
    pixels_per_radii_unit: number; // screen pixels of edge move per 1 of stroke_radii_at, at the spot
  };

  // Below this distance between the pen and the line's centre, the outward direction is the
  // line's screen normal instead (the pen sits on the centre of a thin line).
  const STROKE_WIDTH_DRAG_CENTER_PIXELS = 2;

  // Starts a width drag of `stroke_id` grabbed at `screen`; false when the spot can't be
  // found on screen (then the pen-down is not a width drag).
  function stroke_width_drag_begin(stroke_id: StrokeId, screen: V2): boolean {
    const spot = stroke_width_grabbed_spot(stroke_id, screen);
    if (spot === null) return false;
    const project = camera_screen_projector(camera, canvas.clientWidth, canvas.clientHeight);
    // The camera scale at the spot's own depth: one RIBBON_RADIUS step to screen-right.
    const stepped_screen = project(v3_add(spot.center_world, v3_scale(camera_basis(camera).right, RIBBON_RADIUS)));
    if (stepped_screen === null) return false;
    const pixels_per_radii_unit = Math.hypot(stepped_screen.x - spot.center_screen.x, stepped_screen.y - spot.center_screen.y);
    const to_pen_x = screen.x - spot.center_screen.x;
    const to_pen_y = screen.y - spot.center_screen.y;
    const to_pen_length = Math.hypot(to_pen_x, to_pen_y);
    let outward_direction_screen: V2 = { x: to_pen_x / to_pen_length, y: to_pen_y / to_pen_length };
    if (to_pen_length < STROKE_WIDTH_DRAG_CENTER_PIXELS) {
      outward_direction_screen = { x: -spot.line_direction_screen.y, y: spot.line_direction_screen.x };
    }
    stroke_width_drag = {
      stroke_id,
      radii_index: strongest_stroke_radii_index_at(spot.length_fraction),
      length_fraction: spot.length_fraction,
      radii_at_begin: stroke_radii(stroke_by_id(tablet_document, stroke_id)),
      pen_down_screen: screen,
      outward_direction_screen,
      pixels_per_radii_unit,
    };
    return true;
  }

  // The pen's move since pen down, along the outward direction, is the edge's move: the
  // grabbed number changes so the width at the spot changes by exactly that much.
  function stroke_width_drag_move(screen: V2): void {
    const drag = stroke_width_drag!;
    const edge_move_pixels = (screen.x - drag.pen_down_screen.x) * drag.outward_direction_screen.x
      + (screen.y - drag.pen_down_screen.y) * drag.outward_direction_screen.y;
    // The edge stops at the line's centre: the width at the spot does not go below zero
    // (or below where it began, for a line that is already negative there).
    const width_at_begin = stroke_radii_at(drag.radii_at_begin, drag.length_fraction);
    const lowest_width_change = Math.min(width_at_begin, 0) - width_at_begin;
    const width_change = Math.max(edge_move_pixels / drag.pixels_per_radii_unit, lowest_width_change);
    const radii = stroke_radii_with_width_change_at(drag.radii_at_begin, drag.radii_index, drag.length_fraction, width_change);
    // The grabbed number itself does not go below zero either (the other three numbers
    // can leave some width at the spot when it is zero).
    const lowest_number = Math.min(drag.radii_at_begin[drag.radii_index], 0);
    radii[drag.radii_index] = Math.max(radii[drag.radii_index], lowest_number);
    store_stroke_radii(drag.stroke_id, radii);
    request_render();
  }

  // The undo label of the drag (a tap changed nothing, so the pen gesture's step adds no
  // entry), and the drag is over.
  function stroke_width_drag_end(): string {
    const label = `set radii stroke ${stroke_width_drag!.stroke_id}`;
    stroke_width_drag = null;
    return label;
  }

  // What the tint along a line shows this frame: the number a running width drag changes,
  // or in width mode the number a pen-down at the hovering pen would grab; null otherwise.
  function resolve_stroke_width_tint(): StrokeWidthTint | null {
    if (stroke_width_drag !== null) return { stroke_id: stroke_width_drag.stroke_id, radii_index: stroke_width_drag.radii_index };
    if (!width_mode_is_on() || armed_tool !== null || hover_screen === null) return null;
    if (hot_item === null || hot_item.kind !== "stroke") return null;
    const spot = stroke_width_grabbed_spot(hot_item.stroke_id, hover_screen);
    if (spot === null) return null;
    return { stroke_id: hot_item.stroke_id, radii_index: strongest_stroke_radii_index_at(spot.length_fraction) };
  }

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
    if (vertex_is_pinned(tablet_document, selected_vertex)) return;
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
  // run of nudges on one handle is one history entry. A nudge that moved the handle
  // of an on-surface line refits that line (refit_on_surface_stroke_control_points).
  function nudge_selected_handle(right_steps: number, up_steps: number, forward_steps: number, shift: boolean, in_plane: boolean): void {
    if (edit_state === null || edit_state.selected_handle === null || pen_down_screen !== null) return;
    const handle = edit_state.selected_handle;
    const step_pixels = NUDGE_STEP_PIXELS * (shift ? NUDGE_SHIFT_MULTIPLIER : 1);
    begin_history_step(history, tablet_document);
    const nudged_stroke = stroke_by_id(tablet_document, edit_state.stroke_id);
    const offsets_before = JSON.stringify([nudged_stroke.d0, nudged_stroke.d3]);
    edit_nudge_handle(
      edit_state, tablet_document, camera, canvas, handle,
      { x: right_steps * step_pixels, y: -up_steps * step_pixels },
      forward_steps * step_pixels * camera_world_units_per_pixel(camera, canvas.clientHeight),
      in_plane ? "plane" : "swing",
    );
    // edit_nudge_handle can move nothing (handle behind the eye, a forward nudge in a
    // plane facing the camera); the refit is not idempotent, so it runs only after a move.
    if (JSON.stringify([nudged_stroke.d0, nudged_stroke.d3]) !== offsets_before) {
      refit_on_surface_stroke_control_points(tablet_document, [nudged_stroke.id]);
    }
    end_history_step(history, tablet_document, `nudge handle ${handle} of stroke ${edit_state.stroke_id}`, true);
    request_render();
  }
  window.addEventListener("keydown", (event) => {
    if (!owns_keyboard()) return;
    // The only modifier chord the sketchpad takes: Ctrl/cmd + a nudge key while a
    // handle is selected = nudge inside the line's plane (a bare nudge swings).
    // Every other chord is the browser's.
    const modifier_held = event.ctrlKey || event.metaKey || event.altKey;
    const handle_selected = edit_state !== null && edit_state.selected_handle !== null;
    const in_plane = (event.ctrlKey || event.metaKey) && !event.altKey;
    if (modifier_held && !(handle_selected && in_plane)) return;
    const nudge = (right_steps: number, up_steps: number, forward_steps: number) => {
      if (handle_selected) {
        event.preventDefault(); // Ctrl + arrows / i / o are browser shortcuts
        nudge_selected_handle(right_steps, up_steps, forward_steps, event.shiftKey, in_plane);
      } else {
        event.preventDefault(); // the arrows scroll the page
        nudge_selected_vertex(right_steps, up_steps, forward_steps, event.shiftKey);
      }
    };
    const typing = document.activeElement instanceof HTMLInputElement;
    switch (event.key.toLowerCase()) {
      case "arrowleft": if (!typing) nudge(-1, 0, 0); break;
      case "arrowright": if (!typing) nudge(1, 0, 0); break;
      case "arrowdown": if (!typing) nudge(0, -1, 0); break;
      case "arrowup": if (!typing) nudge(0, 1, 0); break;
      case "i": nudge(0, 0, 1); break;
      case "o": nudge(0, 0, -1); break;
      // l = the line button, p = the patch button. Not while typing a name or a width.
      case "l":
        if (modifier_held || event.repeat || typing) return;
        line_button.click();
        break;
      case "p":
        if (modifier_held || event.repeat || typing) return;
        patch_button.click();
        break;
      // Delete / Backspace (a Mac keyboard's "delete") / d = the del button, lock
      // included (plan-patch-subcurve-boundary.md Q12). Not while typing a name
      // or a width.
      case "d":
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
      if (armed_tool === "line") {
        // The fills a tap would pick catch the pen's ends, only while "surf" is on.
        const line_surface_layers = surface_drawn ? pickable_layers() : null;
        line_state = line_pen_down(tablet_document, camera, position, canvas, line_start_vertex, line_surface_layers);
        update_preview_line();
      } else if (width_mode_is_on() && armed_tool === null) {
        // Width mode: a line under the pen becomes the selected line (unless it already
        // is) and its width drag starts; anywhere else the pen orbits.
        const picked = pick_width_mode_stroke(position);
        if (picked !== null && stroke_width_drag_begin(picked, position)) {
          if (edit_state === null || edit_state.stroke_id !== picked) select_stroke_by_tap(picked, false);
        } else {
          pen_orbit_last_screen = position;
        }
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
      // pixels of jitter is still a tap, anything more is a drag of it. The same for a
      // width drag, whose edge would otherwise jump by the whole tap distance.
      const pen_landed_on_point = selected_vertex_drag_last_screen !== null || stroke_width_drag !== null
        || (edit_state !== null && (edit_state.dragging !== null || edit_state.dragging_pin !== null));
      pen_tap_max_movement_pixels = pen_landed_on_point ? 4 : TAP_MAX_MOVEMENT_PIXELS;
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
          line_pen_move(line_state, tablet_document, camera, position, canvas);
          update_preview_line();
        }
      } else if (pen_orbit_last_screen !== null) {
        pen_orbit(position);
        // Only the camera moved.
        request_camera_render();
        return;
      } else if (pen_max_displacement_pixels < pen_tap_max_movement_pixels) {
        // Still a tap: nothing moves (a tap only selects). The drag's last
        // screen position stays at the pen-down point, so the first real move
        // catches up with the pen.
      } else if (stroke_width_drag !== null) {
        stroke_width_drag_move(position);
      } else if (selected_vertex_drag_last_screen !== null && selected_vertex !== null) {
        const pin = pin_by_vertex(tablet_document, selected_vertex);
        if (surface_pin_by_vertex(tablet_document, selected_vertex) !== null) {
          // A surface-pinned vertex slides over its layer's surface (step 5).
          drag_surface_pin(tablet_document, selected_vertex, camera, position, canvas);
        } else if (pin !== null) {
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
      } else if (stroke_width_drag !== null) {
        // A tap moved nothing, so the gesture's history step adds no entry.
        pen_history_label = stroke_width_drag_end();
      } else if (edit_state !== null) {
        edit_mode_pen_up(position, multi);
      } else if (pen_max_displacement_pixels < pen_tap_max_movement_pixels) {
        select_tap_target(pick_tap_target(position), multi);
      } else if (selected_vertex_drag_last_screen !== null) {
        pen_history_label = `move vertex ${selected_vertex}`;
        // The on-surface lines ending on the dragged vertex are refitted to the surface.
        if (selected_vertex !== null) refit_on_surface_stroke_control_points(tablet_document, strokes_ending_on_vertex(selected_vertex));
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
      request_hover_render();
    },
    on_undo_tap: perform_undo,
    on_redo_tap: perform_redo,
  }, () => {
    request_camera_render();
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
  // boundary, so while one exists its strokes can't be deleted and
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
  // The patches a locked del takes along once the user says yes: every patch that
  // lists a selected line (delete_stroke removes exactly those), or that has the
  // selected vertex as a corner.
  function patch_ids_deleted_with_selected_strokes(): number[] {
    const stroke_ids = selected_strokes();
    return tablet_document.patches.filter((patch) => patch.strokes.some((stroke_id) => stroke_ids.includes(stroke_id))).map((patch) => patch.id);
  }
  function patch_ids_with_corner_at_vertex(vertex: VertexId): number[] {
    return tablet_document.patches.filter((patch) => patch_junction_vertices(patch, tablet_document).includes(vertex)).map((patch) => patch.id);
  }
  function patches_label(patch_ids: number[]): string {
    return patch_ids.length === 1 ? `patch ${patch_ids[0]}` : `patches ${patch_ids.join(", ")}`;
  }
  // A join is not locked by a patch: it carries the patches along, and is
  // refused only when one of them would lose its fill (stroke_merge.ts).
  function selected_join_refusal_reason(): string | null {
    const other = single_extra_stroke();
    if (edit_state === null || other === null) return null;
    return join_refusal_reason(tablet_document, edit_state.stroke_id, other);
  }
  function unpin_is_locked(): boolean {
    const pin = selected_vertex_pin();
    return pin !== null && pin_is_locked(tablet_document, pin.vertex);
  }
  // A button with nothing to act on in the current selection is hidden. A button
  // whose action a patch blocks stays, greyed out, with the reason as its tooltip;
  // the button's own tooltip from the page comes back once it is usable.
  // With lock_asks_first the greyed button still takes a click: its handler asks
  // "are you sure" and then removes what blocks it too (del does this).
  // The reason is not the browser's `title` tooltip: that one comes after a delay, and
  // not at all on a disabled button in some browsers. It is a box drawn by CSS under the
  // button the moment the pointer is over it (the style is added here so every page
  // that embeds the sketchpad has it).
  const own_button_titles = new Map<HTMLButtonElement, string>();
  const lock_reason_style = document.createElement("style");
  lock_reason_style.textContent = `
    button[data-lock-reason] { position: relative; }
    button[data-lock-reason]:hover::after {
      content: attr(data-lock-reason); position: absolute; top: calc(100% + 6px); right: 0; z-index: 100;
      width: max-content; max-width: 280px; white-space: normal; text-align: left; padding: 6px 10px;
      font-size: 13px; line-height: 1.3; color: #eee; background: #1b1b20; border: 1px solid #888; border-radius: 6px;
      pointer-events: none;
    }
    #toolbar button[data-lock-asks-first] { color: #666; border-color: #333; }`;
  document.head.append(lock_reason_style);
  function set_button_availability(button: HTMLButtonElement, selection_fits: boolean, lock_reason: string | null, lock_asks_first = false): void {
    if (!own_button_titles.has(button)) own_button_titles.set(button, button.title);
    button.hidden = !selection_fits;
    button.disabled = lock_reason !== null && !lock_asks_first;
    if (lock_reason !== null && lock_asks_first) button.dataset.lockAsksFirst = "";
    else delete button.dataset.lockAsksFirst;
    if (lock_reason !== null) {
      button.dataset.lockReason = lock_reason;
      button.title = "";
    } else {
      delete button.dataset.lockReason;
      button.title = own_button_titles.get(button)!;
    }
  }
  function refresh_lock_buttons(): void {
    const line_selected = edit_state !== null;
    const line_or_vertex_selected = edit_state !== null || selected_vertex !== null;
    const two_lines_selected = single_extra_stroke() !== null;

    let delete_lock_reason: string | null = null;
    if (selected_patch === null && selected_vertex === null && delete_is_locked()) {
      delete_lock_reason = `the selected line bounds ${patches_label(patch_ids_deleted_with_selected_strokes())}: del asks, then deletes that too`;
    }
    if (selected_patch === null && selected_vertex !== null && pin_is_locked(tablet_document, selected_vertex)) {
      delete_lock_reason = `this vertex is a corner of ${patches_label(patch_ids_with_corner_at_vertex(selected_vertex))}: del asks, then deletes that too`;
    }
    // del does nothing while two kinds are selected together: a vertex and a
    // line (line plan Q2), or a patch with either (surface plan Q2).
    const vertex_and_line_selected = edit_state !== null && selected_vertex !== null;
    const patch_and_other_selected = selected_patch !== null && line_or_vertex_selected;
    set_button_availability(
      delete_button, !patch_and_other_selected && (selected_patch !== null || (line_or_vertex_selected && !vertex_and_line_selected)), delete_lock_reason, true,
    );
    set_button_availability(weld_button, selected_weld_pair() !== null, null);
    const detach_end_pair = selected_detach_end_pair();
    set_button_availability(
      detach_end_button, detach_end_pair !== null,
      detach_end_pair !== null && stroke_bounds_a_patch(tablet_document, detach_end_pair.stroke_id)
        ? `the selected line bounds a patch, ${DELETE_PATCH_FIRST}` : null,
    );
    set_button_availability(
      join_button, two_lines_selected, selected_join_refusal_reason(),
    );
    // The pin button pins the selected vertex + line / patch pair when there is one;
    // the unpin lock only matters when it would unpin.
    const pins_selected_pair = selected_pin_to_line_pair() !== null || selected_pin_to_surface_pair() !== null;
    set_button_availability(
      pin_button, pins_selected_pair || line_selected || selected_pinned_vertex() !== null,
      !pins_selected_pair && unpin_is_locked() ? `this vertex bounds a patch, ${DELETE_PATCH_FIRST}` : null,
    );
    set_button_availability(split_button, line_selected, null);
    set_button_availability(smooth_button, two_lines_selected, null);
    set_button_availability(straighten_button, line_selected, null);
    set_button_availability(patch_button, edit_state !== null && extra_selection.length > 0, null);
    set_button_availability(midline_button, line_or_vertex_selected, null);
    set_button_availability(on_surface_button, edit_state !== null, null);
    set_button_availability(one_handle_button, edit_state !== null, null);
    set_button_availability(always_drawn_button, edit_state !== null, null);
    set_button_availability(name_button, line_or_vertex_selected, null);
  }
  delete_button.addEventListener("click", () => {
    if (selected_patch !== null && (edit_state !== null || selected_vertex !== null)) return; // which is meant is unclear (surface plan Q2)
    if (selected_patch !== null) {
      begin_history_step(history, tablet_document);
      const deleted = tablet_document.patches[selected_patch];
      tablet_document.patches.splice(selected_patch, 1);
      drop_surface_pins_of_missing_patches(tablet_document); // its points on surface become free vertices
      end_history_step(history, tablet_document, `delete patch [${deleted.strokes.join(" ")}]`);
      selected_patch = null;
      request_render();
      return;
    }
    if (edit_state !== null && selected_vertex !== null) return; // which of the two is meant is unclear (Q2)
    if (selected_vertex !== null) {
      // A patch's sub-curve ends here: the patch goes first, after a yes.
      const corner_patch_ids = patch_ids_with_corner_at_vertex(selected_vertex);
      if (corner_patch_ids.length > 0 && !window.confirm(`Vertex ${selected_vertex} is a corner of ${patches_label(corner_patch_ids)}. Delete that too? (undo brings it back)`)) return;
      begin_history_step(history, tablet_document);
      if (corner_patch_ids.length > 0) {
        tablet_document.patches = tablet_document.patches.filter((patch) => !corner_patch_ids.includes(patch.id));
        drop_surface_pins_of_missing_patches(tablet_document);
      }
      // Deleting a pinned vertex takes its pin along; without that the pin
      // keeps the vertex alive and delete does nothing.
      const deleted_vertex = selected_vertex;
      unpin_vertices(tablet_document, [deleted_vertex]);
      delete vertex_by_id(tablet_document, selected_vertex).name;
      garbage_collect_vertices(tablet_document);
      end_history_step(history, tablet_document, `delete vertex ${selected_vertex}`);
      selected_vertex = null;
      extra_vertex = null;
      request_render();
      return;
    }
    if (edit_state === null) return;
    const deleted_strokes = selected_strokes();
    if (delete_is_locked()) {
      const line_label = deleted_strokes.length === 1 ? `Line ${deleted_strokes[0]} bounds` : `Lines ${deleted_strokes.join(", ")} bound`;
      if (!window.confirm(`${line_label} ${patches_label(patch_ids_deleted_with_selected_strokes())}. Delete that too? (undo brings it back)`)) return;
    }
    begin_history_step(history, tablet_document);
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
    if (!owns_keyboard()) return;
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
    if (!owns_keyboard()) return;
    if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === "z") {
      event.preventDefault();
      if (event.shiftKey) perform_redo();
      else perform_undo();
    }
  });

  // Reference mesh on/off: the "ref" button, or the r key (q too, the desktop app's key).
  const reference_button = document.getElementById("reference_button") as HTMLButtonElement;
  function toggle_reference_visible(): void {
    reference_visible = !reference_visible;
    reference_button.classList.toggle("armed", reference_visible);
    request_render();
  }
  reference_button.addEventListener("click", toggle_reference_visible);
  reference_button.classList.toggle("armed", reference_visible);
  window.addEventListener("keydown", (event) => {
    if (!owns_keyboard()) return;
    if (event.repeat || event.ctrlKey || event.metaKey || event.altKey) return;
    if (document.activeElement instanceof HTMLInputElement) return;
    if (event.key === "r" || event.key === "q") toggle_reference_visible();
    if (event.key === "s") toggle_surface_drawn();
    if (event.key === "v") toggle_picture_view();
    if (event.key === "k") document_toggle_always_drawn();
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

  // Surfaces on/off: the "surf" button, or the s key.
  const surface_button = document.getElementById("surface_button") as HTMLButtonElement;
  function refresh_surface_button(): void {
    surface_button.classList.toggle("armed", surface_drawn);
    surface_button.textContent = surface_drawn ? "surf (S)" : "no surf (S)";
  }
  function toggle_surface_drawn(): void {
    surface_drawn = !surface_drawn;
    refresh_surface_button();
    request_render();
  }
  surface_button.addEventListener("click", toggle_surface_drawn);
  refresh_surface_button();

  // Picture view on/off: the "picture" button, or the v key. It drops the selection, like
  // a layer toggle: the selected line or vertex may just have stopped being drawn and picked.
  const picture_button = document.getElementById("picture_button") as HTMLButtonElement;
  function toggle_picture_view(): void {
    is_picture_view = !is_picture_view;
    picture_button.classList.toggle("armed", is_picture_view);
    clear_selection_after_history_jump();
  }
  picture_button.addEventListener("click", toggle_picture_view);

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
    if (setup.active_layer === "first_layer_with_strokes") active_layer = first_layer_with_strokes(tablet_document);
    request_render();
    // The loaded document has no history step yet, so request_render reported nothing.
    setup.on_document_edited?.(tablet_document);
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
      if (vertex_is_pinned(tablet_document, vertex_id)) continue;
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
  // A test may have changed `window.tablet_document` without a render request, so the
  // hook checks the patch grids again when no request came since the last frame. After a
  // request it leaves them to the request: a full one has already dealt with them, a
  // camera or hover one keeps them (a profiling script timing those frames).
  (window as unknown as { debug_render_now: unknown }).debug_render_now = () => {
    if (!frame_requested) begin_patch_geometry_check_pass(patch_geometry_cache, tablet_document);
    // As the animation frame callback does: a script calling request + this hook in a loop
    // never yields to the animation frame, and a stuck flag would make every later camera
    // request look like a full one. The pending animation frame still renders, harmlessly.
    frame_requested = false;
    render_now();
  };
  // Profiling (2026-11-22): the frame timings above, and a hash of what the last frame
  // put in the stroke, surface and reference vertex sinks (the filled prefix, bit for
  // bit), so a script can check that a speed-up leaves the picture unchanged.
  const vertex_sink_hash = (sink: VertexSink): string => {
    const words = new Uint32Array(sink.data.buffer, sink.data.byteOffset, sink.length);
    let hash = 0x811c9dc5;
    for (let i = 0; i < words.length; i++) hash = Math.imul(hash ^ words[i], 0x01000193) >>> 0;
    return `${sink.length}:${hash.toString(16)}`;
  };
  Object.assign(window, {
    debug_frame_timings: () => frame_timings,
    debug_reset_frame_timings: () => {
      frame_timings = { frame_count: 0, full_request_count: 0, camera_request_count: 0, hover_request_count: 0, milliseconds_by_part: {} };
    },
    debug_set_hover_screen: (screen: V2 | null) => {
      hover_screen = screen;
    },
    debug_frame_vertex_hash: () => ({
      strokes: vertex_sink_hash(stroke_sink),
      surfaces: vertex_sink_hash(surface_sink),
      reference: vertex_sink_hash(reference_sink),
    }),
    // The next surface mesh goes up whole (so a check can compare a frame drawn from the
    // buffer kept slice by slice with one drawn from a whole upload).
    debug_forget_uploaded_surface_slices: () => {
      uploaded_surface_slices = null;
    },
  });
  (window as unknown as { debug_selection_text: unknown }).debug_selection_text = selection_readout_text;
  (window as unknown as { debug_pull_document_changes_if_any: unknown }).debug_pull_document_changes_if_any = pull_document_changes_if_any;
  // Drive the editor from the browser console or a scripted tab without clicking
  // (2026-11-07): select a stroke and a handle, read the selection, nudge it the
  // way the keyboard does. `edit_state` is reassigned, so it is read through a
  // function, not stored.
  Object.assign(window, {
    debug_edit_state: () => edit_state,
    debug_select_stroke: (stroke_id: StrokeId, handle: "p1" | "p2" | null = null) => {
      edit_state = begin_edit_state(stroke_id);
      edit_state.selected_handle = handle;
      extra_selection = [];
      selected_vertex = null;
      extra_vertex = null;
      request_render();
    },
    debug_nudge_selected_handle: nudge_selected_handle,
    debug_request_render: request_render,
    debug_request_camera_render: request_camera_render,
  });
  // Read by the page-reload script of the build (vite.config.ts reload_on_rebuild_plugin).
  (window as unknown as { tablet_has_unsaved_edits: unknown }).tablet_has_unsaved_edits = () => has_unsaved_edits(persistence);
  console.log("autodraw tablet: Sketchpad rework — line tool + vertex-connected single-cubic strokes");
}
