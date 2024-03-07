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
  
  TODO: Let's assert that the our build flags are correct.
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



#define DLL_EXPORT extern "C" __attribute__((visibility("default")))
#define EXTERN_C_BEGIN extern "C" {
#define EXTERN_C_END   }


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


#define internal static
#define local_persist static
#define global static



/* Types */
typedef uint8_t  u8;
typedef uint16_t u16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef int8_t   b8;
typedef int32_t  b32;
typedef uint32_t u32;
typedef uint64_t u64;
typedef size_t   umm;
typedef i64      imm;

typedef float    r32;
typedef float    f32;
typedef float    v1;
/* Types: end */

#define for_i32(VAR, INITIAL, FINAL)  for(i32 VAR=INITIAL; VAR<FINAL; VAR++)
#define for_u32(VAR, INITIAL, FINAL)  for(u32 VAR=INITIAL; VAR<FINAL; VAR++)
#define for_i64(VAR, INITIAL, FINAL)  for(i64 VAR=INITIAL; VAR<FINAL; VAR++)
#define for_u64(VAR, INITIAL, FINAL)  for(u64 VAR=INITIAL; VAR<FINAL; VAR++)

/* Intrinsics */

inline void
block_zero(void *mem, u64 size)
{
    gb_zero_size(mem, size);
}

inline void
block_fill_ones(void *mem, u64 size)
{
    gb_memset(mem, 0xff, size);
}


// todo: Do we really need overlap handling?
internal void
block_copy(void *dst, const void *src, u64 size)
{
#if 1
    gb_memmove(dst, src, size);
#else  // NOTE: old code
    u8 *d = (u8*)dst;
    u8 *s = (u8*)src;
    if (d < s)
    {
        u8 *e = d + size;
        for (; d < e; d += 1, s += 1) *d = *s;
    }
    else if (d > s)
    {
        u8 *e = d;
        d += size - 1;
        s += size - 1;
        for (; d >= e; d -= 1, s -= 1) *d = *s;
    }
#endif
}

inline i32
absoslute(i32 in)
{
    return ((in >= 0) ? in : -in);
}

inline f32
squared(f32 x)
{
    f32 result = x*x;
    return result;
}

inline f32 
cubed(f32 value)
{
    return value*value*value;
}

inline f32
square_root(f32 x)
{
#if COMPILER_MSVC
    f32 result = sqrtf(x);
#else
    f32 result = __builtin_sqrtf(x);
#endif
    return result;
}

// TODO: These are real bad! should only be one simd instruction. Watch hmh 379 (or something)
inline f32
round_f32(f32 Real32)
{
#if COMPILER_MSVC
    f32 Result = roundf(Real32);
#else
    f32 Result = __builtin_roundf(Real32);
#endif
    return(Result);
}

inline f32
floor_f32(f32 value)
{
#if COMPILER_MSVC
    f32 Result = floorf(value);
#else
    f32 Result = __builtin_floorf(value);
#endif
    return(Result);
}

inline f32
ceil_f32(f32 value)
{
#if COMPILER_MSVC
    f32 Result = ceilf(value);
#else
    f32 Result = __builtin_ceilf(value);
#endif
    return(Result);
}

inline f32
cycle01(f32 value)
{
    f32 result = value - floor_f32(value);
    return result;
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

inline i32
absolute(i32 x)
{
#if COMPILER_MSVC
#error "unimplemented"
#else
    i32 result = abs(x);
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

// NOTE(kv): Don't enable this warning!
// #define UNUSED_VAR __attribute__((unused))
// #define unused_var __attribute__((unused))

typedef uintptr_t uptr;
typedef intptr_t  iptr;

#define kiloBytes(value) ((value)*1024LL)
#define megaBytes(value) (kiloBytes(value)*1024LL)
#define gigaBytes(value) (megaBytes(value)*1024LL)
#define teraBytes(value) (gigaBytes(value)*1024LL)

#if COMPILER_MSVC
#  define kv_fail __debugbreak()
#else
#  define kv_fail __builtin_trap()
#endif

#define kv_assert(claim) do{if (!(claim)) { kv_fail; }} while(0)

#define invalid_code_path   kv_fail
#define nono                kv_fail  // ignore_nono

#if KV_INTERNAL
#    define fail_in_debug  kv_fail
#else
#    define fail_in_debug
#endif

#define todo_test_me        fail_in_debug
#define todo_testme         fail_in_debug
#define todo_untested       fail_in_debug
#define todo_error_report   fail_in_debug
#define todo_incomplete     fail_in_debug
#define kv_debug_trap       fail_in_debug

#define invalid_default_case default: { kv_fail; };
#define breakhere       do{ int x = 5; (void)x; }while(0)

#if KV_INTERNAL
#    define soft_assert                  kv_assert
#    define assert_defend(CLAIM, DEFEND) kv_assert(CLAIM)
#else
#    define soft_assert(CLAIM)
#    define assert_defend(CLAIM, DEFEND)   if (!(CLAIM))  { DEFEND; }
#endif

#define alert_if_false soft_assert

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

#define kv_array_count(array)  (isize)(sizeof(array) / sizeof((array)[0]))
#define arlen kv_array_count
#define alen kv_array_count

// source: https://groups.google.com/g/comp.std.c/c/d-6Mj5Lko_s
#define PP_NARG(...) PP_NARG_(__VA_ARGS__,PP_RSEQ_N())
#define PP_NARG_(...) PP_ARG_N(__VA_ARGS__)
#define PP_ARG_N(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,N,...) N
#define PP_RSEQ_N() 16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0

#define CONCATENATE(arg1, arg2)   CONCATENATE1(arg1, arg2)
#define CONCATENATE1(arg1, arg2)  CONCATENATE2(arg1, arg2)
#define CONCATENATE2(arg1, arg2)  arg1##arg2

#define PPConcat CONCATENATE

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
  assert_defend(temp.arena.used >= temp.original_used, 
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

// todo #cleanup same as "inArena"?
inline b32
belongsToArena(kv_Arena *arena, u8 *memory)
{
  return ((memory >= arena->base) && (memory < arena->base + arena->cap));
}

#define macro_min(a, b) ((a < b) ? a : b)
#define macro_max(a, b) ((a < b) ? b : a)
#define minimum  macro_min // @ Deprecated
#define maximum  macro_max // @ Deprecated

// Metaprogramming tags
#define forward_declare(FILE_NAME)
#define function_typedef(FILE_NAME)

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

#define macro_swap(a, b) { \
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

#define PI32  3.14159265359f
#define TAU32 6.28318530717958647692f

#define macro_clamp_bot(VAR, VAL)   if (VAR < VAL) VAR = VAL
#define macro_clamp_top(VAR, VAL)   if (VAR > VAL) VAR = VAL;
// @Deprecated
#define kv_clamp_bot macro_clamp_bot
#define kv_clamp_top macro_clamp_top

inline f32
bilateral(f32 r)
{
    return (r * 2.0f) - 1.0f;
}

inline f32
unilateral(f32 r)
{
    return (r * 0.5f) + 0.5f;
}

inline i32
unilateral(i32 r)
{
    return (r + 1) / 2;
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

// NOTE: This can't be its own type because C++ doesn't allow us to convert float to v1, because they don't (know how to) programm, at all.
// They don't even think. Because what good is it to allow conversion from user-defined type to built-in type, but not the revers?

/* ;v2 */

union v2
{
    struct 
    {
        f32 x;
        f32 y;
    };
    f32 E[2];
    f32 v[2];
};

inline v2 v2_all(f32 input)
{
    return v2{input, input};
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
    f32 result = squared(v.x) + squared(v.y);
    return result;
}

inline f32
length(v2 v)
{
    f32 result = square_root(lengthSq(v));
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
        result = v2{};
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
    if (lsq > squared(0.0001f))
    {
        // prevent the result from getting too big
        result = v * 1.f / square_root(lsq);
    }
    return result;
}

inline v2
perp(v2 v)
{
    v2 result = v2{-v.y, v.x};
    return result;
}

inline v2
bilateral(v2 v)
{
    v2 result = {bilateral(v.x), bilateral(v.y)};
    return result;
}

// ;v3

union v3 
{
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
    
    f32 operator[](i32);
};

inline f32 v3::operator[](i32 index)
{
    return v[index];
}

inline v3
V3(v2 xy, f32 z)
{
    v3 result;
    result.xy = xy;
    result.z = z;
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
    f32 result = squared(v.x) + squared(v.y) + squared(v.z);
    return result;
}

inline f32
length(v3 v)
{
    f32 result = square_root(lengthSq(v));
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
    if (lsq > squared(0.0001f)) 
    {
        // prevent the result from getting too big
        result = v * 1.f / square_root(lsq);
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
bilateral(v3 v)
{
    v3 result;
    result.x = bilateral(v.x);
    result.y = bilateral(v.y);
    result.z = bilateral(v.z);
    return result;
}

// ;v4

union v4 
{
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
    v1 xyz_w;
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

inline v3 &
operator+=(v3 &v, v3 u)
{
    v = u + v;
    return v;
}

inline v2 &
operator+=(v2 &v, v2 u)
{
    v = u + v;
    return v;
}

inline v4 &
operator+=(v4 &v, v4 u)
{
    v = u + v;
    return v;
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
    v4 v4_value;
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
rect_dim(rect2 rect)
{
    return (rect.max - rect.min);
}

inline rect2
rect2_center_radius(v2 center, v2 radius)
{
    kv_assert((radius.x >= 0) && (radius.y >= 0));
    rect2 result;
    result.min = center - radius;
    result.max = center + radius;
    return result;
}

inline rect2
rect2_center_dim(v2 center, v2 dim)
{
    rect2 result = rect2_center_radius(center, 0.5f*dim);
    return result;
}

inline rect2
rect2_min_dim(v2 min, v2 dim)
{
  rect2 out = rect2{.min=min, .max=min+dim};
  return out;
}

inline rect2
rect2_min_max(v2 min, v2 max)
{
    rect2 result = rect2{min, max};
    return result;
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
rect3_get_dim(v3 dim)
{
    Rect3 result = rectCenterDim(v3{}, dim);
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
  struct{ i32 x, y, z; };
  struct{ i32 r, g, b; };
  struct{
    v2i xy;
  };
  i32 v[3];
};

/* todo: Old names */
#define kvXmalloc    kv_xmalloc
#define kvAssert     kv_assert
typedef kv_Arena     KvArena;
/* Old names > */

#define v2_expand(v) v.x, v.y
#define v3_expand(v) v.x, v.y, v.z
#define v4_expand(v) v.x, v.y, v.z, v.w

// X macros //////////////////////////////////
#define XTypedef(N,R,P) typedef R N##_type P;
#define XPointer(N,R,P)        N##_type *N;
#define XGlobalPointer(N,R,P)  global N##_type *N;
#define XInternalFunction(N,R,P)  internal N##_type N;


// Bitmap //////////////////////////////////////

struct Bitmap 
{
  u32 *data;
  v2i  dim;
  i32  pitch;
};

internal v4
linearToSrgb(v4 linear)
{
    v4 result;
    result.r = square_root(linear.r);
    result.g = square_root(linear.g);
    result.b = square_root(linear.b);
    result.a = linear.a;
    return result;
}

internal u32
pack_sRGBA(v4 color)
{
  // linear to srgb
  color.r = square_root(color.r);
  color.g = square_root(color.g);
  color.b = square_root(color.b);
  u32 result = ((u32)(color.a*255.0f + 0.5f) << 24
                | (u32)(color.b*255.0f + 0.5f) << 16
                | (u32)(color.g*255.0f + 0.5f) << 8
                | (u32)(color.r*255.0f + 0.5f));
  return result;
}

union m3x3
{
    v3 columns[3];
    struct
    {
        v3 x;
        v3 y;
        v3 z;
    };
};

union m4x4
{
    v4 columns[4];
    v1 v[4][4];
    struct
    {
        v4 x,y,z,w;
    };
};

internal v3 
matvmul3(m3x3 *matrix, v3 v)
{
    v3 result = (v.x * matrix->columns[0] + 
                 v.y * matrix->columns[1] + 
                 v.z * matrix->columns[2]);
    return result;
}

internal f32
pow(f32 input, u32 power)
{
    f32 result = 1; 
    for_u32 (index, 0, power)
    {
        result *= input;
    }
    return result;
}

//////////////////////////////////////////////////

// 4coder_base_types.h

#if defined(__clang__)

# define COMPILER_CLANG 1

# if defined(__APPLE__) && defined(__MACH__)
#  define OS_MAC 1
# elif defined(_WIN32)
#  define OS_WINDOWS 1
# else
#  error This compiler/platform combo is not supported yet
# endif

# if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64)
#  define ARCH_X64 1
# elif defined(i386) || defined(__i386) || defined(__i386__)
#  define ARCH_X86 1
# elif defined(__aarch64__)
#  define ARCH_ARM64 1
# elif defined(__arm__)
#  define ARCH_ARM32 1

#elif defined(_MSC_VER)

# define COMPILER_CL 1

# if defined(_WIN32)
#  define OS_WINDOWS 1
# else
#  error This compiler/platform combo is not supported yet
# endif

# if defined(_M_AMD64)
#  define ARCH_X64 1
# elif defined(_M_IX86)
#  define ARCH_X86 1
# elif defined(_M_ARM64)
#  define ARCH_ARM64 1
# elif defined(_M_ARM)
#  define ARCH_ARM32 1
# else
#  error architecture not supported yet
# endif

# else
#  error architecture not supported yet
# endif

#elif defined(__GNUC__) || defined(__GNUG__)

# define COMPILER_GCC 1

# if defined(__gnu_linux__)
#  define OS_LINUX 1
# else
#  error This compiler/platform combo is not supported yet
# endif

# if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64)
#  define ARCH_X64 1
# elif defined(i386) || defined(__i386) || defined(__i386__)
#  define ARCH_X86 1
# elif defined(__aarch64__)
#  define ARCH_ARM64 1
# elif defined(__arm__)
#  define ARCH_ARM32 1
# else
#  error architecture not supported yet
# endif

#else
# error This compiler is not supported yet
#endif

#if defined(ARCH_X64)
# define ARCH_64BIT 1
#elif defined(ARCH_X86)
# define ARCH_32BIT 1

#endif

// zeroify

#if !defined(ARCH_32BIT)
#define ARCH_32BIT 0
#endif
#if !defined(ARCH_64BIT)
#define ARCH_64BIT 0
#endif
#if !defined(ARCH_X64)
#define ARCH_X64 0
#endif
#if !defined(ARCH_X86)
#define ARCH_X86 0
#endif
#if !defined(ARCH_ARM64)
#define ARCH_ARM64 0
#endif
#if !defined(ARCH_ARM32)
#define ARCH_ARM32 0
#endif
#if !defined(COMPILER_CL)
#define COMPILER_CL 0
#endif
#if !defined(COMPILER_GCC)
#define COMPILER_GCC 0
#endif
#if !defined(COMPILER_CLANG)
#define COMPILER_CLANG 0
#endif
#if !defined(OS_WINDOWS)
#define OS_WINDOWS 0
#endif
#if !defined(OS_LINUX)
#define OS_LINUX 0
#endif
#if !defined(OS_MAC)
#define OS_MAC 0
#endif

#if !defined(SHIP_MODE)
#    define SHIP_MODE 0
#endif

// names

#if COMPILER_CL
# define COMPILER_NAME "cl"
#elif COMPILER_CLANG
# define COMPILER_NAME "clang"
#elif COMPILER_GCC
# define COMPILER_NAME "gcc"
#else
# error no name for this compiler
#endif

#if OS_WINDOWS
# define OS_NAME "win"
#elif OS_LINUX
# define OS_NAME "linux"
#elif OS_MAC
# define OS_NAME "mac"
#else
# error no name for this operating system
#endif

#if ARCH_X86
# define ARCH_NAME "x86"
#elif ARCH_X64
# define ARCH_NAME "x64"
#elif ARCH_ARM64
# define ARCH_NAME "arm64"
#elif ARCH_ARM32
# define ARCH_NAME "arm32"
#else
# error no name for this architecture
#endif

////////////////////////////////

#if COMPILER_CL
# if _MSC_VER <= 1800
#  define snprintf _snprintf
# endif

# if (_MSC_VER <= 1500)
#  define JUST_GUESS_INTS
# endif
#endif

// NOTE(yuval): Changed this so that CALL_CONVENTION will be defined for all platforms
#if ARCH_32BIT && OS_WINDOWS
# define CALL_CONVENTION __stdcall
#else
# define CALL_CONVENTION
#endif

#if defined(JUST_GUESS_INTS)
typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
#else
#include <stdint.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
#endif

typedef i8 b8;
typedef i32 b32;
typedef i64 b64;

typedef float f32;
typedef double f64;

typedef void Void_Func(void);

typedef i32 Generated_Group;
enum{
  GeneratedGroup_Core,
  GeneratedGroup_Custom
};

#define glue_(a,b) a##b
#define glue(a,b) glue_(a,b)

#define stringify_(a) #a
#define stringify(a) stringify_(a)

#if COMPILER_CL
#define __VA_OPT__(x)
#endif

#define function static
#define api(x)

#define local_const  static const
#define global_const static const
#define external extern "C"  // TODO(kv): @Cleanup wtf removeme?

#define ArrayCount(a) ((sizeof(a))/(sizeof(*a)))

// TODO(kv): temporary
#define ArrayCountSigned(a)  (isize)ArrayCount(a)

#define ArraySafe(a,i) ((a)[(i)%ArrayCount(a)])
#define ExpandArray(a) (a), (ArrayCount(a))
#define FixSize(s) struct{ u8 __size_fixer__[s]; }

#define PtrDif(a,b) ((u8*)(a) - (u8*)(b))

// @Experiment(kv) : Pointer to integer exploits
//
// NOTE(kv): compiler doesn't like this version
// #define PtrAsInt(a) PtrDif(a,0)
// #define IntAsPtr(a) (void*)( ((u8*)0) + a )
//
#define PtrAsInt(a) (uptr)(a)
#define IntAsPtr(a) (void*)((uptr)(a))


#define HandleAsU64(a) (u64)(PtrAsInt(a))
#define Member(S,m) (((S*)0)->m)
#define NullMember(S,m) (&Member(S,m))
// #define OffsetOfMember(S,m) PtrAsInt(&Member(S,m))
#define OffsetOfMemberStruct(s,m) PtrDif(&(s)->m, (s))
#define SizeAfterMember(S,m) (sizeof(S) - gb_offset_of(S,m))
#define CastFromMember(S,m,ptr) (S*)( (u8*)(ptr) - gb_offset_of(S,m) )

#define Stmnt(s) do{ s }while(0)

// NOTE(allen): Assert notes:
// Break = the run time implementation of break
//                - replace this to get fancier behavior on assert
// Always = assert that is not compiled out in SHIP_MODE
//                - helpful for debugging specific issues
//                - used rarely in normal code
// Message = unconditional asserts with an attached message
//                - InvalidPath version for paths of a switch or if-else dispatch that should always be unreachable
//                - NotImplemented version for stubs functions that are not yet completed
// Static = asserts that contain only compile time constants and become compilation errors
// Disambiguate = for static asserts that happen to have name conflicts

#define AssertBreak(m) kv_fail
#define AssertAlways(c) Stmnt( if (!(c)) { AssertBreak(c); } )
#define AssertMessageAlways(m) AssertBreak(m)
#define StaticAssertDisambiguateAlways(c,d) char glue(__ignore__, glue(__LINE__, d))[(c)?1:-1];
#define StaticAssertAlways(c) StaticAssertDisambiguateAlways(c,__default__)

#if !SHIP_MODE
#define Assert(c) AssertAlways(c)
#define AssertMessage(m) AssertMessageAlways(m)
#define StaticAssertDisambiguate(c,d) StaticAssertDisambiguateAlways(c,d)
#define StaticAssert(c) StaticAssertAlways(c)
#else
#define Assert(c)
#define AssertMessage(m)
#define StaticAssertDisambiguate(c,d)
#define StaticAssert(c)
#endif

#define AssertImplies(a,b) Assert(!(a) || (b))
#define InvalidPath AssertMessage("invalid path")
#define NotImplemented AssertMessage("not implemented")
#define DontCompile NoSeriouslyDontCompile

#define  B(x)  (x)
#define KB(x) ((x) << 10)
#define MB(x) ((x) << 20)
#define GB(x) ((x) << 30)
#define TB(x) (((u64)x) << 40)

#define Thousand(x) ((x)*1000)
#define Million(x)  ((x)*1000000)
#define Billion(x)  ((x)*1000000000)

#define HasFlag(fi,fl) (((fi)&(fl))!=0)
#define HasAllFlag(fi,fl) (((fi)&(fl))==(fl))
#define AddFlag(fi,fl) ((fi)|=(fl))
#define RemFlag(fi,fl) ((fi)&=(~(fl)))
#define MovFlag(fi1,fl1,fi2,fl2) ((HasFlag(fi1,fl1))?(AddFlag(fi2,fl2)):(fi2))

#define Swap(t,a,b) do { t glue(hidden_temp_,__LINE__) = a; a = b; b = glue(hidden_temp_,__LINE__); } while(0)

#define div_round_up_positive_(n,d) (n + d - 1)/d
#define div_round_up_positive(n,d) (div_round_up_positive_((n),(d)))

#define DrCase(PC) case PC: goto resumespot_##PC
#define DrYield(PC, n) { *S_ptr = S; S_ptr->__pc__ = PC; return(n); resumespot_##PC:; }
#define DrReturn(n) { *S_ptr = S; S_ptr->__pc__ = -1; return(n); }

#define Max(a,b) (((a)>(b))?(a):(b))
#define Min(a,b) (((a)<(b))?(a):(b))
#define clamp_top(a,b) Min(a,b)
#define clamp_bot(a,b) Max(a,b)
#define clamp_between_(a,x,b) ((a>x) ? a : ((b<x) ? b : x))
#define clamp_between(a,x,b)  clamp_between_((a),(x),(b))

#define array_initr(a) {(a), ArrayCount(a)}

global_const u8 max_u8 = 0xFF;
global_const u16 max_u16 = 0xFFFF;
global_const u32 max_u32 = 0xFFFFFFFF;
global_const u64 max_u64 = 0xFFFFFFFFFFFFFFFF;

global_const i8 max_i8 = 127;
global_const i16 max_i16 = 32767;
global_const i32 max_i32 = 2147483647;
global_const i64 max_i64 = 9223372036854775807;

global_const i8 min_i8   = -127 - 1;
global_const i16 min_i16 = -32767 - 1;
global_const i32 min_i32 = -2147483647 - 1;
global_const i64 min_i64 = -9223372036854775807 - 1;

global_const f32 max_f32 = 3.402823466e+38f;
global_const f32 min_f32 = -max_f32;
global_const f32 smallest_positive_f32 = 1.1754943508e-38f;
global_const f32 epsilon_f32 = 5.96046448e-8f;

global_const f32 pi_f32 = 3.14159265359f;
global_const f32 half_pi_f32 = 1.5707963267f;

global_const f64 max_f64 = 1.79769313486231e+308;
global_const f64 min_f64 = -max_f64;
global_const f64 smallest_positive_f64 = 4.94065645841247e-324;
global_const f64 epsilon_f64 = 1.11022302462515650e-16;

#define clamp_signed_to_i8(x) (i8)(clamp((i64)i8_min, (i64)(x), (i64)i8_max))
#define clamp_signed_to_i16(x) (i16)(clamp((i64)i16_min, (i64)(x), (i64)i16_max))
#define clamp_signed_to_i32(x) (i32)(clamp((i64)i32_min, (i64)(x), (i64)i32_max))
#define clamp_signed_to_i64(x) (i64)(clamp((i64)i64_min, (i64)(x), (i64)i64_max))
#define clamp_unsigned_to_i8(x) (i8)(clamp_top((u64)(x), (u64)i8_max))
#define clamp_unsigned_to_i16(x) (i16)(clamp_top((u64)(x), (u64)i16_max))
#define clamp_unsigned_to_i32(x) (i32)(clamp_top((u64)(x), (u64)i32_max))
#define clamp_unsigned_to_i64(x) (i64)(clamp_top((u64)(x), (u64)i64_max))
#define clamp_signed_to_u8(x) (u8)(clamp_top((u64)clamp_bot(0, (i64)(x)), (u64)u8_max))
#define clamp_signed_to_u16(x) (u16)(clamp_top((u64)clamp_bot(0, (i64)(x)), (u64)u16_max))
#define clamp_signed_to_u32(x) (u32)(clamp_top((u64)clamp_bot(0, (i64)(x)), (u64)u32_max))
#define clamp_signed_to_u64(x) (u64)(clamp_top((u64)clamp_bot(0, (i64)(x)), (u64)u64_max))
#define clamp_unsigned_to_u8(x) (u8)(clamp_top((u64)(x), (u64)u8_max))
#define clamp_unsigned_to_u16(x) (u16)(clamp_top((u64)(x), (u64)u16_max))
#define clamp_unsigned_to_u32(x) (u32)(clamp_top((u64)(x), (u64)u32_max))
#define clamp_unsigned_to_u64(x) (u64)(clamp_top((u64)(x), (u64)u64_max))

#define line_number_as_string stringify(__LINE__)
#define filename_line_number __FILE__ ":" line_number_as_string ":"

#define macro_require(c) Stmnt( if (!(c)){ return(0); } )

////////////////////////////////

global_const u32 bit_1  = 0x00000001;
global_const u32 bit_2  = 0x00000002;
global_const u32 bit_3  = 0x00000004;
global_const u32 bit_4  = 0x00000008;
global_const u32 bit_5  = 0x00000010;
global_const u32 bit_6  = 0x00000020;
global_const u32 bit_7  = 0x00000040;
global_const u32 bit_8  = 0x00000080;
global_const u32 bit_9  = 0x00000100;
global_const u32 bit_10 = 0x00000200;
global_const u32 bit_11 = 0x00000400;
global_const u32 bit_12 = 0x00000800;
global_const u32 bit_13 = 0x00001000;
global_const u32 bit_14 = 0x00002000;
global_const u32 bit_15 = 0x00004000;
global_const u32 bit_16 = 0x00008000;
global_const u32 bit_17 = 0x00010000;
global_const u32 bit_18 = 0x00020000;
global_const u32 bit_19 = 0x00040000;
global_const u32 bit_20 = 0x00080000;
global_const u32 bit_21 = 0x00100000;
global_const u32 bit_22 = 0x00200000;
global_const u32 bit_23 = 0x00400000;
global_const u32 bit_24 = 0x00800000;
global_const u32 bit_25 = 0x01000000;
global_const u32 bit_26 = 0x02000000;
global_const u32 bit_27 = 0x04000000;
global_const u32 bit_28 = 0x08000000;
global_const u32 bit_29 = 0x10000000;
global_const u32 bit_30 = 0x20000000;
global_const u32 bit_31 = 0x40000000;
global_const u32 bit_32 = 0x80000000;

global_const u64 bit_33 = 0x0000000100000000;
global_const u64 bit_34 = 0x0000000200000000;
global_const u64 bit_35 = 0x0000000400000000;
global_const u64 bit_36 = 0x0000000800000000;
global_const u64 bit_37 = 0x0000001000000000;
global_const u64 bit_38 = 0x0000002000000000;
global_const u64 bit_39 = 0x0000004000000000;
global_const u64 bit_40 = 0x0000008000000000;
global_const u64 bit_41 = 0x0000010000000000;
global_const u64 bit_42 = 0x0000020000000000;
global_const u64 bit_43 = 0x0000040000000000;
global_const u64 bit_44 = 0x0000080000000000;
global_const u64 bit_45 = 0x0000100000000000;
global_const u64 bit_46 = 0x0000200000000000;
global_const u64 bit_47 = 0x0000400000000000;
global_const u64 bit_48 = 0x0000800000000000;
global_const u64 bit_49 = 0x0001000000000000;
global_const u64 bit_50 = 0x0002000000000000;
global_const u64 bit_51 = 0x0004000000000000;
global_const u64 bit_52 = 0x0008000000000000;
global_const u64 bit_53 = 0x0010000000000000;
global_const u64 bit_54 = 0x0020000000000000;
global_const u64 bit_55 = 0x0040000000000000;
global_const u64 bit_56 = 0x0080000000000000;
global_const u64 bit_57 = 0x0100000000000000;
global_const u64 bit_58 = 0x0200000000000000;
global_const u64 bit_59 = 0x0400000000000000;
global_const u64 bit_60 = 0x0800000000000000;
global_const u64 bit_61 = 0x1000000000000000;
global_const u64 bit_62 = 0x2000000000000000;
global_const u64 bit_63 = 0x4000000000000000;
global_const u64 bit_64 = 0x8000000000000000;

global_const u32 bitmask_1  = 0x00000001;
global_const u32 bitmask_2  = 0x00000003;
global_const u32 bitmask_3  = 0x00000007;
global_const u32 bitmask_4  = 0x0000000f;
global_const u32 bitmask_5  = 0x0000001f;
global_const u32 bitmask_6  = 0x0000003f;
global_const u32 bitmask_7  = 0x0000007f;
global_const u32 bitmask_8  = 0x000000ff;
global_const u32 bitmask_9  = 0x000001ff;
global_const u32 bitmask_10 = 0x000003ff;
global_const u32 bitmask_11 = 0x000007ff;
global_const u32 bitmask_12 = 0x00000fff;
global_const u32 bitmask_13 = 0x00001fff;
global_const u32 bitmask_14 = 0x00003fff;
global_const u32 bitmask_15 = 0x00007fff;
global_const u32 bitmask_16 = 0x0000ffff;
global_const u32 bitmask_17 = 0x0001ffff;
global_const u32 bitmask_18 = 0x0003ffff;
global_const u32 bitmask_19 = 0x0007ffff;
global_const u32 bitmask_20 = 0x000fffff;
global_const u32 bitmask_21 = 0x001fffff;
global_const u32 bitmask_22 = 0x003fffff;
global_const u32 bitmask_23 = 0x007fffff;
global_const u32 bitmask_24 = 0x00ffffff;
global_const u32 bitmask_25 = 0x01ffffff;
global_const u32 bitmask_26 = 0x03ffffff;
global_const u32 bitmask_27 = 0x07ffffff;
global_const u32 bitmask_28 = 0x0fffffff;
global_const u32 bitmask_29 = 0x1fffffff;
global_const u32 bitmask_30 = 0x3fffffff;
global_const u32 bitmask_31 = 0x7fffffff;

global_const u64 bitmask_32 = 0x00000000ffffffff;
global_const u64 bitmask_33 = 0x00000001ffffffff;
global_const u64 bitmask_34 = 0x00000003ffffffff;
global_const u64 bitmask_35 = 0x00000007ffffffff;
global_const u64 bitmask_36 = 0x0000000fffffffff;
global_const u64 bitmask_37 = 0x0000001fffffffff;
global_const u64 bitmask_38 = 0x0000003fffffffff;
global_const u64 bitmask_39 = 0x0000007fffffffff;
global_const u64 bitmask_40 = 0x000000ffffffffff;
global_const u64 bitmask_41 = 0x000001ffffffffff;
global_const u64 bitmask_42 = 0x000003ffffffffff;
global_const u64 bitmask_43 = 0x000007ffffffffff;
global_const u64 bitmask_44 = 0x00000fffffffffff;
global_const u64 bitmask_45 = 0x00001fffffffffff;
global_const u64 bitmask_46 = 0x00003fffffffffff;
global_const u64 bitmask_47 = 0x00007fffffffffff;
global_const u64 bitmask_48 = 0x0000ffffffffffff;
global_const u64 bitmask_49 = 0x0001ffffffffffff;
global_const u64 bitmask_50 = 0x0003ffffffffffff;
global_const u64 bitmask_51 = 0x0007ffffffffffff;
global_const u64 bitmask_52 = 0x000fffffffffffff;
global_const u64 bitmask_53 = 0x001fffffffffffff;
global_const u64 bitmask_54 = 0x003fffffffffffff;
global_const u64 bitmask_55 = 0x007fffffffffffff;
global_const u64 bitmask_56 = 0x00ffffffffffffff;
global_const u64 bitmask_57 = 0x01ffffffffffffff;
global_const u64 bitmask_58 = 0x03ffffffffffffff;
global_const u64 bitmask_59 = 0x07ffffffffffffff;
global_const u64 bitmask_60 = 0x0fffffffffffffff;
global_const u64 bitmask_61 = 0x1fffffffffffffff;
global_const u64 bitmask_62 = 0x3fffffffffffffff;
global_const u64 bitmask_63 = 0x7fffffffffffffff;

////////////////////////////////

struct Node{
  Node *next;
  Node *prev;
};
union SNode{
  SNode *next;
  SNode *prev;
};

#define dll_init_sentinel_NP_(s,next,prev) s->next=s,s->prev=s
#define dll_insert_NP_(p,n1,n2,next,prev) n2->next=p->next,n1->prev=p,p->next->prev=n2,p->next=n1
#define dll_remove_NP_(n1,n2,next,prev) n2->next->prev=n1->prev,n1->prev->next=n2->next,n2->next=n1->prev=0

#define dll_init_sentinel_(s) dll_init_sentinel_NP_(s,next,prev)
#define dll_insert_(p,n) dll_insert_NP_(p,n,n,next,prev)
#define dll_insert_multiple_(p,n1,n2) dll_insert_NP_(p,n1,n2,next,prev)
#define dll_insert_back_(p,n) dll_insert_NP_(p,n,n,prev,next)
#define dll_insert_multiple_back_(p,n1,n2) dll_insert_NP_(p,n2,n1,prev,next)
#define dll_remove_(n) dll_remove_NP_(n,n,next,prev)
#define dll_remove_multiple_(n1,n2) dll_remove_NP_(n1,n2,next,prev)

#define dll_init_sentinel(s) (dll_init_sentinel_((s)))
#define dll_insert(p,n) (dll_insert_((p),(n)))
#define dll_insert_multiple(p,n1,n2) (dll_insert_multiple_((p),(n1),(n2)))
#define dll_insert_back(p,n) (dll_insert_back_((p),(n)))
#define dll_insert_multiple_back(p,n1,n2) (dll_insert_multiple_back_((p),(n1),(n2)))
#define dll_remove(n) (dll_remove_((n)))
#define dll_remove_multiple(n1,n2) (dll_remove_multiple_((n1),(n2)))

#define sll_stack_push_(h,n) n->next=h,h=n
#define sll_stack_pop_(h) h=h=h->next
#define sll_queue_push_multiple_(f,l,ff,ll) if(ll){if(f){l->next=ff;}else{f=ff;}l=ll;l->next=0;}
#define sll_queue_push_(f,l,n) sll_queue_push_multiple_(f,l,n,n)
#define sll_queue_pop_(f,l) if (f==l) { f=l=0; } else { f=f->next; }

#define sll_stack_push(h,n) (sll_stack_push_((h),(n)))
#define sll_stack_pop(h) (sll_stack_pop_((h)))
#define sll_queue_push_multiple(f,l,ff,ll) Stmnt( sll_queue_push_multiple_((f),(l),(ff),(ll)) )
// NOTE(kv): pretty sure "queue_push" means "push_last"
#define sll_queue_push(f,l,n) Stmnt( sll_queue_push_((f),(l),(n)) )
#define sll_queue_pop(f,l) Stmnt( sll_queue_pop_((f),(l)) )

#define zdll_push_back_NP_(f,l,n,next,prev) ((f==0)?(n->next=n->prev=0,f=l=n):(n->prev=l,n->next=0,l->next=n,l=n))
#define zdll_remove_back_NP_(f,l,next,prev) ((f==l)?(f=l=0):(l->prev->next=0,l=l->prev))
#define zdll_remove_NP_(f,l,n,next,prev)       \
((l==n)?(zdll_remove_back_NP_(f,l,next,prev))  \
:(f==n)?(zdll_remove_back_NP_(l,f,prev,next)) \
:       (dll_remove_NP_(n,n,next,prev)))

#define zdll_push_back(f,l,n) zdll_push_back_NP_((f),(l),(n),next,prev)
#define zdll_push_front(f,l,n) zdll_push_back_NP_((l),(f),(n),prev,next)
#define zdll_remove_back(f,l) zdll_remove_back_NP_((f),(l),next,prev)
#define zdll_remove_front(f,l) zdll_remove_back_NP_((l),(f),prev,next)
#define zdll_remove(f,l,n) zdll_remove_NP_((f),(l),(n),next,prev)

#define zdll_assert_good(T,f) Stmnt( if (f != 0){ Assert(f->prev == 0); \
for(T *p_ = f; p_ != 0; p_ = p_->next){ Assert(p_->prev == 0 || p_->prev->next == p_); Assert(p_->next == 0 || p_->next->prev == p_); }  } )

////////////////////////////////

union Vec2_i8{
  struct{
    i8 x;
    i8 y;
  };
  i8 v[2];
};
union Vec3_i8{
  struct{
    i8 x;
    i8 y;
    i8 z;
  };
  struct{
    i8 r;
    i8 g;
    i8 b;
  };
  i8 v[3];
};
union Vec4_i8{
  struct{
    i8 x;
    i8 y;
    i8 z;
    i8 w;
  };
  struct{
    i8 r;
    i8 g;
    i8 b;
    i8 a;
  };
  i8 v[4];
};
union Vec2_i16{
  struct{
    i16 x;
    i16 y;
  };
  i16 v[2];
};
union Vec3_i16{
  struct{
    i16 x;
    i16 y;
    i16 z;
  };
  struct{
    i16 r;
    i16 g;
    i16 b;
  };
  i16 v[3];
};
union Vec4_i16{
  struct{
    i16 x;
    i16 y;
    i16 z;
    i16 w;
  };
  struct{
    i16 r;
    i16 g;
    i16 b;
    i16 a;
  };
  i16 v[4];
};

typedef v2i Vec2_i32;
typedef v3i Vec3_i32;

union Vec4_i32{
  struct{
    i32 x;
    i32 y;
    i32 z;
    i32 w;
  };
  struct{
    i32 r;
    i32 g;
    i32 b;
    i32 a;
  };
  i32 v[4];
};

typedef v3 Vec3_f32;

union Range_i32{
  struct{
    i32 min;
    i32 max;
  };
  struct{
    i32 start;
    i32 end;
  };
  struct{
    i32 first;
    i32 one_past_last;
  };
};
union Range_i64{
  struct{
    i64 min;
    i64 max;
  };
  struct{
    i64 start;
    i64 end;
  };
  struct{
    i64 first;
    i64 one_past_last;
  };
};
union Range_u64{
  struct{
    u64 min;
    u64 max;
  };
  struct{
    u64 start;
    u64 end;
  };
  struct{
    u64 first;
    u64 one_past_last;
  };
};
union Range_f32{
  struct{
    f32 min;
    f32 max;
  };
  struct{
    f32 start;
    f32 end;
  };
  struct{
    f32 first;
    f32 one_past_last;
  };
};

struct Range_i32_Array{
  Range_i32 *ranges;
  i32 count;
};
struct Range_i64_Array{
  Range_i64 *ranges;
  i32 count;
};
struct Range_u64_Array{
  Range_u64 *ranges;
  i32 count;
};
struct Range_f32_Array{
  Range_f32 *ranges;
  i32 count;
};

union rect2i {
    struct{
        i32 x0;
        i32 y0;
        i32 x1;
        i32 y1;
    };
    struct{
        Vec2_i32 p0;
        Vec2_i32 p1;
    };
    struct{
        Vec2_i32 min;
        Vec2_i32 max;
    };
    Vec2_i32 p[2];
};

typedef rect2i Rect_i32;
typedef rect2 Rect_f32;

union Rect_f32_Pair{
  struct{
    Rect_f32 a;
    Rect_f32 b;
  };
  struct{
    Rect_f32 min;
    Rect_f32 max;
  };
  struct{
    Rect_f32 e[2];
  };
};

typedef u32 ARGB_Color;

////////////////////////////////

struct i8_Array{
  i8 *vals;
  i32 count;
};
struct i16_Array{
  i16 *vals;
  i32 count;
};
struct i32_Array{
  i32 *vals;
  i32 count;
};
struct i64_Array{
  i64 *vals;
  i32 count;
};

struct u8_Array{
  u8 *vals;
  i32 count;
};
struct u16_Array{
  u16 *vals;
  i32 count;
};
struct u32_Array{
  u32 *vals;
  i32 count;
};
struct u64_Array{
  u64 *vals;
  i32 count;
};

////////////////////////////////

typedef i32 String_Fill_Terminate_Rule;
enum{
  StringFill_NoTerminate = 0,
  StringFill_NullTerminate = 1,
};
typedef u32 String_Separator_Flag;
enum{
  StringSeparator_NoFlags = 0,
};
enum{
  StringSeparator_BeforeFirst = 1,
  StringSeparator_AfterLast = 2,
};
typedef i32 String_Match_Rule;
enum{
  StringMatch_Exact = 0,
  StringMatch_CaseInsensitive = 1,
};

struct String_Const_char{
  char *str;
  u64 size;
};

struct String
{
    u8 *str;
    union
    {
        u64 size;
        u64 len;
        u64 length;
    };
};
typedef String String8;  // @Deprecated
typedef String String_Const_u8;

struct String_Const_u16{
  u16 *str;
  u64 size;
};
struct String_Const_u32{
  u32 *str;
  u64 size;
};

struct String_Const_char_Array{
  union{
    String_Const_char *strings;
    String_Const_char *vals;
  };
  i32 count;
};
struct String_Const_u8_Array{
  union{
    String_Const_u8 *strings;
    String_Const_u8 *vals;
  };
  i32 count;
};
typedef String_Const_u8_Array String8_Array;
struct String_Const_u16_Array{
  union{
    String_Const_u16 *strings;
    String_Const_u16 *vals;
  };
  i32 count;
};
struct String_Const_u32_Array{
  union{
    String_Const_u32 *strings;
    String_Const_u32 *vals;
  };
  i32 count;
};

typedef String_Const_u8_Array String8Array;

typedef i32 String_Encoding;
enum{
  StringEncoding_ASCII = 0,
  StringEncoding_UTF8  = 1,
  StringEncoding_UTF16 = 2,
  StringEncoding_UTF32 = 3,
};

struct String_Const_Any{
  String_Encoding encoding;
  union{
    struct{
      void *str;
      u64 size;
    };
    String_Const_char s_char;
    String_Const_u8 s_u8;
    String_Const_u16 s_u16;
    String_Const_u32 s_u32;
  };
};

#define str8_lit(s) {(u8*)(s), sizeof(s) - 1}
#define string_litinit(s) {(u8*)(s), sizeof(s) - 1}
#define string_u8_litinit(s) {(u8*)(s), sizeof(s) - 1}

struct Node_String_Const_char{
  Node_String_Const_char *next;
  String_Const_char string;
};
struct Node_String_Const_u8{
  Node_String_Const_u8 *next;
  String_Const_u8 string;
};
struct Node_String_Const_u16{
  Node_String_Const_u16 *next;
  String_Const_u16 string;
};
struct Node_String_Const_u32{
  Node_String_Const_u32 *next;
  String_Const_u32 string;
};
struct List_String_Const_char{
  Node_String_Const_char *first;
  Node_String_Const_char *last;
  u64 total_size;
  i32 node_count;
};
struct List_String_Const_u8{
  Node_String_Const_u8 *first;
  Node_String_Const_u8 *last;
  u64 total_size;
  i32 node_count;
};
struct List_String_Const_u16{
  Node_String_Const_u16 *first;
  Node_String_Const_u16 *last;
  u64 total_size;
  i32 node_count;
};
struct List_String_Const_u32{
  Node_String_Const_u32 *first;
  Node_String_Const_u32 *last;
  u64 total_size;
  i32 node_count;
};

typedef Node_String_Const_u8 String8Node;
typedef List_String_Const_u8 String8List;

struct Node_String_Const_Any{
  Node_String_Const_Any *next;
  String_Const_Any string;
};
struct List_String_Const_Any{
  Node_String_Const_Any *first;
  Node_String_Const_Any *last;
  u64 total_size;
  i32 node_count;
};

struct String_char{
  union{
    String_Const_char string;
    struct{
      char *str;
      u64 size;
    };
  };
  u64 cap;
};
struct String_u8
{
    union
    {
        String_Const_u8 string;
        struct
        {
            u8 *str;
            u64 size;
        };
    };
    u64 cap;
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
struct String_u32{
  union{
    String_Const_u32 string;
    struct{
      u32 *str;
      u64 size;
    };
  };
  u64 cap;
};

struct String_Any{
  String_Encoding encoding;
  union{
    struct{
      void *str;
      u64 size;
      u64 cap;
    };
    String_char s_char;
    String_u8 s_u8;
    String_u16 s_u16;
    String_u32 s_u32;
  };
};

typedef i32 Line_Ending_Kind;
enum{
  LineEndingKind_Binary,
  LineEndingKind_LF,
  LineEndingKind_CRLF,
};

struct Character_Consume_Result{
  u32 inc;
  u32 codepoint;
};

global u32 surrogate_min = 0xD800;
global u32 surrogate_max = 0xDFFF;

global u32 nonchar_min = 0xFDD0;
global u32 nonchar_max = 0xFDEF;

////////////////////////////////

typedef u32 Access_Flag;
enum{
  AccessFlag_Read  = 1,
  AccessFlag_Write = 2,
  AccessFlag_Exec  = 4,
};

typedef i32 Dimension;
enum{
  Dimension_X = 0,
  Dimension_Y = 1,
  Dimension_Z = 2,
  Dimension_W = 3,
};

typedef i32 Coordinate;
enum{
  Coordinate_X = 0,
  Coordinate_Y = 1,
  Coordinate_Z = 2,
  Coordinate_W = 3,
};

typedef i32 Side;
enum{
  Side_Min = 0,
  Side_Max = 1,
};

typedef i32 Scan_Direction;
enum{
  Scan_Backward = -1,
  Scan_Forward  =  1,
};

////////////////////////////////

struct Date_Time{
  u32 year; // Real year, no adjustment
  u8 mon;   // [0,11]
  u8 day;   // [0,30]
  u8 hour;  // [0,23]
  u8 min;   // [0,59]
  u8 sec;   // [0,60]
  u16 msec; // [0,999]
};

global String_Const_u8 month_full_name[] = {
  str8_lit("January"),
  str8_lit("February"),
  str8_lit("March"),
  str8_lit("April"),
  str8_lit("May"),
  str8_lit("June"),
  str8_lit("July"),
  str8_lit("August"),
  str8_lit("September"),
  str8_lit("October"),
  str8_lit("November"),
  str8_lit("December"),
};

global String_Const_u8 month_abrev_name[] = {
  str8_lit("Jan"),
  str8_lit("Feb"),
  str8_lit("Mar"),
  str8_lit("Apr"),
  str8_lit("May"),
  str8_lit("Jun"),
  str8_lit("Jul"),
  str8_lit("Aug"),
  str8_lit("Sep"),
  str8_lit("Oct"),
  str8_lit("Nov"),
  str8_lit("Dec"),
};

global String_Const_u8 ordinal_numeric_name[] = {
  str8_lit("1st"),
  str8_lit("2nd"),
  str8_lit("3rd"),
  str8_lit("4th"),
  str8_lit("5th"),
  str8_lit("6th"),
  str8_lit("7th"),
  str8_lit("8th"),
  str8_lit("9th"),
  str8_lit("10th"),
  str8_lit("11th"),
  str8_lit("12th"),
  str8_lit("13th"),
  str8_lit("14th"),
  str8_lit("15th"),
  str8_lit("16th"),
  str8_lit("17th"),
  str8_lit("18th"),
  str8_lit("19th"),
  str8_lit("20th"),
  str8_lit("21st"),
  str8_lit("22nd"),
  str8_lit("23rd"),
  str8_lit("24th"),
  str8_lit("25th"),
  str8_lit("26th"),
  str8_lit("27th"),
  str8_lit("28th"),
  str8_lit("29th"),
  str8_lit("30th"),
  str8_lit("31st"),
  str8_lit("32nd"),
  str8_lit("33rd"),
  str8_lit("34th"),
  str8_lit("35th"),
  str8_lit("36th"),
  str8_lit("37th"),
  str8_lit("38th"),
  str8_lit("39th"),
  str8_lit("40th"),
  str8_lit("41st"),
  str8_lit("42nd"),
  str8_lit("43rd"),
  str8_lit("44th"),
  str8_lit("45th"),
  str8_lit("46th"),
  str8_lit("47th"),
  str8_lit("48th"),
  str8_lit("49th"),
  str8_lit("50th"),
  str8_lit("51st"),
  str8_lit("52nd"),
  str8_lit("53rd"),
  str8_lit("54th"),
  str8_lit("55th"),
  str8_lit("56th"),
  str8_lit("57th"),
  str8_lit("58th"),
  str8_lit("59th"),
  str8_lit("60th"),
  str8_lit("61st"),
  str8_lit("62nd"),
  str8_lit("63rd"),
  str8_lit("64th"),
  str8_lit("65th"),
  str8_lit("66th"),
  str8_lit("67th"),
  str8_lit("68th"),
  str8_lit("69th"),
  str8_lit("70th"),
  str8_lit("71st"),
  str8_lit("72nd"),
  str8_lit("73rd"),
  str8_lit("74th"),
  str8_lit("75th"),
  str8_lit("76th"),
  str8_lit("77th"),
  str8_lit("78th"),
  str8_lit("79th"),
  str8_lit("80th"),
  str8_lit("81st"),
  str8_lit("82nd"),
  str8_lit("83rd"),
  str8_lit("84th"),
  str8_lit("85th"),
  str8_lit("86th"),
  str8_lit("87th"),
  str8_lit("88th"),
  str8_lit("89th"),
  str8_lit("90th"),
  str8_lit("91st"),
  str8_lit("92nd"),
  str8_lit("93rd"),
  str8_lit("94th"),
  str8_lit("95th"),
  str8_lit("96th"),
  str8_lit("97th"),
  str8_lit("98th"),
  str8_lit("99th"),
  str8_lit("100th"),
};

////////////////////////////////

typedef void *Base_Allocator_Reserve_Signature(void *user_data, u64 size, u64 *size_out, String_Const_u8 location);
typedef void  Base_Allocator_Commit_Signature(void *user_data, void *ptr, u64 size);
typedef void  Base_Allocator_Uncommit_Signature(void *user_data, void *ptr, u64 size);
typedef void  Base_Allocator_Free_Signature(void *user_data, void *ptr);
typedef void  Base_Allocator_Set_Access_Signature(void *user_data, void *ptr, u64 size, Access_Flag flags);
struct Base_Allocator
{
  Base_Allocator_Reserve_Signature    *reserve;
  Base_Allocator_Commit_Signature     *commit;
  Base_Allocator_Uncommit_Signature   *uncommit;
  Base_Allocator_Free_Signature       *free;
  Base_Allocator_Set_Access_Signature *set_access;
  void *user_data;
};

struct Cursor
{
  u8 *base;
  u64 pos;
  u64 cap;
};
struct Temp_Memory_Cursor
{
  Cursor *cursor;
  u64 pos;
};
struct Cursor_Node
{
    union
    {
        Cursor_Node *next;
        Cursor_Node *prev;
    };
    Cursor cursor;
};
struct Arena
{
    Base_Allocator *base_allocator;
    Cursor_Node    *cursor_node;
    u64 chunk_size;
    u64 alignment;
};
struct Temp_Memory_Arena
{
    Arena *arena;
    Cursor_Node *cursor_node;
    u64 pos;
};
typedef i32 Linear_Allocator_Kind;
enum{
  LinearAllocatorKind_Cursor,
  LinearAllocatorKind_Arena,
};
struct Temp_Memory{
  Linear_Allocator_Kind kind;
  union{
    Temp_Memory_Cursor temp_memory_cursor;
    Temp_Memory_Arena temp_memory_arena;
  };
};

////////////////////////////////

typedef u64 Profile_ID;
struct Profile_Record{
  Profile_Record *next;
  Profile_ID id;
  u64 time;
  String_Const_u8 location;
  String_Const_u8 name;
};

struct Profile_Thread{
  Profile_Thread *next;
  Profile_Record *first_record;
  Profile_Record *last_record;
  i32 record_count;
  i32 thread_id;
  String_Const_u8 name;
};

typedef u32 Profile_Enable_Flag;
enum{
  ProfileEnable_UserBit    = 0x1,
  ProfileEnable_InspectBit = 0x2,
};

// NOTE(allen): full definition in 4coder_profile.h, due to dependency on System_Mutex.
struct Profile_Global_List;

////////////////////////////////

typedef i32 Thread_Kind;
enum{
  ThreadKind_Main,
  ThreadKind_MainCoroutine,
  ThreadKind_AsyncTasks,
};

struct Arena_Node{
  Arena_Node *next;
  Arena_Node *prev;
  Arena arena;
  i32 ref_counter;
};

struct Thread_Context{
  Thread_Kind kind;
  Base_Allocator *allocator;
  Arena node_arena;
  Arena_Node *used_first;
  Arena_Node *used_last;
  Arena_Node *free_arenas;
  
  Base_Allocator *prof_allocator;
  Profile_ID prof_id_counter;
  Arena prof_arena;
  Profile_Record *prof_first;
  Profile_Record *prof_last;
  i32 prof_record_count;
  
  void *user_data;
};

struct Scratch_Block {
  Thread_Context *tctx;
  Arena *arena;
  Temp_Memory temp;
  
  Scratch_Block(struct Thread_Context *tctx);
  Scratch_Block(struct Thread_Context *tctx, Arena *a1);
  Scratch_Block(struct Thread_Context *tctx, Arena *a1, Arena *a2);
  Scratch_Block(struct Thread_Context *tctx, Arena *a1, Arena *a2, Arena *a3);
  Scratch_Block(struct Application_Links *app);
  Scratch_Block(struct Application_Links *app, Arena *a1);
  Scratch_Block(struct Application_Links *app, Arena *a1, Arena *a2);
  Scratch_Block(struct Application_Links *app, Arena *a1, Arena *a2, Arena *a3);
  ~Scratch_Block();
  operator Arena*();
  void restore(void);
};

struct Temp_Memory_Block{
  Temp_Memory temp;
  Temp_Memory_Block(Temp_Memory temp);
  Temp_Memory_Block(Arena *arena);
  ~Temp_Memory_Block();
  void restore(void);
};

////////////////////////////////

struct Heap_Basic_Node{
  Heap_Basic_Node *next;
  Heap_Basic_Node *prev;
};

struct Heap_Node{
  union{
    struct{
      Heap_Basic_Node order;
      Heap_Basic_Node alloc;
      u64 size;
    };
    u8 force_size__[64];
  };
};

struct Heap{
  Arena arena_;
  Arena *arena;
  Heap_Basic_Node in_order;
  Heap_Basic_Node free_nodes;
  u64 used_space;
  u64 total_space;
};

//////////////////////////////////////////////////
// 4coder_base_types.cpp

/*
 * 4coder base types
 * NOTE(kv): color in v4 is RGBA, but when packed it is [register:ARGB -> memory:BGRA]
 * Idk why, but it is what it is.
 */

#ifdef KV_IMPLEMENTATION

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"

// @Cleanup Don't need these anymore
function i32
i32_ceil32(f32 v)
{
    return(((v)>0)?( (v == (i32)(v))?((i32)(v)):((i32)((v)+1.f)) ):( ((i32)(v)) ));
}

function i32
i32_floor32(f32 v)
{
    return(((v)<0)?( (v == (i32)(v))?((i32)(v)):((i32)((v)-1.f)) ):( ((i32)(v)) ));
}

#pragma clang diagnostic pop

function i32
i32_round32(f32 v)
{
    return(i32_floor32(v + 0.5f));
}

function f32
f32_ceil32(f32 v){
    return((f32)i32_ceil32(v));
}

function f32
f32_floor32(f32 v){
    return((f32)i32_floor32(v));
}

function f32
f32_round32(f32 v){
    return((f32)i32_round32(v));
}

function i8
round_up_i8(i8 x, i8 b){
    x += b - 1;
    x -= x%b;
    return(x);
}
function u8
round_up_u8(u8 x, u8 b){
    x += b - 1;
    x -= x%b;
    return(x);
}
function i16
round_up_i16(i16 x, i16 b){
    x += b - 1;
    x -= x%b;
    return(x);
}
function u16
round_up_u16(u16 x, u16 b){
    x += b - 1;
    x -= x%b;
    return(x);
}
function i32
round_up_i32(i32 x, i32 b){
    x += b - 1;
    x -= x%b;
    return(x);
}
function u32
round_up_u32(u32 x, u32 b){
    x += b - 1;
    x -= x%b;
    return(x);
}
function i64
round_up_i64(i64 x, i64 b){
    x += b - 1;
    x -= x%b;
    return(x);
}
function u64
round_up_u64(u64 x, u64 b){
    x += b - 1;
    x -= x%b;
    return(x);
}

function i8
round_down_i8(i8 x, i8 b){
    x -= x%b;
    return(x);
}
function u8
round_down_u8(u8 x, u8 b){
    x -= x%b;
    return(x);
}
function i16
round_down_i16(i16 x, i16 b){
    x -= x%b;
    return(x);
}
function u16
round_down_u16(u16 x, u16 b){
    x -= x%b;
    return(x);
}
function i32
round_down_i32(i32 x, i32 b){
    x -= x%b;
    return(x);
}
function u32
round_down_u32(u32 x, u32 b){
    x -= x%b;
    return(x);
}
function i64
round_down_i64(i64 x, i64 b){
    x -= x%b;
    return(x);
}
function u64
round_down_u64(u64 x, u64 b){
    x -= x%b;
    return(x);
}

function f32
f32_integer(f32 x){
    return((f32)((i32)x));
}

function u32
round_up_pot_u32(u32 x){
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    ++x;
    return(x);
}

////////////////////////////////

function String8
make_data(void *memory, u64 size)
{
    String8 data = {(u8*)memory, size};
    return(data);
}

#define make_data_struct(s) make_data((s), sizeof(*(s)))

global_const String_Const_u8 zero_data = {};

#define data_initr(m,s) {(u8*)(m), (s)}
#define data_initr_struct(s) {(u8*)(s), sizeof(*(s))}
#define data_initr_array(a) {(u8*)(a), sizeof(a)}
#define data_initr_string(s) {(u8*)(s), sizeof(s) - 1}

////////////////////////////////

inline void
block_zero(String8 data){
    block_zero(data.str, data.size);
}
internal void
block_fill_ones(String8 data){
    block_fill_ones(data.str, data.size);
}

function b32
block_match(void *a, void *b, u64 size){
    b32 result = true;
    for (u8 *pa = (u8*)a, *pb = (u8*)b, *ea = pa + size; pa < ea; pa += 1, pb += 1){
        if (*pa != *pb){
            result = false;
            break;
        }
    }
    return(result);
}
inline i32
block_compare(void *a, void *b, u64 size)
{
    return gb_memcompare(a, b, size);
}
inline void
block_fill_u8(void *dst, u64 size, u8 val)
{
    gb_memset(dst, val, size);
}
function void
block_fill_u16(void *a, u64 size, u16 val)
{
    Assert(size%sizeof(u16) == 0);
    u64 count = size/sizeof(u16);
    for (u16 *ptr = (u16*)a, *e = ptr + count; ptr < e; ptr += 1){
        *ptr = val;
    }
}
function void
block_fill_u32(void *a, u64 size, u32 val)
{
    Assert(size%sizeof(u32) == 0);
    u64 count = size/sizeof(u32);
    for (u32 *ptr = (u32*)a, *e = ptr + count; ptr < e; ptr += 1){
        *ptr = val;
    }
}
function void
block_fill_u64(void *a, u64 size, u64 val)
{
    Assert(size%sizeof(u64) == 0);
    u64 count = size/sizeof(u64);
    for (u64 *ptr = (u64*)a, *e = ptr + count; ptr < e; ptr += 1){
        *ptr = val;
    }
}

#define block_zero_struct(p) block_zero((p), sizeof(*(p)))
#define block_zero_array(a)  block_zero((a), sizeof(a))
#define block_zero_dynamic_array(p,c) block_zero((p), sizeof(*(p))*(c))

#define block_copy_struct(d,s) block_copy((d), (s), sizeof(*(d)))
#define block_copy_array(d,s)  block_copy((d), (s), sizeof(d))
#define block_copy_dynamic_array(d,s,c) block_copy((d), (s), sizeof(*(d))*(c))

#define block_match_struct(a,b) block_match((a), (b), sizeof(*(a)))
#define block_match_array(a,b) block_match((a), (b), sizeof(a))

function void
block_range_copy__inner(void *dst, void *src, Range_u64 range, i64 shift){
    block_copy((u8*)dst + range.first + shift, (u8*)src + range.first, range.max - range.min);
}

function void
block_range_copy__inner(void *dst, void *src, Range_u64 range, i64 shift, u64 item_size){
    range.first *= item_size;
    range.one_past_last *= item_size;
    shift *= item_size;
    block_range_copy__inner(dst, src, range, shift);
}

#define block_range_copy(d,s,r,h) block_range_copy__inner((d),(s),Iu64(r),(i64)(h))
#define block_range_copy_sized(d,s,r,h,i) block_range_copy__inner((d),(s),Iu64(r),(i64)(h),(i))
#define block_range_copy_typed(d,s,r,h) block_range_copy_sized((d),(s),(r),(h),sizeof(*(d)))

function void
block_copy_array_shift__inner(void *dst, void *src, u64 it_size, Range_i64 range, i64 shift){
    u8 *dptr = (u8*)dst;
    u8 *sptr = (u8*)src;
    dptr += it_size*(range.first + shift);
    sptr += it_size*range.first;
    block_copy(dptr, sptr, (u64)(it_size*(range.one_past_last - range.first)));
}
function void
block_copy_array_shift__inner(void *dst, void *src, u64 it_size, Range_i32 range, i64 shift){
    u8 *dptr = (u8*)dst;
    u8 *sptr = (u8*)src;
    dptr += it_size*(range.first + shift);
    sptr += it_size*range.first;
    block_copy(dptr, sptr, (u64)(it_size*(range.one_past_last - range.first)));
}

#define block_copy_array_shift(d,s,r,h) block_copy_array_shift__inner((d),(s),sizeof(*(d)),(r),(h))

////////////////////////////////

function f32
abs_f32(f32 x)
{
    if (x < 0){
        x = -x;
    }
    return(x);
}

#include <math.h>

function f32
mod_f32(f32 x, i32 m)
{
    f32 whole;
    f32 frac = modff(x, &whole);
    f32 r = f32((i32)(whole) % m) + frac;
    return(r);
}

// TODO: make these be based on actual turn
inline f32
cos_turn(f32 x)
{
    return cosf(TAU32 * x);
}

inline f32
sin_turn(f32 x)
{
    return sinf(TAU32 * x);
}

////////////////////////////////

function Vec2_i8
V2i8(i8 x, i8 y){
    Vec2_i8 v = {x, y};
    return(v);
}
function Vec3_i8
V3i8(i8 x, i8 y, i8 z){
    Vec3_i8 v = {x, y, z};
    return(v);
}
function Vec4_i8
V4i8(i8 x, i8 y, i8 z, i8 w){
    Vec4_i8 v = {x, y, z, w};
    return(v);
}
function Vec2_i16
V2i16(i16 x, i16 y){
    Vec2_i16 v = {x, y};
    return(v);
}
function Vec3_i16
V3i16(i16 x, i16 y, i16 z){
    Vec3_i16 v = {x, y, z};
    return(v);
}
function Vec4_i16
V4i16(i16 x, i16 y, i16 z, i16 w){
    Vec4_i16 v = {x, y, z, w};
    return(v);
}
function Vec2_i32
V2i(i32 x, i32 y)
{
    Vec2_i32 v = {x, y};
    return(v);
}
function Vec3_i32
V3i32(i32 x, i32 y, i32 z){
    Vec3_i32 v = {x, y, z};
    return(v);
}
function Vec4_i32
V4i32(i32 x, i32 y, i32 z, i32 w)
{
    Vec4_i32 v = {x, y, z, w};
    return(v);
}

internal Vec2_f32
V2f32(f32 x, f32 y)
{
    Vec2_f32 v = {x, y};
    return(v);
}

inline v2
V2()
{
    return v2{};
}

internal v2
V2(f32 x, f32 y)
{
    Vec2_f32 v = {x, y};
    return(v);
}

internal Vec2_f32
castV2(i32 x, i32 y){
    Vec2_f32 v = {(f32)x, (f32)y};
    return(v);
}

internal v2
castV2(v2i v){
    return {(f32)v.x, (f32)v.y};
}

internal Vec3_f32
V3(f32 x, f32 y, f32 z)
{
    Vec3_f32 v = {x, y, z};
    return(v);
}
function Vec4_f32
V4(f32 x, f32 y, f32 z, f32 w)
{
    Vec4_f32 v = {x, y, z, w};
    return(v);
}
internal v4
V4(v3 xyz, v1 w)
{
    v4 v = {.xyz=xyz, .xyz_w=w};
    return v;
}
internal v4
castV4(v3 xyz)
{
    v4 v = {.xyz=xyz};
    return v;
}

function Vec2_i8
V2i8(Vec2_i8 o){
    return(V2i8((i8)o.x, (i8)o.y));
}
function Vec2_i8
V2i8(Vec2_i16 o){
    return(V2i8((i8)o.x, (i8)o.y));
}
function Vec2_i8
V2i8(Vec2_i32 o){
    return(V2i8((i8)o.x, (i8)o.y));
}
function Vec2_i8
V2i8(Vec2_f32 o){
    return(V2i8((i8)o.x, (i8)o.y));
}
function Vec3_i8
V3i8(Vec3_i8 o){
    return(V3i8((i8)o.x, (i8)o.y, (i8)o.z));
}
function Vec3_i8
V3i8(Vec3_i16 o){
    return(V3i8((i8)o.x, (i8)o.y, (i8)o.z));
}
function Vec3_i8
V3i8(Vec3_i32 o){
    return(V3i8((i8)o.x, (i8)o.y, (i8)o.z));
}
function Vec3_i8
V3i8(Vec3_f32 o){
    return(V3i8((i8)o.x, (i8)o.y, (i8)o.z));
}
function Vec4_i8
V4i8(Vec4_i8 o){
    return(V4i8((i8)o.x, (i8)o.y, (i8)o.z, (i8)o.w));
}
function Vec4_i8
V4i8(Vec4_i16 o){
    return(V4i8((i8)o.x, (i8)o.y, (i8)o.z, (i8)o.w));
}
function Vec4_i8
V4i8(Vec4_i32 o){
    return(V4i8((i8)o.x, (i8)o.y, (i8)o.z, (i8)o.w));
}
function Vec4_i8
V4i8(Vec4_f32 o){
    return(V4i8((i8)o.x, (i8)o.y, (i8)o.z, (i8)o.w));
}
function Vec2_i16
V2i16(Vec2_i8 o){
    return(V2i16((i16)o.x, (i16)o.y));
}
function Vec2_i16
V2i16(Vec2_i16 o){
    return(V2i16((i16)o.x, (i16)o.y));
}
function Vec2_i16
V2i16(Vec2_i32 o){
    return(V2i16((i16)o.x, (i16)o.y));
}
function Vec2_i16
V2i16(Vec2_f32 o){
    return(V2i16((i16)o.x, (i16)o.y));
}
function Vec3_i16
V3i16(Vec3_i8 o){
    return(V3i16((i16)o.x, (i16)o.y, (i16)o.z));
}
function Vec3_i16
V3i16(Vec3_i16 o){
    return(V3i16((i16)o.x, (i16)o.y, (i16)o.z));
}
function Vec3_i16
V3i16(Vec3_i32 o){
    return(V3i16((i16)o.x, (i16)o.y, (i16)o.z));
}
function Vec3_i16
V3i16(Vec3_f32 o){
    return(V3i16((i16)o.x, (i16)o.y, (i16)o.z));
}
function Vec4_i16
V4i16(Vec4_i8 o){
    return(V4i16((i16)o.x, (i16)o.y, (i16)o.z, (i16)o.w));
}
function Vec4_i16
V4i16(Vec4_i16 o){
    return(V4i16((i16)o.x, (i16)o.y, (i16)o.z, (i16)o.w));
}
function Vec4_i16
V4i16(Vec4_i32 o){
    return(V4i16((i16)o.x, (i16)o.y, (i16)o.z, (i16)o.w));
}
function Vec4_i16
V4i16(Vec4_f32 o){
    return(V4i16((i16)o.x, (i16)o.y, (i16)o.z, (i16)o.w));
}
function Vec2_i32
V2i(Vec2_i8 o){
    return(V2i((i32)o.x, (i32)o.y));
}
function Vec2_i32
V2i(Vec2_i16 o){
    return(V2i((i32)o.x, (i32)o.y));
}
/*
function Vec2_i32
V2i32(Vec2_i32 o){
    return(V2i((i32)o.x, (i32)o.y));
}
*/
function Vec2_i32
V2i(Vec2_f32 o)
{
    return(V2i((i32)o.x, (i32)o.y));
}
function Vec3_i32
V3i32(Vec3_i8 o){
    return(V3i32((i32)o.x, (i32)o.y, (i32)o.z));
}
function Vec3_i32
V3i32(Vec3_i16 o){
    return(V3i32((i32)o.x, (i32)o.y, (i32)o.z));
}
function Vec3_i32
V3i32(Vec3_i32 o){
    return(V3i32((i32)o.x, (i32)o.y, (i32)o.z));
}
function Vec3_i32
V3i32(Vec3_f32 o){
    return(V3i32((i32)o.x, (i32)o.y, (i32)o.z));
}
function Vec4_i32
V4i32(Vec4_i8 o){
    return(V4i32((i32)o.x, (i32)o.y, (i32)o.z, (i32)o.w));
}
function Vec4_i32
V4i32(Vec4_i16 o){
    return(V4i32((i32)o.x, (i32)o.y, (i32)o.z, (i32)o.w));
}
function Vec4_i32
V4i32(Vec4_i32 o){
    return(V4i32((i32)o.x, (i32)o.y, (i32)o.z, (i32)o.w));
}
function Vec4_i32
V4i32(Vec4_f32 o){
    return(V4i32((i32)o.x, (i32)o.y, (i32)o.z, (i32)o.w));
}
function Vec2_f32
V2(Vec2_i8 o){
    return(V2((f32)o.x, (f32)o.y));
}
function Vec2_f32
V2(Vec2_i16 o){
    return(V2((f32)o.x, (f32)o.y));
}
function Vec2_f32
V2(Vec2_i32 o){
    return(V2((f32)o.x, (f32)o.y));
}
function Vec2_f32
V2(Vec2_f32 o){
    return(V2((f32)o.x, (f32)o.y));
}
function Vec3_f32
V3(Vec3_i8 o){
    return(V3((f32)o.x, (f32)o.y, (f32)o.z));
}
function Vec3_f32
V3(Vec3_i16 o){
    return(V3((f32)o.x, (f32)o.y, (f32)o.z));
}
function Vec3_f32
V3(Vec3_i32 o){
    return(V3((f32)o.x, (f32)o.y, (f32)o.z));
}
function Vec3_f32
V3(Vec3_f32 o){
    return(V3((f32)o.x, (f32)o.y, (f32)o.z));
}
function Vec4_f32
V4(Vec4_i8 o){
    return(V4((f32)o.x, (f32)o.y, (f32)o.z, (f32)o.w));
}
function Vec4_f32
V4(Vec4_i16 o){
    return(V4((f32)o.x, (f32)o.y, (f32)o.z, (f32)o.w));
}
function Vec4_f32
V4(Vec4_i32 o){
    return(V4((f32)o.x, (f32)o.y, (f32)o.z, (f32)o.w));
}
function Vec4_f32
V4(Vec4_f32 o){
    return(V4((f32)o.x, (f32)o.y, (f32)o.z, (f32)o.w));
}

function Vec2_i8
operator+(Vec2_i8 a, Vec2_i8 b){
    a.x += b.x;
    a.y += b.y;
    return(a);
}
function Vec3_i8
operator+(Vec3_i8 a, Vec3_i8 b){
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    return(a);
}
function Vec4_i8
operator+(Vec4_i8 a, Vec4_i8 b){
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    a.w += b.w;
    return(a);
}
function Vec2_i16
operator+(Vec2_i16 a, Vec2_i16 b){
    a.x += b.x;
    a.y += b.y;
    return(a);
}
function Vec3_i16
operator+(Vec3_i16 a, Vec3_i16 b){
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    return(a);
}
function Vec4_i16
operator+(Vec4_i16 a, Vec4_i16 b){
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    a.w += b.w;
    return(a);
}
function Vec2_i32
operator+(Vec2_i32 a, Vec2_i32 b){
    a.x += b.x;
    a.y += b.y;
    return(a);
}
function Vec3_i32
operator+(Vec3_i32 a, Vec3_i32 b){
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    return(a);
}
function Vec4_i32
operator+(Vec4_i32 a, Vec4_i32 b){
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    a.w += b.w;
    return(a);
}

function Vec2_i8&
operator+=(Vec2_i8 &a, Vec2_i8 b){
    a.x += b.x;
    a.y += b.y;
    return(a);
}
function Vec3_i8&
operator+=(Vec3_i8 &a, Vec3_i8 b){
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    return(a);
}
function Vec4_i8&
operator+=(Vec4_i8 &a, Vec4_i8 b){
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    a.w += b.w;
    return(a);
}
function Vec2_i16&
operator+=(Vec2_i16 &a, Vec2_i16 b){
    a.x += b.x;
    a.y += b.y;
    return(a);
}
function Vec3_i16&
operator+=(Vec3_i16 &a, Vec3_i16 b){
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    return(a);
}
function Vec4_i16&
operator+=(Vec4_i16 &a, Vec4_i16 b){
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    a.w += b.w;
    return(a);
}
function Vec2_i32&
operator+=(Vec2_i32 &a, Vec2_i32 b){
    a.x += b.x;
    a.y += b.y;
    return(a);
}
function Vec3_i32&
operator+=(Vec3_i32 &a, Vec3_i32 b){
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    return(a);
}
function Vec4_i32&
operator+=(Vec4_i32 &a, Vec4_i32 b){
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    a.w += b.w;
    return(a);
}

function Vec2_i8
operator-(Vec2_i8 a, Vec2_i8 b){
    a.x -= b.x;
    a.y -= b.y;
    return(a);
}
function Vec3_i8
operator-(Vec3_i8 a, Vec3_i8 b){
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
    return(a);
}
function Vec4_i8
operator-(Vec4_i8 a, Vec4_i8 b){
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
    a.w -= b.w;
    return(a);
}
function Vec2_i16
operator-(Vec2_i16 a, Vec2_i16 b){
    a.x -= b.x;
    a.y -= b.y;
    return(a);
}
function Vec3_i16
operator-(Vec3_i16 a, Vec3_i16 b){
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
    return(a);
}
function Vec4_i16
operator-(Vec4_i16 a, Vec4_i16 b){
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
    a.w -= b.w;
    return(a);
}
function Vec2_i32
operator-(Vec2_i32 a, Vec2_i32 b){
    a.x -= b.x;
    a.y -= b.y;
    return(a);
}
function Vec3_i32
operator-(Vec3_i32 a, Vec3_i32 b){
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
    return(a);
}
function Vec4_i32
operator-(Vec4_i32 a, Vec4_i32 b){
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
    a.w -= b.w;
    return(a);
}

function Vec2_i8&
operator-=(Vec2_i8 &a, Vec2_i8 b){
    a.x -= b.x;
    a.y -= b.y;
    return(a);
}
function Vec3_i8&
operator-=(Vec3_i8 &a, Vec3_i8 b){
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
    return(a);
}
function Vec4_i8&
operator-=(Vec4_i8 &a, Vec4_i8 b){
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
    a.w -= b.w;
    return(a);
}
function Vec2_i16&
operator-=(Vec2_i16 &a, Vec2_i16 b){
    a.x -= b.x;
    a.y -= b.y;
    return(a);
}
function Vec3_i16&
operator-=(Vec3_i16 &a, Vec3_i16 b){
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
    return(a);
}
function Vec4_i16&
operator-=(Vec4_i16 &a, Vec4_i16 b){
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
    a.w -= b.w;
    return(a);
}
function Vec2_i32&
operator-=(Vec2_i32 &a, Vec2_i32 b){
    a.x -= b.x;
    a.y -= b.y;
    return(a);
}
function Vec3_i32&
operator-=(Vec3_i32 &a, Vec3_i32 b){
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
    return(a);
}
function Vec4_i32&
operator-=(Vec4_i32 &a, Vec4_i32 b){
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
    a.w -= b.w;
    return(a);
}

function Vec2_i8
operator*(i8 s, Vec2_i8 v){
    v.x *= s;
    v.y *= s;
    return(v);
}
function Vec2_i8
operator*(Vec2_i8 v, i8 s){
    v.x *= s;
    v.y *= s;
    return(v);
}
function Vec3_i8
operator*(i8 s, Vec3_i8 v){
    v.x *= s;
    v.y *= s;
    v.z *= s;
    return(v);
}
function Vec3_i8
operator*(Vec3_i8 v, i8 s){
    v.x *= s;
    v.y *= s;
    v.z *= s;
    return(v);
}
function Vec4_i8
operator*(i8 s, Vec4_i8 v){
    v.x *= s;
    v.y *= s;
    v.z *= s;
    v.w *= s;
    return(v);
}
function Vec4_i8
operator*(Vec4_i8 v, i8 s){
    v.x *= s;
    v.y *= s;
    v.z *= s;
    v.w *= s;
    return(v);
}
function Vec2_i16
operator*(i16 s, Vec2_i16 v){
    v.x *= s;
    v.y *= s;
    return(v);
}
function Vec2_i16
operator*(Vec2_i16 v, i16 s){
    v.x *= s;
    v.y *= s;
    return(v);
}
function Vec3_i16
operator*(i16 s, Vec3_i16 v){
    v.x *= s;
    v.y *= s;
    v.z *= s;
    return(v);
}
function Vec3_i16
operator*(Vec3_i16 v, i16 s){
    v.x *= s;
    v.y *= s;
    v.z *= s;
    return(v);
}
function Vec4_i16
operator*(i16 s, Vec4_i16 v){
    v.x *= s;
    v.y *= s;
    v.z *= s;
    v.w *= s;
    return(v);
}
function Vec4_i16
operator*(Vec4_i16 v, i16 s){
    v.x *= s;
    v.y *= s;
    v.z *= s;
    v.w *= s;
    return(v);
}
function Vec2_i32
operator*(i32 s, Vec2_i32 v){
    v.x *= s;
    v.y *= s;
    return(v);
}
function Vec2_i32
operator*(Vec2_i32 v, i32 s){
    v.x *= s;
    v.y *= s;
    return(v);
}
function Vec3_i32
operator*(i32 s, Vec3_i32 v){
    v.x *= s;
    v.y *= s;
    v.z *= s;
    return(v);
}
function Vec3_i32
operator*(Vec3_i32 v, i32 s){
    v.x *= s;
    v.y *= s;
    v.z *= s;
    return(v);
}
function Vec4_i32
operator*(i32 s, Vec4_i32 v){
    v.x *= s;
    v.y *= s;
    v.z *= s;
    v.w *= s;
    return(v);
}
function Vec4_i32
operator*(Vec4_i32 v, i32 s){
    v.x *= s;
    v.y *= s;
    v.z *= s;
    v.w *= s;
    return(v);
}

function Vec2_i8&
operator*=(Vec2_i8 &v, i8 s){
    v.x *= s;
    v.y *= s;
    return(v);
}
function Vec3_i8&
operator*=(Vec3_i8 &v, i8 s){
    v.x *= s;
    v.y *= s;
    v.z *= s;
    return(v);
}
function Vec4_i8&
operator*=(Vec4_i8 &v, i8 s){
    v.x *= s;
    v.y *= s;
    v.z *= s;
    v.w *= s;
    return(v);
}
function Vec2_i16&
operator*=(Vec2_i16 &v, i16 s){
    v.x *= s;
    v.y *= s;
    return(v);
}
function Vec3_i16&
operator*=(Vec3_i16 &v, i16 s){
    v.x *= s;
    v.y *= s;
    v.z *= s;
    return(v);
}
function Vec4_i16&
operator*=(Vec4_i16 &v, i16 s){
    v.x *= s;
    v.y *= s;
    v.z *= s;
    v.w *= s;
    return(v);
}
function Vec2_i32&
operator*=(Vec2_i32 &v, i32 s){
    v.x *= s;
    v.y *= s;
    return(v);
}
function Vec3_i32&
operator*=(Vec3_i32 &v, i32 s){
    v.x *= s;
    v.y *= s;
    v.z *= s;
    return(v);
}
function Vec4_i32&
operator*=(Vec4_i32 &v, i32 s){
    v.x *= s;
    v.y *= s;
    v.z *= s;
    v.w *= s;
    return(v);
}

function Vec2_i8
operator/(Vec2_i8 v, i8 s){
    v.x /= s;
    v.y /= s;
    return(v);
}
function Vec3_i8
operator/(Vec3_i8 v, i8 s){
    v.x /= s;
    v.y /= s;
    v.z /= s;
    return(v);
}
function Vec4_i8
operator/(Vec4_i8 v, i8 s){
    v.x /= s;
    v.y /= s;
    v.z /= s;
    v.w /= s;
    return(v);
}
function Vec2_i16
operator/(Vec2_i16 v, i16 s){
    v.x /= s;
    v.y /= s;
    return(v);
}
function Vec3_i16
operator/(Vec3_i16 v, i16 s){
    v.x /= s;
    v.y /= s;
    v.z /= s;
    return(v);
}
function Vec4_i16
operator/(Vec4_i16 v, i16 s){
    v.x /= s;
    v.y /= s;
    v.z /= s;
    v.w /= s;
    return(v);
}
function Vec2_i32
operator/(Vec2_i32 v, i32 s){
    v.x /= s;
    v.y /= s;
    return(v);
}
function Vec3_i32
operator/(Vec3_i32 v, i32 s){
    v.x /= s;
    v.y /= s;
    v.z /= s;
    return(v);
}
function Vec4_i32
operator/(Vec4_i32 v, i32 s){
    v.x /= s;
    v.y /= s;
    v.z /= s;
    v.w /= s;
    return(v);
}
function Vec4_f32
operator/(Vec4_f32 v, f32 s){
    v.x /= s;
    v.y /= s;
    v.z /= s;
    v.w /= s;
    return(v);
}

function Vec2_i8&
operator/=(Vec2_i8 &v, i8 s){
    v.x /= s;
    v.y /= s;
    return(v);
}
function Vec3_i8&
operator/=(Vec3_i8 &v, i8 s){
    v.x /= s;
    v.y /= s;
    v.z /= s;
    return(v);
}
function Vec4_i8&
operator/=(Vec4_i8 &v, i8 s){
    v.x /= s;
    v.y /= s;
    v.z /= s;
    v.w /= s;
    return(v);
}
function Vec2_i16&
operator/=(Vec2_i16 &v, i16 s){
    v.x /= s;
    v.y /= s;
    return(v);
}
function Vec3_i16&
operator/=(Vec3_i16 &v, i16 s){
    v.x /= s;
    v.y /= s;
    v.z /= s;
    return(v);
}
function Vec4_i16&
operator/=(Vec4_i16 &v, i16 s){
    v.x /= s;
    v.y /= s;
    v.z /= s;
    v.w /= s;
    return(v);
}
function Vec2_i32&
operator/=(Vec2_i32 &v, i32 s){
    v.x /= s;
    v.y /= s;
    return(v);
}
function Vec3_i32&
operator/=(Vec3_i32 &v, i32 s){
    v.x /= s;
    v.y /= s;
    v.z /= s;
    return(v);
}
function Vec4_i32&
operator/=(Vec4_i32 &v, i32 s){
    v.x /= s;
    v.y /= s;
    v.z /= s;
    v.w /= s;
    return(v);
}
function Vec2_f32&
operator/=(Vec2_f32 &v, f32 s){
    v.x /= s;
    v.y /= s;
    return(v);
}
function Vec3_f32&
operator/=(Vec3_f32 &v, f32 s){
    v.x /= s;
    v.y /= s;
    v.z /= s;
    return(v);
}
function Vec4_f32&
operator/=(Vec4_f32 &v, f32 s){
    v.x /= s;
    v.y /= s;
    v.z /= s;
    v.w /= s;
    return(v);
}

function b32
operator==(Vec2_i8 a, Vec2_i8 b){
    return(a.x == b.x && a.y == b.y);
}
function b32
operator==(Vec3_i8 a, Vec3_i8 b){
    return(a.x == b.x && a.y == b.y && a.z == b.z);
}
function b32
operator==(Vec4_i8 a, Vec4_i8 b){
    return(a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w);
}
function b32
operator==(Vec2_i16 a, Vec2_i16 b){
    return(a.x == b.x && a.y == b.y);
}
function b32
operator==(Vec3_i16 a, Vec3_i16 b){
    return(a.x == b.x && a.y == b.y && a.z == b.z);
}
function b32
operator==(Vec4_i16 a, Vec4_i16 b){
    return(a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w);
}
function b32
operator==(Vec2_i32 a, Vec2_i32 b){
    return(a.x == b.x && a.y == b.y);
}
function b32
operator==(Vec3_i32 a, Vec3_i32 b){
    return(a.x == b.x && a.y == b.y && a.z == b.z);
}
function b32
operator==(Vec4_i32 a, Vec4_i32 b){
    return(a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w);
}
function b32
operator==(Vec4_f32 a, Vec4_f32 b){
    return(a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w);
}

function b32
operator!=(Vec2_i8 a, Vec2_i8 b){
    return(a.x != b.x || a.y != b.y);
}
function b32
operator!=(Vec3_i8 a, Vec3_i8 b){
    return(a.x != b.x || a.y != b.y || a.z != b.z);
}
function b32
operator!=(Vec4_i8 a, Vec4_i8 b){
    return(a.x != b.x || a.y != b.y || a.z != b.z || a.w != b.w);
}
function b32
operator!=(Vec2_i16 a, Vec2_i16 b){
    return(a.x != b.x || a.y != b.y);
}
function b32
operator!=(Vec3_i16 a, Vec3_i16 b){
    return(a.x != b.x || a.y != b.y || a.z != b.z);
}
function b32
operator!=(Vec4_i16 a, Vec4_i16 b){
    return(a.x != b.x || a.y != b.y || a.z != b.z || a.w != b.w);
}
function b32
operator!=(Vec2_i32 a, Vec2_i32 b){
    return(a.x != b.x || a.y != b.y);
}
function b32
operator!=(Vec3_i32 a, Vec3_i32 b){
    return(a.x != b.x || a.y != b.y || a.z != b.z);
}
function b32
operator!=(Vec4_i32 a, Vec4_i32 b){
    return(a.x != b.x || a.y != b.y || a.z != b.z || a.w != b.w);
}
function b32
operator!=(Vec4_f32 a, Vec4_f32 b){
    return(a.x != b.x || a.y != b.y || a.z != b.z || a.w != b.w);
}

////////////////////////////////

function b32
near_zero(f32 p, f32 epsilon){
    return(-epsilon <= p && p <= epsilon);
}
function b32
near_zero(Vec2_f32 p, f32 epsilon){
    return(-epsilon <= p.x && p.x <= epsilon &&
           -epsilon <= p.y && p.y <= epsilon);
}
function b32
near_zero(Vec3_f32 p, f32 epsilon){
    return(-epsilon <= p.x && p.x <= epsilon &&
           -epsilon <= p.y && p.y <= epsilon &&
           -epsilon <= p.z && p.z <= epsilon);
}
function b32
near_zero(Vec4_f32 p, f32 epsilon){
    return(-epsilon <= p.x && p.x <= epsilon &&
           -epsilon <= p.y && p.y <= epsilon &&
           -epsilon <= p.z && p.z <= epsilon &&
           -epsilon <= p.w && p.w <= epsilon);
}

function b32
near_zero(f32 p){ return(near_zero(p, epsilon_f32)); }
function b32
near_zero(Vec2_f32 p){ return(near_zero(p, epsilon_f32)); }
function b32
near_zero(Vec3_f32 p){ return(near_zero(p, epsilon_f32)); }
function b32
near_zero(Vec4_f32 p){ return(near_zero(p, epsilon_f32)); }

////////////////////////////////

function f32
lerp(f32 t, Range_f32 x){
    return(x.min + (x.max - x.min)*t);
}

function i32
lerp(i32 a, f32 t, i32 b){
    return((i32)(lerp((f32)a, t, (f32)b)));
}

function Vec2_f32
lerp(Vec2_f32 a, f32 t, Vec2_f32 b){
    return(a + (b-a)*t);
}

function Vec3_f32
lerp(Vec3_f32 a, f32 t, Vec3_f32 b){
    return(a + (b-a)*t);
}

function f32
unlerp(f32 a, f32 x, f32 b){
    f32 r = x;
    if (b != a)
    {
        r = (x - a)/(b - a);
    }
    return(r);
}

function f32
unlerp(u64 a, u64 x, u64 b){
    f32 r = 0.f;
    if (b <= x){
        r = 1.f;
    }
    else if (a < x){
        u64 n = x - a;
        u64 d = b - a;
        r = (f32)(((f64)n)/((f64)d));
    }
    return(r);
}

function Range_f32
unlerp(f32 a, Range_f32 x, f32 b){
    x.min = unlerp(a, x.min, b);
    x.max = unlerp(a, x.max, b);
    return(x);
}

function Range_f32
lerp(f32 a, Range_f32 x, f32 b){
    x.min = lerp(a, x.min, b);
    x.max = lerp(a, x.max, b);
    return(x);
}

function f32
lerp(Range_f32 range, f32 t){
    return(lerp(range.min, t, range.max));
}

function f32
clamp_range(Range_f32 range, f32 x)
{
    return(clamp_between(range.min, x, range.max));
}

////////////////////////////////

function b32
operator==(Rect_i32 a, Rect_i32 b){
    return(a.p0 == b.p0 && a.p1 == b.p1);
}
function b32
operator==(Rect_f32 a, Rect_f32 b){
    return(a.p0 == b.p0 && a.p1 == b.p1);
}

function b32
operator!=(Rect_i32 a, Rect_i32 b){
    return(!(a == b));
}
function b32
operator!=(Rect_f32 a, Rect_f32 b){
    return(!(a == b));
}

////////////////////////////////

function Vec4_f32
unpack_argb(ARGB_Color color)
{
    Vec4_f32 result;
    result.a = ((color >> 24) & 0xFF)/255.f;
    result.r = ((color >> 16) & 0xFF)/255.f;
    result.g = ((color >> 8)  & 0xFF)/255.f;
    result.b = ((color >> 0)  & 0xFF)/255.f;
    return(result);
}

function ARGB_Color
pack_argb(v4 color)
{
    ARGB_Color result =
        ((u8)(color.a*255) << 24) |
        ((u8)(color.r*255) << 16) |
        ((u8)(color.g*255) << 8) |
        ((u8)(color.b*255) << 0);
    return(result);
}

function ARGB_Color
color_blend(ARGB_Color a, f32 t, ARGB_Color b)
{
    Vec4_f32 av = unpack_argb(a);
    Vec4_f32 bv = unpack_argb(b);
    Vec4_f32 v = lerp(av, t, bv);
    return(pack_argb(v));
}

function Vec4_f32
rgba_to_hsla(Vec4_f32 rgba)
{
    Vec4_f32 hsla = {};
    f32 max, min, delta;
    i32 maxc;
    hsla.a = rgba.a;
    max = rgba.r; min = rgba.r;
    maxc = 0;
    if (rgba.r < rgba.g){
        max = rgba.g;
        maxc = 1;
    }
    if (rgba.b > max){
        max = rgba.b;
        maxc = 2;
    }
    if (rgba.r > rgba.g){
        min = rgba.g;
    }
    if (rgba.b < min){
        min = rgba.b;
    }
    delta = max - min;
    
    hsla.z = (max + min)*.5f;
    if (delta == 0){
        hsla.x = 0.f;
        hsla.y = 0.f;
    }
    else{
        switch (maxc){
            case 0:
            {
                hsla.x = (rgba.g - rgba.b) / delta;
                hsla.x += (rgba.g < rgba.b)*6.f;
            }break;
            
            case 1:
            {
                hsla.x = (rgba.b - rgba.r) / delta;
                hsla.x += 2.f;
            }break;
            
            case 2:
            {
                hsla.x = (rgba.r - rgba.g) / delta;
                hsla.x += 4.f;
            }break;
        }
        hsla.x *= (1/6.f); //*60 / 360
        hsla.y = delta / (1.f - abs_f32(2.f*hsla.z - 1.f));
    }
    
    return(hsla);
}

function Vec4_f32
hsla_to_rgba(Vec4_f32 hsla)
{
    if (hsla.h >= 1.f){
        hsla.h = 0.f;
    }
    f32 C = (1.f - abs_f32(2*hsla.z - 1.f))*hsla.y;
    f32 X = C*(1.f-abs_f32(mod_f32(hsla.x*6.f, 2)-1.f));
    f32 m = hsla.z - C*.5f;
    i32 H = i32_floor32(hsla.x*6.f);
    Vec4_f32 rgba = {};
    rgba.a = hsla.a;
    switch (H){
        case 0: rgba.r = C; rgba.g = X; rgba.b = 0; break;
        case 1: rgba.r = X; rgba.g = C; rgba.b = 0; break;
        case 2: rgba.r = 0; rgba.g = C; rgba.b = X; break;
        case 3: rgba.r = 0; rgba.g = X; rgba.b = C; break;
        case 4: rgba.r = X; rgba.g = 0; rgba.b = C; break;
        case 5: rgba.r = C; rgba.g = 0; rgba.b = X; break;
    }
    rgba.r += m;
    rgba.g += m;
    rgba.b += m;
    return(rgba);
}

////////////////////////////////

function Range_i32
Ii32(i32 a, i32 b){
    Range_i32 interval = {a, b};
    if (b < a){
        interval.min = b;
        interval.max = a;
    }
    return(interval);
}
function Range_i64
Ii64(i64 a, i64 b){
    Range_i64 interval = {a, b};
    if (b < a){
        interval.min = b;
        interval.max = a;
    }
    return(interval);
}
function Range_u64
Iu64(u64 a, u64 b){
    Range_u64 interval = {a, b};
    if (b < a){
        interval.min = b;
        interval.max = a;
    }
    return(interval);
}
function Range_f32
If32(f32 a, f32 b){
    Range_f32 interval = {a, b};
    if (b < a){
        interval.min = b;
        interval.max = a;
    }
    return(interval);
}

function Range_i32
Ii32_size(i32 pos, i32 size){
    return(Ii32(pos, pos + size));
}
function Range_i64
Ii64_size(i64 pos, i64 size){
    return(Ii64(pos, pos + size));
}
function Range_u64
Iu64_size(u64 pos, u64 size){
    return(Iu64(pos, pos + size));
}
function Range_f32
If32_size(f32 pos, f32 size){
    return(If32(pos, pos + size));
}

function Range_i32
Ii32(i32 a){
    Range_i32 interval = {a, a};
    return(interval);
}
function Range_i64
Ii64(i64 a){
    Range_i64 interval = {a, a};
    return(interval);
}
function Range_u64
Iu64(u64 a){
    Range_u64 interval = {a, a};
    return(interval);
}
function Range_f32
If32(f32 a){
    Range_f32 interval = {a, a};
    return(interval);
}

function Range_i32
Ii32(){
    Range_i32 interval = {};
    return(interval);
}
function Range_i64
Ii64(){
    Range_i64 interval = {};
    return(interval);
}
function Range_u64
Iu64(){
    Range_u64 interval = {};
    return(interval);
}
function Range_f32
If32(){
    Range_f32 interval = {};
    return(interval);
}

function Range_u64
Iu64(Range_i32 r){
    return(Iu64(r.min, r.max));
}

global Range_i32 Ii32_neg_inf = {max_i32, min_i32};
global Range_i64 Ii64_neg_inf = {max_i64, min_i64};
global Range_u64 Iu64_neg_inf = {max_u64, 0};
global Range_f32 If32_neg_inf = {max_f32, -max_f32};

function b32
operator==(Range_i32 a, Range_i32 b){
    return(a.min == b.min && a.max == b.max);
}
function b32
operator==(Range_i64 a, Range_i64 b){
    return(a.min == b.min && a.max == b.max);
}
function b32
operator==(Range_u64 a, Range_u64 b){
    return(a.min == b.min && a.max == b.max);
}
function b32
operator==(Range_f32 a, Range_f32 b){
    return(a.min == b.min && a.max == b.max);
}

function Range_i32
operator+(Range_i32 r, i32 s){
    return(Ii32(r.min + s, r.max + s));
}
function Range_i64
operator+(Range_i64 r, i64 s){
    return(Ii64(r.min + s, r.max + s));
}
function Range_u64
operator+(Range_u64 r, u64 s){
    return(Iu64(r.min + s, r.max + s));
}
function Range_f32
operator+(Range_f32 r, f32 s){
    return(If32(r.min + s, r.max + s));
}

function Range_i32
operator-(Range_i32 r, i32 s){
    return(Ii32(r.min - s, r.max - s));
}
function Range_i64
operator-(Range_i64 r, i64 s){
    return(Ii64(r.min - s, r.max - s));
}
function Range_u64
operator-(Range_u64 r, u64 s){
    return(Iu64(r.min - s, r.max - s));
}
function Range_f32
operator-(Range_f32 r, f32 s){
    return(If32(r.min - s, r.max - s));
}

function Range_i32&
operator+=(Range_i32 &r, i32 s){
    r = r + s;
    return(r);
}
function Range_i64&
operator+=(Range_i64 &r, i64 s){
    r = r + s;
    return(r);
}
function Range_u64&
operator+=(Range_u64 &r, u64 s){
    r = r + s;
    return(r);
}
function Range_f32&
operator+=(Range_f32 &r, f32 s){
    r = r + s;
    return(r);
}

function Range_i32&
operator-=(Range_i32 &r, i32 s){
    r = r - s;
    return(r);
}
function Range_i64&
operator-=(Range_i64 &r, i64 s){
    r = r - s;
    return(r);
}
function Range_u64&
operator-=(Range_u64 &r, u64 s){
    r = r - s;
    return(r);
}
function Range_f32&
operator-=(Range_f32 &r, f32 s){
    r = r - s;
    return(r);
}

function Range_i32
range_margin(Range_i32 range, i32 margin){
    range.min += margin;
    range.max += margin;
    return(range);
}
function Range_i64
range_margin(Range_i64 range, i64 margin){
    range.min += margin;
    range.max += margin;
    return(range);
}
function Range_u64
range_margin(Range_u64 range, u64 margin){
    range.min += margin;
    range.max += margin;
    return(range);
}
function Range_f32
range_margin(Range_f32 range, f32 margin){
    range.min += margin;
    range.max += margin;
    return(range);
}

function b32
range_overlap(Range_i32 a, Range_i32 b){
    return(a.min < b.max && b.min < a.max);
}
function b32
range_overlap(Range_i64 a, Range_i64 b){
    return(a.min < b.max && b.min < a.max);
}
function b32
range_overlap(Range_u64 a, Range_u64 b){
    return(a.min < b.max && b.min < a.max);
}
function b32
range_overlap(Range_f32 a, Range_f32 b){
    return(a.min < b.max && b.min < a.max);
}

function Range_i32
range_intersect(Range_i32 a, Range_i32 b){
    Range_i32 result = {};
    if (range_overlap(a, b)){
        result = Ii32(Max(a.min, b.min), Min(a.max, b.max));
    }
    return(result);
}
function Range_i64
range_intersect(Range_i64 a, Range_i64 b){
    Range_i64 result = {};
    if (range_overlap(a, b)){
        result = Ii64(Max(a.min, b.min), Min(a.max, b.max));
    }
    return(result);
}
function Range_u64
range_intersect(Range_u64 a, Range_u64 b){
    Range_u64 result = {};
    if (range_overlap(a, b)){
        result = Iu64(Max(a.min, b.min), Min(a.max, b.max));
    }
    return(result);
}
function Range_f32
range_intersect(Range_f32 a, Range_f32 b){
    Range_f32 result = {};
    if (range_overlap(a, b)){
        result = If32(Max(a.min, b.min), Min(a.max, b.max));
    }
    return(result);
}

function Range_i32
range_union(Range_i32 a, Range_i32 b){
    return(Ii32(Min(a.min, b.min), Max(a.max, b.max)));
}
function Range_i64
range_union(Range_i64 a, Range_i64 b){
    return(Ii64(Min(a.min, b.min), Max(a.max, b.max)));
}
function Range_u64
range_union(Range_u64 a, Range_u64 b){
    return(Iu64(Min(a.min, b.min), Max(a.max, b.max)));
}
function Range_f32
range_union(Range_f32 a, Range_f32 b){
    return(If32(Min(a.min, b.min), Max(a.max, b.max)));
}

function b32
range_contains_inclusive(Range_i32 a, i32 p){
    return(a.min <= p && p <= a.max);
}
function b32
range_contains_inclusive(Range_i64 a, i64 p){
    return(a.min <= p && p <= a.max);
}
function b32
range_contains_inclusive(Range_u64 a, u64 p){
    return(a.min <= p && p <= a.max);
}
function b32
range_inclusive_contains(Range_f32 a, f32 p){
    return(a.min <= p && p <= a.max);
}

function b32
range_contains(Range_i32 a, i32 p){
    return(a.min <= p && p < a.max);
}
function b32
range_contains(Range_i64 a, i64 p){
    return(a.min <= p && p < a.max);
}
function b32
range_contains(Range_u64 a, u64 p){
    return(a.min <= p && p < a.max);
}
function b32
range_contains(Range_f32 a, f32 p){
    return(a.min <= p && p < a.max);
}

function i32
range_size(Range_i32 a){
    i32 size = a.max - a.min;
    size = clamp_bot(0, size);
    return(size);
}
function i64
range_size(Range_i64 a){
    i64 size = a.max - a.min;
    size = clamp_bot(0, size);
    return(size);
}
function u64
range_size(Range_u64 a){
    u64 size = a.max - a.min;
    size = clamp_bot(0, size);
    return(size);
}
function f32
range_size(Range_f32 a){
    f32 size = a.max - a.min;
    size = clamp_bot(0, size);
    return(size);
}

function i32
range_size_inclusive(Range_i32 a){
    i32 size = a.max - a.min + 1;
    size = clamp_bot(0, size);
    return(size);
}
function i64
range_size_inclusive(Range_i64 a){
    i64 size = a.max - a.min + 1;
    size = clamp_bot(0, size);
    return(size);
}
function u64
range_size_inclusive(Range_u64 a){
    u64 size = a.max - a.min + 1;
    size = clamp_bot(0, size);
    return(size);
}
function f32
range_size_inclusive(Range_f32 a){
    f32 size = a.max - a.min + 1;
    size = clamp_bot(0, size);
    return(size);
}

function Range_i32
rectify(Range_i32 a){
    return(Ii32(a.min, a.max));
}
function Range_i64
rectify(Range_i64 a){
    return(Ii64(a.min, a.max));
}
function Range_u64
rectify(Range_u64 a){
    return(Iu64(a.min, a.max));
}
function Range_f32
rectify(Range_f32 a){
    return(If32(a.min, a.max));
}

function Range_i32
range_clamp_size(Range_i32 a, i32 max_size){
    i32 max = a.min + max_size;
    a.max = clamp_top(a.max, max);
    return(a);
}
function Range_i64
range_clamp_size(Range_i64 a, i64 max_size){
    i64 max = a.min + max_size;
    a.max = clamp_top(a.max, max);
    return(a);
}
function Range_u64
range_clamp_size(Range_u64 a, u64 max_size){
    u64 max = a.min + max_size;
    a.max = clamp_top(a.max, max);
    return(a);
}
function Range_f32
range_clamp_size(Range_f32 a, f32 max_size){
    f32 max = a.min + max_size;
    a.max = clamp_top(a.max, max);
    return(a);
}

function b32
range_is_valid(Range_i32 a){
    return(a.min <= a.max);
}
function b32
range_is_valid(Range_i64 a){
    return(a.min <= a.max);
}
function b32
range_is_valid(Range_u64 a){
    return(a.min <= a.max);
}
function b32
range_is_valid(Range_f32 a){
    return(a.min <= a.max);
}

function i32
range_side(Range_i32 a, Side side){
    return(side == Side_Min?a.min:a.max);
}
function i64
range_side(Range_i64 a, Side side){
    return(side == Side_Min?a.min:a.max);
}
function u64
range_side(Range_u64 a, Side side){
    return(side == Side_Min?a.min:a.max);
}
function f32
range_side(Range_f32 a, Side side){
    return(side == Side_Min?a.min:a.max);
}

function i32
range_distance(Range_i32 a, Range_i32 b){
    i32 result = 0;
    if (!range_overlap(a, b)){
        if (a.max < b.min){
            result = b.min - a.max;
        }
        else{
            result = a.min - b.max;
        }
    }
    return(result);
}
function i64
range_distance(Range_i64 a, Range_i64 b){
    i64 result = 0;
    if (!range_overlap(a, b)){
        if (a.max < b.min){
            result = b.min - a.max;
        }
        else{
            result = a.min - b.max;
        }
    }
    return(result);
}
function u64
range_distance(Range_u64 a, Range_u64 b){
    u64 result = 0;
    if (!range_overlap(a, b)){
        if (a.max < b.min){
            result = b.min - a.max;
        }
        else{
            result = a.min - b.max;
        }
    }
    return(result);
}
function f32
range_distance(Range_f32 a, Range_f32 b){
    f32 result = 0;
    if (!range_overlap(a, b)){
        if (a.max < b.min){
            result = b.min - a.max;
        }
        else{
            result = a.min - b.max;
        }
    }
    return(result);
}

////////////////////////////////

function i32
replace_range_shift(i32 replace_length, i32 insert_length){
    return(insert_length - replace_length);
}
function i32
replace_range_shift(i32 start, i32 end, i32 insert_length){
    return(insert_length - (end - start));
}
function i32
replace_range_shift(Range_i32 range, i32 insert_length){
    return(insert_length - (range.end - range.start));
}
function i64
replace_range_shift(i64 replace_length, i64 insert_length){
    return(insert_length - replace_length);
}
function i64
replace_range_shift(i64 start, i64 end, i64 insert_length){
    return(insert_length - (end - start));
}
function i64
replace_range_shift(Range_i64 range, i64 insert_length){
    return(insert_length - (range.end - range.start));
}
function i64
replace_range_shift(u64 replace_length, u64 insert_length){
    return((i64)insert_length - replace_length);
}
function i64
replace_range_shift(i64 start, i64 end, u64 insert_length){
    return((i64)insert_length - (end - start));
}
function i64
replace_range_shift(Range_i64 range, u64 insert_length){
    return((i64)insert_length - (range.end - range.start));
}

////////////////////////////////

function Rect_i32
Ri32(i32 x0, i32 y0, i32 x1, i32 y1){
    Rect_i32 rect = {x0, y0, x1, y1};
    return(rect);
}
function Rect_f32
Rf32(f32 x0, f32 y0, f32 x1, f32 y1){
    Rect_f32 rect = {x0, y0, x1, y1};
    return(rect);
}

function Rect_i32
Ri32(Vec2_i32 p0, Vec2_i32 p1){
    Rect_i32 rect = {p0.x, p0.y, p1.x, p1.y};
    return(rect);
}
function Rect_f32
Rf32(Vec2_f32 p0, Vec2_f32 p1){
    Rect_f32 rect = {p0.x, p0.y, p1.x, p1.y};
    return(rect);
}

function Rect_i32
Ri32(Rect_f32 o){
    Rect_i32 rect = {(i32)(o.x0), (i32)(o.y0), (i32)(o.x1), (i32)(o.y1)};
    return(rect);
}
function Rect_f32
Rf32(Rect_i32 o){
    Rect_f32 rect = {(f32)(o.x0), (f32)(o.y0), (f32)(o.x1), (f32)(o.y1)};
    return(rect);
}

function Rect_i32
Ri32_xy_wh(i32 x0, i32 y0, i32 w, i32 h){
    Rect_i32 rect = {x0, y0, x0 + w, y0 + h};
    return(rect);
}
function Rect_f32
Rf32_xy_wh(f32 x0, f32 y0, f32 w, f32 h){
    Rect_f32 rect = {x0, y0, x0 + w, y0 + h};
    return(rect);
}

function Rect_i32
Ri32_xy_wh(Vec2_i32 p0, Vec2_i32 d){
    Rect_i32 rect = {p0.x, p0.y, p0.x + d.x, p0.y + d.y};
    return(rect);
}
function Rect_f32
Rf32_xy_wh(Vec2_f32 p0, Vec2_f32 d){
    Rect_f32 rect = {p0.x, p0.y, p0.x + d.x, p0.y + d.y};
    return(rect);
}

function Rect_i32
Ri32(Range_i32 x, Range_i32 y){
    return(Ri32(x.min, y.min, x.max, y.max));
}
function Rect_f32
Rf32(Range_f32 x, Range_f32 y){
    return(Rf32(x.min, y.min, x.max, y.max));
}

global_const Rect_f32 Rf32_infinity          = {-max_f32, -max_f32,  max_f32,  max_f32};
global_const Rect_f32 Rf32_negative_infinity = { max_f32,  max_f32, -max_f32, -max_f32};

global_const Rect_i32 Ri32_infinity          = {-max_i32, -max_i32,  max_i32,  max_i32};
global_const Rect_i32 Ri32_negative_infinity = { max_i32,  max_i32, -max_i32, -max_i32};

function b32
rect_equals(Rect_i32 a, Rect_i32 b){
    return(a.x0 == b.x0 && a.y0 == b.y0 && a.x1 == b.x1 && a.y1 == b.y1);
}
function b32
rect_equals(Rect_f32 a, Rect_f32 b){
    return(a.x0 == b.x0 && a.y0 == b.y0 && a.x1 == b.x1 && a.y1 == b.y1);
}

function b32
rect_contains_point(Rect_i32 a, Vec2_i32 b){
    return(a.x0 <= b.x && b.x < a.x1 && a.y0 <= b.y && b.y < a.y1);
}
function b32
rect_contains_point(Rect_f32 a, Vec2_f32 b){
    return(a.x0 <= b.x && b.x < a.x1 && a.y0 <= b.y && b.y < a.y1);
}

function Rect_i32
rect_inner(Rect_i32 r, i32 m){
    r.x0 += m;
    r.y0 += m;
    r.x1 -= m;
    r.y1 -= m;
    return(r);
}
function Rect_f32
rect_inner(Rect_f32 r, f32 m){
    r.x0 += m;
    r.y0 += m;
    r.x1 -= m;
    r.y1 -= m;
    return(r);
}

function Vec2_i32
rect2i_get_dim(Rect_i32 r)
{
    Vec2_i32 v = {r.x1 - r.x0, r.y1 - r.y0};
    return(v);
}
function Range_i32
rect_x(Rect_i32 r){
    return(Ii32(r.x0, r.x1));
}
function Range_i32
rect_y(Rect_i32 r){
    return(Ii32(r.y0, r.y1));
}
function i32
rect_width(Rect_i32 r){
    return(r.x1 - r.x0);
}
function i32
rect_height(Rect_i32 r){
    return(r.y1 - r.y0);
}
function Range_f32
rect_x(Rect_f32 r){
    return(If32(r.x0, r.x1));
}
function Range_f32
rect_y(Rect_f32 r){
    return(If32(r.y0, r.y1));
}
function f32
rect_width(Rect_f32 r){
    return(r.x1 - r.x0);
}
function f32
rect_height(Rect_f32 r){
    return(r.y1 - r.y0);
}

function Vec2_i32
rect_center(Rect_i32 r){
    return((r.p0 + r.p1)/2);
}
function Vec2_f32
rect_center(Rect_f32 r){
    return((r.p0 + r.p1)*0.5f);
}

function Range_i32
rect_range_x(Rect_i32 r){
    return(Ii32(r.x0, r.x1));
}
function Range_i32
rect_range_y(Rect_i32 r){
    return(Ii32(r.y0, r.y1));
}
function Range_f32
rect_range_x(Rect_f32 r){
    return(If32(r.x0, r.x1));
}
function Range_f32
rect_range_y(Rect_f32 r){
    return(If32(r.y0, r.y1));
}

function i32
rect_area(Rect_i32 r){
    return((r.x1 - r.x0)*(r.y1 - r.y0));
}
function f32
rect_area(Rect_f32 r){
    return((r.x1 - r.x0)*(r.y1 - r.y0));
}

function b32
rect_overlap(Rect_i32 a, Rect_i32 b){
    return(range_overlap(rect_range_x(a), rect_range_x(b)) &&
           range_overlap(rect_range_y(a), rect_range_y(b)));
}
function b32
rect_overlap(Rect_f32 a, Rect_f32 b){
    return(range_overlap(rect_range_x(a), rect_range_x(b)) &&
           range_overlap(rect_range_y(a), rect_range_y(b)));
}

function Vec2_i32
rect2_half_dim(Rect_i32 r){
    return(rect2i_get_dim(r)/2);
}
function Vec2_f32
rect2_half_dim(Rect_f32 r){
    return(rect_dim(r)*0.5f);
}

function Rect_i32
rect_intersect(Rect_i32 a, Rect_i32 b){
    a.x0 = Max(a.x0, b.x0);
    a.y0 = Max(a.y0, b.y0);
    a.x1 = Min(a.x1, b.x1);
    a.y1 = Min(a.y1, b.y1);
    a.x0 = Min(a.x0, a.x1);
    a.y0 = Min(a.y0, a.y1);
    return(a);
}
function Rect_i32
rect_union(Rect_i32 a, Rect_i32 b){
    a.x0 = Min(a.x0, b.x0);
    a.y0 = Min(a.y0, b.y0);
    a.x1 = Max(a.x1, b.x1);
    a.y1 = Max(a.y1, b.y1);
    return(a);
}
function Rect_f32
rect_intersect(Rect_f32 a, Rect_f32 b){
    a.x0 = Max(a.x0, b.x0);
    a.y0 = Max(a.y0, b.y0);
    a.x1 = Min(a.x1, b.x1);
    a.y1 = Min(a.y1, b.y1);
    a.x0 = Min(a.x0, a.x1);
    a.y0 = Min(a.y0, a.y1);
    return(a);
}
function Rect_f32
rect_union(Rect_f32 a, Rect_f32 b){
    a.x0 = Min(a.x0, b.x0);
    a.y0 = Min(a.y0, b.y0);
    a.x1 = Max(a.x1, b.x1);
    a.y1 = Max(a.y1, b.y1);
    return(a);
}

////////////////////////////////

function Rect_f32_Pair
rect_split_top_bottom__inner(Rect_f32 rect, f32 y){
    y = clamp_between(rect.y0, y, rect.y1);
    Rect_f32_Pair pair = {};
    pair.a = Rf32(rect.x0, rect.y0, rect.x1, y      );
    pair.b = Rf32(rect.x0, y      , rect.x1, rect.y1);
    return(pair);
}

function Rect_f32_Pair
rect_split_left_right__inner(Rect_f32 rect, f32 x){
    x = clamp_between(rect.x0, x, rect.x1);
    Rect_f32_Pair pair = {};
    pair.a = Rf32(rect.x0, rect.y0, x      , rect.y1);
    pair.b = Rf32(x      , rect.y0, rect.x1, rect.y1);
    return(pair);
}

function Rect_f32_Pair
rect_split_top_bottom(Rect_f32 rect, f32 y){
    return(rect_split_top_bottom__inner(rect, rect.y0 + y));
}

function Rect_f32_Pair
rect_split_left_right(Rect_f32 rect, f32 x){
    return(rect_split_left_right__inner(rect, rect.x0 + x));
}

function Rect_f32_Pair
rect_split_top_bottom_neg(Rect_f32 rect, f32 y){
    return(rect_split_top_bottom__inner(rect, rect.y1 - y));
}

function Rect_f32_Pair
rect_split_left_right_neg(Rect_f32 rect, f32 x){
    return(rect_split_left_right__inner(rect, rect.x1 - x));
}

function Rect_f32_Pair
rect_split_top_bottom_lerp(Rect_f32 rect, f32 t){
    return(rect_split_top_bottom__inner(rect, lerp(rect.y0, t, rect.y1)));
}

function Rect_f32_Pair
rect_split_left_right_lerp(Rect_f32 rect, f32 t){
    return(rect_split_left_right__inner(rect, lerp(rect.x0, t, rect.x1)));
}

////////////////////////////////

function Scan_Direction
flip_direction(Scan_Direction direction){
    switch (direction){
        case Scan_Forward:
        {
            direction = Scan_Backward;
        }break;
        case Scan_Backward:
        {
            direction = Scan_Forward;
        }break;
    }
    return(direction);
}

function Side
flip_side(Side side){
    switch (side){
        case Side_Min:
        {
            side = Side_Max;
        }break;
        case Side_Max:
        {
            side = Side_Min;
        }break;
    }
    return(side);
}

////////////////////////////////

function u64
cstring_length(char *str){
    u64 length = 0;
    for (;str[length] != 0; length += 1);
    return(length);
}
function u64
cstring_length(u8 *str){
    u64 length = 0;
    for (;str[length] != 0; length += 1);
    return(length);
}
function u64
cstring_length(u16 *str){
    u64 length = 0;
    for (;str[length] != 0; length += 1);
    return(length);
}
function u64
cstring_length(u32 *str){
    u64 length = 0;
    for (;str[length] != 0; length += 1);
    return(length);
}

function String_char
Schar(char *str, u64 size, u64 cap){
    String_char string = {str, size, cap};
    return(string);
}
function String_u8
Su8(u8 *str, u64 size, u64 cap){
    String_u8 string = {str, size, cap};
    return(string);
}
function String_u16
Su16(u16 *str, u64 size, u64 cap){
    String_u16 string = {str, size, cap};
    return(string);
}
function String_u32
Su32(u32 *str, u64 size, u64 cap){
    String_u32 string = {str, size, cap};
    return(string);
}

function String_Any
Sany(void *str, u64 size, u64 cap, String_Encoding encoding){
    String_Any string = {.encoding=encoding};
    switch (encoding){
        case StringEncoding_ASCII: string.s_char = Schar((char*)str, size, cap); break;
        case StringEncoding_UTF8:  string.s_u8 = Su8((u8*)str, size, cap); break;
        case StringEncoding_UTF16: string.s_u16 = Su16((u16*)str, size, cap); break;
        case StringEncoding_UTF32: string.s_u32 = Su32((u32*)str, size, cap); break;
    }
    return(string);
}

function String_char
Schar(char *str, u64 size){
    String_char string = {str, size, size + 1};
    return(string);
}
function String_u8
Su8(u8 *str, u64 size){
    String_u8 string = {str, size, size + 1};
    return(string);
}
function String_u16
Su16(u16 *str, u64 size){
    String_u16 string = {str, size, size + 1};
    return(string);
}
function String_u32
Su32(u32 *str, u64 size){
    String_u32 string = {str, size, size + 1};
    return(string);
}

function String_Any
Sany(void *str, u64 size, String_Encoding encoding){
    String_Any string = {.encoding=encoding};
    switch (encoding){
        case StringEncoding_ASCII: string.s_char = Schar((char*)str, size); break;
        case StringEncoding_UTF8:  string.s_u8 = Su8((u8*)str, size); break;
        case StringEncoding_UTF16: string.s_u16 = Su16((u16*)str, size); break;
        case StringEncoding_UTF32: string.s_u32 = Su32((u32*)str, size); break;
    }
    return(string);
}

function String_char
Schar(char *str, char *one_past_last){
    return(Schar(str, (u64)(one_past_last - str)));
}
function String_u8
Su8(u8 *str, u8 *one_past_last){
    return(Su8(str, (u64)(one_past_last - str)));
}
function String_u16
Su16(u16 *str, u16 *one_past_last){
    return(Su16(str, (u64)(one_past_last - str)));
}
function String_u32
Su32(u32 *str, u32 *one_past_last){
    return(Su32(str, (u64)(one_past_last - str)));
}

function String_Any
Sany(void *str, void *one_past_last, String_Encoding encoding){
    String_Any string = {.encoding=encoding};
    switch (encoding){
        case StringEncoding_ASCII: string.s_char = Schar((char*)str, (char*)one_past_last); break;
        case StringEncoding_UTF8:  string.s_u8 = Su8((u8*)str, (u8*)one_past_last); break;
        case StringEncoding_UTF16: string.s_u16 = Su16((u16*)str, (u16*)one_past_last); break;
        case StringEncoding_UTF32: string.s_u32 = Su32((u32*)str, (u32*)one_past_last); break;
    }
    return(string);
}

function String_char
Schar(char *str){
    u64 size = cstring_length(str);
    String_char string = {str, size, size + 1};
    return(string);
}
function String_u8
Su8(u8 *str){
    u64 size = cstring_length(str);
    String_u8 string = {str, size, size + 1};
    return(string);
}
function String_u16
Su16(u16 *str){
    u64 size = cstring_length(str);
    String_u16 string = {str, size, size + 1};
    return(string);
}
function String_u32
Su32(u32 *str){
    u64 size = cstring_length(str);
    String_u32 string = {str, size, size + 1};
    return(string);
}

function String_Any
Sany(void *str, String_Encoding encoding){
    String_Any string = {.encoding=encoding};
    switch (encoding){
        case StringEncoding_ASCII: string.s_char = Schar((char*)str); break;
        case StringEncoding_UTF8:  string.s_u8 = Su8((u8*)str); break;
        case StringEncoding_UTF16: string.s_u16 = Su16((u16*)str); break;
        case StringEncoding_UTF32: string.s_u32 = Su32((u32*)str); break;
    }
    return(string);
}

function String_char
Schar(String_Const_char str, u64 cap){
    String_char string = {str.str, str.size, cap};
    return(string);
}
function String_u8
Su8(String_Const_u8 str, u64 cap){
    String_u8 string = {str.str, str.size, cap};
    return(string);
}
function String_u16
Su16(String_Const_u16 str, u64 cap){
    String_u16 string = {str.str, str.size, cap};
    return(string);
}
function String_u32
Su32(String_Const_u32 str, u64 cap){
    String_u32 string = {str.str, str.size, cap};
    return(string);
}

function String_Any
SCany(String_char str){
    String_Any string = {.encoding=StringEncoding_ASCII};
    string.s_char = str;
    return(string);
}
function String_Any
SCany(String_u8 str){
    String_Any string = {.encoding=StringEncoding_UTF8};
    string.s_u8 = str;
    return(string);
}
function String_Any
SCany(String_u16 str){
    String_Any string = {.encoding=StringEncoding_UTF16};
    string.s_u16 = str;
    return(string);
}
function String_Any
SCany(String_u32 str){
    String_Any string = {.encoding=StringEncoding_UTF32};
    string.s_u32 = str;
    return(string);
}

function String_Const_char
SCchar(char *str, u64 size){
    String_Const_char string = {str, size};
    return(string);
}
function String_Const_u8
SCu8(u8 *str, u64 size){
    String_Const_u8 string = {str, size};
    return(string);
}
function String_Const_u16
SCu16(u16 *str, u64 size){
    String_Const_u16 string = {str, size};
    return(string);
}
function String_Const_u32
SCu32(u32 *str, u64 size){
    String_Const_u32 string = {str, size};
    return(string);
}

function String_Const_Any
SCany(void *str, u64 size, String_Encoding encoding){
    String_Const_Any string = {.encoding=encoding};
    switch (encoding){
        case StringEncoding_ASCII: string.s_char = SCchar((char*)str, size); break;
        case StringEncoding_UTF8:  string.s_u8 = SCu8((u8*)str, size); break;
        case StringEncoding_UTF16: string.s_u16 = SCu16((u16*)str, size); break;
        case StringEncoding_UTF32: string.s_u32 = SCu32((u32*)str, size); break;
    }
    return(string);
}

function String_Const_char
SCchar(void){
    String_Const_char string = {};
    return(string);
}
function String_Const_u8
SCu8(void){
    String_Const_u8 string = {};
    return(string);
}
function String_Const_u16
SCu16(void){
    String_Const_u16 string = {};
    return(string);
}
function String_Const_u32
SCu32(void){
    String_Const_u32 string = {};
    return(string);
}

function String_Const_char
SCchar(char *str, char *one_past_last){
    return(SCchar(str, (u64)(one_past_last - str)));
}
function String_Const_u8
SCu8(u8 *str, u8 *one_past_last){
    return(SCu8(str, (u64)(one_past_last - str)));
}
function String_Const_u16
SCu16(u16 *str, u16 *one_past_last){
    return(SCu16(str, (u64)(one_past_last - str)));
}
function String_Const_u32
SCu32(u32 *str, u32 *one_past_last){
    return(SCu32(str, (u64)(one_past_last - str)));
}

function String_Const_Any
SCany(void *str, void *one_past_last, String_Encoding encoding){
    String_Const_Any string = {.encoding=encoding};
    switch (encoding){
        case StringEncoding_ASCII: string.s_char = SCchar((char*)str, (char*)one_past_last); break;
        case StringEncoding_UTF8:  string.s_u8 = SCu8((u8*)str, (u8*)one_past_last); break;
        case StringEncoding_UTF16: string.s_u16 = SCu16((u16*)str, (u16*)one_past_last); break;
        case StringEncoding_UTF32: string.s_u32 = SCu32((u32*)str, (u32*)one_past_last); break;
    }
    return(string);
}

function String_Const_char
SCchar(char *str){
    u64 size = cstring_length(str);
    String_Const_char string = {str, size};
    return(string);
}
function String_Const_u8
SCu8(u8 *str){
    u64 size = cstring_length(str);
    String_Const_u8 string = {str, size};
    return(string);
}
function String_Const_u16
SCu16(u16 *str){
    u64 size = cstring_length(str);
    String_Const_u16 string = {str, size};
    return(string);
}
function String_Const_u32
SCu32(u32 *str){
    u64 size = cstring_length(str);
    String_Const_u32 string = {str, size};
    return(string);
}

function String_Const_char
SCchar(String_char string){
    return(string.string);
}
function String_Const_u8
SCu8(String_u8 string){
    return(string.string);
}
function String_Const_u16
SCu16(String_u16 string){
    return(string.string);
}
function String_Const_u32
SCu32(String_u32 string){
    return(string.string);
}

function String_Const_char
SCchar(String_Const_u8 str){
    return(SCchar((char*)str.str, str.size));
}
function String_Const_u8
SCu8(String_Const_char str){
    return(SCu8((u8*)str.str, str.size));
}

function String_Const_u8
SCu8(char *str, u64 length){
    return(SCu8((u8*)str, length));
}
function String_Const_u8
SCu8(char *first, char *one_past_last){
    return(SCu8((u8*)first, (u8*)one_past_last));
}
function String_Const_u8
SCu8(char *str){
    return(SCu8((u8*)str));
}

function String_Const_u16
SCu16(wchar_t *str, u64 size){
    return(SCu16((u16*)str, size));
}
function String_Const_u16
SCu16(wchar_t *str){
    return(SCu16((u16*)str));
}

function String_Const_Any
SCany(void *str, String_Encoding encoding){
    String_Const_Any string = {.encoding=encoding};
    switch (encoding){
        case StringEncoding_ASCII: string.s_char = SCchar((char*)str); break;
        case StringEncoding_UTF8:  string.s_u8 = SCu8((u8*)str); break;
        case StringEncoding_UTF16: string.s_u16 = SCu16((u16*)str); break;
        case StringEncoding_UTF32: string.s_u32 = SCu32((u32*)str); break;
    }
    return(string);
}

function String_Const_Any
SCany(String_Const_char str){
    String_Const_Any string = {.encoding=StringEncoding_ASCII};
    string.s_char = str;
    return(string);
}
function String_Const_Any
SCany(String_Const_u8 str){
    String_Const_Any string = {.encoding=StringEncoding_UTF8};
    string.s_u8 = str;
    return(string);
}
function String_Const_Any
SCany(String_Const_u16 str){
    String_Const_Any string = {.encoding=StringEncoding_UTF16};
    string.s_u16 = str;
    return(string);
}
function String_Const_Any
SCany(String_Const_u32 str){
    String_Const_Any string = {.encoding=StringEncoding_UTF32};
    string.s_u32 = str;
    return(string);
}

#define string_litexpr(s) SCchar((s), sizeof(s) - 1)
#define string_u8_litexpr(s) SCu8((u8*)(s), (u64)(sizeof(s) - 1))
#define str8lit string_u8_litexpr
#define string_u16_litexpr(s) SCu16((u16*)(s), (u64)(sizeof(s)/2 - 1))

#define string_expand(s) (i32)(s).size, (char*)(s).str

function String_Const_char string_empty = {"", 0};
function String_Const_u8 string_u8_empty = {(u8*)"", 0};

#define filename_line_number_lit_u8 string_u8_litexpr(filename_line_number)

////////////////////////////////

function void*
base_reserve__noop(void *user_data, u64 size, u64 *size_out, String_Const_u8 location){
    *size_out = 0;
    return(0);
}
function void
base_commit__noop(void *user_data, void *ptr, u64 size){}
function void
base_uncommit__noop(void *user_data, void *ptr, u64 size){}
function void
base_free__noop(void *user_data, void *ptr){}
function void
base_set_access__noop(void *user_data, void *ptr, u64 size, Access_Flag flags){}

function Base_Allocator
make_base_allocator(Base_Allocator_Reserve_Signature *func_reserve,
                    Base_Allocator_Commit_Signature *func_commit,
                    Base_Allocator_Uncommit_Signature *func_uncommit,
                    Base_Allocator_Free_Signature *func_free,
                    Base_Allocator_Set_Access_Signature *func_set_access,
                    void *user_data){
    if (func_reserve == 0){
        func_reserve = base_reserve__noop;
    }
    if (func_commit == 0){
        func_commit = base_commit__noop;
    }
    if (func_uncommit == 0){
        func_uncommit = base_uncommit__noop;
    }
    if (func_free == 0){
        func_free = base_free__noop;
    }
    if (func_set_access == 0){
        func_set_access = base_set_access__noop;
    }
    Base_Allocator base_allocator = {
        func_reserve,
        func_commit,
        func_uncommit,
        func_free,
        func_set_access,
        user_data,
    };
    return(base_allocator);
}
function String8
base_allocate__inner(Base_Allocator *allocator, u64 size, String8 debug_location)
{
    u64 full_size = 0;
    void *memory = allocator->reserve(allocator->user_data, size, &full_size, debug_location);
    allocator->commit(allocator->user_data, memory, full_size);
    return(make_data(memory, (u64)full_size));
}
function void
base_free(Base_Allocator *allocator, void *ptr){
    if (ptr != 0){
        allocator->free(allocator->user_data, ptr);
    }
}

#define base_allocate(a,s) base_allocate__inner((a), (s), filename_line_number_lit_u8)
#define base_array_loc(a,T,c,l) (T*)(base_allocate__inner((a), sizeof(T)*(c), (l)).str)
#define base_array(a,T,c) base_array_loc(a,T,c, filename_line_number_lit_u8)

////////////////////////////////

internal Cursor
make_cursor(void *base, u64 size){
    Cursor cursor = {(u8*)base, 0, size};
    return(cursor);
}
internal Cursor
make_cursor(String8 data){
    return(make_cursor(data.str, data.size));
}
internal Cursor
make_cursor(Base_Allocator *allocator, u64 size)
{
    String8 memory = base_allocate(allocator, size);
    return(make_cursor(memory));
}
internal String8
linalloc_bump(Cursor *cursor, u64 size, String8 location)
{
    String8 result = {};
    if (cursor->pos + size <= cursor->cap)
    {
        result.str = cursor->base + cursor->pos;
        result.size = size;
        cursor->pos += size;
    }
    return(result);
}
internal void
linalloc_pop(Cursor *cursor, u64 size)
{
    if (cursor->pos > size)
    {
        cursor->pos -= size;
    }
    else
    {
        cursor->pos = 0;
    }
}
internal String8
linalloc_align(Cursor *cursor, u64 alignment)
{
    u64 pos = round_up_u64(cursor->pos, alignment);
    u64 new_size = pos - cursor->pos;
    return linalloc_bump(cursor, new_size, filename_line_number_lit_u8);
}
internal Temp_Memory_Cursor
linalloc_begin_temp(Cursor *cursor)
{
    Temp_Memory_Cursor temp = {cursor, cursor->pos};
    return(temp);
}
internal void
linalloc_end_temp(Temp_Memory_Cursor temp)
{
    temp.cursor->pos = temp.pos;
}
internal void
linalloc_clear(Cursor *cursor){
    cursor->pos = 0;
}
internal Arena
make_arena(Base_Allocator *allocator, u64 chunk_size, u64 alignment)
{
    Arena arena = {allocator, 0, chunk_size, alignment};
    return(arena);
}
internal Arena
make_arena(Base_Allocator *allocator, u64 chunk_size)
{
    return(make_arena(allocator, chunk_size, 8));
}
function Arena
make_arena(Base_Allocator *allocator)
{
    return(make_arena(allocator, KB(64), 8));
}

internal Cursor_Node *
make_cursor_node_from_memory(void *memory, u64 size)
{
    Cursor_Node *cursor_node = (Cursor_Node *)memory;
    cursor_node->cursor = make_cursor(cursor_node + 1, size - sizeof(Cursor_Node));
    return cursor_node;
}

function Cursor_Node*
arena__new_node(Arena *arena, u64 min_size, String8 location)
{
    min_size = clamp_bot(min_size, arena->chunk_size);
    String8 memory = base_allocate__inner(arena->base_allocator, min_size + sizeof(Cursor_Node), location);
    Cursor_Node *cursor_node = make_cursor_node_from_memory(memory.str, memory.size);
    sll_stack_push(arena->cursor_node, cursor_node);
    return(cursor_node);
}
function String8
linalloc_push(Arena *arena, u64 size, String8 location)
{
    String8 result = {};
    if (size > 0)
    {
        Cursor_Node *cursor_node = arena->cursor_node;
        if (cursor_node == 0)
        {
            cursor_node = arena__new_node(arena, size, location);
        }
        result = linalloc_bump(&cursor_node->cursor, size, location);
        if (result.str == 0)
        {
            cursor_node = arena__new_node(arena, size, location);
            result = linalloc_bump(&cursor_node->cursor, size, location);
        }
        String8 alignment_data = linalloc_align(&cursor_node->cursor, arena->alignment);
        result.size += alignment_data.size;
    }
    return(result);
}
function void
linalloc_pop(Arena *arena, u64 size)
{
    Base_Allocator *allocator = arena->base_allocator;
    Cursor_Node *cursor_node = arena->cursor_node;
    for (Cursor_Node *prev = 0;
         cursor_node != 0 && size != 0;
         cursor_node = prev){
        prev = cursor_node->prev;
        if (size >= cursor_node->cursor.pos){
            size -= cursor_node->cursor.pos;
            base_free(allocator, cursor_node);
        }
        else{
            linalloc_pop(&cursor_node->cursor, size);
            break;
        }
    }
    arena->cursor_node = cursor_node;
}
function String_Const_u8
linalloc_align(Arena *arena, u64 alignment){
    arena->alignment = alignment;
    String_Const_u8 data = {};
    Cursor_Node *cursor_node = arena->cursor_node;
    if (cursor_node != 0){
        data = linalloc_align(&cursor_node->cursor, arena->alignment);
    }
    return(data);
}
function Temp_Memory_Arena
linalloc_begin_temp(Arena *arena){
    Cursor_Node *cursor_node = arena->cursor_node;
    Temp_Memory_Arena temp = {arena, cursor_node,
        cursor_node == 0?0:cursor_node->cursor.pos};
    return(temp);
}
function void
linalloc_end_temp(Temp_Memory_Arena temp)
{
    Base_Allocator *allocator = temp.arena->base_allocator;
    Cursor_Node *cursor_node = temp.arena->cursor_node;
    for (Cursor_Node *prev = 0;
         cursor_node != temp.cursor_node && cursor_node != 0;
         cursor_node = prev){
        prev = cursor_node->prev;
        base_free(allocator, cursor_node);
    }
    temp.arena->cursor_node = cursor_node;
    if (cursor_node != 0){
        if (temp.pos > 0){
            cursor_node->cursor.pos = temp.pos;
        }
        else{
            temp.arena->cursor_node = cursor_node->prev;
            base_free(allocator, cursor_node);
        }
    }
}
function void
linalloc_clear(Arena *arena){
    Temp_Memory_Arena temp = {arena, 0, 0};
    linalloc_end_temp(temp);
}
function void*
linalloc_wrap_unintialized(String8 data){
    return(data.str);
}
function void *
linalloc_wrap_zero(String8 data)
{
    block_zero(data.str, data.size);
    return(data.str);
}
function void*
linalloc_wrap_write(String8 data, u64 size, void *src)
{
    block_copy(data.str, src, clamp_top(data.size, size));
    return(data.str);
}
#define push_array(a,T,c)         ((T*)linalloc_wrap_unintialized(linalloc_push((a), sizeof(T)*(c), filename_line_number_lit_u8)))
#define push_array_zero(a,T,c)    ((T*)linalloc_wrap_zero        (linalloc_push((a), sizeof(T)*(c), filename_line_number_lit_u8)))
#define push_array_write(a,T,c,s) ((T*)linalloc_wrap_write       (linalloc_push((a), sizeof(T)*(c), filename_line_number_lit_u8), sizeof(T)*(c), (s)))
#define push_array_cursor(a,T,c)  ((T*)linalloc_wrap_unintialized(linalloc_bump((a), sizeof(T)*(c), filename_line_number_lit_u8)))
#define pop_array(a,T,c) (linalloc_pop((a), sizeof(T)*(c)))
#define push_align(a,b)      (linalloc_align((a), (b)))
#define push_align_zero(a,b) (linalloc_wrap_zero(linalloc_align((a), (b))))
#define push_struct(a,T)          push_array(a,T,1)
#define push_struct_zero(a,T)     push_array_zero(a,T,1)

internal Temp_Memory
begin_temp(Cursor *cursor)
{
    Temp_Memory temp = {.kind=LinearAllocatorKind_Cursor};
    temp.temp_memory_cursor = linalloc_begin_temp(cursor);
    return(temp);
}

internal Temp_Memory
begin_temp(Arena *arena)
{
    Temp_Memory temp = {.kind=LinearAllocatorKind_Arena};
    temp.temp_memory_arena = linalloc_begin_temp(arena);
    return(temp);
}

internal void
end_temp(Temp_Memory temp)
{
    switch (temp.kind)
    {
        case LinearAllocatorKind_Cursor:
        {
            linalloc_end_temp(temp.temp_memory_cursor);
        }break;
        case LinearAllocatorKind_Arena:
        {
            linalloc_end_temp(temp.temp_memory_arena);
        }break;
    }
}

////////////////////////////////

function void
thread_ctx_init(Thread_Context *tctx, Thread_Kind kind, Base_Allocator *allocator,
                Base_Allocator *prof_allocator){
    block_zero_struct(tctx);
    tctx->kind = kind;
    tctx->allocator = allocator;
    tctx->node_arena = make_arena(allocator, KB(4), 8);
    
    tctx->prof_allocator = prof_allocator;
    tctx->prof_id_counter = 1;
    tctx->prof_arena = make_arena(prof_allocator, KB(16));
}

function void
thread_ctx_release(Thread_Context *tctx){
    for (Arena_Node *node = tctx->free_arenas;
         node != 0;
         node = node->next){
        linalloc_clear(&node->arena);
    }
    for (Arena_Node *node = tctx->used_first;
         node != 0;
         node = node->next){
        linalloc_clear(&node->arena);
    }
    linalloc_clear(&tctx->node_arena);
    block_zero_struct(tctx);
}

function Arena_Node*
tctx__alloc_arena_node(Thread_Context *tctx){
    Arena_Node *result = tctx->free_arenas;
    if (result != 0){
        sll_stack_pop(tctx->free_arenas);
    }
    else{
        result = push_array_zero(&tctx->node_arena, Arena_Node, 1);
        result->arena = make_arena(tctx->allocator, KB(16), 8);
    }
    return(result);
}

function void
tctx__free_arena_node(Thread_Context *tctx, Arena_Node *node){
    sll_stack_push(tctx->free_arenas, node);
}

function Arena*
tctx_reserve(Thread_Context *tctx){
    Arena_Node *node = tctx->used_first;
    if (node == 0){
        node = tctx__alloc_arena_node(tctx);
        zdll_push_back(tctx->used_first, tctx->used_last, node);
    }
    node->ref_counter += 1;
    return(&node->arena);
}

function Arena*
tctx_reserve(Thread_Context *tctx, Arena *a1){
    Arena_Node *node = tctx->used_first;
    for (; node != 0; node = node->next){
        Arena *na = &node->arena;
        if (na != a1){
            break;
        }
    }
    if (node == 0){
        node = tctx__alloc_arena_node(tctx);
        zdll_push_back(tctx->used_first, tctx->used_last, node);
    }
    node->ref_counter += 1;
    return(&node->arena);
}

function Arena*
tctx_reserve(Thread_Context *tctx, Arena *a1, Arena *a2){
    Arena_Node *node = tctx->used_first;
    for (; node != 0; node = node->next){
        Arena *na = &node->arena;
        if (na != a1 && na != a2){
            break;
        }
    }
    if (node == 0){
        node = tctx__alloc_arena_node(tctx);
        zdll_push_back(tctx->used_first, tctx->used_last, node);
    }
    node->ref_counter += 1;
    return(&node->arena);
}

function Arena*
tctx_reserve(Thread_Context *tctx, Arena *a1, Arena *a2, Arena *a3){
    Arena_Node *node = tctx->used_first;
    for (; node != 0; node = node->next){
        Arena *na = &node->arena;
        if (na != a1 && na != a2 && na != a3){
            break;
        }
    }
    if (node == 0){
        node = tctx__alloc_arena_node(tctx);
        zdll_push_back(tctx->used_first, tctx->used_last, node);
    }
    node->ref_counter += 1;
    return(&node->arena);
}

function void
tctx_release(Thread_Context *tctx, Arena *arena){
    Arena_Node *node = CastFromMember(Arena_Node, arena, arena);
#if 0
    CastFromMember(S=Arena_Node, m=arena, ptr=arena)
    ( (u8*)(arena) - OffsetOfMember(Arena_Node,arena) )
    ( (u8*)(arena) - PtrAsInt(&Member(Arena_Node,arena)) )
#endif
    node->ref_counter -= 1;
    if (node->ref_counter == 0){
        // TODO(allen): make a version of clear that keeps memory allocated from the sytem level
        // but still resets to zero.
        linalloc_clear(&node->arena);
        zdll_remove(tctx->used_first, tctx->used_last, node);
        sll_stack_push(tctx->free_arenas, node);
    }
}

#define heap__sent_init(s) (s)->next=(s)->prev=(s)
#define heap__insert_next(p,n) ((n)->next=(p)->next,(n)->prev=(p),(n)->next->prev=(n),(p)->next=(n))
#define heap__insert_prev(p,n) ((n)->prev=(p)->prev,(n)->next=(p),(n)->prev->next=(n),(p)->prev=(n))
#define heap__remove(n) ((n)->next->prev=(n)->prev,(n)->prev->next=(n)->next)

#if defined(DO_HEAP_CHECKS)
function void
heap_assert_good(Heap *heap){
    if (heap->in_order.next != 0){
        Assert(heap->in_order.prev != 0);
        Assert(heap->free_nodes.next != 0);
        Assert(heap->free_nodes.prev != 0);
        for (Heap_Basic_Node *node = &heap->in_order;;){
            Assert(node->next->prev == node);
            Assert(node->prev->next == node);
            node = node->next;
            if (node == &heap->in_order){
                break;
            }
        }
        for (Heap_Basic_Node *node = &heap->free_nodes;;){
            Assert(node->next->prev == node);
            Assert(node->prev->next == node);
            node = node->next;
            if (node == &heap->free_nodes){
                break;
            }
        }
    }
}
#else
#define heap_assert_good(heap) ((void)(heap))
#endif

function void
heap_init(Heap *heap, Base_Allocator *allocator){
    heap->arena_ = make_arena(allocator);
    heap->arena = &heap->arena_;
    heap__sent_init(&heap->in_order);
    heap__sent_init(&heap->free_nodes);
    heap->used_space = 0;
    heap->total_space = 0;
}

function void
heap_init(Heap *heap, Arena *arena){
    heap->arena = arena;
    heap__sent_init(&heap->in_order);
    heap__sent_init(&heap->free_nodes);
    heap->used_space = 0;
    heap->total_space = 0;
}

function Base_Allocator*
heap_get_base_allocator(Heap *heap){
    return(heap->arena->base_allocator);
}

function void
heap_free_all(Heap *heap){
    if (heap->arena == &heap->arena_){
        linalloc_clear(heap->arena);
    }
    block_zero_struct(heap);
}

function void
heap__extend(Heap *heap, void *memory, u64 size){
    heap_assert_good(heap);
    if (size >= sizeof(Heap_Node)){
        Heap_Node *new_node = (Heap_Node*)memory;
        heap__insert_prev(&heap->in_order, &new_node->order);
        heap__insert_next(&heap->free_nodes, &new_node->alloc);
        new_node->size = size - sizeof(*new_node);
        heap->total_space += size;
    }
    heap_assert_good(heap);
}

function void
heap__extend_automatic(Heap *heap, u64 size){
    void *memory = push_array(heap->arena, u8, size);
    heap__extend(heap, memory, size);
}

function void*
heap__reserve_chunk(Heap *heap, Heap_Node *node, u64 size){
    u8 *ptr = (u8*)(node + 1);
    Assert(node->size >= size);
    u64 left_over_size = node->size - size;
    if (left_over_size > sizeof(*node)){
        u64 new_node_size = left_over_size - sizeof(*node);
        Heap_Node *new_node = (Heap_Node*)(ptr + size);
        heap__insert_next(&node->order, &new_node->order);
        heap__insert_next(&node->alloc, &new_node->alloc);
        new_node->size = new_node_size;
    }
    heap__remove(&node->alloc);
    node->alloc.next = 0;
    node->alloc.prev = 0;
    node->size = size;
    heap->used_space += sizeof(*node) + size;
    return(ptr);
}

function void*
heap_allocate(Heap *heap, u64 size){
    b32 first_try = true;
    for (;;){
        if (heap->in_order.next != 0){
            heap_assert_good(heap);
            u64 aligned_size = (size + sizeof(Heap_Node) - 1);
            aligned_size = aligned_size - (aligned_size%sizeof(Heap_Node));
            for (Heap_Basic_Node *n = heap->free_nodes.next;
                 n != &heap->free_nodes;
                 n = n->next){
                Heap_Node *node = CastFromMember(Heap_Node, alloc, n);
                if (node->size >= aligned_size){
                    void *ptr = heap__reserve_chunk(heap, node, aligned_size);
                    heap_assert_good(heap);
                    return(ptr);
                }
            }
            heap_assert_good(heap);
        }
        
        if (first_try){
            u64 extension_size = clamp_bot(KB(64), size*2);
            heap__extend_automatic(heap, extension_size);
            first_try = false;
        }
        else{
            break;
        }
    }
    return(0);
}

function void
heap__merge(Heap *heap, Heap_Node *l, Heap_Node *r){
    if (&l->order != &heap->in_order && &r->order != &heap->in_order &&
        l->alloc.next != 0 && l->alloc.prev != 0 &&
        r->alloc.next != 0 && r->alloc.prev != 0){
        u8 *ptr = (u8*)(l + 1) + l->size;
        if (PtrDif(ptr, r) == 0){
            heap__remove(&r->order);
            heap__remove(&r->alloc);
            heap__remove(&l->alloc);
            l->size += r->size + sizeof(*r);
            heap__insert_next(&heap->free_nodes, &l->alloc);
        }
    }
}

function void
heap_free(Heap *heap, void *memory){
    if (heap->in_order.next != 0 && memory != 0){
        Heap_Node *node = ((Heap_Node*)memory) - 1;
        Assert(node->alloc.next == 0);
        Assert(node->alloc.prev == 0);
        heap->used_space -= sizeof(*node) + node->size;
        heap_assert_good(heap);
        heap__insert_next(&heap->free_nodes, &node->alloc);
        heap_assert_good(heap);
        heap__merge(heap, node, CastFromMember(Heap_Node, order, node->order.next));
        heap_assert_good(heap);
        heap__merge(heap, CastFromMember(Heap_Node, order, node->order.prev), node);
        heap_assert_good(heap);
    }
}

#define heap_array(heap, T, c) (T*)(heap_allocate((heap), sizeof(T)*(c)))

////////////////////////////////

function void*
base_reserve__heap(void *user_data, u64 size, u64 *size_out, String_Const_u8 location){
    Heap *heap = (Heap*)user_data;
    void *memory = heap_allocate(heap, size);
    *size_out = size;
    return(memory);
}

function void
base_free__heap(void *user_data, void *ptr){
    Heap *heap = (Heap*)user_data;
    heap_free(heap, ptr);
}

function Base_Allocator
base_allocator_on_heap(Heap *heap){
    return(make_base_allocator(base_reserve__heap, 0, 0, base_free__heap, 0, heap));
}

////////////////////////////////

function String8
push_data(Arena *arena, u64 size)
{
    String_Const_u8 result = {};
    result.str = push_array(arena, u8, size);
    result.size = size;
    return(result);
}

function String8
push_data_copy(Arena *arena, String8 data)
{
    String8 result = {};
    result.str = push_array_write(arena, u8, data.size, data.str);
    result.size = data.size;
    return(result);
}

function b32
string_match(String a, String b)
{
    return(a.size == b.size && block_match(a.str, b.str, a.size));
}

////////////////////////////////

function b32
character_is_basic_ascii(char c){
    return(' ' <= c && c <= '~');
}
function b32
character_is_basic_ascii(u8 c){
    return(' ' <= c && c <= '~');
}
function b32
character_is_basic_ascii(u16 c){
    return(' ' <= c && c <= '~');
}
function b32
character_is_basic_ascii(u32 c){
    return(' ' <= c && c <= '~');
}

function b32
character_is_slash(char c){
    return((c == '/') || (c == '\\'));
}
function b32
character_is_slash(u8 c){
    return((c == '/') || (c == '\\'));
}
function b32
character_is_slash(u16 c){
    return((c == '/') || (c == '\\'));
}
function b32
character_is_slash(u32 c){
    return((c == '/') || (c == '\\'));
}

function b32
character_is_upper(char c)
{
    return(('A' <= c) && (c <= 'Z'));
}
inline b32 is_uppercase(u8 c)
{
    return (('A' <= c) && (c <= 'Z'));
}
function b32
character_is_upper(u8 c){
    return(('A' <= c) && (c <= 'Z'));
}
function b32
character_is_upper(u16 c){
    return(('A' <= c) && (c <= 'Z'));
}
function b32
character_is_upper(u32 c){
    return(('A' <= c) && (c <= 'Z'));
}

function b32
character_is_lower(char c)
{
    return(('a' <= c) && (c <= 'z'));
}
function b32
character_is_lower(u8 c){
    return(('a' <= c) && (c <= 'z'));
}
inline b32 is_lowercase(u8 c) { return(('a' <= c) && (c <= 'z'));
}

function b32
character_is_lower(u16 c){
    return(('a' <= c) && (c <= 'z'));
}
function b32
character_is_lower(u32 c){
    return(('a' <= c) && (c <= 'z'));
}

function b32
character_is_lower_unicode(u8 c){
    return((('a' <= c) && (c <= 'z')) || c >= 128);
}
function b32
character_is_lower_unicode(u16 c){
    return((('a' <= c) && (c <= 'z')) || c >= 128);
}
function b32
character_is_lower_unicode(u32 c){
    return((('a' <= c) && (c <= 'z')) || c >= 128);
}

function char
character_to_upper(char c){
    if (('a' <= c) && (c <= 'z')){
        c -= 'a' - 'A';
    }
    return(c);
}
function u8
character_to_upper(u8 c){
    if (('a' <= c) && (c <= 'z')){
        c -= 'a' - 'A';
    }
    return(c);
}
function u16
character_to_upper(u16 c){
    if (('a' <= c) && (c <= 'z')){
        c -= 'a' - 'A';
    }
    return(c);
}
function u32
character_to_upper(u32 c){
    if (('a' <= c) && (c <= 'z')){
        c -= 'a' - 'A';
    }
    return(c);
}
function char
character_to_lower(char c){
    if (('A' <= c) && (c <= 'Z')){
        c += 'a' - 'A';
    }
    return(c);
}
function u8
character_to_lower(u8 c){
    if (('A' <= c) && (c <= 'Z')){
        c += 'a' - 'A';
    }
    return(c);
}
function u16
character_to_lower(u16 c){
    if (('A' <= c) && (c <= 'Z')){
        c += 'a' - 'A';
    }
    return(c);
}
function u32
character_to_lower(u32 c){
    if (('A' <= c) && (c <= 'Z')){
        c += 'a' - 'A';
    }
    return(c);
}

function b32
character_is_whitespace(char c){
    return(c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '\f' || c == '\v');
}
function b32
character_is_whitespace(u8 c){
    return(c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '\f' || c == '\v');
}
function b32
character_is_whitespace(u16 c){
    return(c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '\f' || c == '\v');
}
function b32
character_is_whitespace(u32 c){
    return(c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '\f' || c == '\v');
}

function b32
character_is_base10(char c){
    return('0' <= c && c <= '9');
}
function b32
character_is_base10(u8 c){
    return('0' <= c && c <= '9');
}
function b32
character_is_base10(u16 c){
    return('0' <= c && c <= '9');
}
function b32
character_is_base10(u32 c){
    return('0' <= c && c <= '9');
}

function b32
character_is_base16(char c){
    return(('0' <= c && c <= '9') || ('A' <= c && c <= 'F'));
}
function b32
character_is_base16(u8 c){
    return(('0' <= c && c <= '9') || ('A' <= c && c <= 'F'));
}
function b32
character_is_base16(u16 c){
    return(('0' <= c && c <= '9') || ('A' <= c && c <= 'F'));
}
function b32
character_is_base16(u32 c){
    return(('0' <= c && c <= '9') || ('A' <= c && c <= 'F'));
}

function b32
character_is_base64(char c){
    return(('0' <= c && c <= '9') ||
           ('a' <= c && c <= 'z') ||
           ('A' <= c && c <= 'Z') ||
           c == '_' || c == '$' || c == '?');
}
function b32
character_is_base64(u8 c){
    return(('0' <= c && c <= '9') ||
           ('a' <= c && c <= 'z') ||
           ('A' <= c && c <= 'Z') ||
           c == '_' || c == '$' || c == '?');
}
function b32
character_is_base64(u16 c){
    return(('0' <= c && c <= '9') ||
           ('a' <= c && c <= 'z') ||
           ('A' <= c && c <= 'Z') ||
           c == '_' || c == '$' || c == '?');
}
function b32
character_is_base64(u32 c){
    return(('0' <= c && c <= '9') ||
           ('a' <= c && c <= 'z') ||
           ('A' <= c && c <= 'Z') ||
           c == '_' || c == '$' || c == '?');
}

function b32
character_is_alpha(char c){
    return( (('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z')) || c == '_');
}
function b32
character_is_alpha(u8 c){
    return( (('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z')) || c == '_');
}
function b32
character_is_alpha(u16 c){
    return( (('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z')) || c == '_');
}
function b32
character_is_alpha(u32 c){
    return( (('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z')) || c == '_');
}

function b32
character_is_alpha_numeric(char c){
    return((('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z')) || (('0' <= c) && (c <= '9')) || c == '_');
}
function b32
character_is_alpha_numeric(u8 c){
    return((('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z')) || (('0' <= c) && (c <= '9')) || c == '_');
}
function b32
character_is_alpha_numeric(u16 c){
    return((('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z')) || (('0' <= c) && (c <= '9')) || c == '_');
}
function b32
character_is_alpha_numeric(u32 c){
    return((('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z')) || (('0' <= c) && (c <= '9')) || c == '_');
}


function b32
character_is_alpha_unicode(u8 c){
    return( (('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z')) || c == '_' || c >= 128);
}
function b32
character_is_alpha_unicode(u16 c){
    return( (('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z')) || c == '_' || c >= 128);
}
function b32
character_is_alpha_unicode(u32 c){
    return( (('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z')) || c == '_' || c >= 128);
}

function b32
character_is_alpha_numeric_unicode(u8 c){
    return((('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z')) || (('0' <= c) && (c <= '9')) || c == '_' || c >= 128);
}
function b32
character_is_alpha_numeric_unicode(u16 c){
    return((('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z')) || (('0' <= c) && (c <= '9')) || c == '_' || c >= 128);
}
function b32
character_is_alpha_numeric_unicode(u32 c){
    return((('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z')) || (('0' <= c) && (c <= '9')) || c == '_' || c >= 128);
}

function char
string_get_character(String_Const_char str, u64 i){
    char r = 0;
    if (i < str.size){
        r = str.str[i];
    }
    return(r);
}
function u8
string_get_character(String_Const_u8 str, u64 i){
    u8 r = 0;
    if (i < str.size){
        r = str.str[i];
    }
    return(r);
}
function u16
string_get_character(String_Const_u16 str, u64 i){
    u16 r = 0;
    if (i < str.size){
        r = str.str[i];
    }
    return(r);
}
function u32
string_get_character(String_Const_u32 str, u64 i){
    u32 r = 0;
    if (i < str.size){
        r = str.str[i];
    }
    return(r);
}

function String_Const_char
string_prefix(String_Const_char str, u64 size){
    size = clamp_top(size, str.size);
    str.size = size;
    return(str);
}
function String_Const_u8
string_prefix(String_Const_u8 str, u64 size){
    size = clamp_top(size, str.size);
    str.size = size;
    return(str);
}
function String_Const_u16
string_prefix(String_Const_u16 str, u64 size){
    size = clamp_top(size, str.size);
    str.size = size;
    return(str);
}
function String_Const_u32
string_prefix(String_Const_u32 str, u64 size){
    size = clamp_top(size, str.size);
    str.size = size;
    return(str);
}

function String_Const_Any
string_prefix(String_Const_Any str, u64 size){
    switch (str.encoding){
        case StringEncoding_ASCII: str.s_char = string_prefix(str.s_char, size); break;
        case StringEncoding_UTF8:  str.s_u8   = string_prefix(str.s_u8  , size); break;
        case StringEncoding_UTF16: str.s_u16  = string_prefix(str.s_u16 , size); break;
        case StringEncoding_UTF32: str.s_u32  = string_prefix(str.s_u32 , size); break;
    }
    return(str);
}

function String_Const_char
string_postfix(String_Const_char str, u64 size){
    size = clamp_top(size, str.size);
    str.str += (str.size - size);
    str.size = size;
    return(str);
}
function String_Const_u8
string_postfix(String_Const_u8 str, u64 size){
    size = clamp_top(size, str.size);
    str.str += (str.size - size);
    str.size = size;
    return(str);
}
function String_Const_u16
string_postfix(String_Const_u16 str, u64 size){
    size = clamp_top(size, str.size);
    str.str += (str.size - size);
    str.size = size;
    return(str);
}
function String_Const_u32
string_postfix(String_Const_u32 str, u64 size){
    size = clamp_top(size, str.size);
    str.str += (str.size - size);
    str.size = size;
    return(str);
}

function String_Const_Any
string_postfix(String_Const_Any str, u64 size){
    switch (str.encoding){
        case StringEncoding_ASCII: str.s_char = string_postfix(str.s_char, size); break;
        case StringEncoding_UTF8:  str.s_u8   = string_postfix(str.s_u8  , size); break;
        case StringEncoding_UTF16: str.s_u16  = string_postfix(str.s_u16 , size); break;
        case StringEncoding_UTF32: str.s_u32  = string_postfix(str.s_u32 , size); break;
    }
    return(str);
}

function String_Const_char
string_skip(String_Const_char str, u64 n){
    n = clamp_top(n, str.size);
    str.str += n;;
    str.size -= n;
    return(str);
}
function String_Const_u8
string_skip(String_Const_u8 str, u64 n){
    n = clamp_top(n, str.size);
    str.str += n;;
    str.size -= n;
    return(str);
}
function String_Const_u16
string_skip(String_Const_u16 str, u64 n){
    n = clamp_top(n, str.size);
    str.str += n;;
    str.size -= n;
    return(str);
}
function String_Const_u32
string_skip(String_Const_u32 str, u64 n){
    n = clamp_top(n, str.size);
    str.str += n;;
    str.size -= n;
    return(str);
}

function String_Const_Any
string_skip(String_Const_Any str, u64 n){
    switch (str.encoding){
        case StringEncoding_ASCII: str.s_char = string_skip(str.s_char, n); break;
        case StringEncoding_UTF8:  str.s_u8   = string_skip(str.s_u8  , n); break;
        case StringEncoding_UTF16: str.s_u16  = string_skip(str.s_u16 , n); break;
        case StringEncoding_UTF32: str.s_u32  = string_skip(str.s_u32 , n); break;
    }
    return(str);
}

function String_Const_char
string_chop(String_Const_char str, u64 n)
{
    n = clamp_top(n, str.size);
    str.size -= n;
    return(str);
}
function String_Const_u8
string_chop(String_Const_u8 str, u64 n)
{
    n = clamp_top(n, str.size);
    str.size -= n;
    return(str);
}
function String_Const_u16
string_chop(String_Const_u16 str, u64 n){
    n = clamp_top(n, str.size);
    str.size -= n;
    return(str);
}
function String_Const_u32
string_chop(String_Const_u32 str, u64 n){
    n = clamp_top(n, str.size);
    str.size -= n;
    return(str);
}

function String_Const_Any
string_chop(String_Const_Any str, u64 n){
    switch (str.encoding){
        case StringEncoding_ASCII: str.s_char = string_chop(str.s_char, n); break;
        case StringEncoding_UTF8:  str.s_u8   = string_chop(str.s_u8  , n); break;
        case StringEncoding_UTF16: str.s_u16  = string_chop(str.s_u16 , n); break;
        case StringEncoding_UTF32: str.s_u32  = string_chop(str.s_u32 , n); break;
    }
    return(str);
}

function String_Const_char
string_substring(String_Const_char str, Range_i64 range){
    return(SCchar(str.str + range.min, str.str + range.max));
}
function String_Const_u8
string_substring(String_Const_u8 str, Range_i64 range){
    return(SCu8(str.str + range.min, str.str + range.max));
}
function String_Const_u16
string_substring(String_Const_u16 str, Range_i64 range){
    return(SCu16(str.str + range.min, str.str + range.max));
}
function String_Const_u32
string_substring(String_Const_u32 str, Range_i64 range){
    return(SCu32(str.str + range.min, str.str + range.max));
}

function u64
string_find_first(String_Const_char str, u64 start_pos, char c){
    u64 i = start_pos;
    for (;i < str.size && c != str.str[i]; i += 1);
    return(i);
}
function u64
string_find_first(String_Const_u8 str, u64 start_pos, u8 c){
    u64 i = start_pos;
    for (;i < str.size && c != str.str[i]; i += 1);
    return(i);
}
function u64
string_find_first(String_Const_u16 str, u64 start_pos, u16 c){
    u64 i = start_pos;
    for (;i < str.size && c != str.str[i]; i += 1);
    return(i);
}
function u64
string_find_first(String_Const_u32 str, u64 start_pos, u32 c){
    u64 i = start_pos;
    for (;i < str.size && c != str.str[i]; i += 1);
    return(i);
}

function u64
string_find_first(String_Const_char str, char c){
    return(string_find_first(str, 0, c));
}
function u64
string_find_first(String_Const_u8 str, u8 c){
    return(string_find_first(str, 0, c));
}
function u64
string_find_first(String_Const_u16 str, u16 c){
    return(string_find_first(str, 0, c));
}
function u64
string_find_first(String_Const_u32 str, u32 c){
    return(string_find_first(str, 0, c));
}

function i64
string_find_last(String_Const_char str, char c){
    i64 size = (i64)str.size;
    i64 i = size - 1;
    for (;i >= 0 && c != str.str[i]; i -= 1);
    return(i);
}
function i64
string_find_last(String_Const_u8 str, u8 c){
    i64 size = (i64)str.size;
    i64 i = size - 1;
    for (;i >= 0 && c != str.str[i]; i -= 1);
    return(i);
}
function i64
string_find_last(String_Const_u16 str, u16 c){
    i64 size = (i64)str.size;
    i64 i = size - 1;
    for (;i >= 0 && c != str.str[i]; i -= 1);
    return(i);
}
function i64
string_find_last(String_Const_u32 str, u32 c){
    i64 size = (i64)str.size;
    i64 i = size - 1;
    for (;i >= 0 && c != str.str[i]; i -= 1);
    return(i);
}

function u64
string_find_first_whitespace(String_Const_char str){
    u64 i = 0;
    for (;i < str.size && !character_is_whitespace(str.str[i]); i += 1);
    return(i);
}
function u64
string_find_first_whitespace(String_Const_u8 str){
    u64 i = 0;
    for (;i < str.size && !character_is_whitespace(str.str[i]); i += 1);
    return(i);
}
function u64
string_find_first_whitespace(String_Const_u16 str){
    u64 i = 0;
    for (;i < str.size && !character_is_whitespace(str.str[i]); i += 1);
    return(i);
}
function u64
string_find_first_whitespace(String_Const_u32 str){
    u64 i = 0;
    for (;i < str.size && !character_is_whitespace(str.str[i]); i += 1);
    return(i);
}
function i64
string_find_last_whitespace(String_Const_char str){
    i64 size = (i64)str.size;
    i64 i = size - 1;
    for (;i >= 0 && !character_is_whitespace(str.str[i]); i -= 1);
    return(i);
}
function i64
string_find_last_whitespace(String_Const_u8 str){
    i64 size = (i64)str.size;
    i64 i = size - 1;
    for (;i >= 0 && !character_is_whitespace(str.str[i]); i -= 1);
    return(i);
}
function i64
string_find_last_whitespace(String_Const_u16 str){
    i64 size = (i64)str.size;
    i64 i = size - 1;
    for (;i >= 0 && !character_is_whitespace(str.str[i]); i -= 1);
    return(i);
}
function i64
string_find_last_whitespace(String_Const_u32 str){
    i64 size = (i64)str.size;
    i64 i = size - 1;
    for (;i >= 0 && !character_is_whitespace(str.str[i]); i -= 1);
    return(i);
}

function u64
string_find_first_non_whitespace(String_Const_char str){
    u64 i = 0;
    for (;i < str.size && character_is_whitespace(str.str[i]); i += 1);
    return(i);
}
function u64
string_find_first_non_whitespace(String_Const_u8 str){
    u64 i = 0;
    for (;i < str.size && character_is_whitespace(str.str[i]); i += 1);
    return(i);
}
function u64
string_find_first_non_whitespace(String_Const_u16 str){
    u64 i = 0;
    for (;i < str.size && character_is_whitespace(str.str[i]); i += 1);
    return(i);
}
function u64
string_find_first_non_whitespace(String_Const_u32 str){
    u64 i = 0;
    for (;i < str.size && character_is_whitespace(str.str[i]); i += 1);
    return(i);
}
function i64
string_find_last_non_whitespace(String_Const_char str){
    i64 size = (i64)str.size;
    i64 i = size - 1;
    for (;i >= 0 && character_is_whitespace(str.str[i]); i -= 1);
    return(i);
}
function i64
string_find_last_non_whitespace(String_Const_u8 str){
    i64 size = (i64)str.size;
    i64 i = size - 1;
    for (;i >= 0 && character_is_whitespace(str.str[i]); i -= 1);
    return(i);
}
function i64
string_find_last_non_whitespace(String_Const_u16 str){
    i64 size = (i64)str.size;
    i64 i = size - 1;
    for (;i >= 0 && character_is_whitespace(str.str[i]); i -= 1);
    return(i);
}
function i64
string_find_last_non_whitespace(String_Const_u32 str){
    i64 size = (i64)str.size;
    i64 i = size - 1;
    for (;i >= 0 && character_is_whitespace(str.str[i]); i -= 1);
    return(i);
}

function u64
string_find_first_slash(String_Const_char str){
    u64 i = 0;
    for (;i < str.size && !character_is_slash(str.str[i]); i += 1);
    return(i);
}
function u64
string_find_first_slash(String_Const_u8 str){
    u64 i = 0;
    for (;i < str.size && !character_is_slash(str.str[i]); i += 1);
    return(i);
}
function u64
string_find_first_slash(String_Const_u16 str){
    u64 i = 0;
    for (;i < str.size && !character_is_slash(str.str[i]); i += 1);
    return(i);
}
function u64
string_find_first_slash(String_Const_u32 str){
    u64 i = 0;
    for (;i < str.size && !character_is_slash(str.str[i]); i += 1);
    return(i);
}
function i64
string_find_last_slash(String_Const_char str){
    i64 size = (i64)str.size;
    i64 i = size - 1;
    for (;i >= 0 && !character_is_slash(str.str[i]); i -= 1);
    return(i);
}
function i64
string_find_last_slash(String_Const_u8 str){
    i64 size = (i64)str.size;
    i64 i = size - 1;
    for (;i >= 0 && !character_is_slash(str.str[i]); i -= 1);
    return(i);
}
function i64
string_find_last_slash(String_Const_u16 str){
    i64 size = (i64)str.size;
    i64 i = size - 1;
    for (;i >= 0 && !character_is_slash(str.str[i]); i -= 1);
    return(i);
}
function i64
string_find_last_slash(String_Const_u32 str){
    i64 size = (i64)str.size;
    i64 i = size - 1;
    for (;i >= 0 && !character_is_slash(str.str[i]); i -= 1);
    return(i);
}

function String_Const_char
string_remove_last_folder(String_Const_char str)
{
    if (str.size > 0){
        str.size -= 1;
    }
    i64 slash_pos = string_find_last_slash(str);
    if (slash_pos < 0){
        str.size = 0;
    }
    else{
        str.size = slash_pos + 1;
    }
    return(str);
}

function String8
path_dirname(String8 str)
{
    if (str.size > 0)
    {// NOTE: Remove the last slash
        str.size -= 1;
    }
    i64 slash_pos = string_find_last_slash(str);
    
    str.size = 0;
    if (slash_pos >= 0) str.size = slash_pos + 1;
    
    return(str);
}

function String_Const_u16
string_remove_last_folder(String_Const_u16 str){
    if (str.size > 0){
        str.size -= 1;
    }
    i64 slash_pos = string_find_last_slash(str);
    if (slash_pos < 0){
        str.size = 0;
    }
    else{
        str.size = slash_pos + 1;
    }
    return(str);
}
function String_Const_u32
string_remove_last_folder(String_Const_u32 str){
    if (str.size > 0){
        str.size -= 1;
    }
    i64 slash_pos = string_find_last_slash(str);
    if (slash_pos < 0){
        str.size = 0;
    }
    else{
        str.size = slash_pos + 1;
    }
    return(str);
}

function b32
string_looks_like_drive_letter(String_Const_u8 string){
    b32 result = false;
    if (string.size == 3 &&
        character_is_alpha(string.str[0]) &&
        string.str[1] == ':' &&
        character_is_slash(string.str[2])){
        result = true;
    }
    return(result);
}

function String_Const_char
string_remove_front_of_path(String_Const_char str){
    i64 slash_pos = string_find_last_slash(str);
    if (slash_pos < 0){
        str.size = 0;
    }
    else{
        str.size = slash_pos + 1;
    }
    return(str);
}
function String_Const_u8
string_remove_front_of_path(String_Const_u8 str){
    i64 slash_pos = string_find_last_slash(str);
    if (slash_pos < 0){
        str.size = 0;
    }
    else{
        str.size = slash_pos + 1;
    }
    return(str);
}
function String_Const_u16
string_remove_front_of_path(String_Const_u16 str){
    i64 slash_pos = string_find_last_slash(str);
    if (slash_pos < 0){
        str.size = 0;
    }
    else{
        str.size = slash_pos + 1;
    }
    return(str);
}
function String_Const_u32
string_remove_front_of_path(String_Const_u32 str){
    i64 slash_pos = string_find_last_slash(str);
    if (slash_pos < 0){
        str.size = 0;
    }
    else{
        str.size = slash_pos + 1;
    }
    return(str);
}

function String_Const_char
string_front_of_path(String_Const_char str){
    i64 slash_pos = string_find_last_slash(str);
    if (slash_pos >= 0){
        str = string_skip(str, slash_pos + 1);
    }
    return(str);
}
function String_Const_u8
string_front_of_path(String_Const_u8 str){
    i64 slash_pos = string_find_last_slash(str);
    if (slash_pos >= 0){
        str = string_skip(str, slash_pos + 1);
    }
    return(str);
}
function String_Const_u16
string_front_of_path(String_Const_u16 str){
    i64 slash_pos = string_find_last_slash(str);
    if (slash_pos >= 0){
        str = string_skip(str, slash_pos + 1);
    }
    return(str);
}
function String_Const_u32
string_front_of_path(String_Const_u32 str){
    i64 slash_pos = string_find_last_slash(str);
    if (slash_pos >= 0){
        str = string_skip(str, slash_pos + 1);
    }
    return(str);
}

function String_Const_u8
string_remove_front_folder_of_path(String_Const_u8 str){
    i64 slash_pos = string_find_last_slash(string_chop(str, 1));
    if (slash_pos < 0){
        str.size = 0;
    }
    else{
        str.size = slash_pos + 1;
    }
    return(str);
}
function String_Const_u8
string_front_folder_of_path(String_Const_u8 str){
    i64 slash_pos = string_find_last_slash(string_chop(str, 1));
    if (slash_pos >= 0){
        str = string_skip(str, slash_pos + 1);
    }
    return(str);
}

function String_Const_char
string_file_extension(String_Const_char string){
    return(string_skip(string, string_find_last(string, '.') + 1));
}
function String_Const_u8
string_file_extension(String_Const_u8 string){
    return(string_skip(string, string_find_last(string, '.') + 1));
}
function String_Const_u16
string_file_extension(String_Const_u16 string){
    return(string_skip(string, string_find_last(string, '.') + 1));
}
function String_Const_u32
string_file_extension(String_Const_u32 string){
    return(string_skip(string, string_find_last(string, '.') + 1));
}

function String_Const_char
string_file_without_extension(String_Const_char string){
    i64 pos = string_find_last(string, '.');
    if (pos > 0){
        string = string_prefix(string, pos);
    }
    return(string);
}
function String_Const_u8
string_file_without_extension(String_Const_u8 string){
    i64 pos = string_find_last(string, '.');
    if (pos > 0){
        string = string_prefix(string, pos);
    }
    return(string);
}
function String_Const_u16
string_file_without_extension(String_Const_u16 string){
    i64 pos = string_find_last(string, '.');
    if (pos > 0){
        string = string_prefix(string, pos);
    }
    return(string);
}
function String_Const_u32
string_file_without_extension(String_Const_u32 string){
    i64 pos = string_find_last(string, '.');
    if (pos > 0){
        string = string_prefix(string, pos);
    }
    return(string);
}

function String_Const_char
string_skip_whitespace(String_Const_char str){
    u64 f = string_find_first_non_whitespace(str);
    str = string_skip(str, f);
    return(str);
}
function String_Const_u8
string_skip_whitespace(String_Const_u8 str){
    u64 f = string_find_first_non_whitespace(str);
    str = string_skip(str, f);
    return(str);
}
function String_Const_u16
string_skip_whitespace(String_Const_u16 str){
    u64 f = string_find_first_non_whitespace(str);
    str = string_skip(str, f);
    return(str);
}
function String_Const_u32
string_skip_whitespace(String_Const_u32 str){
    u64 f = string_find_first_non_whitespace(str);
    str = string_skip(str, f);
    return(str);
}

function String_Const_char
string_chop_whitespace(String_Const_char str){
    i64 e = string_find_last_non_whitespace(str);
    str = string_prefix(str, (u64)(e + 1));
    return(str);
}
function String_Const_u8
string_chop_whitespace(String_Const_u8 str){
    i64 e = string_find_last_non_whitespace(str);
    str = string_prefix(str, (u64)(e + 1));
    return(str);
}
function String_Const_u16
string_chop_whitespace(String_Const_u16 str){
    i64 e = string_find_last_non_whitespace(str);
    str = string_prefix(str, (u64)(e + 1));
    return(str);
}
function String_Const_u32
string_chop_whitespace(String_Const_u32 str){
    i64 e = string_find_last_non_whitespace(str);
    str = string_prefix(str, (u64)(e + 1));
    return(str);
}

function String_Const_char
string_skip_chop_whitespace(String_Const_char str){
    u64 f = string_find_first_non_whitespace(str);
    str = string_skip(str, f);
    i64 e = string_find_last_non_whitespace(str);
    str = string_prefix(str, (u64)(e + 1));
    return(str);
}
function String_Const_u8
string_skip_chop_whitespace(String_Const_u8 str){
    u64 f = string_find_first_non_whitespace(str);
    str = string_skip(str, f);
    i64 e = string_find_last_non_whitespace(str);
    str = string_prefix(str, (u64)(e + 1));
    return(str);
}
function String_Const_u16
string_skip_chop_whitespace(String_Const_u16 str){
    u64 f = string_find_first_non_whitespace(str);
    str = string_skip(str, f);
    i64 e = string_find_last_non_whitespace(str);
    str = string_prefix(str, (u64)(e + 1));
    return(str);
}
function String_Const_u32
string_skip_chop_whitespace(String_Const_u32 str){
    u64 f = string_find_first_non_whitespace(str);
    str = string_skip(str, f);
    i64 e = string_find_last_non_whitespace(str);
    str = string_prefix(str, (u64)(e + 1));
    return(str);
}

function b32
string_match(String_Const_char a, String_Const_char b)
{
    b32 result = false;
    if (a.size == b.size)
    {
        result = true;
        for (u64 i = 0; i < a.size; i += 1)
        {
            if (a.str[i] != b.str[i]){
                result = false;
                break;
            }
        }
    }
    return(result);
}
function b32
string_match(String_Const_u16 a, String_Const_u16 b){
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
function b32
string_match(String_Const_u32 a, String_Const_u32 b){
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

function b32
string_match(String_Const_Any a, String_Const_Any b){
    b32 result = false;
    if (a.encoding == b.encoding){
        switch (a.encoding){
            case StringEncoding_ASCII: result = string_match(a.s_char, b.s_char); break;
            case StringEncoding_UTF8:  result = string_match(a.s_u8  , b.s_u8  ); break;
            case StringEncoding_UTF16: result = string_match(a.s_u16 , b.s_u16 ); break;
            case StringEncoding_UTF32: result = string_match(a.s_u32 , b.s_u32 ); break;
        }
    }
    return(result);
}

internal b32
string_ends_with(String string, String test)
{
    b32 result = string_match(string_postfix(string, test.size), test);
    return result;
}

function b32
string_match_insensitive(String_Const_char a, String_Const_char b){
    b32 result = false;
    if (a.size == b.size){
        result = true;
        for (u64 i = 0; i < a.size; i += 1){
            if (character_to_upper(a.str[i]) != character_to_upper(b.str[i])){
                result = false;
                break;
            }
        }
    }
    return(result);
}
function b32
string_match_insensitive(String_Const_u8 a, String_Const_u8 b){
    b32 result = false;
    if (a.size == b.size){
        result = true;
        for (u64 i = 0; i < a.size; i += 1){
            if (character_to_upper(a.str[i]) != character_to_upper(b.str[i])){
                result = false;
                break;
            }
        }
    }
    return(result);
}
function b32
string_match_insensitive(String_Const_u16 a, String_Const_u16 b){
    b32 result = false;
    if (a.size == b.size){
        result = true;
        for (u64 i = 0; i < a.size; i += 1){
            if (character_to_upper(a.str[i]) != character_to_upper(b.str[i])){
                result = false;
                break;
            }
        }
    }
    return(result);
}
function b32
string_match_insensitive(String_Const_u32 a, String_Const_u32 b){
    b32 result = false;
    if (a.size == b.size){
        result = true;
        for (u64 i = 0; i < a.size; i += 1){
            if (character_to_upper(a.str[i]) != character_to_upper(b.str[i])){
                result = false;
                break;
            }
        }
    }
    return(result);
}

function b32
string_match(String_Const_char a, String_Const_char b, String_Match_Rule rule)
{
    b32 result = false;
    switch (rule){
        case StringMatch_Exact:
        {
            result = string_match(a, b);
        }break;
        case StringMatch_CaseInsensitive:
        {
            result = string_match_insensitive(a, b);
        }break;
    }
    return(result);
}
function b32
string_match(String8 a, String8 b, String_Match_Rule rule)
{
    b32 result = false;
    switch (rule)
    {
        case StringMatch_Exact:
        {
            result = string_match(a, b);
        }break;
        
        case StringMatch_CaseInsensitive:
        {
            result = string_match_insensitive(a, b);
        }break;
    }
    return(result);
}
function b32
string_match(String_Const_u16 a, String_Const_u16 b, String_Match_Rule rule){
    b32 result = false;
    switch (rule){
        case StringMatch_Exact:
        {
            result = string_match(a, b);
        }break;
        case StringMatch_CaseInsensitive:
        {
            result = string_match_insensitive(a, b);
        }break;
    }
    return(result);
}
function b32
string_match(String_Const_u32 a, String_Const_u32 b, String_Match_Rule rule){
    b32 result = false;
    switch (rule){
        case StringMatch_Exact:
        {
            result = string_match(a, b);
        }break;
        case StringMatch_CaseInsensitive:
        {
            result = string_match_insensitive(a, b);
        }break;
    }
    return(result);
}

function u64
string_find_first(String_Const_char str, String_Const_char needle, String_Match_Rule rule){
    u64 i = 0;
    if (needle.size > 0){
        i = str.size;
        if (str.size >= needle.size){
            i = 0;
            char c = character_to_upper(needle.str[0]);
            u64 one_past_last = str.size - needle.size + 1;
            for (;i < one_past_last; i += 1){
                if (character_to_upper(str.str[i]) == c){
                    String_Const_char source_part = string_prefix(string_skip(str, i), needle.size);
                    if (string_match(source_part, needle, rule)){
                        break;
                    }
                }
            }
            if (i == one_past_last){
                i = str.size;
            }
        }
    }
    return(i);
}
function u64
string_find_first(String_Const_u8 str, String_Const_u8 needle, String_Match_Rule rule){
    u64 i = 0;
    if (needle.size > 0){
        i = str.size;
        if (str.size >= needle.size){
            i = 0;
            u8 c = character_to_upper(needle.str[0]);
            u64 one_past_last = str.size - needle.size + 1;
            for (;i < one_past_last; i += 1){
                if (character_to_upper(str.str[i]) == c){
                    String_Const_u8 source_part = string_prefix(string_skip(str, i), needle.size);
                    if (string_match(source_part, needle, rule)){
                        break;
                    }
                }
            }
            if (i == one_past_last){
                i = str.size;
            }
        }
    }
    return(i);
}
function u64
string_find_first(String_Const_u16 str, String_Const_u16 needle, String_Match_Rule rule){
    u64 i = 0;
    if (needle.size > 0){
        i = str.size;
        if (str.size >= needle.size){
            i = 0;
            u16 c = character_to_upper(needle.str[0]);
            u64 one_past_last = str.size - needle.size + 1;
            for (;i < one_past_last; i += 1){
                if (character_to_upper(str.str[i]) == c){
                    String_Const_u16 source_part = string_prefix(string_skip(str, i), needle.size);
                    if (string_match(source_part, needle, rule)){
                        break;
                    }
                }
            }
            if (i == one_past_last){
                i = str.size;
            }
        }
    }
    return(i);
}
function u64
string_find_first(String_Const_u32 str, String_Const_u32 needle, String_Match_Rule rule){
    u64 i = 0;
    if (needle.size > 0){
        i = str.size;
        if (str.size >= needle.size){
            i = 0;
            u32 c = character_to_upper(needle.str[0]);
            u64 one_past_last = str.size - needle.size + 1;
            for (;i < one_past_last; i += 1){
                if (character_to_upper(str.str[i]) == c){
                    String_Const_u32 source_part = string_prefix(string_skip(str, i), needle.size);
                    if (string_match(source_part, needle, rule)){
                        break;
                    }
                }
            }
            if (i == one_past_last){
                i = str.size;
            }
        }
    }
    return(i);
}

function u64
string_find_first(String_Const_char str, String_Const_char needle){
    return(string_find_first(str, needle, StringMatch_Exact));
}
function u64
string_find_first(String_Const_u8 str, String_Const_u8 needle){
    return(string_find_first(str, needle, StringMatch_Exact));
}
function u64
string_find_first(String_Const_u16 str, String_Const_u16 needle){
    return(string_find_first(str, needle, StringMatch_Exact));
}
function u64
string_find_first(String_Const_u32 str, String_Const_u32 needle){
    return(string_find_first(str, needle, StringMatch_Exact));
}
function u64
string_find_first_insensitive(String_Const_char str, String_Const_char needle){
    return(string_find_first(str, needle, StringMatch_CaseInsensitive));
}
function u64
string_find_first_insensitive(String_Const_u8 str, String_Const_u8 needle){
    return(string_find_first(str, needle, StringMatch_CaseInsensitive));
}
function u64
string_find_first_insensitive(String_Const_u16 str, String_Const_u16 needle){
    return(string_find_first(str, needle, StringMatch_CaseInsensitive));
}
function u64
string_find_first_insensitive(String_Const_u32 str, String_Const_u32 needle){
    return(string_find_first(str, needle, StringMatch_CaseInsensitive));
}

function b32
string_has_substr(String_Const_u8 str, String_Const_u8 needle, String_Match_Rule rule){
    return(string_find_first(str, needle, rule) < str.size);
}

function b32
string_has_substr(String_Const_u8 str, String_Const_u8 needle){
    return(string_find_first(str, needle, StringMatch_Exact) < str.size);
}

function i32
string_compare(String_Const_char a, String_Const_char b){
    i32 result = 0;
    for (u64 i = 0; i < a.size || i < b.size; i += 1){
        char ca = (i < a.size)?a.str[i]:0;
        char cb = (i < b.size)?b.str[i]:0;
        i32 dif = ((ca) - (cb));
        if (dif != 0){
            result = (dif > 0)?1:-1;
            break;
        }
    }
    return(result);
}
function i32
string_compare(String8 a, String8 b)
{
    i32 result = 0;
    for (u64 i = 0; i < a.size || i < b.size; i += 1){
        u8 ca = (i < a.size)?a.str[i]:0;
        u8 cb = (i < b.size)?b.str[i]:0;
        i32 dif = ((ca) - (cb));
        if (dif != 0){
            result = (dif > 0)?1:-1;
            break;
        }
    }
    return(result);
}

// @Deprecated Use "string_match"
inline b32 string_equal(String8 a, String8 b) { return string_match(a,b); }

inline b32
string_equal(String8 a, char *b)
{
  return string_match(a, SCu8(b));
}
inline b32
string_equal(String8 a, char c)
{
  return (a.size == 1) && (a.str[0] == c);
}
function i32
string_compare(String_Const_u16 a, String_Const_u16 b){
    i32 result = 0;
    for (u64 i = 0; i < a.size || i < b.size; i += 1){
        u16 ca = (i < a.size)?a.str[i]:0;
        u16 cb = (i < b.size)?b.str[i]:0;
        i32 dif = ((ca) - (cb));
        if (dif != 0){
            result = (dif > 0)?1:-1;
            break;
        }
    }
    return(result);
}
function i32
string_compare(String_Const_u32 a, String_Const_u32 b){
    i32 result = 0;
    for (u64 i = 0; i < a.size || i < b.size; i += 1){
        u32 ca = (i < a.size)?a.str[i]:0;
        u32 cb = (i < b.size)?b.str[i]:0;
        i32 dif = ((ca) - (cb));
        if (dif != 0){
            result = (dif > 0)?1:-1;
            break;
        }
    }
    return(result);
}

function i32
string_compare_insensitive(String_Const_char a, String_Const_char b){
    i32 result = 0;
    for (u64 i = 0; i < a.size || i < b.size; i += 1){
        char ca = (i <= a.size)?0:a.str[i];
        char cb = (i <= b.size)?0:b.str[i];
        i32 dif = character_to_upper(ca) - character_to_upper(cb);
        if (dif != 0){
            result = (dif > 0)?1:-1;
            break;
        }
    }
    return(result);
}
function i32
string_compare_insensitive(String_Const_u8 a, String_Const_u8 b){
    i32 result = 0;
    for (u64 i = 0; i < a.size || i < b.size; i += 1){
        u8 ca = (i <= a.size)?0:a.str[i];
        u8 cb = (i <= b.size)?0:b.str[i];
        i32 dif = character_to_upper(ca) - character_to_upper(cb);
        if (dif != 0){
            result = (dif > 0)?1:-1;
            break;
        }
    }
    return(result);
}
function i32
string_compare_insensitive(String_Const_u16 a, String_Const_u16 b){
    i32 result = 0;
    for (u64 i = 0; i < a.size || i < b.size; i += 1){
        u16 ca = (i <= a.size)?0:a.str[i];
        u16 cb = (i <= b.size)?0:b.str[i];
        i32 dif = character_to_upper(ca) - character_to_upper(cb);
        if (dif != 0){
            result = (dif > 0)?1:-1;
            break;
        }
    }
    return(result);
}
function i32
string_compare_insensitive(String_Const_u32 a, String_Const_u32 b){
    i32 result = 0;
    for (u64 i = 0; i < a.size || i < b.size; i += 1){
        u32 ca = (i <= a.size)?0:a.str[i];
        u32 cb = (i <= b.size)?0:b.str[i];
        i32 dif = character_to_upper(ca) - character_to_upper(cb);
        if (dif != 0){
            result = (dif > 0)?1:-1;
            break;
        }
    }
    return(result);
}

function String_Const_char
string_mod_upper(String_Const_char str){
    for (u64 i = 0; i < str.size; i += 1){
        str.str[i] = character_to_upper(str.str[i]);
    }
    return(str);
}
function String_Const_u8
string_mod_upper(String_Const_u8 str){
    for (u64 i = 0; i < str.size; i += 1){
        str.str[i] = character_to_upper(str.str[i]);
    }
    return(str);
}
function String_Const_u16
string_mod_upper(String_Const_u16 str){
    for (u64 i = 0; i < str.size; i += 1){
        str.str[i] = character_to_upper(str.str[i]);
    }
    return(str);
}
function String_Const_u32
string_mod_upper(String_Const_u32 str){
    for (u64 i = 0; i < str.size; i += 1){
        str.str[i] = character_to_upper(str.str[i]);
    }
    return(str);
}
function String_Const_char
string_mod_lower(String_Const_char str){
    for (u64 i = 0; i < str.size; i += 1){
        str.str[i] = character_to_lower(str.str[i]);
    }
    return(str);
}
function String_Const_u8
string_mod_lower(String_Const_u8 str){
    for (u64 i = 0; i < str.size; i += 1){
        str.str[i] = character_to_lower(str.str[i]);
    }
    return(str);
}
function String_Const_u16
string_mod_lower(String_Const_u16 str){
    for (u64 i = 0; i < str.size; i += 1){
        str.str[i] = character_to_lower(str.str[i]);
    }
    return(str);
}
function String_Const_u32
string_mod_lower(String_Const_u32 str){
    for (u64 i = 0; i < str.size; i += 1){
        str.str[i] = character_to_lower(str.str[i]);
    }
    return(str);
}

function String_Const_char
string_mod_replace_character(String_Const_char str, char o, char n){
    for (u64 i = 0; i < str.size; i += 1){
        char c = str.str[i];
        str.str[i] = (c == o)?(n):(c);
    }
    return(str);
}
function void
string_mod_replace_character(String_Const_u8 str, u8 o, u8 n){
    for (u64 i = 0; i < str.size; i += 1){
        u8 c = str.str[i];
        str.str[i] = (c == o)?(n):(c);
    }
}
function String_Const_u16
string_mod_replace_character(String_Const_u16 str, u16 o, u16 n){
    for (u64 i = 0; i < str.size; i += 1){
        u16 c = str.str[i];
        str.str[i] = (c == o)?(n):(c);
    }
    return(str);
}
function String_Const_u32
string_mod_replace_character(String_Const_u32 str, u32 o, u32 n){
    for (u64 i = 0; i < str.size; i += 1){
        u32 c = str.str[i];
        str.str[i] = (c == o)?(n):(c);
    }
    return(str);
}

function b32
string_append(String_char *dst, String_Const_char src){
    b32 result = false;
    u64 available = dst->cap - dst->size;
    if (src.size <= available){
        result = true;
    }
    u64 copy_size = clamp_top(src.size, available);
    block_copy(dst->str + dst->size, src.str, copy_size);
    dst->size += copy_size;
    return(result);
}
function b32
string_append(String_u8 *dst, String_Const_u8 src){
    b32 result = false;
    u64 available = dst->cap - dst->size;
    if (src.size <= available){
        result = true;
    }
    u64 copy_size = clamp_top(src.size, available);
    block_copy(dst->str + dst->size, src.str, copy_size);
    dst->size += copy_size;
    return(result);
}
function b32
string_append(String_u16 *dst, String_Const_u16 src){
    b32 result = false;
    u64 available = dst->cap - dst->size;
    if (src.size <= available){
        result = true;
    }
    u64 copy_size = clamp_top(src.size, available);
    block_copy(dst->str + dst->size, src.str, copy_size);
    dst->size += copy_size;
    return(result);
}
function b32
string_append(String_u32 *dst, String_Const_u32 src){
    b32 result = false;
    u64 available = dst->cap - dst->size;
    if (src.size <= available){
        result = true;
    }
    u64 copy_size = clamp_top(src.size, available);
    block_copy(dst->str + dst->size, src.str, copy_size);
    dst->size += copy_size;
    return(result);
}

function b32
string_append_character(String_char *dst, char c){
    return(string_append(dst, SCchar(&c, 1)));
}
function b32
string_append_character(String_u8 *dst, u8 c){
    return(string_append(dst, SCu8(&c, 1)));
}
function b32
string_append_character(String_u16 *dst, u16 c){
    return(string_append(dst, SCu16(&c, 1)));
}
function b32
string_append_character(String_u32 *dst, u32 c){
    return(string_append(dst, SCu32(&c, 1)));
}

function b32
string_null_terminate(String_char *str){
    b32 result = false;
    if (str->size < str->cap){
        str->str[str->size] = 0;
    }
    return(result);
}
function b32
string_null_terminate(String_u8 *str){
    b32 result = false;
    if (str->size < str->cap){
        str->str[str->size] = 0;
    }
    return(result);
}
function b32
string_null_terminate(String_u16 *str){
    b32 result = false;
    if (str->size < str->cap){
        str->str[str->size] = 0;
    }
    return(result);
}
function b32
string_null_terminate(String_u32 *str){
    b32 result = false;
    if (str->size < str->cap){
        str->str[str->size] = 0;
    }
    return(result);
}

function String_char
string_char_push(Arena *arena, u64 size){
    String_char string = {};
    string.str = push_array(arena, char, size);
    string.cap = size;
    return(string);
}
function String_u8
string_u8_push(Arena *arena, u64 size){
    String_u8 string = {};
    string.str = push_array(arena, u8, size);
    string.cap = size;
    return(string);
}
function String_u16
string_u16_push(Arena *arena, u64 size){
    String_u16 string = {};
    string.str = push_array(arena, u16, size);
    string.cap = size;
    return(string);
}
function String_u32
string_u32_push(Arena *arena, u64 size){
    String_u32 string = {};
    string.str = push_array(arena, u32, size);
    string.cap = size;
    return(string);
}

function String_Any
string_any_push(Arena *arena, u64 size, String_Encoding encoding){
    String_Any string = {};
    switch (encoding){
        case StringEncoding_ASCII: string.s_char = string_char_push(arena, size); break;
        case StringEncoding_UTF8:  string.s_u8   = string_u8_push  (arena, size); break;
        case StringEncoding_UTF16: string.s_u16  = string_u16_push (arena, size); break;
        case StringEncoding_UTF32: string.s_u32  = string_u32_push (arena, size); break;
    }
    return(string);
}

#define push_string_u8 string_u8_push
#define push_string_u16 string_u16_push
#define push_string_u32 string_u32_push
#define push_string_u64 string_u64_push

function String_Const_char
string_const_char_push(Arena *arena, u64 size){
    String_Const_char string = {};
    string.str = push_array(arena, char, size);
    string.size = size;
    return(string);
}
function String_Const_u8
string_const_u8_push(Arena *arena, u64 size){
    String_Const_u8 string = {};
    string.str = push_array(arena, u8, size);
    string.size = size;
    return(string);
}
function String_Const_u16
string_const_u16_push(Arena *arena, u64 size){
    String_Const_u16 string = {};
    string.str = push_array(arena, u16, size);
    string.size = size;
    return(string);
}
function String_Const_u32
string_const_u32_push(Arena *arena, u64 size){
    String_Const_u32 string = {};
    string.str = push_array(arena, u32, size);
    string.size = size;
    return(string);
}

function String_Const_Any
string_const_any_push(Arena *arena, u64 size, String_Encoding encoding){
    String_Const_Any string = {};
    switch (encoding){
        case StringEncoding_ASCII: string.s_char = string_const_char_push(arena, size); break;
        case StringEncoding_UTF8:  string.s_u8   = string_const_u8_push  (arena, size); break;
        case StringEncoding_UTF16: string.s_u16  = string_const_u16_push (arena, size); break;
        case StringEncoding_UTF32: string.s_u32  = string_const_u32_push (arena, size); break;
    }
    return(string);
}

#define push_string_const_u8 string_const_u8_push
#define push_string_const_u16 string_const_u16_push
#define push_string_const_u32 string_const_u32_push
#define push_string_const_u64 string_const_u64_push

function String_Const_char
push_string_copy(Arena *arena, String_Const_char src)
{
    String_Const_char string = {};
    string.str  = push_array(arena, char, src.size + 1);
    string.size = src.size;
    block_copy_dynamic_array(string.str, src.str, src.size);
    string.str[string.size] = 0;
    return(string);
}

function String8
push_string_copy(Arena *arena, String8 src)
{
    String_Const_u8 string = {};
    string.str = push_array(arena, u8, src.size + 1);
    string.size = src.size;
    block_copy_dynamic_array(string.str, src.str, src.size);
    string.str[string.size] = 0;
    return(string);
}

inline char *
to_c_string(Arena *arena, String8 string)
{
    String8 result = push_string_copy(arena, string);
    return (char *)result.str;
}

function String_Const_u16
push_string_copy(Arena *arena, String_Const_u16 src){
    String_Const_u16 string = {};
    string.str = push_array(arena, u16, src.size + 1);
    string.size = src.size;
    block_copy_dynamic_array(string.str, src.str, src.size);
    string.str[string.size] = 0;
    return(string);
}
function String_Const_u32
push_string_copy(Arena *arena, String_Const_u32 src){
    String_Const_u32 string = {};
    string.str = push_array(arena, u32, src.size + 1);
    string.size = src.size;
    block_copy_dynamic_array(string.str, src.str, src.size);
    string.str[string.size] = 0;
    return(string);
}

function String_Const_Any
push_string_copy(Arena *arena, u64 size, String_Const_Any src){
    String_Const_Any string = {};
    switch (src.encoding){
        case StringEncoding_ASCII: string.s_char = push_string_copy(arena, src.s_char); break;
        case StringEncoding_UTF8:  string.s_u8   = push_string_copy(arena, src.s_u8  ); break;
        case StringEncoding_UTF16: string.s_u16  = push_string_copy(arena, src.s_u16 ); break;
        case StringEncoding_UTF32: string.s_u32  = push_string_copy(arena, src.s_u32 ); break;
    }
    return(string);
}

function String_Const_u8_Array
push_string_array_copy(Arena *arena, String_Const_u8_Array src){
    String_Const_u8_Array result = {};
    result.vals = push_array(arena, String_Const_u8, src.count);
    result.count = src.count;
    for (i32 i = 0; i < src.count; i += 1){
        result.vals[i] = push_string_copy(arena, src.vals[i]);
    }
    return(result);
}

function void
string_list_push(List_String_Const_char *list, Node_String_Const_char *node){
    sll_queue_push(list->first, list->last, node);
    list->node_count += 1;
    list->total_size += node->string.size;
}
function void
string_list_push(List_String_Const_u8 *list, Node_String_Const_u8 *node){
    sll_queue_push(list->first, list->last, node);
    list->node_count += 1;
    list->total_size += node->string.size;
}
function void
string_list_push(List_String_Const_u16 *list, Node_String_Const_u16 *node){
    sll_queue_push(list->first, list->last, node);
    list->node_count += 1;
    list->total_size += node->string.size;
}
function void
string_list_push(List_String_Const_u32 *list, Node_String_Const_u32 *node){
    sll_queue_push(list->first, list->last, node);
    list->node_count += 1;
    list->total_size += node->string.size;
}

function void
string_list_push(Arena *arena, List_String_Const_char *list, String_Const_char string){
    Node_String_Const_char *node = push_array(arena, Node_String_Const_char, 1);
    sll_queue_push(list->first, list->last, node);
    node->string = string;
    list->node_count += 1;
    list->total_size += string.size;
}
function void
string_list_push(Arena *arena, List_String_Const_u8 *list, String_Const_u8 string){
    Node_String_Const_u8 *node = push_array(arena, Node_String_Const_u8, 1);
    sll_queue_push(list->first, list->last, node);
    node->string = string;
    list->node_count += 1;
    list->total_size += string.size;
}
function void
string_list_push(Arena *arena, List_String_Const_u16 *list, String_Const_u16 string){
    Node_String_Const_u16 *node = push_array(arena, Node_String_Const_u16, 1);
    sll_queue_push(list->first, list->last, node);
    node->string = string;
    list->node_count += 1;
    list->total_size += string.size;
}
function void
string_list_push(Arena *arena, List_String_Const_u32 *list, String_Const_u32 string){
    Node_String_Const_u32 *node = push_array(arena, Node_String_Const_u32, 1);
    sll_queue_push(list->first, list->last, node);
    node->string = string;
    list->node_count += 1;
    list->total_size += string.size;
}

function void
string_list_push(Arena *arena, List_String_Const_Any *list, String_Const_Any string){
    Node_String_Const_Any *node = push_array(arena, Node_String_Const_Any, 1);
    sll_queue_push(list->first, list->last, node);
    node->string = string;
    list->node_count += 1;
    list->total_size += string.size;
}

#define string_list_push_lit(a,l,s) string_list_push((a), (l), string_litexpr(s))
#define string_list_push_u8_lit(a,l,s) string_list_push((a), (l), string_u8_litexpr(s))

function void
string_list_push(List_String_Const_char *list, List_String_Const_char *src_list){
    sll_queue_push_multiple(list->first, list->last, src_list->first, src_list->last);
    list->node_count += src_list->node_count;
    list->total_size += src_list->total_size;
    block_zero_array(src_list);
}
function void
string_list_push(List_String_Const_u8 *list, List_String_Const_u8 *src_list){
    sll_queue_push_multiple(list->first, list->last, src_list->first, src_list->last);
    list->node_count += src_list->node_count;
    list->total_size += src_list->total_size;
    block_zero_array(src_list);
}
function void
string_list_push(List_String_Const_u16 *list, List_String_Const_u16 *src_list){
    sll_queue_push_multiple(list->first, list->last, src_list->first, src_list->last);
    list->node_count += src_list->node_count;
    list->total_size += src_list->total_size;
    block_zero_array(src_list);
}
function void
string_list_push(List_String_Const_u32 *list, List_String_Const_u32 *src_list){
    sll_queue_push_multiple(list->first, list->last, src_list->first, src_list->last);
    list->node_count += src_list->node_count;
    list->total_size += src_list->total_size;
    block_zero_array(src_list);
}

function void
string_list_push(List_String_Const_Any *list, List_String_Const_Any *src_list){
    sll_queue_push_multiple(list->first, list->last, src_list->first, src_list->last);
    list->node_count += src_list->node_count;
    list->total_size += src_list->total_size;
    block_zero_array(src_list);
}

function void
string_list_push_overlap(Arena *arena, List_String_Const_char *list, char overlap, String_Const_char string){
    b32 tail_has_overlap = false;
    b32 string_has_overlap = false;
    if (list->last != 0){
        String_Const_char tail = list->last->string;
        if (string_get_character(tail, tail.size - 1) == overlap){
            tail_has_overlap = true;
        }
    }
    if (string_get_character(string, 0) == overlap){
        string_has_overlap = true;
    }
    if (tail_has_overlap == string_has_overlap){
        if (!tail_has_overlap){
            string_list_push(arena, list, push_string_copy(arena, SCchar(&overlap, 1)));
        }
        else{
            string = string_skip(string, 1);
        }
    }
    if (string.size > 0){
        string_list_push(arena, list, string);
    }
}
function void
string_list_push_overlap(Arena *arena, List_String_Const_u8 *list, u8 overlap, String_Const_u8 string){
    b32 tail_has_overlap = false;
    b32 string_has_overlap = false;
    if (list->last != 0){
        String_Const_u8 tail = list->last->string;
        if (string_get_character(tail, tail.size - 1) == overlap){
            tail_has_overlap = true;
        }
    }
    if (string_get_character(string, 0) == overlap){
        string_has_overlap = true;
    }
    if (tail_has_overlap == string_has_overlap){
        if (!tail_has_overlap){
            string_list_push(arena, list, push_string_copy(arena, SCu8(&overlap, 1)));
        }
        else{
            string = string_skip(string, 1);
        }
    }
    if (string.size > 0){
        string_list_push(arena, list, string);
    }
}
function void
string_list_push_overlap(Arena *arena, List_String_Const_u16 *list, u16 overlap, String_Const_u16 string){
    b32 tail_has_overlap = false;
    b32 string_has_overlap = false;
    if (list->last != 0){
        String_Const_u16 tail = list->last->string;
        if (string_get_character(tail, tail.size - 1) == overlap){
            tail_has_overlap = true;
        }
    }
    if (string_get_character(string, 0) == overlap){
        string_has_overlap = true;
    }
    if (tail_has_overlap == string_has_overlap){
        if (!tail_has_overlap){
            string_list_push(arena, list, push_string_copy(arena, SCu16(&overlap, 1)));
        }
        else{
            string = string_skip(string, 1);
        }
    }
    if (string.size > 0){
        string_list_push(arena, list, string);
    }
}
function void
string_list_push_overlap(Arena *arena, List_String_Const_u32 *list, u32 overlap, String_Const_u32 string){
    b32 tail_has_overlap = false;
    b32 string_has_overlap = false;
    if (list->last != 0){
        String_Const_u32 tail = list->last->string;
        if (string_get_character(tail, tail.size - 1) == overlap){
            tail_has_overlap = true;
        }
    }
    if (string_get_character(string, 0) == overlap){
        string_has_overlap = true;
    }
    if (tail_has_overlap == string_has_overlap){
        if (!tail_has_overlap){
            string_list_push(arena, list, push_string_copy(arena, SCu32(&overlap, 1)));
        }
        else{
            string = string_skip(string, 1);
        }
    }
    if (string.size > 0){
        string_list_push(arena, list, string);
    }
}

#define push_string_list string_list_push
#define push_string_list_lit(a,l,s) string_list_push_lit(a,l,s)
#define push_string_list_u8_lit(a,l,s) string_list_u8_push_lit(a,l,s)
#define push_string_list_overlap(a,l,o,s) string_list_push_overlap(a,l,o,s)

typedef String_Const_char String_char_Mod_Function_Type(String_Const_char string);
typedef String_Const_u8 String_u8_Mod_Function_Type(String_Const_u8 string);
typedef String_Const_u16 String_u16_Mod_Function_Type(String_Const_u16 string);
typedef String_Const_u32 String_u32_Mod_Function_Type(String_Const_u32 string);

function String_Const_char
string_list_flatten(Arena *arena, List_String_Const_char list, String_char_Mod_Function_Type *mod, String_Const_char separator, String_Separator_Flag separator_flags, String_Fill_Terminate_Rule rule){
    u64 term_padding = (rule == StringFill_NullTerminate)?(1):(0);b32 before_first = HasFlag(separator_flags, StringSeparator_BeforeFirst);
    b32 after_last = HasFlag(separator_flags, StringSeparator_AfterLast);
    u64 separator_size = separator.size*(list.node_count + before_first + after_last - 1);
    String_char string = string_char_push(arena, list.total_size + separator_size + term_padding);
    if (before_first){
        string_append(&string, separator);
    }
    for (Node_String_Const_char *node = list.first;
         node != 0;
         node = node->next){
        block_copy_dynamic_array(string.str + string.size, node->string.str, node->string.size);
        if (mod != 0){
            mod(SCchar(string.str + string.size, node->string.size));
        }
        string.size += node->string.size;
        string_append(&string, separator);
    }
    if (after_last){
        string_append(&string, separator);
    }
    if (term_padding == 1){
        string_null_terminate(&string);
    }
    return(string.string);
}
function String_Const_u8
string_list_flatten(Arena *arena, List_String_Const_u8 list, String_u8_Mod_Function_Type *mod, String_Const_u8 separator, String_Separator_Flag separator_flags, String_Fill_Terminate_Rule rule){
    u64 term_padding = (rule == StringFill_NullTerminate)?(1):(0);b32 before_first = HasFlag(separator_flags, StringSeparator_BeforeFirst);
    b32 after_last = HasFlag(separator_flags, StringSeparator_AfterLast);
    u64 separator_size = separator.size*(list.node_count + before_first + after_last - 1);
    String_u8 string = string_u8_push(arena, list.total_size + separator_size + term_padding);
    if (before_first){
        string_append(&string, separator);
    }
    for (Node_String_Const_u8 *node = list.first;
         node != 0;
         node = node->next){
        block_copy_dynamic_array(string.str + string.size, node->string.str, node->string.size);
        if (mod != 0){
            mod(SCu8(string.str + string.size, node->string.size));
        }
        string.size += node->string.size;
        string_append(&string, separator);
    }
    if (after_last){
        string_append(&string, separator);
    }
    if (term_padding == 1){
        string_null_terminate(&string);
    }
    return(string.string);
}
function String_Const_u16
string_list_flatten(Arena *arena, List_String_Const_u16 list, String_u16_Mod_Function_Type *mod, String_Const_u16 separator, String_Separator_Flag separator_flags, String_Fill_Terminate_Rule rule){
    u64 term_padding = (rule == StringFill_NullTerminate)?(1):(0);b32 before_first = HasFlag(separator_flags, StringSeparator_BeforeFirst);
    b32 after_last = HasFlag(separator_flags, StringSeparator_AfterLast);
    u64 separator_size = separator.size*(list.node_count + before_first + after_last - 1);
    String_u16 string = string_u16_push(arena, list.total_size + separator_size + term_padding);
    if (before_first){
        string_append(&string, separator);
    }
    for (Node_String_Const_u16 *node = list.first;
         node != 0;
         node = node->next){
        block_copy_dynamic_array(string.str + string.size, node->string.str, node->string.size);
        if (mod != 0){
            mod(SCu16(string.str + string.size, node->string.size));
        }
        string.size += node->string.size;
        string_append(&string, separator);
    }
    if (after_last){
        string_append(&string, separator);
    }
    if (term_padding == 1){
        string_null_terminate(&string);
    }
    return(string.string);
}
function String_Const_u32
string_list_flatten(Arena *arena, List_String_Const_u32 list, String_u32_Mod_Function_Type *mod, String_Const_u32 separator, String_Separator_Flag separator_flags, String_Fill_Terminate_Rule rule){
    u64 term_padding = (rule == StringFill_NullTerminate)?(1):(0);b32 before_first = HasFlag(separator_flags, StringSeparator_BeforeFirst);
    b32 after_last = HasFlag(separator_flags, StringSeparator_AfterLast);
    u64 separator_size = separator.size*(list.node_count + before_first + after_last - 1);
    String_u32 string = string_u32_push(arena, list.total_size + separator_size + term_padding);
    if (before_first){
        string_append(&string, separator);
    }
    for (Node_String_Const_u32 *node = list.first;
         node != 0;
         node = node->next){
        block_copy_dynamic_array(string.str + string.size, node->string.str, node->string.size);
        if (mod != 0){
            mod(SCu32(string.str + string.size, node->string.size));
        }
        string.size += node->string.size;
        string_append(&string, separator);
    }
    if (after_last){
        string_append(&string, separator);
    }
    if (term_padding == 1){
        string_null_terminate(&string);
    }
    return(string.string);
}
function String_Const_char
string_list_flatten(Arena *arena, List_String_Const_char list, String_Const_char separator, String_Separator_Flag separator_flags, String_Fill_Terminate_Rule rule){
    return(string_list_flatten(arena, list, 0, separator, separator_flags, rule));
}
function String_Const_u8
string_list_flatten(Arena *arena, List_String_Const_u8 list, String_Const_u8 separator, String_Separator_Flag separator_flags, String_Fill_Terminate_Rule rule){
    return(string_list_flatten(arena, list, 0, separator, separator_flags, rule));
}
function String_Const_u16
string_list_flatten(Arena *arena, List_String_Const_u16 list, String_Const_u16 separator, String_Separator_Flag separator_flags, String_Fill_Terminate_Rule rule){
    return(string_list_flatten(arena, list, 0, separator, separator_flags, rule));
}
function String_Const_u32
string_list_flatten(Arena *arena, List_String_Const_u32 list, String_Const_u32 separator, String_Separator_Flag separator_flags, String_Fill_Terminate_Rule rule){
    return(string_list_flatten(arena, list, 0, separator, separator_flags, rule));
}
function String_Const_char
string_list_flatten(Arena *arena, List_String_Const_char list, String_char_Mod_Function_Type *mod, String_Fill_Terminate_Rule rule){
    return(string_list_flatten(arena, list, mod, SCchar(), 0, rule));
}
function String_Const_u8
string_list_flatten(Arena *arena, List_String_Const_u8 list, String_u8_Mod_Function_Type *mod, String_Fill_Terminate_Rule rule){
    return(string_list_flatten(arena, list, mod, SCu8(), 0, rule));
}
function String_Const_u16
string_list_flatten(Arena *arena, List_String_Const_u16 list, String_u16_Mod_Function_Type *mod, String_Fill_Terminate_Rule rule){
    return(string_list_flatten(arena, list, mod, SCu16(), 0, rule));
}
function String_Const_u32
string_list_flatten(Arena *arena, List_String_Const_u32 list, String_u32_Mod_Function_Type *mod, String_Fill_Terminate_Rule rule){
    return(string_list_flatten(arena, list, mod, SCu32(), 0, rule));
}
function String_Const_char
string_list_flatten(Arena *arena, List_String_Const_char string, String_Fill_Terminate_Rule rule){
    return(string_list_flatten(arena, string, 0, SCchar(), 0, rule));
}
function String_Const_u8
string_list_flatten(Arena *arena, List_String_Const_u8 string, String_Fill_Terminate_Rule rule){
    return(string_list_flatten(arena, string, 0, SCu8(), 0, rule));
}
function String_Const_u16
string_list_flatten(Arena *arena, List_String_Const_u16 string, String_Fill_Terminate_Rule rule){
    return(string_list_flatten(arena, string, 0, SCu16(), 0, rule));
}
function String_Const_u32
string_list_flatten(Arena *arena, List_String_Const_u32 string, String_Fill_Terminate_Rule rule){
    return(string_list_flatten(arena, string, 0, SCu32(), 0, rule));
}
function String_Const_char
string_list_flatten(Arena *arena, List_String_Const_char string){
    return(string_list_flatten(arena, string, 0, SCchar(), 0, StringFill_NoTerminate));
}
function String_Const_u8
string_list_flatten(Arena *arena, List_String_Const_u8 string){
    return(string_list_flatten(arena, string, 0, SCu8(), 0, StringFill_NoTerminate));
}
function String_Const_u16
string_list_flatten(Arena *arena, List_String_Const_u16 string){
    return(string_list_flatten(arena, string, 0, SCu16(), 0, StringFill_NoTerminate));
}
function String_Const_u32
string_list_flatten(Arena *arena, List_String_Const_u32 string){
    return(string_list_flatten(arena, string, 0, SCu32(), 0, StringFill_NoTerminate));
}

function List_String_Const_char
string_split(Arena *arena, String_Const_char string, char *split_characters, i32 split_character_count){
    List_String_Const_char list = {};
    for (;;){
        u64 i = string.size;
        String_Const_char prefix = string;
        for (i32 j = 0; j < split_character_count; j += 1){
            u64 pos = string_find_first(prefix, split_characters[j]);
            prefix = string_prefix(prefix, pos);
            i = Min(i, pos);
        }
        if (prefix.size > 0){
            string_list_push(arena, &list, prefix);
        }
        string = string_skip(string, i + 1);
        if (string.size == 0){
            break;
        }
    }
    return(list);
}
function List_String_Const_u8
string_split(Arena *arena, String_Const_u8 string, u8 *split_characters, i32 split_character_count){
    List_String_Const_u8 list = {};
    for (;;){
        u64 i = string.size;
        String_Const_u8 prefix = string;
        for (i32 j = 0; j < split_character_count; j += 1){
            u64 pos = string_find_first(prefix, split_characters[j]);
            prefix = string_prefix(prefix, pos);
            i = Min(i, pos);
        }
        if (prefix.size > 0){
            string_list_push(arena, &list, prefix);
        }
        string = string_skip(string, i + 1);
        if (string.size == 0){
            break;
        }
    }
    return(list);
}
function List_String_Const_u16
string_split(Arena *arena, String_Const_u16 string, u16 *split_characters, i32 split_character_count){
    List_String_Const_u16 list = {};
    for (;;){
        u64 i = string.size;
        String_Const_u16 prefix = string;
        for (i32 j = 0; j < split_character_count; j += 1){
            u64 pos = string_find_first(prefix, split_characters[j]);
            prefix = string_prefix(prefix, pos);
            i = Min(i, pos);
        }
        if (prefix.size > 0){
            string_list_push(arena, &list, prefix);
        }
        string = string_skip(string, i + 1);
        if (string.size == 0){
            break;
        }
    }
    return(list);
}
function List_String_Const_u32
string_split(Arena *arena, String_Const_u32 string, u32 *split_characters, i32 split_character_count){
    List_String_Const_u32 list = {};
    for (;;){
        u64 i = string.size;
        String_Const_u32 prefix = string;
        for (i32 j = 0; j < split_character_count; j += 1){
            u64 pos = string_find_first(prefix, split_characters[j]);
            prefix = string_prefix(prefix, pos);
            i = Min(i, pos);
        }
        if (prefix.size > 0){
            string_list_push(arena, &list, prefix);
        }
        string = string_skip(string, i + 1);
        if (string.size == 0){
            break;
        }
    }
    return(list);
}

function List_String_Const_char
string_split_needle(Arena *arena, String_Const_char string, String_Const_char needle){
    List_String_Const_char list = {};
    for (;string.size > 0;){
        u64 pos = string_find_first(string, needle);
        String_Const_char prefix = string_prefix(string, pos);
        if (pos < string.size){
            string_list_push(arena, &list, needle);
        }
        if (prefix.size > 0){
            string_list_push(arena, &list, prefix);
        }
        string = string_skip(string, prefix.size + needle.size);
    }
    return(list);
}
function List_String_Const_u8
string_split_needle(Arena *arena, String_Const_u8 string, String_Const_u8 needle){
    List_String_Const_u8 list = {};
    for (;string.size > 0;){
        u64 pos = string_find_first(string, needle);
        String_Const_u8 prefix = string_prefix(string, pos);
        if (pos < string.size){
            string_list_push(arena, &list, needle);
        }
        if (prefix.size > 0){
            string_list_push(arena, &list, prefix);
        }
        string = string_skip(string, prefix.size + needle.size);
    }
    return(list);
}
function List_String_Const_u16
string_split_needle(Arena *arena, String_Const_u16 string, String_Const_u16 needle){
    List_String_Const_u16 list = {};
    for (;string.size > 0;){
        u64 pos = string_find_first(string, needle);
        String_Const_u16 prefix = string_prefix(string, pos);
        if (pos < string.size){
            string_list_push(arena, &list, needle);
        }
        if (prefix.size > 0){
            string_list_push(arena, &list, prefix);
        }
        string = string_skip(string, prefix.size + needle.size);
    }
    return(list);
}
function List_String_Const_u32
string_split_needle(Arena *arena, String_Const_u32 string, String_Const_u32 needle){
    List_String_Const_u32 list = {};
    for (;string.size > 0;){
        u64 pos = string_find_first(string, needle);
        String_Const_u32 prefix = string_prefix(string, pos);
        if (pos < string.size){
            string_list_push(arena, &list, needle);
        }
        if (prefix.size > 0){
            string_list_push(arena, &list, prefix);
        }
        string = string_skip(string, prefix.size + needle.size);
    }
    return(list);
}

function void
string_list_insert_separators(Arena *arena, List_String_Const_char *list, String_Const_char separator, String_Separator_Flag flags){
    Node_String_Const_char *last = list->last;
    for (Node_String_Const_char *node = list->first, *next = 0;
         node != last;
         node = next){
        next = node->next;
        Node_String_Const_char *new_node = push_array(arena, Node_String_Const_char, 1);
        node->next = new_node;
        new_node->next = next;
        new_node->string = separator;
        list->node_count += 1;
        list->total_size += separator.size;
    }
    if (HasFlag(flags, StringSeparator_BeforeFirst)){
        Node_String_Const_char *new_node = push_array(arena, Node_String_Const_char, 1);
        new_node->next = list->first;
        list->first = new_node;
        new_node->string = separator;
        list->node_count += 1;
        list->total_size += separator.size;
    }
    if (HasFlag(flags, StringSeparator_AfterLast)){
        Node_String_Const_char *new_node = push_array(arena, Node_String_Const_char, 1);
        list->last->next = new_node;
        list->last = new_node;
        new_node->next = 0;
        new_node->string = separator;
        list->node_count += 1;
        list->total_size += separator.size;
    }
}
function void
string_list_insert_separators(Arena *arena, List_String_Const_u8 *list, String_Const_u8 separator, String_Separator_Flag flags){
    Node_String_Const_u8 *last = list->last;
    for (Node_String_Const_u8 *node = list->first, *next = 0;
         node != last;
         node = next){
        next = node->next;
        Node_String_Const_u8 *new_node = push_array(arena, Node_String_Const_u8, 1);
        node->next = new_node;
        new_node->next = next;
        new_node->string = separator;
        list->node_count += 1;
        list->total_size += separator.size;
    }
    if (HasFlag(flags, StringSeparator_BeforeFirst)){
        Node_String_Const_u8 *new_node = push_array(arena, Node_String_Const_u8, 1);
        new_node->next = list->first;
        list->first = new_node;
        new_node->string = separator;
        list->node_count += 1;
        list->total_size += separator.size;
    }
    if (HasFlag(flags, StringSeparator_AfterLast)){
        Node_String_Const_u8 *new_node = push_array(arena, Node_String_Const_u8, 1);
        list->last->next = new_node;
        list->last = new_node;
        new_node->next = 0;
        new_node->string = separator;
        list->node_count += 1;
        list->total_size += separator.size;
    }
}
function void
string_list_insert_separators(Arena *arena, List_String_Const_u16 *list, String_Const_u16 separator, String_Separator_Flag flags){
    Node_String_Const_u16 *last = list->last;
    for (Node_String_Const_u16 *node = list->first, *next = 0;
         node != last;
         node = next){
        next = node->next;
        Node_String_Const_u16 *new_node = push_array(arena, Node_String_Const_u16, 1);
        node->next = new_node;
        new_node->next = next;
        new_node->string = separator;
        list->node_count += 1;
        list->total_size += separator.size;
    }
    if (HasFlag(flags, StringSeparator_BeforeFirst)){
        Node_String_Const_u16 *new_node = push_array(arena, Node_String_Const_u16, 1);
        new_node->next = list->first;
        list->first = new_node;
        new_node->string = separator;
        list->node_count += 1;
        list->total_size += separator.size;
    }
    if (HasFlag(flags, StringSeparator_AfterLast)){
        Node_String_Const_u16 *new_node = push_array(arena, Node_String_Const_u16, 1);
        list->last->next = new_node;
        list->last = new_node;
        new_node->next = 0;
        new_node->string = separator;
        list->node_count += 1;
        list->total_size += separator.size;
    }
}
function void
string_list_insert_separators(Arena *arena, List_String_Const_u32 *list, String_Const_u32 separator, String_Separator_Flag flags){
    Node_String_Const_u32 *last = list->last;
    for (Node_String_Const_u32 *node = list->first, *next = 0;
         node != last;
         node = next){
        next = node->next;
        Node_String_Const_u32 *new_node = push_array(arena, Node_String_Const_u32, 1);
        node->next = new_node;
        new_node->next = next;
        new_node->string = separator;
        list->node_count += 1;
        list->total_size += separator.size;
    }
    if (HasFlag(flags, StringSeparator_BeforeFirst)){
        Node_String_Const_u32 *new_node = push_array(arena, Node_String_Const_u32, 1);
        new_node->next = list->first;
        list->first = new_node;
        new_node->string = separator;
        list->node_count += 1;
        list->total_size += separator.size;
    }
    if (HasFlag(flags, StringSeparator_AfterLast)){
        Node_String_Const_u32 *new_node = push_array(arena, Node_String_Const_u32, 1);
        list->last->next = new_node;
        list->last = new_node;
        new_node->next = 0;
        new_node->string = separator;
        list->node_count += 1;
        list->total_size += separator.size;
    }
}

function void
string_list_rewrite_nodes(Arena *arena, List_String_Const_char *list, String_Const_char needle, String_Const_char new_value){
    for (Node_String_Const_char *node = list->first;
         node != 0;
         node = node->next){
        if (string_match(node->string, needle)){
            node->string = new_value;
            list->total_size += new_value.size;
            list->total_size -= needle.size;
        }
    }
}
function void
string_list_rewrite_nodes(Arena *arena, List_String_Const_u8 *list, String_Const_u8 needle, String_Const_u8 new_value){
    for (Node_String_Const_u8 *node = list->first;
         node != 0;
         node = node->next){
        if (string_match(node->string, needle)){
            node->string = new_value;
            list->total_size += new_value.size;
            list->total_size -= needle.size;
        }
    }
}
function void
string_list_rewrite_nodes(Arena *arena, List_String_Const_u16 *list, String_Const_u16 needle, String_Const_u16 new_value){
    for (Node_String_Const_u16 *node = list->first;
         node != 0;
         node = node->next){
        if (string_match(node->string, needle)){
            node->string = new_value;
            list->total_size += new_value.size;
            list->total_size -= needle.size;
        }
    }
}
function void
string_list_rewrite_nodes(Arena *arena, List_String_Const_u32 *list, String_Const_u32 needle, String_Const_u32 new_value){
    for (Node_String_Const_u32 *node = list->first;
         node != 0;
         node = node->next){
        if (string_match(node->string, needle)){
            node->string = new_value;
            list->total_size += new_value.size;
            list->total_size -= needle.size;
        }
    }
}

function String_Const_char
string_condense_whitespace(Arena *arena, String_Const_char string){
    char split_characters[] = { ' ', '\t', '\n', '\r', '\f', '\v', };
    List_String_Const_char list = string_split(arena, string, split_characters, ArrayCount(split_characters));
    string_list_insert_separators(arena, &list, SCchar(split_characters, 1), StringSeparator_NoFlags);
    return(string_list_flatten(arena, list, StringFill_NullTerminate));
}
function String_Const_u8
string_condense_whitespace(Arena *arena, String_Const_u8 string){
    u8 split_characters[] = { ' ', '\t', '\n', '\r', '\f', '\v', };
    List_String_Const_u8 list = string_split(arena, string, split_characters, ArrayCount(split_characters));
    string_list_insert_separators(arena, &list, SCu8(split_characters, 1), StringSeparator_NoFlags);
    return(string_list_flatten(arena, list, StringFill_NullTerminate));
}
function String_Const_u16
string_condense_whitespace(Arena *arena, String_Const_u16 string){
    u16 split_characters[] = { ' ', '\t', '\n', '\r', '\f', '\v', };
    List_String_Const_u16 list = string_split(arena, string, split_characters, ArrayCount(split_characters));
    string_list_insert_separators(arena, &list, SCu16(split_characters, 1), StringSeparator_NoFlags);
    return(string_list_flatten(arena, list, StringFill_NullTerminate));
}
function String_Const_u32
string_condense_whitespace(Arena *arena, String_Const_u32 string){
    u32 split_characters[] = { ' ', '\t', '\n', '\r', '\f', '\v', };
    List_String_Const_u32 list = string_split(arena, string, split_characters, ArrayCount(split_characters));
    string_list_insert_separators(arena, &list, SCu32(split_characters, 1), StringSeparator_NoFlags);
    return(string_list_flatten(arena, list, StringFill_NullTerminate));
}

function List_String_Const_u8
string_split_wildcards(Arena *arena, String_Const_u8 string){
    List_String_Const_u8 list = {};
    if (string_get_character(string, 0) == '*'){
        string_list_push(arena, &list, SCu8());
    }
    {
        List_String_Const_u8 splits = string_split(arena, string, (u8*)"*", 1);
        string_list_push(&list, &splits);
    }
    if (string.size > 1 && string_get_character(string, string.size - 1) == '*'){
        string_list_push(arena, &list, SCu8());
    }
    return(list);
}

function b32
string_wildcard_match(List_String_Const_u8 list, String_Const_u8 string, String_Match_Rule rule){
    b32 success = true;
    if (list.node_count > 0){
        String_Const_u8 head = list.first->string;
        if (!string_match(head, string_prefix(string, head.size), rule)){
            success = false;
        }
        else if (list.node_count > 1){
            string = string_skip(string, head.size);
            String_Const_u8 tail = list.last->string;
            if (!string_match(tail, string_postfix(string, tail.size), rule)){
                success = false;
            }
            else if (list.node_count > 2){
                string = string_chop(string, tail.size);
                Node_String_Const_u8 *one_past_last = list.last;
                for (Node_String_Const_u8 *node = list.first->next;
                     node != one_past_last;
                     node = node->next){
                    u64 position = string_find_first(string, node->string, rule);
                    if (position < string.size){
                        string = string_skip(string, position + node->string.size);
                    }
                    else{
                        success = false;
                        break;
                    }
                }
            }
        }
    }
    return(success);
}

function b32
string_wildcard_match(List_String_Const_u8 list, String_Const_u8 string){
    return(string_wildcard_match(list, string, StringMatch_Exact));
}
function b32
string_wildcard_match_insensitive(List_String_Const_u8 list, String_Const_u8 string){
    return(string_wildcard_match(list, string, StringMatch_CaseInsensitive));
}

function void
string_list_reverse(List_String_Const_char *list){
    Node_String_Const_char *first = 0;
    Node_String_Const_char *last = list->first;
    for (Node_String_Const_char *node = list->first, *next = 0;
         node != 0;
         node = next){
        next = node->next;
        sll_stack_push(first, node);
    }
    list->first = first;
    list->last = last;
}
function void
string_list_reverse(List_String_Const_u8 *list){
    Node_String_Const_u8 *first = 0;
    Node_String_Const_u8 *last = list->first;
    for (Node_String_Const_u8 *node = list->first, *next = 0;
         node != 0;
         node = next){
        next = node->next;
        sll_stack_push(first, node);
    }
    list->first = first;
    list->last = last;
}
function void
string_list_reverse(List_String_Const_u16 *list){
    Node_String_Const_u16 *first = 0;
    Node_String_Const_u16 *last = list->first;
    for (Node_String_Const_u16 *node = list->first, *next = 0;
         node != 0;
         node = next){
        next = node->next;
        sll_stack_push(first, node);
    }
    list->first = first;
    list->last = last;
}
function void
string_list_reverse(List_String_Const_u32 *list){
    Node_String_Const_u32 *first = 0;
    Node_String_Const_u32 *last = list->first;
    for (Node_String_Const_u32 *node = list->first, *next = 0;
         node != 0;
         node = next){
        next = node->next;
        sll_stack_push(first, node);
    }
    list->first = first;
    list->last = last;
}

function b32
string_list_match(List_String_Const_u8 a, List_String_Const_u8 b){
    b32 result = true;
    for (Node_String_Const_u8 *a_node = a.first, *b_node = b.first;
         a_node != 0 && b_node != 0;
         a_node = a_node->next, b_node = b_node->next){
        if (!string_match(a_node->string, b_node->string)){
            result = false;
            break;
        }
    }
    return(result);
}

////////////////////////////////

global_const u8 utf8_class[32] = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,2,2,2,2,3,3,4,5,
};

function Character_Consume_Result
utf8_consume(u8 *str, u64 max){
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

function Character_Consume_Result
utf16_consume(u16 *str, u64 max){
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

////////////////////////////////

function String_u8
string_u8_from_string_char(Arena *arena, String_Const_char string, String_Fill_Terminate_Rule rule){
    String_u8 out = {};
    out.cap = string.size;
    if (rule == StringFill_NullTerminate){
        out.cap += 1;
    }
    out.str = push_array(arena, u8, out.cap);
    for (u64 i = 0; i < string.size; i += 1){
        out.str[i] = ((u8)string.str[i])&bitmask_7;
    }
    out.size = string.size;
    if (rule == StringFill_NullTerminate){
        string_null_terminate(&out);
    }
    return(out);
}

function String_u16
string_u16_from_string_char(Arena *arena, String_Const_char string, String_Fill_Terminate_Rule rule){
    String_u16 out = {};
    out.cap = string.size;
    if (rule == StringFill_NullTerminate){
        out.cap += 1;
    }
    out.str = push_array(arena, u16, out.cap);
    for (u64 i = 0; i < string.size; i += 1){
        out.str[i] = ((u16)string.str[i])&bitmask_7;
    }
    out.size = string.size;
    if (rule == StringFill_NullTerminate){
        string_null_terminate(&out);
    }
    return(out);
}

function String_u32
string_u32_from_string_char(Arena *arena, String_Const_char string, String_Fill_Terminate_Rule rule){
    String_u32 out = {};
    out.cap = string.size;
    if (rule == StringFill_NullTerminate){
        out.cap += 1;
    }
    out.str = push_array(arena, u32, string.size);
    for (u64 i = 0; i < string.size; i += 1){
        out.str[i] = ((u32)string.str[i])&bitmask_7;
    }
    out.size = string.size;
    if (rule == StringFill_NullTerminate){
        string_null_terminate(&out);
    }
    return(out);
}

function String_char
string_char_from_string_u8(Arena *arena, String_Const_u8 string, String_Fill_Terminate_Rule rule){
    String_char out = {};
    out.cap = string.size;
    if (rule == StringFill_NullTerminate){
        out.cap += 1;
    }
    out.str = push_array(arena, char, out.cap);
    u8 *ptr = string.str;
    u8 *one_past_last = ptr + string.size;
    u64 cap = string.size;
    Character_Consume_Result consume;
    for (;ptr < one_past_last; ptr += consume.inc, cap -= consume.inc){
        consume = utf8_consume(ptr, cap);
        out.str[out.size++] = (consume.codepoint <= 127)?((char)consume.codepoint):('?');
    }
    if (rule == StringFill_NullTerminate){
        string_null_terminate(&out);
    }
    return(out);
}

function String_u16
string_u16_from_string_u8(Arena *arena, String_Const_u8 string, String_Fill_Terminate_Rule rule){
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

function String_u32
string_u32_from_string_u8(Arena *arena, String_Const_u8 string, String_Fill_Terminate_Rule rule){
    String_u32 out = {};
    out.cap = string.size;
    if (rule == StringFill_NullTerminate){
        out.cap += 1;
    }
    out.str = push_array(arena, u32, out.cap);
    u8 *ptr = string.str;
    u8 *one_past_last = ptr + string.size;
    u64 cap = string.size;
    Character_Consume_Result consume;
    for (;ptr < one_past_last; ptr += consume.inc, cap -= consume.inc){
        consume = utf8_consume(ptr, cap);
        out.str[out.size++] = (consume.codepoint == max_u32)?(u64)'?':(consume.codepoint);
    }
    if (rule == StringFill_NullTerminate){
        string_null_terminate(&out);
    }
    return(out);
}

function String_char
string_char_from_string_u16(Arena *arena, String_Const_u16 string, String_Fill_Terminate_Rule rule){
    String_char out = {};
    out.cap = string.size;
    if (rule == StringFill_NullTerminate){
        out.cap += 1;
    }
    out.str = push_array(arena, char, out.cap);
    u16 *ptr = string.str;
    u16 *one_past_last = ptr + string.size;
    u64 cap = string.size;
    Character_Consume_Result consume;
    for (;ptr < one_past_last; ptr += consume.inc, cap -= consume.inc){
        consume = utf16_consume(ptr, cap);
        out.str[out.size++] = (consume.codepoint <= 127)?((char)consume.codepoint):('?');
    }
    if (rule == StringFill_NullTerminate){
        string_null_terminate(&out);
    }
    return(out);
}

function String_u8
string_u8_from_string_u16(Arena *arena, String_Const_u16 string, String_Fill_Terminate_Rule rule){
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

function String_u32
string_u32_from_string_u16(Arena *arena, String_Const_u16 string, String_Fill_Terminate_Rule rule){
    String_u32 out = {};
    out.cap = string.size;
    if (rule == StringFill_NullTerminate){
        out.cap += 1;
    }
    out.str = push_array(arena, u32, out.cap);
    u16 *ptr = string.str;
    u16 *one_past_last = ptr + string.size;
    u64 cap = string.size;
    Character_Consume_Result consume;
    for (;ptr < one_past_last; ptr += consume.inc, cap -= consume.inc){
        consume = utf16_consume(ptr, cap);
        out.str[out.size++] = consume.codepoint;
    }
    if (rule == StringFill_NullTerminate){
        string_null_terminate(&out);
    }
    return(out);
}

function String_char
string_char_from_string_u32(Arena *arena, String_Const_u32 string, String_Fill_Terminate_Rule rule){
    String_char out = {};
    out.cap = string.size;
    if (rule == StringFill_NullTerminate){
        out.cap += 1;
    }
    out.str = push_array(arena, char, string.size);
    u32 *ptr = string.str;
    u32 *one_past_last = ptr + string.size;
    for (;ptr < one_past_last; ptr += 1){
        u32 codepoint = *ptr;
        out.str[out.size++] = (codepoint <= 127)?((char)codepoint):('?');
    }
    if (rule == StringFill_NullTerminate){
        string_null_terminate(&out);
    }
    return(out);
}

function String_u8
string_u8_from_string_u32(Arena *arena, String_Const_u32 string, String_Fill_Terminate_Rule rule){
    String_u8 out = {};
    out.cap = string.size*4;
    if (rule == StringFill_NullTerminate){
        out.cap += 1;
    }
    out.str = push_array(arena, u8, out.cap);
    u32 *ptr = string.str;
    u32 *one_past_last = ptr + string.size;
    for (;ptr < one_past_last; ptr += 1){
        out.size += utf8_write(out.str + out.size, *ptr);
    }
    if (rule == StringFill_NullTerminate){
        string_null_terminate(&out);
    }
    return(out);
}

function String_u16
string_u16_from_string_u32(Arena *arena, String_Const_u32 string, String_Fill_Terminate_Rule rule){
    String_u16 out = {};
    out.cap = string.size*2;
    if (rule == StringFill_NullTerminate){
        out.cap += 1;
    }
    out.str = push_array(arena, u16, out.cap);
    u32 *ptr = string.str;
    u32 *one_past_last = ptr + string.size;
    for (;ptr < one_past_last; ptr += 1){
        out.size += utf16_write(out.str + out.size, *ptr);
    }
    if (rule == StringFill_NullTerminate){
        string_null_terminate(&out);
    }
    return(out);
}

////////////////////////////////

function String_char
string_char_from_string_u8(Arena *arena, String_Const_u8 string){
    return(string_char_from_string_u8(arena, string, StringFill_NullTerminate));
}
function String_char
string_char_from_string_u16(Arena *arena, String_Const_u16 string){
    return(string_char_from_string_u16(arena, string, StringFill_NullTerminate));
}
function String_char
string_char_from_string_u32(Arena *arena, String_Const_u32 string){
    return(string_char_from_string_u32(arena, string, StringFill_NullTerminate));
}
function String_u8
string_u8_from_string_char(Arena *arena, String_Const_char string){
    return(string_u8_from_string_char(arena, string, StringFill_NullTerminate));
}
function String_u8
string_u8_from_string_u16(Arena *arena, String_Const_u16 string){
    return(string_u8_from_string_u16(arena, string, StringFill_NullTerminate));
}
function String_u8
string_u8_from_string_u32(Arena *arena, String_Const_u32 string){
    return(string_u8_from_string_u32(arena, string, StringFill_NullTerminate));
}
function String_u16
string_u16_from_string_char(Arena *arena, String_Const_char string){
    return(string_u16_from_string_char(arena, string, StringFill_NullTerminate));
}
function String_u16
string_u16_from_string_u8(Arena *arena, String_Const_u8 string){
    return(string_u16_from_string_u8(arena, string, StringFill_NullTerminate));
}
function String_u16
string_u16_from_string_u32(Arena *arena, String_Const_u32 string){
    return(string_u16_from_string_u32(arena, string, StringFill_NullTerminate));
}
function String_u32
string_u32_from_string_char(Arena *arena, String_Const_char string){
    return(string_u32_from_string_char(arena, string, StringFill_NullTerminate));
}
function String_u32
string_u32_from_string_u8(Arena *arena, String_Const_u8 string){
    return(string_u32_from_string_u8(arena, string, StringFill_NullTerminate));
}
function String_u32
string_u32_from_string_u16(Arena *arena, String_Const_u16 string){
    return(string_u32_from_string_u16(arena, string, StringFill_NullTerminate));
}

////////////////////////////////

function String_Const_char
string_char_from_any(Arena *arena, String_Const_Any string){
    String_Const_char result = {};
    switch (string.encoding){
        case StringEncoding_ASCII: result = string.s_char; break;
        case StringEncoding_UTF8:  result = string_char_from_string_u8 (arena, string.s_u8 ).string; break;
        case StringEncoding_UTF16: result = string_char_from_string_u16(arena, string.s_u16).string; break;
        case StringEncoding_UTF32: result = string_char_from_string_u32(arena, string.s_u32).string; break;
    }
    return(result);
}
function String_Const_u8
string_u8_from_any(Arena *arena, String_Const_Any string){
    String_Const_u8 result = {};
    switch (string.encoding){
        case StringEncoding_ASCII: result = string_u8_from_string_char(arena, string.s_char).string; break;
        case StringEncoding_UTF8:  result = string.s_u8; break;
        case StringEncoding_UTF16: result = string_u8_from_string_u16(arena, string.s_u16).string; break;
        case StringEncoding_UTF32: result = string_u8_from_string_u32(arena, string.s_u32).string; break;
    }
    return(result);
}
function String_Const_u16
string_u16_from_any(Arena *arena, String_Const_Any string){
    String_Const_u16 result = {};
    switch (string.encoding){
        case StringEncoding_ASCII: result = string_u16_from_string_char(arena, string.s_char).string; break;
        case StringEncoding_UTF8:  result = string_u16_from_string_u8  (arena, string.s_u8  ).string; break;
        case StringEncoding_UTF16: result = string.s_u16; break;
        case StringEncoding_UTF32: result = string_u16_from_string_u32(arena, string.s_u32).string; break;
    }
    return(result);
}
function String_Const_u32
string_u32_from_any(Arena *arena, String_Const_Any string){
    String_Const_u32 result = {};
    switch (string.encoding){
        case StringEncoding_ASCII: result = string_u32_from_string_char(arena, string.s_char).string; break;
        case StringEncoding_UTF8:  result = string_u32_from_string_u8  (arena, string.s_u8  ).string; break;
        case StringEncoding_UTF16: result = string_u32_from_string_u16 (arena, string.s_u16 ).string; break;
        case StringEncoding_UTF32: result = string.s_u32; break;
    }
    return(result);
}

function String_Const_Any
string_any_from_any(Arena *arena, String_Encoding encoding, String_Const_Any string){
    String_Const_Any result = {.encoding=encoding};
    switch (encoding){
        case StringEncoding_ASCII: result.s_char = string_char_from_any(arena, string); break;
        case StringEncoding_UTF8:  result.s_u8   = string_u8_from_any  (arena, string); break;
        case StringEncoding_UTF16: result.s_u16  = string_u16_from_any (arena, string); break;
        case StringEncoding_UTF32: result.s_u32  = string_u32_from_any (arena, string); break;
    }
    return(result);
}

function List_String_Const_char
string_list_char_from_any(Arena *arena, List_String_Const_Any list){
    List_String_Const_char result = {};
    for (Node_String_Const_Any *node = list.first;
         node != 0;
         node = node->next){
        string_list_push(arena, &result, string_char_from_any(arena, node->string));
    }
    return(result);
}
function List_String_Const_u8
string_list_u8_from_any(Arena *arena, List_String_Const_Any list){
    List_String_Const_u8 result = {};
    for (Node_String_Const_Any *node = list.first;
         node != 0;
         node = node->next){
        string_list_push(arena, &result, string_u8_from_any(arena, node->string));
    }
    return(result);
}
function List_String_Const_u16
string_list_u16_from_any(Arena *arena, List_String_Const_Any list){
    List_String_Const_u16 result = {};
    for (Node_String_Const_Any *node = list.first;
         node != 0;
         node = node->next){
        string_list_push(arena, &result, string_u16_from_any(arena, node->string));
    }
    return(result);
}
function List_String_Const_u32
string_list_u32_from_any(Arena *arena, List_String_Const_Any list){
    List_String_Const_u32 result = {};
    for (Node_String_Const_Any *node = list.first;
         node != 0;
         node = node->next){
        string_list_push(arena, &result, string_u32_from_any(arena, node->string));
    }
    return(result);
}

////////////////////////////////

function Line_Ending_Kind
string_guess_line_ending_kind(String_Const_u8 string){
    b32 looks_like_binary = false;
    u64 crlf_count = 0;
    u64 lf_count = 0;
    for (u64 i = 0; i < string.size; i += 1){
        if (string.str[i] == 0){
            looks_like_binary = true;
            break;
        }
        if (string.str[i] == '\r' &&
            (i + 1 == string.size || string.str[i + 1] != '\n')){
            looks_like_binary = true;
            break;
        }
        if (string.str[i] == '\n'){
            if (i > 0 && string.str[i - 1] == '\r'){
                crlf_count += 1;
            }
            else{
                lf_count += 1;
            }
        }
    }
    Line_Ending_Kind kind = LineEndingKind_Binary;
    if (!looks_like_binary){
        if (crlf_count > lf_count){
            kind = LineEndingKind_CRLF;
        }
        else{
            kind = LineEndingKind_LF;
        }
    }
    return(kind);
}

////////////////////////////////

function List_String_Const_char
string_replace_list(Arena *arena, String_Const_char source, String_Const_char needle, String_Const_char replacement){
    List_String_Const_char list = {};
    for (;;){
        u64 i = string_find_first(source, needle);
        string_list_push(arena, &list, string_prefix(source, i));
        if (i < source.size){
            string_list_push(arena, &list, replacement);
            source = string_skip(source, i + needle.size);
        }
        else{
            break;
        }
    }
    return(list);
}
function List_String_Const_u8
string_replace_list(Arena *arena, String_Const_u8 source, String_Const_u8 needle, String_Const_u8 replacement){
    List_String_Const_u8 list = {};
    for (;;){
        u64 i = string_find_first(source, needle);
        string_list_push(arena, &list, string_prefix(source, i));
        if (i < source.size){
            string_list_push(arena, &list, replacement);
            source = string_skip(source, i + needle.size);
        }
        else{
            break;
        }
    }
    return(list);
}
function List_String_Const_u16
string_replace_list(Arena *arena, String_Const_u16 source, String_Const_u16 needle, String_Const_u16 replacement){
    List_String_Const_u16 list = {};
    for (;;){
        u64 i = string_find_first(source, needle);
        string_list_push(arena, &list, string_prefix(source, i));
        if (i < source.size){
            string_list_push(arena, &list, replacement);
            source = string_skip(source, i + needle.size);
        }
        else{
            break;
        }
    }
    return(list);
}
function List_String_Const_u32
string_replace_list(Arena *arena, String_Const_u32 source, String_Const_u32 needle, String_Const_u32 replacement){
    List_String_Const_u32 list = {};
    for (;;){
        u64 i = string_find_first(source, needle);
        string_list_push(arena, &list, string_prefix(source, i));
        if (i < source.size){
            string_list_push(arena, &list, replacement);
            source = string_skip(source, i + needle.size);
        }
        else{
            break;
        }
    }
    return(list);
}

function String_Const_char
string_replace(Arena *arena, String_Const_char source, String_Const_char needle, String_Const_char replacement, String_Fill_Terminate_Rule rule){
    List_String_Const_char list = string_replace_list(arena, source, needle, replacement);
    return(string_list_flatten(arena, list, rule));
}
function String_Const_u8
string_replace(Arena *arena, String_Const_u8 source, String_Const_u8 needle, String_Const_u8 replacement, String_Fill_Terminate_Rule rule){
    List_String_Const_u8 list = string_replace_list(arena, source, needle, replacement);
    return(string_list_flatten(arena, list, rule));
}
function String_Const_u16
string_replace(Arena *arena, String_Const_u16 source, String_Const_u16 needle, String_Const_u16 replacement, String_Fill_Terminate_Rule rule){
    List_String_Const_u16 list = string_replace_list(arena, source, needle, replacement);
    return(string_list_flatten(arena, list, rule));
}
function String_Const_u32
string_replace(Arena *arena, String_Const_u32 source, String_Const_u32 needle, String_Const_u32 replacement, String_Fill_Terminate_Rule rule){
    List_String_Const_u32 list = string_replace_list(arena, source, needle, replacement);
    return(string_list_flatten(arena, list, rule));
}

function String_Const_char
string_replace(Arena *arena, String_Const_char source, String_Const_char needle, String_Const_char replacement){
    return(string_replace(arena, source, needle, replacement, StringFill_NullTerminate));
}
function String_Const_u8
string_replace(Arena *arena, String_Const_u8 source, String_Const_u8 needle, String_Const_u8 replacement){
    return(string_replace(arena, source, needle, replacement, StringFill_NullTerminate));
}
function String_Const_u16
string_replace(Arena *arena, String_Const_u16 source, String_Const_u16 needle, String_Const_u16 replacement){
    return(string_replace(arena, source, needle, replacement, StringFill_NullTerminate));
}
function String_Const_u32
string_replace(Arena *arena, String_Const_u32 source, String_Const_u32 needle, String_Const_u32 replacement){
    return(string_replace(arena, source, needle, replacement, StringFill_NullTerminate));
}

////////////////////////////////

function b32
byte_is_ascii(u8 byte){
    return(byte == '\r' || byte == '\n' || byte == '\t' || (' ' <= byte && byte <= '~'));
}

function b32
data_is_ascii(String_Const_u8 data){
    u8 *ptr = data.str;
    u8 *one_past_last = ptr + data.size;
    b32 result = true;
    for (;ptr < one_past_last; ptr += 1){
        if (!byte_is_ascii(*ptr) && !(*ptr == 0 && ptr + 1 == one_past_last)){
            result = false;
            break;
        }
    }
    return(result);
}

////////////////////////////////

function String_Const_u8
string_escape(Arena *arena, String_Const_u8 string){
    List_String_Const_u8 list = string_replace_list(arena, string, string_u8_litexpr("\\"),
                                                    string_u8_litexpr("\\\\"));
    Node_String_Const_u8 **fixup_ptr = &list.first;
    for (Node_String_Const_u8 *node = list.first, *next = 0;
         node != 0;
         node = next){
        next = node->next;
        List_String_Const_u8 relist = string_replace_list(arena, node->string, string_u8_litexpr("\""),
                                                          string_u8_litexpr("\\\""));
        if (relist.first != 0){
            *fixup_ptr = relist.first;
            relist.last->next = next;
            fixup_ptr = &relist.last->next;
            list.last = relist.last;
        }
        else{
            *fixup_ptr = next;
        }
    }
    return(string_list_flatten(arena, list, StringFill_NullTerminate));
}

function String_Const_char
string_interpret_escapes(Arena *arena, String_Const_char string){
    char *space = push_array(arena, char, string.size + 1);
    String_char result = Schar(space, 0, string.size);
    for (;;){
        u64 back_slash_pos = string_find_first(string, '\\');
        string_append(&result, string_prefix(string, back_slash_pos));
        string = string_skip(string, back_slash_pos + 1);
        if (string.size == 0){
            break;
        }
        switch (string.str[0]){
            case '\\':
            {
                string_append_character(&result, '\\');
            }break;
            
            case 'n':
            {
                string_append_character(&result, '\n');
            }break;
            
            case 't':
            {
                string_append_character(&result, '\t');
            }break;
            
            case '"':
            {
                string_append_character(&result, '\"');
            }break;
            
            case '0':
            {
                string_append_character(&result, '\0');
            }break;
            
            default:
            {
                char c[2] = {'\\'};
                c[1] = string.str[0];
                string_append(&result, SCchar(c, 2));
            }break;
        }
        string = string_skip(string, 1);
    }
    result.str[result.size] = 0;
    pop_array(arena, char, result.cap - result.size);
    return(result.string);
}

function String_Const_u8
string_interpret_escapes(Arena *arena, String_Const_u8 string){
    return(SCu8(string_interpret_escapes(arena, SCchar(string))));
}

global_const u8 integer_symbols[] = {
    '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
};

global_const u8 integer_symbol_reverse[128] = {
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
};

global_const u8 base64[64] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    '_', '$',
};

global_const u8 base64_reverse[128] = {
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0x3F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,
    0xFF,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,0x30,0x31,0x32,
    0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0xFF,0xFF,0xFF,0xFF,0x3E,
    0xFF,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,
    0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x23,0xFF,0xFF,0xFF,0xFF,0xFF,
};

function u64
digit_count_from_integer(u64 x, u32 radix){
    u64 result = 0;
    if (radix >= 2 && radix <= 16){
        if (x == 0){
            result = 1;
        }
        else{
            do{
                x /= radix;
                result += 1;
            } while(x > 0);
        }
    }
    return(result);
}

function String_Const_u8
string_from_integer(Arena *arena, u64 x, u32 radix){
    String_Const_u8 result = {};
    if (radix >= 2 && radix <= 16){
        if (x == 0){
            result = push_string_copy(arena, string_u8_litexpr("0"));
        }
        else{
            u8 string_space[64];
            u64 length = 0;
            for (u64 X = x;
                 X > 0;
                 X /= radix, length += 1){
                string_space[length] = integer_symbols[X%radix];
            }
            for (u64 j = 0, i = length - 1;
                 j < i;
                 j += 1, i -= 1){
                Swap(u8, string_space[i], string_space[j]);
            }
            result = push_string_copy(arena, SCu8(string_space, length));
        }
    }
    return(result);
}

function b32
string_is_integer(String_Const_u8 string, u32 radix){
    b32 is_integer = false;
    if (string.size > 0 && radix <= 16){
        is_integer = true;
        for (u64 i = 0; i < string.size; i += 1){
            if (string.str[i] < 128){
                u8 x = integer_symbol_reverse[character_to_upper(string.str[i])];
                if (x >= radix){
                    is_integer = false;
                    break;
                }
            }
            else{
                is_integer = false;
                break;
            }
        }
    }
    return(is_integer);
}

function u64
string_to_integer(String8 string, u32 radix)
{
    u64 x = 0;
    if (radix <= 16)
    {
        for (u64 i = 0; i < string.size; i += 1)
        {
            x *= radix;
            if (string.str[i] < 128)
            {
                x += integer_symbol_reverse[character_to_upper(string.str[i])];
            }
            else
            {
                x += 0xFF;
            }
        }
    }
    return(x);
}

function u64
string_to_integer(String_Const_char string, u32 radix){
    return(string_to_integer(SCu8((u8*)string.str, string.size), radix));
}

#if 0  // @CompileError
function String_Const_u8
string_base64_encode_from_binary(Arena *arena, void *data, u64 size){
    u64 char_count = div_round_up_positive(size*8, 6);
    char_count = round_up_u64(char_count, 4);
    String_Const_u8 string = string_const_u8_push(arena, char_count);
    u8 *s = string.str;
    u8 *d = (u8*)data;
    u8 *de = d + size;
    for (;d < de; d += 3, s += 4){
        i32 in_byte_count = (i32)(de - d);
        u8 *D = d;
        b32 partial_fill = (in_byte_count < 3);
        u8 D_space[3] = {};
        if (partial_fill){
            block_copy(D_space, d, clamp_top(sizeof(D_space), in_byte_count));
            D = D_space;
        }
        s[0] =   D[0]      &bitmask_6;
        s[1] = ((D[0] >> 6)&bitmask_2) | ((D[1]&bitmask_4) << 2);
        s[2] = ((D[1] >> 4)&bitmask_4) | ((D[2]&bitmask_2) << 4);
        s[3] =  (D[2] >> 2)&bitmask_6;
        for (i32 j = 0; j < 4; j += 1){
            s[j] = base64[s[j]];
        }
        switch (in_byte_count){
            case 1:
            {
                s[2] = '?';
                s[3] = '?';
            }break;
            case 2:
            {
                s[3] = '?';
            }break;
        }
    }
    return(string);
}

function String_Const_u8
data_decode_from_base64(Arena *arena, u8 *str, u64 size){
    String_Const_u8 data = {};
    if (size%4 == 0){
        u64 data_size = size*6/8;
        if (str[size - 2] == '?'){
            data_size -= 2;
        }
        else if (str[size - 1] == '?'){
            data_size -= 1;
        }
        data = push_data(arena, data_size);
        u8 *s = str;
        u8 *se = s + size;
        u8 *d = data.str;
        u8 *de = d + data_size;
        for (;s < se; d += 3, s += 4){
            u8 *D = d;
            i32 out_byte_count = (i32)(de - d);
            b32 partial_fill = (out_byte_count < 3);
            u8 D_space[2];
            if (partial_fill){
                D = D_space;
            }
            u8 S[4];
            for (i32 j = 0; j < 4; j += 1){
                if (s[j] < 128){
                    S[j] = base64_reverse[s[j]];
                }
                else{
                    S[j] = 0xFF;
                }
            }
            D[0] = ( S[0]      &bitmask_6) | ((S[1]&bitmask_2) << 6);
            D[1] = ((S[1] >> 2)&bitmask_4) | ((S[2]&bitmask_4) << 4);
            D[2] = ((S[2] >> 4)&bitmask_2) | ((S[3]&bitmask_6) << 2);
            if (partial_fill){
                Assert(out_byte_count <= sizeof(D_space));
                block_copy(D, D_space, out_byte_count);
            }
        }
    }
    return(data);
}
#endif

////////////////////////////////

function u64
time_stamp_from_date_time(Date_Time *date_time){
    u64 result = 0;
    result += date_time->year;
    result *= 12;
    result += date_time->mon;
    result *= 30;
    result += date_time->day;
    result *= 24;
    result += date_time->hour;
    result *= 60;
    result += date_time->min;
    result *= 61;
    result += date_time->sec;
    result *= 1000;
    result += date_time->msec;
    return(result);
}

function Date_Time
date_time_from_time_stamp(u64 time_stamp){
    Date_Time result = {};
    result.msec = time_stamp%1000;
    time_stamp /= 1000;
    result.sec = time_stamp%61;
    time_stamp /= 61;
    result.min = time_stamp%60;
    time_stamp /= 60;
    result.hour = time_stamp%24;
    time_stamp /= 24;
    result.day = time_stamp%30;
    time_stamp /= 30;
    result.mon = time_stamp%12;
    time_stamp /= 12;
    result.year = (u32)time_stamp;
    return(result);
}

Scratch_Block::Scratch_Block(Thread_Context *t){
    this->tctx = t;
    this->arena = tctx_reserve(t);
    this->temp = begin_temp(this->arena);
}

Scratch_Block::Scratch_Block(Thread_Context *t, Arena *a1){
    this->tctx = t;
    this->arena = tctx_reserve(t, a1);
    this->temp = begin_temp(this->arena);
}

Scratch_Block::Scratch_Block(Thread_Context *t, Arena *a1, Arena *a2){
    this->tctx = t;
    this->arena = tctx_reserve(t, a1, a2);
    this->temp = begin_temp(this->arena);
}

Scratch_Block::Scratch_Block(Thread_Context *t, Arena *a1, Arena *a2, Arena *a3){
    this->tctx = t;
    this->arena = tctx_reserve(t, a1, a2, a3);
    this->temp = begin_temp(this->arena);
}

Scratch_Block::~Scratch_Block(){
    end_temp(this->temp);
    tctx_release(this->tctx, this->arena);
}

Scratch_Block::operator Arena*(){
    return(this->arena);
}

void
Scratch_Block::restore(void){
    end_temp(this->temp);
}

Temp_Memory_Block::Temp_Memory_Block(Temp_Memory t){
    this->temp = t;
}

Temp_Memory_Block::Temp_Memory_Block(Arena *arena){
    this->temp = begin_temp(arena);
}

Temp_Memory_Block::~Temp_Memory_Block(){
    end_temp(this->temp);
}

void
Temp_Memory_Block::restore(void){
    end_temp(this->temp);
}

///////////////////////////////////

internal Arena
make_static_arena(u8 *buffer, u64 size)
{
    Arena result = {};
    result.base_allocator = 0;
    result.cursor_node    = make_cursor_node_from_memory(buffer, size);
    result.chunk_size     = 0;
    result.alignment      = 8;
    return result;
}

inline b32
move_file(char const *existing_filename, char const *new_filename)
{
    return gb_file_move(existing_filename, new_filename);
}

inline b32
remove_file(Arena *scratch, String8 filename)
{
    return gb_file_remove(to_c_string(scratch, filename));
}

inline b32
move_file(String8 existing_filename, String8 new_filename)
{
    u8 buffer[512];
    Arena arena = make_static_arena(buffer, 512);
    return gb_file_move(to_c_string(&arena, existing_filename), 
                        to_c_string(&arena, new_filename));
}

function String_Const_u8
push_stringfv(Arena *arena, char *format, va_list args)
{
    va_list args2;
    va_copy(args2, args);
    i32 size = vsnprintf(0, 0, format, args);
    String_Const_u8 result = string_const_u8_push(arena, size + 1);
    vsnprintf((char*)result.str, (size_t)result.size, format, args2);
    result.size -= 1;
    result.str[result.size] = 0;
    return(result);
}
function String8
push_stringf(Arena *arena, char *format, ...)
{
    va_list args;
    va_start(args, format);
    String_Const_u8 result = push_stringfv(arena, format, args);
    va_end(args);
    return(result);
}

// @Cleanup move to helper file
internal String8
pjoin(Arena *arena, String8 a, String8 b)
{
    String8 result = push_stringf(arena, "%.*s/%.*s", string_expand(a), string_expand(b));
    return result;
}

internal String8
pjoin(Arena *arena, String8 a, const char *b)
{
    String8 result = push_stringf(arena, "%.*s/%s", string_expand(a), b);
    return result;
}

inline String8
to_string(Arena *arena, i32 value)
{
    return push_stringf(arena, "%d", value);
}

#endif  // KV_IMPLEMENTATION

//////////////////////////////////////////////////

inline v2 V2All(v1 value) { return v2{value,value}; }
inline v3 V3All(v1 value) { return v3{value,value,value}; }
inline v4 V4All(v1 value) { return v4{value,value,value,value}; }
inline v2 V2Zero() { return v2{}; }
inline v3 V3Zero() { return v3{}; }
inline v4 V4Zero() { return v4{}; }

inline v2 arm2(v1 turn)
{
    return v2{cos_turn(turn), sin_turn(turn)};
}

// TODO: nono Temporary put this here to transition to camera transform!
struct Camera
{
    v1 distance;  // NOTE: by its axis z
    union
    {
        m3x3 axes;    // transform to project the object onto the camera axes
        struct
        {
            v3 x;
            v3 y;
            v3 z;
        };
    };
    m3x3 project;
    v1   focal_length;
};

internal m4x4
operator*(m4x4 &A, m4x4 &B)
{
    m4x4 R = {};
    for_i32(r,0,4) // NOTE(casey): Rows (of A)
    {
        for_i32(c,0,4) // NOTE(casey): Column (of B)
        {
            for_i32(i,0,4) // NOTE(casey): Columns of A, rows of B!
            {
                R.v[c][r] += A.v[i][r]*B.v[c][i];
            }
        }
    }

    return(R);
}

internal v1
dot(v4 &a, v4 &b)
{
    return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
}

internal v4
operator*(m4x4 &A, v4 B)
{
    v4 result = (B.x*A.columns[0] +
                 B.y*A.columns[1] +
                 B.z*A.columns[2] +
                 B.w*A.columns[3]
                 );
    return result;
}

// IMPORTANT: NO TRESPASS ////////////////////////////////////

// NOTE(kv): do the people including this file a favor and not pollute.
#ifdef KV_IMPLEMENTATION
#    undef KV_IMPLEMENTATION
#endif
