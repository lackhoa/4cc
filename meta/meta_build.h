//
global b32 BUILD_USE_SUSPICIOUS_FEATURES = 1;

enum Compiler
{
 Compiler_ClangCl,
 Compiler_MSVC,
};
struct Define_Symbol
{
 String name;
 u32    value;
};
struct Build_Params
{
 darray(String) input_files;
 String output;
 
 // NOTE Compiler
 darray(String) compiler_args;
 darray(String) includes;
 darray(Define_Symbol) define_symbols;
 
 darray(String) linker_args;
 
 b32 compile_only;
 b32 link_only;
 b32 optimized;
 b32 no_debug_info;
 b32 no_warnings;
 b32 no_force_inline;
};
global String clang_warnings;
global String msvc_warnings;

function Stringz
join_strings(Arena *arena, String separator, String *strings, u32 count)
{
 Stringz result = {};
 if(count > 0)
 {
  //note(kv) Compute the count
  result.count = 0;
  for_u32(i, 0, count){
   result.count += strings[i].count;
  }
  result.count += (count-1) * separator.count;
  
  //note(kv) Printing things
  usize pushed_size = result.count+1;
  result.str = push_size(arena, pushed_size);
  u8 *at = result.str;
  for_u32(string_index, 0, count)
  {
   if(string_index != 0){
    block_copy(at, separator.str, separator.count);
    at += separator.count;
   }
   
   String string = strings[string_index];
   block_copy(at, string.str, string.count);
   at += string.count;
  }
  *at++ = 0; // note nil terminator
  kv_assert(at == result.str + pushed_size);
 }
 return result;
}
//~
struct Work_Queue;

struct Thread_Info
{
 i32 thread_index;
 Work_Queue *work_queue;
};

typedef b32 Work_Function(Thread_Info info, void *arg);

struct Work_Queue_Entry
{
 Work_Function *func;
 void *arg;
 b32   ok;
};
struct Work_Queue
{
 Work_Queue_Entry entries[128];
 volatile i32 next_index_to_do;
 volatile i32 push_index;
 volatile i32 completion_count;
 HANDLE semaphore;
};
//~
struct Build_Shared
{
 b32 release_editor;
 sarray(String) imgui_TUs;
 String libs_dir;
 String imgui_dir;
 b32 asan_on;
 b32 driver_enabled;
 b32 notebook_mode;
 b32 no_force_inline;
};
//