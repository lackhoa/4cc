// Dev-server plugin: serves reference models from Google Drive and persists
// tablet documents as git-tracked JSON in tablet/documents/ (plan-skull-reference.md).
//   GET  /reference/<file>        -> ~/personal-drive/autodraw/reference-models/<file>
//   GET  /api/documents           -> [{ name, mtime_ms }]
//   GET  /api/documents/<name>    -> stored JSON
//   POST /api/documents/<name>    -> write body to documents/<name>.json

import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { IncomingMessage, ServerResponse } from "node:http";
import { Plugin, defineConfig } from "vite";

const documents_directory = path.join(path.dirname(fileURLToPath(import.meta.url)), "documents");
const reference_models_directory = path.join(os.homedir(), "personal-drive/autodraw/reference-models");

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

function tablet_api_middleware(request: IncomingMessage, response: ServerResponse, next: () => void): void {
  const url = request.url ?? "";
  if (url.startsWith("/reference/") && request.method === "GET") {
    handle_reference_request(request, response);
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

export default defineConfig({
  plugins: [tablet_server_plugin()],
});
