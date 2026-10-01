// Scratch check of document_backup against temp dirs (never the real Drive folder).
// Run: npx tsx tmp/document_backup_check.ts
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { backup_folder_name, document_backup } from "../document_backup";

const scratch = fs.mkdtempSync(path.join(os.tmpdir(), "document-backup-check-"));
const documents = path.join(scratch, "documents");
const root = path.join(scratch, "backup");
fs.mkdirSync(documents);
fs.mkdirSync(root);
fs.writeFileSync(path.join(documents, "a.json"), "x".repeat(1000));
fs.writeFileSync(path.join(documents, "b.json"), "y".repeat(1000));
fs.writeFileSync(path.join(documents, "notes.txt"), "not a document");

let failures = 0;
function expect(label: string, actual: unknown, wanted: unknown): void {
  const ok = JSON.stringify(actual) === JSON.stringify(wanted);
  if (!ok) failures++;
  console.log(`${ok ? "ok  " : "FAIL"} ${label}${ok ? "" : `: got ${JSON.stringify(actual)}, wanted ${JSON.stringify(wanted)}`}`);
}
const folders = (): string[] => fs.readdirSync(root).sort();
const at = (day: number, hour: number, minute: number): Date => new Date(2026, 9, day, hour, minute, 30);

expect("slot name floors minutes", backup_folder_name(at(2, 9, 17)), "2026-10-02_09-10");

document_backup(documents, root, at(1, 9, 3));
expect("first save makes a folder", folders(), ["2026-10-01_09-00"]);
expect("only json copied", fs.readdirSync(path.join(root, "2026-10-01_09-00")).sort(), ["a.json", "b.json"]);

fs.writeFileSync(path.join(documents, "a.json"), "changed");
document_backup(documents, root, at(1, 9, 8));
expect("same slot: no new folder", folders(), ["2026-10-01_09-00"]);
expect("same slot: backup untouched", fs.readFileSync(path.join(root, "2026-10-01_09-00", "a.json"), "utf8").length, 1000);

// Day 1: 8 slots in total. Newest 6 kept; of the 2 pushed out only the day's earliest stays.
for (let i = 1; i < 8; i++) document_backup(documents, root, at(1, 9 + Math.floor(i / 6), (i % 6) * 10));
expect("day 1 after 8 slots", folders(), ["2026-10-01_09-00", "2026-10-01_09-20", "2026-10-01_09-30", "2026-10-01_09-40", "2026-10-01_09-50", "2026-10-01_10-00", "2026-10-01_10-10"]);

// Next day, nothing thinned by age alone: one new slot pushes out one more of day 1.
document_backup(documents, root, at(2, 8, 0));
expect("next morning: yesterday's recent ones still there", folders(), ["2026-10-01_09-00", "2026-10-01_09-30", "2026-10-01_09-40", "2026-10-01_09-50", "2026-10-01_10-00", "2026-10-01_10-10", "2026-10-02_08-00"]);

// Seven more slots on day 2: day 1 collapses to its earliest, day 2 keeps earliest + newest 6.
for (let i = 1; i <= 7; i++) document_backup(documents, root, new Date(2026, 9, 2, 8 + Math.floor(i / 6), (i % 6) * 10, 30));
expect("two days", folders(), ["2026-10-01_09-00", "2026-10-02_08-00", "2026-10-02_08-20", "2026-10-02_08-30", "2026-10-02_08-40", "2026-10-02_08-50", "2026-10-02_09-00", "2026-10-02_09-10"]);

// Size cap: each folder is ~1 KB ("changed" + 1000). A 3 KB cap deletes oldest first.
document_backup(documents, root, at(3, 8, 0), 3100);
const after_cap = folders();
expect("size cap keeps the newest", after_cap[after_cap.length - 1], "2026-10-03_08-00");
expect("size cap deleted the oldest", after_cap.includes("2026-10-01_09-00"), false);
expect("size cap leaves about 3 folders", after_cap.length, 3);

// No Drive: logs, does not throw, makes nothing.
document_backup(documents, null, at(4, 8, 0));
expect("no root: nothing created", folders().some((name) => name.startsWith("2026-10-04")), false);
expect("no .partial left behind", folders().some((name) => name.endsWith(".partial")), false);

fs.rmSync(scratch, { recursive: true, force: true });
console.log(failures === 0 ? "ALL OK" : `${failures} FAILED`);
