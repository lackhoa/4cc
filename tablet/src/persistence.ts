// Document persistence (plan-skull-reference.md Q6b-Q9): the dev server's
// documents/ dir is the source of truth (git-tracked JSON, one file per
// document); localStorage is only a crash buffer bridging reloads while the
// server is unreachable. Debounced autosave, no save button.

import { OrbitCamera } from "./camera";
import { SKULL_BONE_ID, Stroke, TabletDocument, Vertex, fallback_perpendicular, skull_bone, stroke_handles_from_control_points } from "./document";
import { V2, V3, v3, v3_add, v3_cross, v3_length, v3_normalize, v3_scale, v3_sub } from "./math";

// Version 5 (2026-10-03): bones and layers — the document carries a `bones`
// table, every vertex a `bone_id` with its position in that bone's space,
// every stroke a `layer`. Loading an older file puts everything on the `skull`
// bone (identity, so positions are unchanged) in the `skull` layer.
// Version 4 (2026-09-05): stable ids — vertices are {id, position}, strokes
// carry an id, every cross-reference holds ids, and the document stores the
// two id counters. Versions 1-3 referenced strokes/vertices by array index
// (loading assigns id = index and counters = array lengths). Version 3
// (2026-09-02): free-handle strokes {d0: V3, d3: V3}, coplanar with the chord
// by invariant. Version 2 (2026-09-01) was explicit-normal {normal: V3, d0:
// V2, d3: V2}; version 1 bez_v3v2 {d0: V3, d3: V2}. Older files convert on
// load; saves are always the current version.
const DOCUMENT_FORMAT_VERSION = 5;
const LOADABLE_VERSIONS = [1, 2, 3, 4, DOCUMENT_FORMAT_VERSION];
const AUTOSAVE_DEBOUNCE_MS = 2000;

export type PersistenceState = {
  current_document_name: string;
  // localStorage keys, from the sketchpad setup's prefix: each sketchpad page keeps its
  // own current document, crash buffer and camera. Two pages (or two tabs of one page)
  // can still have the same file open; `base_revision` keeps them from overwriting
  // each other.
  current_name_storage_key: string;
  crash_buffer_storage_key: string;
  camera_storage_key: string;
  // The revision (server-computed hash of the file's bytes, vite.config.ts) of the file
  // this tab's document was loaded from or last saved as; sent with every save, which
  // the server refuses when the file on disk is another revision. Null = the tab
  // believes the file does not exist yet.
  base_revision: string | null;
  // The file changed elsewhere while this tab has unsaved edits: autosave is stopped
  // until the conflict bar (`load newer` / `fork`) resolves it.
  is_in_conflict: boolean;
  conflict_bar_element: HTMLElement | null; // set by the sketchpad; shown while in conflict
  saving_json: string | null; // the save in flight (no second save starts during it)
  autosave_timer: number | null;
  // False until the startup load (or a document switch) finishes — blocks
  // autosave from overwriting the stored document with the pre-load state.
  ready: boolean;
  // Toolbar `#save_status` (null when the page has none): shows where the autosave is.
  save_status_element: HTMLElement | null;
  // The history snapshot the document currently equals (sketchpad sets it before each
  // schedule_autosave) and the one current at the last successful save: "unsaved" means
  // they differ. Compared by reference, so this costs nothing per render; camera-only
  // changes are not in the history and never save.
  history_snapshot: string | null;
  last_saved_history_snapshot: string | null;
};

// `last_saved_history_snapshot` of a tab that restored a crash buffer the server never
// got: equal to no history snapshot, so the tab counts as unsaved until a save succeeds.
const RESTORED_CRASH_BUFFER_NOT_SAVED = "restored crash buffer, not saved";

export function has_unsaved_edits(state: PersistenceState): boolean {
  return state.history_snapshot !== state.last_saved_history_snapshot;
}

// `kind` selects the color (CSS class on #save_status).
type SaveStatusKind = "pending" | "saving" | "saved" | "failed" | "off" | "conflict";
function show_save_status(state: PersistenceState, kind: SaveStatusKind, text: string): void {
  if (state.save_status_element === null) return;
  state.save_status_element.className = kind;
  state.save_status_element.textContent = text;
}

// What sits in localStorage: the last autosaved snapshot plus whether the
// server confirmed it. server_saved=false after a reload means the server
// never confirmed it — push it on reconnect, but only if the server's file is
// still the revision the buffer was based on. Otherwise the file was edited
// elsewhere since: the buffer is restored into the tab as a conflict, never
// pushed over the file.
// `base_revision`: of the file `json` was based on while server_saved=false, of
// `json` itself once server_saved=true.
type CrashBuffer = { name: string; json: string; server_saved: boolean; base_revision: string | null };

// `default_document_name` is opened when localStorage remembers no current document.
export function create_persistence_state(default_document_name: string, storage_key_prefix: string): PersistenceState {
  const current_name_storage_key = `${storage_key_prefix}_current_document`;
  let name = default_document_name;
  try {
    name = localStorage.getItem(current_name_storage_key) ?? default_document_name;
  } catch { /* storage unavailable (private mode) — default name */ }
  return {
    current_document_name: name,
    current_name_storage_key,
    crash_buffer_storage_key: `${storage_key_prefix}_crash_buffer`,
    camera_storage_key: `${storage_key_prefix}_camera`,
    base_revision: null,
    is_in_conflict: false,
    conflict_bar_element: null,
    saving_json: null,
    autosave_timer: null,
    ready: false,
    save_status_element: document.getElementById("save_status"),
    history_snapshot: null,
    last_saved_history_snapshot: null,
  };
}

export function serialize_document_state(tablet_document: TabletDocument, camera: OrbitCamera): string {
  return JSON.stringify({
    version: DOCUMENT_FORMAT_VERSION,
    camera: { pivot: camera.pivot, yaw: camera.yaw, pitch: camera.pitch, distance: camera.distance },
    document: tablet_document,
  }, null, 1);
}

export function clear_document_in_place(tablet_document: TabletDocument): void {
  tablet_document.next_vertex_id = 0;
  tablet_document.next_stroke_id = 0;
  tablet_document.bones.length = 0;
  tablet_document.bones.push(skull_bone());
  tablet_document.vertices.length = 0;
  tablet_document.vertex_pins.length = 0;
  tablet_document.smooth_knots.length = 0;
  tablet_document.strokes.length = 0;
  tablet_document.patches.length = 0;
}

// v1 stroke: d0 a free V3 defining the plane, d3 in the (u2 = p3 - p1, v)
// frame. Reconstruct the four world control points with the v1 math, then
// re-express them in offset form — exact, since v1 curves are planar by
// construction.
type StrokeV1 = { p0_vertex: number; d0: V3; d3: V2; p3_vertex: number };

// Strokes of versions 1-3 (and their vertex references) are array positions;
// the converters produce v4 strokes whose id is that position.
type StrokeV3 = { p0_vertex: number; p3_vertex: number; d0: V3; d3: V3; name?: string };

// v2 stroke: stored unit normal, handles as 2D offsets in the plane frame
// (u along the chord, v = cross(normal, u)). The world offsets are those same
// frame combinations — exact.
type StrokeV2 = { p0_vertex: number; p3_vertex: number; normal: V3; d0: V2; d3: V2 };

function stroke_from_v2(old: StrokeV2, id: number, vertices: V3[]): Stroke {
  const p0 = vertices[old.p0_vertex];
  const p3 = vertices[old.p3_vertex];
  const chord = v3_sub(p3, p0);
  const u = v3_length(chord) > 1e-9 ? v3_normalize(chord) : v3(1, 0, 0);
  const v_raw = v3_cross(old.normal, u);
  const v = v3_length(v_raw) > 1e-9 ? v3_normalize(v_raw) : fallback_perpendicular(u);
  const in_plane = (offset: V2) => v3_add(v3_scale(u, offset.x), v3_scale(v, offset.y));
  return { id, layer: "skull", p0_vertex: old.p0_vertex, p3_vertex: old.p3_vertex, d0: in_plane(old.d0), d3: in_plane(old.d3) };
}

function stroke_from_v1(old: StrokeV1, id: number, vertices: V3[]): Stroke {
  const p0 = vertices[old.p0_vertex];
  const p3 = vertices[old.p3_vertex];
  const p1 = v3_add(v3_scale(v3_add(v3_scale(p0, 2), p3), 1 / 3), old.d0);
  const chord = v3_sub(p3, p0);
  let w_raw = v3_cross(chord, old.d0);
  if (v3_length(w_raw) < 1e-9) w_raw = v3_cross(chord, v3(0, 1, 0));
  if (v3_length(w_raw) < 1e-9) w_raw = v3_cross(chord, v3(1, 0, 0));
  const w = v3_length(w_raw) < 1e-9 ? v3(0, 0, 1) : v3_normalize(w_raw);
  const u2 = v3_sub(p3, p1);
  const v_axis = v3_cross(w, u2);
  const p2 = v3_add(
    v3_scale(v3_add(p1, p3), 0.5),
    v3_add(v3_scale(u2, old.d3.x), v3_scale(v_axis, old.d3.y)),
  );
  const handles = stroke_handles_from_control_points(p0, p1, p2, p3);
  return { id, layer: "skull", p0_vertex: old.p0_vertex, p3_vertex: old.p3_vertex, ...handles };
}

function stroke_from_v3(old: StrokeV3, id: number): Stroke {
  return { id, layer: "skull", ...old };
}

// Index-addressed documents (versions 1-3) become id-addressed: every array
// position turns into that entry's id, so stored references stay valid as-is.
function document_from_indexed(parsed_version: number, stored: any, tablet_document: TabletDocument): void {
  const stored_vertices: V3[] = stored.vertices;
  const vertices: Vertex[] = stored_vertices.map((position, id) => ({ id, bone_id: SKULL_BONE_ID, position }));
  const strokes: Stroke[] = parsed_version === 1
    ? stored.strokes.map((stroke: StrokeV1, id: number) => stroke_from_v1(stroke, id, stored_vertices))
    : parsed_version === 2
      ? stored.strokes.map((stroke: StrokeV2, id: number) => stroke_from_v2(stroke, id, stored_vertices))
      : stored.strokes.map((stroke: StrokeV3, id: number) => stroke_from_v3(stroke, id));
  tablet_document.next_vertex_id = vertices.length;
  tablet_document.next_stroke_id = strokes.length;
  tablet_document.vertices.push(...vertices);
  tablet_document.strokes.push(...strokes);
}

// Parse + apply a stored snapshot into the live document/camera (in place —
// sketchpad.ts holds references to both). Returns false on a bad payload.
export function apply_document_state(json: string, tablet_document: TabletDocument, camera: OrbitCamera): boolean {
  let parsed;
  try {
    parsed = JSON.parse(json);
  } catch (error) {
    console.error("stored document is not valid JSON", error);
    return false;
  }
  if (!LOADABLE_VERSIONS.includes(parsed.version)) {
    console.error(`stored document has version ${parsed.version}, expected one of ${LOADABLE_VERSIONS}`);
    return false;
  }
  clear_document_in_place(tablet_document);
  if (parsed.version < 4) {
    document_from_indexed(parsed.version, parsed.document, tablet_document);
  } else if (parsed.version === 4) {
    // v4 -> v5: the Frankfurt frame the skull was drawn in IS the skull bone's
    // space (identity), so positions are copied as they are.
    tablet_document.next_vertex_id = parsed.document.next_vertex_id;
    tablet_document.next_stroke_id = parsed.document.next_stroke_id;
    tablet_document.vertices.push(...parsed.document.vertices.map((vertex: Omit<Vertex, "bone_id">): Vertex => ({ ...vertex, bone_id: SKULL_BONE_ID })));
    tablet_document.strokes.push(...parsed.document.strokes.map((stroke: Omit<Stroke, "layer">): Stroke => ({ ...stroke, layer: "skull" })));
  } else {
    tablet_document.next_vertex_id = parsed.document.next_vertex_id;
    tablet_document.next_stroke_id = parsed.document.next_stroke_id;
    tablet_document.bones.length = 0;
    tablet_document.bones.push(...parsed.document.bones);
    tablet_document.vertices.push(...parsed.document.vertices);
    tablet_document.strokes.push(...parsed.document.strokes);
  }
  // vertex_pins arrived after the v2 bump — absent in v1 docs and early v2 saves.
  tablet_document.vertex_pins.push(...(parsed.document.vertex_pins ?? []));
  // smooth_knots arrived within v4 — absent in earlier files and early v4 saves.
  tablet_document.smooth_knots.push(...(parsed.document.smooth_knots ?? []));
  // Files saved before revolve/inflate were removed still carry `revolves` /
  // `inflates` arrays — ignored, dropped on the next save. Files from before
  // `patches` carry `lofts` (two rails) and `coons` (four sides) instead:
  // both are just stroke sets now, migrated here and dropped on the next save.
  tablet_document.patches.push(...(parsed.document.patches ?? []));
  for (const loft of parsed.document.lofts ?? []) tablet_document.patches.push({ strokes: [loft.stroke_a, loft.stroke_b] });
  for (const coons of parsed.document.coons ?? []) tablet_document.patches.push({ strokes: [...coons.strokes] });
  camera.pivot = parsed.camera.pivot;
  camera.yaw = parsed.camera.yaw;
  camera.pitch = parsed.camera.pitch;
  camera.distance = parsed.camera.distance;
  return true;
}

function read_crash_buffer(state: PersistenceState): CrashBuffer | null {
  try {
    const raw = localStorage.getItem(state.crash_buffer_storage_key);
    return raw === null ? null : JSON.parse(raw);
  } catch {
    return null;
  }
}

function write_crash_buffer(state: PersistenceState, buffer: CrashBuffer): void {
  try {
    localStorage.setItem(state.crash_buffer_storage_key, JSON.stringify(buffer));
  } catch { /* storage unavailable — server save still happens */ }
}

function remember_current_name(state: PersistenceState, name: string): void {
  try {
    localStorage.setItem(state.current_name_storage_key, name);
  } catch { /* storage unavailable */ }
}

// The view belongs to the page, not to the file: two tabs on one document each keep
// their own, and moving the camera never writes the file. Called when the tab is hidden
// or left, and before a document switch.
export function remember_camera(state: PersistenceState, camera: OrbitCamera): void {
  if (!state.ready) return; // a tab whose load failed still shows the default camera
  try {
    localStorage.setItem(state.camera_storage_key, JSON.stringify(camera));
  } catch { /* storage unavailable */ }
}

// The camera stored in a file is only the starting view for a page that has none of
// its own yet.
function restore_remembered_camera(state: PersistenceState, camera: OrbitCamera): void {
  try {
    const raw = localStorage.getItem(state.camera_storage_key);
    if (raw !== null) Object.assign(camera, JSON.parse(raw));
  } catch { /* storage unavailable — keep the file's camera */ }
}

function enter_conflict_state(state: PersistenceState): void {
  state.is_in_conflict = true;
  if (state.autosave_timer !== null) {
    window.clearTimeout(state.autosave_timer);
    state.autosave_timer = null;
  }
  show_save_status(state, "conflict", "CONFLICT");
  if (state.conflict_bar_element !== null) state.conflict_bar_element.style.display = "flex";
}

function leave_conflict_state(state: PersistenceState): void {
  state.is_in_conflict = false;
  if (state.conflict_bar_element !== null) state.conflict_bar_element.style.display = "none";
}

// "conflict" = the server refused (409): the file on disk is not `base_revision`.
type SaveResult = { outcome: "saved" | "conflict" | "failed"; revision: string | null };

// `base_revision` null = create the file (the server refuses when it already exists).
// `keepalive` lets the request outlive the page (flush on pagehide); bodies are limited to
// 64 KB then, so it stays off for ordinary saves.
async function save_to_server(name: string, json: string, base_revision: string | null, keepalive: boolean = false): Promise<SaveResult> {
  try {
    const headers: Record<string, string> = { "Content-Type": "application/json" };
    if (base_revision !== null) headers["X-Base-Revision"] = base_revision;
    const response = await fetch(`/api/documents/${encodeURIComponent(name)}`, { method: "POST", headers, body: json, keepalive });
    if (response.status === 409) {
      console.error(`document save refused: '${name}' on the server is not revision ${base_revision}`);
      return { outcome: "conflict", revision: null };
    }
    if (!response.ok) {
      console.error(`document save failed: ${response.status}`);
      return { outcome: "failed", revision: null };
    }
    return { outcome: "saved", revision: response.headers.get("X-Revision") };
  } catch (error) {
    console.error("document save failed (server unreachable)", error);
    return { outcome: "failed", revision: null };
  }
}

async function save_now(state: PersistenceState, tablet_document: TabletDocument, camera: OrbitCamera, keepalive: boolean = false): Promise<void> {
  const json = serialize_document_state(tablet_document, camera);
  const name = state.current_document_name;
  const history_snapshot = state.history_snapshot; // what `json` holds, document-wise
  // Before the request: the buffer has the edits even when the page dies during it.
  write_crash_buffer(state, { name, json, server_saved: false, base_revision: state.base_revision });
  // One save at a time: a second one would carry the same base revision and be refused
  // once the first lands. Edits made meanwhile are saved by the re-arm below.
  if (state.saving_json !== null) return;
  show_save_status(state, "saving", "saving…");
  state.saving_json = json;
  const result = await save_to_server(name, json, state.base_revision, keepalive);
  state.saving_json = null;
  if (result.outcome === "saved") {
    state.base_revision = result.revision;
    write_crash_buffer(state, { name, json, server_saved: true, base_revision: result.revision });
    state.last_saved_history_snapshot = history_snapshot;
    show_save_status(state, "saved", "saved");
    schedule_autosave(state, tablet_document, camera); // edits made during the save
  } else if (result.outcome === "conflict") {
    enter_conflict_state(state);
  } else {
    // Not saved: the tab stays unsaved, so the next render request retries the server.
    show_save_status(state, "failed", "SAVE FAILED");
  }
}

// Called on every render request; fires one save ~2 s after the first unsaved mutation.
export function schedule_autosave(state: PersistenceState, tablet_document: TabletDocument, camera: OrbitCamera): void {
  if (!state.ready) {
    show_save_status(state, "off", "autosave OFF");
    return;
  }
  if (state.is_in_conflict) return;
  // Render requests also come from hover/selection/camera/panel refreshes: only a
  // history change (a real edit, or an undo/redo) means "unsaved" and arms the timer.
  // The camera is written along with an edit, never on its own.
  if (!has_unsaved_edits(state)) return;
  show_save_status(state, "pending", "unsaved…");
  // Do not push a running timer back: hover and pen movement request renders
  // continuously, and a trailing debounce would wait until the pen holds still.
  if (state.autosave_timer !== null) return;
  state.autosave_timer = window.setTimeout(() => {
    state.autosave_timer = null;
    if (!has_unsaved_edits(state)) { // undone back to the saved state meanwhile
      show_save_status(state, "saved", "saved");
      return;
    }
    void save_now(state, tablet_document, camera);
  }, AUTOSAVE_DEBOUNCE_MS);
}

// Save a pending autosave right now instead of waiting out the debounce. Used before
// leaving the page (pages button, pagehide): a stroke drawn in the last ~2 s would
// otherwise die with the tab. A no-op when nothing changed or the tab is not `ready`.
// In a conflict nothing is sent: the edits go to the crash buffer, and the next start of
// this page restores them as the same conflict.
export async function flush_autosave(state: PersistenceState, tablet_document: TabletDocument, camera: OrbitCamera, keepalive: boolean = false): Promise<void> {
  if (!state.ready || !has_unsaved_edits(state)) return;
  if (state.autosave_timer !== null) {
    window.clearTimeout(state.autosave_timer);
    state.autosave_timer = null;
  }
  if (state.is_in_conflict) {
    write_crash_buffer(state, {
      name: state.current_document_name, json: serialize_document_state(tablet_document, camera),
      server_saved: false, base_revision: state.base_revision,
    });
    return;
  }
  await save_now(state, tablet_document, camera, keepalive);
}

export type DocumentListEntry = { name: string; mtime_ms: number; revision: string };

export async function list_documents_from_server(): Promise<DocumentListEntry[] | null> {
  try {
    const response = await fetch("/api/documents", { cache: "no-store" });
    if (!response.ok) return null;
    return await response.json();
  } catch {
    return null;
  }
}

// The revision the server lists for the current document. Null when it cannot be told:
// server unreachable, file gone, or a server started before revisions existed.
async function current_document_revision_on_server(state: PersistenceState): Promise<string | null> {
  const entries = await list_documents_from_server();
  const entry = entries?.find((candidate) => candidate.name === state.current_document_name);
  return entry?.revision ?? null;
}

// One file as the server holds it; `revision` is of exactly `json`.
export type FetchedDocument = { name: string; json: string; revision: string | null };

// GET the document itself (never trust the list alone: a name absent from it may still
// be a file on disk). "missing" only on a 404, "unreachable" when no response came.
async function fetch_document_from_server(name: string): Promise<FetchedDocument | "missing" | "unreachable" | "failed"> {
  try {
    const response = await fetch(`/api/documents/${encodeURIComponent(name)}`, { cache: "no-store" });
    if (response.status === 404) return "missing";
    if (!response.ok) {
      console.error(`loading document '${name}' failed: ${response.status}`);
      return "failed";
    }
    return { name, json: await response.text(), revision: response.headers.get("X-Revision") };
  } catch (error) {
    console.error(`loading document '${name}' failed (server unreachable)`, error);
    return "unreachable";
  }
}

// One poll tick (sketchpad.ts pull_document_changes_if_any). When the file on the
// server is another revision than this tab's base: a tab without unsaved edits gets the
// file back, to take it over; a tab with unsaved edits goes into the conflict state.
// Null in every other case.
export async function fetch_document_if_changed_elsewhere(state: PersistenceState): Promise<FetchedDocument | null> {
  if (!state.ready || state.is_in_conflict) return null;
  const name = state.current_document_name;
  const base_revision = state.base_revision;
  const listed_revision = await current_document_revision_on_server(state);
  if (listed_revision === null || listed_revision === state.base_revision) return null;
  // This tab saved or switched documents while the list was on its way: the listed
  // revision may be this tab's own save.
  if (state.saving_json !== null || state.base_revision !== base_revision || state.current_document_name !== name) return null;
  if (has_unsaved_edits(state)) {
    enter_conflict_state(state);
    return null;
  }
  const fetched = await fetch_document_from_server(name);
  if (typeof fetched === "string" || state.current_document_name !== name) return null;
  if (has_unsaved_edits(state)) { // an edit made during the fetch
    enter_conflict_state(state);
    return null;
  }
  return fetched;
}

// For the conflict bar's `load newer`: the current document's file, null when it cannot
// be fetched.
export async function fetch_current_document(state: PersistenceState): Promise<FetchedDocument | null> {
  const fetched = await fetch_document_from_server(state.current_document_name);
  return typeof fetched === "string" ? null : fetched;
}

// Make the tab's document equal a fetched file. The camera stays: it belongs to the
// page. Ends a conflict. The caller (sketchpad.ts) wraps this in a history step and
// marks that step's snapshot as the saved one. Returns false, with nothing changed,
// when the file is unreadable.
export function apply_fetched_document(state: PersistenceState, tablet_document: TabletDocument, camera: OrbitCamera, fetched: FetchedDocument): boolean {
  const camera_before = { ...camera };
  if (!apply_document_state(fetched.json, tablet_document, camera)) return false;
  Object.assign(camera, camera_before);
  state.base_revision = fetched.revision;
  // Resolving a conflict drops this tab's unsaved buffer. A tab that merely follows
  // leaves the buffer alone: it may hold the unsaved edits of another tab of this page.
  if (state.is_in_conflict) write_crash_buffer(state, { name: fetched.name, json: fetched.json, server_saved: true, base_revision: fetched.revision });
  leave_conflict_state(state);
  show_save_status(state, "saved", "saved");
  return true;
}

// For the conflict bar's `fork`: write this tab's document to a new file
// `<name>-conflict-NN` (the first free NN). Returns the new name, null when nothing was
// written.
export async function fork_document_to_conflict_copy(state: PersistenceState, tablet_document: TabletDocument, camera: OrbitCamera): Promise<string | null> {
  const existing = await list_documents_from_server();
  if (existing === null) return null;
  const conflict_copy_name = (number: number) => `${state.current_document_name}-conflict-${String(number).padStart(2, "0")}`;
  let number = 1;
  while (existing.some((entry) => entry.name === conflict_copy_name(number))) number += 1;
  const result = await save_to_server(conflict_copy_name(number), serialize_document_state(tablet_document, camera), null);
  return result.outcome === "saved" ? conflict_copy_name(number) : null;
}

// Startup: prefer the server copy of the current document; a crash buffer the
// server never confirmed holds newer edits — restore it and push it up (or, when
// the file changed elsewhere since, restore it as a conflict). With the server
// unreachable, fall back to the buffer alone.
export async function load_current_document_on_startup(
  state: PersistenceState, tablet_document: TabletDocument, camera: OrbitCamera,
): Promise<void> {
  const name = state.current_document_name;
  // NOTE(kv): 2026-09-28 a tab came up empty through a silent branch here, was marked
  // ready anyway, and its first autosave replaced the 13 KB skull-zanatomy file with an
  // empty document. `ready` (= autosave allowed) is now only set when the document's
  // state is actually known: loaded, or confirmed absent on the server. Otherwise the
  // page stays read-only for this session and says so.
  const known = await load_current_document_inner(state, tablet_document, camera, name);
  state.ready = known;
  if (known && !has_unsaved_edits(state)) show_save_status(state, "saved", "saved");
  if (!known) window.alert(`Document '${name}' could not be loaded (missing on the server, or the server is down). Autosave is OFF for this tab; reload to try again.`);
}

// Returns whether the document's state is known (loaded, or confirmed new).
async function load_current_document_inner(
  state: PersistenceState, tablet_document: TabletDocument, camera: OrbitCamera, name: string,
): Promise<boolean> {
  const buffer = read_crash_buffer(state);
  const fetched = await fetch_document_from_server(name);
  if (fetched === "failed") return false;
  if (fetched === "unreachable") {
    console.error("document server unreachable at startup — using localStorage buffer");
    if (buffer === null || buffer.name !== name) return false;
    if (!apply_document_state(buffer.json, tablet_document, camera)) return false;
    restore_remembered_camera(state, camera);
    state.base_revision = buffer.base_revision ?? null;
    if (!buffer.server_saved) state.last_saved_history_snapshot = RESTORED_CRASH_BUFFER_NOT_SAVED;
    return true;
  }
  const server_json = fetched === "missing" ? null : fetched.json;
  const server_revision = fetched === "missing" ? null : fetched.revision;
  // An unconfirmed buffer equal to the file was saved after all (the exit flush's
  // response is never seen). Buffers written before base_revision existed cannot be
  // placed and are ignored.
  if (buffer !== null && buffer.name === name && !buffer.server_saved && buffer.json !== server_json && buffer.base_revision !== undefined) {
    if (!apply_document_state(buffer.json, tablet_document, camera)) return false;
    restore_remembered_camera(state, camera);
    state.base_revision = buffer.base_revision;
    state.last_saved_history_snapshot = RESTORED_CRASH_BUFFER_NOT_SAVED;
    if (buffer.base_revision === server_revision) {
      console.log(`restoring unsaved crash buffer for '${name}' and pushing to server`);
      await save_now(state, tablet_document, camera);
    } else {
      console.warn(`restoring unsaved crash buffer for '${name}' as a conflict: the server file changed since`);
      enter_conflict_state(state);
    }
    return true;
  }
  // NOTE(kv): a missing file is an error at startup, not a new document (Khoa,
  // 2026-09-28): the tab stays empty with autosave OFF, so it can never create (or
  // later clobber) a file by accident. New documents come only from "new…" in the docs
  // panel (switch_document).
  if (fetched === "missing") {
    console.error(`document '${name}' does not exist on the server`);
    return false;
  }
  // `base_revision` is set only once the loaded document is really in the tab.
  if (!apply_document_state(fetched.json, tablet_document, camera)) return false;
  restore_remembered_camera(state, camera);
  state.base_revision = fetched.revision;
  return true;
}

// Rename the current document: save it under the new name, then delete the
// old server file. Refuses a name the server already has (no silent clobber).
// Returns false when nothing changed (name taken / server unreachable).
export async function rename_document(
  state: PersistenceState, tablet_document: TabletDocument, camera: OrbitCamera, new_name: string,
): Promise<boolean> {
  const old_name = state.current_document_name;
  if (new_name === old_name) return false;
  if (state.is_in_conflict) { // the old file, deleted below, holds someone else's newer work
    window.alert("This document changed elsewhere. Choose 'load newer' or 'fork' first.");
    return false;
  }
  const existing = await list_documents_from_server();
  if (existing === null) return false;
  if (existing.some((entry) => entry.name === new_name)) {
    window.alert(`A document named '${new_name}' already exists.`);
    return false;
  }
  if (state.autosave_timer !== null) {
    window.clearTimeout(state.autosave_timer);
    state.autosave_timer = null;
  }
  const json = serialize_document_state(tablet_document, camera);
  const history_snapshot = state.history_snapshot;
  const result = await save_to_server(new_name, json, null);
  if (result.outcome !== "saved") return false;
  state.current_document_name = new_name;
  state.base_revision = result.revision;
  state.last_saved_history_snapshot = history_snapshot;
  remember_current_name(state, new_name);
  write_crash_buffer(state, { name: new_name, json, server_saved: true, base_revision: result.revision });
  // The old file only exists if it was saved at least once.
  if (existing.some((entry) => entry.name === old_name)) {
    try {
      const response = await fetch(`/api/documents/${encodeURIComponent(old_name)}`, { method: "DELETE" });
      if (!response.ok) window.alert(`Renamed, but the old copy '${old_name}' could not be deleted (${response.status}).`);
    } catch (error) {
      console.error(`deleting old document '${old_name}' failed`, error);
      window.alert(`Renamed, but the old copy '${old_name}' could not be deleted (server unreachable).`);
    }
  }
  return true;
}

// Switch to (or create) another document: flush the current one first, then
// load the target — a name the server doesn't know starts empty.
export async function switch_document(
  state: PersistenceState, tablet_document: TabletDocument, camera: OrbitCamera, name: string,
): Promise<void> {
  if (state.autosave_timer !== null) {
    window.clearTimeout(state.autosave_timer);
    state.autosave_timer = null;
  }
  await flush_autosave(state, tablet_document, camera);
  remember_camera(state, camera); // the page's view carries over to the next document
  state.ready = false; // no autosave of the half-switched state

  state.current_document_name = name;
  state.base_revision = null;
  state.history_snapshot = null; // the sketchpad clears its history on switch
  state.last_saved_history_snapshot = null;
  remember_current_name(state, name);
  clear_document_in_place(tablet_document);
  const fetched = await fetch_document_from_server(name);
  // Autosave comes back only when the document's state is known: the file is loaded
  // into the tab, or the server said it does not exist (a new document).
  let known = fetched === "missing";
  if (typeof fetched !== "string" && apply_document_state(fetched.json, tablet_document, camera)) {
    restore_remembered_camera(state, camera);
    state.base_revision = fetched.revision;
    known = true;
  }
  state.ready = known;
  if (known) {
    show_save_status(state, "saved", "saved");
  } else {
    show_save_status(state, "off", "autosave OFF");
    window.alert(`Document '${name}' could not be loaded. Autosave is OFF for this tab; reload to try again.`);
  }
}
