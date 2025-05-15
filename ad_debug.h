#if KV_DEBUG_MEMORY
// NOTE(kv) Just like in hmh, arenas can be moved around,
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
 void *real_chunk;
 Debug_Allocation *last_allocation;
 usize size;
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
enum Debug_Event_Type
{
 Debug_Event_None,
 Debug_Event_Register_Arena_Chunk,
 Debug_Event_Free_Arena_Chunk,
};
struct Debug_Event
{
 Debug_Event_Type type;
 u16 thread_index;
 File_Line file_line;
 
 void *chunk_address;
 usize chunk_size;
 void *chunk_prev;
};
#define DEBUG_EVENT_ARRAY_COUNT 65536
#define MAX_THREAD_COUNT 16
struct Debug_State
{
 Arena arena;
 Ticket_Mutex mutex;
 
 u32 thread_count;
 Thread_Arena_Chunk_Store *chunk_stores[MAX_THREAD_COUNT];
 u32 event_index_written[MAX_THREAD_COUNT];
 
 volatile u32 event_index_to_write;
 volatile u32 event_index_to_read;
 Debug_Event events[DEBUG_EVENT_ARRAY_COUNT];
 
 Debug_Arena *first_arena;
 Debug_Arena *first_free_arena;
 Debug_Arena_Chunk *first_free_chunk;
 Debug_Allocation *first_free_allocation;
 
 i32 total_events_read;
};
global Debug_State global_debug_state;

global thread_local i32 debug_thread_index = -1;
//-KV_DEBUG_MEMORY
#endif

#if !KV_DEBUG_MEMORY
#  define DEBUG_end_frame(...)
#endif

function void debug_end_frame();
//-