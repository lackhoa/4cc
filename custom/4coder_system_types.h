/*
 * 4coder_system_types.h - Types relating to the system api.
 */

// TOP

#pragma once

typedef i1 Key_Mode;
enum{
 Key_Mode_LanguageArranged,
 Key_Mode_Physical,
};

struct Plat_Handle{
 u32 d[4];
};
typedef Plat_Handle System_Library;
typedef Plat_Handle System_Thread;
typedef Plat_Handle System_Mutex;
typedef Plat_Handle System_Condition_Variable;
struct Coroutine;
typedef void Thread_Function(void *ptr);
struct CLI_Handles{
 Plat_Handle proc;
 Plat_Handle out_read;
 Plat_Handle out_write;
 Plat_Handle in_read;
 Plat_Handle in_write;
 u32 scratch_space[4];
 i1 exit;
};

struct Memory_Annotation_Node{
 Memory_Annotation_Node *next;
 String location;
 void *address;
 u64 size;
};

struct Memory_Annotation{
 Memory_Annotation_Node *first;
 Memory_Annotation_Node *last;
 i1 count;
};
struct Mutex_Lock{
 Mutex_Lock(System_Mutex mutex);
 ~Mutex_Lock();
 operator System_Mutex();
 System_Mutex mutex;
};
api(custom)
typedef u32 File_Attribute_Flag;
enum{
 FileAttribute_IsDirectory = 1,
};

api(custom)
struct File_Attributes{
 u64 size;
 u64 last_write_time;
 File_Attribute_Flag flags;
};

api(custom)
struct File_Info{
 File_Info *next;
 String filename;
 File_Attributes attributes;
};

api(custom)
struct File_List{
 File_Info **infos;
 u32 count;
};

typedef i1 System_Path_Code;
enum
{
 SystemPath_CurrentDirectory,
 SystemPath_BinaryDirectory,
 SystemPath_UserDirectory,
};
// BOTTOM
