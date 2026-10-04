// Page `skin-draw` (plan-skin-over-skull-study.md): the sketchpad drawing the skin over
// the finished skull. It opens the SAME document as skull-draw, `skull-zanatomy`: the skull
// strokes + landmarks live on its `skull` layer (locked for good here), the skin goes on
// the `skin` layer. The reference is the Z-Anatomy skin mesh in the skull's Frankfurt frame
// (mm * WORLD_PER_MM, same as the skull-* pages), its mouth regions colored, the eyeballs
// behind a toggle.
// Separate localStorage keys so this page never steals another page's camera/tool state.
// Both pages save the whole file. They may be open together: a save based on an older
// revision of the file is refused (`base_revision`, src/persistence.ts), and an idle page
// takes over what the other one saved.
import { REFERENCE_COLOR, ReferenceMesh, fetch_reference_text, frankfurt_coordinates, parse_obj_mesh_raw, reference_mesh_from_positions } from "../../src/reference";
import { SKIN_NAME, Skull, WORLD_PER_MM, load_eyeball, load_skull } from "../../src/reference_skull_view";
import { Rgb } from "../../src/vertex_sink";
import { start_sketchpad } from "../../src/sketchpad";
import { v3_scale } from "../../src/math";

const PAGE_NAME = "skin-draw";

function reference_mesh_from_skull_frame_mesh(mesh: Skull): ReferenceMesh {
  return reference_mesh_from_positions(mesh.positions.map((p) => v3_scale(p, WORLD_PER_MM)), mesh.triangle_indices);
}

// The eyeball mesh has no landmarks of its own: the skull is loaded for
// its Frankfurt frame only (the frame every document vertex was drawn in).
async function load_skull_frame_mesh(load: (page_name: string, frame: Skull["frame"]) => Promise<Skull | null>): Promise<ReferenceMesh | null> {
  const skull = await load_skull(PAGE_NAME);
  if (skull === null) return null;
  const mesh = await load(PAGE_NAME, skull.frame);
  return mesh === null ? null : reference_mesh_from_skull_frame_mesh(mesh);
}

// The skin .obj names Z-Anatomy's skin regions on its `o` lines ("Oral region.l", ...).
// The regions that locate the mouth get a color of their own, left and right alike; the
// rest of the skin stays REFERENCE_COLOR. The scan has no region for the red of the lips.
const mouth_region_colors: Record<string, Rgb> = {
  "Oral region": { r: 0.62, g: 0.42, b: 0.4 }, // both lips and the skin around them
  "Labial commissure": { r: 0.35, g: 0.12, b: 0.14 }, // the line where the lips meet
  "Angle of mouth": { r: 0.85, g: 0.55, b: 0.15 }, // the two corners
  "Philtrum": { r: 0.35, g: 0.55, b: 0.6 }, // the groove under the nose
  "Tubercle of upper lip": { r: 0.8, g: 0.3, b: 0.35 }, // the bump at the upper lip's middle
  "Mentolabial sulcus": { r: 0.45, g: 0.4, b: 0.65 }, // the groove between lower lip and chin
  "Nasolabial sulcus": { r: 0.35, g: 0.5, b: 0.4 }, // the fold from the nose wing past the mouth corner
};

// The head's skin (the same scan as the skull, so it sits on it without alignment) in the
// skull's Frankfurt frame, world units (plan-skin-over-skull-study.md Q5).
async function load_skin_reference_mesh(): Promise<ReferenceMesh | null> {
  const [skull, obj_text] = await Promise.all([load_skull(PAGE_NAME), fetch_reference_text(`/reference/${SKIN_NAME}.obj`)]);
  if (skull === null || obj_text === null) return null;
  const raw = parse_obj_mesh_raw(obj_text);
  if (raw === null) {
    console.error(`${PAGE_NAME}: ${SKIN_NAME} mesh unreadable`);
    return null;
  }
  const positions_world = raw.positions.map((p) => v3_scale(frankfurt_coordinates(skull.frame, p), WORLD_PER_MM));
  const triangle_colors = raw.triangle_object_names.map((name) => mouth_region_colors[name.replace(/\.[lr]$/, "")] ?? REFERENCE_COLOR);
  return reference_mesh_from_positions(positions_world, raw.triangle_indices, triangle_colors);
}

start_sketchpad({
  load_reference_mesh: load_skin_reference_mesh,
  load_eyeball_mesh: () => load_skull_frame_mesh(load_eyeball),
  default_document_name: "skull-zanatomy",
  storage_key_prefix: "autodraw_skin_draw",
  can_switch_documents: false,
  // The skull is done: draw skin over it, skull strokes untouchable (Q4/Q10).
  active_layer: "skin",
  locked_layers: ["skull"],
  hidden_layers: [],
});
