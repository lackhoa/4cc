#if KV_DEBUG_MEMORY
//~NOTE(kv) Memory debug system
//TODO(kv) We need an actual event system!.
function Debug_Arena *
debug_get_arena_by_last_chunk(Debug_State *debug, Arena_Chunk *chunk)
{
 Debug_Arena *result = 0;
 if(chunk)
 {
  for(Debug_Arena *test = debug->first_arena;
      test;
      test = test->next)
  {
   if(test->last_chunk->real_chunk == chunk){
    result = test;
    break;
   }
  }
 }
 return result;
}
function void
DEBUG_register_arena_chunk(Arena_Chunk *chunk, File_Line file_line)
{
 Debug_State *debug = &memory_debug_state;
 system_mutex_acquire(debug->mutex);
 
 Debug_Arena_Chunk *shadow_chunk = debug->first_free_chunk;
 if(shadow_chunk){
  debug->first_free_chunk = shadow_chunk->next_free;
 }else{
  shadow_chunk = push_struct(&debug->arena, Debug_Arena_Chunk);
 }
 *shadow_chunk = {};
 shadow_chunk->real_chunk = chunk;
 
 Debug_Arena *shadow_arena = 0;
 if(chunk->prev)
 {//-Existing arena
  shadow_arena = debug_get_arena_by_last_chunk(debug, chunk->prev);
 } else {
  //-First chunk in a new arena
  shadow_arena = debug->first_free_arena;
  if(shadow_arena){
   debug->first_free_arena = shadow_arena->next_free;
  }else{
   shadow_arena = push_struct(&debug->arena, Debug_Arena);
  }
  *shadow_arena = {};
  //NOTE(kv) You can give name to arena, but temporary arenas don't have names.
  //  So we record the file+line, in case we somehow leak those temp arenas.
  shadow_arena->file_line = file_line;
  {//-add to arena list
   shadow_arena->next = debug->first_arena;
   debug->first_arena = shadow_arena;
  }
 }
 shadow_chunk->prev = shadow_arena->last_chunk;
 shadow_arena->last_chunk = shadow_chunk;
 system_mutex_release(debug->mutex);
}
function void
DEBUG_free_arena_chunk(Arena_Chunk *chunk)
{//NOTE(kv) Called before "chunk" is actually freed.
 Debug_State *debug = &memory_debug_state;
 system_mutex_acquire(debug->mutex);
 Debug_Arena *shadow_arena = debug_get_arena_by_last_chunk(debug, chunk);
 Debug_Arena_Chunk *shadow_chunk = shadow_arena->last_chunk;
 shadow_arena->last_chunk = shadow_chunk->prev;
 if(shadow_arena->last_chunk == 0){
  //-Free the arena
  Debug_Arena *arena = shadow_arena;
  if(debug->first_arena == arena){
   debug->first_arena = arena->next;
  }else{
   for(Debug_Arena *scan = debug->first_arena;
       scan;
       scan = scan->next)
   {//NOTE(kv) Arenas usually are created and destroyed like stacks,
    //  so this scan might not be that bad.
    if(scan->next == arena){
     scan->next = arena->next;
     break;
    }
   }
  }
  
  arena->next_free = debug->first_free_arena;
  debug->first_free_arena = arena;
 }
 shadow_chunk->next_free = debug->first_free_chunk;
 debug->first_free_chunk = shadow_chunk;
 system_mutex_release(debug->mutex);
}
function void
DEBUG_arena_chunk_push(Arena_Chunk *chunk, usize pos_in_chunk, usize size,
                       File_Line file_line)
{
}
function void
DEBUG_arena_chunk_truncate(Arena_Chunk *chunk)
{
}
#endif
//-