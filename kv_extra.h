//-
myinline String
push_data(Arena *arena, u64 size)
{
 String result = {};
 result.str = push_array(arena, u8, size);
 result.size = size;
 return(result);
}
myinline Stringz
push_string(Arena *arena, String data)
{
 Stringz result = push_stringf(arena, "%S", data);
 return(result);
}
//
inline b32
string_match(String a, String b){
 return(a.size == b.size &&
        block_match(a.str, b.str, a.size));
}
inline b32
string_match(String a, char b){
 return(a.size == 1 &&
        a.str[0] == (u8)b);
}
//
myinline bool
operator==(String a, String b){
 return string_match(a,b);
}
myinline bool
operator==(String a, const char *b){
 return a == SCu8(b);
}
myinline bool
operator==(String a, char b){
 return a.count == 1 && *a.str == b;
}
myinline b32
char_is_whitespace(u8 c){
 return(c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '\f' || c == '\v');
}
function u64
string_find_first_non_whitespace(String str){
 u64 i = 0;
 for (;i < str.size && char_is_whitespace(str.str[i]); i += 1);
 return(i);
}
function String
string_skip_whitespace(String str){
 u64 f = string_find_first_non_whitespace(str);
 str = string_skip(str, f);
 return(str);
}
function u8
get_matching_group_closer(u8 c){
 if(c == '(') return ')';
 if(c == '[') return ']';
 if(c == '{') return '}';
 return 0;
}
function b32
is_group_opener(u8 c)
{
 return get_matching_group_closer(c) != 0;
}
function u8
get_matching_group_opener(u8 c){
 if(c == ')') return '(';
 if(c == ']') return '[';
 if(c == '}') return '{';
 return 0;
}
function b32
is_group_closer(u8 c)
{
 return get_matching_group_opener(c) != 0;
}

function u64
string_find_first_whitespace(String str){
 u64 i = 0;
 for (;i < str.size && !char_is_whitespace(str.str[i]); i += 1);
 return(i);
}
function i64
string_find_last_whitespace(String str){
 i64 size = (i64)str.size;
 i64 i = size - 1;
 for (;i >= 0 && !char_is_whitespace(str.str[i]); i -= 1);
 return(i);
}

function i64
string_find_last_non_whitespace(String str){
 i64 size = (i64)str.size;
 i64 i = size - 1;
 for (;i >= 0 && char_is_whitespace(str.str[i]); i -= 1);
 return(i);
}
function u64
string_find_first_slash(String str){
 u64 i = 0;
 for (;i < str.size && !is_file_slash(str.str[i]); i += 1);
 return(i);
}
function i64
string_find_last_slash(String str){
 i64 size = (i64)str.size;
 i64 i = size - 1;
 for (;i >= 0 && !is_file_slash(str.str[i]); i -= 1);
 return(i);
}
function i64
string_find_last(String str, u8 c){
 i64 size = (i64)str.size;
 i64 i = size - 1;
 for (;i >= 0 && c != str.str[i]; i -= 1);
 return(i);
}
function String
string_prefix(String str, u64 size){
 size = clamp_max(size, str.size);
 str.size = size;
 return(str);
}
//-
struct Node_String
{
 Node_String *next;
 String string;
};
struct List_String
{
 Node_String *first;
 Node_String *last;
 u64 total_size;
 i32 node_count;
};
function u64
string_find_first(String str, u64 start_pos, u8 c)
{
 u64 i = start_pos;
 for (;i < str.size && c != str.str[i]; i += 1);
 return(i);
}
function u64
string_find_first(String str, u8 c)
{
 return(string_find_first(str, 0, c));
}
function void
string_list_push(List_String *list, Node_String *node)
{
 sll_queue_push(list->first, list->last, node);
 list->node_count += 1;
 list->total_size += node->string.size;
}

function void
string_list_push(Arena *arena, List_String *list, String string)
{
 Node_String *node = push_array(arena, Node_String, 1);
 sll_queue_push(list->first, list->last, node);
 node->string = string;
 list->node_count += 1;
 list->total_size += string.size;
}

#define string_list_push_lit(a,l,s)    string_list_push((a), (l), string_litexpr(s))
#define string_list_push_u8_lit(a,l,s) string_list_push((a), (l), strlit(s))

function void
string_list_push(List_String *list, List_String *src_list){
 sll_queue_push_multiple(list->first, list->last, src_list->first, src_list->last);
 list->node_count += src_list->node_count;
 list->total_size += src_list->total_size;
 block_zero_array(src_list);
}

function List_String
string_split(Arena *arena, String string, u8 *split_characters, i32 split_character_count)
{
 List_String list = {};
 for(;;)
 {
  u64 i = string.size;
  String prefix = string;
  for_i32(j,0,split_character_count)
  {
   u64 pos = string_find_first(prefix, split_characters[j]);
   prefix = string_prefix(prefix, pos);
   i = Min(i, pos);
  }
  if(prefix.size > 0)
  {
   string_list_push(arena, &list, prefix);
  }
  string = string_skip(string, i + 1);
  if (string.size == 0){ break; }
 }
 return(list);
}
//-
function List_String
path_split(Arena *arena, String path)
{
 return string_split(arena, path, (u8 *)"/\\", 2);
}
function b32
path_contains(String path, String name)
{
 b32 result = 0;
 Scratch_Scope tmp;
 List_String split = path_split(tmp, path);
 for(Node_String *node = split.first;
     node != 0;
     node = node->next)
 {
  if(node->string == name)
  {
   result = 1;
   break;
  }
 }
 return result;
}

function String
path_dir(String str)
{
 if(str.size > 0){
  // NOTE(kv) Remove the last slash, if exists
  str.size -= 1;
 }
 i64 slash_pos = string_find_last_slash(str);
 String result = {.str=str.str};
 if(slash_pos >= 0){ result.size = slash_pos+1; }
 return result;
}
function String
path_basename(String str)
{
 if(str.count > 0 &&
    str.str[str.count-1]==OS_SLASH){
  //-NOTE(kv) remove last slash
  str.size -= 1;
 }
 i64 slash_pos = string_find_last_slash(str);
 if(slash_pos >= 0)
 {
  str = string_skip(str, slash_pos + 1);
 }
 return(str);
}
function String
path_filename(String str){
 //NOTE(kv) Different from "basename" in that if you provide me a path with a slash at the end
 //  I will return empty string. Do we really need it? I have no idea...
 i64 slash_pos = string_find_last_slash(str);
 if(slash_pos >= 0){
  str = string_skip(str, slash_pos + 1);
 }
 return(str);
}
function String
path_extension(String path){
 return(string_skip(path, string_find_last(path, '.') + 1));
}
function String
path_no_extension(String path){
 i64 pos = string_find_last(path, '.');
 if(pos > 0){
  //NOTE(kv) Ignore the first dot, because that could be a dot file.
  path = string_prefix(path, pos);
 }
 return path;
}
function String
path_stem(String path)
{
 path = path_filename(path);
 path = path_no_extension(path);
 return(path);
}
//-
inline String
SCu8(u8 *str, u8 *one_past_last){
 return(SCu8(str, (u64)(one_past_last - str)));
}
myinline String
SCu8(char *str, u64 length){
 return(SCu8((u8*)str, length));
}
//
myinline String
SCu8(char *first, char *one_past_last){
 return(SCu8((u8*)first, (u8*)one_past_last));
}

function b32
string_contains(String big, String small, i1 *first_match=0)
{
 b32 result = false;
 if (small.len == 0)
 {
  result = big.len == 0;
 }
 else
 {
  i1 lendiff = i1(big.len - small.len);
  for_i1(index,0,lendiff+1)
  {
   if (big.str[index] == small.str[0])
   {
    String substring = {big.str+index, small.len};
    if(substring == small)
    {
     result = true;
     if (first_match) { *first_match = index; }
     break;
    }
   }
  }
 }
 return result;
}

function b32
starts_with(String str, String prefix) {
 return string_match(string_prefix(str, prefix.size), prefix);
}
function b32
starts_with(String str, char prefix) {
 return str.count > 0 and str[0] == prefix;
}
#define starts_with_lit(string,prefix) starts_with(string, strlit(prefix))

#define string_match_lit(a,b)  string_match(a,strlit(b))
//-

inline u32
cast_u64_to_u32(u64 u){
 kv_assert(u < (1ULL << 32));
 return (u32)u;
}
function isize
gb__scan_i64(String string, i1 base, i64 *value){
	u8 *text_begin = string.str;
	i64 result = 0;
	b32 negative = false;
	if(string[0] == '-'){
		negative = true;
		string_skip(&string, 1);
	}
	if(base == 16 && string_prefix(string, 2) == "0x"){
		string_skip(&string, 2);
	}
	for(;;){
		i64 v;
		if(gb_char_is_digit(*string.str)){
			v = *string.str - '0';
		}else if(base == 16 && gb_char_is_hex_digit(*string.str)) {
			v = gb_hex_digit_to_int(*string.str);
		}else{
			break;
		}
		result *= base;
		result += v;
		string_skip(&string, 1);
	}
	if(value){
		if(negative){ result = -result; }
		*value = result;
	}
	return (string.str - text_begin);
}
function i64
str_to_i64(String string, char **end_ptr, i1 base){
	if(!base){
  base = 10;
		if(string_prefix(string, 2) == strlit("0x")){
			base = 16;
		}
	}
	i64 value;
	isize len = gb__scan_i64(string, base, &value);
	if(end_ptr){ *end_ptr = (char *)string.str + len; }
	return value;
}
function f64
str_to_f64(String string, char **end_ptr_out){
 char *str = (char *)string.str;
	f64 result, value, sign, scale;
	i1 frac = 0;
 
	while(gb_char_is_space(*str)){
		str++;
	}
 
	sign = 1.0;
	if(*str == '-'){
		sign = -1.0;
		str++;
	}else if(*str == '+'){
		str++;
	}
 
	for (value = 0.0; gb_char_is_digit(*str); str++) {
  // note(kv): before the decimal point
		value = value * 10.0 + (*str-'0');
	}
 
	if (*str == '.') {
  // note(kv): after the decimal point
		f64 pow10 = 10.0;
		str++;
		while (gb_char_is_digit(*str)) {
			value += (*str-'0') / pow10;
			pow10 *= 10.0;
			str++;
		}
	}
 
	scale = 1.0;
	if((*str == 'e') || (*str == 'E')){
		u32 exp;
  
		str++;
		if (*str == '-') {
			frac = 1;
			str++;
		} else if (*str == '+') {
			str++;
		}
  
		for(exp = 0; gb_char_is_digit(*str); str++){
			exp = exp * 10 + (*str-'0');
		}
		if (exp > 308) exp = 308;
  
		while (exp >= 50) { scale *= 1e50; exp -= 50; }
		while (exp >=  8) { scale *= 1e8;  exp -=  8; }
		while (exp >   0) { scale *= 10.0; exp -=  1; }
	}
 
	result = sign * (frac ? (value / scale) : (value * scale));
 
	if(end_ptr_out){ *end_ptr_out = cast(char *)str; }
 
	return result;
}
//-Sorting
typedef i32 Compare_Function(void *a, void *b);

#define GB__SORT_PUSH(_base, _limit) do { \
stack_ptr[0] = (_base); \
stack_ptr[1] = (_limit); \
stack_ptr += 2; \
} while (0)

#define GB__SORT_POP(_base, _limit) do { \
stack_ptr -= 2; \
(_base)  = stack_ptr[0]; \
(_limit) = stack_ptr[1]; \
} while (0)

function void
gb_sort(void *base_, isize count, isize size, Compare_Function compare)
{
 // NOTE(bill): Uses quick sort for large arrays but insertion sort for small
 const i32 insert_sort_threshold = 8;
	u8 *i, *j;
	u8 *base = cast(u8 *)base_;
	u8 *limit = base + count*size;
	isize threshold = insert_sort_threshold * size;
 
 const i32 stack_size = 64;  //NOTE(kv) We could math it out if we care, but 64 items is enough for everything.
 u8 *stack[stack_size];
 u8 **stack_ptr = stack;
 
 while(true){
  if((limit-base) > threshold){
   //-NOTE(bill): Quick sort
   i = base + size;
   j = limit - size;
   //NOTE(kv) Swap pivot to the beginning (so that it's not involved in the swapping below)
   gb_memswap(((limit-base)/size/2) * size + base, base, size);
   {//NOTE(kv) Maintain the ordering base < i < j
    if (compare(i, j) > 0)    gb_memswap(i, j, size);
    if (compare(base, j) > 0) gb_memswap(base, j, size);
    if (compare(i, base) > 0) gb_memswap(i, base, size);
   }
   for (;;) {
    do i += size; while (compare(i, base) < 0);
    do j -= size; while (compare(j, base) > 0);
    if (i > j) break;
    gb_memswap(i, j, size);
   }
   {//NOTE(kv) Swap the pivot back to its place,
    //  now all items to the left of the pivot are less, items to the right greater.
    gb_memswap(base, j, size);
   }
   //NOTE(kv) Between the region to the left and right of the pivot,
   //  sort the smaller one first. (I guess since we wanna minimize stack usage).
   if(j - base > limit - i){
    GB__SORT_PUSH(base, j);
    base = i;
   } else {
    GB__SORT_PUSH(i, limit);
    limit = j;
   }
  }else{
   //-NOTE(bill): Insertion sort
   for (j = base, i = j+size;
        i < limit;
        j = i, i += size) {
    for(; compare(j, j+size) > 0; j -= size){
     gb_memswap(j, j+size, size);
     if (j == base) break;
    }
   }
   if(stack_ptr == stack){
    break; // NOTE(bill): Sorting is done!
   } else {
    GB__SORT_POP(base, limit);
   }
  }
 }
}
#undef GB__SORT_PUSH
#undef GB__SORT_POP

#define gb_sort_array(array, count, compare) \
gb_sort(array, count, gb_size_of(*(array)), compare)
//-
//NOTE(kv) f32 is enough because its range is HUGE.
//  Also we don't need that much precision for anything.
struct Sort_Entry
{
 i32 index;
 f32 key;
};
function void
small_insertion_sort(Sort_Entry *input, i32 input_count,
                     Sort_Entry *output, i32 output_count)
{
 kv_assert(output_count <= input_count);
 if(output_count > 0)
 {
  //NOTE bootstrap
  output[0] = input[0];
  i32 filled_count = 1;
  
  for_i32(input_index, 1, input_count)
  {//-Looping over the input (could use binary search, but the list is small so who cares)
   Sort_Entry entry = input[input_index];
   b32 add_entry = false;
   if(filled_count < output_count){
    filled_count++;
    add_entry = true;
   }else{
    add_entry = entry.key < output[filled_count-1].key;
   }
   
   if(add_entry)
   {//NOTE This item is in!
    for(i32 output_index = filled_count-2;
        output_index >= 0;
        output_index--)
    {
     if(entry.key < output[output_index].key){
      //NOTE Slide over
      output[output_index+1] = output[output_index];
      if(output_index == 0){
       //NOTE This is the new smallest value
       output[0] = entry;
      }
     }else{
      //NOTE We found the place for this entry
      output[output_index+1] = entry;
      break;
     }
    }
   }
  }
 }
}
//-
function u32
gb_murmur32_seed(void const *data, isize len, u32 seed)
{
	u32 const c1 = 0xcc9e2d51;
	u32 const c2 = 0x1b873593;
	u32 const r1 = 15;
	u32 const r2 = 13;
	u32 const m  = 5;
	u32 const n  = 0xe6546b64;
 
	isize i, nblocks = len / 4;
	u32 hash = seed, k1 = 0;
	u32 const *blocks = cast(u32 const*)data;
	u8 const *tail = cast(u8 const *)(data) + nblocks*4;
 
	for (i = 0; i < nblocks; i++) {
		u32 k = blocks[i];
		k *= c1;
		k = (k << r1) | (k >> (32 - r1));
		k *= c2;
  
		hash ^= k;
		hash = ((hash << r2) | (hash >> (32 - r2))) * m + n;
	}
 
	switch (len & 3) {
  case 3:
		k1 ^= tail[2] << 16;
  case 2:
		k1 ^= tail[1] << 8;
  case 1:
		k1 ^= tail[0];
  
		k1 *= c1;
		k1 = (k1 << r1) | (k1 >> (32 - r1));
		k1 *= c2;
		hash ^= k1;
	}
 
	hash ^= len;
	hash ^= (hash >> 16);
	hash *= 0x85ebca6b;
	hash ^= (hash >> 13);
	hash *= 0xc2b2ae35;
	hash ^= (hash >> 16);
 
	return hash;
}
function u64
gb_murmur64_seed(void const *data_, isize len, u64 seed)
{
	u64 const m = 0xc6a4a7935bd1e995ULL;
	i1 const r = 47;
 
	u64 h = seed ^ (len * m);
 
	u64 const *data = cast(u64 const *)data_;
	u8  const *data2 = cast(u8 const *)data_;
	u64 const* end = data + (len / 8);
 
	while (data != end) {
		u64 k = *data++;
  
		k *= m;
		k ^= k >> r;
		k *= m;
  
		h ^= k;
		h *= m;
	}
 
	switch (len & 7) {
  case 7: h ^= cast(u64)(data2[6]) << 48;
  case 6: h ^= cast(u64)(data2[5]) << 40;
  case 5: h ^= cast(u64)(data2[4]) << 32;
  case 4: h ^= cast(u64)(data2[3]) << 24;
  case 3: h ^= cast(u64)(data2[2]) << 16;
  case 2: h ^= cast(u64)(data2[1]) << 8;
  case 1: h ^= cast(u64)(data2[0]);
		h *= m;
	};
 
	h ^= h >> r;
	h *= m;
	h ^= h >> r;
 
	return h;
}

myinline u32
gb_murmur32(void const *data, isize len)
{
 return gb_murmur32_seed(data, len, 0x9747b28c);
}
myinline u64
gb_murmur64(void const *data, isize len)
{
 return gb_murmur64_seed(data, len, 0x9747b28c);
}
//-

function String
time_format(Arena *arena, char *format)
{// NOTE(kv) Don't pass crazy long format string in here please!
 String result = {};
 time_t rawtime;
 time(&rawtime);
 struct tm *timeinfo = localtime(&rawtime);
 
 if(timeinfo)
 {
  Scratch_Scope tmp(arena);
  i32 bufsize = 1024;
  char *buf = (char *)push_size(tmp, bufsize);
  size_t strftime_result = strftime(buf, bufsize, format, timeinfo);
  if(strftime_result != 0)
  {
   String string = SCu8(buf);
   result = push_string(arena, string);
  }
  else
  {
   bufsize *= 2;
  }
 }
 
 return result;
}
//-
typedef u32 File_Attribute_Flag;
enum
{
 FileAttribute_IsDirectory = 1,
};

struct File_Attributes
{
 u64 size;
 u64 last_write_time;
 File_Attribute_Flag flags;
};

struct File_Info
{
 File_Info *next;
 Stringz filename;
 File_Attributes attributes;
};

struct File_List
{
 File_Info **infos;
 u32 count;
};

function u8
string_get_character(String str, u64 i)
{
 u8 r = 0;
 if(i < str.size)
 {
  r = str.str[i];
 }
 return(r);
}

typedef i32 String_Fill_Terminate_Rule;
enum{
 StringFill_NoTerminate = 0,
 StringFill_NullTerminate = 1,
};

function Stringz
pjoin(Arena *arena, String a, String b)
{
 char slash = OS_SLASH;
 String joiner = {.str=(u8 *)&slash, .count=1};
 if(is_file_slash(a[a.count-1]))
 {
  joiner = empty_string;
 }
 Stringz result = push_stringf(arena, "%S%S%S", a, joiner, b);
 return result;
}
myinline Stringz
pjoin(Arena *arena, String a, char *b)
{// NOTE(kv) Sorry, due to popular request from the metaprogram, we want a cstring variant.
 return pjoin(arena, a, SCu8(b));
}
myinline Stringz
pjoin(Arena *arena, String a, String b, String c)
{
 Scratch_Scope tmp(arena);
 Stringz result = pjoin(tmp, a, b);
 result = pjoin(arena, result, c);
 return result;
}

myinline b32
file_exists(Stringz path)
{
 return gb_file_exists(to_cstring(path));
}
myinline u64
file_mtime(Stringz path)
{
 return gb_file_last_write_time(to_cstring(path));
}
function b32
remove_file(Stringz path)
{
 b32 result = true;
 if(file_exists(path))
 {
  result = gb_file_remove(to_cstring(path));
 }
 return result;
}
function b32
move_file(Stringz from, Stringz to){
 remove_file(to);
 b32 result = gb_file_move(to_cstring(from), to_cstring(to));
 return result;
}
myinline b32 
copy_file(Stringz from, Stringz to, b32 fail_if_exists=0)
{
 return gb_file_copy(to_cstring(from), to_cstring(to), fail_if_exists);
}

#if OS_WINDOWS
function b32
mkdir_p(Stringz path)
{
 b32 ok = 1;
 if(!CreateDirectoryA(to_cstring(path),0))
 {
  DWORD error = GetLastError();
  if(error != ERROR_ALREADY_EXISTS){
   ok = 0;
  }
 }
 return ok;
}
function b32
mkdir_p(String path)
{
 Scratch_Block scratch;
 Stringz pathz = to_stringz(scratch, path);
 return mkdir_p(pathz);
}
#endif

myinline FILE *
open_file(Stringz name, char *mode)
{
 return fopen(to_cstring(name), mode);
}
function FILE *
open_or_create_file(Stringz name, char *mode, b32 *created=0)
{
 FILE *file = open_file(name, mode);
 if(created){
  *created = file == 0;
 }
 if(file == 0){
  if(errno == ENOENT){
   mkdir_p(path_dir(name));
   open_file(name, mode);
  }
 }
 return file;
}
inline void
close_file(FILE *file)
{// NOTE(kv): Turns out writing a wrapper is sometimes beneficial.
 if(file != 0){
  fclose(file);
 }
}

#if OS_WINDOWS
function b32
path_is_directory(Stringz path)
{
 DWORD attr = GetFileAttributes(to_cstring(path));
 return (attr & FILE_ATTRIBUTE_DIRECTORY);
}
#endif

function Stringz
read_file(Arena *arena, FILE *file, usize size)
{
 Stringz result = empty_string;
 if(file)
 {
  char *mem = push_array(arena, char, size+1);
  usize read_size = fread(mem, 1, (size_t)size, file);
  if(read_size != size)
  {
   // TODO(kv) Error handling, hello?
  }
  else
  {
   mem[size] = 0;  // NOTE: null-termination
   result = {(u8*)mem, size};
  }
 }
 return(result);
}
function Stringz
read_file(Arena *arena, Stringz filename, usize size)
{
 FILE *file = open_file(filename, "rb");
 Stringz result = read_file(arena, file, size);
 close_file(file);
 return result;
}
function Stringz
read_entire_file(Arena *arena, FILE *file)
{
 u64 size = 0;
 if(file)
 {
  fseek(file, 0, SEEK_END);
  size = ftell(file);
  fseek(file, 0, SEEK_SET);
 }
 Stringz result = read_file(arena, file, size);
 return result;
}
function Stringz
read_entire_file(Arena *arena, Stringz filename)
{
 FILE *file = open_file(filename, "rb");
 Stringz result = read_entire_file(arena, file);
 close_file(file);
 return(result);
}

struct Character_Consume_Result
{
 u32 inc;
 u32 codepoint;
};

global const u8 utf8_class[32] = {
 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,2,2,2,2,3,3,4,5,
};

function Character_Consume_Result
utf8_consume(u8 *str, u64 max)
{
 Character_Consume_Result result = {1, max_u32};
 u8 byte = str[0];
 u8 byte_class = utf8_class[byte >> 3];
 switch (byte_class){
  case 1:
  {
   result.codepoint = byte;
  }break;
  case 2:
  {
   if (1 < max){
    u8 cont_byte = str[1];
    if (utf8_class[cont_byte >> 3] == 0){
     result.codepoint = (byte & bitmask_5) << 6;
     result.codepoint |= (cont_byte & bitmask_6);
     result.inc = 2;
    }
   }
  }break;
  case 3:
  {
   if (2 < max){
    u8 cont_byte[2] = {str[1], str[2]};
    if (utf8_class[cont_byte[0] >> 3] == 0 &&
        utf8_class[cont_byte[1] >> 3] == 0){
     result.codepoint = (byte & bitmask_4) << 12;
     result.codepoint |= ((cont_byte[0] & bitmask_6) << 6);
     result.codepoint |=  (cont_byte[1] & bitmask_6);
     result.inc = 3;
    }
   }
  }break;
  case 4:
  {
   if (3 < max){
    u8 cont_byte[3] = {str[1], str[2], str[3]};
    if (utf8_class[cont_byte[0] >> 3] == 0 &&
        utf8_class[cont_byte[1] >> 3] == 0 &&
        utf8_class[cont_byte[2] >> 3] == 0){
     result.codepoint = (byte & bitmask_3) << 18;
     result.codepoint |= ((cont_byte[0] & bitmask_6) << 12);
     result.codepoint |= ((cont_byte[1] & bitmask_6) <<  6);
     result.codepoint |=  (cont_byte[2] & bitmask_6);
     result.inc = 4;
    }
   }
  }break;
 }
 return(result);
}

function u32
utf8_write(u8 *str, u32 codepoint){
 u32 inc = 0;
 if (codepoint <= 0x7F){
  str[0] = (u8)codepoint;
  inc = 1;
 }
 else if (codepoint <= 0x7FF){
  str[0] = (bitmask_2 << 6) | ((codepoint >> 6) & bitmask_5);
  str[1] = bit_8 | (codepoint & bitmask_6);
  inc = 2;
 }
 else if (codepoint <= 0xFFFF){
  str[0] = (bitmask_3 << 5) | ((codepoint >> 12) & bitmask_4);
  str[1] = bit_8 | ((codepoint >> 6) & bitmask_6);
  str[2] = bit_8 | ( codepoint       & bitmask_6);
  inc = 3;
 }
 else if (codepoint <= 0x10FFFF){
  str[0] = (bitmask_4 << 3) | ((codepoint >> 18) & bitmask_3);
  str[1] = bit_8 | ((codepoint >> 12) & bitmask_6);
  str[2] = bit_8 | ((codepoint >>  6) & bitmask_6);
  str[3] = bit_8 | ( codepoint        & bitmask_6);
  inc = 4;
 }
 else{
  str[0] = '?';
  inc = 1;
 }
 return(inc);
}

function b32
string_null_terminate(String_u8 *str)
{
 b32 result = false;
 if (str->size < str->cap){
  str->str[str->size] = 0;
 }
 return(result);
}

#if OS_WINDOWS
struct String_Const_u16
{
 u16 *str;
 u64 size;
};

struct String_u16
{
 union{
  String_Const_u16 string;
  struct{
   u16 *str;
   u64 size;
  };
 };
 u64 cap;
};

function b32
string_match(String_Const_u16 a, String_Const_u16 b)
{
 b32 result = false;
 if (a.size == b.size){
  result = true;
  for (u64 i = 0; i < a.size; i += 1){
   if (a.str[i] != b.str[i]){
    result = false;
    break;
   }
  }
 }
 return(result);
}

function Character_Consume_Result
utf16_consume(u16 *str, u64 max)
{
 Character_Consume_Result result = {1, max_u32};
 result.codepoint = str[0];
 result.inc = 1;
 if (0xD800 <= str[0] && str[0] < 0xDC00 && max > 1 && 0xDC00 <= str[1] && str[1] < 0xE000){
  result.codepoint = ((str[0] - 0xD800) << 10) | (str[1] - 0xDC00);
  result.inc = 2;
 }
 return(result);
}

function u32
utf16_write(u16 *str, u32 codepoint){
 u32 inc = 1;
 if (codepoint == max_u32){
  str[0] = (u16)'?';
 }
 else if (codepoint < 0x10000){
  str[0] = (u16)codepoint;
 }
 else{
  u32 v = codepoint - 0x10000;
  str[0] = 0xD800 + (u16)(v >> 10);
  str[1] = 0xDC00 + (v & bitmask_10);
  inc = 2;
 }
 return(inc);
}


function u64
win32_u64_from_u32_u32(u32 hi, u32 lo)
{
 return( (((u64)hi) << 32) | ((u64)lo) );
}
function u64
win32_u64_from_filetime(FILETIME time)
{
 return(win32_u64_from_u32_u32(time.dwHighDateTime, time.dwLowDateTime));
}
function File_Attribute_Flag
win32_convert_file_attribute_flags(DWORD dwFileAttributes)
{
 File_Attribute_Flag result = {};
 MovFlag(dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY, result, FileAttribute_IsDirectory);
 return(result);
}
function b32
string_null_terminate(String_u16 *str)
{
 b32 result = false;
 if (str->size < str->cap){
  str->str[str->size] = 0;
 }
 return(result);
}
function String_u16
string_u16_from_string_u8(Arena *arena, String string, String_Fill_Terminate_Rule rule)
{
 String_u16 out = {};
 out.cap = string.size;
 if (rule == StringFill_NullTerminate){
  out.cap += 1;
 }
 out.str = push_array(arena, u16, out.cap);
 u8 *ptr = string.str;
 u8 *one_past_last = ptr + string.size;
 u64 cap = string.size;
 Character_Consume_Result consume;
 for (;ptr < one_past_last; ptr += consume.inc, cap -= consume.inc){
  consume = utf8_consume(ptr, cap);
  out.size += utf16_write(out.str + out.size, consume.codepoint);
 }
 if (rule == StringFill_NullTerminate){
  string_null_terminate(&out);
 }
 return(out);
}
function HANDLE
FindFirstFile_utf8(Arena *scratch, u8 *name, LPWIN32_FIND_DATAW find_data)
{
 Temp_Memory temp = begin_temp_memory(scratch);
 String_u16 name_16 = string_u16_from_string_u8(scratch, SCu8(name), StringFill_NullTerminate);
 HANDLE result = FindFirstFileW((LPWSTR)name_16.str, find_data);
 end_temp_memory(temp);
 return(result);
}
function u64
cstring_length(u16 *str)
{
 u64 length = 0;
 for(;str[length] != 0; length += 1);
 return(length);
}
function String_Const_u16
SCu16(u16 *str)
{
 u64 size = cstring_length(str);
 String_Const_u16 string = {str, size};
 return(string);
}

function String_Const_u16
SCu16(u16 *str, u64 size){
 String_Const_u16 string = {str, size};
 return(string);
}
function String_Const_u16
SCu16(wchar_t *str, u64 size){
 return(SCu16((u16*)str, size));
}
function String_Const_u16
SCu16(wchar_t *str){
 return(SCu16((u16*)str));
}

function String_u8
string_u8_from_string_u16(Arena *arena, String_Const_u16 string, String_Fill_Terminate_Rule rule)
{
 String_u8 out = {};
 out.cap = string.size*3;
 if (rule == StringFill_NullTerminate){
  out.cap += 1;
 }
 out.str = push_array(arena, u8, out.cap);
 u16 *ptr = string.str;
 u16 *one_past_last = ptr + string.size;
 u64 cap = string.size;
 Character_Consume_Result consume;
 for (;ptr < one_past_last; ptr += consume.inc, cap -= consume.inc){
  consume = utf16_consume(ptr, cap);
  out.size += utf8_write(out.str + out.size, consume.codepoint);
 }
 if (rule == StringFill_NullTerminate){
  string_null_terminate(&out);
 }
 return(out);
}

function File_List
get_file_list(Arena* arena, String directory)
{// NOTE(kv) #copypasta
 File_List result = {};
 String search_pattern = {};
 if(is_file_slash(string_get_character(directory, directory.size - 1)))
 {
  search_pattern = push_stringf(arena, "%.*s*", string_expand(directory));
 }
 else
 {
  search_pattern = push_stringf(arena, "%.*s\\*", string_expand(directory));
 }
 
 WIN32_FIND_DATAW find_data = {};
 HANDLE search = FindFirstFile_utf8(arena, search_pattern.str, &find_data);
 if (search != INVALID_HANDLE_VALUE)
 {
  File_Info *first = 0;
  File_Info *last = 0;
  i1 count = 0;
  
  for (;;)
  {
   String_Const_u16 filename_utf16 = SCu16(find_data.cFileName);
   if (!(string_match(filename_utf16, string_u16_litexpr(L".")) ||
         string_match(filename_utf16, string_u16_litexpr(L".."))))
   {
    String_u8 filename_ = string_u8_from_string_u16(arena, filename_utf16, StringFill_NullTerminate);
    Stringz filename = Stringz(filename_.string);
    
    File_Info *info = push_array(arena, File_Info, 1);
    sll_queue_push(first, last, info);
    count += 1;
    
    info->filename = filename;
    info->attributes.size = win32_u64_from_u32_u32(find_data.nFileSizeHigh,
                                                   find_data.nFileSizeLow);
    info->attributes.last_write_time = win32_u64_from_filetime(find_data.ftLastWriteTime);
    info->attributes.flags = win32_convert_file_attribute_flags(find_data.dwFileAttributes);
   }
   if (!FindNextFileW(search, &find_data))
   {
    break;
   }
  }
  
  result.infos = push_array(arena, File_Info*, count);
  result.count = count;
  
  i1 counter = 0;
  for(File_Info *node = first;
      node != 0;
      node = node->next)
  {
   result.infos[counter] = node;
   counter += 1;
  }
 }
 
 return(result);
}
#endif
//~EOF