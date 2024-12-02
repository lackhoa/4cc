//NOTE File created programmatically by C:\Users\vodan\4ed\code\meta_template.cpp:345:
//NOTE Source template: C:\Users\vodan\4ed\code\game\basic_types.kt

 function void
  read_binary_String(Data_Reader &r, String *dst)
 {
  read_binary_size(r, sizeof(u32), &dst->count);
  read_binary_size(r, dst->count, dst->str);
 }
 
   force_inline void
   read_binary_v1(Data_Reader &r, v1 *dst)
  {
   read_binary_size(r, sizeof(v1), dst);
  }
  
  force_inline v1
   read_binary_v1(Data_Reader &r)
  {
   v1 dst;
   read_binary_size(r, sizeof(v1), &dst);
   return dst;
  }
   force_inline void
   read_binary_v2(Data_Reader &r, v2 *dst)
  {
   read_binary_size(r, sizeof(v2), dst);
  }
  
  force_inline v2
   read_binary_v2(Data_Reader &r)
  {
   v2 dst;
   read_binary_size(r, sizeof(v2), &dst);
   return dst;
  }
   force_inline void
   read_binary_v3(Data_Reader &r, v3 *dst)
  {
   read_binary_size(r, sizeof(v3), dst);
  }
  
  force_inline v3
   read_binary_v3(Data_Reader &r)
  {
   v3 dst;
   read_binary_size(r, sizeof(v3), &dst);
   return dst;
  }
   force_inline void
   read_binary_v4(Data_Reader &r, v4 *dst)
  {
   read_binary_size(r, sizeof(v4), dst);
  }
  
  force_inline v4
   read_binary_v4(Data_Reader &r)
  {
   v4 dst;
   read_binary_size(r, sizeof(v4), &dst);
   return dst;
  }
   force_inline void
   read_binary_i1(Data_Reader &r, i1 *dst)
  {
   read_binary_size(r, sizeof(i1), dst);
  }
  
  force_inline i1
   read_binary_i1(Data_Reader &r)
  {
   i1 dst;
   read_binary_size(r, sizeof(i1), &dst);
   return dst;
  }
   force_inline void
   read_binary_i2(Data_Reader &r, i2 *dst)
  {
   read_binary_size(r, sizeof(i2), dst);
  }
  
  force_inline i2
   read_binary_i2(Data_Reader &r)
  {
   i2 dst;
   read_binary_size(r, sizeof(i2), &dst);
   return dst;
  }
   force_inline void
   read_binary_i3(Data_Reader &r, i3 *dst)
  {
   read_binary_size(r, sizeof(i3), dst);
  }
  
  force_inline i3
   read_binary_i3(Data_Reader &r)
  {
   i3 dst;
   read_binary_size(r, sizeof(i3), &dst);
   return dst;
  }
   force_inline void
   read_binary_i4(Data_Reader &r, i4 *dst)
  {
   read_binary_size(r, sizeof(i4), dst);
  }
  
  force_inline i4
   read_binary_i4(Data_Reader &r)
  {
   i4 dst;
   read_binary_size(r, sizeof(i4), &dst);
   return dst;
  }
   force_inline void
   read_binary_u32(Data_Reader &r, u32 *dst)
  {
   read_binary_size(r, sizeof(u32), dst);
  }
  
  force_inline u32
   read_binary_u32(Data_Reader &r)
  {
   u32 dst;
   read_binary_size(r, sizeof(u32), &dst);
   return dst;
  }
   force_inline void
   read_binary_u64(Data_Reader &r, u64 *dst)
  {
   read_binary_size(r, sizeof(u64), dst);
  }
  
  force_inline u64
   read_binary_u64(Data_Reader &r)
  {
   u64 dst;
   read_binary_size(r, sizeof(u64), &dst);
   return dst;
  }
 
