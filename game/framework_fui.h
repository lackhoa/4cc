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
 Location_Type_Drawn,
 Location_Type_Text_Object,
 Location_Type_Slider,
};
struct Location_Map_Entry
{
 Range_i16 range;
 Location_Type type;
 i16 index;
 i16 parent_location;
};

// NOTE(kv) We could also tag this with a file index, but whatevs
typedef sarray(Location_Map_Entry) Location_Map;

struct Location_Iterator
{
 i32 file;
 Range_i64 iterator_range;
 Location_Map_Entry *entry;
 Range_i64 entry_range;
};
struct Game_Command
{
 String name;
};
//-