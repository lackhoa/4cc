// NOTE(kv): it is really annoying to have different command sets for debug and release build,
//           so let's have the same commands for both.

#if KV_INTERNAL
struct Debug_Entry
{
  char *name;
  v4    value;
};

global b32 DEBUG_draw_hud_p;  // NOTE(kv): don't set this here, it is set via config
global Debug_Entry *DEBUG_entries;

internal void
DEBUG_draw_entry(App *app, Face_ID face_id, Debug_Entry entry, v2 *at)
{
    Scratch_Block scratch(app);
    
    Face_Metrics face_metrics = get_face_metrics(app, face_id);
    f32 line_height = face_metrics.line_height;
    
    Fancy_Line line = {};
    push_fancy_stringf(scratch, &line, f_pink, "%s: ", entry.name);
    push_fancy_stringf(scratch, &line, f_white, "[%.2f, %.2f, %.2f, %.2f]", v4_expand(entry.value));
    
    draw_fancy_line(app, face_id, fcolor_zero(), &line, *at);
    at->y += line_height;
}

internal void
DEBUG_draw_hud(App *app, Face_ID face_id, Rect_f32 rect)
{
    draw_rectangle_fcolor(app, rect, 0.f, fcolor_change_alpha(f_black, 0.3f));
    
    v2 at = rect.p0;
    for (i32 entry_index=0; 
         entry_index < arrlen(DEBUG_entries);
         entry_index++)
    {
        DEBUG_draw_entry(app, face_id, DEBUG_entries[entry_index], &at);
    }
}

internal void
DEBUG_VALUE_inner(char *name, rect2 value)
{
  Debug_Entry entry = {.name=name, .value=value.v4_value};
  arrput(DEBUG_entries, entry);
}

internal void
DEBUG_VALUE_inner(char *name, i32 value)
{
    Debug_Entry entry = {.name=name, .value=v4{.x=(f32)value}};
    arrput(DEBUG_entries, entry);
}

internal void
DEBUG_VALUE_inner(char *name, v1 value)
{
    Debug_Entry entry = {.name=name, .value=v4{.x=value}};
    arrput(DEBUG_entries, entry);
}

internal void
DEBUG_VALUE_inner(char *name, v2 value)
{
    Debug_Entry entry = {.name=name, .value=v4{.x=value.x, .y=value.y}};
    arrput(DEBUG_entries, entry);
}

internal void
DEBUG_VALUE_inner(char *name, v3 v)
{
    v4 value = castV4(v);
    Debug_Entry entry = {.name=name, .value=value};
    arrput(DEBUG_entries, entry);
}

#    define DEBUG_NAME(NAME, VALUE)  DEBUG_VALUE_inner(NAME, VALUE)
#    define DEBUG_VALUE(VALUE)       DEBUG_VALUE_inner(#VALUE, VALUE)
#    define DEBUG_CLEAR              arrsetlen(DEBUG_entries, 0)
#else
#    define DEBUG_NAME(...)
#    define DEBUG_VALUE
#    define DEBUG_CLEAR
#endif

CUSTOM_COMMAND_SIG(DEBUG_draw_hud_toggle)
CUSTOM_DOC("toggle debug hud")
{
#if KV_INTERNAL
    DEBUG_draw_hud_p ^= 1;
#endif
}
