//~ Static array, C style
struct Static_Array
{
 u32 count;
 u32 cap;
};

#define make_array(arena, T, cap) \
(T *)array__make(arena, sizeof(T), cap);

#define array_push(array)          (array__push(array), array_lastp(array))

#define array_push_value(array, value) \
array__push(array); \
*array_lastp(array) = (value)

#define array_getp(array, index) \
(array__check_index(array, index), array+index)

#define array_get(array, index) \
(array__check_index(array, index), array[index])

//NOTE(kv) The array is the items pointer, not the header.
//  Because we need a way to let the brain-dead C compiler
//  know the item type.
#define array_lastp(array)    array_getp(array, array_count(array) - 1)
//NOTE(kv) Resist the temptation to put a space in between "array_last" and "(array)"...
//  The C preprocessor can slow-burn in a fire.
#define array_last(array)     array_get(array, array_count(array) - 1)

//-
myinline Static_Array *
array__header(void *items)
{
 return (Static_Array *)(items) - 1;
}
myinline void *
items_from_header(Static_Array *header)
{
 return (void *)(header + 1);
}

myinline u32
array_count(void *items)
{
 Static_Array *header = array__header(items);
 return header->count;
}
function void
array_set_count(void *items, u32 count)
{
 Static_Array *header = array__header(items);
 kv_assert(count <= header->cap);
 header->count = count;
}
myinline void
array__check_index(void *array, u32 index)
{
 kv_assert(index < array_count(array));
}
function void *
array__make(Arena *arena, usize item_size, usize cap)
{
 usize total_size = sizeof(Static_Array) + item_size*cap;
 Static_Array *header = (Static_Array *)push_size(arena, total_size);
 *header = {};
 header->cap = u32(cap);
 
 void *items = items_from_header(header);
 return items;
}
function void
array__push(void *items)
{
 Static_Array *header = array__header(items);
 header->count++;
 kv_assert(header->count <= header->cap);
}
function void
array_pop(void *items)
{
 Static_Array *header = array__header(items);
 kv_assert(header->count > 0);
 header->count--;
}
//-