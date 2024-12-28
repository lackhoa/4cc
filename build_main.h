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
  result.count += 1;  // note nil terminator
  
  //note(kv) Printing things
  result.str = push_size(arena, result.count);
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
  kv_assert(at == result.str + result.count);
 }
 return result;
}

global String clang_warnings;
//