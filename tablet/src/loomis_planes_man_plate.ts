// The Loomis planes-head plates (a man, front and profile, two separate scans) as a
// tracing guide behind the template head (document `skull-zanatomy`): the page
// fit-hairline-to-loomis-man draws the hairline on them and lifts it onto the skin
// (notes/tasks/autodraw_draw_as_data/plan-hairline-reference-plate.md).
//
// The head is not deformed. Each plate is registered to the head on its own, by
// landmarks Khoa marks on the plate against points of the head: one scale and one
// offset per view, least squares over all the landmarks marked in that view, both axes
// each. The landmarks are a free list (added, removed and renamed on the page); the
// starting ones are the brow, nasion and nose base in both views and the ear hole
// (`porion_r`) in profile. Both views are orthographic; the profile faces left, so world
// z grows to the viewer's left, as on the girl plate (src/loomis_girl_plate.ts).
import { TabletDocument } from "./document";
import { TemplatePoint, template_point_world_position } from "./loomis_girl_plate";
import { V2, V3, v3 } from "./math";

export const LOOMIS_PLANES_MAN_LANDMARKS_NAME = "loomis-planes-man";

export type PlateImage = { url: string; width_pixels: number; height_pixels: number };
export const FRONT_PLATE: PlateImage = { url: "/loomis-planes-man-front.webp", width_pixels: 414, height_pixels: 529 };
export const PROFILE_PLATE: PlateImage = { url: "/loomis-planes-man-profile.webp", width_pixels: 561, height_pixels: 670 };

// One landmark: a point of the head and where it is on each plate, in image pixels
// (null = not marked in that view, so the view's registration ignores it).
export type PlateLandmark = { name: string; template_point: TemplatePoint; front: V2 | null; profile: V2 | null };

// The file tablet/fit-landmarks/loomis-planes-man.json (server route
// /api/fit-landmarks/<name>). The hairlines are polylines drawn on the plates, image
// pixels; lifted_stroke_ids are the document strokes the last lift wrote, so the next
// lift replaces them.
export type LoomisPlanesManLandmarksFile = {
  version: 2;
  landmarks: PlateLandmark[];
  front_hairline: V2[];
  profile_hairline: V2[];
  lifted_stroke_ids: number[];
};

// A landmark given by the head vertex's name instead of a template point: the starting
// landmarks (read off the scans by eye, 2026-10-11, for Khoa to drag into place), and
// what a version 1 file converts to. Names resolve to vertex ids once the document is
// loaded (`landmarks_from_named_marks`).
export type NamedMark = { name: string; vertex_name: string; front: V2 | null; profile: V2 | null };

export const STARTING_MARKS: readonly NamedMark[] = [
  { name: "brow", vertex_name: "brow", front: { x: 207, y: 255 }, profile: { x: 120, y: 210 } },
  { name: "nasion", vertex_name: "nasion", front: { x: 207, y: 275 }, profile: { x: 112, y: 232 } },
  { name: "nose base", vertex_name: "nose_base", front: { x: 207, y: 370 }, profile: { x: 60, y: 365 } },
  { name: "ear hole", vertex_name: "porion_r", front: null, profile: { x: 340, y: 290 } },
];

export function starting_landmarks_file(): LoomisPlanesManLandmarksFile {
  return { version: 2, landmarks: [], front_hairline: [], profile_hairline: [], lifted_stroke_ids: [] };
}

// Null when the document has none of the named vertices (not loaded yet); a mark whose
// vertex the document lacks is dropped with a console error.
export function landmarks_from_named_marks(tablet_document: TabletDocument, marks: readonly NamedMark[]): PlateLandmark[] | null {
  const landmarks: PlateLandmark[] = [];
  for (const mark of marks) {
    const vertex = tablet_document.vertices.find((v) => v.name === mark.vertex_name);
    if (vertex === undefined) continue;
    landmarks.push({ name: mark.name, template_point: { vertex: vertex.id }, front: mark.front, profile: mark.profile });
  }
  if (landmarks.length === 0) return null;
  for (const mark of marks) {
    if (!landmarks.some((landmark) => landmark.name === mark.name)) console.error(`landmark '${mark.name}': the head has no vertex named '${mark.vertex_name}', dropped`);
  }
  return landmarks;
}

// Version 1 files (before 2026-10-11) held five fixed marks per view. The midline column
// is dropped: the brow, nasion and nose base sit on the midline, so their front columns
// already fix it.
type Version1Marks = { brow: V2; nasion?: V2; nose_base: V2; porion: V2; midline_column: number };
type Version1File = { version: 1; front: Version1Marks; profile: Version1Marks; front_hairline: V2[]; profile_hairline: V2[]; lifted_stroke_ids: number[] };

function named_marks_from_version_1(file: Version1File): NamedMark[] {
  const starting = (name: string) => STARTING_MARKS.find((mark) => mark.name === name)!;
  return [
    { ...starting("brow"), front: file.front.brow, profile: file.profile.brow },
    { ...starting("nasion"), front: file.front.nasion ?? starting("nasion").front, profile: file.profile.nasion ?? starting("nasion").profile },
    { ...starting("nose base"), front: file.front.nose_base, profile: file.profile.nose_base },
    { ...starting("ear hole"), profile: file.profile.porion },
  ];
}

export function landmark_mark(landmark: PlateLandmark, is_front_view: boolean): V2 | null {
  return is_front_view ? landmark.front : landmark.profile;
}

// ---- registration -------------------------------------------------------------------

// World -> image pixel of one view, a scale and an offset. Front: pixel x grows with
// world x, profile: pixel x grows with -z. Pixel y grows with -y in both.
export type PlateRegistration = { pixels_per_world_unit: number; pixel_of_world_origin: V2; is_front_view: boolean };

// Least squares over the view's marks for one scale s and one origin (ox, oy):
// pixel.x = ox + across * s, pixel.y = oy - y * s, where `across` is world x in front
// and -z in profile. Each landmark marked in the view gives one equation per axis; a
// landmark whose point the document lacks is skipped. Null when the marks don't spread
// (fewer than two, or all at one point): nothing to scale by.
export function plate_registration(tablet_document: TabletDocument, landmarks: readonly PlateLandmark[], is_front_view: boolean): PlateRegistration | null {
  // Pairs (pixel coordinate, world coordinate along the same axis, sign): x pairs have
  // sign +1 (pixel grows with world), y pairs -1 (pixel row grows downward).
  const x_pairs: { pixel: number; world: number }[] = [];
  const y_pairs: { pixel: number; world: number }[] = [];
  for (const landmark of landmarks) {
    const mark = landmark_mark(landmark, is_front_view);
    if (mark === null) continue;
    const world = template_point_world_position(tablet_document, landmark.template_point);
    if (world === null) continue;
    x_pairs.push({ pixel: mark.x, world: is_front_view ? world.x : -world.z });
    y_pairs.push({ pixel: mark.y, world: world.y });
  }
  if (x_pairs.length < 2) return null;
  const mean = (values: number[]) => values.reduce((sum, value) => sum + value, 0) / values.length;
  const x_pixel_mean = mean(x_pairs.map((pair) => pair.pixel)), x_world_mean = mean(x_pairs.map((pair) => pair.world));
  const y_pixel_mean = mean(y_pairs.map((pair) => pair.pixel)), y_world_mean = mean(y_pairs.map((pair) => pair.world));
  let numerator = 0, denominator = 0;
  for (const pair of x_pairs) {
    numerator += (pair.world - x_world_mean) * (pair.pixel - x_pixel_mean);
    denominator += (pair.world - x_world_mean) ** 2;
  }
  for (const pair of y_pairs) {
    numerator -= (pair.world - y_world_mean) * (pair.pixel - y_pixel_mean);
    denominator += (pair.world - y_world_mean) ** 2;
  }
  if (denominator < 1e-12) return null;
  const scale = numerator / denominator;
  if (Math.abs(scale) < 1e-6) return null;
  return {
    pixels_per_world_unit: scale,
    pixel_of_world_origin: { x: x_pixel_mean - x_world_mean * scale, y: y_pixel_mean + y_world_mean * scale },
    is_front_view,
  };
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

// The file, plus marks by vertex name the page must turn into landmarks once the
// document is loaded: the starting marks when the server has no file, a version 1
// file's marks. Null when the request failed.
export type FetchedLandmarksFile = { file: LoomisPlanesManLandmarksFile; named_marks_to_add: readonly NamedMark[] | null };

export async function fetch_landmarks_file(): Promise<FetchedLandmarksFile | null> {
  try {
    const response = await fetch(`/api/fit-landmarks/${LOOMIS_PLANES_MAN_LANDMARKS_NAME}`, { cache: "no-store" });
    if (response.status === 404) return { file: starting_landmarks_file(), named_marks_to_add: STARTING_MARKS };
    if (!response.ok) {
      console.error(`loading landmarks '${LOOMIS_PLANES_MAN_LANDMARKS_NAME}' failed: status ${response.status}`);
      return null;
    }
    const json = (await response.json()) as LoomisPlanesManLandmarksFile | Version1File;
    if (json.version === 1) {
      const file: LoomisPlanesManLandmarksFile = { version: 2, landmarks: [], front_hairline: json.front_hairline, profile_hairline: json.profile_hairline, lifted_stroke_ids: json.lifted_stroke_ids };
      return { file, named_marks_to_add: named_marks_from_version_1(json) };
    }
    return { file: json, named_marks_to_add: null };
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
