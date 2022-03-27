#ifndef AR_OS_H
#define AR_OS_H

//-----------------------------------------------------------------------------
// Cache line alignment
//-----------------------------------------------------------------------------
#if defined(__i386__) || defined(__x86_64__)
#define CACHELINE_SIZE 64
#elif defined(__powerpc64__)
// TODO
  // This is the L1 D-cache line size of our Power7 machines.
  // Need to check if this is appropriate for other PowerPC64 systems.
  #define CACHELINE_SIZE 128
#elif defined(__arm__)
  // Cache line sizes for ARM: These values are not strictly correct since
  // cache line sizes depend on implementations, not architectures.
  // There are even implementations with cache line sizes configurable
  // at boot time.
  #if defined(__ARM_ARCH_5T__)
    #define CACHELINE_SIZE 32
  #elif defined(__ARM_ARCH_7A__)
    #define CACHELINE_SIZE 64
  #endif
#endif

#ifndef CACHELINE_SIZE
// A reasonable default guess.  Note that overestimates tend to waste more
// space, while underestimates tend to waste more time.
  #define CACHELINE_SIZE 64
#endif

#endif //AR_OS_H
