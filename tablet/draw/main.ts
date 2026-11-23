// The sketchpad page (`/draw/`), three ways:
//   /draw/                                  the plain sketchpad: the height-normalized `skull.obj`
//                                           behind the drawing, documents switchable, the
//                                           `autodraw_tablet_*` localStorage keys.
//   /draw/?page=<page name>                 a drawing page of src/drawing_pages.ts: its document,
//                                           layers and reference mesh.
//   /draw/?document=<name>[&layer=<layer>]  any document without a record: nothing hidden or
//                                           locked, no mesh; without `layer`, new strokes go to
//                                           the first layer that has strokes.
// The URL is parsed here and only here; a bad one says so on the page instead of drawing.
import { ALL_LAYERS, Layer } from "../src/document";
import { DrawingPage, drawing_page_storage_key_prefix, find_drawing_page } from "../src/drawing_pages";
import { fetch_reference_mesh } from "../src/reference";
import { LineDepthMode } from "../src/render";
import { start_sketchpad } from "../src/sketchpad";

const DOCUMENT_NAME_PATTERN = /^[A-Za-z0-9_-]{1,64}$/; // mirrors is_safe_document_name in vite.config.ts

type DrawUrlRequest =
  | { kind: "plain_sketchpad" }
  | { kind: "drawing_page"; page: DrawingPage }
  | { kind: "document_link"; document_name: string; active_layer: Layer | "first_layer_with_strokes" }
  | { kind: "bad_url"; message: string };

function parse_layer(text: string): Layer | null {
  return ALL_LAYERS.find((layer) => layer === text) ?? null;
}

function parse_draw_url(search: string): DrawUrlRequest {
  const parameters = new URLSearchParams(search);
  const page_name = parameters.get("page");
  const document_name = parameters.get("document");
  const layer_text = parameters.get("layer");
  if (page_name !== null) {
    if (document_name !== null || layer_text !== null) {
      return { kind: "bad_url", message: "Give either ?page=<page name> or ?document=<name>[&layer=<layer>], not both." };
    }
    const page = find_drawing_page(page_name);
    if (page === null) return { kind: "bad_url", message: `No drawing page named '${page_name}' (src/drawing_pages.ts).` };
    return { kind: "drawing_page", page };
  }
  if (document_name !== null) {
    if (!DOCUMENT_NAME_PATTERN.test(document_name)) {
      return { kind: "bad_url", message: `Bad document name '${document_name}': letters, digits, - and _ only.` };
    }
    if (layer_text === null) return { kind: "document_link", document_name, active_layer: "first_layer_with_strokes" };
    const layer = parse_layer(layer_text);
    if (layer === null) return { kind: "bad_url", message: `No layer named '${layer_text}'; the layers are ${ALL_LAYERS.join(", ")}.` };
    return { kind: "document_link", document_name, active_layer: layer };
  }
  if (layer_text !== null) return { kind: "bad_url", message: "?layer= needs a ?document=<name> to go with it." };
  return { kind: "plain_sketchpad" };
}

// TEMPORARY (plan-lines-not-hidden-by-nearby-patch.md step 2): `?line_depth=fixed` or
// `?line_depth=tilt` tries a way to keep lines on a patch out of it in the depth test; absent
// or anything else draws as today. Works with all three forms of the URL above.
function parse_line_depth_mode(search: string): LineDepthMode {
  const line_depth_text = new URLSearchParams(search).get("line_depth");
  if (line_depth_text === "fixed") return "fixed_line_offset";
  if (line_depth_text === "tilt") return "tilt_scaled_surface_push";
  return "as_today";
}

function show_element(id: string): void {
  document.getElementById(id)!.hidden = false;
}

function hide_element(id: string): void {
  document.getElementById(id)!.hidden = true;
}

function start_plain_sketchpad(line_depth_mode: LineDepthMode): void {
  start_sketchpad({
    line_depth_mode,
    load_reference_mesh: () => fetch_reference_mesh("/reference/skull.obj"),
    default_document_name: "untitled",
    storage_key_prefix: "autodraw_tablet",
    can_switch_documents: true,
    active_layer: "skull",
    locked_layers: [],
    hidden_layers: [],
  });
}

function start_drawing_page(page: DrawingPage, line_depth_mode: LineDepthMode): void {
  document.title = page.page_name;
  // Without a mesh the "ref" button has nothing to toggle; sketchpad.ts still wires it, so it stays, hidden.
  if (page.load_reference_mesh === null) hide_element("reference_button");
  if (page.load_eyeball_mesh !== null) show_element("eyeball_button");
  if (page.shows_opacity_and_clip_sliders) {
    show_element("reference_alpha");
    show_element("clip_offset");
  }
  const load_reference_mesh = page.load_reference_mesh;
  const load_eyeball_mesh = page.load_eyeball_mesh;
  start_sketchpad({
    line_depth_mode,
    load_reference_mesh: async () => {
      if (load_reference_mesh === null) return null;
      return load_reference_mesh(page.page_name);
    },
    load_eyeball_mesh: load_eyeball_mesh === null ? undefined : () => load_eyeball_mesh(page.page_name),
    default_document_name: page.document_name,
    storage_key_prefix: drawing_page_storage_key_prefix(page),
    can_switch_documents: false,
    active_layer: page.active_layer,
    locked_layers: page.locked_layers,
    hidden_layers: page.hidden_layers,
  });
}

// Its own localStorage keys per document: the remembered current document of a prefix wins
// over the default name (src/persistence.ts), so a shared prefix would open the last
// document linked instead of this one.
function start_document_link(document_name: string, active_layer: Layer | "first_layer_with_strokes", line_depth_mode: LineDepthMode): void {
  document.title = document_name;
  hide_element("reference_button");
  start_sketchpad({
    line_depth_mode,
    load_reference_mesh: async () => null,
    default_document_name: document_name,
    storage_key_prefix: `autodraw_document_${document_name}`,
    can_switch_documents: false,
    active_layer,
    locked_layers: [],
    hidden_layers: [],
  });
}

// Instead of the sketchpad: the message and a way back to the menu.
function show_bad_url(message: string): void {
  document.title = "bad drawing link";
  const box = document.createElement("div");
  box.style.cssText = "padding: 24px; color: #e6e6ef; font: 16px/1.5 system-ui, sans-serif;";
  const text = document.createElement("p");
  text.textContent = message;
  const menu_link = document.createElement("a");
  menu_link.href = "/";
  menu_link.textContent = "back to the menu";
  menu_link.style.color = "#7ab8ff";
  box.append(text, menu_link);
  document.body.replaceChildren(box);
}

const request = parse_draw_url(window.location.search);
const line_depth_mode = parse_line_depth_mode(window.location.search);
if (request.kind === "plain_sketchpad") start_plain_sketchpad(line_depth_mode);
if (request.kind === "drawing_page") start_drawing_page(request.page, line_depth_mode);
if (request.kind === "document_link") start_document_link(request.document_name, request.active_layer, line_depth_mode);
if (request.kind === "bad_url") show_bad_url(request.message);
