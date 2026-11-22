// The fit of the scan head (skull-zanatomy) onto the native head (plan-native-head-topology,
// stage 3), by the landmarks of fit-landmarks/native-head.json. One fit for two callers:
// tmp/make_corrected_head_document.ts writes it as the document head-corrected-trial, the
// page fit-head-to-native-head shows it live while the landmarks are edited.
// No DOM here: the script runs this under node.
import { TabletDocument, enforce_midline, stroke_control_points, vertex_position } from "./document";
import { LandmarkPair, fitted_document_from_landmark_pairs, mirrored_landmark_pairs } from "./landmark_fit";
import { TemplatePoint, template_point_world_position } from "./loomis_girl_plate";
import { V3 } from "./math";
import { update_pinned_vertex_positions } from "./patch";

// Where a landmark lands on the native head: one of its vertices (follows the native head
// as it is edited), or a fixed position in its frame where it has no vertex.
export type NativeHeadLandmarkTarget = { native_vertex: number } | { native_position: V3 };
export type NativeHeadLandmark = { name: string; template_point: TemplatePoint; target: NativeHeadLandmarkTarget; note: string };
export type NativeHeadLandmarksFile = {
  about: string;
  template_document: string;
  target_document: string;
  stiffness: number;
  landmarks: NativeHeadLandmark[];
};
export type PlacedNativeHeadLandmark = { name: string; template_position: V3; target_position: V3 };
// fitted_document is null when there is no warp (no landmark placed). A landmark whose
// template point or native vertex is missing is left out, with a message saying why.
export type NativeHeadFit = {
  fitted_document: TabletDocument | null;
  placed_landmarks: PlacedNativeHeadLandmark[];
  unplaced_landmark_messages: string[];
};

// The copy that gets warped: every id unchanged, drawn mirrored, its centre plane closed
// (the pass of tmp/close_centre_plane_of_document.ts). The scan head itself is only read.
export function scan_head_prepared_for_native_fit(scan_head: TabletDocument): TabletDocument {
  const copy: TabletDocument = structuredClone(scan_head);
  copy.is_mirrored_across_midline = true;
  for (const stroke of copy.strokes) {
    if (stroke.midline === true) continue;
    const points = stroke_control_points(stroke, copy);
    const farthest = Math.max(Math.abs(points.p0.x), Math.abs(points.p1.x), Math.abs(points.p2.x), Math.abs(points.p3.x));
    if (farthest < 0.005) {
      stroke.midline = true;
      console.log(`stroke ${stroke.id} lies on the centre plane without the flag: now a midline stroke`);
    }
  }
  enforce_midline(copy);
  update_pinned_vertex_positions(copy);
  return copy;
}

// Null when the target is a vertex the native head does not have (any more).
export function native_head_landmark_target_position(native_head: TabletDocument, target: NativeHeadLandmarkTarget): V3 | null {
  if ("native_position" in target) return target.native_position;
  const native_vertex_id = target.native_vertex;
  if (!native_head.vertices.some((vertex) => vertex.id === native_vertex_id)) return null;
  return vertex_position(native_head, native_vertex_id);
}

// prepared_scan_head comes from scan_head_prepared_for_native_fit. The pairs are mirrored in
// the file's order of landmarks.
export function native_head_fit(prepared_scan_head: TabletDocument, native_head: TabletDocument, landmarks_file: NativeHeadLandmarksFile, stiffness: number): NativeHeadFit {
  const placed_landmarks: PlacedNativeHeadLandmark[] = [];
  const unplaced_landmark_messages: string[] = [];
  for (const landmark of landmarks_file.landmarks) {
    const template_position = template_point_world_position(prepared_scan_head, landmark.template_point);
    if (template_position === null) {
      unplaced_landmark_messages.push(`pair "${landmark.name}": its template point is not in ${landmarks_file.template_document}`);
      continue;
    }
    const target_position = native_head_landmark_target_position(native_head, landmark.target);
    if (target_position === null) {
      const native_vertex_id = "native_vertex" in landmark.target ? landmark.target.native_vertex : null;
      unplaced_landmark_messages.push(`pair "${landmark.name}": vertex ${native_vertex_id} is not in ${landmarks_file.target_document}`);
      continue;
    }
    placed_landmarks.push({ name: landmark.name, template_position: template_position, target_position: target_position });
  }
  const pairs: LandmarkPair[] = [];
  for (const placed of placed_landmarks) pairs.push(...mirrored_landmark_pairs(placed.template_position, placed.target_position));
  const fitted_document = fitted_document_from_landmark_pairs(prepared_scan_head, pairs, stiffness);
  return { fitted_document: fitted_document, placed_landmarks: placed_landmarks, unplaced_landmark_messages: unplaced_landmark_messages };
}
