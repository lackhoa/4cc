// Document `skull-draw` (plan-sketchpad-landmarks.md): the sketchpad tied to the document
// `skull-zanatomy`, whose named vertices are the landmarks imported from the reference
// `.landmarks.txt` files (`tmp/import_landmarks_into_document.ts`). The skull drawing is
// done; the page now draws the skin over it (plan-skin-over-skull-study.md): the reference
// is the Z-Anatomy skin mesh in the skull's Frankfurt frame (mm * WORLD_PER_MM, same as
// the skull-* pages), the eyeballs behind a toggle, the skull layer locked.
// Separate localStorage keys so this page never steals the plain sketchpad's document.
import { ReferenceMesh, reference_mesh_from_positions } from "../../src/reference";
import { Skull, WORLD_PER_MM, load_eyeball, load_skin_mesh, load_skull } from "../../src/reference_skull_view";
import { start_sketchpad } from "../../src/sketchpad";
import { v3_scale } from "../../src/math";

const PAGE_NAME = "skull-draw";

function reference_mesh_from_skull_frame_mesh(mesh: Skull): ReferenceMesh {
  return reference_mesh_from_positions(mesh.positions.map((p) => v3_scale(p, WORLD_PER_MM)), mesh.triangle_indices);
}

// The skin and eyeball meshes have no landmarks of their own: the skull is loaded for
// its Frankfurt frame only (the frame every document vertex was drawn in).
async function load_skull_frame_mesh(load: (page_name: string, frame: Skull["frame"]) => Promise<Skull | null>): Promise<ReferenceMesh | null> {
  const skull = await load_skull(PAGE_NAME);
  if (skull === null) return null;
  const mesh = await load(PAGE_NAME, skull.frame);
  return mesh === null ? null : reference_mesh_from_skull_frame_mesh(mesh);
}

start_sketchpad({
  load_reference_mesh: () => load_skull_frame_mesh(load_skin_mesh),
  load_eyeball_mesh: () => load_skull_frame_mesh(load_eyeball),
  default_document_name: "skull-zanatomy",
  storage_key_prefix: "autodraw_skull_draw",
  can_switch_documents: false,
  // The skull is done: draw skin over it, skull strokes untouchable (Q4/Q10).
  active_layer: "skin",
  locked_layers: ["skull"],
});
