//-NOTE(kv) The meta layer gotta access OS stuff too!
function usize
system_page_size(){
 SYSTEM_INFO info;
 GetSystemInfo(&info);
 return info.dwPageSize;
}
function u8 *
system_memory_reserve(usize size){
 usize granularity = KB(64);  //TODO(kv) This number is from Raymond Chen, but idk how to query for it?
 usize reserve_size = size;
 reserve_size = align_pow2(granularity, reserve_size);
 
 u8 *memory = (u8 *)VirtualAlloc(0, reserve_size, MEM_RESERVE, PAGE_READWRITE);
 return memory;
}
function void
system_memory_free(void *base){
 //NOTE(kv) Free memory we've reserved (NOT often called).
 b32 ok = VirtualFree(base, 0, MEM_RELEASE);
 kv_assert(ok);
}
function b32
system_memory_commit(void *base, usize size){
 //NOTE(kv) This CAN'T fail, since we've reserved the memory
 void *memory = VirtualAlloc(base, size, MEM_COMMIT, PAGE_READWRITE);
 return(memory != 0);
}
function void
system_memory_decommit(void *base, usize size){
 b32 ok = VirtualFree(base, size, MEM_DECOMMIT);
 kv_assert(ok);
}
//-