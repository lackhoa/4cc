#pragma once

#if KV_INTERNAL
struct Timed_Block
{
 u64 cycle_start;
 u32 *counter;
 
 Timed_Block(u32 *counter)
 {
  this->cycle_start = __rdtsc();
  this->counter = counter;
 }
 
 ~Timed_Block()
 {
  *this->counter += u32(__rdtsc() - cycle_start);
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