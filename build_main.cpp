//-
//NOTE(kv) The only way to build your code, is with more code
//-
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
struct Compile_Params
{
 darray(String) input_files;
 String output_file;
 
 // NOTE Compiler
 darray(String) includes;
 darray(String) compiler_flags;
 darray(Define_Symbol) define_symbols;
 
 darray(String) linker_flags;
 b32 debug_symbols;
};
function void
add_define_symbol(Compile_Params &params, String key, u32 value)
{
 Define_Symbol symbol = {key, value};
 push(params.define_symbols, symbol);
}
myinline void
add_define_symbol(Compile_Params &params, char *key, u32 value)
{
 add_define_symbol(params, SCu8(key), value);
}
myinline void
add_include(Compile_Params &params, String path)
{
 push(params.includes, path);
}
function void
add_input_file(Compile_Params &params, String path)
{
 push(params.input_files, path);
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
 Scratch_Block_Fast scratch;
 
 //NOTE(kv) Monkey business: why is it so hard to use this freaking API?
 push_first(command, String(strlit("/C")));
 Stringz arguments = join_strings(scratch, strlit(" "), command.items, command.count);
 myprintf("Running command: %S\n", string_skip(arguments, 3));
 
 STARTUPINFO startup_info = {};
 startup_info.cb = sizeof(startup_info);
 PROCESS_INFORMATION process_info = {};
 Stringz cmd_exe = strlit("c:\\windows\\system32\\cmd.exe");
 b32 ok = CreateProcessA(to_cstring(cmd_exe),
                         to_cstring(arguments),
                         0, 0, FALSE, 0, 0, 0,
                         &startup_info,
                         &process_info);
 HANDLE process_handle = process_info.hProcess;
 if(ok)
 {//NOTE(kv) Blocking call for now
  u32 wait_result = WaitForSingleObject(process_handle, 0);
  CloseHandle(process_handle);
  ok = ok and (wait_result == WAIT_OBJECT_0);
 }
 if(ok)
 {
  DWORD exit_code;
  ok = ok and GetExitCodeProcess(process_handle, &exit_code);
  ok = ok and (exit_code == 0);
 }
 return ok;
}
//NOTE(kv) stroustrup!
function void
push(darray(String) &array, Stringz string)
{
 push(array, String(string));
}
function b32
run_compiler(Compiler compiler, Compile_Params params)
{//NOTE(kv) Let's assume that this is running in the correct build directory.
 Scratch_Block scratch;
 darray(String) cmd;
 init_dynamic(cmd, scratch, 128);
 
 b32 is_msvc  = compiler == Compiler_MSVC;
 b32 is_clang = compiler == Compiler_ClangCl;
 
 String compiler_name = strlit("cl");
 if(compiler == Compiler_ClangCl){
  compiler_name = strlit("clang-cl");
 }
 push(cmd, compiler_name);
 
 for_u32(i, 0, params.input_files.count){
  push(cmd, params.input_files[i]);
 }
 
 for_u32(i, 0, params.compiler_flags.count){
  String flag = params.compiler_flags.items[i];
  push(cmd, flag);
 }
 
 for_u32(i, 0, params.define_symbols.count){
  Define_Symbol symbol = params.define_symbols.items[i];
  push(cmd, push_stringf(scratch, "-D%S=%u", symbol.name, symbol.value));
 }
 
 for_u32(i, 0, params.includes.count){
  String include = params.includes.items[i];
  push(cmd, push_stringf(scratch, "-I%S", include));
 }
 
 for_u32(i, 0, params.linker_flags.count){
  String flag = params.linker_flags.items[i];
  push(cmd, flag);
 }
 
 push(cmd, strlit("-std:c++20 -D_CRT_SECURE_NO_WARNINGS -FC"));
 if(is_msvc){
  push(cmd, strlit("-Zc:strictStrings-"));
 }
 
 if(is_clang){
  push(cmd, clang_warnings);
 }
 
 b32 ok = run_command(cmd);
 return ok;
}
function Compile_Params
compile_parameters()
{
 Compile_Params params = {};
 //NOTE(kv) We don't init arrays, because it's annoying and pointless anyway.
 params.debug_symbols = true;
 return params;
}
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
build_main(i32 arg_count, String *args)
{
 return true;
 Scratch_Block scratch;
 Meta_Directories dirs = meta_dirs;
 String built_from_file = {};
 
 for_i32(argi, 1, arg_count)
 {//-Build arguments
  String arg = args[argi];
  //myprintf("argument %d: %S\n", argi, arg);
  if(arg == "--file")
  {
   argi++;
   if(argi < arg_count){
    built_from_file = args[argi];
   }
  }
 }
 
 {//-Building warnings
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
  clang_warnings = join_strings(scratch, strlit(" "), clang_warnings_string, count);
 }
 
 Stringz CUSTOM = pjoin(scratch, dirs.code, strlit("custom"));
 b32 DEV_BUILD = true;
 b32 define_KV_SLOW = false;
 b32 define_KV_INTERNAL = DEV_BUILD;
 
 b32 ok = true;
 {//-Build the editor
  Compile_Params params = compile_parameters();
  String BINARY_NAME = DEV_BUILD ? strlit("4ed") : strlit("4ed_stable");
  add_input_file(params, pjoin(scratch, dirs.code, strlit("meta_main.cpp")));
  add_include(params, dirs.code);
  add_include(params, pjoin(scratch, dirs.code, strlit("libs")));
  add_include(params, pjoin(scratch, dirs.code, strlit("libs/imgui")));
  params.output_file = BINARY_NAME;
  add_define_symbol(params, "KV_INTERNAL", DEV_BUILD);
  
  Stringz imgui_dir = pjoin(scratch, dirs.code, strlit("libs/imgui"));
  sarray(Stringz) imgui_cpp_basenames = list_all_cpp_files_top_level(scratch, imgui_dir);
  
  ok = ok and run_compiler(Compiler_ClangCl, params);
 }
 return ok;
}
//-