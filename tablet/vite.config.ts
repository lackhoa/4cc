// Dev-server plugin: serves the git-tracked reference models (data/reference-models/,
// copies of the Google Drive originals -- the C++ driver still reads those) and persists
// tablet documents as git-tracked JSON in tablet/documents/ (plan-skull-reference.md).
//   GET  /reference/<file>        -> ../data/reference-models/<file>
//   POST /reference/<mesh>.landmarks.txt -> overwrite that sidecar with the body
//   GET  /api/documents           -> [{ name, mtime_ms }]
//   GET  /api/documents/<name>    -> stored JSON
//   POST /api/documents/<name>    -> write body to documents/<name>.json

import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { IncomingMessage, ServerResponse } from "node:http";
import { Plugin, defineConfig } from "vite";
import checker from "vite-plugin-checker";

const documents_directory = path.join(path.dirname(fileURLToPath(import.meta.url)), "documents");
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

function handle_document_list(response: ServerResponse): void {
  if (!fs.existsSync(documents_directory)) fs.mkdirSync(documents_directory);
  const entries = fs.readdirSync(documents_directory)
    .filter((file_name) => file_name.endsWith(".json"))
    .map((file_name) => ({
      name: file_name.slice(0, -".json".length),
      mtime_ms: fs.statSync(path.join(documents_directory, file_name)).mtimeMs,
    }));
  send_json(response, 200, entries);
}

function handle_document_load(name: string, response: ServerResponse): void {
  const file_path = path.join(documents_directory, `${name}.json`);
  if (!fs.existsSync(file_path)) {
    send_json(response, 404, { error: `no document ${name}` });
    return;
  }
  response.setHeader("Content-Type", "application/json");
  response.end(fs.readFileSync(file_path));
}

function handle_document_save(name: string, request: IncomingMessage, response: ServerResponse): void {
  const chunks: Buffer[] = [];
  request.on("data", (chunk: Buffer) => chunks.push(chunk));
  request.on("end", () => {
    const body = Buffer.concat(chunks).toString("utf8");
    try {
      JSON.parse(body); // refuse to persist a corrupt payload
    } catch {
      send_json(response, 400, { error: "body is not valid JSON" });
      return;
    }
    if (!fs.existsSync(documents_directory)) fs.mkdirSync(documents_directory);
    const file_path = path.join(documents_directory, `${name}.json`);
    fs.writeFileSync(file_path, body);
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
// button so a drawing stroke is never cut. Dev already hot-reloads and is left alone.
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
              if (reload_is_pending && !pointer_is_down) location.reload();
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
  // Type errors surface in dev (tsc --watch in the dev server: terminal +
  // browser overlay); the build itself doesn't type-check, so it stays fast.
  plugins: [tablet_server_plugin(), reload_on_rebuild_plugin(), checker({ typescript: true, enableBuild: false })],
  build: { rollupOptions: { input: page_inputs } },
});
