//
PACK_BEGIN
struct Meta_Map_File_Header
{
 u32 magic;
 i32 source_name_offset;
 i32 source_name_count;
 i32 gen_name_offset;
 i32 gen_name_count;
 i32 count;
}
PACK_END ;

function Stringz
get_map_path_from_source_path(Arena *arena, String source_path)
{
 String dir      = path_dir(source_path);
 String filename = path_filename(source_path);
 Stringz map_path = push_stringf(arena, "%S/generated/%S.map", dir, filename);
 return map_path;
}
//