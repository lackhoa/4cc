#include "time.h"
function void
write_binary_func(Writer *writer, Type_Info *type, void *void_pointer);

#define write_binary(writer, POINTER) \
write_binary_func(writer, type_info_from_pointer(POINTER), POINTER)
//-