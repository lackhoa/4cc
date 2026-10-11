// The Loomis planes-head plates (a man, front and profile, two separate scans) behind
// the template head (document `skull-zanatomy`): the page fit-hairline-to-loomis-man
// shows the head fitted to the plates while the hairline is drawn on the template in an
// embedded sketchpad (notes/tasks/autodraw_draw_as_data/plan-hairline-reference-plate.md,
// Q19-Q28).
//
// Each plate is registered to the head on its own, by landmarks Khoa marks on the plate
// against points of the head: one scale and one offset per view, least squares over all
// the landmarks marked in that view, both axes each. The same marks are then the targets
// of a thin-plate fit of the head (`man_fit_pairs`, src/landmark_fit.ts), so the plate
// views can show the head with the plate's proportions. The landmarks are a free list
// (removed and renamed on the page); the starting ones are the brow, nasion and nose base
// in both views and the ear hole (`porion_r`) in profile, plus the girl plate's landmarks
// (fit-landmarks/loomis-school-girl.json) placed by the registration. Both views are
// orthographic; the profile faces left, so world z grows to the viewer's left, as on the
// girl plate (src/loomis_girl_plate.ts).
import { TabletDocument, VertexId } from "./document";
import { LandmarkPair, mirrored_landmark_pairs } from "./landmark_fit";
import { TemplatePoint, template_point_world_position, template_position_is_on_midline } from "./loomis_girl_plate";
import { V2, V3, v3 } from "./math";

export const LOOMIS_PLANES_MAN_LANDMARKS_NAME = "loomis-planes-man";

export type PlateImage = { url: string; width_pixels: number; height_pixels: number };
export const FRONT_PLATE: PlateImage = { url: "/loomis-planes-man-front.webp", width_pixels: 414, height_pixels: 529 };
export const PROFILE_PLATE: PlateImage = { url: "/loomis-planes-man-profile.webp", width_pixels: 561, height_pixels: 670 };

// One landmark: a point of the head and where it is on each plate, in image pixels
// (null = not marked in that view, so the view's registration ignores it).
export type PlateLandmark = { name: string; template_point: TemplatePoint; front: V2 | null; profile: V2 | null };

// The file tablet/fit-landmarks/loomis-planes-man.json (server route
// /api/fit-landmarks/<name>). stiffness: 0 = every landmark lands exactly on its marks
// (plan Q22; no UI, edit the file); larger = a smoother warp that lets landmarks miss
// (src/landmark_fit.ts).
export type LoomisPlanesManLandmarksFile = {
  version: 4;
  landmarks: PlateLandmark[];
  stiffness: number;
};

export const DEFAULT_STIFFNESS = 0;

// A landmark given by its head vertex, by name or by id, instead of a template point: the
// starting landmarks (read off the scans by eye, 2026-10-11, for Khoa to drag into
// place), and what a version 1 file converts to. They become landmarks once the document
// is loaded (`landmarks_from_named_marks`). The vertices are skin points (plan Q12): the
// document's `nasion` is the skull's, so the nasion is skin vertex 73, the unnamed vertex
// the girl page's starting landmarks call "nasion".
export type NamedMark = { name: string; vertex: string | VertexId; front: V2 | null; profile: V2 | null };

export const STARTING_MARKS: readonly NamedMark[] = [
  { name: "brow", vertex: "brow", front: { x: 207, y: 255 }, profile: { x: 120, y: 210 } },
  { name: "nasion", vertex: 73, front: { x: 207, y: 275 }, profile: { x: 112, y: 232 } },
  { name: "nose base", vertex: "nose_base", front: { x: 207, y: 370 }, profile: { x: 60, y: 365 } },
  { name: "ear hole", vertex: "porion_r", front: null, profile: { x: 340, y: 290 } },
];

export function starting_landmarks_file(): LoomisPlanesManLandmarksFile {
  return { version: 4, landmarks: [], stiffness: DEFAULT_STIFFNESS };
}

// The girl plate's landmarks as marks for the man's plates (plan Q20): their names and
// template points, no marks of their own; the page places each on the plates where the
// registration puts its template point. One whose template point some landmark already
// has (the nasion is a starting mark too) is skipped: two landmarks on one point make
// the fit singular. Appended, so a saved file that lacks some of them gets them too.
export type TemplatePointMark = { name: string; template_point: TemplatePoint };

export function template_point_marks_missing_from(landmarks: readonly PlateLandmark[], marks: readonly TemplatePointMark[]): TemplatePointMark[] {
  const taken = new Set(landmarks.map((landmark) => JSON.stringify(landmark.template_point)));
  const names = new Set(landmarks.map((landmark) => landmark.name));
  return marks.filter((mark) => !taken.has(JSON.stringify(mark.template_point)) && !names.has(mark.name));
}

// Null when the document has none of the marks' vertices (not loaded yet); a mark whose
// vertex the document lacks is dropped with a console error.
export function landmarks_from_named_marks(tablet_document: TabletDocument, marks: readonly NamedMark[]): PlateLandmark[] | null {
  const landmarks: PlateLandmark[] = [];
  for (const mark of marks) {
    const vertex = tablet_document.vertices.find((v) => (typeof mark.vertex === "string" ? v.name === mark.vertex : v.id === mark.vertex));
    if (vertex === undefined) continue;
    landmarks.push({ name: mark.name, template_point: { vertex: vertex.id }, front: mark.front, profile: mark.profile });
  }
  if (landmarks.length === 0) return null;
  for (const mark of marks) {
    if (!landmarks.some((landmark) => landmark.name === mark.name)) console.error(`landmark '${mark.name}': the head has no vertex '${mark.vertex}', dropped`);
  }
  return landmarks;
}

// Version 1 files (before 2026-10-11) held five fixed marks per view. The midline column
// is dropped: the brow, nasion and nose base sit on the midline, so their front columns
// already fix it.
type Version1Marks = { brow: V2; nasion?: V2; nose_base: V2; porion: V2; midline_column: number };
type Version1File = { version: 1; front: Version1Marks; profile: Version1Marks };
// Version 2 and 3 files (2026-10-11) held hairlines drawn on the plates (polylines, then
// cubics) and the strokes of the last lift; all of that went with the lift (plan Q28),
// only the landmarks are kept.
type Version2Or3File = { version: 2 | 3; landmarks: PlateLandmark[] };

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

// ---- the fit ------------------------------------------------------------------------

// The landmark pairs of a thin-plate fit of the head to the plates (plan Q19/Q20): each
// landmark's template point must land where its marks are. The front mark gives x and y,
// the profile mark y and z; y is averaged when both are marked; a coordinate no mark
// gives keeps the template's value, and a midline point's x is 0 either way. Pairs are
// mirrored as the girl fit's. A landmark the document lacks, or marked in no registered
// view, gives no pair.
export function man_fit_pairs(tablet_document: TabletDocument, landmarks: readonly PlateLandmark[], front: PlateRegistration | null, profile: PlateRegistration | null): LandmarkPair[] {
  const pairs: LandmarkPair[] = [];
  for (const landmark of landmarks) {
    const template_position = template_point_world_position(tablet_document, landmark.template_point);
    if (template_position === null) continue;
    const from_front = front === null || landmark.front === null ? null : world_from_pixel(front, landmark.front);
    const from_profile = profile === null || landmark.profile === null ? null : world_from_pixel(profile, landmark.profile);
    if (from_front === null && from_profile === null) continue;
    const ys = [from_front, from_profile].filter((p): p is V3 => p !== null).map((p) => p.y);
    const target_position = v3(
      template_position_is_on_midline(template_position) ? 0 : (from_front?.x ?? template_position.x),
      ys.reduce((sum, y) => sum + y, 0) / ys.length,
      from_profile?.z ?? template_position.z,
    );
    pairs.push(...mirrored_landmark_pairs(template_position, target_position));
  }
  return pairs;
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
    const json = (await response.json()) as LoomisPlanesManLandmarksFile | Version2Or3File | Version1File;
    if (json.version === 1) return { file: starting_landmarks_file(), named_marks_to_add: named_marks_from_version_1(json) };
    if (json.version === 4) return { file: json, named_marks_to_add: null };
    return { file: { version: 4, landmarks: json.landmarks, stiffness: DEFAULT_STIFFNESS }, named_marks_to_add: null };
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
