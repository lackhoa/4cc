// The Loomis school girl plate as a fit target: where the two views sit in the image, and
// the landmarks marked on them. Shared by fit_head_to_loomis_girl.ts and
// render_loomis_girl_fit_overlay.ts.
//
// Both views are orthographic and share heights (Loomis's grid lines run across both).
// Front view: the head's sagittal plane is the image column FRONT_MIDLINE_X_PIXELS.
// Profile view: the face looks left, so world z grows to the left; PROFILE_PORION_X_PIXELS
// is the image column of z = 0 (the ear canal).
import os from "node:os";
import path from "node:path";
import { V3, v3 } from "../src/math";

export const LOOMIS_GIRL_IMAGE_PATH = path.join(os.homedir(), "Downloads/autodraw-loomis-girl/loomis-girl-front-and-profile.webp");
export const IMAGE_WIDTH_PIXELS = 1230;
export const IMAGE_HEIGHT_PIXELS = 740;

// Scale: the template's skin is 2.078 world units from chin bottom to top of head, the
// girl about 610 px, so the fitted head keeps the template's height.
export const PIXELS_PER_WORLD_UNIT = 293.5;
export const FRONT_MIDLINE_X_PIXELS = 302;
export const PROFILE_PORION_X_PIXELS = 988;
// Image row of world y = 0: chin bottom (row 672) sits at world y = -0.81, as in the template.
export const WORLD_Y_ZERO_ROW_PIXELS = 672 - 0.81 * PIXELS_PER_WORLD_UNIT;

export function front_pixel_from_world(p: V3): { x: number; y: number } {
  return { x: FRONT_MIDLINE_X_PIXELS + p.x * PIXELS_PER_WORLD_UNIT, y: WORLD_Y_ZERO_ROW_PIXELS - p.y * PIXELS_PER_WORLD_UNIT };
}

export function profile_pixel_from_world(p: V3): { x: number; y: number } {
  return { x: PROFILE_PORION_X_PIXELS - p.z * PIXELS_PER_WORLD_UNIT, y: WORLD_Y_ZERO_ROW_PIXELS - p.y * PIXELS_PER_WORLD_UNIT };
}

// One landmark: a template vertex and where it must land on the plate.
//  - front_half_width_pixels: distance from the front view's midline (0 = midline point).
//    Paired features were read on both sides of the face and averaged.
//  - row_pixels: image row, shared by both views (the profile wins where they disagree).
//  - profile_column_pixels: image column in the profile view.
//  - seen: which of the three numbers were read off the drawing. The rest are guesses,
//    because hair or the face itself hides the point in that view.
export type TargetLandmark = {
  template_vertex: number;
  label: string;
  front_half_width_pixels: number;
  row_pixels: number;
  profile_column_pixels: number;
  seen: "both views" | "front only" | "profile only" | "guess";
};

export const TARGET_LANDMARKS: TargetLandmark[] = [
  // midline, top to bottom
  { template_vertex: 70, label: "top of head", front_half_width_pixels: 0, row_pixels: 62, profile_column_pixels: 923, seen: "guess" },
  { template_vertex: 69, label: "upper forehead", front_half_width_pixels: 0, row_pixels: 196, profile_column_pixels: 675, seen: "profile only" },
  { template_vertex: 113, label: "glabella", front_half_width_pixels: 0, row_pixels: 322, profile_column_pixels: 659, seen: "profile only" },
  { template_vertex: 73, label: "nasion", front_half_width_pixels: 0, row_pixels: 360, profile_column_pixels: 672, seen: "profile only" },
  { template_vertex: 74, label: "nose tip", front_half_width_pixels: 0, row_pixels: 462, profile_column_pixels: 617, seen: "profile only" },
  { template_vertex: 101, label: "under nose tip", front_half_width_pixels: 0, row_pixels: 474, profile_column_pixels: 624, seen: "profile only" },
  { template_vertex: 78, label: "subnasale", front_half_width_pixels: 0, row_pixels: 488, profile_column_pixels: 662, seen: "profile only" },
  { template_vertex: 136, label: "upper lip front", front_half_width_pixels: 0, row_pixels: 531, profile_column_pixels: 652, seen: "profile only" },
  { template_vertex: 139, label: "between the lips", front_half_width_pixels: 0, row_pixels: 552, profile_column_pixels: 668, seen: "both views" },
  { template_vertex: 141, label: "lower lip front", front_half_width_pixels: 0, row_pixels: 570, profile_column_pixels: 668, seen: "profile only" },
  { template_vertex: 82, label: "fold under lower lip", front_half_width_pixels: 0, row_pixels: 595, profile_column_pixels: 683, seen: "profile only" },
  { template_vertex: 81, label: "chin bottom", front_half_width_pixels: 0, row_pixels: 670, profile_column_pixels: 705, seen: "both views" },
  { template_vertex: 76, label: "back of head", front_half_width_pixels: 0, row_pixels: 462, profile_column_pixels: 1160, seen: "guess" },
  // eye
  { template_vertex: 116, label: "inner eye corner", front_half_width_pixels: 49, row_pixels: 374, profile_column_pixels: 730, seen: "front only" },
  { template_vertex: 112, label: "outer eye corner", front_half_width_pixels: 143, row_pixels: 373, profile_column_pixels: 770, seen: "both views" },
  { template_vertex: 117, label: "upper lid top", front_half_width_pixels: 93, row_pixels: 356, profile_column_pixels: 722, seen: "both views" },
  // nose
  { template_vertex: 104, label: "nose wing top", front_half_width_pixels: 42, row_pixels: 456, profile_column_pixels: 693, seen: "both views" },
  { template_vertex: 94, label: "nose wing base", front_half_width_pixels: 46, row_pixels: 488, profile_column_pixels: 684, seen: "both views" },
  // mouth
  { template_vertex: 137, label: "upper lip peak", front_half_width_pixels: 33, row_pixels: 528, profile_column_pixels: 654, seen: "both views" },
  { template_vertex: 138, label: "mouth corner", front_half_width_pixels: 70, row_pixels: 556, profile_column_pixels: 722, seen: "both views" },
  // jaw border, chin to ear
  { template_vertex: 154, label: "chin bottom side", front_half_width_pixels: 55, row_pixels: 665, profile_column_pixels: 740, seen: "both views" },
  { template_vertex: 88, label: "jaw border middle", front_half_width_pixels: 78, row_pixels: 654, profile_column_pixels: 815, seen: "both views" },
  { template_vertex: 84, label: "jaw angle", front_half_width_pixels: 150, row_pixels: 583, profile_column_pixels: 920, seen: "both views" },
  // side of the face and head
  { template_vertex: 91, label: "cheek outline, mouth height", front_half_width_pixels: 170, row_pixels: 542, profile_column_pixels: 813, seen: "front only" },
  { template_vertex: 92, label: "cheekbone, widest", front_half_width_pixels: 200, row_pixels: 445, profile_column_pixels: 843, seen: "front only" },
  { template_vertex: 1, label: "ear canal", front_half_width_pixels: 151, row_pixels: 462, profile_column_pixels: 988, seen: "profile only" },
  { template_vertex: 75, label: "widest point of the cranium", front_half_width_pixels: 205, row_pixels: 211, profile_column_pixels: 923, seen: "guess" },
];

// A midline landmark that is not a vertex: the most forward point (largest z) of the
// template stroke between two vertices, and where that point must land in the profile.
export type TargetMostForwardPointLandmark = {
  stroke_p0_vertex: number;
  stroke_p3_vertex: number;
  label: string;
  row_pixels: number;
  profile_column_pixels: number;
};

export const TARGET_MOST_FORWARD_POINT_LANDMARKS: TargetMostForwardPointLandmark[] = [
  { stroke_p0_vertex: 81, stroke_p3_vertex: 82, label: "chin front", row_pixels: 626, profile_column_pixels: 669 },
];

export function target_most_forward_point_world_position(landmark: TargetMostForwardPointLandmark): V3 {
  return v3(0, (WORLD_Y_ZERO_ROW_PIXELS - landmark.row_pixels) / PIXELS_PER_WORLD_UNIT, (PROFILE_PORION_X_PIXELS - landmark.profile_column_pixels) / PIXELS_PER_WORLD_UNIT);
}

export function target_landmark_world_position(landmark: TargetLandmark): V3 {
  return v3(
    landmark.front_half_width_pixels / PIXELS_PER_WORLD_UNIT,
    (WORLD_Y_ZERO_ROW_PIXELS - landmark.row_pixels) / PIXELS_PER_WORLD_UNIT,
    (PROFILE_PORION_X_PIXELS - landmark.profile_column_pixels) / PIXELS_PER_WORLD_UNIT,
  );
}
