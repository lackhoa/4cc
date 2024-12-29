//-
function sarray(Location_Map)
init_location_maps()
{
 static Location_Map maps[1] = {};
 sarray(Location_Map) result = {maps, 1};
 return result;
}
// @build_location_maps
sarray(Location_Map) location_maps = init_location_maps();

global u32 slider_cycle_counter;
global Active_Slider fui_active_slider;

function b32
fui_is_active()
{// @game_api
 return fui_active_slider.data != 0;
}

myinline Range_i64
get_slider_range(Slider &slider)
{
 return resolve_location(slider.location);
}
function Range_i64
fui_get_slider_range(i32 index)
{
 Slider &slider = driver_data.sliders[index];
 return get_slider_range(slider);
}
//-
function Type_Info *
type_info_from_index(Type_Index type0)
{
 Type_Info *result = (type0 < Basic_Type_Count
                      ? type_info_from_basic_type((Basic_Type)type0)
                      : type_info_of(FUI_Line_Params));
 return result;
}
myinline Type_Info *
get_slider_type_info(Slider &slider)
{
 return type_info_from_index(slider.type);
}
function Type_Info *
active_slider_member_type_info()
{
 auto slider = fui_active_slider;
 Type_Info *result = 0;
 Type_Info *type_info = get_slider_type_info(*slider.data);
 if(is_struct(type_info))
 {
  I_Struct_Member &active_member = type_info->members[slider.active_member_index];
  result = active_member.type;
 }
 else if(is_basic_type(type_info))
 {
  result = type_info;
 }
 else{ InvalidCodePath; }
 
 kv_assert(is_basic_type(result));
 return result;
}
myinline Data_And_Size
active_slider_data()
{
 auto slider = fui_active_slider;
 Data_And_Size result = {};
 Type_Info *type_info = get_slider_type_info(*slider.data);
 if(is_struct(type_info))
 {
  I_Struct_Member &active_member = type_info->members[slider.active_member_index];
  result.data = (u8 *)slider.data->value + active_member.offset;
  result.size = active_member.type->size;
 }
 else if(is_basic_type(type_info))
 {
  result.data = (u8 *)slider.data->value;
  result.size = type_info->size;
 }
 else{ InvalidCodePath; }
 
 return(result);
}
function b32
active_slider_is_discrete(v1 *out_float_increment)
{
 *out_float_increment = 0.f;
 Active_Slider slider = fui_active_slider;
 Type_Info *type = get_slider_type_info(*slider.data);
 if(equal(type, type_info_of(FUI_Line_Params)))
 {
  i32 member_index = slider.active_member_index;
  if(member_index == get_member_index_by_name(type, strlit("radii")))
  {
   *out_float_increment = 1.f / 6.f;
   return true;
  }
  else if(member_index == get_member_index_by_name(type, strlit("lightness")))
  {
   *out_float_increment = 1.f / 2.f;
   return true;
  }
 }
 else
 {
  Type_Info *memtype = active_slider_member_type_info();
  switch(memtype->Basic_Type)
  {
   case Basic_Type_i1:
   case Basic_Type_i2:
   case Basic_Type_i3:
   case Basic_Type_i4:
   return true;
  }
 }
 return false;
}
function b32
active_slider_is_continuous()
{
 v1 ignore;
 return not active_slider_is_discrete(&ignore);
}

//~
function b32 
filename_match(String a0, String b0)
{
 String a = path_filename(a0);
 String b = path_filename(b0);
 return string_match(a,b);
}
function i32
get_file_index_by_name(String path)
{// NOTE(kv) Let's have a convention that file names are all that matters,
 // since I don't wanna have to deal with canonicalizing paths,
 // which is complicated and doesn't help me much.
 i32 result = 0;
 if(driver_data.valid)
 {
  if(filename_match(path, DRIVER_FILE_NAME))
  {// @Incomplete
   result = 1;
  }
 }
 return result;
}
function i32
get_file_index_by_buffer(App *app, Buffer_ID buffer)
{
 Scratch_Scope tmp;
 String path = push_buffer_filepath(app, tmp, buffer);
 return get_file_index_by_name(path);
}

function i16
safe_cast_i16(i64 value)
{
 kv_assert(value >= i16_min and value <= i16_max);
 return (i16)value;
}

function void
build_location_maps(Arena *arena)
{
 Scratch_Block tmp;
 sarray(Location_Map) &maps = location_maps;
 maps.count = 2;
 maps.items = push_array0(arena, Location_Map, maps.count);
 {//-Do one file
  // NOTE piggy stuff!
  darray(Location_Map_Entry*) active_ranges;
  init_dynamic(active_ranges, tmp, 4);
  
  i32 file_index = 1;
  Location_Map map = {};
  //-Map already zeroed
  sarray(Vertex_Info) vertices = driver_data.vertices_info;
  sarray(Text_Object) text_objects = driver_data.text_objects;
  sarray(Marker_Pair) pairs    = driver_data.marker_pairs[file_index];
  Sliders             sliders  = driver_data.sliders;
  struct Type
  {
   Location_Type type;
   i32 index;
   i32 count;
  };
  Type types[] = {
   {.type=Location_Type_Vertex,    .count=vertices.count},
   {.type=Location_Type_Drawn,     .count=pairs.count},
   {.type=Location_Type_Text_Object, .count=text_objects.count},
   {.type=Location_Type_Slider,    .count=sliders.count}
  };
  const i32 type_count = alen(types);
  
  for_i32(type_index, 0, type_count)
  {
   map.count += types[type_index].count;
  }
  map.count += 1;  // NOTE for the null location
  map.items = push_array(arena, Location_Map_Entry, map.count);
  map[0] = {};
  
  for_i32(entry_index, 1, map.count)
  {
   Location_Map_Entry candidates[type_count] = {};
   for_i32(type_index, 0, type_count)
   {
    Location_Map_Entry *candidate = candidates + type_index;
    // NOTE candidate zeroed
    Type type = types[type_index];
    candidate->type      = types[type_index].type;
    candidate->range.min = i16_max;
    candidate->index     = (i16)types[type_index].index;
    if(candidate->index < type.count)
    {
     switch(candidate->type)
     {
      case Location_Type_Vertex:
      {
       Vertex_Info &vertex = vertices[candidate->index];
       candidate->range = vertex.location.range;
      }break;
      
      case Location_Type_Text_Object:
      {
       Text_Object &object = text_objects[candidate->index];
       candidate->range = object.location.range;
      }break;
      
      case Location_Type_Drawn:
      {
       candidate->range = pairs[candidate->index];
      }break;
      
      case Location_Type_Slider:
      {
       candidate->range = sliders[candidate->index].location.range;
      }break;
      
      InvalidDefaultCase;
     }
    }
   }
   
   //-pick the best one to merge in
   i32 least_type_index = -1;
   i16 least_range_min = i16_max;
   for_i32(type_index, 0, type_count)
   {
    i16 test_range_min = candidates[type_index].range.min;
    if(test_range_min < least_range_min)
    {
     least_type_index = type_index;
     least_range_min = test_range_min;
    }
   }
   
   kv_assert(least_type_index >= 0);
   types[least_type_index].index++;
   Location_Map_Entry *new_entry = &map[entry_index];
   *new_entry = candidates[least_type_index];
   
   while(active_ranges.count > 0)
   {//-filter active ranges
    Location_Map_Entry *active_range = get_last(active_ranges);
    if(active_range->range.max < new_entry->range.min)
    {// NOTE Pop this one
     active_ranges.count--;
    }else{
     // NOTE This one is still active
     break;
    }
   }
   if(active_ranges.count > 0)
   {
    Location_Map_Entry *parent = get_last(active_ranges);
    isize parent_location = parent - map.items;  // NOTE(kv) Get array index, bro!
    new_entry->parent_location = safe_cast_i16(parent_location);
   }
   push(&active_ranges, new_entry);
  }
  
  maps[file_index] = map;
  
  //-Check that everything is kosher
  for_i32(type_index, 0, type_count)
  {
   kv_assert(types[type_index].index == types[type_index].count);
  }
 }
}
function i32
get_min_touched_location(i32 file, Range_i64 range)
{
 Location_Map map = location_maps[file];
 i32 result_entry_index = map.count;
 i32 start = 1;
 i32 end   = map.count;
 while(start < end)
 {
  i32 index0 = start + (end-start) / 2;
  i32 index = index0;
  while(map[index].parent_location != 0)
  {// NOTE only test parent ranges
   index = map[index].parent_location;
  }
  
  Range_i64 tested = resolve_location(file, map[index].range);
  if(range.max <= tested.min)
  {// range_is_before_tested
   end = index;
  }
  else if(range.min >= tested.max)
  {// range_is_after_tested
   start = index + 1;
   while(start < end and map[start].parent_location == index)
   {
    start++;
   }
  }
  else
  {// NOTE Check if there's a touched entry somewhere to the left...
   b32 has_touched_to_the_left = false;
   if(index > 1)
   {
    i32 check_index = index - 1;
    while(map[check_index].parent_location != 0)
    {
     check_index = map[check_index].parent_location;
    }
    
    Location_Map_Entry &left_entry = map[check_index];
    Range_i64 left_range = resolve_location(file, left_entry.range);
    if(left_range.max > range.min)
    {
     has_touched_to_the_left = true;
    }
   }
   
   if(has_touched_to_the_left){
    end = index;
   }else{
    result_entry_index = index;
    break;
   }
  }
 }
 return result_entry_index;
}
function Location_Iterator
iterate_touched_locations(i32 file, Range_i64 range)
{// NOTE(kv) As per the convention: returns empty range when no touch.
 Location_Map map = location_maps[file];
 Location_Iterator it = {};
 it.iterator_range = range;
 it.file = file;
 i32 min_touched = get_min_touched_location(file, range);
 if(min_touched < map.count)
 {
  it.entry = &map[min_touched];
  it.entry_range = resolve_location(file, it.entry->range);
 }
 return it;
}
function void
advance(Location_Iterator *it)
{
 i32 file = it->file;
 Location_Map map = location_maps[file];
 Range_i64 test_range = it->iterator_range;
 Location_Map_Entry *max_entry = map.items + map.count;
 
 Location_Map_Entry *next_entry = 0;
 Range_i64 next_range = {};
 for(Location_Map_Entry *entry = it->entry+1;
     entry < max_entry;
     entry++)
 {
  Range_i64 resolved = resolve_location(file, entry->range);
  if(range_overlap(resolved, test_range))
  {
   next_entry = entry;
   next_range = resolved;
   break;
  }
  else if(resolved.min >= test_range.max)
  {
   break;
  }
 }
 it->entry = next_entry;
 it->entry_range = next_range;
}
function Slider *
get_slider_at_pos(i32 file, i64 pos)
{
 Slider *result = 0;
 i32 slider_index = -1;
 for(Location_Iterator it = iterate_touched_locations(file, {pos, pos+1});
     it.entry;
     advance(&it))
 {
  if(it.entry->type == Location_Type_Slider)
  {
   slider_index = it.entry->index;
   break;
  }
 }
 
 if(slider_index != -1)
 {
  result = driver_data.sliders.items + slider_index;
 }
 return result;
}
function Range_i32
get_touched_sliders(i32 file, Range_i64 range)
{// NOTE(kv) The returned range is slider indices
 Range_i32 result = {};
 for(Location_Iterator it = iterate_touched_locations(file, range);
     it.entry;
     advance(&it))
 {
  if(it.entry->type == Location_Type_Slider)
  {
   if(result.min == 0){ result.min = it.entry->index; }
   result.max = it.entry->index + 1;
  }
 }
 return result;
}
function Range_i32
fui_get_sliders_in_range(App *app, Buffer_ID buffer,
                         i64 pos_begin, i64 pos_end)
{// NOTE See @game_api
 i32 file = get_file_index_by_buffer(app, buffer);
 Range_i32 result = get_touched_sliders(file, {pos_begin, pos_end});
 return result;
}
function Slider *
get_hot_slider_under_cursor(App *app)
{
 GET_VIEW_AND_BUFFER;
 i32 file_index = get_file_index_by_buffer(app, buffer);
 i64 curpos = view_get_cursor_pos(app, view);
 Slider *result = get_slider_at_pos(file_index, curpos);
 if(not result)
 {// NOTE Expand to the whole line.
  Range_i64 line_range = get_line_range_from_pos(app, buffer, curpos);
  Range_i32 touched = get_touched_sliders(file_index, line_range);
  if(range_size(touched) > 0)
  {
   result = driver_data.sliders.items + touched.min;
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
function void
fui_set_active_slider(Slider *slider)
{
 fui_active_slider = {};
 fui_active_slider.data = slider;
 
 if(slider)
 {//-Set default active
  Type_Info *type = get_slider_type_info(*slider);
  if(equal(type, type_info_of(FUI_Line_Params)))
  {
   fui_active_slider.active_member_index =
    get_member_index_by_name(type, strlit("radii"));
  }
 }
}
//-
#define MAX_SLIDER_VALUE_SIZE sizeof(FUI_Line_Params)
global u8 global_fui_saved_value[MAX_SLIDER_VALUE_SIZE];  // TODO(kv) why is this a global?

#undef MAX_SLIDER_VALUE_SIZE

function void
fui_save_value(Slider *slider)
{
 void *src = slider->value;
 usize size = get_slider_type_info(*slider)->size;
 block_copy(global_fui_saved_value, src, size);
}
function void
fui_restore_value(Slider *slider)
{
 void *dst = slider->value;
 usize size = get_slider_type_info(*slider)->size;
 block_copy(dst, global_fui_saved_value, size);
}
//-
function void
print_float_trimmed(Printer &p, v1 value)
{
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

function b32
block_is_zero(Data_And_Size data)
{
 for_i32(i, 0, data.size)
 {
  if(data.data[i] != 0){ return false; }
 }
 return true;
}
function void
print_code_basic_type(Printer &p, Basic_Type type, void *value0, b32 wrapped)
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
 }
}
myinline Data_And_Size
get_member_data(Type_Info *type, void *struct_base, i32 member_index)
{
 I_Struct_Member &member_info = type->members[member_index];
 Data_And_Size result;
 result.data = (u8 *)struct_base + member_info.offset;
 result.size = member_info.type->size;
 return result;
}
function void
print_code(Printer &p, Type_Info *type, void *value, b32 wrapped)
{
 if(is_basic_type(type))
 {
  print_code_basic_type(p, type->Basic_Type, value, wrapped);
 }
 else if(is_struct(type))
 {
  // TODO(kv) Is there a "wrapped" version? Like putting a type at the start?
  PrintBraces(p)
  {
   b32 first_member_printed = true;
   for_i32(member_index, 0, type->members.count)
   {
    I_Struct_Member &member_info = type->members[member_index];
    Data_And_Size member_data = get_member_data(type, value, member_index);
    if(not block_is_zero(member_data))
    {
     if(not first_member_printed){ print(p, ", "); }
     first_member_printed = false;
     print_format(p, ".%S = ", member_info.name);
     print_code(p, member_info.type, member_data.data, true);
    }
   }
  }
 }
 else { InvalidCodePath; }
}
function String
fui_print_slider(Arena *arena, Slider &slider)
{// NOTE(kv) Print the slider (value+option) as code, as pretty as we can.
 Printer printer = make_printer_buffer(arena, 128);
 String op = strlit("fval");
 
 b32 is_vertex = slider.flags & Slider_Vertex;
 b32 is_vector = slider.flags & Slider_Vector;
 b32 wrapped = true;
 Type_Info *type = get_slider_type_info(slider);
 b32 is_line_params = type->name == Type_Info_FUI_Line_Params.name;
 if(is_vertex or is_vector)
 {
  op = (is_vertex ? strlit("fvert") : strlit("fvec"));
  wrapped = false;
 }
 else if(type->name == strlit("v2") and
         slider.flags == 0)
 {
  op = strlit("fv2");
  wrapped = false;
 }
 else if(is_line_params)
 {
  op = strlit("flp");
  wrapped = false;
 }
 else if(type->name == "i1" and
         (slider.flags & Slider_Clamp_01))
 {
  op = strlit("fbool");
 }
 
 //-Actual printing
 print(printer, op);
 PrintParens(printer)
 {
  print_code(printer, type, slider.value, wrapped);
 }
 
 String result = printer_get_string(printer);
 return result;
}
function String
fui_push_active_slider_value(Arena *arena)
{// NOTE(kv) game_api.kt
 String result = {};
 if(fui_active_slider.data)
 {
  result = fui_print_slider(arena, *fui_active_slider.data);
 }
 return result;
}
function void
game_send_command(Game_State *state, Game_Command command)
{
 push(&state->command_queue, command);
}
function void
game_send_command(Game_State *state, String command_name)
{// @game_api
 game_send_command(state, {.name=command_name});
}
function b32
fui_handle_enter(Game_State *state, App_Cmd *app)
{// @game_api
 // NOTE(kv) So the editor calls this,
 // in response to a user pressing a slider (if it even is a slider).
 
 // TODO(kv) The reason why this is a callback is because of my stupidity.
 // It should just be a normal thing that it done when the game updates.
 
 View_ID view = get_active_view(app, Access_ReadVisible);
 Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
 b32 result = false;
 Scratch_Block tmp;
 
 Slider *slider = get_hot_slider_under_cursor(app);
 if(slider)
 {//-Slider
  result = true;
  {//-slider is controlled over multiple frames
   fui_save_value(slider);
   fui_set_active_slider(slider);
   b32 writeback = false;
   for(;;)
   {//-UI loop
    // NOTE(kv) Hide the input from 4coder, and pass input to the game.
    User_Input in = get_next_input(app, EventPropertyGroup_AnyKeyboardEvent, EventProperty_Escape);
    if(in.abort)
    {
     break;
    }
    else if(in.event.kind == InputEventKind_KeyStroke and
            in.event.key.code == Key_Code_Return)
    {
     writeback = true; 
     break;
    }
   }
   if(writeback)
   {// NOTE save the results
    String slider_string = fui_print_slider(tmp, *slider);
    Range_i64 slider_range = get_slider_range(*slider);
    buffer_replace_range(app, buffer, slider_range, slider_string);
   }
   else
   {
    fui_restore_value(slider);
   }
  }
  
  fui_set_active_slider(0);
 }
 return result;
}

function void
fixup_markers(i32 file_index, i64 edit_begin, i64 edit_delta)
{
 sarray(i32) positions = driver_data.marked_positions[file_index];
 for(i32 position_index=positions.count-1;
     position_index >= 0;
     position_index--)
 {
  i32 *position = &positions[position_index];
  if(*position > edit_begin){
   *position += (i32)edit_delta;
  }else{
   break;
  }
 }
}
function void
game_buffer_edit_range(Game_State *state,
                       App *app, Buffer_ID buffer,
                       Range_i64 new_range, Range_Cursor old_range)
{
 i64 old_min = old_range.min.pos;
 i64 old_max = old_range.max.pos;
 i64 edit_delta = range_size(new_range) - (old_max - old_min);
 kv_assert(new_range.min == old_min);
 i32 file_index = get_file_index_by_buffer(app, buffer);
 if(file_index != 0)
 {
  fixup_markers(file_index, old_min, edit_delta);
 }
}
function void
fui_draw_over_text_buffer(App *app, Buffer_ID buffer, Text_Layout_ID layout)
{// @game_api
 auto get_character_underline_rect =
 [&](i64 pos) -> rect2
 {
  v1 highlight_thick = 2.0f;
  Rect_f32 rect = text_layout_character_on_screen(app, layout, pos);
  v2 dim = V2(rect.x1 - rect.x0, highlight_thick);
  rect = Rf32_xy_wh(V2(rect.x0, rect.y1 - highlight_thick), dim);
  return rect;
 };
 
 Range_i64 visible_range = text_layout_get_visible_range(app, layout);
 i32 file = get_file_index_by_buffer(app, buffer);
 ARGB_Color function_color = 0xFF587898;
 for(Location_Iterator it = iterate_touched_locations(file, visible_range);
     it.entry;
     advance(&it))
 {
  Range_i64 highlight_range = it.entry_range;
  
  // NOTE(kv) gotta underline two characters, otherwise it's too to see.
  rect2 rect = get_character_underline_rect(highlight_range.min);
  rect.x1 += rect.x1 - rect.x0;
  draw_rect(app, rect, 5.f, function_color, 0);
  
  rect = get_character_underline_rect(highlight_range.max-1);
  rect.x0 -= rect.x1 - rect.x0;
  draw_rect(app, rect, 5.f, function_color, 0);
 }
}
//~