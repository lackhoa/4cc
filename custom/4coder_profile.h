/*
 * 4coder_profile.cpp - Built in self profiling report.
 */

// TOP

#if !defined(FCODER_PROFILE_H)
#define FCODER_PROFILE_H

struct Profile_Global_List{
    System_Mutex mutex;
    Arena node_arena;
    Arena_Node *first_arena;
    Arena_Node *last_arena;
    Profile_Thread *first_thread;
    Profile_Thread *last_thread;
    i1 thread_count;
    Profile_Enable_Flag disable_bits;
};

struct Profile_Block{
    Thread_Context *tctx;
    Profile_Global_List *list;
    b32 is_closed;
    Profile_ID id;
    
    Profile_Block(Thread_Context *tctx, Profile_Global_List *list,
                  String name, String location);
    Profile_Block(App *app, String name, String location);
    ~Profile_Block();
    void close_now();
};

struct Profile_Scope_Block{
    Thread_Context *tctx;
    Profile_Global_List *list;
    b32 is_closed;
    Profile_ID id;
    
    Profile_Scope_Block(Thread_Context *tctx, Profile_Global_List *list,
                        String name, String location);
    Profile_Scope_Block(App *app, String name, String location);
    ~Profile_Scope_Block();
    void close_now();
};

#endif

// BOTTOM

