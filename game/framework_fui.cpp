//-
function sarray(Location_Map)
init_location_maps()
{
 local_persist Location_Map maps[1] = {};
 sarray(Location_Map) result = {maps, 1};
 return result;
}
// @build_location_maps
struct Location_Maps
{
 sarray(Location_Map) game;
 sarray(Location_Map) driver;
};

global Location_Maps location_maps = {
 .game   = init_location_maps(),
 .driver = init_location_maps(),
};

function Location_Map
get_location_map(FUI_File file)
{
 sarray(Location_Map) maps = (file.is_driver ?
                              location_maps.driver :
                              location_maps.game);
 return maps[file.index];
}

global u32 slider_cycle_counter;
global Active_Slider fui_active_slider;

function b32
fui_is_active()
{// @game_api
 return fui_active_slider.data != 0;
}

myinline Type_Info *
get_slider_type_info(Slider &slider)
{
 return slider.type;
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
 else
 {
  result = type_info;
 }
 
 return result;
}
myinline Type_Info *
strip_to_basic_type(Type_Info *input)
{
 Type_Info *result = input;
 if(result->kind == I_Type_Kind_Wrapper)
 {
  result = input->wrapped_type;
 }
 kv_assert(result->kind == I_Type_Kind_Basic);
 return result;
}
function Data_And_Size
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
 else
 {
  result.data = (u8 *)slider.data->value;
  result.size = type_info->size;
 }
 return(result);
}
function b32
active_slider_is_discrete(v1 *out_float_increment)
{
 *out_float_increment = 0.f;
 Active_Slider slider = fui_active_slider;
 Type_Info *type = get_slider_type_info(*slider.data);
 if(is_struct(type))
 {// NOTE(kv) Line-params members (FUI_Line_Params, Curve): radii step in sixths of
  // the radius unit, lightness in halves. Other members (Curve offsets) are continuous.
  String member_name = type->members[slider.active_member_index].name;
  if(member_name == strcode(radii))
  {
   *out_float_increment = 1.f / 6.f;
   return true;
  }
  else if(member_name == strcode(lightness_additions))
  {
   *out_float_increment = 1.f / 2.f;
   return true;
  }
 }
 Type_Info *memtype = active_slider_member_type_info();
 switch(strip_to_basic_type(memtype)->Basic_Type)
 {
  case Basic_Type_i1:
  case Basic_Type_i2:
  case Basic_Type_i3:
  case Basic_Type_i4:
  return true;
 }
 return false;
}
function b32
struct_member_is_editable(I_Struct_Member &member)
{// NOTE(kv) Flags members (Line_Flags, FLP_Flags) are set from code, not by keys.
 switch(strip_to_basic_type(member.type)->Basic_Type)
 {
  case Basic_Type_v1:
  case Basic_Type_v2:
  case Basic_Type_v3:
  case Basic_Type_v4:
  return true;
 }
 return false;
}
function void
fui_cycle_active_member()
{// NOTE(kv) Tab on a struct slider: next editable member, wrapping around.
 Type_Info *type = get_slider_type_info(*fui_active_slider.data);
 if(is_struct(type))
 {
  i32 count = type->members.count;
  i32 index = fui_active_slider.active_member_index;
  for_i32(step, 1, count+1)
  {
   i32 candidate = (index + step) % count;
   if(struct_member_is_editable(type->members[candidate]))
   {
    fui_active_slider.active_member_index = candidate;
    break;
   }
  }
 }
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

global String NOTEBOOK_FILE_NAME = strlit("notebook.cpp");

function b32
is_driver_file_path(String path)
{
 String filename = path_filename(path);
 return starts_with(filename, strlit("driver"));
}

function FUI_File
get_fui_file_by_name(String path)
{// NOTE(kv) Let's have a convention that file names are all that matters,
 // since I don't wanna have to deal with canonicalizing paths,
 // which is complicated and doesn't help me much.
 FUI_File result = {};
 result.is_driver = (i16)is_driver_file_path(path);
 auto files = get_file_array(result);
 String filename = path_filename(path);
 for_i32(file_index, 1, files.count)
 {
  if(filename == files[file_index].name)
  {
   result.index = (i16)file_index;
   break;
  }
 }
 return result;
}
function FUI_File
get_fui_file_by_buffer(App *app, Buffer_ID buffer)
{
 Scratch_Scope tmp;
 String path = push_buffer_filepath(app, tmp, buffer);
 return get_fui_file_by_name(path);
}

function i16
safe_cast_i16(i64 value)
{
 kv_assert(value >= i16_min and value <= i16_max);
 return (i16)value;
}

function void
build_location_maps(Arena *arena, b32 is_driver)
{
 // TODO(kv) This program currently requires that all of the input arrays
 // be arranged in the correct source-code order, which is dumb.
 // How should we fix it? Well... maybe it just gets kinda ugly?
 sarray(FUI_File_Data) files = is_driver ? driver_data.files : game_files;
 sarray(Location_Map) &maps = is_driver ? location_maps.driver : location_maps.game;
 
 Scratch_Block tmp;
 // TODO(kv) @Incomplete
 init_zero(maps, arena, files.count);
 
 for_i32(file_index, 1, files.count)
 {//-NOTE One file
  FUI_File_Data &file = files[file_index];
  Location_Map map = {};
  //-Map already zeroed
  sarray(Vertex_Info) vertices     = file.vertices_info;
  sarray(Text_Object) text_objects = file.text_objects;
  sarray(Slider)      sliders      = file.sliders;
  struct Type
  {
   Location_Type type;
   i32 index;
   i32 count;
  };
  Type types[] = {
   {.type=Location_Type_Vertex,      .count=vertices.count},
   {.type=Location_Type_Text_Object, .count=text_objects.count},
   {.type=Location_Type_Slider,      .count=sliders.count}
  };
  const i32 type_count = alen(types);
  
  darray(Location_Map_Entry*) active_ranges;
  init_dynamic(active_ranges, tmp, type_count);
  
  for_i32(type_index, 0, type_count)
  {
   map.count += types[type_index].count;
  }
  map.count += 1;  // NOTE For the null location
  init_static(map, arena, map.count);
  map[0] = {};
  
  for_i32(entry_index, 1, map.count)
  {
   Location_Map_Entry candidates[type_count] = {};
   for_i32(type_index, 0, type_count)
   {
    Location_Map_Entry *candidate = candidates + type_index;
    // NOTE candidate zeroed
    Type type = types[type_index];
    candidate->type      = type.type;
    candidate->range.min = i16_max;
    candidate->index_in_file     = type.index;
    if(candidate->index_in_file < type.count)
    {
     switch(candidate->type)
     {
      case Location_Type_Vertex:
      {
       Vertex_Info &vertex = vertices[candidate->index_in_file];
       candidate->range = vertex.location.range;
      }break;
      
      case Location_Type_Text_Object:
      {
       Text_Object &object = text_objects[candidate->index_in_file];
       candidate->range = object.location;
      }break;
      
      case Location_Type_Slider:
      {
       Slider &slider = sliders[candidate->index_in_file];
       candidate->range = slider.location.range;
       Type_Info *slider_type = get_slider_type_info(slider);
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
get_min_touched_location(FUI_File file, Range_i64 range)
{
 Location_Map map = get_location_map(file);
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
   while(start < end and
         (map[start].parent_location != 0))
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
iterate_touched_locations(FUI_File file, Range_i64 range)
{// NOTE(kv) As per the convention: returns empty range when no touch.
 Location_Map map = get_location_map(file);
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
 FUI_File file = it->file;
 Location_Map map = get_location_map(file);
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
function b32
is_driver_file(i32 file)
{// todo @Incomplete
 b32 result = 0;
 if(file == 1)
 {
  result = 1;
 }
 return result;
}

//-
struct Slider_Slot
{
 Slider_Slot *next;
 Slider slider;
};

#define SLOW_SLIDER_SLOT_COUNT 1024

global Slider_Slot slow_sliders[SLOW_SLIDER_SLOT_COUNT];
//-

function Slider *
get_slider_at_pos(App *app, Buffer_ID buffer, FUI_File file, i64 pos)
{
 Slider *result = 0;
 i32 slider_index = -1;
 for(Location_Iterator it = iterate_touched_locations(file, {pos, pos+1});
     it.entry;
     advance(&it))
 {
  if(it.entry->type == Location_Type_Slider)
  {
   slider_index = it.entry->index_in_file;
   break;
  }
 }
 
 if(slider_index != -1)
 {
  result = &get_fui_file(file).sliders[slider_index];
 }
 
 return result;
}
function Range_i32
get_touched_sliders(FUI_File file, Range_i64 range)
{// NOTE(kv) The returned range is slider indices
 Range_i32 result = {};
 for(Location_Iterator it = iterate_touched_locations(file, range);
     it.entry;
     advance(&it))
 {
  if(it.entry->type == Location_Type_Slider)
  {
   if(result.min == 0){ result.min = it.entry->index_in_file; }
   result.max = it.entry->index_in_file + 1;
  }
 }
 return result;
}
function Slider *
get_hot_slider_under_cursor(App *app)
{
 GET_VIEW_AND_BUFFER;
 FUI_File file = get_fui_file_by_buffer(app, buffer);
 i64 curpos = view_get_cursor_pos(app, view);
 Slider *result = get_slider_at_pos(app, buffer, file, curpos);
 if(not result)
 {// NOTE Expand to the whole line.
  Range_i64 line_range = get_line_range_from_pos(app, buffer, curpos);
  Range_i32 touched = get_touched_sliders(file, line_range);
  if(range_size(touched) > 0)
  {
   result = get_fui_file(file).sliders.items + touched.min;
  }
 }
 return result;
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
union Slider_Value_Storage
{// NOTE(kv) Big enough for every slider type (TypeInfoPointerList)
 v4 v; i4 i; tvert vert; FUI_Line_Params line_params; Curve curve;
 Reference_Placement reference_placement;
};
global Slider_Value_Storage global_fui_saved_value;  // TODO(kv) why is this a global?

function void
fui_save_value(Slider *slider)
{
 void *src = slider->value;
 usize size = get_slider_type_info(*slider)->size;
 block_copy(&global_fui_saved_value, src, size);
}
function void
fui_restore_value(Slider *slider)
{
 void *dst = slider->value;
 usize size = get_slider_type_info(*slider)->size;
 block_copy(dst, &global_fui_saved_value, size);
}
//-
function void
print_float_trimmed(Printer &p, v1 value)
{
 //NOTE(kv) there's some delete action going on, so we have to make a temp buffer
 Scratch_Block scratch;
 String result = push_stringf(scratch, "%.4ff", value);
 // NOTE: trim trailing zeros
 while (result.len > 0)
 {
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
  {
   PrintBraces(p);
   b32 first_member_printed = true;
   for_i32(member_index, 0, type->members.count)
   {
    I_Struct_Member &member_info = type->members[member_index];
    Data_And_Size member_data = get_member_data(type, value, member_index);
    if(not block_is_zero(member_data))
    {
     if(not first_member_printed){ print(p, ", "); }
     first_member_printed = false;
     printf(p, ".%S = ", member_info.name);
     print_code(p, member_info.type, member_data.data, true);
    }
   }
  }
 }
 else if(type->kind == I_Type_Kind_Wrapper)
 {
  print(p, type->constructor);
  {
   PrintParens(p);
   print_code(p, type->wrapped_type, value, 1);
  }
 }
 else { InvalidCodePath; }
}
function String
fui_print_slider(Arena *arena, Slider &slider)
{// NOTE(kv) Print the slider (value+option) as code, as pretty as we can.
 Printer printer = make_printer_buffer(arena, 128);
 String op = strcode(fv);
 
 b32 wrapped = true;
 Type_Info *type = get_slider_type_info(slider);
 b32 is_line_params = type->name == Type_Info_FUI_Line_Params.name;
 if(0);
 //
 else if(type_info_equals(type, v2) and
         slider.flags == 0)
 {
  op = strcode(fv2);
  wrapped = false;
 }
 else if(is_line_params)
 {
  op = strcode(flp);
  wrapped = false;
 }
 else if(type_info_equals(type, Curve))
 {
  op = strcode(fcurve);
  wrapped = false;
 }
 else if(type_info_equals(type, i1) and
         (slider.flags == Slider_Clamp_01))
 {
  op = strcode(fbool);
 }
 //-Actual printing
 print(printer, op);
 {
  PrintParens(printer);
  print_code(printer, type, slider.value, wrapped);
  if(op == strcode(fv) and
     slider.flags != 0)
  {// NOTE(kv) Currently we don't use more slider attributes,
   // so checking flags only is fine.
   printf(printer, ", %S", strlit("TODO CANNOT PRINT FLAGS YET"));
  }
 }
 
 String result = printer_get_string(printer);
 return result;
}
function String
fui_push_active_slider_value(Arena *arena)
{// NOTE(kv) @game_api
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

function void
fixup_markers(FUI_File file, i64 edit_begin, i64 edit_delta)
{
 sarray(i32) positions = get_fui_file(file).marked_positions;
 for(i32 position_index=positions.count-1;
     position_index >= 0;
     position_index--)
 {
  i32 *position = &positions[position_index];
  if(*position > edit_begin)
  {
   *position += (i32)edit_delta;
  }
  else { break; }
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
 FUI_File file = get_fui_file_by_buffer(app, buffer);
 if(file.is_driver and not driver_data.valid)
 {
  // NOTE(kv) can't do work in this state.
 }
 else
 {
  fixup_markers(file, old_min, edit_delta);
 }
}
function void
fui_draw_over_text_buffer(App *app, Buffer_ID buffer, Text_Layout_ID layout)
{// @game_api
 auto get_character_underline_rect = [&](i64 pos) -> rect2
 {
  v1 highlight_thick = 2.0f;
  rect2 rect = text_layout_character_on_screen(app, layout, pos);
  v2 dim = V2(rect.x1 - rect.x0, highlight_thick);
  rect = Rf32_xy_wh(V2(rect.x0, rect.y1 - highlight_thick), dim);
  return rect;
 };
 
 Range_i64 visible_range = text_layout_get_visible_range(app, layout);
 FUI_File file = get_fui_file_by_buffer(app, buffer);
 
 ARGB_Color underline_color = 0x99587898;
 ARGB_Color hot_color_ = underline_color | 0xFF000000;
 Range_i64 hot_range = {};
 {
  Slider *slider = get_hot_slider_under_cursor(app);
  if(slider)
  {
   hot_range = resolve_location(slider->location);
  }
 }
 for(Location_Iterator it = iterate_touched_locations(file, visible_range);
     it.entry;
     advance(&it))
 {
  Range_i64 highlight_range = it.entry_range;
  b32 is_hot = (highlight_range.min == hot_range.min);
  argb highlight_color = (is_hot ? hot_color_ : underline_color);
  if(is_hot)
  {
   rect2 rect0 = text_layout_character_on_screen(app, layout, highlight_range.min);
   rect2 rect1 = text_layout_character_on_screen(app, layout, highlight_range.max-1);
   rect2 rect = rect_union(rect0, rect1);
   draw_rect_outline(app, rect, 5.f, 2.f, highlight_color, 0);
  }
  else
  {
   // NOTE(kv) Gotta underline two characters, otherwise it's too hard to see.
   rect2 rect = get_character_underline_rect(highlight_range.min);
   rect.x1 += rect.x1 - rect.x0;
   draw_rect(app, rect, 5.f, highlight_color, 0);
   
   rect = get_character_underline_rect(highlight_range.max-1);
   rect.x0 -= rect.x1 - rect.x0;
   draw_rect(app, rect, 5.f, highlight_color, 0);
  }
 }
}
//~