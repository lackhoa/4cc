/*
 * 4coder system base allocator
 */

// TOP

function void *
base_reserve__system(void *user_data, u64 wanted_size, u64 *usable_size, String8 location){
 //NOTE(kv) We used to put the size at the front in the this call,
 //  But the OS layer should already do that, so let's just use that instead.
 void *allocated_ptr = system_memory_allocate_at_least(wanted_size, location, usable_size);
 return allocated_ptr;
}
function void
base_free__system(void *user_data, void *ptr){
 system_memory_free(ptr);
}

function Base_Allocator
make_base_allocator_system(void){
 return(make_base_allocator_generic(base_reserve__system, 0, 0, base_free__system, 0, 0));
}

global Base_Allocator base_allocator_system = {};

function Base_Allocator *
get_base_allocator_system(void){
 if(base_allocator_system.type == 0){
  base_allocator_system = make_base_allocator_system();
 }
 return(&base_allocator_system);
}
//NOTE(kv) "chunk_size" is pretty silly because
//  it depends on the operating system anyway!
function Arena
make_arena_system(u64 chunk_size=KB(32)){
 return(make_arena(get_base_allocator_system(), chunk_size));
}
// BOTTOM
