// NOTE(kv): it is really annoying to have different command sets for debug and release build,
//           so let's have the same commands for both.

#if KV_INTERNAL
struct Debug_Entry
{
  char *name;
  rect2 rect;
};

global b32 DEBUG_draw_hud_p;  // NOTE(kv): don't set this here, it is set via config
global Debug_Entry *DEBUG_entries;

internal void
debug_rect_position(FApp *app, Arena *scratch, Face_ID face_id, Debug_Entry entry, v2 *at)
{
  Face_Metrics face_metrics = get_face_metrics(app, face_id);
  f32 line_height = face_metrics.line_height;
  
  Fancy_Line line = {};
  push_fancy_stringf(scratch, &line, f_pink, "%s: ", entry.name);
  rect2 r = entry.rect;
  push_fancy_stringf(scratch, &line, f_white, "[(%.0f, %.0f), (%.0f, %.0f)]; ",
                     r.x0, r.y0, r.x1, r.y1);
  draw_fancy_line(app, face_id, fcolor_zero(), &line, *at);
  at->y += line_height;
}

internal void
DEBUG_draw_hud(Application_Links *app, Face_ID face_id, Text_Layout_ID text_layout_id, Rect_f32 rect)
{
  draw_rectangle_fcolor(app, rect, 0.f, fcolor_change_alpha(f_black, 0.3f));
  
  Scratch_Block scratch(app);

  Debug_Entry entry;
  v2 at = rect.p0;
  
  entry.name = "cursor"; entry.rect = get_cursor_rect(app, text_layout_id);
  debug_rect_position(app, scratch, face_id, entry, &at);
  for (i32 entry_index=0; 
       entry_index < arrlen(DEBUG_entries);
       entry_index++)
  {
    debug_rect_position(app, scratch, face_id, DEBUG_entries[entry_index], &at);
  }
}

internal void
DEBUG_text_inner(char *name, rect2 value)
{
  Debug_Entry entry = {.name=name, .rect=value};
  arrput(DEBUG_entries, entry);
}

internal void
DEBUG_text_inner(char *name, i32 value)
{
    rect2 rect = {.x0=(f32)value};  // nono
    Debug_Entry entry = {.name=name, .rect=rect};
    arrput(DEBUG_entries, entry);
}

internal void
DEBUG_text_inner(char *name, v3 v)
{
  rect2 rect = {v.x,v.y,v.z,0};
  Debug_Entry entry = {.name=name, .rect=rect};
  arrput(DEBUG_entries, entry);
}

#    define DEBUG_text(NAME, VALUE) DEBUG_text_inner(NAME, VALUE)
#    define DEBUG_clear   arrsetlen(DEBUG_entries, 0)
#else
#    define DEBUG_text(...)
#    define DEBUG_clear
#endif

CUSTOM_COMMAND_SIG(DEBUG_draw_hud_toggle)
CUSTOM_DOC("toggle debug hud")
{
#if KV_INTERNAL
  DEBUG_draw_hud_p ^= 1;
#endif
}