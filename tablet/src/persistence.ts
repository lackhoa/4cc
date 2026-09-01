// Document persistence (plan-skull-reference.md Q6b-Q9): the dev server's
// documents/ dir is the source of truth (git-tracked JSON, one file per
// document); localStorage is only a crash buffer bridging reloads while the
// server is unreachable. Debounced autosave, no save button.

import { OrbitCamera } from "./camera";
import { TabletDocument } from "./document";

export const DEFAULT_DOCUMENT_NAME = "untitled";
const DOCUMENT_FORMAT_VERSION = 1;
const AUTOSAVE_DEBOUNCE_MS = 2000;
const CURRENT_NAME_STORAGE_KEY = "autodraw_tablet_current_document";
const BUFFER_STORAGE_KEY = "autodraw_tablet_crash_buffer";

export type PersistenceState = {
  current_document_name: string;
  last_saved_json: string | null; // skip autosaves when nothing changed
  autosave_timer: number | null;
  // False until the startup load (or a document switch) finishes — blocks
  // autosave from overwriting the stored document with the pre-load state.
  ready: boolean;
};

// What sits in localStorage: the last autosaved snapshot plus whether the
// server confirmed it. server_saved=false after a reload means the server
// never got it — push it on reconnect.
type CrashBuffer = { name: string; json: string; server_saved: boolean };

export function create_persistence_state(): PersistenceState {
  let name = DEFAULT_DOCUMENT_NAME;
  try {
    name = localStorage.getItem(CURRENT_NAME_STORAGE_KEY) ?? DEFAULT_DOCUMENT_NAME;
  } catch { /* storage unavailable (private mode) — default name */ }
  return { current_document_name: name, last_saved_json: null, autosave_timer: null, ready: false };
}

function serialize_document_state(tablet_document: TabletDocument, camera: OrbitCamera): string {
  return JSON.stringify({
    version: DOCUMENT_FORMAT_VERSION,
    camera: { pivot: camera.pivot, yaw: camera.yaw, pitch: camera.pitch, distance: camera.distance },
    document: tablet_document,
  }, null, 1);
}

export function clear_document_in_place(tablet_document: TabletDocument): void {
  tablet_document.vertices.length = 0;
  tablet_document.strokes.length = 0;
  tablet_document.lofts.length = 0;
  tablet_document.revolves.length = 0;
  tablet_document.inflates.length = 0;
  tablet_document.coons.length = 0;
}

// Parse + apply a stored snapshot into the live document/camera (in place —
// main.ts holds references to both). Returns false on a bad payload.
function apply_document_state(json: string, tablet_document: TabletDocument, camera: OrbitCamera): boolean {
  let parsed;
  try {
    parsed = JSON.parse(json);
  } catch (error) {
    console.error("stored document is not valid JSON", error);
    return false;
  }
  if (parsed.version !== DOCUMENT_FORMAT_VERSION) {
    console.error(`stored document has version ${parsed.version}, expected ${DOCUMENT_FORMAT_VERSION}`);
    return false;
  }
  clear_document_in_place(tablet_document);
  tablet_document.vertices.push(...parsed.document.vertices);
  tablet_document.strokes.push(...parsed.document.strokes);
  tablet_document.lofts.push(...parsed.document.lofts);
  tablet_document.revolves.push(...parsed.document.revolves);
  tablet_document.inflates.push(...parsed.document.inflates);
  tablet_document.coons.push(...parsed.document.coons);
  camera.pivot = parsed.camera.pivot;
  camera.yaw = parsed.camera.yaw;
  camera.pitch = parsed.camera.pitch;
  camera.distance = parsed.camera.distance;
  return true;
}

function read_crash_buffer(): CrashBuffer | null {
  try {
    const raw = localStorage.getItem(BUFFER_STORAGE_KEY);
    return raw === null ? null : JSON.parse(raw);
  } catch {
    return null;
  }
}

function write_crash_buffer(buffer: CrashBuffer): void {
  try {
    localStorage.setItem(BUFFER_STORAGE_KEY, JSON.stringify(buffer));
  } catch { /* storage unavailable — server save still happens */ }
}

function remember_current_name(name: string): void {
  try {
    localStorage.setItem(CURRENT_NAME_STORAGE_KEY, name);
  } catch { /* storage unavailable */ }
}

async function save_to_server(name: string, json: string): Promise<boolean> {
  try {
    const response = await fetch(`/api/documents/${encodeURIComponent(name)}`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: json,
    });
    if (!response.ok) console.error(`document save failed: ${response.status}`);
    return response.ok;
  } catch (error) {
    console.error("document save failed (server unreachable)", error);
    return false;
  }
}

async function save_now(state: PersistenceState, tablet_document: TabletDocument, camera: OrbitCamera): Promise<void> {
  const json = serialize_document_state(tablet_document, camera);
  if (json === state.last_saved_json) return;
  const name = state.current_document_name;
  write_crash_buffer({ name, json, server_saved: false });
  const saved = await save_to_server(name, json);
  if (saved) {
    write_crash_buffer({ name, json, server_saved: true });
    state.last_saved_json = json;
  }
  // Not saved: last_saved_json stays stale so the next autosave retries the server.
}

// Called on every render request; fires one save ~2 s after the last mutation.
export function schedule_autosave(state: PersistenceState, tablet_document: TabletDocument, camera: OrbitCamera): void {
  if (!state.ready) return;
  if (state.autosave_timer !== null) window.clearTimeout(state.autosave_timer);
  state.autosave_timer = window.setTimeout(() => {
    state.autosave_timer = null;
    void save_now(state, tablet_document, camera);
  }, AUTOSAVE_DEBOUNCE_MS);
}

export type DocumentListEntry = { name: string; mtime_ms: number };

export async function list_documents_from_server(): Promise<DocumentListEntry[] | null> {
  try {
    const response = await fetch("/api/documents");
    if (!response.ok) return null;
    return await response.json();
  } catch {
    return null;
  }
}

// Startup: prefer the server copy of the current document; a crash buffer the
// server never confirmed is newer — restore it and push it up. With the server
// unreachable, fall back to the buffer alone.
export async function load_current_document_on_startup(
  state: PersistenceState, tablet_document: TabletDocument, camera: OrbitCamera,
): Promise<void> {
  const name = state.current_document_name;
  const buffer = read_crash_buffer();
  const server_list = await list_documents_from_server();
  try {
    await load_current_document_inner(state, tablet_document, camera, name, buffer, server_list);
  } finally {
    state.ready = true;
  }
}

async function load_current_document_inner(
  state: PersistenceState, tablet_document: TabletDocument, camera: OrbitCamera,
  name: string, buffer: CrashBuffer | null, server_list: DocumentListEntry[] | null,
): Promise<void> {
  if (server_list === null) {
    console.error("document server unreachable at startup — using localStorage buffer");
    if (buffer !== null && buffer.name === name) apply_document_state(buffer.json, tablet_document, camera);
    return;
  }
  if (buffer !== null && buffer.name === name && !buffer.server_saved) {
    console.log(`restoring unsaved crash buffer for '${name}' and pushing to server`);
    if (apply_document_state(buffer.json, tablet_document, camera)) {
      await save_now(state, tablet_document, camera);
    }
    return;
  }
  if (server_list.some((entry) => entry.name === name)) {
    try {
      const response = await fetch(`/api/documents/${encodeURIComponent(name)}`);
      const json = await response.text();
      if (apply_document_state(json, tablet_document, camera)) state.last_saved_json = json;
    } catch (error) {
      console.error(`loading document '${name}' failed`, error);
    }
  }
  // Name unknown to the server and no buffer: fresh empty document.
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
  await save_now(state, tablet_document, camera);
  state.ready = false; // no autosave of the half-switched state

  state.current_document_name = name;
  state.last_saved_json = null;
  remember_current_name(name);
  clear_document_in_place(tablet_document);
  try {
    const response = await fetch(`/api/documents/${encodeURIComponent(name)}`);
    if (response.ok) {
      const json = await response.text();
      if (apply_document_state(json, tablet_document, camera)) state.last_saved_json = json;
    }
  } catch (error) {
    console.error(`loading document '${name}' failed`, error);
  } finally {
    state.ready = true;
  }
}
