// Does the Loomis girl fit put the hairline on the plate's hair edge without the three
// hairline landmarks? Fits twice (with / without) and prints where vertices 163/164/165
// land in plate pixels next to the marks. Node-only: npx tsx tmp/hairline_fit_without_marks.ts
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { default_camera } from "../src/camera";
import { TabletDocument, empty_document, vertex_by_id } from "../src/document";
import { fitted_document_from_landmarks } from "../src/landmark_fit";
import { FitLandmarksFile, front_pixel_from_world, profile_pixel_from_world } from "../src/loomis_girl_plate";
import { apply_document_state } from "../src/persistence";

const tablet_directory = path.join(path.dirname(fileURLToPath(import.meta.url)), "..");
const template: TabletDocument = empty_document();
apply_document_state(fs.readFileSync(path.join(tablet_directory, "documents/skull-zanatomy.json"), "utf8"), template, default_camera());
const file: FitLandmarksFile = JSON.parse(fs.readFileSync(path.join(tablet_directory, "fit-landmarks/loomis-school-girl.json"), "utf8"));
const hairline_names = ["hairline", "hairline temple", "hairline sideburn"];
const marks = file.landmarks.filter((l) => hairline_names.includes(l.name));

function report(label: string, landmarks_file: FitLandmarksFile): void {
  const fitted = fitted_document_from_landmarks(template, landmarks_file);
  if (fitted === null) { console.log(`${label}: fit failed`); return; }
  console.log(label);
  for (const mark of marks) {
    const vertex = (mark.template_point as { vertex: number }).vertex;
    const p = vertex_by_id(fitted, vertex).position;
    const front = front_pixel_from_world(p, file.front_midline_column_pixels);
    const profile = profile_pixel_from_world(p);
    console.log(`  ${mark.name.padEnd(18)} front (${(front.x - file.front_midline_column_pixels).toFixed(0).padStart(4)}, ${front.y.toFixed(0)}) mark (${mark.front_half_width_pixels}, ${mark.row_pixels})   profile col ${profile.x.toFixed(0)} mark ${mark.profile_column_pixels}`);
  }
}
report("with hairline marks", file);
report("without hairline marks", { ...file, landmarks: file.landmarks.filter((l) => !hairline_names.includes(l.name)) });

// Canon on the fitted girl (without hairline marks): brow + (brow - nose base), in plate rows.
{
  const fitted = fitted_document_from_landmarks(template, { ...file, landmarks: file.landmarks.filter((l) => !hairline_names.includes(l.name)) })!;
  const brow = vertex_by_id(fitted, 113).position, nose = vertex_by_id(fitted, 101).position;
  const canon_y = brow.y + (brow.y - nose.y);
  const row = front_pixel_from_world({ x: 0, y: canon_y, z: 0 }, file.front_midline_column_pixels).y;
  console.log(`canon hairline on the fitted girl: brow row ${front_pixel_from_world(brow, file.front_midline_column_pixels).y.toFixed(0)}, nose base row ${front_pixel_from_world(nose, file.front_midline_column_pixels).y.toFixed(0)} -> hairline row ${row.toFixed(0)} (plate: 150)`);
}
