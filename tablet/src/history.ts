// Undo/redo (Q1 in notes plan-tablet-undo-redo.md): whole-document JSON snapshots, one
// per gesture. A step opens when a mutating gesture begins (pen-down, or just
// before a button action) and closes when it ends — the closing snapshot is
// pushed only if the document actually changed, so orbit drags and no-op taps
// never pollute the history. Camera state is not part of a snapshot.
// In-memory only, per document (cleared on switch), capped.
//
// Same shape as the C++ app's Document_History (game/game_document_history.cpp):
// entry 0 is the state before the first edit ("loaded"), every later entry is the
// document after one labelled edit, `position` is the entry the document equals.
// Undo/redo/panel click = jump to an entry.

import { TabletDocument } from "./document";
import { clear_document_in_place } from "./persistence";

const MAX_HISTORY_ENTRIES = 100;

export type HistoryEntry = {
  label: string; // one row of the history panel, e.g. "move vertex 3"
  snapshot: string; // serialized document after the edit
};

export type HistoryState = {
  entries: HistoryEntry[];
  position: number; // index into entries the document currently equals; -1 while empty
  pending: string | null; // snapshot taken when the current gesture began
};

export function create_history_state(): HistoryState {
  return { entries: [], position: -1, pending: null };
}

export function clear_history(history: HistoryState): void {
  history.entries.length = 0;
  history.position = -1;
  history.pending = null;
}

function serialize_document(tablet_document: TabletDocument): string {
  return JSON.stringify(tablet_document);
}

function restore_document_in_place(tablet_document: TabletDocument, snapshot: string): void {
  const parsed = JSON.parse(snapshot);
  clear_document_in_place(tablet_document);
  tablet_document.next_vertex_id = parsed.next_vertex_id;
  tablet_document.next_stroke_id = parsed.next_stroke_id;
  tablet_document.next_patch_id = parsed.next_patch_id;
  tablet_document.bones.length = 0;
  tablet_document.bones.push(...parsed.bones);
  tablet_document.vertices.push(...parsed.vertices);
  tablet_document.vertex_pins.push(...parsed.vertex_pins);
  tablet_document.vertex_surface_pins.push(...(parsed.vertex_surface_pins ?? []));
  tablet_document.smooth_knots.push(...parsed.smooth_knots);
  tablet_document.strokes.push(...parsed.strokes);
  tablet_document.patches.push(...parsed.patches);
}

export function begin_history_step(history: HistoryState, tablet_document: TabletDocument): void {
  history.pending = serialize_document(tablet_document);
}

// `label` names the edit in the panel; ignored when the gesture changed nothing.
// With `merge_into_same_label`, a run of edits with the same label (keyboard
// nudges of one vertex) collapses into the entry it extends: the snapshot is
// replaced instead of a new entry pushed. Any other edit in between (or an
// undo) ends the run.
export function end_history_step(
  history: HistoryState, tablet_document: TabletDocument, label: string, merge_into_same_label: boolean = false,
): void {
  if (history.pending === null) return;
  const before = history.pending;
  history.pending = null;
  const after = serialize_document(tablet_document);
  if (before === after) return; // gesture changed nothing
  if (history.entries.length === 0) {
    history.entries.push({ label: "loaded", snapshot: before });
    history.position = 0;
  }
  history.entries.length = history.position + 1; // drop the redo tail
  const current = history.entries[history.position];
  if (merge_into_same_label && history.position > 0 && current.label === label) {
    current.snapshot = after;
    return;
  }
  if (history.entries.length === MAX_HISTORY_ENTRIES) history.entries.shift();
  history.entries.push({ label, snapshot: after });
  history.position = history.entries.length - 1;
}

// Make the document equal entry `position`; false when out of range or already there.
export function jump_history(history: HistoryState, tablet_document: TabletDocument, position: number): boolean {
  if (position < 0 || position >= history.entries.length || position === history.position) return false;
  history.pending = null;
  history.position = position;
  restore_document_in_place(tablet_document, history.entries[position].snapshot);
  return true;
}

export function undo(history: HistoryState, tablet_document: TabletDocument): boolean {
  return jump_history(history, tablet_document, history.position - 1);
}

export function redo(history: HistoryState, tablet_document: TabletDocument): boolean {
  return jump_history(history, tablet_document, history.position + 1);
}
