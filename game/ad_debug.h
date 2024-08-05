#pragma once

#if AD_IS_DRIVER
u64 ad_rdtsc(void);

#else  // NOTE: Framework

extern inline u64 ad_rdtsc(void) { return __rdtsc(); }

#endif

#if AD_PROFILE
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