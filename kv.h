#include "kv_core.h"

force_inline u32
AtomicAddU32AndReturnOriginal(u32 volatile *Value, u32 Addend)
{
 // NOTE(casey): Returns the original value _prior_ to adding
 u32 Result = _InterlockedExchangeAdd((long volatile*)Value, (long)Addend);
 return(Result);
}


////////////////////////////////

struct Scratch_Block
{
 Thread_Context *tctx;
 Arena *arena;
 Temp_Memory temp;
 
 Scratch_Block(struct Thread_Context *tctx);
 Scratch_Block(struct Thread_Context *tctx, Arena *a1);
 Scratch_Block(struct Thread_Context *tctx, Arena *a1, Arena *a2);
 Scratch_Block(struct Thread_Context *tctx, Arena *a1, Arena *a2, Arena *a3);
 Scratch_Block(struct App *app);
 Scratch_Block(struct App *app, Arena *a1);
 Scratch_Block(struct App *app, Arena *a1, Arena *a2);
 Scratch_Block(struct App *app, Arena *a1, Arena *a2, Arena *a3);
 Scratch_Block(Arena *arena);
 ~Scratch_Block();
 operator Arena*();
 void restore(void);
};

inline Scratch_Block::Scratch_Block(Thread_Context *t)
{
 this->tctx = t;
 this->arena = tctx_reserve(t);
 this->temp = begin_temp(this->arena);
}

inline Scratch_Block::Scratch_Block(Thread_Context *t, Arena *a1){
 this->tctx = t;
 this->arena = tctx_reserve(t, a1);
 this->temp = begin_temp(this->arena);
}

inline Scratch_Block::Scratch_Block(Arena *arena)
{
 this->tctx = 0;
 this->arena = arena;
 this->temp = begin_temp(this->arena);
}

inline Scratch_Block::Scratch_Block(Thread_Context *t, Arena *a1, Arena *a2){
 this->tctx = t;
 this->arena = tctx_reserve(t, a1, a2);
 this->temp = begin_temp(this->arena);
}

inline Scratch_Block::Scratch_Block(Thread_Context *t, Arena *a1, Arena *a2, Arena *a3){
 this->tctx = t;
 this->arena = tctx_reserve(t, a1, a2, a3);
 this->temp = begin_temp(this->arena);
}

inline Scratch_Block::~Scratch_Block()
{
 end_temp(this->temp);
 if (this->tctx)
 {
  tctx_release(this->tctx, this->arena);
 }
}

force_inline Scratch_Block::operator Arena*(){
 return(this->arena);
}

inline void
Scratch_Block::restore(void){
 end_temp(this->temp);
}




///////////////////////////////////

internal void*
base_reserve__malloc(void *user_data, u64 size, u64 *size_out, String location)
{
 *size_out = size;
 return(malloc((size_t)size));
}

internal void
base_free__malloc(void *user_data, void *ptr){
 free(ptr);
}

internal Base_Allocator
make_malloc_base_allocator(void){
 return(make_base_allocator(base_reserve__malloc, 0, 0,
                            base_free__malloc, 0, 0));
}

global Base_Allocator malloc_base_allocator = {};

internal Base_Allocator*
get_allocator_malloc(void){
 if (malloc_base_allocator.reserve == 0){
  malloc_base_allocator = make_malloc_base_allocator();
 }
 return(&malloc_base_allocator);
}

internal Arena
make_arena_malloc(u64 chunk_size, u64 align){
 Base_Allocator *allocator = get_allocator_malloc();
 return(make_arena(allocator, chunk_size, align));
}

internal Arena
make_arena_malloc(u64 chunk_size){
 return(make_arena_malloc(chunk_size, 8));
}

internal Arena
make_arena_malloc(void){
 return(make_arena_malloc(KB(16), 8));
}

// IMPORTANT(kv): This is for my use only, it can't grow so don't pass it around
internal Arena
make_static_arena(u8 *memory, u64 size, u64 alignment=8)
{
 local_persist Base_Allocator noop_allocator = make_base_allocator(0,0,0,0,0,0);
 
 Arena result = {};
 result.cursor_node    = make_cursor_node_from_memory(memory, size);
 result.alignment      = alignment;
 result.base_allocator = &noop_allocator;
 return result;
}

internal Arena
sub_arena_static(Arena *arena, usize size, u64 alignment=8)
{
 u8 *memory = push_array(arena, u8, size);
 Arena result = make_static_arena(memory, size, alignment);
 return result;
}

internal String
arena_to_string(Arena *arena)
{
 Cursor *cursor = get_arena_cursor(arena);
 String result = String{cursor->base, cursor->pos};
 return result;
}

//~

force_inline String
SCu8(u8 *str){
 u64 size = cstring_length(str);
 String string = {str, size};
 return(string);
}

force_inline String
SCu8(char *str){
 return(SCu8((u8*)str));
}

function String
push_string_copyz(Arena *arena, String src)
{
 String string = {};
 string.str = push_array(arena, u8, src.size + 1);
 string.size = src.size;
 block_copy_count(string.str, src.str, src.size);
 string.str[string.size] = 0;
 return(string);
}
//
force_inline char *
to_c_string(Arena *arena, String8 string)
{
 String8 result = push_string_copyz(arena, string);
 return (char *)result.str;
}

typedef i32 Scan_Direction;
enum
{
 Scan_Backward = -1,
 Scan_Forward  =  1,
};


//~

// NOTE: You could also use a stack buffer 
// if you don't need to work with a parent arena,
// but you might need to copy the result somewhere
struct Printer
{
 Arena arena;
 operator Arena*() { return &arena; };
};
//
force_inline Printer
make_printer(Arena *arena, int cap)
{
 Printer result;
 result.arena = sub_arena_static(arena, cap, /*alignment*/1);
 return result;
}
// NOTE: For string concatenation
function String
end_printer(Printer *printer)
{// TODO: Leak memory!
 Cursor *cursor = get_arena_cursor(&printer->arena);
 String string = {
  .str  = cursor->base,
  .size = cursor->pos,
 };
 return string;
}

function u64
string_find_first_non_whitespace(String str){
 u64 i = 0;
 for (;i < str.size && character_is_whitespace(str.str[i]); i += 1);
 return(i);
}

internal String
string_skip_whitespace(String str)
{
 u64 f = string_find_first_non_whitespace(str);
 str = string_skip(str, f);
 return(str);
}

internal String
push_stringf(Arena *arena, char *format, ...)
{
 va_list args;
 va_start(args, format);
 String result = push_stringfv(arena, format, args, false);
 va_end(args);
 return(result);
}

internal String
push_stringfz(Arena *arena, char *format, ...)
{
 va_list args;
 va_start(args, format);
 String result = push_stringfv(arena, format, args, true);
 va_end(args);
 return(result);
}

inline String8
to_string(Arena *arena, i32 value)
{
 return push_stringfz(arena, "%d", value);
}

//~

#if OS_WINDOWS
#define OS_SLASH '\\'
#else
#define SLASH '/'
#endif

internal String8
pjoin(Arena *arena, String8 a, String8 b)
{
 String8 result = push_stringfz(arena, "%.*s%c%.*s", string_expand(a), OS_SLASH, string_expand(b));
 return result;
}

internal String8
pjoin(Arena *arena, String8 a, const char *b)
{
 String8 result = push_stringfz(arena, "%.*s%c%s", string_expand(a), OS_SLASH, b);
 return result;
}


force_inline b32
move_file(char const *from_filename, char const *to_filename)
{
 return gb_file_move(from_filename, to_filename);
}

inline b32
move_file(String from_filename, String to_filename)
{
 u8 buffer[512];
 Arena arena = make_static_arena(buffer, 512);
 char *from = to_c_string(&arena, from_filename);
 char *to = to_c_string(&arena, to_filename);
 return gb_file_move(from, to);
}

inline b32 
copy_file(String from_filename, String to_filename, b32 fail_if_exists)
{
 u8 buffer[512];
 Arena arena = make_static_arena(buffer, 512);
 char *from = to_c_string(&arena, from_filename);
 char *to = to_c_string(&arena, to_filename);
 return gb_file_copy(from, to, fail_if_exists);
}

inline b32
remove_file(String filename)
{
 u8 buffer[512];
 Arena arena = make_static_arena(buffer, 512);
 char *filenamec = to_c_string(&arena, filename);
 return gb_file_remove(filenamec);
}




// NOTE(kv): Just fopen, but let's keep this so we can switch it out later.
internal FILE *
open_file(String name, char *mode)
{
 make_temp_arena(scratch);
 String namez = push_string_copyz(scratch, name);
 FILE *file = fopen((char*)namez.str, mode);
 return(file);
}

internal b32
write_entire_file(String filename, void *data, u64 size)
{
 make_temp_arena(arena);
 
 b32 result = false;
 gbFile file_value = {}; gbFile *file = &file_value;
 char *filename_c = to_c_string(arena, filename);
 gbFileError err = gb_file_open_mode(file, gbFileMode_Write, filename_c);
 if (err == gbFileError_None)
 {
  result = gb_file_write(file, data, size);
 }
 gb_file_close(file);
 return result;
}
//
force_inline b32
write_entire_file(String filename, String data)
{
 return write_entire_file(filename, data.str, data.size);
}

function String
read_entire_file_handle(Arena *arena, FILE *file)
{
 String result = {};
 if (file != 0)
 {
  fseek(file, 0, SEEK_END);
  u64 size = ftell(file);
  char *mem = push_array(arena, char, size+1);
  if (mem != 0)
  {
   fseek(file, 0, SEEK_SET);
   fread(mem, 1, (size_t)size, file);
   result = make_data(mem, size);
   mem[size] = 0;// NOTE: null-termination
  }
 }
 return(result);
}

internal String
read_entire_file(Arena *arena, String filename)
{
 String result = {};
 FILE *file = open_file(filename, "rb");
 if (file != 0)
 {
  result = read_entire_file_handle(arena, file);
  fclose(file);
 }
 return(result);
}

force_inline char *
read_entire_file_cstring(Arena *arena, String filename)
{// NOTE: Files are null-terminated so we're fine
 String result = read_entire_file(arena, filename);
 return (char *)result.str;
}


//~