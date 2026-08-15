//-NOTE(kv) Recording persistence (draw-as-data step 5): data/recording.ad, a sibling
// of autosave.ad with the same magic+version framing. Written on the autosave cadence;
// loaded once at startup. Any mismatch (magic, version, struct sizes, counts vs
// remaining bytes) -> log + ignore the file; the seed keeps default settings and the
// per-frame recapture repopulates the capture.
// Plan: ~/notes/tasks/autodraw_draw_as_data/plan-preset-rethink.md
//
// NOTE(kv) File layout: header, all 10 Preset_Settings rows as one raw block, then the
// ONE capture (Recorded_Primitive/Recorded_Group raw blocks). Every field is flat POD
// except Recorded_Image.filename, whose bytes are appended after the blocks and whose
// pointer is patched into the recording arena on load. sizeof() guards in the header
// plus a Data_Version bump on any struct change keep stale files from being misread.

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
  u32 settings_size  = sizeof(Preset_Settings);
  write_lvalue(writer, primitive_size);
  write_lvalue(writer, group_size);
  write_lvalue(writer, settings_size);
 }

 {//-Preset settings table (all rows, raw block)
  write_size(writer, recordings.preset_settings,
             sizeof(Preset_Settings) * Game_Preset_Count);
 }

 Recording &rec = recordings.recording;
 write_lvalue(writer, rec.captured);
 if(rec.captured)
 {
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
 {// NOTE(kv) Q53: no migration -- ignore the file; the seed + recapture repopulate.
  log_string("recording load: version %u != current %u, ignoring file",
             r->read_version, Version_Current);
  return false;
 }

 {//-Struct-size guards
  u32 primitive_size = read_binary_u32(r);
  u32 group_size     = read_binary_u32(r);
  u32 settings_size  = read_binary_u32(r);
  if(r->ok and (primitive_size != sizeof(Recorded_Primitive) or
                group_size     != sizeof(Recorded_Group) or
                settings_size  != sizeof(Preset_Settings)))
  {
   log_error("recording load: struct sizes %u/%u/%u don't match code %u/%u/%u (missing version bump?), ignoring file",
             primitive_size, group_size, settings_size,
             cast(u32)sizeof(Recorded_Primitive), cast(u32)sizeof(Recorded_Group),
             cast(u32)sizeof(Preset_Settings));
   return false;
  }
 }

 {//-Preset settings table (overwrites the seeded rows)
  if(not reader_can_take(r, Game_Preset_Count, sizeof(Preset_Settings)))
  {
   log_error("recording load: file too short for settings table, ignoring file (%S)", path);
   return false;
  }
  read_binary_size(r, sizeof(Preset_Settings) * Game_Preset_Count,
                   state->model.recordings.preset_settings);
 }

 Recording &rec = state->model.recordings.recording;
 rec.captured = false;  // NOTE(kv) stays false if the capture's read dies midway
 b32 file_has_capture = false;
 read_lvalue(r, file_has_capture);
 if(r->ok and file_has_capture)
 {
  i32 primitive_count = read_binary_i1(r);
  if(reader_can_take(r, primitive_count, sizeof(Recorded_Primitive)))
  {
   arena_clear(&rec.arena);
   init_dynamic(rec.primitives, &rec.arena, maximum(1, primitive_count));
   set_count(&rec.primitives, primitive_count);
   read_binary_size(r, sizeof(Recorded_Primitive) * primitive_count, rec.primitives.items);

   i32 group_count = read_binary_i1(r);
   if(reader_can_take(r, group_count, sizeof(Recorded_Group)))
   {
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
    if(r->ok){ rec.captured = true; }
   }
   else{ r->ok = false; }
  }
  else{ r->ok = false; }
 }

 read_debug_string(r, strlit("EOF"));

 if(r->ok){
  log_string("recording load: settings table + %s from %S",
             rec.captured ? "capture" : "no capture", path);
 }else{
  log_error("recording load: file corrupt, ignoring (%S)", path);
 }
 return r->ok;
}
//-EOF
