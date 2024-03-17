#include "game_fui.h"
#include "4ed_kv_parser.cpp"

global b32 fui_v4_zw_active;

#define X_Fui_Types(X) \
X(v1) \
X(v2) \
X(v3) \
X(v4) \
X(i32) \
X(v2i) \
X(v3i) \
X(v4i) \

union Fui_Value
{
#define X(T)   T T;
    X_Fui_Types(X);
#undef X
};

enum Fui_Type
{
#define X(T)   Fui_Type_##T,
    X_Fui_Types(X)
#undef X
};

typedef u32 Fui_Flags;
enum Fui_Flag
{// NOTE: These are total hacks, man!
 Fui_Flag_NULL            = 0x0,
 Fui_Flag_Camera_Aligned  = 0x1,
 Fui_Flag_NOZ             = 0x2,
 Fui_Flag_Clamp_X         = 0x4,
 Fui_Flag_Clamp_Y         = 0x8,
 Fui_Flag_Clamp_Z         = 0x10,
 Fui_Flag_Clamp_01        = 0x20,
 Fui_Flag_Inited          = 1 << 31,
};

struct Fui_Options
{
#define FUI_OPTIONS(X)  \
Fui_Flags flags; \
v1  delta_scale;    \
 
 FUI_OPTIONS(X);
};

struct Fui_Slider
{
 Fui_Value value;  // 16
 
 Fui_Type  type;   // 1
 union
 {
  Fui_Options options;  // 5
  struct { FUI_OPTIONS(X); };
 };
};

struct Fui_Other_Slider
{
 Fui_Slider slider;
 char *filename;
 i32 line_number;
};

struct Fui_Store
{
 char *filename;
 Fui_Slider *sliders;
 i32 count;
};
global Fui_Store *fui_stores;
global Fui_Other_Slider fui_other_sliders[64];
global i32 fui_other_slider_count;
global i32 fui_stores_count;  //NOTE: number of stores, not number of sliders in the store
global Fui_Slider *fui_active_slider;

force_inline b32
fui_is_active()
{
 return fui_active_slider != 0;
}

inline b32 
filename_match(char *a0, char *b0)
{
 String a = string_front_of_path(SCu8(a0));
 String b = string_front_of_path(SCu8(b0));
 return string_match(a,b);
}

//@GetSlider
internal Fui_Slider *
fui_get_other_slider(char *filename, i32 line_number)
{
 auto &sliders = fui_other_sliders;
 for_i32(index,0,fui_other_slider_count)
 {
  Fui_Other_Slider *slider = &sliders[index];
  if (filename_match(slider->filename, filename) && 
      line_number == slider->line_number)
  {
   return &slider->slider;
  }
 }
 return {};
}

//@GetSlider
internal Fui_Slider *
fui_get_slider(char *filename, i32 line_number)
{
 Fui_Store *store = 0;
 for_i32(index,0,fui_stores_count)
 {//NOTE: Get the store
  if (filename_match(fui_stores[index].filename, filename))
  {
   store = &fui_stores[index];
   break;
  }
 }
 
 if (store)
 {
  if (line_number < store->count)
  {
   return &store->sliders[line_number];
  }
  else return {};
 }
 else 
 {
  return fui_get_other_slider(filename, line_number);
 }
}

//@GetSlider
internal Fui_Value
fui_user_main(Fui_Type type, Fui_Value init_value, 
              char *filename, i32 line_number,
              Fui_Options options)
{
 Fui_Store *store = 0;
 for_i32 (index,0,fui_stores_count)
 {//NOTE: Get the store
  if (fui_stores[index].filename == filename)
  {
   store = &fui_stores[index];
   break;
  }
 }
 
 if (store)
 {
  if (line_number < store->count)
  {
   Fui_Slider *slider = &store->sliders[line_number];
   if (!(slider->flags & Fui_Flag_Inited))
   {// NOTE: init slider value
    Fui_Slider new_slider =
    {
     .value   = init_value,
     .type    = type, 
     .options = options,
    };
    new_slider.flags |= Fui_Flag_Inited;
    *slider = new_slider;
   }
   return slider->value;
  }
  else { return {}; }
 }
 else
 {
  Fui_Slider *slider = fui_get_other_slider(filename, line_number);
  if (slider == 0)
  {
   if (fui_other_slider_count < alen(fui_other_sliders))
   {
    Fui_Other_Slider *new_slider = &fui_other_sliders[fui_other_slider_count++];
    *new_slider = {
     .slider = {
      .value   = init_value,
      .type    = type, 
      .options = options,
     },
     .filename    = filename,
     .line_number = line_number,
    };
    slider = &new_slider->slider;
   }
  }
  if (slider) { return slider->value; }
  else { return{}; }
 }
}

#if 0
jump fui_user_type();
#endif
// NOTE: We define overloads to avoid having to specify the type as a separate argument 
// as well as receive the appropriate values as output.
#define X(T) \
\
inline T \
fui_user_type(T init_value_T, char *filename, u32 line_number, Fui_Options options={}) \
{ \
    Fui_Type  type = Fui_Type_##T; \
    Fui_Value init_val = { .T = init_value_T }; \
    Fui_Value value = fui_user_main(type, init_val, filename, line_number, options); \
    return value.T; \
}
//
X_Fui_Types(X)
//
#undef X

internal fui_get_active_slider_return
fui_get_active_slider(fui_get_active_slider_params)
{
    return fui_active_slider;
}

internal fui_set_active_slider_return
fui_set_active_slider(fui_set_active_slider_params)
{
    fui_active_slider = slider;
}

#define fval(value, ...) \
fui_user_type(value, __FILE_NAME__, __LINE__, ##__VA_ARGS__)
//-
#define fval2(x,y,...)      fval(vec2(x,y),     ##__VA_ARGS__)
#define fval3(x,y,z,...)    fval(vec3(x,y,z),   ##__VA_ARGS__)
#define fval4(x,y,z,w,...)  fval(vec4(x,y,z,w), ##__VA_ARGS__)
#define fvali               fval
//NOTE: "c"=convert, "i"=integer, meaning convert integer to floats
#define fci(...)            i2f6(fval(__VA_ARGS__))
#define fval2i(x,y)         fval(vec2i(x,y))
#define fval3i(x,y,z)       fval(vec3i(x,y,z))
#define fval4i(x,y,z,w)     fval(vec4i(x,y,z,w))
#define fbool(value)        fval(value, Fui_Options{.flags = Fui_Flag_Clamp_01})
#define fval01(x)           clamp01(fval(x))
//-

global Fui_Value global_fui_saved_value;

internal fui_save_value_return
fui_save_value(fui_save_value_params)
{
    global_fui_saved_value = slider->value;
}

internal fui_save_value_return
fui_restore_value(fui_save_value_params)
{
 slider->value = global_fui_saved_value;
}

internal i32
fui_is_single_component_slider(String at_string)
{
 i32 component_plus_one = 0;
 if (string_match_lit(at_string, "fvertx")) { component_plus_one = 1; }
 if (string_match_lit(at_string, "fverty")) { component_plus_one = 2; }
 if (string_match_lit(at_string, "fvertz")) { component_plus_one = 3; }
 
 return component_plus_one;
}

internal b32
fui_is_wrapped_slider(String at_string)
{
 return (starts_with_lit(at_string, "fui")    ||
         string_match_lit(at_string, "fval")  ||
         string_match_lit(at_string, "fvert") ||
         string_match_lit(at_string, "funit") ||
         string_match_lit(at_string, "fci") ||
         false);
}

// NOTE: Still in some cases, we want non-wrapped sliders (for like copying values)
internal b32
fui_string_is_slider(String at_string)
{
 return (fui_is_wrapped_slider(at_string)     ||
         starts_with_lit(at_string, "fval")   ||
         starts_with_lit(at_string, "fvert")  ||
         starts_with_lit(at_string, "fkeyframe")  ||
         starts_with_lit(at_string, "fhsv")  ||
         starts_with_lit(at_string, "fbool") ||
         false);
}

internal String
push_float_trimmed(Arena *arena, v1 value)
{
 String result = push_stringf(arena, "%.4ff", value);
 // NOTE: trim trailing zeros
 while (result.len > 0)
 {
  if (result.str[result.len-2] == '0') { result.len -= 1; }
  else { break; }
 }
 result.str[result.len-1] = 'f';
 return result;
}

internal fui_push_slider_value_return
fui_push_slider_value(fui_push_slider_value_params)
{
 String result = {};
 Fui_Value value = slider->value;
 if (i32 component_plus_one = fui_is_single_component_slider(at_string))
 {
  result = push_float_trimmed(arena, value.v3[component_plus_one-1]);
 }
 else
 {
  b32 is_wrapped = fui_is_wrapped_slider(at_string);
  
  switch (slider->type)
  {
   case Fui_Type_v1:
   {// NOTE: Add an extra "f" for readback
    result = push_float_trimmed(arena, value.v1);
   }break;
   case Fui_Type_v2:
   {
    v2 v = value.v2;
    const i32 count = 2;
    String s[count];
    for_i32(index,0,count) { s[index] = push_float_trimmed(arena, v[index]); }
    result = push_stringf(arena, "vec2(%.*s, %.*s)", string_expand(s[0]), string_expand(s[1]));
   }break;
   case Fui_Type_v3:
   {
    v3 v = value.v3;
    const i32 count = 3;
    String s[count];
    for_i32(index,0,count) { s[index] = push_float_trimmed(arena, v[index]); }
    result = push_stringf(arena, "vec3(%.*s, %.*s, %.*s)", string_expand(s[0]), string_expand(s[1]), string_expand(s[2]));
   }break;
   case Fui_Type_v4:
   {
    v4 v = value.v4;
    const i32 count = 4;
    String s[count];
    for_i32(index,0,count) { s[index] = push_float_trimmed(arena, v[index]); }
    result = push_stringf(arena, "vec4(%.*s, %.*s, %.*s, %.*s)", string_expand(s[0]), string_expand(s[1]), string_expand(s[2]), string_expand(s[3]));
   }break;
   
   case Fui_Type_i32:
   {
    i32 v = value.i32;
    result = push_stringf(arena, "%d", v);
   }break;
   case Fui_Type_v2i:
   {
    v2i v = value.v2i;
    result = push_stringf(arena, "vec2i(%d,%d)", v[0], v[1]);
   }break;
   case Fui_Type_v3i:
   {
    v3i v = value.v3i;
    result = push_stringf(arena, "vec3i(%d,%d,%d)", v[0], v[1], v[2]);
   }break;
   case Fui_Type_v4i:
   {
    v4i v = value.v4i;
    result = push_stringf(arena, "vec4i(%d,%d,%d,%d)", v[0], v[1], v[2], v[3]);
   }break;
   invalid_default_case;
  }
  
  if (is_wrapped == false)
  {
   switch (slider->type)
   {
    case Fui_Type_v2:
    case Fui_Type_v3:
    case Fui_Type_v4:
    {
     result = string_substring(result, Ii64(5, result.size-1));
    }break;
    
    case Fui_Type_v2i:
    case Fui_Type_v3i:
    case Fui_Type_v4i:
    {
     result = string_substring(result, Ii64(6, result.size-1));
    }break;
   }
  }
 }
 
 return result;
}

internal fui_at_slider_p_return
fui_at_slider_p(fui_at_slider_p_params)
{
 Token_Iterator_Array tk_value = token_it_at_cursor(app); 
 Token_Iterator_Array *tk = &tk_value;
 if ( tk->tokens )
 {
  Token *token = tk->ptr;
  Scratch_Block scratch(app);
  String at_string = push_token_lexeme(app, scratch, buffer, token);
  
  return fui_string_is_slider(at_string);
 } 
 else { return false; }
}

internal fui_handle_slider_return
fui_handle_slider(fui_handle_slider_params)
{
 b32 result = false;
 
 b32 at_slider = fui_at_slider_p(app, buffer);
 String at_string = {};
 if (at_slider)
 {
  Scratch_Block scratch(app);
  
  Range_i64 slider_value_range = {};
  char *filename_c = to_c_string(scratch, filename);
  Fui_Slider *slider = fui_get_slider(filename_c, line_number);
  if (slider)
  {
   b32 parse_ok = false;
   {// NOTE(kv): Parsing
    Token_Iterator_Array tk_value = token_it_at_cursor(app); 
    Token_Iterator_Array *tk = &tk_value;
    Quick_Parser parser_value = qp_new(app, buffer, tk);
    Quick_Parser *p = &parser_value;
    at_string = qp_push_token(p,scratch);
    qp_eat_token_kind(p, TokenBaseKind_Identifier);
    qp_eat_token_kind(p, TokenBaseKind_ParentheticalOpen);
    // NOTE: At value
    slider_value_range.min = get_token_pos(p);
    {
     i32 component_count = 1;
     if (!(fui_is_wrapped_slider(at_string) || 
           fui_is_single_component_slider(at_string)))
     {
      switch(slider->type)
      {
       case Fui_Type_v2: case Fui_Type_v2i: { component_count = 2; }break;
       case Fui_Type_v3: case Fui_Type_v3i: { component_count = 3; }break;
       case Fui_Type_v4: case Fui_Type_v4i: { component_count = 4; }break;
       default: { component_count = 1; }break;
      }
     }
     
     for_i32(index,0,component_count)
     {
      i32 eat_result = qp_eat_until_char_lit(p, ",)");
      if (index < component_count-1)
      {
       if (eat_result == 2)
       {// NOTE: early closing paren
        p->ok = false;
        break;
       }
       else { qp_eat_token(p); }
      }
     }
    }
    slider_value_range.max = qp_get_pos(p);
    parse_ok = p->ok;
   }
   
   if (parse_ok)
   {
    fui_save_value(slider);
    fui_set_active_slider(slider);
    //NOTE
    b32 writeback = fui_editor_ui_loop(app);
    
    if (writeback)
    {// NOTE(kv): save the results
     String value_string = fui_push_slider_value(scratch, slider, at_string);
     buffer_replace_range(app, buffer, slider_value_range, value_string);
    }
    else 
    {
     fui_restore_value(slider);
    }
    
    result = true;
    fui_set_active_slider(0);
    fui_v4_zw_active = false;
   }
  }
 }
 return result;
}

force_inline Fui_Options
fopts(u32 flags, v1 delta_scale)
{
 return Fui_Options{.flags=flags, .delta_scale = delta_scale};
}

force_inline Fui_Options
fopts_add_flags(Fui_Options options, u32 flags)
{
 options.flags |= flags;
 return options;
}

force_inline Fui_Options
fopts_add_delta_scale(Fui_Options options, v1 delta_scale)
{
 if(options.delta_scale == 0.f)
 {
  options.delta_scale = delta_scale;
 }
 return options;
}

force_inline Fui_Options fopts_maybe(u32 flags) { return {.flags=flags}; }
force_inline Fui_Options fopts_maybe(Fui_Options options) { return options; }
force_inline Fui_Options fopts_maybe() { return {}; }
