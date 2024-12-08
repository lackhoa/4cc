#if KV_INTERNAL
#  define KV_INTERNAL_ONLY(...) __VA_ARGS__
#else
#  define KV_INTERNAL_ONLY(...)
#endif

#if KV_INTERNAL
typedef File_Line DEBUG_File_Line;
myinline DEBUG_File_Line
DEBUG_file_line(const char *file=__builtin_FILE(),
                u32 line=__builtin_LINE())
{
 return {(char *)file, line};
}
#else
struct DEBUG_File_Line {};
inline DEBUG_File_Line DEBUG_file_line() { return {}; }
#endif

#define DEBUG_file_line_defparams \
DEBUG_File_Line file_line=DEBUG_file_line()

//-
#ifndef KV_DEBUG_MEMORY
#  if KV_INTERNAL && !KV_H_IS_METAPROGRAM
#    define KV_DEBUG_MEMORY 1
#  else
#    define KV_DEBUG_MEMORY 0
#  endif
#endif
//-
struct Arena_Chunk;
struct Arena_Chunk_Store;
struct Thread_Arena_Chunk_Store;

#if KV_DEBUG_MEMORY
#  if KV_GLOBAL_ARENA_CHUNK_STORE
#define X(NAME) function wrap_function(NAME);
debug_memory_functions_xlist(X);
#undef X
#  endif
#endif

//NOTE(kv) This code is used by the meta-generator,
//  so it can't be generated -> suck on that, me!
#if !KV_DEBUG_MEMORY
#  define DEBUG_arena_chunk_push(...)
#  define DEBUG_register_arena_chunk(...)
#  define DEBUG_free_arena_chunk(...)
#  define DEBUG_arena_chunk_truncate(...)
#endif
//-