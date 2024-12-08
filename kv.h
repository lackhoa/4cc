/*
  NOTE(kv): This is the most basic file, containing things EVERYONE needs.
*/

#include <stdarg.h>
#include <stddef.h>
#include <float.h>
#include <stdlib.h> // malloc, free
#include <stdio.h>  // printf, perror
#include <cstdint>
#include <string.h>
#include <math.h>

#include "kv_fundamental.h"

//~gb
#define GB_IMPLEMENTATION
#define GB_STATIC
#undef function
#include "gb.h"
#define function static
#undef GB_STATIC
#undef GB_IMPLEMENTATION
//~
#include "stb_sprintf.h"
//~

#define implies(a,b)  !(a) || b
#define cast_to_var(type, variable, value)  type variable = (type)value
#define cast_to(variable, value)            variable = (mytypeof(variable))(value)

#if COMPILER_LLVM
#    define PACK_BEGIN
#    define PACK_END    __attribute__((packed));  //NOTE: semicolon placement
#elif COMPILER_MSVC
#    define PACK_BEGIN  __pragma( pack(push, 1) )
#    define PACK_END    ; __pragma( pack(pop))
#endif

#ifdef KV_NO_FORCE_INLINE
#  define myinline inline
#else
#  if COMPILER_MSVC
#    define myinline __forceinline
#  else
#    define myinline __attribute__ ((__always_inline__))
#  endif
#endif

/* Intrinsics */

myinline void
block_zero(void *mem, u64 size)
{
#if AD_IS_DRIVER
 memset(mem, 0, size);
#else
 gb_zero_size(mem, size);
#endif
}

#if !AD_IS_DRIVER
myinline void
block_fill_ones(void *mem, u64 size)
{
 gb_memset(mem, 0xff, size);
}
#endif

myinline i32
absoslute(i32 in)
{
    return ((in >= 0) ? in : -in);
}

myinline v1
squared(f32 x)
{
    f32 result = x*x;
    return result;
}

myinline v1 
cubed(v1 value)
{
 return value*value*value;
}

myinline v1
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
myinline v1
roundv1(v1 Real32)
{
#if COMPILER_MSVC
 v1 Result = roundf(Real32);
#else
 v1 Result = __builtin_roundf(Real32);
#endif
 return(Result);
}

myinline v1
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

myinline i32
round_to_integer(v1 value)
{
 return i32(value+0.5f);
}

// TODO @Cleanup myinline all these functions
myinline v1
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

myinline v1
cycle01(v1 value)
{
 v1 result = value - floorv1(value);
 return result;
}

myinline v1
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

myinline v1
kv_cos(v1 angle)
{
#if COMPILER_MSVC
    v1 result = cosf(angle);
#else
    v1 result = __builtin_cosf(angle);
#endif
    return(result);
}

myinline v1
kv_atan2(v1 y, v1 x)
{
#if COMPILER_MSVC
 v1 result = atan2f(y, x);
#else
 v1 result = __builtin_atan2f(y, x);
#endif
 return(result);
}

myinline u64
find_least_significant_set_bit(u64 mask)
{
 u64 result = 0;
#if COMPILER_MSVC
 {
  _BitScanForward64((unsigned long *)&result, mask);
 }
#else
 {
  result = __builtin_ctzll(mask);
 }
#endif
 return result;
}
myinline u64
find_most_significant_set_bit(u64 mask)
{
 u64 result = 0;
 {
#if COMPILER_MSVC
  {
   _BitScanReverse64((unsigned long *)&result, mask);
  }
#else
  {
   result = 63-__builtin_clzll(mask);
  }
#endif
 }
 return result;
}

myinline v1
absolute(v1 x)
{
#if COMPILER_MSVC
 v1 result = (v1)fabs(x);
#else
 v1 result = (v1)__builtin_fabs(x);
#endif
 return result;
}
myinline i32
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

//-
#define filename_linum strlit(filename_line_number)

#if KV_INTERNAL
#define debug_location_defparams \
, \
const char *debug_file=__builtin_FILE(), \
i32 debug_line=__builtin_LINE()

#define debug_location_params \
, \
const char *debug_file, \
i32 debug_line

#define debug_location_args \
, debug_file, debug_line
#else
#define debug_location_defparams
#define debug_location_params
#define debug_location_args
#endif
//-

// source: https://groups.google.com/g/comp.std.c/c/d-6Mj5Lko_s
// NOTE: Doesn't work with MSVC, idk why man!
#define PP_ARG_N(_1,_2,_3,_4,_5,_6,_7,_8,N,...) N
#define PP_NARG(...) PP_ARG_N(__VA_ARGS__,8,7,6,5,4,3,2,1,0)


#define macro_min(a, b) ((a < b) ? a : b)
#define macro_max(a, b) ((a < b) ? b : a)
#define minimum  macro_min // @ Deprecated
#define maximum  macro_max // @ Deprecated

#define toggle_boolean(VAR)  VAR = !(VAR)

#define kv_function_declare(N) N##__return N(N##__params)
#define kv_function_pointer(N) N##__return (*N)(N##__params)

#define x_function_declare(N)  kv_function_declare(N);
#define x_function_pointer(N)  kv_function_pointer(N);

/* MARK: End of String */

inline v1 min(v1 a, v1 b){ return macro_min(a,b); }
inline v1 max(v1 a, v1 b){ return macro_max(a,b); }
inline v1 min(v1 a, v1 b, v1 c){ return macro_min(macro_min(a,b),c); }
inline v1 max(v1 a, v1 b, v1 c){ return macro_max(macro_max(a,b),c); }

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

#define ArrayAndCount(v)   v, alen(v)
#define swap_minmax(a,b) if (a > b) { macro_swap(a,b); }
// TODO: Deprecate these
#define v2_expand(v) v.x, v.y
#define v3_expand(v) v.x, v.y, v.z
#define v4_expand(v) v.x, v.y, v.z, v.w

#define expand2(v)   v[0], v[1]
#define expand3(v)   v[0], v[1], v[2]
#define expand4(v)   v[0], v[1], v[2], v[3]
//
#define repeat2(v)   v,v
#define repeat3(v)   v,v,v
#define repeat4(v)   v,v,v,v

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
#define macro_clamp01(var)          macro_clamp(0.f,var,1.f)
#define macro_clamp01i(var)         macro_clamp(0,var,1)

myinline v1
bilateral(v1 r)
{
    return (r * 2.0f) - 1.0f;
}

myinline v1
unilateral(v1 r)
{
    return (r * 0.5f) + 0.5f;
}

myinline v1
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

myinline v1
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

myinline v2
operator+(v2 u, v2 v)
{
    v2 result;
    result.x = u.x + v.x;
    result.y = u.y + v.y;
    return result;
}

myinline v1
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

myinline v2
operator-=(v2 &v, v2 u)
{
    v = v - u;
    return v;
}

myinline v2
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

myinline v2 operator*(v2 v, v1 c) { return c*v; }
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

myinline v2
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

myinline v2 perp(v2 v) { return v2{-v.y, v.x}; }

myinline v2 bilateral(v2 v)  { return v2{bilateral(v.x), bilateral(v.y)}; }

// ;v3

union v3{
 struct { v1 x, y, z; };
 struct { v1 r, g, b; };
 struct { v2 xy; v1 xy_z; };
 struct { v1 x_yz; v2 yz; };
 v1 e[3];
 v1 v[3];
 
 myinline v1 &operator[](i32 index) {return v[index];}
};


inline v3
absolute(v3 v){
 for_i32(index,0,3){ v[index] = absolute(v[index]); };
 return v;
}
myinline v3 V3(v2 xy)       { return v3{.xy=xy}; }
myinline v3 V3(v2 xy, v1 z) { return v3{.xy=xy, .xy_z=z}; }
myinline v3 yzx(v3 v) { return v3{v.y, v.z, v.x}; }
myinline v3 zxy(v3 v) { return v3{v.z, v.x, v.y}; }
inline v3 min(v3 a, v3 b){ return v3{min(a.x,b.x),min(a.y,b.y),min(a.z,b.z),}; }
inline v3 max(v3 a, v3 b){ return v3{max(a.x,b.x),max(a.y,b.y),max(a.z,b.z),}; }
inline v3 min(v3 a, v1 b){ return min(a,v3{repeat3(b)}); }
inline v3 max(v3 a, v1 b){ return max(a,v3{repeat3(b)}); }
inline v3
operator-(v3 u, v3 v){
 v3 result;
 result.x = u.x - v.x;
 result.y = u.y - v.y;
 result.z = u.z - v.z;
 return result;
}
inline b32
operator<(v3 u, v3 v){
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

myinline v3
operator-(v3 v)
{
    v3 result;
    result.x = -v.x;
    result.y = -v.y;
    result.z = -v.z;
    return result;
}
myinline v3
operator*(v1 c, v3 v)
{
 v.x *= c;
 v.y *= c;
 v.z *= c;
 return v;
}
myinline v3
operator*(v3 v, f32 c)
{
 v3 result = c*v;
 return result;
}

myinline v3 &
operator*=(v3 &v, f32 c)
{
    v = c * v;
    return v;
}

myinline v3
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
myinline v3 
operator*(v3 u, v3 v)
{
 return hadamard(u,v);
}
myinline v3 
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


myinline v3
V3(v1 x, v1 y, v1 z)
{
 return v3{x, y, z};
}
myinline v4
V4(v1 x, v1 y, v1 z, v1 w)
{
 return v4{x, y, z, w};
}
myinline v4
vert4(v1 x, v1 y, v1 z)
{
 return v4{x, y, z, 1.f};
}
myinline v4
V4_symmetric(v1 x, v1 y)
{
 return v4{x,y,y,x};
}
myinline v4
V4_symmetric(v2 xy)
{
 return V4_symmetric(xy.x,xy.y);
}
myinline v4
V4(v3 xyz, v1 w)
{
 v4 v;
 v.xyz = xyz;
 v.w   = w;
 return v;
}
myinline v4
cast_V4(v3 xyz)
{
 v4 v = {};
 v.xyz = xyz;
 return v;
}

myinline v1 &
v4::operator[](i32 index)
{
 return v[index];
}

myinline v3
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

myinline b32 
almost_equal(v1 a, v1 b, v1 epsilon=1e-6)
{
 return absolute(a - b) < epsilon;
}

myinline b32 
almost_equal(v3 a, v3 b, v1 epsilon=1e-6) {
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

myinline void
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
 ClampBot(radius.x,0);
 ClampBot(radius.y,0);
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
myinline i32
i2::operator[](i32 index)
{
 return e[index];
}

myinline v2
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
myinline i32
i3::operator[](i32 index)
{
 return e[index];
}

myinline i3
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
myinline i32&
i4::operator[](i32 index)
{
 return e[index];
}

/* todo: Old names */
#define kvXmalloc    kv_xmalloc
#define kvAssert     kv_assert
/* Old names > */


// Bitmap //////////////////////////////////////

struct Loaded_Bitmap 
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

//
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
struct mat4i
{
 union { mat4 forward; mat4 m; };
 union { mat4 inverse; mat4 inv; };
 myinline operator mat4&() { return forward; }  // @ClangSafe
};

myinline v1
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

myinline b32 
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

myinline v1 *
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

#if OS_WINDOWS
#define OS_SLASH '\\'
#else
#define OS_SLASH '/'
#endif

typedef void Void_Func(void);

typedef i32 Generated_Group;
enum{
 GeneratedGroup_Core,
 GeneratedGroup_Custom
};

#define api(...)

#define local_const  static const
#define global_const static const  // TODO(kv) Just say "global const" man! The language already helped you do it!

#define ArrayCount(a) i32((sizeof(a))/(sizeof(*a)))

// TODO(kv): temporary
#define ArrayCountSigned(a)  (isize)ArrayCount(a)

#define ArraySafe(a,i) ((a)[(i)%ArrayCount(a)])
#define ExpandArray(a) (a), (ArrayCount(a))
#define FixSize(s) struct{ u8 __size_fixer__[s]; }

#define PtrDif(a,b) ((u8*)(a) - (u8*)(b))

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

#define KB(x) ((x) * 1024LL)
#define MB(x) (KB(x) * 1024LL)
#define GB(x) (MB(x) * 1024LL)
#define TB(x) (GB(x) * 1024LL)

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
#define linum_defparam  i32 linum=__builtin_LINE()

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
#define sll_queue_push_multiple_(f,l,ff,ll) \
if(ll){if(f){l->next=ff;}else{f=ff;} l=ll;l->next=0;}
#define sll_queue_push_(f,l,n) sll_queue_push_multiple_(f,l,n,n)
#define sll_queue_pop_(f,l) if (f==l) { f=l=0; } else { f=f->next; }

#define sll_stack_push(head,node) (sll_stack_push_((head),(node)))
#define sll_stack_pop(h) (sll_stack_pop_((h)))
#define sll_queue_push_multiple(f,l,ff,ll) Stmnt( sll_queue_push_multiple_((f),(l),(ff),(ll)) )
// NOTE(kv): pretty sure "queue_push" means "push_last"
#define sll_queue_push(first,last,node) \
Stmnt( sll_queue_push_((first),(last),(node)) )
#define sll_queue_pop(f,l) Stmnt( sll_queue_pop_((f),(l)) )

#define zdll_push_back_NP_(first,last,node,next,prev) \
((first==0) ? (node->next=node->prev=0, first=last=node): \
(node->prev=last, node->next=0, last->next=node, last=node))

#define zdll_remove_back_NP_(f,l,next,prev) ((f==l)?(f=l=0):(l->prev->next=0,l=l->prev))
#define zdll_remove_NP_(f,l,n,next,prev)       \
((l==n)?(zdll_remove_back_NP_(f,l,next,prev))  \
:(f==n)?(zdll_remove_back_NP_(l,f,prev,next)) \
:       (dll_remove_NP_(n,n,next,prev)))

#define zdll_push_back(first,last,node) zdll_push_back_NP_((first),(last),(node),next,prev)
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

typedef String String8;  // @Deprecated

//NOTE(kv) nil-terminated string (cutnpaste)
struct Stringz{
 union{u8 *str, *data; };
 union{ u64 size, len, length, count; };
 myinline operator String&(){ return *(String*)this; }
};
inline char *
to_cstring(Stringz string){
 char *result = "";
 if(string.len){
  result = (char *)string.str;
 }
 return result;
}
inline String
string_skip(String str, u64 n){
 n = clamp_max(n, str.size);
 str.str  += n;;
 str.size -= n;
 return(str);
}
inline void
string_skip(String *str, u64 n){
 n = clamp_max(n, str->size);
 str->str  += n;;
 str->size -= n;
}

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
#else
#    define ASAN_ON 0
#endif

function u8 *
kv_malloc(usize size){
 u8 *result = (u8 *)malloc(size);
 return result;
}
function void
kv_free(void *pointer){
 if(pointer){
  free(pointer);
 }
}
xfunction usize system_page_size();
xfunction u8   *system_memory_reserve(usize size);
xfunction void  system_memory_free(void *base);
xfunction b32   system_memory_commit(void *base, usize size);
xfunction void  system_memory_decommit(void *base, usize size);
//~
//-

#define BODY     x += b-1; x -= x%b; return(x); 
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

myinline i32
block_compare(void *a, void *b, u64 size)
{
    return gb_memcompare(a, b, size);
}

myinline b32
block_match(void *a, void *b, u64 size)
{
    return (block_compare(a,b,size) == 0);
}

inline void
block_fill_u8(void *dst, u64 size, u8 val) {
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
inline v1 cosine(v1 v01) { return cosf(TAU32 * v01); }
inline v1 sine(v1 v01){ return sinf(TAU32 * v01); }
inline v1 arctan2(v1 y, v1 x) { return atan2f(y,x) / TAU32; }
inline v1 arcsin(v1 v01) { return asinf(v01) / TAU32; }
inline v1 arccos(v1 v01){ return acosf(v01) / TAU32; }

////////////////////////////////

myinline i2 I2(i32 x, i32 y) { return {x, y}; }
myinline i3 I3(i32 x, i32 y, i32 z) { return {x, y, z}; }
myinline i4 I4(i32 x, i32 y, i32 z, i32 w) { return {x, y, z, w}; }
myinline i4 I4() { return {}; }
myinline i4 I4(i32 x) { return i4{repeat4(x)}; }

myinline v2
V2(v1 x, v1 y)
{
 v2 v = {x, y};
 return(v);
}

myinline v2
cast_V2(i32 x, i32 y)
{
 v2 v = {(v1)x, (v1)y};
 return(v);
}
//

myinline i2
I2(i2 o)
{
    return(I2((i32)o.x, (i32)o.y));
}
myinline v3
V3(i3 o)
{
    return(V3((f32)o.x, (f32)o.y, (f32)o.z));
}

myinline Vec2_i32
operator+(Vec2_i32 a, Vec2_i32 b){
 a.x += b.x;
 a.y += b.y;
 return(a);
}
myinline i2&
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

function Vec2_f32
lerp(Vec2_f32 a, f32 t, Vec2_f32 b){
    return(a + (b-a)*t);
}

myinline v3
lerp(v3 a, v1 t, v3 b){
    return(a + (b-a)*t);
}

myinline v1
unlerp(v1 a, v1 x, v1 b)
{
 v1 r = 0.f;
 if (b != a)
 {
  r = (x - a)/(b - a);
 }
 return(r);
}

myinline v1
clamp01(v1 v)
{
 macro_clamp01(v);
 return v;
}

myinline v1
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

function u64
cstring_length(u8 *str){
 u64 length = 0;
 if (str) {
  for (;str[length] != 0; length += 1);
 }
 return(length);
}
//
myinline u64
cstring_length(char *str) {
 return cstring_length((u8 *)str);
}

global Stringz empty_string = {(u8*)"", 0};
myinline String  SCu8()                  { return empty_string; }
//TODO(kv) Character-string should point to a table
myinline String  SCu8(char &c)           { return {(u8*)&c, 1}; }
myinline String  SCu8(u8 *str, u64 size) { return {str, size}; }
myinline Stringz SCu8z(u8 *str, u64 size){ return {str, size}; }
myinline Stringz SCu8(u8 *str)           { return {(u8 *)str, cstring_length(str)}; }
myinline Stringz SCu8(char *str)         { return {(u8 *)str, cstring_length(str)}; }
myinline Stringz SCu8(const char *str)   { return {(u8 *)str, cstring_length((char *)str)}; }
//-
struct String_u8 {
 union {
  String string;
  struct {
   u8 *str;
   u64 size;
  };
 };
 u64 cap;
};
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

#define string_litexpr(s) SCchar((s), sizeof(s) - 1)
//NOTE(kv) sizeof takes into account the null terminator, for some reason.
#define strlit(s)    SCu8z((u8*)(s), (u64)(sizeof(s) - 1))
#define str8lit strlit
#define string_u16_litexpr(s) SCu16((u16*)(s), (u64)(sizeof(s)/2 - 1))

#define string_expand(s) (i32)(s).size, (char*)(s).str
#define strexpand string_expand
//-
#if OS_LINUX
#  include <immintrin.h>
#elif OS_MAC
#  include <immintrin.h>
#else
#  include <intrin.h>
#endif

#if COMPILER_MSVC
#define CompletePreviousReadsBeforeFutureReads _ReadBarrier()
#define CompletePreviousWritesBeforeFutureWrites _WriteBarrier()
//NOTE(kv) These functions by default will return the original value
//  Because what else do they return?
myinline u32
atomic_add_u32(u32 volatile *Value, u32 Addend)
{
 u32 Result = _InterlockedExchangeAdd((long volatile*)Value, (long)Addend);
 return(Result);
}
myinline u64
atomic_add_u64(u64 volatile *Value, u64 Addend)
{
 u64 Result = _InterlockedExchangeAdd64((__int64 volatile *)Value, Addend);
 return(Result);
}
myinline u32
atomic_compare_exchange_u32(u32 volatile *Value, u32 New, u32 Expected)
{
 u32 Result = _InterlockedCompareExchange((long volatile *)Value, New, Expected);
 return(Result);
}
myinline u64
atomic_exchange_u64(u64 volatile *Value, u64 New)
{
 u64 Result = _InterlockedExchange64((__int64 volatile *)Value, New);
 return(Result);
}
#endif//-MSVC
#if COMPILER_LLVM
#define CompletePreviousReadsBeforeFutureReads asm volatile("" ::: "memory")
#define CompletePreviousWritesBeforeFutureWrites asm volatile("" ::: "memory")
myinline u32
atomic_add_u32(u32 volatile *Value, u32 Addend)
{
 u32 Result = __sync_fetch_and_add(Value, Addend);
 return(Result);
}
myinline u64
atomic_add_u64(u64 volatile *Value, u64 Addend)
{
 u64 Result = __sync_fetch_and_add(Value, Addend);
 return(Result);
}
myinline u32
atomic_compare_exchange_u32(u32 volatile *Value, u32 New, u32 Expected)
{
 u32 Result = __sync_val_compare_and_swap(Value, Expected, New);
 return(Result);
}
myinline u64
atomic_exchange_u64(u64 volatile *Value, u64 New)
{
 u64 Result = __sync_lock_test_and_set(Value, New);
 return(Result);
}
#endif//-LLVM

struct Ticket_Mutex{
 volatile u64 serving;
 volatile u64 next_ticket;
};
myinline void
acquire_ticket_mutex(Ticket_Mutex *mutex)
{
 u64 ticket = atomic_add_u64(&mutex->next_ticket, 1);
 while(mutex->serving != ticket);
}
myinline void
release_ticket_mutex(Ticket_Mutex *mutex)
{
 CompletePreviousWritesBeforeFutureWrites;
 mutex->serving += 1;
}
//-
#if defined(KV_H_NO_GLOBAL_ARENA_CHUNK_STORE)
#  define KV_GLOBAL_ARENA_CHUNK_STORE 0
#endif
#if !defined(KV_GLOBAL_ARENA_CHUNK_STORE)
#  define KV_GLOBAL_ARENA_CHUNK_STORE 1
#endif

#if !KV_H_IS_METAPROGRAM  //NOTE(kv) You can't generate the code if you're the generator.
#  include "generated/kv_memory.gen.h"
#endif

//NOTE(kv) We'll just include the debug files,
//  since it defines the symbols that get compiled out for us.
#include "ad_debug_interface.h"

#include "kv_memory.h"

#include "ad_array.h"
//-
enum Base_Allocator_Type {
 Allocator_None,
 Allocator_Generic,
 Allocator_Malloc,
 Allocator_Arena,
};
typedef void *Allocator_Allocate_Signature(void *user_data, u64 size, u64 *size_out, DEBUG_File_Line file_line);
typedef void  Allocator_Free_Signature(void *user_data, void *ptr);

struct Base_Allocator_Generic{
 Allocator_Allocate_Signature *allocate;
 Allocator_Free_Signature    *free;
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
function void*
base_reserve__noop(void *user_data, u64 size, u64 *size_out, DEBUG_File_Line file_line)
{
 *size_out = 0;
 return(0);
}
function void
base_free__noop(void *user_data, void *ptr){}

global Base_Allocator malloc_base_allocator = {
 .type = Allocator_Malloc
};
function Base_Allocator
make_base_allocator_generic(Allocator_Allocate_Signature *func_allocate,
                            Allocator_Free_Signature     *func_free,
                            void *userdata)
{
 if(func_allocate    == 0){ func_allocate  = base_reserve__noop; }
 if(func_free       == 0) { func_free      = base_free__noop; }
 Base_Allocator result = {
  .type=Allocator_Generic,
  .generic={
   .allocate=func_allocate,
   .free    =func_free,
   .userdata=userdata,
  },
 };
 return(result);
}
function u8 *
base_allocate(Base_Allocator *allocator, u64 size,
              DEBUG_file_line_defparams)
{// @todo_leak_check
 u8 *result = {};
 
 switch(allocator->type)
 {
  case Allocator_Generic:
  {
   auto &a = allocator->generic;
   usize result_size;
   result = (u8 *)a.allocate(a.userdata, size, &result_size, file_line);
  }break;
  case Allocator_Arena:
  {// NOTE(kv): This is a cyclic dependency situation.
   result = arena_push(allocator->arena, size, 8, default_push_params, file_line);
  }break;
  case Allocator_Malloc:
  {
   result = kv_malloc(size);
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
   case Allocator_Generic:{
    auto &a = allocator->generic;
    a.free(a.userdata, ptr);
   }break;
   case Allocator_Arena:{
    //NOTE(kv) Not really "free", just a poison.
    ASAN_POISON_MEMORY_REGION(ptr, optional_size);
   }break;
   case Allocator_Malloc:{
    free(ptr);
   }break;
   invalid_default_case;
  } 
 }
}

// TODO(kv): Does anyone actually care about the returned size? And why do they care?
#define base_array(a,T,count) \
(T*)(base_allocate(a, sizeof(T)*(count)))
//~

//-
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

myinline v2 V2(v1 value) { return v2{repeat2(value)}; }
myinline v3 V3(v1 value) { return v3{repeat3(value)}; }
myinline v4 V4(v1 value) { return v4{repeat4(value)}; }
myinline v2 V2() { return v2{}; }
myinline v3 V3() { return v3{}; }
myinline v4 V4() { return v4{}; }

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
matmul(mat4 const*A, mat4 const*B) {
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
myinline mat4
matmul(mat4 const&A, mat4 const&B)
{
 return matmul(&A,&B);
}
//NOTE: This actually allows us to "pass by value"
// And clang actually does the right optimization in debug, which is refreshing.
myinline mat4
operator*(mat4 const&A, mat4 const&B)
{
 return matmul(&A,&B);
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

myinline v3
mat4vert_div(mat4 const&A, v3 v)
{
 v4 Av = A * V4(v,1.f);
 return Av.xyz / Av.w;
}
myinline v3
mat4vert(mat4 const&A, v3 v)
{
 v4 Av = A * V4(v, 1.f);
 return Av.xyz;
}
// IMPORTANT IMPORTANT IMPORTANT: I am a bad person! But there's no way around it!
myinline v3
operator*(mat4 const&A, v3 v)
{
return mat4vert(A,v);
}

myinline v3
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

myinline mat4
mat4_scales(v3 scales)
{
 return mat4_scales(v3_expand(scales));
}

myinline mat4
mat4_scale(v1 s)
{
 return mat4_scales(V3(s));
}

myinline mat4i
mat4i_scales(v3 s)
{
 mat4i result;
 result.forward = mat4_scales(s);
 result.inverse = mat4_scales(1.f/s.x, 1.f/s.y, 1.f/s.z);
 return result;
}

myinline mat4i
mat4i_scales(v1 sx, v1 sy, v1 sz)
{
 return mat4i_scales(V3(sx,sy,sz));
}

myinline mat4i
mat4i_scale(v1 s)
{
 mat4i result;
 result.forward = mat4_scale(s);
 result.inverse = mat4_scale(1.f/s);
 return result;
}

function mat4 
transpose(mat4 mat){
 for_i32(r,0,4) { 
  for_i32(c,0,r) {
   macro_swap(mat.e[r][c], mat.e[c][r]);
  }
 }
 return mat;
}
function mat3
transpose(mat3 mat){
 for_i32(r,0,3) {
  for_i32(c,0,r) {
   macro_swap(mat.e[r][c], mat.e[c][r]);
  }
 }
 return mat;
}
function mat4
mat4_columns(v3 x, v3 y, v3 z, v3 w){
 mat4 inverse;
 inverse.rows[0] = V4(x,0);
 inverse.rows[1] = V4(y,0);
 inverse.rows[2] = V4(z,0);
 inverse.rows[3] = V4(w,1);
 
 return transpose(inverse);
}
function mat4i
mat4i_columns(v3 x, v3 y, v3 z, v3 w){
 mat4i result;
 mat4 &inverse = result.inverse;
 inverse.rows[0] = V4(x,0);
 inverse.rows[1] = V4(y,0);
 inverse.rows[2] = V4(z,0);
 inverse.rows[3] = V4(w,1);
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

myinline v3
get_translation(mat4 const&mat)
{
 return get_column(mat, 3).xyz;
}

myinline mat3
mat3_scale(v1 s)
{
 mat3 result = {{
   s,0,0,
   0,s,0,
   0,0,s,
  }};
 return result;
}

myinline v3
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

myinline mat4i
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

myinline mat4i
mat4i_rotate(mat3 rot)
{
 mat4i result;
 result.forward = to_mat4(rot);
 result.inverse = to_mat4(transpose(rot));
 return result;
}

function mat4i
matmul(mat4i const*A, mat4i const*B)
{
 mat4i result;
 result.forward = matmul(&A->forward, &B->forward);
 result.inverse = matmul(&B->inverse, &A->inverse);
 return result;
}
myinline mat4i
matmul(mat4i const& A, mat4i const& B)
{
 return matmul(&A, &B);
}
//NOTE: Compose transformations
myinline mat4i
operator*(mat4i const& A, mat4i const& B)
{
 return matmul(&A, &B);
}

myinline v2 arm2(v1 turn)
{
 return v2{cosine(turn), sine(turn)};
}

myinline v3 V3x(v1 x) { v3 result = {}; result.x=x; return result; }
myinline v3 V3y(v1 y) { v3 result = {}; result.y=y; return result;  }
myinline v3 V3z(v1 z) { v3 result = {}; result.z=z; return result; }
myinline v3 setx(v3 v, v1 x) { v.x=x; return v; }
myinline v3 sety(v3 v, v1 y) { v.y=y; return v; }
myinline v3 setz(v3 v, v1 z) { v.z=z; return v; }
myinline v3 addx(v3 v, v1 x) { v.x+=x; return v; }
myinline v3 addy(v3 v, v1 y) { v.y+=y; return v; }
myinline v3 addz(v3 v, v1 z) { v.z+=z; return v; }
myinline v3 zeroX(v3 value) { value.x=0; return value; };

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

myinline v3 
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
#define add_in_block(variable, value) \
set_in_block(variable, variable+value)

myinline v1 i2f6 (i32 integer) { return v1(integer) / 6.f; }
myinline v1 i2f(i32 integer, v1 div) { return v1(integer) / div; }
myinline v4
i2f6(i4 vi)
{
 v4 result;
 for_i32(index,0,4) { result[index] = v1(vi[index]) / 6.f; }
 return result;
}


myinline v1
step(v1 edge, v1 x)
{
 return (x < edge) ? 0.f : 1.f;
}


myinline v3
step(v3 edge, v3 v)
{
 return V3(step(edge.x, v.x),
           step(edge.y, v.y),
           step(edge.z, v.z));
}

myinline i1
signof(i1 x)
{
 return (x == 0 ? 0 :
         x > 0  ? 1 :
         -1);
}

myinline v1
signof(v1 x)
{
 return (x == 0.f ? 0.f :
         x > 0.f  ? 1.f :
         -1.f);
}
myinline v3
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

myinline v3 
negateX(v3 vert){
 return V3(-vert.x, vert.y, vert.z);
}
//NOTE(kv) I think this is like multiple by a negateX matrix on the right.
inline mat4
negateX(mat4 mat){
 for_i32(row,0,4) { mat.e[row][0] *= -1.f; }
 return mat;
}
inline mat4i
negateX(mat4i mat){
 for_i32(row,0,4){ mat.forward.e[row][0] *= -1.f; }
 for_i32(col,0,4){ mat.inverse.e[0][col] *= -1.f; }
 return mat;
}
function mat4
remove_translation(mat4 result){
 result[0][3] = 0.f;
 result[1][3] = 0.f;
 result[2][3] = 0.f;
 return result;
}

//~NOTE: Array
// NOTE(kv): Can be zero-inited -> GOOD!
//TODO(kv) Please don't templatize so much code!
template<class T>
struct arrayof{
 i1 count;
 i1 cap;
 b32 fixed_size;
 T *items;
 Base_Allocator *allocator;
 
 //-
 
 inline T& get(i32 index){
  kv_assert(index>=0 and index<count);
  return items[index];
 }
 inline T& operator[](i32 index){ return get(index); }
 inline T &last() {
  kv_assert(count > 0);
  return items[count-1];
 }
 
 void set_cap_inner(i32 new_cap, DEBUG_File_Line file_line)
 {// NOTE(kv): Can only grow for now
  if(new_cap > cap)
  {
   kv_assert(!fixed_size);
   Base_Allocator *used_allocator = allocator;
   // NOTE(kv): get malloc allocator here to avoid the "stale pointer" problem.
   if(not used_allocator) used_allocator = &malloc_base_allocator;
   T *old_items = items;
   items = cast(T *)base_allocate(used_allocator, new_cap*sizeof(T), file_line);
   block_copy(items, old_items, count*sizeof(T));
   // NOTE(kv): to free 100% of the space we allocated with the arena allocator,
   //   We'd have to store the allocated size too. But I don't really care.
   base_free(used_allocator, old_items, cap*sizeof(T));
   cap = new_cap;
  }
 }
 void set_cap_min(i1 cap_min, DEBUG_file_line_defparams)
 {// TODO(kv): This grow logic is wonky: there are two cases:
  // 1. Natural growth: doubling
  // 2. User-dictated growth: just set the cap to the dictated value
  if (cap_min > cap) {
   i32 new_cap = (cap == 0);
   if (cap == 0) {
    new_cap = cap_min;
   } else {
    new_cap = macro_min(cap_min, 2*cap);
   }
   set_cap_inner(new_cap, file_line);
  }
 }
 void set_count(i32 new_count, DEBUG_file_line_defparams){
  kv_assert(new_count >= 0);
  set_cap_min(new_count, file_line);
  count = new_count;
  kv_assert(count <= cap);
 }
 
 inline void pop(){
  set_count(count-1);
 }
 inline T *push(DEBUG_file_line_defparams){
  set_count(count+1, file_line);
  return items + (count-1);
 }
 inline T *push_value(const T& value, DEBUG_file_line_defparams){
  T *item = push(file_line);
  *item = value;
  return item;
 }
 inline T& push_first(const T& new_item, DEBUG_file_line_defparams){
  set_count(count+1, file_line);
  for(i32 index=count-1;
      index >= 1;
      index--){
   items[index] = items[index-1];
  }
  items[0] = new_item;
  return items[0];
 }
 inline T *push_zero(DEBUG_file_line_defparams){
  T *result = push(file_line);
  *result = {};
  return result;
 }
 
 arrayof<T> copy(Arena *arena) {
  arrayof<T> result = *this;
  result.items = push_array(arena, T, count);
  umm size = count*sizeof(T);
  block_copy(result.items, items, size);
  return result;
 }
};
template<class T>
inline void
init_static(arrayof<T> &array, Arena *arena, i32 cap,
            Push_Params params=default_push_params){
 array = {
  .cap        = cap,
  .fixed_size = true,
  .items      = push_array(arena, T, cap, params),
 };
}
template<class T>
inline arrayof<T>
static_array(Arena *arena, i32 cap,
             Push_Params params=default_push_params){
 arrayof<T> array;
 init_static(array, arena, cap, params);
 return array;
}

template<class T>
inline void
init_static(arrayof<T> &array, T *backing_buffer, i32 cap){
 array = {
  .cap        = cap,
  .fixed_size = true,
  .items      = backing_buffer,
 };
}
template<class T>
inline arrayof<T>
static_array(T *backing_buffer, i32 cap){
 arrayof<T> array; init_static(array, backing_buffer, cap); return array;
}
template<class T>
inline void
init_dynamic(arrayof<T> &array, Base_Allocator *allocator, i1 initial_size=0){
 array = { .allocator = allocator };
 array.set_cap_min(initial_size);
}
template<class T>
inline arrayof<T>
dynamic_array(Base_Allocator *allocator, i1 initial_size=0){
 arrayof<T> array; init_dynamic(array, allocator, initial_size); return array;
}

function Base_Allocator *
push_arena_base_allocator(Arena *arena);
template<class T>
inline void
init_dynamic(arrayof<T> &array, Arena *arena, i1 initial_size=0){
 auto alloc = push_arena_base_allocator(arena);
 init_dynamic<T>(array, alloc, initial_size);
}
template<class T>
inline arrayof<T>
dynamic_array(Arena *arena, i1 initial_size=0){
 arrayof<T> array; init_dynamic(array, arena, initial_size); return array;
}
//~

#if !AD_IS_DRIVER

template<class T>
function T *
push_unique(arrayof<T> &array, T const&item)
{
 T *result = 0;
 b32 ok = true;
 for_i1(index,0,array.count){
  if(array.items[index] == item){
   ok = false;
   break;
  }
 }
 if(ok){
  result = array.push_value(item);
 }
 return result;
}
//~
struct Scratch_Block{
 Arena arena;
 //-
 //NOTE(kv) Deleting implicit copy constructor: This is why C++ is garbage!
 Scratch_Block(const Scratch_Block&) = delete;
 
 Scratch_Block();
 
 //@deprecated
 Scratch_Block(Arena *a1);
 Scratch_Block(struct App *app);
 Scratch_Block(struct App *app, Arena *a1);  //@deprecated
 
 ~Scratch_Block();
 
 myinline operator Arena*(){ return &this->arena; }
};
//-
myinline void
init_scratch_block(Scratch_Block *scratch){
 scratch->arena = make_arena();
}
//NOTE(kv) Hoist the constructor out so that I can share the code
//  between different constructors like, you know, a regular function.
myinline Scratch_Block::Scratch_Block(){
 init_scratch_block(this);
}
myinline Scratch_Block::~Scratch_Block() {
 arena_free(&this->arena);
}
//-
function Base_Allocator
make_arena_base_allocator(Arena *arena){
 Base_Allocator allocator = {
  .type  = Allocator_Arena,
  .arena = arena,
 };
 return allocator;
}
function Base_Allocator *
push_arena_base_allocator(Arena *arena){
 auto *alloc = push_struct(arena, Base_Allocator);
 *alloc = make_arena_base_allocator(arena);
 return alloc;
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
//~
function Stringz
push_stringz(Arena *arena, String src){
 Stringz string = {};
 string.str = push_array(arena, u8, src.size + 1);
 string.size = src.size;
 block_copy_count(string.str, src.str, src.size);
 string.str[string.size] = 0;
 return(string);
}
inline Stringz
to_stringz(Arena *a, String s){
 return push_stringz(a,s);
}
//
myinline char *
to_cstring(Arena *arena, String8 string){
 String8 result = push_stringz(arena, string);
 return (char *)result.str;
}

typedef i32 Scan_Direction;
enum{
 Scan_Backward = -1,
 Scan_Forward  =  1,
};

//~
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
function Stringz
push_stringfz(Arena *arena, char *format, ...)
{
 va_list args;
 va_start(args, format);
 String result0 = push_stringfv(arena, format, args, true);
 Stringz result = *(Stringz*)&result0;
 va_end(args);
 return(result);
}
//TODO(kv) Hackjob to concat strings together, goddamn it dude!
inline String
strcat(Arena *arena, char *a, String b){
 return push_stringf(arena, "%s%.*s", a, strexpand(b));
}
inline String
strcat(Arena *arena, String a, char *b){
 return push_stringf(arena, "%.*s%s", strexpand(a), b);
}

myinline String
to_string(Arena *arena, i32 value){
 return push_stringfz(arena, "%d", value);
}
myinline String
to_string(Arena *arena, u32 value){
 return push_stringfz(arena, "%u", value);
}

//~
function Stringz
pjoin(Arena *arena, String a, String b){
 return push_stringfz(arena, "%.*s%c%.*s", strexpand(a), OS_SLASH, strexpand(b));
}
function Stringz
pjoin(Arena *arena, String a, String b, String c){
 return push_stringfz(arena, "%.*s%c%.*s%c%.*s",
                      strexpand(a), OS_SLASH,
                      strexpand(b), OS_SLASH,
                      strexpand(c));
}
//~
inline b32
file_exists(Stringz file){
 return gb_file_exists(to_cstring(file));
}
function b32
remove_file(Stringz filename){
 b32 result = true;
 if(file_exists(filename)){
  result = gb_file_remove(to_cstring(filename));
 }
 return result;
}
function b32
move_file(Stringz from, Stringz to){
 remove_file(to);
 b32 result = gb_file_move(to_cstring(from), to_cstring(to));
 return result;
}
inline b32 
copy_file(Stringz from, Stringz to, b32 fail_if_exists){
 return gb_file_copy(to_cstring(from), to_cstring(to), fail_if_exists);
}
#if OS_WINDOWS
function b32
create_directory(Stringz path){
 b32 ok = 1;
 if(!CreateDirectoryA(to_cstring(path),0)){
  DWORD error = GetLastError();
  if(error != ERROR_ALREADY_EXISTS){
   ok = 0;
  }
 }
 return ok;
}
function b32
create_directory(String path){
 Scratch_Block scratch;
 Stringz pathz = to_stringz(scratch, path);
 return create_directory(pathz);
}
#endif
inline FILE *
open_file(Stringz name, char *mode){
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
   create_directory(path_dir(name));
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
path_is_directory(Stringz path){
 DWORD attr = GetFileAttributes(to_cstring(path));
 return (attr & FILE_ATTRIBUTE_DIRECTORY);
}
#endif

function Stringz
read_entire_file_handle(Arena *arena, FILE *file){
 Stringz result = {};
 if(file){
  fseek(file, 0, SEEK_END);
  u64 size = ftell(file);
  char *mem = push_array(arena, char, size+1);
  fseek(file, 0, SEEK_SET);
  fread(mem, 1, (size_t)size, file);
  result = {(u8*)mem,size};
  mem[size] = 0;// NOTE: null-termination
 }
 return(result);
}
function Stringz
read_entire_file(Arena *arena, Stringz filename){
 Stringz result = {};
 FILE *file = open_file(filename, "rb");
 result = read_entire_file_handle(arena, file);
 close_file(file);
 return(result);
}

//~ Printer
typedef i32 Print_Function(void *userdata, char *format, va_list args);
enum Printer_Type{
 Printer_Type_None,
 Printer_Type_Buffer,
 Printer_Type_FILE,
 Printer_Type_Generic,
};
struct Printer{
 b32 error;
 Printer_Type type;
 b32 print_separator_before_anything_else;
 String separator;
 usize byte_pos;
 union{
  struct{
   u8  *base;
   usize cap;
  };
  FILE *FILE;
  struct{
   void *userdata;
   Print_Function *print_function;
  };
 };
};
//-NOTE(kv) Brought to you by Meta-Programming needs
inline void
begin_separator(Printer &p, char *separator){
 p.print_separator_before_anything_else = false;
 p.separator = SCu8(separator);
}
inline void
end_separator(Printer &p){
 p.print_separator_before_anything_else = false;
 p.separator = {};
}
//NOTE(kv) The separator signal
function void
separator(Printer &p){
 p.print_separator_before_anything_else = true;
}
#define separator_block(printer, separator) \
defer_block(begin_separator(printer, separator), \
end_separator(printer))
//-
inline Printer
make_printer_buffer(u8 *buffer, usize cap){
 Printer result = {
  .type = Printer_Type_Buffer,
  .base = buffer,
  .cap  = cap,
 };
 return result;
}
inline Printer
make_printer_buffer(Arena *arena, usize cap){
 u8 *buffer = arena_push(arena, cap, 1);
 Printer result = make_printer_buffer(buffer, cap);
 return result;
}
inline Printer
make_printer_file(FILE *file){
 Printer result = {
  .type = Printer_Type_FILE,
  .FILE = file,
 };
 return result;
}
//-
function Stringz
printer_get_string(Printer &p){
 if(p.type==Printer_Type_Buffer){
  Stringz string = {
   .str  = p.base,
   .size = (u64)p.byte_pos,
  };
  p.base[p.byte_pos++] = 0;  //NOTE nil-termination
  return string;
 }else{
  invalid_code_path;
  return {};
 }
}
inline void
printer_delete(Printer &p){
 kv_assert(p.type == Printer_Type_Buffer);
 kv_assert(p.byte_pos > 0);
 p.byte_pos--;
}
//-
#define print_parens_block(printer) \
defer_block(print(printer, '('), print(printer, ')'))

#define print_brace_block(printer) \
defer_block(print(printer, '{'), print(printer, '}'))

//-NOTE Base print function overloads
function void
print_format2v(Printer &p, char *format, va_list args){
 i32 written = 0;
 switch(p.type){
  case Printer_Type_Buffer:{
   usize remaining = p.cap-p.byte_pos;
   written = vsnprintf((char *)(p.base+p.byte_pos), remaining, format, args);
   kv_assert(usize(written) < remaining);
  }break;
  case Printer_Type_FILE:{
   written = vfprintf(p.FILE, format, args);
  }break;
  case Printer_Type_Generic:{
   written = p.print_function(p.userdata, format, args);
  }break;
  invalid_default_case;
 }
 p.byte_pos += written;
}
//NOTE(kv) omg totally unnecessary
function void
print_format2(Printer &p, char *format, ...){
 va_list args;
 va_start(args, format);
 print_format2v(p,format,args);
 va_end(args);
}
function void
print_format(Printer &p, char *format, ...){
 va_list args;
 va_start(args, format);
 if(p.print_separator_before_anything_else){
  p.print_separator_before_anything_else = false;
  print_format2(p, "%.*s", strexpand(p.separator));
 }
 print_format2v(p, format, args);
 va_end(args);
}
//~Printing different types
myinline void print(Printer &p, const char *cstring) { print_format(p, "%s", cstring); }
myinline void print(Printer &p, String string){
 print_format(p, "%.*s", strexpand(string));
}
myinline void print(Printer &p, char c)  { print_format(p, "%c", c); }
myinline void print(Printer &p, u8   c)  { print_format(p, "%c", c); }
myinline void print(Printer &p, i32 d)   { print_format(p, "%d", d); }
myinline void print(Printer &p, u32 u)   { print_format(p, "%u", u); }
myinline void print(Printer &p, i64 ld)  { print_format(p, "%zd", ld); }
myinline void print(Printer &p, u64 lu)  { print_format(p, "%zu", lu); }
//-
// NOTE(kv): This is an absolutely ridiculous hack
template <class T>
inline Printer &
operator<<(Printer &p, T object){
 print(p, object);
 return p;
}
template <class T>
inline Printer &
operator<(Printer &p, T object){
 print(p, object);
 return p;
}
//-
struct Writer{
 b32 ok;
 FILE *file;
};
function Writer
make_writer(FILE *file){
 Writer result = {};
 result.ok   = file != 0;
 result.file = file;
 return result;
}
function void
write_size(Writer *writer, void *data, usize size){
 if(writer->ok){
  usize result = fwrite(data, size, 1, writer->file);
  writer->ok = result != 0;
 }
}
myinline void
write_size(Writer *writer, const char *data, usize size)
{//NOTE fuck you clang
 write_size(writer, (void *)data, size);
}

#define write_lvalue(writer, lvalue) \
write_size(writer, &lvalue, sizeof(lvalue))

//-
struct Binary_Reader
{
 b32 ok;
 u32 read_version;
 u8 *base;
 u8 *pos;
 u8 *end_pos;
};
function Binary_Reader
make_binary_reader(u8 *base, usize size)
{
 Binary_Reader r = {};
 r.ok      = true;
 r.base    = base;
 r.pos     = r.base;
 r.end_pos = r.base + size;
 return r;
}
function void
read_binary_size(Binary_Reader *r, usize size, void *dst)
{
 if(r->end_pos - r->pos < isize(size)){
  //NOTE not enough data left
  r->ok = false;
 }
 if(r->ok){
  block_copy(dst, r->pos, size);
  r->pos += size;
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
struct File_Name_Data{
 String name;
 String data;
};

#if 0
#define meta_table
#define gen_file
#define gen_for
#endif
//~
#undef KV_H_IS_METAPROGRAM
//~EOF