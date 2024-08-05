//-

#include "kv.h"

struct Struct_Member
{
 Basic_Type type;  // todo: this is not the final product
 String name;
};

struct Type_Metadata
{
 String name;
 dyn_array<Struct_Member> members;
};

internal void
main()
{
 Type_Metadata vertex_data_meta;
 auto &meta = vertex_data_meta;
 {
  add_field(meta, String, name);
  add_field(meta, i1,     object_index);
  add_field(meta, b32,    symx);
  add_field(meta, v3,     pos);
  add_field(meta, i1,     basis_index);
 }
 print(type); print(' '); print(name);
 newline;
}

//-EOF