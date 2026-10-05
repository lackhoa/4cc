// The Loomis school girl plate as a fit target: where the two views sit in the image, and
// the landmarks that tie the template head (document `skull-zanatomy`) to it. Shared by
// the pages fit-head-to-loomis-girl (shows the fit) and loomis-girl-fit-landmarks (edits
// the landmarks and computes the fit).
//
// Both views are orthographic and share heights (Loomis's grid lines run across both).
// Front view: the head's sagittal plane is one image column, kept in the landmark file
// (front_midline_column_pixels). Profile view: the face looks left, so world z grows to
// the left; PROFILE_PORION_X_PIXELS is the image column of z = 0 (the ear canal).
import { TabletDocument, bezier_point, stroke_control_points, vertex_world_position } from "./document";
import { V2, V3, v3 } from "./math";

export const IMAGE_WIDTH_PIXELS = 1230;
export const IMAGE_HEIGHT_PIXELS = 740;

// Scale: the template's skin is 2.078 world units from chin bottom to top of head, the
// girl about 610 px, so the fitted head keeps the template's height.
export const PIXELS_PER_WORLD_UNIT = 293.5;
export const PROFILE_PORION_X_PIXELS = 988;
// Image row of world y = 0: chin bottom (row 672) sits at world y = -0.81, as in the template.
export const WORLD_Y_ZERO_ROW_PIXELS = 672 - 0.81 * PIXELS_PER_WORLD_UNIT;

export function front_pixel_from_world(p: V3, front_midline_column_pixels: number): V2 {
  return { x: front_midline_column_pixels + p.x * PIXELS_PER_WORLD_UNIT, y: WORLD_Y_ZERO_ROW_PIXELS - p.y * PIXELS_PER_WORLD_UNIT };
}

export function profile_pixel_from_world(p: V3): V2 {
  return { x: PROFILE_PORION_X_PIXELS - p.z * PIXELS_PER_WORLD_UNIT, y: WORLD_Y_ZERO_ROW_PIXELS - p.y * PIXELS_PER_WORLD_UNIT };
}

// ---- where a plate view sits in its canvas ------------------------------------------

// The part of the plate a view shows when fitted, in image pixels.
export type PlateRegion = { left: number; top: number; width: number; height: number };
export const FRONT_REGION: PlateRegion = { left: 50, top: 30, width: 505, height: 690 };
export const PROFILE_REGION: PlateRegion = { left: 580, top: 30, width: 630, height: 690 };

// canvas CSS pixel = (image pixel - image_pixel_at_canvas_origin) * image_pixels_to_canvas_scale
export type PlateViewTransform = { image_pixels_to_canvas_scale: number; image_pixel_at_canvas_origin: V2 };

// The transform that shows all of `region`, centered in the canvas.
export function plate_view_transform_showing_region(canvas_width: number, canvas_height: number, region: PlateRegion): PlateViewTransform {
  const scale = Math.min(canvas_width / region.width, canvas_height / region.height);
  return {
    image_pixels_to_canvas_scale: scale,
    image_pixel_at_canvas_origin: {
      x: region.left - (canvas_width / scale - region.width) / 2,
      y: region.top - (canvas_height / scale - region.height) / 2,
    },
  };
}

// `transform` zoomed by `factor`, keeping the image pixel under `canvas_point` where it is.
export function plate_view_transform_zoomed_at(transform: PlateViewTransform, canvas_point: V2, factor: number): PlateViewTransform {
  const new_scale = Math.max(0.2, Math.min(40, transform.image_pixels_to_canvas_scale * factor));
  const image_x = transform.image_pixel_at_canvas_origin.x + canvas_point.x / transform.image_pixels_to_canvas_scale;
  const image_y = transform.image_pixel_at_canvas_origin.y + canvas_point.y / transform.image_pixels_to_canvas_scale;
  return {
    image_pixels_to_canvas_scale: new_scale,
    image_pixel_at_canvas_origin: { x: image_x - canvas_point.x / new_scale, y: image_y - canvas_point.y / new_scale },
  };
}

// ---- landmarks ----------------------------------------------------------------------

// A point of the template head: a vertex, or a point along a stroke (0 = the stroke's
// first endpoint, 1 = its last).
export type TemplatePoint = { vertex: number } | { stroke: number; position_along_stroke: number };

// One landmark: a template point and the mark that says where it must land on the plate.
//  - front_half_width_pixels: distance of the mark from the front view's midline. Not
//    used for a midline landmark (template point on x = 0), which sits on the midline.
//  - row_pixels: image row, shared by both views.
//  - profile_column_pixels: image column in the profile view.
//  - read_from_front / read_from_profile: the mark was placed by looking at that view.
//    A coordinate no view gave is a guess, because hair or the face itself hides the
//    point there.
export type FitLandmark = {
  name: string;
  template_point: TemplatePoint;
  front_half_width_pixels: number;
  row_pixels: number;
  profile_column_pixels: number;
  read_from_front: boolean;
  read_from_profile: boolean;
};

// The file tablet/fit-landmarks/<name>.json (server route /api/fit-landmarks/<name>).
// stiffness: 0 = every landmark lands exactly on its mark; larger = a smoother warp that
// lets landmarks miss (src/landmark_fit.ts).
export type FitLandmarksFile = {
  version: 1;
  landmarks: FitLandmark[];
  front_midline_column_pixels: number;
  stiffness: number;
};

export const LOOMIS_GIRL_FIT_LANDMARKS_NAME = "loomis-school-girl";

// The landmarks as Claude marked them (2026-10-04): what a page shows while the server
// has no landmark file, and what "reset" goes back to.
export function starting_fit_landmarks_file(): FitLandmarksFile {
  const vertex_landmark = (vertex: number, name: string, front_half_width_pixels: number, row_pixels: number, profile_column_pixels: number, read_from: "both views" | "front only" | "profile only" | "guess"): FitLandmark => ({
    name,
    template_point: { vertex },
    front_half_width_pixels,
    row_pixels,
    profile_column_pixels,
    read_from_front: read_from === "both views" || read_from === "front only",
    read_from_profile: read_from === "both views" || read_from === "profile only",
  });
  return {
    version: 1,
    front_midline_column_pixels: 302,
    stiffness: 0,
    landmarks: [
      // midline, top to bottom
      vertex_landmark(70, "top of head", 0, 62, 923, "guess"),
      vertex_landmark(69, "upper forehead", 0, 196, 675, "profile only"),
      vertex_landmark(113, "glabella", 0, 322, 659, "profile only"),
      vertex_landmark(73, "nasion", 0, 360, 672, "profile only"),
      vertex_landmark(74, "nose tip", 0, 462, 617, "profile only"),
      vertex_landmark(101, "under nose tip", 0, 474, 624, "profile only"),
      vertex_landmark(78, "subnasale", 0, 488, 662, "profile only"),
      vertex_landmark(136, "upper lip front", 0, 531, 652, "profile only"),
      vertex_landmark(139, "between the lips", 0, 552, 668, "both views"),
      vertex_landmark(141, "lower lip front", 0, 570, 668, "profile only"),
      vertex_landmark(82, "fold under lower lip", 0, 595, 683, "profile only"),
      vertex_landmark(81, "chin bottom", 0, 670, 705, "both views"),
      vertex_landmark(76, "back of head", 0, 462, 1160, "guess"),
      // eye
      vertex_landmark(116, "inner eye corner", 49, 374, 730, "front only"),
      vertex_landmark(112, "outer eye corner", 143, 373, 770, "both views"),
      vertex_landmark(117, "upper lid top", 93, 356, 722, "both views"),
      // nose
      vertex_landmark(104, "nose wing top", 42, 456, 693, "both views"),
      vertex_landmark(94, "nose wing base", 46, 488, 684, "both views"),
      // mouth
      vertex_landmark(137, "upper lip peak", 33, 528, 654, "both views"),
      vertex_landmark(138, "mouth corner", 70, 556, 722, "both views"),
      // jaw border, chin to ear
      vertex_landmark(154, "chin bottom side", 55, 665, 740, "both views"),
      vertex_landmark(88, "jaw border middle", 78, 654, 815, "both views"),
      vertex_landmark(84, "jaw angle", 150, 583, 920, "both views"),
      // side of the face and head
      vertex_landmark(91, "cheek outline, mouth height", 170, 542, 813, "front only"),
      vertex_landmark(92, "cheekbone, widest", 200, 445, 843, "front only"),
      vertex_landmark(1, "ear canal", 151, 462, 988, "profile only"),
      vertex_landmark(75, "widest point of the cranium", 205, 211, 923, "guess"),
      // The most forward point of the chin's midline stroke (fold under the lip to chin bottom).
      { name: "chin front", template_point: { stroke: 76, position_along_stroke: 0.41 }, front_half_width_pixels: 0, row_pixels: 626, profile_column_pixels: 669, read_from_front: false, read_from_profile: true },
    ],
  };
}

// Where the template point is in `tablet_document` (the template, or a document fitted
// from it: the fit keeps vertex and stroke ids). Null when the document has no such
// vertex or stroke.
export function template_point_world_position(tablet_document: TabletDocument, template_point: TemplatePoint): V3 | null {
  if ("vertex" in template_point) {
    const vertex = tablet_document.vertices.find((v) => v.id === template_point.vertex);
    return vertex === undefined ? null : vertex_world_position(tablet_document, vertex);
  }
  const stroke = tablet_document.strokes.find((s) => s.id === template_point.stroke);
  return stroke === undefined ? null : bezier_point(stroke_control_points(stroke, tablet_document), template_point.position_along_stroke);
}

// A template point this near x = 0 (world units) makes its landmark a midline landmark.
const MIDLINE_HALF_THICKNESS = 1e-3;

export function template_position_is_on_midline(template_position: V3): boolean {
  return Math.abs(template_position.x) < MIDLINE_HALF_THICKNESS;
}

// Where the landmark's mark is in world space, on the side of the face the documents
// hold (x >= 0).
export function fit_landmark_mark_world_position(landmark: FitLandmark, is_on_midline: boolean): V3 {
  return v3(
    is_on_midline ? 0 : landmark.front_half_width_pixels / PIXELS_PER_WORLD_UNIT,
    (WORLD_Y_ZERO_ROW_PIXELS - landmark.row_pixels) / PIXELS_PER_WORLD_UNIT,
    (PROFILE_PORION_X_PIXELS - landmark.profile_column_pixels) / PIXELS_PER_WORLD_UNIT,
  );
}

// The landmark file on the server, or the starting landmarks when the server has none.
// Null when the request failed.
export async function fetch_fit_landmarks_file(name: string): Promise<FitLandmarksFile | null> {
  try {
    const response = await fetch(`/api/fit-landmarks/${encodeURIComponent(name)}`, { cache: "no-store" });
    if (response.status === 404) return starting_fit_landmarks_file();
    if (!response.ok) {
      console.error(`loading fit landmarks '${name}' failed: status ${response.status}`);
      return null;
    }
    return (await response.json()) as FitLandmarksFile;
  } catch (error) {
    console.error(`loading fit landmarks '${name}' failed (server unreachable)`, error);
    return null;
  }
}

// True when the server stored the file.
export async function save_fit_landmarks_file(name: string, file: FitLandmarksFile): Promise<boolean> {
  try {
    const response = await fetch(`/api/fit-landmarks/${encodeURIComponent(name)}`, { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify(file, null, 1) });
    if (!response.ok) console.error(`saving fit landmarks '${name}' failed: status ${response.status}`);
    return response.ok;
  } catch (error) {
    console.error(`saving fit landmarks '${name}' failed (server unreachable)`, error);
    return false;
  }
}
