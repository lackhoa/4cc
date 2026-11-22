// One-time import of the native head (game/driver/documents/main-01.ad) into a tablet
// document, documents/native-head.json. Prints a report of what was left out or changed,
// checks the result, then writes the file.
// Run: npx tsx tmp/convert_native_ad_to_tablet_document.ts [--stroke-groups-only]
//   --stroke-groups-only: documents/native-head.json is NOT written (it has hand edits since);
//   only tmp/native-head-stroke-groups.json is, the native group and vis tag of every stroke
//   id the conversion hands out (read by tmp/copy_native_features_into_corrected_head.ts).
import assert from "node:assert";
import { readFileSync, writeFileSync } from "node:fs";
import { default_camera } from "../src/camera";
import {
  Patch, Stroke, StrokeId, StrokeRadii, TabletDocument, VertexId, SKULL_BONE_ID,
  add_patch, add_stroke, add_vertex, bezier_point, empty_document, split_stroke, stroke_by_id,
  stroke_control_points, vertex_is_on_midline, vertex_position,
} from "../src/document";
import { V3, v3, v3_add, v3_cross, v3_dot, v3_length, v3_lerp, v3_normalize, v3_scale, v3_sub } from "../src/math";
import { patch_surface_grid, resolve_patch_fill } from "../src/patch";
import { apply_document_state, serialize_document_state } from "../src/persistence";
import { Rgb } from "../src/vertex_sink";

// ---------------------------------------------------------------------------
// Generic reader of the .ad format: a table of type descriptions, then one value
// of the root type. Every value comes back as plain JSON-like data.

type TypeReference =
  | { kind: "named"; name: string }
  | { kind: "array"; count: number; element: TypeReference }
  | { kind: "darray"; element: TypeReference };
type StructMember = { name: string; type: TypeReference; discriminator_member: string };
type EnumMember = { name: string; value: number };
type UnionVariant = { name: string; type: TypeReference; value: number };
type TypeDescription = {
  name: string;
  kind: number;
  size: number;
  members: StructMember[];
  enum_members: EnumMember[];
  variants: UnionVariant[];
};
type ByteReader = { view: DataView; bytes: Uint8Array; position: number };

const TYPE_KIND_BASIC = 1;
const TYPE_KIND_STRUCT = 2;
const TYPE_KIND_UNION = 3;
const TYPE_KIND_ENUM = 4;
const TYPE_KIND_WRAPPER = 6;

function read_u32(reader: ByteReader): number {
  const value = reader.view.getUint32(reader.position, true);
  reader.position += 4;
  return value;
}

function read_i32(reader: ByteReader): number {
  const value = reader.view.getInt32(reader.position, true);
  reader.position += 4;
  return value;
}

function read_string(reader: ByteReader): string {
  const length = read_u32(reader);
  let text = "";
  for (let i = 0; i < length; i++) text += String.fromCharCode(reader.bytes[reader.position + i]);
  reader.position += length;
  return text;
}

function read_type_reference(reader: ByteReader): TypeReference {
  const kind = read_u32(reader);
  if (kind === 0) return { kind: "named", name: read_string(reader) };
  if (kind === 1) {
    const count = read_u32(reader);
    return { kind: "array", count, element: read_type_reference(reader) };
  }
  if (kind === 2) return { kind: "darray", element: read_type_reference(reader) };
  throw new Error(`unknown type reference kind ${kind} at byte ${reader.position}`);
}

function read_type_description(reader: ByteReader): TypeDescription {
  const description: TypeDescription = {
    name: read_string(reader), kind: read_u32(reader), size: read_u32(reader), members: [], enum_members: [], variants: [],
  };
  if (description.kind === TYPE_KIND_STRUCT || description.kind === TYPE_KIND_WRAPPER) {
    const count = read_u32(reader);
    for (let i = 0; i < count; i++) {
      description.members.push({ name: read_string(reader), type: read_type_reference(reader), discriminator_member: read_string(reader) });
    }
  } else if (description.kind === TYPE_KIND_ENUM) {
    const count = read_u32(reader);
    for (let i = 0; i < count; i++) description.enum_members.push({ name: read_string(reader), value: read_i32(reader) });
  } else if (description.kind === TYPE_KIND_UNION) {
    read_string(reader); // the discriminator's name, repeated on the struct member
    const count = read_u32(reader);
    for (let i = 0; i < count; i++) {
      description.variants.push({ name: read_string(reader), type: read_type_reference(reader), value: read_i32(reader) });
    }
  }
  return description;
}

function read_basic_value(reader: ByteReader, description: TypeDescription): unknown {
  if (description.name === "String") return read_string(reader);
  const start = reader.position;
  reader.position += description.size;
  if (/^v\d+$/.test(description.name)) {
    const floats: number[] = [];
    for (let offset = 0; offset < description.size; offset += 4) floats.push(reader.view.getFloat32(start + offset, true));
    return floats;
  }
  if (description.size === 4 && ["i1", "b32", "i32"].includes(description.name)) return reader.view.getInt32(start, true);
  if (description.size === 4) return reader.view.getUint32(start, true);
  if (description.size === 2) return reader.view.getInt16(start, true);
  if (description.size === 1) return reader.view.getUint8(start);
  throw new Error(`basic type ${description.name} of size ${description.size} is not handled`);
}

function enum_member_name(description: TypeDescription, value: number): string | number {
  const member = description.enum_members.find((candidate) => candidate.value === value);
  if (member === undefined) return value;
  return member.name;
}

function read_value(reader: ByteReader, types: Map<string, TypeDescription>, reference: TypeReference): unknown {
  if (reference.kind === "array") {
    const items: unknown[] = [];
    for (let i = 0; i < reference.count; i++) items.push(read_value(reader, types, reference.element));
    return items;
  }
  if (reference.kind === "darray") {
    const count = read_u32(reader);
    const items: unknown[] = [];
    for (let i = 0; i < count; i++) items.push(read_value(reader, types, reference.element));
    return items;
  }
  const description = types.get(reference.name)!;
  if (description.kind === TYPE_KIND_BASIC) return read_basic_value(reader, description);
  if (description.kind === TYPE_KIND_ENUM) return enum_member_name(description, read_i32(reader));
  if (description.kind !== TYPE_KIND_STRUCT && description.kind !== TYPE_KIND_WRAPPER) {
    throw new Error(`type ${description.name} of kind ${description.kind} cannot be read as a value`);
  }
  const record: Record<string, unknown> = {};
  const raw_enum_values: Record<string, number> = {};
  for (const member of description.members) {
    let member_type: TypeDescription | undefined;
    if (member.type.kind === "named") member_type = types.get(member.type.name);
    if (member_type !== undefined && member_type.kind === TYPE_KIND_UNION) {
      // A union holds the variant picked by an earlier enum member of the same struct.
      const picked = raw_enum_values[member.discriminator_member];
      const variant = member_type.variants.find((candidate) => candidate.value === picked);
      if (variant === undefined) throw new Error(`union ${member_type.name}: no variant for discriminator ${picked} at byte ${reader.position}`);
      record[member.name] = { [variant.name]: read_value(reader, types, variant.type) };
    } else if (member_type !== undefined && member_type.kind === TYPE_KIND_ENUM) {
      const value = read_i32(reader);
      raw_enum_values[member.name] = value;
      record[member.name] = enum_member_name(member_type, value);
    } else {
      record[member.name] = read_value(reader, types, member.type);
    }
  }
  return record;
}

function read_ad_file(path: URL): unknown {
  const bytes = new Uint8Array(readFileSync(path));
  const reader: ByteReader = { view: new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength), bytes, position: 0 };
  reader.position += 16; // magic, tag, time stamp
  const types = new Map<string, TypeDescription>();
  const type_count = read_u32(reader);
  for (let i = 0; i < type_count; i++) {
    const description = read_type_description(reader);
    types.set(description.name, description);
  }
  const root_type_name = read_string(reader);
  return read_value(reader, types, { kind: "named", name: root_type_name });
}

// ---------------------------------------------------------------------------
// The native document, as far as the import reads it.

type NativeBoneId = { type: string; id: number };
type NativeVertex = { p: number[] };
type NativeGroup = {
  parent_index: number;
  params: {
    line: { flags: number; alignment_min: number[] };
    fill: { color: number };
    radius_mult: number[];
    line_color: number;
  };
  vis_tag: string;
  cam_vis: { active: number };
  one_sided: number;
  bone_id: NativeBoneId;
};
type NativeCurve = {
  handle_offset: { v: number[] }[];
  radii: number[];
  lightness_additions: number[];
  straight: number;
  midline: number;
  key: string;
};
type NativeDualBezier = { P: { e: { v: number[] }[] }; Q: { e: { v: number[] }[] } };
type NativePrimitive = {
  group_index: number;
  vertex_index: number[];
  data: {
    curve?: NativeCurve;
    poly3?: unknown;
    patch?: unknown;
    dual_bezier?: NativeDualBezier;
    curve_patch?: { curve_count: number; curve_index: number[] };
  };
};
type NativeDocument = { vertices: NativeVertex[]; groups: NativeGroup[]; primitives: NativePrimitive[] };

// The head ball has radius 1 in the native head bone and is 9.2 cm; a tablet unit is 10 cm.
const TABLET_UNITS_PER_HEAD_UNIT = 0.92;
// Native line width unit is 1.728125 mm of world space, whatever the bone's scale; the
// tablet's is 1 mm (RIBBON_RADIUS in ribbon.ts).
const TABLET_RADII_PER_NATIVE_RADII = 1.728125;
const MILLIMETERS_PER_TABLET_UNIT = 100;
const LINE_FLAG_INVISIBLE = 8;
// Rest transform of the eyeball bone inside the head bone: translate(eye anchor + offset) * scale.
const EYE_ANCHOR_VERTEX_INDEX = 44;
const EYEBALL_OFFSET_FROM_ANCHOR = v3(0.12744, 0.009981, -0.292468);
const EYEBALL_SCALE = 0.141958;

type Converter = {
  native: NativeDocument;
  tablet_document: TabletDocument;
  group_is_included: boolean[];
  vertex_id_of_native_vertex: Map<number, VertexId>;
  stroke_id_of_native_curve: Map<number, StrokeId>;
  hidden_straight_stroke_of_vertex_pair: Map<string, StrokeId>;
  // The native primitive each stroke was made for: its curve, or the fill whose edge it is
  // (the first fill, for an edge shared by several).
  native_primitive_of_stroke: Map<StrokeId, number>;
  report_lines: string[];
};

function v3_of(numbers: number[]): V3 {
  return v3(numbers[0], numbers[1], numbers[2]);
}

function group_is_in_eyeball_bone(group: NativeGroup): boolean {
  return group.bone_id.type !== "Bone_Head";
}

// A point of a group's bone space, in tablet space.
function tablet_point(converter: Converter, group: NativeGroup, point: V3): V3 {
  let in_head_space = point;
  if (group_is_in_eyeball_bone(group)) {
    const eye_anchor = v3_of(converter.native.vertices[EYE_ANCHOR_VERTEX_INDEX].p);
    in_head_space = v3_add(v3_add(eye_anchor, EYEBALL_OFFSET_FROM_ANCHOR), v3_scale(point, EYEBALL_SCALE));
  }
  return v3_scale(in_head_space, TABLET_UNITS_PER_HEAD_UNIT);
}

// An offset (a handle) of a group's bone space, in tablet space.
function tablet_offset(group: NativeGroup, offset: V3): V3 {
  let in_head_space = offset;
  if (group_is_in_eyeball_bone(group)) in_head_space = v3_scale(offset, EYEBALL_SCALE);
  return v3_scale(in_head_space, TABLET_UNITS_PER_HEAD_UNIT);
}

function srgb_from_linear(linear: number): number {
  if (linear <= 0.0031308) return linear * 12.92;
  return Math.pow(linear, 1 / 2.4) * 1.055 - 0.055;
}

// Native colors are 0xAARRGGBB in linear light; alpha is dropped (every fill is opaque).
function tablet_color(native_argb: number): Rgb {
  return {
    r: srgb_from_linear(((native_argb >>> 16) & 0xff) / 255),
    g: srgb_from_linear(((native_argb >>> 8) & 0xff) / 255),
    b: srgb_from_linear((native_argb & 0xff) / 255),
  };
}

function tablet_vertex(converter: Converter, native_vertex_index: number, group: NativeGroup): VertexId {
  const existing = converter.vertex_id_of_native_vertex.get(native_vertex_index);
  if (existing !== undefined) return existing;
  const position = tablet_point(converter, group, v3_of(converter.native.vertices[native_vertex_index].p));
  const vertex_id = add_vertex(converter.tablet_document, position, SKULL_BONE_ID);
  converter.vertex_id_of_native_vertex.set(native_vertex_index, vertex_id);
  return vertex_id;
}

function stroke_from_native_curve(converter: Converter, primitive_index: number): StrokeId {
  const existing = converter.stroke_id_of_native_curve.get(primitive_index);
  if (existing !== undefined) return existing;
  const primitive = converter.native.primitives[primitive_index];
  const group = converter.native.groups[primitive.group_index];
  const curve = primitive.data.curve!;
  const tablet_document = converter.tablet_document;
  const p0_vertex = tablet_vertex(converter, primitive.vertex_index[0], group);
  const p3_vertex = tablet_vertex(converter, primitive.vertex_index[1], group);
  const is_straight = curve.straight !== 0;
  let d0 = v3(0, 0, 0);
  let d3 = v3(0, 0, 0);
  if (!is_straight) {
    d0 = tablet_offset(group, v3_of(curve.handle_offset[0].v));
    d3 = tablet_offset(group, v3_of(curve.handle_offset[1].v));
  }
  const stroke = stroke_by_id(tablet_document, add_stroke(tablet_document, p0_vertex, p3_vertex, d0, d3, "skin"));

  const width_factor = group.params.radius_mult[0] * TABLET_RADII_PER_NATIVE_RADII;
  let native_radii = curve.radii;
  if (is_straight) native_radii = [curve.radii[1], curve.radii[1], curve.radii[1], curve.radii[1]];
  stroke.radii = native_radii.map((radius) => radius * width_factor) as StrokeRadii;
  stroke.line_color = tablet_color(group.params.line_color);
  if (curve.lightness_additions.some((addition) => addition !== 0)) {
    stroke.lightness_additions = [...curve.lightness_additions] as StrokeRadii;
  }
  if (curve.midline !== 0) stroke.midline = true;
  if (group.one_sided !== 0) stroke.is_one_sided = true;

  const has_no_width = curve.radii.every((radius) => radius === 0);
  const group_line_is_invisible = (group.params.line.flags & LINE_FLAG_INVISIBLE) !== 0;
  const group_is_left_out = !converter.group_is_included[primitive.group_index];
  if (has_no_width || group_line_is_invisible || group_is_left_out) stroke.is_hidden = true;

  converter.stroke_id_of_native_curve.set(primitive_index, stroke.id);
  converter.native_primitive_of_stroke.set(stroke.id, primitive_index);
  return stroke.id;
}

// The edge of a flat fill: one hidden straight stroke per pair of vertices, shared by
// every fill that has the edge. A new one runs from `from_vertex` to `to_vertex`.
// fill_primitive_index: the fill asking for the edge.
function hidden_straight_stroke_between(converter: Converter, fill_primitive_index: number, from_vertex: VertexId, to_vertex: VertexId): StrokeId {
  const pair_key = `${Math.min(from_vertex, to_vertex)}-${Math.max(from_vertex, to_vertex)}`;
  const existing = converter.hidden_straight_stroke_of_vertex_pair.get(pair_key);
  if (existing !== undefined) return existing;
  const zero = v3(0, 0, 0);
  const stroke_id = add_stroke(converter.tablet_document, from_vertex, to_vertex, zero, zero, "skin");
  stroke_by_id(converter.tablet_document, stroke_id).is_hidden = true;
  converter.hidden_straight_stroke_of_vertex_pair.set(pair_key, stroke_id);
  converter.native_primitive_of_stroke.set(stroke_id, fill_primitive_index);
  return stroke_id;
}

function add_fill_patch(converter: Converter, primitive: NativePrimitive, strokes: StrokeId[]): Patch {
  const group = converter.native.groups[primitive.group_index];
  const tablet_document = converter.tablet_document;
  const patch_id = add_patch(tablet_document, strokes);
  const patch = tablet_document.patches.find((candidate) => candidate.id === patch_id)!;
  patch.fill_color = tablet_color(group.params.fill.color);
  if (group.one_sided !== 0) patch.is_one_sided = true;
  return patch;
}

function millimeters(tablet_length: number): string {
  return `${(tablet_length * MILLIMETERS_PER_TABLET_UNIT).toFixed(3)} mm`;
}

// One rail of a dual bezier, as a stroke. The rail is stored as 4 absolute control points
// whose ends are table vertices. When a curve primitive joins the same two vertices, that
// curve is the rail (hidden when its own group is left out); else a hidden stroke is made.
function stroke_of_dual_bezier_rail(
  converter: Converter, primitive_index: number, rail_name: string, rail: { e: { v: number[] }[] },
  native_start_vertex: number, native_end_vertex: number,
): StrokeId {
  const primitive = converter.native.primitives[primitive_index];
  const group = converter.native.groups[primitive.group_index];
  const tablet_document = converter.tablet_document;
  const start_vertex = tablet_vertex(converter, native_start_vertex, group);
  const end_vertex = tablet_vertex(converter, native_end_vertex, group);
  const p0 = vertex_position(tablet_document, start_vertex);
  const p3 = vertex_position(tablet_document, end_vertex);
  const stored_p1 = tablet_point(converter, group, v3_of(rail.e[1].v));
  const stored_p2 = tablet_point(converter, group, v3_of(rail.e[2].v));

  const curve_primitive_index = converter.native.primitives.findIndex((candidate) =>
    candidate.data.curve !== undefined
    && candidate.vertex_index[0] === native_start_vertex && candidate.vertex_index[1] === native_end_vertex);
  if (curve_primitive_index !== -1) {
    const stroke_id = stroke_from_native_curve(converter, curve_primitive_index);
    const points = stroke_control_points(stroke_by_id(tablet_document, stroke_id), tablet_document);
    const difference = Math.max(v3_length(v3_sub(points.p1, stored_p1)), v3_length(v3_sub(points.p2, stored_p2)));
    converter.report_lines.push(
      `dual bezier ${primitive_index} rail ${rail_name}: curve ${curve_primitive_index} is the fill edge; `
      + `its control points are ${millimeters(difference)} from the stored rail`);
    return stroke_id;
  }
  const d0 = v3_sub(stored_p1, v3_scale(v3_add(v3_scale(p0, 2), p3), 1 / 3));
  const d3 = v3_sub(stored_p2, v3_scale(v3_add(p0, v3_scale(p3, 2)), 1 / 3));
  if (v3_length(d0) < 1e-6 && v3_length(d3) < 1e-6) return hidden_straight_stroke_between(converter, primitive_index, start_vertex, end_vertex);
  const stroke_id = add_stroke(tablet_document, start_vertex, end_vertex, d0, d3, "skin");
  stroke_by_id(tablet_document, stroke_id).is_hidden = true;
  converter.native_primitive_of_stroke.set(stroke_id, primitive_index);
  return stroke_id;
}

// Largest distance from a point of the tablet's fill to the native loft lerp(P(u), Q(u), v).
function largest_distance_from_native_loft(converter: Converter, patch: Patch, primitive: NativePrimitive): number {
  const group = converter.native.groups[primitive.group_index];
  const dual_bezier = primitive.data.dual_bezier!;
  const rail_points = (rail: { e: { v: number[] }[] }) => {
    const control = rail.e.map((entry) => tablet_point(converter, group, v3_of(entry.v)));
    return { p0: control[0], p1: control[1], p2: control[2], p3: control[3] };
  };
  // The ends of the stored rails are stale; the vertex table is the authority.
  const rail_p = rail_points(dual_bezier.P);
  const rail_q = rail_points(dual_bezier.Q);
  rail_p.p0 = vertex_position(converter.tablet_document, tablet_vertex(converter, primitive.vertex_index[0], group));
  rail_p.p3 = vertex_position(converter.tablet_document, tablet_vertex(converter, primitive.vertex_index[1], group));
  rail_q.p0 = vertex_position(converter.tablet_document, tablet_vertex(converter, primitive.vertex_index[2], group));
  rail_q.p3 = vertex_position(converter.tablet_document, tablet_vertex(converter, primitive.vertex_index[3], group));
  const native_samples: V3[] = [];
  for (let i = 0; i <= 96; i++) {
    const on_p = bezier_point(rail_p, i / 96);
    const on_q = bezier_point(rail_q, i / 96);
    for (let j = 0; j <= 32; j++) native_samples.push(v3_lerp(on_p, on_q, j / 32));
  }
  const grid = patch_surface_grid(patch, converter.tablet_document)!;
  let largest = 0;
  for (const column of grid.positions) {
    for (const position of column) {
      let nearest = Infinity;
      for (const sample of native_samples) nearest = Math.min(nearest, v3_length(v3_sub(position, sample)));
      largest = Math.max(largest, nearest);
    }
  }
  return largest;
}

function convert_dual_bezier(converter: Converter, primitive_index: number): void {
  const primitive = converter.native.primitives[primitive_index];
  const group = converter.native.groups[primitive.group_index];
  const dual_bezier = primitive.data.dual_bezier!;
  const vertex_index = primitive.vertex_index;
  const tablet_document = converter.tablet_document;
  const rail_p_is_a_point = vertex_index[0] === vertex_index[1];
  if (!rail_p_is_a_point) {
    const stroke_p = stroke_of_dual_bezier_rail(converter, primitive_index, "P", dual_bezier.P, vertex_index[0], vertex_index[1]);
    const stroke_q = stroke_of_dual_bezier_rail(converter, primitive_index, "Q", dual_bezier.Q, vertex_index[2], vertex_index[3]);
    const patch = add_fill_patch(converter, primitive, [stroke_p, stroke_q]);
    converter.report_lines.push(
      `dual bezier ${primitive_index}: the tablet fill is at most `
      + `${millimeters(largest_distance_from_native_loft(converter, patch, primitive))} from the native one`);
    return;
  }
  // A fan from the point vertex to rail Q: the curve and two straight edges. The fill
  // collapses at the corner where the patch's first stroke ends, so that stroke must end
  // in the point vertex.
  const point_vertex = tablet_vertex(converter, vertex_index[0], group);
  const stroke_q = stroke_of_dual_bezier_rail(converter, primitive_index, "Q", dual_bezier.Q, vertex_index[2], vertex_index[3]);
  const edge_from_start = hidden_straight_stroke_between(converter, primitive_index, tablet_vertex(converter, vertex_index[2], group), point_vertex);
  const edge_from_end = hidden_straight_stroke_between(converter, primitive_index, tablet_vertex(converter, vertex_index[3], group), point_vertex);
  let strokes = [edge_from_start, edge_from_end, stroke_q];
  if (stroke_by_id(tablet_document, edge_from_start).p3_vertex !== point_vertex) strokes = [edge_from_end, edge_from_start, stroke_q];
  assert.strictEqual(
    stroke_by_id(tablet_document, strokes[0]).p3_vertex, point_vertex,
    `dual bezier ${primitive_index}: no edge ends in the point vertex`);
  add_fill_patch(converter, primitive, strokes);
}

function convert_poly3(converter: Converter, primitive_index: number): void {
  const primitive = converter.native.primitives[primitive_index];
  const group = converter.native.groups[primitive.group_index];
  const corners = [0, 1, 2].map((corner) => tablet_vertex(converter, primitive.vertex_index[corner], group));
  const strokes = [
    hidden_straight_stroke_between(converter, primitive_index, corners[0], corners[1]),
    hidden_straight_stroke_between(converter, primitive_index, corners[1], corners[2]),
    hidden_straight_stroke_between(converter, primitive_index, corners[2], corners[0]),
  ];
  add_fill_patch(converter, primitive, strokes);
}

function convert_curve_patch(converter: Converter, primitive_index: number): void {
  const primitive = converter.native.primitives[primitive_index];
  const curve_patch = primitive.data.curve_patch!;
  const strokes: StrokeId[] = [];
  for (let i = 0; i < curve_patch.curve_count; i++) strokes.push(stroke_from_native_curve(converter, curve_patch.curve_index[i]));
  add_fill_patch(converter, primitive, strokes);
}

// How far a stroke's second handle leaves the plane of its chord and first handle.
function stroke_distance_from_flat(stroke: Stroke, tablet_document: TabletDocument): number {
  const points = stroke_control_points(stroke, tablet_document);
  const plane_normal = v3_cross(v3_sub(points.p3, points.p0), v3_sub(points.p1, points.p0));
  if (v3_length(plane_normal) < 1e-12) return 0;
  return Math.abs(v3_dot(v3_sub(points.p2, points.p0), v3_normalize(plane_normal)));
}

// Splits a curve that is not flat at its middle with a smooth knot, so each half is
// nearly flat (a tablet stroke is flat by contract).
function split_native_curve_at_middle(converter: Converter, primitive_index: number): void {
  const tablet_document = converter.tablet_document;
  const first_half_id = converter.stroke_id_of_native_curve.get(primitive_index)!;
  const whole_stroke = stroke_by_id(tablet_document, first_half_id);
  const distance_before = stroke_distance_from_flat(whole_stroke, tablet_document);
  const whole_points = stroke_control_points(whole_stroke, tablet_document);
  const stroke_count_before = tablet_document.strokes.length;
  const knot_vertex = split_stroke(tablet_document, first_half_id, 0.5);
  assert.notStrictEqual(knot_vertex, null, `curve ${primitive_index} was not split`);
  const second_half = tablet_document.strokes[stroke_count_before];
  converter.native_primitive_of_stroke.set(second_half.id, primitive_index);
  const first_half = stroke_by_id(tablet_document, first_half_id);
  const first_half_points = stroke_control_points(first_half, tablet_document);
  const second_half_points = stroke_control_points(second_half, tablet_document);
  let largest_move = 0;
  for (let i = 0; i <= 32; i++) {
    const t = i / 32;
    const on_first_half = v3_length(v3_sub(bezier_point(first_half_points, t), bezier_point(whole_points, t / 2)));
    const on_second_half = v3_length(v3_sub(bezier_point(second_half_points, t), bezier_point(whole_points, 0.5 + t / 2)));
    largest_move = Math.max(largest_move, on_first_half, on_second_half);
  }
  converter.report_lines.push(
    `curve ${primitive_index} split at the middle: ${millimeters(distance_before)} from flat before, `
    + `halves ${millimeters(stroke_distance_from_flat(first_half, tablet_document))} and `
    + `${millimeters(stroke_distance_from_flat(second_half, tablet_document))}; `
    + `the two halves are at most ${millimeters(largest_move)} from the native curve`);
}

const MIDLINE_PLANE_TOLERANCE = 1e-6;

// A stroke whose two ends and two handles are at x = 0 lies in the centre plane: it
// becomes a `midline` stroke, so its ends are held on the plane and it is drawn once.
// The native file does not always flag these.
function flag_strokes_lying_in_midline_plane(converter: Converter): void {
  const tablet_document = converter.tablet_document;
  const flagged_strokes: StrokeId[] = [];
  for (const stroke of tablet_document.strokes) {
    if (stroke.midline === true) continue;
    const points = stroke_control_points(stroke, tablet_document);
    const lies_in_midline_plane = [points.p0, points.p1, points.p2, points.p3].every(
      (point) => Math.abs(point.x) < MIDLINE_PLANE_TOLERANCE);
    if (!lies_in_midline_plane) continue;
    stroke.midline = true;
    flagged_strokes.push(stroke.id);
  }
  converter.report_lines.push(`strokes lying in the centre plane, flagged midline: ${flagged_strokes.join(", ")}`);
}

// A stroke drawn on both sides with an end on the centre plane: that end must be held
// there, or dragging it opens a gap between the two sides.
function check_mirrored_stroke_ends_on_midline_plane_are_held(tablet_document: TabletDocument): void {
  for (const stroke of tablet_document.strokes) {
    if (stroke.midline === true || stroke.is_one_sided === true) continue;
    for (const vertex_id of [stroke.p0_vertex, stroke.p3_vertex]) {
      if (Math.abs(vertex_position(tablet_document, vertex_id).x) >= MIDLINE_PLANE_TOLERANCE) continue;
      assert.ok(
        vertex_is_on_midline(tablet_document, vertex_id),
        `mirrored stroke ${stroke.id} ends on the centre plane at vertex ${vertex_id}, which is not held there`);
    }
  }
  console.log("ok   every mirrored stroke end on the centre plane is held there");
}


function convert_native_document(native: NativeDocument): Converter {
  assert.deepStrictEqual(
    { vertices: native.vertices.length, groups: native.groups.length, primitives: native.primitives.length },
    { vertices: 62, groups: 47, primitives: 76 },
    "main-01.ad is not the document the import was planned on");

  const left_out_vis_tags = ["Vis_Level1", "Vis_Eyeball"];
  const group_is_included = native.groups.map((group) => {
    let current: NativeGroup | undefined = group;
    while (current !== undefined) {
      if (left_out_vis_tags.includes(current.vis_tag)) return false;
      current = native.groups[current.parent_index]; // -1 at a root
    }
    return true;
  });
  const left_out_groups = native.groups.map((_, index) => index).filter((index) => !group_is_included[index]);
  assert.deepStrictEqual(left_out_groups, [19, 21, 26, 27, 29, 30, 31, 32, 34, 35, 38, 39, 40, 41, 44, 45]);
  const eyeball_groups = native.groups.map((_, index) => index).filter((index) => group_is_in_eyeball_bone(native.groups[index]));
  assert.deepStrictEqual(eyeball_groups, [43, 44, 45, 46]);

  const tablet_document = empty_document();
  tablet_document.is_mirrored_across_midline = true;
  const converter: Converter = {
    native, tablet_document, group_is_included,
    vertex_id_of_native_vertex: new Map(), stroke_id_of_native_curve: new Map(),
    hidden_straight_stroke_of_vertex_pair: new Map(), native_primitive_of_stroke: new Map(), report_lines: [],
  };

  const left_out_curves: number[] = [];
  const dropped_old_patches: number[] = [];
  const curves_with_shape_key: number[] = [];
  native.primitives.forEach((primitive, primitive_index) => {
    if (primitive.data.curve === undefined) return;
    if (!group_is_included[primitive.group_index]) {
      left_out_curves.push(primitive_index);
      return;
    }
    if (primitive.data.curve.key !== "Weight_None") curves_with_shape_key.push(primitive_index);
    stroke_from_native_curve(converter, primitive_index);
  });
  native.primitives.forEach((primitive, primitive_index) => {
    if (!group_is_included[primitive.group_index]) return;
    if (primitive.data.poly3 !== undefined) convert_poly3(converter, primitive_index);
    if (primitive.data.dual_bezier !== undefined) convert_dual_bezier(converter, primitive_index);
    if (primitive.data.curve_patch !== undefined) convert_curve_patch(converter, primitive_index);
    if (primitive.data.patch !== undefined) dropped_old_patches.push(primitive_index);
  });
  split_native_curve_at_middle(converter, 16);
  split_native_curve_at_middle(converter, 74);
  flag_strokes_lying_in_midline_plane(converter);

  for (const stroke of tablet_document.strokes) {
    if (stroke.midline !== true) continue;
    for (const vertex_id of [stroke.p0_vertex, stroke.p3_vertex]) {
      assert.ok(Math.abs(vertex_position(tablet_document, vertex_id).x) < MIDLINE_PLANE_TOLERANCE, `midline stroke ${stroke.id} has an end off x = 0`);
    }
  }

  const guide_curves_kept_hidden = left_out_curves.filter((index) => converter.stroke_id_of_native_curve.has(index));
  const groups_where = (test: (group: NativeGroup) => boolean): number[] =>
    native.groups.map((_, index) => index).filter((index) => group_is_included[index] && test(native.groups[index]));
  converter.report_lines.unshift(
    `left-out groups (guides and eyeball spheres): ${left_out_groups.join(", ")}`,
    `left-out curves: ${left_out_curves.filter((index) => !guide_curves_kept_hidden.includes(index)).join(", ")}`,
    `guide curves kept as hidden fill edges: ${guide_curves_kept_hidden.join(", ")}`,
    `old 4x4 patches dropped: primitives ${dropped_old_patches.join(", ")}`,
    `curves with a blink shape key, rest shape only: ${curves_with_shape_key.join(", ")}`,
    `groups hidden natively below a view alignment, always shown here: ${groups_where((group) => group.params.line.alignment_min[0] !== 0).join(", ")}`,
    `groups hidden natively by camera direction, always shown here: ${groups_where((group) => group.cam_vis.active !== 0).join(", ")}`,
    "not imported: line and fill depth offsets, hair and the Loomis ball (code), round end caps, the 0.1065 mm thin-part cutoff",
    "vertex 42 lies behind the face (eye triangles 52, 53); imported as it is",
  );
  converter.report_lines.push(
    `imported: ${tablet_document.vertices.length} vertices (native: ${converter.vertex_id_of_native_vertex.size} used of ${native.vertices.length}), `
    + `${tablet_document.strokes.length} strokes (${tablet_document.strokes.filter((stroke) => stroke.is_hidden === true).length} hidden), `
    + `${tablet_document.patches.length} patches`);
  return converter;
}

// ---------------------------------------------------------------------------
// Checks, then the file.

function check_roundtrip_is_unchanged(document_name: string, text: string): TabletDocument {
  const reloaded = empty_document();
  const camera = default_camera();
  assert.ok(apply_document_state(text, reloaded, camera), `${document_name} did not load`);
  assert.strictEqual(serialize_document_state(reloaded, camera), text, `${document_name} changed in a load and save`);
  console.log(`ok   ${document_name}: load and save gives the same file`);
  return reloaded;
}

function check_every_patch_builds_a_fill(tablet_document: TabletDocument): void {
  for (const patch of tablet_document.patches) {
    assert.notStrictEqual(resolve_patch_fill(patch, tablet_document), null, `patch ${patch.id} builds no fill`);
  }
  console.log(`ok   every one of the ${tablet_document.patches.length} patches builds a fill`);
}

// A fan (a fill of 3 sides) is the native fan only when it collapses at the point vertex.
function check_fans_collapse_at_their_point(converter: Converter, reloaded: TabletDocument): void {
  converter.native.primitives.forEach((primitive, primitive_index) => {
    if (primitive.data.dual_bezier === undefined || primitive.vertex_index[0] !== primitive.vertex_index[1]) return;
    const point_vertex = converter.vertex_id_of_native_vertex.get(primitive.vertex_index[0])!;
    const point = vertex_position(reloaded, point_vertex);
    const edge_strokes = reloaded.strokes.filter((stroke) => stroke.is_hidden === true && stroke.p3_vertex === point_vertex);
    const patch = reloaded.patches.find((candidate) => edge_strokes.filter((stroke) => candidate.strokes.includes(stroke.id)).length === 2)!;
    const fill = resolve_patch_fill(patch, reloaded)!;
    assert.strictEqual(fill.kind, "coons", `dual bezier ${primitive_index}: the fan is not a fill of 3 sides`);
    const grid = patch_surface_grid(patch, reloaded)!;
    for (const position of grid.positions[0]) {
      assert.ok(v3_length(v3_sub(position, point)) < 1e-9, `dual bezier ${primitive_index}: the fan does not collapse at its point vertex`);
    }
    console.log(`ok   dual bezier ${primitive_index}: the fan collapses at its point vertex`);
  });
}

// The native group of every stroke, by the stroke id this conversion handed out. The ends
// are written too, so a reader can tell the id still means the same stroke in a
// native-head.json edited by hand since.
type NativeHeadStrokeGroup = {
  stroke_id: StrokeId;
  p0_vertex: VertexId;
  p3_vertex: VertexId;
  is_hidden: boolean;
  native_primitive_index: number;
  native_group_index: number;
  vis_tag: string;
};

function native_head_stroke_groups(converter: Converter): NativeHeadStrokeGroup[] {
  return converter.tablet_document.strokes.map((stroke) => {
    const native_primitive_index = converter.native_primitive_of_stroke.get(stroke.id);
    assert.notStrictEqual(native_primitive_index, undefined, `stroke ${stroke.id} has no native primitive`);
    const native_group_index = converter.native.primitives[native_primitive_index!].group_index;
    return {
      stroke_id: stroke.id,
      p0_vertex: stroke.p0_vertex,
      p3_vertex: stroke.p3_vertex,
      is_hidden: stroke.is_hidden === true,
      native_primitive_index: native_primitive_index!,
      native_group_index: native_group_index,
      vis_tag: converter.native.groups[native_group_index].vis_tag,
    };
  });
}

function write_native_head_stroke_groups(converter: Converter): void {
  const stroke_groups = native_head_stroke_groups(converter);
  const stroke_count_of_vis_tag = new Map<string, number>();
  for (const stroke_group of stroke_groups) {
    stroke_count_of_vis_tag.set(stroke_group.vis_tag, (stroke_count_of_vis_tag.get(stroke_group.vis_tag) ?? 0) + 1);
  }
  console.log(`strokes by vis tag: ${[...stroke_count_of_vis_tag].map(([vis_tag, count]) => `${vis_tag} ${count}`).join(", ")}`);
  writeFileSync(new URL("./native-head-stroke-groups.json", import.meta.url), JSON.stringify(stroke_groups, null, 1) + "\n");
  console.log(`wrote tmp/native-head-stroke-groups.json (${stroke_groups.length} strokes)`);
}

const native = read_ad_file(new URL("../../game/driver/documents/main-01.ad", import.meta.url)) as NativeDocument;
console.log("read main-01.ad");
const converter = convert_native_document(native);
console.log("converted; report:");
for (const line of converter.report_lines) console.log(`  ${line}`);

// The groups do not depend on the checks of the document below, so they are written first.
write_native_head_stroke_groups(converter);

const native_head_text = serialize_document_state(converter.tablet_document, default_camera());
const reloaded = check_roundtrip_is_unchanged("native-head.json", native_head_text);
check_every_patch_builds_a_fill(reloaded);
check_fans_collapse_at_their_point(converter, reloaded);
check_mirrored_stroke_ends_on_midline_plane_are_held(reloaded);
check_roundtrip_is_unchanged("skull-zanatomy.json", readFileSync(new URL("../documents/skull-zanatomy.json", import.meta.url), "utf8"));

if (process.argv.includes("--stroke-groups-only")) {
  console.log("--stroke-groups-only: documents/native-head.json not written");
} else {
  writeFileSync(new URL("../documents/native-head.json", import.meta.url), native_head_text);
  console.log("wrote documents/native-head.json");
}
