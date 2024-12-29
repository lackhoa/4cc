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

#if !defined(KV_IMPLEMENTATION)
#  define KV_IMPLEMENTATION 1
#endif

#if !defined(AD_HAS_OS_CODE)
#  define AD_HAS_OS_CODE 1
#endif

//TODO(kv) Deprecate this flag, just use precompiled header bro!
#if !defined(AD_HAS_INTRINSIC)
#  define AD_HAS_INTRINSIC 1
#endif

#include "kv_fundamental.h"

//~
#undef function

#if AD_HAS_OS_CODE
# if OS_WINDOWS
#  define NOMINMAX            1
#		define WIN32_LEAN_AND_MEAN 1
#		define WIN32_MEAN_AND_LEAN 1
#		define VC_EXTRALEAN        1
#		include <windows.h>
#		undef NOMINMAX
#		undef WIN32_LEAN_AND_MEAN
#		undef WIN32_MEAN_AND_LEAN
#		undef VC_EXTRALEAN

#	include <malloc.h> // NOTE(bill): _aligned_*()
# else
#		include <dlfcn.h>
#		include <errno.h>
#		include <fcntl.h>
#		include <pthread.h>
#		ifndef _IOSC11_SOURCE
#		define _IOSC11_SOURCE
#		endif
#		include <stdlib.h> // NOTE(bill): malloc on linux
#		include <sys/mman.h>
#		if !defined(GB_SYSTEM_OSX)
#			include <sys/sendfile.h>
#		endif
#		include <sys/stat.h>
#		include <sys/time.h>
#  include <sys/types.h>
#  include <time.h>
#  include <unistd.h>
# endif
#endif

#define function static

//~

#if AD_HAS_INTRINSIC
#  if OS_LINUX
#    include <immintrin.h>
#  elif OS_MAC
#    include <immintrin.h>
#  elif OS_WINDOWS
#    include <intrin.h>
#  endif
#endif

//~gb
#define GB_IMPLEMENTATION
#define GB_STATIC
#include "gb.h"
#undef GB_STATIC
#undef GB_IMPLEMENTATION
//~
#if !defined(AD_STB_SPRINTF_IMPLEMENTATION) || AD_STB_SPRINTF_IMPLEMENTATION
#  define STB_SPRINTF_IMPLEMENTATION
#endif
#if COMPILER_MSVC
#  pragma warning(push)
#  pragma warning(disable:4244)
#endif
#include "stb_sprintf.h"
#if COMPILER_MSVC
#  pragma warning(pop)
#endif
//~

#define implies(a,b)  (!(a) || (b))
#define cast_to_var(type, variable, value)  type variable = (type)value
#define cast_to(variable, value)            variable = (mytypeof(variable))(value)
#define cast_assign(variable, value)        variable = (mytypeof(variable))(value)

#if COMPILER_LLVM
#    define PACK_BEGIN
#    define PACK_END    __attribute__((packed));  //NOTE: semicolon placement
#elif COMPILER_MSVC
#    define PACK_BEGIN  __pragma( pack(push, 1) )
#    define PACK_END    ; __pragma( pack(pop))
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

/* Intrinsics */

myinline void
block_zero(void *mem, u64 size)
{
 gb_zero_size(mem, size);
}
#define zero_struct(POINTER)     block_zero(POINTER, sizeof(*POINTER))

myinline void
block_fill_ones(void *mem, u64 size)
{
 gb_memset(mem, 0xff, size);
}

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

myinline v1
cycle01_positive(v1 value)
{
 v1 result = value - v1(i32(value));
 return result;
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


#define todo_test_me        fail_in_debug
#define todo_testme         fail_in_debug
#define todo_untested       fail_in_debug
#define kv_debug_trap       fail_in_debug
#define todo_incomplete     fail_in_debug
#define todo_implement      fail_in_debug
#define todo_error_report



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
#define minimum  macro_min
#define maximum  macro_max

#define toggle_boolean(VAR)  VAR = !(VAR)
#define ToggleBoolean(VAR)  VAR = !(VAR)

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
#define in_range_exclusive(value, bot, top) ((bot) <= (value) && (value) <  (top))
#define in_range_inclusive(value, bot, top) ((bot) <= (value) && (value) <= (top))

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

/* todo: Old names */
#define kvXmalloc    kv_xmalloc
#define kvAssert     kv_assert
/* Old names > */

//////////////////////////////////////////////////

#if !defined(KV_INTERNAL)
#  define KV_INTERNAL 0
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
#  define OS_SLASH '\\'
#else
#  define OS_SLASH '/'
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

#if KV_INTERNAL
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


typedef u32 ARGB_Color;
typedef u32 argb;

////////////////////////////////

typedef String String8;  // @Deprecated

//NOTE(kv) nil-terminated string (cutnpaste)
struct Stringz : String
{
};
myinline char *
to_cstring(Stringz string){
 char *result = "";  //NOTE(kv) Allow zero-init
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

myinline void
block_copy(void *dst, const void *src, u64 size)
{
 gb_memmove(dst, src, size);
}
myinline void
block_copy_non_overlap(void *dst, const void *src, u64 size)
{
 gb_memcopy(dst, src, size);
}

////////////////////////////////
#define make_data_struct(s) make_data((s), sizeof(*(s)))

#define data_initr(m,s) {(u8*)(m), (s)}
#define data_initr_struct(s) {(u8*)(s), sizeof(*(s))}
#define data_initr_array(a) {(u8*)(a), sizeof(a)}
#define data_initr_string(s) {(u8*)(s), sizeof(s) - 1}

////////////////////////////////

myinline void
block_zero(String8 data) {
 block_zero(data.str, data.size);
}
myinline void
block_fill_ones(String8 data){
 block_fill_ones(data.str, data.size);
}

function i32
block_compare(void *s1, void *s2, u64 size)
{
	// TODO(bill): Heavily optimize
	u8 const *s1p8 = cast(u8 const *)s1;
	u8 const *s2p8 = cast(u8 const *)s2;
 
	if (s1 == NULL || s2 == NULL) {
		return 0;
	}
 
	while (size--) {
		if (*s1p8 != *s2p8) {
			return (*s1p8 - *s2p8);
		}
		s1p8++, s2p8++;
	}
	return 0;
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




////////////////////////////////


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

#if COMPILER_MSVC
#define CompletePastReadsBeforeFutureReads   _ReadBarrier()
#define CompletePastWritesBeforeFutureWrites _WriteBarrier()
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
#endif
//
#if COMPILER_LLVM
#define CompletePastReadsBeforeFutureReads   asm volatile("" ::: "memory")
#define CompletePastWritesBeforeFutureWrites asm volatile("" ::: "memory")
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
#endif
//
myinline i32
atomic_compare_exchange_i32(i32 volatile *Value, i32 New, i32 Expected)
{//NOTE(kv) Add this variant because casting all the parameters is annoying.
 u32 result = atomic_compare_exchange_u32((u32 volatile *)Value, (u32)New, (u32)Expected);
 return (i32)result;
}
myinline i32
atomic_add_i32(i32 volatile *Value, i32 Addend)
{
 u32 result = atomic_add_u32((u32 volatile *)Value, u32(Addend));
 return i32(result);
}

struct Ticket_Mutex
{
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
 CompletePastWritesBeforeFutureWrites;
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
#  include "kv_memory.gen.h"
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
  InvalidDefaultCase;
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
   InvalidDefaultCase;
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

//~
myinline b32
is_file_slash(char c)
{
 return((c == '/') or (c == '\\'));
}
myinline b32
is_file_slash(u8 c)
{
 return((c == '/') or (c == '\\'));
}

#if !AD_IS_DRIVER
#    include "kv_extra.h"
#endif

//-

#define SetInBlock(var, value) \
auto line_unique_var = var; var = (value); defer(var = line_unique_var;)

#define scale_in_block(var, mult) \
SetInBlock(var, var*mult) \

#define add_in_block(var, addend) \
SetInBlock(var, var+addend)

//~NOTE: Array
template<class T>
struct Static_Array2
{
 T  *items;
 i32 count;
 
 myinline T &operator[](i32 index){ return items[index]; }
};

#define sarray(T) Static_Array2<T>

template<class T>
function void
init(sarray(T) &array, Arena *arena, i32 count)
{
 array.count = count;
 array.items = push_array(arena, T, count);
}

// NOTE(kv) Because I hate the terrible template syntax
#define darray(T) Dynamic_Array<T>

// NOTE(kv): Dynamic_Array can be zero-inited -> GOOD!
//TODO(kv) Please don't templatize so much code!
template<class T>
struct Dynamic_Array : Static_Array2<T>
{
 i32 cap;
 // NOTE(kv) The fixed_sized capability is needed to make crappy stacks.
 //  I've thought about moving it, but whatever man...
 //  this data structure is supposed to be crappy anyway.
 b32 fixed_size;
 Arena *arena;
 //-
 myinline T& get(i32 index) {
  kv_assert(index < this->count);
  return this->items[index];
 }
 
 void set_cap_inner(i32 new_cap, DEBUG_File_Line file_line)
 {// NOTE(kv): Can only grow for now
  if(new_cap > cap)
  {
   kv_assert(not fixed_size);
   T *old_items = this->items;
   
   Arena *used_arena = arena;
   if(not arena){ used_arena = &thread_permanent_arena; }
   this->items = push_array(used_arena, T, new_cap, default_push_params, file_line);
   
   block_copy(this->items, old_items, this->count*sizeof(T));
   cap = new_cap;
  }
 }
 void set_cap_min(i32 cap_min, DEBUG_file_line_defparams)
 {// NOTE(kv): Growth logic:
  // 1. Natural growth: doubling
  // 2. User-dictated growth: just set the cap to the dictated value
  if(cap_min > cap)
  {
   i32 new_cap = cap_min;
   if(cap != 0){
    new_cap = macro_min(cap_min, 2*cap);
   }
   set_cap_inner(new_cap, file_line);
  }
 }
 void set_count(i32 new_count, DEBUG_file_line_defparams)
 {
  set_cap_min(new_count, file_line);
  this->count = new_count;
  kv_assert(this->count <= cap);
 }
 
 inline void pop()
 {
  kv_assert(this->count > 0);
  set_count(this->count - 1);
 }
 inline T *push_nozero(DEBUG_file_line_defparams)
 {
  set_count(this->count+1, file_line);
  T *result = this->items + (this->count-1);
  return result;
 }
 inline T *push(DEBUG_file_line_defparams)
 {
  T *result = this->push_nozero(file_line);
  *result = {};
  return result;
 }
 
 darray(T) copy(Arena *to_arena) {
  darray(T) result = *this;
  result.items = push_array(to_arena, T, this->count);
  umm size = this->count*sizeof(T);
  block_copy(result.items, this->items, size);
  return result;
 }
};

template<class T>
myinline T &
get_last(darray(T) &array)
{
 kv_assert(array.count > 0);
 return array[array.count-1];
}

template<class T>
function void
set_count(darray(T) *array, i32 new_count, DEBUG_file_line_defparams)
{
 array->set_count(new_count, file_line);
}

// NOTE(kv) Usually I don't like passing by reference,
//  but maybe we can make exceptions for init functions?
template<class T>
function void
init_static(darray(T) &array, Arena *arena, i32 cap,
            Push_Params params=default_push_params)
{
 array = {};
 array.cap        = cap;
 array.fixed_size = true;
 array.items      = push_array(arena, T, cap, params);
}
template<class T>
function void
init_static(darray(T) &array, T *backing_buffer, i32 cap)
{
 array = {};
 array.cap        = cap;
 array.fixed_size = true;
 array.items      = backing_buffer;
}
template<class T>
function void
init_dynamic(darray(T) &array, Arena *arena, i32 initial_size=0)
{
 array = {};
 array.arena = arena;
 array.set_cap_min(initial_size);
}

template<class T>
myinline T *
push(darray(T) *array, DEBUG_file_line_defparams)
{
 return array->push(file_line);
}
template<class T>
function T *
push(darray(T) *array, T const&value, DEBUG_file_line_defparams)
{//note(kv) push value
 T *item = push(array, file_line);
 *item = value;
 return item;
}
template<class T>
function void
pop(darray(T) *array)
{
 kv_assert(array->count > 0);
 array->count--;
}
template<class T>
function void
insert_at(darray(T) *array, T const&new_item, i32 insert_index,
          DEBUG_file_line_defparams)
{
 i32 new_count = array->count+1;
 set_count(array, new_count, file_line);
 kv_assert(insert_index < new_count);
 for(i32 index = new_count-1;
     index >= insert_index+1;
     index--)
 {
  array->items[index] = array->items[index-1];
 }
 array->items[insert_index] = new_item;
}
template<class T>
function void
push_first(darray(T) *array, T const&new_item, DEBUG_file_line_defparams)
{
 insert_at(array, new_item, 0, file_line);
}

//NOTE(kv) stroustrup!
function void
push(darray(String) *array, Stringz string)
{
 push(array, String(string));
}
//~
function Base_Allocator *
push_arena_base_allocator(Arena *arena);

template<class T>
function T *
push_unique(darray(T) *array, T const&item)
{
 T *result = 0;
 b32 ok = true;
 for_i32(index,0,array->count){
  if(array->items[index] == item){
   ok = false;
   break;
  }
 }
 if(ok){
  result = push(array, item);
 }
 return result;
}
//~
// TODO(kv) Rename this to "Scratch_Auto"
struct Scratch_Block
{
 Arena arena;
 //-
 //NOTE(kv) Deleting implicit copy constructor: This is why C++ is garbage!
 Scratch_Block(const Scratch_Block&) = delete;
 
 Scratch_Block();
 //NOTE(kv) Reserved for dual arena scheme.
 Scratch_Block(Arena *conflict);
 
 //@deprecated
 Scratch_Block(struct App *app);
 Scratch_Block(struct App *app, Arena *a1);  //@deprecated
 
 ~Scratch_Block();
 
 myinline operator Arena*(){ return &this->arena; }
};

// NOTE(kv) Reserved for when I add Ryan's dual arena scheme.
// The plan is to have a double-buffer situation.
typedef Scratch_Block Scratch_Scope;

//NOTE(kv) Hoist the constructor out so that I can share the code
//  between different constructors like, you know, a regular function.
myinline void
init_scratch_block(Scratch_Block *scratch){
 scratch->arena = make_arena();
}

myinline Scratch_Block::Scratch_Block(){
 init_scratch_block(this);
}
myinline Scratch_Block::Scratch_Block(Arena *conflict){
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
to_cstring(Arena *arena, String8 string)
{
 String8 result = push_stringz(arena, string);
 return (char *)result.str;
}

typedef i32 Scan_Direction;
enum{
 Scan_Backward = -1,
 Scan_Forward  =  1,
};

//~

function Stringz
push_stringfv(Arena *arena, char *format, va_list args)
{
 va_list args2;
 va_copy(args2, args);
 
 usize pushed_size = usize(stbsp_vsnprintf(0, 0, format, args));
 // NOTE(kv): vsnprintf is always terminated, and it won't print unless you reserve the buffer for nil-termination
 pushed_size += 1;
 
 Stringz result;
 result.str  = push_size(arena, pushed_size);
 result.size = pushed_size - 1;
 stbsp_vsnprintf((char*)result.str, i32(pushed_size), format, args2);
 
 return(result);
}
function Stringz
push_stringf(Arena *arena, char *format, ...)
{
 va_list args;
 va_start(args, format);
 Stringz result = push_stringfv(arena, format, args);
 va_end(args);
 return(result);
}
//
//TODO(kv) Hackjob to concat strings together!
myinline Stringz
strcat(Arena *arena, String a, String b)
{
 Stringz result = {};
 result.str = push_size(arena, a.count + b.count + 1);
 result.count = a.count + b.count;
 block_copy(result.str,           a.str, a.count);
 block_copy(result.str + a.count, b.str, b.count);
 *(result.str + result.count) = 0;
 return result;
}
myinline Stringz
strcat(Arena *arena, char *a, String b){
 return strcat(arena, SCu8(a), b);
}
myinline Stringz
strcat(Arena *arena, String a, char *b){
 return strcat(arena, a, SCu8(b));
}

myinline Stringz
to_string(Arena *arena, i32 value){
 return push_stringf(arena, "%d", value);
}
myinline Stringz
to_string(Arena *arena, u32 value){
 return push_stringf(arena, "%u", value);
}

//~
#if AD_HAS_OS_CODE
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
remove_file(Stringz path){
 b32 result = true;
 if(file_exists(path)){
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
copy_file(Stringz from, Stringz to, b32 fail_if_exists)
{
 return gb_file_copy(to_cstring(from), to_cstring(to), fail_if_exists);
}
#if OS_WINDOWS
function b32
mkdir_p(Stringz path){
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
mkdir_p(String path){
 Scratch_Block scratch;
 Stringz pathz = to_stringz(scratch, path);
 return mkdir_p(pathz);
}
#endif

myinline FILE *
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
 fseek(file, 0, SEEK_END);
 u64 size = ftell(file);
 fseek(file, 0, SEEK_SET);
 
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
#endif

//~ Printer
typedef i32 Print_Function(void *userdata, char *format, va_list args);

enum Printer_Type{
 Printer_Type_None,
 Printer_Type_Buffer,
 Printer_Type_FILE,
 Printer_Type_Generic,
};
struct Printer
{
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
myinline void
begin_separator(Printer &p, char *separator){
 p.print_separator_before_anything_else = false;
 p.separator = SCu8(separator);
}
myinline void
end_separator(Printer &p){
 p.print_separator_before_anything_else = false;
 p.separator = {};
}
//NOTE(kv) The separator signal
myinline void
separator(Printer &p){
 p.print_separator_before_anything_else = true;
}
#define separator_block(printer, separator) \
defer_block(begin_separator(printer, separator), \
end_separator(printer))
//-
myinline Printer
make_printer_buffer(u8 *buffer, usize cap){
 Printer result = {
  .type = Printer_Type_Buffer,
  .base = buffer,
  .cap  = cap,
 };
 return result;
}
myinline Printer
make_printer_buffer(Arena *arena, usize cap){
 u8 *buffer = arena_push(arena, cap, 1);
 Printer result = make_printer_buffer(buffer, cap);
 return result;
}
myinline Printer
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
 Stringz result = {};
 if(p.type==Printer_Type_Buffer){
  result = { p.base, (u64)p.byte_pos };
  p.base[p.byte_pos++] = 0;  //NOTE nil-termination
 }else{
  invalid_code_path;
 }
 return result;
}
myinline void
printer_delete(Printer &p){
 kv_assert(p.type == Printer_Type_Buffer);
 kv_assert(p.byte_pos > 0);
 p.byte_pos--;
}
//-
#define PrintParens(printer) \
defer_block(print(printer, '('), print(printer, ')'))

#define PrintBraces(printer) \
defer_block(print(printer, '{'), print(printer, '}'))

function usize
my_vfprintf(FILE *file, char *format, va_list args)
{
 Scratch_Scope scratch;
 String string = push_stringfv(scratch, format, args);
 
 int result2 = fputs((char *)string.str, file);
 
 usize result = string.count;
 if(result2 < 0){
  result = 0;
 }
 return result;
}
function void
myprintf(char *format, ...)
{
 va_list args;
 va_start(args, format);
 my_vfprintf(stdout, format, args);
 va_end(args);
}
//-NOTE Base print function overloads
function void
print_format2v(Printer &p, char *format, va_list args)
{
 usize written = 0;
 switch(p.type){
  case Printer_Type_Buffer:{
   usize remaining = p.cap-p.byte_pos;
   //TODO(kv) cast
   written = (usize)stbsp_vsnprintf((char *)(p.base+p.byte_pos), i32(remaining), format, args);
   kv_assert(written < remaining);
  }break;
  case Printer_Type_FILE:{
   written = my_vfprintf(p.FILE, format, args);
  }break;
  case Printer_Type_Generic:{
   written = p.print_function(p.userdata, format, args);
  }break;
  InvalidDefaultCase;
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
print_format(Printer &p, char *format, ...)
{
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

//-
struct File_Name_Data{
 String name;
 String data;
};
//-
union v2 {
 struct  { v1 x; v1 y; };
 v1 e[2];
 v1 v[2];
 
 myinline v1 &operator[](i32 index) {return v[index];}
};
union v3{
 struct { v1 x, y, z; };
 struct { v1 r, g, b; };
 struct { v2 xy; v1 xy_z; };
 struct { v1 x_yz; v2 yz; };
 v1 e[3];
 v1 v[3];
 
 myinline v1 &operator[](i32 index) {return v[index];}
};
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

union i2
{
 struct{
  i32 x;
  i32 y;
 };
 i32 e[2];
 
 myinline i32 operator[](i32 index){return e[index];}
};
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
  v2 p0;
  v2 p1;
 };
 v2 p[2];
 v4 v4_value;
};

typedef rect2i Rect_i32;
typedef rect2 Rect_f32;

struct Range_i32 {
 union{ i32 min,start,first,begin; };
 union{ i32 max,end,opl; };
};
struct Range_i64 {
 union{ i64 min,start,first,begin; };
 union{ i64 max,end,opl; };
};
union Range_u64 {
 struct{ u64 min; u64 max; };
 struct{ u64 start; u64 end; };
 struct{ u64 first; u64 opl; };
};
union Range_f32 {
 struct{ f32 min; f32 max; };
 struct{ f32 start; f32 end; };
 struct{ f32 first; };
};
#define RangeExpand(range) range.min, range.max

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

#if 0
#  define meta_table
#  define gen_file
#  define gen_for
#endif
//~
#undef KV_H_IS_METAPROGRAM
//~EOF