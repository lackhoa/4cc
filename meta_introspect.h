#pragma once

struct Meta_Struct_Member {
 String type;
 String name;
 String version_added;
 String version_removed;
 String default_value;
 String discriminator;  //NOTE(kv) union only
};

struct Meta_Union_Member{
 String type;
 String name;
 String version_added;
 String version_removed;
 String variant;
};

//-EOF