// Dev-server plugin: serves the git-tracked reference models (data/reference-models/,
// copies of the Google Drive originals -- the C++ driver still reads those) and persists
// tablet documents as git-tracked JSON in tablet/documents/ (plan-skull-reference.md).
//   GET  /reference/<file>        -> ../data/reference-models/<file>
//   POST /reference/<mesh>.landmarks.txt -> overwrite that sidecar with the body
//   GET  /api/documents           -> [{ name, mtime_ms, revision }]
//   GET  /api/documents/<name>    -> stored JSON, header X-Revision
//   POST /api/documents/<name>    -> write body to documents/<name>.json
//                                    (after document_backup, see document_backup.ts);
//                                    header X-Base-Revision in, X-Revision out; 409 when
//                                    the file on disk is not the revision the save is based on
//   GET  /api/fit-landmarks/<name> -> fit-landmarks/<name>.json (src/loomis_girl_plate.ts), 404 when there is none
//   POST /api/fit-landmarks/<name> -> write body to fit-landmarks/<name>.json (after document_backup)
// A document's `revision` is the SHA-256 of its file's bytes: computed here only, never
// written to disk, and compared for equality only.

import crypto from "node:crypto";
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { IncomingMessage, ServerResponse } from "node:http";
import { Plugin, defineConfig } from "vite";
import checker from "vite-plugin-checker";
import { default_document_backup_root, document_backup } from "./document_backup";

const documents_directory = path.join(path.dirname(fileURLToPath(import.meta.url)), "documents");
const fit_landmarks_directory = path.join(path.dirname(fileURLToPath(import.meta.url)), "fit-landmarks");
const reference_models_directory = path.join(path.dirname(fileURLToPath(import.meta.url)), "../data/reference-models");

// Document names come from URLs — restrict to a safe charset so they can never
// escape the documents directory.
function is_safe_document_name(name: string): boolean {
  return /^[A-Za-z0-9_-]{1,64}$/.test(name);
}

function send_json(response: ServerResponse, status: number, body: unknown): void {
  response.statusCode = status;
  response.setHeader("Content-Type", "application/json");
  response.end(JSON.stringify(body));
}

function handle_reference_request(request: IncomingMessage, response: ServerResponse): void {
  const file_name = decodeURIComponent(request.url!.slice("/reference/".length));
  if (file_name !== path.basename(file_name)) {
    send_json(response, 400, { error: "bad reference file name" });
    return;
  }
  const file_path = path.join(reference_models_directory, file_name);
  if (!fs.existsSync(file_path)) {
    send_json(response, 404, { error: `no reference model ${file_name}` });
    return;
  }
  response.setHeader("Content-Type", "application/octet-stream");
  response.end(fs.readFileSync(file_path));
}

// Pages write landmarks back (skull-ball picks the glabella); only `.landmarks.txt`
// sidecars are writable, the meshes never are.
function handle_reference_landmarks_save(request: IncomingMessage, response: ServerResponse): void {
  const file_name = decodeURIComponent(request.url!.slice("/reference/".length));
  if (file_name !== path.basename(file_name) || !file_name.endsWith(".landmarks.txt")) {
    send_json(response, 400, { error: "only <mesh>.landmarks.txt files can be written" });
    return;
  }
  const chunks: Buffer[] = [];
  request.on("data", (chunk: Buffer) => chunks.push(chunk));
  request.on("end", () => {
    fs.writeFileSync(path.join(reference_models_directory, file_name), Buffer.concat(chunks));
    send_json(response, 200, { ok: true });
  });
}

function compute_document_revision(file_bytes: Buffer): string {
  return crypto.createHash("sha256").update(file_bytes).digest("hex");
}

// What the list remembers per document: pages poll the list every 2 s, and a file whose
// modified time and size are unchanged is not hashed again.
type ListedDocumentRevision = { mtime_ms: number; size: number; revision: string };
const listed_revision_by_document_name = new Map<string, ListedDocumentRevision>();

function handle_document_list(response: ServerResponse): void {
  if (!fs.existsSync(documents_directory)) fs.mkdirSync(documents_directory);
  const entries = fs.readdirSync(documents_directory)
    .filter((file_name) => file_name.endsWith(".json"))
    .map((file_name) => {
      const name = file_name.slice(0, -".json".length);
      const file_path = path.join(documents_directory, file_name);
      const stat = fs.statSync(file_path);
      let listed = listed_revision_by_document_name.get(name);
      if (listed === undefined || listed.mtime_ms !== stat.mtimeMs || listed.size !== stat.size) {
        listed = { mtime_ms: stat.mtimeMs, size: stat.size, revision: compute_document_revision(fs.readFileSync(file_path)) };
        listed_revision_by_document_name.set(name, listed);
      }
      return { name, mtime_ms: stat.mtimeMs, revision: listed.revision };
    });
  send_json(response, 200, entries);
}

function handle_document_load(name: string, response: ServerResponse): void {
  const file_path = path.join(documents_directory, `${name}.json`);
  if (!fs.existsSync(file_path)) {
    send_json(response, 404, { error: `no document ${name}` });
    return;
  }
  // The revision is of the very bytes sent.
  const file_bytes = fs.readFileSync(file_path);
  response.setHeader("Content-Type", "application/json");
  response.setHeader("X-Revision", compute_document_revision(file_bytes));
  response.end(file_bytes);
}

function handle_document_save(name: string, request: IncomingMessage, response: ServerResponse): void {
  const chunks: Buffer[] = [];
  request.on("data", (chunk: Buffer) => chunks.push(chunk));
  request.on("end", () => {
    const body_bytes = Buffer.concat(chunks);
    try {
      JSON.parse(body_bytes.toString("utf8")); // refuse to persist a corrupt payload
    } catch {
      send_json(response, 400, { error: "body is not valid JSON" });
      return;
    }
    if (!fs.existsSync(documents_directory)) fs.mkdirSync(documents_directory);
    const file_path = path.join(documents_directory, `${name}.json`);
    // A save replaces only the file it is based on: the sender names the revision it
    // loaded (or last saved), and anything else on disk means someone else wrote in
    // between. No header is right only for a file that does not exist yet. The other
    // server process (dev / preview) can still write between this check and the rename.
    if (fs.existsSync(file_path)) {
      const revision_on_disk = compute_document_revision(fs.readFileSync(file_path));
      if (request.headers["x-base-revision"] !== revision_on_disk) {
        console.error(`refused to save document ${name}: based on revision ${request.headers["x-base-revision"]}, the file is ${revision_on_disk}`);
        send_json(response, 409, { error: `document ${name} changed since this save's base revision`, revision: revision_on_disk });
        return;
      }
    }
    // Before the write: the backup holds the documents as they were before this edit.
    document_backup(documents_directory, fit_landmarks_directory, default_document_backup_root());
    // Temp file + rename: a reader never sees a half-written document.
    const temporary_path = `${file_path}.tmp-${process.pid}`;
    try {
      fs.writeFileSync(temporary_path, body_bytes);
      fs.renameSync(temporary_path, file_path);
    } catch (error) {
      console.error(`writing document ${name} failed`, error);
      send_json(response, 500, { error: `writing document ${name} failed: ${error}` });
      return;
    }
    response.setHeader("X-Revision", compute_document_revision(body_bytes));
    send_json(response, 200, { ok: true, mtime_ms: fs.statSync(file_path).mtimeMs });
  });
}

function handle_document_delete(name: string, response: ServerResponse): void {
  const file_path = path.join(documents_directory, `${name}.json`);
  if (!fs.existsSync(file_path)) {
    send_json(response, 404, { error: "no such document" });
    return;
  }
  fs.unlinkSync(file_path);
  send_json(response, 200, { ok: true });
}

function handle_fit_landmarks_load(name: string, response: ServerResponse): void {
  const file_path = path.join(fit_landmarks_directory, `${name}.json`);
  if (!fs.existsSync(file_path)) {
    send_json(response, 404, { error: `no fit landmarks ${name}` });
    return;
  }
  response.setHeader("Content-Type", "application/json");
  response.end(fs.readFileSync(file_path));
}

// The last write wins: one page edits a landmark file, and it saves on every edit.
function handle_fit_landmarks_save(name: string, request: IncomingMessage, response: ServerResponse): void {
  const chunks: Buffer[] = [];
  request.on("data", (chunk: Buffer) => chunks.push(chunk));
  request.on("end", () => {
    const body_bytes = Buffer.concat(chunks);
    try {
      JSON.parse(body_bytes.toString("utf8")); // refuse to persist a corrupt payload
    } catch {
      send_json(response, 400, { error: "body is not valid JSON" });
      return;
    }
    // Before the write: the backup holds the landmarks as they were before this edit.
    document_backup(documents_directory, fit_landmarks_directory, default_document_backup_root());
    const file_path = path.join(fit_landmarks_directory, `${name}.json`);
    // Temp file + rename: a reader never sees a half-written file.
    const temporary_path = `${file_path}.tmp-${process.pid}`;
    try {
      if (!fs.existsSync(fit_landmarks_directory)) fs.mkdirSync(fit_landmarks_directory);
      fs.writeFileSync(temporary_path, body_bytes);
      fs.renameSync(temporary_path, file_path);
    } catch (error) {
      console.error(`writing fit landmarks ${name} failed`, error);
      send_json(response, 500, { error: `writing fit landmarks ${name} failed: ${error}` });
      return;
    }
    send_json(response, 200, { ok: true });
  });
}

function tablet_api_middleware(request: IncomingMessage, response: ServerResponse, next: () => void): void {
  const url = request.url ?? "";
  if (url.startsWith("/reference/") && request.method === "GET") {
    handle_reference_request(request, response);
    return;
  }
  if (url.startsWith("/reference/") && request.method === "POST") {
    handle_reference_landmarks_save(request, response);
    return;
  }
  if (url === "/api/documents" && request.method === "GET") {
    handle_document_list(response);
    return;
  }
  if (url.startsWith("/api/fit-landmarks/")) {
    const name = decodeURIComponent(url.slice("/api/fit-landmarks/".length));
    if (!is_safe_document_name(name)) {
      send_json(response, 400, { error: "bad fit landmarks name" });
      return;
    }
    if (request.method === "GET") {
      handle_fit_landmarks_load(name, response);
      return;
    }
    if (request.method === "POST") {
      handle_fit_landmarks_save(name, request, response);
      return;
    }
  }
  if (url.startsWith("/api/documents/")) {
    const name = decodeURIComponent(url.slice("/api/documents/".length));
    if (!is_safe_document_name(name)) {
      send_json(response, 400, { error: "bad document name" });
      return;
    }
    if (request.method === "GET") {
      handle_document_load(name, response);
      return;
    }
    if (request.method === "POST") {
      handle_document_save(name, request, response);
      return;
    }
    if (request.method === "DELETE") {
      handle_document_delete(name, response);
      return;
    }
  }
  next();
}

// The same API on both servers: `vite` (dev, hot-reloading — the agent's
// sandbox) and `vite preview` (serves the built dist/ to the iPad; only a
// rebuild — "deploy" — changes what it serves).
function tablet_server_plugin(): Plugin {
  return {
    name: "tablet-server",
    configureServer(server) {
      server.middlewares.use(tablet_api_middleware);
    },
    configurePreviewServer(server) {
      server.middlewares.use(tablet_api_middleware);
    },
  };
}

// Preview has no live reload, so a rebuild would sit unseen until a manual refresh.
// Build only: every page gets a script that polls dist/build-id.txt and reloads when
// the id changes (i.e. after the next build); the reload waits for a held pointer
// button so a drawing stroke is never cut, and for unsaved edits to be saved. Dev
// already hot-reloads and is left alone.
function reload_on_rebuild_plugin(): Plugin {
  const build_id = Date.now().toString();
  return {
    name: "reload-on-rebuild",
    apply: "build",
    generateBundle() {
      this.emitFile({ type: "asset", fileName: "build-id.txt", source: build_id });
    },
    transformIndexHtml() {
      return [{
        tag: "script",
        injectTo: "body",
        children: `
          (() => {
            let pointer_is_down = false;
            window.addEventListener("pointerdown", () => { pointer_is_down = true; }, true);
            window.addEventListener("pointerup", () => { pointer_is_down = false; }, true);
            let reload_is_pending = false;
            setInterval(async () => {
              try {
                const response = await fetch("/build-id.txt", { cache: "no-store" });
                if (response.ok && (await response.text()) !== "${build_id}") reload_is_pending = true;
              } catch {}
              // A sketchpad page with unsaved edits (window.tablet_has_unsaved_edits,
              // src/sketchpad.ts) is not reloaded until they are saved.
              const has_unsaved_edits = typeof window.tablet_has_unsaved_edits === "function" && window.tablet_has_unsaved_edits();
              if (reload_is_pending && !pointer_is_down && !has_unsaved_edits) location.reload();
            }, 2000);
          })();`,
      }];
    },
  };
}

// Multi-page build: the main menu at /, the drawing view at /draw/, and every document
// page under pages/ (pages/<name>/index.html = document <name> = URL /pages/<name>/).
const tablet_directory = path.dirname(fileURLToPath(import.meta.url));
const pages_directory = path.join(tablet_directory, "pages");
const page_inputs: Record<string, string> = {
  menu: path.join(tablet_directory, "index.html"),
  draw: path.join(tablet_directory, "draw/index.html"),
};
for (const entry of fs.readdirSync(pages_directory, { withFileTypes: true })) {
  const page_html = path.join(pages_directory, entry.name, "index.html");
  if (entry.isDirectory() && fs.existsSync(page_html)) page_inputs[`page-${entry.name}`] = page_html;
}

export default defineConfig({
  // Multi-page app: a path with no page answers 404 instead of falling back to the main menu.
  appType: "mpa",
  // Type errors surface in dev (tsc --watch in the dev server: terminal +
  // browser overlay); the build itself doesn't type-check, so it stays fast.
  plugins: [tablet_server_plugin(), reload_on_rebuild_plugin(), checker({ typescript: true, enableBuild: false })],
  build: { rollupOptions: { input: page_inputs } },
});
