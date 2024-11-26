//-NOTE Generated: C:/Users/vodan/4ed/code/generated/kv_memory.gen.h
meta_table(return, name, params) memory_functions
{
 `(u8 *) arena_push_inner `(Arena *arena, usize size, usize alignment, DEBUG_File_Line file_line),
 void    arena_pop_size   `(Arena *arena, usize size),
 void    arena_pop_to     `(Arena *arena, Arena_Chunk *to_chunk, umm to_pos),
}
meta_table(return, name, params) debug_memory_functions
{
 void DEBUG_register_arena_chunk `(Arena_Chunk *chunk, File_Line file_line),
 void DEBUG_free_arena_chunk     `(Arena_Chunk *chunk),
 void DEBUG_arena_chunk_push     `(Arena_Chunk *chunk, usize pos_in_chunk, usize size, File_Line file_line),
 void DEBUG_arena_chunk_truncate `(Arena_Chunk *chunk),
}

gen_file "kv_memory.gen.h"
{
 gen_for(memory_functions)
 {
#define `(name)__return `return
#define `(name)__params `params
 }
 gen_for(debug_memory_functions)
 {
#define `(name)__return `return
#define `(name)__params `params
 }
 //-
#define memory_functions_xlist(X) \
gen_for(memory_functions)
 {
  X(`name) \
 }
 
#define debug_memory_functions_xlist(X) \
gen_for(debug_memory_functions)
 {
  X(`name) \
 }
}
//-