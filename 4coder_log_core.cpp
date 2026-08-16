/*
 * Mr. 4th Dimention - Allen Webster
 *
 * 14.08.2019
 *
 * Core logging implementation. 
 *
 */

// NOTE(kv) The vision for this system is that it's fast (faster than print_message).
//  has minimal dependencies, and thread-safe.
//  Is it there yet? Nope, but that's the idea.

global Log_State global_log = {};

function void
output_file_append(Thread_Context *tctx, Models *models, Editing_File *file, String value, b32 automated);

function void
log_init(void)
{
 global_log.mutex         = system_mutex_make();
 global_log.message_arena = make_arena();
 init_dynamic(global_log.spam_list, &thread_permanent_arena, 32);
}

function b32
log_enabled()
{
 i32 thread_id = system_thread_get_id();
 return global_log.disabled_thread_id != thread_id;
}
function void
log_string_push(String str)
{// TODO(kv) omg mutex!
 Arena *msg_arena = &global_log.message_arena;
 string_list_push(msg_arena, &global_log.list, push_stringz(msg_arena, str));
}
function void
log_string_core(String string)
{
 if(log_enabled())
 {
  system_mutex_acquire(global_log.mutex);
  log_string_push(string);
  system_mutex_release(global_log.mutex);
 }
}
//-
function void
log_string_spam(String string)
{
 if(log_enabled())
 {
  system_mutex_acquire(global_log.mutex);
  u64 hash = gb_murmur64(string.str, string.size);
  
  Log_Spam_Entry *spam_entry = 0;
  {
   sarray(Log_Spam_Entry) spam_list = global_log.spam_list;
   for_i32(i, 0, spam_list.count)
   {// NOTE(kv) Not gonna be many entries here.
    Log_Spam_Entry *test = spam_list.items + i;
    if(test->hash == hash)
    {
     spam_entry = test;
     break;
    }
   }
  }
  
  if(not spam_entry)
  {//NOTE new entry
   darray(Log_Spam_Entry) *spam_list = &global_log.spam_list;
   if(spam_list->count >= 128){
    // TODO(kv) Have a least-spammy entry eviction thing here.
    spam_list->count = 0;
   }
   
   spam_entry = push_zero(spam_list);
   *spam_entry = {};
  }
  
  spam_entry->hash = hash;
  spam_entry->count += 1;
  
  b32 should_print = is_power_of_2(u32(spam_entry->count));
  if(should_print)
  {
   Scratch_Scope tmp; // TODO(kv) something very unsatisfying about these strings...
   string = push_stringf(tmp, "%S (x%d)", string, spam_entry->count);
   log_string_push(string);
  }
  system_mutex_release(global_log.mutex);
 }
}
//-
function b32
log_flush(Thread_Context *tctx, Models *models)
{
 b32 result = false;
 
 system_mutex_acquire(global_log.mutex);
 global_log.disabled_thread_id = system_thread_get_id();
 
 if(global_log.list.total_size > 0)
 {
  String separator = strlit("\n");
  String text = string_list_flatten(&global_log.message_arena, global_log.list, 0,
                                    separator, StringSeparator_AfterLast,
                                    StringFill_NoTerminate);
  output_file_append(tctx, models, models->log_buffer, text, /*automated*/true);
  {// NOTE(kv) Tee the log to <exe_dir>/debug/log_<pid>.txt so it's readable from
   // outside the editor (tail -f, Claude). One file per instance (pid suffix),
   // truncated at startup.
   local_persist FILE *log_file = 0;
   local_persist b32 open_attempted = false;
   if(not open_attempted)
   {
    open_attempted = true;
    Scratch_Scope scratch;
    String binary_dir = system_get_path(scratch, SystemPath_BinaryDirectory);
    char dir_path[MAX_PATH];
    snprintf(dir_path, sizeof(dir_path), "%.*s/debug",
             (i32)binary_dir.size, binary_dir.str);
    CreateDirectoryA(dir_path, 0);
    {// NOTE(kv) Sweep stale logs from previous runs. A live instance holds its log
     // open (CRT denies delete-sharing), so DeleteFile fails on it and we skip it.
     char pattern[MAX_PATH];
     snprintf(pattern, sizeof(pattern), "%s/log_*.txt", dir_path);
     WIN32_FIND_DATAA find_data;
     HANDLE find_handle = FindFirstFileA(pattern, &find_data);
     if(find_handle != INVALID_HANDLE_VALUE)
     {
      do{
       char stale_path[MAX_PATH];
       snprintf(stale_path, sizeof(stale_path), "%s/%s", dir_path, find_data.cFileName);
       DeleteFileA(stale_path);
      }while(FindNextFileA(find_handle, &find_data));
      FindClose(find_handle);
     }
    }
    char file_path[MAX_PATH];
    snprintf(file_path, sizeof(file_path), "%s/log_%lu.txt",
             dir_path, GetCurrentProcessId());
    log_file = fopen(file_path, "wb");
   }
   if(log_file)
   {
    fwrite(text.str, 1, text.size, log_file);
    fflush(log_file);
   }
  }
  result = true;
 }
 arena_free(&global_log.message_arena);
 block_zero_struct(&global_log.list);
 
 global_log.disabled_thread_id = 0;
 system_mutex_release(global_log.mutex);
 
 return(result);
}
// BOTTOM
