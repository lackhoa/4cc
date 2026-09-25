// NOTE(kv) The simplified skull, fitted to the reference (plan-simplified-skull): least
// squares of drawable shapes to the Z-Anatomy skull layer's vertices, in mesh space (mm).
// Step 1/2: the cranium ball (Skull_Ball). Debug commands `construction_fit` (fit, write
// the slider Skull_Ball_1, save driver.values.ad) and `construction_dump` (residuals of
// the current slider values, no refit). The driver draws the ball (draw_skull_construction).

struct Construction_Vault_Points
{// NOTE(kv) The skull layer's vertices on the vault side of the Frankfurt plane, mesh mm.
 // Borrowed from the driver's mesh cache like Reference_Mesh_Triangles: use within the call.
 v3 *points;
 i32 count;
 i32 skull_vertex_count;  // before the plane cut
 Reference_Frankfurt_Frame frame;
 v1 porion_width_mm;      // calibration: ~120-130 on a real skull
};

function b32
construction_vault_points(Game_State *state, Arena *arena, Construction_Vault_Points *out)
{// NOTE(kv) false without a mesh scene, the skull layer, or the Frankfurt landmarks. The
 // vault = everything above the Frankfurt plane (plan Q4; expect to tighten once the
 // residual map shows the brow ridge and orbit rims pulling the sphere).
 Reference_Mesh_Placement *placement = get_reference_mesh_placement(state);
 if(placement == 0){ return false; }
 Driver_API *driver = &state->driver_api;
 Reference_Scene_Data data = driver->driver_get_scene_data(active_preset_row(state).scene);
 if(data.mesh_layer_count == 0){ return false; }
 Reference_Mesh_Triangles skull = driver->driver_get_reference_mesh(data.mesh_layers[0].filename);
 if(not skull.ok){ return false; }
 Reference_Frankfurt_Frame frame;
 if(not reference_frankfurt_frame(state, &frame)){ return false; }
 Reference_Landmark *porion_l = reference_landmark_find_any_layer(state, strlit("porion_l"));
 Reference_Landmark *porion_r = reference_landmark_find_any_layer(state, strlit("porion_r"));
 v3 *points = push_array(arena, v3, skull.vertex_count);
 i32 count = 0;
 for_i32(vi, 0, skull.vertex_count)
 {
  v3 p = skull.vertices[vi];
  if(dot(p - frame.porion_middle, frame.up) > 0.f){ points[count++] = p; }
 }
 *out = {.points = points, .count = count, .skull_vertex_count = skull.vertex_count,
         .frame = frame, .porion_width_mm = lengthof(porion_r->p - porion_l->p)};
 return true;
}

function b32
solve_linear_system_gaussian(i32 n, v1 *A, v1 *b, v1 *x_out)
{// NOTE(kv) A is n x n row-major, solved in place with partial pivoting (n <= 4 here).
 // false when a pivot is ~0 (degenerate points, e.g. all coplanar for a sphere).
 for_i32(col, 0, n)
 {
  i32 pivot = col;
  for_i32(row, col+1, n){ if(absolute(A[row*n+col]) > absolute(A[pivot*n+col])){ pivot = row; } }
  if(absolute(A[pivot*n+col]) < 1e-9f){ return false; }
  if(pivot != col)
  {
   for_i32(k, 0, n){ macro_swap(A[col*n+k], A[pivot*n+k]); }
   macro_swap(b[col], b[pivot]);
  }
  for_i32(row, col+1, n)
  {
   v1 factor = A[row*n+col] / A[col*n+col];
   for_i32(k, col, n){ A[row*n+k] -= factor * A[col*n+k]; }
   b[row] -= factor * b[col];
  }
 }
 for(i32 row = n-1; row >= 0; row--)
 {
  v1 sum = b[row];
  for_i32(k, row+1, n){ sum -= A[row*n+k] * x_out[k]; }
  x_out[row] = sum / A[row*n+row];
 }
 return true;
}

function b32
fit_sphere_algebraic(v3 *points, i32 count, v3 *center_out, v1 *radius_out)
{// NOTE(kv) Linear least squares of |p|^2 = 2 c.p + k with k = r^2 - |c|^2 (the algebraic
 // sphere fit): 4x4 normal equations, points recentered on their mean for conditioning.
 // Not the geometric (distance) fit, but within a fraction of a mm of it for a near-sphere.
 if(count < 4){ return false; }
 v3 mean = {};
 for_i32(i, 0, count){ mean += points[i]; }
 mean = mean / v1(count);
 f64 A[16] = {}; f64 b[4] = {};
 for_i32(i, 0, count)
 {
  v3 p = points[i] - mean;
  f64 row[4] = {2.0*p.x, 2.0*p.y, 2.0*p.z, 1.0};
  f64 rhs = f64(p.x)*p.x + f64(p.y)*p.y + f64(p.z)*p.z;
  for_i32(r, 0, 4){ for_i32(c, 0, 4){ A[r*4+c] += row[r]*row[c]; } b[r] += row[r]*rhs; }
 }
 // NOTE(kv) Accumulated in f64 (sums of mm^4 over 10k points), solved in v1 after scaling
 // by the count so the entries are O(mm^2).
 v1 A1[16]; v1 b1[4]; v1 x[4];
 for_i32(k, 0, 16){ A1[k] = v1(A[k] / count); }
 for_i32(k, 0, 4){ b1[k] = v1(b[k] / count); }
 if(not solve_linear_system_gaussian(4, A1, b1, x)){ return false; }
 v3 c = V3(x[0], x[1], x[2]);
 v1 r2 = x[3] + dot(c, c);
 if(r2 <= 0.f){ return false; }
 *center_out = c + mean;
 *radius_out = square_root(r2);
 return true;
}

function b32
fit_circle_algebraic(v2 *points, i32 count, v2 *center_out, v1 *radius_out)
{// NOTE(kv) The 2D version of fit_sphere_algebraic (side-view circle, plan Q4).
 if(count < 3){ return false; }
 v2 mean = {};
 for_i32(i, 0, count){ mean += points[i]; }
 mean = mean / v1(count);
 f64 A[9] = {}; f64 b[3] = {};
 for_i32(i, 0, count)
 {
  v2 p = points[i] - mean;
  f64 row[3] = {2.0*p.x, 2.0*p.y, 1.0};
  f64 rhs = f64(p.x)*p.x + f64(p.y)*p.y;
  for_i32(r, 0, 3){ for_i32(c, 0, 3){ A[r*3+c] += row[r]*row[c]; } b[r] += row[r]*rhs; }
 }
 v1 A1[9]; v1 b1[3]; v1 x[3];
 for_i32(k, 0, 9){ A1[k] = v1(A[k] / count); }
 for_i32(k, 0, 3){ b1[k] = v1(b[k] / count); }
 if(not solve_linear_system_gaussian(3, A1, b1, x)){ return false; }
 v2 c = V2(x[0], x[1]);
 v1 r2 = x[2] + dot(c, c);
 if(r2 <= 0.f){ return false; }
 *center_out = c + mean;
 *radius_out = square_root(r2);
 return true;
}

struct Construction_Residuals
{// NOTE(kv) Signed distance vertex -> shape surface, mm; positive = outside the shape.
 v1 min;
 v1 max;
 v1 rms;
 i32 count;
};

function Construction_Residuals
skull_ball_residuals(Skull_Ball ball, v3 *points, i32 count)
{
 Construction_Residuals result = {.min = INFINITY, .max = -INFINITY, .count = count};
 f64 sum_squares = 0;
 for_i32(i, 0, count)
 {
  v1 residual = lengthof(points[i] - ball.center) - ball.radius;
  result.min = min(result.min, residual);
  result.max = max(result.max, residual);
  sum_squares += f64(residual)*residual;
 }
 if(count > 0){ result.rms = v1(sqrt(sum_squares / count)); }
 return result;
}

function Skull_Ball *
find_skull_ball_slider()
{// NOTE(kv) The one ball slider, declared as fv(Skull_Ball_1) in driver.kc.
 Slider *slider = find_slider_by_id(/*is_driver*/true, strlit("Skull_Ball_1"));
 return slider ? (Skull_Ball *)slider->value : 0;
}

function void
debug_channel_construction_print(FILE *out, Skull_Ball ball, Construction_Vault_Points &vault)
{
 Construction_Residuals residuals = skull_ball_residuals(ball, vault.points, vault.count);
 fprintf(out, "  porion width %.1f mm (calibration, real skulls ~120-130); vault %d of %d skull vertices\n",
         vault.porion_width_mm, vault.count, vault.skull_vertex_count);
 v3 c_frame = ball.center - vault.frame.porion_middle;
 fprintf(out, "  Skull_Ball_1: center mesh (%.2f %.2f %.2f) = Frankfurt frame side %+.1f up %+.1f front %+.1f, radius %.2f mm\n",
         ball.center.x, ball.center.y, ball.center.z,
         dot(c_frame, vault.frame.side), dot(c_frame, vault.frame.up), dot(c_frame, vault.frame.front),
         ball.radius);
 fprintf(out, "  residual (vertex - sphere, + = outside): min %+.2f max %+.2f rms %.2f mm over %d vertices\n",
         residuals.min, residuals.max, residuals.rms, residuals.count);
}

function void
debug_channel_construction_fit(FILE *out, Game_State *state)
{
 Scratch_Block scratch;
 Construction_Vault_Points vault;
 if(not construction_vault_points(state, scratch, &vault))
 {
  fprintf(out, "construction_fit: needs a drawn mesh scene (skull = layer 0) and landmarks porion_l, porion_r, orbitale\n");
  return;
 }
 Skull_Ball *slider_ball = find_skull_ball_slider();
 if(slider_ball == 0)
 {
  fprintf(out, "construction_fit: no slider Skull_Ball_1 (fv(Skull_Ball_1) in driver.kc)\n");
  return;
 }
 Skull_Ball ball = {};
 if(not fit_sphere_algebraic(vault.points, vault.count, &ball.center, &ball.radius))
 {
  fprintf(out, "construction_fit: sphere fit failed (%d vault vertices)\n", vault.count);
  return;
 }
 fprintf(out, "construction_fit:\n");
 debug_channel_construction_print(out, ball, vault);
 {// NOTE(kv) Side-view circle (plan Q4): the vault projected onto the midline plane
  // (Frankfurt up/front coordinates, mm from the porion middle), so "the cranium is an
  // egg" shows as the difference between this and the sphere.
  v2 *side = push_array(scratch, v2, vault.count);
  for_i32(i, 0, vault.count)
  {
   v3 p = vault.points[i] - vault.frame.porion_middle;
   side[i] = V2(dot(p, vault.frame.front), dot(p, vault.frame.up));
  }
  v2 circle_center; v1 circle_radius;
  if(fit_circle_algebraic(side, vault.count, &circle_center, &circle_radius))
  {
   v3 c_frame = ball.center - vault.frame.porion_middle;
   fprintf(out, "  side-view circle: center front %+.1f up %+.1f, radius %.2f mm (sphere: front %+.1f up %+.1f, radius %.2f)\n",
           circle_center.x, circle_center.y, circle_radius,
           dot(c_frame, vault.frame.front), dot(c_frame, vault.frame.up), ball.radius);
  }
 }
 *slider_ball = ball;
 save_slider_values_file(state, /*is_driver*/1);
 fprintf(out, "  written to Skull_Ball_1, driver.values.ad saved\n");
}

function void
debug_channel_construction_dump(FILE *out, Game_State *state)
{
 Scratch_Block scratch;
 Construction_Vault_Points vault;
 Skull_Ball *slider_ball = find_skull_ball_slider();
 if(slider_ball == 0 or slider_ball->radius <= 0.f)
 {
  fprintf(out, "construction_dump: Skull_Ball_1 not fitted yet (run construction_fit)\n");
 }
 else if(not construction_vault_points(state, scratch, &vault))
 {
  fprintf(out, "construction_dump: needs a drawn mesh scene (skull = layer 0) and landmarks porion_l, porion_r, orbitale\n");
 }
 else
 {
  fprintf(out, "construction_dump:\n");
  debug_channel_construction_print(out, *slider_ball, vault);
 }
}
