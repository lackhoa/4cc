/* Types and functions for communication between C++ and Objective-C layers. */

#if !defined(MAC_OBJECTIVE_C_TO_CPP_LINKS_H)
#define MAC_OBJECTIVE_C_TO_CPP_LINKS_H

// In C++ layer
extern "C" String
mac_SCu8(u8* str, u64 size);

extern "C" String
mac_push_string_copy(Arena *arena, String src);

extern "C" void
mac_init();

// In Objective-C layer
extern "C" String
mac_standardize_path(Arena* arena, String path);

extern "C" i1
mac_get_binary_path(void* buffer, u32 size);

#endif

