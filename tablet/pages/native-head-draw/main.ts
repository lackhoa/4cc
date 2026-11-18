// Page `native-head-draw`: the sketchpad on document `native-head`, the head drawing ported
// from the native C++ editor. The document is stored one-sided and mirrored across the
// midline (`is_mirrored_across_midline`, src/document.ts), so the sketchpad draws the other
// side itself. No reference mesh, overlay or landmarks: just the drawing.
// Separate localStorage keys so this page never steals the plain sketchpad's document.
import { start_sketchpad } from "../../src/sketchpad";

start_sketchpad({
  // No mesh behind the drawing (null = none).
  load_reference_mesh: async () => null,
  default_document_name: "native-head",
  storage_key_prefix: "autodraw_native_head_draw",
  can_switch_documents: false,
  // Every stroke of the ported head is on the `skin` layer; nothing is hidden or locked.
  active_layer: "skin",
  locked_layers: [],
  hidden_layers: [],
});
