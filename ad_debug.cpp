#if KV_DEBUG_MEMORY

function i32
debug_get_thread_index()
{
 if(debug_thread_index == -1){
  Debug_State *debug = &memory_debug_state;
  debug_thread_index = atomic_add_u32(&debug->thread_count, 1);
  debug->chunk_stores[debug_thread_index] = &thread_arena_chunk_store;
 }
 return debug_thread_index;
}
function void
wrap_u32_around_u32(u32 *input, u32 wrap_value){
 if(*input >= wrap_value){
  *input -= wrap_value;
 }
}
function void
push_debug_event(Debug_Event *event)
{//NOTE(kv) Could make this function more efficeint by not passing in a value,.
 //  but it's too tedious and error prone.
 Debug_State *debug = &memory_debug_state;
 u32 index = debug->event_index_to_write;
 while(true){
  //TODO(kv) Only need an atomic add here, if we only store the modulo value.
  //  or we could do a double-buffer thing.
  u32 new_index_to_write = index + 1;
  wrap_u32_around_u32(&new_index_to_write, DEBUG_EVENT_ARRAY_COUNT);
  u32 actual = atomic_compare_exchange_u32(&debug->event_index_to_write,
                                           new_index_to_write,
                                           index);
  if(actual == index){
   break;
  }else{
   index = actual;
  }
 }
 
 //NOTE(kv) Check we're not out of space in the ring buffer
 kv_assert(debug->event_index_to_read != debug->event_index_to_write);
 
 i32 thread_index = debug_get_thread_index();
 event->thread_index = thread_index;
 Debug_Event *result = debug->events + index;
 *result = *event;
 
 CompletePreviousWritesBeforeFutureWrites;
 debug->event_index_written[thread_index] = index;
}

//~NOTE(kv) Memory debug system
function Debug_Arena *
debug_get_arena_by_last_chunk(Debug_State *debug, void *chunk)
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
debug_register_arena_chunk(void *chunk_address, usize size, void *chunk_prev,
                           File_Line file_line)
{
 Debug_State *debug = &memory_debug_state;
 
 Debug_Arena_Chunk *shadow_chunk = debug->first_free_chunk;
 if(shadow_chunk){
  debug->first_free_chunk = shadow_chunk->next_free;
 }else{
  shadow_chunk = push_struct(&debug->arena, Debug_Arena_Chunk);
 }
 *shadow_chunk = {};
 shadow_chunk->real_chunk = chunk_address;
 shadow_chunk->size = size;
 
 Debug_Arena *shadow_arena = 0;
 if(chunk_prev){
  //-Existing arena
  shadow_arena = debug_get_arena_by_last_chunk(debug, chunk_prev);
 }else{
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
}
function void
debug_free_arena_chunk(void *chunk)
{//NOTE(kv) Called before "chunk" is actually freed.
 Debug_State *debug = &memory_debug_state;
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
}
//-
function i32
compare_wrapped_u32(u32 a, u32 b, u32 relative, u32 wrap_value)
{
 if(a < relative){ a += wrap_value; }
 if(b < relative){ b += wrap_value; }
 i32 result = (i32)a - (i32)b;
 return result;
}
function void
debug_collate_events()
{
 Debug_State *debug = &memory_debug_state;
 u32 read_begin = debug->event_index_to_read;
 u32 read_end = debug->event_index_to_write;
 
 u32 total_events_read = 0;
 for(u32 event_index = read_begin;
     event_index != read_end;
     event_index++)
 {//-Processing each event
  wrap_u32_around_u32(&event_index, DEBUG_EVENT_ARRAY_COUNT);
  Debug_Event *event = debug->events + event_index;
  u32 written = debug->event_index_written[event->thread_index];
  if(compare_wrapped_u32(event_index, written, read_begin, DEBUG_EVENT_ARRAY_COUNT) > 0){
   //NOTE We're not done writing to this event yet! Halt early!
   read_end = event_index;
   break;
  }else{
   switch(event->type){
    case Debug_Event_Register_Arena_Chunk:{
     debug_register_arena_chunk(event->chunk_address, event->chunk_size, event->chunk_prev,
                                event->file_line);
    }break;
    case Debug_Event_Free_Arena_Chunk:{
     debug_free_arena_chunk(event->chunk_address);
    }
   }
   total_events_read++;
  }
 }
 debug->event_index_to_read = read_end;
 ImGui::Text("total events: %u", total_events_read);
}
//-
function void
DEBUG_register_arena_chunk(Arena_Chunk *chunk, File_Line file_line)
{
 Debug_Event event;
 event.type = Debug_Event_Register_Arena_Chunk;
 event.file_line = file_line;
 event.chunk_address = chunk;
 event.chunk_size = chunk->size;
 event.chunk_prev = chunk->prev;
 push_debug_event(&event);
}
function void
DEBUG_free_arena_chunk(Arena_Chunk *chunk)
{
 Debug_Event event;
 event.type = Debug_Event_Free_Arena_Chunk;
 event.chunk_address = chunk;
 push_debug_event(&event);
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
//-
function usize
debug_arena_get_size(Debug_Arena *arena)
{
 usize size = 0;
 for(Debug_Arena_Chunk *chunk = arena->last_chunk;
     chunk;
     chunk = chunk->prev)
 {
  size += chunk->size;
 }
 return size;
}
function void
debug_render_gui()
{
 struct Arena_Entry{
  Debug_Arena *arena;
  usize size;
 };
 
 Debug_State *debug = &memory_debug_state;
 Arena_Chunk_Store *global_store = &global_arena_chunk_store;
 Scratch_Block scratch;
 
 if(0)
 {
  const i32 input_count = 16;
  f32 test_random[input_count] = {
   79, 87, 100, 66,
   69,7,88,6,
   33,18,80,53,
   31,14,70,17
  };
  Sort_Entry *input = push_array(scratch, Sort_Entry, alen(test_random));
  for_i32(index, 0, alen(test_random)){
   input[index].index = index;
   input[index].key = test_random[index];
  }
  
  i32 output_count = 4;
  Sort_Entry *output = push_array(scratch, Sort_Entry, output_count);
  small_insertion_sort(input, input_count, output, output_count);
 }
 
 i32 arena_count = 0;
 for(Debug_Arena *arena = debug->first_arena;
     arena;
     arena = arena->next)
 {
  arena_count++;
 }
 
 usize total_used = 0;
 Sort_Entry *arena_sort_entries = push_array(scratch, Sort_Entry, arena_count);
 Arena_Entry *arena_entries = push_array(scratch, Arena_Entry, arena_count);
 {
  i32 arena_index = 0;
  for(Debug_Arena *arena = debug->first_arena;
      arena;
      arena = arena->next)
  {//-Gathering arena info into an array
   usize arena_size = debug_arena_get_size(arena);
   total_used += arena_size;
   
   {
    Sort_Entry *entry = arena_sort_entries + arena_index;
    entry->index = arena_index;
    entry->key   = -(f32)arena_size;
   }
   
   {
    Arena_Entry *entry = arena_entries + arena_index;
    entry->arena = arena;
    entry->size = arena_size;
   }
   
   arena_index++;
  }
 }
 
 usize global_free = 0;
 for_u32(bin_index, 0, ARENA_CHUNK_BIN_COUNT)
 {
  Arena_Chunk_Bin *bin = &global_store->bins[bin_index];
  for(Arena_Chunk *chunk = bin->first_free;
      chunk;
      chunk = chunk->next_free)
  {
   global_free += chunk->size;
  }
 }
 
 ImGui::Text("arena count: %d", arena_count);
 
 {//-top arenas
  Sort_Entry top_arenas[8];
  small_insertion_sort(arena_sort_entries, arena_count,
                       top_arenas, alen(top_arenas));
  for_i32(top_arena_index, 0, alen(top_arenas))
  {
   i32 entry_index = top_arenas[top_arena_index].index;
   Arena_Entry *entry = arena_entries + entry_index;
   String arena_filename = empty_string;
   if(not global_dll_reloaded_so_watch_out_for_debug_strings){
    //TODO(kv) Well, we store filenames in persistent debug arenas,
    //  so those also need to be copied somewhere if we want stuff to work.
    //arena_filename = path_filename(SCu8(entry->arena->file_line.file));
   }
   ImGui::Text("arena %S:%d: size: %_$I64u",
               arena_filename,
               entry->arena->file_line.line,
               entry->size);
  }
 }
 ImGui::Text("total committed / used / free: %_$I64u / %_$I64u / %_$I64u",
             global_store->committed, total_used, global_store->total_free);
 for_u32(store_index, 0, debug->thread_count)
 {
  Thread_Arena_Chunk_Store *store = debug->chunk_stores[store_index];
  ImGui::Text("thread free: %_$I64u", store->total_free);
 }
}
function void
DEBUG_end_frame()
{
 bool window_open = true;
 ImGui::Begin("debug", &window_open, ImGuiViewportFlags_NoFocusOnAppearing);
 
 debug_collate_events();
 debug_render_gui();
 
 ImGui::End();
 //TODO(kv) WHY does the "NoFocusOnAppearing" flag not work?
 ImGui::SetWindowFocus(0);
}
#endif//-KV_DEBUG_MEMORY
//-