// Document `skull-draw` (plan-sketchpad-landmarks.md): the sketchpad over the Z-Anatomy
// skull + mandible + upper teeth in the Frankfurt frame (mm * WORLD_PER_MM, same as the skull-* pages),
// tied to the document `skull-zanatomy`, whose named vertices are the landmarks imported
// from the reference `.landmarks.txt` files (`tmp/import_landmarks_into_document.ts`).
// Separate localStorage keys so this page never steals the plain sketchpad's document.
import { ReferenceMesh, reference_mesh_from_positions } from "../../src/reference";
import { Skull, WORLD_PER_MM, load_mandible, load_skull, load_teeth_upper } from "../../src/reference_skull_view";
import { start_sketchpad } from "../../src/sketchpad";
import { v3_scale } from "../../src/math";

const PAGE_NAME = "skull-draw";

// Skull + mandible + upper teeth (the skull mesh stops at the alveolar edge, and the
// mandible's own teeth come with it). A missing extra mesh is just left out.
async function load_skull_mandible_teeth_mesh(): Promise<ReferenceMesh | null> {
  const skull = await load_skull(PAGE_NAME);
  if (skull === null) return null;
  const [mandible, teeth_upper] = await Promise.all([load_mandible(PAGE_NAME, skull.frame), load_teeth_upper(PAGE_NAME, skull.frame)]);
  const meshes = [skull, mandible, teeth_upper]
    .filter((mesh): mesh is Skull => mesh !== null)
    .map((mesh) => reference_mesh_from_positions(mesh.positions.map((p) => v3_scale(p, WORLD_PER_MM)), mesh.triangle_indices));
  return {
    triangle_positions: meshes.flatMap((mesh) => mesh.triangle_positions),
    triangle_normals: meshes.flatMap((mesh) => mesh.triangle_normals),
  };
}

start_sketchpad({
  load_reference_mesh: load_skull_mandible_teeth_mesh,
  default_document_name: "skull-zanatomy",
  storage_key_prefix: "autodraw_skull_draw",
  can_switch_documents: false,
});
