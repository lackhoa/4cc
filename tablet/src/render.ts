// Raw-WebGL line renderer. Step 2 draws a ground grid + axis triad so camera
// motion is visible; stroke ribbons and loft surfaces come in later steps.

import { Mat4, V3 } from "./math";

const VERTEX_SHADER = `
attribute vec3 a_position;
attribute vec3 a_color;
uniform mat4 u_view_projection;
varying vec3 v_color;
void main() {
  gl_Position = u_view_projection * vec4(a_position, 1.0);
  v_color = a_color;
}`;

const FRAGMENT_SHADER = `
precision mediump float;
varying vec3 v_color;
void main() {
  gl_FragColor = vec4(v_color, 1.0);
}`;

export type LineRenderer = {
  gl: WebGLRenderingContext;
  program: WebGLProgram;
  u_view_projection: WebGLUniformLocation;
  grid_buffer: WebGLBuffer;
  grid_vertex_count: number;
  reference_buffer: WebGLBuffer; // reference model, triangle list, drawn first (depth writes on)
  reference_vertex_count: number;
  stroke_buffer: WebGLBuffer; // committed stroke ribbons, triangle list
  stroke_vertex_count: number;
  surface_buffer: WebGLBuffer; // loft surfaces, triangle list, shading baked into color
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

// Interleaved [x,y,z, r,g,b] per vertex, two vertices per line segment.
function build_grid_vertices(): Float32Array {
  const lines: number[] = [];
  const extent = 5;
  const grid_gray = [0.3, 0.3, 0.32];
  for (let i = -extent; i <= extent; i++) {
    lines.push(i, 0, -extent, ...grid_gray, i, 0, extent, ...grid_gray);
    lines.push(-extent, 0, i, ...grid_gray, extent, 0, i, ...grid_gray);
  }
  const lift = 0.005; // keep axes off the grid plane so they don't z-fight the gridlines
  lines.push(0, lift, 0, 0.9, 0.25, 0.25, 1.5, lift, 0, 0.9, 0.25, 0.25); // x axis, red
  lines.push(0, lift, 0, 0.3, 0.9, 0.3, 0, 1.5, 0, 0.3, 0.9, 0.3); // y axis, green
  lines.push(0, lift, 0, 0.35, 0.5, 1.0, 0, lift, 1.5, 0.35, 0.5, 1.0); // z axis, blue
  return new Float32Array(lines);
}

export function create_line_renderer(gl: WebGLRenderingContext): LineRenderer {
  const program = gl.createProgram()!;
  gl.attachShader(program, compile_shader(gl, gl.VERTEX_SHADER, VERTEX_SHADER));
  gl.attachShader(program, compile_shader(gl, gl.FRAGMENT_SHADER, FRAGMENT_SHADER));
  gl.linkProgram(program);
  if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
    throw new Error(`program link failed: ${gl.getProgramInfoLog(program)}`);
  }

  const grid_vertices = build_grid_vertices();
  const grid_buffer = gl.createBuffer()!;
  gl.bindBuffer(gl.ARRAY_BUFFER, grid_buffer);
  gl.bufferData(gl.ARRAY_BUFFER, grid_vertices, gl.STATIC_DRAW);

  return {
    gl,
    program,
    u_view_projection: gl.getUniformLocation(program, "u_view_projection")!,
    grid_buffer,
    grid_vertex_count: grid_vertices.length / 6,
    reference_buffer: gl.createBuffer()!,
    reference_vertex_count: 0,
    stroke_buffer: gl.createBuffer()!,
    stroke_vertex_count: 0,
    surface_buffer: gl.createBuffer()!,
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

// vertices: interleaved [x,y,z, r,g,b] triangle list of all stroke ribbons.
export function set_stroke_mesh(renderer: LineRenderer, vertices: Float32Array): void {
  const gl = renderer.gl;
  gl.bindBuffer(gl.ARRAY_BUFFER, renderer.stroke_buffer);
  gl.bufferData(gl.ARRAY_BUFFER, vertices, gl.DYNAMIC_DRAW);
  renderer.stroke_vertex_count = vertices.length / 6;
}

// vertices: interleaved [x,y,z, r,g,b] triangle list of all loft surfaces.
export function set_surface_mesh(renderer: LineRenderer, vertices: Float32Array): void {
  const gl = renderer.gl;
  gl.bindBuffer(gl.ARRAY_BUFFER, renderer.surface_buffer);
  gl.bufferData(gl.ARRAY_BUFFER, vertices, gl.DYNAMIC_DRAW);
  renderer.surface_vertex_count = vertices.length / 6;
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

export function render_frame(renderer: LineRenderer, view_projection: Mat4): void {
  const gl = renderer.gl;
  // Desktop app background: tweaks->background_rgb = gray 0.12 linear -> 0.384 sRGB.
  gl.clearColor(0.384, 0.384, 0.384, 1.0);
  gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
  gl.enable(gl.DEPTH_TEST);

  gl.useProgram(renderer.program);
  gl.uniformMatrix4fv(renderer.u_view_projection, false, view_projection);

  bind_interleaved_buffer(renderer, renderer.grid_buffer);
  gl.drawArrays(gl.LINES, 0, renderer.grid_vertex_count);

  if (renderer.reference_vertex_count > 0) {
    bind_interleaved_buffer(renderer, renderer.reference_buffer);
    gl.drawArrays(gl.TRIANGLES, 0, renderer.reference_vertex_count);
  }

  if (renderer.surface_vertex_count > 0) {
    bind_interleaved_buffer(renderer, renderer.surface_buffer);
    gl.drawArrays(gl.TRIANGLES, 0, renderer.surface_vertex_count);
  }
  if (renderer.stroke_vertex_count > 0) {
    bind_interleaved_buffer(renderer, renderer.stroke_buffer);
    gl.drawArrays(gl.TRIANGLES, 0, renderer.stroke_vertex_count);
  }
  if (renderer.preview_vertex_count > 1) {
    bind_interleaved_buffer(renderer, renderer.preview_buffer);
    gl.drawArrays(gl.LINE_STRIP, 0, renderer.preview_vertex_count);
  }

  // Edit-mode overlay: always on top of the geometry.
  if (renderer.overlay_line_vertex_count > 0 || renderer.overlay_triangle_vertex_count > 0) {
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
void main() {
  gl_Position = u_view_projection * vec4(a_position, 1.0);
  v_color = a_color;
}`;

const TRANSLUCENT_FRAGMENT_SHADER = `
precision mediump float;
varying vec4 v_color;
void main() {
  gl_FragColor = vec4(v_color.rgb * v_color.a, v_color.a);
}`;

export const FLOATS_PER_TRANSLUCENT_VERTEX = 7; // x y z r g b a

export type TranslucentMesh = {
  gl: WebGLRenderingContext;
  program: WebGLProgram;
  u_view_projection: WebGLUniformLocation;
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
export function draw_mesh_translucent(mesh: TranslucentMesh, view_projection: Mat4, eye: V3): void {
  const vertex_count = mesh.vertices.length / FLOATS_PER_TRANSLUCENT_VERTEX;
  if (vertex_count === 0) return;
  const eye_moved = mesh.sorted_eye === null
    || mesh.sorted_eye.x !== eye.x || mesh.sorted_eye.y !== eye.y || mesh.sorted_eye.z !== eye.z;
  if (eye_moved) sort_translucent_mesh(mesh, eye);
  const gl = mesh.gl;
  gl.useProgram(mesh.program);
  gl.uniformMatrix4fv(mesh.u_view_projection, false, view_projection);
  const stride = FLOATS_PER_TRANSLUCENT_VERTEX * 4;
  gl.bindBuffer(gl.ARRAY_BUFFER, mesh.buffer);
  const a_position = gl.getAttribLocation(mesh.program, "a_position");
  const a_color = gl.getAttribLocation(mesh.program, "a_color");
  gl.enableVertexAttribArray(a_position);
  gl.vertexAttribPointer(a_position, 3, gl.FLOAT, false, stride, 0);
  gl.enableVertexAttribArray(a_color);
  gl.vertexAttribPointer(a_color, 4, gl.FLOAT, false, stride, 3 * 4);
  gl.enable(gl.DEPTH_TEST);
  gl.depthMask(false);
  gl.enable(gl.BLEND);
  gl.blendFunc(gl.ONE, gl.ONE_MINUS_SRC_ALPHA); // premultiplied in the fragment shader
  gl.drawArrays(gl.TRIANGLES, 0, vertex_count);
  gl.disable(gl.BLEND);
  gl.depthMask(true);
}
