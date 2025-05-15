//
#if KV_INTERNAL

global SpallProfile spall_ctx;
global thread_local SpallBuffer spall_buffer;

global thread_local i32 the_thread_id;

function void
DEBUG_profile_init_thread(Arena *arena, usize buffer_size)
{
 the_thread_id = system_thread_get_id();
 
 u8 *buffer = push_size(arena, buffer_size);
 
 spall_buffer.data = buffer;
 spall_buffer.length = buffer_size;
 
 // NOTE(kv) Touch the memory to make it fast!
 gb_memset(spall_buffer.data, 1, spall_buffer.length);
 
 spall_buffer_init(&spall_ctx, &spall_buffer);
}
function void
DEBUG_profile_flush()
{
 spall_buffer_flush(&spall_ctx, &spall_buffer);
}
function void
DEBUG_profile_quit_thread()
{
 spall_buffer_quit(&spall_ctx, &spall_buffer);
}

// NOTE(kv) Is it safe to cast rdtsc from u64 to f64?
// We have 52 bits, which comes out to to 11 days on a 4 GHz core.
#define ProfileBegin2(name_String) \
spall_buffer_begin_ex(&spall_ctx, &spall_buffer, \
(char *)name_String.str, (long)name_String.count, \
f64(gb_rdtsc()), the_thread_id, /*pid*/0) \

#define ProfileBegin(name_char) \
ProfileBegin2(strlit(name_char))

#define ProfileEnd() \
spall_buffer_end_ex(&spall_ctx, &spall_buffer, \
f64(gb_rdtsc()), the_thread_id, /*pid*/0) \

#define ProfileBlockNamed(block_name, var_name) \
Profile_Block var_name \
(strlit(block_name), strlit(filename_line_number))

#define ProfileBlock(block_name) \
ProfileBlockNamed(block_name, PP_Concat(profile_block_, __LINE__))

#define ProfileBlockEnd(var_name) \
profile_block_end(var_name)

#else

#define DEBUG_profile_init_thread(...)
#define DEBUG_profile_flush(...)
#define DEBUG_profile_quit_thread(...)
#define ProfileBegin(...)
#define ProfileBegin2(...)
#define ProfileEnd(...)
#define ProfileBlock(...)
#define ProfileBlockEnd(...)
#define ProfileBlockNamed(...)

#endif
//