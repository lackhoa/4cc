#pragma once
//-TODO(kv) Shouldn't we just expose __rdtsc directly?
#if AD_IS_FRAMEWORK
function u64 ad_rdtsc(void){ return __rdtsc(); }
#else
global_decl u64 (*ad_rdtsc)(void);
#endif

#if KV_INTERNAL
struct Timed_Block
{
 u64 cycle_start;
 u32 *counter;
 
 inline Timed_Block(u32 *counter)
 {
  this->cycle_start = ad_rdtsc();
  this->counter = counter;
 }
 
 inline ~Timed_Block()
 {
  *this->counter += u32(ad_rdtsc() - cycle_start);
 }
};

#    define TIMED_BLOCK(counter) Timed_Block timed_block(&counter)
#    define BEGIN_TIMED          u64 cycle_start = ad_rdtsc();
#    define END_TIMED(counter)   counter += u32(ad_rdtsc() - cycle_start);
#else
#    define TIMED_BLOCK(...)
#    define BEGIN_TIMED
#    define END_TIMED(counter)
#endif

//~