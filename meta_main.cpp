/*
 * Mr. 4th Dimention - Allen Webster
 * (Modified by kv)
 *
 * Do all the meta programming things
 *
 */

// TOP

#include "kv.h"

#include "4coder_token.h"
#include "generated/lexer_cpp.h"
#include "4ed_api_definition.h"
#include "meta.h"

#include "4coder_stringf.cpp"
#include "4coder_malloc_allocator.cpp"
#include "4coder_token.cpp"
#include "generated/lexer_cpp.cpp"
#include "4ed_api_definition.cpp"
#include "4ed_api_parser.cpp"
#include "meta_introspect.cpp"

function b32
list_files_recursive(Arena *arena, arrayof<char *> &outfiles, char *path)
{
 b32 ok = true;
 WIN32_FIND_DATA fdFile;
 char sPath[2048];
 sprintf(sPath, "%s\\*.*", path);
 
 HANDLE hFind = FindFirstFile(sPath, &fdFile);
 if(hFind == INVALID_HANDLE_VALUE) {
  printf("error: path not found '%s'\n", path);
  ok = false;
 }
 
 if (ok){
  do {
   if(strcmp(fdFile.cFileName, ".")  != 0 &&
      strcmp(fdFile.cFileName, "..") != 0) {
    sprintf(sPath, "%s\\%s", path, fdFile.cFileName);
    
    if(fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
     //NOTE(kv) this is a directory
     ok = list_files_recursive(arena, outfiles, sPath);
    }else{
     //NOTE(kv) This is a file
     char *copy = push_array_copy(arena, char, strlen(sPath)+1, sPath);
     outfiles.push(copy);
    }
   }
  } while(ok && FindNextFile(hFind, &fdFile));
  
  FindClose(hFind);
 }
 
 return ok;
}

xfunction int
main(int argc, char **argv)
{
 b32 ok = true;
 meta_logging_level = 1;
 command_name = argv[0];
 if (argc < 2) {
  printf("Usage: %s <source> {<source>}\n"
         "(source : source files for introspection)\n",
         command_name);
  ok = false;
 }
 
 Scratch_Block scratch;
 
 arrayof<File_Name_Data> source_files = {};
 if (ok) {
  // NOTE: Reading the input files
  for_i1(argi,1,argc) {
   char *path = argv[argi];
   arrayof<char *> filenames = {};
   if (path_is_directory(path)){
    ok = list_files_recursive(scratch, filenames, path);
    if(!ok){
     printf("error: could not list files under path: [%s]\n", path);
    }
   }else{
    filenames.push(path);
   }
   
   if (ok){
    for_i32(ifile, 0, filenames.count){
     char *filename = filenames[ifile];
     FILE *file = fopen(filename, "rb");
     if (file) {
      String text = read_entire_file_handle(scratch, file);
      fclose(file);
      source_files.push(File_Name_Data{
                         .name=push_string(scratch, filename),
                         .data=text});
     } else {
      printf("error: could not open input file: [%s]\n", filename);
      ok = false;
      break;
     }
    }
   }
  }
 }
 
 ok = ok && api_parser_main(source_files);
 ok = ok && introspect_main(source_files);
 
 int exit_code = !ok;
 return exit_code;
}

// BOTTOM
