// NOTE(kv) Text serialization over Type_Info: struct-literal format for the state file
// (plan-presets-text-file Q2/Q12). Generic -- knows nothing about Serialized_State.
//
//   kb_cursor = {on = 1, pos = {0.1, -0.08, 0.08}, vel = 0.5}
//   presets = [
//     {name = "head profile", viz_level = 2, scene = Scene_Head_Profile, show_grid = 1},
//   ]
//
// - structs `{member = value, ...}`; the top level is a struct body without braces,
//   one member per line. The writer omits members whose bytes are all zero.
// - vectors (v2..v4, i2..i4) positional `{...}`; arrays `[...]` (reader accepts either
//   bracket for both). An array member with an `<name>_count` i32 sibling is a variable
//   list: the writer emits that many items, the reader sets the sibling to the number read.
// - enums by member name (unknown name -> zero + log); `char[N]` and String as one
//   quoted string (`\"`, `\\`, `\n` escapes); floats `%.9g`; `#` line comments;
//   trailing commas allowed.
// - reader: a struct value zeroes its destination first, so a member missing from the
//   file is its zero default; unknown member -> skip one balanced value + log; any
//   syntax error -> `ok = false`, the caller rejects the whole file.

//~ Writing

function b32
text_bytes_all_zero(u8 *pointer, i32 size)
{
 for_i32(i, 0, size){ if(pointer[i]){ return false; } }
 return true;
}
function I_Struct_Member *
text_find_count_sibling(Type_Info *type, String array_member_name)
{// NOTE(kv) `<name>_count` i32 member next to an array member, or 0.
 Scratch_Scope tmp;
 String count_name = push_stringf(tmp, "%S_count", array_member_name);
 for_i32(index, 0, type->members.count)
 {
  I_Struct_Member &member = type->members[index];
  if(member.name == count_name and member.type->kind == I_Type_Kind_Basic and
     member.type->Basic_Type == Basic_Type_i1)
  {
   return &member;
  }
 }
 return 0;
}
function void
text_print_quoted(Printer &p, u8 *chars, i32 count)
{
 print(p, '"');
 for_i32(i, 0, count)
 {
  char c = cast(char)chars[i];
  if(c == '"' or c == '\\'){ print(p, '\\'); print(p, c); }
  else if(c == '\n'){ print(p, "\\n"); }
  else { print(p, c); }
 }
 print(p, '"');
}
function void
text_print_indent(Printer &p, i32 indent)
{
 for_i32(i, 0, indent){ print(p, "  "); }
}

function void write_text_value(Printer &p, Type_Info *type, void *pointer, i32 indent);

function void
write_text_struct_body(Printer &p, Type_Info *type, u8 *pointer, i32 indent, b32 multiline)
{// NOTE(kv) The members, comma-separated (multiline: one per line, the top-level layout).
 b32 first = true;
 for_i32(member_index, 0, type->members.count)
 {
  I_Struct_Member &member = type->members[member_index];
  if(member.unserialized){ continue; }
  if(member.type->kind == I_Type_Kind_Union){ continue; }  // NOTE(kv) not supported in text
  u8 *member_data = pointer + member.offset;
  i32 list_count = -1;  // -1 = not a variable list
  if(member.type->kind == I_Type_Kind_Array)
  {
   I_Struct_Member *count_member = text_find_count_sibling(type, member.name);
   if(count_member)
   {
    list_count = *cast(i32 *)(pointer + count_member->offset);
    list_count = clamp_between(0, list_count, member.type->count);
   }
  }
  {// NOTE(kv) The `_count` sibling itself is implied by the list length.
   b32 is_count_sibling = false;
   i32 name_len = cast(i32)member.name.len;
   if(name_len > 6 and member.name.str[name_len-6] == '_' and
      SCu8(cast(char *)member.name.str + name_len-6, 6) == "_count")
   {
    for_i32(other, 0, type->members.count)
    {
     I_Struct_Member &array_member = type->members[other];
     if(array_member.type->kind == I_Type_Kind_Array and
        text_find_count_sibling(type, array_member.name) == &member)
     {
      is_count_sibling = true;
     }
    }
   }
   if(is_count_sibling){ continue; }
  }
  if(list_count < 0 and text_bytes_all_zero(member_data, member.type->size)){ continue; }

  if(not first){ print(p, multiline ? "," : ", "); }
  if(multiline){ print(p, '\n'); text_print_indent(p, indent); }
  first = false;
  print(p, member.name);
  print(p, " = ");
  if(list_count >= 0)
  {//-Variable list
   Type_Info *item_type = member.type->array_item_type;
   print(p, '[');
   for_i32(item_index, 0, list_count)
   {
    print(p, '\n'); text_print_indent(p, indent+1);
    write_text_value(p, item_type, member_data + item_type->size*item_index, indent+1);
    print(p, ',');
   }
   if(list_count){ print(p, '\n'); text_print_indent(p, indent); }
   print(p, ']');
  }
  else
  {
   write_text_value(p, member.type, member_data, indent);
  }
 }
}
function void
write_text_value(Printer &p, Type_Info *type, void *void_pointer, i32 indent)
{
 u8 *pointer = cast(u8 *)void_pointer;
 switch(type->kind)
 {
  case I_Type_Kind_Basic:
  {
   switch(type->Basic_Type)
   {
    case Basic_Type_v1:{ printf(p, "%.9g", *cast(v1 *)pointer); }break;
    case Basic_Type_v2: case Basic_Type_v3: case Basic_Type_v4:
    {
     v1 *values = cast(v1 *)pointer;
     i32 count = type->size / 4;
     print(p, '{');
     for_i32(i, 0, count){ if(i){ print(p, ", "); } printf(p, "%.9g", values[i]); }
     print(p, '}');
    }break;
    case Basic_Type_i1:{ printf(p, "%d", *cast(i32 *)pointer); }break;
    case Basic_Type_i16:{ printf(p, "%d", cast(i32)*cast(i16 *)pointer); }break;
    case Basic_Type_i2: case Basic_Type_i3: case Basic_Type_i4:
    {
     i32 *values = cast(i32 *)pointer;
     i32 count = type->size / 4;
     print(p, '{');
     for_i32(i, 0, count){ if(i){ print(p, ", "); } printf(p, "%d", values[i]); }
     print(p, '}');
    }break;
    case Basic_Type_u32:{ printf(p, "%u", *cast(u32 *)pointer); }break;
    case Basic_Type_u64:{ printf(p, "%llu", *cast(u64 *)pointer); }break;
    case Basic_Type_String:
    {
     String *string = cast(String *)pointer;
     text_print_quoted(p, string->str, cast(i32)string->len);
    }break;
    case Basic_Type_char:{ text_print_quoted(p, pointer, 1); }break;
    InvalidDefaultCase;
   }
  }break;
  case I_Type_Kind_Struct:
  {
   print(p, '{');
   write_text_struct_body(p, type, pointer, indent, false);
   print(p, '}');
  }break;
  case I_Type_Kind_Array:
  {
   Type_Info *item_type = type->array_item_type;
   if(item_type->kind == I_Type_Kind_Basic and item_type->Basic_Type == Basic_Type_char)
   {// NOTE(kv) char[N] = nul-terminated string
    i32 len = 0;
    while(len < type->count and pointer[len]){ len++; }
    text_print_quoted(p, pointer, len);
   }
   else
   {
    print(p, '[');
    for_i32(item_index, 0, type->count)
    {
     if(item_index){ print(p, ", "); }
     write_text_value(p, item_type, pointer + item_type->size*item_index, indent);
    }
    print(p, ']');
   }
  }break;
  case I_Type_Kind_Enum:
  {
   i32 value = read_enum(*type, pointer);
   b32 found = false;
   for_i32(i, 0, type->enum_members.count)
   {
    if(type->enum_members[i].value == value)
    {
     print(p, type->enum_members[i].name);
     found = true;
     break;
    }
   }
   if(not found){ printf(p, "%d", value); }
  }break;
  case I_Type_Kind_Wrapper:
  {
   write_text_value(p, type->wrapped_type, pointer, indent);
  }break;
  case I_Type_Kind_Union:
  {
   invalid_code_path;  // NOTE(kv) no variant information; struct bodies skip unions
  }break;
  InvalidDefaultCase;
 }
}
function void
write_text_top_level(Printer &p, Type_Info *type, void *pointer)
{// NOTE(kv) A whole file: the struct body without braces, one member per line.
 kv_assert(type->kind == I_Type_Kind_Struct);
 print(p, "# ");
 print(p, type->name);
 print(p, " -- written by autodraw; zero members are omitted, unknown ones are skipped.");
 write_text_struct_body(p, type, cast(u8 *)pointer, 0, true);
 print(p, '\n');
}

//~ Reading

enum Text_Token_Kind
{
 Text_Token_End = 0,
 Text_Token_Word,    // identifier or number
 Text_Token_String,  // quoted, `text` = unescaped contents
 Text_Token_Punct,   // one of { } [ ] = ,
};
struct Text_Token
{
 Text_Token_Kind kind;
 String text;
 i32 line;
};
struct Text_Reader
{
 b32 ok;
 u8 *at;
 u8 *end;
 i32 line;
 Arena *arena;       // NOTE(kv) unescaped strings and String members live here
 char const *label;  // for log lines
 b32 has_peek;
 Text_Token peek;
};
function Text_Reader
make_text_reader(String text, Arena *arena, char const *label)
{
 Text_Reader r = {};
 r.ok = true;
 r.at = text.str;
 r.end = text.str + text.len;
 r.line = 1;
 r.arena = arena;
 r.label = label;
 return r;
}
function void
text_error(Text_Reader *r, i32 line, char const *what)
{
 if(r->ok){ log_error("%s: line %d: %s", r->label, line, what); }
 r->ok = false;
}
function b32
text_is_word_char(u8 c)
{
 return (('a' <= c and c <= 'z') or ('A' <= c and c <= 'Z') or ('0' <= c and c <= '9') or
         c == '_' or c == '.' or c == '-' or c == '+');
}
function Text_Token
text_lex(Text_Reader *r)
{
 Text_Token token = {};
 //-Skip whitespace and comments
 for(;;)
 {
  while(r->at < r->end and (*r->at == ' ' or *r->at == '\t' or *r->at == '\r' or *r->at == '\n'))
  {
   if(*r->at == '\n'){ r->line++; }
   r->at++;
  }
  if(r->at < r->end and *r->at == '#')
  {
   while(r->at < r->end and *r->at != '\n'){ r->at++; }
   continue;
  }
  break;
 }
 token.line = r->line;
 if(r->at >= r->end){ token.kind = Text_Token_End; return token; }
 u8 c = *r->at;
 if(c == '{' or c == '}' or c == '[' or c == ']' or c == '=' or c == ',')
 {
  token.kind = Text_Token_Punct;
  token.text = SCu8(cast(char *)r->at, 1);
  r->at++;
 }
 else if(c == '"')
 {
  r->at++;
  token.kind = Text_Token_String;
  // NOTE(kv) Unescape into the arena (the result is never longer than the source).
  u8 *start = r->at;
  while(r->at < r->end and *r->at != '"'){ if(*r->at == '\\'){ r->at++; } r->at++; }
  if(r->at >= r->end){ text_error(r, token.line, "unterminated string"); return token; }
  u8 *out = cast(u8 *)push_size(r->arena, (r->at - start) + 1);
  i32 len = 0;
  for(u8 *src = start; src < r->at; src++)
  {
   u8 ch = *src;
   if(ch == '\\' and src+1 < r->at)
   {
    src++;
    ch = (*src == 'n') ? '\n' : *src;
   }
   out[len++] = ch;
  }
  out[len] = 0;
  token.text = SCu8(cast(char *)out, cast(u64)len);
  r->at++;  // closing quote
 }
 else if(text_is_word_char(c))
 {
  token.kind = Text_Token_Word;
  u8 *start = r->at;
  while(r->at < r->end and text_is_word_char(*r->at)){ r->at++; }
  token.text = SCu8(cast(char *)start, cast(u64)(r->at - start));
 }
 else
 {
  text_error(r, token.line, "unexpected character");
 }
 return token;
}
function Text_Token
text_peek(Text_Reader *r)
{
 if(not r->has_peek){ r->peek = text_lex(r); r->has_peek = true; }
 return r->peek;
}
function Text_Token
text_next(Text_Reader *r)
{
 Text_Token token = text_peek(r);
 r->has_peek = false;
 return token;
}
function b32
text_is_punct(Text_Token token, char c)
{
 return token.kind == Text_Token_Punct and token.text.str[0] == cast(u8)c;
}
function b32
text_accept_punct(Text_Reader *r, char c)
{
 if(text_is_punct(text_peek(r), c)){ text_next(r); return true; }
 return false;
}
function void
text_expect_punct(Text_Reader *r, char c)
{
 Text_Token token = text_next(r);
 if(not text_is_punct(token, c))
 {
  Scratch_Scope tmp;
  text_error(r, token.line, to_cstring(push_stringf(tmp, "expected '%c'", c)));
 }
}
function void
text_skip_value(Text_Reader *r)
{// NOTE(kv) One balanced value: a word, a string, or a bracketed group.
 Text_Token token = text_next(r);
 if(text_is_punct(token, '{') or text_is_punct(token, '['))
 {
  i32 depth = 1;
  while(r->ok and depth > 0)
  {
   Text_Token inner = text_next(r);
   if(inner.kind == Text_Token_End){ text_error(r, inner.line, "unbalanced brackets"); break; }
   if(text_is_punct(inner, '{') or text_is_punct(inner, '[')){ depth++; }
   if(text_is_punct(inner, '}') or text_is_punct(inner, ']')){ depth--; }
  }
 }
 else if(token.kind != Text_Token_Word and token.kind != Text_Token_String)
 {
  text_error(r, token.line, "expected a value");
 }
}

function void read_text_value(Text_Reader *r, Type_Info *type, void *pointer);

function void
read_text_number_list(Text_Reader *r, i32 count, b32 is_float, void *pointer)
{// NOTE(kv) `{a, b, c}` into count v1s or i32s (extra items skipped + logged).
 text_expect_punct(r, '{');
 i32 index = 0;
 while(r->ok)
 {
  if(text_accept_punct(r, '}')){ break; }
  Text_Token token = text_next(r);
  if(token.kind != Text_Token_Word){ text_error(r, token.line, "expected a number"); break; }
  if(index < count)
  {
   Scratch_Scope tmp;
   char *cstring = to_cstring(push_stringz(tmp, token.text));
   if(is_float){ (cast(v1 *)pointer)[index] = cast(v1)strtod(cstring, 0); }
   else        { (cast(i32 *)pointer)[index] = cast(i32)strtol(cstring, 0, 10); }
  }
  else if(index == count)
  {
   log_error("%s: line %d: too many values in a vector (%d expected)", r->label, token.line, count);
  }
  index++;
  text_accept_punct(r, ',');
 }
}
function void
read_text_struct_body(Text_Reader *r, Type_Info *type, u8 *pointer, b32 top_level)
{// NOTE(kv) Members until `}` (or end of input at the top level). Zeroes the struct first.
 block_zero(pointer, type->size);
 while(r->ok)
 {
  Text_Token token = text_next(r);
  if(token.kind == Text_Token_End)
  {
   if(not top_level){ text_error(r, token.line, "unexpected end of file inside a struct"); }
   break;
  }
  if(text_is_punct(token, '}'))
  {
   if(top_level){ text_error(r, token.line, "unexpected '}'"); }
   break;
  }
  if(token.kind != Text_Token_Word){ text_error(r, token.line, "expected a member name"); break; }
  text_expect_punct(r, '=');
  if(not r->ok){ break; }
  i32 member_index = find_member_index_by_name(type, token.text);
  if(member_index < 0)
  {
   log_error("%s: line %d: unknown member \"%.*s\" of %.*s, skipped",
             r->label, token.line, strexpand(token.text), strexpand(type->name));
   text_skip_value(r);
  }
  else
  {
   I_Struct_Member &member = type->members[member_index];
   u8 *member_data = pointer + member.offset;
   I_Struct_Member *count_member = 0;
   if(member.type->kind == I_Type_Kind_Array){ count_member = text_find_count_sibling(type, member.name); }
   if(member.type->kind == I_Type_Kind_Union)
   {
    log_error("%s: line %d: union member \"%.*s\" not supported, skipped", r->label, token.line, strexpand(member.name));
    text_skip_value(r);
   }
   else if(count_member)
   {//-Variable list: the number of items read becomes the count
    Type_Info *item_type = member.type->array_item_type;
    Text_Token open = text_next(r);
    char closer = text_is_punct(open, '[') ? ']' : '}';
    if(not (text_is_punct(open, '[') or text_is_punct(open, '{'))){ text_error(r, open.line, "expected '['"); break; }
    i32 item_index = 0;
    while(r->ok)
    {
     if(text_accept_punct(r, closer)){ break; }
     if(item_index < member.type->count)
     {
      read_text_value(r, item_type, member_data + item_type->size*item_index);
     }
     else
     {
      if(item_index == member.type->count)
      {
       log_error("%s: line %d: list \"%.*s\" holds at most %d items, rest dropped",
                 r->label, open.line, strexpand(member.name), member.type->count);
      }
      text_skip_value(r);
     }
     item_index++;
     text_accept_punct(r, ',');
    }
    *cast(i32 *)(pointer + count_member->offset) = minimum(item_index, member.type->count);
   }
   else
   {
    read_text_value(r, member.type, member_data);
   }
  }
  text_accept_punct(r, ',');
 }
}
function void
read_text_value(Text_Reader *r, Type_Info *type, void *void_pointer)
{
 u8 *pointer = cast(u8 *)void_pointer;
 switch(type->kind)
 {
  case I_Type_Kind_Basic:
  {
   switch(type->Basic_Type)
   {
    case Basic_Type_v1: case Basic_Type_i1: case Basic_Type_i16: case Basic_Type_u32: case Basic_Type_u64:
    {
     Text_Token token = text_next(r);
     if(token.kind != Text_Token_Word){ text_error(r, token.line, "expected a number"); break; }
     Scratch_Scope tmp;
     char *cstring = to_cstring(push_stringz(tmp, token.text));
     switch(type->Basic_Type)
     {
      case Basic_Type_v1: { *cast(v1 *)pointer  = cast(v1)strtod(cstring, 0); }break;
      case Basic_Type_i1: { *cast(i32 *)pointer = cast(i32)strtol(cstring, 0, 10); }break;
      case Basic_Type_i16:{ *cast(i16 *)pointer = cast(i16)strtol(cstring, 0, 10); }break;
      case Basic_Type_u32:{ *cast(u32 *)pointer = cast(u32)strtoul(cstring, 0, 10); }break;
      case Basic_Type_u64:{ *cast(u64 *)pointer = cast(u64)strtoull(cstring, 0, 10); }break;
      default: break;
     }
    }break;
    case Basic_Type_v2: case Basic_Type_v3: case Basic_Type_v4:
    {
     read_text_number_list(r, type->size/4, true, pointer);
    }break;
    case Basic_Type_i2: case Basic_Type_i3: case Basic_Type_i4:
    {
     read_text_number_list(r, type->size/4, false, pointer);
    }break;
    case Basic_Type_String:
    {
     Text_Token token = text_next(r);
     if(token.kind != Text_Token_String){ text_error(r, token.line, "expected a quoted string"); break; }
     *cast(String *)pointer = token.text;  // NOTE(kv) points into the reader's arena
    }break;
    case Basic_Type_char:
    {
     Text_Token token = text_next(r);
     if(token.kind != Text_Token_String or token.text.len != 1){ text_error(r, token.line, "expected a one-character string"); break; }
     *pointer = token.text.str[0];
    }break;
    InvalidDefaultCase;
   }
  }break;
  case I_Type_Kind_Struct:
  {
   text_expect_punct(r, '{');
   if(r->ok){ read_text_struct_body(r, type, pointer, false); }
  }break;
  case I_Type_Kind_Array:
  {
   Type_Info *item_type = type->array_item_type;
   if(item_type->kind == I_Type_Kind_Basic and item_type->Basic_Type == Basic_Type_char)
   {//-char[N]: quoted string, truncated to N-1 + nul
    Text_Token token = text_next(r);
    if(token.kind != Text_Token_String){ text_error(r, token.line, "expected a quoted string"); break; }
    i32 len = minimum(cast(i32)token.text.len, type->count-1);
    if(len < cast(i32)token.text.len)
    {
     log_error("%s: line %d: string longer than %d, truncated", r->label, token.line, type->count-1);
    }
    block_zero(pointer, type->size);
    block_copy(pointer, token.text.str, len);
   }
   else
   {//-Fixed array: positional, missing items stay as they are
    Text_Token open = text_next(r);
    char closer = text_is_punct(open, '[') ? ']' : '}';
    if(not (text_is_punct(open, '[') or text_is_punct(open, '{'))){ text_error(r, open.line, "expected '['"); break; }
    i32 item_index = 0;
    while(r->ok)
    {
     if(text_accept_punct(r, closer)){ break; }
     if(item_index < type->count)
     {
      read_text_value(r, item_type, pointer + item_type->size*item_index);
     }
     else
     {
      if(item_index == type->count)
      {
       log_error("%s: line %d: array of %d got more items, rest dropped", r->label, open.line, type->count);
      }
      text_skip_value(r);
     }
     item_index++;
     text_accept_punct(r, ',');
    }
   }
  }break;
  case I_Type_Kind_Enum:
  {
   Text_Token token = text_next(r);
   if(token.kind != Text_Token_Word){ text_error(r, token.line, "expected an enum name"); break; }
   i32 value = 0;
   b32 found = false;
   for_i32(i, 0, type->enum_members.count)
   {
    if(type->enum_members[i].name == token.text){ value = type->enum_members[i].value; found = true; break; }
   }
   if(not found)
   {
    u8 c = token.text.str[0];
    if(('0' <= c and c <= '9') or c == '-')
    {
     Scratch_Scope tmp;
     value = cast(i32)strtol(to_cstring(push_stringz(tmp, token.text)), 0, 10);
    }
    else
    {
     log_error("%s: line %d: unknown %.*s value \"%.*s\", using 0",
               r->label, token.line, strexpand(type->name), strexpand(token.text));
    }
   }
   block_zero(pointer, type->size);
   block_copy(pointer, &value, type->size);
  }break;
  case I_Type_Kind_Wrapper:
  {
   read_text_value(r, type->wrapped_type, pointer);
  }break;
  case I_Type_Kind_Union:
  {
   text_error(r, text_peek(r).line, "union values are not supported");
  }break;
  InvalidDefaultCase;
 }
}
function b32
read_text_top_level(String text, Arena *arena, char const *label, Type_Info *type, void *pointer)
{// NOTE(kv) A whole file into one struct. false = syntax error (logged); the destination
 // is then partially written -- callers read into a temporary.
 kv_assert(type->kind == I_Type_Kind_Struct);
 Text_Reader reader = make_text_reader(text, arena, label);
 read_text_struct_body(&reader, type, cast(u8 *)pointer, true);
 return reader.ok;
}
