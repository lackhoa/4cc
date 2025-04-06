//~ IMPORTANT Fundamental types, idioms
//-Compilers
//-NOTE(kv) Allow overwriting compiler (for e.g clang-cl)
#if !defined(KV_H_IS_METAPROGRAM)
#  define KV_H_IS_METAPROGRAM 0
#endif

#if !defined(COMPILER_MSVC)
#  define COMPILER_MSVC 0
#endif

#if !defined(COMPILER_LLVM)
#  define COMPILER_LLVM 0
#endif

#if !defined(COMPILER_GCC)
#  define COMPILER_GCC 0
#endif
//-

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


#if !(COMPILER_LLVM || COMPILER_MSVC || COMPILER_GCC)
#  if __llvm__
#    undef  COMPILER_LLVM
#    define COMPILER_LLVM 1
#  elif defined(__GNUC__) 
#    undef  COMPILER_GCC 1
#    define COMPILER_GCC 1
#  elif _MSC_VER
#    undef  COMPILER_MSVC
#    define COMPILER_MSVC 1
#  else
#    error Compiler not recognized!
#  endif
#endif

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

#ifdef KV_NO_FORCE_INLINE
#  define myinline inline
#else
#  if COMPILER_MSVC
#    define myinline __forceinline
#  else
#    define myinline __attribute__ ((__always_inline__))
#  endif
#endif

//-

#if defined(__cplusplus)
#  define EXTERN_C_BEGIN extern "C" {
#  define EXTERN_C_END   }
#else
#  define EXTERN_C_BEGIN
#  define EXTERN_C_END
#endif

//-

#if defined(COMPILER_MSVC)
#if _MSC_VER < 1300
typedef unsigned char     u8;
typedef   signed char     i8;
typedef unsigned short   u16;
typedef   signed short   i16;
typedef unsigned int     u32;
typedef   signed int     i32;
#else
typedef unsigned __int8   u8;
typedef   signed __int8   i8;
typedef unsigned __int16 u16;
typedef   signed __int16 i16;
typedef unsigned __int32 u32;
typedef   signed __int32 i32;
#endif
typedef unsigned __int64 u64;
typedef   signed __int64 i64;
#else//-msvc
#include <stdint.h>
typedef uint8_t   u8;
typedef  int8_t   i8;
typedef uint16_t u16;
typedef  int16_t i16;
typedef uint32_t u32;
typedef int32_t  i32;
typedef uint64_t u64;
typedef  int64_t i64;
#endif//-non msvc

typedef i32       i1;
typedef int8_t    b8;
typedef uintptr_t umm; // NOTE(kv): "umm" stands for "memory model"
typedef i64       imm;
typedef size_t    usize;
typedef ptrdiff_t isize;

typedef i32   b32;
typedef i64   b64;

typedef float r32;
typedef float f32;
typedef double f64;
typedef float v1;

#define U8_MAX 0xffu
#define I8_MIN (-0x7f - 1)
#define I8_MAX 0x7f

#define u16_min 0u
#define u16_max 0xffffu
#define i16_min (-0x7fff - 1)
#define i16_max 0x7fff

#define f32_min 1.17549435e-38f
#define f32_max 3.40282347e+38f

#define f64_min 2.2250738585072014e-308
#define f64_max 1.7976931348623157e+308

#define function      static
#define xfunction             //NOTE(kv) exported function
#define local_persist static
#define global        static
#define global_decl   extern //NOTE(kv) Global variable that is forward-declared (C doesn't let us forward-declare static variable, so it must be "extern")
#define xglobal              //NOTE(kv) exported variable
#define auto_lambda   auto

#define PP_Concat(arg1, arg2)   PP_Concat1(arg1, arg2)
#define PP_Concat1(arg1, arg2)  PP_Concat2(arg1, arg2)
#define PP_Concat2(arg1, arg2)  arg1##arg2
#define line_unique_var   PP_Concat(i, __LINE__)
#define count_unique_var  PP_Concat(i, __COUNT__)
#define stringify_(a) #a
#define stringify(a) stringify_(a)

#if COMPILER_MSVC
#    define mytypeof decltype
#else
#    define mytypeof __typeof__
#endif

#if COMPILER_MSVC
#  define kv_fail __debugbreak()
#else
#  define kv_fail __builtin_trap()
#endif

#include "stdio.h"  // TODO(kv) Sorry guys! We need it to print assertions

// NOTE(kv) Provide rudimentary error message,
// so we can use it as a poor man's error report in simple throw-away tools.
#define kv_assert_inner(CLAIM) \
do{ \
if (!(CLAIM)){  \
printf("%s:%d: error: assertion fired: %s", __FILE__, __LINE__, #CLAIM); \
fflush(stdout); \
kv_fail; \
}} while(0)

#if KV_INTERNAL
#    define kv_assert                    kv_assert_inner
#    define assert_defend(CLAIM, DEFEND) kv_assert_inner(CLAIM)
#else
#    define kv_assert(CLAIM)
#    define assert_defend(CLAIM, DEFEND)   if (!(CLAIM))  { DEFEND; }
#endif

#if KV_INTERNAL
#    define fail_in_debug  kv_fail
#else
#    define fail_in_debug
#endif

#define InvalidCodePath     kv_assert(0)
#define invalid_code_path   InvalidCodePath  // NOTE deprecated

#define InvalidDefaultCase default: { invalid_code_path; };
#define breakhere          do{ int please_break = 5; (void)please_break; }while(0)

#define wrap_function(NAME)           NAME##__return NAME(NAME##__params)
#define wrap_function_pointer(NAME)   NAME##__return (*NAME)(NAME##__params)

#define for_inc(TYPE, VAR, MIN, MAX)  for(TYPE VAR=MIN; VAR<MAX; VAR++)

#define for_i32(VAR, MIN, MAX)  for_inc(i32, VAR,MIN,MAX)
#define for_u32(VAR, MIN, MAX)  for_inc(u32, VAR,MIN,MAX)
#define for_i64(VAR, MIN, MAX)  for_inc(i64, VAR,MIN,MAX)
#define for_u64(VAR, MIN, MAX)  for_inc(u64, VAR,MIN,MAX)
#define for_i1  for_i32
#define for_repeat(TIMES)       for_i32(line_unique_var,0,TIMES)

#define for_each(VAR, ARRAY) \
for(auto *VAR = ARRAY.items; \
/**/VAR < ARRAY.items + ARRAY.count; \
/**/VAR++)

#define alen(array) isize(sizeof(array) / sizeof(*(array)))

// NOTE(kv) I don't care if this isn't compiled out. My life is too short...
myinline void assert_expr(b32 condition){ kv_assert(condition); }

#define ArrayGetChecked(ARRAY, INDEX) \
(assert_expr((INDEX) < alen(ARRAY)), ARRAY[INDEX])

#define and &&
#define or  ||
#define not !

#define ClampBot(VAR, VAL)   if (VAR < VAL) VAR = VAL
#define ClampTop(VAR, VAL)   if (VAR > VAL) VAR = VAL

struct String{
 union{u8 *str, *data; };
 union{ u64 size, len, length, count; };
 u8 &operator[](i64 index){ return str[index]; }
};
typedef String Data_And_Size;

myinline b32 is_empty(String s) { return s.count == 0; }
myinline b32 not_empty(String s){ return s.count != 0; }

struct File_Line
{
 char *file;
 u32  line;
};

#define cstrcode(...) #__VA_ARGS__
#define stringize cstrcode
typedef i32 Marker;
//-