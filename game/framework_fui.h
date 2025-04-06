//-
struct Active_Slider
{
 Slider *data;
 i32 active_member_index;
};
//-
enum Location_Type
{
 Location_Type_None,
 Location_Type_Vertex,
 Location_Type_Text_Object,
 Location_Type_Slider,
};
struct Location_Map_Entry
{
 Range_i16 range;
 i32 index_in_file;
 Location_Type type;
 i16 parent_location;
};

// NOTE(kv) We could also tag this with a file index, but whatevs.
typedef sarray(Location_Map_Entry) Location_Map;

struct Location_Iterator
{
 FUI_File file;
 Range_i64 iterator_range;
 Location_Map_Entry *entry;
 Range_i64 entry_range;
};
struct Game_Command
{
 String name;
};

sarray(FUI_File_Data) game_files = {ArrayAndCount(fui_files_)};

function sarray(FUI_File_Data)
get_file_array(FUI_File file)
{
 return (file.is_driver ?
         driver_data.files : game_files);
}
function FUI_File_Data
get_fui_file(FUI_File file)
{
 auto files = get_file_array(file);
 return files[file.index];
}
function Range_i64
resolve_location(FUI_File file, Range_i16 range)
{
 sarray(i32) positions = get_fui_file(file).marked_positions;
 Range_i64 result = {
  positions[range.min],
  positions[range.max],
 };
 return result;
}
myinline Range_i64
resolve_location(Location location)
{
 return resolve_location(location.file, location.range);
}
//-