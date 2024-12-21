global b32 fui_v4_zw_active;
global u32 slider_cycle_counter;

struct Sliders{
 union{
  Slider *sliders;
  Slider *items;
 };
 u32 count;
};

myinline usize
slider_value_size(u32 index){
 usize result = get_basic_type_size(global_sliders[index].type);
 return(result);
}
myinline usize
slider_value_size(Slider *slider){
 usize result = get_basic_type_size(slider->type);
 return(result);
}
myinline void *
get_slider_value(Slider *slider){
 return global_slider_values[slider->index];
}

global Slider *fui_active_slider;

function b32
fui_is_active(){
 return fui_active_slider != 0;
}
//~
//@GetSlider
//-
function b32 
filename_match(String a0, String b0){
 String a = path_filename(a0);
 String b = path_filename(b0);
 return string_match(a,b);
}
function Sliders
get_sliders_for_file(String file)
{//NOTE(kv) Let's have a convention that file names are all that matters,
 //  since I don't wanna have to deal with canonicalizing paths,
 //  which is complicated and doesn't help me much.
 Sliders result = {};
 if(filename_match(file, DRIVER_FILE_NAME))
 {
  result.sliders = global_sliders;
  result.count   = alen(global_sliders);
 }
 return result;
}
function Sliders
get_sliders_for_buffer(App *app, Buffer_ID buffer)
{//TODO(kv) Don't return anything if we're not synced?
 Scratch_Block scratch;
 String file = push_buffer_filepath(app, scratch, buffer);
 return get_sliders_for_file(file);
}
function u32
get_min_touched_slider(Sliders sliders, i64 left, i64 right)
{
 u32 result_index = u32_max;
 u32 start = 0;
 u32 end   = sliders.count;
 while(start < end){
  
  u32 index = start + (end-start) / 2;
  Slider *slider = sliders.sliders+index;
  
  b32 range_is_before = right <= slider->pos;
  b32 range_is_after  = left >= slider->pos + slider->size;
  if(range_is_before){
   
   end = index;
  }else if(range_is_after){
   
   start = index + 1;
  }else{
   //NOTE Touched
   b32 left_slider_is_touched = false;
   if(index != 0){
    Slider *left_slider = sliders.sliders+index-1;
    //NOTE(kv) Since this one is touched, the one to the left is touched
    //  only if it is touched on the right.
    left_slider_is_touched = left_slider->pos + left_slider->size > left;
   }
   if(left_slider_is_touched){
    end = index;
   }else{
    result_index = index;
    break;
   }
  }
 }
 return result_index;
}
function Slider *
get_slider_at_pos(Sliders sliders, i64 pos)
{
 Slider *result = 0;
 u32 index = get_min_touched_slider(sliders, pos, pos+1);
 if(index != u32_max){
  result = global_sliders + index;
 }
 return result;
}
myinline b32
slider_overlaps_range(Slider *slider, i64 begin, i64 end)
{
 b32 slider_is_to_the_left  = slider->pos + slider->size < begin;
 b32 slider_is_to_the_right = slider->pos >= end;
 return not (slider_is_to_the_left or slider_is_to_the_right);
}
function u32
get_touched_sliders(Sliders sliders, i64 pos_begin, i64 pos_end,
                    u32 *out_end_index)
{
 u32 begin_index = get_min_touched_slider(sliders, pos_begin, pos_end);
 u32 end_index = begin_index;
 if(begin_index != u32_max){
  end_index = begin_index+1;
  //NOTE Check sliders to the right
  for_u32(index, begin_index+1, alen(global_sliders)){
   Slider *slider = global_sliders + index;
   if(slider_overlaps_range(slider, pos_begin, pos_end)){
    end_index = index+1;
   }else{
    break;
   }
  }
 }
 
 *out_end_index = end_index;
 return begin_index;
}
function u32
fui_get_sliders_in_range(App *app, Buffer_ID buffer,
                         i64 pos_begin, i64 pos_end,
                         u32 *out_end_index)
{
 Sliders sliders = get_sliders_for_buffer(app, buffer);
 u32 result = get_touched_sliders(sliders, pos_begin, pos_end,
                                  out_end_index);
 return result;
}
function Range_i64
fui_get_slider_range(u32 index)
{
 Slider *slider = global_sliders + index;
 return {slider->pos, slider->pos + slider->size};
}
function Slider *
get_hot_slider_under_cursor(App *app)
{
 Scratch_Block scratch;
 GET_VIEW_AND_BUFFER;
 String file = push_buffer_filepath(app, scratch, buffer);
 i64 curpos = view_get_cursor_pos(app, view);
 
 Sliders sliders = get_sliders_for_file(file);
 Slider *result = get_slider_at_pos(sliders, curpos);
 if(not result){
  //NOTE Expand to the whole line.
  Range_i64 line_range = get_line_range_from_pos(app, buffer, curpos);
  u32 end;
  u32 begin = get_touched_sliders(sliders, RangeExpand(line_range), &end);
  if((begin != u32_max) and (end == begin+1)){
   result = sliders.sliders+begin;
  }
 }
 return result;
}
function b32
fui_at_slider_p(App *app)
{
 Slider *slider = get_hot_slider_under_cursor(app);
 return slider != 0;
}
function Slider *
fui_get_active_slider(void) { return fui_active_slider; }
function void
fui_set_active_slider(Slider *slider) {
 fui_active_slider = slider;
}
//-
global const u32 MAX_SLIDER_VALUE_SIZE = 32;
global u8 global_fui_saved_value[MAX_SLIDER_VALUE_SIZE];  //TODO(kv) why is this a global?

function void
fui_save_value(u32 index)
{
 void *src = global_slider_values[index];
 usize size = slider_value_size(index);
 block_copy(global_fui_saved_value, src, size);
}
function void
fui_restore_value(u32 index)
{
 void *dst = global_slider_values[index];
 usize size = slider_value_size(index);
 block_copy(dst, global_fui_saved_value, size);
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
         false);
}

// NOTE: Still in some cases, we want non-wrapped sliders (for like copying values)
function b32
fui_string_is_slider(String at_string)
{
 return (fui_is_wrapped_slider(at_string)     ||
         starts_with_lit(at_string, "fval")   ||
         starts_with_lit(at_string, "fvert")  ||
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
fui_print_slider(Arena *arena, Slider *slider)
{
 void *value = global_slider_values[slider->index];
 Printer printer = make_printer_buffer(arena, 128);
 
 //TODO(kv) We'll print back everything about the slider, with options and whatever.
 print(printer, strlit("fval"));
 print_parens_block(printer){
  print_code(printer, slider->type, value, true);
 }
 
 String result = printer_get_string(printer);
 return result;
}
function String
fui_push_active_slider_value(Arena *arena)
{
 String result = {};
 if(fui_active_slider) {
  result = fui_print_slider(arena, fui_active_slider);
 }
 return result;
}
function b32
fui_handle_slider(App *app)
{
 GET_VIEW_AND_BUFFER;
 b32 result = false;
 Scratch_Block scratch;
 
 Slider *slider = get_hot_slider_under_cursor(app);
 if(slider){
  result = true;
  fui_save_value(slider->index);
  fui_set_active_slider(slider);
  
  b32 writeback = fui_editor_ui_loop(app);
  if(writeback)
  {// NOTE save the results
   String slider_string = fui_print_slider(scratch, slider);
   Range_i64 slider_range = {slider->pos, slider->pos + slider->size};
   buffer_replace_range(app, buffer, slider_range, slider_string);
  }else{
   fui_restore_value(slider->index);
  }
  
  fui_set_active_slider(0);
  fui_v4_zw_active = false;
 }
 return result;
}
function b32
is_buffer_synced(Game_State *state, String filename)
{
 b32 result = true;
 arrayof<String> *unsynced_files = &state->unsynced_files;
 for_i32(i, 0, unsynced_files->count){
  if(unsynced_files->items[i] == filename){
   result = false;
   break;
  }
 }
 return result;
}
function void
game_buffer_edit_range(Game_State *state,
                       App *app, Buffer_ID buffer,
                       Range_i64 new_range, Range_Cursor old_range)
{
 i64 old_min = old_range.min.pos;
 i64 old_max = old_range.max.pos;
 i64 edit_delta = range_size(new_range) - (old_max - old_min);
 Scratch_Block scratch;
 Sliders sliders = get_sliders_for_buffer(app, buffer);
 if(sliders.count and
    edit_delta != 0)
 {
  kv_assert(new_range.min == old_min);
  String filename = push_buffer_filepath(app, scratch, buffer);
  filename = path_filename(filename);
  
  b32 was_synced = is_buffer_synced(state, filename);
  if(was_synced){
   //-Hopefully resync this buffer!
   b32 resynced = false;
   
   u32 touch_end;
   u32 touch_begin = get_touched_sliders(sliders, old_min, old_max, &touch_end);
   u32 touch_count = touch_end - touch_begin;
   
   u32 min_shift_index = -1;
   
   if(touch_count == 0){
    resynced = true;
    min_shift_index = get_min_touched_slider(sliders, old_min, i64_max);
    
   }else if(touch_count == 1){
    
    Slider *slider0 = sliders.items+touch_begin;
    if(old_min >= slider0->pos and
       old_max <= slider0->pos + slider0->size)
    {//NOTE The edit touches one slider on the right,
     //  meaning that it is a modification to the slider.
     i64 new_size = slider0->size + edit_delta;
     if(new_size > 0){
      slider0->size = new_size;
      min_shift_index = touch_begin+1;
      resynced = true;
     }
    }
   }
   
   //-Shifting stuff
   for_u32(slider_index, min_shift_index, sliders.count){
    Slider *slider = sliders.sliders+slider_index;
    i64 new_pos = slider->pos + edit_delta;
    kv_assert(new_pos >= 0);
    slider->pos = new_pos;
   }
   
   if(not resynced){
    //-All hope is lost...
    filename = push_string(&state->dll_arena, filename);
    state->unsynced_files.push_value(filename);
   }
  }
 }
}
function b32
fui_is_buffer_synced(Game_State *state, App *app, Buffer_ID buffer)
{
 Scratch_Block scratch;
 String filename = push_buffer_filepath(app, scratch, buffer);
 filename = path_filename(filename);
 return is_buffer_synced(state, filename);
}
//~