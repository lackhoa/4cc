//-
myinline String
push_data(Arena *arena, u64 size)
{
 String result = {};
 result.str = push_array(arena, u8, size);
 result.size = size;
 return(result);
}
myinline String
push_string(Arena *arena, String data)
{
 Stringz result;
 result.str   = push_array_copy(arena, u8, data.len, data.str);
 result.count = data.len;
 return(result);
}
myinline Stringz
push_string(Arena *arena, Stringz data)
{
 Stringz result;
 result.str   = push_array_copy(arena, u8, data.len+1, data.str);
 result.count = data.len;
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
path_stem(String path){
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
gb_sort(void *base_, isize count, isize size, Compare_Function compare){
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

function u32
gb_murmur32(void const *data, isize len) {
 return gb_murmur32_seed(data, len, 0x9747b28c);
}
function u64
gb_murmur64(void const *data, isize len) {
 return gb_murmur64_seed(data, len, 0x9747b28c);
}
//~EOF