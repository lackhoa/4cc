//~NOTE(kv) Basic memory stuff like arenas
#if KV_DEBUG_MEMORY
#  define DEBUG_MEMORY_ONLY(...) __VA_ARGS__
#else
#  define DEBUG_MEMORY_ONLY
#endif

//-
function u64
align_pow2(u64 pow2_value, u64 input){
 u64 mask = pow2_value - 1;
 u64 result = (input + mask) & (~mask);
 return result;
}
function void
align_pow2(u64 pow2_value, u64 *input)
{
 u64 mask = pow2_value - 1;
 *input = (*input + mask) & (~mask);
}
function u64
round_up_to_next_pow2(u64 value)
{
 u64 result = 1;
 if(value != 0){
  //NOTE(kv) In case it's already a power of two, knock it down.
  value -= 1;
  i64 most_significant_bit = find_most_significant_set_bit(value);
  //NOTE(kv) The result is one bit to the left of the most significant bit
  result = 1LL << (most_significant_bit + 1);
 }
 return result;
}
function u64
round_down_to_pow2(u64 pow2_value, u64 input){
 u64 mask = pow2_value - 1;
 u64 result = (input) & (~mask);
 return result;
}
//-
struct Push_Params { b32 zero; };
//NOTE(kv) I've tried to push zero by default, ultimately didn't like it.
//  There are more place where you don't need to zero than you think.
global Push_Params default_push_params = {.zero=false};

myinline Push_Params
push_zero(Push_Params params=default_push_params)
{
 params.zero = true;
 return params;
}

#if ASAN_ON
struct Arena_ASAN_Tracker{
 Arena_ASAN_Tracker *prev;
 usize size;
};
#else

struct Arena_Chunk{
 union{
  Arena_Chunk *prev;
  Arena_Chunk *next_free;
 };
 usize pos;
 usize size;
};
#endif

struct Arena
{
#if ASAN_ON
 Arena_ASAN_Tracker *last_allocation;  //NOTE(kv) "last" chronologically
#else
 usize default_chunk_size;
 Arena_Chunk *last_chunk;
#endif
};
struct Temp_Memory
{
 Arena *arena;
#if ASAN_ON
 Arena_ASAN_Tracker *saved_tracker;
#else
 Arena_Chunk *saved_chunk;
 u64          saved_pos;
#endif
};

#if !ASAN_ON
#if KV_GLOBAL_ARENA_CHUNK_STORE
struct Arena_Chunk_Bin{
 Arena_Chunk *first_free;
};
global const u32 ARENA_CHUNK_BIN_COUNT = 20;
//NOTE(kv) This is like a mini-arena.
struct Arena_Chunk_Store
{
 Arena_Chunk_Bin bins[ARENA_CHUNK_BIN_COUNT];
 u8 *base;
 usize pos;
 usize committed;
 usize total_free;  //NOTE(kv) Just a debug counter
 
 Ticket_Mutex mutex;
};
struct Thread_Arena_Chunk_Store
{
 Arena_Chunk_Bin bins[ARENA_CHUNK_BIN_COUNT];
 usize total_free;
 b32 registered;
};

global Arena_Chunk_Store global_arena_chunk_store;
thread_local Thread_Arena_Chunk_Store thread_arena_chunk_store;

function u32
get_arena_chunk_bin_index_by_size(usize size)
{
 u32 size_log = (u32)find_most_significant_set_bit(size);
 u32 bin_index = size_log - 12;
 kv_assert(bin_index < ARENA_CHUNK_BIN_COUNT);
 return bin_index;
}
function Arena_Chunk *
get_arena_chunk(usize size)
{
 Thread_Arena_Chunk_Store *local_store = &thread_arena_chunk_store;
 ClampBot(size, KB(4));
 u32 bin_index = get_arena_chunk_bin_index_by_size(size);
 Arena_Chunk_Bin *bin = local_store->bins + bin_index;
 Arena_Chunk *chunk = bin->first_free;
 if(chunk){
  //-In thread-local bin -> win!
  bin->first_free = chunk->next_free;
  chunk->next_free = 0;
  local_store->total_free -= size;
 }else{
  //-Not in thread-local bin
  Arena_Chunk_Store *store = &global_arena_chunk_store;
  //NOTE(kv) Fun: we could make a system that doesn't wait at all.
  acquire_ticket_mutex(&store->mutex);
  bin = store->bins + bin_index;
  chunk = bin->first_free;
  if(chunk){
   //-in global bin -> less great
   bin->first_free = chunk->next_free;
   chunk->next_free = 0;
   store->total_free -= size;
  }else{
   //-Gotta grab a new chunk from OS -> worst!
   usize chunk_pos = store->pos;
   if(store->base == 0){
    //NOTE Reserve memory from OS
    store->base = system_memory_reserve(GB(8));
   }
   usize chunk_size_total = sizeof(Arena_Chunk) + size;
   store->pos = chunk_pos + chunk_size_total;
   if(store->pos > store->committed){
    //NOTE Commit more memory
    u8 *commit_base = store->base + store->committed;
    usize commit_size = MB(128);
    ClampBot(commit_size, chunk_size_total);  //NOTE(kv) Actual constraint
    b32 commit_ok = system_memory_commit(commit_base, commit_size);
    // NOTE(kv) A failed commit (or running past the GB(8) reservation) used to go
    // unnoticed -- the caller then writes into uncommitted pages and AVs far away.
    kv_assert(commit_ok);
    store->committed += commit_size;
   }
   kv_assert(store->pos <= store->committed);
   {//NOTE Initialize the new chunk
    chunk = (Arena_Chunk *)(store->base + chunk_pos);
    chunk->size = size;
   }
  }
  release_ticket_mutex(&store->mutex);
 }
 return chunk;
}
function void
move_free_thread_local_chunks_to_global()
{
 Thread_Arena_Chunk_Store *store = &thread_arena_chunk_store;
 Arena_Chunk_Store *global_store = &global_arena_chunk_store;
 acquire_ticket_mutex(&global_store->mutex);
 for_u32(bin_index, 0, ARENA_CHUNK_BIN_COUNT){
  Arena_Chunk_Bin *bin = store->bins + bin_index;
  Arena_Chunk_Bin *global_bin = global_store->bins + bin_index;
  
  Arena_Chunk *last_free = 0;
  u32 free_count = 0;
  for(Arena_Chunk *chunk = bin->first_free;
      chunk;
      chunk = chunk->next_free)
  {
   last_free = chunk;
   free_count++;
  }
  
  if(last_free){
   last_free->next_free = global_bin->first_free;
   global_bin->first_free = bin->first_free;
   bin->first_free = 0;
   global_store->total_free += free_count * last_free->size;
  }
 }
 store->total_free = 0;
 release_ticket_mutex(&global_store->mutex);
}
function void
free_arena_chunk(Arena_Chunk *chunk){
 Thread_Arena_Chunk_Store *store = &thread_arena_chunk_store;
 {//-Free the chunk
  chunk->pos = 0;
  u32 bin_index = get_arena_chunk_bin_index_by_size(chunk->size);
  Arena_Chunk_Bin *bin = store->bins +  bin_index;
  chunk->next_free = bin->first_free;
  bin->first_free = chunk;
  store->total_free += chunk->size;
 }
 const usize thread_free_threshold = MB(16);
 if(store->total_free > thread_free_threshold){
  //-slow path: release chunks for other threads
  move_free_thread_local_chunks_to_global();
 }
}
#endif
#endif//-!ASAN_ON

function Arena
make_arena(u64 chunk_size=KB(4)){
 Arena arena = {};
#if !ASAN_ON
 arena.default_chunk_size = chunk_size;
#endif
 return(arena);
}
//-
#if KV_GLOBAL_ARENA_CHUNK_STORE

#if !ASAN_ON
function u8 *
arena_chunk_push(Arena_Chunk *chunk, usize size, usize alignment,
                 DEBUG_File_Line file_line)
{
 u8 *result_mem = 0;
 if(chunk){
  u8 *chunk_base = (u8 *)(chunk+1);
  umm mem_umm = umm(chunk_base + chunk->pos);
  align_pow2(alignment, &mem_umm);
  u8 *aligned_mem = (u8 *)(mem_umm);
  usize new_pos = (aligned_mem + size) - chunk_base;
  if(new_pos <= chunk->size){
   //-Has enough space left in chunk
   result_mem = aligned_mem;
   chunk->pos = new_pos;
   DEBUG_arena_chunk_push(chunk, result_mem - chunk_base, size, file_line);
  }
 }
 return result_mem;
}
#endif

function u8 *
arena_push_inner(Arena *arena, usize size, usize alignment,
                 DEBUG_File_Line file_line)
{
 u8 *result = 0;
 if(size > 0)
 {
  if(ASAN_ON){
   ClampBot(alignment, 8);
  }
  usize align_mask = alignment-1;
#if ASAN_ON
  {
   if(alignment > 8){
    //NOTE(kv) _aligned_malloc will help here
    todo_incomplete;
   }
   usize tracker_size = sizeof(Arena_ASAN_Tracker);
   Arena_ASAN_Tracker *tracker;
   cast_to(tracker, kv_malloc(tracker_size + size));
   tracker->prev = arena->last_allocation;
   tracker->size = size;
   arena->last_allocation = tracker;
   result = (u8 *)(tracker + 1);
   ASAN_POISON_MEMORY_REGION(tracker, tracker_size);
  }
#else
  {//-Non-asan
   {//-First try (fast path)
    result = arena_chunk_push(arena->last_chunk, size, alignment, file_line);
   }
   if(result == 0)
   {//-Need new chunk (slow path)
    usize new_chunk_size;
    {
     if(arena->last_chunk){
      new_chunk_size = arena->last_chunk->size << 1;
     }else{
      new_chunk_size = arena->default_chunk_size;
      ClampBot(new_chunk_size, KB(4));
     }
     
     usize min_chunk_size = size + alignment;
     if(new_chunk_size < min_chunk_size){
      new_chunk_size = round_up_to_next_pow2(min_chunk_size);
     }
    }
    Arena_Chunk *chunk = get_arena_chunk(new_chunk_size);
    chunk->prev = arena->last_chunk;
    arena->last_chunk = chunk;
    DEBUG_register_arena_chunk(chunk, file_line);
    result = arena_chunk_push(chunk, size, alignment, file_line);
   }
  }
#endif
  
  kv_assert(result);
  kv_assert((umm(result) & align_mask) == 0);
 }
 return(result);
}
function void
arena_pop_size(Arena *arena, usize size)
{
 if(size > 0)
 {
#if ASAN_ON
  {
   usize last_allocation_size = 0;
   Arena_ASAN_Tracker *tracker = arena->last_allocation;
   {
    ASAN_UNPOISON_MEMORY_REGION(tracker, sizeof(*tracker));
    last_allocation_size = tracker->size;
    ASAN_POISON_MEMORY_REGION(tracker, sizeof(*tracker));
   }
   kv_assert(size <= last_allocation_size);
   u8 *usable_base = (u8 *)(tracker + 1);
   ASAN_POISON_MEMORY_REGION(usable_base + last_allocation_size - size, size);
  }
#else
  {
   Arena_Chunk *chunk = arena->last_chunk;
   kv_assert(size <= chunk->pos);
   chunk->pos -= size;
#if KV_DEBUG_MEMORY
   DEBUG_arena_chunk_truncate(chunk);
#endif
  }
#endif
 }
}
#if ASAN_ON
inline void
arena_pop_to(Arena *arena, Arena_ASAN_Tracker *to_tracker){
 while(true){
  Arena_ASAN_Tracker *tracker = arena->last_allocation;
  if(tracker == to_tracker){
   break;
  }else{
   ASAN_UNPOISON_MEMORY_REGION(tracker, sizeof(*tracker));
   arena->last_allocation = tracker->prev;
   kv_free(tracker);
  }
 }
}
#else //-asan off
function void
arena_free_last_chunk(Arena *arena)
{
 Arena_Chunk *chunk = arena->last_chunk;
 if(chunk){
#if KV_DEBUG_MEMORY
  DEBUG_free_arena_chunk(chunk);
#endif
  
  //NOTE(kv) Should have kept arena->chunk_size constant,
  //  then view the chunk size from the last chunk. Oh well...
  arena->last_chunk = chunk->prev;
  free_arena_chunk(chunk);
 }
}
inline void
arena_pop_to(Arena *arena, Arena_Chunk *to_chunk, umm to_pos)
{
 while(true){
  Arena_Chunk *chunk = arena->last_chunk;
  if(chunk == to_chunk){
   break;
  }else{
   arena_free_last_chunk(arena);
  }
 }
 
 kv_assert(to_chunk == arena->last_chunk);
 if(to_chunk)
 {//NOTE Truncate last chunk
  kv_assert(to_pos <= to_chunk->pos);
  to_chunk->pos = to_pos;
#if KV_DEBUG_MEMORY
  DEBUG_arena_chunk_truncate(to_chunk);
#endif
 }
}
#endif

#endif

#if !KV_GLOBAL_ARENA_CHUNK_STORE
#  define X(N) global_decl wrap_function_pointer(N);
memory_functions_xlist(X);
#  undef X
#endif

myinline Push_Params
push_default()
{
 return default_push_params;
}
function u8 *
arena_push(Arena *arena, usize size, usize alignment=8,
           Push_Params params=default_push_params,
           DEBUG_file_line_defparams)
{
 u8 *result = arena_push_inner(arena, size, alignment, file_line);
 if(params.zero){
  block_zero(result, size);
 }
 return result;
}
function Temp_Memory
begin_temp_memory(Arena *arena)
{
 Temp_Memory temp = {};
 temp.arena  = arena;
#if ASAN_ON
 temp.saved_tracker = arena->last_allocation;
#else
 Arena_Chunk *last_chunk = arena->last_chunk;
 temp.saved_chunk = last_chunk;
 if(last_chunk){
  temp.saved_pos = last_chunk->pos;
 }
#endif
 return(temp);
}
function void
end_temp_memory(Temp_Memory temp)
{
 Arena *arena = temp.arena;
#if ASAN_ON
 arena_pop_to(arena, temp.saved_tracker);
#else
 arena_pop_to(arena, temp.saved_chunk, temp.saved_pos);
#endif
}
function void
arena_free(Arena *arena)
{
#if ASAN_ON
 Arena_ASAN_Tracker *null_tracker = 0;
 arena_pop_to(arena, null_tracker);
#else
 arena_pop_to(arena, 0, 0);
#endif
}
function void
arena_clear(Arena *arena)
{// NOTE(kv) This call is supposed to be more "light-weight" than arena-free.
 // TODO(kv) Implement: it'd be cool if the arena can remember its size.
#if ASAN_ON
 arena_free(arena);
#else
 // NOTE(kv) We don't want to free the last chunk.
 arena_pop_to(arena, arena->last_chunk, 0);
#endif
}
//-
function u8 *
linalloc_wrap_write(void *dest, void *src, usize size){
 block_copy(dest, src, size);
 return((u8 *)dest);
}
#define push_size arena_push
#define push_size_zero(arena, size) \
arena_push(arena, size, 8, push_zero())

#define push_array(arena,T,count,...) \
(T*)arena_push(arena, sizeof(T)*(count), alignof(T), ##__VA_ARGS__)

#define push_struct(arena,T,...)   push_array(arena, T, 1, ##__VA_ARGS__)

#define push_struct0(arena, T)      push_struct(arena, T, push_zero())
#define push_array0(arena,T,count)  push_array(arena, T, count, push_zero())
#define push_array_zero             push_array0

#define push_copy(arena, size, source, alignment) \
linalloc_wrap_write(push_size(arena, size, alignment), source, size)

#define push_array_copy(arena,T,count,source) \
(T *)push_copy(arena, count*sizeof(T), source, alignof(T))

#define pop_array(arena,T,count)   arena_pop_size(arena, sizeof(T)*(count))

//TODO(kv) I'm so suspicious of anything template-related...
//  but maybe with inlining, it's fine? Gah!
template<class T>
myinline T *
push_value(Arena *arena, const T &value){
 T *pointer = push_struct(arena, T);
 *pointer = value;
 return pointer;
}

struct Temp_Memory_Block
{
 // IMPORTANT(kv) I don't use this mechanism anymore when arenas are so cheap.
 // Just make a new arena, then call clear at the start/end of the loop.
 // That way we don't accidentally push onto an arena while in a temp block.
 // Yeah, if you call "break" out of the loop, then we "leak" some memory, big whoops!
 // That arena should be a temp arena anyway.
 Temp_Memory temp;
 //-
 myinline Temp_Memory_Block(Temp_Memory temp){
  this->temp = temp;
 }
 myinline Temp_Memory_Block(Arena *arena){
  this->temp = begin_temp_memory(arena);
 }
 myinline ~Temp_Memory_Block(){
  end_temp_memory(this->temp);
 }
 myinline void restore(void){
  end_temp_memory(this->temp);
 }
};

// NOTE(kv) We use precompiled header,
// which would create another object file and will bark multiply defined symbol.
// I guess duplicating permanent arenas are cool,
// Since it's not the end of the world if we don't deduplicate them?
global thread_local Arena thread_permanent_arena;
//-