// Reference meshes the drawing pages (src/drawing_pages.ts) put behind the drawing, in
// the skull's Frankfurt frame (mm * WORLD_PER_MM, same as the skull-* pages). Each takes
// the page name, which only labels the console errors of a missing mesh.
import { ReferenceMesh, reference_mesh_from_positions } from "./reference";
import { Skull, WORLD_PER_MM, load_eyeball, load_mandible, load_skull, load_teeth_upper } from "./reference_skull_view";
import { v3_scale } from "./math";

export { load_skin_reference_mesh } from "./skin_reference_mesh";

function reference_mesh_from_skull_frame_mesh(mesh: Skull): ReferenceMesh {
  return reference_mesh_from_positions(mesh.positions.map((p) => v3_scale(p, WORLD_PER_MM)), mesh.triangle_indices);
}

// Skull + mandible + upper teeth (the skull mesh stops at the alveolar edge, and the
// mandible's own teeth come with it). A missing extra mesh is just left out.
export async function load_skull_mandible_teeth_mesh(page_name: string): Promise<ReferenceMesh | null> {
  const skull = await load_skull(page_name);
  if (skull === null) return null;
  const [mandible, teeth_upper] = await Promise.all([load_mandible(page_name, skull.frame), load_teeth_upper(page_name, skull.frame)]);
  const meshes = [skull, mandible, teeth_upper]
    .filter((mesh): mesh is Skull => mesh !== null)
    .map(reference_mesh_from_skull_frame_mesh);
  return {
    triangle_positions: meshes.flatMap((mesh) => mesh.triangle_positions),
    triangle_normals: meshes.flatMap((mesh) => mesh.triangle_normals),
    triangle_colors: meshes.flatMap((mesh) => mesh.triangle_colors),
  };
}

// The eyeball mesh has no landmarks of its own: the skull is loaded for
// its Frankfurt frame only (the frame every document vertex was drawn in).
export async function load_eyeball_mesh_in_skull_frame(page_name: string): Promise<ReferenceMesh | null> {
  const skull = await load_skull(page_name);
  if (skull === null) return null;
  const eyeball = await load_eyeball(page_name, skull.frame);
  if (eyeball === null) return null;
  return reference_mesh_from_skull_frame_mesh(eyeball);
}
