// The plain sketchpad (`/draw/`): the height-normalized `skull.obj` behind the drawing,
// documents switchable, the `autodraw_tablet_*` localStorage keys.
import { fetch_reference_mesh } from "../src/reference";
import { start_sketchpad } from "../src/sketchpad";

start_sketchpad({
  load_reference_mesh: () => fetch_reference_mesh("/reference/skull.obj"),
  default_document_name: "untitled",
  storage_key_prefix: "autodraw_tablet",
  can_switch_documents: true,
});
