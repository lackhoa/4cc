//-NOTE(kv) Recording persistence (draw-as-data step 5): data/recording.ad, a sibling
// of autosave.ad with the same magic+version framing. Written on the autosave cadence
// from the captured Model.recordings slots; loaded once at startup. Any mismatch
// (magic, version, struct sizes, counts vs remaining bytes) -> log + ignore the file;
// the per-frame recapture repopulates the slots.
// Plan: ~/notes/tasks/autodraw_draw_as_data/plan-recording-serialization.md
//
// NOTE(kv) The slot payloads (Recorded_Primitive/Recorded_Group and everything
// embedded in them) are dumped as raw blocks: every field is flat POD except
// Recorded_Image.filename, whose bytes are appended after the blocks and whose
// pointer is patched into the slot arena on load. sizeof() guards in the header plus
// a Data_Version bump on any struct change keep stale files from being misread.

function Stringz
recording_file_path(Arena *arena, Game_State *state)
{
 return pjoin(arena, state->save_dir, strlit("recording.ad"));
}

function b32
write_recording_file(FILE *file, Game_State *state)
{
 Writer writer_value = make_writer(file);
 Writer *writer = &writer_value;
 Model_Recordings &recordings = state->model.recordings;

 {//-Magic and version
  write_lvalue(writer, autodraw_data_magic);
  write_lvalue(writer, Version_Current);
  time_t rawtime;
  time(&rawtime);
  u64 time64 = rawtime;
  write_lvalue(writer, time64);
 }

 {//-Struct-size guards
  u32 primitive_size = sizeof(Recorded_Primitive);
  u32 group_size     = sizeof(Recorded_Group);
  write_lvalue(writer, primitive_size);
  write_lvalue(writer, group_size);
 }

 i32 captured_count = 0;
 for_i32(preset, 0, Game_Preset_Count)
 {
  if(recordings.slots[preset].captured){ captured_count += 1; }
 }
 write_lvalue(writer, captured_count);

 for_i32(preset, 0, Game_Preset_Count)
 {
  Recording &rec = recordings.slots[preset];
  if(not rec.captured){ continue; }
  write_lvalue(writer, preset);
  write_lvalue(writer, rec.viz_level);
  write_lvalue(writer, rec.ignore_radii);
  write_lvalue(writer, rec.ignore_alignment_min);
  write_lvalue(writer, rec.show_grid);
  write_lvalue(writer, rec.primitives.count);
  write_size(writer, rec.primitives.items,
             sizeof(Recorded_Primitive) * rec.primitives.count);
  write_lvalue(writer, rec.groups.count);
  write_size(writer, rec.groups.items,
             sizeof(Recorded_Group) * rec.groups.count);
  for_i32(iprim, 0, rec.primitives.count)
  {// NOTE(kv) The raw block stored a dangling filename pointer; the bytes go here.
   Recorded_Primitive &prim = rec.primitives.items[iprim];
   if(prim.type == Primitive_Type_Image)
   {
    u32 filename_len = cast(u32)prim.image.filename.len;
    write_lvalue(writer, filename_len);
    write_size(writer, prim.image.filename.str, filename_len);
   }
  }
 }
 {// NOTE(kv) Same trailing marker as serialize_state (nul included, for dumb tools).
  const char eof_string[] = "EOF";
  write_size(writer, eof_string, sizeof(eof_string));
 }
 return writer->ok;
}

function b32
save_recording_file(Game_State *state)
{
 Scratch_Scope tmp;
 Stringz outpath   = recording_file_path(tmp, state);
 Stringz temp_path = pjoin(tmp, state->save_dir, strlit("recording_temp.ad"));

 FILE *file = open_file(temp_path, "wb");
 b32 ok = (file != 0);
 if(ok)
 {
  ok = write_recording_file(file, state);
  close_file(file);
 }
 if(ok)
 {// NOTE(kv) move_file fails if the destination exists; a crash in this window just
  // costs the recording file (recapture rebuilds it), so no temp_old dance here.
  if(file_exists(outpath)){ remove_file(outpath); }
  ok = move_file(temp_path, outpath);
 }
 if(ok){
  log_string("recording saved to %S", outpath);
 }else{
  log_error("recording save FAILED (%S)", outpath);
 }
 return ok;
}

function b32
reader_can_take(Binary_Reader *r, i32 count, usize item_size)
{
 return (r->ok and count >= 0 and
         (r->end_pos - r->pos) >= isize(count * item_size));
}

function b32
load_recording_file(Game_State *state)
{
 Scratch_Scope tmp;
 Stringz path = recording_file_path(tmp, state);
 String file_data = read_entire_file(tmp, path);
 if(file_data.len == 0)
 {// NOTE(kv) Missing file is the normal first-run case, not an error.
  log_string("recording load: no file at %S", path);
  return false;
 }

 Binary_Reader reader = make_binary_reader(file_data.data, file_data.size);
 Binary_Reader *r = &reader;

 u32 magic = read_binary_u32(r);
 if(magic != autodraw_data_magic){ r->ok = false; }
 r->read_version = read_binary_u32(r);
 u64 timestamp = read_binary_u64(r);
 (void)timestamp;
 if(r->ok and r->read_version != Version_Current)
 {// NOTE(kv) Q53: no migration -- ignore the file, the recapture repopulates it.
  log_string("recording load: version %u != current %u, ignoring file",
             r->read_version, Version_Current);
  return false;
 }

 {//-Struct-size guards
  u32 primitive_size = read_binary_u32(r);
  u32 group_size     = read_binary_u32(r);
  if(r->ok and (primitive_size != sizeof(Recorded_Primitive) or
                group_size     != sizeof(Recorded_Group)))
  {
   log_error("recording load: struct sizes %u/%u don't match code %u/%u (missing version bump?), ignoring file",
             primitive_size, group_size,
             cast(u32)sizeof(Recorded_Primitive), cast(u32)sizeof(Recorded_Group));
   return false;
  }
 }

 i32 captured_count = read_binary_i1(r);
 i32 loaded_count = 0;
 for_i32(irecord, 0, captured_count)
 {
  if(not r->ok){ break; }
  i32 preset = read_binary_i1(r);
  if(not (0 <= preset and preset < Game_Preset_Count)){ r->ok = false; break; }
  Recording &rec = state->model.recordings.slots[preset];
  rec.captured = false;  // NOTE(kv) stays false if this slot's read dies midway
  read_lvalue(r, rec.viz_level);
  read_lvalue(r, rec.ignore_radii);
  read_lvalue(r, rec.ignore_alignment_min);
  read_lvalue(r, rec.show_grid);

  i32 primitive_count = read_binary_i1(r);
  if(not reader_can_take(r, primitive_count, sizeof(Recorded_Primitive))){ r->ok = false; break; }
  arena_clear(&rec.arena);
  init_dynamic(rec.primitives, &rec.arena, maximum(1, primitive_count));
  set_count(&rec.primitives, primitive_count);
  read_binary_size(r, sizeof(Recorded_Primitive) * primitive_count, rec.primitives.items);

  i32 group_count = read_binary_i1(r);
  if(not reader_can_take(r, group_count, sizeof(Recorded_Group))){ r->ok = false; break; }
  init_dynamic(rec.groups, &rec.arena, maximum(1, group_count));
  set_count(&rec.groups, group_count);
  read_binary_size(r, sizeof(Recorded_Group) * group_count, rec.groups.items);

  for_i32(iprim, 0, primitive_count)
  {//-Patch image filename pointers
   Recorded_Primitive &prim = rec.primitives.items[iprim];
   if(prim.type == Primitive_Type_Image)
   {
    u32 filename_len = read_binary_u32(r);
    if(not reader_can_take(r, cast(i32)filename_len, 1)){ r->ok = false; break; }
    u8 *bytes = cast(u8 *)push_size(&rec.arena, filename_len + 1);
    read_binary_size(r, filename_len, bytes);
    bytes[filename_len] = 0;  // NOTE(kv) to_cstring(Stringz) returns .str directly
    prim.image.filename = {};
    prim.image.filename.str = bytes;
    prim.image.filename.len = filename_len;
   }
  }
  if(not r->ok){ break; }
  rec.captured = true;
  loaded_count += 1;
 }

 read_debug_string(r, strlit("EOF"));

 if(r->ok){
  log_string("recording load: %d slot(s) from %S", loaded_count, path);
 }else{
  log_error("recording load: file corrupt after %d slot(s), ignoring the rest (%S)",
            loaded_count, path);
 }
 return r->ok;
}
//-EOF
