/*
 * Mr. 4th Dimention - Allen Webster
 *
 * 03.08.2019
 *
 * Coroutine implementation from thread+mutex+cv
 *
 */

// TOP

#if !defined(FRED_COROUTINE_H)
#define FRED_COROUTINE_H

typedef void Coroutine_Function(struct Coroutine *head);

typedef u32 Coroutine_State;
enum{
    CoroutineState_Dead,
    CoroutineState_Active,
    CoroutineState_Inactive,
    CoroutineState_Waiting,
};

typedef u32 Coroutine_Type;
enum{
 CoroutineType_Uninitialized,
 CoroutineType_Root,
 CoroutineType_Sub,
};

typedef i1 Co_Request;
enum{
 CoRequest_None = 0,
 CoRequest_NewFontFace = 1,
 CoRequest_ModifyFace = 2,
 CoRequest_AcquireGlobalFrameMutex = 3,
 CoRequest_ReleaseGlobalFrameMutex = 4,
};
struct Co_In
{
 union{
  struct{
   struct Models *models;
   Custom_Command_Function *event_context_base;
  };
  User_Input user_input;
  Face_ID face_id;
  b32 success;
 };
};
struct Co_Out{
 Co_Request request;
 Face_Description *face_description;
 Face_ID face_id;
};
struct Coroutine
{
 Coroutine *next;
 Thread_Context *tctx;
 Co_In *in;
 Co_Out *out;
 System_Thread thread;
 System_Condition_Variable cv;
 struct Coroutine_Group *sys;
 Coroutine_Function *func;
 Coroutine *yield_ctx;
 Coroutine_State state;
 Coroutine_Type type;
 void *user_data;
 b32 automated;
};
myinline Thread_Function *
to_thread_function(Coroutine_Function *func)
{
 return (Thread_Function*)func;
}

struct Coroutine_Group{
    Arena arena;
    System_Mutex lock;
    System_Condition_Variable init_cv;
    b32 did_init;
    Coroutine *active;
    Coroutine *unused;
    Coroutine root;
};

////////////////////////////////

typedef i1 Coroutine_Pass_Control;
enum{
    CoroutinePassControl_ExitMe,
    CoroutinePassControl_BlockMe,
};

#endif

// BOTTOM
