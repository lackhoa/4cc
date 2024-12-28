#pragma once

global char *meta_command_name;
global i1 meta_logging_level = 0;

struct Meta_Directories{
 Stringz code;
 String code_gen;
 Stringz game;
 Stringz game_gen;
};
global Meta_Directories meta_dirs;  //@meta_dirs_init

struct Lexed_File{
 String name;
 String data;
 Token_List token_list;
 b32 ok;
};
#define meta_logf(...) if(meta_logging_level){ printf(__VA_ARGS__); }
function void meta_process_ast(Statement_Root root, String source_path);

typedef b32 String_Predicate(String path);

enum Is_Recursive{
 No_Recursive,
 Yes_Recursive,
};

function b32
list_files_in_dir(Arena *arena, darray(Stringz) *outfiles, String path,
                  Is_Recursive recursive, String_Predicate predicate=0)
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
    
    if(fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
     //NOTE(kv) this is a directory
     if(recursive){
      ok = list_files_in_dir(arena, outfiles, file_path, recursive, predicate);
     }
    }else{
     //NOTE(kv) This is a file
     b32 satisfies_predicate = true;
     if(predicate != 0){
      satisfies_predicate = predicate(file_path);
     }
     if(satisfies_predicate){
      char *copy = push_array_copy(arena, char, file_path.len+1, buffer);
      Stringz item = {(u8 *)copy, file_path.len};
      push(*outfiles, item);
     }
    }
   }
  }while(ok && FindNextFile(hFind, &fdFile));
  
  FindClose(hFind);
 }
 
 return ok;
}
//-
