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
*/


#ifndef GB_INCLUDE_GB_H
#define GB_INCLUDE_GB_H

EXTERN_C_BEGIN

#if defined(__cplusplus)
#	define GB_EXTERN extern "C"
#else
#	define GB_EXTERN extern
#endif

#if defined(_WIN32)
#	define dll_export GB_EXTERN __declspec(dllexport)
#	define DLL_IMPORT GB_EXTERN __declspec(dllimport)
#else
#	define dll_export GB_EXTERN __attribute__((visibility("default")))
#	define DLL_IMPORT GB_EXTERN
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
#        error This UNIX operating system is not supported
#endif
#else
#    error This operating system is not supported
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

GB_STATIC_ASSERT(sizeof(u8)  == sizeof(i8));
GB_STATIC_ASSERT(sizeof(u16) == sizeof(i16));
GB_STATIC_ASSERT(sizeof(u32) == sizeof(i1));
GB_STATIC_ASSERT(sizeof(u64) == sizeof(i64));

GB_STATIC_ASSERT(sizeof(u8)  == 1);
GB_STATIC_ASSERT(sizeof(u16) == 2);
GB_STATIC_ASSERT(sizeof(u32) == 4);
GB_STATIC_ASSERT(sizeof(u64) == 8);

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

GB_STATIC_ASSERT(sizeof(f32) == 4);
GB_STATIC_ASSERT(sizeof(f64) == 8);

typedef i1 Rune; // NOTE(bill): Unicode codepoint
#define GB_RUNE_INVALID cast(Rune)(0xfffd)
#define GB_RUNE_MAX     cast(Rune)(0x0010ffff)
#define GB_RUNE_BOM     cast(Rune)(0xfeff)
#define GB_RUNE_EOF     cast(Rune)(-1)


typedef i8  b8;
typedef i16 b16;

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
#define u32_max 0xffffffffu
#define i32_min (-0x7fffffff - 1)
#define i32_max 0x7fffffff

#define u64_max 0xffffffffffffffffull
#define i64_min (-0x7fffffffffffffffll - 1)
#define i64_max 0x7fffffffffffffffll

#if defined(GB_ARCH_32_BIT)
	#define USIZE_MIX 0u
	#define USIZE_MAX u32_max

	#define ISIZE_MIX S32_MIN
	#define ISIZE_MAX S32_MAX
#elif defined(GB_ARCH_64_BIT)
	#define USIZE_MIX 0ull
	#define USIZE_MAX u64_max

	#define ISIZE_MIX i64_min
	#define ISIZE_MAX i64_max
#else
	#error Unknown architecture size. This library only supports 32 bit and 64 bit architectures.
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
#	if defined(_MSC_VER) && _MSC_VER <= 1800
#	 define inline __inline
#	elif !defined(__STDC_VERSION__)
#	 define inline __inline__
#	else
#	 define inline
#	endif
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
#define offsetof(Type, member) ((isize)&(((Type *)0)->member))
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
#define function      static // Internal linkage
#define gb_local_persist static // Local Persisting variables
#define function static
#endif


#ifndef gb_unused
	#if COMPILER_MSVC
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
extern "C++"
{
	// NOTE(bill): Stupid fucking templates
	template <typename T> struct gbRemoveReference       { typedef T Type; };
	template <typename T> struct gbRemoveReference<T &>  { typedef T Type; };
	template <typename T> struct gbRemoveReference<T &&> { typedef T Type; };
 
	/// NOTE(bill): "Move" semantics - invented because the C++ committee are idiots (as a collective not as indiviuals (well a least some aren't))
	template <typename T> inline T &&gb_forward(typename gbRemoveReference<T>::Type &t)  { return static_cast<T &&>(t); }
	template <typename T> inline T &&gb_forward(typename gbRemoveReference<T>::Type &&t) { return static_cast<T &&>(t); }
	template <typename T> inline T &&gb_move   (T &&t)                                   { return static_cast<typename gbRemoveReference<T>::Type &&>(t); }
	template <typename F>
  struct gbprivDefer
 {
		F f;
		gbprivDefer(F &&f) : f(gb_forward<F>(f)) {}
		~gbprivDefer() { f(); }
	};
	template <typename F>
  gbprivDefer<F> gb__defer_func(F &&f)
 {
  return gbprivDefer<F>(gb_forward<F>(f));
 }

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

GB_DEF void        gb_memcopy   (void *dest, void const *source, isize size);
GB_DEF void        gb_memmove   (void *dest, void const *source, isize size);
GB_DEF void *      gb_memset    (void *data, u8 byte_value, isize size);
GB_DEF i1         gb_memcompare(void const *s1, void const *s2, isize size);
GB_DEF void        gb_memswap   (void *i, void *j, isize size);
GB_DEF void const *gb_memchr    (void const *data, u8 byte_value, isize size);
GB_DEF void const *gb_memrchr   (void const *data, u8 byte_value, isize size);


#ifndef gb_memcopy_array_dst
#define gb_memcopy_array_dst(dst, src, count) gb_memcopy((dst), (src), gb_size_of(*(dst))*(count))
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
GB_DEF void sleep_ms    (u32 ms);


// Atomics

// TODO(bill): Be specific with memory order?
// e.g. relaxed, acquire, release, acquire_release

#if defined(COMPILER_MSVC)
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

inline gbAllocator
gb_heap_allocator(void) {
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

GB_DEF isize gb_strlcpy(char *dest, char const *source, isize len);
GB_DEF char *gb_strrev (char *str); // NOTE(bill): ASCII only

GB_DEF void gb_str_concat(char *dest, isize dest_len,
                          char const *src_a, isize src_a_len,
                          char const *src_b, isize src_b_len);

GB_DEF u64   gb_str_to_u64(char const *str, char **end_ptr, i1 base); // TODO(bill): Support more than just decimal and hexadecimal
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

typedef u64 gbFileTime;

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

GB_DEF char *gb_bprintf    (char const *fmt, ...) GB_PRINTF_ARGS(1); // NOTE(bill): A locally persisting buffer is used internally
GB_DEF char *gb_bprintf_va (char const *fmt, va_list va);            // NOTE(bill): A locally persisting buffer is used internally
GB_DEF isize gb_snprintf   (char *str, isize n, char const *fmt, ...) GB_PRINTF_ARGS(3);

////////////////////////////////////////////////////////////////
//
// DLL Handling
//
//

typedef void *DLL_Handle;
typedef void (*gbDllProc)(void);

GB_DEF DLL_Handle gb_dll_load        (char const *filepath);
GB_DEF b32         gb_dll_unload      (DLL_Handle dll);
GB_DEF gbDllProc   gb_dll_proc_address(DLL_Handle dll, char const *proc_name);


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

//~
//~Implementation
// It's turtles all the way down!
//~
#if defined(GB_IMPLEMENTATION) && !defined(GB_IMPLEMENTATION_DONE)
#define GB_IMPLEMENTATION_DONE

EXTERN_C_BEGIN

#if !AD_IS_DRIVER
GB_ALLOCATOR_PROC(gb_heap_allocator_proc) {
	void *ptr = NULL;
	gb_unused(allocator_data);
	gb_unused(old_size);
 // TODO(bill): Throughly test!
	switch (type) {
#if (GB_SYSTEM_WINDOWS)
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

//-
b32 gb_is_power_of_two(isize x) {
	if (x <= 0)
		return false;
	return !(x & (x-1));
}

inline void *gb_align_forward(void *ptr, isize alignment) {
	uintptr p;
 
	GB_ASSERT(gb_is_power_of_two(alignment));
 
	p = cast(uintptr)ptr;
	return cast(void *)((p + (alignment-1)) &~ (alignment-1));
}



inline void *      gb_pointer_add      (void *ptr, isize bytes)             { return cast(void *)(cast(u8 *)ptr + bytes); }
inline void *      gb_pointer_sub      (void *ptr, isize bytes)             { return cast(void *)(cast(u8 *)ptr - bytes); }
inline void const *gb_pointer_add_const(void const *ptr, isize bytes)       { return cast(void const *)(cast(u8 const *)ptr + bytes); }
inline void const *gb_pointer_sub_const(void const *ptr, isize bytes)       { return cast(void const *)(cast(u8 const *)ptr - bytes); }
inline isize       gb_pointer_diff     (void const *begin, void const *end) { return cast(isize)(cast(u8 const *)end - cast(u8 const *)begin); }

myinline void gb_zero_size(void *ptr, isize size) { gb_memset(ptr, 0, size); }

#if AD_HAS_INTRINSIC
#if COMPILER_MSVC
#  pragma intrinsic(__movsb)
#endif
#endif

#if AD_HAS_INTRINSIC
function void
gb_memcopy(void *dest, void const *source, isize n)
{
	if (dest == NULL) { return; }
 
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
#else
#  error "Not supported"
#endif
}
#elif !AD_HAS_INTRINSIC
function void
gb_memcopy(void *dest, void const *source, isize n)
{
 if(dest == 0) { return; }
 memcpy(dest, source, n);
}
#endif

function void
gb_memmove(void *dest, void const *source, isize n) {
	u8 *d = cast(u8 *)dest;
	u8 const *s = cast(u8 const *)source;
 
	if (d == s) {
		return;
	}
	if (s+n <= d || d+n <= s) { // NOTE(bill): Non-overlapping
		return gb_memcopy(d, s, n);
	}

	if (d < s) {
		if (cast(uintptr)s % gb_size_of(isize) == cast(uintptr)d % gb_size_of(isize)) {
			while (cast(uintptr)d % gb_size_of(isize)) {
				if (!n--) return;
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
					return;
				d[n] = s[n];
			}
			while (n >= gb_size_of(isize)) {
				n -= gb_size_of(isize);
				*cast(isize *)(d+n) = *cast(isize *)(s+n);
			}
		}
		while (n) n--, d[n] = s[n];
	}
}
function void *
gb_memset(void *dest, u8 c, isize n)
{
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

function void
gb_memswap(void *i, void *j, isize size) {
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

/*function const void
*gb_memchr(void const *data, u8 c, isize n) {
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
}*/

inline void *gb_alloc_align (gbAllocator a, isize size, isize alignment)                                { return a.proc(a.data, gbAllocation_Alloc, size, alignment, NULL, 0, GB_DEFAULT_ALLOCATOR_FLAGS); }
inline void *gb_alloc       (gbAllocator a, isize size)                                                 { return gb_alloc_align(a, size, GB_DEFAULT_MEMORY_ALIGNMENT); }
inline void  gb_free        (gbAllocator a, void *ptr)                                                  { if (ptr != NULL) a.proc(a.data, gbAllocation_Free, 0, 0, ptr, 0, GB_DEFAULT_ALLOCATOR_FLAGS); }
inline void  gb_free_all    (gbAllocator a)                                                             { a.proc(a.data, gbAllocation_FreeAll, 0, 0, NULL, 0, GB_DEFAULT_ALLOCATOR_FLAGS); }
inline void *gb_resize_align(gbAllocator a, void *ptr, isize old_size, isize new_size, isize alignment) { return a.proc(a.data, gbAllocation_Resize, new_size, alignment, ptr, old_size, GB_DEFAULT_ALLOCATOR_FLAGS); }
//-

#if AD_HAS_OS_CODE
////////////////////////////////////////////////////////////////
//
// Time
//
//

#if defined(COMPILER_MSVC) && !defined(__clang__)
inline u64 gb_rdtsc(void) { return __rdtsc(); }
#elif defined(__x86_64__)
inline u64 gb_rdtsc(void) {
 u32 hi, lo;
 __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
 return (cast(u64)lo) | ((cast(u64)hi)<<32);
}
#endif

#if defined(GB_SYSTEM_WINDOWS)

function f64
gb_time_now(void)
{//note(kv) return seconds
 gb_local_persist LARGE_INTEGER win32_perf_count_freq = {};
 f64 result;
 LARGE_INTEGER counter;
 if (!win32_perf_count_freq.QuadPart) {
  QueryPerformanceFrequency(&win32_perf_count_freq);
  GB_ASSERT(win32_perf_count_freq.QuadPart != 0);
 }
 
 QueryPerformanceCounter(&counter);
 
 result = cast(f64)counter.QuadPart / cast(f64)(win32_perf_count_freq.QuadPart);
 return result;
}

inline u64 gb_utc_time_now(void) {
 FILETIME ft;
 ULARGE_INTEGER li;
 
 GetSystemTimeAsFileTime(&ft);
 li.LowPart = ft.dwLowDateTime;
 li.HighPart = ft.dwHighDateTime;
 
 return li.QuadPart/10;
}

inline void sleep_ms(u32 ms) { Sleep(ms); }

#else

gb_global f64 gb__timebase  = 0.0;
gb_global u64 gb__timestart = 0;

inline f64 gb_time_now(void) {
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

inline u64 gb_utc_time_now(void) {
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

inline void sleep_ms(u32 ms) {
 struct timespec req = {cast(time_t)ms/1000, cast(long)((ms%1000)*1000000)};
 struct timespec rem = {0, 0};
 nanosleep(&req, &rem);
}

#endif
#endif


////////////////////////////////////////////////////////////////
//
// Char things
//
//
#if !AD_IS_DRIVER
inline char gb_char_to_lower(char c) {
	if (c >= 'A' && c <= 'Z')
		return 'a' + (c - 'A');
	return c;
}

inline char gb_char_to_upper(char c) {
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

inline b32 gb_char_is_digit(char c) {
	if (c >= '0' && c <= '9')
		return true;
	return false;
}

inline b32 gb_char_is_hex_digit(char c) {
	if (gb_char_is_digit(c) ||
	    (c >= 'a' && c <= 'f') ||
	    (c >= 'A' && c <= 'F'))
	    return true;
	return false;
}

inline b32 gb_char_is_alpha(char c) {
	if ((c >= 'A' && c <= 'Z') ||
	    (c >= 'a' && c <= 'z'))
	    return true;
	return false;
}

inline b32 gb_char_is_alphanumeric(char c) {
	return gb_char_is_alpha(c) || gb_char_is_digit(c);
}

inline i1 gb_digit_to_int(char c) {
	return gb_char_is_digit(c) ? c - '0' : c - 'W';
}

inline i1 gb_hex_digit_to_int(char c) {
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

function isize
gb_utf8_strnlen(u8 const *str, isize max_len) {
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
function b32
string_match(char *a, char *b){
 while(*a){
  if(*a == *b){
   a++;
   b++;
  }else{
   break;
  }
 }
 b32 result = *a ==0 and *b == 0;
 return result;
}
// TODO(bill): Are these good enough for characters?
gb_global char const gb__num_to_char_table[] =
"0123456789"
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"@$";

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

function wchar_t *
gb__alloc_utf8_to_ucs2(gbAllocator a, char const *text, isize *w_len_){
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

function GB_FILE_SEEK_PROC(gb__win32_file_seek) {
 LARGE_INTEGER li_offset;
 li_offset.QuadPart = offset;
 if (!SetFilePointerEx(fd.p, li_offset, &li_offset, whence)) {
  return false;
 }
 
 if (new_offset) *new_offset = li_offset.QuadPart;
 return true;
}

function GB_FILE_READ_AT_PROC(gb__win32_file_read) {
 b32 result = false;
 DWORD size_ = cast(DWORD)(size > i32_max ? i32_max : size);
 DWORD bytes_read_;
 gb__win32_file_seek(fd, offset, gbSeekWhence_Begin, NULL);
 if (ReadFile(fd.p, buffer, size_, &bytes_read_, NULL)) {
  if (bytes_read) *bytes_read = bytes_read_;
  result = true;
 }
 
 return result;
}

	function GB_FILE_WRITE_AT_PROC(gb__win32_file_write) {
		DWORD size_ = cast(DWORD)(size > i32_max ? i32_max : size);
		DWORD bytes_written_;
		gb__win32_file_seek(fd, offset, gbSeekWhence_Begin, NULL);
		if (WriteFile(fd.p, buffer, size_, &bytes_written_, NULL)) {
			if (bytes_written) *bytes_written = bytes_written_;
			return true;
		}
		return false;
	}

	function GB_FILE_CLOSE_PROC(gb__win32_file_close) {
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
			LARGE_INTEGER offset = {};
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
	function GB_FILE_SEEK_PROC(gb__posix_file_seek) {
		#if defined(GB_SYSTEM_OSX)
		i64 res = lseek(fd.i, offset, whence);
		#else
		i64 res = lseek64(fd.i, offset, whence);
		#endif
		if (res < 0) return false;
		if (new_offset) *new_offset = res;
		return true;
	}

	function GB_FILE_READ_AT_PROC(gb__posix_file_read) {
		isize res = pread(fd.i, buffer, size, offset);
		if (res < 0) return false;
		if (bytes_read) *bytes_read = res;
		return true;
	}

	function GB_FILE_WRITE_AT_PROC(gb__posix_file_write) {
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


	function GB_FILE_CLOSE_PROC(gb__posix_file_close) {
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

#if defined(GB_SYSTEM_WINDOWS)
function b32
gb_file_exists(char const *name)
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

inline b32 gb_file_exists(char const *name) {
	return access(name, F_OK) != -1;
}
#endif

#if defined(GB_SYSTEM_WINDOWS)
function gbFileTime
gb_file_last_write_time(char const *filepath) {
	ULARGE_INTEGER li = {};
	FILETIME last_write_time = {};
	WIN32_FILE_ATTRIBUTE_DATA data = {};
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


function b32
gb_file_copy(char const *existing_filename, char const *new_filename, b32 fail_if_exists)
{
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
	gb_free(a, w_new);
	gb_free(a, w_old);
	return result;
}

function b32
gb_file_move(char const *existing_filename, char const *new_filename)
{
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
 (void)error;
	gb_free(a, w_new);
	gb_free(a, w_old);
	return result;
}

function b32
gb_file_remove(char const *filename){
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
//-non Windows
function gbFileTime
gb_file_last_write_time(char const *filepath) {
	time_t result = 0;
	struct stat file_stat;

	if (stat(filepath, &file_stat) == 0) {
		result = file_stat.st_mtime;
	}
 
	return cast(gbFileTime)result;
}


function b32
gb_file_copy(char const *existing_filename, char const *new_filename, b32 fail_if_exists)
{
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

function b32 gb_file_move(char const *existing_filename, char const *new_filename) {
	if (link(existing_filename, new_filename) == 0) {
		return unlink(existing_filename) != -1;
	}
	return false;
}

function b32 gb_file_remove(char const *filename) {
#if defined(GB_SYSTEM_OSX)
	return unlink(filename) != -1;
#else
	return remove(filename) == 0;
#endif
}

#endif

inline b32 gb_path_is_absolute(char const *path) {
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

inline b32 gb_path_is_relative(char const *path) { return !gb_path_is_absolute(path); }

inline b32 gb_path_is_root(char const *path) {
	b32 result = false;
	GB_ASSERT_NOT_NULL(path);
#if defined(GB_SYSTEM_WINDOWS)
	result = gb_path_is_absolute(path) && (strlen(path) == 3);
#else
	result = gb_path_is_absolute(path) && (strlen(path) == 1);
#endif
	return result;
}

//~DLL Handling

#if defined(GB_SYSTEM_WINDOWS)

myinline DLL_Handle
gb_dll_load(char const *filepath)
{
 DLL_Handle result = LoadLibraryA(filepath);
	return result;
}
myinline b32       gb_dll_unload      (DLL_Handle dll)                        { return FreeLibrary(cast(HMODULE)dll); }
myinline gbDllProc gb_dll_proc_address(DLL_Handle dll, char const *proc_name) { return cast(gbDllProc)GetProcAddress(cast(HMODULE)dll, proc_name); }

#else // POSIX

myinline DLL_Handle
gb_dll_load(char const *filepath) {
	// TODO(bill): Should this be RTLD_LOCAL?
	return cast(DLL_Handle)dlopen(filepath, RTLD_LAZY|RTLD_GLOBAL);
}

myinline b32       gb_dll_unload      (DLL_Handle dll)                        { return !dlclose(dll); }
myinline gbDllProc gb_dll_proc_address(DLL_Handle dll, char const *proc_name) { return cast(gbDllProc)dlsym(dll, proc_name); }

#endif

#if defined(GB_SYSTEM_WINDOWS)
inline void gb_exit(u32 code) { ExitProcess(code); }
#else
inline void gb_exit(u32 code) { exit(code); }
#endif

inline void gb_yield(void) {
#if defined(GB_SYSTEM_WINDOWS)
	Sleep(0);
#else
	sched_yield();
#endif
}

inline void gb_set_env(char const *name, char const *value) {
#if defined(GB_SYSTEM_WINDOWS)
	// TODO(bill): Should this be a Wide version?
	SetEnvironmentVariableA(name, value);
#else
	setenv(name, value, 1);
#endif
}

inline void gb_unset_env(char const *name) {
#if defined(GB_SYSTEM_WINDOWS)
	// TODO(bill): Should this be a Wide version?
	SetEnvironmentVariableA(name, NULL);
#else
	unsetenv(name);
#endif
}


inline u16 gb_endian_swap16(u16 i) {
	return (i>>8) | (i<<8);
}

inline u32 gb_endian_swap32(u32 i) {
	return (i>>24) |(i<<24) |
	       ((i&0x00ff0000u)>>8)  | ((i&0x0000ff00u)<<8);
}

inline u64 gb_endian_swap64(u64 i) {
	return (i>>56) | (i<<56) |
	       ((i&0x00ff000000000000ull)>>40) | ((i&0x000000000000ff00ull)<<40) |
	       ((i&0x0000ff0000000000ull)>>24) | ((i&0x0000000000ff0000ull)<<24) |
	       ((i&0x000000ff00000000ull)>>8)  | ((i&0x00000000ff000000ull)<<8);
}


inline isize gb_count_set_bits(u64 mask) {
	isize count = 0;
	while (mask) {
		count += (mask & 1);
		mask >>= 1;
	}
	return count;
}
#endif

#if COMPILER_MSVC
#pragma warning(pop)
#endif

#if defined(__GCC__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

EXTERN_C_END

#endif // GB_IMPLEMENTATION