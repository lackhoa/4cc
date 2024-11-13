//~NOTE(kv) Basic memory stuff like arenas
#ifndef KV_DEBUG_MEMORY
#  if KV_INTERNAL && !KV_H_IS_METAPROGRAM
#    define KV_DEBUG_MEMORY 1
#  else
#    define KV_DEBUG_MEMORY 0
#  endif
#endif

#if KV_DEBUG_MEMORY
#  define DEBUG_MEMORY_ONLY(...) __VA_ARGS__
#else
#  define DEBUG_MEMORY_ONLY
#endif

#if !KV_H_IS_METAPROGRAM  //NOTE You can't generate the code, if you're the generator
#  include "generated/kv_memory.gen.h"
#endif

#if KV_DEBUG_MEMORY
typedef File_Line DEBUG_MEMORY_File_Line;
#else
struct DEBUG_MEMORY_File_Line {};
#endif

#if KV_DEBUG_MEMORY
function DEBUG_MEMORY_File_Line
DEBUG_MEMORY_file_line(const char *file=__builtin_FILE(),
                       u32 line=__builtin_LINE())
{
 return {(char *)file,line};
}
#else
function DEBUG_MEMORY_File_Line
DEBUG_MEMORY_file_line()
{
 return {};
}
#endif

#define memory_file_line_defparams \
DEBUG_MEMORY_File_Line file_line=DEBUG_MEMORY_file_line()

//-
function u64
round_up_to_pow2(u64 pow2_value, u64 input){
 u64 mask = pow2_value - 1;
 u64 result = (input + mask) & (~mask);
 return result;
}
function void
round_up_to_pow2(u64 pow2_value, u64 *input){
 u64 mask = pow2_value - 1;
 *input = (*input + mask) & (~mask);
}
function u64
round_up_to_next_pow2(u64 value){
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
struct Push_Params{
 b32 zero;
 u32 alignment;
};
inline Push_Params
make_default_arena_push_params(){
 Push_Params result = {};
 result.zero = false;
 return result;
}
Push_Params default_push_params = make_default_arena_push_params();

inline Push_Params
push_zero(Push_Params params=default_push_params){
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
  Arena_Chunk *next;
 };
 usize pos;
 usize cap;
};
inline u8 *
arena_chunk_get_base(Arena_Chunk *chunk){
 return (u8 *)(chunk + 1);
}
#endif

struct Arena
{
#if ASAN_ON
 Arena_ASAN_Tracker *last_allocation;  //NOTE(kv) "last" chronologically
#else
 usize chunk_size;
 Arena_Chunk *last_chunk;
#endif
};
struct Temp_Memory{
 Arena *arena;
#if ASAN_ON
 Arena_ASAN_Tracker *saved_tracker;
#else
 Arena_Chunk *saved_chunk;
 u64          saved_pos;
#endif
};

#if ASAN_ON
//-Doing monkey business because we don't have these functions when ASAN is on
struct Arena_Chunk;
struct Arena_Chunk_Bin;
function Arena_Chunk* get_arena_chunk(usize size){ return 0; }
function void free_arena_chunk(Arena_Chunk *chunk){}
//
#else
struct Arena_Chunk_Bin{
 u8 *base;
 usize committed;
 usize reserved;
 Arena_Chunk *free_chunk;
 u32 committed_chunk_count;//NOTE(kv) redundant information
};
struct Arena_Chunk_Store{
 Arena_Chunk_Bin bins[20];
};

#if defined(KV_H_NO_GLOBAL_ARENA_CHUNK_STORE)
//NOTE(kv) These calls are dynamic so that arena chunks can be shared between DLLs.
//NOTE(kv) We use "global_decl" so that if you define the globals via a metaprogram,
//  you don't run into trouble.
global_decl Arena_Chunk *(*get_arena_chunk)(usize size);
global_decl void (*free_arena_chunk)(Arena_Chunk *chunk);
#else
thread_global Arena_Chunk_Store arena_chunk_store;

function void
arena_chunk_store_destroy(){
 Arena_Chunk_Store *store = &arena_chunk_store;
 for_i32(bin_index, 0, alen(store->bins)){
  Arena_Chunk_Bin *bin = &store->bins[bin_index];
  system_memory_free(bin->base);
 }
}
function Arena_Chunk_Bin *
get_arena_chunk_bin_by_size(usize size){
 i64 size_log = find_most_significant_set_bit(size);
 i64 bin_index = size_log - 12;
 Arena_Chunk_Store *store = &arena_chunk_store;
 kv_assert(bin_index >= 0 and
           bin_index < alen(store->bins));
 Arena_Chunk_Bin *bin = &store->bins[bin_index];
 return bin;
}
function Arena_Chunk *
get_arena_chunk(usize size){
 macro_clamp_min(size, KB(4));
 Arena_Chunk_Bin *bin = get_arena_chunk_bin_by_size(size);
 if(not bin->free_chunk){
  //-Allocate new chunks
  if(bin->base == 0){
   //-Reserve memory
   bin->base = system_memory_reserve(GB(4));
  }
  u8 *commit_base = bin->base + bin->committed;
  usize chunk_size_total = sizeof(Arena_Chunk) + size;
  //NOTE(kv) At least double the current amount amount
  usize commit_size = macro_max(bin->committed, chunk_size_total);
  system_memory_commit(commit_base, commit_size);
  bin->committed += commit_size;
  kv_assert(commit_size % chunk_size_total == 0);
  usize new_chunk_count = commit_size / chunk_size_total;
  bin->committed_chunk_count += (u32)new_chunk_count;
  for_inc(usize, chunk_index, 0, new_chunk_count){
   //-Initialize the new chunks
   Arena_Chunk *new_chunk = (Arena_Chunk *)(commit_base + chunk_size_total * chunk_index);
   new_chunk->cap = size;
   new_chunk->next = bin->free_chunk;
   bin->free_chunk = new_chunk;
  }
 }
 Arena_Chunk *chunk = bin->free_chunk;
 bin->free_chunk = chunk->next;
 chunk->next = 0;
 return chunk;
}
function void
free_arena_chunk(Arena_Chunk *chunk){
 chunk->pos = 0;
 Arena_Chunk_Bin *bin = get_arena_chunk_bin_by_size(chunk->cap);
 chunk->next = bin->free_chunk;
 bin->free_chunk = chunk;
}
#endif
#endif//-!ASAN_ON

function Arena
make_arena(u64 chunk_size=KB(4)){
 Arena arena = {};
#if !ASAN_ON
 arena.chunk_size = chunk_size;
#endif
 return(arena);
}
//-
#if !ASAN_ON
function u8 *
arena_chunk_push(Arena_Chunk *chunk, usize size, usize alignment,
                 DEBUG_MEMORY_File_Line file_line){
 u8 *result_mem = 0;
 if(chunk){
  u8 *chunk_base = (u8 *)(chunk+1);
  umm mem_umm = umm(chunk_base + chunk->pos);
  round_up_to_pow2(alignment, &mem_umm);
  u8 *aligned_mem = (u8 *)(mem_umm);
  usize new_pos = (aligned_mem + size) - chunk_base;
  if(new_pos <= chunk->cap){
   //-Has enough space left in chunk
   result_mem = aligned_mem;
   chunk->pos = new_pos;
#if KV_DEBUG_MEMORY
   DEBUG_arena_chunk_push(chunk, result_mem - chunk_base, size, file_line);
#endif
  }
 }
 return result_mem;
}
#endif
function u8 *
arena_push_inner(Arena *arena, usize size, usize alignment,
                 DEBUG_MEMORY_File_Line file_line)
{
 u8 *pos = 0;
 if(size > 0)
 {
  if(ASAN_ON){
   macro_clamp_min(alignment, 8);
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
   pos = (u8 *)(tracker + 1);
   ASAN_POISON_MEMORY_REGION(tracker, tracker_size);
  }
#else
  {//-Non-asan
   {//-First try (fast path)
    pos = arena_chunk_push(arena->last_chunk, size, alignment, file_line);
   }
   if(pos == 0)
   {//-Need new chunk (slow path)
    usize new_chunk_size;
    {
     macro_clamp_min(arena->chunk_size, KB(4));
     new_chunk_size = arena->chunk_size;
     arena->chunk_size *= 2;
     usize min_chunk_size = round_up_to_next_pow2(size + alignment);
     macro_clamp_min(new_chunk_size, min_chunk_size);
    }
    Arena_Chunk *chunk = get_arena_chunk(new_chunk_size);
    chunk->prev = arena->last_chunk;
    arena->last_chunk = chunk;
#if KV_DEBUG_MEMORY
    DEBUG_register_arena_chunk(chunk, file_line);
#endif
    pos = arena_chunk_push(chunk, size, alignment, file_line);
   }
  }
#endif
  
  kv_assert(pos);
  kv_assert((umm(pos) & align_mask) == 0);
 }
 return(pos);
}
function u8 *
arena_push(Arena *arena, usize size, usize alignment,
           Push_Params params=default_push_params,
           memory_file_line_defparams)
{
 u8 *result = arena_push_inner(arena, size, alignment, file_line);
 if(params.zero){
  block_zero(result, size);
 }
 return result;
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
  arena->chunk_size /= 2LL;
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
 if(to_pos == 0){
  //-Empty chunk -> free it!
  arena_free_last_chunk(arena);
 }else{
  kv_assert(to_pos <= to_chunk->pos);
  to_chunk->pos = to_pos;
#if KV_DEBUG_MEMORY
  DEBUG_arena_chunk_truncate(to_chunk);
#endif
 }
}
#endif

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
end_temp_memory(Temp_Memory temp){
 Arena *arena = temp.arena;
#if ASAN_ON
 arena_pop_to(arena, temp.saved_tracker);
#else
 arena_pop_to(arena, temp.saved_chunk, temp.saved_pos);
#endif
}
function void
arena_free(Arena *arena){
#if ASAN_ON
 Arena_ASAN_Tracker *null_tracker = 0;
 arena_pop_to(arena, null_tracker);
#else
 arena_pop_to(arena, 0, 0);
#endif
}
function void
arena_clear(Arena *arena){
 //NOTE(kv) This call is supposed to be more "light-weight" than arena-free.
#if ASAN_ON
 arena_free(arena);
#else
 arena_free(arena);
#endif
}
//-
function u8 *
linalloc_wrap_write(void *dest, void *src, usize size){
 block_copy(dest, src, size);
 return((u8 *)dest);
}
#define push_size(arena,size,alignment, ...) \
arena_push(arena, size, alignment, ##__VA_ARGS__)

#define push_array(arena,T,count,...) \
(T*)push_size(arena, sizeof(T)*(count), alignof(T), ##__VA_ARGS__)

#define push_struct(arena,T,...)         push_array(arena,T,1,##__VA_ARGS__)
#define push_array_zero(arena,T,count)   push_array(arena,T,count,push_zero())

#define push_copy(arena, size, source, alignment) \
linalloc_wrap_write(push_size(arena, size, alignment), source, size)

#define push_array_copy(arena,T,count,source) \
(T *)push_copy(arena, count*sizeof(T), source, alignof(T))

#define pop_array(arena,T,count)   arena_pop_size(arena, sizeof(T)*(count))

template<class T>
inline T *
push_value(Arena *arena, const T &value){
 T *pointer = push_struct(arena, T);
 *pointer = value;
 return pointer;
}
struct Temp_Memory_Block{
 Temp_Memory temp;
 //-
 Temp_Memory_Block(Temp_Memory temp){
  this->temp = temp;
 }
 Temp_Memory_Block(Arena *arena){
  this->temp = begin_temp_memory(arena);
 }
 ~Temp_Memory_Block(){
  end_temp_memory(this->temp);
 }
 void restore(void){
  end_temp_memory(this->temp);
 }
};
//-