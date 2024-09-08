/*
 * 4coder malloc base allocator
 */

// TOP

function void *
base_reserve__system(void *user_data, u64 size, u64 *usable_size, String8 location)
{
 const u64 extra_size = 128;
 u64 allocated_size = round_up_u64(size + extra_size, KB(4));
 *usable_size = allocated_size - extra_size;
 
 void *allocated_ptr = system_memory_allocate(allocated_size, location);
 *(u64*)allocated_ptr = size;
 
 return ((u8*)allocated_ptr + extra_size);
}

function void
base_free__system(void *user_data, void *ptr)
{
 const u64 extra_size = 128;
 ptr = (u8*)ptr - extra_size;
 u64 allocated_size = *(u64*)ptr;
 system_memory_free(ptr, allocated_size);
}

function Base_Allocator
make_base_allocator_system(void)
{
 return(make_base_allocator_generic(base_reserve__system, 0, 0, base_free__system, 0, 0));
}

global Base_Allocator base_allocator_system = {};

function Base_Allocator*
get_base_allocator_system(void)
{
 if (base_allocator_system.type == 0)
 {
  base_allocator_system = make_base_allocator_system();
 }
 return(&base_allocator_system);
}

function Arena
make_arena_system(u64 chunk_size=KB(16))
{
 return(make_arena(get_base_allocator_system(), chunk_size));
}

// BOTTOM

