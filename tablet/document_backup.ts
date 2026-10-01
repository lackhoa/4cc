// Automatic document backup (plan-tablet-document-auto-backup.md), the tablet twin of
// game/game_document_backup.cpp. Runs inside the document server, right before a save
// writes a file, so a backup holds the documents as they were before that edit.
//
//   <root>/YYYY-MM-DD_HH-MM/*.json   one folder per 10-minute slot that had a save
//
// Two tiers, one mechanism: the newest RECENT_BACKUP_COUNT folders are always kept (by
// count, not by age: the last hour of actual work, even days later). A folder pushed
// past that count survives only if it is the earliest of its day. On top of that, the
// oldest folders go while the root is over DOCUMENT_BACKUP_MAX_BYTES.
//
// Backups are never loaded by the app; restoring is a manual copy back into documents/.

import fs from "node:fs";
import os from "node:os";
import path from "node:path";

export const DOCUMENT_BACKUP_MAX_BYTES = 100 * 1024 * 1024;
const BACKUP_SLOT_MINUTES = 10;
const RECENT_BACKUP_COUNT = 6;
const BACKUP_FOLDER_PATTERN = /^\d{4}-\d{2}-\d{2}_\d{2}-\d{2}$/;

// A sibling of the C++ app's autodraw-backup, not inside it: each app prunes the oldest
// folder under its own root and would delete the other's backups. Null when Google Drive
// is not mounted, so no stray folder appears in the home directory.
export function default_document_backup_root(): string | null {
  const drive = path.join(os.homedir(), "personal-drive");
  if (!fs.existsSync(drive)) return null;
  return path.join(drive, "autodraw-tablet-backup");
}

function two_digits(value: number): string {
  return value.toString().padStart(2, "0");
}

// Local time, minutes floored to the slot. Sorts chronologically as a plain string.
export function backup_folder_name(now: Date): string {
  const minutes = Math.floor(now.getMinutes() / BACKUP_SLOT_MINUTES) * BACKUP_SLOT_MINUTES;
  return `${now.getFullYear()}-${two_digits(now.getMonth() + 1)}-${two_digits(now.getDate())}_${two_digits(now.getHours())}-${two_digits(minutes)}`;
}

function backup_folders_oldest_first(root: string): string[] {
  return fs.readdirSync(root).filter((name) => BACKUP_FOLDER_PATTERN.test(name)).sort();
}

function delete_backup_folder(root: string, name: string): void {
  fs.rmSync(path.join(root, name), { recursive: true, force: true });
}

function thin_old_backups(root: string): void {
  const folders = backup_folders_oldest_first(root);
  const old_folders = folders.slice(0, Math.max(0, folders.length - RECENT_BACKUP_COUNT));
  const days_seen = new Set<string>();
  for (const name of old_folders) {
    const day = name.slice(0, "YYYY-MM-DD".length);
    if (days_seen.has(day)) delete_backup_folder(root, name);
    else days_seen.add(day); // oldest first, so the first one seen is the day's earliest
  }
}

function folder_bytes(folder: string): number {
  let bytes = 0;
  for (const file_name of fs.readdirSync(folder)) bytes += fs.statSync(path.join(folder, file_name)).size;
  return bytes;
}

function prune_over_size_cap(root: string, max_bytes: number): void {
  const folders = backup_folders_oldest_first(root);
  const sizes = folders.map((name) => folder_bytes(path.join(root, name)));
  let total_bytes = sizes.reduce((sum, size) => sum + size, 0);
  // Never the last folder: one oversized backup beats none.
  for (let i = 0; i < folders.length - 1 && total_bytes > max_bytes; i++) {
    delete_backup_folder(root, folders[i]);
    total_bytes -= sizes[i];
    console.log(`document backup: over the size cap, deleted ${folders[i]}`);
  }
}

// Copy every documents/*.json into this slot's folder unless the slot already has one,
// then thin and prune. Never throws: a failed backup must not block the save.
export function document_backup(documents_directory: string, root: string | null, now: Date = new Date(), max_bytes: number = DOCUMENT_BACKUP_MAX_BYTES): void {
  try {
    if (root === null) {
      console.error("document backup: no backup location (Google Drive not mounted?)");
      return;
    }
    const slot_folder = path.join(root, backup_folder_name(now));
    if (fs.existsSync(slot_folder)) return;
    const file_names = fs.existsSync(documents_directory) ? fs.readdirSync(documents_directory).filter((file_name) => file_name.endsWith(".json")) : [];
    if (file_names.length === 0) return;
    // Copy into a scratch folder and rename, so a half-copied folder is never mistaken
    // for a finished backup (which would also block this slot from retrying).
    const partial_folder = `${slot_folder}.partial`;
    fs.rmSync(partial_folder, { recursive: true, force: true });
    fs.mkdirSync(partial_folder, { recursive: true });
    for (const file_name of file_names) fs.copyFileSync(path.join(documents_directory, file_name), path.join(partial_folder, file_name));
    fs.renameSync(partial_folder, slot_folder);
    console.log(`document backup: copied to ${slot_folder}`);
    thin_old_backups(root);
    prune_over_size_cap(root, max_bytes);
  } catch (error) {
    console.error(`document backup FAILED: ${error}`);
  }
}
