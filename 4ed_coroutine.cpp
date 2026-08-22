/*
 * Mr. 4th Dimention - Allen Webster
 *
 * 19.07.2017
 *
 * Coroutine implementation from thread+mutex+cv
 *
 */

// TOP

function void
default_wait_for_condition_variable(System_Condition_Variable cv, System_Mutex mutex, i32 timeout_ms=100)
{
 DEBUG_profile_flush();
 
 system_condition_variable_wait(cv, mutex, timeout_ms);
}

struct Thread_Wrapper_Input
{
 Arena *arena;
 Thread_Function *proc;
 void *data;
 usize profile_buffer_size;
};
function void
thread_wrapper(void *input0)
{
 Thread_Wrapper_Input *input = (Thread_Wrapper_Input *)input0;
 
 DEBUG_profile_init_thread(input->arena, input->profile_buffer_size);
 
 input->proc(input->data);
 
 DEBUG_profile_quit_thread();
 
 arena_free(input->arena);
}
function System_Thread
thread_launch(Thread_Function *proc, void *data, usize profile_buffer_size=MB(16))
{
 Arena arena0 = make_arena();
 Arena *arena = push_struct(&arena0, Arena);
 *arena = arena0;
 
 Thread_Wrapper_Input *input = push_struct0(arena, Thread_Wrapper_Input);
 input->arena = arena;
 input->proc = proc;
 input->data = data;
 input->profile_buffer_size = profile_buffer_size;
 System_Thread result = system_thread_launch(thread_wrapper, input);
 return result;
}

function void
coroutine__pass_control(Coroutine *me, Coroutine *other,
                        Coroutine_State my_new_state, Coroutine_Pass_Control control)
{
 Assert(me->state == CoroutineState_Active);
 Assert(me->sys == other->sys);
 
 me->state = my_new_state;
 other->state = CoroutineState_Active;
 me->sys->active = other;
 system_condition_variable_signal(other->cv);
 if (control == CoroutinePassControl_BlockMe)
 {
  while(me->state != CoroutineState_Active)
  {
   default_wait_for_condition_variable(me->cv, me->sys->lock);
  }
 }
}

function void
coroutine_main(Coroutine *me)
{
 Thread_Context_Extra_Info tctx_info = {};
 tctx_info.coroutine = me;
 
 Thread_Context tctx_ = {};
 thread_context_init(&tctx_, ThreadKind_MainCoroutine,
                     &malloc_base_allocator, &malloc_base_allocator);
 tctx_.user_data = &tctx_info;
 me->tctx = &tctx_;
 
 // NOTE(allen): Init handshake
 Assert(me->state == CoroutineState_Dead);
 system_mutex_acquire(me->sys->lock);
 me->sys->did_init = true;
 system_condition_variable_signal(me->sys->init_cv);
 
 while(1)
 {
  // NOTE(allen): Wait until someone wakes us up.
  while(me->state != CoroutineState_Active)
  {
   default_wait_for_condition_variable(me->cv, me->sys->lock);
  }
  
  Assert(me->type != CoroutineType_Root);
  Assert(me->yield_ctx != 0);
  Assert(me->func != 0);
  
  me->func(me);
  
  // NOTE(allen): Wake up the caller and set this coroutine back to being dead.
  Coroutine *other = me->yield_ctx;
  Assert(other->state == CoroutineState_Waiting);
  
  coroutine__pass_control(me, other, CoroutineState_Dead, CoroutinePassControl_ExitMe);
  me->func = 0;
 }
}

function void
coroutine_sub_init(Coroutine *co, Coroutine_Group *sys, b32 automated)
{
 block_zero_struct(co);
 co->sys = sys;
 co->state = CoroutineState_Dead;
 co->type = CoroutineType_Sub;
 co->cv = system_condition_variable_make();
 co->automated = automated;
 sys->did_init = false;
 co->thread = thread_launch(to_thread_function(coroutine_main), co);
 
 // NOTE(kv) Wait until the coroutine is inited, because... of what?
 while(not sys->did_init)
 {
  default_wait_for_condition_variable(sys->init_cv, sys->lock);
 }
}

function void
coroutine_system_init(Coroutine_Group *sys){
 sys->arena = make_arena();
 
 Coroutine *root = &sys->root;
 
 sys->lock = system_mutex_make();
 sys->init_cv = system_condition_variable_make();
 sys->active = root;
 
 block_zero_struct(root);
 root->sys = sys;
 root->state = CoroutineState_Active;
 root->type = CoroutineType_Root;
 root->cv = system_condition_variable_make();
 
 sys->unused = 0;
 
 system_mutex_acquire(sys->lock);
}

function Coroutine*
coroutine_system_alloc(Coroutine_Group *sys, b32 automated)
{
 Coroutine *result = sys->unused;
 if (result != 0)
 {
  sll_stack_pop(sys->unused);
 }
 else
 {
  result = push_array(&sys->arena, Coroutine, 1);
  coroutine_sub_init(result, sys, automated);
 }
 result->next = 0;
 return(result);
}

function void
coroutine_system_free(Coroutine_Group *sys, Coroutine *coroutine){
 sll_stack_push(sys->unused, coroutine);
}

////////////////////////////////

function Coroutine*
coroutine_create(Coroutine_Group *coroutines, Coroutine_Function *func, b32 automated)
{
 Coroutine *result = coroutine_system_alloc(coroutines, automated);
 Assert(result->state == CoroutineState_Dead);
 result->func = func;
 return(result);
}

function Coroutine *
coroutine_run(Coroutine_Group *sys, Coroutine *other, Co_In *in, Co_Out *out)
{
 other->in = in;
 other->out = out;
 
 Coroutine *me = other->sys->active;
 Assert(me != 0);
 Assert(me->sys == other->sys);
 Assert(other->state == CoroutineState_Dead || other->state == CoroutineState_Inactive);
 other->yield_ctx = me;
 coroutine__pass_control(me, other, CoroutineState_Waiting, CoroutinePassControl_BlockMe);
 Assert(me == other->sys->active);
 
 Coroutine *result = other;
 if (other->state == CoroutineState_Dead)
 {
  coroutine_system_free(sys, other);
  result = 0;
 }
 return(result);
}

function void
coroutine_yield(Coroutine *me)
{
 Coroutine *other = me->yield_ctx;
 Assert(other != 0);
 Assert(me->sys == other->sys);
 Assert(other->state == CoroutineState_Waiting);
 coroutine__pass_control(me, other, CoroutineState_Inactive, CoroutinePassControl_BlockMe);
}

// BOTTOM

