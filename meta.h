#pragma once

global char *command_name;
global i1 meta_logging_level = 0;
global Arena meta_permanent_arena = make_arena_malloc();

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

#define meta_logf(...) if(meta_logging_level){ printf(__VA_ARGS__); }
//-
