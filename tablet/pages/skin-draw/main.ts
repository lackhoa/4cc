// Page `skin-draw` (plan-skin-over-skull-study.md): the sketchpad drawing the skin over
// the finished skull. It opens the SAME document as skull-draw, `skull-zanatomy`: the skull
// strokes + landmarks live on its `skull` layer (locked for good here), the skin goes on
// the `skin` layer. The reference is the Z-Anatomy skin mesh in the skull's Frankfurt frame
// (mm * WORLD_PER_MM, same as the skull-* pages), its mouth and eyebrow regions colored, the eyeballs
// behind a toggle.
// Separate localStorage keys so this page never steals another page's camera/tool state.
// Both pages save the whole file. They may be open together: a save based on an older
// revision of the file is refused (`base_revision`, src/persistence.ts), and an idle page
// takes over what the other one saved.
import { ReferenceMesh, reference_mesh_from_positions } from "../../src/reference";
import { Skull, WORLD_PER_MM, load_eyeball, load_skull } from "../../src/reference_skull_view";
import { load_skin_reference_mesh } from "../../src/skin_reference_mesh";
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

start_sketchpad({
  load_reference_mesh: () => load_skin_reference_mesh(PAGE_NAME),
  load_eyeball_mesh: () => load_skull_frame_mesh(load_eyeball),
  default_document_name: "skull-zanatomy",
  storage_key_prefix: "autodraw_skin_draw",
  can_switch_documents: false,
  // The skull is done: draw skin over it, skull strokes untouchable (Q4/Q10).
  active_layer: "skin",
  locked_layers: ["skull"],
  hidden_layers: [],
});
