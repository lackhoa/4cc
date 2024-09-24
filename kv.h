#pragma once
/*
  NOTE: This is the most basic file, containing things EVERYONE needs.
*/

#include <stdarg.h>
#include <stddef.h>
#include <float.h>
#include <stdlib.h> // malloc, free
#include <stdio.h>  // printf, perror
#include <cstdint>
#include <string.h>
#include <math.h>  // NOTE: This is for msvc

/* Compilers */

#if __llvm__
#    undef COMPILER_LLVM
#    define COMPILER_LLVM 1
#else
#    undef COMPILER_MSVC
#    define COMPILER_MSVC 1
#endif

//~  BEGIN: Other single-header libraries

// NOTE: Just compile the implementation, it's small enough!
#define STB_DEFINE
#define STB_DS_IMPLEMENTATION
#define GB_IMPLEMENTATION

#define DLL_EXPORT GB_DLL_EXPORT
#define DLL_IMPORT GB_DLL_IMPORT

#define EXTERN_C_BEGIN extern "C" {
#define EXTERN_C_END   }



// NOTE: These header files are supposed to be in the same directory as this file.
#if COMPILER_LLVM
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#elif COMPILER_MSVC
#endif

#include "stb_ds.h"
#define GB_STATIC
#include "gb.h"

#if COMPILER_LLVM
#pragma clang diagnostic pop
#elif COMPILER_MSVC
#endif

#undef STB_DEFINE
#undef STB_DS_IMPLEMENTATION
#undef GB_IMPLEMENTATION

//~END: Other single-header libraries

//~ Unorganized

#if COMPILER_LLVM
#    define PACK_BEGIN
#    define PACK_END    __attribute__((packed));  //NOTE: semicolon placement
#elif COMPILER_MSVC
#    define PACK_BEGIN  __pragma( pack(push, 1) )
#    define PACK_END    ; __pragma( pack(pop))
#endif

#define internal      static
#define function      static  // NOTE: I guess we're using this now...
#define xfunction     //NOTE(kv) exported function
#define local_persist static
#define global        static
// NOTE(kv): Global variable that is exported to the symbol table to be linked.
#define xglobal

//~ END Unorganized



/* Types */
typedef uint8_t   u8;
typedef uint16_t  u16;
typedef int32_t   i32;
typedef int64_t   i64;
typedef int8_t    b8;
typedef int32_t   b32;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef uintptr_t umm; // NOTE(kv): "umm" stands for "memory model"
typedef i64       imm;

typedef float r32;
typedef float f32;
typedef float v1;
/* Types: end */

#define for_i1(VAR, INITIAL, FINAL)  for(i32 VAR=INITIAL; VAR<FINAL; VAR++)
#define for_i32  for_i1
#define for_i32_test(VAR, INITIAL, FINAL, TEST)  for(i32 VAR=INITIAL; VAR<FINAL && TEST; VAR++)
#define for_u32(VAR, INITIAL, FINAL)  for(u32 VAR=INITIAL; VAR<FINAL; VAR++)
#define for_i64(VAR, INITIAL, FINAL)  for(i64 VAR=INITIAL; VAR<FINAL; VAR++)
#define for_u64(VAR, INITIAL, FINAL)  for(u64 VAR=INITIAL; VAR<FINAL; VAR++)
#define line_unique_var               PP_Concat(i, __LINE__)
#define for_repeat(TIMES)   for(i32 line_unique_var=0; line_unique_var<TIMES; line_unique_var++)

#ifdef KV_NO_FORCE_INLINE
# define force_inline inline
#else
# define force_inline gb_inline
#endif

/* Intrinsics */

force_inline void
block_zero(void *mem, u64 size)
{
#if AD_IS_DRIVER
 memset(mem, 0, size);
#else
 gb_zero_size(mem, size);
#endif
}

#if !AD_IS_DRIVER
force_inline void
block_fill_ones(void *mem, u64 size)
{
 gb_memset(mem, 0xff, size);
}
#endif

force_inline i32
absoslute(i32 in)
{
    return ((in >= 0) ? in : -in);
}

force_inline v1
squared(f32 x)
{
    f32 result = x*x;
    return result;
}

force_inline v1 
cubed(v1 value)
{
 return value*value*value;
}

force_inline v1
square_root(f32 x)
{
#if COMPILER_MSVC
    v1 result = sqrtf(x);
#else
    v1 result = __builtin_sqrtf(x);
#endif
    return result;
}

// TODO: These are real bad! Should only be one simd instruction. Watch hmh 379 for details.
force_inline v1
roundv1(v1 Real32)
{
#if COMPILER_MSVC
 v1 Result = roundf(Real32);
#else
 v1 Result = __builtin_roundf(Real32);
#endif
 return(Result);
}

force_inline v1
log_with_base(v1 base, v1 input)
{
 v1 result = logf(input) / logf(base);
 return result;
}

// NOTE: Integer power
function v1
integer_power(v1 base, i1 exponent)
{
 v1 result = 1.f;
 if (exponent < 0)
 {
  base = 1.f / base;
  exponent = -exponent;
 };
 for_i32 (_i, 0, exponent) {
  result *= base;
 }
 
 return result;
}

force_inline i32
round_to_integer(v1 value)
{
 return i32(value+0.5f);
}

// TODO @Cleanup force_inline all these functions
force_inline v1
floorv1(v1 value)
{
#if COMPILER_MSVC
    v1 Result = floorf(value);
#else
    v1 Result = __builtin_floorf(value);
#endif
    return(Result);
}

inline v1
ceilv1(v1 value)
{
#if COMPILER_MSVC
    v1 Result = ceilf(value);
#else
    v1 Result = __builtin_ceilf(value);
#endif
    return(Result);
}

force_inline v1
cycle01(v1 value)
{
 v1 result = value - floorv1(value);
 return result;
}

force_inline v1
cycle01_positive(v1 value)
{
 v1 result = value - v1(i32(value));
 return result;
}

// NOTE: weird names to avoid name collision (haizz)
inline v1
kv_sin(v1 angle)
{
#if COMPILER_MSVC
    v1 result = sinf(angle);
#else
    v1 result = __builtin_sinf(angle);
#endif
    return(result);
}

force_inline v1
kv_cos(v1 angle)
{
#if COMPILER_MSVC
    v1 result = cosf(angle);
#else
    v1 result = __builtin_cosf(angle);
#endif
    return(result);
}

force_inline v1
kv_atan2(v1 y, v1 x)
{
#if COMPILER_MSVC
    v1 result = atan2f(y, x);
#else
    v1 result = __builtin_atan2f(y, x);
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

force_inline v1
absolute(v1 x)
{
#if COMPILER_MSVC
 v1 result = (v1)fabs(x);
#else
 v1 result = (v1)__builtin_fabs(x);
#endif
 return result;
}
force_inline i32
absolute(i32 x)
{
 i32 result = (x >= 0) ? x : -x;
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

/* Intrinsics end */

// NOTE(kv): Don't enable this warning!
// #define UNUSED_VAR __attribute__((unused))
// #define unused_var __attribute__((unused))

typedef uintptr_t uptr;
typedef intptr_t  iptr;

#define kiloBytes(value) ((value)*1024LL)
#define megaBytes(value) (kiloBytes(value)*1024LL)
#define gigaBytes(value) (megaBytes(value)*1024LL)
#define teraBytes(value) (gigaBytes(value)*1024LL)
global v1 millimeter = 0.001f;
global v1 centimeter = 0.01f;

#if COMPILER_MSVC
#  define kv_fail __debugbreak()
#else
#  define kv_fail __builtin_trap()
#endif

#define kv_fail_ifnot(claim) do{if (!(claim)) { kv_fail; }} while(0)

#if KV_INTERNAL
#    define fail_in_debug  kv_fail
#else
#    define fail_in_debug
#endif






#define invalid_code_path   kv_fail

#define todo_test_me        fail_in_debug
#define todo_testme         fail_in_debug
#define todo_untested       fail_in_debug
#define kv_debug_trap       fail_in_debug
#define todo_incomplete     fail_in_debug
#define todo_implement      fail_in_debug
#define todo_error_report

#define invalid_default_case default: { invalid_code_path; };
#define breakhere       do{ int please_break = 5; (void)please_break; }while(0)

#if KV_INTERNAL
#    define kv_assert                    kv_fail_ifnot
#    define assert_defend(CLAIM, DEFEND) kv_fail_ifnot(CLAIM)
#else
#    define kv_assert(CLAIM)
#    define assert_defend(CLAIM, DEFEND)   if (!(CLAIM))  { DEFEND; }
#endif

// TODO: Wouldn't all asserts make you slow? I guess this is like "slower".
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

#define alen(array) (isize)(sizeof(array) / sizeof((array)[0]))

// source: https://groups.google.com/g/comp.std.c/c/d-6Mj5Lko_s
// NOTE: Doesn't work with MSVC, idk why man!
#define PP_ARG_N(_1,_2,_3,_4,_5,_6,_7,_8,N,...) N
#define PP_NARG(...) PP_ARG_N(__VA_ARGS__,8,7,6,5,4,3,2,1,0)

#define PP_Concat(arg1, arg2)   PP_Concat1(arg1, arg2)
#define PP_Concat1(arg1, arg2)  PP_Concat2(arg1, arg2)
#define PP_Concat2(arg1, arg2)  arg1##arg2

#if COMPILER_MSVC
#    define mytypeof decltype
#else
#    define mytypeof __typeof__
#endif

#define macro_min(a, b) ((a < b) ? a : b)
#define macro_max(a, b) ((a < b) ? b : a)
#define minimum  macro_min // @ Deprecated
#define maximum  macro_max // @ Deprecated

#define toggle_boolean(VAR)  VAR = !(VAR)

#define kv_function_typedef(N) typedef N##_return N##_type(N##_params)
#define kv_function_declare(N) N##_return N(N##_params)
#define kv_function_pointer(N) N##_type *N

/* MARK: End of String */

force_inline v1 min(v1 a, v1 b)       { return macro_min(a,b); }
force_inline v1 min(v1 a, v1 b, v1 c) { return macro_min(macro_min(a,b),c); }
force_inline v1 max(v1 a, v1 b)       { return macro_max(a,b); }
force_inline v1 max(v1 a, v1 b, v1 c) { return macro_max(macro_max(a,b),c); }

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

#define swap_minmax(a,b) if (a > b) { macro_swap(a,b); }

inline void *kv_xmalloc(size_t size) {
  void *ptr = malloc(size);
 if (!ptr) {
  perror("kv_xmalloc failed");
  exit(1);
 }
 return ptr;
}

#define breakable_block for (i32 __kv_breakable_block__=0; __kv_breakable_block__ == 0; __kv_breakable_block__++)
#define in_range_exclusive(bot,mid,top) ((bot) <= (mid) && (mid) < (top))
#define in_range_inclusive(bot,mid,top) ((bot) <= (mid) && (mid) <= (top))

/* ;math */

/* ;scalar */

#define PI32  3.14159265359f
#define TAU32 6.28318530717958647692f

#define macro_clamp(min,var,max)    if (var < min) { var = min; } else if (var > max) { var = max; }
#define macro_clamp_min(VAR, VAL)   if (VAR < VAL) VAR = VAL
#define macro_clamp_max(VAR, VAL)   if (VAR > VAL) VAR = VAL
#define macro_clamp01(var)          macro_clamp(0.f,var,1.f)
#define macro_clamp01i(var)         macro_clamp(0,var,1)

force_inline v1
bilateral(v1 r)
{
    return (r * 2.0f) - 1.0f;
}

force_inline v1
unilateral(v1 r)
{
    return (r * 0.5f) + 0.5f;
}

force_inline v1
lerp(v1 a, v1 t, v1 b)
{
    v1 result = a + t*(b - a);
    return result;
}

inline v1
unlerp_or_zero(v1 a, v1 v, v1 b)
{
  v1 range = (b - a);
  v1 result = (range != 0.0f) ? ((v - a) / range) : 0.0f;
  return result;
}

// NOTE: This can't be its own type because C++ doesn't allow us to convert float to v1, because they don't (know how to) programm, at all.
// They don't even think. Because what good is it to allow conversion from user-defined type to built-in type, but not the revers?

/* ;v2 */

union v2
{
 struct 
 {
  v1 x;
  v1 y;
 };
 v1 e[2];
 v1 v[2];
 
 v1 operator[](i32);
};

force_inline v1
v2::operator[](i32 index)
{
 return v[index];
}

inline v2 v2_all(v1 input)
{
 return v2{input, input};
}

inline bool
operator==(v2 u, v2 v)
{
    bool result;
    result = (u.x == v.x) && (u.y == v.y);
    return result;
}

force_inline v2
operator+(v2 u, v2 v)
{
    v2 result;
    result.x = u.x + v.x;
    result.y = u.y + v.y;
    return result;
}

force_inline v1
lerp(v2 ab, v1 t)
{
 return lerp(ab[0], t, ab[1]);
}


inline v2
operator-(v2 u, v2 v)
{
    v2 result;
    result.x = u.x - v.x;
    result.y = u.y - v.y;
    return result;
}

force_inline v2
operator-=(v2 &v, v2 u)
{
    v = v - u;
    return v;
}

force_inline v2
operator-(v2 v)
{
 v2 result;
 result.x = -v.x;
 result.y = -v.y;
 return result;
}

inline v2
operator*(v1 c, v2 v)
{
 v2 result;
 result.x = c * v.x;
 result.y = c * v.y;
 return result;
}

force_inline v2 operator*(v2 v, v1 c) { return c*v; }
inline v2 operator/(v2 v, v2 u) { return {v.x / u.x, v.y / u.y}; }
inline void operator*=(v2 &v, v1 c) { v = c*v; }
inline v2 operator/(v2 v, v1 c) { return v2{v.x / c, v.y / c}; }
inline v1 dot(v2 v, v2 u) { return v.x*u.x + v.y*u.y; }
inline v1 lensq(v2 v)    { return squared(v.x) + squared(v.y); }
inline v1 lengthof(v2 v) { return square_root(lensq(v)); }

inline v1
projectLen(v2 onto, v2 v)
{
 v1 innerProd = dot(onto, v);
 f32 result = (innerProd / lengthof(onto));
 return result;
}

inline v2
project_on(v2 onto, v2 v)
{
    f32 innerProd = dot(onto, v);
    v2 result = (innerProd / lensq(onto)) * onto;
    return result;
}

force_inline v2
hadamard(v2 v, v2 u)
{
    v2 result;
    result.x = v.x*u.x;
    result.y = v.y*u.y;
    return result;
}

function v2
noz(v2 v)  // normalize or zero
{
 v1 lsq = lensq(v);
 v2 result = {};
 if (lsq > 1e-8)
 {
  result = v * 1.f / square_root(lsq);
 }
 return result;
}

force_inline v2 perp(v2 v) { return v2{-v.y, v.x}; }

force_inline v2 bilateral(v2 v)  { return v2{bilateral(v.x), bilateral(v.y)}; }

// ;v3

union v3 
{
 struct { v1 x, y, z; };
 struct { v1 r, g, b; };
 struct { v2 xy; v1 xy_z; };
 struct { v1 x_yz; v2 yz; };
 v1 e[3];
 v1 v[3];
 
 force_inline v1 &operator[](i32 index) {return v[index];}
};

force_inline v3 
max(v3 u, v3 v) 
{
 return (v3
         {max(u.x,v.x),
          max(u.y,v.y),
          max(u.z,v.z)});
}

inline v3
absolute(v3 v)
{
 for_i32(index,0,3){ v[index] = absolute(v[index]); };
 return v;
}

force_inline v3 V3(v2 xy)       { return v3{.xy=xy}; }
force_inline v3 V3(v2 xy, v1 z) { return v3{.xy=xy, .xy_z=z}; }
force_inline v3 yzx(v3 v) { return v3{v.y, v.z, v.x}; }
force_inline v3 zxy(v3 v) { return v3{v.z, v.x, v.y}; }

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

inline bool
operator==(v3 u, v3 v)
{
    bool result;
    result = (u.x == v.x) && (u.y == v.y) && (u.z == v.z);
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

force_inline v3
operator-(v3 v)
{
    v3 result;
    result.x = -v.x;
    result.y = -v.y;
    result.z = -v.z;
    return result;
}

force_inline v3
operator*(v1 c, v3 v)
{
 v.x *= c;
 v.y *= c;
 v.z *= c;
 return v;
}

force_inline v3
operator*(v3 v, f32 c)
{
 v3 result = c*v;
 return result;
}

force_inline v3 &
operator*=(v3 &v, f32 c)
{
    v = c * v;
    return v;
}

force_inline v3
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


// todo: pick better name for this thing?
inline v1
cross2d(v2 u, v2 v)
{
    return u.x*v.y - u.y*v.x;
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
force_inline v3 
operator*(v3 u, v3 v)
{
 return hadamard(u,v);
}
force_inline v3 
operator/(v3 u, v3 v)
{
 return v3{u.x/v.x,
           u.y/v.y,
           u.z/v.z};
}

inline v1
lensq(v3 v)
{
    v1 result = squared(v.x) + squared(v.y) + squared(v.z);
    return result;
}

inline v1
lengthof(v3 v)
{
    v1 result = square_root(lensq(v));
    return result;
}

inline v3
project_on(v3 onto, v3 v)
{
    v1 innerProd = dot(onto, v);
    v3 result = (innerProd / lensq(onto)) * onto;
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
  struct { v1 x, y, z, w; };
  struct { v1 r, g, b, a; };
  struct { v1 h; v1 s; v1 l; v1 __a; };
  struct { v3 rgb; f32 a_ignored; };
  struct { v3 xyz; v1 xyz_w; };
  struct { v1 x_yzw; v3 yzw; };
  struct { v2 xy; v2 zw; };
  v1 e[4];
  v1 v[4];
 
 v1 &operator[](i32);
};


force_inline v3
V3(v1 x, v1 y, v1 z)
{
 return v3{x, y, z};
}
force_inline v4
V4(v1 x, v1 y, v1 z, v1 w)
{
 return v4{x, y, z, w};
}
force_inline v4
vert4(v1 x, v1 y, v1 z)
{
 return v4{x, y, z, 1.f};
}
force_inline v4
V4_symmetric(v1 x, v1 y)
{
 return v4{x,y,y,x};
}
force_inline v4
V4_symmetric(v2 xy)
{
 return V4_symmetric(xy.x,xy.y);
}
force_inline v4
V4(v3 xyz, v1 w)
{
 v4 v;
 v.xyz = xyz;
 v.w   = w;
 return v;
}
force_inline v4
cast_V4(v3 xyz)
{
 v4 v = {};
 v.xyz = xyz;
 return v;
}

force_inline v1 &
v4::operator[](i32 index)
{
 return v[index];
}

force_inline v3
operator /(v1 n, v3 d)
{
 return V3(n/d.x, n/d.y, n/d.z);
}

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
operator-(v4 v)
{
 v4 result = {-v.x, -v.y, -v.z, -v.w};
 return result;
}

inline v4
operator-(v4 u, v4 v)
{
 v4 result = {u.x - v.x, u.y - v.y, u.z - v.z, u.w - v.w};
 return result;
}

force_inline b32 
almost_equal(v1 a, v1 b, v1 epsilon=1e-6)
{
 return absolute(a - b) < epsilon;
}

force_inline b32 
almost_equal(v3 a, v3 b, v1 epsilon=1e-6)
{
 v3 d = a - b;
 for_i32(i,0,3) {
  if ( !almost_equal(a[i],b[i],epsilon) ) {
   return false;
  }
 }
 return true;
}


inline v4
lerp(v4 a, f32 t, v4 b)
{
    v4 result;
    result = a + t*(b - a);
    return result;
}

force_inline void
operator+=(v3 &v, v3 u)
{
    v = u + v;
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

inline v3
noz(v3 v)  // normalize or zero
{
 v1 lsq = lensq(v);
 v3 result = {};
 if (lsq > 1e-8) 
 {
  // prevent the result from getting too big
  result = v * 1.f / square_root(lsq);
 }
 return result;
}

inline v1 
lensq(v4 v)
{
 return (v.x*v.x +
         v.y*v.y +
         v.z*v.z +
         v.w*v.w);
}

inline v4
noz(v4 v)
{
 v1 lsq = lensq(v);
 v4 result = {};
 if (lsq > squared(0.0001f)) 
 {
  // prevent the result from getting too big
  result = (1.f / square_root(lsq))*v;
 }
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
 v4 v4_value;
};

inline b32
contains(rect2 rect, v2 point)
{
 return ((point.x >= rect.x0) && (point.y >= rect.y0) &&
         (point.x <  rect.x1) && (point.y <  rect.y1));
}

inline rect2 rect2_radius(v2 radius) { return {-radius, radius}; }

inline v2 get_dim(rect2 rect) { return (rect.max - rect.min); }
inline v2 get_radius(rect2 rect) { return 0.5f*(rect.max - rect.min); }

inline rect2
rect2_center_radius(v2 center, v2 radius)
{
 macro_clamp_min(radius.x,0);
 macro_clamp_min(radius.y,0);
 rect2 result;
 result.min = center - radius;
 result.max = center + radius;
 return result;
}

inline rect2
rect2_center_dim(v2 center, v2 dim) {
 rect2 result = rect2_center_radius(center, 0.5f*dim);
 return result;
}

inline rect2
rect2_min_dim(v2 min, v2 dim)
{
  rect2 out = rect2{min, min+dim};
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
    result.min.x = macro_max(a.min.x, b.min.x);
    result.min.y = macro_max(a.min.y, b.min.y);
    result.max.x = minimum(a.max.x, b.max.x);
    result.max.y = minimum(a.max.y, b.max.y);
 return result;
}

//
// ;rect3
//

struct rect3 {
 union{
  struct {v1 x0,y0,z0;};
  v3 min;
 };
 union{
  struct {v1 x1,y1,z1;};
  v3 max;
 };
};

function rect3
rect3_center_radius(v3 center, v3 radius) {
 radius = absolute(radius);
 return rect3{
  .min=center-radius,
  .max=center+radius,
 };
}

inline b32
contains(rect3 rect, v3 p)
{
 b32 result = ((p.x >= rect.min.x)
               && (p.y >= rect.min.y)
               && (p.z >= rect.min.z)
               && (p.x < rect.max.x)
               && (p.y < rect.max.y)
               && (p.z < rect.max.z));
 return result;
}

inline v3
get_radius(rect3 rect) {
 v3 result = 0.5f * (rect.max - rect.min);
 return result;
}

inline b32
overlap(rect3 a, rect3 b)
{
 b32 separate = ((a.max.x <= b.min.x) || (a.min.x >= b.max.x)
                 || (a.max.y <= b.min.y) || (a.min.y >= b.max.y)
                 || (a.max.z <= b.min.z) || (a.min.z >= b.max.z));
 return !separate;
}

inline v3
getBarycentricCoordinate(rect3 rect, v3 pos)
{
 v3 result;
 v3 dim = rect.max - rect.min;
 result.x = ((pos.x - rect.min.x) / dim.x);
 result.y = ((pos.y - rect.min.y) / dim.y);
 result.z = ((pos.z - rect.min.z) / dim.z);
 return result;
}

typedef i32 i1;

// ;i2 ;i3

union i2
{
  struct{
    i32 x;
    i32 y;
  };
  i32 e[2];
 
 i32 operator[](i32);
};
force_inline i32
i2::operator[](i32 index)
{
 return e[index];
}

force_inline v2
V2(i2 v)
{
 return {(f32)v.x, (f32)v.y};
}

union i3{
 struct{ i32 x,y,z; };
 struct{ i32 r,g,b; };
 struct{ i2 xy; };
 i32 e[3];
 
 i32 operator[](i32);
};
force_inline i32
i3::operator[](i32 index)
{
 return e[index];
}

force_inline i3
operator-(i3 v)
{
 v.x = -v.x;
 v.y = -v.y;
 v.z = -v.z;
 return v;
}

union i4{
 struct{ i32 x,y,z,w; };
 struct{ i32 r,g,b,a; };
 i32 e[4];
 
 i32 &operator[](i32);
};
force_inline i32&
i4::operator[](i32 index)
{
 return e[index];
}

/* todo: Old names */
#define kvXmalloc    kv_xmalloc
#define kvAssert     kv_assert
/* Old names > */

// TODO: Deprecate these
#define v2_expand(v) v.x, v.y
#define v3_expand(v) v.x, v.y, v.z
#define v4_expand(v) v.x, v.y, v.z, v.w

#define array_expand(v) v, alen(v)
#define expand2(v)   v[0], v[1]
#define expand3(v)   v[0], v[1], v[2]
#define expand4(v)   v[0], v[1], v[2], v[3]
//
#define repeat2(v)   v,v
#define repeat3(v)   v,v,v
#define repeat4(v)   v,v,v,v

// X macros //////////////////////////////////
#define XTypedef(N,R,P)              typedef R N##_type P;
#define XPointer(N,R,P)              N##_type *N;
#define XGlobalPointer(N,R,P)        global N##_type *N;
#define XInternalFunction(N,R,P)     function N##_type N;
#define X_Field_Type_Name(type,name) type name;

// Bitmap //////////////////////////////////////

struct Bitmap 
{
  u8 *data;
  i2  dim;
  i32 pitch;
};

function v4
linearToSrgb(v4 linear)
{
    v4 result;
    result.r = square_root(linear.r);
    result.g = square_root(linear.g);
    result.b = square_root(linear.b);
    result.a = linear.a;
    return result;
}

function u32
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

union mat3
{
 v3 rows[3];
 v1 e[3][3];
};

union mat4
{
 v4 rows[4];
 v1 e[4][4];
 v1* operator[](i32 i);
};
///
struct mat4i
{
 union { mat4 forward; mat4 m; };
 union { mat4 inverse; mat4 inv; };
 force_inline operator mat4&() { return forward; }  // @ClangSafe
};

force_inline v1
get_xscale(mat4 const&mat)
{
 return lengthof(mat.rows[0].xyz);
}

global mat3 mat3_identity = {{
  1,0,0,
  0,1,0,
  0,0,1,
 }};

global mat4 mat4_identity = {{
  1,0,0,0,
  0,1,0,0,
  0,0,1,0,
  0,0,0,1,
 }};

force_inline b32 
almost_equal(mat4 const&a, mat4 const&b)
{
 for_i32(i,0,4)
 {
  for_i32(j,0,4)
  {
   if ( !almost_equal(a.e[i][j], b.e[i][j]) ) { return false; }
  }
 }
 return true;
}

force_inline v1 *
mat4::operator[](i32 i)
{
 return e[i];
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
# endif

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

#if 0  // NOTE: not sure what this is for?
#if COMPILER_CL
#define __VA_OPT__(x)
#endif
#endif

#define function static
#define api(x)

#define local_const  static const
#define global_const static const  // NOTE(kv): Pretty useful you wanna eliminate globals, these aren't problematic

#define ArrayCount(a) i32((sizeof(a))/(sizeof(*a)))

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
//#define OffsetOfMember(S,m) PtrAsInt(&Member(S,m))
//#define OffsetOfMemberStruct(s,m) PtrDif(&(s)->m, (s))
//#define SizeAfterMember(S,m) (sizeof(S) - offsetof(S,m))
#define CastFromMember(S,m,ptr) (S*)( (u8*)(ptr) - offsetof(S,m) )

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
#  define Assert(c) AssertAlways(c)
#  define AssertMessage(m) AssertMessageAlways(m)
#  define StaticAssertDisambiguate(c,d) StaticAssertDisambiguateAlways(c,d)
#  define StaticAssert(c) StaticAssertAlways(c)
#else
#  define Assert(c)
#  define AssertMessage(m)
#  define StaticAssertDisambiguate(c,d)
#  define StaticAssert(c)
#endif

#define AssertImplies(a,b) Assert(!(a) || (b))
#define InvalidPath AssertMessage("invalid path")
#define NotImplemented AssertMessage("not implemented")
#define DontCompile NoSeriouslyDontCompile

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

#define div_round_up_positive_(n,d) (n + d - 1)/d
#define div_round_up_positive(n,d) (div_round_up_positive_((n),(d)))

#define DrCase(PC) case PC: goto resumespot_##PC
#define DrYield(PC, n) { *S_ptr = S; S_ptr->__pc__ = PC; return(n); resumespot_##PC:; }
#define DrReturn(n) { *S_ptr = S; S_ptr->__pc__ = -1; return(n); }

#define Max(a,b) (((a)>(b))?(a):(b))
#define Min(a,b) (((a)<(b))?(a):(b))
#define clamp_max(a,b) Min(a,b)
#define clamp_min(a,b) Max(a,b)
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
#define clamp_unsigned_to_i8(x) (i8)(clamp_max((u64)(x), (u64)i8_max))
#define clamp_unsigned_to_i16(x) (i16)(clamp_max((u64)(x), (u64)i16_max))
#define clamp_unsigned_to_i32(x) (i32)(clamp_max((u64)(x), (u64)i32_max))
#define clamp_unsigned_to_i64(x) (i64)(clamp_max((u64)(x), (u64)i64_max))
#define clamp_signed_to_u8(x) (u8)(clamp_max((u64)clamp_min(0, (i64)(x)), (u64)u8_max))
#define clamp_signed_to_u16(x) (u16)(clamp_max((u64)clamp_min(0, (i64)(x)), (u64)u16_max))
#define clamp_signed_to_u32(x) (u32)(clamp_max((u64)clamp_min(0, (i64)(x)), (u64)u32_max))
#define clamp_signed_to_u64(x) (u64)(clamp_max((u64)clamp_min(0, (i64)(x)), (u64)u64_max))
#define clamp_unsigned_to_u8(x) (u8)(clamp_max((u64)(x), (u64)u8_max))
#define clamp_unsigned_to_u16(x) (u16)(clamp_max((u64)(x), (u64)u16_max))
#define clamp_unsigned_to_u32(x) (u32)(clamp_max((u64)(x), (u64)u32_max))
#define clamp_unsigned_to_u64(x) (u64)(clamp_max((u64)(x), (u64)u64_max))

#define line_number_as_string stringify(__LINE__)
#define filename_line_number __FILE__ ":" line_number_as_string ":"

#define macro_require(c) Stmnt( if (!(c)){ return(0); } )

////////////////////////////////

#if 1
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
#endif

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

#define sll_stack_push(head,new_item) (sll_stack_push_((head),(new_item)))
#define sll_stack_pop(h) (sll_stack_pop_((h)))
#define sll_queue_push_multiple(f,l,ff,ll) Stmnt( sll_queue_push_multiple_((f),(l),(ff),(ll)) )
// NOTE(kv): pretty sure "queue_push" means "push_last"
#define sll_queue_push(first,last,new_item) \
Stmnt( sll_queue_push_((first),(last),(new_item)) )
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

typedef i2 Vec2_i32;
typedef i3 Vec3_i32;
typedef v3 Vec3_f32;

union Range_i32 {
 struct{ i32 min; i32 max; };
 struct{ i32 start; i32 end; };
 struct{ i32 first; i32 opl; };
};
union Range_i64 {
 struct{ i64 min; i64 max; };
 struct{ i64 start; i64 end; };
 struct{ i64 first; i64 opl; };
};
union Range_u64 {
 struct{ u64 min; u64 max; };
 struct{ u64 start; u64 end; };
 struct{ u64 first; u64 opl; };
};
union Range_f32 {
 struct{ f32 min; f32 max; };
 struct{ f32 start; f32 end; };
 struct{ f32 first; f32 opl; };
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

union rect2_Pair{
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
typedef u32 argb;

////////////////////////////////

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

struct String_Array{
 union{
  String *strings;
  String *vals;
 };
 i32 count;
};

api(custom)
typedef u32 Access_Flag;
enum{
 Access_Write = 0x1,
 Access_Read = 0x2,
 Access_Visible = 0x4,
};

////////////////////////////////

enum Base_Allocator_Type
{
 Allocator_None,
 Allocator_Generic,
 Allocator_Malloc,
 Allocator_Arena,
};

typedef void *Allocator_Reserve_Signature(void *user_data, u64 size, u64 *size_out, String location);
typedef void  Allocator_Commit_Signature(void *user_data, void *ptr, u64 size);
typedef void  Allocator_Uncommit_Signature(void *user_data, void *ptr, u64 size);
typedef void  Allocator_Free_Signature(void *user_data, void *ptr);
typedef void  Allocator_Set_Access_Signature(void *user_data, void *ptr, u64 size, Access_Flag flags);
// NOTE: Allocator function pointers (Can't used in reloadable code :<)
struct Base_Allocator_Generic
{
 Allocator_Reserve_Signature    *reserve;
 Allocator_Commit_Signature     *commit;
 Allocator_Uncommit_Signature   *uncommit;
 Allocator_Free_Signature       *free;
 Allocator_Set_Access_Signature *set_access;
 void *userdata;
};

struct Base_Allocator
{
 Base_Allocator_Type type;
 union
 {
  struct Arena *arena;
  // or
  Base_Allocator_Generic generic;
  // or
  // malloc-based allocators don't need anything
 };
};

// NOTE(kv): Cursors are static arenas
// TODO(kv): We really should just bake them into arenas
struct Memory_Cursor
{
 Memory_Cursor *next;
 u8 *base;  // TODO(kv): Maybe we can use flexible array?
 u64 used_;
 u64 cap;
};
struct Arena
{
 Base_Allocator *base_allocator;
 Memory_Cursor  *cursor;
 u64 chunk_size;
};
struct Temp_Memory
{
 Arena *arena;
 Memory_Cursor *cursor;
 u64 used;
};

////////////////////////////////

typedef u64 Profile_ID;
struct Profile_Record{
  Profile_Record *next;
  Profile_ID id;
  u64 time;
  String location;
  String name;
};

struct Profile_Thread{
  Profile_Thread *next;
  Profile_Record *first_record;
  Profile_Record *last_record;
  i32 record_count;
  i32 thread_id;
  String name;
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

struct Thread_Context {
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

// NOTE: these are inline functions anyway?

#define BODY { x += b-1; x -= x%b; return(x); }
inline u32 round_up_u32(u32 x, u32 b) {BODY}
inline u64 round_up_u64(u64 x, u64 b) {BODY}
inline i32 round_up_i32(i32 x, i32 b) {BODY}
inline i64 round_up_i64(i64 x, i64 b) {BODY}
#undef BODY

inline String8
make_data(void *memory, u64 size)
{
 String8 data = {(u8*)memory, size};
 return(data);
}

inline void
block_copy(void *dst, const void *src, u64 size)
{
#if AD_IS_DRIVER
 memmove(dst, src, size);
#else
 // NOTE(kv): We got screwed over by changing this to "memcpy" once.
 // Seriously it's NOT worth it man!
 gb_memmove(dst, src, size);
#endif
}

inline void
block_copy_non_overlap(void *dst, const void *src, u64 size)
{
#if AD_IS_DRIVER
 memcpy(dst, src, size);
#else
 gb_memcopy(dst, src, size);
#endif
}

#if !AD_IS_DRIVER
////////////////////////////////
#define make_data_struct(s) make_data((s), sizeof(*(s)))

#define data_initr(m,s) {(u8*)(m), (s)}
#define data_initr_struct(s) {(u8*)(s), sizeof(*(s))}
#define data_initr_array(a) {(u8*)(a), sizeof(a)}
#define data_initr_string(s) {(u8*)(s), sizeof(s) - 1}

////////////////////////////////

inline void
block_zero(String8 data) {
 block_zero(data.str, data.size);
}
function void
block_fill_ones(String8 data){
    block_fill_ones(data.str, data.size);
}

force_inline i32
block_compare(void *a, void *b, u64 size)
{
    return gb_memcompare(a, b, size);
}

force_inline b32
block_match(void *a, void *b, u64 size)
{
    return (block_compare(a,b,size) == 0);
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


function void
block_range_copy__inner(void *dst, void *src, Range_u64 range, i64 shift){
    block_copy((u8*)dst + range.first + shift, (u8*)src + range.first, range.max - range.min);
}

function void
block_range_copy__inner(void *dst, void *src, Range_u64 range, i64 shift, u64 item_size){
    range.first *= item_size;
    range.opl *= item_size;
    shift *= item_size;
    block_range_copy__inner(dst, src, range, shift);
}

#define block_range_copy(d,s,r,h) block_range_copy__inner((d),(s),Iu64(r),(i64)(h))
#define block_range_copy_sized(d,s,r,h,i) block_range_copy__inner((d),(s),Iu64(r),(i64)(h),(i))
#define block_range_copy_typed(d,s,r,h) block_range_copy_sized((d),(s),(r),(h),sizeof(*(d)))

function void
block_copy_array_dst_shift__inner(void *dst, void *src, u64 it_size, Range_i64 range, i64 shift){
    u8 *dptr = (u8*)dst;
    u8 *sptr = (u8*)src;
    dptr += it_size*(range.first + shift);
    sptr += it_size*range.first;
    block_copy(dptr, sptr, (u64)(it_size*(range.opl - range.first)));
}
function void
block_copy_array_dst_shift__inner(void *dst, void *src, u64 it_size, Range_i32 range, i64 shift){
    u8 *dptr = (u8*)dst;
 u8 *sptr = (u8*)src;
 dptr += it_size*(range.first + shift);
 sptr += it_size*range.first;
 block_copy(dptr, sptr, (u64)(it_size*(range.opl - range.first)));
}

#define block_copy_array_dst_shift(d,s,r,h) block_copy_array_dst_shift__inner((d),(s),sizeof(*(d)),(r),(h))
#endif

//TODO(kv) What is with all the "block"?
#define block_zero_struct(p) block_zero((p), sizeof(*(p)))
#define block_zero_array(a)  block_zero((a), sizeof(a))
#define block_zero_dynamic_array(p,c) block_zero((p), sizeof(*(p))*(c))

#define block_copy_struct(d,s) block_copy((d), (s), sizeof(*(d)))
//NOTE(kv) Due to C array being major ass, we don't know the size of arrays.
//  This is not a language for content creation.
#define copy_array_dst(d,s)  block_copy((d), (s), sizeof(d))
#define copy_array_src(d,s)  block_copy((d), (s), sizeof(s))
#define block_copy_count(d,s,c) block_copy((d), (s), sizeof(*(d))*(c))

#define block_match_struct(a,b) block_match((a), (b), sizeof(*(a)))
#define block_match_array(a,b) block_match((a), (b), sizeof(a))

//-


function f32
abs_f32(f32 x)
{
 if (x < 0){
  x = -x;
 }
 return(x);
}

function f32
mod_f32(f32 x, i32 m)
{
 f32 whole;
 f32 frac = modff(x, &whole);
 f32 r = f32((i32)(whole) % m) + frac;
 return(r);
}

//~ NOTE(kv): Trig functions

// TODO: make these be based on actual turn
force_inline v1
cosine(v1 x)
{
    return cosf(TAU32 * x);
}

force_inline f32
sine(v1 x)
{
    return sinf(TAU32 * x);
}

force_inline v1
arctan2(v1 y, v1 x)
{
 return atan2f(y,x) / TAU32;
}

force_inline v1
arcsin(v1 between_zero_and_one)
{
 return asinf(between_zero_and_one) / TAU32;
}

force_inline v1
arccos(v1 between_minus_one_and_one)
{
 return acosf(between_minus_one_and_one) / TAU32;
}

////////////////////////////////

force_inline i2 I2(i32 x, i32 y) { return {x, y}; }
force_inline i3 I3(i32 x, i32 y, i32 z) { return {x, y, z}; }
force_inline i4 I4(i32 x, i32 y, i32 z, i32 w) { return {x, y, z, w}; }
force_inline i4 I4() { return {}; }
force_inline i4 I4(i32 x) { return i4{repeat4(x)}; }

force_inline v2
V2(v1 x, v1 y)
{
 v2 v = {x, y};
 return(v);
}

force_inline v2
cast_V2(i32 x, i32 y)
{
 v2 v = {(v1)x, (v1)y};
 return(v);
}
//

force_inline i2
I2(i2 o)
{
    return(I2((i32)o.x, (i32)o.y));
}
force_inline v3
V3(i3 o)
{
    return(V3((f32)o.x, (f32)o.y, (f32)o.z));
}

force_inline Vec2_i32
operator+(Vec2_i32 a, Vec2_i32 b){
 a.x += b.x;
 a.y += b.y;
 return(a);
}
force_inline i2&
operator+=(i2 &a, i2 b){
 a.x += b.x;
 a.y += b.y;
 return(a);
}
function i3
operator+(i3 a, i3 b){
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    return(a);
}
function Vec3_i32&
operator+=(Vec3_i32 &a, Vec3_i32 b){
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
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
function Vec4_f32
operator/(Vec4_f32 v, f32 s){
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
function v2&
operator/=(v2 &v, f32 s){
    v.x /= s;
    v.y /= s;
    return(v);
}
function v3&
operator/=(v3 &v, f32 s){
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

function bool
operator==(Vec2_i32 a, Vec2_i32 b){
    return(a.x == b.x && a.y == b.y);
}
function bool
operator==(Vec3_i32 a, Vec3_i32 b){
    return(a.x == b.x && a.y == b.y && a.z == b.z);
}
function bool
operator==(Vec4_f32 a, Vec4_f32 b){
    return(a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w);
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
near_zero(v4 p, v1 epsilon)
{
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

#if 0
function i32
lerp(i32 a, f32 t, i32 b){
    return((i32)(lerp((f32)a, t, (f32)b)));
}
#endif

function Vec2_f32
lerp(Vec2_f32 a, f32 t, Vec2_f32 b){
    return(a + (b-a)*t);
}

force_inline v3
lerp(v3 a, v1 t, v3 b){
    return(a + (b-a)*t);
}

force_inline v1
unlerp(v1 a, v1 x, v1 b)
{
 v1 r = 0.f;
 if (b != a)
 {
  r = (x - a)/(b - a);
 }
 return(r);
}

force_inline v1
clamp01(v1 v)
{
 macro_clamp01(v);
 return v;
}

force_inline v1
unlerp01(v1 a, v1 v, v1 b)
{
 return clamp01( unlerp(a,v,b) );
}

function v1
smoothstep(v1 a, v1 x, v1 b)
{
 if (a != b)
 {
  v1 t = clamp01((x - a) / (b - a));
  return t*t*(3.f - (2.f*t));
 }
 else if (x > a) { return 1.f; }
 else { return 0.f; }
}

function Range_f32
unlerp(f32 a, Range_f32 x, f32 b)
{
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

function bool
operator==(Rect_i32 a, Rect_i32 b){
    return(a.p0 == b.p0 && a.p1 == b.p1);
}
function bool
operator==(Rect_f32 a, Rect_f32 b){
    return(a.p0 == b.p0 && a.p1 == b.p1);
}

function Vec2_f32
rect_center(Rect_f32 r){
 return((r.p0 + r.p1)*0.5f);
}

////////////////////////////////

function v4
argb_unpack(ARGB_Color color)
{
    v4 result;
    result.a = ((color >> 24) & 0xFF)/255.f;
    result.r = ((color >> 16) & 0xFF)/255.f;
    result.g = ((color >> 8)  & 0xFF)/255.f;
    result.b = ((color >> 0)  & 0xFF)/255.f;
    return(result);
}

function ARGB_Color
argb_pack(v4 color)
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
    Vec4_f32 av = argb_unpack(a);
    Vec4_f32 bv = argb_unpack(b);
    Vec4_f32 v = lerp(av, t, bv);
    return(argb_pack(v));
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

function bool
operator==(Range_i32 a, Range_i32 b){
    return(a.min == b.min && a.max == b.max);
}
function bool
operator==(Range_i64 a, Range_i64 b){
    return(a.min == b.min && a.max == b.max);
}
function bool
operator==(Range_u64 a, Range_u64 b){
    return(a.min == b.min && a.max == b.max);
}
function bool
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

#define BODY  return(a.min <= p && p <= a.max);
function b32 range_contains_inclusive(Range_i32 a, i32 p){ BODY }
function b32 range_contains_inclusive(Range_i64 a, i64 p){ BODY }
function b32 range_contains_inclusive(Range_u64 a, u64 p){ BODY }
function b32 range_inclusive_contains(Range_f32 a, f32 p){ BODY }
#undef BODY

#define BODY return(a.min <= p && p < a.max);
function b32 range_contains(Range_i32 a, i32 p){ BODY }
function b32 range_contains(Range_i64 a, i64 p){ BODY }
function b32 range_contains(Range_u64 a, u64 p){ BODY }
function b32 range_contains(Range_f32 a, f32 p){ BODY }
#undef BODY

#define BODY  return(clamp_min(0, a.max - a.min));
function i32 range_size(Range_i32 a){ BODY; }
function i64 range_size(Range_i64 a){ BODY; }
function u64 range_size(Range_u64 a){ BODY; }
function f32 range_size(Range_f32 a){ BODY; }
#undef BODY

#define BODY  return(clamp_min(0, a.max - a.min + 1));
function i32 range_size_inclusive(Range_i32 a) { BODY }
function i64 range_size_inclusive(Range_i64 a) { BODY }
function u64 range_size_inclusive(Range_u64 a) { BODY }
function f32 range_size_inclusive(Range_f32 a) { BODY }
#undef BODY

function Range_i32 rectify(Range_i32 a){ return(Ii32(a.min, a.max)); }
function Range_i64 rectify(Range_i64 a){ return(Ii64(a.min, a.max)); }
function Range_u64 rectify(Range_u64 a){ return(Iu64(a.min, a.max)); }
function Range_f32 rectify(Range_f32 a){ return(If32(a.min, a.max)); }

function Range_i32
range_clamp_size(Range_i32 a, i32 max_size){
    i32 max = a.min + max_size;
    a.max = clamp_max(a.max, max);
    return(a);
}
function Range_i64
range_clamp_size(Range_i64 a, i64 max_size){
    i64 max = a.min + max_size;
    a.max = clamp_max(a.max, max);
    return(a);
}
function Range_u64
range_clamp_size(Range_u64 a, u64 max_size){
    u64 max = a.min + max_size;
    a.max = clamp_max(a.max, max);
    return(a);
}
function Range_f32
range_clamp_size(Range_f32 a, f32 max_size){
    f32 max = a.min + max_size;
    a.max = clamp_max(a.max, max);
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

function u64
cstring_length(u8 *str){
 u64 length = 0;
 if (str) {
  for (;str[length] != 0; length += 1);
 }
 return(length);
}
//
force_inline u64 cstring_length(char *str) {
 return cstring_length((u8 *)str);
}

force_inline String SCu8(void)             { return {}; }
force_inline String SCu8(char &c)          { return {(u8*)&c, 1}; }
force_inline String SCu8(u8 *str, u64 size){ return {str, size}; }
force_inline String SCu8(u8 *str)          { return {str, cstring_length(str)}; }
force_inline String SCu8(char *str)        { return(SCu8((u8*)str)); }
force_inline String SCu8(const char *str)  { return(SCu8((u8*)str)); }

struct String_u8
{
 union
 {
  String string;
  struct
  {
   u8 *str;
   u64 size;
  };
 };
 u64 cap;
};
#if !AD_IS_DRIVER
function b32
string_concat(String_u8 *dst, String src)
{
 b32 result = false;
 u64 available = dst->cap - dst->size;
 if (src.size <= available){
  result = true;
 }
 u64 copy_size = clamp_max(src.size, available);
 block_copy(dst->str + dst->size, src.str, copy_size);
 dst->size += copy_size;
 return(result);
}
#endif

#define string_litexpr(s) SCchar((s), sizeof(s) - 1)
//NOTE(kv) The length of the string is the size minus one for the null terminator.
#define string_u8_litexpr(s) SCu8((u8*)(s), (u64)(sizeof(s) - 1))
#define str8lit string_u8_litexpr
#define strlit  string_u8_litexpr
#define string_u16_litexpr(s) SCu16((u16*)(s), (u64)(sizeof(s)/2 - 1))

#define string_expand(s) (i32)(s).size, (char*)(s).str
#define strexpand string_expand

global_const String empty_string = {(u8*)"", 0};

#define filename_linum strlit(filename_line_number)

//~Arena
#include "sanitizer/asan_interface.h"
#if defined(__has_feature)
#    if __has_feature(address_sanitizer) // this is clang
#        define __SANITIZE_ADDRESS__
#    endif
#endif
//
#ifdef __SANITIZE_ADDRESS__
#    define ASAN_ON 1
#endif

function void*
base_reserve__noop(void *user_data, u64 size, u64 *size_out, String location){
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
make_base_allocator_generic(Allocator_Reserve_Signature *func_reserve,
                            Allocator_Commit_Signature  *func_commit,
                            Allocator_Uncommit_Signature *func_uncommit,
                            Allocator_Free_Signature     *func_free,
                            Allocator_Set_Access_Signature *func_set_access,
                            void *userdata)
{
 if (func_reserve    == 0){ func_reserve    = base_reserve__noop; }
 if (func_commit     == 0){ func_commit     = base_commit__noop; }
 if (func_uncommit   == 0){ func_uncommit   = base_uncommit__noop; }
 if (func_free       == 0){ func_free       = base_free__noop; }
 if (func_set_access == 0){ func_set_access = base_set_access__noop; }
 Base_Allocator result = {
  .type=Allocator_Generic,
  .generic={
   .reserve   =func_reserve,
   .commit    =func_commit,
   .uncommit  =func_uncommit,
   .free      =func_free,
   .set_access=func_set_access,
   .userdata  =userdata,
  },
 };
 return(result);
}

function String
arena_push(Arena *arena, usize size, String location, u32 align_pow2);

function String
base_allocate_function(Base_Allocator *allocator, u64 size, String debug_location)
{// @todo_leak_check
 String result = {};
 
 switch(allocator->type)
 {
  case Allocator_Generic:
  {
   auto &a = allocator->generic;
   // TODO(kv): Just do one call and simplify this allocator interface?
   result.str = (u8 *)a.reserve(a.userdata, size, &result.size, debug_location);
   a.commit(a.userdata, result.str, result.size);
  }break;
  
  case Allocator_Arena:
  {// NOTE(kv): This is a cyclic dependency situation.
   result = arena_push(allocator->arena, size, debug_location, 3);
  }break;
  
  case Allocator_Malloc:
  {
   result.str  = (u8 *)malloc(size);
   result.size = size;
  }break;
  
  invalid_default_case;
 }
 
 return result;
}

function void
base_free(Base_Allocator *allocator, void *ptr, umm optional_size=0)
{
 if (ptr != 0)
 {
  switch(allocator->type)
  {
   case Allocator_Generic: {
    auto &a = allocator->generic;
    a.free(a.userdata, ptr);
   }break;
   
   case Allocator_Arena: {
    auto a = allocator->arena;
    ASAN_POISON_MEMORY_REGION(ptr, optional_size);
   } break;
   
   case Allocator_Malloc: { free(ptr); } break;
   
   invalid_default_case;
  } 
 }
}
inline void
free_cursor(Base_Allocator *allocator, Memory_Cursor *cursor)
{
 base_free(allocator, cursor);
}

// TODO(kv): Does anyone actually care about the returned size? And why do they care?
#define base_allocate2(a,s)      base_allocate_function((a), (s), filename_linum)
#define base_allocate(a,s)       base_allocate2(a,s).str
#define base_array_loc(a,T,c,l) (T*)(base_allocate_function((a), sizeof(T)*(c), (l)).str)
#define base_array(a,T,c)       base_array_loc(a,T,c, filename_linum)

//-

inline Memory_Cursor
make_cursor(void *base, u64 cap)
{
    Memory_Cursor cursor = {.base=(u8*)base, .cap=cap};
    return(cursor);
}
inline Memory_Cursor
make_cursor(String data) {
 return(make_cursor(data.str, data.size));
}
inline Memory_Cursor
make_cursor(Base_Allocator *allocator, u64 size)
{
 String memory = base_allocate2(allocator, size);
 return(make_cursor(memory));
}

function String
cursor_push(Memory_Cursor *cursor, usize size, String location, u32 align_pow2)
{// TODO(kv): This allocation is not optimal yet in non-asan builds.
 kv_assert(align_pow2 <= 3);  //NOTE(kv): don't support simd rn
#if ASAN_ON
 // NOTE(kv): we want a poisoned region to the left of the
 //   unpoisoned allocation region, then the allocation has to
 //   begin on an 8-byte boundary.
 macro_clamp_min(align_pow2, 3);
#endif
 
 String result = {};
 usize align_size = 1ULL << align_pow2;
 usize align_mask = align_size-1;
 u8 *current_pos = cursor->base+cursor->used_;
 
 // NOTE(kv): "redzone_size" doesn't include alignment
#if ASAN_ON
 umm redzone_size = 128;
#else
 umm redzone_size = 0;
#endif
 
 umm pos_umm = umm(current_pos) + redzone_size;
 pos_umm = (pos_umm + align_mask) & (~align_mask);
 u8 *pos = cast(u8*)(pos_umm);
 usize new_used = (pos+size)-cursor->base;
 if (new_used <= cursor->cap)
 {
  cursor->used_ = new_used;
  result = {(u8*)pos, size};
  kv_assert((usize(pos) & align_mask) == 0);
  kv_assert((usize(pos) & 7)          == 0);
  // NOTE(kv): Unpoisoned region has to start 8-byte aligned,
  // which is this case right here (with the redzone being on the left).
  ASAN_UNPOISON_MEMORY_REGION(pos, size);
 }
 return(result);
}
function void
cursor_pop(Memory_Cursor *cursor, u64 size)
{
 kv_assert(cursor->used_ >= size);
 cursor->used_ -= size;
 ASAN_POISON_MEMORY_REGION(cursor->base+cursor->used_, size);
}
inline void
cursor_pop_to(Memory_Cursor *cursor, umm used)
{
 cursor_pop(cursor, cursor->used_-used);
}

function Arena
make_arena(Base_Allocator *allocator, u64 chunk_size=KB(64))
{
 Arena arena = {allocator, 0, chunk_size};
 return(arena);
}



function String
arena_push(Arena *arena, usize size, String location, u32 align_pow2)
{
 kv_assert(align_pow2 <= 3);  //NOTE(kv): don't support simd rn
 String result = {};
 if (size > 0)
 {
  Memory_Cursor *cursor = arena->cursor;
  if (cursor) {
   // NOTE(kv): This might fail if the cursor doesn't have enough space.
   result = cursor_push(cursor, size, location, align_pow2);
  }
  
  if (result.str == 0)
  {
#if ASAN_ON
   // NOTE(kv): alignment+redzone
   umm max_allocation_waste = 256;
#else
   umm max_allocation_waste = 0;  // NOTE(kv): Assuming the cursor doesn't mess with us
#endif
   umm min_cursor_size = size + max_allocation_waste;
   {// NOTE(kv): Make a new cursor
    u64 usable_size = clamp_min(min_cursor_size, arena->chunk_size);
    String memory = base_allocate_function(arena->base_allocator, usable_size+sizeof(Memory_Cursor), location);
    // NOTE(kv): Tricky business: put the cursor in its own memory
    cursor = cast(Memory_Cursor *)memory.str;
    // NOTE(kv): We keep the base at the first useful address
    //   rather than the first allocated address, so it's cleaner.
    *cursor = make_cursor(cursor+1, usable_size);
    sll_stack_push(arena->cursor, cursor);
    ASAN_POISON_MEMORY_REGION(cursor+1, usable_size);
    // NOTE(kv): cursor+1 is usable memory, but when you push something on,
    //   There will be poison to the left (see cursor_push),
    //   so asan can detect somebody overwriting the cursor.
   }
   
   result = cursor_push(cursor, size, location, align_pow2);
  }
  kv_assert(result.str);
 }
 return(result);
}
//
function void
arena_pop(Arena *arena, u64 size)
{
 Memory_Cursor *cursor = arena->cursor;
 if (size >= cursor->used_) {
  // NOTE(kv): we can't handle this, because alignment complicates the calculus
  invalid_code_path;
 } else {
  cursor_pop(cursor, size);
 }
}
//
function Temp_Memory
begin_temp(Arena *arena)
{
 Memory_Cursor *cursor = arena->cursor;
 Temp_Memory temp = {
  .arena  = arena,
  .cursor = cursor,
  .used   = (cursor == 0 ? 0 : cursor->used_),
 };
 return(temp);
}
function void
end_temp(Temp_Memory temp)
{
 Arena *arena = temp.arena;
 Base_Allocator *allocator = arena->base_allocator;
 Memory_Cursor *cursor = arena->cursor;
 for (Memory_Cursor *prev = 0;
      cursor != temp.cursor && cursor != 0;
      cursor = prev)
 {
  prev = cursor->next;
  free_cursor(allocator, cursor);
 }
 arena->cursor = cursor;
 if (cursor != 0)
 {
  if (temp.used > 0)
  {
   cursor_pop(cursor, cursor->used_ - temp.used);
  }
  else
  {// TODO(kv): This should never happen, right?
   // Why would a cursor exist when it's empty?
   arena->cursor = cursor->next;
   free_cursor(allocator, cursor);
  }
 }
}

function void
arena_clear(Arena *arena)
{
 Temp_Memory temp = {arena, 0, 0};
 end_temp(temp);
}

//-
function void*
linalloc_wrap(String8 data, b32 zero=false)
{
 if (zero) { block_zero(data.str, data.size); }
 return(data.str);
}
function void*
linalloc_wrap_write(String8 data, u64 size, void *src)
{
 block_copy(data.str, src, clamp_max(data.size, size));
 return(data.str);
}
#define push_size(a,s,...)        (u8 *)linalloc_wrap(arena_push(a, s, filename_linum, 3), __VA_ARGS__)
#define push_struct(a,T,...)      (T*)push_size(a,sizeof(T),__VA_ARGS__)
#define push_array(a,T,c,...)     (T*)push_size(a, sizeof(T)*(c), __VA_ARGS__)
#define push_array_zero(a,T,c)    push_array(a,T,c,true)
#define push_array_copy(a,T,c,s)  ((T*)linalloc_wrap_write(arena_push(a, sizeof(T)*(c), filename_linum, 3), sizeof(T)*(c), (s)))
#define pop_array(a,T,c)          (arena_pop((a), sizeof(T)*(c)))
//-

//-
struct Temp_Memory_Block{
 Temp_Memory temp;
 Temp_Memory_Block(Temp_Memory temp);
 Temp_Memory_Block(Arena *arena);
 ~Temp_Memory_Block();
 void restore(void);
};

inline Temp_Memory_Block::Temp_Memory_Block(Temp_Memory t){
 this->temp = t;
}

inline Temp_Memory_Block::Temp_Memory_Block(Arena *arena){
 this->temp = begin_temp(arena);
}

inline Temp_Memory_Block::~Temp_Memory_Block(){
 end_temp(this->temp);
}

inline void
Temp_Memory_Block::restore(void){
 end_temp(this->temp);
}
//-

// NOTE(kv): Not usable in hot-reloaded code!!!
global Base_Allocator malloc_base_allocator = {
 .type = Allocator_Malloc
};
//
function Base_Allocator *
get_allocator_malloc(void) {
 return(&malloc_base_allocator);
}
//
function Arena
make_arena_malloc(u64 chunk_size=KB(16))
{
 Base_Allocator *allocator = get_allocator_malloc();
 return(make_arena(allocator, chunk_size));
}


//TODO: Scratch_Block requires that we pass it a Thread_Context, which is annoying,
// But we can maybe use thread-local storage whatever.
// NOTE: We use make_arena_malloc because this is basically malloc that automatically frees for us.
#define make_temp_arena(ARENA_NAME) \
Arena value_##ARENA_NAME = make_arena_malloc(); \
Arena *ARENA_NAME = &value_##ARENA_NAME; \
defer( arena_clear(ARENA_NAME) );

//NOTE(kv) "defer_block" courtesy of Ryan Fleury.
#define defer_block(STARTUP, SHUTDOWN) \
for(int line_unique_var = ((STARTUP), 0); \
!line_unique_var; \
line_unique_var++, (SHUTDOWN))


////////////////////////////////

#if !AD_IS_DRIVER
#    include "kv_extra.h"
#endif

//-


force_inline v2 V2(v1 value) { return v2{repeat2(value)}; }
force_inline v3 V3(v1 value) { return v3{repeat3(value)}; }
force_inline v4 V4(v1 value) { return v4{repeat4(value)}; }
force_inline v2 V2() { return v2{repeat2(0)}; }
force_inline v3 V3() { return v3{repeat3(0)}; }
force_inline v4 V4() { return v4{repeat4(0)}; }

inline v1
dot(v4 const v, v4 const u)
{
 v1 result = v.x*u.x + v.y*u.y + v.z*u.z + v.w*u.w;
 return result;
}

function v3 
matvmul3(mat3 const&matrix, v3 v)
{
    v1 row0 = dot(v, matrix.rows[0]);
    v1 row1 = dot(v, matrix.rows[1]);
    v1 row2 = dot(v, matrix.rows[2]);
    v3 result = V3(row0, row1, row2);
    return result;
}

function mat3
operator*(mat3 const&A, mat3 const&B)
{
 mat3 R = {};
 for_i32(r,0,3) // NOTE(casey): Rows (of A)
 {
  for_i32(c,0,3) // NOTE(casey): Column (of B)
  {
   for_i32(i,0,3) // NOTE(casey): i = Column of A = Row of B
   {
    R.e[r][c] += A.e[r][i] * B.e[i][c];
   }
  }
 }
 return(R);
}

// NOTE: Everyone should get fired, for writing compilers that do stupid things.
function v4
operator*(mat4 const&matrix, v4 v)
{
 v4 result = {};
 for_i32(r,0,4)
 {
  for_i32(i,0,4)
  {
   result.v[r] += matrix.e[r][i] * v.v[i];
  }
 }
 return result;
}

function mat4
mat4mul(mat4 const*A, mat4 const*B)
{
 mat4 R = {};
 for_i32(r,0,4) // NOTE(casey): Rows (of A)
 {
  for_i32(c,0,4) // NOTE(casey): Column (of B)
  {
   for_i32(i,0,4) // NOTE(casey): i = Column of A = Row of B
   {
    R.e[r][c] += A->e[r][i] * B->e[i][c];
   }
  }
 }
 return(R);
}

//NOTE: This actually allows us to "pass by value"
// And clang actually does the right optimization in debug, which is refreshing.
force_inline mat4
operator*(mat4 const&A, mat4 const&B)
{
 return mat4mul(&A,&B);
}

inline mat3
to_mat3(mat4 const&m)
{
 mat3 result;
 for_i32(index,0,3)
 {
  result.rows[index] = m.rows[index].xyz;
 }
 return result;
}

force_inline v3
mat4vert_div(mat4 const&A, v3 v)
{
 v4 Av = A * V4(v,1.f);
 return Av.xyz / Av.w;
}
force_inline v3
mat4vert(mat4 const&A, v3 v)
{
 v4 Av = A * V4(v, 1.f);
 return Av.xyz;
}
// IMPORTANT IMPORTANT IMPORTANT: I am a bad person! But there's no way around it!
force_inline v3
operator*(mat4 const&A, v3 v)
{
return mat4vert(A,v);
}

force_inline v3
mat4vec(mat4 const&A, v3 v)
{
 v4 result = A * V4(v,0.f);
 return result.xyz;
}

global mat4i mat4i_identity = {mat4_identity, mat4_identity};

function mat4i
invert(mat4i in)
{
 return mat4i{in.inverse, in.forward};
}

function mat4
mat4_scales(v1 sx, v1 sy, v1 sz)
{
 mat4 result = {{
   sx,0,0,0,
   0,sy,0,0,
   0,0,sz,0,
   0,0,0,1,
  }};
 return result;
}

force_inline mat4
mat4_scales(v3 scales)
{
 return mat4_scales(v3_expand(scales));
}

force_inline mat4
mat4_scale(v1 s)
{
 return mat4_scales(V3(s));
}

force_inline mat4i
mat4i_scales(v3 s)
{
 mat4i result;
 result.forward = mat4_scales(s);
 result.inverse = mat4_scales(1.f/s.x, 1.f/s.y, 1.f/s.z);
 return result;
}

force_inline mat4i
mat4i_scales(v1 sx, v1 sy, v1 sz)
{
 return mat4i_scales(V3(sx,sy,sz));
}

force_inline mat4i
mat4i_scale(v1 s)
{
 mat4i result;
 result.forward = mat4_scale(s);
 result.inverse = mat4_scale(1.f/s);
 return result;
}

function mat4 
transpose(mat4 mat)
{
 for_i32(r,0,4) { 
  for_i32(c,0,r) {
   macro_swap(mat.e[r][c], mat.e[c][r]);
  }
 }
 return mat;
}

function mat3
transpose(mat3 mat)
{
 for_i32(r,0,3) {
  for_i32(c,0,r) {
   macro_swap(mat.e[r][c], mat.e[c][r]);
  }
 }
 return mat;
}

function mat4i
mat4_columns(v3 x, v3 y, v3 z)
{
 mat4i result;
 
 mat4 &inverse = result.inverse;
 inverse.rows[0] = V4(x,0);
 inverse.rows[1] = V4(y,0);
 inverse.rows[2] = V4(z,0);
 inverse.rows[3] = V4(0,0,0,1);
 
 result.forward = transpose(inverse);
 
 return result;
}

function v4
get_column(mat4 const&m, i32 index)
{
 v4 result;
 for_i32(i,0,4)
 {
  result[i] = m.e[i][index];
 }
 return result;
}

force_inline v3
get_translation(mat4 const&mat)
{
 return get_column(mat, 3).xyz;
}

force_inline mat3
mat3_scale(v1 s)
{
 mat3 result = {{
   s,0,0,
   0,s,0,
   0,0,s,
  }};
 return result;
}

force_inline v3
operator*(mat3 const&m, v3 v)
{
 return matvmul3(m,v);
}

function mat4
to_mat4(mat3 mat, v3 translation=V3())
{
 mat4 result;
 for_i32(index,0,3)
 {
  result.rows[index] = V4(mat.rows[index], translation[index]);
 }
 result.rows[3] = V4(0,0,0,1);
 return result;
}

// NOTE(kv): I'm not sure what this is for.
struct TRS
{
 v3   translation;
 mat3 rotation;
 v1   scale;
};

function mat3
operator*(v1 s, mat3 mat)
{
 for_i32(r,0,3)
 {
  for_i32(c,0,3)
  {
   mat.e[r][c] *= s;
  }
 }
 return mat;
}

function mat3
get_rotation(mat4 const&transform)
{
 v1 scale = get_xscale(transform);
 return (1.f/scale)*to_mat3(transform);
}

function TRS
trs_decompose(mat4 const&transform)
{
 v1 scale = get_xscale(transform);
 TRS out;
 out.translation = get_translation(transform);
 out.rotation    = (1.f/scale) * to_mat3(transform);
 out.scale       = scale;
 return out;
}

function mat4
mat4_translate(v3 vector)
{
 mat4 result = {{
   1,0,0,vector.x,
   0,1,0,vector.y,
   0,0,1,vector.z,
   0,0,0,1,
  }};
 return result;
}

force_inline mat4i
mat4i_translate(v3 vector)
{
 mat4i result;
 result.forward = mat4_translate(vector);
 result.inverse = mat4_translate(-vector);
 return result;
}

function void
rotation_pivot_helper(mat4 *matrix, v3 pivot)
{
 if ( pivot != v3{} )
 {
  v3 translation = mat4vec(*matrix, (-pivot)) + pivot;
  for_i32 (index,0,3) { matrix->e[index][3] = translation[index]; }
 }
}
//
inline void
rotation_pivot_helper(mat4i *matrix, v3 pivot)
{
 rotation_pivot_helper(&matrix->forward, pivot);
 rotation_pivot_helper(&matrix->inverse, pivot);
}

function mat4
rotateX(v1 turn, v3 pivot={})
{
 v1 c = cosine(turn);
 v1 s = sine  (turn);
 mat4 result = {{
   1, 0, 0, 0, 
   0, c,-s, 0,
   0, s, c, 0,
   0, 0, 0, 1,
  }};
 rotation_pivot_helper(&result, pivot);
 return result;
}
//
function mat4i
mat4i_rotateX(v1 turn, v3 pivot={})
{
 mat4i result;
 result.forward = rotateX(turn, pivot);
 result.inverse = rotateX(-turn, pivot);
 return result;
}

function mat4
rotateY(v1 turn, v3 pivot={})
{
 v1 c = cosine(turn);
 v1 s = sine  (turn);
 mat4 result = {{
   +c, 0, s, 0, 
   +0, 1, 0, 0,
   -s, 0, c, 0,
   +0, 0, 0, 1,
  }};
 rotation_pivot_helper(&result, pivot);
 return result;
}
//
function mat4i
mat4i_rotateY(v1 turn, v3 pivot={})
{
 mat4i result;
 result.forward = rotateY(turn, pivot);
 result.inverse = rotateY(-turn, pivot);
 return result;
}

function mat4
rotateZ(v1 turn, v3 pivot={})
{
 v1 c = cosine(turn);
 v1 s = sine  (turn);
 mat4 result = {{
   c, -s, 0, 0, 
   s, c, 0, 0,
   0, 0, 1, 0,
   0, 0, 0, 1,
  }};
 rotation_pivot_helper(&result, pivot);
 return result;
}
//
function mat4i
mat4i_rotateZ(v1 turn, v3 pivot={})
{
 mat4i result;
 result.forward = rotateZ(turn, pivot);
 result.inverse = rotateZ(-turn, pivot);
 return result;
}

//////////////////////////////////////////////////

force_inline mat4i
mat4i_rotate(mat3 rot)
{
 mat4i result;
 result.forward = to_mat4(rot);
 result.inverse = to_mat4(transpose(rot));
 return result;
}

//NOTE: Compose transformations
force_inline mat4i
operator*(mat4i const& A, mat4i const& B)
{
 mat4i result;
 result.forward = A.forward * B.forward;
 result.inverse = B.inverse * A.inverse;
 return result;
}

force_inline v2 arm2(v1 turn)
{
 return v2{cosine(turn), sine(turn)};
}

force_inline v3 V3x(v1 x) { v3 result = {}; result.x=x; return result; }
force_inline v3 V3y(v1 y) { v3 result = {}; result.y=y; return result;  }
force_inline v3 V3z(v1 z) { v3 result = {}; result.z=z; return result; }
force_inline v3 setx(v3 v, v1 x) { v.x=x; return v; }
force_inline v3 sety(v3 v, v1 y) { v.y=y; return v; }
force_inline v3 setz(v3 v, v1 z) { v.z=z; return v; }
force_inline v3 addx(v3 v, v1 x) { v.x+=x; return v; }
force_inline v3 addy(v3 v, v1 y) { v.y+=y; return v; }
force_inline v3 addz(v3 v, v1 z) { v.z+=z; return v; }
force_inline v3 zeroX(v3 value) { value.x=0; return value; };

inline v1 srgb_to_linear1(v1 x)
{
 v1 r = ((x <= 0.04045f) ? 
         x/12.92f : 
         powf((x + 0.055f)/1.055f, 2.4f));
 return(r);
}

inline v1 linear_to_srgb1(v1 x)
{
 v1 r = ((x <= 0.0031308) ? 
         x*12.92f : 
         powf(x, 1/2.4f)*1.055f - 0.055f);
 return(r);
}

force_inline v3 
clamp01(v3 v)
{
 for_i32(i,0,3)
 {
  macro_clamp01(v.v[i]);
 }
 return v;
}

#define scale_in_block(variable, multiplier) \
variable *= multiplier; \
defer(variable /= multiplier)

// TODO: This should be specialized to a 128 value or something
#define set_in_block(variable, value) \
auto PP_Concat(old_value, __LINE__) = variable; variable = value; defer(variable = PP_Concat(old_value,__LINE__);)

force_inline v1 i2f6 (i32 integer) { return v1(integer) / 6.f; }
force_inline v1 i2f(i32 integer, v1 div) { return v1(integer) / div; }
force_inline v4
i2f6(i4 vi)
{
 v4 result;
 for_i32(index,0,4) { result[index] = v1(vi[index]) / 6.f; }
 return result;
}


force_inline v1
step(v1 edge, v1 x)
{
 return (x < edge) ? 0.f : 1.f;
}


force_inline v3
step(v3 edge, v3 v)
{
 return V3(step(edge.x, v.x),
           step(edge.y, v.y),
           step(edge.z, v.z));
}

force_inline i1
signof(i1 x)
{
 return (x == 0 ? 0 :
         x > 0  ? 1 :
         -1);
}

force_inline v1
signof(v1 x)
{
 return (x == 0.f ? 0.f :
         x > 0.f  ? 1.f :
         -1.f);
}
force_inline v3
signof(v3 v)
{
 return V3(signof(v.x),
           signof(v.y),
           signof(v.z));
}


function mat4i
mat4i_rotate_tpr(v1 theta, v1 phi, v1 roll, v3 pivot={})
{// NOTE: Roll around z, then rotate around x, then rotate around y
 // NOTE Weird, in the inverse, we want to the roll_inv *last*
 // and so we endup doing the roll *first* in the forward direction.
 
 phi  *= -1.f;  // NOTE: But the rotation axes are "mirrored" since we want camera control to be intuitive
 roll *= -1.f;
 
 mat4i result;
 {
  v1 ct = cosine(theta);
  v1 st = sine  (theta);
  v1 cp = cosine(phi);
  v1 sp = sine  (phi);
  //NOTE: we're just doing a matmul by ourselves here, for reasons.
  result.inverse = {{
    ct,     0,   -st,    0,
    sp*st,  cp,   ct*sp, 0,
    cp*st, -sp,   cp*ct, 0,
    0,      0,    0,     1,}};
 }
 
 result.inverse = rotateZ(-roll)*result.inverse;
 result.forward = transpose(result.inverse);
 rotation_pivot_helper(&result, pivot);
 
 return result;
}

inline v3
tpr_point(v1 theta, v1 phi)
{
 return mat4i_rotate_tpr(theta,phi,0,V3()) * V3(0,0,1);
}

global_const mat4 mat4_negateX = {{
  -1,0,0,0,
  0,1,0,0,
  0,0,1,0,
  0,0,0,1,
 }};


force_inline v3 
negateX(v3 vert) 
{
 return V3(-vert.x, vert.y, vert.z);
}

inline mat4
negateX(mat4 mat)
{
 for_i32(row,0,4) { mat.e[row][0] *= -1.f; }
 return mat;
}
//
inline mat4i
negateX(mat4i mat)
{
 for_i32(row,0,4) { mat.forward.e[row][0] *= -1.f; }
 for_i32(col,0,4) { mat.inverse.e[0][col] *= -1.f; }
 return mat;
}

function mat4
remove_translation(mat4 result)
{
 result[0][3] = 0.f;
 result[1][3] = 0.f;
 result[2][3] = 0.f;
 return result;
}

//~NOTE: array
// NOTE(kv): Can be zero-inited -> GOOD!
//TODO(kv) Split the metadata out to a header, that we can allocate
//  before the data, so we can pass the array around without fear.
template<class T>
struct arrayof
{
 i1 count;
 i1 cap;
 b32 fixed_size;
 T *items;
 // NOTE(kv): Optional allocator to grow
 Base_Allocator *allocator;
 
 //-
 
 force_inline T& get       (i32 index) { return items[index]; }
 force_inline T& operator[](i32 index) { return items[index]; }
 //
 force_inline T &last() { return items[count-1]; }
 
 void set_cap_(i32 new_cap)
 {// NOTE(kv): Can only grow for now
  if (new_cap > cap)
  {
   kv_assert(!fixed_size);
   // NOTE(kv): get malloc allocator here to avoid the "stale pointer" problem.
   Base_Allocator *used_allocator = (allocator ? allocator :
                                     get_allocator_malloc());
   T *old_items = items;
   items = cast(T *)base_allocate(used_allocator, new_cap*sizeof(T));
   block_copy(items, old_items, count*sizeof(T));
   // NOTE(kv): to free 100% of the space we allocated with the arena allocator,
   //   We'd have to store the allocated size too. But I don't really care.
   base_free(used_allocator, old_items, cap*sizeof(T));
   cap = new_cap;
  }
 }
 void set_cap_min(i1 cap_min)
 {// TODO(kv): This grow logic is wonky: there are two cases:
  // 1. Natural growth: doubling
  // 2. User-dictated growth: just set the cap to the dictated value
  if (cap_min > cap)
  {
   i32 new_cap = (cap == 0);
   if (cap == 0) {
    new_cap = cap_min;
   } else {
    new_cap = macro_min(cap_min, 2*cap);
   }
   set_cap_(new_cap);
  }
 }
 void set_count(i32 new_count)
 {
  kv_assert(new_count >= 0);
  set_cap_min(new_count);
  count = new_count;
  kv_assert(count <= cap);
 }
 
 inline void pop() {
  set_count(count-1);
 }
 // NOTE(kv): Have to return pointer on this one, because the push might fail.
 inline T& push(const T& item)
 {//TODO(kv): implement sorting
  set_count(count+1);
  T &result = last();
  result = item;
  return result;
 }
 //
 inline T &push2()
 {
  set_count(count+1);
  return last();
 }
 
 arrayof<T> copy(Arena *arena)
 {
  arrayof<T> result = *this;
  result.items = push_array(arena, T, count);
  umm size = count*sizeof(T);
  block_copy(result.items, items, size);
  return result;
 }
};

template<class T>
inline void
init_static(arrayof<T> &array, Arena *arena, i32 cap, b32 zero=false) {
 array = {
  .cap        = cap,
  .fixed_size = true,
  .items      = push_array(arena, T, cap, zero),
 };
}

template<class T>
inline void
init_static(arrayof<T> &array, T *backing_buffer, i32 cap)
{
 array = {
  .cap        = cap,
  .fixed_size = true,
  .items      = backing_buffer,
 };
}

template<class T>
inline void
init_dynamic(arrayof<T> &array, Base_Allocator *allocator, i1 initial_size=0) {
 array = { .allocator = allocator };
 array.set_cap_min(initial_size);
}

#define X_Basic_Types(X)  \
X(v1) X(v2) X(v3) X(v4)   \
X(i1) X(i2) X(i3) X(i4)   \
X(String)    \

enum Basic_Type
{
 Basic_Type_None = 0,
#define X(T) Basic_Type_##T,
 X_Basic_Types(X)
#undef X
 Basic_Type_Count,
};

#if 0
function Basic_Type basic_type_from_pointer(T *pointer);
#endif
//
#define X(T) \
function Basic_Type basic_type_from_pointer(T *pointer) \
{ return Basic_Type_##T; }
X_Basic_Types(X)
#undef X

#if !AD_IS_DRIVER

template<class T>
function T *
push_unique(arrayof<T> &array, T const&item)
{
 T *result = 0;
 b32 ok = true;
 
 for_i1(index,0,array.count) {
  if (array.items[index] == item) {
   ok = false;
   break;
  }
 }
 
 if (ok) {
  result = &array.push(item);
 }
 return result;
}

//~
struct Struct_Member {
 struct Type_Info *type;
 String name;
 u32    offset;
 u32    discriminator_offset;  //NOTE(kv) union only
};
struct Union_Member{
 struct Type_Info *type;
 String name;
 i32 variant;
};
struct Enum_Member {
 String name;
 i32    value;
};
//NOTE(kv) If you have a better name, I'm all ears man!
enum Type_Kind {
 Type_Kind_None = 0,
 Type_Kind_Basic,
 Type_Kind_Struct,
 Type_Kind_Union,
 Type_Kind_Enum,
};
struct Type_Info {
 String name;
 i1     size;
 Type_Kind kind;
 union {
  Basic_Type Basic_Type;
  arrayof<Struct_Member> members;
  struct {
   Type_Info *discriminator_type;
   arrayof<Union_Member> union_members;
  };
  arrayof<Enum_Member>   enum_members;
 };
};

#define X(T)                 \
Type_Info {                  \
.name=strlit(#T),            \
.size=i1(sizeof(T)),         \
.kind=Type_Kind_Basic,       \
.Basic_Type=Basic_Type_##T,  \
},                           \
//
global Type_Info basic_types_info[Basic_Type_Count+1] = {
 Type_Info{},
 X_Basic_Types(X)
};
#undef X

#define X(T)   global Type_Info &PP_Concat(Type_Info_, T) = basic_types_info[Basic_Type_##T];
X_Basic_Types(X)
#undef X

inline Type_Info &
get_basic_type_info(Basic_Type type)
{
 return basic_types_info[type];
}
//
inline i1
get_basic_type_size(Basic_Type type)
{
 return (i1)basic_types_info[type].size;
}

//~



force_inline u32
AtomicAddU32AndReturnOriginal(u32 volatile *Value, u32 Addend)
{
 // NOTE(casey): Returns the original value _prior_ to adding
 u32 Result = _InterlockedExchangeAdd((long volatile*)Value, (long)Addend);
 return(Result);
}


struct Scratch_Block {
 Thread_Context *tctx;
 Arena arena_value;  // NOTE(kv): Optional
 Arena *arena;
 Temp_Memory temp;
 
 Scratch_Block(struct Thread_Context *tctx);
 Scratch_Block(struct Thread_Context *tctx, Arena *a1);
 Scratch_Block(struct Thread_Context *tctx, Arena *a1, Arena *a2);
 Scratch_Block(struct Thread_Context *tctx, Arena *a1, Arena *a2, Arena *a3);
 inline Scratch_Block(void) {
  //NOTE(kv): in a scratch block, you don't need to worry about the malloc allocator being a global var.
  this->arena_value = make_arena( get_allocator_malloc() );
  this->arena = &arena_value;
  this->temp = begin_temp(this->arena);
 };
 Scratch_Block(struct App *app);
 Scratch_Block(struct App *app, Arena *a1);
 Scratch_Block(struct App *app, Arena *a1, Arena *a2);
 Scratch_Block(struct App *app, Arena *a1, Arena *a2, Arena *a3);
 Scratch_Block(Arena *arena);
 ~Scratch_Block();
 operator Arena*();
 void restore(void);
};

inline Scratch_Block::Scratch_Block(Thread_Context *t) {
 this->tctx = t;
 this->arena = tctx_reserve(t);
 this->temp = begin_temp(this->arena);
}

inline Scratch_Block::Scratch_Block(Thread_Context *t, Arena *a1){
 this->tctx = t;
 this->arena = tctx_reserve(t, a1);
 this->temp = begin_temp(this->arena);
}

inline Scratch_Block::Scratch_Block(Arena *arena) {
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

inline Scratch_Block::~Scratch_Block() {
 end_temp(this->temp);
 if (this->tctx) {
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

//-

function Base_Allocator
make_arena_base_allocator(Arena *arena)
{// NOTE: We put the result on the arena and return the pointer.
 Base_Allocator allocator = {
  .type  = Allocator_Arena,
  .arena = arena,
 };
 return allocator;
}


//~NOTE: Templated array
enum Container_Flag {
 Container_Unique  = 0x1,
 Container_Sorted  = 0x2,
};
typedef u32 Container_Flags;




//-

function void *
base_reserve__arena(void *userdata, u64 size, u64 *size_out, String location)
{// @todo_leak_check
 Arena *arena = cast(Arena *)userdata;
 u8 *result = push_array(arena, u8, size);
 return result;
}

//-

// IMPORTANT(kv): This is for my use only,
// it can't grow so don't pass it around
function Arena
make_static_arena(u8 *memory, u64 size)
{
 auto cursor = cast(Memory_Cursor *)memory;
 *cursor = make_cursor(cursor+1, size);
 Arena result = {
  .cursor    = cursor,
 };
 return result;
}

function Arena
sub_arena_static(Arena *arena, usize size)
{
 u8 *memory = push_array(arena, u8, size);
 Arena result = make_static_arena(memory, size);
 return result;
}

//~

function String
push_stringz(Arena *arena, String src)
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
to_cstring(Arena *arena, String8 string)
{
 String8 result = push_stringz(arena, string);
 return (char *)result.str;
}

typedef i32 Scan_Direction;
enum
{
 Scan_Backward = -1,
 Scan_Forward  =  1,
};



//~

function u64
string_find_first_non_whitespace(String str){
 u64 i = 0;
 for (;i < str.size && character_is_whitespace(str.str[i]); i += 1);
 return(i);
}

function String
string_skip_whitespace(String str)
{
 u64 f = string_find_first_non_whitespace(str);
 str = string_skip(str, f);
 return(str);
}

function String
push_stringf(Arena *arena, char *format, ...)
{
 va_list args;
 va_start(args, format);
 String result = push_stringfv(arena, format, args, false);
 va_end(args);
 return(result);
}
//
function String
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

function String8
pjoin(Arena *arena, String8 a, String8 b)
{
 String8 result = push_stringfz(arena, "%.*s%c%.*s", string_expand(a), OS_SLASH, string_expand(b));
 return result;
}

function String8
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
 char *from = to_cstring(&arena, from_filename);
 char *to = to_cstring(&arena, to_filename);
 return gb_file_move(from, to);
}

inline b32 
copy_file(String from_filename, String to_filename, b32 fail_if_exists)
{
 u8 buffer[512];
 Arena arena = make_static_arena(buffer, 512);
 char *from = to_cstring(&arena, from_filename);
 char *to = to_cstring(&arena, to_filename);
 return gb_file_copy(from, to, fail_if_exists);
}

function b32
remove_file(String filename)
{
 u8 buffer[512];
 Arena arena = make_static_arena(buffer, 512);
 char *filenamec = to_cstring(&arena, filename);
 return gb_file_remove(filenamec);
}

inline FILE *
fopen(String name, char *mode) {
 make_temp_arena(scratch);
 FILE *file = fopen(to_cstring(scratch, name), mode);
 return(file);
}

inline void
close_file(FILE *file)
{// NOTE(kv): Turns out writing a wrapper is sometimes beneficial.
 if (file != 0) {
  fclose(file);
 }
}

#if OS_WINDOWS
function b32
path_is_directory(char *path){
 DWORD attr = GetFileAttributes(path);
 return (attr & FILE_ATTRIBUTE_DIRECTORY);
}
#endif

function b32
write_entire_file(String filename, void *data, u64 size)
{
 make_temp_arena(arena);
 
 b32 result = false;
 gbFile file_value = {}; gbFile *file = &file_value;
 char *filename_c = to_cstring(arena, filename);
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
  fseek(file, 0, SEEK_SET);
  fread(mem, 1, (size_t)size, file);
  result = make_data(mem, size);
  mem[size] = 0;// NOTE: null-termination
 }
 return(result);
}

function String
read_entire_file(Arena *arena, String filename)
{
 String result = {};
 FILE *file = fopen(filename, "rb");
 if (file != 0)
 {
  result = read_entire_file_handle(arena, file);
  close_file(file);
 }
 return(result);
}

force_inline char *
read_entire_file_cstring(Arena *arena, String filename)
{// NOTE: Files are null-terminated so we're fine
 String result = read_entire_file(arena, filename);
 return (char *)result.str;
}

//~ Printer

enum Printer_Type
{
 Printer_Type_None,
 Printer_Type_Buffer,
 Printer_Type_FILE,
 Printer_Type_Generic,
};

typedef void Print_Function(void *userdata, char *format, va_list args);

struct Printer {
 Printer_Type type;
 union {
  struct {
   u8  *base;
   i32  used;
   i32  cap;
  };
  FILE *FILE;
  struct {
   void *userdata;
   Print_Function *print_function;
  };
 };
};

inline Printer
make_printer_buffer(Arena *arena, i32 cap)
{
 Printer result = {
  .type = Printer_Type_Buffer,
  .base = (u8*)push_size(arena, (usize)cap),
  .cap  = cap,
 };
 return result;
}

inline Printer
make_printer_file(FILE *file)
{
 Printer result = {
  .type = Printer_Type_FILE,
  .FILE = file,
 };
 return result;
}

function String
printer_get_string(Printer &p)
{
 kv_assert(p.type == Printer_Type_Buffer);
 String string = {
  .str  = p.base,
  .size = (u32)p.used,
 };
 return string;
}

inline void
printer_delete(Printer &p)
{
 kv_assert(p.type == Printer_Type_Buffer);
 kv_assert(p.used > 0);
 p.used--;
}

//-NOTE: "Hey, would you like variadic macros that actually works?"

#define printn2(p,x,y)       print(p,x); print(p,y);
#define printn3(p,x,y,z)     print(p,x); print(p,y); print(p,z);
#define printn4(p,x,y,z,w)   print(p,x); print(p,y); print(p,z); print(p,w);
#define printn5(p,x,...)     print(p,x); printn4(p,__VA_ARGS__);
#define printn6(p,x,...)     print(p,x); printn5(p,__VA_ARGS__);
#define printn7(p,x,...)     print(p,x); printn6(p,__VA_ARGS__);
#define printn8(p,x,...)     print(p,x); printn7(p,__VA_ARGS__);
#define printn9(p,x,...)     print(p,x); printn8(p,__VA_ARGS__);

//-NOTE Base print function overloads
function void
printer_printf(Printer &p, char *format, ...)
{
 va_list args;
 va_start(args, format);
 switch(p.type)
 {
  case Printer_Type_Buffer:
  {
   i32 remaining = p.cap-p.used;
   i32 written = vsnprintf((char *)(p.base+p.used), remaining, format, args);
   kv_assert(written >= 0 && written < remaining);
   p.used += written;
  }break;
  
  case Printer_Type_FILE:
  {
   vfprintf(p.FILE, format, args);
  }break;
  
  case Printer_Type_Generic:
  {
   p.print_function(p.userdata, format, args);
  }break;
  
  invalid_default_case;
 }
 va_end(args);
}
//-NOTE: Printing different types
inline void print(Printer &p, const char *cstring) { printer_printf(p, "%s", cstring); }
inline void print(Printer &p, String string) {
 printer_printf(p, "%.*s", string_expand(string));
}
inline void print(Printer &p, char c)  { printer_printf(p, "%c", c); }
inline void print(Printer &p, i32 d)   { printer_printf(p, "%d", d); }
inline void print(Printer &p, u32 u)   { printer_printf(p, "%u", u); }
inline void print(Printer &p, i64 ld)  { printer_printf(p, "%ld", ld); }
inline void print(Printer &p, u64 lu)  { printer_printf(p, "%lu", lu); }
//-

// NOTE(kv): This is an absolutely ridiculous hack
template <class T>
inline Printer &
operator<<(Printer &p, T object)
{
 print(p, object);
 return p;
}

//-

inline void
begin_struct(Printer &p, char *name)
{
 print(p, "(");
 print(p, name);
 print(p, "{ ");
}
//
inline void
end_struct(Printer &p)
{
 print(p, "})");
}

function void
print_code(Printer &p, Basic_Type type, void *value0, b32 wrapped);

function void
print_fieldf(Printer &p, Basic_Type type, char *name, void *value)
{
 print(p, ".");
 print(p, name);
 print(p, "=");
 print_code(p,type,value,/*wrapped*/true);
 print(p, ", ");
}

#define print_field(printer, type, value_pointer, name)\
print_fieldf(\
printer,\
Type_##type,\
#name,\
&value_pointer->name)

function void
print_float_trimmed(Printer &p, v1 value)
{
 // NOTE: there's some delete action going on, so we have to make a temp buffer
 u8 buf[256];
 Arena temp = make_static_arena(buf, sizeof(buf));
 String result = push_stringf(&temp, "%.4ff", value);
 // NOTE: trim trailing zeros
 while (result.len > 0)
 {
  if (result.str[result.len-2] == '0') { result.len -= 1; }
  else { break; }
 }
 result.str[result.len-1] = 'f';
 print(p, result);
}

function void
print_code(Printer &p, Basic_Type type, void *value0, b32 wrapped)
{
 switch(type)
 {
  case Basic_Type_v1:
  case Basic_Type_v2:
  case Basic_Type_v3:
  case Basic_Type_v4:
  {
   v1 *values = cast(v1*)value0;
   i1 count = get_basic_type_size(type) / 4;
   if (count == 1)
   {
    print_float_trimmed(p, *values);
   }
   else
   {
    if (wrapped) { print(p,"V"); print(p,count); print(p,"("); }
    for_i32(index,0,count)
    {
     if (index != 0) { print(p, ", "); }
     print_float_trimmed(p, values[index]);
    }
    if (wrapped) { print(p, ")"); }
   }
  }break;
  
  case Basic_Type_i1:
  {
   i1 v = *(i1*)value0;
   print(p, v);
  }break;
  case Basic_Type_i2:
  case Basic_Type_i3:
  case Basic_Type_i4:
  {
   i1 *v = (i1*)value0;
   i1 count = get_basic_type_info(type).size / 4;
   const i1 max_count = 4;
   
   if (wrapped) { print(p, "I"); print(p, count); print(p, "("); }
   for_i32(index,0,count)
   {
    if (index != 0) { print(p, ","); }
    print(p, v[index]);
   }
   if (wrapped) { print(p, ")"); }
  }break;
  
  invalid_default_case;
 }
}

inline void
print_nspaces(Printer &p, i1 n)
{
 for_i32(i,0,n) {
  print(p, " ");
 }
}

function void
write_basic_type(Printer &p, Basic_Type type, void *value0)
{
 switch(type)
 {
  //-Floats
  case Basic_Type_v1:
  case Basic_Type_v2:
  case Basic_Type_v3:
  case Basic_Type_v4:
  {
   v1 *values = cast(v1*)value0;
   i1 count = get_basic_type_size(type) / 4;
   if (count == 1) {
    print_float_trimmed(p, *values);
   } else {
    for_i32(index,0,count) {
     if (index != 0) { print(p, " "); }
     print_float_trimmed(p, values[index]);
    }
   }
  }break;
  
  //-Integers
  case Basic_Type_i1:
  case Basic_Type_i2:
  case Basic_Type_i3:
  case Basic_Type_i4:
  {
   i1 *v = (i1*)value0;
   i1 count = get_basic_type_size(type) / 4;
   
   for_i32(index,0,count) {
    if (index != 0) { print(p, " "); }
    print(p, v[index]);
   }
  }break;
  
  //-
  case Basic_Type_String: { print(p, *(String*)value0); }break;
  
  invalid_default_case;
 }
}

//~NOTE(kv): bucket array

#if 0
struct bucket_array_bucket
{
 bucket_array_bucket *next;
 bucket_array_bucket *prev;
 i32 count;
 i32 cap;
 
 u8  items[0];
};
// NOTE(kv): Probably wouldn't wanna use this type with simd anyway
static_assert((offsetof(bucket_array_bucket, items) & 7) == 0);

struct bucket_array_void
{
 Arena *arena;
 bucket_array_bucket *first_bucket;
 bucket_array_bucket *last_bucket;  // NOTE(kv): always exists
 bucket_array_bucket *free_bucket;
 i32 item_size;
 i32 last_bucket_cap;
 //-
 
 i32 item_count()
 {
  i32 result = 0;
  for(auto bucket = first_bucket;
      bucket;
      bucket = bucket->next)
  {
   result += bucket->count;
  }
  return result;
 }
 
 void expand(i32 delta)
 {// NOTE(kv): Expand
  i32 cur_cap = get_cap();
  if (new_count <= cur_cap) {
   // NOTE(kv): Lucky, we're within capacity
   last_bucket->count += delta;
   kv_assert(last_bucket->count <= last_bucket->cap);
  } else {
   // NOTE(kv): Need a new bucket
   delta -= last_bucket->cap - last_bucket->count;
   last_bucket->count = last_bucket->cap;
   {
    i32 new_bucket_cap = clamp_min(delta, 2*last_bucket->cap);
    u8 *new_bucket_memory = push_size(arena, (new_bucket_cap*item_size+
                                              sizeof(bucket_array_bucket)));
    auto new_bucket = (bucket_array_bucket *)new_bucket_memory;
    *new_bucket = {
     .prev  = last_bucket,
     .count = delta,
     .cap   = new_bucket_cap,
    };
    last_bucket->next = new_bucket;
   }
  }
 }
 
 void shrink(i32 size_to_pop)
 {// NOTE(kv): Pop and contract
  for_i32(auto bucket = last_bucket;
          bucket;
          bucket = bucket->prev)
  {
   if (bucket->count < size_to_pop) {
    // NOTE(kv): Free the whole bucket
    size_to_pop -= bucket->count;
    sll_stack_push(free_bucket, bucket);
   } else {
    // NOTE(kv): Stopping point
    bucket->count -= size_to_pop;
    size_to_pop = 0;  // @decor
    break;
   }
  }
  kv_assert(size_to_pop == 0);
 }
 
 inline void pop() { set_count(count-1); }
 
 void push(T const& item)
 {
  i32 new_count = get_count()+1;
  set_count(new_count);
  last_bucket->items[last_bucket->count] = item;
 }
 
 inline T& operator[](i32 index)
 {
  kv_assert(index >= 0 && index < count());
  i1 index_of_bucket = index / bucket_size;
  i1 index_in_bucket = index % bucket_size;
  auto *bucket = first_bucket;
  for_i1(i, 0, index_of_bucket) {
   bucket = bucket->next;
  }
  return bucket[index_in_bucket];
 }
};
//
template <class T>
function void
init(bucket_array<T> &array, Arena *arena, i1 bucket_size)
{
 // NOTE: Must have at least one bucket!
 array.arena = arena;
 array.bucket_size = bucket_size;
 array.first_bucket = array.last_bucket = &array.new_bucket();
}
#endif

#endif

//-

struct File_Name_Data {
 String name;
 String data;
};

#define introspect(...)
#define meta_tag(...)
#define meta_added(...)
#define meta_removed(...)
#define meta_unserialized
#define tagged_by(discriminator)
#define m_variant(tag)  //NOTE(kv) Use to tag union member with the variant it corresponds to

#define implies(a,b)  !a || b

//~EOF