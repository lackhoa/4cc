// Raw-WebGL renderer: reference model, stroke ribbons, fills and the edit overlay.

import { Mat4, V3 } from "./math";
import { FLOATS_PER_SHADED_VERTEX } from "./vertex_sink";

// One clipping plane (plan-skin-over-skull-study.md Q5): fragments with
// dot(world, normal) > offset are discarded, in every program that draws geometry
// (not the edit overlay). WebGL 1 has no gl_ClipDistance, hence the
// fragment-shader discard.
export type ClipPlane = { normal: V3; offset: number; enabled: boolean };

const CLIP_SHADER_UNIFORMS = `
uniform vec4 u_clip_plane; // xyz = normal, w = offset
uniform float u_clip_enabled;`;
const CLIP_SHADER_DISCARD = `
  if (u_clip_enabled > 0.5 && dot(v_world_position, u_clip_plane.xyz) > u_clip_plane.w) discard;`;

const VERTEX_SHADER = `
attribute vec3 a_position;
attribute vec3 a_color;
uniform mat4 u_view_projection;
varying vec3 v_color;
varying vec3 v_world_position;
void main() {
  gl_Position = u_view_projection * vec4(a_position, 1.0);
  v_color = a_color;
  v_world_position = a_position;
}`;

const FRAGMENT_SHADER = `
precision mediump float;
varying vec3 v_color;
varying vec3 v_world_position;
${CLIP_SHADER_UNIFORMS}
void main() {${CLIP_SHADER_DISCARD}
  gl_FragColor = vec4(v_color, 1.0);
}`;

// Headlight on the patch fills and the spheres: brightness
// SURFACE_AMBIENT + (1 - SURFACE_AMBIENT) * |dot(unit normal, camera forward)|, two-sided
// through the abs. Lit by SHADED_SURFACE_VERTEX_SHADER; patch.ts brightness_of_normal is
// the same formula CPU-side. (The reference model is lit CPU-side, reference.ts.)
export const SURFACE_AMBIENT = 0.35;

// The fill program: per-vertex base color and unit normal (FLOATS_PER_SHADED_VERTEX),
// lit per vertex against u_camera_forward and the lit color interpolated, as the
// CPU-baked colors were. A zero normal (degenerate triangle, flat fill) is unlit.
const SHADED_SURFACE_VERTEX_SHADER = `
attribute vec3 a_position;
attribute vec3 a_color;
attribute vec3 a_normal;
uniform mat4 u_view_projection;
uniform vec3 u_camera_forward;
varying vec3 v_color;
varying vec3 v_world_position;
void main() {
  gl_Position = u_view_projection * vec4(a_position, 1.0);
  float facing = abs(dot(a_normal, u_camera_forward));
  float brightness = dot(a_normal, a_normal) > 0.0 ? ${SURFACE_AMBIENT.toFixed(6)} + ${(1 - SURFACE_AMBIENT).toFixed(6)} * facing : 1.0;
  v_color = a_color * brightness;
  v_world_position = a_position;
}`;

function set_clip_plane_uniforms(gl: WebGLRenderingContext, u_clip_plane: WebGLUniformLocation, u_clip_enabled: WebGLUniformLocation, clip: ClipPlane | null): void {
  if (clip === null || !clip.enabled) {
    gl.uniform1f(u_clip_enabled, 0);
    return;
  }
  gl.uniform4f(u_clip_plane, clip.normal.x, clip.normal.y, clip.normal.z, clip.offset);
  gl.uniform1f(u_clip_enabled, 1);
}

export type LineRenderer = {
  gl: WebGLRenderingContext;
  program: WebGLProgram;
  u_view_projection: WebGLUniformLocation;
  u_clip_plane: WebGLUniformLocation;
  u_clip_enabled: WebGLUniformLocation;
  reference_buffer: WebGLBuffer; // reference model, triangle list, drawn first (depth writes on)
  reference_vertex_count: number;
  stroke_buffer: WebGLBuffer; // committed stroke ribbons, triangle list
  stroke_buffer_capacity_bytes: number; // see upload_into_growing_buffer
  stroke_vertex_count: number;
  // Patch fills and spheres, triangle list in the shaded-surface layout, drawn with
  // surface_program and lit from surface_camera_forward.
  surface_program: WebGLProgram;
  surface_u_view_projection: WebGLUniformLocation;
  surface_u_clip_plane: WebGLUniformLocation;
  surface_u_clip_enabled: WebGLUniformLocation;
  surface_u_camera_forward: WebGLUniformLocation;
  surface_a_position: number;
  surface_a_color: number;
  surface_a_normal: number;
  surface_camera_forward: V3;
  surface_buffer: WebGLBuffer;
  surface_buffer_capacity_bytes: number; // see upload_into_growing_buffer
  surface_vertex_count: number;
  preview_buffer: WebGLBuffer; // in-progress pen stroke, line strip
  preview_vertex_count: number;
  overlay_line_buffer: WebGLBuffer; // edit-mode handle lines, drawn depth-test-off
  overlay_line_vertex_count: number;
  overlay_triangle_buffer: WebGLBuffer; // edit-mode anchor/handle squares, depth-test-off
  overlay_triangle_vertex_count: number;
};

function compile_shader(gl: WebGLRenderingContext, type: number, source: string): WebGLShader {
  const shader = gl.createShader(type)!;
  gl.shaderSource(shader, source);
  gl.compileShader(shader);
  if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
    throw new Error(`shader compile failed: ${gl.getShaderInfoLog(shader)}`);
  }
  return shader;
}

function link_program(gl: WebGLRenderingContext, vertex_source: string, fragment_source: string): WebGLProgram {
  const program = gl.createProgram()!;
  gl.attachShader(program, compile_shader(gl, gl.VERTEX_SHADER, vertex_source));
  gl.attachShader(program, compile_shader(gl, gl.FRAGMENT_SHADER, fragment_source));
  gl.linkProgram(program);
  if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
    throw new Error(`program link failed: ${gl.getProgramInfoLog(program)}`);
  }
  return program;
}

export function create_line_renderer(gl: WebGLRenderingContext): LineRenderer {
  const program = link_program(gl, VERTEX_SHADER, FRAGMENT_SHADER);
  const surface_program = link_program(gl, SHADED_SURFACE_VERTEX_SHADER, FRAGMENT_SHADER);

  return {
    gl,
    program,
    u_view_projection: gl.getUniformLocation(program, "u_view_projection")!,
    u_clip_plane: gl.getUniformLocation(program, "u_clip_plane")!,
    u_clip_enabled: gl.getUniformLocation(program, "u_clip_enabled")!,
    reference_buffer: gl.createBuffer()!,
    reference_vertex_count: 0,
    stroke_buffer: gl.createBuffer()!,
    stroke_buffer_capacity_bytes: 0,
    stroke_vertex_count: 0,
    surface_program,
    surface_u_view_projection: gl.getUniformLocation(surface_program, "u_view_projection")!,
    surface_u_clip_plane: gl.getUniformLocation(surface_program, "u_clip_plane")!,
    surface_u_clip_enabled: gl.getUniformLocation(surface_program, "u_clip_enabled")!,
    surface_u_camera_forward: gl.getUniformLocation(surface_program, "u_camera_forward")!,
    surface_a_position: gl.getAttribLocation(surface_program, "a_position"),
    surface_a_color: gl.getAttribLocation(surface_program, "a_color"),
    surface_a_normal: gl.getAttribLocation(surface_program, "a_normal"),
    surface_camera_forward: { x: 0, y: 0, z: 1 },
    surface_buffer: gl.createBuffer()!,
    surface_buffer_capacity_bytes: 0,
    surface_vertex_count: 0,
    preview_buffer: gl.createBuffer()!,
    preview_vertex_count: 0,
    overlay_line_buffer: gl.createBuffer()!,
    overlay_line_vertex_count: 0,
    overlay_triangle_buffer: gl.createBuffer()!,
    overlay_triangle_vertex_count: 0,
  };
}

// vertices: interleaved [x,y,z, r,g,b] triangle list of the reference model;
// empty array hides it.
export function set_reference_mesh(renderer: LineRenderer, vertices: Float32Array): void {
  const gl = renderer.gl;
  gl.bindBuffer(gl.ARRAY_BUFFER, renderer.reference_buffer);
  gl.bufferData(gl.ARRAY_BUFFER, vertices, gl.DYNAMIC_DRAW);
  renderer.reference_vertex_count = vertices.length / 6;
}

// Writes `vertices` to the front of `buffer`, whose storage only grows: an upload that fits
// in `capacity_bytes` goes in with bufferSubData, a bigger one first reallocates the storage
// (at least doubled). The draw reads only the vertex count, never the bytes past it.
// Returns the buffer's capacity afterwards.
function upload_into_growing_buffer(gl: WebGLRenderingContext, buffer: WebGLBuffer, capacity_bytes: number, vertices: Float32Array): number {
  gl.bindBuffer(gl.ARRAY_BUFFER, buffer);
  let capacity_after = capacity_bytes;
  if (vertices.byteLength > capacity_bytes) {
    capacity_after = Math.max(vertices.byteLength, 2 * capacity_bytes);
    gl.bufferData(gl.ARRAY_BUFFER, capacity_after, gl.DYNAMIC_DRAW);
  }
  if (vertices.byteLength > 0) gl.bufferSubData(gl.ARRAY_BUFFER, 0, vertices);
  return capacity_after;
}

// vertices: interleaved [x,y,z, r,g,b] triangle list of all stroke ribbons.
export function set_stroke_mesh(renderer: LineRenderer, vertices: Float32Array): void {
  renderer.stroke_buffer_capacity_bytes = upload_into_growing_buffer(renderer.gl, renderer.stroke_buffer, renderer.stroke_buffer_capacity_bytes, vertices);
  renderer.stroke_vertex_count = vertices.length / 6;
}

// vertices: triangle list of all patch fills and spheres in the shaded-surface layout
// (vertex_sink.ts FLOATS_PER_SHADED_VERTEX: position, base color, unit normal).
export function set_surface_mesh(renderer: LineRenderer, vertices: Float32Array): void {
  renderer.surface_buffer_capacity_bytes = upload_into_growing_buffer(renderer.gl, renderer.surface_buffer, renderer.surface_buffer_capacity_bytes, vertices);
  renderer.surface_vertex_count = vertices.length / FLOATS_PER_SHADED_VERTEX;
}

// The camera forward the fills are lit from in the next render_frame.
export function set_surface_camera_forward(renderer: LineRenderer, camera_forward: V3): void {
  renderer.surface_camera_forward = camera_forward;
}

// vertices: interleaved [x,y,z, r,g,b] line strip; empty array clears the preview.
export function set_preview_line(renderer: LineRenderer, vertices: Float32Array): void {
  const gl = renderer.gl;
  gl.bindBuffer(gl.ARRAY_BUFFER, renderer.preview_buffer);
  gl.bufferData(gl.ARRAY_BUFFER, vertices, gl.DYNAMIC_DRAW);
  renderer.preview_vertex_count = vertices.length / 6;
}

// vertices: interleaved [x,y,z, r,g,b] line list (pairs); empty array clears.
export function set_overlay_lines(renderer: LineRenderer, vertices: Float32Array): void {
  const gl = renderer.gl;
  gl.bindBuffer(gl.ARRAY_BUFFER, renderer.overlay_line_buffer);
  gl.bufferData(gl.ARRAY_BUFFER, vertices, gl.DYNAMIC_DRAW);
  renderer.overlay_line_vertex_count = vertices.length / 6;
}

// vertices: interleaved [x,y,z, r,g,b] triangle list; empty array clears.
export function set_overlay_triangles(renderer: LineRenderer, vertices: Float32Array): void {
  const gl = renderer.gl;
  gl.bindBuffer(gl.ARRAY_BUFFER, renderer.overlay_triangle_buffer);
  gl.bufferData(gl.ARRAY_BUFFER, vertices, gl.DYNAMIC_DRAW);
  renderer.overlay_triangle_vertex_count = vertices.length / 6;
}

function bind_interleaved_buffer(renderer: LineRenderer, buffer: WebGLBuffer): void {
  const gl = renderer.gl;
  const stride = 6 * 4;
  gl.bindBuffer(gl.ARRAY_BUFFER, buffer);
  const a_position = gl.getAttribLocation(renderer.program, "a_position");
  const a_color = gl.getAttribLocation(renderer.program, "a_color");
  gl.enableVertexAttribArray(a_position);
  gl.vertexAttribPointer(a_position, 3, gl.FLOAT, false, stride, 0);
  gl.enableVertexAttribArray(a_color);
  gl.vertexAttribPointer(a_color, 3, gl.FLOAT, false, stride, 3 * 4);
}

function draw_shaded_surfaces(renderer: LineRenderer, view_projection: Mat4, clip: ClipPlane | null): void {
  const gl = renderer.gl;
  const forward = renderer.surface_camera_forward;
  gl.useProgram(renderer.surface_program);
  gl.uniformMatrix4fv(renderer.surface_u_view_projection, false, view_projection);
  gl.uniform3f(renderer.surface_u_camera_forward, forward.x, forward.y, forward.z);
  set_clip_plane_uniforms(gl, renderer.surface_u_clip_plane, renderer.surface_u_clip_enabled, clip);
  const stride = FLOATS_PER_SHADED_VERTEX * 4;
  gl.bindBuffer(gl.ARRAY_BUFFER, renderer.surface_buffer);
  gl.enableVertexAttribArray(renderer.surface_a_position);
  gl.vertexAttribPointer(renderer.surface_a_position, 3, gl.FLOAT, false, stride, 0);
  gl.enableVertexAttribArray(renderer.surface_a_color);
  gl.vertexAttribPointer(renderer.surface_a_color, 3, gl.FLOAT, false, stride, 3 * 4);
  gl.enableVertexAttribArray(renderer.surface_a_normal);
  gl.vertexAttribPointer(renderer.surface_a_normal, 3, gl.FLOAT, false, stride, 6 * 4);
  gl.drawArrays(gl.TRIANGLES, 0, renderer.surface_vertex_count);
  // The other programs read two attributes: leave no third array enabled behind them.
  gl.disableVertexAttribArray(renderer.surface_a_normal);
}

// `clip`: the clipping plane for the geometry (null = none). `draw_translucent`: the
// caller's blended pass (the translucent reference), run after the opaque geometry and
// before the overlay so handles stay on top of it.
export function render_frame(renderer: LineRenderer, view_projection: Mat4, clip: ClipPlane | null = null, draw_translucent: (() => void) | null = null): void {
  const gl = renderer.gl;
  // Desktop app background: tweaks->background_rgb = gray 0.12 linear -> 0.384 sRGB.
  gl.clearColor(0.384, 0.384, 0.384, 1.0);
  gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
  gl.enable(gl.DEPTH_TEST);

  gl.useProgram(renderer.program);
  gl.uniformMatrix4fv(renderer.u_view_projection, false, view_projection);

  set_clip_plane_uniforms(gl, renderer.u_clip_plane, renderer.u_clip_enabled, clip);

  if (renderer.reference_vertex_count > 0) {
    bind_interleaved_buffer(renderer, renderer.reference_buffer);
    gl.drawArrays(gl.TRIANGLES, 0, renderer.reference_vertex_count);
  }

  if (renderer.surface_vertex_count > 0) {
    draw_shaded_surfaces(renderer, view_projection, clip);
    gl.useProgram(renderer.program);
  }
  if (renderer.stroke_vertex_count > 0) {
    bind_interleaved_buffer(renderer, renderer.stroke_buffer);
    gl.drawArrays(gl.TRIANGLES, 0, renderer.stroke_vertex_count);
  }
  if (renderer.preview_vertex_count > 1) {
    bind_interleaved_buffer(renderer, renderer.preview_buffer);
    gl.drawArrays(gl.LINE_STRIP, 0, renderer.preview_vertex_count);
  }

  if (draw_translucent !== null) {
    draw_translucent();
    gl.useProgram(renderer.program);
  }

  // Edit-mode overlay: always on top of the geometry, never clipped.
  if (renderer.overlay_line_vertex_count > 0 || renderer.overlay_triangle_vertex_count > 0) {
    set_clip_plane_uniforms(gl, renderer.u_clip_plane, renderer.u_clip_enabled, null);
    gl.disable(gl.DEPTH_TEST);
    if (renderer.overlay_line_vertex_count > 0) {
      bind_interleaved_buffer(renderer, renderer.overlay_line_buffer);
      gl.drawArrays(gl.LINES, 0, renderer.overlay_line_vertex_count);
    }
    if (renderer.overlay_triangle_vertex_count > 0) {
      bind_interleaved_buffer(renderer, renderer.overlay_triangle_buffer);
      gl.drawArrays(gl.TRIANGLES, 0, renderer.overlay_triangle_vertex_count);
    }
    gl.enable(gl.DEPTH_TEST);
  }
}

// Translucent triangle mesh (plan-skull-construction-docs Q5, Q11): per-vertex color +
// alpha, blended over whatever was drawn before, painter-sorted back to front once per
// camera move (depth writes off, so the sort is what makes overlapping triangles look
// right). Its own program because the line renderer's vertices carry no alpha.
const TRANSLUCENT_VERTEX_SHADER = `
attribute vec3 a_position;
attribute vec4 a_color;
uniform mat4 u_view_projection;
varying vec4 v_color;
varying vec3 v_world_position;
void main() {
  gl_Position = u_view_projection * vec4(a_position, 1.0);
  v_color = a_color;
  v_world_position = a_position;
}`;

const TRANSLUCENT_FRAGMENT_SHADER = `
precision mediump float;
varying vec4 v_color;
varying vec3 v_world_position;
${CLIP_SHADER_UNIFORMS}
void main() {${CLIP_SHADER_DISCARD}
  gl_FragColor = vec4(v_color.rgb * v_color.a, v_color.a);
}`;

export const FLOATS_PER_TRANSLUCENT_VERTEX = 7; // x y z r g b a

export type TranslucentMesh = {
  gl: WebGLRenderingContext;
  program: WebGLProgram;
  u_view_projection: WebGLUniformLocation;
  u_clip_plane: WebGLUniformLocation;
  u_clip_enabled: WebGLUniformLocation;
  buffer: WebGLBuffer;
  vertices: Float32Array; // unsorted, FLOATS_PER_TRANSLUCENT_VERTEX per vertex, triangle list
  sorted_vertices: Float32Array; // same triangles, back to front for sorted_eye
  sorted_eye: V3 | null; // eye the upload in `buffer` was sorted for; null = needs a sort
};

export function create_translucent_mesh(gl: WebGLRenderingContext): TranslucentMesh {
  const program = gl.createProgram()!;
  gl.attachShader(program, compile_shader(gl, gl.VERTEX_SHADER, TRANSLUCENT_VERTEX_SHADER));
  gl.attachShader(program, compile_shader(gl, gl.FRAGMENT_SHADER, TRANSLUCENT_FRAGMENT_SHADER));
  gl.linkProgram(program);
  if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
    throw new Error(`program link failed: ${gl.getProgramInfoLog(program)}`);
  }
  return {
    gl,
    program,
    u_view_projection: gl.getUniformLocation(program, "u_view_projection")!,
    u_clip_plane: gl.getUniformLocation(program, "u_clip_plane")!,
    u_clip_enabled: gl.getUniformLocation(program, "u_clip_enabled")!,
    buffer: gl.createBuffer()!,
    vertices: new Float32Array(0),
    sorted_vertices: new Float32Array(0),
    sorted_eye: null,
  };
}

// vertices: interleaved [x,y,z, r,g,b,a] triangle list; empty array hides the mesh.
export function set_translucent_mesh(mesh: TranslucentMesh, vertices: Float32Array): void {
  mesh.vertices = vertices;
  mesh.sorted_vertices = new Float32Array(vertices.length);
  mesh.sorted_eye = null;
}

function sort_translucent_mesh(mesh: TranslucentMesh, eye: V3): void {
  const stride = FLOATS_PER_TRANSLUCENT_VERTEX;
  const triangle_count = mesh.vertices.length / (3 * stride);
  const distances = new Float64Array(triangle_count);
  const order = new Uint32Array(triangle_count);
  for (let triangle = 0; triangle < triangle_count; triangle++) {
    const base = triangle * 3 * stride;
    const x = (mesh.vertices[base] + mesh.vertices[base + stride] + mesh.vertices[base + 2 * stride]) / 3 - eye.x;
    const y = (mesh.vertices[base + 1] + mesh.vertices[base + stride + 1] + mesh.vertices[base + 2 * stride + 1]) / 3 - eye.y;
    const z = (mesh.vertices[base + 2] + mesh.vertices[base + stride + 2] + mesh.vertices[base + 2 * stride + 2]) / 3 - eye.z;
    distances[triangle] = x * x + y * y + z * z;
    order[triangle] = triangle;
  }
  order.sort((a, b) => distances[b] - distances[a]);
  for (let slot = 0; slot < triangle_count; slot++) {
    const source = order[slot] * 3 * stride;
    mesh.sorted_vertices.set(mesh.vertices.subarray(source, source + 3 * stride), slot * 3 * stride);
  }
  const gl = mesh.gl;
  gl.bindBuffer(gl.ARRAY_BUFFER, mesh.buffer);
  gl.bufferData(gl.ARRAY_BUFFER, mesh.sorted_vertices, gl.DYNAMIC_DRAW);
  mesh.sorted_eye = { x: eye.x, y: eye.y, z: eye.z };
}

// Draws over the current framebuffer contents (call after the opaque geometry); depth
// test on so opaque things in front still hide it, depth writes off.
export function draw_mesh_translucent(mesh: TranslucentMesh, view_projection: Mat4, eye: V3, clip: ClipPlane | null = null): void {
  draw_translucent_mesh_triangles(mesh, view_projection, eye, clip, false, 0);
}

// The same draw with depth writes on, so what is drawn later and lies behind this mesh is
// hidden by it, whatever its own alpha. `depth_offset` > 0 pushes the mesh back by that
// many depth units, so a later mesh lying ON its surface (strokes on the surface they bound)
// wins the depth test instead of z-fighting.
export function draw_mesh_translucent_writing_depth(mesh: TranslucentMesh, view_projection: Mat4, eye: V3, depth_offset: number): void {
  draw_translucent_mesh_triangles(mesh, view_projection, eye, null, true, depth_offset);
}

// The same draw with the depth test off: the mesh shows through everything drawn before
// it (markers that must stay visible inside or behind a surface).
export function draw_mesh_translucent_on_top(mesh: TranslucentMesh, view_projection: Mat4, eye: V3): void {
  draw_translucent_mesh_triangles(mesh, view_projection, eye, null, false, 0, false);
}

function draw_translucent_mesh_triangles(mesh: TranslucentMesh, view_projection: Mat4, eye: V3, clip: ClipPlane | null, writes_depth: boolean, depth_offset: number, depth_tested = true): void {
  const vertex_count = mesh.vertices.length / FLOATS_PER_TRANSLUCENT_VERTEX;
  if (vertex_count === 0) return;
  const eye_moved = mesh.sorted_eye === null
    || mesh.sorted_eye.x !== eye.x || mesh.sorted_eye.y !== eye.y || mesh.sorted_eye.z !== eye.z;
  if (eye_moved) sort_translucent_mesh(mesh, eye);
  const gl = mesh.gl;
  gl.useProgram(mesh.program);
  gl.uniformMatrix4fv(mesh.u_view_projection, false, view_projection);
  set_clip_plane_uniforms(gl, mesh.u_clip_plane, mesh.u_clip_enabled, clip);
  const stride = FLOATS_PER_TRANSLUCENT_VERTEX * 4;
  gl.bindBuffer(gl.ARRAY_BUFFER, mesh.buffer);
  const a_position = gl.getAttribLocation(mesh.program, "a_position");
  const a_color = gl.getAttribLocation(mesh.program, "a_color");
  gl.enableVertexAttribArray(a_position);
  gl.vertexAttribPointer(a_position, 3, gl.FLOAT, false, stride, 0);
  gl.enableVertexAttribArray(a_color);
  gl.vertexAttribPointer(a_color, 4, gl.FLOAT, false, stride, 3 * 4);
  if (depth_tested) gl.enable(gl.DEPTH_TEST);
  else gl.disable(gl.DEPTH_TEST);
  gl.depthMask(writes_depth);
  if (depth_offset > 0) {
    gl.enable(gl.POLYGON_OFFSET_FILL);
    gl.polygonOffset(1, depth_offset);
  }
  gl.enable(gl.BLEND);
  gl.blendFunc(gl.ONE, gl.ONE_MINUS_SRC_ALPHA); // premultiplied in the fragment shader
  gl.drawArrays(gl.TRIANGLES, 0, vertex_count);
  gl.disable(gl.BLEND);
  gl.disable(gl.POLYGON_OFFSET_FILL);
  gl.depthMask(true);
  gl.enable(gl.DEPTH_TEST);
}
