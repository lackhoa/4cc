#include "game_fui.h"

global b32 fui_v4_zw_active;
global u32 slider_cycle_counter;

kv_inline usize
slider_value_size(Fui_Slider *slider){
 return(get_basic_type_size(slider->type));
}

// todo: Put these in a struct
global Fui_Slider *fui_active_slider;
global String      fui_active_slider_string;

struct Line_Map_Entry{
 i32 linum;
 Fui_Slider *slider;
};
//
global Line_Map_Entry *line_map;
//global Arena *slider_store;
global Arena *dll_arena;

//-The Slow Path

struct Slow_Line_Map_Entry{
 String file;
 i32 linum;
 Fui_Slider *slider;
};

struct Slow_Line_Map{
 i32 cap;
 i32 count;
 struct Slow_Line_Map_Entry *map;
};
global Slow_Line_Map slow_line_map;
//global Arena *slow_slider_store;

function fui_is_active__return
fui_is_active(fui_is_active__params) {
 return fui_active_slider != 0;
}
//~
//@GetSlider
void *
fast_fval_inner(Basic_Type type, void *init_value,
                i32 linum, Fui_Options options)
{
 void *result = 0;
 u64 cycle_start = gb_rdtsc();
 
 Fui_Slider *slider = line_map[linum].slider;
 // @fui_ensure_nonzero_offset
 if(slider != 0)
 {
  result = slider+1;
 }
 else
 {//NOTE: Not found -> add new slider to the store
  usize value_size = get_basic_type_size(type);
  slider = cast(Fui_Slider *)(push_size(dll_arena, sizeof(Fui_Slider)+value_size));
  line_map[linum].slider = slider;
  
  b32 is_vertex = (options.flags & Slider_Vertex);
  b32 is_vector = (options.flags & Slider_Vector);
  if (is_vertex || is_vector){
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
function void *
slow_fval_inner(Basic_Type type, void *init_value,
                const char *file_c, i32 linum,
                Fui_Options options)
{
 void *result = 0;
 u64 cycle_start = gb_rdtsc();
 Slow_Line_Map &map = slow_line_map;
 Fui_Slider *slider = 0;
 
 String file = SCu8(file_c);
 for_i32(index,0,map.count)
 {
  // NOTE: In the init call, we put a string pointer in the linemap,
  // in later calls, we pass in the same pointer -> match -> win.
  Slow_Line_Map_Entry entry = map.map[index];
  if (entry.file  == file &&
      entry.linum == linum)
  {
   slider = entry.slider;
   break;
  }
 }
 
 if(slider)
 {
  result = slider+1;
 }
 else
 {//-Not found
  usize value_size = get_basic_type_size(type);
  slider = cast(Fui_Slider*)push_size(dll_arena, sizeof(Fui_Slider) + value_size);
  map.map[map.count++] = Slow_Line_Map_Entry{
   .file   = file,
   .linum  = linum,
   .slider = slider,
  };
  kv_assert(map.count < map.cap);
  
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
inline b32 
filename_match(String a0, String b0) {
 String a = path_filename(a0);
 String b = path_filename(b0);
 return string_match(a,b);
}
//@GetSlider
function Fui_Slider *
fui_get_slider_external(String file, i32 linum){
 Fui_Slider *slider = 0;
 if(filename_match(file, DRIVER_FILE_NAME)){
  slider = line_map[linum].slider;
 }else{
  auto &map = slow_line_map;
  u32 offset = 0;
  for_i32(index,0,map.count){
   Slow_Line_Map_Entry entry = map.map[index];
   if(filename_match(entry.file, file) &&
      entry.linum == linum){
    slider = entry.slider;
    break;
   }
  }
 }
 return slider;
}
function Fui_Slider *
fui_get_active_slider(void) { return fui_active_slider; }
function void
fui_set_active_slider(Fui_Slider *slider, String string) {
 fui_active_slider        = slider;
 fui_active_slider_string = string;
}
//-
global const i32 MAX_SLIDER_VALUE_SIZE = 32;
global u8 global_fui_saved_value[MAX_SLIDER_VALUE_SIZE];  //TODO: why is this a global?

function void
fui_save_value(Fui_Slider *slider)
{
 void *value = slider+1;
 usize size = slider_value_size(slider);
 block_copy(global_fui_saved_value, value, size);
}

function void
fui_restore_value(Fui_Slider *slider)
{
 void *value = slider+1;
 usize size = slider_value_size(slider);
 block_copy(value, global_fui_saved_value, size);
}

function b32
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
function b32
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
function void
print_code(Printer &p, Basic_Type type, void *value0, b32 wrapped);

function void
print_fieldf(Printer &p, Basic_Type type, char *name, void *value){
 print(p, ".");
 print(p, name);
 print(p, "=");
 print_code(p,type,value,/*wrapped*/true);
 print(p, ", ");
}

#define print_field(printer, type, value_pointer, name)\
print_fieldf(\
printer,\
Type_##type,\
#name,\
&value_pointer->name)

function void
print_float_trimmed(Printer &p, v1 value){
 //NOTE(kv) there's some delete action going on, so we have to make a temp buffer
 Scratch_Block scratch;
 String result = push_stringf(scratch, "%.4ff", value);
 // NOTE: trim trailing zeros
 while (result.len > 0){
  if (result.str[result.len-2] == '0') { result.len -= 1; }
  else { break; }
 }
 result.str[result.len-1] = 'f';
 print(p, result);
}

function void
print_code(Printer &p, Basic_Type type, void *value0, b32 wrapped)
{
 switch(type)
 {
  case Basic_Type_v1:
  case Basic_Type_v2:
  case Basic_Type_v3:
  case Basic_Type_v4:
  {
   v1 *values = cast(v1*)value0;
   i1 count = i1(get_basic_type_size(type) / 4);
   if (count == 1) {
    print_float_trimmed(p, *values);
   } else {
    if (wrapped) { print(p,"V"); print(p,count); print(p,"("); }
    for_i32(index,0,count) {
     if (index != 0) { print(p, ", "); }
     print_float_trimmed(p, values[index]);
    }
    if (wrapped) { print(p, ")"); }
   }
  }break;
  
  case Basic_Type_i1:
  {
   i1 v = *(i1*)value0;
   print(p, v);
  }break;
  case Basic_Type_i2:
  case Basic_Type_i3:
  case Basic_Type_i4:
  {
   i1 *v = (i1*)value0;
   i1 count = i1(get_basic_type_size(type) / 4);
   
   if (wrapped) { print(p, "I"); print(p, count); print(p, "("); }
   for_i32(index,0,count) {
    if (index != 0) { print(p, ","); }
    print(p, v[index]);
   }
   if (wrapped) { print(p, ")"); }
  }break;
  
  invalid_default_case;
 }
}


function String
fui_push_slider_value(Arena *arena, Fui_Slider *slider)
{
 String at_string = fui_active_slider_string;
 void *value0 = slider+1;
 
 b32 wrapped = fui_is_wrapped_slider(at_string);
 Basic_Type type = slider->type;
 i32 cap = 128;
 Printer printer = make_printer_buffer(arena, cap);
 print_code(printer, type, value0, wrapped);
 String result = printer_get_string(printer);
 return result;
}

#define fui_push_active_slider_value_return String
#define fui_push_active_slider_value_params Arena *arena

function fui_push_active_slider_value_return
fui_push_active_slider_value(fui_push_active_slider_value_params)
{
 String result = {};
 if (fui_active_slider)
 {
  result = fui_push_slider_value(arena, fui_active_slider);
 }
 return result;
}

function i64
fui_at_slider_p(App *app, Buffer_ID buffer, Token_Iterator_Array *it_out)
{
 i64 result = 0;
 Scratch_Block scratch(app);
 i64 max_pos = 0;
 Token_Iterator_Array it_value = get_token_it_on_current_line(app, buffer, &max_pos);
 Token_Iterator_Array *it = &it_value;
 
 Token *token = tkarr_read(it);
 while(result == 0 &&
       token       &&
       token->pos < max_pos)
 {
  String at_string = push_token_lexeme(app, scratch, buffer, token);
  if ( fui_string_is_slider(at_string) ) { result = token->pos; }
  else { token = tkarr_inc(it); }
 }
 
 if (it_out) { *it_out = it_value; }
 return result;
}

function fui_handle_slider__return
fui_handle_slider(fui_handle_slider__params)
{
 b32 result = false;
 
 Token_Iterator_Array tk_value; 
 i64 slider_pos = fui_at_slider_p(app, buffer, &tk_value);
 String at_string = {};
 if(slider_pos){
  Scratch_Block scratch(app);
  
  Range_i64 slider_value_range = {};
  Fui_Slider *slider = fui_get_slider_external(filename, line_number);
  if(slider){
   b32 parse_ok = false;
   {// NOTE(kv): Parsing
    Ed_Parser parser_value = make_ep_from_buffer(app, buffer, tk_value);
    Ed_Parser *p = &parser_value;
    at_string = ep_print_token(scratch, p);
    ep_eat_kind(p, TokenBaseKind_Identifier);
    ep_char(p, '(');
    // NOTE: At value
    slider_value_range.min = ep_get_pos(p);
    {
     i32 component_count = 1;
     if(!(fui_is_wrapped_slider(at_string))){
      switch(slider->type){
       case Basic_Type_v2: case Basic_Type_i2: { component_count = 2; }break;
       case Basic_Type_v3: case Basic_Type_i3: { component_count = 3; }break;
       case Basic_Type_v4: case Basic_Type_i4: { component_count = 4; }break;
       default: { component_count = 1; }break;
      }
     }
     
     for_i32(index,0,component_count){
      if(index<component_count-1){
       ep_eat_until_char(p, strlit(","));
       ep_eat(p);
      }else{
       ep_eat_until_char(p, strlit(")"));
       ep_eat(p);
      }
     }
    }
    parse_ok = p->ok_;
    if (p->ok_){
     slider_value_range.max = ep_get_token_delta(p,-1)->pos;
    }
   }
   
   if (parse_ok) {
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
//-
global u32 fui_editor_magic = 'fui_';

function i64
get_millisecond_unix_timestamp()
{
#if OS_WINDOWS
 FILETIME filetime;
 GetSystemTimeAsFileTime(&filetime); //returns ticks in UTC
 
 LARGE_INTEGER li;
 li.LowPart  = filetime.dwLowDateTime;
 li.HighPart = filetime.dwHighDateTime;
 
 // Convert ticks since into seconds
 i64 UNIX_TIME_START = 0x019DB1DED53E8000; // NOTE January 1, 1970 (start of Unix epoch) in "ticks"
 i64 result = (li.QuadPart - UNIX_TIME_START) / Thousand(10);  // NOTE 1 tick = 100 nanoseconds
 return result;
#endif
 
 // NOTE Use "gettimeofday" for linux
}
function u64
get_slider_index()
{//NOTE(kv) Since one frame is 16ms, we'll never get past millisecond
 i64 result = get_millisecond_unix_timestamp();
 return (u64)result;
}
function fui_generate_slider__return
fui_generate_slider(fui_generate_slider__params)
{
 Scratch_Block scratch;
 GET_VIEW_AND_BUFFER;
 b32 ok = true;
 b32 at_fval = false;
 Range_i64 range;
 {
  Ed_Parser parser_value = make_ed_parser_at_cursor(app);
  Ed_Parser *parser = &parser_value;
  Token *token0 = ep_get_token(parser);
  ep_id(parser, strlit("fval"));
  at_fval = parser->ok_;
  range.min = token0->pos;
  range.max = token0->pos + token0->size;
 }
 
 b32 changed = false;
 if(at_fval){
  u64 slider_index = get_slider_index();
  if(ok){
   String replacement = push_stringf(scratch, "_fval_(%zu)", slider_index);
   buffer_replace_range(app, buffer, range, replacement);
   changed = true;
  }
 }
 
 if(not ok){
  vim_set_bottom_text(strlit("ERROR!"));
 }
 return changed;
}
//~