#pragma once

global char *command_name;
global i1 meta_logging_level;
global Arena meta_permanent_arena = make_arena_malloc();

struct Meta_Struct_Member{
 String type;  //NOTE(kv) @janky no star
 i32    type_star_count;
 String name;
 String version_added;
 String version_removed;
 String default_value;
 String discriminator;  //NOTE(kv) for union type only
 b32    unserialized;
};
typedef arrayof<Meta_Struct_Member> Meta_Struct_Members;
inline b32
member_was_removed(Meta_Struct_Member &member){
 return member.version_removed.len != 0;
}

#if 0
//TODO(kv) Parsing union is stupid, just generate the darn thing from enum!
struct Meta_Union_Member{
 String type;  //NOTE(kv) no star
 i32    type_star_count;
 String variant_;
 String version_added;
 String version_removed;
};
#endif

struct Union_Variant{
 //-NOTE Input data
 String name;
 i32    enum_value;
 Meta_Struct_Members struct_members;
 //-NOTE Auto-generated fields
 String enum_name;
 String struct_name;
};

#define meta_logf(...) if(meta_logging_level){ printf(__VA_ARGS__); }
//-
