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
  case I_Type_Kind_Enum:{
   i32 enum_value = 0;
   block_copy(&enum_value, pointer, type->size);
   write_lvalue(writer, enum_value);
  }break;
  InvalidDefaultCase;
 }
}
//-
function b32
serialize_state(FILE *file, Game_State *state)
{
 Writer writer_value = make_writer(file);
 Writer *writer = &writer_value;
 
 //NOTE(kv) We also write the nil terminator, so that dumb tools can pick it up.
#define write_debug_string(string) \
write_size(writer, string, sizeof(string))
 
 {//-Content
  {//-Magic and version
   write_lvalue(writer, autodraw_data_magic);
   write_lvalue(writer, Version_Current);
   {//-Time
    time_t rawtime;
    time(&rawtime);
    static_assert(sizeof(time_t) <= 8);
    u64 time64 = rawtime;
    write_lvalue(writer, time64);
   }
  }
  {//-Miscellaneous state
   write_debug_string("Serialized_State");
   for_i32(viewport_index, 0, GAME_VIEWPORT_COUNT)
   {
    state->serialized.saved_viewports[viewport_index] =
     state->viewports[viewport_index].saved;
   }
   write_binary(writer, &state->serialized);
  }
  write_debug_string("EOF");
 }
 return writer->ok;
}
//-