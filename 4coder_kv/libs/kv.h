/*
  Usage: Define "KV_IMPLEMENTATION" before including this file to get the
  implementation for your compilation unit.

  --------------- Naming -----------------

  To avoid name conflicts, let's just put kv_ in front of names that are short.
  For names that are long and/or unique, it probably wouldn't matter.

  --------------- String -----------------

  Our "String" is are static length-strings, made to work with "kvArena".

  After printing a string to an arena, there is nil-termination, but that might
  be overwritten. You can lock the termination by simply incrementing the arena
  base pointer.

  When converting from C string to our string (using e.g "toString"), always put
  a +1 nil terminator (and keep it there as long as the string can be referenced
  ), because we feel like it's just a conversion, and can be converted back.

  All non-nil-terminated strings must be marked explicitly with "non_nil"

  todo: debate: Actually let's just use C string, because it's more ergonomic.
  Because of printf, concate and all that.
  If we don't have nil terminator, we're screwed when using those standard C funcitons.
  But if we don't use nil terminator, we can't have cheap substrings (which is
  the only attraction).
*/

#pragma once  // NOTE: #pragma once means that you have to define the
              // implementation at the top of whatever your main file is, since
              // this file ain't gonna apear twice.

#include <stdlib.h> // malloc, free
#include <stdio.h>  // printf, perror
#include <stdarg.h>
#include <stddef.h>
#include <cstdint>
#include <float.h>
#include <string.h>

/*
  BEGIN: Other single-header libraries
*/

#ifdef KV_IMPLEMENTATION
#    define STB_DEFINE
#    define STB_DS_IMPLEMENTATION
#    define GB_IMPLEMENTATION
#endif

// NOTE: These header files are supposed to be in the same directory as this file.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"

#    include "stb_ds.h"
#    include "gb.h"

#pragma clang diagnostic pop

#undef STB_DEFINE
#undef STB_DS_IMPLEMENTATION
#undef GB_IMPLEMENTATION

/*
  END: Other single-header libraries
*/











/* Types */
typedef uint8_t  u8;
typedef uint16_t u16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef int8_t   b8;
typedef int32_t  b32;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float    r32;
typedef float    f32;
/* Types: end */

/* Intrinsics */

inline void
zeroMemory(void *dst, size_t size)
{
  // todo #speed
  u8 *dst8 = (u8 *)dst;
  while (size--) {
    *dst8++ = 0;
  }
}

inline void
copyMemory_(void *src, void *dst, size_t size) // source to dest
{
  // todo #speed
  u8 *dst8 = (u8 *)dst;
  u8 *src8 = (u8 *)src;
  while (size--) {
    *dst8++ = *src8++;
  }
}

inline i32
absoslute(i32 in)
{
    return ((in >= 0) ? in : -in);
}

inline f32
square(f32 x)
{
    f32 result = x*x;
    return result;
}

inline f32
squareRoot(f32 x)
{
#if COMPILER_MSVC
    f32 result = sqrtf(x);
#else
    f32 result = __builtin_sqrtf(x);
#endif
    return result;
}

inline u32
roundF32ToU32(f32 Real32)
{
#if COMPILER_MSVC
    u32 Result = (u32)roundf(Real32);
#else
    u32 Result = (u32)__builtin_roundf(Real32);
#endif
    return(Result);
}

inline f32
roundF32(f32 Real32)
{
#if COMPILER_MSVC
    f32 Result = roundf(Real32);
#else
    f32 Result = __builtin_roundf(Real32);
#endif
    return(Result);
}

// todo I don't like this, just do the round myself
// inline i32
// roundF32ToI32(f32 Real32)
// {
// #if COMPILER_MSVC
//     i32 Result = (i32)roundf(Real32);
// #else
//     i32 Result = (i32)__builtin_roundf(Real32);
// #endif
//     return(Result);
// }

inline i32
floorF32ToI32(f32 Real32)
{
#if COMPILER_MSVC
    i32 Result = (i32)floorf(Real32);
#else
    i32 Result = (i32)__builtin_floorf(Real32);
#endif
    return(Result);
}

inline i32
ceilF32ToI32(f32 Real32)
{
#if COMPILER_MSVC
    i32 Result = (i32)ceilf(Real32);
#else
    i32 Result = (i32)__builtin_ceilf(Real32);
#endif
    return(Result);
}

// NOTE: weird names to avoid name collision (haizz)
inline f32
kv_sin(f32 angle)
{
#if COMPILER_MSVC
    f32 result = sinf(angle);
#else
    f32 result = __builtin_sinf(angle);
#endif
    return(result);
}

inline f32
kv_cos(f32 angle)
{
#if COMPILER_MSVC
    f32 result = cosf(angle);
#else
    f32 result = __builtin_cosf(angle);
#endif
    return(result);
}

inline f32
kv_atan2(f32 y, f32 x)
{
#if COMPILER_MSVC
    f32 result = atan2f(y, x);
#else
    f32 result = __builtin_atan2f(y, x);
#endif
    return(result);
}

struct bit_scan_result
{
    b32 found;
    u32 index;
};

inline bit_scan_result
findLeastSignificantSetBit(u32 mask)
{
    bit_scan_result result = {};

#if COMPILER_MSVC
    result.found = _BitScanForward((unsigned long *)&result.index, mask);
#elif COMPILER_LLVM
    if (mask != 0)
    {
        result.found = true;
        result.index = __builtin_ctz(mask);
    }
#else
        for (u32 index = 0;
             index < 32;
             index++)
        {
            if((mask & (1 << index)) != 0)
            {
                result.found = true;
                result.index = index;
                return result;
            }
        }
#endif

    return result;
}


inline f32
absolute(f32 x)
{
#if COMPILER_MSVC
    f32 result = (f32)fabs(x);
#else
    f32 result = (f32)__builtin_fabs(x);
#endif
    return result;
}

inline u32
rotateLeft(u32 value, i32 rotateAmount)
{
#if COMPILER_MSVC
    u32 result = _rotl(value, rotateAmount);
#elif COMPILER_LLVM
    u32 result = __builtin_rotateleft32(value, rotateAmount);
#else
    i32 r = rotateAmount & 31;
    u32 result = (value << r) | (value >> (32 - r));
#endif
    return result;
}

inline u32
rotateRight(u32 value, i32 rotateAmount)
{
#if COMPILER_MSVC
    u32 result = _rotr(value, rotateAmount);
#elif COMPILER_LLVM
    u32 result = __builtin_rotateright32(value, rotateAmount);
#else
    i32 r = rotateAmount & 31;
    u32 result = (value >> r) | (value << (32 - r));
#endif
    return result;
}

#if defined(_WIN32) || defined(_WIN64)
	#ifndef KV_SYSTEM_WINDOWS
	#define KV_SYSTEM_WINDOWS 1
	#endif
#elif defined(__APPLE__) && defined(__MACH__)
	#ifndef KV_SYSTEM_OSX
	#define KV_SYSTEM_OSX 1
	#endif
#elif defined(__unix__)
	#ifndef KV_SYSTEM_UNIX
	#define KV_SYSTEM_UNIX 1
	#endif

	#if defined(__linux__)
		#ifndef KV_SYSTEM_LINUX
		#define KV_SYSTEM_LINUX 1
		#endif
	#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
		#ifndef KV_SYSTEM_FREEBSD
		#define KV_SYSTEM_FREEBSD 1
		#endif
	#else
		#error This UNIX operating system is not supported
	#endif
#else
	#error This operating system is not supported
#endif


/* Intrinsics end */

#include "limits.h"

/*
  Compilers
*/

#if __llvm__
#    undef COMPILER_LLVM
#    define COMPILER_LLVM 1
#else
#    undef COMPILER_MSVC
#    define COMPILER_MSVC 1
#endif

#define UNUSED_VAR __attribute__((unused))
#define unused_var __attribute__((unused))

#define internal        static
#define global_variable UNUSED_VAR static
#define global_constant UNUSED_VAR static
#define local_persist   static

#define kiloBytes(value) ((value)*1024LL)
#define megaBytes(value) (kiloBytes(value)*1024LL)
#define gigaBytes(value) (megaBytes(value)*1024LL)
#define teraBytes(value) (gigaBytes(value)*1024LL)

#if COMPILER_MSVC
#  define debugbreak __debugbreak()
#else
#  define debugbreak __builtin_trap()
#endif

#define kv_assert(claim) do{if (!(claim)) { debugbreak; }} while(0)

#define invalidCodePath kv_assert(false)
#define todoErrorReport kv_assert(false)
#define todoIncomplete  kv_assert(false)
#define todoTestMe      kv_assert(false)
#define todoOutlaw      kv_assert(false)
#define todoUnknown     kv_assert(false)
#define invalidDefaultCase default: { kv_assert(false); };
#define breakhere       { int x = 5; (void)x; }

#if KV_INTERNAL
#    define kv_soft_assert        kv_assert
#    define kv_assert_defend(CLAIM, DEFEND)   kv_assert(CLAIM)
#else
#    define kv_soft_assert(CLAIM)
#    define kv_assert_defend(CLAIM, DEFEND)   if (!(CLAIM))  { DEFEND; }
#endif

#if KV_SLOW
#    define slow_assert kv_assert
#else
#    define slow_assert
#endif

inline i32 safeTruncateToInt32(u64 value)
{
  // NOTE: this is not really "safe" but what are you gonna do
  kv_assert(value < INT_MAX);
  return (i32)value;
}

#define arrayCount(array) safeTruncateToInt32(sizeof(array) / sizeof((array)[0]))
#define arrayLength arrayCount
#define array_count arrayCount

// source: https://groups.google.com/g/comp.std.c/c/d-6Mj5Lko_s
#define PP_NARG(...) PP_NARG_(__VA_ARGS__,PP_RSEQ_N())
#define PP_NARG_(...) PP_ARG_N(__VA_ARGS__)
#define PP_ARG_N(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,N,...) N
#define PP_RSEQ_N() 16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0

#define CONCATENATE(arg1, arg2)   CONCATENATE1(arg1, arg2)
#define CONCATENATE1(arg1, arg2)  CONCATENATE2(arg1, arg2)
#define CONCATENATE2(arg1, arg2)  arg1##arg2

#define DUMP_1(x) dump(x)
#define DUMP_2(x, ...) dump(x); DUMP_1(__VA_ARGS__)
#define DUMP_3(x, ...) dump(x); DUMP_2(__VA_ARGS__)
#define DUMP_4(x, ...) dump(x); DUMP_3(__VA_ARGS__)
#define DUMP_5(x, ...) dump(x); DUMP_4(__VA_ARGS__)
#define DUMP_6(x, ...) dump(x); DUMP_5(__VA_ARGS__)
#define DUMP_7(x, ...) dump(x); DUMP_6(__VA_ARGS__)
#define DUMP_8(x, ...) dump(x); DUMP_7(__VA_ARGS__)
#define DUMP_9(x, ...) dump(x); DUMP_8(__VA_ARGS__)
#define DUMP_N(N) CONCATENATE(DUMP_, N)
#define DUMP_NO_NEWLINE(...) DUMP_N(PP_NARG(__VA_ARGS__))(__VA_ARGS__)
#define DUMP(...) DUMP_NO_NEWLINE(__VA_ARGS__, "\n")
// DUMP(a,b) -> DUMP_N(2,a,b)(a,b) -> DUMP_2()

inline void
zeroSize(void *base, size_t size)
{
    u8 *ptr = (u8 *) base;
    while(size--)
        *ptr++ = 0;
}

#define zeroStruct(base, type) zeroSize(base, sizeof(type));
#define zeroOut(base) zeroSize(base, sizeof(base))

struct kv_Arena
{
  u8     *base;
  size_t  used;
  size_t  cap;

  // support backward push
  size_t original_cap;

  i32 temp_count;
};

inline kv_Arena
newArena(void *base, size_t cap)
{
    kv_Arena arena = {};
    arena.cap          = cap;
    arena.base         = (u8 *)base;
    arena.original_cap = cap;
    return arena;
}

inline size_t
getArenaFree(kv_Arena &arena)
{
    size_t out = arena.cap - arena.used;
    return out;
}

inline void *
pushSize(kv_Arena &arena, size_t size, b32 zero = false)
{
  void *out = arena.base + arena.used;
  arena.used += size;
  kv_assert(arena.used <= arena.cap);
  if (zero) zeroSize(out, size);
  return(out);
}

inline void *
pushSizeBackward(kv_Arena &arena, size_t size)
{
  arena.cap -= size;
  kv_assert(arena.used <= arena.cap);
  void *out = arena.base + arena.cap;
  return(out);
}

// NOTE: Apparently ##__VA_ARGS__ is the thing
#define pushStruct(arena, type, ...)    (type *) pushSize(arena, sizeof(type), ##__VA_ARGS__)
#define pushStructBackward(arena, type) (type *) pushSizeBackward(arena, sizeof(type))
#define pushArray(arena, count, type)   (type *) pushSize(arena, (count)*sizeof(type))
#define allocate(arena, x, ...) x = (mytypeof(x)) pushSize(arena, sizeof(*x), ##__VA_ARGS__)
#define allocateArray(arena, count, x, ...) x = (mytypeof(x)) pushSize(arena, (count)*sizeof(*x), ##__VA_ARGS__)

#define pushItems_1(array, index, item) array[index] = item;
#define pushItems_2(array, index, item, ...) array[index] = item; pushItems_1(array, index+1, ##__VA_ARGS__);
#define pushItems_3(array, index, item, ...) array[index] = item; pushItems_2(array, index+1, ##__VA_ARGS__);
#define pushItems_N(N, ...) CONCATENATE(pushItems_, N)

#define pushItems(arena, array, item, ...)     \
  array = (mytypeof(item) *) pushArray(arena, PP_NARG(item, ##__VA_ARGS__), mytypeof(item)); \
  pushItems_N(PP_NARG(item, ##__VA_ARGS__), ##__VA_ARGS__)(array, 0, item, ##__VA_ARGS__)

#define pushItemsAs(...) \
  auto pushItems(##__VA_ARGS__)

inline kv_Arena
subArena(kv_Arena &parent, size_t size)
{
  u8 *base = (u8 *)pushSize(parent, size);
  kv_Arena result = newArena(base, size);
  return result;
}

inline kv_Arena
subArenaWithRemainingMemory(kv_Arena &parent)
{
    kv_Arena result = {};
    auto size = parent.cap - parent.used;
    result.base = (u8 *)pushSize(parent, size);
    result.cap  = size;
    return result;
}

struct TempMemoryMarker
{
    kv_Arena  &arena;
    size_t  original_used;
};


inline TempMemoryMarker
beginTemporaryMemory(kv_Arena &arena)
{
  TempMemoryMarker out = {arena, arena.used};
  arena.temp_count++;
  return out;
}

inline void
endTemporaryMemory(TempMemoryMarker temp)
{
  temp.arena.temp_count--;
  kv_assert_defend(temp.arena.used >= temp.original_used, 
                   printf("Memory leak detected!\n"));
  temp.arena.used = temp.original_used;
}

inline void
commitTemporaryMemory(TempMemoryMarker temp)
{
  temp.arena.temp_count--;
}

inline void
checkArena(kv_Arena *arena)
{
    kv_assert(arena->temp_count == 0);
}

inline void
resetArena(kv_Arena &arena, b32 zero=false)
{
  arena.used = 0;
  if (zero) {
    zeroMemory(arena.base, arena.cap);
  }
}

inline void *
pushCopySize(kv_Arena &arena, void *src, size_t size)
{
  void *dst = pushSize(arena, size);
  copyMemory_(src, dst, size);
  return dst;
}

#if COMPILER_MSVC
#    define mytypeof decltype
#else
#    define mytypeof __typeof__
#endif

#define pushCopy(arena, src) (mytypeof(src)) pushCopySize(arena, (src), sizeof(*(src)))
/* #define copyStructNoCast(arena, src) copySize(arena, src, sizeof(*(src))) */
#define pushCopyArray(arena, count, src) (mytypeof(src)) pushCopySize(arena, (src), count*sizeof(*(src)))

inline u8 *getNext(kv_Arena &buffer)
{
  return (buffer.base + buffer.used);
}

/* MARK: String */

struct String
{
  char *chars;
  i32   length;                 // note: does not include the nil terminator
  operator bool() {return chars;}
};

inline i32
stringLength(char *string)
{
    i32 out = 0;
    char *c = string;
    while (*c)
    {
        out++;
        c++;
    }
    return out;
}

typedef kv_Arena StringBuffer;

struct StartString {
  StringBuffer &arena;
  char         *chars;
};

inline StartString
startString(kv_Arena &arena)
{
  char *start = (char *)getNext(arena);
  return {.arena=arena, .chars=start};
};

inline String
endString(StartString start)
{
  String out = {};
  out.chars = start.chars;
  out.length = (i32)((char*)getNext(start.arena) - start.chars);
  int zero = 0;
  pushCopy(start.arena, &zero);
  return out;
}

inline b32
equal(String s, const char *cstring)
{
  if (!s.chars)
  {
    return false;
  }
  else
  {
    i32 at = 0;
    for (;
         at < s.length;
         at++)
    {
      if ((cstring[at] == 0) || (s.chars[at] != cstring[at]))
      {
        return false;
      }
    }
    return (cstring[at] == 0);
  }
}

inline b32
stringEqual(const char *cstring, String s)
{
  return equal(s, cstring);
}

inline b32
stringEqual(String a, String b)
{
    b32 out = true;
    if (a.length != b.length)
        out = false;
    else
    {
        for (int i = 0; i < a.length; i++)
        {
            if (a.chars[i] != b.chars[i])
            {
                out = false;
                break;
            }
        }
    }
    return out;
}

inline b32
stringEqual(String a, char c)
{
  return a.length == 1 && a.chars[0] == c;
}

inline String
toString(const char *c)
{
  String out;
  out.chars  = (char*)c;
  out.length = 0;
  while (*c++) {
    out.length++;
  }
  return out;
}

inline String
toString(kv_Arena &arena, const char *c)
{
  String out = {};
  out.chars = (char *)getNext(arena);
  char *dst = out.chars;
  while ((*dst++ = *c++)) {
    out.length++;
  }
  *dst = '\0';
  pushSize(arena, out.length+1);
  return out;
}

inline char *
toCString(kv_Arena &arena, String string)
{
  char *out = (char *)pushCopySize(arena, string.chars, string.length+1);
  u8 *next = getNext(arena) - 1;
  *next = 0;
  return out;
}

// NOTE: "temporary" means that your string nil terminator might be stomped on in the future
// Useful for passing the string to fopen or something.
inline char *
toCStringTemporary(String string)
{
  kv_assert(*(string.chars + string.length) == 0);
  return string.chars;
}

inline String
printVA(kv_Arena &buffer, char *format, va_list arg_list)
{
  char *at = (char *)getNext(buffer);
  int printed = vsnprintf(at, (buffer.cap - buffer.used), format, arg_list);
  buffer.used += printed;
  return String{at, printed};
}

extern String
print(kv_Arena &buffer, char *format, ...)
#ifdef KV_IMPLEMENTATION
{
  String out = {};

  va_list arg_list;
  va_start(arg_list, format);

  out.chars = (char *)getNext(buffer);
  auto printed = vsnprintf(out.chars, (buffer.cap-1 - buffer.used), format, arg_list);
  buffer.used += printed;
  out.length   = printed;
  buffer.base[buffer.used] = 0; // nil-termination

  va_end(arg_list);

  return out;
}
#else
;
#endif

inline String
print(kv_Arena &buffer, String s)
{
  String out = {};
  {
    out.chars = (char *)getNext(buffer);
    char *at = out.chars;
    const char *c = s.chars;
    for (i32 index = 0; index < s.length; index++)
      *at++ = *c++;
    *at = 0;
    out.length = (i32)(at - out.chars);
    buffer.used += out.length;
    kv_assert(buffer.used <= buffer.cap);  // todo: don't crash!
  }

  return out;
}

// todo #cleanup same as "inArena"?
inline b32
belongsToArena(kv_Arena *arena, u8 *memory)
{
  return ((memory >= arena->base) && (memory < arena->base + arena->cap));
}

#define maximum(a, b) ((a < b) ? b : a)
#define minimum(a, b) ((a < b) ? a : b)

// Metaprogramming tags
#define forward_declare(FILE_NAME)
#define function_typedef(FILE_NAME)

inline char
toLowerCase(char c)
{
  if (('a' <= c) && (c <= 'z'))
    return c - 32;
  return c;
}

inline b32
isSubstring(String full, String sub, b32 case_sensitive=true)
{
  b32 out = true;
  if (sub.length > full.length)
  {
    out = false;
  }
  else
  {
    for (i32 id = 0;
         id < sub.length;
         id++)
    {
      char s = sub.chars[id];
      char f = full.chars[id];
      b32 mismatch = case_sensitive ? (s != f) : (toLowerCase(s) != toLowerCase(f));
      if (mismatch)
      {
        out = false;
        break;
      }
    }
  }
  return out;
}

inline String
copyString(kv_Arena &buffer, String src)
{
  String out;
  out.chars  = pushCopyArray(buffer, src.length, src.chars);
  out.length = src.length;
  return out;
}

inline void dump() {printf("\n");}
inline void dump(int d) {printf("%d", d);}
inline void dump(char *c) {printf("%s", c);}
inline void dump(String s) {printf("%.*s", s.length, s.chars);}

inline String
concatenate(kv_Arena &arena, String a, String b)
{
  auto string = startString(arena);
  print(string.arena, a);
  print(string.arena, b);
  String out = endString(string);
  return out;
}

inline String
concatenate(kv_Arena &arena, String a, char *b)
{
  return concatenate(arena, a, toString(b));
}

inline String
concatenate(String a, String b)
{
  i32 length = a.length + b.length;
  char *chars = (char *)malloc(a.length + b.length + 1);
  if (!chars)
  {
    return String{};
  }
  strncpy(chars, a.chars, a.length);
  strncpy(chars + a.length, b.chars, b.length);
  chars[length] = 0;
  return String{chars, length};
}

/* MARK: End of String */

inline b32
inArena(kv_Arena &arena, void *p)
{
  return ((u64)p >= (u64)arena.base && (u64)p < (u64)arena.base+arena.cap);
}

inline b32
checkFlag(u32 flags, u32 flag)
{
  return flags & flag;
}

inline void
setFlag(u32 *flags, u32 flag)
{
  *flags |= flag;
}

inline void
unsetFlag(u32 *flags, u32 flag)
{
  *flags &= ~flag;
}

#define SWAP(a, b) { \
    auto temp = a; \
    a = b; \
    b = temp; \
}

#define llPush(arena, member, list)            \
  mytypeof(list) new_list = pushStruct(arena, mytypeof(*list)); \
  new_list->head          = member;             \
  new_list->tail          = list;               \
  list                    = new_list;

#if 0
// defer macro from https://www.gingerbill.org/article/2015/08/19/defer-in-cpp/
template <typename F>
struct privDefer {
	F f;
	privDefer(F f) : f(f) {}
	~privDefer() { f(); }
};

template <typename F>
privDefer<F> defer_func(F f) {
	return privDefer<F>(f);
}

#define DEFER_1(x, y) x##y
#define DEFER_2(x, y) DEFER_1(x, y)
#define DEFER_3(x)    DEFER_2(x, __COUNTER__)
#define defer(code)   auto DEFER_3(_defer_) = defer_func([&](){code;})
// end defer macro //////////////////
#endif

#define EAT_TYPE(POINTER, TYPE) (TYPE *)(POINTER += sizeof(TYPE), POINTER - sizeof(TYPE))

#define DLL_EXPORT extern "C" __attribute__((visibility("default")))


inline void *kv_xmalloc(size_t size) {
  void *ptr = malloc(size);
  if (!ptr) {
    perror("kv_xmalloc failed");
    exit(1);
  }
  return ptr;
}

#define breakable_block for (i32 __kv_breakable_block__=0; __kv_breakable_block__ == 0; __kv_breakable_block__++)

/* todo: Old names */
#define kvXmalloc      kv_xmalloc
#define kvAssert       kv_assert
typedef kv_Arena KvArena;
/* Old names > */
