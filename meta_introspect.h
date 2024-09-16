#pragma once

struct Meta_Struct_Member {
 String type;//NOTE(kv) no star
 i32 type_star_count;
 String name;
 String version_added;
 String version_removed;
 String default_value;
 String discriminator;  //NOTE(kv) union only
};

struct Meta_Union_Member{
 String type;//NOTE(kv) no star
 String name;
 String version_added;
 String version_removed;
 String variant;
};

//-EOF