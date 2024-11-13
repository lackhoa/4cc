//-NOTE Generated: C:/Users/vodan/4ed/code/generated/kv_memory.gen.h
meta_table(name, return, params) debug_memory_functions
{
 register_arena_chunk void `(Arena_Chunk *chunk, File_Line file_line),
 free_arena_chunk     void `(Arena_Chunk *chunk),
 arena_chunk_push     void `(Arena_Chunk *chunk, usize pos_in_chunk, usize size, File_Line file_line),
 arena_chunk_truncate void `(Arena_Chunk *chunk),
}

gen_for(debug_memory_functions)
{
#define DEBUG_`(name)__return `(return)
#define DEBUG_`(name)__params `(params)
}
//-