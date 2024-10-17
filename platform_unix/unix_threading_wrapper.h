/*
 * Mr. 4th Dimention - Allen Webster
 *
 * 18.07.2017
 *
 * Linux threading wrapper
 *
 */

// TOP

#error IS THIS STILL REAL? (February 27th 2020)

#if !defined(MAC_THREADING_WRAPPER)
#define MAC_THREADING_WRAPPER

#define PLAT_THREAD_SIG(n) void* n(void *ptr)
typedef PLAT_THREAD_SIG(Thread_Function);

union Thread{
    pthread_t t;
    FixSize(THREAD_TYPE_SIZE);
};

union Mutex{
    pthread_mutex_t crit;
    FixSize(MUTEX_TYPE_SIZE);
};

union Condition_Variable{
    pthread_cond_t cv;
    FixSize(CONDITION_VARIABLE_TYPE_SIZE);
};

function void
system_init_and_launch_thread(Thread *t, Thread_Function *proc, void *ptr){
    pthread_create(&t->t, 0, proc, ptr);
}

function void
system_init_lock(Mutex *m){
    pthread_mutex_init(&m->crit, NULL);
}

function void
system_acquire_lock(Mutex *m){
    pthread_mutex_lock(&m->crit);
}

function void
system_release_lock(Mutex *m){
    pthread_mutex_unlock(&m->crit);
}

function void
system_init_cv(Condition_Variable *cv){
    pthread_cond_init(&cv->cv, NULL);
}

function void
system_wait_cv(Condition_Variable *cv, Mutex *m){
    pthread_cond_wait(&cv->cv, &m->crit);
}

function void
system_signal_cv(Condition_Variable *cv, Mutex *m){
    pthread_cond_signal(&cv->cv);
}

#endif

// BOTTOM

