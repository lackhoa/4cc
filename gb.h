/* gb.h - v0.33  - Ginger Bill's C Helper Library - public domain
                 - no warranty implied; use at your own risk

	This is a single header file with a bunch of useful stuff
	to replace the C/C++ standard library

===========================================================================
	YOU MUST

		#define GB_IMPLEMENTATION

	in EXACTLY _one_ C or C++ file that includes this header, BEFORE the
	include like this:

		#define GB_IMPLEMENTATION
		#include "gb.h"

	All other files should just #include "gb.h" without #define


	If you want the platform layer, YOU MUST

		#define GB_PLATFORM

	BEFORE the include like this:

		#define GB_PLATFORM
		#include "gb.h"

===========================================================================

LICENSE
	This software is dual-licensed to the public domain and under the following
	license: you are granted a perpetual, irrevocable license to copy, modify,
	publish, and distribute this file as you see fit.

WARNING
	- This library is _slightly_ experimental and features may not work as expected.
	- This also means that many functions are not documented.

CREDITS
	Written by Ginger Bill

TODOS
	- Remove CRT dependency for people who want that
		- But do I really?
		- Or make it only depend on the really needed stuff?
	- Older compiler support?
		- How old do you wanna go?
		- Only support C90+extension and C99 not pure C89.
	- File handling
		- All files to be UTF-8 (even on windows)
	- Better Virtual Memory handling
	- Generic Heap Allocator (tcmalloc/dlmalloc/?)
	- Fixed Heap Allocator
	- Better UTF support and conversion
	- Free List, best fit rather than first fit
	- More date & time functions

VERSION HISTORY
	0.33  - Minor fixes
	0.32  - Minor fixes
	0.31  - Add gb_file_remove
	0.30  - Changes to gbThread (and gbMutex on Windows)
	0.29  - Add extras for gbString
	0.28  - Handle UCS2 correctly in Win32 part
	0.27  - OSX fixes and Linux gbAffinity
	0.26d - Minor changes to how gbFile works
	0.26c - gb_str_to_f* fix
	0.26b - Minor fixes
	0.26a - gbString Fix
	0.26  - Default allocator flags and generic hash table
	0.25a - Fix UTF-8 stuff
	0.25  - OS X gbPlatform Support (missing some things)
	0.24b - Compile on OSX (excluding platform part)
	0.24a - Minor additions
	0.24  - Enum convention change
	0.23  - Optional Windows.h removal (because I'm crazy)
	0.22a - Remove gbVideoMode from gb_platform_init_*
	0.22  - gbAffinity - (Missing Linux version)
	0.21  - Platform Layer Restructuring
	0.20  - Improve file io
	0.19  - Clipboard Text
	0.18a - Controller vibration
	0.18  - Raw keyboard and mouse input for WIN32
	0.17d - Fixed printf bug for strings
	0.17c - Compile as 32 bit
	0.17b - Change formating style because why not?
	0.17a - Dropped C90 Support (For numerous reasons)
	0.17  - Instantiated Hash Table
	0.16a - Minor code layout changes
	0.16  - New file API and improved platform layer
	0.15d - Linux Experimental Support (DON'T USE IT PLEASE)
	0.15c - Linux Experimental Support (DON'T USE IT)
	0.15b - C90 Support
	0.15a - gb_atomic(32|64)_spin_(lock|unlock)
	0.15  - Recursive "Mutex"; Key States; gbRandom
	0.14  - Better File Handling and better printf (WIN32 Only)
	0.13  - Highly experimental platform layer (WIN32 Only)
	0.12b - Fix minor file bugs
	0.12a - Compile as C++
	0.12  - New File Handing System! No stdio or stdlib! (WIN32 Only)
	0.11a - Add string precision and width (experimental)
	0.11  - Started making stdio & stdlib optional (Not tested much)
	0.10c - Fix gb_endian_swap32()
	0.10b - Probable timing bug for gb_time_now()
	0.10a - Work on multiple compilers
	0.10  - Scratch Memory Allocator
	0.09a - Faster Mutex and the Free List is slightly improved
	0.09  - Basic Virtual Memory System and Dreadful Free List allocator
	0.08a - Fix *_appendv bug
	0.08  - Huge Overhaul!
	0.07a - Fix alignment in gb_heap_allocator_proc
	0.07  - Hash Table and Hashing Functions
	0.06c - Better Documentation
	0.06b - OS X Support
	0.06a - Linux Support
	0.06  - Windows GCC Support and MSVC x86 Support
	0.05b - Formatting
	0.05a - Minor function name changes
	0.05  - Radix Sort for unsigned integers (TODO: Other primitives)
	0.04  - Better UTF support and search/sort procs
	0.03  - Completely change procedure naming convention
	0.02a - Bug fixes
	0.02  - Change naming convention and gbArray(Type)
	0.01  - Initial Version
*/


#ifndef GB_INCLUDE_GB_H
#define GB_INCLUDE_GB_H

#if defined(__cplusplus)
// extern "C" {
EXTERN_C_BEGIN
#endif

#if defined(__cplusplus)
	#define GB_EXTERN extern "C"
#else
	#define GB_EXTERN extern
#endif

#if defined(_WIN32)
	#define GB_DLL_EXPORT GB_EXTERN __declspec(dllexport)
	#define GB_DLL_IMPORT GB_EXTERN __declspec(dllimport)
#else
	#define GB_DLL_EXPORT GB_EXTERN __attribute__((visibility("default")))
	#define GB_DLL_IMPORT GB_EXTERN
#endif

// NOTE(bill): Redefine for DLL, etc.
#ifndef GB_DEF
	#ifdef GB_STATIC
		#define GB_DEF static
	#else
		#define GB_DEF extern
	#endif
#endif

#if defined(_WIN64) || defined(__x86_64__) || defined(_M_X64) || defined(__64BIT__) || defined(__powerpc64__) || defined(__ppc64__)
	#ifndef GB_ARCH_64_BIT
	#define GB_ARCH_64_BIT 1
	#endif
#else
	// NOTE(bill): I'm only supporting 32 bit and 64 bit systems
	#ifndef GB_ARCH_32_BIT
	#define GB_ARCH_32_BIT 1
	#endif
#endif


#ifndef GB_ENDIAN_ORDER
#define GB_ENDIAN_ORDER
	// TODO(bill): Is the a good way or is it better to test for certain compilers and macros?
	#define GB_IS_BIG_ENDIAN    (!*(u8*)&(u16){1})
	#define GB_IS_LITTLE_ENDIAN (!GB_IS_BIG_ENDIAN)
#endif

#if defined(_WIN32) || defined(_WIN64)
	#ifndef GB_SYSTEM_WINDOWS
	#define GB_SYSTEM_WINDOWS 1
	#endif
#elif defined(__APPLE__) && defined(__MACH__)
	#ifndef GB_SYSTEM_OSX
	#define GB_SYSTEM_OSX 1
	#endif
#elif defined(__unix__)
	#ifndef GB_SYSTEM_UNIX
	#define GB_SYSTEM_UNIX 1
	#endif

	#if defined(__linux__)
		#ifndef GB_SYSTEM_LINUX
		#define GB_SYSTEM_LINUX 1
		#endif
	#else
		#error This UNIX operating system is not supported
	#endif
#else
	#error This operating system is not supported
#endif

#if defined(_MSC_VER)
	#define GB_COMPILER_MSVC 1
#elif defined(__GNUC__)
	#define GB_COMPILER_GCC 1
#elif defined(__clang__)
	#define GB_COMPILER_CLANG 1
#else
	#error Unknown compiler
#endif

#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
	#ifndef GB_CPU_X86
	#define GB_CPU_X86 1
	#endif
	#ifndef GB_CACHE_LINE_SIZE
	#define GB_CACHE_LINE_SIZE 64
	#endif
#else
	#error Unknown CPU Type
#endif



#ifndef GB_STATIC_ASSERT
	#define GB_STATIC_ASSERT3(cond, msg) typedef char static_assertion_##msg[(!!(cond))*2-1]
	// NOTE(bill): Token pasting madness!!
	#define GB_STATIC_ASSERT2(cond, line) GB_STATIC_ASSERT3(cond, static_assertion_at_line_##line)
	#define GB_STATIC_ASSERT1(cond, line) GB_STATIC_ASSERT2(cond, line)
	#define GB_STATIC_ASSERT(cond)        GB_STATIC_ASSERT1(cond, __LINE__)
#endif


////////////////////////////////////////////////////////////////
//
// Headers
//
//

#if defined(_WIN32) && !defined(__MINGW32__)
	#ifndef _CRT_SECURE_NO_WARNINGS
	#define _CRT_SECURE_NO_WARNINGS
	#endif
#endif

#if defined(GB_SYSTEM_UNIX)
	#define _GNU_SOURCE
	#define _LARGEFILE64_SOURCE
#endif


// TODO(bill): How many of these headers do I really need?
// #include <stdarg.h>
#if !defined(GB_SYSTEM_WINDOWS)
#include <stddef.h>
#include <stdarg.h>
#endif



#if !AD_IS_DRIVER
#if defined(GB_SYSTEM_WINDOWS)
	#if !defined(GB_NO_WINDOWS_H)
		#define NOMINMAX            1
		#define WIN32_LEAN_AND_MEAN 1
		#define WIN32_MEAN_AND_LEAN 1
		#define VC_EXTRALEAN        1
		#include <windows.h>
		#undef NOMINMAX
		#undef WIN32_LEAN_AND_MEAN
		#undef WIN32_MEAN_AND_LEAN
		#undef VC_EXTRALEAN
	#endif

	#include <malloc.h> // NOTE(bill): _aligned_*()
	#include <intrin.h>
#else
	#include <dlfcn.h>
	#include <errno.h>
	#include <fcntl.h>
	#include <pthread.h>
	#ifndef _IOSC11_SOURCE
	#define _IOSC11_SOURCE
	#endif
	#include <stdlib.h> // NOTE(bill): malloc on linux
	#include <sys/mman.h>
	#if !defined(GB_SYSTEM_OSX)
		#include <sys/sendfile.h>
	#endif
	#include <sys/stat.h>
	#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#if defined(GB_CPU_X86)
#  include <xmmintrin.h>
#endif
#endif
#endif

#if defined(GB_SYSTEM_OSX)
	#include <mach/mach.h>
	#include <mach/mach_init.h>
	#include <mach/mach_time.h>
	#include <mach/thread_act.h>
	#include <mach/thread_policy.h>
	#include <sys/sysctl.h>
	#include <copyfile.h>
	#include <mach/clock.h>
#endif

#if defined(GB_SYSTEM_UNIX)
	#include <semaphore.h>
#endif


////////////////////////////////////////////////////////////////
//
// Base Types
//
//

#if defined(GB_COMPILER_MSVC)
	#if _MSC_VER < 1300
	typedef unsigned char     u8;
	typedef   signed char     i8;
	typedef unsigned short   u16;
	typedef   signed short   i16;
	typedef unsigned int     u32;
	typedef   signed int     i1;
	#else
	typedef unsigned __int8   u8;
	typedef   signed __int8   i8;
	typedef unsigned __int16 u16;
	typedef   signed __int16 i16;
	typedef unsigned __int32 u32;
	typedef   signed __int32 i1;
	#endif
	typedef unsigned __int64 u64;
	typedef   signed __int64 i64;
#else
	#include <stdint.h>
	typedef uint8_t   u8;
	typedef  int8_t   i8;
	typedef uint16_t u16;
	typedef  int16_t i16;
	typedef uint32_t u32;
	typedef  int32_t i1;
	typedef uint64_t u64;
	typedef  int64_t i64;
#endif

GB_STATIC_ASSERT(sizeof(u8)  == sizeof(i8));
GB_STATIC_ASSERT(sizeof(u16) == sizeof(i16));
GB_STATIC_ASSERT(sizeof(u32) == sizeof(i1));
GB_STATIC_ASSERT(sizeof(u64) == sizeof(i64));

GB_STATIC_ASSERT(sizeof(u8)  == 1);
GB_STATIC_ASSERT(sizeof(u16) == 2);
GB_STATIC_ASSERT(sizeof(u32) == 4);
GB_STATIC_ASSERT(sizeof(u64) == 8);

typedef size_t    usize;
typedef ptrdiff_t isize;

GB_STATIC_ASSERT(sizeof(usize) == sizeof(isize));

// NOTE(bill): (u)intptr is only here for semantic reasons really as this library will only support 32/64 bit OSes.
// NOTE(bill): Are there any modern OSes (not 16 bit) where intptr != isize ?
#if defined(_WIN64)
	typedef signed   __int64  intptr;
	typedef unsigned __int64 uintptr;
#elif defined(_WIN32)
	// NOTE(bill); To mark types changing their size, e.g. intptr
	#ifndef _W64
		#if !defined(__midl) && (defined(_X86_) || defined(_M_IX86)) && _MSC_VER >= 1300
			#define _W64 __w64
		#else
			#define _W64
		#endif
	#endif

	typedef _W64   signed int  intptr;
	typedef _W64 unsigned int uintptr;
#else
	typedef uintptr_t uintptr;
	typedef  intptr_t  intptr;
#endif

GB_STATIC_ASSERT(sizeof(uintptr) == sizeof(intptr));

typedef float  f32;
typedef double f64;

GB_STATIC_ASSERT(sizeof(f32) == 4);
GB_STATIC_ASSERT(sizeof(f64) == 8);

typedef i1 Rune; // NOTE(bill): Unicode codepoint
#define GB_RUNE_INVALID cast(Rune)(0xfffd)
#define GB_RUNE_MAX     cast(Rune)(0x0010ffff)
#define GB_RUNE_BOM     cast(Rune)(0xfeff)
#define GB_RUNE_EOF     cast(Rune)(-1)


typedef i8  b8;
typedef i16 b16;
typedef i1 b32; // NOTE(bill): Prefer this!!!

// NOTE(bill): Get true and false
#if !defined(__cplusplus)
	#if (defined(_MSC_VER) && _MSC_VER < 1800) || (!defined(_MSC_VER) && !defined(__STDC_VERSION__))
		#ifndef true
		#define true  (0 == 0)
		#endif
		#ifndef false
		#define false (0 != 0)
		#endif
		typedef b8 bool;
	#else
		#include <stdbool.h>
	#endif
#endif

// NOTE(bill): These do are not prefixed with gb because the types are not.
#ifndef U8_MIN
#define U8_MIN 0u
#define U8_MAX 0xffu
#define I8_MIN (-0x7f - 1)
#define I8_MAX 0x7f

#define U16_MIN 0u
#define U16_MAX 0xffffu
#define I16_MIN (-0x7fff - 1)
#define I16_MAX 0x7fff

#define U32_MIN 0u
#define U32_MAX 0xffffffffu
#define I32_MIN (-0x7fffffff - 1)
#define I32_MAX 0x7fffffff

#define U64_MIN 0ull
#define U64_MAX 0xffffffffffffffffull
#define I64_MIN (-0x7fffffffffffffffll - 1)
#define I64_MAX 0x7fffffffffffffffll

#if defined(GB_ARCH_32_BIT)
	#define USIZE_MIX U32_MIN
	#define USIZE_MAX U32_MAX

	#define ISIZE_MIX S32_MIN
	#define ISIZE_MAX S32_MAX
#elif defined(GB_ARCH_64_BIT)
	#define USIZE_MIX U64_MIN
	#define USIZE_MAX U64_MAX

	#define ISIZE_MIX I64_MIN
	#define ISIZE_MAX I64_MAX
#else
	#error Unknown architecture size. This library only supports 32 bit and 64 bit architectures.
#endif

#define F32_MIN 1.17549435e-38f
#define F32_MAX 3.40282347e+38f

#define F64_MIN 2.2250738585072014e-308
#define F64_MAX 1.7976931348623157e+308

#endif

#ifndef NULL
	#if defined(__cplusplus)
		#if __cplusplus >= 201103L
			#define NULL nullptr
		#else
			#define NULL 0
		#endif
	#else
		#define NULL ((void *)0)
	#endif
#endif

// TODO(bill): Is this enough to get inline working?
#if !defined(__cplusplus)
	#if defined(_MSC_VER) && _MSC_VER <= 1800
	#define inline __inline
	#elif !defined(__STDC_VERSION__)
	#define inline __inline__
	#else
	#define inline
	#endif
#endif

#if !defined(gb_restrict)
	#if defined(_MSC_VER)
		#define gb_restrict __restrict
	#elif defined(__STDC_VERSION__)
		#define gb_restrict restrict
	#else
		#define gb_restrict
	#endif
#endif

// TODO(bill): Should force inline be a separate keyword and gb_inline be inline?
#if !defined(gb_inline)
	#if defined(_MSC_VER)
		#if _MSC_VER < 1300
		#define gb_inline
		#else
		#define gb_inline __forceinline
		#endif
	#else
		#define gb_inline __attribute__ ((__always_inline__))
	#endif
#endif

#if !defined(gb_no_inline)
	#if defined(_MSC_VER)
		#define gb_no_inline __declspec(noinline)
	#else
		#define gb_no_inline __attribute__ ((noinline))
	#endif
#endif


#if !defined(gb_thread_local)
	#if defined(_MSC_VER) && _MSC_VER >= 1300
		#define gb_thread_local __declspec(thread)
	#elif defined(__GNUC__)
		#define gb_thread_local __thread
	#else
		#define gb_thread_local thread_local
	#endif
#endif


// NOTE(bill): Easy to grep
// NOTE(bill): Not needed in macros
#ifndef cast
#define cast(Type) (Type)
#endif

// NOTE(bill): Because a signed sizeof is more useful
#ifndef gb_size_of
#define gb_size_of(x) (isize)(sizeof(x))
#endif

#ifndef gb_count_of
#define gb_count_of(x) ((gb_size_of(x)/gb_size_of(0[x])) / ((isize)(!(gb_size_of(x) % gb_size_of(0[x])))))
#endif

#ifndef offsetof
#define offsetof(Type, element) ((isize)&(((Type *)0)->element))
#endif

#if defined(__cplusplus)
#ifndef gb_align_of
	#if __cplusplus >= 201103L
		#define gb_align_of(Type) (isize)alignof(Type)
	#else
extern "C++" {
		// NOTE(bill): Fucking Templates!
		template <typename T> struct gbAlignment_Trick { char c; T member; };
		#define gb_align_of(Type) offsetof(gbAlignment_Trick<Type>, member)
}
	#endif
#endif
#else
	#ifndef gb_align_of
	#define gb_align_of(Type) offsetof(struct { char c; Type member; }, member)
	#endif
#endif

// NOTE(bill): I do wish I had a type_of that was portable
#ifndef gb_swap
#define gb_swap(Type, a, b) do { Type tmp = (a); (a) = (b); (b) = tmp; } while (0)
#endif

// NOTE(bill): Because static means 3/4 different things in C/C++. Great design (!)
#ifndef gb_global
#define gb_global        static // Global variables
#define gb_internal      static // Internal linkage
#define gb_local_persist static // Local Persisting variables
#define internal static
#endif


#ifndef gb_unused
	#if defined(_MSC_VER)
		#define gb_unused(x) (__pragma(warning(suppress:4100))(x))
	#elif defined (__GCC__)
		#define gb_unused(x) __attribute__((__unused__))(x)
	#else
		#define gb_unused(x) ((void)(gb_size_of(x)))
	#endif
#endif




////////////////////////////////////////////////////////////////
//
// Defer statement
// Akin to D's SCOPE_EXIT or
// similar to Go's defer but scope-based
//
// NOTE: C++11 (and above) only!
//
#if !defined(GB_NO_DEFER) && defined(__cplusplus) && ((defined(_MSC_VER) && _MSC_VER >= 1400) || (__cplusplus >= 201103L))
extern "C++" {
	// NOTE(bill): Stupid fucking templates
	template <typename T> struct gbRemoveReference       { typedef T Type; };
	template <typename T> struct gbRemoveReference<T &>  { typedef T Type; };
	template <typename T> struct gbRemoveReference<T &&> { typedef T Type; };

	/// NOTE(bill): "Move" semantics - invented because the C++ committee are idiots (as a collective not as indiviuals (well a least some aren't))
	template <typename T> inline T &&gb_forward(typename gbRemoveReference<T>::Type &t)  { return static_cast<T &&>(t); }
	template <typename T> inline T &&gb_forward(typename gbRemoveReference<T>::Type &&t) { return static_cast<T &&>(t); }
	template <typename T> inline T &&gb_move   (T &&t)                                   { return static_cast<typename gbRemoveReference<T>::Type &&>(t); }
	template <typename F>
	struct gbprivDefer {
		F f;
		gbprivDefer(F &&f) : f(gb_forward<F>(f)) {}
		~gbprivDefer() { f(); }
	};
	template <typename F> gbprivDefer<F> gb__defer_func(F &&f) { return gbprivDefer<F>(gb_forward<F>(f)); }

	#define GB_DEFER_1(x, y) x##y
	#define GB_DEFER_2(x, y) GB_DEFER_1(x, y)
	#define GB_DEFER_3(x)    GB_DEFER_2(x, __LINE__)
	#define defer(code)      auto GB_DEFER_3(_defer_) = gb__defer_func([&]()->void{code;})
}
#endif


////////////////////////////////////////////////////////////////
//
// Macro Fun!
//
//

#ifndef GB_JOIN_MACROS
#define GB_JOIN_MACROS
	#define GB_JOIN2_IND(a, b) a##b

	#define GB_JOIN2(a, b)       GB_JOIN2_IND(a, b)
	#define GB_JOIN3(a, b, c)    GB_JOIN2(GB_JOIN2(a, b), c)
	#define GB_JOIN4(a, b, c, d) GB_JOIN2(GB_JOIN2(GB_JOIN2(a, b), c), d)
#endif


#ifndef GB_BIT
#define GB_BIT(x) (1<<(x))
#endif

#ifndef gb_min
#define gb_min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef gb_max
#define gb_max(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef gb_min3
#define gb_min3(a, b, c) gb_min(gb_min(a, b), c)
#endif

#ifndef gb_max3
#define gb_max3(a, b, c) gb_max(gb_max(a, b), c)
#endif

#ifndef gb_clamp
#define gb_clamp(x, lower, upper) gb_min(gb_max((x), (lower)), (upper))
#endif

#ifndef gb_clamp01
#define gb_clamp01(x) gb_clamp((x), 0, 1)
#endif

#ifndef gb_is_between
#define gb_is_between(x, lower, upper) (((lower) <= (x)) && ((x) <= (upper)))
#endif

#ifndef gb_abs
#define gb_abs(x) ((x) < 0 ? -(x) : (x))
#endif

/* NOTE(bill): Very useful bit setting */
#ifndef GB_MASK_SET
#define GB_MASK_SET(var, set, mask) do { \
	if (set) (var) |=  (mask); \
	else     (var) &= ~(mask); \
} while (0)
#endif


// NOTE(bill): Some compilers support applying printf-style warnings to user functions.
#if defined(__clang__) || defined(__GNUC__)
#define GB_PRINTF_ARGS(FMT) __attribute__((format(printf, FMT, (FMT+1))))
#else
#define GB_PRINTF_ARGS(FMT)
#endif

////////////////////////////////////////////////////////////////
//
// Debug
//
//


#ifndef GB_DEBUG_TRAP
	#if defined(_MSC_VER)
	 	#if _MSC_VER < 1300
		#define GB_DEBUG_TRAP() __asm int 3 /* Trap to debugger! */
#else
#define GB_DEBUG_TRAP() __debugbreak()
#endif
#else
#define GB_DEBUG_TRAP() __builtin_trap()
#endif
#endif

//gb_assert_handler("Assertion Failure", #cond, __FILE__, cast(i64)__LINE__, msg, ##__VA_ARGS__);

#ifndef GB_ASSERT_MSG
#define GB_ASSERT_MSG(cond, msg, ...) do { \
if (!(cond)) { \
		GB_DEBUG_TRAP(); \
	} \
} while (0)
#endif

#ifndef GB_ASSERT
#define GB_ASSERT(cond) GB_ASSERT_MSG(cond, NULL)
#endif

#ifndef GB_ASSERT_NOT_NULL
#define GB_ASSERT_NOT_NULL(ptr) GB_ASSERT_MSG((ptr) != NULL, #ptr " must not be NULL")
#endif

// NOTE(bill): Things that shouldn't happen with a message!
// gb_assert_handler("Panic", NULL, __FILE__, cast(i64)__LINE__, msg, ##__VA_ARGS__);
#ifndef GB_PANIC
#define GB_PANIC(msg, ...) do { \
GB_DEBUG_TRAP(); \
} while (0)
#endif

GB_DEF void gb_assert_handler(char const *prefix, char const *condition, char const *file, i1 line, char const *msg, ...);



#if !AD_IS_DRIVER
////////////////////////////////////////////////////////////////
//
// Memory
//
//


GB_DEF b32 gb_is_power_of_two(isize x);

GB_DEF void *      gb_align_forward(void *ptr, isize alignment);

GB_DEF void *      gb_pointer_add      (void *ptr, isize bytes);
GB_DEF void *      gb_pointer_sub      (void *ptr, isize bytes);
GB_DEF void const *gb_pointer_add_const(void const *ptr, isize bytes);
GB_DEF void const *gb_pointer_sub_const(void const *ptr, isize bytes);
GB_DEF isize       gb_pointer_diff     (void const *begin, void const *end);


GB_DEF void gb_zero_size(void *ptr, isize size);
#ifndef     gb_zero_item
#define     gb_zero_item(t)         gb_zero_size((t), gb_size_of(*(t))) // NOTE(bill): Pass pointer of struct
#define     gb_zero_array(a, count) gb_zero_size((a), gb_size_of(*(a))*count)
#endif

GB_DEF void *      gb_memcopy   (void *dest, void const *source, isize size);
GB_DEF void *      gb_memmove   (void *dest, void const *source, isize size);
GB_DEF void *      gb_memset    (void *data, u8 byte_value, isize size);
GB_DEF i1         gb_memcompare(void const *s1, void const *s2, isize size);
GB_DEF void        gb_memswap   (void *i, void *j, isize size);
GB_DEF void const *gb_memchr    (void const *data, u8 byte_value, isize size);
GB_DEF void const *gb_memrchr   (void const *data, u8 byte_value, isize size);
#endif


#ifndef gb_memcopy_array
#define gb_memcopy_array(dst, src, count) gb_memcopy((dst), (src), gb_size_of(*(dst))*(count))
#endif

#ifndef gb_memmove_array
#define gb_memmove_array(dst, src, count) gb_memmove((dst), (src), gb_size_of(*(dst))*(count))
#endif

// NOTE(bill): Very similar to doing `*cast(T *)(&u)`
#ifndef GB_BIT_CAST
#define GB_BIT_CAST(dest, source) do { \
	GB_STATIC_ASSERT(gb_size_of(*(dest)) <= gb_size_of(source)); \
	gb_memcopy((dest), &(source), gb_size_of(*dest)); \
} while (0)
#endif




#ifndef gb_kilobytes
#define gb_kilobytes(x) (            (x) * (i64)(1024))
#define gb_megabytes(x) (gb_kilobytes(x) * (i64)(1024))
#define gb_gigabytes(x) (gb_megabytes(x) * (i64)(1024))
#define gb_terabytes(x) (gb_gigabytes(x) * (i64)(1024))
#endif


#if !AD_IS_DRIVER

GB_DEF u64  gb_rdtsc       (void);
GB_DEF f64  gb_time_now    (void); // NOTE(bill): This is only for relative time e.g. game loops
GB_DEF u64  gb_utc_time_now(void); // NOTE(bill): Number of microseconds since 1601-01-01 UTC
GB_DEF void gb_sleep_ms    (u32 ms);


// Atomics

// TODO(bill): Be specific with memory order?
// e.g. relaxed, acquire, release, acquire_release

#if defined(GB_COMPILER_MSVC)
typedef struct gbAtomic32  { i1   volatile value; } gbAtomic32;
typedef struct gbAtomic64  { i64   volatile value; } gbAtomic64;
typedef struct gbAtomicPtr { void *volatile value; } gbAtomicPtr;
#else
	#if defined(GB_ARCH_32_BIT)
	#define GB_ATOMIC_PTR_ALIGNMENT 4
	#elif defined(GB_ARCH_64_BIT)
	#define GB_ATOMIC_PTR_ALIGNMENT 8
	#else
	#error Unknown architecture
	#endif

typedef struct gbAtomic32  { i1   volatile value; } __attribute__ ((aligned(4))) gbAtomic32;
typedef struct gbAtomic64  { i64   volatile value; } __attribute__ ((aligned(8))) gbAtomic64;
typedef struct gbAtomicPtr { void *volatile value; } __attribute__ ((aligned(GB_ATOMIC_PTR_ALIGNMENT))) gbAtomicPtr;
#endif

GB_DEF i1  gb_atomic32_load            (gbAtomic32 const volatile *a);
GB_DEF void gb_atomic32_store           (gbAtomic32 volatile *a, i1 value);
GB_DEF i1  gb_atomic32_compare_exchange(gbAtomic32 volatile *a, i1 expected, i1 desired);
GB_DEF i1  gb_atomic32_exchanged       (gbAtomic32 volatile *a, i1 desired);
GB_DEF i1  gb_atomic32_fetch_add       (gbAtomic32 volatile *a, i1 operand);
GB_DEF i1  gb_atomic32_fetch_and       (gbAtomic32 volatile *a, i1 operand);
GB_DEF i1  gb_atomic32_fetch_or        (gbAtomic32 volatile *a, i1 operand);
GB_DEF b32  gb_atomic32_spin_lock       (gbAtomic32 volatile *a, isize time_out); // NOTE(bill): time_out = -1 as default
GB_DEF void gb_atomic32_spin_unlock     (gbAtomic32 volatile *a);
GB_DEF b32  gb_atomic32_try_acquire_lock(gbAtomic32 volatile *a);


GB_DEF i64  gb_atomic64_load            (gbAtomic64 const volatile *a);
GB_DEF void gb_atomic64_store           (gbAtomic64 volatile *a, i64 value);
GB_DEF i64  gb_atomic64_compare_exchange(gbAtomic64 volatile *a, i64 expected, i64 desired);
GB_DEF i64  gb_atomic64_exchanged       (gbAtomic64 volatile *a, i64 desired);
GB_DEF i64  gb_atomic64_fetch_add       (gbAtomic64 volatile *a, i64 operand);
GB_DEF i64  gb_atomic64_fetch_and       (gbAtomic64 volatile *a, i64 operand);
GB_DEF i64  gb_atomic64_fetch_or        (gbAtomic64 volatile *a, i64 operand);
GB_DEF b32  gb_atomic64_spin_lock       (gbAtomic64 volatile *a, isize time_out); // NOTE(bill): time_out = -1 as default
GB_DEF void gb_atomic64_spin_unlock     (gbAtomic64 volatile *a);
GB_DEF b32  gb_atomic64_try_acquire_lock(gbAtomic64 volatile *a);


GB_DEF void *gb_atomic_ptr_load            (gbAtomicPtr const volatile *a);
GB_DEF void  gb_atomic_ptr_store           (gbAtomicPtr volatile *a, void *value);
GB_DEF void *gb_atomic_ptr_compare_exchange(gbAtomicPtr volatile *a, void *expected, void *desired);
GB_DEF void *gb_atomic_ptr_exchanged       (gbAtomicPtr volatile *a, void *desired);
GB_DEF void *gb_atomic_ptr_fetch_add       (gbAtomicPtr volatile *a, void *operand);
GB_DEF void *gb_atomic_ptr_fetch_and       (gbAtomicPtr volatile *a, void *operand);
GB_DEF void *gb_atomic_ptr_fetch_or        (gbAtomicPtr volatile *a, void *operand);
GB_DEF b32   gb_atomic_ptr_spin_lock       (gbAtomicPtr volatile *a, isize time_out); // NOTE(bill): time_out = -1 as default
GB_DEF void  gb_atomic_ptr_spin_unlock     (gbAtomicPtr volatile *a);
GB_DEF b32   gb_atomic_ptr_try_acquire_lock(gbAtomicPtr volatile *a);


// Fences
GB_DEF void gb_yield_thread(void);
GB_DEF void gb_mfence      (void);
GB_DEF void gb_sfence      (void);
GB_DEF void gb_lfence      (void);


#if defined(GB_SYSTEM_WINDOWS)
typedef struct gbSemaphore { void *win32_handle; }     gbSemaphore;
#elif defined(GB_SYSTEM_OSX)
typedef struct gbSemaphore { semaphore_t osx_handle; } gbSemaphore;
#elif defined(GB_SYSTEM_UNIX)
typedef struct gbSemaphore { sem_t unix_handle; }      gbSemaphore;
#else
#error
#endif

GB_DEF void gb_semaphore_init   (gbSemaphore *s);
GB_DEF void gb_semaphore_destroy(gbSemaphore *s);
GB_DEF void gb_semaphore_post   (gbSemaphore *s, i1 count);
GB_DEF void gb_semaphore_release(gbSemaphore *s); // NOTE(bill): gb_semaphore_post(s, 1)
GB_DEF void gb_semaphore_wait   (gbSemaphore *s);


// Mutex
typedef struct gbMutex {
#if defined(GB_SYSTEM_WINDOWS)
	CRITICAL_SECTION win32_critical_section;
#else
	pthread_mutex_t pthread_mutex;
	pthread_mutexattr_t pthread_mutexattr;
#endif
} gbMutex;

GB_DEF void gb_mutex_init    (gbMutex *m);
GB_DEF void gb_mutex_destroy (gbMutex *m);
GB_DEF void gb_mutex_lock    (gbMutex *m);
GB_DEF b32  gb_mutex_try_lock(gbMutex *m);
GB_DEF void gb_mutex_unlock  (gbMutex *m);

#define GB_THREAD_PROC(name) isize name(struct gbThread *thread)
typedef GB_THREAD_PROC(gbThreadProc);

typedef struct gbThread {
#if defined(GB_SYSTEM_WINDOWS)
	void *        win32_handle;
#else
	pthread_t     posix_handle;
#endif

	gbThreadProc *proc;
	void *        user_data;
	isize         user_index;
	isize         return_value;

	gbSemaphore   semaphore;
	isize         stack_size;
	b32 volatile  is_running;
} gbThread;

GB_DEF void gb_thread_init            (gbThread *t);
GB_DEF void gb_thread_destroy         (gbThread *t);
GB_DEF void gb_thread_start           (gbThread *t, gbThreadProc *proc, void *data);
GB_DEF void gb_thread_start_with_stack(gbThread *t, gbThreadProc *proc, void *data, isize stack_size);
GB_DEF void gb_thread_join            (gbThread *t);
GB_DEF b32  gb_thread_is_running      (gbThread const *t);
GB_DEF u32  gb_thread_current_id      (void);
GB_DEF void gb_thread_set_name        (gbThread *t, char const *name);


// NOTE(bill): Thread Merge Operation
// Based on Sean Barrett's stb_sync
typedef struct gbSync {
	i1 target;  // Target Number of threads
	i1 current; // Threads to hit
	i1 waiting; // Threads waiting

	gbMutex start;
	gbMutex mutex;
	gbSemaphore release;
} gbSync;

GB_DEF void gb_sync_init          (gbSync *s);
GB_DEF void gb_sync_destroy       (gbSync *s);
GB_DEF void gb_sync_set_target    (gbSync *s, i1 count);
GB_DEF void gb_sync_release       (gbSync *s);
GB_DEF i1  gb_sync_reach         (gbSync *s);
GB_DEF void gb_sync_reach_and_wait(gbSync *s);



#if defined(GB_SYSTEM_WINDOWS)

typedef struct gbAffinity {
	b32   is_accurate;
	isize core_count;
	isize thread_count;
	#define GB_WIN32_MAX_THREADS (8 * gb_size_of(usize))
	usize core_masks[GB_WIN32_MAX_THREADS];

} gbAffinity;

#elif defined(GB_SYSTEM_OSX)
typedef struct gbAffinity {
	b32   is_accurate;
	isize core_count;
	isize thread_count;
	isize threads_per_core;
} gbAffinity;

#elif defined(GB_SYSTEM_LINUX)
typedef struct gbAffinity {
	b32   is_accurate;
	isize core_count;
	isize thread_count;
	isize threads_per_core;
} gbAffinity;
#else
#error TODO(bill): Unknown system
#endif

GB_DEF void  gb_affinity_init   (gbAffinity *a);
GB_DEF void  gb_affinity_destroy(gbAffinity *a);
GB_DEF b32   gb_affinity_set    (gbAffinity *a, isize core, isize thread);
GB_DEF isize gb_affinity_thread_count_for_core(gbAffinity *a, isize core);
#endif




////////////////////////////////////////////////////////////////
//
// Virtual Memory
//
//

typedef struct gbVirtualMemory {
	void *data;
	isize size;
} gbVirtualMemory;

GB_DEF gbVirtualMemory gb_virtual_memory(void *data, isize size);
GB_DEF gbVirtualMemory gb_vm_alloc      (void *addr, isize size);
GB_DEF b32             gb_vm_free       (gbVirtualMemory vm);
GB_DEF gbVirtualMemory gb_vm_trim       (gbVirtualMemory vm, isize lead_size, isize size);
GB_DEF b32             gb_vm_purge      (gbVirtualMemory vm);
GB_DEF isize gb_virtual_memory_page_size(isize *alignment_out);




////////////////////////////////////////////////////////////////
//
// Custom Allocation
//
//

typedef enum gbAllocationType {
	gbAllocation_Alloc,
	gbAllocation_Free,
	gbAllocation_FreeAll,
	gbAllocation_Resize,
} gbAllocationType;

// NOTE(bill): This is useful so you can define an allocator of the same type and parameters
#define GB_ALLOCATOR_PROC(name)                         \
void *name(void *allocator_data, gbAllocationType type, \
           isize size, isize alignment,                 \
           void *old_memory, isize old_size,            \
           u64 flags)
typedef GB_ALLOCATOR_PROC(gbAllocatorProc);

typedef struct gbAllocator {
	gbAllocatorProc *proc;
	void *           data;
} gbAllocator;

typedef enum gbAllocatorFlag {
	gbAllocatorFlag_ClearToZero = GB_BIT(0),
} gbAllocatorFlag;

// TODO(bill): Is this a decent default alignment?
#ifndef GB_DEFAULT_MEMORY_ALIGNMENT
#define GB_DEFAULT_MEMORY_ALIGNMENT (2 * gb_size_of(void *))
#endif

#ifndef GB_DEFAULT_ALLOCATOR_FLAGS
#define GB_DEFAULT_ALLOCATOR_FLAGS (gbAllocatorFlag_ClearToZero)
#endif

#if !AD_IS_DRIVER
GB_DEF void *gb_alloc_align (gbAllocator a, isize size, isize alignment);
GB_DEF void *gb_alloc       (gbAllocator a, isize size);
GB_DEF void  gb_free        (gbAllocator a, void *ptr);
GB_DEF void  gb_free_all    (gbAllocator a);
GB_DEF void *gb_resize      (gbAllocator a, void *ptr, isize old_size, isize new_size);
GB_DEF void *gb_resize_align(gbAllocator a, void *ptr, isize old_size, isize new_size, isize alignment);
// TODO(bill): For gb_resize, should the use need to pass the old_size or only the new_size?

GB_DEF void *gb_alloc_copy      (gbAllocator a, void const *src, isize size);
GB_DEF void *gb_alloc_copy_align(gbAllocator a, void const *src, isize size, isize alignment);
GB_DEF char *gb_alloc_str       (gbAllocator a, char const *str);
GB_DEF char *gb_alloc_str_len   (gbAllocator a, char const *str, isize len);


// NOTE(bill): These are very useful and the type cast has saved me from numerous bugs
#ifndef gb_alloc_item
#define gb_alloc_item(allocator_, Type)         (Type *)gb_alloc(allocator_, gb_size_of(Type))
#define gb_alloc_array(allocator_, Type, count) (Type *)gb_alloc(allocator_, gb_size_of(Type) * (count))
#endif

// NOTE(bill): Use this if you don't need a "fancy" resize allocation
GB_DEF void *gb_default_resize_align(gbAllocator a, void *ptr, isize old_size, isize new_size, isize alignment);



// TODO(bill): Probably use a custom heap allocator system that doesn't depend on malloc/free
// Base it off TCMalloc or something else? Or something entirely custom?
GB_DEF gbAllocator gb_heap_allocator(void);
GB_DEF GB_ALLOCATOR_PROC(gb_heap_allocator_proc);

// NOTE(bill): Yep, I use my own allocator system!
#ifndef gb_malloc
#define gb_malloc(sz) gb_alloc(gb_heap_allocator(), sz)
#define gb_mfree(ptr) gb_free(gb_heap_allocator(), ptr)
#endif

gb_inline gbAllocator gb_heap_allocator(void) {
	gbAllocator a;
	a.proc = gb_heap_allocator_proc;
	a.data = NULL;
	return a;
}
#endif











////////////////////////////////////////////////////////////////
//
// Char Functions
//
//

GB_DEF char gb_char_to_lower       (char c);
GB_DEF char gb_char_to_upper       (char c);
GB_DEF b32  gb_char_is_space       (char c);
GB_DEF b32  gb_char_is_digit       (char c);
GB_DEF b32  gb_char_is_hex_digit   (char c);
GB_DEF b32  gb_char_is_alpha       (char c);
GB_DEF b32  gb_char_is_alphanumeric(char c);
GB_DEF i1  gb_digit_to_int        (char c);
GB_DEF i1  gb_hex_digit_to_int    (char c);

// NOTE(bill): ASCII only
GB_DEF void gb_str_to_lower(char *str);
GB_DEF void gb_str_to_upper(char *str);

GB_DEF isize gb_strlen (char const *str);
GB_DEF isize gb_strnlen(char const *str, isize max_len);
GB_DEF i1   gb_strcmp (char const *s1, char const *s2);
GB_DEF i1   gb_strncmp(char const *s1, char const *s2, isize len);
GB_DEF char *gb_strcpy (char *dest, char const *source);
GB_DEF char *gb_strncpy(char *dest, char const *source, isize len);
GB_DEF isize gb_strlcpy(char *dest, char const *source, isize len);
GB_DEF char *gb_strrev (char *str); // NOTE(bill): ASCII only

// NOTE(bill): A less fucking crazy strtok!
GB_DEF char const *gb_strtok(char *output, char const *src, char const *delimit);

GB_DEF b32 gb_str_has_prefix(char const *str, char const *prefix);
GB_DEF b32 gb_str_has_suffix(char const *str, char const *suffix);

GB_DEF char const *gb_char_first_occurence(char const *str, char c);
GB_DEF char const *gb_char_last_occurence (char const *str, char c);

GB_DEF void gb_str_concat(char *dest, isize dest_len,
                          char const *src_a, isize src_a_len,
                          char const *src_b, isize src_b_len);

GB_DEF u64   gb_str_to_u64(char const *str, char **end_ptr, i1 base); // TODO(bill): Support more than just decimal and hexadecimal
GB_DEF i64   gb_str_to_i64(char const *str, char **end_ptr, i1 base); // TODO(bill): Support more than just decimal and hexadecimal
GB_DEF f32   gb_str_to_f32(char const *str, char **end_ptr);
GB_DEF f64   gb_str_to_f64(char const *str, char **end_ptr);
GB_DEF void  gb_i64_to_str(i64 value, char *string, i1 base);
GB_DEF void  gb_u64_to_str(u64 value, char *string, i1 base);


////////////////////////////////////////////////////////////////
//
// UTF-8 Handling
//
//

// NOTE(bill): Does not check if utf-8 string is valid
GB_DEF isize gb_utf8_strlen (u8 const *str);
GB_DEF isize gb_utf8_strnlen(u8 const *str, isize max_len);

// NOTE(bill): Windows doesn't handle 8 bit filenames well ('cause Micro$hit)
GB_DEF u16 *gb_utf8_to_ucs2    (u16 *buffer, isize len, u8 const *str);
GB_DEF u8 * gb_ucs2_to_utf8    (u8 *buffer, isize len, u16 const *str);
GB_DEF u16 *gb_utf8_to_ucs2_buf(u8 const *str);   // NOTE(bill): Uses locally persisting buffer
GB_DEF u8 * gb_ucs2_to_utf8_buf(u16 const *str); // NOTE(bill): Uses locally persisting buffer

// NOTE(bill): Returns size of codepoint in bytes
GB_DEF isize gb_utf8_decode        (u8 const *str, isize str_len, Rune *codepoint);
GB_DEF isize gb_utf8_codepoint_size(u8 const *str, isize str_len);
GB_DEF isize gb_utf8_encode_rune   (u8 buf[4], Rune r);

////////////////////////////////////////////////////////////////
//
// gbString - C Read-Only-Compatible
//
//
/*
Reasoning:

	By default, strings in C are null terminated which means you have to count
	the number of character up to the null character to calculate the length.
	Many "better" C string libraries will create a struct for a string.
	i.e.

	    struct String {
	    	Allocator allocator;
	        size_t    length;
	        size_t    capacity;
	        char *    cstring;
	    };

	This library tries to augment normal C strings in a better way that is still
	compatible with C-style strings.

	+--------+-----------------------+-----------------+
	| Header | Binary C-style String | Null Terminator |
	+--------+-----------------------+-----------------+
	         |
	         +-> Pointer returned by functions

	Due to the meta-data being stored before the string pointer and every gb string
	having an implicit null terminator, gb strings are full compatible with c-style
	strings and read-only functions.

Advantages:

    * gb strings can be passed to C-style string functions without accessing a struct
      member of calling a function, i.e.

          gb_printf("%s\n", gb_str);

      Many other libraries do either of these:

          gb_printf("%s\n", string->cstr);
          gb_printf("%s\n", get_cstring(string));

    * You can access each character just like a C-style string:

          gb_printf("%c %c\n", str[0], str[13]);

    * gb strings are singularly allocated. The meta-data is next to the character
      array which is better for the cache.

Disadvantages:

    * In the C version of these functions, many return the new string. i.e.
          str = gb_string_concatc(str, "another string");
      This could be changed to gb_string_concatc(&str, "another string"); but I'm still not sure.

	* This is incompatible with "gb_string.h" strings
*/

#if 0
#define GB_IMPLEMENTATION
#include "gb.h"
int main(int argc, char **argv) {
	gbString str = gb_string_make("Hello");
	gbString other_str = gb_string_make_length(", ", 2);
	str = gb_string_concat(str, other_str);
	str = gb_string_concatc(str, "world!");

	gb_printf("%s\n", str); // Hello, world!

	gb_printf("str length = %d\n", gb_string_length(str));

	str = gb_string_set(str, "Potato soup");
	gb_printf("%s\n", str); // Potato soup

	str = gb_string_set(str, "Hello");
	other_str = gb_string_set(other_str, "Pizza");
	if (gb_strings_are_equal(str, other_str))
		gb_printf("Not called\n");
	else
		gb_printf("Called\n");

	str = gb_string_set(str, "Ab.;!...AHello World       ??");
	str = gb_string_trim(str, "Ab.;!. ?");
	gb_printf("%s\n", str); // "Hello World"

	gb_string_free(str);
	gb_string_free(other_str);

	return 0;
}
#endif

// TODO(bill): Should this be a wrapper to gbArray(char) or this extra type safety better?
typedef char *gbString;

// NOTE(bill): If you only need a small string, just use a standard c string or change the size from isize to u16, etc.
typedef struct gbStringHeader {
	gbAllocator allocator;
	isize       length;
	isize       capacity;
} gbStringHeader;

#define GB_STRING_HEADER(str) (cast(gbStringHeader *)(str) - 1)

GB_DEF gbString gb_string_make_reserve   (gbAllocator a, isize capacity);
GB_DEF gbString gb_string_make           (gbAllocator a, char const *str);
GB_DEF gbString gb_string_make_length    (gbAllocator a, void const *str, isize num_bytes);
GB_DEF void     gb_string_free           (gbString str);
GB_DEF gbString gb_string_duplicate      (gbAllocator a, gbString const str);
GB_DEF isize    gb_string_length         (gbString const str);
GB_DEF isize    gb_string_capacity       (gbString const str);
GB_DEF isize    gb_string_available_space(gbString const str);
GB_DEF void     gb_string_clear          (gbString str);
GB_DEF gbString gb_string_concat         (gbString str, gbString const other);
GB_DEF gbString gb_string_concat_length  (gbString str, void const *other, isize num_bytes);
GB_DEF gbString gb_string_concatc        (gbString str, char const *other);
GB_DEF gbString gb_string_concat_rune    (gbString str, Rune r);
GB_DEF gbString gb_string_concat_fmt     (gbString str, char const *fmt, ...);
GB_DEF gbString gb_string_set            (gbString str, char const *cstr);
GB_DEF gbString gb_string_make_space_for (gbString str, isize add_len);
GB_DEF isize    gb_string_allocation_size(gbString const str);
GB_DEF b32      gb_string_are_equal      (gbString const lhs, gbString const rhs);
GB_DEF gbString gb_string_trim           (gbString str, char const *cut_set);
GB_DEF gbString gb_string_trim_space     (gbString str); // Whitespace ` \t\r\n\v\f`


////////////////////////////////////////////////////////////////
//
// File Handling
//


typedef u32 gbFileMode;
typedef enum gbFileModeFlag {
	gbFileMode_Read       = GB_BIT(0),
	gbFileMode_Write      = GB_BIT(1),
	gbFileMode_Append     = GB_BIT(2),
	gbFileMode_Rw         = GB_BIT(3),

	gbFileMode_Modes = gbFileMode_Read | gbFileMode_Write | gbFileMode_Append | gbFileMode_Rw,
} gbFileModeFlag;

// NOTE(bill): Only used internally and for the file operations
typedef enum gbSeekWhenceType {
	gbSeekWhence_Begin   = 0,
	gbSeekWhence_Current = 1,
	gbSeekWhence_End     = 2,
} gbSeekWhenceType;

typedef enum gbFileError {
	gbFileError_None,
	gbFileError_Invalid,
	gbFileError_InvalidFilename,
	gbFileError_Exists,
	gbFileError_NotExists,
	gbFileError_Permission,
	gbFileError_TruncationFailure,
} gbFileError;

typedef union gbFileDescriptor {
	void *  p;
	intptr  i;
	uintptr u;
} gbFileDescriptor;

typedef struct gbFileOperations gbFileOperations;

#define GB_FILE_OPEN_PROC(name)     gbFileError name(gbFileDescriptor *fd, gbFileOperations *ops, gbFileMode mode, char const *filename)
#define GB_FILE_READ_AT_PROC(name)  b32         name(gbFileDescriptor fd, void *buffer, isize size, i64 offset, isize *bytes_read)
#define GB_FILE_WRITE_AT_PROC(name) b32         name(gbFileDescriptor fd, void const *buffer, isize size, i64 offset, isize *bytes_written)
#define GB_FILE_SEEK_PROC(name)     b32         name(gbFileDescriptor fd, i64 offset, gbSeekWhenceType whence, i64 *new_offset)
#define GB_FILE_CLOSE_PROC(name)    void        name(gbFileDescriptor fd)
typedef GB_FILE_OPEN_PROC(gbFileOpenProc);
typedef GB_FILE_READ_AT_PROC(gbFileReadProc);
typedef GB_FILE_WRITE_AT_PROC(gbFileWriteProc);
typedef GB_FILE_SEEK_PROC(gbFileSeekProc);
typedef GB_FILE_CLOSE_PROC(gbFileCloseProc);

struct gbFileOperations {
	gbFileReadProc  *read_at;
	gbFileWriteProc *write_at;
	gbFileSeekProc  *seek;
	gbFileCloseProc *close;
};

//gbFileOperations const gbDefaultFileOperations;


// typedef struct gbDirInfo {
// 	u8 *buf;
// 	isize buf_count;
// 	isize buf_pos;
// } gbDirInfo;

typedef u64 gbFileTime;

typedef struct gbFile {
	gbFileOperations ops;
	gbFileDescriptor fd;
	char const *     filename;
	gbFileTime       last_write_time;
	// gbDirInfo *   dir_info; // TODO(bill): Get directory info
} gbFile;

// TODO(bill): gbAsyncFile

typedef enum gbFileStandardType {
	gbFileStandard_Input,
	gbFileStandard_Output,
	gbFileStandard_Error,

	gbFileStandard_Count,
} gbFileStandardType;

GB_DEF gbFile *const gb_file_get_standard(gbFileStandardType std);

GB_DEF gbFileError gb_file_create        (gbFile *file, char const *filename);
GB_DEF gbFileError gb_file_open          (gbFile *file, char const *filename);
GB_DEF gbFileError gb_file_open_mode     (gbFile *file, gbFileMode mode, char const *filename);
GB_DEF gbFileError gb_file_new           (gbFile *file, gbFileDescriptor fd, gbFileOperations ops, char const *filename);
GB_DEF b32         gb_file_read_at_check (gbFile *file, void *buffer, isize size, i64 offset, isize *bytes_read);
GB_DEF b32         gb_file_write_at_check(gbFile *file, void const *buffer, isize size, i64 offset, isize *bytes_written);
GB_DEF b32         gb_file_read_at       (gbFile *file, void *buffer, isize size, i64 offset);
GB_DEF b32         gb_file_write_at      (gbFile *file, void const *buffer, isize size, i64 offset);
GB_DEF i64         gb_file_seek          (gbFile *file, i64 offset);
GB_DEF i64         gb_file_seek_to_end   (gbFile *file);
GB_DEF i64         gb_file_skip          (gbFile *file, i64 bytes); // NOTE(bill): Skips a certain amount of bytes
GB_DEF i64         gb_file_tell          (gbFile *file);
GB_DEF gbFileError gb_file_close         (gbFile *file);
GB_DEF b32         gb_file_read          (gbFile *file, void *buffer, isize size);
GB_DEF b32         gb_file_write         (gbFile *file, void const *buffer, isize size);
GB_DEF i64         gb_file_size          (gbFile *file);
GB_DEF char const *gb_file_name          (gbFile *file);
GB_DEF gbFileError gb_file_truncate      (gbFile *file, i64 size);
GB_DEF b32         gb_file_has_changed   (gbFile *file); // NOTE(bill): Changed since lasted checked
// TODO(bill):
// gbFileError gb_file_temp(gbFile *file);
//

typedef struct gbFileContents {
	gbAllocator allocator;
	void *      data;
	isize       size;
} gbFileContents;


GB_DEF gbFileContents gb_file_read_contents(gbAllocator a, b32 zero_terminate, char const *filepath);
GB_DEF void           gb_file_free_contents(gbFileContents *fc);


// TODO(bill): Should these have different na,es as they do not take in a gbFile * ???
GB_DEF b32        gb_file_exists         (char const *filepath);
GB_DEF gbFileTime gb_file_last_write_time(char const *filepath);
GB_DEF b32        gb_file_copy           (char const *existing_filename, char const *new_filename, b32 fail_if_exists);
GB_DEF b32        gb_file_move           (char const *existing_filename, char const *new_filename);
GB_DEF b32        gb_file_remove         (char const *filename);


#ifndef GB_PATH_SEPARATOR
	#if defined(GB_SYSTEM_WINDOWS)
		#define GB_PATH_SEPARATOR '\\'
	#else
		#define GB_PATH_SEPARATOR '/'
	#endif
#endif

GB_DEF b32         gb_path_is_absolute  (char const *path);
GB_DEF b32         gb_path_is_relative  (char const *path);
GB_DEF b32         gb_path_is_root      (char const *path);
GB_DEF char const *gb_path_base_name    (char const *path);
GB_DEF char const *gb_path_extension    (char const *path);
GB_DEF char *      gb_path_get_full_name(gbAllocator a, char const *path);


////////////////////////////////////////////////////////////////
//
// Printing
//
//

GB_DEF isize gb_printf        (char const *fmt, ...) GB_PRINTF_ARGS(1);
GB_DEF isize gb_printf_va     (char const *fmt, va_list va);
GB_DEF isize gb_printf_err    (char const *fmt, ...) GB_PRINTF_ARGS(1);
GB_DEF isize gb_printf_err_va (char const *fmt, va_list va);
GB_DEF isize gb_fprintf       (gbFile *f, char const *fmt, ...) GB_PRINTF_ARGS(2);
GB_DEF isize gb_fprintf_va    (gbFile *f, char const *fmt, va_list va);

GB_DEF char *gb_bprintf    (char const *fmt, ...) GB_PRINTF_ARGS(1); // NOTE(bill): A locally persisting buffer is used internally
GB_DEF char *gb_bprintf_va (char const *fmt, va_list va);            // NOTE(bill): A locally persisting buffer is used internally
GB_DEF isize gb_snprintf   (char *str, isize n, char const *fmt, ...) GB_PRINTF_ARGS(3);
GB_DEF isize gb_snprintf_va(char *str, isize n, char const *fmt, va_list va);

////////////////////////////////////////////////////////////////
//
// DLL Handling
//
//

typedef void *gbDllHandle;
typedef void (*gbDllProc)(void);

GB_DEF gbDllHandle gb_dll_load        (char const *filepath);
GB_DEF b32         gb_dll_unload      (gbDllHandle dll);
GB_DEF gbDllProc   gb_dll_proc_address(gbDllHandle dll, char const *proc_name);


////////////////////////////////////////////////////////////////
//
// Time
//
//


#if defined(__cplusplus)
//}
EXTERN_C_END
#endif

#endif // GB_INCLUDE_GB_H






////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
// Implementation
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
// It's turtles all the way down!
////////////////////////////////////////////////////////////////
#if defined(GB_IMPLEMENTATION) && !defined(GB_IMPLEMENTATION_DONE)
#define GB_IMPLEMENTATION_DONE

#if defined(__cplusplus)
EXTERN_C_BEGIN
#endif

#if !AD_IS_DRIVER
GB_ALLOCATOR_PROC(gb_heap_allocator_proc) {
	void *ptr = NULL;
	gb_unused(allocator_data);
	gb_unused(old_size);
 // TODO(bill): Throughly test!
	switch (type) {
#if defined(GB_COMPILER_MSVC)
  case gbAllocation_Alloc:
		ptr = _aligned_malloc(size, alignment);
		if (flags & gbAllocatorFlag_ClearToZero)
			gb_zero_size(ptr, size);
		break;
  case gbAllocation_Free:
		_aligned_free(old_memory);
		break;
  case gbAllocation_Resize:
		ptr = _aligned_realloc(old_memory, size, alignment);
		break;
  
#elif defined(GB_SYSTEM_LINUX)
  // TODO(bill): *nix version that's decent
  case gbAllocation_Alloc: {
   ptr = aligned_alloc(alignment, size);
   // ptr = malloc(size+alignment);
   
   if (flags & gbAllocatorFlag_ClearToZero) {
    gb_zero_size(ptr, size);
   }
  } break;
  
  case gbAllocation_Free: {
   free(old_memory);
  } break;
  
  case gbAllocation_Resize: {
   // ptr = realloc(old_memory, size);
   ptr = gb_default_resize_align(gb_heap_allocator(), old_memory, old_size, size, alignment);
  } break;
#else
  // TODO(bill): *nix version that's decent
  case gbAllocation_Alloc: {
   posix_memalign(&ptr, alignment, size);
   
   if (flags & gbAllocatorFlag_ClearToZero) {
    gb_zero_size(ptr, size);
   }
  } break;
  
  case gbAllocation_Free: {
   free(old_memory);
  } break;
  
  case gbAllocation_Resize: {
   ptr = gb_default_resize_align(gb_heap_allocator(), old_memory, old_size, size, alignment);
  } break;
#endif
  
  case gbAllocation_FreeAll:
		break;
	}
 
	return ptr;
}
#endif




#if defined(__GCC__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"
#pragma GCC diagnostic ignored "-Wmissing-braces"
#endif

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4201)
#pragma warning(disable:4127) // Conditional expression is constant
#endif

#define GB__ONES        (cast(usize)-1/U8_MAX)
#define GB__HIGHS       (GB__ONES * (U8_MAX/2+1))
#define GB__HAS_ZERO(x) ((x)-GB__ONES & ~(x) & GB__HIGHS)

#if !AD_IS_DRIVER
b32 gb_is_power_of_two(isize x) {
	if (x <= 0)
		return false;
	return !(x & (x-1));
}

gb_inline void *gb_align_forward(void *ptr, isize alignment) {
	uintptr p;
 
	GB_ASSERT(gb_is_power_of_two(alignment));
 
	p = cast(uintptr)ptr;
	return cast(void *)((p + (alignment-1)) &~ (alignment-1));
}



gb_inline void *      gb_pointer_add      (void *ptr, isize bytes)             { return cast(void *)(cast(u8 *)ptr + bytes); }
gb_inline void *      gb_pointer_sub      (void *ptr, isize bytes)             { return cast(void *)(cast(u8 *)ptr - bytes); }
gb_inline void const *gb_pointer_add_const(void const *ptr, isize bytes)       { return cast(void const *)(cast(u8 const *)ptr + bytes); }
gb_inline void const *gb_pointer_sub_const(void const *ptr, isize bytes)       { return cast(void const *)(cast(u8 const *)ptr - bytes); }
gb_inline isize       gb_pointer_diff     (void const *begin, void const *end) { return cast(isize)(cast(u8 const *)end - cast(u8 const *)begin); }

gb_inline void gb_zero_size(void *ptr, isize size) { gb_memset(ptr, 0, size); }


#if defined(_MSC_VER)
#pragma intrinsic(__movsb)
#endif

internal void *gb_memcopy(void *dest, void const *source, isize n) {
	if (dest == NULL) { return NULL; }
 
#if defined(_MSC_VER)
	// TODO(bill): Is this good enough?
	__movsb(cast(u8 *)dest, cast(u8 *)source, n);
// #elif defined(GB_SYSTEM_OSX) || defined(GB_SYSTEM_UNIX)
	// NOTE(zangent): I assume there's a reason this isn't being used elsewhere,
	//   but casting pointers as arguments to an __asm__ call is considered an
	//   error on MacOS and (I think) Linux
	// TODO(zangent): Figure out how to refactor the asm code so it works on MacOS,
	//   since this is probably not the way the author intended this to work.
	// memcpy(dest, source, n);
#elif defined(GB_CPU_X86)
	void *dest_copy = dest;
	__asm__ __volatile__("rep movsb" : "+D"(dest_copy), "+S"(source), "+c"(n) : : "memory");
#endif
 
	return dest;
}

internal void *gb_memmove(void *dest, void const *source, isize n) {
	u8 *d = cast(u8 *)dest;
	u8 const *s = cast(u8 const *)source;

	if (dest == NULL) {
		return NULL;
	}

	if (d == s) {
		return d;
	}
	if (s+n <= d || d+n <= s) { // NOTE(bill): Non-overlapping
		return gb_memcopy(d, s, n);
	}

	if (d < s) {
		if (cast(uintptr)s % gb_size_of(isize) == cast(uintptr)d % gb_size_of(isize)) {
			while (cast(uintptr)d % gb_size_of(isize)) {
				if (!n--) return dest;
				*d++ = *s++;
			}
			while (n>=gb_size_of(isize)) {
				*cast(isize *)d = *cast(isize *)s;
				n -= gb_size_of(isize);
				d += gb_size_of(isize);
				s += gb_size_of(isize);
			}
		}
		for (; n; n--) *d++ = *s++;
	} else {
		if ((cast(uintptr)s % gb_size_of(isize)) == (cast(uintptr)d % gb_size_of(isize))) {
			while (cast(uintptr)(d+n) % gb_size_of(isize)) {
				if (!n--)
					return dest;
				d[n] = s[n];
			}
			while (n >= gb_size_of(isize)) {
				n -= gb_size_of(isize);
				*cast(isize *)(d+n) = *cast(isize *)(s+n);
			}
		}
		while (n) n--, d[n] = s[n];
	}
 
	return dest;
}

internal void *gb_memset(void *dest, u8 c, isize n) {
	u8 *s = cast(u8 *)dest;
	isize k;
	u32 c32 = ((u32)-1)/255 * c;

	if (dest == NULL) {
		return NULL;
	}

	if (n == 0)
		return dest;
	s[0] = s[n-1] = c;
	if (n < 3)
		return dest;
	s[1] = s[n-2] = c;
	s[2] = s[n-3] = c;
	if (n < 7)
		return dest;
	s[3] = s[n-4] = c;
	if (n < 9)
		return dest;

	k = -cast(intptr)s & 3;
	s += k;
	n -= k;
	n &= -4;

	*cast(u32 *)(s+0) = c32;
	*cast(u32 *)(s+n-4) = c32;
	if (n < 9) {
		return dest;
	}
	*cast(u32 *)(s +  4)    = c32;
	*cast(u32 *)(s +  8)    = c32;
	*cast(u32 *)(s+n-12) = c32;
	*cast(u32 *)(s+n- 8) = c32;
	if (n < 25) {
		return dest;
	}
	*cast(u32 *)(s + 12) = c32;
	*cast(u32 *)(s + 16) = c32;
	*cast(u32 *)(s + 20) = c32;
	*cast(u32 *)(s + 24) = c32;
	*cast(u32 *)(s+n-28) = c32;
	*cast(u32 *)(s+n-24) = c32;
	*cast(u32 *)(s+n-20) = c32;
	*cast(u32 *)(s+n-16) = c32;

	k = 24 + (cast(uintptr)s & 4);
	s += k;
	n -= k;


	{
		u64 c64 = (cast(u64)c32 << 32) | c32;
		while (n > 31) {
			*cast(u64 *)(s+0) = c64;
			*cast(u64 *)(s+8) = c64;
			*cast(u64 *)(s+16) = c64;
			*cast(u64 *)(s+24) = c64;

			n -= 32;
			s += 32;
		}
	}

	return dest;
}

gb_inline i1 gb_memcompare(void const *s1, void const *s2, isize size)
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

void gb_memswap(void *i, void *j, isize size) {
	if (i == j) return;

	if (size == 4) {
		gb_swap(u32, *cast(u32 *)i, *cast(u32 *)j);
	} else if (size == 8) {
		gb_swap(u64, *cast(u64 *)i, *cast(u64 *)j);
	} else if (size < 8) {
		u8 *a = cast(u8 *)i;
		u8 *b = cast(u8 *)j;
		if (a != b) {
			while (size--) {
				gb_swap(u8, *a, *b);
				a++, b++;
			}
		}
	} else {
		char buffer[256];

		// TODO(bill): Is the recursion ever a problem?
		while (size > gb_size_of(buffer)) {
			gb_memswap(i, j, gb_size_of(buffer));
			i = gb_pointer_add(i, gb_size_of(buffer));
			j = gb_pointer_add(j, gb_size_of(buffer));
			size -= gb_size_of(buffer);
		}

		gb_memcopy(buffer, i,      size);
		gb_memcopy(i,      j,      size);
		gb_memcopy(j,      buffer, size);
	}
}



void const *gb_memchr(void const *data, u8 c, isize n) {
	u8 const *s = cast(u8 const *)data;
	while ((cast(uintptr)s & (sizeof(usize)-1)) &&
	       n && *s != c) {
		s++;
		n--;
	}
	if (n && *s != c) {
		isize const *w;
		isize k = GB__ONES * c;
		w = cast(isize const *)s;
		while (n >= gb_size_of(isize) && !GB__HAS_ZERO(*w ^ k)) {
			w++;
			n -= gb_size_of(isize);
		}
		s = cast(u8 const *)w;
		while (n && *s != c) {
			s++;
			n--;
		}
	}

	return n ? cast(void const *)s : NULL;
}


void const *gb_memrchr(void const *data, u8 c, isize n) {
	u8 const *s = cast(u8 const *)data;
	while (n--) {
		if (s[n] == c)
			return cast(void const *)(s + n);
	}
	return NULL;
}



gb_inline void *gb_alloc_align (gbAllocator a, isize size, isize alignment)                                { return a.proc(a.data, gbAllocation_Alloc, size, alignment, NULL, 0, GB_DEFAULT_ALLOCATOR_FLAGS); }
gb_inline void *gb_alloc       (gbAllocator a, isize size)                                                 { return gb_alloc_align(a, size, GB_DEFAULT_MEMORY_ALIGNMENT); }
gb_inline void  gb_free        (gbAllocator a, void *ptr)                                                  { if (ptr != NULL) a.proc(a.data, gbAllocation_Free, 0, 0, ptr, 0, GB_DEFAULT_ALLOCATOR_FLAGS); }
gb_inline void  gb_free_all    (gbAllocator a)                                                             { a.proc(a.data, gbAllocation_FreeAll, 0, 0, NULL, 0, GB_DEFAULT_ALLOCATOR_FLAGS); }
gb_inline void *gb_resize      (gbAllocator a, void *ptr, isize old_size, isize new_size)                  { return gb_resize_align(a, ptr, old_size, new_size, GB_DEFAULT_MEMORY_ALIGNMENT); }
gb_inline void *gb_resize_align(gbAllocator a, void *ptr, isize old_size, isize new_size, isize alignment) { return a.proc(a.data, gbAllocation_Resize, new_size, alignment, ptr, old_size, GB_DEFAULT_ALLOCATOR_FLAGS); }

gb_inline void *gb_alloc_copy      (gbAllocator a, void const *src, isize size) {
	return gb_memcopy(gb_alloc(a, size), src, size);
}
gb_inline void *gb_alloc_copy_align(gbAllocator a, void const *src, isize size, isize alignment) {
	return gb_memcopy(gb_alloc_align(a, size, alignment), src, size);
}

gb_inline char *gb_alloc_str(gbAllocator a, char const *str) {
	return gb_alloc_str_len(a, str, strlen(str));
}

gb_inline char *gb_alloc_str_len(gbAllocator a, char const *str, isize len) {
	char *result;
	result = cast(char *)gb_alloc_copy(a, str, len+1);
	result[len] = '\0';
	return result;
}


gb_inline void *gb_default_resize_align(gbAllocator a, void *old_memory, isize old_size, isize new_size, isize alignment) {
	if (!old_memory) return gb_alloc_align(a, new_size, alignment);

	if (new_size == 0) {
		gb_free(a, old_memory);
		return NULL;
	}

	if (new_size < old_size)
		new_size = old_size;

	if (old_size == new_size) {
		return old_memory;
	} else {
		void *new_memory = gb_alloc_align(a, new_size, alignment);
		if (!new_memory) return NULL;
		gb_memmove(new_memory, old_memory, gb_min(new_size, old_size));
		gb_free(a, old_memory);
		return new_memory;
	}
}
#endif





#if !AD_IS_DRIVER
////////////////////////////////////////////////////////////////
//
// Time
//
//

#if defined(GB_COMPILER_MSVC) && !defined(__clang__)
gb_inline u64 gb_rdtsc(void) { return __rdtsc(); }
#elif defined(__x86_64__)
gb_inline u64 gb_rdtsc(void) {
 u32 hi, lo;
 __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
 return (cast(u64)lo) | ((cast(u64)hi)<<32);
}
#endif

#if defined(GB_SYSTEM_WINDOWS)

gb_inline f64 gb_time_now(void) {
 gb_local_persist LARGE_INTEGER win32_perf_count_freq = {0};
 f64 result;
 LARGE_INTEGER counter;
 if (!win32_perf_count_freq.QuadPart) {
  QueryPerformanceFrequency(&win32_perf_count_freq);
  GB_ASSERT(win32_perf_count_freq.QuadPart != 0);
 }
 
 QueryPerformanceCounter(&counter);
 
 result = counter.QuadPart / cast(f64)(win32_perf_count_freq.QuadPart);
 return result;
}

gb_inline u64 gb_utc_time_now(void) {
 FILETIME ft;
 ULARGE_INTEGER li;
 
 GetSystemTimeAsFileTime(&ft);
 li.LowPart = ft.dwLowDateTime;
 li.HighPart = ft.dwHighDateTime;
 
 return li.QuadPart/10;
}

gb_inline void gb_sleep_ms(u32 ms) { Sleep(ms); }

#else

gb_global f64 gb__timebase  = 0.0;
gb_global u64 gb__timestart = 0;

gb_inline f64 gb_time_now(void) {
#if defined(GB_SYSTEM_OSX)
 f64 result;
 
 if (!gb__timestart) {
  mach_timebase_info_data_t tb = {0};
  mach_timebase_info(&tb);
  gb__timebase = tb.numer;
  gb__timebase /= tb.denom;
  gb__timestart = mach_absolute_time();
 }
 
 // NOTE(bill): mach_absolute_time() returns things in nanoseconds
 result = 1.0e-9 * (mach_absolute_time() - gb__timestart) * gb__timebase;
 return result;
#else
 struct timespec t;
 f64 result;
 
 // IMPORTANT TODO(bill): THIS IS A HACK
 clock_gettime(1 /*CLOCK_MONOTONIC*/, &t);
 result = t.tv_sec + 1.0e-9 * t.tv_nsec;
 return result;
#endif
}

gb_inline u64 gb_utc_time_now(void) {
 struct timespec t;
#if defined(GB_SYSTEM_OSX)
 clock_serv_t cclock;
 mach_timespec_t mts;
 host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
 clock_get_time(cclock, &mts);
 mach_port_deallocate(mach_task_self(), cclock);
 t.tv_sec = mts.tv_sec;
 t.tv_nsec = mts.tv_nsec;
#else
 // IMPORTANT TODO(bill): THIS IS A HACK
 clock_gettime(0 /*CLOCK_REALTIME*/, &t);
#endif
 return cast(u64)t.tv_sec * 1000000ull + t.tv_nsec/1000 + 11644473600000000ull;
}

gb_inline void gb_sleep_ms(u32 ms) {
 struct timespec req = {cast(time_t)ms/1000, cast(long)((ms%1000)*1000000)};
 struct timespec rem = {0, 0};
 nanosleep(&req, &rem);
}

#endif
#endif












////////////////////////////////////////////////////////////////
//
// Custom Allocation
//
//











////////////////////////////////////////////////////////////////
//
// Char things
//
//




#if !AD_IS_DRIVER
gb_inline char gb_char_to_lower(char c) {
	if (c >= 'A' && c <= 'Z')
		return 'a' + (c - 'A');
	return c;
}

gb_inline char gb_char_to_upper(char c) {
	if (c >= 'a' && c <= 'z')
		return 'A' + (c - 'a');
	return c;
}

inline b32 gb_char_is_space(char c) {
	if (c == ' '  ||
	    c == '\t' ||
	    c == '\n' ||
	    c == '\r' ||
	    c == '\f' ||
	    c == '\v')
	    return true;
	return false;
}

gb_inline b32 gb_char_is_digit(char c) {
	if (c >= '0' && c <= '9')
		return true;
	return false;
}

gb_inline b32 gb_char_is_hex_digit(char c) {
	if (gb_char_is_digit(c) ||
	    (c >= 'a' && c <= 'f') ||
	    (c >= 'A' && c <= 'F'))
	    return true;
	return false;
}

gb_inline b32 gb_char_is_alpha(char c) {
	if ((c >= 'A' && c <= 'Z') ||
	    (c >= 'a' && c <= 'z'))
	    return true;
	return false;
}

gb_inline b32 gb_char_is_alphanumeric(char c) {
	return gb_char_is_alpha(c) || gb_char_is_digit(c);
}

gb_inline i1 gb_digit_to_int(char c) {
	return gb_char_is_digit(c) ? c - '0' : c - 'W';
}

gb_inline i1 gb_hex_digit_to_int(char c) {
	if (gb_char_is_digit(c))
		return gb_digit_to_int(c);
	else if (gb_is_between(c, 'a', 'f'))
		return c - 'a' + 10;
	else if (gb_is_between(c, 'A', 'F'))
		return c - 'A' + 10;
	return -1;
}




inline void gb_str_to_lower(char *str) {
	if (!str) return;
	while (*str) {
		*str = gb_char_to_lower(*str);
		str++;
	}
}

inline void gb_str_to_upper(char *str) {
	if (!str) return;
	while (*str) {
		*str = gb_char_to_upper(*str);
		str++;
	}
}


inline isize gb_strlen(char const *str) {
	char const *begin = str;
	isize const *w;
	if (str == NULL)  {
		return 0;
	}
	while (cast(uintptr)str % sizeof(usize)) {
		if (!*str)
			return str - begin;
		str++;
	}
	w = cast(isize const *)str;
	while (!GB__HAS_ZERO(*w)) {
		w++;
	}
	str = cast(char const *)w;
	while (*str) {
		str++;
	}
	return str - begin;
}

inline isize gb_strnlen(char const *str, isize max_len) {
	char const *end = cast(char const *)gb_memchr(str, 0, max_len);
	if (end) {
		return end - str;
	}
	return max_len;
}


inline isize gb_utf8_strlen(u8 const *str) {
	isize count = 0;
	for (; *str; count++) {
		u8 c = *str;
		isize inc = 0;
  if (c < 0x80)           inc = 1;
		else if ((c & 0xe0) == 0xc0) inc = 2;
		else if ((c & 0xf0) == 0xe0) inc = 3;
		else if ((c & 0xf8) == 0xf0) inc = 4;
		else return -1;
  
		str += inc;
	}
	return count;
}

inline isize gb_utf8_strnlen(u8 const *str, isize max_len) {
	isize count = 0;
	for (; *str && max_len > 0; count++) {
		u8 c = *str;
		isize inc = 0;
  if (c < 0x80)           inc = 1;
		else if ((c & 0xe0) == 0xc0) inc = 2;
		else if ((c & 0xf0) == 0xe0) inc = 3;
		else if ((c & 0xf8) == 0xf0) inc = 4;
		else return -1;
  
		str += inc;
		max_len -= inc;
	}
	return count;
}


inline i1 gb_strcmp(char const *s1, char const *s2) {
	while (*s1 && (*s1 == *s2)) {
		s1++, s2++;
	}
	return *(u8 *)s1 - *(u8 *)s2;
}

inline char *gb_strcpy(char *dest, char const *source) {
	GB_ASSERT_NOT_NULL(dest);
	if (source) {
		char *str = dest;
		while (*source) *str++ = *source++;
	}
	return dest;
}


inline char *gb_strncpy(char *dest, char const *source, isize len) {
	GB_ASSERT_NOT_NULL(dest);
	if (source) {
		char *str = dest;
		while (len > 0 && *source) {
			*str++ = *source++;
			len--;
		}
		while (len > 0) {
			*str++ = '\0';
			len--;
		}
	}
	return dest;
}

inline isize gb_strlcpy(char *dest, char const *source, isize len) {
	isize result = 0;
	GB_ASSERT_NOT_NULL(dest);
	if (source) {
		char const *source_start = source;
		char *str = dest;
		while (len > 0 && *source) {
			*str++ = *source++;
			len--;
		}
		while (len > 0) {
			*str++ = '\0';
			len--;
		}
  
		result = source - source_start;
	}
	return result;
}

inline char *gb_strrev(char *str) {
	isize len = strlen(str);
	char *a = str + 0;
	char *b = str + len-1;
	len /= 2;
	while (len--) {
		gb_swap(char, *a, *b);
		a++, b--;
	}
	return str;
}




inline i1 gb_strncmp(char const *s1, char const *s2, isize len) {
	for (; len > 0;
	     s1++, s2++, len--) {
		if (*s1 != *s2) {
			return ((s1 < s2) ? -1 : +1);
		} else if (*s1 == '\0') {
			return 0;
		}
	}
	return 0;
}


gb_inline char const *gb_strtok(char *output, char const *src, char const *delimit) {
	while (*src && gb_char_first_occurence(delimit, *src) != NULL) {
		*output++ = *src++;
	}
 
	*output = 0;
	return *src ? src+1 : src;
}

gb_inline b32 gb_str_has_prefix(char const *str, char const *prefix) {
	while (*prefix) {
		if (*str++ != *prefix++) {
			return false;
		}
	}
	return true;
}

gb_inline b32 gb_str_has_suffix(char const *str, char const *suffix) {
	isize i = strlen(str);
	isize j = strlen(suffix);
	if (j <= i) {
		return gb_strcmp(str+i-j, suffix) == 0;
	}
	return false;
}




gb_inline char const *gb_char_first_occurence(char const *s, char c) {
	char ch = c;
	for (; *s != ch; s++) {
		if (*s == '\0') {
			return NULL;
		}
	}
	return s;
}


gb_inline char const *gb_char_last_occurence(char const *s, char c) {
	char const *result = NULL;
	do {
		if (*s == c) {
			result = s;
		}
	} while (*s++);
 
	return result;
}



gb_inline void gb_str_concat(char *dest, isize dest_len,
                             char const *src_a, isize src_a_len,
                             char const *src_b, isize src_b_len) {
	GB_ASSERT(dest_len >= src_a_len+src_b_len+1);
	if (dest) {
		gb_memcopy(dest, src_a, src_a_len);
		gb_memcopy(dest+src_a_len, src_b, src_b_len);
		dest[src_a_len+src_b_len] = '\0';
	}
}


gb_internal isize gb__scan_i64(char const *text, i1 base, i64 *value) {
	char const *text_begin = text;
	i64 result = 0;
	b32 negative = false;
 
	if (*text == '-') {
		negative = true;
		text++;
	}
 
	if (base == 16 && gb_strncmp(text, "0x", 2) == 0) {
		text += 2;
	}
 
	for (;;) {
		i64 v;
		if (gb_char_is_digit(*text)) {
			v = *text - '0';
		} else if (base == 16 && gb_char_is_hex_digit(*text)) {
			v = gb_hex_digit_to_int(*text);
		} else {
			break;
		}
  
		result *= base;
		result += v;
		text++;
	}
 
	if (value) {
		if (negative) result = -result;
		*value = result;
	}
 
	return (text - text_begin);
}

gb_internal isize gb__scan_u64(char const *text, i1 base, u64 *value) {
	char const *text_begin = text;
	u64 result = 0;
 
	if (base == 16 && gb_strncmp(text, "0x", 2) == 0) {
		text += 2;
	}
 
	for (;;) {
		u64 v;
		if (gb_char_is_digit(*text)) {
			v = *text - '0';
		} else if (base == 16 && gb_char_is_hex_digit(*text)) {
			v = gb_hex_digit_to_int(*text);
		} else {
			break;
		}
  
		result *= base;
		result += v;
		text++;
	}
 
	if (value) *value = result;
	return (text - text_begin);
}


// TODO(bill): Make better
u64 gb_str_to_u64(char const *str, char **end_ptr, i1 base) {
	isize len;
	u64 value = 0;
 
	if (!base) {
		if ((strlen(str) > 2) && (gb_strncmp(str, "0x", 2) == 0)) {
			base = 16;
		} else {
			base = 10;
		}
	}
 
	len = gb__scan_u64(str, base, &value);
	if (end_ptr) *end_ptr = (char *)str + len;
	return value;
}

i64 gb_str_to_i64(char const *str, char **end_ptr, i1 base) {
	isize len;
	i64 value;
 
	if (!base) {
		if ((strlen(str) > 2) && (gb_strncmp(str, "0x", 2) == 0)) {
			base = 16;
		} else {
			base = 10;
		}
	}
 
	len = gb__scan_i64(str, base, &value);
	if (end_ptr) *end_ptr = (char *)str + len;
	return value;
}

// TODO(bill): Are these good enough for characters?
gb_global char const gb__num_to_char_table[] =
"0123456789"
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"@$";

gb_inline void gb_i64_to_str(i64 value, char *string, i1 base) {
	char *buf = string;
	b32 negative = false;
	u64 v;
	if (value < 0) {
		negative = true;
		value = -value;
	}
	v = cast(u64)value;
	if (v != 0) {
		while (v > 0) {
			*buf++ = gb__num_to_char_table[v % base];
			v /= base;
		}
	} else {
		*buf++ = '0';
	}
	if (negative) {
		*buf++ = '-';
	}
	*buf = '\0';
	gb_strrev(string);
}



gb_inline void gb_u64_to_str(u64 value, char *string, i1 base) {
	char *buf = string;
 
	if (value) {
		while (value > 0) {
			*buf++ = gb__num_to_char_table[value % base];
			value /= base;
		}
	} else {
		*buf++ = '0';
	}
	*buf = '\0';
 
	gb_strrev(string);
}

gb_inline f32 gb_str_to_f32(char const *str, char **end_ptr) {
	f64 f = gb_str_to_f64(str, end_ptr);
	f32 r = cast(f32)f;
	return r;
}

gb_inline f64 gb_str_to_f64(char const *str, char **end_ptr_out) {
	f64 result, value, sign, scale;
	i1 frac;
 
	while ( gb_char_is_space(*str) ) {
		str++;
	}
 
	sign = 1.0;
	if (*str == '-') {
		sign = -1.0;
		str++;
	} else if (*str == '+') {
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
 
	frac = 0;
	scale = 1.0;
	if ((*str == 'e') || (*str == 'E')) {
		u32 exp;
  
		str++;
		if (*str == '-') {
			frac = 1;
			str++;
		} else if (*str == '+') {
			str++;
		}
  
		for (exp = 0; gb_char_is_digit(*str); str++) {
			exp = exp * 10 + (*str-'0');
		}
		if (exp > 308) exp = 308;
  
		while (exp >= 50) { scale *= 1e50; exp -= 50; }
		while (exp >=  8) { scale *= 1e8;  exp -=  8; }
		while (exp >   0) { scale *= 10.0; exp -=  1; }
	}
 
	result = sign * (frac ? (value / scale) : (value * scale));
 
	if (end_ptr_out) *end_ptr_out = cast(char *)str;
 
	return result;
}







gb_inline void gb__set_string_length  (gbString str, isize len) { GB_STRING_HEADER(str)->length = len; }
gb_inline void gb__set_string_capacity(gbString str, isize cap) { GB_STRING_HEADER(str)->capacity = cap; }


gbString gb_string_make_reserve(gbAllocator a, isize capacity) {
	isize header_size = gb_size_of(gbStringHeader);
	void *ptr = gb_alloc(a, header_size + capacity + 1);
 
	gbString str;
	gbStringHeader *header;
 
	if (ptr == NULL) return NULL;
	gb_zero_size(ptr, header_size + capacity + 1);
 
	str = cast(char *)ptr + header_size;
	header = GB_STRING_HEADER(str);
	header->allocator = a;
	header->length    = 0;
	header->capacity  = capacity;
	str[capacity] = '\0';
 
	return str;
}


gb_inline gbString gb_string_make(gbAllocator a, char const *str) {
	isize len = str ? strlen(str) : 0;
	return gb_string_make_length(a, str, len);
}

gbString gb_string_make_length(gbAllocator a, void const *init_str, isize num_bytes) {
	isize header_size = gb_size_of(gbStringHeader);
	void *ptr = gb_alloc(a, header_size + num_bytes + 1);
 
	gbString str;
	gbStringHeader *header;
 
	if (ptr == NULL) return NULL;
	if (!init_str) gb_zero_size(ptr, header_size + num_bytes + 1);
 
	str = cast(char *)ptr + header_size;
	header = GB_STRING_HEADER(str);
	header->allocator = a;
	header->length    = num_bytes;
	header->capacity  = num_bytes;
	if (num_bytes && init_str) {
		gb_memcopy(str, init_str, num_bytes);
	}
	str[num_bytes] = '\0';
 
	return str;
}

gb_inline void gb_string_free(gbString str) {
	if (str) {
		gbStringHeader *header = GB_STRING_HEADER(str);
		gb_free(header->allocator, header);
	}
 
}

gb_inline gbString gb_string_duplicate(gbAllocator a, gbString const str) { return gb_string_make_length(a, str, gb_string_length(str)); }

gb_inline isize gb_string_length  (gbString const str) { return GB_STRING_HEADER(str)->length; }
gb_inline isize gb_string_capacity(gbString const str) { return GB_STRING_HEADER(str)->capacity; }

gb_inline isize gb_string_available_space(gbString const str) {
	gbStringHeader *h = GB_STRING_HEADER(str);
	if (h->capacity > h->length) {
		return h->capacity - h->length;
	}
	return 0;
}


gb_inline void gb_string_clear(gbString str) { gb__set_string_length(str, 0); str[0] = '\0'; }

gb_inline gbString gb_string_concat(gbString str, gbString const other) { return gb_string_concat_length(str, other, gb_string_length(other)); }

gbString gb_string_concat_length(gbString str, void const *other, isize other_len) {
	if (other_len > 0) {
		isize curr_len = gb_string_length(str);
  
		str = gb_string_make_space_for(str, other_len);
		if (str == NULL) {
			return NULL;
		}
  
		gb_memcopy(str + curr_len, other, other_len);
		str[curr_len + other_len] = '\0';
		gb__set_string_length(str, curr_len + other_len);
	}
	return str;
}

gb_inline gbString gb_string_concatc(gbString str, char const *other) {
	return gb_string_concat_length(str, other, strlen(other));
}

gbString gb_string_concat_rune(gbString str, Rune r) {
	if (r >= 0) {
		u8 buf[8] = {0};
		isize len = gb_utf8_encode_rune(buf, r);
		return gb_string_concat_length(str, buf, len);
	}
	return str;
}


gbString gb_string_set(gbString str, char const *cstr) {
	isize len = strlen(cstr);
	if (gb_string_capacity(str) < len) {
		str = gb_string_make_space_for(str, len - gb_string_length(str));
		if (str == NULL) {
			return NULL;
		}
	}
 
	gb_memcopy(str, cstr, len);
	str[len] = '\0';
	gb__set_string_length(str, len);
 
	return str;
}



gbString gb_string_make_space_for(gbString str, isize add_len) {
	isize available = gb_string_available_space(str);
 
	// NOTE(bill): Return if there is enough space left
	if (available >= add_len) {
		return str;
	} else {
		isize new_len, old_size, new_size;
		void *ptr, *new_ptr;
		gbAllocator a = GB_STRING_HEADER(str)->allocator;
		gbStringHeader *header;
  
		new_len = gb_string_length(str) + add_len;
		ptr = GB_STRING_HEADER(str);
		old_size = gb_size_of(gbStringHeader) + gb_string_length(str) + 1;
		new_size = gb_size_of(gbStringHeader) + new_len + 1;
  
		new_ptr = gb_resize(a, ptr, old_size, new_size);
		if (new_ptr == NULL) return NULL;
  
		header = cast(gbStringHeader *)new_ptr;
		header->allocator = a;
  
		str = cast(gbString)(header+1);
		gb__set_string_capacity(str, new_len);
  
		return str;
	}
}

gb_inline isize gb_string_allocation_size(gbString const str) {
	isize cap = gb_string_capacity(str);
	return gb_size_of(gbStringHeader) + cap;
}


gb_inline b32 gb_string_are_equal(gbString const lhs, gbString const rhs) {
	isize lhs_len, rhs_len, i;
	lhs_len = gb_string_length(lhs);
	rhs_len = gb_string_length(rhs);
	if (lhs_len != rhs_len) {
		return false;
	}
 
	for (i = 0; i < lhs_len; i++) {
		if (lhs[i] != rhs[i]) {
			return false;
		}
	}
 
	return true;
}


gbString gb_string_trim(gbString str, char const *cut_set) {
	char *start, *end, *start_pos, *end_pos;
	isize len;
 
	start_pos = start = str;
	end_pos   = end   = str + gb_string_length(str) - 1;
 
	while (start_pos <= end && gb_char_first_occurence(cut_set, *start_pos)) {
		start_pos++;
	}
	while (end_pos > start_pos && gb_char_first_occurence(cut_set, *end_pos)) {
		end_pos--;
	}
 
	len = cast(isize)((start_pos > end_pos) ? 0 : ((end_pos - start_pos)+1));
 
	if (str != start_pos)
		gb_memmove(str, start_pos, len);
	str[len] = '\0';
 
	gb__set_string_length(str, len);
 
	return str;
}

gb_inline gbString gb_string_trim_space(gbString str) { return gb_string_trim(str, " \t\r\n\v\f"); }




////////////////////////////////////////////////////////////////
//
// Windows UTF-8 Handling
//
//


u16 *gb_utf8_to_ucs2(u16 *buffer, isize len, u8 const *str) {
	Rune c;
	isize i = 0;
	len--;
	while (*str) {
		if (i >= len)
			return NULL;
		if (!(*str & 0x80)) {
			buffer[i++] = *str++;
		} else if ((*str & 0xe0) == 0xc0) {
			if (*str < 0xc2)
				return NULL;
			c = (*str++ & 0x1f) << 6;
			if ((*str & 0xc0) != 0x80)
				return NULL;
			buffer[i++] = cast(u16)(c + (*str++ & 0x3f));
		} else if ((*str & 0xf0) == 0xe0) {
			if (*str == 0xe0 &&
			    (str[1] < 0xa0 || str[1] > 0xbf))
				return NULL;
			if (*str == 0xed && str[1] > 0x9f) // str[1] < 0x80 is checked below
				return NULL;
			c = (*str++ & 0x0f) << 12;
			if ((*str & 0xc0) != 0x80)
				return NULL;
			c += (*str++ & 0x3f) << 6;
			if ((*str & 0xc0) != 0x80)
				return NULL;
			buffer[i++] = cast(u16)(c + (*str++ & 0x3f));
		} else if ((*str & 0xf8) == 0xf0) {
			if (*str > 0xf4)
				return NULL;
			if (*str == 0xf0 && (str[1] < 0x90 || str[1] > 0xbf))
				return NULL;
			if (*str == 0xf4 && str[1] > 0x8f) // str[1] < 0x80 is checked below
				return NULL;
			c = (*str++ & 0x07) << 18;
			if ((*str & 0xc0) != 0x80)
				return NULL;
			c += (*str++ & 0x3f) << 12;
			if ((*str & 0xc0) != 0x80)
				return NULL;
			c += (*str++ & 0x3f) << 6;
			if ((*str & 0xc0) != 0x80)
				return NULL;
			c += (*str++ & 0x3f);
			// UTF-8 encodings of values used in surrogate pairs are invalid
			if ((c & 0xfffff800) == 0xd800)
				return NULL;
			if (c >= 0x10000) {
				c -= 0x10000;
				if (i+2 > len)
					return NULL;
				buffer[i++] = 0xd800 | (0x3ff & (c>>10));
				buffer[i++] = 0xdc00 | (0x3ff & (c    ));
			}
		} else {
			return NULL;
		}
	}
	buffer[i] = 0;
	return buffer;
}

u8 *gb_ucs2_to_utf8(u8 *buffer, isize len, u16 const *str) {
	isize i = 0;
	len--;
	while (*str) {
		if (*str < 0x80) {
			if (i+1 > len)
				return NULL;
			buffer[i++] = (char) *str++;
		} else if (*str < 0x800) {
			if (i+2 > len)
				return NULL;
			buffer[i++] = cast(char)(0xc0 + (*str >> 6));
			buffer[i++] = cast(char)(0x80 + (*str & 0x3f));
			str += 1;
		} else if (*str >= 0xd800 && *str < 0xdc00) {
			Rune c;
			if (i+4 > len)
				return NULL;
			c = ((str[0] - 0xd800) << 10) + ((str[1]) - 0xdc00) + 0x10000;
			buffer[i++] = cast(char)(0xf0 +  (c >> 18));
			buffer[i++] = cast(char)(0x80 + ((c >> 12) & 0x3f));
			buffer[i++] = cast(char)(0x80 + ((c >>  6) & 0x3f));
			buffer[i++] = cast(char)(0x80 + ((c      ) & 0x3f));
			str += 2;
		} else if (*str >= 0xdc00 && *str < 0xe000) {
			return NULL;
		} else {
			if (i+3 > len)
				return NULL;
			buffer[i++] = 0xe0 +  (*str >> 12);
			buffer[i++] = 0x80 + ((*str >>  6) & 0x3f);
			buffer[i++] = 0x80 + ((*str      ) & 0x3f);
			str += 1;
		}
	}
	buffer[i] = 0;
	return buffer;
}

u16 *gb_utf8_to_ucs2_buf(u8 const *str) { // NOTE(bill): Uses locally persisting buffer
	gb_local_persist u16 buf[4096];
	return gb_utf8_to_ucs2(buf, gb_count_of(buf), str);
}

u8 *gb_ucs2_to_utf8_buf(u16 const *str) { // NOTE(bill): Uses locally persisting buffer
	gb_local_persist u8 buf[4096];
	return gb_ucs2_to_utf8(buf, gb_count_of(buf), str);
}



gb_global u8 const gb__utf8_first[256] = {
	0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, // 0x00-0x0F
	0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, // 0x10-0x1F
	0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, // 0x20-0x2F
	0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, // 0x30-0x3F
	0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, // 0x40-0x4F
	0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, // 0x50-0x5F
	0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, // 0x60-0x6F
	0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, // 0x70-0x7F
	0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, // 0x80-0x8F
	0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, // 0x90-0x9F
	0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, // 0xA0-0xAF
	0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, // 0xB0-0xBF
	0xf1, 0xf1, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, // 0xC0-0xCF
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, // 0xD0-0xDF
	0x13, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x23, 0x03, 0x03, // 0xE0-0xEF
	0x34, 0x04, 0x04, 0x04, 0x44, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, // 0xF0-0xFF
};


typedef struct gbUtf8AcceptRange {
	u8 lo, hi;
} gbUtf8AcceptRange;

gb_global gbUtf8AcceptRange const gb__utf8_accept_ranges[] = {
	{0x80, 0xbf},
	{0xa0, 0xbf},
	{0x80, 0x9f},
	{0x90, 0xbf},
	{0x80, 0x8f},
};


isize gb_utf8_decode(u8 const *str, isize str_len, Rune *codepoint_out) {
	isize width = 0;
	Rune codepoint = GB_RUNE_INVALID;
 
	if (str_len > 0) {
		u8 s0 = str[0];
		u8 x = gb__utf8_first[s0], sz;
		u8 b1, b2, b3;
		gbUtf8AcceptRange accept;
		if (x >= 0xf0) {
			Rune mask = (cast(Rune)x << 31) >> 31;
			codepoint = (cast(Rune)s0 & (~mask)) | (GB_RUNE_INVALID & mask);
			width = 1;
			goto end;
		}
		if (s0 < 0x80) {
			codepoint = s0;
			width = 1;
			goto end;
		}
  
		sz = x&7;
		accept = gb__utf8_accept_ranges[x>>4];
		if (str_len < gb_size_of(sz))
			goto invalid_codepoint;
  
		b1 = str[1];
		if (b1 < accept.lo || accept.hi < b1)
			goto invalid_codepoint;
  
		if (sz == 2) {
			codepoint = (cast(Rune)s0&0x1f)<<6 | (cast(Rune)b1&0x3f);
			width = 2;
			goto end;
		}
  
		b2 = str[2];
		if (!gb_is_between(b2, 0x80, 0xbf))
			goto invalid_codepoint;
  
		if (sz == 3) {
			codepoint = (cast(Rune)s0&0x1f)<<12 | (cast(Rune)b1&0x3f)<<6 | (cast(Rune)b2&0x3f);
			width = 3;
			goto end;
		}
  
		b3 = str[3];
		if (!gb_is_between(b3, 0x80, 0xbf))
			goto invalid_codepoint;
  
		codepoint = (cast(Rune)s0&0x07)<<18 | (cast(Rune)b1&0x3f)<<12 | (cast(Rune)b2&0x3f)<<6 | (cast(Rune)b3&0x3f);
		width = 4;
		goto end;
  
  invalid_codepoint:
		codepoint = GB_RUNE_INVALID;
		width = 1;
	}
 
 end:
	if (codepoint_out) *codepoint_out = codepoint;
	return width;
}

isize gb_utf8_codepoint_size(u8 const *str, isize str_len) {
	isize i = 0;
	for (; i < str_len && str[i]; i++) {
		if ((str[i] & 0xc0) != 0x80)
			break;
	}
	return i+1;
}

isize gb_utf8_encode_rune(u8 buf[4], Rune r) {
	u32 i = cast(u32)r;
	u8 mask = 0x3f;
	if (i <= (1<<7)-1) {
		buf[0] = cast(u8)r;
		return 1;
	}
	if (i <= (1<<11)-1) {
		buf[0] = 0xc0 | cast(u8)(r>>6);
		buf[1] = 0x80 | (cast(u8)(r)&mask);
		return 2;
	}
 
	// Invalid or Surrogate range
	if (i > GB_RUNE_MAX ||
	    gb_is_between(i, 0xd800, 0xdfff)) {
		r = GB_RUNE_INVALID;
  
		buf[0] = 0xe0 | cast(u8)(r>>12);
		buf[1] = 0x80 | (cast(u8)(r>>6)&mask);
		buf[2] = 0x80 | (cast(u8)(r)&mask);
		return 3;
	}
 
	if (i <= (1<<16)-1) {
		buf[0] = 0xe0 | cast(u8)(r>>12);
		buf[1] = 0x80 | (cast(u8)(r>>6)&mask);
		buf[2] = 0x80 | (cast(u8)(r)&mask);
		return 3;
	}
 
	buf[0] = 0xf0 | cast(u8)(r>>18);
	buf[1] = 0x80 | (cast(u8)(r>>12)&mask);
	buf[2] = 0x80 | (cast(u8)(r>>6)&mask);
	buf[3] = 0x80 | (cast(u8)(r)&mask);
	return 4;
}

////////////////////////////////////////////////////////////////
//
// File Handling
//
//

#if defined(GB_SYSTEM_WINDOWS)

gb_internal wchar_t *gb__alloc_utf8_to_ucs2(gbAllocator a, char const *text, isize *w_len_)
{
 wchar_t *w_text = NULL;
 isize len = 0, w_len = 0, w_len1 = 0;
 if (text == NULL) {
  if (w_len_) *w_len_ = w_len;
  return NULL;
 }
 len = strlen(text);
 if (len == 0) {
  if (w_len_) *w_len_ = w_len;
  return NULL;
 }
 w_len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, cast(int)len, NULL, 0);
 if (w_len == 0) {
  if (w_len_) *w_len_ = w_len;
  return NULL;
 }
 w_text = gb_alloc_array(a, wchar_t, w_len+1);
 w_len1 = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, cast(int)len, w_text, cast(int)w_len);
 if (w_len1 == 0) {
  gb_free(a, w_text);
  if (w_len_) *w_len_ = 0;
  return NULL;
 }
 w_text[w_len] = 0;
 if (w_len_) *w_len_ = w_len;
 return w_text;
}

	gb_internal GB_FILE_SEEK_PROC(gb__win32_file_seek) {
		LARGE_INTEGER li_offset;
		li_offset.QuadPart = offset;
		if (!SetFilePointerEx(fd.p, li_offset, &li_offset, whence)) {
			return false;
		}

		if (new_offset) *new_offset = li_offset.QuadPart;
		return true;
	}

	gb_internal GB_FILE_READ_AT_PROC(gb__win32_file_read) {
		b32 result = false;
		DWORD size_ = cast(DWORD)(size > I32_MAX ? I32_MAX : size);
		DWORD bytes_read_;
		gb__win32_file_seek(fd, offset, gbSeekWhence_Begin, NULL);
		if (ReadFile(fd.p, buffer, size_, &bytes_read_, NULL)) {
			if (bytes_read) *bytes_read = bytes_read_;
			result = true;
		}

		return result;
	}

	gb_internal GB_FILE_WRITE_AT_PROC(gb__win32_file_write) {
		DWORD size_ = cast(DWORD)(size > I32_MAX ? I32_MAX : size);
		DWORD bytes_written_;
		gb__win32_file_seek(fd, offset, gbSeekWhence_Begin, NULL);
		if (WriteFile(fd.p, buffer, size_, &bytes_written_, NULL)) {
			if (bytes_written) *bytes_written = bytes_written_;
			return true;
		}
		return false;
	}

	gb_internal GB_FILE_CLOSE_PROC(gb__win32_file_close) {
		CloseHandle(fd.p);
	}

	gb_global gbFileOperations const gbDefaultFileOperations = {
		gb__win32_file_read,
		gb__win32_file_write,
		gb__win32_file_seek,
		gb__win32_file_close
	};

	gb_no_inline GB_DEF GB_FILE_OPEN_PROC(gb__win32_file_open) {
		DWORD desired_access;
		DWORD creation_disposition;
		void *handle;
		wchar_t *w_text;

		switch (mode & gbFileMode_Modes) {
		case gbFileMode_Read:
			desired_access = GENERIC_READ;
			creation_disposition = OPEN_EXISTING;
			break;
		case gbFileMode_Write:
			desired_access = GENERIC_WRITE;
			creation_disposition = CREATE_ALWAYS;
			break;
		case gbFileMode_Append:
			desired_access = GENERIC_WRITE;
			creation_disposition = OPEN_ALWAYS;
			break;
		case gbFileMode_Read | gbFileMode_Rw:
			desired_access = GENERIC_READ | GENERIC_WRITE;
			creation_disposition = OPEN_EXISTING;
			break;
		case gbFileMode_Write | gbFileMode_Rw:
			desired_access = GENERIC_READ | GENERIC_WRITE;
			creation_disposition = CREATE_ALWAYS;
			break;
		case gbFileMode_Append | gbFileMode_Rw:
			desired_access = GENERIC_READ | GENERIC_WRITE;
			creation_disposition = OPEN_ALWAYS;
			break;
		default:
			GB_PANIC("Invalid file mode");
			return gbFileError_Invalid;
		}

		w_text = gb__alloc_utf8_to_ucs2(gb_heap_allocator(), filename, NULL);
		if (w_text == NULL) {
			return gbFileError_InvalidFilename;
		}
		handle = CreateFileW(w_text,
		                     desired_access,
		                     FILE_SHARE_READ|FILE_SHARE_DELETE, NULL,
		                     creation_disposition, FILE_ATTRIBUTE_NORMAL, NULL);

		gb_free(gb_heap_allocator(), w_text);

		if (handle == INVALID_HANDLE_VALUE) {
			DWORD err = GetLastError();
			switch (err) {
			case ERROR_FILE_NOT_FOUND: return gbFileError_NotExists;
			case ERROR_FILE_EXISTS:    return gbFileError_Exists;
			case ERROR_ALREADY_EXISTS: return gbFileError_Exists;
			case ERROR_ACCESS_DENIED:  return gbFileError_Permission;
			}
			return gbFileError_Invalid;
		}

		if (mode & gbFileMode_Append) {
			LARGE_INTEGER offset = {0};
			if (!SetFilePointerEx(handle, offset, NULL, gbSeekWhence_End)) {
				CloseHandle(handle);
				return gbFileError_Invalid;
			}
		}

		fd->p = handle;
		*ops = gbDefaultFileOperations;
		return gbFileError_None;
	}

#else // POSIX
	gb_internal GB_FILE_SEEK_PROC(gb__posix_file_seek) {
		#if defined(GB_SYSTEM_OSX)
		i64 res = lseek(fd.i, offset, whence);
		#else
		i64 res = lseek64(fd.i, offset, whence);
		#endif
		if (res < 0) return false;
		if (new_offset) *new_offset = res;
		return true;
	}

	gb_internal GB_FILE_READ_AT_PROC(gb__posix_file_read) {
		isize res = pread(fd.i, buffer, size, offset);
		if (res < 0) return false;
		if (bytes_read) *bytes_read = res;
		return true;
	}

	gb_internal GB_FILE_WRITE_AT_PROC(gb__posix_file_write) {
		isize res;
		i64 curr_offset = 0;
		gb__posix_file_seek(fd, 0, gbSeekWhence_Current, &curr_offset);
		if (curr_offset == offset) {
			// NOTE(bill): Writing to stdout et al. doesn't like pwrite for numerous reasons
			res = write(cast(int)fd.i, buffer, size);
		} else {
			res = pwrite(cast(int)fd.i, buffer, size, offset);
		}
		if (res < 0) return false;
		if (bytes_written) *bytes_written = res;
		return true;
	}


	gb_internal GB_FILE_CLOSE_PROC(gb__posix_file_close) {
		close(fd.i);
	}

	gb_global gbFileOperations const gbDefaultFileOperations = {
		gb__posix_file_read,
		gb__posix_file_write,
		gb__posix_file_seek,
		gb__posix_file_close
	};

	gb_no_inline GB_FILE_OPEN_PROC(gb__posix_file_open) {
		i1 os_mode;
		switch (mode & gbFileMode_Modes) {
		case gbFileMode_Read:
			os_mode = O_RDONLY;
			break;
		case gbFileMode_Write:
			os_mode = O_WRONLY | O_CREAT | O_TRUNC;
			break;
		case gbFileMode_Append:
			os_mode = O_WRONLY | O_APPEND | O_CREAT;
			break;
		case gbFileMode_Read | gbFileMode_Rw:
			os_mode = O_RDWR;
			break;
		case gbFileMode_Write | gbFileMode_Rw:
			os_mode = O_RDWR | O_CREAT | O_TRUNC;
			break;
		case gbFileMode_Append | gbFileMode_Rw:
			os_mode = O_RDWR | O_APPEND | O_CREAT;
			break;
		default:
			GB_PANIC("Invalid file mode");
			return gbFileError_Invalid;
		}

		fd->i = open(filename, os_mode, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
		if (fd->i < 0) {
			// TODO(bill): More file errors
			return gbFileError_Invalid;
		}

		*ops = gbDefaultFileOperations;
		return gbFileError_None;
	}

#endif



gbFileError gb_file_new(gbFile *f, gbFileDescriptor fd, gbFileOperations ops, char const *filename) {
	gbFileError err = gbFileError_None;
	isize len = strlen(filename);

	// gb_printf_err("gb_file_new: %s\n", filename);

	f->ops = ops;
	f->fd = fd;
	f->filename = gb_alloc_array(gb_heap_allocator(), char, len+1);
	gb_memcopy(cast(char *)f->filename, cast(char *)filename, len+1);
	f->last_write_time = gb_file_last_write_time(f->filename);

	return err;
}



gbFileError gb_file_open_mode(gbFile *f, gbFileMode mode, char const *filename) {
	gbFileError err;
#if defined(GB_SYSTEM_WINDOWS)
	err = gb__win32_file_open(&f->fd, &f->ops, mode, filename);
#else
	err = gb__posix_file_open(&f->fd, &f->ops, mode, filename);
#endif
	if (err == gbFileError_None) {
		return gb_file_new(f, f->fd, f->ops, filename);
	}
	return err;
}

gbFileError gb_file_close(gbFile *f) {
	if (f == NULL) {
		return gbFileError_Invalid;
	}

#if defined(GB_COMPILER_MSVC)
	if (f->filename != NULL) {
		gb_free(gb_heap_allocator(), cast(char *)f->filename);
	}
#else
	// TODO HACK(bill): Memory Leak!!!
#endif

#if defined(GB_SYSTEM_WINDOWS)
	if (f->fd.p == INVALID_HANDLE_VALUE) {
		return gbFileError_Invalid;
	}
#else
	if (f->fd.i < 0) {
		return gbFileError_Invalid;
	}
#endif

	if (!f->ops.read_at) f->ops = gbDefaultFileOperations;
	f->ops.close(f->fd);

	return gbFileError_None;
}

gb_inline b32 gb_file_read_at_check(gbFile *f, void *buffer, isize size, i64 offset, isize *bytes_read) {
	if (!f->ops.read_at) f->ops = gbDefaultFileOperations;
	return f->ops.read_at(f->fd, buffer, size, offset, bytes_read);
}

gb_inline b32 gb_file_write_at_check(gbFile *f, void const *buffer, isize size, i64 offset, isize *bytes_written) {
	if (!f->ops.read_at) f->ops = gbDefaultFileOperations;
	return f->ops.write_at(f->fd, buffer, size, offset, bytes_written);
}


gb_inline b32 gb_file_read_at(gbFile *f, void *buffer, isize size, i64 offset) {
	return gb_file_read_at_check(f, buffer, size, offset, NULL);
}

gb_inline b32 gb_file_write_at(gbFile *f, void const *buffer, isize size, i64 offset) {
	return gb_file_write_at_check(f, buffer, size, offset, NULL);
}

gb_inline i64 gb_file_seek(gbFile *f, i64 offset) {
	i64 new_offset = 0;
	if (!f->ops.read_at) f->ops = gbDefaultFileOperations;
	f->ops.seek(f->fd, offset, gbSeekWhence_Begin, &new_offset);
	return new_offset;
}

gb_inline i64 gb_file_seek_to_end(gbFile *f) {
	i64 new_offset = 0;
	if (!f->ops.read_at) f->ops = gbDefaultFileOperations;
	f->ops.seek(f->fd, 0, gbSeekWhence_End, &new_offset);
	return new_offset;
}

// NOTE(bill): Skips a certain amount of bytes
gb_inline i64 gb_file_skip(gbFile *f, i64 bytes) {
	i64 new_offset = 0;
	if (!f->ops.read_at) f->ops = gbDefaultFileOperations;
	f->ops.seek(f->fd, bytes, gbSeekWhence_Current, &new_offset);
	return new_offset;
}

gb_inline i64 gb_file_tell(gbFile *f) {
	i64 new_offset = 0;
	if (!f->ops.read_at) f->ops = gbDefaultFileOperations;
	f->ops.seek(f->fd, 0, gbSeekWhence_Current, &new_offset);
	return new_offset;
}
gb_inline b32 gb_file_read (gbFile *f, void *buffer, isize size)       { return gb_file_read_at(f, buffer, size, gb_file_tell(f)); }
gb_inline b32 gb_file_write(gbFile *f, void const *buffer, isize size) { return gb_file_write_at(f, buffer, size, gb_file_tell(f)); }


gbFileError gb_file_create(gbFile *f, char const *filename) {
	return gb_file_open_mode(f, gbFileMode_Write|gbFileMode_Rw, filename);
}


gbFileError gb_file_open(gbFile *f, char const *filename) {
	return gb_file_open_mode(f, gbFileMode_Read, filename);
}


char const *gb_file_name(gbFile *f) { return f->filename ? f->filename : ""; }

gb_inline b32 gb_file_has_changed(gbFile *f) {
	b32 result = false;
	gbFileTime last_write_time = gb_file_last_write_time(f->filename);
	if (f->last_write_time != last_write_time) {
		result = true;
		f->last_write_time = last_write_time;
	}
	return result;
}

// TODO(bill): Is this a bad idea?
gb_global b32    gb__std_file_set = false;
gb_global gbFile gb__std_files[gbFileStandard_Count] = {{0}};


#if defined(GB_SYSTEM_WINDOWS)

gb_inline gbFile *const gb_file_get_standard(gbFileStandardType std) {
	if (!gb__std_file_set) {
	#define GB__SET_STD_FILE(type, v) gb__std_files[type].fd.p = v; gb__std_files[type].ops = gbDefaultFileOperations
		GB__SET_STD_FILE(gbFileStandard_Input,  GetStdHandle(STD_INPUT_HANDLE));
		GB__SET_STD_FILE(gbFileStandard_Output, GetStdHandle(STD_OUTPUT_HANDLE));
		GB__SET_STD_FILE(gbFileStandard_Error,  GetStdHandle(STD_ERROR_HANDLE));
	#undef GB__SET_STD_FILE
		gb__std_file_set = true;
	}
	return &gb__std_files[std];
}

gb_inline i64 gb_file_size(gbFile *f) {
	LARGE_INTEGER size;
	GetFileSizeEx(f->fd.p, &size);
	return size.QuadPart;
}

gbFileError gb_file_truncate(gbFile *f, i64 size) {
	gbFileError err = gbFileError_None;
	i64 prev_offset = gb_file_tell(f);
	gb_file_seek(f, size);
	if (!SetEndOfFile(f)) {
		err = gbFileError_TruncationFailure;
	}
	gb_file_seek(f, prev_offset);
	return err;
}


b32 gb_file_exists(char const *name)
{
	WIN32_FIND_DATAW data;
	wchar_t *w_text;
	void *handle;
	b32 found = false;
	gbAllocator a = gb_heap_allocator();
 
	w_text = gb__alloc_utf8_to_ucs2(a, name, NULL);
	if (w_text == NULL) {
		return false;
	}
	handle = FindFirstFileW(w_text, &data);
	gb_free(a, w_text);
	found = handle != INVALID_HANDLE_VALUE;
	if (found) FindClose(handle);
	return found;
}

#else // POSIX

gb_inline gbFile *const gb_file_get_standard(gbFileStandardType std) {
	if (!gb__std_file_set) {
	#define GB__SET_STD_FILE(type, v) gb__std_files[type].fd.i = v; gb__std_files[type].ops = gbDefaultFileOperations
		GB__SET_STD_FILE(gbFileStandard_Input,  0);
		GB__SET_STD_FILE(gbFileStandard_Output, 1);
		GB__SET_STD_FILE(gbFileStandard_Error,  2);
	#undef GB__SET_STD_FILE
		gb__std_file_set = true;
	}
	return &gb__std_files[std];
}

gb_inline i64 gb_file_size(gbFile *f) {
	i64 size = 0;
	i64 prev_offset = gb_file_tell(f);
	gb_file_seek_to_end(f);
	size = gb_file_tell(f);
	gb_file_seek(f, prev_offset);
	return size;
}

gb_inline gbFileError gb_file_truncate(gbFile *f, i64 size) {
	gbFileError err = gbFileError_None;
	int i = ftruncate(f->fd.i, size);
	if (i != 0) err = gbFileError_TruncationFailure;
	return err;
}

gb_inline b32 gb_file_exists(char const *name) {
	return access(name, F_OK) != -1;
}
#endif



#if defined(GB_SYSTEM_WINDOWS)
gbFileTime gb_file_last_write_time(char const *filepath) {
	ULARGE_INTEGER li = {0};
	FILETIME last_write_time = {0};
	WIN32_FILE_ATTRIBUTE_DATA data = {0};
	gbAllocator a = gb_heap_allocator();

	wchar_t *w_text = gb__alloc_utf8_to_ucs2(a, filepath, NULL);
	if (w_text == NULL) {
		return 0;
	}

	if (GetFileAttributesExW(w_text, GetFileExInfoStandard, &data)) {
		last_write_time = data.ftLastWriteTime;
	}
	gb_free(a, w_text);

	li.LowPart = last_write_time.dwLowDateTime;
	li.HighPart = last_write_time.dwHighDateTime;
	return cast(gbFileTime)li.QuadPart;
}


inline b32 gb_file_copy(char const *existing_filename, char const *new_filename, b32 fail_if_exists) {
	wchar_t *w_old = NULL;
	wchar_t *w_new = NULL;
	gbAllocator a = gb_heap_allocator();
	b32 result = false;

	w_old = gb__alloc_utf8_to_ucs2(a, existing_filename, NULL);
	if (w_old == NULL) {
		return false;
	}
	w_new = gb__alloc_utf8_to_ucs2(a, new_filename, NULL);
	if (w_new != NULL) {
		result = CopyFileW(w_old, w_new, fail_if_exists);
	}
 if (!result)
 {
  DWORD last_error = GetLastError();
 }
	gb_free(a, w_new);
	gb_free(a, w_old);
	return result;
}

b32 gb_file_move(char const *existing_filename, char const *new_filename) {
	wchar_t *w_old = NULL;
	wchar_t *w_new = NULL;
	gbAllocator a = gb_heap_allocator();
	b32 result = false;

	w_old = gb__alloc_utf8_to_ucs2(a, existing_filename, NULL);
	if (w_old == NULL) {
		return false;
	}
	w_new = gb__alloc_utf8_to_ucs2(a, new_filename, NULL);
	if (w_new != NULL) {
		result = MoveFileW(w_old, w_new);
	}
    DWORD error = GetLastError();
	gb_free(a, w_new);
	gb_free(a, w_old);
	return result;
}

b32 gb_file_remove(char const *filename) {
	wchar_t *w_filename = NULL;
	gbAllocator a = gb_heap_allocator();
	b32 result = false;
	w_filename = gb__alloc_utf8_to_ucs2(a, filename, NULL);
	if (w_filename == NULL) {
		return false;
	}
	result = DeleteFileW(w_filename);
	gb_free(a, w_filename);
	return result;
}



#else

gbFileTime gb_file_last_write_time(char const *filepath) {
	time_t result = 0;
	struct stat file_stat;

	if (stat(filepath, &file_stat) == 0) {
		result = file_stat.st_mtime;
	}

	return cast(gbFileTime)result;
}


inline b32 gb_file_copy(char const *existing_filename, char const *new_filename, b32 fail_if_exists) {
#if defined(GB_SYSTEM_OSX)
	return copyfile(existing_filename, new_filename, NULL, COPYFILE_DATA) == 0;
#else
	isize size;
	int existing_fd = open(existing_filename, O_RDONLY, 0);
	int new_fd      = open(new_filename, O_WRONLY|O_CREAT, 0666);

	struct stat stat_existing;
	fstat(existing_fd, &stat_existing);

	size = sendfile(new_fd, existing_fd, 0, stat_existing.st_size);

	close(new_fd);
	close(existing_fd);

	return size == stat_existing.st_size;
#endif
}

b32 gb_file_move(char const *existing_filename, char const *new_filename) {
	if (link(existing_filename, new_filename) == 0) {
		return unlink(existing_filename) != -1;
	}
	return false;
}

b32 gb_file_remove(char const *filename) {
#if defined(GB_SYSTEM_OSX)
	return unlink(filename) != -1;
#else
	return remove(filename) == 0;
#endif
}


#endif





gbFileContents gb_file_read_contents(gbAllocator a, b32 zero_terminate, char const *filepath) {
	gbFileContents result = {0};
	gbFile file = {0};

	result.allocator = a;

	if (gb_file_open(&file, filepath) == gbFileError_None) {
		isize file_size = cast(isize)gb_file_size(&file);
		if (file_size > 0) {
			result.data = gb_alloc(a, zero_terminate ? file_size+1 : file_size);
			result.size = file_size;
			gb_file_read_at(&file, result.data, result.size, 0);
			if (zero_terminate) {
				u8 *str = cast(u8 *)result.data;
				str[file_size] = '\0';
			}
		}
		gb_file_close(&file);
	}

	return result;
}

void gb_file_free_contents(gbFileContents *fc) {
	GB_ASSERT_NOT_NULL(fc->data);
	gb_free(fc->allocator, fc->data);
	fc->data = NULL;
	fc->size = 0;
}





gb_inline b32 gb_path_is_absolute(char const *path) {
	b32 result = false;
	GB_ASSERT_NOT_NULL(path);
#if defined(GB_SYSTEM_WINDOWS)
	result == (strlen(path) > 2) &&
	          gb_char_is_alpha(path[0]) &&
	          (path[1] == ':' && path[2] == GB_PATH_SEPARATOR);
#else
	result = (strlen(path) > 0 && path[0] == GB_PATH_SEPARATOR);
#endif
	return result;
}

gb_inline b32 gb_path_is_relative(char const *path) { return !gb_path_is_absolute(path); }

gb_inline b32 gb_path_is_root(char const *path) {
	b32 result = false;
	GB_ASSERT_NOT_NULL(path);
#if defined(GB_SYSTEM_WINDOWS)
	result = gb_path_is_absolute(path) && (strlen(path) == 3);
#else
	result = gb_path_is_absolute(path) && (strlen(path) == 1);
#endif
	return result;
}

gb_inline char const *gb_path_base_name(char const *path) {
	char const *ls;
	GB_ASSERT_NOT_NULL(path);
	ls = gb_char_last_occurence(path, '/');
	return (ls == NULL) ? path : ls+1;
}

gb_inline char const *gb_path_extension(char const *path) {
	char const *ld;
	GB_ASSERT_NOT_NULL(path);
	ld = gb_char_last_occurence(path, '.');
	return (ld == NULL) ? NULL : ld+1;
}


#if !defined(_WINDOWS_) && defined(GB_SYSTEM_WINDOWS)
GB_DLL_IMPORT DWORD WINAPI GetFullPathNameA(char const *lpFileName, DWORD nBufferLength, char *lpBuffer, char **lpFilePart);
GB_DLL_IMPORT DWORD WINAPI GetFullPathNameW(wchar_t const *lpFileName, DWORD nBufferLength, wchar_t *lpBuffer, wchar_t **lpFilePart);
#endif

char *gb_path_get_full_name(gbAllocator a, char const *path) {
#if defined(GB_SYSTEM_WINDOWS)
// TODO(bill): Make UTF-8
	wchar_t *w_path = NULL;
	wchar_t *w_fullpath = NULL;
	isize w_len = 0;
	isize new_len = 0;
	isize new_len1 = 0;
	char *new_path = 0;
	w_path = gb__alloc_utf8_to_ucs2(gb_heap_allocator(), path, NULL);
	if (w_path == NULL) {
		return NULL;
	}
	w_len = GetFullPathNameW(w_path, 0, NULL, NULL);
	if (w_len == 0) {
		return NULL;
	}
	w_fullpath = gb_alloc_array(gb_heap_allocator(), wchar_t, w_len+1);
	GetFullPathNameW(w_path, cast(int)w_len, w_fullpath, NULL);
	w_fullpath[w_len] = 0;
	gb_free(gb_heap_allocator(), w_path);

	new_len = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, w_fullpath, cast(int)w_len, NULL, 0, NULL, NULL);
	if (new_len == 0) {
		gb_free(gb_heap_allocator(), w_fullpath);
		return NULL;
	}
	new_path = gb_alloc_array(a, char, new_len+1);
	new_len1 = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, w_fullpath, cast(int)w_len, new_path, cast(int)new_len, NULL, NULL);
	if (new_len1 == 0) {
		gb_free(gb_heap_allocator(), w_fullpath);
		gb_free(a, new_path);
		return NULL;
	}
	new_path[new_len] = 0;
	return new_path;
#else
	char *p, *result, *fullpath = NULL;
	isize len;
	p = realpath(path, NULL);
	fullpath = p;
	if (p == NULL) {
		// NOTE(bill): File does not exist
		fullpath = cast(char *)path;
	}

	len = strlen(fullpath);

	result = gb_alloc_array(a, char, len + 1);
	gb_memmove(result, fullpath, len);
	result[len] = 0;
	free(p);

	return result;
#endif
}







////////////////////////////////////////////////////////////////
//
// DLL Handling
//
//

#if defined(GB_SYSTEM_WINDOWS)

gbDllHandle gb_dll_load(char const *filepath) {
	return cast(gbDllHandle)LoadLibraryA(filepath);
}
gb_inline b32       gb_dll_unload      (gbDllHandle dll)                        { return FreeLibrary(cast(HMODULE)dll); }
gb_inline gbDllProc gb_dll_proc_address(gbDllHandle dll, char const *proc_name) { return cast(gbDllProc)GetProcAddress(cast(HMODULE)dll, proc_name); }

#else // POSIX

gbDllHandle gb_dll_load(char const *filepath) {
	// TODO(bill): Should this be RTLD_LOCAL?
	return cast(gbDllHandle)dlopen(filepath, RTLD_LAZY|RTLD_GLOBAL);
}

gb_inline b32       gb_dll_unload      (gbDllHandle dll)                        { return !dlclose(dll); }
gb_inline gbDllProc gb_dll_proc_address(gbDllHandle dll, char const *proc_name) { return cast(gbDllProc)dlsym(dll, proc_name); }

#endif





#if defined(GB_SYSTEM_WINDOWS)
gb_inline void gb_exit(u32 code) { ExitProcess(code); }
#else
gb_inline void gb_exit(u32 code) { exit(code); }
#endif

gb_inline void gb_yield(void) {
#if defined(GB_SYSTEM_WINDOWS)
	Sleep(0);
#else
	sched_yield();
#endif
}

gb_inline void gb_set_env(char const *name, char const *value) {
#if defined(GB_SYSTEM_WINDOWS)
	// TODO(bill): Should this be a Wide version?
	SetEnvironmentVariableA(name, value);
#else
	setenv(name, value, 1);
#endif
}

gb_inline void gb_unset_env(char const *name) {
#if defined(GB_SYSTEM_WINDOWS)
	// TODO(bill): Should this be a Wide version?
	SetEnvironmentVariableA(name, NULL);
#else
	unsetenv(name);
#endif
}


gb_inline u16 gb_endian_swap16(u16 i) {
	return (i>>8) | (i<<8);
}

gb_inline u32 gb_endian_swap32(u32 i) {
	return (i>>24) |(i<<24) |
	       ((i&0x00ff0000u)>>8)  | ((i&0x0000ff00u)<<8);
}

gb_inline u64 gb_endian_swap64(u64 i) {
	return (i>>56) | (i<<56) |
	       ((i&0x00ff000000000000ull)>>40) | ((i&0x000000000000ff00ull)<<40) |
	       ((i&0x0000ff0000000000ull)>>24) | ((i&0x0000000000ff0000ull)<<24) |
	       ((i&0x000000ff00000000ull)>>8)  | ((i&0x00000000ff000000ull)<<8);
}


gb_inline isize gb_count_set_bits(u64 mask) {
	isize count = 0;
	while (mask) {
		count += (mask & 1);
		mask >>= 1;
	}
	return count;
}
#endif



#if defined(GB_COMPILER_MSVC)
#pragma warning(pop)
#endif

#if defined(__GCC__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif


#if defined(__cplusplus)
EXTERN_C_END
#endif

#endif // GB_IMPLEMENTATION