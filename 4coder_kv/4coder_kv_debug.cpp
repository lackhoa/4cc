// NOTE(kv): This is more of a primitive UI that I can always use.
// There's no reason to leave it out of stable (we're not shipping user facing code).

#pragma once

#include "4coder_fancy.cpp"

global Debug_Entry *DEBUG_entries;
global b32 DEBUG_draw_hud_p = true;

internal DEBUG_send_entry_return
DEBUG_send_entry(DEBUG_send_entry_params)
{
 arrput(DEBUG_entries, entry);
}

internal void
DEBUG_draw_entry(App *app, Face_ID face_id, Debug_Entry entry, v2 *at)
{
 Scratch_Block scratch(app);
 
 Face_Metrics face_metrics = get_face_metrics(app, face_id);
 v1 line_height = face_metrics.line_height;
 
 FColor color = f_yellow;
 if (entry.color != 0) { color.argb = entry.color; }
 
 Fancy_Line line = {};
 char *scope = entry.scope;
 char *scope_delim = "/";
 if (scope == 0) { scope = ""; scope_delim = ""; }
 push_fancy_stringf(scratch, &line, color, "%s%s%s: ", scope, scope_delim, entry.name);
 push_fancy_stringf(scratch, &line, f_white, "[%g, %g, %g, %g]", v4_expand(entry.value));
 
 draw_fancy_line(app, face_id, fcolor_zero(), &line, *at);
 at->y += line_height;
}

internal void
DEBUG_draw_hud(App *app, Face_ID face_id, Rect_f32 rect)
{
    draw_rect_fcolor(app, rect, 0.f, fcolor_change_alpha(f_black, 0.3f));
    
    v2 at = rect.p0;
    for (i1 entry_index=0; 
         entry_index < arrlen(DEBUG_entries);
         entry_index++)
    {
        DEBUG_draw_entry(app, face_id, DEBUG_entries[entry_index], &at);
    }
}

// NOTE(kv): It's annoying to have different command sets for different builds,
//           so let's have the same commands for both.
CUSTOM_COMMAND_SIG(DEBUG_draw_hud_toggle)
CUSTOM_DOC("toggle debug hud")
{
    DEBUG_draw_hud_p ^= 1;
}
