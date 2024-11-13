#if KV_INTERNAL
#  define KV_INTERNAL_ONLY(...) __VA_ARGS__
#else
#  define KV_INTERNAL_ONLY(...)
#endif

#if KV_INTERNAL
typedef File_Line DEBUG_File_Line;
#else
struct DEBUG_File_Line {};
#endif
//-
struct Arena_Chunk;

#if !defined(KV_H_NO_GLOBAL_ARENA_CHUNK_STORE)

api(ed) function void
DEBUG_register_arena_chunk(Arena_Chunk *chunk, File_Line file_line);

api(ed) function void
DEBUG_free_arena_chunk(Arena_Chunk *chunk);

api(ed) function void
DEBUG_arena_chunk_push(Arena_Chunk *chunk, usize pos_in_chunk, usize size,
                       File_Line file_line);

api(ed) function void
DEBUG_arena_chunk_truncate(Arena_Chunk *chunk);

#else//-
//TODO(kv) Is this another layer of API?
global_decl void
(*DEBUG_register_arena_chunk)(Arena_Chunk *chunk, File_Line file_line);

global_decl void
(*DEBUG_free_arena_chunk)(Arena_Chunk *chunk);

global_decl void
(*DEBUG_arena_chunk_push)(Arena_Chunk *chunk, usize pos_in_chunk, usize size,
                          File_Line file_line);

global_decl void
(*DEBUG_arena_chunk_truncate)(Arena_Chunk *chunk);

#endif
//-