// A growable Float32Array of interleaved [x,y,z, r,g,b] vertices that the
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

export function push_vertex(sink: VertexSink, position: V3, color: Rgb): void {
  if (sink.length + FLOATS_PER_VERTEX > sink.data.length) {
    const grown = new Float32Array(sink.data.length * 2);
    grown.set(sink.data);
    sink.data = grown;
  }
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

// The filled prefix, as a view (no copy) suitable for gl.bufferData.
export function vertex_sink_view(sink: VertexSink): Float32Array {
  return sink.data.subarray(0, sink.length);
}
