//-NOTE(kv) Self-describing document format (plan-document-self-describing-format, Q12/Q13):
// file = magic + format tag + timestamp + SCHEMA of the root type + one VALUE of the
// root type + "EOF". The schema lists every named type reachable from the root
// (Document_File): name, kind, size, struct members (name + type ref + discriminator
// member name for unions), enum members (name + value), union members (name + type ref +
// discriminator value). No offsets: the reader matches file members to Type_Info members
// BY NAME, skips unknown members using the file schema, zeroes missing ones, remaps enum
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
 write_size(writer, string.str, len);  // NOTE(kv) len 0 is a no-op (empty discriminator names)
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
// NOTE(kv) Naming: the reader always holds two descriptions of the same type side by
// side. `Type_Info_In_File` (and `*_In_File`, variables `in_file`) is what the struct
// looked like when the file was saved, parsed from the schema. `Type_Info` (variables
// `type_info`) is what it looks like in the running code, generated by ad_meta.
struct Type_Info_In_File;
struct Member_In_File
{
 String name;
 Type_Info_In_File *type;
 String discriminator;  // NOTE(kv) union members only: name of the enum member holding the variant
 b32 warned;            // NOTE(kv) "not in the code any more" logged once per type, not per item
};
struct Union_Member_In_File
{
 String name;
 Type_Info_In_File *type;
 i32 variant;
};
struct Enum_Member_In_File
{
 String name;
 i32 value;
};
struct Type_Info_In_File
{
 String name;  // NOTE(kv) empty for the anonymous array/darray refs
 I_Type_Kind kind;
 i32 size;
 darray(Member_In_File) members;
 darray(Enum_Member_In_File) enum_members;
 String discriminator_type;
 darray(Union_Member_In_File) union_members;
 i32 count;         // NOTE(kv) Array
 Type_Info_In_File *item;   // NOTE(kv) Array / Darray
 Type_Info *checked_against;  // NOTE(kv) Type_Info whose missing members were already reported
};
struct Schema_In_File
{
 Arena *arena;  // NOTE(kv) schema tables (scratch; strings point into the file buffer)
 darray(Type_Info_In_File *) types;
 Type_Info_In_File *root;
 char const *label;
};

function Type_Info_In_File *
schema_find_type(Schema_In_File *schema, String name)
{
 for_i32(index, 0, schema->types.count)
 {
  Type_Info_In_File *type = schema->types.items[index];
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
function Type_Info_In_File *
read_schema_type_ref(Binary_Reader *r, Schema_In_File *schema)
{
 u32 ref_kind = read_binary_u32(r);
 if(not r->ok){ return 0; }
 Type_Info_In_File *result = 0;
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
   result = push_struct0(schema->arena, Type_Info_In_File);
   result->kind  = I_Type_Kind_Array;
   result->count = cast(i32)read_binary_u32(r);
   result->item  = read_schema_type_ref(r, schema);
   if(result->item){ result->size = result->count * result->item->size; }
  }break;
  case Schema_Ref_Darray:
  {
   result = push_struct0(schema->arena, Type_Info_In_File);
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
read_schema_type(Binary_Reader *r, Schema_In_File *schema)
{
 Type_Info_In_File *type = push_struct0(schema->arena, Type_Info_In_File);
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
    Member_In_File member = {};
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
    Enum_Member_In_File member = {};
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
    Union_Member_In_File member = {};
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
read_schema(Binary_Reader *r, Schema_In_File *schema)
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
 Schema_In_File *schema;
 Arena *arena;  // NOTE(kv) where darray items and String bytes go (the document's arena)
 char const *label;
};

function void read_schema_value(Schema_Reader *ctx, Type_Info_In_File *in_file, Type_Info *type_info, u8 *dst);

function i32
find_type_info_member_index(Type_Info *type, String name)
{// NOTE(kv) find_member_index_by_name asserts Struct; wrappers walk the same way here.
 for_i32(index, 0, type->members.count)
 {
  if(type->members[index].name == name){ return index; }
 }
 return -1;
}
function b32
kinds_compatible(Type_Info_In_File *in_file, Type_Info *type_info)
{
 b32 in_file_structish = (in_file->kind == I_Type_Kind_Struct or in_file->kind == I_Type_Kind_Wrapper);
 b32 type_info_structish = (type_info->kind == I_Type_Kind_Struct or type_info->kind == I_Type_Kind_Wrapper);
 if(in_file_structish or type_info_structish){ return in_file_structish and type_info_structish; }
 return in_file->kind == type_info->kind;
}
function b32
in_file_is_string(Type_Info_In_File *in_file)
{
 return in_file->kind == I_Type_Kind_Basic and in_file->name == strlit("String");
}
function b32
type_info_is_string(Type_Info *type_info)
{
 return type_info->kind == I_Type_Kind_Basic and type_info->Basic_Type == Basic_Type_String;
}

function i32
remap_enum_value(Schema_Reader *ctx, Type_Info_In_File *in_file, Type_Info *type_info, i32 file_value)
{// NOTE(kv) file value -> file name -> value in the code. Generalises the old vis-tag-by-
 // name patch: inserting an enum member in code no longer shifts the meaning of the
 // integers already saved (the nose became Vis_Ref_Front_4 once).
 String name = {};
 for_i32(index, 0, in_file->enum_members.count)
 {
  if(in_file->enum_members.items[index].value == file_value){ name = in_file->enum_members.items[index].name; break; }
 }
 if(name.len == 0)
 {
  log_error("%s load: enum %.*s value %d has no name in the file schema, using 0", ctx->label, strexpand(in_file->name), file_value);
  return 0;
 }
 for_i32(index, 0, type_info->enum_members.count)
 {
  if(type_info->enum_members[index].name == name){ return type_info->enum_members[index].value; }
 }
 log_error("%s load: enum %.*s has no member named \"%.*s\" any more (renamed/removed?), using 0",
           ctx->label, strexpand(type_info->name), strexpand(name));
 return 0;
}

function void
report_members_missing_in_file(Schema_Reader *ctx, Type_Info_In_File *in_file, Type_Info *type_info)
{// NOTE(kv) Once per (Type_Info_In_File, Type_Info) pair: Type_Info members the file doesn't have stay zero.
 if(in_file->checked_against == type_info){ return; }
 in_file->checked_against = type_info;
 for_i32(index, 0, type_info->members.count)
 {
  I_Struct_Member &member = type_info->members[index];
  if(member.unserialized){ continue; }
  b32 found = false;
  for_i32(index_in_file, 0, in_file->members.count){ if(in_file->members.items[index_in_file].name == member.name){ found = true; break; } }
  if(not found)
  {
   log_string("%s load: %.*s.%.*s is not in the file, zeroed", ctx->label, strexpand(type_info->name), strexpand(member.name));
  }
 }
}

function void
read_schema_struct(Schema_Reader *ctx, Type_Info_In_File *in_file, Type_Info *type_info, u8 *dst)
{// NOTE(kv) type_info/dst null = skip mode (consume the bytes, store nothing). Walks the
 // FILE member order; each member finds its Type_Info twin by name.
 Binary_Reader *r = ctx->r;
 Scratch_Scope tmp;
 // NOTE(kv) Raw file enum values by file member index: a union later in the struct
 // needs its discriminator's FILE value to know which variant follows.
 i32 *file_enum_values = push_array0(tmp, i32, maximum(1, in_file->members.count));
 if(type_info)
 {
  block_zero(dst, type_info->size);
  report_members_missing_in_file(ctx, in_file,type_info);
 }
 for_i32(index_in_file, 0, in_file->members.count)
 {
  Member_In_File &member_in_file =in_file->members.items[index_in_file];
  I_Struct_Member *member = 0;
  if(type_info)
  {
   i32 member_index = find_type_info_member_index(type_info, member_in_file.name);
   if(member_index >= 0){ member = &type_info->members[member_index]; }
   else if(not member_in_file.warned)
   {
    member_in_file.warned = true;
    log_string("%s load: %.*s.%.*s is not in the code any more, skipped", ctx->label, strexpand(in_file->name), strexpand(member_in_file.name));
   }
  }
  u8 *member_dst = member ? dst + member->offset : 0;
  Type_Info *member_type = member ? member->type : 0;
  if(member_type and not kinds_compatible(member_in_file.type, member_type))
  {
   if(not member_in_file.warned)
   {
    member_in_file.warned = true;
    log_error("%s load: %.*s.%.*s changed kind (file %d, code %d), skipped and zeroed",
              ctx->label, strexpand(in_file->name), strexpand(member_in_file.name), member_in_file.type->kind, member_type->kind);
   }
   member_type = 0; member_dst = 0;
  }

  switch(member_in_file.type->kind)
  {
   case I_Type_Kind_Enum:
   {
    i32 file_value = read_binary_i1(r);
    if(not r->ok){ return; }
    file_enum_values[index_in_file] = file_value;
    if(member_type)
    {
     i32 value_in_code = remap_enum_value(ctx, member_in_file.type, member_type, file_value);
     block_copy(member_dst, &value_in_code, member_type->size);
    }
   }break;
   case I_Type_Kind_Union:
   {
    i32 dindex = -1;
    for_i32(index, 0, index_in_file)
    {
     if(in_file->members.items[index].name == member_in_file.discriminator and
        in_file->members.items[index].type->kind == I_Type_Kind_Enum){ dindex = index; break; }
    }
    if(dindex < 0)
    {
     log_error("%s load: union %.*s.%.*s: discriminator \"%.*s\" is not an enum member before it in the schema",
               ctx->label, strexpand(in_file->name), strexpand(member_in_file.name), strexpand(member_in_file.discriminator));
     r->ok = false; return;
    }
    i32 variant = file_enum_values[dindex];
    Union_Member_In_File *union_member_in_file =0;
    for_i32(index, 0, member_in_file.type->union_members.count)
    {
     if(member_in_file.type->union_members.items[index].variant == variant){ union_member_in_file = &member_in_file.type->union_members.items[index]; break; }
    }
    if(not union_member_in_file)
    {
     log_error("%s load: union %.*s.%.*s: no variant for discriminator value %d in the file schema",
               ctx->label, strexpand(in_file->name), strexpand(member_in_file.name), variant);
     r->ok = false; return;
    }
    Type_Info *union_member_type = 0;
    if(member_type)
    {
     for_i32(index, 0, member_type->union_members.count)
     {
      I_Union_Member &union_member =member_type->union_members[index];
      if(union_member.name == union_member_in_file->name){ union_member_type = union_member.type; break; }
     }
     if(not union_member_type)
     {
      log_error("%s load: union %.*s.%.*s: variant \"%.*s\" is not in the code any more, skipped and zeroed",
                ctx->label, strexpand(in_file->name), strexpand(member_in_file.name), strexpand(union_member_in_file->name));
     }
     else if(not kinds_compatible(union_member_in_file->type, union_member_type))
     {
      log_error("%s load: union %.*s.%.*s: variant \"%.*s\" changed kind, skipped and zeroed",
                ctx->label, strexpand(in_file->name), strexpand(member_in_file.name), strexpand(union_member_in_file->name));
      union_member_type = 0;
     }
    }
    // NOTE(kv) The union's storage is the member itself (pointer of member == pointer to union).
    read_schema_value(ctx, union_member_in_file->type, union_member_type, union_member_type ? member_dst : 0);
   }break;
   default:
   {
    read_schema_value(ctx, member_in_file.type, member_type, member_dst);
   }break;
  }
  if(not r->ok){ return; }
 }
}

function void
read_schema_value(Schema_Reader *ctx, Type_Info_In_File *in_file, Type_Info *type_info, u8 *dst)
{// NOTE(kv) type_info/dst null = skip mode. Caller guarantees kinds_compatible when type_info is set.
 Binary_Reader *r = ctx->r;
 switch(in_file->kind)
 {
  case I_Type_Kind_Basic:
  {
   if(in_file_is_string(in_file))
   {
    String bytes = {};
    read_binary_String(r, &bytes);  // NOTE(kv) points into the file buffer
    if(not r->ok){ return; }
    if(type_info)
    {
     if(type_info_is_string(type_info))
     {
      u8 *copy = cast(u8 *)push_size(ctx->arena, bytes.len + 1);
      block_copy(copy, bytes.str, bytes.len);
      copy[bytes.len] = 0;  // NOTE(kv) Recorded_Image.filename is used as a C string
      String *string = cast(String *)dst;
      *string = {};
      string->str = copy;
      string->len = bytes.len;
     }
     else
     {
      log_error("%s load: a String in the file lands on basic type %.*s in the code, zeroed", ctx->label, strexpand(type_info->name));
     }
    }
   }
   else
   {
    if(type_info and not type_info_is_string(type_info) and type_info->size == in_file->size)
    {// NOTE(kv) Same-size basics copy raw (argb/Line_Flags are u32 under other names).
     read_binary_size(r, in_file->size, dst);
    }
    else
    {
     if(type_info)
     {
      log_error("%s load: basic %.*s (%d bytes) in the file vs %.*s (%d bytes) in the code, zeroed",
                ctx->label, strexpand(in_file->name), in_file->size, strexpand(type_info->name), type_info->size);
     }
     if(not reader_can_take(r, in_file->size, 1)){ r->ok = false; return; }
     r->pos += in_file->size;
    }
   }
  }break;
  case I_Type_Kind_Wrapper:
  case I_Type_Kind_Struct:
  {
   read_schema_struct(ctx, in_file,type_info, dst);
  }break;
  case I_Type_Kind_Enum:
  {// NOTE(kv) Enums outside a struct (array items) still remap by name.
   i32 file_value = read_binary_i1(r);
   if(not r->ok){ return; }
   if(type_info)
   {
    i32 value_in_code = remap_enum_value(ctx, in_file,type_info, file_value);
    block_copy(dst, &value_in_code, type_info->size);
   }
  }break;
  case I_Type_Kind_Union:
  {
   log_error("%s load: union %.*s outside a struct (no discriminator), rejecting file", ctx->label, strexpand(in_file->name));
   r->ok = false;
  }break;
  case I_Type_Kind_Array:
  {
   Type_Info *item_type_info = 0;
   i32 count_in_code = 0;
   if(type_info)
   {
    item_type_info = type_info->array_item_type;
    count_in_code = type_info->count;
    if(not kinds_compatible(in_file->item, item_type_info))
    {
     log_error("%s load: array items changed kind (file %d, code %d), zeroed", ctx->label, in_file->item->kind, item_type_info->kind);
     item_type_info = 0;
    }
    else if(count_in_code != in_file->count)
    {
     log_string("%s load: array count %d in the file vs %d in the code, extra dropped / missing zeroed",
                ctx->label, in_file->count, count_in_code);
    }
    block_zero(dst, type_info->size);
   }
   for_i32(index, 0, in_file->count)
   {
    b32 keep = (item_type_info and index < count_in_code);
    read_schema_value(ctx, in_file->item, keep ? item_type_info : 0, keep ? dst + item_type_info->size*index : 0);
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
   Type_Info *item_type_info = 0;
   if(type_info)
   {
    item_type_info = type_info->array_item_type;
    if(not kinds_compatible(in_file->item, item_type_info))
    {
     log_error("%s load: darray items changed kind (file %d, code %d), left empty", ctx->label, in_file->item->kind, item_type_info->kind);
     item_type_info = 0;
    }
   }
   u8 *items = 0;
   if(item_type_info)
   {// NOTE(kv) Every darray(T) shares the Dynamic_Array header layout (see write_binary_func).
    darray(u8) *array = cast(darray(u8) *)dst;
    *array = {};
    array->arena = ctx->arena;
    items = push_size_zero(ctx->arena, maximum(1, cast(i32)count * item_type_info->size));
    array->items = items;
    array->count = cast(i32)count;
    array->cap   = cast(i32)count;
    array->Static_Array2<u8>::cap = cast(i32)count;
   }
   for_u32(index, 0, count)
   {
    read_schema_value(ctx, in_file->item, item_type_info, item_type_info ? items + item_type_info->size*index : 0);
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
 Schema_In_File schema = {};
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
load_document_schema_file(Game_State *state, Stringz path, String file_data)
{// NOTE(kv) Reads into a fresh arena and swaps it in only on success, so a rejected
 // file leaves the live document untouched (the banner says REJECTED).
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
