// Page `fit-head-to-loomis-girl` (plan-loomis-girl-elaboration.md): the template head
// (skull and skin drawing) fitted to Loomis's school girl plate. Fixed to the document
// `loomis-school-girl`, which tmp/fit_head_to_loomis_girl.ts writes from `skull-zanatomy`.
// Left half: the sketchpad on that document, free orbit, no Z-Anatomy meshes (they do not
// line up with a fitted drawing). Right half: the plate's front and profile views with
// the document drawn over them (loomis_girl_plate_views.ts).
// Separate localStorage keys so this page never steals another page's camera/tool state.
import { start_sketchpad } from "../../src/sketchpad";
import { start_loomis_girl_plate_views } from "./loomis_girl_plate_views";

const DOCUMENT_NAME = "loomis-school-girl";

start_sketchpad({
  load_reference_mesh: async () => null,
  default_document_name: DOCUMENT_NAME,
  storage_key_prefix: "autodraw_fit_head_to_loomis_girl",
  can_switch_documents: false,
  // Only the skin can be checked against the plate, so hand touch-up is on skin curves;
  // the fitted skull is a guess carried by the warp.
  active_layer: "skin",
  locked_layers: ["skull"],
  hidden_layers: [],
});
start_loomis_girl_plate_views(DOCUMENT_NAME);
