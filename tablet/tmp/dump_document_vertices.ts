// Prints every vertex of a tablet document: id, name, layers, world position.
//   npx tsx tmp/dump_document_vertices.ts <document-name>
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { default_camera } from "../src/camera";
import { TabletDocument, empty_document, vertex_is_on_layers, vertex_world_position } from "../src/document";
import { apply_document_state } from "../src/persistence";

const tablet_directory = path.join(path.dirname(fileURLToPath(import.meta.url)), "..");
const document_name = process.argv[2];
if (document_name === undefined) throw new Error("usage: dump_document_vertices.ts <document-name>");

const tablet_document: TabletDocument = empty_document();
apply_document_state(fs.readFileSync(path.join(tablet_directory, `documents/${document_name}.json`), "utf8"), tablet_document, default_camera());

for (const vertex of tablet_document.vertices) {
  const p = vertex_world_position(tablet_document, vertex);
  const layers = [
    vertex_is_on_layers(tablet_document, vertex.id, new Set(["skull"] as const)) ? "skull" : "",
    vertex_is_on_layers(tablet_document, vertex.id, new Set(["skin"] as const)) ? "skin" : "",
  ].join(" ").trim();
  console.log(`${String(vertex.id).padStart(4)} ${(vertex.name ?? "").padEnd(20)} ${layers.padEnd(10)} ${p.x.toFixed(3).padStart(7)} ${p.y.toFixed(3).padStart(7)} ${p.z.toFixed(3).padStart(7)}`);
}
