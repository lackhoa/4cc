#include "time.h"
//-
struct Writer{
 b32 ok;
 FILE *file;
};
function Writer
make_writer(FILE *file){
 Writer result = {.ok = true};
 result.file = file;
 return result;
}
inline void
write_size(Writer *writer, void *data, usize size){
 if(writer->ok){
  usize result = fwrite(data, size, 1, writer->file);
  writer->ok = result != 0;
 }
}
//-
function void
write_binary_func(Writer *writer, Type_Info *type, void *void_pointer);

#define write_lvalue(writer, lvalue) \
write_size(writer, &lvalue, sizeof(lvalue))

#define write_binary(writer, POINTER) \
write_binary_func(writer, &type_info_from_pointer(POINTER), POINTER)

//-