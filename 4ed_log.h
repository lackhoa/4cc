/*
 * Mr. 4th Dimention - Allen Webster
 *
 * 14.08.2019
 *
 * Core logging structures.
 *
 */

// TOP

#if !defined(FRED_LOG_H)
#define FRED_LOG_H

struct Log{
    System_Mutex mutex;
    Arena arena;
    List_String list;
    volatile i1 disabled_thread_id;
    b32 stdout_log_enabled;
};

#endif

// BOTTOM