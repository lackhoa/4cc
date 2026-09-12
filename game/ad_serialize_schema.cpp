//-NOTE(kv) Self-describing document format (plan-document-self-describing-format, Q12/Q13):
// file = magic + format tag + timestamp + SCHEMA of the root type + one VALUE of the
// root type + "EOF". The schema lists every named type reachable from the root
// (Document_File): name, kind, size, struct members (name + type ref + discriminator
// member name for unions), enum members (name + value), union members (name + type ref +
// discriminator value). No offsets: the reader matches file members to live members BY
// NAME, skips unknown members using the file schema, zeroes missing ones, remaps enum
// values and union variants by name. Arrays/darrays are anonymous type refs (count +
// item type / item type), so the table only holds named types.
//
// Value encoding is the existing binary walker (write_binary_func, ad_serialize.cpp):
// basic = raw bytes, String = u32 len + bytes, enum = i32, array = items, darray = u32
// count + items, union = only the active variant (the discriminator is a plain enum
// member written earlier in the same struct). Unserialized members are neither in the
// schema nor in the value.
//
// Reader allocations (darray items, String bytes) go into the caller's arena; String
// bytes get a trailing 0 so Recorded_Image.filename stays usable as a C string.

global u32 const schema_format_tag = 'adsc';  // NOTE(kv) after the magic; old headers had a Data_Version here

//~ Writing

enum Schema_Ref_Kind
{// NOTE(kv) How a type is referenced from a member / item slot.
 Schema_Ref_Named  = 0,  // u32 len + name (a table entry)
 Schema_Ref_Array  = 1,  // u32 count + ref
 Schema_Ref_Darray = 2,  // ref
};

function b32
schema_is_named_type(Type_Info *type)
{
 return (type->kind == I_Type_Kind_Basic  or
         type->kind == I_Type_Kind_Struct or
         type->kind == I_Type_Kind_Wrapper or
         type->kind == I_Type_Kind_Enum   or
         type->kind == I_Type_Kind_Union);
}
function void
schema_collect_types(darray(Type_Info *) &types, Type_Info *type)
{// NOTE(kv) Post-order: dependencies before dependents, so the reader resolves every
 // name the moment it sees it (root comes last).
 switch(type->kind)
 {
  case I_Type_Kind_Array:
  case I_Type_Kind_Darray:
  {
   schema_collect_types(types, type->array_item_type);
   return;
  }
  default: break;
 }
 for_i32(index, 0, types.count){ if(types.items[index] == type){ return; } }
 switch(type->kind)
 {
  case I_Type_Kind_Wrapper:
  case I_Type_Kind_Struct:
  {
   for_i32(index, 0, type->members.count)
   {
    I_Struct_Member &member = type->members[index];
    if(not member.unserialized){ schema_collect_types(types, member.type); }
   }
  }break;
  case I_Type_Kind_Union:
  {
   schema_collect_types(types, type->discriminator_type);
   for_i32(index, 0, type->union_members.count)
   {
    schema_collect_types(types, type->union_members[index].type);
   }
  }break;
  default: break;
 }
 push(&types, type);
}

function void
write_schema_string(Writer *writer, String string)
{
 u32 len = cast(u32)string.len;
 write_lvalue(writer, len);
 // NOTE(kv) fwrite(_, 0, 1, _) returns 0, which write_size reads as failure: skip empties
 // (the discriminator name of a non-union member is empty).
 if(len > 0){ write_size(writer, string.str, len); }
}
function void
write_schema_type_ref(Writer *writer, Type_Info *type)
{
 u32 ref_kind = Schema_Ref_Named;
 switch(type->kind)
 {
  case I_Type_Kind_Array:
  {
   ref_kind = Schema_Ref_Array;
   write_lvalue(writer, ref_kind);
   u32 count = cast(u32)type->count;
   write_lvalue(writer, count);
   write_schema_type_ref(writer, type->array_item_type);
  }break;
  case I_Type_Kind_Darray:
  {
   ref_kind = Schema_Ref_Darray;
   write_lvalue(writer, ref_kind);
   write_schema_type_ref(writer, type->array_item_type);
  }break;
  default:
  {
   kv_assert(schema_is_named_type(type));
   write_lvalue(writer, ref_kind);
   write_schema_string(writer, type->name);
  }break;
 }
}
function String
schema_member_name_at_offset(Type_Info *type, u32 offset)
{// NOTE(kv) The union's discriminator is referenced by offset in Type_Info; the file
 // wants the member NAME (offsets are exactly what the file must not depend on).
 for_i32(index, 0, type->members.count)
 {
  I_Struct_Member &member = type->members[index];
  if(member.offset == offset and member.type->kind == I_Type_Kind_Enum){ return member.name; }
 }
 return {};
}
function void
write_schema_type(Writer *writer, Type_Info *type)
{
 write_schema_string(writer, type->name);
 u32 kind = type->kind;
 u32 size = cast(u32)type->size;
 write_lvalue(writer, kind);
 write_lvalue(writer, size);
 switch(type->kind)
 {
  case I_Type_Kind_Basic: break;  // NOTE(kv) name + size say it all (String is by name)
  case I_Type_Kind_Wrapper:
  case I_Type_Kind_Struct:
  {
   u32 member_count = 0;
   for_i32(index, 0, type->members.count){ if(not type->members[index].unserialized){ member_count++; } }
   write_lvalue(writer, member_count);
   for_i32(index, 0, type->members.count)
   {
    I_Struct_Member &member = type->members[index];
    if(member.unserialized){ continue; }
    write_schema_string(writer, member.name);
    write_schema_type_ref(writer, member.type);
    String discriminator = {};
    if(member.type->kind == I_Type_Kind_Union)
    {
     discriminator = schema_member_name_at_offset(type, member.discriminator_offset);
     kv_assert(discriminator.len > 0);
    }
    write_schema_string(writer, discriminator);
   }
  }break;
  case I_Type_Kind_Enum:
  {
   u32 count = cast(u32)type->enum_members.count;
   write_lvalue(writer, count);
   for_i32(index, 0, type->enum_members.count)
   {
    I_Enum_Member &member = type->enum_members[index];
    write_schema_string(writer, member.name);
    write_lvalue(writer, member.value);
   }
  }break;
  case I_Type_Kind_Union:
  {
   write_schema_string(writer, type->discriminator_type->name);
   u32 count = cast(u32)type->union_members.count;
   write_lvalue(writer, count);
   for_i32(index, 0, type->union_members.count)
   {
    I_Union_Member &member = type->union_members[index];
    write_schema_string(writer, member.name);
    write_schema_type_ref(writer, member.type);
    write_lvalue(writer, member.variant);
   }
  }break;
  InvalidDefaultCase;
 }
}
function void
write_schema(Writer *writer, Type_Info *root)
{
 Scratch_Scope tmp;
 darray(Type_Info *) types;
 init_dynamic(types, tmp, 64);
 schema_collect_types(types, root);
 u32 type_count = cast(u32)types.count;
 write_lvalue(writer, type_count);
 for_i32(index, 0, types.count){ write_schema_type(writer, types.items[index]); }
 write_schema_string(writer, root->name);
}

function void
write_schema_file_header(Writer *writer)
{
 u32 tag = schema_format_tag;
 write_lvalue(writer, autodraw_data_magic);
 write_lvalue(writer, tag);
 time_t rawtime;
 time(&rawtime);
 u64 time64 = rawtime;
 write_lvalue(writer, time64);
}
function b32
write_document_schema_file(FILE *file, Game_State *state)
{// NOTE(kv) Document_File is just a view of the three Recording darrays.
 Writer writer_value = make_writer(file);
 Writer *writer = &writer_value;
 Recording &doc = state->model.recordings.document;
 Document_File value = {};
 value.vertices   = doc.vertices;
 value.groups     = doc.groups;
 value.primitives = doc.primitives;
 write_schema_file_header(writer);
 write_schema(writer, &Type_Info_Document_File);
 write_binary_func(writer, &Type_Info_Document_File, &value);
 write_eof_marker(writer);
 return writer->ok;
}

//~ Reading: the file schema

struct File_Type;
struct File_Member
{
 String name;
 File_Type *type;
 String discriminator;  // NOTE(kv) union members only: name of the enum member holding the variant
 b32 warned;            // NOTE(kv) "no live member" logged once per file type, not per item
};
struct File_Union_Member
{
 String name;
 File_Type *type;
 i32 variant;
};
struct File_Enum_Member
{
 String name;
 i32 value;
};
struct File_Type
{
 String name;  // NOTE(kv) empty for the anonymous array/darray refs
 I_Type_Kind kind;
 i32 size;
 darray(File_Member) members;
 darray(File_Enum_Member) enum_members;
 String discriminator_type;
 darray(File_Union_Member) union_members;
 i32 count;         // NOTE(kv) Array
 File_Type *item;   // NOTE(kv) Array / Darray
 Type_Info *checked_against;  // NOTE(kv) live type whose missing members were already reported
};
struct File_Schema
{
 Arena *arena;  // NOTE(kv) schema tables (scratch; strings point into the file buffer)
 darray(File_Type *) types;
 File_Type *root;
 char const *label;
};

function File_Type *
schema_find_type(File_Schema *schema, String name)
{
 for_i32(index, 0, schema->types.count)
 {
  File_Type *type = schema->types.items[index];
  if(type->name == name){ return type; }
 }
 return 0;
}
function String
read_schema_string(Binary_Reader *r)
{
 String result = {};
 read_binary_String(r, &result);
 if(not r->ok){ result = {}; }
 return result;
}
function File_Type *
read_schema_type_ref(Binary_Reader *r, File_Schema *schema)
{
 u32 ref_kind = read_binary_u32(r);
 if(not r->ok){ return 0; }
 File_Type *result = 0;
 switch(ref_kind)
 {
  case Schema_Ref_Named:
  {
   String name = read_schema_string(r);
   result = schema_find_type(schema, name);
   if(r->ok and not result)
   {
    log_error("%s load: schema references unknown type \"%.*s\"", schema->label, strexpand(name));
    r->ok = false;
   }
  }break;
  case Schema_Ref_Array:
  {
   result = push_struct0(schema->arena, File_Type);
   result->kind  = I_Type_Kind_Array;
   result->count = cast(i32)read_binary_u32(r);
   result->item  = read_schema_type_ref(r, schema);
   if(result->item){ result->size = result->count * result->item->size; }
  }break;
  case Schema_Ref_Darray:
  {
   result = push_struct0(schema->arena, File_Type);
   result->kind = I_Type_Kind_Darray;
   result->item = read_schema_type_ref(r, schema);
  }break;
  default:
  {
   log_error("%s load: schema has unknown type ref kind %u", schema->label, ref_kind);
   r->ok = false;
  }break;
 }
 if(not r->ok){ result = 0; }
 return result;
}
function b32
read_schema_count(Binary_Reader *r, char const *label, u32 *count)
{// NOTE(kv) Bound the table sizes by the bytes left, so a corrupt count can't blow the arena.
 *count = read_binary_u32(r);
 if(r->ok and isize(*count) > (r->end_pos - r->pos))
 {
  log_error("%s load: schema count %u exceeds the file", label, *count);
  r->ok = false;
 }
 return r->ok;
}
function void
read_schema_type(Binary_Reader *r, File_Schema *schema)
{
 File_Type *type = push_struct0(schema->arena, File_Type);
 type->name = read_schema_string(r);
 type->kind = cast(I_Type_Kind)read_binary_u32(r);
 type->size = cast(i32)read_binary_u32(r);
 if(not r->ok){ return; }
 switch(type->kind)
 {
  case I_Type_Kind_Basic: break;
  case I_Type_Kind_Wrapper:
  case I_Type_Kind_Struct:
  {
   u32 count = 0;
   if(not read_schema_count(r, schema->label, &count)){ return; }
   init_dynamic(type->members, schema->arena, cast(i32)count);
   for_u32(index, 0, count)
   {
    File_Member member = {};
    member.name = read_schema_string(r);
    member.type = read_schema_type_ref(r, schema);
    member.discriminator = read_schema_string(r);
    if(not r->ok){ return; }
    push(&type->members, member);
   }
  }break;
  case I_Type_Kind_Enum:
  {
   u32 count = 0;
   if(not read_schema_count(r, schema->label, &count)){ return; }
   init_dynamic(type->enum_members, schema->arena, cast(i32)count);
   for_u32(index, 0, count)
   {
    File_Enum_Member member = {};
    member.name  = read_schema_string(r);
    member.value = read_binary_i1(r);
    if(not r->ok){ return; }
    push(&type->enum_members, member);
   }
  }break;
  case I_Type_Kind_Union:
  {
   type->discriminator_type = read_schema_string(r);
   u32 count = 0;
   if(not read_schema_count(r, schema->label, &count)){ return; }
   init_dynamic(type->union_members, schema->arena, cast(i32)count);
   for_u32(index, 0, count)
   {
    File_Union_Member member = {};
    member.name    = read_schema_string(r);
    member.type    = read_schema_type_ref(r, schema);
    member.variant = read_binary_i1(r);
    if(not r->ok){ return; }
    push(&type->union_members, member);
   }
  }break;
  default:
  {
   log_error("%s load: schema type \"%.*s\" has unknown kind %d", schema->label, strexpand(type->name), type->kind);
   r->ok = false;
   return;
  }
 }
 if(schema_find_type(schema, type->name))
 {
  log_error("%s load: schema lists type \"%.*s\" twice", schema->label, strexpand(type->name));
  r->ok = false;
  return;
 }
 push(&schema->types, type);
}
function b32
read_schema(Binary_Reader *r, File_Schema *schema)
{
 u32 type_count = 0;
 if(not read_schema_count(r, schema->label, &type_count)){ return false; }
 init_dynamic(schema->types, schema->arena, cast(i32)type_count);
 for_u32(index, 0, type_count)
 {
  read_schema_type(r, schema);
  if(not r->ok){ return false; }
 }
 String root_name = read_schema_string(r);
 schema->root = schema_find_type(schema, root_name);
 if(r->ok and not schema->root)
 {
  log_error("%s load: schema root \"%.*s\" is not in the type table", schema->label, strexpand(root_name));
  r->ok = false;
 }
 return r->ok;
}

//~ Reading: the value

struct Schema_Reader
{
 Binary_Reader *r;
 File_Schema *schema;
 Arena *arena;  // NOTE(kv) where darray items and String bytes go (the document's arena)
 char const *label;
};

function void read_schema_value(Schema_Reader *ctx, File_Type *ft, Type_Info *lt, u8 *dst);

function i32
find_live_member_index(Type_Info *type, String name)
{// NOTE(kv) find_member_index_by_name asserts Struct; wrappers walk the same way here.
 for_i32(index, 0, type->members.count)
 {
  if(type->members[index].name == name){ return index; }
 }
 return -1;
}
function b32
kinds_compatible(File_Type *ft, Type_Info *lt)
{
 b32 file_structish = (ft->kind == I_Type_Kind_Struct or ft->kind == I_Type_Kind_Wrapper);
 b32 live_structish = (lt->kind == I_Type_Kind_Struct or lt->kind == I_Type_Kind_Wrapper);
 if(file_structish or live_structish){ return file_structish and live_structish; }
 return ft->kind == lt->kind;
}
function b32
schema_is_string(File_Type *ft)
{
 return ft->kind == I_Type_Kind_Basic and ft->name == strlit("String");
}
function b32
live_is_string(Type_Info *lt)
{
 return lt->kind == I_Type_Kind_Basic and lt->Basic_Type == Basic_Type_String;
}

function i32
remap_enum_value(Schema_Reader *ctx, File_Type *ft, Type_Info *lt, i32 file_value)
{// NOTE(kv) file value -> file name -> live value. Generalises the old vis-tag-by-name
 // patch: reordering/inserting enum members never retags saved data.
 String name = {};
 for_i32(index, 0, ft->enum_members.count)
 {
  if(ft->enum_members.items[index].value == file_value){ name = ft->enum_members.items[index].name; break; }
 }
 if(name.len == 0)
 {
  log_error("%s load: enum %.*s value %d has no name in the file schema, using 0", ctx->label, strexpand(ft->name), file_value);
  return 0;
 }
 for_i32(index, 0, lt->enum_members.count)
 {
  if(lt->enum_members[index].name == name){ return lt->enum_members[index].value; }
 }
 log_error("%s load: enum %.*s has no member named \"%.*s\" any more (renamed/removed?), using 0",
           ctx->label, strexpand(lt->name), strexpand(name));
 return 0;
}

function void
report_missing_live_members(Schema_Reader *ctx, File_Type *ft, Type_Info *lt)
{// NOTE(kv) Once per (file type, live type): live members the file doesn't have stay zero.
 if(ft->checked_against == lt){ return; }
 ft->checked_against = lt;
 for_i32(index, 0, lt->members.count)
 {
  I_Struct_Member &member = lt->members[index];
  if(member.unserialized){ continue; }
  b32 found = false;
  for_i32(findex, 0, ft->members.count){ if(ft->members.items[findex].name == member.name){ found = true; break; } }
  if(not found)
  {
   log_string("%s load: %.*s.%.*s is not in the file, zeroed", ctx->label, strexpand(lt->name), strexpand(member.name));
  }
 }
}

function void
read_schema_struct(Schema_Reader *ctx, File_Type *ft, Type_Info *lt, u8 *dst)
{// NOTE(kv) lt/dst null = skip mode (consume the bytes, store nothing). Walks the FILE
 // member order; each member finds its live twin by name.
 Binary_Reader *r = ctx->r;
 Scratch_Scope tmp;
 // NOTE(kv) Raw file enum values by file member index: a union later in the struct
 // needs its discriminator's FILE value to know which variant follows.
 i32 *file_enum_values = push_array0(tmp, i32, maximum(1, ft->members.count));
 if(lt)
 {
  block_zero(dst, lt->size);
  report_missing_live_members(ctx, ft, lt);
 }
 for_i32(findex, 0, ft->members.count)
 {
  File_Member &fm = ft->members.items[findex];
  I_Struct_Member *lm = 0;
  if(lt)
  {
   i32 lindex = find_live_member_index(lt, fm.name);
   if(lindex >= 0){ lm = &lt->members[lindex]; }
   else if(not fm.warned)
   {
    fm.warned = true;
    log_string("%s load: %.*s.%.*s is not in the code any more, skipped", ctx->label, strexpand(ft->name), strexpand(fm.name));
   }
  }
  u8 *member_dst = lm ? dst + lm->offset : 0;
  Type_Info *lmt = lm ? lm->type : 0;
  if(lmt and not kinds_compatible(fm.type, lmt))
  {
   if(not fm.warned)
   {
    fm.warned = true;
    log_error("%s load: %.*s.%.*s changed kind (file %d, code %d), skipped and zeroed",
              ctx->label, strexpand(ft->name), strexpand(fm.name), fm.type->kind, lmt->kind);
   }
   lmt = 0; member_dst = 0;
  }

  switch(fm.type->kind)
  {
   case I_Type_Kind_Enum:
   {
    i32 file_value = read_binary_i1(r);
    if(not r->ok){ return; }
    file_enum_values[findex] = file_value;
    if(lmt)
    {
     i32 live_value = remap_enum_value(ctx, fm.type, lmt, file_value);
     block_copy(member_dst, &live_value, lmt->size);
    }
   }break;
   case I_Type_Kind_Union:
   {
    i32 dindex = -1;
    for_i32(index, 0, findex)
    {
     if(ft->members.items[index].name == fm.discriminator and
        ft->members.items[index].type->kind == I_Type_Kind_Enum){ dindex = index; break; }
    }
    if(dindex < 0)
    {
     log_error("%s load: union %.*s.%.*s: discriminator \"%.*s\" is not an enum member before it in the schema",
               ctx->label, strexpand(ft->name), strexpand(fm.name), strexpand(fm.discriminator));
     r->ok = false; return;
    }
    i32 variant = file_enum_values[dindex];
    File_Union_Member *fum = 0;
    for_i32(index, 0, fm.type->union_members.count)
    {
     if(fm.type->union_members.items[index].variant == variant){ fum = &fm.type->union_members.items[index]; break; }
    }
    if(not fum)
    {
     log_error("%s load: union %.*s.%.*s: no variant for discriminator value %d in the file schema",
               ctx->label, strexpand(ft->name), strexpand(fm.name), variant);
     r->ok = false; return;
    }
    Type_Info *lum_type = 0;
    if(lmt)
    {
     for_i32(index, 0, lmt->union_members.count)
     {
      I_Union_Member &lum = lmt->union_members[index];
      if(lum.name == fum->name){ lum_type = lum.type; break; }
     }
     if(not lum_type)
     {
      log_error("%s load: union %.*s.%.*s: variant \"%.*s\" is not in the code any more, skipped and zeroed",
                ctx->label, strexpand(ft->name), strexpand(fm.name), strexpand(fum->name));
     }
     else if(not kinds_compatible(fum->type, lum_type))
     {
      log_error("%s load: union %.*s.%.*s: variant \"%.*s\" changed kind, skipped and zeroed",
                ctx->label, strexpand(ft->name), strexpand(fm.name), strexpand(fum->name));
      lum_type = 0;
     }
    }
    // NOTE(kv) The union's storage is the member itself (pointer of member == pointer to union).
    read_schema_value(ctx, fum->type, lum_type, lum_type ? member_dst : 0);
   }break;
   default:
   {
    read_schema_value(ctx, fm.type, lmt, member_dst);
   }break;
  }
  if(not r->ok){ return; }
 }
}

function void
read_schema_value(Schema_Reader *ctx, File_Type *ft, Type_Info *lt, u8 *dst)
{// NOTE(kv) lt/dst null = skip mode. Caller guarantees kinds_compatible when lt is set.
 Binary_Reader *r = ctx->r;
 switch(ft->kind)
 {
  case I_Type_Kind_Basic:
  {
   if(schema_is_string(ft))
   {
    String bytes = {};
    read_binary_String(r, &bytes);  // NOTE(kv) points into the file buffer
    if(not r->ok){ return; }
    if(lt)
    {
     if(live_is_string(lt))
     {
      u8 *copy = cast(u8 *)push_size(ctx->arena, bytes.len + 1);
      block_copy(copy, bytes.str, bytes.len);
      copy[bytes.len] = 0;  // NOTE(kv) Recorded_Image.filename is used as a C string
      String *live = cast(String *)dst;
      *live = {};
      live->str = copy;
      live->len = bytes.len;
     }
     else
     {
      log_error("%s load: a String in the file lands on basic type %.*s in the code, zeroed", ctx->label, strexpand(lt->name));
     }
    }
   }
   else
   {
    if(lt and not live_is_string(lt) and lt->size == ft->size)
    {// NOTE(kv) Same-size basics copy raw (argb/Line_Flags are u32 under other names).
     read_binary_size(r, ft->size, dst);
    }
    else
    {
     if(lt)
     {
      log_error("%s load: basic %.*s (%d bytes) in the file vs %.*s (%d bytes) in the code, zeroed",
                ctx->label, strexpand(ft->name), ft->size, strexpand(lt->name), lt->size);
     }
     if(not reader_can_take(r, ft->size, 1)){ r->ok = false; return; }
     r->pos += ft->size;
    }
   }
  }break;
  case I_Type_Kind_Wrapper:
  case I_Type_Kind_Struct:
  {
   read_schema_struct(ctx, ft, lt, dst);
  }break;
  case I_Type_Kind_Enum:
  {// NOTE(kv) Enums outside a struct (array items) still remap by name.
   i32 file_value = read_binary_i1(r);
   if(not r->ok){ return; }
   if(lt)
   {
    i32 live_value = remap_enum_value(ctx, ft, lt, file_value);
    block_copy(dst, &live_value, lt->size);
   }
  }break;
  case I_Type_Kind_Union:
  {
   log_error("%s load: union %.*s outside a struct (no discriminator), rejecting file", ctx->label, strexpand(ft->name));
   r->ok = false;
  }break;
  case I_Type_Kind_Array:
  {
   Type_Info *item_lt = 0;
   i32 live_count = 0;
   if(lt)
   {
    item_lt = lt->array_item_type;
    live_count = lt->count;
    if(not kinds_compatible(ft->item, item_lt))
    {
     log_error("%s load: array items changed kind (file %d, code %d), zeroed", ctx->label, ft->item->kind, item_lt->kind);
     item_lt = 0;
    }
    else if(live_count != ft->count)
    {
     log_string("%s load: array count %d in the file vs %d in the code, extra dropped / missing zeroed",
                ctx->label, ft->count, live_count);
    }
    block_zero(dst, lt->size);
   }
   for_i32(index, 0, ft->count)
   {
    b32 keep = (item_lt and index < live_count);
    read_schema_value(ctx, ft->item, keep ? item_lt : 0, keep ? dst + item_lt->size*index : 0);
    if(not r->ok){ return; }
   }
  }break;
  case I_Type_Kind_Darray:
  {
   u32 count = read_binary_u32(r);
   if(not r->ok){ return; }
   if(count > cast(u32)(r->end_pos - r->pos))
   {// NOTE(kv) Every item is at least one byte; cheap sanity bound before allocating.
    log_error("%s load: darray count %u exceeds the file", ctx->label, count);
    r->ok = false; return;
   }
   Type_Info *item_lt = 0;
   if(lt)
   {
    item_lt = lt->array_item_type;
    if(not kinds_compatible(ft->item, item_lt))
    {
     log_error("%s load: darray items changed kind (file %d, code %d), left empty", ctx->label, ft->item->kind, item_lt->kind);
     item_lt = 0;
    }
   }
   u8 *items = 0;
   if(item_lt)
   {// NOTE(kv) Every darray(T) shares the Dynamic_Array header layout (see write_binary_func).
    darray(u8) *array = cast(darray(u8) *)dst;
    *array = {};
    array->arena = ctx->arena;
    items = push_size_zero(ctx->arena, maximum(1, cast(i32)count * item_lt->size));
    array->items = items;
    array->count = cast(i32)count;
    array->cap   = cast(i32)count;
    array->Static_Array2<u8>::cap = cast(i32)count;
   }
   for_u32(index, 0, count)
   {
    read_schema_value(ctx, ft->item, item_lt, item_lt ? items + item_lt->size*index : 0);
    if(not r->ok){ return; }
   }
  }break;
  InvalidDefaultCase;
 }
}

//~ Document load through the schema

function b32
read_document_schema_file(Binary_Reader *r, Arena *arena, Document_File *out, char const *label)
{// NOTE(kv) false = rejected (already logged). `out` gets darrays living in `arena`.
 Scratch_Scope tmp;
 u32 magic = read_binary_u32(r);
 u32 tag   = read_binary_u32(r);
 u64 timestamp = read_binary_u64(r); (void)timestamp;
 if(not r->ok or magic != autodraw_data_magic or tag != schema_format_tag)
 {
  log_error("%s load: not a schema-format file (magic %08x tag %08x)", label, magic, tag);
  return false;
 }
 File_Schema schema = {};
 schema.arena = tmp;
 schema.label = label;
 if(not read_schema(r, &schema)){ return false; }
 if(not kinds_compatible(schema.root, &Type_Info_Document_File))
 {
  log_error("%s load: schema root \"%.*s\" is not a struct", label, strexpand(schema.root->name));
  return false;
 }
 Schema_Reader ctx = {};
 ctx.r = r;
 ctx.schema = &schema;
 ctx.arena = arena;
 ctx.label = label;
 read_schema_value(&ctx, schema.root, &Type_Info_Document_File, cast(u8 *)out);
 read_debug_string(r, strlit("EOF"));
 if(not r->ok)
 {
  log_error("%s load: value does not match its schema (truncated/corrupt), rejecting file", label);
 }
 return r->ok;
}

function b32
load_document_schema_file(Game_State *state, Stringz path)
{// NOTE(kv) Reads into a fresh arena and swaps it in only on success, so a rejected
 // file leaves the live document untouched (the banner says REJECTED).
 Scratch_Scope tmp;
 String file_data = read_entire_file(tmp, path);
 if(file_data.len == 0)
 {
  log_string("document load: no file at %S", path);
  return false;
 }
 Binary_Reader reader = make_binary_reader(file_data.data, file_data.size);
 Arena arena = make_arena();
 Document_File value = {};
 b32 ok = read_document_schema_file(&reader, &arena, &value, "document");
 if(ok)
 {
  Recording &doc = state->model.recordings.document;
  arena_free(&doc.arena);
  doc.arena      = arena;
  doc.vertices   = value.vertices;
  doc.groups     = value.groups;
  doc.primitives = value.primitives;
  doc.captured   = true;
  // NOTE(kv) The darray headers must point at the arena they now live in.
  doc.vertices.arena = doc.groups.arena = doc.primitives.arena = &doc.arena;
  log_string("document load: %d primitives, %d groups, %d vertices from %S (schema)",
             doc.primitives.count, doc.groups.count, doc.vertices.count, path);
 }
 else
 {
  arena_free(&arena);
  log_error("document load: REJECTED (%S)", path);
 }
 return ok;
}
//-EOF
