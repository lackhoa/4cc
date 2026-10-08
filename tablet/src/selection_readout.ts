// Selection readout: what is selected and hovered, as text with document ids
// ("line 22", "vertex 107", "patch 4"), so a bug report can name the items.

import { resolve_patch_fill } from "./patch";
import { StrokeId, TabletDocument, VertexId, pin_by_vertex, pins_on_stroke, smooth_knots_at_vertex, stroke_by_id, surface_pin_by_vertex, vertex_by_id } from "./document";

export type SelectionSnapshot = {
  stroke: StrokeId | null; // the primary selected line
  selected_handle: "p1" | "p2" | null; // of the primary line
  extra_strokes: StrokeId[]; // ctrl-tapped additions
  vertex: VertexId | null;
  extra_vertex: VertexId | null;
  patch: number | null; // index into tablet_document.patches
  hot:
    | { kind: "stroke"; stroke_id: StrokeId }
    | { kind: "handle"; key: "p1" | "p2" }
    | { kind: "vertex"; vertex: VertexId }
    | null;
};

function short_vertex_text(tablet_document: TabletDocument, vertex_id: VertexId): string {
  const name = vertex_by_id(tablet_document, vertex_id).name;
  return name === undefined ? `v${vertex_id}` : `v${vertex_id} "${name}"`;
}

function stroke_text(tablet_document: TabletDocument, stroke_id: StrokeId): string {
  const stroke = stroke_by_id(tablet_document, stroke_id);
  const parts = [
    stroke.name === undefined ? `line ${stroke_id}` : `line ${stroke_id} "${stroke.name}"`,
    `${short_vertex_text(tablet_document, stroke.p0_vertex)} -> ${short_vertex_text(tablet_document, stroke.p3_vertex)}`,
    `layer ${stroke.layer}`,
  ];
  const patches = tablet_document.patches.flatMap((patch, index) => (patch.strokes.includes(stroke_id) ? [index] : []));
  const patch_texts = patches.map((index) => (resolve_patch_fill(tablet_document.patches[index], tablet_document) === null ? `${index}(no fill)` : `${index}`));
  if (patches.length > 0) parts.push(`in patch ${patch_texts.join(" ")}`);
  const pinned = pins_on_stroke(tablet_document, stroke_id).map((pin) => `v${pin.vertex}`);
  if (pinned.length > 0) parts.push(`pins ${pinned.join(" ")}`);
  if (stroke.midline === true) parts.push("midline");
  if (stroke.on_surface === true) parts.push("on surface");
  return parts.join("  ");
}

function vertex_text(tablet_document: TabletDocument, vertex_id: VertexId): string {
  const vertex = vertex_by_id(tablet_document, vertex_id);
  const parts = [
    vertex.name === undefined ? `vertex ${vertex_id}` : `vertex ${vertex_id} "${vertex.name}"`,
    `bone ${vertex.bone_id}`,
  ];
  const strokes = tablet_document.strokes
    .filter((stroke) => stroke.p0_vertex === vertex_id || stroke.p3_vertex === vertex_id)
    .map((stroke) => stroke.id);
  parts.push(strokes.length > 0 ? `ends lines ${strokes.join(" ")}` : "ends no line");
  const pin = pin_by_vertex(tablet_document, vertex_id);
  if (pin !== null) parts.push(`pinned to line ${pin.host_stroke} at t ${pin.t.toFixed(3)}`);
  const surface_pin = surface_pin_by_vertex(tablet_document, vertex_id);
  // The patch by its list position, the number "in patch N" shows on a line.
  const patch_index = surface_pin === null ? -1 : tablet_document.patches.findIndex((patch) => patch.id === surface_pin.patch);
  if (surface_pin !== null) parts.push(`on surface (patch ${patch_index} at u ${surface_pin.u.toFixed(3)}, v ${surface_pin.v.toFixed(3)})`);
  for (const knot of smooth_knots_at_vertex(tablet_document, vertex_id)) parts.push(`smooth ${knot.stroke_a}~${knot.stroke_b}`);
  if (vertex.midline === true) parts.push("midline");
  return parts.join("  ");
}

export function describe_selection(tablet_document: TabletDocument, selection: SelectionSnapshot): string {
  const lines: string[] = [];
  if (selection.stroke !== null) {
    lines.push(stroke_text(tablet_document, selection.stroke));
    if (selection.selected_handle !== null) lines.push(`  handle ${selection.selected_handle}`);
    for (const extra of selection.extra_strokes) lines.push(`+ ${stroke_text(tablet_document, extra)}`);
  }
  if (selection.vertex !== null) {
    lines.push(vertex_text(tablet_document, selection.vertex));
    if (selection.extra_vertex !== null) lines.push(`+ ${vertex_text(tablet_document, selection.extra_vertex)}`);
  }
  if (selection.patch !== null) {
    lines.push(`patch ${selection.patch}  lines ${tablet_document.patches[selection.patch].strokes.join(" ")}`);
  }
  if (lines.length === 0) lines.push("nothing selected");
  const hot = selection.hot;
  if (hot !== null) {
    const hot_text = hot.kind === "stroke" ? `line ${hot.stroke_id}` : hot.kind === "vertex" ? short_vertex_text(tablet_document, hot.vertex) : `handle ${hot.key}`;
    lines.push(`hover: ${hot_text}`);
  }
  return lines.join("\n");
}
