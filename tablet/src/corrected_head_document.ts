// The corrected head (plan-native-head-topology, stage 3): a copy of the scan head
// (skull-zanatomy) with every id unchanged, drawn mirrored, its centre plane closed, and
// warped to the proportions of the native head with the landmark pairs of
// fit-landmarks/native-head.json. One build for two writers: tmp/make_corrected_head_document.ts
// and the button "write head-corrected-trial" of the page fit-head-to-native-head, so the
// two cannot write different documents from the same pairs.
// No I/O and no DOM here: the script runs this under node, the page in the browser.
import { default_camera } from "./camera";
import { TabletDocument } from "./document";
import { NativeHeadLandmarksFile, PlacedNativeHeadLandmark, native_head_fit, scan_head_prepared_for_native_fit } from "./native_head_fit";
import { serialize_document_state } from "./persistence";

// placed_landmarks: the pairs as the fit placed them (where each template point was before
// the warp, and its target), in the file's order, for reporting how well it landed.
export type CorrectedHeadDocument = {
  corrected_head: TabletDocument;
  placed_landmarks: PlacedNativeHeadLandmark[];
};

// scan_head and native_head as loaded, the documents pairs_file names; both are only read.
// Throws when a landmark is not placed, when there is no warp, and when a vertex ends up
// across the centre plane: there is no corrected head to write then.
export function corrected_head_document_from_landmarks(scan_head: TabletDocument, native_head: TabletDocument, pairs_file: NativeHeadLandmarksFile): CorrectedHeadDocument {
  // The copy: drawn mirrored, centre plane closed (the pass of tmp/close_centre_plane_of_document.ts).
  const copy: TabletDocument = scan_head_prepared_for_native_fit(scan_head);

  const fit = native_head_fit(copy, native_head, pairs_file, pairs_file.stiffness);
  if (fit.unplaced_landmark_messages.length > 0) throw new Error(fit.unplaced_landmark_messages[0]);
  const corrected_head = fit.fitted_document;
  if (corrected_head === null) throw new Error("the fit has no warp");

  // A vertex pinned onto a midline stroke comes out a rounding error off the plane.
  const vertices_across_the_plane = corrected_head.vertices.filter((vertex) => vertex.position.x < -1e-6);
  if (vertices_across_the_plane.length > 0) throw new Error(`vertices at x < 0 after the warp: ${vertices_across_the_plane.map((vertex) => vertex.id).join(", ")}; nothing written`);

  return { corrected_head: corrected_head, placed_landmarks: fit.placed_landmarks };
}

// The text of documents/<name>.json for the corrected head: the document format of every
// document (serialize_document_state), with the default camera.
export function corrected_head_document_text(corrected_head: TabletDocument): string {
  return serialize_document_state(corrected_head, default_camera());
}
