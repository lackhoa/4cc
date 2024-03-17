#pragma once

#include "4coder_default_colors.cpp"
#include "4ed_render_target.cpp"

api(custom)
struct Fancy_String{
    Fancy_String *next;
    String value;
    Face_ID face;
    FColor fore;
    f32 pre_margin;
    f32 post_margin;
};

api(custom)
struct Fancy_Line{
    Fancy_Line *next;
    Face_ID face;
    FColor fore;
    Fancy_String *first;
    Fancy_String *last;
};

api(custom)
struct Fancy_Block{
    Fancy_Line *first;
    Fancy_Line *last;
    i32 line_count;
};

internal FColor
fcolor_argb(ARGB_Color color)
{
    FColor result = {};
    result.argb = color;
    if (result.a_byte == 0)
    {
        result.argb = 0;
    }
    return(result);
}
internal FColor
fcolor_argb(v4 color)
{
    return(fcolor_argb(argb_pack(color)));
}
internal FColor
fcolor_argb(f32 r, f32 g, f32 b, f32 a)
{
    return(fcolor_argb(argb_pack(vec4(r, g, b, a))));
}

internal FColor
fcolor_id(Managed_ID id){
    FColor result = {};
    result.id = (ID_Color)id;
    return(result);
}

internal FColor
fcolor_id(Managed_ID id, u32 sub_index){
    FColor result = {};
    result.id = (ID_Color)id;
    result.sub_index = (u8)sub_index;
    return(result);
}

internal ARGB_Color
argb_color_blend(ARGB_Color a, f32 at, ARGB_Color b, f32 bt)
{
    Vec4_f32 av = argb_unpack(a);
    Vec4_f32 bv = argb_unpack(b);
    Vec4_f32 value = at*av + bt*bv;
    return(argb_pack(value));
}
internal ARGB_Color
argb_color_blend(ARGB_Color a, f32 t, ARGB_Color b)
{
    return(argb_color_blend(a, 1.f - t, b, t));
}

internal ARGB_Color
fcolor_resolve(FColor color)
{
    ARGB_Color result = 0;
    if (color.a_byte == 0)
    {
        if (color.id != 0)
        {
            result = finalize_color(color.id, color.sub_index);
        }
    }
    else result = color.argb;
    
    return(result);
}

function void
draw_rect_fcolor(App *app, rect2 rect, v1 roundness, FColor color)
{
    ARGB_Color argb = fcolor_resolve(color);
    draw_rect(app, rect, roundness, argb, 0);
}

inline ARGB_Color
fcolor_resolve(Managed_ID color_id)
{
    return(fcolor_resolve(fcolor_id(color_id)));
}

internal FColor
fcolor_change_alpha(FColor color, f32 alpha)
{
    Vec4_f32 v = argb_unpack(fcolor_resolve(color));
    v.a = alpha;
    return(fcolor_argb(argb_pack(v)));
}
internal FColor
fcolor_blend(FColor a, f32 at, FColor b, f32 bt){
    ARGB_Color a_argb = fcolor_resolve(a);
    ARGB_Color b_argb = fcolor_resolve(b);
    return(fcolor_argb(argb_color_blend(a_argb, at, b_argb, bt)));
}
internal FColor
fcolor_blend(FColor a, f32 t, FColor b){
    return(fcolor_blend(a, 1.f - t, b, t));
}

internal FColor
fcolor_zero(void){
    FColor result = {};
    return(result);
}

internal b32
fcolor_is_valid(FColor color){
    return(color.argb != 0);
}

////////////////////////////////

internal void
push_fancy_string(Fancy_Line *line, Fancy_String *string){
    sll_queue_push(line->first, line->last, string);
}

internal void
push_fancy_line(Fancy_Block *block, Fancy_Line *line){
    sll_queue_push(block->first, block->last, line);
    block->line_count += 1;
}

////////////////////////////////

internal Fancy_String*
fill_fancy_string(Fancy_String *ptr, Face_ID face, FColor fore, f32 pre_margin, f32 post_margin,
                  String_Const_u8 value){
    ptr->value = value;
    ptr->face = face;
    ptr->fore = fore;
    ptr->pre_margin = pre_margin;
    ptr->post_margin = post_margin;
    return(ptr);
}

internal Fancy_String*
push_fancy_string(Arena *arena, Fancy_Line *line, Face_ID face, FColor fore,
                  f32 pre_margin, f32 post_margin, String_Const_u8 value){
    Fancy_String *result = push_array_zero(arena, Fancy_String, 1);
    fill_fancy_string(result, face, fore, pre_margin, post_margin, value);
    if (line != 0){
        push_fancy_string(line, result);
    }
    return(result);
}

internal Fancy_String*
push_fancy_string(Arena *arena, Fancy_Line *line, Face_ID face, FColor fore,
                  String_Const_u8 value){
    return(push_fancy_string(arena, line, face, fore, 0, 0, value));
}
internal Fancy_String*
push_fancy_string(Arena *arena, Fancy_Line *line, Face_ID face,
                  f32 pre_margin, f32 post_margin, String_Const_u8 value){
    return(push_fancy_string(arena, line, face, fcolor_zero(),
                             pre_margin, post_margin, value));
}
internal Fancy_String*
push_fancy_string(Arena *arena, Fancy_Line *line, FColor fore,
                  f32 pre_margin, f32 post_margin, String_Const_u8 value){
    return(push_fancy_string(arena, line, 0, fore, pre_margin, post_margin, value));
}
internal Fancy_String*
push_fancy_string(Arena *arena, Fancy_Line *line, Face_ID face, String_Const_u8 value){
    return(push_fancy_string(arena, line, face, fcolor_zero(), 0, 0, value));
}
internal Fancy_String*
push_fancy_string(Arena *arena, Fancy_Line *line, FColor color, String_Const_u8 value){
    return(push_fancy_string(arena, line, 0, color, 0, 0, value));
}
internal Fancy_String*
push_fancy_string(Arena *arena, Fancy_Line *line, f32 pre_margin, f32 post_margin,
                  String_Const_u8 value){
    return(push_fancy_string(arena, line, 0, fcolor_zero(), pre_margin, post_margin,
                             value));
}
internal Fancy_String*
push_fancy_string(Arena *arena, Fancy_Line *line, String_Const_u8 value){
    return(push_fancy_string(arena, line, 0, fcolor_zero(), 0, 0, value));
}

////////////////////////////////

internal Fancy_String*
push_fancy_stringfv(Arena *arena, Fancy_Line *line, Face_ID face, FColor fore,
                    f32 pre_margin, f32 post_margin,
                    char *format, va_list args){
    return(push_fancy_string(arena, line, face, fore, pre_margin, post_margin,
                             push_stringfv(arena, format, args)));
}
internal Fancy_String*
push_fancy_stringfv(Arena *arena, Fancy_Line *line, Face_ID face, FColor fore,
                    char *format, va_list args){
    return(push_fancy_stringfv(arena, line, face, fore, 0, 0, format, args));
}
internal Fancy_String*
push_fancy_stringfv(Arena *arena, Fancy_Line *line, Face_ID face,
                    f32 pre_margin, f32 post_margin,
                    char *format, va_list args){
    return(push_fancy_stringfv(arena, line, face, fcolor_zero(),
                               pre_margin, post_margin, format, args));
}
internal Fancy_String*
push_fancy_stringfv(Arena *arena, Fancy_Line *line, FColor fore,
                    f32 pre_margin, f32 post_margin,
                    char *format, va_list args){
    return(push_fancy_stringfv(arena, line, 0, fore, pre_margin, post_margin,
                               format, args));
}
internal Fancy_String*
push_fancy_stringfv(Arena *arena, Fancy_Line *line, Face_ID face,
                    char *format, va_list args){
    return(push_fancy_stringfv(arena, line, face, fcolor_zero(), 0, 0,
                               format, args));
}
internal Fancy_String*
push_fancy_stringfv(Arena *arena, Fancy_Line *line, FColor color,
                    char *format, va_list args){
    return(push_fancy_stringfv(arena, line, 0, color, 0, 0, format, args));
}
internal Fancy_String*
push_fancy_stringfv(Arena *arena, Fancy_Line *line, f32 pre_margin, f32 post_margin,
                    char *format, va_list args){
    return(push_fancy_stringfv(arena, line, 0, fcolor_zero(), pre_margin, post_margin,
                               format, args));
}
internal Fancy_String*
push_fancy_stringfv(Arena *arena, Fancy_Line *line,
                    char *format, va_list args){
    return(push_fancy_stringfv(arena, line, 0, fcolor_zero(), 0, 0, format, args));
}

#define StringFBegin() va_list args; va_start(args, format)
#define StringFPass(N) Fancy_String *result = N
#define StringFEnd() va_end(args); return(result)

internal Fancy_String*
push_fancy_stringf(Arena *arena, Fancy_Line *line, Face_ID face, FColor fore,
                   f32 pre_margin, f32 post_margin,
                   char *format, ...){
    StringFBegin();
    StringFPass(push_fancy_string(arena, line, face, fore, pre_margin, post_margin,
                                  push_stringfv(arena, format, args)));
    StringFEnd();
}
internal Fancy_String*
push_fancy_stringf(Arena *arena, Fancy_Line *line, Face_ID face, FColor fore,
                   char *format, ...){
    StringFBegin();
    StringFPass(push_fancy_stringfv(arena, line, face, fore, 0, 0, format, args));
    StringFEnd();
}
internal Fancy_String*
push_fancy_stringf(Arena *arena, Fancy_Line *line, Face_ID face,
                   f32 pre_margin, f32 post_margin,
                   char *format, ...){
    StringFBegin();
    StringFPass(push_fancy_stringfv(arena, line, face, fcolor_zero(),
                                    pre_margin, post_margin, format, args));
    StringFEnd();
}
internal Fancy_String*
push_fancy_stringf(Arena *arena, Fancy_Line *line, FColor fore,
                   f32 pre_margin, f32 post_margin,
                   char *format, ...){
    StringFBegin();
    StringFPass(push_fancy_stringfv(arena, line, 0, fore, pre_margin, post_margin,
                                    format, args));
    StringFEnd();
}
internal Fancy_String*
push_fancy_stringf(Arena *arena, Fancy_Line *line, Face_ID face,
                   char *format, ...){
    StringFBegin();
    StringFPass(push_fancy_stringfv(arena, line, face, fcolor_zero(), 0, 0,
                                    format, args));
    StringFEnd();
}
internal Fancy_String*
push_fancy_stringf(Arena *arena, Fancy_Line *line, FColor color,
                   char *format, ...){
    StringFBegin();
    StringFPass(push_fancy_stringfv(arena, line, 0, color, 0, 0, format, args));
    StringFEnd();
}
internal Fancy_String*
push_fancy_stringf(Arena *arena, Fancy_Line *line, f32 pre_margin, f32 post_margin,
                   char *format, ...){
    StringFBegin();
    StringFPass(push_fancy_stringfv(arena, line, 0, fcolor_zero(),
                                    pre_margin, post_margin, format, args));
    StringFEnd();
}
internal Fancy_String*
push_fancy_stringf(Arena *arena, Fancy_Line *line,
                   char *format, ...){
    StringFBegin();
    StringFPass(push_fancy_stringfv(arena, line, 0, fcolor_zero(), 0, 0, format, args));
    StringFEnd();
}

////////////////////////////////

internal Fancy_String*
push_fancy_string_fixed(Arena *arena, Fancy_Line *line, Face_ID face, FColor fore,
                        f32 pre_margin, f32 post_margin,
                        String_Const_u8 value, u32 max){
    if (value.size <= max){
        return(push_fancy_stringf(arena, line, face, fore, pre_margin, post_margin,
                                  "%-*.*s", max, string_expand(value)));
    }
    else{
        return(push_fancy_stringf(arena, line, face, fore, pre_margin, post_margin,
                                  "%-*.*s...", max - 3, string_expand(value)));
    }
}
internal Fancy_String*
push_fancy_string_fixed(Arena *arena, Fancy_Line *line, Face_ID face, FColor fore,
                        String_Const_u8 value, u32 max){
    if (value.size <= max){
        return(push_fancy_stringf(arena, line, face, fore, 0.f, 0.f,
                                  "%-*.*s", max, string_expand(value)));
    }
    else{
        return(push_fancy_stringf(arena, line, face, fore, 0.f, 0.f,
                                  "%-*.*s...", max - 3, string_expand(value)));
    }
}
internal Fancy_String*
push_fancy_string_fixed(Arena *arena, Fancy_Line *line, Face_ID face,
                        f32 pre_margin, f32 post_margin, String_Const_u8 value,
                        u32 max){
    if (value.size <= max){
        return(push_fancy_stringf(arena, line, face, fcolor_zero(),
                                  pre_margin, post_margin,
                                  "%-*.*s", max, string_expand(value)));
    }
    else{
        return(push_fancy_stringf(arena, line, face, fcolor_zero(),
                                  pre_margin, post_margin,
                                  "%-*.*s...", max - 3, string_expand(value)));
    }
}
internal Fancy_String*
push_fancy_string_fixed(Arena *arena, Fancy_Line *line, FColor fore,
                        f32 pre_margin, f32 post_margin, String_Const_u8 value,
                        u32 max){
    if (value.size <= max){
        return(push_fancy_stringf(arena, line, 0, fore, pre_margin, post_margin,
                                  "%-*.*s", max, string_expand(value)));
    }
    else{
        return(push_fancy_stringf(arena, line, 0, fore, pre_margin, post_margin,
                                  "%-*.*s...", max - 3, string_expand(value)));
    }
}
internal Fancy_String*
push_fancy_string_fixed(Arena *arena, Fancy_Line *line, Face_ID face,
                        String_Const_u8 value, u32 max){
    if (value.size <= max){
        return(push_fancy_stringf(arena, line, face, fcolor_zero(), 0.f, 0.f,
                                  "%-*.*s", max, string_expand(value)));
    }
    else{
        return(push_fancy_stringf(arena, line, face, fcolor_zero(), 0.f, 0.f,
                                  "%-*.*s...", max - 3, string_expand(value)));
    }
}
internal Fancy_String*
push_fancy_string_fixed(Arena *arena, Fancy_Line *line, FColor fore,
                        String_Const_u8 value, u32 max)
{
    if (value.size <= max){
      char *format = "%-*.*s";
      Fancy_String *result = push_fancy_stringf(arena, line, (Face_ID)0, fore, 0.f, 0.f, format, max, string_expand(value));
        return(result);
    }
    else{
      char *format = "%-*.*s...";
        return(push_fancy_stringf(arena, line, (Face_ID)0, fore, 0.f, 0.f, format, max - 3, string_expand(value)));
    }
}
internal Fancy_String*
push_fancy_string_fixed(Arena *arena, Fancy_Line *line,
                        f32 pre_margin, f32 post_margin, String_Const_u8 value,
                        u32 max){
    if (value.size <= max){
      char *format = "%-*.*s";
        return(push_fancy_stringf(arena, line, (Face_ID)0, fcolor_zero(),
                                  pre_margin, post_margin,
                                  format, max, string_expand(value)));
    }
    else{
      char *format = "%-*.*s...";
        return(push_fancy_stringf(arena, line, (Face_ID)0, fcolor_zero(),
                                  pre_margin, post_margin,
                                  format, max - 3, string_expand(value)));
    }
}
internal Fancy_String*
push_fancy_string_fixed(Arena *arena, Fancy_Line *line, String_Const_u8 value,
                        u32 max){
    if (value.size <= max){
      char *format = "%-*.*s";
      Fancy_String *result = push_fancy_stringf(arena, line, (Face_ID)0, fcolor_zero(), 0.f, 0.f,
                                                format, max, string_expand(value));
        return(result);
    }
    else{
      char *format = "%-*.*s...";
        return(push_fancy_stringf(arena, line, (Face_ID)0, fcolor_zero(), 0.f, 0.f, format, max - 3, string_expand(value)));
    }
}

internal Fancy_String*
push_fancy_string_trunc(Arena *arena, Fancy_Line *line, Face_ID face, FColor fore,
                        f32 pre_margin, f32 post_margin,
                        String value, u32 max){
    if (value.size <= max){
      char *format = "%.*s";
        return(push_fancy_stringf(arena, line, face, fore, pre_margin, post_margin,
                                  format, string_expand(value)));
    }
    else{
      char *format = "%.*s...";
        return(push_fancy_stringf(arena, line, face, fore, pre_margin, post_margin,
                                  format, max - 3, value.str));
    }
}
internal Fancy_String*
push_fancy_string_trunc(Arena *arena, Fancy_Line *line, Face_ID face, FColor fore,
                        String_Const_u8 value, u32 max){
    if (value.size <= max){
        return(push_fancy_stringf(arena, line, face, fore, 0.f, 0.f,
                                  "%.*s", string_expand(value)));
    }
    else{
        return(push_fancy_stringf(arena, line, face, fore, 0.f, 0.f,
                                  "%.*s...", max - 3, value.str));
    }
}
internal Fancy_String*
push_fancy_string_trunc(Arena *arena, Fancy_Line *line, Face_ID face,
                        f32 pre_margin, f32 post_margin, String_Const_u8 value,
                        u32 max){
    if (value.size <= max){
      char *format = "%.*s";
        return(push_fancy_stringf(arena, line, face, fcolor_zero(),
                                  pre_margin, post_margin,
                                  format, string_expand(value)));
    }
    else{
      char *format = "%.*s...";
        return(push_fancy_stringf(arena, line, face, fcolor_zero(),
                                  pre_margin, post_margin,
                                  format, max - 3, value.str));
    }
}
internal Fancy_String*
push_fancy_string_trunc(Arena *arena, Fancy_Line *line, FColor fore,
                        f32 pre_margin, f32 post_margin, String_Const_u8 value,
                        u32 max){
    if (value.size <= max){
        return(push_fancy_stringf(arena, line, 0, fore, pre_margin, post_margin,
                                  "%.*s", string_expand(value)));
    }
    else{
        return(push_fancy_stringf(arena, line, 0, fore, pre_margin, post_margin,
                                  "%.*s...", max - 3, value.str));
    }
}
internal Fancy_String*
push_fancy_string_trunc(Arena *arena, Fancy_Line *line, Face_ID face,
                        String_Const_u8 value, u32 max){
    if (value.size <= max){
        return(push_fancy_stringf(arena, line, face, fcolor_zero(), 0.f, 0.f,
                                  "%.*s", string_expand(value)));
    }
    else{
        return(push_fancy_stringf(arena, line, face, fcolor_zero(), 0.f, 0.f,
                                  "%.*s...", max - 3, value.str));
    }
}
internal Fancy_String*
push_fancy_string_trunc(Arena *arena, Fancy_Line *line, FColor fore,
                        String_Const_u8 value, u32 max){
    if (value.size <= max){
      char *format = "%.*s";
        return(push_fancy_stringf(arena, line, (Face_ID)0, fore, 0.f, 0.f,
                                  format, string_expand(value)));
    }
    else{
      char *format = "%.*s...";
        return(push_fancy_stringf(arena, line, (Face_ID)0, fore, 0.f, 0.f,
                                  format, max - 3, value.str));
    }
}
internal Fancy_String*
push_fancy_string_trunc(Arena *arena, Fancy_Line *line,
                        f32 pre_margin, f32 post_margin, String_Const_u8 value,
                        u32 max){
    if (value.size <= max){
      char *format = "%.*s";
        return(push_fancy_stringf(arena, line, (Face_ID)0, fcolor_zero(),
                                  pre_margin, post_margin,
                                  format, string_expand(value)));
    }
    else{
      char *format = "%.*s...";
        return(push_fancy_stringf(arena, line, (Face_ID)0, fcolor_zero(),
                                  pre_margin, post_margin,
                                  format, max - 3, value.str));
    }
}
internal Fancy_String*
push_fancy_string_trunc(Arena *arena, Fancy_Line *line, String_Const_u8 value,
                        u32 max){
    if (value.size <= max){
      char *format = "%.*s";
        return(push_fancy_stringf(arena, line, (Face_ID)0, fcolor_zero(), 0.f, 0.f,
                                  format, string_expand(value)));
    }
    else{
      char *format = "%.*s...";
        return(push_fancy_stringf(arena, line, (Face_ID)0, fcolor_zero(), 0.f, 0.f,
                                  format, max - 3, value.str));
    }
}

////////////////////////////////

internal Fancy_Line*
push_fancy_line(Arena *arena, Fancy_Block *block, Face_ID face, FColor fore,
                String_Const_u8 text){
    Fancy_Line *line = push_array_zero(arena, Fancy_Line, 1);
    line->face = face;
    line->fore = fore;
    if (text.size != 0){
        push_fancy_string(arena, line, text);
    }
    if (block != 0){
        push_fancy_line(block, line);
    }
    return(line);
}
internal Fancy_Line*
push_fancy_line(Arena *arena, Fancy_Block *block, Face_ID face, FColor fcolor){
    return(push_fancy_line(arena, block, face, fcolor, SCu8()));
}
internal Fancy_Line*
push_fancy_line(Arena *arena, Fancy_Block *block, Face_ID face, String_Const_u8 val){
    return(push_fancy_line(arena, block, face, fcolor_zero(), val));
}
internal Fancy_Line*
push_fancy_line(Arena *arena, Fancy_Block *block, FColor color, String_Const_u8 val){
    return(push_fancy_line(arena, block, 0, color, val));
}
internal Fancy_Line*
push_fancy_line(Arena *arena, Fancy_Block *block, Face_ID face){
    return(push_fancy_line(arena, block, face, fcolor_zero(), SCu8()));
}
internal Fancy_Line*
push_fancy_line(Arena *arena, Fancy_Block *block, FColor color){
    return(push_fancy_line(arena, block, 0, color, SCu8()));
}
internal Fancy_Line*
push_fancy_line(Arena *arena, Fancy_Block *block, String_Const_u8 val){
    return(push_fancy_line(arena, block, 0, fcolor_zero(), val));
}
internal Fancy_Line*
push_fancy_line(Arena *arena, Fancy_Block *block){
    return(push_fancy_line(arena, block, 0, fcolor_zero(), SCu8()));
}

////////////////////////////////

internal f32
get_fancy_string_width__inner(Application_Links *app, Face_ID face,
                              Fancy_String *string)
{
    f32 result = 0.f;
    for (;string != 0;
         string = string->next)
    {
        Face_ID use_face = face;
        if (string->face != 0){
            use_face = string->face;
        }
        if (use_face != 0)
        {
            result += get_string_advance(app, use_face, string->value);
            Face_Metrics metrics = get_face_metrics(app, use_face);
            f32 normal_advance = metrics.normal_advance;
            result += (string->pre_margin + string->post_margin)*normal_advance;
        }
    }
    return(result);
}

internal f32
get_fancy_string_height__inner(Application_Links *app, Face_ID face, Fancy_String *string){
    f32 result = 0.f;
    if (face != 0)
    {
        Face_Metrics metrics = get_face_metrics(app, face);
        result = metrics.line_height;
    }
    for (;string != 0;
         string = string->next){
        if (string->face != 0){
            Face_ID use_face = string->face;
            Face_Metrics metrics = get_face_metrics(app, use_face);
            result = Max(result, metrics.line_height);
        }
    }
    return(result);
}

internal f32
get_fancy_string_text_height__inner(Application_Links *app, Face_ID face, Fancy_String *string){
    f32 result = 0.f;
    if (face != 0){
        Face_Metrics metrics = get_face_metrics(app, face);
        result = metrics.text_height;
    }
    for (;string != 0;
         string = string->next){
        if (string->face != 0){
            Face_ID use_face = string->face;
            Face_Metrics metrics = get_face_metrics(app, use_face);
            result = Max(result, metrics.text_height);
        }
    }
    return(result);
}

internal Vec2_f32
draw_fancy_string__inner(App *app, Face_ID face, FColor fore, Fancy_String *first_string, Vec2_f32 p, u32 flags, Vec2_f32 delta)
{
    f32 base_line = 0.f;
    for (Fancy_String *string = first_string;
         string != 0;
         string = string->next)
    {
        Face_ID use_face = face;
        if (string->face != 0)
        {
            use_face = string->face;
        }
        if (use_face != 0)
        {
            Face_Metrics metrics = get_face_metrics(app, use_face);
            base_line = Max(base_line, metrics.ascent);
        }
    }
    
    Vec2_f32 down_delta = vec2(-delta.y, delta.x);
    for (Fancy_String *string = first_string;
         string != 0;
         string = string->next)
    {
        Face_ID use_face = face;
        if (string->face != 0){
            use_face = string->face;
        }
        FColor use_fore = fore;
        if (fcolor_is_valid(string->fore)){
            use_fore = string->fore;
        }
        if (use_face != 0)
        {
            ARGB_Color use_argb = fcolor_resolve(use_fore);
            Face_Metrics metrics = get_face_metrics(app, use_face);
            f32 down_shift = (base_line - metrics.ascent);
            down_shift = clamp_min(0.f, down_shift);
            Vec2_f32 p_shift = down_shift*down_delta;
            Vec2_f32 p_shifted = p + p_shift;
            
            if (fcolor_is_valid(use_fore))
            {
                Vec2_f32 margin_delta = delta*metrics.normal_advance;
                p_shifted += margin_delta*string->pre_margin;
                p_shifted = draw_string_oriented(app, use_face, use_argb, string->value, p_shifted, flags, delta);
                p_shifted += margin_delta*string->post_margin;
            }
            else
            {
                f32 adv = (string->pre_margin + string->post_margin)*metrics.normal_advance;
                adv += get_string_advance(app, use_face, string->value);
                p_shifted += adv*delta;
            }
            
            p = p_shifted - p_shift;
        }
    }
    return(p);
}

internal f32
get_fancy_string_width(Application_Links *app, Face_ID face,
                       Fancy_String *string){
    Fancy_String *next = string->next;
    string->next = 0;
    f32 result = get_fancy_string_width__inner(app, face, string);
    string->next = next;
    return(result);
}

internal f32
get_fancy_string_height(Application_Links *app, Face_ID face,
                        Fancy_String *string){
    Fancy_String *next = string->next;
    string->next = 0;
    f32 result = get_fancy_string_height__inner(app, face, string);
    string->next = next;
    return(result);
}

internal f32
get_fancy_string_text_height(Application_Links *app, Face_ID face,
                             Fancy_String *string){
    Fancy_String *next = string->next;
    string->next = 0;
    f32 result = get_fancy_string_text_height__inner(app, face, string);
    string->next = next;
    return(result);
}

internal Vec2_f32
get_fancy_string_dim(Application_Links *app, Face_ID face, Fancy_String *string){
    Fancy_String *next = string->next;
    string->next = 0;
    Vec2_f32 result = vec2(get_fancy_string_width__inner(app, face, string),
                            get_fancy_string_height__inner(app, face, string));
    string->next = next;
    return(result);
}

internal Vec2_f32
draw_fancy_string(Application_Links *app, Face_ID face, FColor fore,
                  Fancy_String *string, Vec2_f32 p, u32 flags, Vec2_f32 delta){
    Fancy_String *next = string->next;
    string->next = 0;
    Vec2_f32 result = draw_fancy_string__inner(app, face, fore, string, p, flags, delta);
    string->next = next;
    return(result);
}

internal f32
get_fancy_line_width(Application_Links *app, Face_ID face, Fancy_Line *line){
    f32 result = 0.f;
    if (line != 0){
        if (line->face != 0){
            face = line->face;
        }
        result = get_fancy_string_width__inner(app, face, line->first);
    }
    return(result);
}

internal f32
get_fancy_line_height(Application_Links *app, Face_ID face, Fancy_Line *line){
    f32 result = 0.f;
    if (line != 0){
        if (line->face != 0){
            face = line->face;
        }
        result = get_fancy_string_height__inner(app, face, line->first);
    }
    return(result);
}

internal f32
get_fancy_line_text_height(Application_Links *app, Face_ID face, Fancy_Line *line){
    f32 result = 0.f;
    if (line != 0){
        if (line->face != 0){
            face = line->face;
        }
        result = get_fancy_string_text_height__inner(app, face, line->first);
    }
    return(result);
}

internal Vec2_f32
get_fancy_line_dim(Application_Links *app, Face_ID face, Fancy_Line *line){
    Vec2_f32 result = {};
    if (line != 0){
        if (line->face != 0){
            face = line->face;
        }
        result = vec2(get_fancy_string_width__inner(app, face, line->first), get_fancy_string_height__inner(app, face, line->first));
    }
    return(result);
}

internal Vec2_f32
draw_fancy_line(Application_Links *app, Face_ID face, FColor fore,
                Fancy_Line *line, Vec2_f32 p, u32 flags, Vec2_f32 delta){
    Vec2_f32 result = {};
    if (line != 0){
        if (line->face != 0)
        {
            face = line->face;
        }
        if (fcolor_is_valid(line->fore))
        {
            fore = line->fore;
        }
        result = draw_fancy_string__inner(app, face, fore, line->first, p, flags, delta);
    }
    return(result);
}

internal f32
get_fancy_block_width(Application_Links *app, Face_ID face, Fancy_Block *block){
    f32 width = 0.f;
    for (Fancy_Line *node = block->first;
         node != 0;
         node = node->next){
        f32 w = get_fancy_line_width(app, face, node);
        width = Max(width, w);
    }
    return(width);
}

internal f32
get_fancy_block_height(Application_Links *app, Face_ID face, Fancy_Block *block){
    f32 height = 0.f;
    for (Fancy_Line *node = block->first;
         node != 0;
         node = node->next){
        height += get_fancy_line_height(app, face, node);
    }
    return(height);
}

internal Vec2_f32
get_fancy_block_dim(Application_Links *app, Face_ID face, Fancy_Block *block){
    Vec2_f32 result = {};
    result.x = get_fancy_block_width(app, face, block);
    result.y = get_fancy_block_height(app, face, block);
    return(result);
}

internal void
draw_fancy_block(Application_Links *app, Face_ID face, FColor fore,
                 Fancy_Block *block, Vec2_f32 p, u32 flags, Vec2_f32 delta){
    for (Fancy_Line *node = block->first;
         node != 0;
         node = node->next){
        draw_fancy_line(app, face, fore, node, p, flags, delta);
        p.y += get_fancy_line_height(app, face, node);
    }
}

internal Vec2_f32
draw_fancy_string(Application_Links *app, Face_ID face, FColor fore,
                  Fancy_String *string, Vec2_f32 p){
    return(draw_fancy_string(app, face, fore, string, p, 0, vec2(1.f, 0.f)));
}

internal Vec2_f32
draw_fancy_string(Application_Links *app, Fancy_String *string, Vec2_f32 p){
    return(draw_fancy_string(app, 0, fcolor_zero(), string, p, 0, vec2(1.f, 0.f)));
}

internal Vec2_f32
draw_fancy_line(Application_Links *app, Face_ID face, FColor fore,
                Fancy_Line *line, Vec2_f32 p){
    return(draw_fancy_line(app, face, fore, line, p, 0, vec2(1.f, 0.f)));
}

internal void
draw_fancy_block(Application_Links *app, Face_ID face, FColor fore,
                 Fancy_Block *block, Vec2_f32 p)
{
    draw_fancy_block(app, face, fore, block, p, 0, vec2(1.f, 0.f));
}

////////////////////////////////

// TODO(allen): beta: color palette
global FColor f_white      = fcolor_argb(1.0f, 1.0f, 1.0f, 1.0f);
global FColor f_light_gray = fcolor_argb(0.7f, 0.7f, 0.7f, 1.0f);
global FColor f_gray       = fcolor_argb(0.5f, 0.5f, 0.5f, 1.0f);
global FColor f_dark_gray  = fcolor_argb(0.3f, 0.3f, 0.3f, 1.0f);
global FColor f_black      = fcolor_argb(0.0f, 0.0f, 0.0f, 1.0f);
global FColor f_red        = fcolor_argb(1.0f, 0.0f, 0.0f, 1.0f);
global FColor f_green      = fcolor_argb(0.0f, 1.0f, 0.0f, 1.0f);
global FColor f_blue       = fcolor_argb(0.0f, 0.0f, 1.0f, 1.0f);
global FColor f_yellow     = fcolor_argb(1.0f, 1.0f, 0.0f, 1.0f);
global FColor f_pink       = fcolor_argb(1.0f, 0.0f, 1.0f, 1.0f);
global FColor f_cyan       = fcolor_argb(0.0f, 1.0f, 1.0f, 1.0f);
