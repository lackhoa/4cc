#include "game_fui.h"

global b32 fui_v4_zw_active;

global u32 slider_cycle_counter;

force_inline i32
slider_value_size(Fui_Slider *slider)
{
 return(type_sizes[slider->type]);
}

// todo: Put these in a struct
global Fui_Slider *fui_active_slider;
global String      fui_active_slider_string;

PACK_BEGIN
struct Line_Map_Entry
{
 u16 linum;
 u16 offset;  // NOTE: We only have 65k of data, but can expand by enforcing alignment.
}
PACK_END
//
static_assert(sizeof(Line_Map_Entry) == 4, "size check");
global Line_Map_Entry *line_map;
global Arena *slider_store;

//-The Slow Path

PACK_BEGIN
struct Slow_Line_Map_Entry
{
 String file;
 u16 linum;
 u16 offset;
}
PACK_END;

global Slow_Line_Map slow_line_map;
global Arena *slow_slider_store;

internal fui_is_active_return
fui_is_active(fui_is_active_params)
{
 return fui_active_slider != 0;
}

//~
//@GetSlider
internal void *
fast_fval_inner(Basic_Type type, void *init_value,
                i32 linum, Fui_Options options)
{
 void *result = 0;
 u64 cycle_start = gb_rdtsc();
 
 u8 *store_base = get_arena_base(slider_store);
 u16 offset = line_map[linum].offset;
 Fui_Slider *slider;
 // @fui_ensure_nonzero_offset
 if( offset != 0 )
 {
  slider = cast(Fui_Slider *)(store_base + offset);
  result = slider+1;
 }
 else
 {//NOTE: Not found -> add new slider to the store
  i32 value_size = type_sizes[type];
  slider = cast(Fui_Slider *)(push_array(slider_store, u8, sizeof(Fui_Slider)+value_size));
  line_map[linum].offset = cast(u16)(cast(u8 *)(slider) - store_base);
  
  b32 is_vertex = (options.flags & Slider_Vertex);
  b32 is_vector = (options.flags & Slider_Vector);
  if (is_vertex || is_vector)
  {
   options.flags |= Slider_Camera_Aligned;
   if(options.delta_scale == 0.f) {
    options.delta_scale = default_fvert_delta_scale;
   }
  }
  
  *slider = Fui_Slider{
   .type    = type, 
   .options = options,
  };
  result = slider+1;
  block_copy(result, init_value, value_size);
 }
 
 u64 cycle_end = gb_rdtsc();
 slider_cycle_counter += u32(cycle_end-cycle_start);
 return result;
}

//@GetSlider
internal void *
slow_fval_inner(Basic_Type type, void *init_value,
                const char *file_c, i32 linum,
                Fui_Options options)
{
 void *result = 0;
 u64 cycle_start = gb_rdtsc();
 auto &map   = slow_line_map;
 auto &store = slow_slider_store;
 u16 offset = 0;
 
 String file = SCu8(file_c);
 for_i32(index,0,map.count)
 {
  // NOTE: In the init call, we put a string pointer in the linemap,
  // in later calls, we pass in the same pointer -> match -> win.
  Slow_Line_Map_Entry entry = map.map[index];
  if (entry.file  == file &&
      entry.linum == linum)
  {
   offset = entry.offset;
   break;
  }
 }
 
 u8 *store_base = get_arena_base(store);
 Fui_Slider *slider;
 if( offset != 0 )  // @fui_ensure_nonzero_offset
 {
  slider = cast(Fui_Slider *)(store_base + offset);
  result = slider+1;
 }
 else
 {//NOTE: Not found
  i32 value_size = type_sizes[type];
  u8 *slider_u8 = push_array(store, u8, sizeof(Fui_Slider)+value_size);
  map.map[map.count++] = Slow_Line_Map_Entry{
   .file   = file,
   .linum  = cast(u16)(linum),
   .offset = cast(u16)(slider_u8 - store_base),
  };
  kv_assert(map.count < map.cap);
  
  slider = cast(Fui_Slider *)slider_u8;
  *slider = Fui_Slider{
   .type    = type, 
   .options = options,
  };
  
  result = slider+1;
  block_copy(result, init_value, value_size);
 }
 
 u64 cycle_end = gb_rdtsc();
 slider_cycle_counter += u32(cycle_end-cycle_start);
 return result;
}

//-
#if 0
jump fast_fval();
jump slow_fval();
#endif
// NOTE: We define overloads to avoid having to specify the type as a separate argument 
// as well as receive the appropriate values as output.
// NOTE: The fast path
#define X(T) \
T \
fast_fval(T init_value_T, Fui_Options options, i32 line) \
{ \
auto type = Type_##T; \
void *value = fast_fval_inner(type, cast(void*)(&init_value_T), line, options); \
return *(cast(T *)value); \
}
//
X_Types(X)
//
#undef X

// NOTE: The slow path (NOTE: there's a weird compiler error if we don't specify the default params here?)
#define X(T) \
T \
slow_fval(T init_value_T, Fui_Options options, \
const char *file=__builtin_FILE(), i32 line=__builtin_LINE()) \
{ \
auto type = Type_##T; \
void *value = slow_fval_inner(type, cast(void*)(&init_value_T), file, line, options); \
return *(cast(T *)value); \
}
//
X_Types(X)
//
#undef X
//-

inline b32 
filename_match(String a0, String b0)
{
 String a = path_filename(a0);
 String b = path_filename(b0);
 return string_match(a,b);
}

//@GetSlider
internal Fui_Slider *
fui_get_slider_external(String file, i32 linum)
{
 Fui_Slider *slider = 0;
 if ( filename_match(file, GAME_FILE_NAME) )
 {
  u16 offset = line_map[linum].offset;
  if (offset != 0)
  {
   u8 *store_base = get_arena_base(slider_store);
   slider = cast(Fui_Slider *)(store_base + offset);
  }
 }
 else
 {
  auto &map = slow_line_map;
  u16 offset = 0;
  for_i32(index,0,map.count)
  {
   Slow_Line_Map_Entry entry = map.map[index];
   if (filename_match(entry.file, file) &&
       entry.linum == linum)
   {
    offset = entry.offset;
    break;
   }
  }
  
  if (offset != 0)
  {
   u8 *store_base = get_arena_base(slow_slider_store);
   slider = cast(Fui_Slider *)(store_base + offset);
  }
 }
 
 return slider;
}


internal Fui_Slider *
fui_get_active_slider(void)
{
 return fui_active_slider;
}

internal void
fui_set_active_slider(Fui_Slider *slider, String string)
{
 fui_active_slider        = slider;
 fui_active_slider_string = string;
}

//

global const i32 MAX_SLIDER_VALUE_SIZE = 32;
global u8 global_fui_saved_value[MAX_SLIDER_VALUE_SIZE];  //TODO: why is this a global?

internal void
fui_save_value(Fui_Slider *slider)
{
 void *value = slider+1;
 i32 size = slider_value_size(slider);
 block_copy(global_fui_saved_value, value, size);
}

internal void
fui_restore_value(Fui_Slider *slider)
{
 void *value = slider+1;
 i32 size = slider_value_size(slider);
 block_copy(value, global_fui_saved_value, size);
}

internal b32
fui_is_wrapped_slider(String at_string)
{
 return (starts_with_lit(at_string, "fui")    ||
         string_match_lit(at_string, "fval")  ||
         string_match_lit(at_string, "fvert") ||
         string_match_lit(at_string, "fvertx") ||
         string_match_lit(at_string, "fverty") ||
         string_match_lit(at_string, "fvertz") ||
         string_match_lit(at_string, "fvec") ||
         string_match_lit(at_string, "fvecx") ||
         string_match_lit(at_string, "fvecy") ||
         string_match_lit(at_string, "fvecz") ||
         string_match_lit(at_string, "fcam") ||
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

//-

internal String
fui_push_slider_value(Arena *arena, Fui_Slider *slider)
{
 String at_string = fui_active_slider_string;
 void *value0 = slider+1;
 
 b32 wrapped = fui_is_wrapped_slider(at_string);
 Basic_Type type = slider->type;
 i32 cap = 128;
 Printer printer = make_printer(arena, cap);
 print_code(printer, type, value0, wrapped);
 String result = end_printer(&printer);
 return result;
}

#define fui_push_active_slider_value_return String
#define fui_push_active_slider_value_params Arena *arena

internal fui_push_active_slider_value_return
fui_push_active_slider_value(fui_push_active_slider_value_params)
{
 String result = {};
 if (fui_active_slider)
 {
  result = fui_push_slider_value(arena, fui_active_slider);
 }
 return result;
}


internal fui_at_slider_p_return
fui_at_slider_p(fui_at_slider_p_params)
{
 i64 result = 0;
 Scratch_Block scratch(app);
 i64 max_pos = 0;
 Token_Iterator_Array it_value = get_token_it_on_current_line(app, buffer, &max_pos);
 Token_Iterator_Array *it = &it_value;
 
 Token *token = token_it_read(it);
 while(result == 0 &&
       token       &&
       token->pos < max_pos)
 {
  String at_string = push_token_lexeme(app, scratch, buffer, token);
  if ( fui_string_is_slider(at_string) ) { result = token->pos; }
  else { token = token_it_inc(it); }
 }
 
 if (it_out) { *it_out = it_value; }
 return result;
}

internal fui_handle_slider_return
fui_handle_slider(fui_handle_slider_params)
{
 b32 result = false;
 
 Token_Iterator_Array tk_value; 
 i64 slider_pos = fui_at_slider_p(app, buffer, &tk_value);
 String at_string = {};
 if (slider_pos)
 {
  Scratch_Block scratch(app);
  
  Range_i64 slider_value_range = {};
  Fui_Slider *slider = fui_get_slider_external(filename, line_number);
  if (slider)
  {
   b32 parse_ok = false;
   {// NOTE(kv): Parsing
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
     if (!(fui_is_wrapped_slider(at_string)))
     {
      switch(slider->type)
      {
       case Type_v2: case Type_i2: { component_count = 2; }break;
       case Type_v3: case Type_i3: { component_count = 3; }break;
       case Type_v4: case Type_i4: { component_count = 4; }break;
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
    fui_set_active_slider(slider, at_string);
    //NOTE
    b32 writeback = fui_editor_ui_loop(app);
    
    if (writeback)
    {// NOTE(kv): save the results
     String value_string = fui_push_slider_value(scratch, slider);
     buffer_replace_range(app, buffer, slider_value_range, value_string);
    }
    else 
    {
     fui_restore_value(slider);
    }
    
    result = true;
    fui_set_active_slider(0,String{});
    fui_v4_zw_active = false;
   }
  }
 }
 return result;
}

//~