// One-off: compare the working-copy skull-zanatomy.json with HEAD's — only the
// v5 migration fields (bones, bone_id, layer, version) may differ; vertex
// positions, strokes, patches and pins must be identical.
import { execSync } from "node:child_process";
import { readFileSync } from "node:fs";

const head_file = JSON.parse(execSync("git show HEAD:tablet/documents/skull-zanatomy.json", { encoding: "utf8" }));
const work_file = JSON.parse(readFileSync("documents/skull-zanatomy.json", "utf8"));
console.log("version", head_file.version, "->", work_file.version);
// The camera is view state, free to differ.
const head = head_file.document;
const work = work_file.document;
console.log("vertices", head.vertices.length, "->", work.vertices.length);
console.log("strokes", head.strokes.length, "->", work.strokes.length);
console.log("patches", (head.patches ?? []).length, "->", (work.patches ?? []).length);

function strip(value: any): any {
  if (Array.isArray(value)) return value.map(strip);
  if (value && typeof value === "object") {
    const out: any = {};
    for (const [k, v] of Object.entries(value)) {
      if (k === "bones" || k === "bone_id" || k === "layer" || k === "version") continue;
      out[k] = strip(v);
    }
    return out;
  }
  return value;
}
const same = JSON.stringify(strip(head)) === JSON.stringify(strip(work));
console.log("identical apart from migration fields:", same);
if (!same) {
  const h = strip(head), w = strip(work);
  for (const key of Object.keys(w)) {
    if (JSON.stringify(h[key]) !== JSON.stringify(w[key])) console.log("  differs in", key);
  }
}
