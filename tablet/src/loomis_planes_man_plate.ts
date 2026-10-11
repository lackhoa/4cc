// The Loomis planes-head plates (a man, front and profile, two separate scans) as a
// tracing guide behind the template head (document `skull-zanatomy`): the page
// fit-hairline-to-loomis-man draws the hairline on them and lifts it onto the skin
// (notes/tasks/autodraw_draw_as_data/plan-hairline-reference-plate.md).
//
// The head is not deformed. Each plate is registered to the head on its own, by
// landmarks Khoa clicks on the plate against the head's named vertices: `brow` and
// `nose_base` give the scale and the vertical offset in both views; the midline column
// gives the horizontal offset in front, the ear hole (`porion_r`) in profile. Both views
// are orthographic; the profile faces left, so world z grows to the viewer's left, as on
// the girl plate (src/loomis_girl_plate.ts).
import { TabletDocument, vertex_world_position } from "./document";
import { V2, V3, v3 } from "./math";

export const LOOMIS_PLANES_MAN_LANDMARKS_NAME = "loomis-planes-man";

export type PlateImage = { url: string; width_pixels: number; height_pixels: number };
export const FRONT_PLATE: PlateImage = { url: "/loomis-planes-man-front.webp", width_pixels: 414, height_pixels: 529 };
export const PROFILE_PLATE: PlateImage = { url: "/loomis-planes-man-profile.webp", width_pixels: 561, height_pixels: 670 };

// The named vertices of the template head the landmarks are matched to.
export const BROW_VERTEX_NAME = "brow";
export const NOSE_BASE_VERTEX_NAME = "nose_base";
export const PORION_VERTEX_NAME = "porion_r";

// The marks on one plate, in image pixels. `porion` is the ear hole, used in profile
// only; `midline_column` is the sagittal plane, used in front only.
export type PlateLandmarks = { brow: V2; nose_base: V2; porion: V2; midline_column: number };

// The file tablet/fit-landmarks/loomis-planes-man.json (server route
// /api/fit-landmarks/<name>). The hairlines are polylines drawn on the plates, image
// pixels; lifted_stroke_ids are the document strokes the last lift wrote, so the next
// lift replaces them.
export type LoomisPlanesManLandmarksFile = {
  version: 1;
  front: PlateLandmarks;
  profile: PlateLandmarks;
  front_hairline: V2[];
  profile_hairline: V2[];
  lifted_stroke_ids: number[];
};

// Rough marks read off the scans by eye (2026-10-11): what the page shows while the server
// has no file. Khoa drags them into place.
export function starting_landmarks_file(): LoomisPlanesManLandmarksFile {
  return {
    version: 1,
    front: { brow: { x: 207, y: 255 }, nose_base: { x: 207, y: 370 }, porion: { x: 380, y: 300 }, midline_column: 207 },
    profile: { brow: { x: 120, y: 210 }, nose_base: { x: 60, y: 365 }, porion: { x: 340, y: 290 }, midline_column: 0 },
    front_hairline: [],
    profile_hairline: [],
    lifted_stroke_ids: [],
  };
}

// ---- registration -------------------------------------------------------------------

// World -> image pixel of one view, a scale and an offset. Front: pixel x grows with
// world x, profile: pixel x grows with -z. Pixel y grows with -y in both.
export type PlateRegistration = { pixels_per_world_unit: number; pixel_of_world_origin: V2; is_front_view: boolean };

export function named_vertex_world_position(tablet_document: TabletDocument, name: string): V3 | null {
  const vertex = tablet_document.vertices.find((v) => v.name === name);
  return vertex === undefined ? null : vertex_world_position(tablet_document, vertex);
}

// Null while the document has no brow / nose base / ear hole (not loaded yet).
export function plate_registration(tablet_document: TabletDocument, landmarks: PlateLandmarks, is_front_view: boolean): PlateRegistration | null {
  const brow = named_vertex_world_position(tablet_document, BROW_VERTEX_NAME);
  const nose_base = named_vertex_world_position(tablet_document, NOSE_BASE_VERTEX_NAME);
  const porion = named_vertex_world_position(tablet_document, PORION_VERTEX_NAME);
  if (brow === null || nose_base === null || porion === null) return null;
  const world_height = brow.y - nose_base.y;
  const pixel_height = landmarks.nose_base.y - landmarks.brow.y;
  if (Math.abs(world_height) < 1e-6 || Math.abs(pixel_height) < 1e-6) return null;
  const scale = pixel_height / world_height;
  const origin_row = landmarks.brow.y + brow.y * scale;
  const origin_column = is_front_view ? landmarks.midline_column : landmarks.porion.x + porion.z * scale;
  return { pixels_per_world_unit: scale, pixel_of_world_origin: { x: origin_column, y: origin_row }, is_front_view };
}

export function pixel_from_world(registration: PlateRegistration, p: V3): V2 {
  const across = registration.is_front_view ? p.x : -p.z;
  return {
    x: registration.pixel_of_world_origin.x + across * registration.pixels_per_world_unit,
    y: registration.pixel_of_world_origin.y - p.y * registration.pixels_per_world_unit,
  };
}

// The world point the pixel shows, on the view's own plane (z = 0 in front, x = 0 in profile).
export function world_from_pixel(registration: PlateRegistration, pixel: V2): V3 {
  const across = (pixel.x - registration.pixel_of_world_origin.x) / registration.pixels_per_world_unit;
  const y = (registration.pixel_of_world_origin.y - pixel.y) / registration.pixels_per_world_unit;
  return registration.is_front_view ? v3(across, y, 0) : v3(0, y, -across);
}

// ---- file on the server -------------------------------------------------------------

// Null when the request failed; the starting file when the server has none.
export async function fetch_landmarks_file(): Promise<LoomisPlanesManLandmarksFile | null> {
  try {
    const response = await fetch(`/api/fit-landmarks/${LOOMIS_PLANES_MAN_LANDMARKS_NAME}`, { cache: "no-store" });
    if (response.status === 404) return starting_landmarks_file();
    if (!response.ok) {
      console.error(`loading landmarks '${LOOMIS_PLANES_MAN_LANDMARKS_NAME}' failed: status ${response.status}`);
      return null;
    }
    return (await response.json()) as LoomisPlanesManLandmarksFile;
  } catch (error) {
    console.error(`loading landmarks '${LOOMIS_PLANES_MAN_LANDMARKS_NAME}' failed (server unreachable)`, error);
    return null;
  }
}

// True when the server stored the file.
export async function save_landmarks_file(file: LoomisPlanesManLandmarksFile): Promise<boolean> {
  try {
    const response = await fetch(`/api/fit-landmarks/${LOOMIS_PLANES_MAN_LANDMARKS_NAME}`, { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify(file, null, 1) });
    if (!response.ok) console.error(`saving landmarks '${LOOMIS_PLANES_MAN_LANDMARKS_NAME}' failed: status ${response.status}`);
    return response.ok;
  } catch (error) {
    console.error(`saving landmarks '${LOOMIS_PLANES_MAN_LANDMARKS_NAME}' failed (server unreachable)`, error);
    return false;
  }
}
