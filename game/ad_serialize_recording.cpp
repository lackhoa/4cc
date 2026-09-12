//-NOTE(kv) Recording persistence (draw-as-data step 5): data/recording.ad, binary with
// the magic+version framing. Written on the state.txt cadence; loaded once at startup.
// Any mismatch (magic, version, struct sizes, counts vs remaining bytes) -> log + ignore
// the file; the per-frame recapture repopulates the capture.
// Plan: ~/notes/tasks/autodraw_draw_as_data/plan-preset-rethink.md
//
// NOTE(kv) File layout: header, then the ONE capture (Recorded_Primitive/Recorded_Group/
// Recorded_Vertex raw blocks). Every field is flat POD except Recorded_Image.filename,
// whose bytes are appended after the blocks and whose pointer is patched into the
// recording arena on load. sizeof() guards in the header plus a Data_Version bump on any
// struct change keep stale files from being misread. The preset table moved to
// data/state.txt at Version_PresetsInStateFile (plan-presets-text-file).
//
// NOTE(kv) The *document* (game/driver/driver.document.ad, git-tracked) shares the
// header and the recording block -- it is drawing data, not debug state.
// Plan: ~/notes/tasks/autodraw_draw_as_data/plan-data-only-region-poc.md

function Stringz
recording_file_path(Arena *arena, Game_State *state)
{
 return pjoin(arena, state->save_dir, strlit("recording.ad"));
}
function Stringz
document_file_path(Arena *arena, Game_State *state)
{
 return pjoin(arena, state->code_dir, strlit("game/driver/driver.document.ad"));
}

//~ Writing
function void
write_recording_header(Writer *writer)
{
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
  u32 vertex_size    = sizeof(Recorded_Vertex);
  write_lvalue(writer, primitive_size);
  write_lvalue(writer, group_size);
  write_lvalue(writer, vertex_size);
 }
}
function void
write_recording_block(Writer *writer, Recording &rec)
{
 write_lvalue(writer, rec.captured);
 if(rec.captured)
 {
  write_lvalue(writer, rec.primitives.count);
  write_size(writer, rec.primitives.items,
             sizeof(Recorded_Primitive) * rec.primitives.count);
  write_lvalue(writer, rec.groups.count);
  write_size(writer, rec.groups.items,
             sizeof(Recorded_Group) * rec.groups.count);
  for_i32(igroup, 0, rec.groups.count)
  {// NOTE(kv) vis_tag by NAME (Version_GroupTagsByName): the raw block still holds the
   // enum int, but inserting a tag in Group_Vis used to silently retag every saved
   // group after it (the nose became Vis_Ref_Front_4). The name wins on load.
   String name = group_vis_name(rec.groups.items[igroup].vis_tag);
   u32 name_len = cast(u32)name.len;
   write_lvalue(writer, name_len);
   write_size(writer, name.str, name_len);
  }
  write_lvalue(writer, rec.vertices.count);
  write_size(writer, rec.vertices.items,
             sizeof(Recorded_Vertex) * rec.vertices.count);
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
}
function void
write_eof_marker(Writer *writer)
{// NOTE(kv) Same trailing marker as serialize_state (nul included, for dumb tools).
 const char eof_string[] = "EOF";
 write_size(writer, eof_string, sizeof(eof_string));
}

function b32
write_recording_file(FILE *file, Game_State *state)
{
 Writer writer_value = make_writer(file);
 Writer *writer = &writer_value;
 Model_Recordings &recordings = state->model.recordings;
 write_recording_header(writer);
 write_recording_block(writer, recordings.recording);
 write_eof_marker(writer);
 return writer->ok;
}
function b32
write_document_file(FILE *file, Game_State *state)
{
 Writer writer_value = make_writer(file);
 Writer *writer = &writer_value;
 write_recording_header(writer);
 write_recording_block(writer, state->model.recordings.document);
 write_eof_marker(writer);
 return writer->ok;
}

typedef b32 Recording_File_Writer(FILE *file, Game_State *state);
function b32
save_file_via_temp(Game_State *state, Stringz outpath, Stringz temp_path,
                   Recording_File_Writer *write_fn, char const *label)
{
 FILE *file = open_file(temp_path, "wb");
 b32 ok = (file != 0);
 if(ok)
 {
  ok = write_fn(file, state);
  close_file(file);
 }
 if(ok)
 {// NOTE(kv) move_file fails if the destination exists; a crash in this window just
  // costs the file (recapture / re-export rebuilds it), so no temp_old dance here.
  if(file_exists(outpath)){ remove_file(outpath); }
  ok = move_file(temp_path, outpath);
 }
 if(ok){
  log_string("%s saved to %S", label, outpath);
 }else{
  log_error("%s save FAILED (%S)", label, outpath);
 }
 return ok;
}
function b32
save_recording_file(Game_State *state)
{
 Scratch_Scope tmp;
 return save_file_via_temp(state, recording_file_path(tmp, state),
                           pjoin(tmp, state->save_dir, strlit("recording_temp.ad")),
                           write_recording_file, "recording");
}
function b32
save_document_file(Game_State *state)
{
 Scratch_Scope tmp;
 return save_file_via_temp(state, document_file_path(tmp, state),
                           pjoin(tmp, state->save_dir, strlit("document_temp.ad")),
                           write_document_file, "document");
}

//~ Reading
function b32
reader_can_take(Binary_Reader *r, i32 count, usize item_size)
{
 return (r->ok and count >= 0 and
         (r->end_pos - r->pos) >= isize(count * item_size));
}

function b32
read_recording_header(Binary_Reader *r, char const *label, b32 is_document)
{// NOTE(kv) false = ignore the file (already logged).
 // is_document: driver.document.ad is drawing data, so it survives version bumps that
 // don't touch the recording block (accept >= Version_DocumentLayout). recording.ad
 // needs the exact current version.
 u32 magic = read_binary_u32(r);
 if(magic != autodraw_data_magic){ r->ok = false; }
 r->read_version = read_binary_u32(r);
 u64 timestamp = read_binary_u64(r);
 (void)timestamp;
 b32 version_ok = is_document ? (r->read_version >= Version_DocumentLayout and
                                 r->read_version <= Version_Current)
                              : (r->read_version == Version_Current);
 if(r->ok and not version_ok)
 {// NOTE(kv) Q53: no migration -- ignore the file; the seed + recapture repopulate.
  log_error("%s load: version %u not accepted (current %u, document layout %u), ignoring file",
             label, r->read_version, Version_Current, Version_DocumentLayout);
  return false;
 }
 {//-Struct-size guards
  u32 primitive_size = read_binary_u32(r);
  u32 group_size     = read_binary_u32(r);
  if(r->read_version < Version_PresetsInStateFile)
  {// NOTE(kv) Older documents carried a Preset_Settings size guard; nothing to check now.
   u32 settings_size = read_binary_u32(r); (void)settings_size;
  }
  u32 vertex_size    = read_binary_u32(r);
  if(r->ok and (primitive_size != sizeof(Recorded_Primitive) or
                group_size     != sizeof(Recorded_Group) or
                vertex_size    != sizeof(Recorded_Vertex)))
  {
   log_error("%s load: struct sizes %u/%u/%u don't match code %u/%u/%u (missing version bump?), ignoring file",
             label, primitive_size, group_size, vertex_size,
             cast(u32)sizeof(Recorded_Primitive), cast(u32)sizeof(Recorded_Group),
             cast(u32)sizeof(Recorded_Vertex));
   return false;
  }
 }
 return r->ok;
}

function void
read_recording_block(Binary_Reader *r, Recording &rec)
{// NOTE(kv) On any short read r->ok goes false and rec.captured stays false.
 rec.captured = false;
 b32 file_has_capture = false;
 read_lvalue(r, file_has_capture);
 if(not (r->ok and file_has_capture)){ return; }

 i32 primitive_count = read_binary_i1(r);
 if(not reader_can_take(r, primitive_count, sizeof(Recorded_Primitive))){ r->ok = false; return; }
 arena_clear(&rec.arena);
 init_dynamic(rec.primitives, &rec.arena, maximum(1, primitive_count));
 set_count(&rec.primitives, primitive_count);
 read_binary_size(r, sizeof(Recorded_Primitive) * primitive_count, rec.primitives.items);

 i32 group_count = read_binary_i1(r);
 if(not reader_can_take(r, group_count, sizeof(Recorded_Group))){ r->ok = false; return; }
 init_dynamic(rec.groups, &rec.arena, maximum(1, group_count));
 set_count(&rec.groups, group_count);
 read_binary_size(r, sizeof(Recorded_Group) * group_count, rec.groups.items);
 for_i32(igroup, 0, group_count)
 {//-Resolve vis_tag
  Recorded_Group &group = rec.groups.items[igroup];
  if(r->read_version >= Version_GroupTagsByName)
  {// NOTE(kv) The name after the raw block is authoritative (see write_recording_block).
   u32 name_len = read_binary_u32(r);
   if(not reader_can_take(r, cast(i32)name_len, 1)){ r->ok = false; return; }
   char *name = cast(char *)push_size(&rec.arena, name_len + 1);
   read_binary_size(r, name_len, name);
   name[name_len] = 0;
   Group_Vis resolved = Vis_None;
   b32 found = group_vis_from_name(SCu8(name), &resolved);
   if(not found)
   {
    log_error("recording load: group %d has unknown vis tag \"%s\" (renamed/removed from Group_Vis?), using Vis_None", igroup, name);
   }
   group.vis_tag = resolved;
  }
  else if(group.vis_tag >= Vis_Hair)
  {// NOTE(kv) Pre-name documents were written before Vis_Hair existed (2026-09-06), so
   // every tag from that slot on is off by one. Re-saving upgrades the file to names.
   group.vis_tag = cast(Group_Vis)(group.vis_tag + 1);
  }
 }

 i32 vertex_count = read_binary_i1(r);
 if(not reader_can_take(r, vertex_count, sizeof(Recorded_Vertex))){ r->ok = false; return; }
 init_dynamic(rec.vertices, &rec.arena, maximum(1, vertex_count));
 set_count(&rec.vertices, vertex_count);
 read_binary_size(r, sizeof(Recorded_Vertex) * vertex_count, rec.vertices.items);

 for_i32(iprim, 0, primitive_count)
 {//-Patch image filename pointers
  Recorded_Primitive &prim = rec.primitives.items[iprim];
  if(prim.type == Primitive_Type_Image)
  {
   u32 filename_len = read_binary_u32(r);
   if(not reader_can_take(r, cast(i32)filename_len, 1)){ r->ok = false; return; }
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
 state->recording_load_failed = true;  // NOTE(kv) cleared on success below
 Binary_Reader reader = make_binary_reader(file_data.data, file_data.size);
 Binary_Reader *r = &reader;
 if(not read_recording_header(r, "recording", false)){ return false; }

 Recording &rec = state->model.recordings.recording;
 read_recording_block(r, rec);
 read_debug_string(r, strlit("EOF"));

 if(r->ok){
  state->recording_load_failed = false;
  log_string("recording load: %s from %S",
             rec.captured ? "capture" : "no capture", path);
 }else{
  log_error("recording load: file corrupt, ignoring (%S)", path);
 }
 return r->ok;
}

function b32
load_document_file(Game_State *state)
{
 Scratch_Scope tmp;
 Stringz path = document_file_path(tmp, state);
 String file_data = read_entire_file(tmp, path);
 if(file_data.len == 0)
 {// NOTE(kv) No document yet = nothing migrated; the code path draws everything.
  log_string("document load: no file at %S", path);
  return false;
 }
 state->document_load_failed = true;  // NOTE(kv) cleared on success below
 Binary_Reader reader = make_binary_reader(file_data.data, file_data.size);
 Binary_Reader *r = &reader;
 if(not read_recording_header(r, "document", true)){ return false; }

 Recording &doc = state->model.recordings.document;
 read_recording_block(r, doc);
 read_debug_string(r, strlit("EOF"));

 if(r->ok){
  state->document_load_failed = false;
  log_string("document load: %d primitives, %d groups, %d vertices from %S",
             doc.primitives.count, doc.groups.count, doc.vertices.count, path);
 }else{
  log_error("document load: file corrupt, ignoring (%S)", path);
 }
 return r->ok;
}
//-EOF
