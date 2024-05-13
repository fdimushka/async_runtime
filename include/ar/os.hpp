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


#include <string>

#if defined(__i386__) || defined(__x86_64__)
#define TF_CACHELINE_SIZE 64
#elif defined(__powerpc64__)
// TODO
  // This is the L1 D-cache line size of our Power7 machines.
  // Need to check if this is appropriate for other PowerPC64 systems.
#define TF_CACHELINE_SIZE 128
#elif defined(__arm__)
  // Cache line sizes for ARM: These values are not strictly correct since
  // cache line sizes depend on implementations, not architectures.
  // There are even implementations with cache line sizes configurable
  // at boot time.
#if defined(__ARM_ARCH_5T__)
#define TF_CACHELINE_SIZE 32
#elif defined(__ARM_ARCH_7A__)
#define TF_CACHELINE_SIZE 64
#endif
#endif

#ifndef TF_CACHELINE_SIZE
// A reasonable default guess.  Note that overestimates tend to waste more
// space, while underestimates tend to waste more time.
#define TF_CACHELINE_SIZE 64
#endif

namespace AsyncRuntime {

    /**
     * @brief
     * @return
     */
    std::string GetCPUInfo();


// Struct: CachelineAligned
// Due to prefetch, we typically do 2x cacheline for the alignment.
    template<typename T>
    struct CachelineAligned {
        alignas(2 * TF_CACHELINE_SIZE) T data;
    };

// Function: get_env
    inline std::string get_env(const std::string &str) {
#ifdef _MSC_VER
        char *ptr = nullptr;
  size_t len = 0;

  if(_dupenv_s(&ptr, &len, str.c_str()) == 0 && ptr != nullptr) {
    std::string res(ptr, len);
    std::free(ptr);
    return res;
  }
  return "";

#else
        auto ptr = std::getenv(str.c_str());
        return ptr ? ptr : "";
#endif
    }

// Function: has_env
    inline bool has_env(const std::string &str) {
#ifdef _MSC_VER
        char *ptr = nullptr;
  size_t len = 0;

  if(_dupenv_s(&ptr, &len, str.c_str()) == 0 && ptr != nullptr) {
    std::string res(ptr, len);
    std::free(ptr);
    return true;
  }
  return false;

#else
        auto ptr = std::getenv(str.c_str());
        return ptr ? true : false;
#endif
    }

// Procedure: relax_cpu
//inline void relax_cpu() {
//#ifdef TF_HAS_MM_PAUSE
//  _mm_pause();
//#endif
//}

}

#endif //AR_OS_H
