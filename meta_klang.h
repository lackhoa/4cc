#pragma once
enum Parsed_Type_Kind{
 Parsed_Type_None,
 Parsed_Type_Named,
 Parsed_Type_Pointer,
 Parsed_Type_Array,
};
//NOTE(kv) Cheesy type!!!
struct Parsed_Type{
 Parsed_Type_Kind kind;
 String name;
 i32    count; // NOTE either pointer count, or array count
};
inline Parsed_Type
make_type_named(String name){
 Parsed_Type result = {};
 result.kind = Parsed_Type_Named;
 result.name = name;
 return result;
}
inline Parsed_Type
make_type_pointer(String name, i32 count){
 Parsed_Type result = {};
 result.kind  = Parsed_Type_Pointer;
 result.name  = name;
 result.count = count;
 return result;
}
inline Parsed_Type
make_type_array(String name, i32 count){
 Parsed_Type result = {};
 result.kind  = Parsed_Type_Array;
 result.name  = name;
 result.count = count;
 result.count = count;
 return result;
}
struct Meta_Struct_Member{
 String name;
 Parsed_Type type;
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