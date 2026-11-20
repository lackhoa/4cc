// The drawing pages: the sketchpad (src/sketchpad.ts) opened on one document with one
// layer setup. One record per page; the page itself is the generic `/draw/?page=<page_name>`
// (draw/main.ts), and the front page (index.html, src/front_page_drawing_list.ts) lists
// every record. Adding a drawing page = adding a record here.
import { Layer } from "./document";
import { ReferenceMesh } from "./reference";
import { load_eyeball_mesh_in_skull_frame, load_skin_reference_mesh, load_skull_mandible_teeth_mesh } from "./drawing_page_meshes";

// Loads a mesh in world units; null = it could not be loaded. Called with the page name,
// which only labels console errors.
export type ReferenceMeshLoader = (page_name: string) => Promise<ReferenceMesh | null>;

export type DrawingPage = {
  page_name: string; // `/draw/?page=<page_name>`; also gives the page its localStorage keys
  document_name: string; // the one document the page opens (no switching)
  active_layer: Layer; // new strokes go here
  locked_layers: Layer[]; // shown, but never picked or drawn on
  hidden_layers: Layer[]; // neither drawn nor picked
  load_reference_mesh: ReferenceMeshLoader | null; // null = no mesh behind the drawing, no "ref" button
  load_eyeball_mesh: ReferenceMeshLoader | null; // non-null = the "eyes" button shows it with the reference
  shows_opacity_and_clip_sliders: boolean; // the reference opacity slider and the sagittal clip plane slider
  description_html: string; // the front page's line after the name (trusted HTML: `<code>` etc.)
};

export const DRAWING_PAGES: DrawingPage[] = [
  // Document `skull-zanatomy` (plan-sketchpad-landmarks.md): the sketchpad over the Z-Anatomy
  // skull + mandible + upper teeth in the Frankfurt frame, whose named vertices are the
  // landmarks imported from the reference `.landmarks.txt` files
  // (`tmp/import_landmarks_into_document.ts`).
  {
    page_name: "skull-draw",
    document_name: "skull-zanatomy",
    active_layer: "skull",
    locked_layers: [],
    // The skin drawing lives in the same document; it belongs to the skin-draw page.
    hidden_layers: ["skin"],
    load_reference_mesh: load_skull_mandible_teeth_mesh,
    load_eyeball_mesh: null,
    shows_opacity_and_clip_sliders: false,
    description_html: "the same drawing view over the Z-Anatomy skull + mandible + upper teeth (Frankfurt frame), document <code>skull-zanatomy</code> with the reference landmarks as named vertices.",
  },
  // Page `skin-draw` (plan-skin-over-skull-study.md): the sketchpad drawing the skin over
  // the finished skull. It opens the SAME document as skull-draw, `skull-zanatomy`: the skull
  // strokes + landmarks live on its `skull` layer (locked for good here), the skin goes on
  // the `skin` layer. The reference is the Z-Anatomy skin mesh in the skull's Frankfurt frame,
  // its mouth and eyebrow regions colored, the eyeballs behind a toggle.
  // Both pages save the whole file. They may be open together: a save based on an older
  // revision of the file is refused (`base_revision`, src/persistence.ts), and an idle page
  // takes over what the other one saved.
  {
    page_name: "skin-draw",
    document_name: "skull-zanatomy",
    // The skull is done: draw skin over it, skull strokes untouchable (Q4/Q10).
    active_layer: "skin",
    locked_layers: ["skull"],
    hidden_layers: [],
    load_reference_mesh: load_skin_reference_mesh,
    load_eyeball_mesh: load_eyeball_mesh_in_skull_frame,
    shows_opacity_and_clip_sliders: true,
    description_html: "the skin drawn over the finished skull: the same <code>skull-zanatomy</code> document as skull-draw (skull layer locked) over the Z-Anatomy skin mesh, with opacity slider, eyeballs and a sagittal clip plane.",
  },
  // Document `native-head`, the head drawing ported from the native C++ editor. The document
  // is stored one-sided and mirrored across the midline (`is_mirrored_across_midline`,
  // src/document.ts), so the sketchpad draws the other side itself. No reference mesh,
  // overlay or landmarks: just the drawing.
  {
    page_name: "native-head-draw",
    document_name: "native-head",
    // Every stroke of the ported head is on the `skin` layer; nothing is hidden or locked.
    active_layer: "skin",
    locked_layers: [],
    hidden_layers: [],
    load_reference_mesh: null,
    load_eyeball_mesh: null,
    shows_opacity_and_clip_sliders: false,
    description_html: "the same drawing view on document <code>native-head</code>: the head from the native editor, one side stored and the other mirrored.",
  },
];

// The record of `page_name`; null when there is none.
export function find_drawing_page(page_name: string): DrawingPage | null {
  return DRAWING_PAGES.find((page) => page.page_name === page_name) ?? null;
}

// `autodraw_<page name with - as _>`: each page keeps its own remembered document, camera,
// and crash buffer. These are the keys the pages had as their own folders
// (`autodraw_skull_draw`, `autodraw_skin_draw`, `autodraw_native_head_draw`).
export function drawing_page_storage_key_prefix(page: DrawingPage): string {
  return `autodraw_${page.page_name.replace(/-/g, "_")}`;
}

export function drawing_page_url(page: DrawingPage): string {
  return `/draw/?page=${encodeURIComponent(page.page_name)}`;
}

// `/draw/?document=<name>`: any document, no record (draw/main.ts).
export function document_link_url(document_name: string): string {
  return `/draw/?document=${encodeURIComponent(document_name)}`;
}
