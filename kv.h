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

  todo: let's assert that the our build flags are correct.
*/

#pragma once  // NOTE: #pragma once means that you have to define the
              // implementation at the top of whatever your main file is, since
              // this file ain't gonna apear twice.

#include <stdarg.h>
#include <stddef.h>
#include <float.h>
#include <stdlib.h> // malloc, free
#include <stdio.h>  // printf, perror
#include <cstdint>
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

#define for_i32(VAR, INITIAL, FINAL)  for(i32 VAR=INITIAL; VAR<FINAL; VAR++)
#define for_u32(VAR, INITIAL, FINAL)  for(u32 VAR=INITIAL; VAR<FINAL; VAR++)
#define for_i64(VAR, INITIAL, FINAL)  for(i64 VAR=INITIAL; VAR<FINAL; VAR++)
#define for_u64(VAR, INITIAL, FINAL)  for(u64 VAR=INITIAL; VAR<FINAL; VAR++)

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

#define kv_array_count(array) safeTruncateToInt32(sizeof(array) / sizeof((array)[0]))

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

/*
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
*/

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
#define in_range(bot,mid,top) ((bot) <= (mid) && (mid) < (top))
#define in_between(bot,mid,top) ((bot) <= (mid) && (mid) <= (top))
#define in_range_inclusive in_between

/* ;math */

/* ;scalar */

#define Pi32 3.14159265359f
#define Tau32 6.28318530717958647692f

#define kv_clamp_min(VAR, VAL)   if (VAR < VAL) VAR = VAL
#define kv_clamp_max(VAR, VAL)   if (VAR > VAL) VAR = VAL;

inline f32
toBilateral(f32 r)
{
    return (r * 2.0f) - 1.0f;
}

inline f32
toUnilateral(f32 r)
{
    return (r * 0.5f) + 0.5f;
}

inline f32
lerp(f32 a, f32 t, f32 b)
{
    f32 result;
    result = a + t*(b - a);
    return result;
}

inline f32
unlerp_or_zero(f32 a, f32 v, f32 b)
{
  f32 range = (b - a);
  f32 result = (range != 0.0f) ? ((v - a) / range) : 0.0f;
  return result;
}

/* ;v2 */

union v2 {
  struct {
    f32 x, y;
  };
  f32 E[2];
  f32 v[2];
};

inline v2
toV2(f32 x, f32 y)
{
    v2 result;
    result.x = x;
    result.y = y;
    return result;
}

inline v2
v2All(f32 c)
{
    return toV2(c, c);
}

inline b32
operator==(v2 u, v2 v)
{
    b32 result;
    result = (u.x == v.x) && (u.y == v.y);
    return result;
}

inline b32
operator!=(v2 u, v2 v)
{
    b32 result;
    result = (u.x != v.x) || (u.y != v.y);
    return result;
}

inline v2
operator+(v2 u, v2 v)
{
    v2 result;
    result.x = u.x + v.x;
    result.y = u.y + v.y;
    return result;
}

inline v2 &
operator+=(v2 &v, v2 u)
{
    v = u + v;
    return v;
}

inline v2
operator-(v2 u, v2 v)
{
    v2 result;
    result.x = u.x - v.x;
    result.y = u.y - v.y;
    return result;
}

inline v2
operator-=(v2 &v, v2 u)
{
    v = v - u;
    return v;
}

inline v2
operator-(v2 v)
{
    v2 result;
    result.x = -v.x;
    result.y = -v.y;
    return result;
}

inline v2
operator*(f32 c, v2 v)
{
    v2 result;
    result.x = c * v.x;
    result.y = c * v.y;
    return result;
}

inline v2
operator*(v2 v, f32 c)
{
    v2 result = c*v;
    return result;
}

inline v2 &
operator*=(v2 &v, f32 c)
{
    v = c * v;
    return v;
}

inline v2
operator/(v2 v, f32 c)
{
    v2 result;
    result.x = v.x / c;
    result.y = v.y / c;
    return result;
}

inline f32
dot(v2 v, v2 u)
{
    f32 result = v.x*u.x + v.y*u.y;
    return result;
}

inline f32
lengthSq(v2 v)
{
    f32 result = square(v.x) + square(v.y);
    return result;
}

inline f32
length(v2 v)
{
    f32 result = squareRoot(lengthSq(v));
    return result;
}

inline f32
projectLen(v2 onto, v2 v)
{
    f32 innerProd = dot(onto, v);
    f32 result = (innerProd / length(onto));
    return result;
}

inline v2
project(v2 onto, v2 v)
{
    f32 innerProd = dot(onto, v);
    v2 result = (innerProd / lengthSq(onto)) * onto;
    return result;
}

inline v2
hadamard(v2 v, v2 u)
{
    v2 result;
    result.x = v.x*u.x;
    result.y = v.y*u.y;
    return result;
}

inline v2
normalize(v2 v)
{
    v2 result;
    f32 len = length(v);
    if (len == 0)
    {
        result = toV2(0,0);
    }
    else
    {
        result = v * (1.0f / len);
    }
    return result;
}

inline v2
noz(v2 v)  // normalize or zero
{
  f32 lsq = lengthSq(v);
  v2 result = {};
  if (lsq > square(.0001f)) {
    // prevent the result from getting too big
    result = v * 1.f / squareRoot(lsq);
  }
  return result;
}

inline v2
perp(v2 v)
{
    v2 result = {-v.y, v.x};
    return result;
}

inline v2
toBilateral(v2 v)
{
    v2 result = {toBilateral(v.x), toBilateral(v.y)};
    return result;
}

// ;v3

union v3 {
  struct {
    f32 x, y, z;
  };
  struct {
    f32 r, g, b;
  };
  struct {
    v2 xy;
    f32 ignored;
  };
  f32 E[3];
  f32 v[3];
};

inline v3
toV3(f32 x, f32 y, f32 z)
{
    v3 result = { x, y, z };
    return result;
}

inline v3
toV3(v2 xy, f32 z)
{
    v3 result;
    result.xy = xy;
    result.z = z;
    return result;
}

inline v3
v3All(f32 c)
{
    return toV3(c, c, c);
}

inline v3
v3z(f32 z)
{
    v3 result = {0,0,z};
    return result;
}

inline v3
v3xy(v2 v)
{
    v3 result = {v.x, v.y, 0};
    return result;
}

inline v3
operator-(v3 u, v3 v)
{
    v3 result;
    result.x = u.x - v.x;
    result.y = u.y - v.y;
    result.z = u.z - v.z;
    return result;
}

inline b32
operator<(v3 u, v3 v)
{
    b32 result = ((u.x < v.x) && (u.y < v.y) && (u.z < v.z));
    return result;
}

inline b32
operator<=(v3 u, v3 v)
{
    b32 result = ((u.x <= v.x) && (u.y <= v.y) && (u.z <= v.z));
    return result;
}

inline b32
operator>(v3 u, v3 v)
{
    b32 result = ((u.x > v.x) && (u.y > v.y) && (u.z > v.z));
    return result;
}

inline b32
operator>=(v3 u, v3 v)
{
    b32 result = ((u.x >= v.x) && (u.y >= v.y) && (u.z >= v.z));
    return result;
}

inline b32
operator==(v3 u, v3 v)
{
    b32 result;
    result = (u.x == v.x) && (u.y == v.y) && (u.z == v.z);
    return result;
}

inline b32
operator!=(v3 u, v3 v)
{
    b32 result;
    result = (u.x != v.x) || (u.y != v.y) || (u.z != v.z);
    return result;
}

inline v3
operator+(v3 u, v3 v)
{
    v3 result;
    result.x = u.x + v.x;
    result.y = u.y + v.y;
    result.z = u.z + v.z;
    return result;
}

inline v3 &
operator+=(v3 &v, v3 u)
{
    v = u + v;
    return v;
}

inline v3
operator-=(v3 &v, v3 u)
{
    v = v - u;
    return v;
}


inline v3
operator-(v3 v)
{
    v3 result;
    result.x = -v.x;
    result.y = -v.y;
    result.z = -v.z;
    return result;
}

inline v3
operator*(f32 c, v3 v)
{
    v3 result;
    result.x = c * v.x;
    result.y = c * v.y;
    result.z = c * v.z;
    return result;
}

inline v3
operator*(v3 v, f32 c)
{
    v3 result = c*v;
    return result;
}

inline v3 &
operator*=(v3 &v, f32 c)
{
    v = c * v;
    return v;
}

inline v3
operator/(v3 v, f32 c)
{
    v3 result;
    result.x = v.x / c;
    result.y = v.y / c;
    result.z = v.z / c;
    return result;
}

inline f32
dot(v3 v, v3 u)
{
    f32 result = v.x*u.x + v.y*u.y + v.z*u.z;
    return result;
}

inline v3
cross(v3 v, v3 u)
{
  return v3{v.y*u.z - v.z*u.y,
            v.z*u.x - v.x*u.z,
            v.x*u.y - v.y*u.x};
}

inline v3
hadamard(v3 v, v3 u)
{
    v3 result;
    result.x = v.x*u.x;
    result.y = v.y*u.y;
    result.z = v.z*u.z;
    return result;
}

inline f32
lengthSq(v3 v)
{
    f32 result = square(v.x) + square(v.y) + square(v.z);
    return result;
}

inline f32
length(v3 v)
{
    f32 result = squareRoot(lengthSq(v));
    return result;
}

inline v3
normalize(v3 v)
{
    f32 len = length(v);
    v3 result = v * (1.f / len);
    return result;
}

inline v3
noz(v3 v)  // normalize or zero
{
  f32 lsq = lengthSq(v);
  v3 result = {};
  if (lsq > square(.0001f)) {
    // prevent the result from getting too big
    result = v * 1.f / squareRoot(lsq);
  }
  return result;
}

inline v3
project(v3 onto, v3 v)
{
    f32 innerProd = dot(onto, v);
    v3 result = (innerProd / lengthSq(onto)) * onto;
    return result;
}

inline v3
toBilateral(v3 v)
{
    v3 result;
    result.x = toBilateral(v.x);
    result.y = toBilateral(v.y);
    result.z = toBilateral(v.z);
    return result;
}

// ;v4

union v4 {
  struct {
    f32 x, y, z, w;
  };
  struct {
    f32 r, g, b, a;
  };
  struct{
    f32 h;
    f32 s;
    f32 l;
    f32 __a;
  };
  struct {
    v3 rgb;
    f32 a_ignored;
  };
  struct {
    v3 xyz;
    f32 w_ignored;
  };
  struct {
    v2 xy;
    v2 yz_ignored;
  };
  f32 E[4];
  f32 v[4];
};

typedef v4 Vec4_f32;

inline v4
toV4(v3 xyz, f32 w)
{
    v4 result = { xyz.x, xyz.y, xyz.z , w };
    return result;
}

inline v4
hadamard(v4 u, v4 v)
{
    v4 result;
    result.x = u.x * v.x;
    result.y = u.y * v.y;
    result.z = u.z * v.z;
    result.w = u.w * v.w;
    return result;
}

inline v4
operator*(f32 c, v4 v)
{
    v4 result = {c * v.x, c * v.y, c * v.z, c * v.w};
    return result;
}

inline v4
operator*(v4 v, f32 c)
{
    v4 result = {c * v.x, c * v.y, c * v.z, c * v.w};
    return result;
}

inline v4 &
operator*=(v4 &v, f32 c)
{
    v = c * v;
    return v;
}

inline v4
operator+(v4 u, v4 v)
{
    v4 result = {u.x + v.x, u.y + v.y, u.z + v.z, u.w + v.w};
    return result;
}

inline v4
operator-(v4 u, v4 v)
{
    v4 result = {u.x - v.x, u.y - v.y, u.z - v.z, u.w - v.w};
    return result;
}

inline v4
lerp(v4 a, f32 t, v4 b)
{
    v4 result;
    result = a + t*(b - a);
    return result;
}

// ;rect2

typedef v2 Vec2_f32;

union rect2 {
  struct {
    v2 min;
    v2 max;
  };
  struct{
    f32 x0;
    f32 y0;
    f32 x1;
    f32 y1;
  };
  struct{
    Vec2_f32 p0;
    Vec2_f32 p1;
  };
  Vec2_f32 p[2];
};

inline b32
inRectangle(rect2 rect, v2 point)
{
    return ((point.x >= rect.min.x) && (point.y >= rect.min.y)
            && (point.x < rect.max.x) && (point.y < rect.max.y));
}

inline rect2
rect_get_radius(v2 radius)
{
    return {-radius, radius};
}

inline v2
rect_get_dim(rect2 rect)
{
    return (rect.max - rect.min);
}

inline rect2
rect_center_radius(v2 center, v2 radius)
{
    kv_assert((radius.x >= 0) && (radius.y >= 0));
    rect2 result;
    result.min = center - radius;
    result.max = center + radius;
    return result;
}

inline rect2
rect_center_dim(v2 center, v2 dim)
{
    rect2 result = rect_center_radius(center, 0.5f*dim);
    return result;
}

inline rect2
rect_min_dim(v2 min, v2 dim)
{
  rect2 out = rect2{.min=min, .max=min+dim};
  return out;
}

inline rect2
intersect(rect2 a, rect2 b)
{
    rect2 result;
    result.min.x = maximum(a.min.x, b.min.x);
    result.min.y = maximum(a.min.y, b.min.y);
    result.max.x = minimum(a.max.x, b.max.x);
    result.max.y = minimum(a.max.y, b.max.y);
    return result;
}

//
// ;rect3
//

struct Rect3
{
    v3 min;
    v3 max;
};

inline b32
verifyRect(Rect3 rect)
{
    b32 result = ((rect.min.x <= rect.max.x)
                  && (rect.min.y <= rect.max.y)
                  && (rect.min.z <= rect.max.z));
    return result;
}

inline Rect3
rectRadius(v3 radius)
{
    return {-radius, radius};
}

inline Rect3
rectCenterRadius(v3 center, v3 radius)
{
    kv_assert((radius.x >= 0) && (radius.y >= 0) && (radius.z >= 0));
    Rect3 result;
    result.min = center - radius;
    result.max = center + radius;
    return result;
}

inline b32
inRectangle(Rect3 rect, v3 p)
{
    b32 result = ((p.x >= rect.min.x)
                  && (p.y >= rect.min.y)
                  && (p.z >= rect.min.z)
                  && (p.x < rect.max.x)
                  && (p.y < rect.max.y)
                  && (p.z < rect.max.z));
    return result;
}

inline Rect3
rectCenterDim(v3 center, v3 dim)
{
    Rect3 result = rectCenterRadius(center, 0.5f*dim);
    return result;
}

inline Rect3
rectDim(v3 dim)
{
    Rect3 result = rectCenterDim(v3All(0), dim);
    return result;
}

inline Rect3
minkowskiGrow(Rect3 a, Rect3 b)
{
    Rect3 result;
    v3 bDiameter = b.max - b.min;
    v3 bRadius = 0.5f * bDiameter;
    result.min = a.min - bRadius;
    result.max = a.max + bRadius;
    return result;
}

inline v3
getRectCenter(Rect3 rect)
{
    v3 result;
    result = 0.5f*(rect.min + rect.max);
    return result;
}

inline v3
getRectRadius(Rect3 rect)
{
    v3 result;
    result = 0.5f * (rect.max - rect.min);
    return result;
}

inline b32
rectOverlap(Rect3 a, Rect3 b)
{
    b32 separate = ((a.max.x <= b.min.x) || (a.min.x >= b.max.x)
                    || (a.max.y <= b.min.y) || (a.min.y >= b.max.y)
                    || (a.max.z <= b.min.z) || (a.min.z >= b.max.z));
    return !separate;
}

inline b32
rectInside(Rect3 in, Rect3 out)
{
    b32 result = ((in.min >= out.min) && (in.max <= out.max));
    return result;
}

inline Rect3
addRadiusTo(Rect3 a, v3 radius)
{
    Rect3 result = Rect3{a.min - radius, a.max + radius};
    return result;
}

inline v3
getBarycentricCoordinate(Rect3 rect, v3 pos)
{
    v3 result;
    v3 dim = rect.max - rect.min;
    result.x = ((pos.x - rect.min.x) / dim.x);
    result.y = ((pos.y - rect.min.y) / dim.y);
    result.z = ((pos.z - rect.min.z) / dim.z);
    return result;
}

// ;v2i ;v3i

union v2i{
  struct{
    i32 x;
    i32 y;
  };
  i32 v[2];
};

union v3i{
  struct{
    i32 x;
    i32 y;
    i32 z;
  };
  struct{
    i32 r;
    i32 g;
    i32 b;
  };
  i32 v[3];
};

/* todo: Old names */
#define kvXmalloc      kv_xmalloc
#define kvAssert       kv_assert
typedef kv_Arena KvArena;
/* Old names > */

// NOTE(kv): do the people including this file a favor and not pollute.
#ifdef KV_IMPLEMENTATION
#    undef KV_IMPLEMENTATION
#endif