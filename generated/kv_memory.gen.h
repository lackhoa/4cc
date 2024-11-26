//NOTE File created programmatically by C:\Users\vodan\4ed\code\meta_template.cpp:313:
//NOTE Source template: C:\Users\vodan\4ed\code\kv_memory.kt

 #define arena_push_inner__return u8 *
#define arena_push_inner__params Arena *arena, usize size, usize alignment, DEBUG_File_Line file_line
 #define arena_pop_size__return void
#define arena_pop_size__params Arena *arena, usize size
 #define arena_pop_to__return void
#define arena_pop_to__params Arena *arena, Arena_Chunk *to_chunk, umm to_pos
 
 #define DEBUG_register_arena_chunk__return void
#define DEBUG_register_arena_chunk__params Arena_Chunk *chunk, File_Line file_line
 #define DEBUG_free_arena_chunk__return void
#define DEBUG_free_arena_chunk__params Arena_Chunk *chunk
 #define DEBUG_arena_chunk_push__return void
#define DEBUG_arena_chunk_push__params Arena_Chunk *chunk, usize pos_in_chunk, usize size, File_Line file_line
 #define DEBUG_arena_chunk_truncate__return void
#define DEBUG_arena_chunk_truncate__params Arena_Chunk *chunk
 
 
#define memory_functions_xlist(X) \
  X(arena_push_inner) \
   X(arena_pop_size) \
   X(arena_pop_to) \
 
 
#define debug_memory_functions_xlist(X) \
  X(DEBUG_register_arena_chunk) \
   X(DEBUG_free_arena_chunk) \
   X(DEBUG_arena_chunk_push) \
   X(DEBUG_arena_chunk_truncate) \
 
