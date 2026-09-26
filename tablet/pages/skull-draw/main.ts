// Document `skull-draw` (plan-sketchpad-landmarks.md): the sketchpad over the Z-Anatomy
// skull + mandible in the Frankfurt frame (mm * WORLD_PER_MM, same as the skull-* pages),
// tied to the document `skull-zanatomy`, whose named vertices are the landmarks imported
// from the reference `.landmarks.txt` files (`tmp/import_landmarks_into_document.ts`).
// Separate localStorage keys so this page never steals the plain sketchpad's document.
import { ReferenceMesh, reference_mesh_from_positions } from "../../src/reference";
import { WORLD_PER_MM, load_mandible, load_skull } from "../../src/reference_skull_view";
import { start_sketchpad } from "../../src/sketchpad";
import { v3_scale } from "../../src/math";

const PAGE_NAME = "skull-draw";

async function load_skull_and_mandible_mesh(): Promise<ReferenceMesh | null> {
  const skull = await load_skull(PAGE_NAME);
  if (skull === null) return null;
  const mandible = await load_mandible(PAGE_NAME, skull.frame);
  const meshes = (mandible === null ? [skull] : [skull, mandible]) // a missing mandible is just left out
    .map((mesh) => reference_mesh_from_positions(mesh.positions.map((p) => v3_scale(p, WORLD_PER_MM)), mesh.triangle_indices));
  return {
    triangle_positions: meshes.flatMap((mesh) => mesh.triangle_positions),
    triangle_normals: meshes.flatMap((mesh) => mesh.triangle_normals),
  };
}

start_sketchpad({
  load_reference_mesh: load_skull_and_mandible_mesh,
  default_document_name: "skull-zanatomy",
  storage_key_prefix: "autodraw_skull_draw",
  can_switch_documents: false,
});
