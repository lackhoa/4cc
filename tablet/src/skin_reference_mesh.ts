// The Z-Anatomy skin mesh as a reference mesh, with the skin regions that are worth
// telling apart colored.
import { REFERENCE_COLOR, ReferenceMesh, fetch_reference_text, frankfurt_coordinates, parse_obj_mesh_raw, reference_mesh_from_positions } from "./reference";
import { SKIN_NAME, WORLD_PER_MM, load_skull } from "./reference_skull_view";
import { Rgb } from "./vertex_sink";
import { v3_scale } from "./math";

// The skin .obj names Z-Anatomy's skin regions on its `o` lines ("Oral region.l", ...).
// The regions that locate the mouth and the eyebrow get a color of their own, left and
// right alike; the rest of the skin stays REFERENCE_COLOR. The scan has no region for the
// red of the lips.
const skin_region_colors: Record<string, Rgb> = {
  "Oral region": { r: 0.62, g: 0.42, b: 0.4 }, // both lips and the skin around them
  "Labial commissure": { r: 0.35, g: 0.12, b: 0.14 }, // the line where the lips meet
  "Angle of mouth": { r: 0.85, g: 0.55, b: 0.15 }, // the two corners
  "Philtrum": { r: 0.35, g: 0.55, b: 0.6 }, // the groove under the nose
  "Tubercle of upper lip": { r: 0.8, g: 0.3, b: 0.35 }, // the bump at the upper lip's middle
  "Mentolabial sulcus": { r: 0.45, g: 0.4, b: 0.65 }, // the groove between lower lip and chin
  "Nasolabial sulcus": { r: 0.35, g: 0.5, b: 0.4 }, // the fold from the nose wing past the mouth corner
  "Eyebrow": { r: 0.3, g: 0.2, b: 0.14 }, // a coarse patch (52 triangles a side): where the brow is, not its outline
};

// The head's skin (the same scan as the skull, so it sits on it without alignment) in the
// skull's Frankfurt frame, world units (plan-skin-over-skull-study.md Q5). `page_name`
// goes into the error messages.
export async function load_skin_reference_mesh(page_name: string): Promise<ReferenceMesh | null> {
  const [skull, obj_text] = await Promise.all([load_skull(page_name), fetch_reference_text(`/reference/${SKIN_NAME}.obj`)]);
  if (skull === null || obj_text === null) return null;
  const raw = parse_obj_mesh_raw(obj_text);
  if (raw === null) {
    console.error(`${page_name}: ${SKIN_NAME} mesh unreadable`);
    return null;
  }
  const positions_world = raw.positions.map((p) => v3_scale(frankfurt_coordinates(skull.frame, p), WORLD_PER_MM));
  const triangle_colors = raw.triangle_object_names.map((name) => skin_region_colors[name.replace(/\.[lr]$/, "")] ?? REFERENCE_COLOR);
  return reference_mesh_from_positions(positions_world, raw.triangle_indices, triangle_colors);
}
