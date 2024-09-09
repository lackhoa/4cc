/*
 * Serial inserts helpers
 */

// TOP

function Buffer_Insertion
begin_buffer_insertion_at(App *app, Buffer_ID buffer_id, i64 at)
{
 Buffer_Insertion result = {
  .app    = app,
  .buffer = buffer_id,
  .at     = at,
 };
 return(result);
}

function Buffer_Insertion
begin_buffer_insertion_at_buffered(App *app, Buffer_ID buffer_id, i64 at, u8 *memory, umm cap)
{
 Buffer_Insertion result = begin_buffer_insertion_at(app, buffer_id, at);
 result.buffering = true;
 result.memory    = memory;
 result.cap       = cap;
 return(result);
}

function Buffer_Insertion
begin_buffer_insertion_at_buffered2(App *app, Buffer_ID buffer_id, i64 at, Arena *arena, u64 buffer_cap)
{
 u8 *memory = push_size(arena, buffer_cap);
 return(begin_buffer_insertion_at_buffered(app, buffer_id, at, memory, buffer_cap));
}

function Buffer_Insertion
begin_buffer_insertion(App *app){
 View_ID view = get_active_view(app, Access_Always);
 Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
 i64 cursor_pos = view_get_cursor_pos(app, view);
 Buffer_Insertion result = begin_buffer_insertion_at(app, buffer, cursor_pos);
 return(result);
}

function void
insert_string__no_buffering(Buffer_Insertion *insertion, String string) {
 buffer_replace_range(insertion->app, insertion->buffer, Ii64(insertion->at), string);
 insertion->at += string.size;
}

function void
insert__flush(Buffer_Insertion *insertion)
{
 String string = SCu8(insertion->memory, insertion->len);
 insert_string__no_buffering(insertion, string);
 insertion->len = 0;
}

function void
end_buffer_insertion(Buffer_Insertion *insertion){
 if (insertion->buffering){
  insert__flush(insertion);
 }
}

inline umm
insert__get_free(Buffer_Insertion *i)
{
 return (i->cap - i->len);
}

function void
insert_string(Buffer_Insertion *i, String string)
{
 if (!i->buffering)
 {
  insert_string__no_buffering(i, string);
 }
 else
 {
  if ( insert__get_free(i) < string.size) {
   insert__flush(i);
  }
  
  if ( insert__get_free(i) < string.size)
  {
   insert_string__no_buffering(i, string);
  }
  else
  {
   u8 *dst = i->memory+i->len;
   block_copy(dst, string.str, string.size);
   i->len += string.size;
  }
 }
}

function u64
insertf(Buffer_Insertion *insertion, char *format, ...){
 Scratch_Block scratch(insertion->app);
 va_list args;
 va_start(args, format);
 String string = push_stringfv(scratch, format, args, true);
 va_end(args);
 insert_string(insertion, string);
 return(string.size);
}

function void
insertc(Buffer_Insertion *insertion, char C){
    insert_string(insertion, SCu8(&C, 1));
}

function b32
insert_line_from_buffer(Buffer_Insertion *insertion, Buffer_ID buffer_id, i1 line, i1 truncate_at){
    b32 success = is_valid_line(insertion->app, buffer_id, line);
    if (success){
        Scratch_Block scratch(insertion->app);
        insert_string(insertion, push_buffer_line(insertion->app, scratch, buffer_id, line));
    }
    return(success);
}

function b32
insert_line_from_buffer(Buffer_Insertion *insertion, Buffer_ID buffer_id, i1 line){
    return(insert_line_from_buffer(insertion, buffer_id, line, 0));
}

// BOTTOM

