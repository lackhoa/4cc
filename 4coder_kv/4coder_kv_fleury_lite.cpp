/* experimental fleury layer */

#include "4coder_fleury/4coder_fleury_hooks.cpp"
#include "4coder_fleury/4coder_fleury_lang_list.h"

function void
F4_TickLite(Application_Links *app, Frame_Info frame_info)
{
    linalloc_clear(&global_frame_arena);
    F4_CLC_Tick(frame_info);
    // NOTE(rjf): Default tick stuff from the 4th dimension:
    default_tick(app, frame_info);
}

function void
F4_RenderBufferLite(Application_Links *app, View_ID view_id, Face_ID face_id,
                    Buffer_ID buffer, Text_Layout_ID text_layout_id,
                    Rect_f32 rect, Frame_Info frame_info)
{
    Scratch_Block scratch(app);
    ProfileScope(app, "[Fleury] Render Buffer");
    
    View_ID active_view = get_active_view(app, Access_Always);
    b32 is_active_view = (active_view == view_id);
    Rect_f32 prev_clip = draw_set_clip(app, rect);
    
    // NOTE(allen): Token colorizing
    Token_Array token_array = get_token_array_from_buffer(app, buffer);
    if(token_array.tokens != 0)
    {
        F4_SyntaxHighlight(app, text_layout_id, &token_array);
        
        // NOTE(allen): Scan for TODOs and NOTEs
        b32 use_comment_keywords = def_get_config_b32(vars_save_string_lit("use_comment_keywords"));
        if(use_comment_keywords)
        {
            Comment_Highlight_Pair pairs[] =
            {
                {str8_lit("NOTE"), finalize_color(defcolor_comment_pop, 0)},
                {str8_lit("TODO"), finalize_color(defcolor_comment_pop, 1)},
                {def_get_config_string(scratch, vars_save_string_lit("user_name")), finalize_color(fleury_color_comment_user_name, 0)},
            };
            draw_comment_highlights(app, buffer, text_layout_id,
                                    &token_array, pairs, ArrayCount(pairs));
        }
    }
    else
    {
        Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
        paint_text_color_fcolor(app, text_layout_id, visible_range, fcolor_id(defcolor_text_default));
    }
    
    i64 cursor_pos = view_correct_cursor(app, view_id);
    view_correct_mark(app, view_id);
    
    // NOTE(allen): Cursor shape
    Face_Metrics metrics = get_face_metrics(app, face_id);
    u64 cursor_roundness_100 = def_get_config_u64(app, vars_save_string_lit("cursor_roundness"));
    f32 cursor_roundness = metrics.normal_advance*cursor_roundness_100*0.01f;
    f32 mark_thickness = (f32)def_get_config_u64(app, vars_save_string_lit("mark_thickness"));
    
    // NOTE(rjf): Cursor
    F4_Cursor_RenderEmacsStyle(app, view_id, is_active_view, buffer, text_layout_id, cursor_roundness, mark_thickness, frame_info);
  
    // NOTE(allen): put the actual text on the actual screen
    draw_text_layout_default(app, text_layout_id);
    
    // NOTE(rjf): Interpret buffer as calc code, if it's the calc buffer.
    Buffer_ID calc_buffer_id = get_buffer_by_name(app, string_u8_litexpr("*calc*"), AccessFlag_Read);
    if(calc_buffer_id == buffer)
    {
        F4_CLC_RenderBuffer(app, buffer, view_id, text_layout_id, frame_info);
    }
    
    // NOTE(rjf): Draw calc comments.
    {
        F4_CLC_RenderComments(app, buffer, view_id, text_layout_id, frame_info);
    }
}

// NOTE(kv): this doesn't do anything interesting regarding the plot
function void
F4_RenderLite(Application_Links *app, Frame_Info frame_info, View_ID view_id)
{
    F4_RecentFiles_RefreshView(app, view_id);
    
    ProfileScope(app, "[Fleury] Render");
    Scratch_Block scratch(app);
    
    View_ID active_view = get_active_view(app, Access_Always);
    b32 is_active_view = (active_view == view_id);
    
    f32 margin_size = (f32)def_get_config_u64(app, vars_save_string_lit("f4_margin_size"));
    Rect_f32 view_rect = view_get_screen_rect(app, view_id);
    Rect_f32 region = rect_inner(view_rect, margin_size);
    
    Buffer_ID buffer = view_get_buffer(app, view_id, Access_Always);
    String_Const_u8 buffer_name = push_buffer_base_name(app, scratch, buffer);
    
    //~ NOTE(rjf): Draw background.
    {
        ARGB_Color color = fcolor_resolve(fcolor_id(defcolor_back));
        if(string_match(buffer_name, string_u8_litexpr("*compilation*")))
        {
            color = color_blend(color, 0.5f, 0xff000000);
        }
        // NOTE(rjf): Inactive background color.
        else if(is_active_view == 0)
        {
            ARGB_Color inactive_bg_color = fcolor_resolve(fcolor_id(fleury_color_inactive_pane_background));
            if(F4_ARGBIsValid(inactive_bg_color))
            {
                color = inactive_bg_color;
            }
        }
        draw_rectangle(app, region, 0.f, color);
        draw_margin(app, view_rect, region, color);
    }
    
    //~ NOTE(rjf): Draw margin.
    {
        ARGB_Color color = fcolor_resolve(fcolor_id(defcolor_margin));
        if(def_get_config_b32(vars_save_string_lit("f4_margin_use_mode_color")) &&
           is_active_view)
        {
            color = F4_GetColor(app, ColorCtx_Cursor(power_mode.enabled ? ColorFlag_PowerMode : 0,
                                                     GlobalKeybindingMode));
        }
        draw_margin(app, view_rect, region, color);
    }
    
    Rect_f32 prev_clip = draw_set_clip(app, region);
    
    Face_ID face_id = get_face_id(app, buffer);
    Face_Metrics face_metrics = get_face_metrics(app, face_id);
    f32 line_height = face_metrics.line_height;
    f32 digit_advance = face_metrics.decimal_digit_advance;
    
    // NOTE(allen): file bar
    b64 showing_file_bar = false;
    if(view_get_setting(app, view_id, ViewSetting_ShowFileBar, &showing_file_bar) && showing_file_bar)
    {
        Rect_f32_Pair pair = layout_file_bar_on_top(region, line_height);
        F4_DrawFileBar(app, view_id, buffer, face_id, pair.min);
        region = pair.max;
    }
    
    Buffer_Scroll scroll = view_get_buffer_scroll(app, view_id);
    Buffer_Point_Delta_Result delta = delta_apply(app, view_id, frame_info.animation_dt, scroll);
    
    if(!block_match_struct(&scroll.position, &delta.point))
    {
        block_copy_struct(&scroll.position, &delta.point);
        view_set_buffer_scroll(app, view_id, scroll, SetBufferScroll_NoCursorChange);
    }
    
    if(delta.still_animating)
    {
        animate_in_n_milliseconds(app, 0);
    }
    
    // NOTE(allen): begin buffer render
    Buffer_Point buffer_point = scroll.position;
    Text_Layout_ID text_layout_id = text_layout_create(app, buffer, region, buffer_point);
    
    // NOTE(allen): draw the buffer
    F4_RenderBufferLite(app, view_id, face_id, buffer, text_layout_id, region, frame_info);
    
    text_layout_free(app, text_layout_id);
    draw_set_clip(app, prev_clip);
}

function void 
fleury_lite_custom_layer_init(Application_Links *app)
{
    default_framework_init(app);
    global_frame_arena = make_arena(get_base_allocator_system());
    permanent_arena = make_arena(get_base_allocator_system());
    
    // NOTE(rjf): Set up hooks.
    {
        set_all_default_hooks(app);
        //t $          ($  , $                             , $                     );
        set_custom_hook(app, HookID_Tick,                    F4_TickLite);
        set_custom_hook(app, HookID_RenderCaller,            F4_RenderLite);
        set_custom_hook(app, HookID_BeginBuffer,             F4_BeginBuffer);  // NOTE: no big deal
        // set_custom_hook(app, HookID_Layout,                  F4_Layout);  // NOTE: not relevant
    
        // NOTE(kv): same
        set_custom_hook(app, HookID_DeltaRule,               F4_DeltaRule);
        set_custom_hook(app, HookID_BufferEditRange,         F4_BufferEditRange);
    
        set_custom_hook_memory_size(app, HookID_DeltaRule, delta_ctx_size(sizeof(Vec2_f32)));
    }
    
    // NOTE(rjf): Set up mapping.
    {
        Thread_Context *tctx = get_thread_context(app);
        mapping_init(tctx, &framework_mapping);
        default_4coder_initialize(app);
        String_ID global_id = vars_save_string_lit("keys_global");
        String_ID file_id   = vars_save_string_lit("keys_file");
        String_ID code_id   = vars_save_string_lit("keys_code");
        setup_essential_mapping(&framework_mapping, global_id, file_id, code_id);
        setup_default_mapping(&framework_mapping, global_id, file_id, code_id);
    }
    
    // NOTE(rjf): Set up custom code index.
    {
        F4_Index_Initialize();
    }
    
    // NOTE(rjf): Register languages.
    {
        F4_RegisterLanguages();
    }
}
