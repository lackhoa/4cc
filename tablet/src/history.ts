// Undo/redo (Q1 in notes plan-tablet-undo-redo.md): whole-document JSON snapshots, one
// per gesture. A step opens when a mutating gesture begins (pen-down, or just
// before a button action) and closes when it ends — the opening snapshot is
// pushed only if the document actually changed, so orbit drags and no-op taps
// never pollute the history. Camera state is not part of a snapshot.
// In-memory only, per document (cleared on switch), capped.

import { TabletDocument } from "./document";
import { clear_document_in_place } from "./persistence";

const MAX_UNDO_SNAPSHOTS = 100;

export type HistoryState = {
  undo_stack: string[]; // serialized documents, oldest first
  redo_stack: string[];
  pending: string | null; // snapshot taken when the current gesture began
};

export function create_history_state(): HistoryState {
  return { undo_stack: [], redo_stack: [], pending: null };
}

export function clear_history(history: HistoryState): void {
  history.undo_stack.length = 0;
  history.redo_stack.length = 0;
  history.pending = null;
}

function serialize_document(tablet_document: TabletDocument): string {
  return JSON.stringify(tablet_document);
}

function restore_document_in_place(tablet_document: TabletDocument, snapshot: string): void {
  const parsed = JSON.parse(snapshot);
  clear_document_in_place(tablet_document);
  tablet_document.vertices.push(...parsed.vertices);
  tablet_document.vertex_pins.push(...parsed.vertex_pins);
  tablet_document.strokes.push(...parsed.strokes);
  tablet_document.lofts.push(...parsed.lofts);
  tablet_document.revolves.push(...parsed.revolves);
  tablet_document.inflates.push(...parsed.inflates);
  tablet_document.coons.push(...parsed.coons);
}

export function begin_history_step(history: HistoryState, tablet_document: TabletDocument): void {
  history.pending = serialize_document(tablet_document);
}

export function end_history_step(history: HistoryState, tablet_document: TabletDocument): void {
  if (history.pending === null) return;
  const before = history.pending;
  history.pending = null;
  if (before === serialize_document(tablet_document)) return; // gesture changed nothing
  history.undo_stack.push(before);
  if (history.undo_stack.length > MAX_UNDO_SNAPSHOTS) history.undo_stack.shift();
  history.redo_stack.length = 0;
}

// Both return false when their stack is empty (nothing happens).
export function undo(history: HistoryState, tablet_document: TabletDocument): boolean {
  const snapshot = history.undo_stack.pop();
  if (snapshot === undefined) return false;
  history.redo_stack.push(serialize_document(tablet_document));
  restore_document_in_place(tablet_document, snapshot);
  return true;
}

export function redo(history: HistoryState, tablet_document: TabletDocument): boolean {
  const snapshot = history.redo_stack.pop();
  if (snapshot === undefined) return false;
  history.undo_stack.push(serialize_document(tablet_document));
  restore_document_in_place(tablet_document, snapshot);
  return true;
}
