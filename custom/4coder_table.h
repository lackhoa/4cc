/*
 * 4coder tables
 */

#pragma once

struct Table_Lookup{
    u64 hash;
    u32 index;
    b8 found_match;
    b8 found_empty_slot;
    b8 found_erased_slot;
};

struct Table_u64_u64{
    Base_Allocator *allocator;
    void *memory;
    u64 *keys;
    u64 *vals;
    u32 slot_count;
    u32 used_count;
    u32 dirty_count;
};

struct Table_u32_u16{
    Base_Allocator *allocator;
    void *memory;
    u32 *keys;
    u16 *vals;
    u32 slot_count;
    u32 used_count;
    u32 dirty_count;
};

struct Table_Data_u64{
    Base_Allocator *allocator;
    void *memory;
    u64 *hashes;
    String *keys;
    u64 *vals;
    u32 slot_count;
    u32 used_count;
    u32 dirty_count;
};

struct Table_u64_Data{
    Base_Allocator *allocator;
    void *memory;
    u64 *keys;
    String *vals;
    u32 slot_count;
    u32 used_count;
    u32 dirty_count;
};

struct Table_Data_Data{
    Base_Allocator *allocator;
    void *memory;
    u64 *hashes;
    String *keys;
    String *vals;
    u32 slot_count;
    u32 used_count;
    u32 dirty_count;
};
