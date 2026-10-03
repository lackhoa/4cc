// Dry run of the sketchpad's wrap_skull_onto_skin (plan-skin-from-skull-wrap.md) on
// documents/skull-zanatomy.json, in memory only: nothing is written. Prints one row per
// copied vertex (method, push distance) and the handle-coplanarity error of every copy.
// Run: node node_modules/tsx/dist/cli.mjs tmp/wrap_skull_onto_skin_dry_run.ts
import { readFileSync } from "node:fs";
import { default_camera } from "../src/camera";
import { copy_layer_strokes, empty_document, enforce_midline, move_vertex, pin_by_vertex, stroke_control_points, update_pinned_vertex_positions, vertex_by_id, vertex_is_on_midline, vertex_position } from "../src/document";
import { v3, v3_add, v3_cross, v3_dot, v3_length, v3_normalize, v3_scale, v3_sub } from "../src/math";
import { nearest_point_on_mesh, project_vertex_onto_mesh } from "../src/mesh_projection";
import { apply_document_state } from "../src/persistence";
import { frankfurt_coordinates, frankfurt_frame_from_landmarks, parse_landmarks_file, parse_obj_mesh_raw, reference_mesh_from_positions } from "../src/reference";
import { WORLD_PER_MM } from "../src/reference_skull_view";

const models = "../data/reference-models/";
const frame = frankfurt_frame_from_landmarks(parse_landmarks_file(readFileSync(models + "z-anatomy-head-skull.landmarks.txt", "utf8")))!;
const raw = parse_obj_mesh_raw(readFileSync(models + "z-anatomy-head-skin_head.obj", "utf8"))!;
const mesh = reference_mesh_from_positions(raw.positions.map((p) => v3_scale(frankfurt_coordinates(frame, p), WORLD_PER_MM)), raw.triangle_indices);

const tablet_document = empty_document();
if (!apply_document_state(readFileSync("documents/skull-zanatomy.json", "utf8"), tablet_document, default_camera())) throw new Error("document unreadable");
console.log("before:", tablet_document.vertices.length, "vertices", tablet_document.strokes.length, "strokes", tablet_document.patches.length, "patches");

let head_center = v3(0, 0, 0);
for (const position of mesh.triangle_positions) head_center = v3_add(head_center, position);
head_center = v3_scale(head_center, 1 / mesh.triangle_positions.length);
console.log("head center mm", v3_scale(head_center, 1 / WORLD_PER_MM));

const copied_vertex = copy_layer_strokes(tablet_document, "skull", "skin");
for (const [source_id, vertex_id] of copied_vertex) {
  const name = vertex_by_id(tablet_document, source_id).name ?? "";
  if (pin_by_vertex(tablet_document, vertex_id) !== null) { console.log(`| ${vertex_id} | ${name} | pinned | |`); continue; }
  const position = vertex_position(tablet_document, vertex_id);
  let outward = v3_sub(position, head_center);
  const midline = vertex_is_on_midline(tablet_document, vertex_id);
  if (midline) outward = v3(0, outward.y, outward.z);
  const projection = project_vertex_onto_mesh(mesh, position, v3_normalize(outward), 25 * WORLD_PER_MM);
  move_vertex(tablet_document, vertex_id, projection.position);
  const nearest_mm = v3_length(v3_sub(nearest_point_on_mesh(mesh, position), position)) / WORLD_PER_MM;
  console.log(`| ${vertex_id} | ${name}${midline ? " (mid)" : ""} | ${projection.method} | ${(v3_length(v3_sub(projection.position, position)) / WORLD_PER_MM).toFixed(1)} | nearest ${nearest_mm.toFixed(1)} |`);
}
enforce_midline(tablet_document);
update_pinned_vertex_positions(tablet_document);

let worst_coplanarity = 0;
let worst_off_mesh_mm = 0;
for (const stroke of tablet_document.strokes.filter((candidate) => candidate.layer === "skin")) {
  const points = stroke_control_points(stroke, tablet_document);
  const chord = v3_sub(points.p3, points.p0);
  const triple = Math.abs(v3_dot(v3_cross(v3_normalize(stroke.d0), v3_normalize(stroke.d3)), v3_normalize(chord)));
  if (v3_length(stroke.d0) > 1e-9 && v3_length(stroke.d3) > 1e-9) worst_coplanarity = Math.max(worst_coplanarity, triple);
  for (const end of [points.p0, points.p3]) {
    worst_off_mesh_mm = Math.max(worst_off_mesh_mm, v3_length(v3_sub(nearest_point_on_mesh(mesh, end), end)) / WORLD_PER_MM);
  }
}
console.log("after:", tablet_document.vertices.length, "vertices", tablet_document.strokes.length, "strokes", tablet_document.patches.length, "patches",
  tablet_document.vertex_pins.length, "pins", tablet_document.smooth_knots.length, "knots");
console.log("worst |d0 x d3 . chord| (unit vectors, 0 = coplanar):", worst_coplanarity.toExponential(2));
console.log("worst skin stroke endpoint off the mesh, mm:", worst_off_mesh_mm.toFixed(2));

// Per layer: the coplanarity error of each stroke, to compare the copies with their sources.
for (const layer of ["skull", "skin"]) {
  const errors: string[] = [];
  for (const stroke of tablet_document.strokes.filter((candidate) => candidate.layer === layer)) {
    const points = stroke_control_points(stroke, tablet_document);
    const chord = v3_normalize(v3_sub(points.p3, points.p0));
    errors.push(Math.abs(v3_dot(v3_cross(v3_normalize(stroke.d0), v3_normalize(stroke.d3)), chord)).toFixed(3));
  }
  console.log(layer, errors.join(" "));
}
