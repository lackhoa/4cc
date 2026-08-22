/*
 * 4coder_profile.cpp - Built in self profiling report.
 */

// TOP

function void
profile_init(Profile_Global_List *list)
{
 list->mutex = system_mutex_make();
 list->disable_bits = ProfileEnable_UserBit;
}

function Profile_Thread *
prof__get_thread(Profile_Global_List *list, i32 thread_id)
{
 Profile_Thread *result = 0;
 for(Profile_Thread *node = list->first_thread;
     node != 0;
     node = node->next)
 {
  if(thread_id == node->thread_id)
  {
   result = node;
   break;
  }
 }
 
 if(result == 0)
 {// NOTE new thread
  result = push_array0(&list->node_arena, Profile_Thread, 1);
  sll_queue_push(list->first_thread, list->last_thread, result);
  list->thread_count += 1;
  result->thread_id = thread_id;
 }
 return(result);
}

function void
profile_clear(Profile_Global_List *list)
{
 Mutex_Lock lock(list->mutex);
 arena_free(&list->node_arena);
 list->first_thread = 0;
 list->last_thread = 0;
 list->thread_count = 0;
}

function void
profile_thread_flush(Thread_Context *tctx, Profile_Global_List *list)
{
 if(tctx->prof_record_count > 0)
 {
  Mutex_Lock lock(list->mutex);
  if (list->disable_bits == 0)
  {
   Profile_Thread* thread = prof__get_thread(list, system_thread_get_id());
   
   tctx->prof_arena = make_arena(KB(16));
   
   if(tctx->prof_first != 0)
   {
    if (thread->first_record == 0)
    {
     thread->first_record = tctx->prof_first;
     thread->last_record = tctx->prof_last;
    }
    else
    {
     thread->last_record->next = tctx->prof_first;
     thread->last_record = tctx->prof_last;
    }
    thread->record_count += tctx->prof_record_count;
   }
  }
  else
  {
   arena_free(&tctx->prof_arena);
  }
  tctx->prof_record_count = 0;
  tctx->prof_first = 0;
  tctx->prof_last = 0;
 }
}

function void
profile_thread_set_name(Thread_Context *tctx, Profile_Global_List *list, String8 name)
{
 Mutex_Lock lock(list->mutex);
 Profile_Thread* thread = prof__get_thread(list, system_thread_get_id());
 thread->name = name;
}

#define ProfileThreadName(tctx,list,name) profile_thread_set_name((tctx), (list), (name))

function void
profile_set_enabled(Profile_Global_List *list, b32 value, Profile_Enable_Flag flag)
{
 Mutex_Lock lock(list->mutex);
 if(value) { RemFlag(list->disable_bits, flag); }
 else      { AddFlag(list->disable_bits, flag); }
}

function void
thread_profile_record__inner(Thread_Context *tctx, Profile_ID id, u64 time,
                             String name, String location)
{
 Profile_Record *record = push_array0(&tctx->prof_arena, Profile_Record, 1);
 sll_queue_push(tctx->prof_first, tctx->prof_last, record);
 tctx->prof_record_count += 1;
 record->id = id;
 record->time = time;
 record->location = location;
 record->name = name;
}

function Profile_ID
thread_profile_record_push(Thread_Context *tctx, u64 time,
                           String name, String location)
{
 Profile_ID id = tctx->prof_id_counter;
 tctx->prof_id_counter += 1;
 thread_profile_record__inner(tctx, id, time, name, location);
 return(id);
}
function void
thread_profile_record_pop(Thread_Context *tctx, u64 time, Profile_ID id)
{
 Assert(tctx->prof_id_counter > 1);
 tctx->prof_id_counter = id;
 thread_profile_record__inner(tctx, id, time, empty_string, empty_string);
}

function Profile_ID
thread_profile_record_push(App *app, u64 time,
                           String name, String location)
{
 Thread_Context *tctx = get_thread_context(app);
 return(thread_profile_record_push(tctx, time, name, location));
}
function void
thread_profile_record_pop(App *app, u64 time, Profile_ID id)
{
 Thread_Context *tctx = get_thread_context(app);
 thread_profile_record_pop(tctx, time, id);
}

////////////////////////////////

member_function
Profile_Block::Profile_Block(String name, String location)
{
 zero_struct(this);
 
 ProfileBegin2(name);  // TODO(kv) Add more info here
}

function void
profile_block_end(Profile_Block *block)
{
 if(not block->ended)
 {
  block->ended = 1;
  ProfileEnd();
 }
}

member_function
Profile_Block::~Profile_Block()
{
 profile_block_end(this);
}
// BOTTOM
