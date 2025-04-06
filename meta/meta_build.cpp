//-
// NOTE(kv) The only way to build your code is with more code.
//-
Build_Shared build_shared;

function b32
is_c_or_cpp_file(String path)
{
 String extension = path_extension(path);
 return(extension == strlit("c") or
        extension == strlit("cpp"));
}
function sarray(Stringz)
list_all_cpp_files_top_level(Arena *arena, Stringz root_path)
{
 Scratch_Block scratch;
 darray(Stringz) result;
 init_dynamic(result, scratch);
 list_files_in_dir(arena, &result, root_path, No_Recursive, is_c_or_cpp_file);
 return result;
}
function b32
create_lock_file(Stringz name)
{
 FILE *lock = open_or_create_file(name, "w");
 b32 ok = lock != 0;
 close_file(lock);
 return ok;
}
//-
function void
add_define_symbol(Build_Params &params, String key, u32 value)
{
 Define_Symbol symbol = {key, value};
 push(&params.define_symbols, symbol);
}
myinline void
add_define_symbol(Build_Params &params, char *key, u32 value)
{
 add_define_symbol(params, SCu8(key), value);
}
myinline void
add_include(Build_Params &params, String path)
{
 push(&params.includes, path);
}
myinline void
add_input_file(Build_Params &params, String path)
{
 push(&params.input_files, path);
}
myinline void
add_linker_arg(Build_Params &params, String arg)
{
 push(&params.linker_args, arg);
}
function b32
chdir(Stringz dirname)
{
 b32 result = SetCurrentDirectoryA(to_cstring(dirname));
 return result;
}
function void
print_cmd(Printer &printer, String string)
{
 print(printer, " ");
 print(printer, string);
}
//-
function b32
run_command(darray(String) command)
{
 Scratch_Scope scratch;
 
 //NOTE(kv) Monkey business: why is it so hard to use this freaking API?
 push_first(&command, String(strlit("/C")));
 Stringz arguments = join_strings(scratch, strlit(" "), command.items, command.count);
 myprintf("Running command: %S", string_skip(arguments, 3));
 myprintf("\n");  //note(kv) when I combien this with the previous print it doesn't work?
 
 STARTUPINFO startup_info = {};
 startup_info.cb = sizeof(startup_info);
 PROCESS_INFORMATION process_info = {};
 Stringz cmd_exe = strlit("c:\\windows\\system32\\cmd.exe");
 f64 time_start = gb_time_now();
 b32 ok = CreateProcessA(to_cstring(cmd_exe),
                         to_cstring(arguments),
                         0, 0, FALSE, 0, 0, 0,
                         &startup_info,
                         &process_info);
 kv_assert(ok);  // NOTE IDK how to handle this, dude! Why does it even fail?
 
 HANDLE process_handle = process_info.hProcess;
 
 u32 wait_result = WaitForSingleObject(process_handle, INFINITE);
 ok = ok and (wait_result == WAIT_OBJECT_0);
 f64 time_elapsed = gb_time_now() - time_start;
 myprintf("Time elapsed: %lf seconds\n", time_elapsed);
 
 DWORD exit_code;
 ok = ok and GetExitCodeProcess(process_handle, &exit_code);
 ok = ok and (exit_code == 0);
 
 CloseHandle(process_handle);
 return ok;
}
function b32
run_compiler(Compiler compiler, Build_Params params)
{//NOTE(kv) Assume that we're running in the correct build directory.
 Scratch_Block tmp;
 darray(String) cmd;
 init_dynamic(cmd, tmp, 32);
 
 b32 is_msvc  = compiler == Compiler_MSVC;
 b32 is_clang = compiler == Compiler_ClangCl;
 kv_assert(is_msvc or is_clang);
 b32 has_debug_info = not params.no_debug_info;
 b32 do_compile     = not params.link_only;
 b32 do_link        = not params.compile_only;
 b32 has_warnings   = not params.no_warnings;
 
 String compiler_name = strlit("cl");
 if(is_clang){
  compiler_name = strlit("clang-cl");
 }
 push(&cmd, compiler_name);
 
 if(params.compile_only){
  push(&cmd, strlit("-c"));
 }
 
 for_i32(i, 0, params.input_files.count){
  push(&cmd, params.input_files[i]);
 }
 
 if(do_compile)
 {//-Compiler
  if(params.compile_only){
   if(params.output.count){
    if(is_msvc){
     push(&cmd, strlit("-Fo:"));
    }
    if(is_clang){
     push(&cmd, strlit("-o"));
    }
    push(&cmd, params.output);
   }
  }
  
  if(params.optimized){
   push(&cmd, strlit("-O2"));
  }else{
   //note not optimized
   push(&cmd, strlit("-Od"));
   //NOTE(kv) Force inline so debugging will be easier.
   if(not build_shared.no_force_inline and
      not params.no_force_inline)
   {
    push(&cmd, strlit("-Ob1"));
   }
  }
  
  if(has_debug_info)
  {
   // NOTE(kv) I've done -Zi for the longest time, but idk Casey does -Z7.
   push(&cmd, strlit("-Z7"));
   // NOTE(kv) If we do -Zi, MSVC might choke because we build multiple things at the same time?
   //push(&cmd, strlit("-FS"));
  }
  
  for_i32(i, 0, params.compiler_args.count){
   String flag = params.compiler_args.items[i];
   push(&cmd, flag);
  }
  
  for_i32(i, 0, params.define_symbols.count){
   Define_Symbol symbol = params.define_symbols.items[i];
   push(&cmd, push_stringf(tmp, "-D%S=%u", symbol.name, symbol.value));
  }
  
  for_i32(i, 0, params.includes.count){
   String include = params.includes.items[i];
   push(&cmd, push_stringf(tmp, "-I%S", include));
  }
  
  push(&cmd, strlit("-std:c++20 -D_CRT_SECURE_NO_WARNINGS -FC"));
  if(is_msvc){
   push(&cmd, strlit("-Zc:strictStrings- -nologo"));
  }
  
  if(has_warnings){
   push(&cmd, strlit("-W4 -WX"));
   if(is_clang){ push(&cmd, clang_warnings); }
   if(is_msvc) { push(&cmd, msvc_warnings); }
  }
 }
 
 if(do_link)
 {//-Linker
  push(&cmd, strlit("-link"));
  
  if(params.output.count){
   String str = strcat(tmp, strlit("-OUT:"), params.output);
   push(&cmd, str);
  }
  if(has_debug_info){
   push(&cmd, strlit("-DEBUG"));
  }
  for_i32(i, 0, params.linker_args.count){
   push(&cmd, params.linker_args.items[i]);
  }
 }
 
 b32 ok = run_command(cmd);
 return ok;
}
function Build_Params
build_parameters()
{
 Build_Params params = {};
 //NOTE(kv) We don't initialize arrays for now,
 //  since it's annoying and moslty pointless anyway.
 return params;
}

//~Work queue
function b32
maybe_do_work(Thread_Info info)
{
 b32 did_work = false;
 Work_Queue *queue = info.work_queue;
 i32 work_index = queue->next_index_to_do;
 if(work_index < queue->push_index)
 {
  did_work = true;
  i32 actual = atomic_compare_exchange_i32(&queue->next_index_to_do, work_index+1, work_index);
  if(actual == work_index)
  {//NOTE We've done all the work required to do the work!
   Work_Queue_Entry *work = &queue->entries[work_index];
   work->ok = work->func(info, work->arg);
   
   atomic_add_i32(&queue->completion_count, 1);
  }
 }
 return did_work;
}
function DWORD
thread_loop(void *param)
{
 Thread_Info info = *(Thread_Info *)param;
 Work_Queue *queue = info.work_queue;
 
 while(true)
 {
  b32 did_work = maybe_do_work(info);
  if(not did_work){
   WaitForSingleObject(queue->semaphore, INFINITE);
  }
 }
 
 return 0;
}

function void
push_work(Work_Queue *queue, Work_Function func, void *arg)
{//TODO(kv) Support multiple producer!!!
 //  otw worker threads can't spin off more work.
 i32 index = queue->push_index;
 kv_assert(index < alen(queue->entries));
 Work_Queue_Entry *work = queue->entries + index;
 work->func = func;
 work->arg  = arg;
 
 CompletePastWritesBeforeFutureWrites;
 queue->push_index += 1;
 ReleaseSemaphore(queue->semaphore, 1, 0);
}
//~
function b32
print_number_function(Thread_Info info, void *arg)
{
 i32 number = *(int *)arg;
 myprintf("[%d] The number is: %d\n", info.thread_index, number);
 return true;
}
function b32
build_editor(Thread_Info info, void *arg)
{
 Scratch_Block tmp;
 b32 ok = true;
 Build_Shared s = build_shared;
 Meta_Directories dirs = meta.dirs;
 Stringz FCODER_ROOT = pjoin(tmp, meta.dirs.home, strlit("4ed"));
 Stringz NON_SOURCE = pjoin(tmp, FCODER_ROOT, strlit("4coder-non-source"));
 
 String binary_stem = s.release_editor ? strlit("4ed_stable") : strlit("4ed");
 Stringz binary_name = strcat(tmp, binary_stem, strlit(".exe"));
 Stringz bkp_name = strcat(tmp, binary_stem, strlit(".bkp.exe"));
 
 if(s.release_editor)
 {// NOTE backup
  ok = ok and move_file(binary_name, bkp_name);
 }
 Build_Params params = build_parameters();
 
 add_input_file(params, pjoin(tmp, dirs.code, strlit("platform_win32/win32_4ed.cpp")));
 add_include(params, dirs.code);
 add_include(params, s.libs_dir);
 add_include(params, s.imgui_dir);
 add_include(params, pjoin(tmp, NON_SOURCE, strlit("foreign/freetype2")));
 
 String WINDOWS_LIBS = strlit("user32.lib winmm.lib gdi32.lib comdlg32.lib userenv.lib");
 String FREETYPE_LIB = pjoin(tmp, NON_SOURCE, strlit("foreign/x64/freetype.lib"));
 //For MacOS: LINKED_LIBS=f"{NON_SOURCE}/foreign/x64/libfreetype-mac.a -framework Cocoa -framework QuartzCore -framework CoreServices -framework OpenGL -framework IOKit -framework Metal -framework MetalKit"
 add_linker_arg(params, WINDOWS_LIBS);
 add_linker_arg(params, FREETYPE_LIB);
 add_linker_arg(params, strlit("opengl32.lib"));
 add_linker_arg(params, pjoin(tmp, NON_SOURCE, strlit("res/icon.res")));
 //NOTE(kv) incremental linking doesn't really help?
 
 params.output = binary_name;
 params.optimized = s.release_editor;
 add_define_symbol(params, "KV_INTERNAL", not s.release_editor);
 
 String imgui_backend_TUs[] = {
  strlit("imgui_impl_win32"),
  strlit("imgui_impl_opengl3"),
 };
 for_i32(i, 0, s.imgui_TUs.count)
 {// NOTE Compile imgui
  add_input_file(params, push_stringf(tmp, "%S.obj", s.imgui_TUs.items[i]));
 }
 for_i32(i, 0, alen(imgui_backend_TUs))
 {
  add_input_file(params, push_stringf(tmp, "%S.obj", imgui_backend_TUs[i]));
 }
 
 Compiler compiler = Compiler_MSVC;  // NOTE(kv) clang debug info is busted
 //push(&params.compiler_args, strlit("-fsanitize=undefined"));
 if(s.asan_on)
 {
  push(&params.compiler_args, strlit("-fsanitize=address"));
 }
 ok = ok and run_compiler(compiler, params);
 
 if(not ok and s.release_editor)
 {// NOTE rollback
  move_file(bkp_name, binary_name);
 }
 return ok;
}
function b32
build_framework(Thread_Info info, void *arg)
{
 Scratch_Block tmp;
 b32 ok = true;
 Meta_Directories dirs = meta.dirs;
 Build_Shared s = build_shared;
 
 Build_Params params = build_parameters();
 
 String GAME_MAIN = pjoin(tmp, dirs.game, strlit("game_main.cpp"));
 add_input_file(params, GAME_MAIN);
 for_i32(i, 0, s.imgui_TUs.count)
 {//NOTE Link with imgui
  add_input_file(params, push_stringf(tmp, "%S.obj", s.imgui_TUs.items[i]));
 }
 
 b32 DEV_BUILD = true;
 add_define_symbol(params, "KV_INTERNAL", DEV_BUILD);
 add_include(params, dirs.code);
 add_include(params, s.libs_dir);
 add_linker_arg(params, strlit("-DLL -export:game_api_export"));
 params.output = strlit("game_main.dll");
 
 Stringz lock = strlit("game.lock");
 ok = ok and create_lock_file(lock);
 ok = ok and run_compiler(Compiler_MSVC, params);
 ok = ok and remove_file(lock);
 
 return ok;
}
function b32
build_main(i32 arg_count, String *args)
{
 Scratch_Block tmp;
 
 Work_Queue queue = {};
 const i32 thread_count = 8;
 {
  i32 initial_count = 0;
  queue.semaphore = CreateSemaphoreExA(0, initial_count, thread_count,
                                       0, 0, SEMAPHORE_ALL_ACCESS);
 }
 
 Thread_Info thread_infos[thread_count] = {};
 for_i32(thread_index, 0, thread_count)
 {
  Thread_Info *info = thread_infos + thread_index;
  info->thread_index = thread_index;
  info->work_queue   = &queue;
 }
 
 for_i32(thread_index, 1, thread_count)
 {//NOTE(kv) It starts with 1
  Thread_Info *info = thread_infos + thread_index;
  DWORD thread_id;
  HANDLE handle = CreateThread(0, 0, thread_loop, info, 0, &thread_id);
  CloseHandle(handle);
 }
 
 if(0)
 {//-Thread test
  for_i32(i, 0, 16)
  {
   i32 *arg = push_value(tmp, i);
   push_work(&queue, print_number_function, arg);
  }
 }
 
 Meta_Directories dirs = meta.dirs;
 Build_Shared &s = build_shared;
 b32 do_build_editor = false;
 b32 do_build_game   = false;
 
 for_i32(argi, 1, arg_count)
 {//-Script arguments
  String arg = args[argi];
  if(0);
  else if(arg == "--build_driver") { s.build_driver = 1; }
  else if(arg == "--release")      { s.release_editor  = 1; }
  else if(arg == "--build-editor") { do_build_editor   = 1; }
  else if(arg == "--build-game")   { do_build_game     = 1; }
  else if(arg == "--asan-on")      { s.asan_on         = 1; }
  else if(arg == "--no-force-inline"){ s.no_force_inline = 1; }
 }
 
 if(s.release_editor){ do_build_editor = true; }
 
 // NOTE Disable hotload to track bugs
 //s.hotload_driver = false;
 
 if(meta.hotload_driver)
 {
  do_build_game = true;
  do_build_editor = false;
 }
 if(s.release_editor){ do_build_game = false; }
 
 {//-Clang warnings
  char *clang_warnings_cstring[] = {
   "-Wimplicit-int-float-conversion",
   "-Wshadow",
   "-Wno-unused-const-variable",
   "-Wno-unused-variable",
   "-Wno-unused-label",
   "-Wno-unused-but-set-variable",
   "-Wno-write-strings",
   "-Wno-null-dereference",
   "-Wno-comment",
   "-Wno-switch",
   "-Wno-missing-declarations",
   "-Wno-deprecated-declarations",
   "-Wno-missing-braces",
   "-Wno-unused-parameter",
   "-Wno-unused-function",
   "-Wno-backslash-newline-escape",
   "-Wno-deprecated-enum-enum-conversion",
   "-Wno-deprecated-anon-enum-enum-conversion",
   "-Wno-multichar",
  };
  const u32 count = alen(clang_warnings_cstring);
  String clang_warnings_string[count];
  for_u32(i, 0, count)
  {
   clang_warnings_string[i] = SCu8(clang_warnings_cstring[i]);
  }
  clang_warnings = join_strings(tmp, strlit(" "), clang_warnings_string, count);
 }
 {//-MSVC warnings
  String unused_var = strlit("-wd4189");
  String signed_unsigned_mismatch = strlit("-wd4245");
  String enum_freedom = strlit("-wd4063");
  String number_lossy_conversion = strlit("-wd4244");
  String msvc_warnings_list[] = {
   unused_var,
   signed_unsigned_mismatch,
   enum_freedom,
   strlit("-wd4200 -wd4146 -wd4201 -wd4100 -wd4101 -wd4815 -wd4505 -wd4701 -wd4816 -wd4702 -wd4211")
  };
  msvc_warnings = join_strings(tmp, strlit(" "), ArrayAndCount(msvc_warnings_list));
 }
 
 Stringz CUSTOM = pjoin(tmp, dirs.code, strlit("custom"));
 
 b32 ok = true;
 s.libs_dir  = pjoin(tmp, dirs.code, strlit("libs"));
 s.imgui_dir = pjoin(tmp, s.libs_dir,  strlit("imgui"));
 
 String imgui_TUs[] = {
  strlit("imgui"),
  strlit("imgui_tables"),
  strlit("imgui_demo"),
  strlit("imgui_draw"),
  strlit("imgui_widgets"),
 };
 
 s.imgui_TUs.items = imgui_TUs;
 s.imgui_TUs.count = alen(imgui_TUs);
 
 if(do_build_editor)
 {
  push_work(&queue, build_editor, 0);
 }
 
 if(do_build_game)
 {//-Game stuff
  b32 DEV_BUILD = 1;
  Stringz GAME_DIR = dirs.game;
  
  if(not meta.hotload_driver)
  {// NOTE Framework
   push_work(&queue, build_framework, 0);
  }
  
  if(s.build_driver)
  {//-Driver
   Build_Params common = build_parameters();
   {// NOTE Common build params
    add_define_symbol(common, "KV_INTERNAL", DEV_BUILD);
    add_include(common, dirs.code);
    add_include(common, s.libs_dir);
    add_include(common, dirs.game);
   }
   
   Stringz precompiled_header = strlit("driver_precompiled.h");
   
   if(not meta.hotload_driver)
   {//-Precompiled header
    Build_Params params = common;
    params.compile_only  = 1;
    params.no_debug_info = 1;
    params.no_warnings   = 1;
    Stringz precompiled_cpp = pjoin(tmp, meta.dirs.driver, strlit("driver_precompiled.cpp"));
    add_input_file(params, precompiled_cpp);
    push(&params.compiler_args, strcat(tmp, strlit("-Yc"), precompiled_header));
    
    ok = ok and run_compiler(Compiler_MSVC, params);
   }
   
   {//-Actual Driver code
    Build_Params params = common;
    String DRIVER_MAIN = pjoin(tmp, meta.dirs.driver, strlit("driver_main.cpp"));
    add_input_file(params, DRIVER_MAIN);
    //push(params.compiler_args, strlit("-GF"));  //NOTE(kv) String deduplication, but we don't need it!
    
    if(meta.hotload_driver)
    {
     add_input_file(params, strlit("driver_precompiled.obj"));
     add_input_file(params, strcat(tmp, strlit("-Yu"), precompiled_header));
    }
    
    params.output = strlit("driver.dll");
    if(meta.hotload_driver)
    {// NOTE(kv) We don't actually know that inlining is faster or not.
     params.no_force_inline = 1;
     params.no_debug_info   = 1;
     params.no_warnings     = 1;
    }
    
    add_linker_arg(params, strlit("-DLL"));
    add_linker_arg(params, strlit("-export:driver_dll_entry"));
    if(BUILD_USE_SUSPICIOUS_FEATURES)
    {// NOTE(kv) Incremental linking shaves off like 100ms from 300ms build -> worth!
     // Don't trust MSVC when they say incremental is on by default.
     add_linker_arg(params, strlit("-INCREMENTAL"));
    }
    
    Stringz lock = strlit("echo lock driver.lock");
    ok = ok and create_lock_file(lock);
    ok = ok and run_compiler(Compiler_MSVC, params);
    ok = ok and remove_file(lock);
   }
  }
 }
 
 while(queue.completion_count < queue.push_index)
 {//-spinlock-ish loop to do work and wait for other jobs
  b32 did_work = maybe_do_work(thread_infos[0]);
  if(not did_work)
  {//NOTE(kv) Sleep for a bit before checking again
   sleep_ms(20);
  }
 }
 
 for_i32(work_index, 0, queue.push_index)
 {//-Tally our work
  Work_Queue_Entry *work = &queue.entries[work_index];
  if(not work->ok)
  {
   ok = false;
   break;
  }
 }
 
 return ok;
}
//-