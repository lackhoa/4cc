// Shared toolkit for the interactive explainer docs (interactive-*.html).
// Load with <script src="explainer/explainer.js"></script> before the page script;
// everything lands on the global `EX`. Plain 2D canvas, no WebGL, no build step.
//
// Recipe for a new doc:
//   const ui = EX.bind_controls(["grid", "rootfind"], draw_all);   // ui.grid.value
//   const camera = EX.orbit_camera();                              // shared orbit
//   EX.attach_orbit(canvases, camera, draw_all);                   // drag + wheel
//   function draw_all() {
//     const { ctx, dpr } = EX.fit_canvas(canvas);                  // device pixels
//     const view = EX.viewer(camera, canvas);                      // view.project(p)
//     EX.draw_surface(ctx, view, (u, v) => EX.coons(B, u, v), 16, quad => color);
//   }
const EX = (() => {
  // ---------- v3 ----------
  const v3 = (x, y, z) => ({ x, y, z });
  const add = (a, b) => v3(a.x + b.x, a.y + b.y, a.z + b.z);
  const sub = (a, b) => v3(a.x - b.x, a.y - b.y, a.z - b.z);
  const scale = (a, s) => v3(a.x * s, a.y * s, a.z * s);
  const dot = (a, b) => a.x * b.x + a.y * b.y + a.z * b.z;
  const cross = (a, b) => v3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
  const length = (a) => Math.hypot(a.x, a.y, a.z);
  const normalize = (a) => { const l = length(a); return l === 0 ? v3(0, 0, 0) : scale(a, 1 / l); };
  const lerp = (a, b, t) => add(scale(a, 1 - t), scale(b, t));

  // ---------- curves and surfaces ----------
  const bernstein = (t) => { const s = 1 - t; return [s * s * s, 3 * s * s * t, 3 * s * t * t, t * t * t]; };
  function bezier(control, t) {
    const w = bernstein(t);
    return add(add(scale(control[0], w[0]), scale(control[1], w[1])), add(scale(control[2], w[2]), scale(control[3], w[3])));
  }
  // Coons patch from 4 cubic boundaries { bottom, top, left, right }, each an array
  // of 4 control points. bottom/top run in u, left/right run in v; corners must agree.
  // A collapsed side (3-sided patch) is a boundary whose 4 points are all the same.
  function coons(B, u, v) {
    const bottom = bezier(B.bottom, u), top = bezier(B.top, u);
    const left = bezier(B.left, v), right = bezier(B.right, v);
    const c00 = B.bottom[0], c10 = B.bottom[3], c01 = B.top[0], c11 = B.top[3];
    const bilinear = lerp(lerp(c00, c10, u), lerp(c01, c11, u), v);
    return sub(add(lerp(bottom, top, v), lerp(left, right, u)), bilinear);
  }
  // Un-normalized normal of S(u,v) by central differences (clamped to [0,1]).
  // Its length doubles as the degeneracy test at collapsed corners.
  function raw_normal(S, u, v, h = 1e-3) {
    const du = scale(sub(S(Math.min(u + h, 1), v), S(Math.max(u - h, 0), v)), 1 / (2 * h));
    const dv = scale(sub(S(u, Math.min(v + h, 1)), S(u, Math.max(v - h, 0))), 1 / (2 * h));
    return cross(du, dv);
  }

  // ---------- camera ----------
  const orbit_camera = (theta = 0.55, phi = 0.7, distance = 5.2) => ({ theta, phi, distance });
  function camera_eye(camera) {
    return v3(
      camera.distance * Math.cos(camera.theta) * Math.sin(camera.phi),
      camera.distance * Math.sin(camera.theta),
      camera.distance * Math.cos(camera.theta) * Math.cos(camera.phi),
    );
  }
  // Perspective projection into device pixels of `canvas` (call after fit_canvas).
  function viewer(camera, canvas) {
    const eye = camera_eye(camera);
    const forward = normalize(scale(eye, -1));
    const right = normalize(cross(forward, v3(0, 1, 0)));
    const up = cross(right, forward);
    const focal = canvas.height * 1.1;
    const project = (p) => {
      const d = sub(p, eye);
      const depth = dot(d, forward);
      return { x: canvas.width / 2 + dot(d, right) / depth * focal, y: canvas.height / 2 - dot(d, up) / depth * focal, depth };
    };
    return { eye, project };
  }
  // Drag to orbit, wheel to zoom; every canvas in the list drives the same camera.
  function attach_orbit(canvases, camera, on_change) {
    for (const canvas of canvases) {
      let last = null;
      canvas.addEventListener("pointerdown", (e) => { last = { x: e.clientX, y: e.clientY }; canvas.setPointerCapture(e.pointerId); });
      canvas.addEventListener("pointermove", (e) => {
        if (last === null) return;
        camera.phi -= (e.clientX - last.x) * 0.008;
        camera.theta = Math.max(-1.4, Math.min(1.4, camera.theta + (e.clientY - last.y) * 0.008));
        last = { x: e.clientX, y: e.clientY };
        on_change();
      });
      canvas.addEventListener("pointerup", () => { last = null; });
      canvas.addEventListener("wheel", (e) => {
        e.preventDefault();
        camera.distance = Math.max(2.5, Math.min(12, camera.distance * Math.exp(e.deltaY * 0.001)));
        on_change();
      }, { passive: false });
    }
    window.addEventListener("resize", on_change);
  }

  // ---------- canvas ----------
  // Sizes the backing store to devicePixelRatio and clears it. The drawing
  // coordinates are device pixels (multiply CSS sizes by `dpr`).
  function fit_canvas(canvas) {
    const dpr = window.devicePixelRatio || 1;
    // Setting canvas.height rewrites the height attribute, so remember the CSS height once.
    if (!canvas.dataset.cssHeight) canvas.dataset.cssHeight = canvas.getAttribute("height");
    const height = Number(canvas.dataset.cssHeight);
    canvas.width = Math.round(canvas.clientWidth * dpr);
    canvas.height = Math.round(height * dpr);
    canvas.style.height = `${height}px`;
    const ctx = canvas.getContext("2d");
    ctx.setTransform(1, 0, 0, 1, 0, 0);
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    return { ctx, dpr };
  }
  // `ids` are element ids; every "input" event calls `on_change`. Returns { id: element }.
  function bind_controls(ids, on_change) {
    const ui = {};
    for (const id of ids) {
      ui[id] = document.getElementById(id);
      ui[id].addEventListener("input", on_change);
    }
    return ui;
  }
  const rgba = (c, a = 1) => `rgba(${c.map((x) => Math.round(Math.max(0, Math.min(255, x)))).join(",")},${a})`;

  // ---------- drawing ----------
  // Tessellates S on a grid×grid net, painter-sorts the quads back to front and
  // fills each with style(quad) -> { fill, stroke? } where quad = { i, j, u, v, corners }.
  function draw_surface(ctx, view, S, grid, style) {
    const quads = [];
    for (let j = 0; j < grid; j++) for (let i = 0; i < grid; i++) {
      const corners = [[i, j], [i + 1, j], [i + 1, j + 1], [i, j + 1]].map(([a, b]) => S(a / grid, b / grid));
      const center = scale(corners.reduce(add, v3(0, 0, 0)), 0.25);
      quads.push({ i, j, u: (i + 0.5) / grid, v: (j + 0.5) / grid, corners, depth: view.project(center).depth });
    }
    quads.sort((a, b) => b.depth - a.depth);
    for (const q of quads) {
      const s = style(q);
      fill_polygon(ctx, q.corners.map(view.project), s.fill, s.stroke ?? "rgba(20,22,28,0.6)");
    }
  }
  function fill_polygon(ctx, points, fill, stroke) {
    ctx.beginPath();
    points.forEach((p, k) => (k === 0 ? ctx.moveTo(p.x, p.y) : ctx.lineTo(p.x, p.y)));
    ctx.closePath();
    ctx.fillStyle = fill;
    ctx.fill();
    if (stroke) { ctx.strokeStyle = stroke; ctx.lineWidth = 1; ctx.stroke(); }
  }
  function stroke_polyline(ctx, points, color, width) {
    if (points.length < 2) return;
    ctx.beginPath();
    points.forEach((p, k) => (k === 0 ? ctx.moveTo(p.x, p.y) : ctx.lineTo(p.x, p.y)));
    ctx.strokeStyle = color;
    ctx.lineWidth = width;
    ctx.lineJoin = "round";
    ctx.lineCap = "round";
    ctx.stroke();
  }
  function fill_dot(ctx, p, radius, color) {
    ctx.beginPath();
    ctx.arc(p.x, p.y, radius, 0, Math.PI * 2);
    ctx.fillStyle = color;
    ctx.fill();
  }
  // Desktop-style taper (.25, 1, 1, .25) as a cubic bezier of the radius multiplier.
  const taper_at = (t) => { const s = 1 - t; return 0.25 * s * s * s + 3 * s * s * t + 3 * s * t * t + 0.25 * t * t * t; };
  // Screen-space ribbon along projected `points`. style(k, t) -> { radius, color }
  // per point, t = arc-length fraction. Quads are filled one per segment so color can vary.
  function draw_ribbon(ctx, points, style) {
    if (points.length < 2) return;
    const cumulative = [0];
    for (let k = 1; k < points.length; k++) cumulative.push(cumulative[k - 1] + Math.hypot(points[k].x - points[k - 1].x, points[k].y - points[k - 1].y));
    const total = cumulative[cumulative.length - 1] || 1;
    const edges = points.map((p, k) => {
      const prev = points[Math.max(0, k - 1)], next = points[Math.min(points.length - 1, k + 1)];
      const tx = next.x - prev.x, ty = next.y - prev.y, tl = Math.hypot(tx, ty) || 1;
      const s = style(k, cumulative[k] / total);
      return { left: { x: p.x - ty / tl * s.radius, y: p.y + tx / tl * s.radius }, right: { x: p.x + ty / tl * s.radius, y: p.y - tx / tl * s.radius }, color: s.color };
    });
    for (let k = 0; k + 1 < edges.length; k++) {
      const a = edges[k], b = edges[k + 1];
      ctx.beginPath();
      ctx.moveTo(a.left.x, a.left.y); ctx.lineTo(b.left.x, b.left.y); ctx.lineTo(b.right.x, b.right.y); ctx.lineTo(a.right.x, a.right.y);
      ctx.closePath();
      ctx.fillStyle = a.color;
      ctx.fill();
      ctx.strokeStyle = a.color; // hides the hairline seams between quads
      ctx.lineWidth = 0.5;
      ctx.stroke();
    }
  }

  // ---------- scalar fields on the (u,v) grid ----------
  // Marching squares on f(u,v) sampled at the grid corners. Returns segments in
  // (u,v) space plus joined polylines. `f_exact` (optional) enables bisection along
  // each crossing edge; `skip_cell(i, j)` (optional) masks cells out.
  function marching_squares(f_grid, grid, f_exact, skip_cell) {
    const at = (i, j) => f_grid[j * (grid + 1) + i];
    function crossing(i0, j0, i1, j1) {
      const f0 = at(i0, j0), f1 = at(i1, j1);
      let s = f0 / (f0 - f1);
      if (f_exact) {
        let lo = 0, hi = 1, flo = f0;
        for (let k = 0; k < 14; k++) {
          const mid = (lo + hi) / 2;
          const fm = f_exact((i0 + (i1 - i0) * mid) / grid, (j0 + (j1 - j0) * mid) / grid);
          if ((fm < 0) === (flo < 0)) { lo = mid; flo = fm; } else hi = mid;
        }
        s = (lo + hi) / 2;
      }
      return { u: (i0 + (i1 - i0) * s) / grid, v: (j0 + (j1 - j0) * s) / grid };
    }
    const crossings = new Map(); // edge id -> uv, memoized so shared edges agree exactly
    const cross_at = (id, i0, j0, i1, j1) => {
      if (!crossings.has(id)) crossings.set(id, crossing(i0, j0, i1, j1));
      return { id, uv: crossings.get(id) };
    };
    const segments = [];
    for (let j = 0; j < grid; j++) for (let i = 0; i < grid; i++) {
      if (skip_cell && skip_cell(i, j)) continue;
      const ends = [];
      if ((at(i, j) < 0) !== (at(i + 1, j) < 0)) ends.push(cross_at(`h${i},${j}`, i, j, i + 1, j));
      if ((at(i, j + 1) < 0) !== (at(i + 1, j + 1) < 0)) ends.push(cross_at(`h${i},${j + 1}`, i, j + 1, i + 1, j + 1));
      if ((at(i, j) < 0) !== (at(i, j + 1) < 0)) ends.push(cross_at(`v${i},${j}`, i, j, i, j + 1));
      if ((at(i + 1, j) < 0) !== (at(i + 1, j + 1) < 0)) ends.push(cross_at(`v${i + 1},${j}`, i + 1, j, i + 1, j + 1));
      for (let k = 0; k + 1 < ends.length; k += 2) segments.push({ a: ends[k], b: ends[k + 1] });
    }
    return { segments, polylines: join_segments(segments), crossings: [...crossings.values()] };
  }
  // Chains segments that share an end id into polylines (arrays of uv).
  function join_segments(segments) {
    const by_id = new Map();
    segments.forEach((seg, index) => {
      for (const end of [seg.a, seg.b]) {
        if (!by_id.has(end.id)) by_id.set(end.id, []);
        by_id.get(end.id).push(index);
      }
    });
    const used = new Array(segments.length).fill(false);
    const next_unused = (id) => (by_id.get(id) || []).find((k) => !used[k]);
    function walk(index, from_id) {
      const points = [];
      let id = from_id;
      while (index !== undefined) {
        used[index] = true;
        const seg = segments[index];
        const other = seg.a.id === id ? seg.b : seg.a;
        points.push(other.uv);
        id = other.id;
        index = next_unused(id);
      }
      return points;
    }
    const polylines = [];
    for (let s = 0; s < segments.length; s++) {
      if (used[s]) continue;
      used[s] = true;
      const seg = segments[s];
      const forward = [seg.b.uv, ...walk(next_unused(seg.b.id), seg.b.id)];
      const backward = [seg.a.uv, ...walk(next_unused(seg.a.id), seg.a.id)];
      polylines.push([...backward.reverse(), ...forward]);
    }
    return polylines;
  }

  return {
    v3, add, sub, scale, dot, cross, length, normalize, lerp,
    bernstein, bezier, coons, raw_normal,
    orbit_camera, camera_eye, viewer, attach_orbit,
    fit_canvas, bind_controls, rgba,
    draw_surface, fill_polygon, stroke_polyline, fill_dot, taper_at, draw_ribbon,
    marching_squares, join_segments,
  };
})();
