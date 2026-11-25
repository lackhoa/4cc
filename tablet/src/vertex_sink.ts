// A growable Float32Array of interleaved [x,y,z, r,g,b] vertices (or the 9-float
// shaded-surface layout below) that the
// per-frame mesh builders write into. One sink per mesh lives across frames:
// reset it, append, upload the filled prefix — no per-frame array growth and
// no number[] -> Float32Array copy for the big meshes (strokes, surfaces).

import { V3 } from "./math";

export type Rgb = { r: number; g: number; b: number };

export type VertexSink = { data: Float32Array; length: number };

export const FLOATS_PER_VERTEX = 6;

export function create_vertex_sink(initial_vertices: number): VertexSink {
  return { data: new Float32Array(initial_vertices * FLOATS_PER_VERTEX), length: 0 };
}

export function reset_vertex_sink(sink: VertexSink): void {
  sink.length = 0;
}

// Doubles the sink's storage until `float_count` more floats fit.
function ensure_room_for(sink: VertexSink, float_count: number): void {
  if (sink.length + float_count <= sink.data.length) return;
  let capacity = sink.data.length * 2;
  while (sink.length + float_count > capacity) capacity *= 2;
  const grown = new Float32Array(capacity);
  grown.set(sink.data);
  sink.data = grown;
}

export function push_vertex(sink: VertexSink, position: V3, color: Rgb): void {
  ensure_room_for(sink, FLOATS_PER_VERTEX);
  const data = sink.data;
  let cursor = sink.length;
  data[cursor++] = position.x;
  data[cursor++] = position.y;
  data[cursor++] = position.z;
  data[cursor++] = color.r;
  data[cursor++] = color.g;
  data[cursor++] = color.b;
  sink.length = cursor;
}

// push_vertex from position components, for a builder that keeps its points in flat arrays.
export function push_vertex_components(sink: VertexSink, x: number, y: number, z: number, color: Rgb): void {
  ensure_room_for(sink, FLOATS_PER_VERTEX);
  const data = sink.data;
  let cursor = sink.length;
  data[cursor++] = x;
  data[cursor++] = y;
  data[cursor++] = z;
  data[cursor++] = color.r;
  data[cursor++] = color.g;
  data[cursor++] = color.b;
  sink.length = cursor;
}

// The shaded-surface layout: interleaved [x,y,z, r,g,b, nx,ny,nz], the base color
// unshaded and the unit surface normal; the shader lights it from the camera. A zero
// normal means unlit (drawn in the base color as is).
export const FLOATS_PER_SHADED_VERTEX = 9;

// The length of the normal of a vertex that is drawn in its color as is, but whose
// direction the shader still needs (render.ts: the tilt-scaled depth push). A lit normal
// has length 1, so the shaders tell the two apart by the length.
export const UNLIT_NORMAL_WITH_DIRECTION_LENGTH = 2;

export function push_shaded_surface_vertex(sink: VertexSink, position: V3, color: Rgb, normal: V3): void {
  ensure_room_for(sink, FLOATS_PER_SHADED_VERTEX);
  const data = sink.data;
  let cursor = sink.length;
  data[cursor++] = position.x;
  data[cursor++] = position.y;
  data[cursor++] = position.z;
  data[cursor++] = color.r;
  data[cursor++] = color.g;
  data[cursor++] = color.b;
  data[cursor++] = normal.x;
  data[cursor++] = normal.y;
  data[cursor++] = normal.z;
  sink.length = cursor;
}

// The filled prefix, as a view (no copy) suitable for gl.bufferData.
export function vertex_sink_view(sink: VertexSink): Float32Array {
  return sink.data.subarray(0, sink.length);
}
