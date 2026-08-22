/*#processed
4coder_jumping.cpp - Routines commonly used when writing code to jump to locations and seek through jump lists.
*/

// TOP

function b32
ms_style_verify(String line, u64 left_paren_pos, u64 right_paren_pos){
    i1 result = false;
    String line_part = string_skip(line, right_paren_pos);
    if (string_match(string_prefix(line_part, 4), strlit(") : ")) ||
        string_match(string_prefix(line_part, 3), strlit("): "))){
        result = true;
    }
    if (result){
        String number = string_skip(string_prefix(line, right_paren_pos), left_paren_pos + 1);
        if (!string_is_integer(number, 10)){
            result = false;
            u64 comma_pos = string_find_first(number, ',');
            if (comma_pos < number.size){
                String sub_number0 = string_prefix(number, comma_pos);
                String sub_number1 = string_skip(number, comma_pos + 1);
                if (string_is_integer(sub_number0, 10) && string_is_integer(sub_number1, 10)){
                    result = true;
                }
            }
        }
    }
    return(result);
}

function u64
try_skip_rust_arrow(String line){
 u64 pos = 0;
 if (string_match(string_prefix(line, 3), strlit("-->"))){
  String sub = string_skip(line, 3);
  sub = string_skip_chop_whitespace(sub);
  pos = (u64)(sub.str - line.str);
 }
 return(pos);
}

function b32
check_is_note(String line, u64 colon_pos){
 b32 is_note = false;
 u64 note_pos = colon_pos + string_find_first(string_skip(line, colon_pos), strlit("note"));
 if (note_pos < line.size){
  b32 is_all_whitespace = true;
  for (u64 i = colon_pos + 1; i < note_pos; i += 1){
   if (!char_is_whitespace(line.str[i])){
    is_all_whitespace = false;
    break;
   }
  }
  if (is_all_whitespace){
   is_note = true;
  }
 }
 return(is_note);
}

function Parsed_Jump
parse_jump_location(String8 line)
{
 Parsed_Jump jump = {};
 jump.sub_jump_indented = (string_get_character(line, 0) == ' ');
 
 String reduced_line = string_skip_chop_whitespace(line);
 u64 whitespace_length = (u64)(reduced_line.str - line.str);
 line = reduced_line;
 
 String kv_jump_magic = strlit("[kv]");
 if(starts_with(line, kv_jump_magic))
 {//-kv jump @kv_jump_syntax
  line = string_skip(line, kv_jump_magic.count + 1);
  {//-file
   u64 end_of_file_path = string_find_first(line, ']');
   jump.location.file = string_prefix(line, end_of_file_path);
   line = string_skip(line, end_of_file_path+2);
  }
  {//-byte pos
   u64 end_of_pos = string_find_first(line, ']');
   String pos_string = string_prefix(line, end_of_pos);
   jump.location.pos = (i32)string_to_u64(pos_string, 10);
  }
  
  if(jump.location.pos){ jump.success = true; }
 }
 else
 {//-Cooked-up syntax by other compilers
  u64 left_paren_pos = string_find_first(line, '(');
  u64 right_paren_pos = left_paren_pos + string_find_first(string_skip(line, left_paren_pos), ')');
  b32 is_ms_style = false;
  for (;!is_ms_style && right_paren_pos < line.size;) {
   //-NOTE(kv) Microsoft style
   if (ms_style_verify(line, left_paren_pos, right_paren_pos)) {
    is_ms_style = true;
    i32 colon_position = (i1)(right_paren_pos + string_find_first(string_skip(line, right_paren_pos), ':'));
    if (colon_position < (i1)line.size) {
     if (check_is_note(line, colon_position)) {
      jump.sub_jump_note = true;
     }
     
     String location_str = string_prefix(line, colon_position);
     location_str = string_skip_chop_whitespace(location_str);
     
     i1 close_pos = (i1)right_paren_pos;
     i1 open_pos = (i1)left_paren_pos;
     
     if (0 < open_pos && open_pos < (i1)location_str.size){
      String file = SCu8(location_str.str, open_pos);
      file = string_skip_chop_whitespace(file);
      
      if (file.size > 0){
       String line_number = string_skip(string_prefix(location_str, close_pos), open_pos + 1);
       line_number = string_skip_chop_whitespace(line_number);
       
       if (line_number.size > 0){
        u64 comma_pos = string_find_first(line_number, ',');
        if (comma_pos < line_number.size){
         String column_number = string_skip(line_number, comma_pos + 1);
         line_number = string_prefix(line_number, comma_pos);
         jump.location.line = (i1)string_to_u64(line_number, 10);
         jump.location.column = (i1)string_to_u64(column_number, 10);
        }else{
         jump.location.line = (i1)string_to_u64(line_number, 10);
         jump.location.column = 0;
        }
        jump.location.file = file;
        colon_position = colon_position + (i1)whitespace_length;
        jump.success = true;
       }
      }
     }
    }
   }else{
    left_paren_pos = string_find_first(string_skip(line, left_paren_pos + 1), '(') + left_paren_pos + 1;
    right_paren_pos = string_find_first(string_skip(line, left_paren_pos), ')') + left_paren_pos;
   }
  }
  
  if (!is_ms_style){
   //-NOTE(kv) non-Microsoft
   i1 start = (i1)try_skip_rust_arrow(line);
   
   u64 colon_pos1 = string_find_first(string_skip(line, start), ':') + start;
   if (line.size > colon_pos1 + 1){
    if (is_file_slash(string_get_character(line, colon_pos1 + 1))){
     //NOTE(kv) This is just the colon in the path
     colon_pos1 = string_find_first(string_skip(line, colon_pos1 + 1), ':') + colon_pos1 + 1;
    }
   }
   
   u64 colon_pos2 = string_find_first(string_skip(line, colon_pos1 + 1), ':') + colon_pos1 + 1;
   u64 colon_pos3 = string_find_first(string_skip(line, colon_pos2 + 1), ':') + colon_pos2 + 1;
   
   if(colon_pos3 < line.size)
   {
    if (check_is_note(line, colon_pos3)){
     jump.sub_jump_note = true;
    }
    
    String file_name = string_skip(string_prefix(line, colon_pos1), start);
    String line_number = string_skip(string_prefix(line, colon_pos2), colon_pos1 + 1);
    String column_number = string_skip(string_prefix(line, colon_pos3), colon_pos2 + 1);
    
    if (file_name.size > 0 && line_number.size > 0 && column_number.size > 0){
     jump.location.file = file_name;
     jump.location.line = (i1)string_to_u64(line_number, 10);
     jump.location.column = (i1)string_to_u64(column_number, 10);
     jump.success = true;
    }
   }
   else
   {
    if (colon_pos2 < line.size){
     if (check_is_note(line, colon_pos2)){
      jump.sub_jump_note = true;
     }
     
     String file_name = string_prefix(line, colon_pos1);
     String line_number = string_skip(string_prefix(line, colon_pos2), colon_pos1 + 1);
     
     if (string_is_integer(line_number, 10)){
      if (file_name.size > 0 && line_number.size > 0){
       jump.location.file = file_name;
       jump.location.line = (i1)string_to_u64(line_number, 10);
       jump.location.column = 0;
       jump.success = true;
      }
     }
    }
   }
  }
 }
 
 if(not jump.success){
  block_zero_struct(&jump);
 }else{
  jump.is_sub_jump = (jump.sub_jump_indented or jump.sub_jump_note);
 }
 return(jump);
}

function Parsed_Jump
parse_jump_location(String line, Jump_Flag flags){
    Parsed_Jump jump = parse_jump_location(line);
    if (HasFlag(flags, JumpFlag_SkipSubs) && jump.is_sub_jump){
        block_zero_struct(&jump);
    }
    return(jump);
}

function Parsed_Jump
parse_jump_from_buffer_line(App *app, Arena *arena, Buffer_ID buffer, i64 line, Jump_Flag flags)
{
    Parsed_Jump jump = {};
    String line_str = push_buffer_line(app, arena, buffer, line);
    if (line_str.size > 0){
        jump = parse_jump_location(line_str, flags);
    }
    return(jump);
}

////////////////////////////////

function b32
get_jump_buffer(App *app, Buffer_ID *buffer, Name_Line_Column_Location *location)
{
 b32 result = open_editing_file(app, buffer, location->file, false, true);
 return result;
}

function b32
get_jump_buffer(App *app, Buffer_ID *buffer, ID_Pos_Jump_Location *location, Access_Flag access) {
 *buffer = location->buffer_id;
 return(buffer_exists(app, *buffer));
}

inline b32
get_jump_buffer(App *app, Buffer_ID *buffer, ID_Pos_Jump_Location *location) {
 return(get_jump_buffer(app, buffer, location, Access_Always));
}

function View_ID
switch_to_existing_view(App *app, View_ID view, Buffer_ID buffer) {
 Buffer_ID current_buffer = view_get_buffer(app, view, Access_Always);
 if (view != 0 || current_buffer != buffer) {
  View_ID existing_view = get_first_view_with_buffer(app, buffer);
  if (existing_view != 0) {
   view = existing_view;
  }
 }
 return(view);
}

function void
set_view_to_location(App *app, View_ID view, Buffer_ID buffer, Buffer_Seek seek){
 Buffer_ID current_buffer = view_get_buffer(app, view, Access_Always);
 if (current_buffer != buffer){
  view_set_buffer(app, view, buffer, 0);
 }
 view_set_cursor_and_preferred_x(app, view, seek);
}

function void
jump_to_location(App_Cmd *app, View_ID view, Buffer_ID buffer,
                 Name_Line_Column_Location location)
{
 view_set_active(app, view);
 set_view_to_location(app, view, buffer, seek_location(location));
 if(auto_center_after_jumps){
  center_view(app);
 }
}
function void
jump_to_location(App_Cmd *app, View_ID view,
                 Name_Line_Column_Location location)
{
 Buffer_ID buffer = 0;
 if(get_jump_buffer(app, &buffer, &location)){
  jump_to_location(app, view, buffer, location);
 }
}
function void
jump_to_location(App_Cmd *app, View_ID view, Buffer_ID buffer, ID_Pos_Jump_Location location){
 view_set_active(app, view);
 set_view_to_location(app, view, buffer, seek_pos(location.pos));
 if (auto_center_after_jumps){
  center_view(app);
 }
}
function void
jump_to_location_parsed(App_Cmd *app, View_ID view, String location){
 Parsed_Jump jump = parse_jump_location(location);
 if (jump.success){
  jump_to_location(app, view, jump.location);
 }
}

////////////////////////////////

// BOTTOM

