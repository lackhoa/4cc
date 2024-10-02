#pragma once

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

struct Union_Variant{
 //-NOTE Input data
 i32    enum_value;
 String name;
 String name_lower;
 Meta_Struct_Members struct_members;
 //-NOTE Auto-generated fields
 String enum_name;
 String struct_name;
};

struct K_Struct{
 String name;
 Meta_Struct_Members members;
};

#define k_struct(text)  k_struct_func(strlit(#text))
function void k_print_struct(Printer &p, K_Struct struc);

//-