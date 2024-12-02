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

#include "generated/basic_types.gen.h"

struct Type_Info{
 String name;
 i1     size;
 I_Type_Kind kind;
 i32 count;
 union{
  Basic_Type Basic_Type;
  arrayof<I_Struct_Member> members;
  struct{
   Type_Info *discriminator_type;
   arrayof<I_Union_Member> union_members;
  };
  arrayof<I_Enum_Member> enum_members;
  Type_Info *array_item_type;
 };
};

#include "generated/basic_types_info.gen.h"

inline usize
get_basic_type_size(Basic_Type type)
{
 return basic_types_info[type].size;
}
//-