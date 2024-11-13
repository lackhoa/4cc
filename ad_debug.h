//NOTE(kv) Just like in hmh, arenas can be moved around,
//  only the arena chunks are stable. So we rely on them to identify arenas.
struct Debug_Allocation
{
 Debug_Allocation *prev;
};
struct Debug_Arena_Chunk
{
 union{
  Debug_Arena_Chunk *prev;
  Debug_Arena_Chunk *next_free;
 };
 struct Arena_Chunk *real_chunk;
 Debug_Allocation *last_allocation;
};
struct Debug_Arena
{
 union{
  Debug_Arena *next;
  Debug_Arena *next_free;
 };
 File_Line file_line;
 Debug_Arena_Chunk *last_chunk;
};
struct Debug_State
{
 /*Debug_Event *events;
 u32 event_count;
 u32 event_cap;*/
 Arena arena;
 System_Mutex mutex;
 //
 Debug_Arena *first_arena;
 Debug_Arena *first_free_arena;
 Debug_Arena_Chunk *first_free_chunk;
 Debug_Allocation *first_free_allocation;
};
global Debug_State memory_debug_state;
//-