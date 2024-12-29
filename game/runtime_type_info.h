//-
struct Type_Info;
struct I_Struct_Member
{
 Type_Info *type;
 String name;
 u32    offset;
 u32    discriminator_offset;  //NOTE(kv) union only
 b32    unserialized;
};
struct I_Union_Member{
 Type_Info *type;
 String name;
 i32 variant;
};
struct I_Enum_Member{
 String name;
 i32    value;
};
enum I_Type_Kind{
 I_Type_Kind_None = 0,
 I_Type_Kind_Basic,
 I_Type_Kind_Struct,
 I_Type_Kind_Union,
 I_Type_Kind_Enum,
 I_Type_Kind_Array,
};

#include "basic_types.gen.h"

struct Type_Info
{
 String name;
 i32    size;
 I_Type_Kind kind;
 i32 count;
 union
 {
  Basic_Type Basic_Type;
  darray(I_Struct_Member) members;
  struct{
   Type_Info *discriminator_type;
   darray(I_Union_Member) union_members;
  };
  darray(I_Enum_Member) enum_members;
  Type_Info *array_item_type;
 };
};

#define type_info_of(TYPE) &Type_Info_##TYPE

myinline b32
equal(Type_Info *a, Type_Info *b)
{
 return a == b;
}
myinline b32
is_basic_type(Type_Info *type)
{
 return type->kind == I_Type_Kind_Basic;
}
myinline b32
is_struct(Type_Info *type)
{
 return type->kind == I_Type_Kind_Struct;
}
function i32
get_member_index_by_name(Type_Info *type, String name)
{
 kv_assert(type->kind == I_Type_Kind_Struct);
 for_i32(member_index, 0, type->members.count)
 {
  I_Struct_Member &member = type->members[member_index];
  if(member.name == name)
  {
   return member_index;
  }
 }
 InvalidCodePath;
 return 0;
}

// NOTE we ensure that the member exists statically too!
#define member_index_of(TYPE, MEMBER_NAME) \
(void(((TYPE*)0)->MEMBER_NAME), \
get_member_index_by_name(type_info_of(TYPE), strlit(#MEMBER_NAME)))

#include "basic_types_info.gen.h"

myinline Type_Info *
type_info_from_basic_type(Basic_Type type)
{
 return &basic_types_info[type];
}
myinline usize
get_basic_type_size(Basic_Type type)
{
 return type_info_from_basic_type(type)->size;
}
//-