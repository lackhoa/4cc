#pragma once

global Stringz meta_command_name;
global i1 meta_logging_level = 0;

struct Meta_Directories
{// @meta_dirs_init
 Stringz home;
 Stringz code;
 Stringz game;
 Stringz backup;
 Stringz driver;
 };
struct Meta_Globals
{
 Meta_Directories dirs;
 b32 testing;
 b32 hotload_driver;
};
global Meta_Globals meta;

struct Lexed_File
{
 Stringz path;
 String data;
 Token_List token_list;
 b32 ok;
};
#define meta_logf(...) if(meta_logging_level){ myprintf(__VA_ARGS__); }

function void meta_process_ast(Statement_Root root, String source_path);

myinline Ed_Parser
ed_parser_from_lexed_file(Lexed_File source)
{
 return ed_parser_from_token_list(source.data, source.token_list);
}

typedef b32 String_Predicate(String path);

enum Is_Recursive
{
 No_Recursive,
 Yes_Recursive,
};

function b32
list_files_in_dir(Arena *arena, darray(Stringz) *outfiles, String path,
                  Is_Recursive recursive,
                  String_Predicate predicate=0,
                  sarray(Stringz) black_listed_paths={})
{
 b32 ok = true;
 WIN32_FIND_DATA fdFile;
 char buffer[2048];
 stbsp_snprintf(buffer, alen(buffer), "%S\\*.*", path);
 
 HANDLE hFind = FindFirstFile(buffer, &fdFile);
 if(hFind == INVALID_HANDLE_VALUE){
  ok = false;
 }
 
 if(ok)
 {
  do{
   if(strcmp(fdFile.cFileName, ".")  != 0 &&
      strcmp(fdFile.cFileName, "..") != 0)
   {
    //NOTE(kv) copy the file path to buffer
    stbsp_snprintf(buffer, alen(buffer), "%S\\%s", path, fdFile.cFileName);
    Stringz file_path = SCu8(buffer);
    
    if(fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {//-Directory
     if(recursive)
     {
      b32 black_listed = false;
      for_i32(i, 0, black_listed_paths.count)
      {
       if(black_listed_paths[i] == file_path)
       {
        black_listed = true;
        break;
       }
      }
      if(not black_listed)
      {
       ok = list_files_in_dir(arena, outfiles, file_path, recursive, predicate, black_listed_paths);
      }
     }
    }
    else
    {//-File
     b32 satisfies_predicate = true;
     if(predicate != 0){
      satisfies_predicate = predicate(file_path);
     }
     if(satisfies_predicate){
      char *copy = push_array_copy(arena, char, file_path.len+1, buffer);
      Stringz item = {(u8 *)copy, file_path.len};
      push(outfiles, item);
     }
    }
   }
  }while(ok && FindNextFile(hFind, &fdFile));
  
  FindClose(hFind);
 }
 
 return ok;
}
function b32
is_cpp_file(String path)
{
 String extension = path_extension(path);
 return (extension == strlit("h") or
         extension == strlit("cpp"));
}
function b32
is_klang_file(String path)
{
 String extension = path_extension(path);
 return (extension == strlit("kh") or
         extension == strlit("kc"));
}
function b32
is_skm_file(String path)
{
 String extension = path_extension(path);
 return (extension == strlit("skm"));
}
function Lexed_File
lex_file(Arena *arena, Stringz path)
{
 Lexed_File result = {};
 Stringz data = read_entire_file(arena, path);
 Token_List token_list = (is_skm_file(path) ?
                          lex_full_input_skm(arena, data) :
                          lex_full_input_cpp(arena, data));
 result.ok        = 1;
 result.path      = path;
 result.data      = data;
 result.token_list= token_list;
 return result;
}
//-
