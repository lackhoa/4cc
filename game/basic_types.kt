meta_table(T) basic_types
{
 v1, v2, v3, v4,
 i1, i2, i3, i4,
 String, u32, u64,
}

gen_file "basic_types.gen.h"
{
 enum Basic_Type
 {
  Basic_Type_None = 0,
  gen_for(basic_types)
  {
   Basic_Type_`T,
  }
  Basic_Type_Count,
 };
 
 gen_for(basic_types)
 {
  function Basic_Type
   basic_type_from_pointer(`T *pointer)
  {
   return Basic_Type_`T;
  }
 }
}

gen_file "basic_types_read.gen.h"
{
 function void
  read_binary_String(Binary_Reader *r, String *dst)
 {
  read_binary_size(r, sizeof(u32), &dst->count);
  //NOTE(kv) ultra piggy! Just point to the file!
  dst->str = r->pos;
  
  if(r->end_pos - r->pos < isize(dst->count)){
   r->ok = false;
  }
  if(r->ok){
   r->pos += dst->count;
  }
 }
 
 gen_for(basic_types except(String))
 {
  kv_inline void
   read_binary_`T(Binary_Reader *r, `T *dst)
  {
   read_binary_size(r, sizeof(`T), dst);
  }
  
  kv_inline `T
   read_binary_`T(Binary_Reader *r)
  {
   `T dst;
   read_binary_size(r, sizeof(`T), &dst);
   return dst;
  }
 }
}

gen_file "basic_types_info.gen.h"
{
 global Type_Info basic_types_info[Basic_Type_Count+1] = {
  Type_Info{},
  gen_for(basic_types)
  {
   Type_Info{
    .name=strlit(`quotes(T)),
    .size=i1(sizeof(`T)),
    .kind=I_Type_Kind_Basic,
    .Basic_Type=Basic_Type_`T,
   },
  }
 };
 
 gen_for(basic_types)
 {
  global Type_Info Type_Info_`T = basic_types_info[Basic_Type_`T];
 }
}
//-