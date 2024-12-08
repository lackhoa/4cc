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

#if defined(__cplusplus)
#  define EXTERN_C_BEGIN extern "C" {
#  define EXTERN_C_END   }
#else
#  define EXTERN_C_BEGIN
#  define EXTERN_C_END
#endif

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

#define U16_MIN 0u
#define U16_MAX 0xffffu
#define I16_MIN (-0x7fff - 1)
#define I16_MAX 0x7fff

#define F32_MIN 1.17549435e-38f
#define F32_MAX 3.40282347e+38f

#define F64_MIN 2.2250738585072014e-308
#define F64_MAX 1.7976931348623157e+308

#define function      static
#define xfunction             //NOTE(kv) exported function
#define local_persist static
#define global        static
#define global_decl   extern //NOTE(kv) Global var that is not intended to exported, but forward-declared (C doesn't let us forward-declare global???)
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

#define wrap_function(NAME)           NAME##__return NAME(NAME##__params)
#define wrap_function_pointer(NAME)   NAME##__return (*NAME)(NAME##__params)
#define x_wrap_function(NAME)           wrap_function(NAME);
#define x_wrap_function_pointer(NAME)   wrap_function_pointer(NAME);

#define for_inc(TYPE, VAR, MIN, MAX)  for(TYPE VAR=MIN; VAR<MAX; VAR++)

#define for_i32(VAR, MIN, MAX)  for_inc(i32, VAR,MIN,MAX)
#define for_u32(VAR, MIN, MAX)  for_inc(u32, VAR,MIN,MAX)
#define for_i64(VAR, MIN, MAX)  for_inc(i64, VAR,MIN,MAX)
#define for_u64(VAR, MIN, MAX)  for_inc(u64, VAR,MIN,MAX)
#define for_i1  for_i32
#define for_repeat(TIMES) for_i32(line_unique_var,0,TIMES)

#define alen(array) (isize)(sizeof(array) / sizeof((array)[0]))

#define and &&
#define or  ||
#define not !

#define ClampBot(VAR, VAL)   if (VAR < VAL) VAR = VAL
#define ClampTop(VAR, VAL)   if (VAR > VAL) VAR = VAL

struct String{
 union{u8 *str, *data; };
 union{ u64 size, len, length, count; };
 u8 &operator[](i32 index){
  return str[index];
 }
};
struct File_Line
{
 char *file;
 u32  line;
};
//-