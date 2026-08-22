//-NOTE(kv) Slider values file (sliders -> data, step 1): `<tu>.values.ad` next to the
// sources it belongs to (game/driver/driver.values.ad, game/game.values.ad). Sliders
// in the source carry only an id (`fv(v3_12)`); every value lives here, keyed by id,
// and is copied over the generated zero table after init_sliders -- for the driver
// that is after EVERY hot reload (do_work_after_loading_driver), since slider_values
// lives in the driver DLL.
// Plan: ~/notes/tasks/autodraw_draw_as_data/plan-reorient-code-vs-data.md
//
// NOTE(kv) File layout: magic, file version, timestamp, row count, then rows of
// {u32 id_len, id bytes, u32 value_size, value bytes}, then "EOF". Rows are
// self-describing, so this file has its OWN version (not Data_Version): a struct
// change elsewhere must never silently zero every slider.
//
// NOTE(kv) Rows whose id no slider claims (e.g. a temporarily commented-out call
// site) are kept as orphans and written back, so a value survives its slider
// disappearing for a while. A row whose size disagrees with the slider's type is
// dropped with a log line -- the bytes mean something else now.

global u32 slider_values_file_version = 1;

function Stringz
slider_values_file_path(Arena *arena, Game_State *state, b32 is_driver)
{
 String relative = (is_driver ?
                    strlit("game/driver/driver.values.ad") :
                    strlit("game/game.values.ad"));
 return pjoin(arena, state->code_dir, relative);
}

function Slider *
find_slider_by_id(b32 is_driver, String id)
{
 sarray(FUI_File_Data) files = get_file_array({i16(is_driver), 0});
 for_i32(file_index, 1, files.count)
 {
  for_each(slider, files[file_index].sliders)
  {
   if(slider->id == id){ return slider; }
  }
 }
 return 0;
}

function b32
write_slider_values_file(FILE *file, Game_State *state, b32 is_driver)
{
 Writer writer_value = make_writer(file);
 Writer *writer = &writer_value;
 sarray(FUI_File_Data) files = get_file_array({i16(is_driver), 0});
 darray(Slider_Value_Row) &orphans = state->orphan_slider_rows[is_driver];

 {//-Magic and version
  write_lvalue(writer, autodraw_data_magic);
  write_lvalue(writer, slider_values_file_version);
  time_t rawtime;
  time(&rawtime);
  u64 time64 = rawtime;
  write_lvalue(writer, time64);
 }

 u32 row_count = orphans.count;
 for_i32(file_index, 1, files.count)
 {
  for_each(slider, files[file_index].sliders)
  {// NOTE(kv) Runtime sliders (empty id) are recomputed every frame: nothing to save.
   if(slider->id.size > 0){ row_count += 1; }
  }
 }
 write_lvalue(writer, row_count);

 for_i32(file_index, 1, files.count)
 {
  for_each(slider, files[file_index].sliders)
  {
   if(slider->id.size == 0){ continue; }
   u32 id_len = cast(u32)slider->id.size;
   u32 value_size = cast(u32)slider->type->size;
   write_lvalue(writer, id_len);
   write_size(writer, slider->id.str, id_len);
   write_lvalue(writer, value_size);
   write_size(writer, slider->value, value_size);
  }
 }
 for_each(row, orphans)
 {
  u32 id_len = cast(u32)row->id.size;
  u32 value_size = cast(u32)row->bytes.size;
  write_lvalue(writer, id_len);
  write_size(writer, row->id.str, id_len);
  write_lvalue(writer, value_size);
  write_size(writer, row->bytes.str, value_size);
 }
 {
  const char eof_string[] = "EOF";
  write_size(writer, eof_string, sizeof(eof_string));
 }
 return writer->ok;
}

function b32
save_slider_values_file(Game_State *state, b32 is_driver)
{
 Scratch_Scope tmp;
 Stringz outpath   = slider_values_file_path(tmp, state, is_driver);
 Stringz temp_path = push_stringf(tmp, "%S.temp", outpath);

 FILE *file = open_file(temp_path, "wb");
 b32 ok = (file != 0);
 if(ok)
 {
  ok = write_slider_values_file(file, state, is_driver);
  close_file(file);
 }
 if(ok)
 {// NOTE(kv) Same temp+move as the recording: a crash here costs one edit, not the file.
  if(file_exists(outpath)){ remove_file(outpath); }
  ok = move_file(temp_path, outpath);
 }
 if(ok){
  log_string("slider values saved to %S", outpath);
 }else{
  log_error("slider values save FAILED (%S)", outpath);
 }
 return ok;
}

function b32
load_slider_values_file(Game_State *state, b32 is_driver)
{
 Scratch_Scope tmp;
 Stringz path = slider_values_file_path(tmp, state, is_driver);
 state->slider_values_mtime[is_driver] = file_mtime(path);
 // NOTE(kv) Driver orphans live in driver_arena (cleared per driver load, same as
 // the location maps); game-side ones load once into the permanent arena.
 Arena *arena = is_driver ? &state->driver_arena : &state->permanent_arena;
 darray(Slider_Value_Row) &orphans = state->orphan_slider_rows[is_driver];
 init_dynamic(orphans, arena, 16);

 String file_data = read_entire_file(tmp, path);
 if(file_data.size == 0)
 {// NOTE(kv) Missing file = first run: every slider stays at zero.
  log_string("slider values load: no file at %S", path);
  return false;
 }

 Binary_Reader reader = make_binary_reader(file_data.data, file_data.size);
 Binary_Reader *r = &reader;

 u32 magic = read_binary_u32(r);
 if(magic != autodraw_data_magic)
 {
  log_error("slider values load: %S has bad magic 0x%x (expected 0x%x), ignoring",
            path, magic, autodraw_data_magic);
  return false;
 }
 u32 version = read_binary_u32(r);
 if(version != slider_values_file_version)
 {
  log_error("slider values load: version %u != current %u, ignoring %S",
            version, slider_values_file_version, path);
  return false;
 }
 u64 timestamp = read_binary_u64(r);
 (void)timestamp;
 u32 row_count = read_binary_u32(r);

 i32 applied = 0, mismatched = 0;
 for_u32(row_index, 0, row_count)
 {
  if(not r->ok){ break; }
  u32 id_len = read_binary_u32(r);
  if(not reader_can_take(r, i32(id_len), 1)){ r->ok = false; break; }
  String id = {r->pos, id_len};
  r->pos += id_len;
  u32 value_size = read_binary_u32(r);
  if(not reader_can_take(r, i32(value_size), 1)){ r->ok = false; break; }
  u8 *bytes = r->pos;
  r->pos += value_size;

  Slider *slider = find_slider_by_id(is_driver, id);
  if(slider == 0)
  {
   Slider_Value_Row row = {};
   row.id    = push_string(arena, id);
   row.bytes = push_string(arena, SCu8z(bytes, value_size));
   push(&orphans, row);
  }
  else if(u32(slider->type->size) != value_size)
  {
   log_error("slider values load: %S has %u bytes, slider type %S needs %d; dropped",
             id, value_size, slider->type->name, slider->type->size);
   mismatched += 1;
  }
  else
  {
   block_copy(slider->value, bytes, value_size);
   applied += 1;
  }
 }
 if(not r->ok)
 {
  log_error("slider values load: %S is truncated or corrupt (applied %d rows before failing)",
            path, applied);
  return false;
 }
 log_string("slider values loaded from %S: applied %d, orphans %d, mismatched %d",
            path, applied, orphans.count, mismatched);
 return true;
}

// NOTE(kv) Values written by another instance (the agent's `slider` command) or an
// external editor show up without a driver reload. Skipped mid-drag so a reload can't
// clobber the slider being edited. Our own Enter-save bumps the mtime too; re-reading
// what we just wrote is harmless.
function void
maybe_reload_slider_values_files(Game_State *state)
{
 if(fui_is_active()){ return; }
 Scratch_Scope tmp;
 for_i32(is_driver, 0, 2)
 {
  Stringz path = slider_values_file_path(tmp, state, is_driver);
  if(file_mtime(path) > state->slider_values_mtime[is_driver])
  {
   log_string("slider values: %S changed on disk, reloading", path);
   load_slider_values_file(state, is_driver);
  }
 }
}
//-EOF
