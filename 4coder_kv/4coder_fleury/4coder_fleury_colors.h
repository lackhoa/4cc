/* date = January 29th 2021 1:40 pm */

#ifndef FCODER_FLEURY_COLORS_H
#define FCODER_FLEURY_COLORS_H

#include "4coder_fleury_ubiquitous.h"

global Managed_ID fleury_color_syntax_crap; //colors
global Managed_ID fleury_color_operators; //colors
global Managed_ID fleury_color_inactive_pane_overlay; //colors
global Managed_ID fleury_color_inactive_pane_background; //colors
global Managed_ID fleury_color_file_progress_bar; //colors
global Managed_ID fleury_color_brace_highlight; //colors
global Managed_ID fleury_color_brace_line; //colors
global Managed_ID fleury_color_brace_annotation; //colors
global Managed_ID fleury_color_index_sum_type; //colors
global Managed_ID fleury_color_index_product_type; //colors
global Managed_ID fleury_color_index_function; //colors
global Managed_ID fleury_color_index_macro; //colors
global Managed_ID fleury_color_index_constant; //colors
global Managed_ID fleury_color_index_comment_tag; //colors
global Managed_ID fleury_color_index_decl; //colors
global Managed_ID fleury_color_cursor_macro; //colors
global Managed_ID fleury_color_cursor_power_mode; //colors
global Managed_ID fleury_color_cursor_inactive; //colors
global Managed_ID fleury_color_plot_cycle; //colors
global Managed_ID fleury_color_token_highlight; //colors
global Managed_ID fleury_color_token_minor_highlight; //colors
global Managed_ID fleury_color_comment_user_name; //colors
global Managed_ID fleury_color_lego_grab; //colors
global Managed_ID fleury_color_lego_splat; //colors
global Managed_ID fleury_color_error_annotation; //colors

static ARGB_Color F4_ARGBFromID(Color_Table table, Managed_ID id, int subindex);
static ARGB_Color F4_ARGBFromID(Color_Table table, Managed_ID id);

inline b32
F4_ARGBIsValid(ARGB_Color color)
{
    return color != 0xFF990099;
}

#endif //4CODER_FLEURY_COLORS_H
