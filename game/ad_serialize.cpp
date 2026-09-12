function i32
read_enum(Type_Info &type, void *pointer){
 kv_assert(type.kind == I_Type_Kind_Enum);
 i32 dst = 0;
 block_copy(&dst, pointer, type.size);
 return dst;
}
function void
write_binary_union(Writer *writer, Type_Info *type,
                   void *pointer, void *pvariant){
 kv_assert(type->kind == I_Type_Kind_Union);
 i32 variant = read_enum(*type->discriminator_type, pvariant);
 
 darray(I_Union_Member) &union_members = type->union_members;
 for_i32(index,0,union_members.count){
  I_Union_Member &member = union_members[index];
  if(member.variant == variant){
   //NOTE(kv) pointer of member is the same as pointer to the union.
   write_binary_func(writer, member.type, pointer);
   break;
  }
 }
}
function void
write_binary_func(Writer *writer, Type_Info *type, void *void_pointer)
{
 u8 *pointer = cast(u8 *)void_pointer;
 switch(type->kind){
  case I_Type_Kind_Basic:{
   if(type->Basic_Type == Basic_Type_String){
    //-String
    String *string = (String *)pointer;
    u32 count = cast(u32)string->count;
    write_lvalue(writer, count);
    write_size(writer, string->data, count);
   }else{
    //-Other basic types
    write_size(writer, pointer, type->size);
   }
  }break;
  case I_Type_Kind_Wrapper:  // NOTE(kv) wrapper_types carry their real members too
  case I_Type_Kind_Struct:{
   for_i32(member_index, 0, type->members.count){
    I_Struct_Member &member = type->members[member_index];
    if(!member.unserialized){
     u8 *member_data = pointer+member.offset;
     if(member.type->kind == I_Type_Kind_Union){
      write_binary_union(writer, member.type, member_data,
                         pointer+member.discriminator_offset);
     }else{
      write_binary_func(writer, member.type, member_data);
     }
    }
   }
  }break;
  case I_Type_Kind_Union:{
   invalid_code_path;//note(kv) can't write without variant information
  }break;
  case I_Type_Kind_Array:{
   Type_Info *item_type = type->array_item_type;
   for_i32(item_index,0,type->count){
    write_binary_func(writer, item_type, pointer + item_type->size*item_index);
   }
  }break;
  case I_Type_Kind_Darray:{
   // NOTE(kv) Every darray(T) shares the Dynamic_Array header layout, so read it as u8.
   darray(u8) *array = (darray(u8) *)pointer;
   Type_Info *item_type = type->array_item_type;
   u32 count = cast(u32)array->count;
   write_lvalue(writer, count);
   for_i32(item_index,0,array->count){
    write_binary_func(writer, item_type, array->items + item_type->size*item_index);
   }
  }break;
  case I_Type_Kind_Enum:{
   i32 enum_value = 0;
   block_copy(&enum_value, pointer, type->size);
   write_lvalue(writer, enum_value);
  }break;
  InvalidDefaultCase;
 }
}
//-
// NOTE(kv) The binary writer above still serves driver.values.ad; the game state itself
// is text now (state.txt, ad_serialize_state.cpp) via this walker:
#include "ad_serialize_text.cpp"
//-