function String
time_format(char *buf, i32 bufsize, char *format){
 String result = {};
 time_t rawtime;
 time(&rawtime);
 struct tm *timeinfo = localtime(&rawtime);
 size_t strftime_result = strftime(buf, bufsize, format, timeinfo);
 if (strftime_result != 0)
 {
  result = SCu8(buf);
 }
 return result;
}
function i32
read_enum(Type_Info &type, void *pointer){
 kv_assert(type.kind == I_Type_Kind_Enum);
 i32 dst = 0;
 block_copy(&dst, pointer, type.size);
 return dst;
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
  {//-Camera
   write_debug_string("cameras");
   
   i32 camera_count = GAME_VIEWPORT_COUNT;
   write_lvalue(writer, camera_count);
   
   for_i32(camera_index, 0, GAME_VIEWPORT_COUNT){
    Camera_Data *cam = &state->viewports[camera_index].target_camera;
    write_binary(writer, cam);
   }
  }
  {//-Miscellaneous state
   write_debug_string("Serialized_State");
   write_binary(writer, &state->Serialized_State);
  }
  if(0)
  {//-Modeler data
   Modeler &m = state->modeler;
   {//-Vertices
    write_debug_string("vertices");
    i32 vertex_count = m.vertices.count; //NOTE(kv) Just in case we change the type of the count!
    write_lvalue(writer, vertex_count);
    for_i32(vi,0,m.vertices.count){
     write_binary(writer, &m.vertices[vi]);
    }
   }
   {//-Entities
    write_debug_string("entities");
    i32 curve_count = m.curves.count;
    write_lvalue(writer, curve_count);
    for_i32(entity_index,0,m.curves.count){
     write_binary(writer, &m.curves[entity_index]);
    }
   }
  }
  write_debug_string("EOF");
 }
 return writer->ok;
}
function void
write_binary_union(Writer *writer, Type_Info *type,
                   void *pointer, void *pvariant){
 kv_assert(type->kind == I_Type_Kind_Union);
 i32 variant = read_enum(*type->discriminator_type, pvariant);
 
 arrayof<I_Union_Member> &union_members = type->union_members;
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
write_binary_func(Writer *writer, Type_Info *type, void *void_pointer){
 u8 *pointer = cast(u8 *)void_pointer;
 switch(type->kind){
  case I_Type_Kind_Basic:{
   if(type->Basic_Type == Basic_Type_String){
    //-String
    String *string = (String *)pointer;
    write_lvalue(writer, string->count);
    write_size(writer, string->data, string->count);
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
  invalid_default_case;
 }
}
//-