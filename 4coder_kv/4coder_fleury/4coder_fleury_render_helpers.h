/* date = January 29th 2021 7:54 pm */

#ifndef FCODER_FLEURY_RENDER_HELPERS_H
#define FCODER_FLEURY_RENDER_HELPERS_H

#include "4coder_fleury_ubiquitous.h"

enum F4_RangeHighlightKind
{
    F4_RangeHighlightKind_Whole,
    F4_RangeHighlightKind_Underline,
    F4_RangeHighlightKind_MinorUnderline,
};

struct F4_Flash
{
    b32 active;
    f32 t;
    
    Buffer_ID buffer;
    Range_i64 range;
    ARGB_Color color;
    f32 decay_rate;
};

function void F4_DrawTooltipRect(App *app, Rect_f32 rect);
function void F4_RenderRangeHighlight(App *app, View_ID view_id, Text_Layout_ID text_layout_id, Range_i64 range, F4_RangeHighlightKind kind, ARGB_Color color);
function void F4_PushTooltip(String string, ARGB_Color color);

function void F4_PushFlash(App *app, Buffer_ID buffer, Range_i64 range, ARGB_Color color, f32 decay_rate);
function void F4_UpdateFlashes(App *app, Frame_Info info);
function void F4_RenderFlashes(App *app, View_ID view, Text_Layout_ID text_layout);

#endif // FCODER_FLEURY_RENDER_HELPERS_H
