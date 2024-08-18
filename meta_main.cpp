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

int
main(int argc, char **argv)
{
 b32 ok = true;
 meta_logging_level = 1;
 command_name = argv[0];
 if (argc < 2)
 {
  printf("Usage: %s <source> {<source>}\n"
         "(source : source files for introspection)\n",
         command_name);
  ok = false;
 }
 
 Scratch_Block scratch;
 
 arrayof<File_Name_Data> source_files = {};
 if (ok)
 {// NOTE: Reading the input files
  for_i1(argi,1,argc)
  {
   char *filename = argv[argi];
   FILE *file = fopen(filename, "rb");
   if (file)
   {
    String text = read_entire_file_handle(scratch, file);
    fclose(file);
    source_files.push(File_Name_Data{ .name=SCu8(filename), .data=text });
   }
   else
   {
    printf("error: could not open input file: '%s'\n", filename);
    ok = false;
    break;
   }
  }
 }
 
 ok = ok && api_parser_main(source_files);
 ok = ok && introspect_main(source_files);
 
 int exit_code = !ok;
 return exit_code;
}

// BOTTOM
